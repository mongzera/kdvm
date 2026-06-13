#include "eval_syscall.h"

int exec_vm_syscall(VM* vm, int opcode){
    switch (opcode) {
        case OP_SYS_READ: {
            // todo implement
            break;
        }
    }

    return EXEC_CONTINUE;
}
