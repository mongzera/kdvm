#ifndef H_EVAL_MEMORY
#define H_EVAL_MEMORY

#include "../kdvm.h"

int stack_vm_execute(VM* vm, uint32_t opcode);

int mem_vm_execute(VM* vm, uint32_t opcode);

#endif
