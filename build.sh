#!/bin/sh
set -e
CFLAGS="-Wall -O2 -g -I/usr/lib/llvm/16/include"
LDFLAGS="-O2 -g -L/usr/lib/llvm/16/lib -lclang"
clang hash.c -o hash.o -c $CFLAGS
clang main.c -o main.o -c $CFLAGS
clang main.o hash.o -o main $LDFLAGS
