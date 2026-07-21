#include "eval_memory.h"
#include <stdio.h>
#include <stdlib.h>
#include "../../util/types/types.h"


int stack_vm_execute(VM*vm, VM_Thread* thread, uint32_t opcode){
    switch (opcode) {
        case OP_HALT: return EXEC_NO_ERR;
        case OP_PUSH: {
            PrimitiveValue value;
            value.type = TYPE_INT;
            value.data.u = VM_GET_INSTRUCTION(vm, thread->pc);
            VM_THREAD_PUSH(thread, value);
            break;
        }
        case OP_FPUSH: {
            PrimitiveValue value;
            value.type = TYPE_FLOAT;
            value.data.u = VM_GET_INSTRUCTION(vm, thread->pc);
            VM_THREAD_PUSH(thread, value);
            break;
        }
        case OP_CPUSH: {
            PrimitiveValue value;
            value.type = TYPE_CHAR;
            value.data.u = VM_GET_INSTRUCTION(vm, thread->pc);
            VM_THREAD_PUSH(thread, value);
            break;
        }
        case OP_BPUSH: {
            PrimitiveValue value;
            value.type = TYPE_BYTE;
            value.data.u = VM_GET_INSTRUCTION(vm, thread->pc);
            VM_THREAD_PUSH(thread, value);
            break;
        }
        case OP_POP:  VM_THREAD_POP(thread); break;
        case OP_PEEK: {
            primitive_print(VM_THREAD_PEEK(thread));
            break;
        }
        case OP_DUP: {
            PrimitiveValue value = VM_THREAD_PEEK(thread);
            VM_THREAD_PUSH(thread, value);
            break;
        };
        case OP_SWAP: {
            PrimitiveValue a = VM_THREAD_POP(thread);
            PrimitiveValue b = VM_THREAD_POP(thread);

            VM_THREAD_PUSH(thread, a);
            VM_THREAD_PUSH(thread, b);

            break;
        };
        case OP_ROT: {
            PrimitiveValue c = VM_THREAD_POP(thread);
            PrimitiveValue b = VM_THREAD_POP(thread);
            PrimitiveValue a = VM_THREAD_POP(thread);

            VM_THREAD_PUSH(thread, b);
            VM_THREAD_PUSH(thread, c);
            VM_THREAD_PUSH(thread, a);

            break;
        }

    }

    return EXEC_CONTINUE;
}

