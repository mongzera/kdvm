#include "eval_math.h"
#include "../util/types/types.h"

int imath_vm_execute(VM* vm, VM_Thread *thread, uint32_t opcode){
    switch (opcode) {
        case OP_ADD: {
            PrimitiveValue b = VM_THREAD_POP(thread);
            PrimitiveValue a = VM_THREAD_POP(thread);
            VM_THREAD_PUSH(thread, primitive_add(a, b));
            break;
        }

        case OP_SUB: {
            PrimitiveValue b = VM_THREAD_POP(thread);
            PrimitiveValue a = VM_THREAD_POP(thread);
            VM_THREAD_PUSH(thread, primitive_sub(a, b));
            break;
        }

        case OP_MUL: {
            PrimitiveValue b = VM_THREAD_POP(thread);
            PrimitiveValue a = VM_THREAD_POP(thread);
            VM_THREAD_PUSH(thread, primitive_mul(a, b));
            break;
        }

        case OP_DIV: {
            PrimitiveValue b = VM_THREAD_POP(thread);
            PrimitiveValue a = VM_THREAD_POP(thread);

            VM_THREAD_PUSH(thread, primitive_div(a, b));
            break;

        }

        case OP_MOD: {
            PrimitiveValue b = VM_THREAD_POP(thread);
            PrimitiveValue a = VM_THREAD_POP(thread);

            // Java: integer mod by zero throws exception
            if ((b.type == TYPE_INT && b.data.u == 0) ||
                (b.type == TYPE_CHAR && b.data.c == 0)) {
                vm_error("ArithmeticException: %  by zero");
                return EXEC_ERR;
            }

            if (b.type == TYPE_FLOAT && b.data.f == 0.0f) {
                VM_THREAD_PUSH(thread, primitive_mod(a, b)); // fmod handles it (NaN)
                break;
            }

            VM_THREAD_PUSH(thread, primitive_mod(a, b));
            break;
        }

        default:
            return EXEC_ERR;
    }

    return EXEC_CONTINUE;
}
