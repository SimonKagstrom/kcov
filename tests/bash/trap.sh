#!/bin/sh

on_trap()
{
	echo "On trap"
	exit 1
}

trap on_trap SIGTERM


while true
	do read x
done
