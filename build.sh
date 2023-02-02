#!/bin/sh

CFLAGS="-ggdb -Wall"
LIBS="-lglfw -ldl -lpthread -lm"

EXE="./bin/paper"

SRCS_FILES=$(find ./src/ -name "*.cpp" -or -name "*.c")

rm -r bin
mkdir bin

echo "Compiling the sources..."

clang++ $CFLAGS $LIBS -o $EXE $SRCS_FILES

