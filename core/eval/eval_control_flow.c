#include "eval_control_flow.h"


int control_flow_vm_execute(VM* vm, uint32_t opcode){
    switch (opcode) {
        // Comparisons & Jumps
        case OP_CMPEQ: {
            uint32_t b = vm->stack[vm->sp--];
            uint32_t a = vm->stack[vm->sp--];
            vm->stack[++vm->sp] = (a == b) ? 1 : 0;
            break;
        }
        case OP_CMPLT: {
            int32_t b = (int32_t)vm->stack[vm->sp--];
            int32_t a = (int32_t)vm->stack[vm->sp--];
            vm->stack[++vm->sp] = (a < b) ? 1 : 0;
            break;
        }
        case OP_JUMP: vm->pc = vm->program[vm->pc]; break;
        case OP_JIF: {
            uint32_t target = vm->program[vm->pc++];
            if (vm->stack[vm->sp--] != 0) vm->pc = target;
            break;
        }

        // Subroutines
        case OP_CALL: {
            uint32_t target = vm->program[vm->pc++];
            vm->call_stack[++vm->csp] = vm->pc; // Save return address
            vm->pc = target;                    // Jump
            break;
        }
        case OP_RET: {
            if (vm->csp < 0) { vm_error("Call Stack Underflow!"); return -1; }
            vm->pc = vm->call_stack[vm->csp--]; // Return to caller
            break;
        }
    }

    return EXEC_CONTINUE;
}
