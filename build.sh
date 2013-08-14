#!/bin/sh

gcc -Wall -g dlnagui1.c $(pkg-config --cflags --libs gtk+-3.0) -o dlnagui1
