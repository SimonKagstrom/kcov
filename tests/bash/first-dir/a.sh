#!/bin/sh

echo A first

if [ $1 = "hej" ] ; then
	source `dirname $0`/c.sh
fi

source `dirname $0`/b.sh