int mem_vm_execute(VM* vm, VM_Thread* thread, uint32_t opcode){
    switch (opcode) {
        // Memory
        case OP_STORE: {
            PrimitiveValue val = VM_THREAD_POP(thread);
            PrimitiveValue addr = VM_THREAD_POP(thread);

            if(addr.type != TYPE_INT){
                vm_error("Address should have a type INTEGER!");
                exit(1);
            }

            store_global(vm, addr.data.u, val);
            break;
        }

        case OP_LOAD: {
            PrimitiveValue addr = VM_THREAD_POP(thread);

            if(addr.type != TYPE_INT){
                vm_error("Address should have a type INTEGER!");
                exit(1);
            }

            VM_THREAD_PUSH(thread, load_global(vm, addr.data.u));
            break;
        }

        // for Integer Store
        case OP_ISTORE: {
            int val  = VM_GET_INSTRUCTION(vm, thread->pc);
            uint32_t addr = VM_GET_INSTRUCTION(vm, thread->pc);
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
            int32_t raw_bits = VM_GET_INSTRUCTION(vm, thread->pc);
            uint32_t addr = VM_GET_INSTRUCTION(vm, thread->pc);

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
            uint16_t val = VM_GET_INSTRUCTION(vm, thread->pc);
            uint32_t addr = VM_GET_INSTRUCTION(vm, thread->pc);

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
            int8_t val = VM_GET_INSTRUCTION(vm, thread->pc);
            uint32_t addr = VM_GET_INSTRUCTION(vm, thread->pc);

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


        // CREATE TEST FOR THIS
        // for H_ALLOC
        case OP_HALLOC: {
            int8_t size = VM_GET_INSTRUCTION(vm, thread->pc);

            uint32_t ptr_val = malloc_heap(thread, size);

            PrimitiveValue value = {
                .type = TYPE_ADDRESS,
                .word_state = WORD_OPEN,
                .data = ptr_val
            };

            VM_THREAD_PUSH(thread, value);

            break;
        }

        // for H_FREE
        case OP_HFREE: {
            // 1. Pop the pointer from the evaluation stack
            PrimitiveValue val = VM_THREAD_POP(thread);

            if (val.type == TYPE_ADDRESS) {

                free_heap(vm, val.data.u);
            }

            break;
        }


        // Local Integer Store
        case OP_STORE_L: {
            PrimitiveValue value = VM_THREAD_POP(thread);
            uint32_t offset = VM_GET_INSTRUCTION(vm, thread->pc);

            LocalStackFrame *frame = VM_THREAD_SF_PEEK(thread);
            if(frame->local_variable_count < offset + 1) frame->local_variable_count = offset + 1;

            store_stack(thread, frame->start, offset, value);
            break;
        }

        // Local Integer Store
        case OP_ISTORE_L: {
            int val = VM_GET_INSTRUCTION(vm, thread->pc);
            uint32_t offset = VM_GET_INSTRUCTION(vm, thread->pc);

            LocalStackFrame *frame = VM_THREAD_SF_PEEK(thread);
            if(frame->local_variable_count < offset + 1) frame->local_variable_count = offset + 1;

            PrimitiveValue value = { .type = TYPE_INT, .data.u = (uint32_t)val };
            store_stack(thread, frame->start, offset, value);
            break;
        }

        // Local Floating Point Store
        case OP_FSTORE_L: {
            int32_t raw_bits = VM_GET_INSTRUCTION(vm, thread->pc);
            uint32_t offset = (uint32_t)VM_GET_INSTRUCTION(vm, thread->pc);

            // TODO: Finish Thread
            LocalStackFrame *frame = VM_THREAD_SF_PEEK(thread);
            if(frame->local_variable_count < offset + 1) frame->local_variable_count = offset + 1;

            PrimitiveValue value = { .type = TYPE_FLOAT, .data.u = (uint32_t)raw_bits };
            store_stack(thread, frame->start, offset, value);
            break;
        }

        // Local Char Store
        case OP_CSTORE_L: {
            uint16_t val = (uint16_t) VM_GET_INSTRUCTION(vm, thread->pc);
            uint32_t offset = (uint32_t) VM_GET_INSTRUCTION(vm, thread->pc);

            LocalStackFrame *frame = VM_THREAD_SF_PEEK(thread);
            if(frame->local_variable_count < offset + 1) frame->local_variable_count = offset + 1;

            PrimitiveValue value = { .type = TYPE_CHAR, .data.c = (char)val };
            store_stack(thread, frame->start, offset, value);
            break;
        }

        // Local Byte Store
        case OP_BSTORE_L: {
            int8_t val = (int8_t) VM_GET_INSTRUCTION(vm, thread->pc);
            uint32_t offset = (uint32_t) VM_GET_INSTRUCTION(vm, thread->pc);

            LocalStackFrame *frame = VM_THREAD_SF_PEEK(thread);
            if(frame->local_variable_count < offset + 1) frame->local_variable_count = offset + 1;

            PrimitiveValue value = { .type = TYPE_BYTE, .data.b = val };
            store_stack(thread, frame->start, offset, value);
            break;
        }

        case OP_LOAD_L: {
            uint32_t offset = (uint32_t) VM_GET_INSTRUCTION(vm, thread->pc);
            LocalStackFrame *frame = VM_THREAD_SF_PEEK(thread);

            // Safety: Ensure the assembler didn't generate an invalid offset
            if (offset >= frame->local_variable_count) {
                vm_error("Index out of local frame bounds!");
                exit(-1);
            }

            VM_THREAD_PUSH(thread, load_stack(thread, frame->start, offset));
            break;
        }

    }

    return EXEC_CONTINUE;
}
