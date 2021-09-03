#!/usr/bin/env bash

python3 -m ipp

export CFLAGS="-g -Wall -I."
export CXXFLAGS="$CFLAGS -std=c++17"

echo "Build with GCC" &&
gcc -c $CFLAGS ipp.c -o ipp.c.o &&
g++ -c $CXXFLAGS ipp.cpp -o ipp.cpp.o &&
g++ -c $CXXFLAGS ipp_test.cpp -o ipp_test.cpp.o &&
g++ ipp.c.o ipp.cpp.o ipp_test.cpp.o -o ipp_test -lgtest -lpthread &&
./ipp_test

echo "Build with Clang" &&
clang -c $CFLAGS ipp.c -o ipp.c.o &&
clang++ -c $CXXFLAGS ipp.cpp -o ipp.cpp.o &&
clang++ -c $CXXFLAGS ipp_test.cpp -o ipp_test.cpp.o &&
clang++ ipp.c.o ipp.cpp.o ipp_test.cpp.o -o ipp_test -lgtest -lpthread &&
./ipp_test
