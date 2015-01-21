#!/bin/sh

if [ $# -ne 1 ] ; then
	echo "Too few arguments"
	exit 1
fi

KCOV=$1

$KCOV /tmp/kcov-perf build/src/kcov
$KCOV /tmp/kcov-perf tests/python/main 5
$KCOV /tmp/kcov-perf tests/bash/shell-main 5
$KCOV /tmp/kcov-perf build/src/kcov /tmp/kcov-perf tests/python/main 5
$KCOV /tmp/kcov-perf build/src/kcov /tmp/kcov-perf tests/bash/shell-main 5

exit 0
