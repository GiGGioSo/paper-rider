#!/bin/sh

CFLAGS="-ggdb"
# CFLAGS="${CFLAGS} -Werror"
 CFLAGS="${CFLAGS} -Wall"
 CFLAGS="${CFLAGS} -Wextra"
# CFLAGS="${CFLAGS} -Waggregate-return"
# CFLAGS="${CFLAGS} -Wbad-function-cast"
# CFLAGS="${CFLAGS} -Wcast-align"
# CFLAGS="${CFLAGS} -Wcast-qual"
# CFLAGS="${CFLAGS} -Wdeclaration-after-statement"
# CFLAGS="${CFLAGS} -Wfloat-equal"
# CFLAGS="${CFLAGS} -Wformat=2"
# CFLAGS="${CFLAGS} -Wlogical-op"
# CFLAGS="${CFLAGS} -Wmissing-declarations"
# CFLAGS="${CFLAGS} -Wmissing-include-dirs"
# CFLAGS="${CFLAGS} -Wmissing-prototypes"
# CFLAGS="${CFLAGS} -Wnested-externs"
# CFLAGS="${CFLAGS} -Wpointer-arith"
# CFLAGS="${CFLAGS} -Wredundant-decls"
# CFLAGS="${CFLAGS} -Wsequence-point"
# CFLAGS="${CFLAGS} -Wshadow"
# CFLAGS="${CFLAGS} -Wstrict-prototypes"
# CFLAGS="${CFLAGS} -Wswitch"
# CFLAGS="${CFLAGS} -Wundef"
# CFLAGS="${CFLAGS} -Wunreachable-code"
# CFLAGS="${CFLAGS} -Wunused-but-set-parameter"
# CFLAGS="${CFLAGS} -Wwrite-strings"
LIBS="-lglfw -ldl -lpthread -lm -lGL"

EXE="./bin/paper"

SRCS_FILES=$(find ./src/ -name "*.cpp" -or -name "*.c")
INCLUDES="-I./include"

rm -r bin
mkdir bin

echo "Compiling the sources..."
echo $SRCS_FILES

clang++ $SRCS_FILES $CFLAGS -o $EXE $LIBS $INCLUDES

