#!/bin/sh

dmsetup create $DM_NAME --table "0 $(blockdev --getsz $ORIG_DEVICE) dust $ORIG_DEVICE 0 512"
