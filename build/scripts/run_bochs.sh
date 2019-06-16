#!/bin/bash

mkdir build/log

./build/scripts/mkimage.sh
bochs -f build/.bochsrc -q