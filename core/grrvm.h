#ifndef GRRVM_H
#define GRRVM_H

#include <stdint.h>
#include <stdbool.h>
#include "vm_handler.h"
#include <stdlib.h>

#define VM_PROGRAM_MEM      1024
#define VM_STACK_SIZE       256
#define THREAD_STACK_SIZE   512

#define GLOBAL_RAM_SIZE     1024
#define HEAP_RAM_SIZE       4096
#define VM_RAM_SIZE         (GLOBAL_RAM_SIZE + HEAP_RAM_SIZE)

#define GLOBAL_RAM_START    0
#define HEAP_RAM_START      (GLOBAL_RAM_SIZE)

#define CALL_STACK_MEM 64
#define MAX_HANDLES 64
#define MAX_THREADS 32

// Stack operations
#define VM_THREAD_POP(vm_thread) vm_thread->stack[vm_thread->sp--]
#define VM_THREAD_PUSH(vm_thread, val) vm_thread->stack[++vm_thread->sp] = val
#define VM_THREAD_PEEK(vm_thread) vm_thread->stack[vm_thread->sp]

// Stack frame operations
#define VM_THREAD_SF_PUSH(vm_thread, stack_frame) vm_thread->stack_frame[++vm_thread->sfp] = stack_frame
#define VM_THREAD_SF_POP(vm_thread) vm_thread->stack_frame[vm_thread->sfp--]
#define VM_THREAD_SF_PEEK(vm_thread) &vm_thread->stack_frame[vm_thread->sfp]

#define VM_GET_INSTRUCTION(vm, pc) vm->program[pc++] // auto increment to next instruction
#define VM_SET_CHECKPOINT(thread) thread->pc_checkpoint = thread->pc // auto increment to next instruction


typedef struct{
    uint32_t start;
    uint32_t local_variable_count;
} LocalStackFrame;

typedef enum {
    WORD_AVAILABLE = 0x0,   // WORD can be claimed by HEAP or STACK for all Data Types
    WORD_OPEN,              // WORD can be modified by all threads, but this has an existing Data Type and was allocated.
    WORD_CONSTANT,          // WORD can be read, not modified.
    WORD_LOCKED,            // WORD is locked by a thread using it mid instruction.
    WORD_MUTEX_LOCKED,      // WORD is locked by owner thread. Other threads cannot use this.
    WORD_GARBAGE,           // WORD is declared as garbage, ready to be collected by garbage collector.
} WordState;

// Helper union for bit-casting
typedef enum {
    TYPE_INT = 0x0,
    TYPE_FLOAT,
    TYPE_CHAR,
    TYPE_BYTE,
    TYPE_ADDRESS
} PrimitiveType;

typedef struct {
    PrimitiveType type;
    uint8_t word_state;
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
    THREAD_FREE = 0x0,
    THREAD_YIELD,
    THREAD_ACTIVE,
    THREAD_IO_BLOCKED

} ThreadStatus;

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
    OP_STORE        = OPT_MEM | 0x00,
    OP_LOAD         = OPT_MEM | 0x01,
    OP_STORE_OFF    = OPT_MEM | 0x02,
    OP_LOAD_OFF     = OPT_MEM | 0x03,

    OP_ISTORE   = OPT_MEM | 0x04,
    OP_FSTORE   = OPT_MEM | 0x05,
    OP_CSTORE   = OPT_MEM | 0x06,
    OP_BSTORE   = OPT_MEM | 0x07,

    OP_HALLOC   = OPT_MEM | 0x08,
    OP_HFREE    = OPT_MEM | 0x09,

    OP_ISTORE_L = OPT_MEM | 0x0A,
    OP_FSTORE_L = OPT_MEM | 0x0B,
    OP_CSTORE_L = OPT_MEM | 0x0C,
    OP_BSTORE_L = OPT_MEM | 0x0D,
    OP_LOAD_L   = OPT_MEM | 0x0E,
    OP_STORE_L  = OPT_MEM | 0x0F,

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
    OP_OUT      = OPT_IO | 0x00,
    OP_OUT_LN   = OPT_IO | 0x01,
    OP_FOUT     = OPT_IO | 0x02,
    OP_FOUT_LN  = OPT_IO | 0x03,
    OP_IN       = OPT_IO | 0x04,

    OP_SYS_READ   = OPT_SYSCALL | 0x01,
    OP_SYS_WRITE  = OPT_SYSCALL | 0x02,

} OpCodes;

// The encapsulated VM State
struct VM;

// VM Thread
typedef struct {
    int64_t pc;    // Program Counter
    int64_t pc_checkpoint;    // Program Counter checkpoint
    int64_t sp;    // Stack Pointer
    int64_t csp;   // Call Stack Pointer
    int64_t sfp;   // Stack Frame Pointer
    struct VM* vm;      // initialize on init
    PrimitiveValue stack[VM_STACK_SIZE];         // operation stack
    PrimitiveValue ram_stack[THREAD_STACK_SIZE]; // ram stack
    LocalStackFrame stack_frame[THREAD_STACK_SIZE];
    uint32_t call_stack[CALL_STACK_MEM]; // stores the previous instruction number before the CALL, so we can make recursion possible.
    uint8_t status;

} VM_Thread;

typedef struct VM{
    uint32_t _global_start;
    uint32_t program_size;
    uint32_t program[VM_PROGRAM_MEM];
    PrimitiveValue ram[VM_RAM_SIZE];         // [0]-> RAM_MEM: GLOBAL [1/4], HEAP[3/4]
    VM_Handle handle[MAX_HANDLES];
    VM_Thread *threads[MAX_THREADS];
} VM;

// Global Function Prototypes
void vm_init(VM* vm);
int vm_execute(VM* vm);
int load_kdm_file(const char* filename, VM* vm);
void vm_error(const char* message);
void vm_depr(const char* message);

//Global Thread functions
VM_Thread *vm_request_thread(VM* vm);
VM_Thread *vm_get_thread(VM* vm, int thread_id);
void vm_init_thread(VM* vm, VM_Thread *thread);

#endif // GRRVM_H
