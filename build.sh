#!/bin/sh
mkdir build
cd build
cmake ..
make
./cppmonad
cd ..
rm -rf build