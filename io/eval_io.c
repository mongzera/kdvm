#include "eval_io.h"

int io_vm_execute(VM *vm, uint32_t opcode){
    switch (opcode) {
        // I/O
        case OP_OUT:  printf("%d\n", (int32_t)vm->stack[vm->sp--]); break;
        case OP_IN: {
            int val;
            printf("> ");
            scanf("%d", &val);
            vm->stack[++vm->sp] = (uint32_t)val;
            break;
        }
    }

    return EXEC_CONTINUE;
}
