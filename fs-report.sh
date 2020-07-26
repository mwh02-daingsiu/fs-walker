#!/bin/bash

NR_BBLOCKS_TEST=(0 1 4 16 64 256 1024 4096 16384 65536)
FS_TO_TEST=(testext2fyp testext2orig)
PREFIX=dust
REPORT_DIR=${REPORT_DIR:-report}
EXEC_SUDO=sudo

mkdir -p "$REPORT_DIR"

DEV_SIZE=`$EXEC_SUDO blockdev --getsz "/dev/disk/by-label/${FS_TO_TEST[0]}"`

for NR_BBLOCKS in "${NR_BBLOCKS_TEST[@]}"; do
	TEST_REPORT_DIR="$REPORT_DIR/$NR_BBLOCKS"

	mkdir -p "$TEST_REPORT_DIR"

	export INPUT_BBLIST="$TEST_REPORT_DIR/bblist"
	./gendust "$DEV_SIZE" "$NR_BBLOCKS" 1 > "$INPUT_BBLIST"

	for FS in "${FS_TO_TEST[@]}"; do
		export DM_NAME="$PREFIX$FS"

		DEV="/dev/mapper/$DM_NAME"
		MNT="/media/$DM_NAME"

		$EXEC_SUDO sync
		$EXEC_SUDO tee /proc/sys/vm/drop_caches <<< 3

		$EXEC_SUDO dmsetup create "$DM_NAME" --table "0 $DEV_SIZE dust /dev/disk/by-label/$FS 0 512"
		$EXEC_SUDO -E ./ADD_BBLOCKS.sh
		$EXEC_SUDO dmsetup message "$DM_NAME" 0 enable
		$EXEC_SUDO blockdev --setro "$DEV"

		$EXEC_SUDO mkdir -p "$MNT"
		if [[ "$FS" == *fyp ]]; then
			$EXEC_SUDO mount -t ext2fyp -o ro "$DEV" "$MNT"
		else
			$EXEC_SUDO mount -t ext2 -o ro "$DEV" "$MNT"
		fi

		RESULT_DIR="$TEST_REPORT_DIR/$FS"
		mkdir -p "$RESULT_DIR"
		./fs-walker "$MNT" 2> "$RESULT_DIR/fs-walker.stderr.log" > "$RESULT_DIR/fs-walker.stdout.log"
		wait
		$EXEC_SUDO umount -f "$MNT"

		$EXEC_SUDO dmsetup remove "$DM_NAME"
	done
done
