#!/bin/bash
test -n "$FILE" && redirect="> ${FILE}"
eval echo stuff  2>&1 ${redirect:-}
