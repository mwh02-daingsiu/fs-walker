#!/bin/bash

INPUT_BBLIST=$1

if [[ -z $INPUT_BBLIST ]]; then
	echo "No input badblock list specified!" > /dev/stderr
	exit 1
fi

for bblock in `cat "$INPUT_BBLIST"`; do
	dmsetup message $DM_NAME 0 addbadblock $((bblock / 8 * 8))
done
