#!/bin/sh
# small helper to run one test program
# usage :  ./run_test.sh sum_runtime

if [ -z "$1" ]; then
    echo "usage: ./run_test.sh <test name without extension>"
    exit 1
fi

# finalize() overwrites data.byte, so we always start from a fresh copy
if [ -f tests/$1.data ]; then
    cp tests/$1.data data.byte
else
    echo "no data file for $1, data memory will start from all zero"
    rm -f data.byte
fi

./mycomputer tests/$1.prog
