# GitHub actions

## Example usage

### `bash-script.sh`

```sh
#!/usr/bin/env bash

if [[ true ]]; then
  echo "Hello, kcov!"
fi
```

### `tests.sh`

```sh
#!/bin/sh

testEquality() {
  assertEquals 1 1
}

. shunit2
```

### `.github/workflows/tests.yml`

```yml
name: Run tests

permissions:
  contents: read

on: [push]

jobs:
  tests:
    name: Run tests
    runs-on: ubuntu-latest
    steps:
        - uses: actions/checkout@v4

        - name: Example with a bash script (running on Docker kcov/kcov)
          uses: sudo-bot/action-kcov@latest
          with:
            cli-args: "--version"

        - name: Run a bash script with kcov coverage
          # Debian based, uses the Docker image kcov/kcov
          uses: sudo-bot/action-kcov@latest
          with:
            cli-args: "--dump-summary ./coverage ./bash-script.sh"

        - name: Run shunit2 tests with kcov coverage
          # Alpine based
          uses: sudo-bot/action-shunit2@latest
          with:
              cli: "kcov --dump-summary ./coverage ./tests.sh"

        - name: Upload coverage reports to Codecov
          uses: codecov/codecov-action@v4
          with:
            # Must be set in the repo or org settings as a secret
            token: ${{ secrets.CODECOV_TOKEN }}
            directory: ./coverage/
            fail_ci_if_error: true
```
