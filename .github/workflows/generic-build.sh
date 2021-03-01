#!/usr/bin/env bash

set -euo pipefail

run () {
  DEB_ARCH=$1
  EXTRA_PACKAGES=""
  if [ "$DEB_ARCH" = "ppc64le" ]; then
    DEB_ARCH="ppc64"
  elif [ "$DEB_ARCH" = "armv7" ]; then
    DEB_ARCH="armhf"
  elif [ "$DEB_ARCH" = "aarch64" ]; then
    DEB_ARCH="arm64"
  else
    EXTRA_PACKAGES="g++-multilib"
  fi

  apt-get update
  apt-mark hold php* google* libobjc* libpq* libidn* postgresql* python3-httplib2 samba* >/dev/null
  apt-get upgrade -y
  apt-get install -y binutils-dev libcurl4-openssl-dev libdw-dev libiberty-dev gcc g++ make cmake libssl-dev git python python3 $EXTRA_PACKAGES
  export PATH="${PATH}:${HOME}/kcov/bin"
  mkdir build build-tests
  cd build
  cmake -DCMAKE_INSTALL_PREFIX=/usr/local .. || exit 64
  make || exit 64
  make install || exit 64
  cd ..
  cd build-tests
  cmake -DCMAKE_INSTALL_PREFIX=/usr/local ../tests || exit 64
  make || exit 64
  cd ..
  tar czf kcov-"$1".tar.gz /usr/local/bin/kcov* /usr/local/share/doc/kcov/* /usr/local/share/man/man1/kcov.1
  readelf -h /usr/local/bin/kcov
  if [[ -e "kcov-$1.tar.gz" ]]; then
    echo "Built for $1".
  fi

  chmod u+x .github/workflows/test-executable.sh
  kcov --include-path=.github/workflows/test-executable.sh coverage .github/workflows/test-executable.sh

  local coverage="$(<coverage/test*.sh*/coverage.json)"
  local percent="${coverage##*percent_covered\": \"}"
  local total_lines="${coverage##*total_lines\": }"
  local covered_lines="${coverage##*covered_lines\": }"
  
  echo -e "Coverage: ${covered_lines%%,*}/${total_lines%%,*} ${percent%%\"*}%"
}

run "$@"
