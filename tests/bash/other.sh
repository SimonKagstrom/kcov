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

echo "Not covered"  # LCOV_EXCL_LINE
echo "Covered"

# Ranges
echo "Not covered"  # LCOV_EXCL_START
echo "Also not covered"
echo "Also not covered?" # LCOV_EXCL_STOP
echo "Covered"

# Stacked ranges
echo "Not covered"  # LCOV_EXCL_START
echo "Not covered"  # LCOV_EXCL_START
echo "Also not covered"
echo "Also not covered?" # LCOV_EXCL_STOP
echo "Also not covered?" # LCOV_EXCL_STOP
echo "Covered"

# Unbalanced
echo "Not covered"  # LCOV_EXCL_START
echo "Also not covered"
echo "Also not covered?" # LCOV_EXCL_STOP
echo "Also not covered?" # LCOV_EXCL_STOP second one
echo "Covered"

# Custom range CUSTOM_RANGE_START
echo "Not covered"
echo "Also not covered"
echo "Also not covered?"
# CUSTOM_RANGE_END
echo "Covered"
