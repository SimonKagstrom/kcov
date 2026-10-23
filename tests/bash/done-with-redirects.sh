#!/usr/bin/env bash

shopt -s expand_aliases
i=0

while ((i < 1)); do
	((i++))
done < /dev/null

while ((i < 2)); do
	((i++))
done > '/dev/null'

while ((i < 3)); do
	((i++))
done >> "/dev/null"

while ((i < 4)); do
	((i++))
done 3> '/dev/null'

while ((i < 5)); do
	((i++))
done 2>&1

while ((i < 6)); do
	((i++))
done 2< /dev/null

while ((i < 7)); do
	((i++))
done 1846< /dev/null

while ((i < 8)); do
	((i++))
done 4>> "/dev/null"

while ((i < 9)); do
	((i++))
done &> '/dev/null'

while ((i < 10)); do
	((i++))
done &>> /dev/null

while ((i < 11)); do
	((i++))
done< '/dev/null'

path=/dev/null
while ((i < 12)); do
	((i++))
done < "$path"

while ((i < 13)); do
	((i++))
done; echo hello world

function doneWithSomeMoreCharacters () { :; }
while ((i < 14)); do
	((i++))
doneWithSomeMoreCharacters < /dev/null
done

while ((i < 15)); do
	((i++))
done < $(echo '/dev/null')

while ((i < 16)); do
	((i++))
done < "/dev/$(echo null)"

while ((i < 17)); do
	((i++))
done < <(echo 'Hello, World!')

while ((i < 18)); do
	((i++))
done < `echo '/dev/null'`

while ((i < 19)); do
	((i++))
done < '/dev/null' > /dev/`echo null`

while ((i < 20)); do
	((i++))
done < /dev/null > >(cat > /dev/null)
