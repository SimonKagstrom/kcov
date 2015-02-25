#!/bin/bash
function some_test
    {
    test "${1}" == "--quiet" &&
        { quiet=${1} ; shift ; }
    }
