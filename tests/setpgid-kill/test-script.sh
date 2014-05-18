#!/bin/bash

prg=$*

$prg &
sleep 1
kill -TERM %1

sleep 2

pid=`cat /tmp/setpgid.pid`
if [ -e /proc/$pid/cmdline ] ; then
	echo "FAIL"
else
	echo "SUCCESS"
fi
