#!/bin/sh
#!/bin/sh
#function-with-spaces.sh

fun1() {
    echo "hello"
}

fun2 ()
{
    echo "world"
}

#
# Main
#

fun1
fun2
