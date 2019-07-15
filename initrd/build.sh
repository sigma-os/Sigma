#!/bin/zsh

mkdir -p build

echo "Hello initrd" >> ./build/test.txt
echo "Hello world" >> ./build/world.txt

cd ./initrd/test
make all
cd ../..
cp ./initrd/test/test ./build/test

cd build

tar -czf ../initrd.tar *

cd ..

rm -rf build