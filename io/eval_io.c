#include "eval_io.h"

int io_vm_execute(VM *vm, uint32_t opcode){
    switch (opcode) {
        // I/O
        case OP_OUT:  printf("%d\n", VM_POP(vm)); break;
        case OP_IN: {
            int val;
            printf("> ");
            scanf("%d", &val);
            VM_PUSH(vm, val);
            break;
        }
    }

    return EXEC_CONTINUE;
}

// TODO: Implement Basic IO using MMIO
