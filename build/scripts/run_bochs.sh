#!/bin/bash

./build/scripts/mkimage.sh
bochs -f build/.bochsrc -q