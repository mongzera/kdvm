#ifndef KDVM_H
#define KDVM_H

#include <stdint.h>
#include <stdbool.h>
#include "vm_handler.h"

#define PROGRAM_MEM 1024
#define STACK_MEM   256
#define RAM_MEM     256
#define CALL_STACK_MEM 64 // New: Dedicated stack for function return addresses
#define MAX_HANDLES 64

#define VM_POP(vm) (int32_t)vm->stack[vm->sp--]
#define VM_PUSH(vm, val) vm->stack[++vm->sp] = (uint32_t)val

typedef enum {
    EXEC_ERR = -1,
    EXEC_NO_ERR = 0,
    EXEC_CONTINUE = 1,
} ExecStatus;

typedef enum{
    OPT_MEMSTACK = 0x00,
    OPT_IMATH = 0x10,
    OPT_MEM = 0x20,
    OPT_CONTROL = 0x30,
    OPT_IO = 0x40,
    OPT_FMATH = 0x50,
    OPT_SYSCALL = 0x60

} OPCODE_TYPE;

typedef enum {
    // Memory & Stack
    OP_HALT   = OPT_MEMSTACK | 0x00,
    OP_PUSH   = OPT_MEMSTACK | 0x01,
    OP_POP    = OPT_MEMSTACK | 0x02,
    OP_PEEK   = OPT_MEMSTACK | 0x03,
    OP_DUP    = OPT_MEMSTACK | 0x04,
    OP_SWAP   = OPT_MEMSTACK | 0x05,
    OP_ROT    = OPT_MEMSTACK | 0x06,

    // Integer Arithmetic
    OP_ADD    = OPT_IMATH | 0x00,
    OP_SUB    = OPT_IMATH | 0x01,
    OP_MUL    = OPT_IMATH | 0x02,
    OP_DIV    = OPT_IMATH | 0x03,
    OP_MOD    = OPT_IMATH | 0x04,
    OP_INC    = OPT_IMATH | 0x05,
    OP_DEC    = OPT_IMATH | 0x06,

    // Memory (RAM)
    OP_STORE  = OPT_MEM | 0x00,
    OP_LOAD   = OPT_MEM | 0x01,
    OP_MSET   = OPT_MEM | 0x02, // Memory set

    // Control Flow & Subroutines
    OP_JUMP   = OPT_CONTROL | 0x00,
    OP_JNZ    = OPT_CONTROL | 0x01,
    OP_JZ     = OPT_CONTROL | 0x02,
    OP_CMPEQ  = OPT_CONTROL | 0x03,
    OP_CMPLT  = OPT_CONTROL | 0x04,
    OP_CALL   = OPT_CONTROL | 0x0A,
    OP_RET    = OPT_CONTROL | 0x0B,

    // I/O
    OP_OUT    = OPT_IO | 0x00,
    OP_FOUT   = OPT_IO | 0x01,
    OP_IN     = OPT_IO | 0x02,

    // Floating Point Arithmetic
    OP_FPUSH  = OPT_FMATH | 0x00,
    OP_FADD   = OPT_FMATH | 0x01,
    OP_FSUB   = OPT_FMATH | 0x02,
    OP_FMUL   = OPT_FMATH | 0x03,
    OP_FDIV   = OPT_FMATH | 0x04,

    OP_SYS_READ   = OPT_SYSCALL | 0x01,
    OP_SYS_WRITE  = OPT_SYSCALL | 0x02,

} OpCodes;

// The encapsulated VM State
typedef struct {
    uint32_t program[PROGRAM_MEM];
    uint32_t stack[STACK_MEM];
    uint32_t ram[RAM_MEM];
    uint32_t call_stack[CALL_STACK_MEM];

    VM_Handle handle[MAX_HANDLES];

    uint32_t pc;    // Program Counter
    int32_t  sp;    // Stack Pointer
    int32_t  csp;   // Call Stack Pointer

    uint32_t _global_start;

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
