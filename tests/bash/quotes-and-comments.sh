#!/bin/bash

# Quotes in a comment do not open a string
echo "one" # it's
echo "two"
echo 'three' # say "hi
echo "four"
echo "five" # << EOF
echo "six"
[ $# -eq 0 ] # it's
echo "seven"
a="b#c" # don't
echo "eight"

# A trailing comment is not part of the code
echo "nine" # continues \
echo "ten"

# A # inside ${...} does not start a comment
a=${a%% #*}; b="x
y"
echo "eleven"
