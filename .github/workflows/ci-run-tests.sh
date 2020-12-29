#!/usr/bin/env bash

set -euo pipefail

run () {
  tests/tools/run-tests build/src/kcov /tmp/ build-tests/ `pwd` -v || exit 64
}

run "$@"
