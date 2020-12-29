#!/usr/bin/env bash

set -euo pipefail

run () {
  export PATH="${PATH}:${HOME}/kcov/bin"

  chmod u+x .github/workflows/test-executable.sh
  kcov --include-path=.github/workflows/test-executable.sh coverage .github/workflows/test-executable.sh

  local coverage="$(<coverage/test*.sh*/coverage.json)"
  local percent="${coverage##*percent_covered\": \"}"
  local total_lines="${coverage##*total_lines\": }"
  local covered_lines="${coverage##*covered_lines\": }"

  echo -e "Coverage: ${covered_lines%%,*}/${total_lines%%,*} ${percent%%\"*}%"
}

run "$@"
