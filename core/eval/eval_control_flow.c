#include "eval_control_flow.h"
#include "../../util/types/types.h"

int control_flow_vm_execute(VM* vm, VM_Thread *thread, uint32_t opcode){
    switch (opcode) {
        case OP_CMPEQ: {
            PrimitiveValue b = VM_THREAD_POP(thread);
            PrimitiveValue a = VM_THREAD_POP(thread);
            PrimitiveValue res = {TYPE_INT, .data.u = primitive_equal(a, b)};
            VM_THREAD_PUSH(thread, res);
            break;
        }

        case OP_CMPNEQ: {
            PrimitiveValue b = VM_THREAD_POP(thread);
            PrimitiveValue a = VM_THREAD_POP(thread);
            PrimitiveValue res = {TYPE_INT, .data.u = primitive_not_equal(a, b)};
            VM_THREAD_PUSH(thread, res);
            break;
        }

        case OP_CMPLT: {
            PrimitiveValue b = VM_THREAD_POP(thread);
            PrimitiveValue a = VM_THREAD_POP(thread);
            PrimitiveValue res = {TYPE_INT, .data.u = primitive_less_than(a, b)};
            VM_THREAD_PUSH(thread, res);
            break;
        }

        case OP_CMPLE: {
            PrimitiveValue b = VM_THREAD_POP(thread);
            PrimitiveValue a = VM_THREAD_POP(thread);
            PrimitiveValue res = {TYPE_INT, .data.u = primitive_less_than_equal(a, b)};
            VM_THREAD_PUSH(thread, res);
            break;
        }

        case OP_CMPGT: {
            PrimitiveValue b = VM_THREAD_POP(thread);
            PrimitiveValue a = VM_THREAD_POP(thread);
            PrimitiveValue res = {TYPE_INT, .data.u = primitive_greater_than(a, b)};
            VM_THREAD_PUSH(thread, res);
            break;
        }

        case OP_CMPGE: {
            PrimitiveValue b = VM_THREAD_POP(thread);
            PrimitiveValue a = VM_THREAD_POP(thread);
            PrimitiveValue res = {TYPE_INT, .data.u = primitive_greater_than_equal(a, b)};
            VM_THREAD_PUSH(thread, res);
            break;
        }

        case OP_JUMP: thread->pc = VM_GET_INSTRUCTION(vm, thread->pc); break;

        case OP_JNZ: {
            uint32_t target = VM_GET_INSTRUCTION(vm, thread->pc);
            if ((VM_THREAD_POP(thread)).data.u != 0) thread->pc = target;
            break;
        }

        case OP_JZ: {
            uint32_t target = VM_GET_INSTRUCTION(vm, thread->pc);
            if ((VM_THREAD_POP(thread)).data.u == 0) thread->pc = target;
            break;
        }

        // Subroutines
        case OP_CALL: {
            uint32_t target = VM_GET_INSTRUCTION(vm, thread->pc);
            // add safe-guards
            if(thread->csp >= CALL_STACK_MEM-1) {
                vm_error("Call stack overflow!");
                exit(-1);
            }

            thread->call_stack[++thread->csp] = thread->pc; // Save return address, for the call-stack of mahuman na ang function.

            LocalStackFrame *lastFrame = VM_THREAD_SF_PEEK(thread);
            LocalStackFrame stack_frame = {lastFrame->start + lastFrame->local_variable_count, 0};
            if(thread->sfp >= THREAD_STACK_SIZE-1) {vm_error("Stack Frame Overflow!"); exit(-1);};
            VM_THREAD_SF_PUSH(thread, stack_frame);

            thread->pc = target; // Change program counter kay muambak na
            break;
        }
        case OP_RET: {
            if (thread->csp < 0) { vm_error("Call Stack Underflow!"); return -1; }
            thread->pc = thread->call_stack[thread->csp--]; // Return to caller

            if(thread->sfp < 0) { vm_error("Stack Frame Underflow!"); exit(-1);};
            VM_THREAD_SF_POP(thread);
            break;
        }
    }

    return EXEC_CONTINUE;
}
