#/bin/sh
eval "(echo Stuff in subshell
echo More stuff in subshell
      ) "
echo "Inbetween stuff"
(echo "Other subshell"
echo More stuff in Other subshell
)

# Issue 457

fn4() (
    echo "fn4"
)

fn4
