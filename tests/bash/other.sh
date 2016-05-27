#!/bin/sh

echo "This is another shell script"

if [ $1 -eq 5 ] ; then
	echo "Kalle anka"
fi

for var in "$@" ; do
	echo $var
done

round=0
for (( ; ; )) do
	round=$(($round + 1))
	if [ $round -gt 10 ] ; then
		break
	fi
	echo "hej $round"
done
