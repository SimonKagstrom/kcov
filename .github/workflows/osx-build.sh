#!/usr/bin/env bash

set -euo pipefail

run () {
  export PATH="${PATH}:${HOME}/kcov/bin"
  mkdir build build-tests

  cd build
  cmake -DOPENSSL_ROOT_DIR=$(brew --prefix openssl) -DCMAKE_INSTALL_PREFIX=/usr/local .. || exit 64
  make || exit 64
  make install || exit 64
  cd ..

  cd build-tests
  cmake ../tests || exit 64
  make || exit 64
  cd ..

  chmod u+x .github/workflows/test-executable.sh
  kcov --include-pattern=test-executable.sh coverage .github/workflows/test-executable.sh
  cat coverage/test-executable.sh/coverage.json
}

run "$@"
