#ifndef H_EVAL_MEMORY
#define H_EVAL_MEMORY

#include "../kdvm.h"
#include "../../memory/memory.h"

int stack_vm_execute(VM* vm, uint32_t opcode);

int mem_vm_execute(VM* vm, uint32_t opcode);

#endif
