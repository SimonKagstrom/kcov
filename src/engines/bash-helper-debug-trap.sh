 #!/bin/bash

trap 'echo "kcov@${BASH_SOURCE}@${LINENO}@" >&$KCOV_BASH_XTRACEFD' DEBUG
unset BASH_ENV