#!/bin/bash

kcov=$1
out=$2
prg=$3

$prg &
pid=$!

$kcov --pid=$pid $out $prg &
kcov_pid=$!

sleep 1.2
kill $kcov_pid
