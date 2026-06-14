#ifndef KDVM_H
#define KDVM_H

#include <stdint.h>
#include <stdbool.h>
#include "vm_handler.h"

#define VM_PROGRAM_MEM      1024
#define VM_STACK_SIZE       256
#define VM_RAM_SIZE         1024

#define GLOBAL_RAM_SIZE     VM_RAM_SIZE / 4
#define HEAP_RAM_SIZE       (VM_RAM_SIZE - GLOBAL_RAM_SIZE) / 2
#define STACK_RAM_SIZE      (VM_RAM_SIZE - GLOBAL_RAM_SIZE) / 2

#define GLOBAL_RAM_START    0
#define HEAP_RAM_START      GLOBAL_RAM_SIZE
#define STACK_RAM_START     GLOBAL_RAM_SIZE + HEAP_RAM_SIZE



#define CALL_STACK_MEM 64
#define MAX_HANDLES 64

#define VM_POP(vm) vm->stack[vm->sp--]
#define VM_PUSH(vm, val) vm->stack[++vm->sp] = val
#define VM_PEEK(vm) vm->stack[vm->sp]

// Helper union for bit-casting
typedef enum {
    TYPE_INT = 0x0,
    TYPE_FLOAT = 0x1,
    TYPE_CHAR = 0x2,
    TYPE_BYTE = 0x3
} PrimitiveType;

typedef struct {
    PrimitiveType type;
    union {
        float f;
        int32_t u;
        uint16_t c;
        int8_t b;
    } data;
} PrimitiveValue;


typedef enum {
    EXEC_ERR = -1,
    EXEC_NO_ERR = 0,
    EXEC_CONTINUE = 1
} ExecStatus;

typedef enum{
    OPT_MEMSTACK = 0x00,
    OPT_IMATH = 0x10,
    OPT_MEM = 0x20,
    OPT_CONTROL = 0x30,
    OPT_IO = 0x40,
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

    OP_FPUSH  = OPT_MEMSTACK | 0x07,
    OP_CPUSH  = OPT_MEMSTACK | 0x08,
    OP_BPUSH  = OPT_MEMSTACK | 0x09,

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
    OP_ISTORE = OPT_MEM | 0x03,
    OP_FSTORE = OPT_MEM | 0x04,
    OP_CSTORE = OPT_MEM | 0x05,
    OP_BSTORE = OPT_MEM | 0x06,

    // Control Flow & Subroutines
    OP_JUMP   = OPT_CONTROL | 0x00,
    OP_JNZ    = OPT_CONTROL | 0x01,
    OP_JZ     = OPT_CONTROL | 0x02,
    OP_CMPEQ  = OPT_CONTROL | 0x03,
    OP_CMPNEQ = OPT_CONTROL | 0x04,
    OP_CMPLT  = OPT_CONTROL | 0x05,
    OP_CMPLE  = OPT_CONTROL | 0x06,
    OP_CMPGT  = OPT_CONTROL | 0x07,
    OP_CMPGE  = OPT_CONTROL | 0x08,
    OP_CALL   = OPT_CONTROL | 0x0A,
    OP_RET    = OPT_CONTROL | 0x0B,

    // I/O
    OP_OUT    = OPT_IO | 0x00,
    OP_FOUT   = OPT_IO | 0x01,
    OP_IN     = OPT_IO | 0x02,

    OP_SYS_READ   = OPT_SYSCALL | 0x01,
    OP_SYS_WRITE  = OPT_SYSCALL | 0x02,

} OpCodes;

// The encapsulated VM State
typedef struct {
    uint32_t program[VM_PROGRAM_MEM];
    PrimitiveValue stack[VM_STACK_SIZE];
    PrimitiveValue ram[VM_RAM_SIZE];         // [0]-> RAM_MEM: GLOBAL [1/4], HEAP[1/2], STACK [1/2]
    uint32_t call_stack[CALL_STACK_MEM]; // stores the previous instruction number before the CALL, so we can make recursion possible.

    VM_Handle handle[MAX_HANDLES];

    uint32_t pc;    // Program Counter
    int32_t  sp;    // Stack Pointer
    int32_t  csp;   // Call Stack Pointer

    uint32_t _global_start;

    int program_size;
} VM;

// Global Function Prototypes
void vm_init(VM* vm);
int vm_execute(VM* vm);
int load_kdm_file(const char* filename, VM* vm);
void vm_error(const char* message);
void vm_depr(const char* message);

#endif // KDVM_H
