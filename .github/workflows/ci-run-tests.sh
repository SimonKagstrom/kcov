#!/usr/bin/env bash

set -euo pipefail

run () {
  export PYTHONPATH=tests/tools
  python3 -m libkcov build/src/kcov /tmp/ build-tests/ "." -v "$@" || exit 64
}

run "$@"
