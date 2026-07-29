#!/bin/bash
set -e  # Exit immediately if a command fails

CPU_FLAGS="-mcpu=cortex-m0plus -mthumb"
SPECS="--specs=nano.specs --specs=nosys.specs"

arm-none-eabi-gcc -O3 $CPU_FLAGS $SPECS -I. \
    main.c \
    core/*.c \
    core/eval/*.c \
    math/*.c \
    io/*.c \
    util/hashmap/*.c \
    util/types/*.c \
    memory/*.c \
    assembler/*.c \
    -o grrvm.elf \
    -lm

echo "Build complete: grrvm.elf"
