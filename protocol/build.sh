#!/bin/bash
INPUT="$(realpath $1)"
touch $2
OUTPUT="$(realpath $2)"

pwd $SYSROOT

cd $4
./iota.py -s $3 -g cpp $INPUT -o $OUTPUT