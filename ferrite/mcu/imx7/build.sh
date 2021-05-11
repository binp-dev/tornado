#!/bin/sh

mkdir -p ./build &&
cd ./build &&
cmake -DCMAKE_TOOLCHAIN_FILE="$FREERTOS_DIR/tools/cmake_toolchain_files/armgcc.cmake" -G "Unix Makefiles" -DCMAKE_BUILD_TYPE=Release  .. &&
make -j4
