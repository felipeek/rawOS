#!/bin/bash
mkdir -p bin/
nasm src/boot_sect.asm -f bin -o bin/boot_sect.bin
