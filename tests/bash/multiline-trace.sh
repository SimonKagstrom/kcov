#!/usr/bin/env bash

multi_line="first
second"

if [[ $multi_line == *nomatch* ]]; then
	echo "unreachable"
fi
