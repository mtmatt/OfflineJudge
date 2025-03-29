#!/bin/bash

mkdir build
cd build
cmake ..
make -j

cp Run ../
cd ..

./Run
rm Run