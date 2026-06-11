#include "kdvm.h"
#include <stdio.h>

void vm_init(VM* vm) {
    vm->pc = 0;
    vm->sp = -1;
    vm->csp = -1;
    vm->program_size = 0;
}

int vm_execute(VM* vm) {
    while (1) {
        uint32_t opcode = vm->program[vm->pc++];

        switch (opcode) {
            case OP_HALT: return 0;
            case OP_PUSH: vm->stack[++vm->sp] = vm->program[vm->pc++]; break;
            case OP_POP:  vm->sp--; break;

            // Integer Math
            case OP_ADD: { uint32_t b = vm->stack[vm->sp--]; vm->stack[vm->sp] += b; break; }
            case OP_SUB: { uint32_t b = vm->stack[vm->sp--]; vm->stack[vm->sp] -= b; break; }
            case OP_MUL: { uint32_t b = vm->stack[vm->sp--]; vm->stack[vm->sp] *= b; break; }
            case OP_DIV: {
                uint32_t b = vm->stack[vm->sp--];
                if (b == 0) { vm_error("Divide by Zero"); return -1; }
                vm->stack[vm->sp] /= b;
                break;
            }

            // Memory
            case OP_STORE: {
                uint32_t val = vm->stack[vm->sp--];
                uint32_t addr = vm->stack[vm->sp--];
                vm->ram[addr] = val; break;
            }
            case OP_LOAD: {
                uint32_t addr = vm->stack[vm->sp--];
                vm->stack[++vm->sp] = vm->ram[addr]; break;
            }

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

            // I/O
            case OP_OUT:  printf("%d\n", (int32_t)vm->stack[vm->sp--]); break;
            case OP_IN: {
                int val;
                printf("> ");
                scanf("%d", &val);
                vm->stack[++vm->sp] = (uint32_t)val;
                break;
            }

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

            default:
                printf("[ERROR] Unknown Opcode at PC %d\n", vm->pc - 1);
                return -1;
        }
    }
}
