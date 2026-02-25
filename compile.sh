#!/bin/bash

# Compile programs using the library
# Usage: ./compile.sh main.c

PROGRAM="${1%.c}"
TMP_DIR="tmp_obj"
CFLAGS="$(sdl2-config --cflags)"
LIBS="$(sdl2-config --libs)"
INCLUDE="-I./include"

# Create temporary directory
mkdir -p "$TMP_DIR"

# Compile object files
g++ -std=c++11 -c src/wxui_c.cpp -o "$TMP_DIR/wxui_c.o" $CFLAGS $INCLUDE
gcc -std=c99 -c "$1" -o "$TMP_DIR/temp.o" $CFLAGS $INCLUDE

# Link and generate executable
PROGRAM="${1##*/}"
PROGRAM="${PROGRAM%.c}"

# Clean up
rm -rf "$TMP_DIR"

echo "Build complete: $PROGRAM"