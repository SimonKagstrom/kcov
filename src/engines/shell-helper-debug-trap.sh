#!/bin/sh

if [ "${BASH_VERSION:-}" ]; then
    set -o functrace
    trap 'echo "kcov@${BASH_SOURCE}@${LINENO}@" >&$KCOV_BASH_XTRACEFD' DEBUG
    unset BASH_ENV

elif [ "${KSH_VERSION:-}" ]; then
    trap '(echo "kcov@${.sh.file}@${LINENO}@" >/dev/fd/$KCOV_BASH_XTRACEFD); IFS=$IFS' DEBUG

elif [ "${ZH_VERSION:-}" ]; then
    echo "DA TRAP ZSH"
    trap '
    if [ ${#funcsourcetrace[@]} -gt 0 ]; then
        echo kcov@${funcsourcetrace[1]%:*}@$((${funcsourcetrace[1]##*:}+LINENO))@
    else
        echo kcov@${0}@${LINENO}@
    fi >&$KCOV_BASH_XTRACEFD
    ' DEBUG
fi
