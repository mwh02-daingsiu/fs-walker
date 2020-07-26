#!/bin/bash

INPUT_BBLIST=${INPUT_BBLIST:-$1}
BLOCK_SIZE=${BLOCK_SIZE:-4096}

if [[ -z $INPUT_BBLIST ]]; then
	echo "No input badblock list specified!" > /dev/stderr
	exit 1
fi

for bblock in `cat "$INPUT_BBLIST"`; do
	for i in `seq 0 $((BLOCK_SIZE / 512 - 1))`; do
		dmsetup message $DM_NAME 0 removebadblock $((bblock * 8 + i))
	done
done
