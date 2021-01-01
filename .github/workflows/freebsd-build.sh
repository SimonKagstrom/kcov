#!/usr/bin/env bash

set -euo pipefail

run () {
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
}

run "$@"
