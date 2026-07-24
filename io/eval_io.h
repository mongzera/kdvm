#ifndef H_EVAL_IO
#define H_EVAL_IO

#include "../core/grrvm.h"
#include <stdio.h>

int io_vm_execute(VM* vm, VM_Thread *thread, uint32_t opcode);

#endif
