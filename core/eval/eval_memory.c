#include "eval_memory.h"
#include <stdio.h>
#include <stdlib.h>
#include "../../util/types/types.h"


int stack_vm_execute(VM* vm, uint32_t opcode){
    switch (opcode) {
        case OP_HALT: return EXEC_NO_ERR;
        case OP_PUSH: {
            PrimitiveValue value;
            value.type = TYPE_INT;
            value.data.u = vm->program[vm->pc++];
            VM_PUSH(vm, value);
            break;
        }
        case OP_POP:  VM_POP(vm); break;
        case OP_PEEK: {
            primitive_print(VM_PEEK(vm));
            break;
        }
        case OP_DUP: {
            PrimitiveValue value = VM_PEEK(vm);
            VM_PUSH(vm, value);
            break;
        };
        case OP_SWAP: {
            PrimitiveValue a = VM_POP(vm);
            PrimitiveValue b = VM_POP(vm);

            VM_PUSH(vm, a);
            VM_PUSH(vm, b);

            break;
        };
        case OP_ROT: {
            PrimitiveValue c = VM_POP(vm);
            PrimitiveValue b = VM_POP(vm);
            PrimitiveValue a = VM_POP(vm);

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
            PrimitiveValue val = VM_POP(vm);
            PrimitiveValue addr = VM_POP(vm);

            if(addr.type != TYPE_INT){
                vm_error("Address should have a type INTEGER!");
                exit(1);
            }

            store_global(vm, addr.data.u, val);
            break;
        }

        case OP_LOAD: {
            PrimitiveValue addr = VM_POP(vm);

            if(addr.type != TYPE_INT){
                vm_error("Address should have a type INTEGER!");
                exit(1);
            }

            VM_PUSH(vm, load_global(vm, addr.data.u));
            break;
        }

        // for Integer Store
        case OP_ISTORE: {
            int val  = vm->program[vm->pc++];
            uint32_t addr = vm->program[vm->pc++];
            if (addr < 0 || addr >= VM_RAM_SIZE) {
                printf("Segfault: Invalid RAM address %d for store.\n", addr);
                exit(1);
            }
            else {
                PrimitiveValue value;
                value.type = TYPE_INT;
                value.data.u = val;
                store_global(vm, addr, value);
            }
            break;
        }

        // for Floating Point Store
        case OP_FSTORE: {
            // 1. Read the raw integer bits from the program stream
            int32_t raw_bits = vm->program[vm->pc++];
            uint32_t addr = (uint32_t)vm->program[vm->pc++];

            if (addr >= VM_RAM_SIZE) {
                printf("Segfault: Invalid RAM address %u for FSTORE.\n", addr);
                exit(1);
            } else {
                PrimitiveValue value;
                value.type = TYPE_FLOAT;
                value.data.u = raw_bits; // Now correctly holds 3.14
                store_global(vm, addr, value);
            }
            break;
        }

        // for Char Store
        case OP_CSTORE: {
            // Cast to uint16_t to match your type definition
            // Note: We pull from program stream as int32_t but cast down
            uint16_t val = (uint16_t)vm->program[vm->pc++];
            uint32_t addr = (uint32_t)vm->program[vm->pc++];

            if (addr >= VM_RAM_SIZE) {
                printf("Segfault: Invalid RAM address %u for CSTORE.\n", addr);
                exit(1);
            } else {
                PrimitiveValue value;
                value.type = TYPE_CHAR; // Using your uint16_t type ID
                value.data.c = (char)val; // Storing as char
                store_global(vm, addr, value);
            }
            break;
        }

        // for Byte Store
        case OP_BSTORE: {
            int8_t val = (int8_t)vm->program[vm->pc++];
            uint32_t addr = (uint32_t)vm->program[vm->pc++];

            if (addr >= VM_RAM_SIZE) {
                printf("Segfault: Invalid RAM address %u for BSTORE.\n", addr);
                exit(1);
            } else {
                PrimitiveValue value;
                value.type = TYPE_BYTE; // Using your uint16_t type ID
                value.data.b = val;
                store_global(vm, addr, value);
            }
            break;
        }

        // Local Integer Store
        case OP_ISTORE_L: {
            int val = vm->program[vm->pc++];
            uint32_t offset = vm->program[vm->pc++];

            LocalStackFrame *frame = VM_SF_PEEK(vm);
            if(frame->local_variable_count < offset + 1) frame->local_variable_count = offset + 1;

            PrimitiveValue value = { .type = TYPE_INT, .data.u = (uint32_t)val };
            store_stack(vm, frame->start, offset, value);
            break;
        }

        // Local Floating Point Store
        case OP_FSTORE_L: {
            int32_t raw_bits = vm->program[vm->pc++];
            uint32_t offset = (uint32_t)vm->program[vm->pc++];

            LocalStackFrame *frame = VM_SF_PEEK(vm);
            if(frame->local_variable_count < offset + 1) frame->local_variable_count = offset + 1;

            PrimitiveValue value = { .type = TYPE_FLOAT, .data.u = (uint32_t)raw_bits };
            store_stack(vm, frame->start, offset, value);
            break;
        }

        // Local Char Store
        case OP_CSTORE_L: {
            uint16_t val = (uint16_t)vm->program[vm->pc++];
            uint32_t offset = (uint32_t)vm->program[vm->pc++];

            LocalStackFrame *frame = VM_SF_PEEK(vm);
            if(frame->local_variable_count < offset + 1) frame->local_variable_count = offset + 1;

            PrimitiveValue value = { .type = TYPE_CHAR, .data.c = (char)val };
            store_stack(vm, frame->start, offset, value);
            break;
        }

        // Local Byte Store
        case OP_BSTORE_L: {
            int8_t val = (int8_t)vm->program[vm->pc++];
            uint32_t offset = (uint32_t)vm->program[vm->pc++];

            LocalStackFrame *frame = VM_SF_PEEK(vm);
            if(frame->local_variable_count < offset + 1) frame->local_variable_count = offset + 1;

            PrimitiveValue value = { .type = TYPE_BYTE, .data.b = val };
            store_stack(vm, frame->start, offset, value);
            break;
        }

        case OP_LOAD_L: {
            uint32_t offset = vm->program[vm->pc++];
            LocalStackFrame *frame = VM_SF_PEEK(vm);

            // Safety: Ensure the assembler didn't generate an invalid offset
            if (offset >= frame->local_variable_count) {
                vm_error("Index out of local frame bounds!");
                exit(-1);
            }

            VM_PUSH(vm, load_stack(vm, frame->start, offset));
            break;
        }

    }

    return EXEC_CONTINUE;
}
