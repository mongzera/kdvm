#!/bin/bash

gcc -O3 -I. main.c core/*.c core/eval/*.c math/*.c io/*.c util/hashmap/*.c util/types/*.c memory/*.c assembler/*.c -o grrvm -lm
