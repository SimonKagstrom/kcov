#!/bin/sh

do_check() {
    if [ $1 -ne $2 ]; then
	echo "FAIL"
    fi
}

kcov /tmp/kcov ./tests > /dev/null
do_check $? 0

# should give an error
kcov /tmp/kcov ./tests-stripped 2> /dev/null
do_check $? 1

# Old regression
kcov --exclude-path=../tests/subdir /tmp/kcov ./tests > /dev/null
do_check $? 0
