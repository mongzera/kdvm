#include "eval_math.h"
#include "../util/types/types.h"

int imath_vm_execute(VM* vm, uint32_t opcode){
    switch (opcode) {
        case OP_ADD: {
            PrimitiveValue b = VM_POP(vm);
            PrimitiveValue a = VM_POP(vm);
            VM_PUSH(vm, primitive_add(a, b));
            break;
        }

        case OP_SUB: {
            PrimitiveValue b = VM_POP(vm);
            PrimitiveValue a = VM_POP(vm);
            VM_PUSH(vm, primitive_sub(a, b));
            break;
        }

        case OP_MUL: {
            PrimitiveValue b = VM_POP(vm);
            PrimitiveValue a = VM_POP(vm);
            VM_PUSH(vm, primitive_mul(a, b));
            break;
        }

        case OP_DIV: {
            PrimitiveValue b = VM_POP(vm);
            PrimitiveValue a = VM_POP(vm);

            VM_PUSH(vm, primitive_div(a, b));
            break;

        }

        case OP_MOD: {
            PrimitiveValue b = VM_POP(vm);
            PrimitiveValue a = VM_POP(vm);

            // Java: integer mod by zero throws exception
            if ((b.type == TYPE_INT && b.data.u == 0) ||
                (b.type == TYPE_CHAR && b.data.c == 0)) {
                vm_error("ArithmeticException: %  by zero");
                return EXEC_ERR;
            }

            if (b.type == TYPE_FLOAT && b.data.f == 0.0f) {
                VM_PUSH(vm, primitive_mod(a, b)); // fmod handles it (NaN)
                break;
            }

            VM_PUSH(vm, primitive_mod(a, b));
            break;
        }

        default:
            return EXEC_ERR;
    }

    return EXEC_CONTINUE;
}

int fmath_vm_execute(VM* vm, uint32_t opcode){
    switch (opcode) {
        // Floating Point
        case OP_FPUSH: vm_depr("OP_FPUSH"); break;
        case OP_FOUT: vm_depr("OP_FOUT"); break;
        case OP_FADD: vm_depr("OP_FADD"); break;
        case OP_FSUB: vm_depr("OP_FSUB"); break;
        case OP_FMUL: vm_depr("OP_FMUL"); break;
        case OP_FDIV: vm_depr("OP_FDIV"); break;
    }

    return EXEC_CONTINUE;
}
