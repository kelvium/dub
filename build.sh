#!/bin/sh
set -e
clang main.c -o main -Wall -O2 -L/usr/lib/llvm/16/lib -I/usr/lib/llvm/16/include -lclang
