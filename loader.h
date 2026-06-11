#ifndef LOADER_H
#define LOADER_H

#include <stdint.h>

#define PROGRAM_MEM 256
#define STACK_MEM   256
#define RAM_MEM     256  // New: Random access data memory

typedef enum {
    OP_HALT = 0x00,
    OP_PUSH = 0x01,
    OP_POP  = 0x02,
    OP_ADD  = 0x10,
    OP_SUB  = 0x11,
    OP_MUL  = 0x12,

    // Turing Complete Extensions
    OP_STORE = 0x20, // Pop value, pop address, RAM[address] = value
    OP_LOAD  = 0x21, // Pop address, push RAM[address]
    OP_JUMP  = 0x30, // Jump unconditionally to PC target
    OP_JIF   = 0x31  // Pop condition value. If != 0, jump to PC target
} OpCodes;

extern uint32_t PROGRAM[PROGRAM_MEM];
extern uint32_t STACK[STACK_MEM];
extern uint32_t RAM[RAM_MEM]; // Declared global RAM

int load_kdm_file(const char* filename);
void error(const char* message);

#endif // LOADER_H
