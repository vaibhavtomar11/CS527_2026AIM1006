#!/bin/sh
# small helper to run one test program
# usage :  ./run_test.sh array_add
#
# the test data file is only read, the result is written in data.byte

if [ -z "$1" ]; then
    echo "usage: ./run_test.sh <test name without extension>"
    exit 1
fi

if [ -f tests/$1.data ]; then
    ./mycomputer tests/$1.prog tests/$1.data data.byte
else
    echo "no data file for $1, data memory will start from all zero"
    rm -f data.byte
    ./mycomputer tests/$1.prog
fi
