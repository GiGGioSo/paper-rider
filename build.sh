#!/bin/bash

# === FLAG COMUNI PER DEBUG E RELEASE ===
COMMON_CFLAGS="
    -Wall
    -Wextra
    -Wswitch
    -Wstrict-prototypes
"

# === CONFIGURAZIONE COMPILAZIONE ===
if [[ "$1" == "release" ]]; then
    CFLAGS="-O3 $COMMON_CFLAGS"
else
    CFLAGS="-ggdb $COMMON_CFLAGS"
fi

# === PATH LIBRERIE E INCLUDE (ADATTALI A LINUX!) ===
LIBS="-lGL -lglfw -lm"
INCLUDES="-I./include"

EXE="./bin/paper"

# === RACCOLTA FILE SORGENTI .C ===
SRCS_FILES=$(find src -type f -name "*.c")

# === PULIZIA E CREAZIONE CARTELLE ===
rm -rf bin
mkdir -p bin

echo "Compiling the following sources:"
echo "$SRCS_FILES"

# === COMPILAZIONE ===
clang $SRCS_FILES $CFLAGS -std=c11 -o "$EXE" $INCLUDES $LIBS

if [[ $? -ne 0 ]]; then
    echo "Build failed!"
    exit 1
fi

echo "Build succeeded!"
