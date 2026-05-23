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

# === PATH LIBRERIE E INCLUDE (macOS) ===
GLFW_PREFIXES=(
    "${GLFW_PREFIX:-}"
    "$(brew --prefix glfw 2>/dev/null)"
    "/opt/homebrew"
    "/usr/local"
)

LIBS="-lglfw -framework OpenGL -framework Cocoa -framework IOKit -framework CoreVideo -lm"
INCLUDES="-I./include"
FOUND_GLFW=0

for prefix in "${GLFW_PREFIXES[@]}"; do
    if [[ -n "$prefix" ]] &&
       [[ -f "$prefix/lib/libglfw.dylib" || -f "$prefix/lib/libglfw3.a" ]]; then
        LIBS="-L$prefix/lib $LIBS"
        INCLUDES="$INCLUDES -I$prefix/include"
        FOUND_GLFW=1
        break
    fi
done

if [[ "$FOUND_GLFW" -ne 1 ]]; then
    echo "GLFW not found. Install it with: brew install glfw"
    exit 1
fi

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
