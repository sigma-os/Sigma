#!/bin/zsh

mkdir -p build

echo "Hello initrd" >> build/test.txt
echo "Hello world" >> build/world.txt

cd build

tar -czf ../initrd.tar *

cd ..

rm -rf build