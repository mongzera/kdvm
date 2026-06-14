#include "eval_control_flow.h"
#include "../../util/types/types.h"



int control_flow_vm_execute(VM* vm, uint32_t opcode){
    switch (opcode) {
        case OP_CMPEQ: {
            PrimitiveValue b = VM_POP(vm);
            PrimitiveValue a = VM_POP(vm);
            PrimitiveValue res = {TYPE_INT, .data.u = primitive_equal(a, b)};
            VM_PUSH(vm, res);
            break;
        }

        case OP_CMPNEQ: {
            PrimitiveValue b = VM_POP(vm);
            PrimitiveValue a = VM_POP(vm);
            PrimitiveValue res = {TYPE_INT, .data.u = primitive_not_equal(a, b)};
            VM_PUSH(vm, res);
            break;
        }

        case OP_CMPLT: {
            PrimitiveValue b = VM_POP(vm);
            PrimitiveValue a = VM_POP(vm);
            PrimitiveValue res = {TYPE_INT, .data.u = primitive_less_than(a, b)};
            VM_PUSH(vm, res);
            break;
        }

        case OP_CMPLE: {
            PrimitiveValue b = VM_POP(vm);
            PrimitiveValue a = VM_POP(vm);
            PrimitiveValue res = {TYPE_INT, .data.u = primitive_less_than_equal(a, b)};
            VM_PUSH(vm, res);
            break;
        }

        case OP_CMPGT: {
            PrimitiveValue b = VM_POP(vm);
            PrimitiveValue a = VM_POP(vm);
            PrimitiveValue res = {TYPE_INT, .data.u = primitive_greater_than(a, b)};
            VM_PUSH(vm, res);
            break;
        }

        case OP_CMPGE: {
            PrimitiveValue b = VM_POP(vm);
            PrimitiveValue a = VM_POP(vm);
            PrimitiveValue res = {TYPE_INT, .data.u = primitive_greater_than_equal(a, b)};
            VM_PUSH(vm, res);
            break;
        }

        case OP_JUMP: vm->pc = vm->program[vm->pc]; break;

        case OP_JNZ: {
            uint32_t target = vm->program[vm->pc++];
            if ((VM_POP(vm)).data.u != 0) vm->pc = target;
            break;
        }

        case OP_JZ: {
            uint32_t target = vm->program[vm->pc++];
            if ((VM_POP(vm)).data.u == 0) vm->pc = target;
            break;
        }

        // Subroutines
        case OP_CALL: {
            uint32_t target = vm->program[vm->pc++];
            // add safe-guards
            if(vm->csp >= CALL_STACK_MEM-1) {
                vm_error("Call stack overflow!");
                exit(-1);
            }

            vm->call_stack[++vm->csp] = vm->pc; // Save return address

            LocalStackFrame *lastFrame = VM_SF_PEEK(vm);
            LocalStackFrame stack_frame = {lastFrame->start + lastFrame->local_variable_count, 0};
            if(vm->sfp >= STACK_RAM_SIZE-1) {vm_error("Stack Frame Overflow!"); exit(-1);};
            VM_SF_PUSH(vm, stack_frame);

            vm->pc = target;                    // Jump
            break;
        }
        case OP_RET: {
            if (vm->csp < 0) { vm_error("Call Stack Underflow!"); return -1; }
            vm->pc = vm->call_stack[vm->csp--]; // Return to caller

            if(vm->sfp < 0) { vm_error("Stack Frame Underflow!"); exit(-1);};
            VM_SF_POP(vm);
            break;
        }
    }

    return EXEC_CONTINUE;
}
