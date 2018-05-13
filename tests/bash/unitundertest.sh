#!/bin/sh
#unitundertest.sh

block1()
{
    echo "In block1()"

    for f in 1 2 3 4 5
    do
        echo $f
    done
}

block2()
{
    echo "In block2()"

    for f in 1 2 3 4 5
    do
        echo $f
    done
}

#
# Main
#

case $1 in
1)
    block1
    ;;
2)
    block2
    ;;
all)
    block1
    block2
    ;;
esac
exit 0
