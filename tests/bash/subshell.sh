#/bin/bash
eval "(echo Stuff in subshell
echo More stuff in subshell
      ) "
echo "Inbetween stuff"
(echo "Other subshell"
echo More stuff in Other subshell
)
