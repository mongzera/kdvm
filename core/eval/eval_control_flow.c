#include "eval_control_flow.h"
#include "../../util/types/types.h"

int control_flow_vm_execute(VM* vm, uint32_t opcode){
    switch (opcode) {
        // Comparisons & Jumps
        case OP_CMPEQ: {
            PrimitiveValue b = VM_POP(vm);
            PrimitiveValue a = VM_POP(vm);

            PrimitiveValue result;
            result.type = TYPE_INT;
            result.data.u = primitive_equal(a, b);

            break;
        }

        case OP_CMPNEQ: {
            PrimitiveValue b = VM_POP(vm);
            PrimitiveValue a = VM_POP(vm);
            vm->stack[++vm->sp] = (a != b) ? 1 : 0;
            break;
        }

        case OP_CMPLT: {
            PrimitiveValue b = VM_POP(vm);
            PrimitiveValue a = VM_POP(vm);
            vm->stack[++vm->sp] = (a < b) ? 1 : 0;
            break;
        }


        case OP_CMPLE: {
            PrimitiveValue b = VM_POP(vm);
            PrimitiveValue a = VM_POP(vm);
            vm->stack[++vm->sp] = (a <= b) ? 1 : 0;
            break;
        }

        case OP_CMPGT: {
            PrimitiveValue b = VM_POP(vm);
            PrimitiveValue a = VM_POP(vm);
            vm->stack[++vm->sp] = (a > b) ? 1 : 0;
            break;
        }
        case OP_CMPGE: {
            PrimitiveValue b = VM_POP(vm);
            PrimitiveValue a = VM_POP(vm);
            vm->stack[++vm->sp] = (a >= b) ? 1 : 0;
            break;
        }

        case OP_JUMP: vm->pc = vm->program[vm->pc]; break;

        case OP_JNZ: {
            uint32_t target = vm->program[vm->pc++];
            if ((VM_POP(vm)).u != 0) vm->pc = target;
            break;
        }

        case OP_JZ: {
            uint32_t target = vm->program[vm->pc++];
            if ((VM_POP(vm)).u == 0) vm->pc = target;
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
