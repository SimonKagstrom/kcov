#!/bin/bash

kcov=$1
out=$2
prg=$3

$prg &
pid=`pidof $prg`

$kcov --pid=$pid $out $prg &

sleep 1.2
kill `pidof $kcov`
