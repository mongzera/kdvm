#include "eval_math.h"

int imath_vm_execute(VM* vm, uint32_t opcode){
    switch (opcode) {
        // Integer Math
        case OP_ADD: { uint32_t b = vm->stack[vm->sp--]; vm->stack[vm->sp] += b; break; }
        case OP_SUB: { uint32_t b = vm->stack[vm->sp--]; vm->stack[vm->sp] -= b; break; }
        case OP_MUL: { uint32_t b = vm->stack[vm->sp--]; vm->stack[vm->sp] *= b; break; }
        case OP_DIV: {
            uint32_t b = vm->stack[vm->sp--];
            if (b == 0) { vm_error("Divide by Zero"); return EXEC_ERR; }
            vm->stack[vm->sp] /= b;
            break;
        }
    }

    return EXEC_CONTINUE;
}

int fmath_vm_execute(VM* vm, uint32_t opcode){
    switch (opcode) {
        // Floating Point
        case OP_FPUSH: vm->stack[++vm->sp] = vm->program[vm->pc++]; break;
        case OP_FOUT: {
            FloatCast c; c.u = vm->stack[vm->sp--];
            printf("%f\n", c.f);
            break;
        }
        case OP_FADD: { FloatCast b={.u=vm->stack[vm->sp--]}, a={.u=vm->stack[vm->sp]}; a.f += b.f; vm->stack[vm->sp] = a.u; break; }
        case OP_FSUB: { FloatCast b={.u=vm->stack[vm->sp--]}, a={.u=vm->stack[vm->sp]}; a.f -= b.f; vm->stack[vm->sp] = a.u; break; }
        case OP_FMUL: { FloatCast b={.u=vm->stack[vm->sp--]}, a={.u=vm->stack[vm->sp]}; a.f *= b.f; vm->stack[vm->sp] = a.u; break; }
        case OP_FDIV: { FloatCast b={.u=vm->stack[vm->sp--]}, a={.u=vm->stack[vm->sp]}; a.f /= b.f; vm->stack[vm->sp] = a.u; break; }
    }

    return EXEC_CONTINUE;
}
