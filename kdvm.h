#ifndef KDVM_H
#define KDVM_H

#include <stdint.h>
#include <stdbool.h>

#define PROGRAM_MEM 1024
#define STACK_MEM   256
#define RAM_MEM     256
#define CALL_STACK_MEM 64 // New: Dedicated stack for function return addresses

typedef enum {
    OP_HALT = 0x00,
    OP_PUSH = 0x01, OP_POP  = 0x02,

    // Integer Math
    OP_ADD  = 0x10, OP_SUB  = 0x11, OP_MUL  = 0x12, OP_DIV  = 0x13,

    // Memory
    OP_STORE = 0x20, OP_LOAD = 0x21,

    // Control Flow & Comparisons
    OP_JUMP  = 0x30, OP_JIF  = 0x31,
    OP_CMPEQ = 0x32, OP_CMPLT = 0x33,

    // Subroutines (Functions)
    OP_CALL  = 0x3A, OP_RET  = 0x3B,

    // I/O
    OP_OUT   = 0x3E, OP_FOUT = 0x3F, OP_IN = 0x4A,

    // Floating Point Math
    OP_FPUSH = 0x50,
    OP_FADD  = 0x51, OP_FSUB  = 0x52, OP_FMUL  = 0x53, OP_FDIV  = 0x54
} OpCodes;

// The encapsulated VM State
typedef struct {
    uint32_t program[PROGRAM_MEM];
    uint32_t stack[STACK_MEM];
    uint32_t ram[RAM_MEM];
    uint32_t call_stack[CALL_STACK_MEM];

    uint32_t pc;    // Program Counter
    int32_t  sp;    // Stack Pointer
    int32_t  csp;   // Call Stack Pointer (for functions)

    int program_size;
} VM;

// Helper union for bit-casting
typedef union {
    float f;
    uint32_t u;
} FloatCast;

// Global Function Prototypes
void vm_init(VM* vm);
int vm_execute(VM* vm);
int load_kdm_file(const char* filename, VM* vm);
void vm_error(const char* message);

#endif // KDVM_H
