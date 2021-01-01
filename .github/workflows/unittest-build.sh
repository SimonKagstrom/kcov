#!/usr/bin/env bash

set -euo pipefail

run () {
  export PATH="${PATH}:${HOME}/kcov/bin"
  git submodule update --recursive --remote

  mkdir build-ut
  cd build-ut
  cmake ../tests/unit-tests || exit 64
  make || exit 64
  ./ut || exit 64
  cd ..
}

run "$@"
