#!/bin/bash

if [ $# -ne 3 ] ; then
	echo "Usage: <kcov-binary> <out-dir> <program-to-run>"
	exit 1
fi

kcov=$1
out=$2
prg=$3

$prg &
pid=$!

sleep 0.5
$kcov --pid=$pid $out &
kcov_pid=$!

sleep 1.2
kill $kcov_pid
