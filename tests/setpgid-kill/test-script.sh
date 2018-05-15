#!/usr/bin/env bash

prg=$*

$prg &
sleep 1
kill -TERM %1

sleep 2

pid=`cat /tmp/setpgid.pid`
if ps -p $pid > /dev/null ; then
	echo "FAIL"
else
	echo "SUCCESS"
fi
