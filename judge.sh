#!/bin/bash

mkdir build
cd build || exit
cmake ..
make -j

cp Run ../
cd .. || exit

printf "Command: ./Run %s %s %s\n" "$1" "$2" "$3"

./Run $1 "$2" "$3"
rm Run