#!/bin/bash

tcc -I. main.c core/*.c core/eval/*.c math/*.c io/*.c util/hashmap/*.c util/types/*.c memory/*.c assembler/*.c -o kdvm -lm
