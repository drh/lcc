#!/bin/bash

mkdir -p build
YACC="bison -o y.tab.c" BUILDDIR=build HOSTFILE=./etc/linux.c CFLAGS=-std=gnu99 make all -B
