#ifndef H_EVAL_CONTROL_FLOW
#define H_EVAL_CONTROL_FLOW

#include "../kdvm.h"

int control_flow_vm_execute(VM* vm, VM_Thread *thread, uint32_t opcode);

#endif
