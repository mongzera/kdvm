#include "eval_io.h"
#include "../util/types/types.h"

int io_vm_execute(VM *vm, uint32_t opcode){
    switch (opcode) {
        // I/O
        case OP_OUT:  primitive_print(VM_POP(vm)); break;
        case OP_IN: {
            vm_depr("OP_IN is still not implemented!");
            // int val;
            // printf("> ");
            // scanf("%d", &val);
            // VM_PUSH(vm, val);
            break;
        }
    }

    return EXEC_CONTINUE;
}

// TODO: Implement Basic IO using MMIO
