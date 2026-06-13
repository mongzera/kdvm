#include "eval_memory.h"
#include <stdio.h>
#include <stdlib.h>

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
        };
        case OP_ROT: {
            uint32_t c = VM_POP(vm);
            uint32_t b = VM_POP(vm);
            uint32_t a = VM_POP(vm);

            VM_PUSH(vm, b);
            VM_PUSH(vm, c);
            VM_PUSH(vm, a);

            break;
        }

    }

    return EXEC_CONTINUE;
}

int mem_vm_execute(VM* vm, uint32_t opcode){
    switch (opcode) {
        // Memory
        case OP_STORE: {
            int val = VM_POP(vm);
            uint32_t addr = VM_POP(vm);
            if (addr < 0 || addr >= RAM_MEM) {
                printf("Segfault: Invalid RAM address %d for store.\n", addr);
                exit(1);
            }
            else vm->ram[addr] = val; break;
        }

        case OP_LOAD: {
            uint32_t addr = VM_POP(vm);
            if (addr < 0 || addr >= RAM_MEM) {
                printf("Segfault: Invalid RAM address %d for load.\n", addr);
                exit(1);
            }
            else vm->stack[++vm->sp] = vm->ram[addr]; break;
        }

        // for Integer Store
        case OP_ISTORE: {
            int val  = vm->program[vm->pc++];
            uint32_t addr = vm->program[vm->pc++];
            if (addr < 0 || addr >= RAM_MEM) {
                printf("Segfault: Invalid RAM address %d for store.\n", addr);
                exit(1);
            }
            else vm->ram[addr] = val; break;
        }

        // for Floating Point Store
        case OP_FSTORE: {
            float val  = vm->program[vm->pc++];
            uint32_t addr = vm->program[vm->pc++];
            if (addr < 0 || addr >= RAM_MEM) {
                printf("Segfault: Invalid RAM address %d for store.\n", addr);
                exit(1);
            }
            else vm->ram[addr] = val; break;
        }

    }

    return EXEC_CONTINUE;
}
