#include "eval_memory.h"
#include <stdio.h>

int stack_vm_execute(VM* vm, uint32_t opcode){
    switch (opcode) {
        case OP_HALT: return EXEC_NO_ERR;
        case OP_PUSH: vm->stack[++vm->sp] = vm->program[vm->pc++]; break;
        case OP_POP:  vm->sp--; break;
        case OP_PEEK: printf("%d\n", vm->stack[vm->sp]); break;
        case OP_DUP: {
            uint32_t value = vm->stack[vm->sp];
            vm->stack[++vm->sp] = value;
            break;
        };
        case OP_SWAP: {
            uint32_t a = vm->stack[vm->sp--];
            uint32_t b = vm->stack[vm->sp--];

            vm->stack[++vm->sp] = a;
            vm->stack[++vm->sp] = b;

            break;
        }

    }

    return EXEC_CONTINUE;
}

int mem_vm_execute(VM* vm, uint32_t opcode){
    switch (opcode) {
        // Memory
        case OP_STORE: {
            uint32_t val = vm->stack[vm->sp--];
            uint32_t addr = vm->stack[vm->sp--];
            vm->ram[addr] = val; break;
        }
        case OP_LOAD: {
            uint32_t addr = vm->stack[vm->sp--];
            vm->stack[++vm->sp] = vm->ram[addr]; break;
        }
    }

    return EXEC_CONTINUE;
}
