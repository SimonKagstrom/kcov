#!/bin/sh

if [ $# -ne 3 ] ; then
	echo "Usage: perf-test.sh <kcov> <src-dir> <build-dir>"
	exit 1
fi

KCOV=$1
SRC=$2
BUILD=$3

$KCOV /tmp/kcov-perf $BUILD/src/kcov
$KCOV /tmp/kcov-perf $SRC/tests/python/main 5
$KCOV /tmp/kcov-perf $SRC/tests/bash/shell-main 5
$KCOV /tmp/kcov-perf $BUILD/src/kcov /tmp/kcov-perf $SRC/tests/python/main 5
$KCOV /tmp/kcov-perf $BUILD/src/kcov /tmp/kcov-perf $SRC/tests/bash/shell-main 5

exit 0
