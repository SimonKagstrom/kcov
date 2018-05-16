#!/usr/bin/env bash
shopt -s expand_aliases
alias test_alias='{
echo called test_alias
} '
test_alias
