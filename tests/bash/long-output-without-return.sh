#! /usr/bin/env bash

for ((i=0; i != 32768; ++i)); do
  printf "%04x" $RANDOM
done
