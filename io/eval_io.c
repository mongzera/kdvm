#include "eval_io.h"
#include "../util/types/types.h"

int io_vm_execute(VM *vm, VM_Thread *thread, uint32_t opcode){
    switch (opcode) {
        // I/O
        case OP_OUT:  primitive_print(VM_THREAD_POP(thread)); break;
        case OP_IN: {
            // Read the type from the next instruction slot
            uint16_t type = (uint16_t) VM_GET_INSTRUCTION(vm, thread->pc);
            PrimitiveValue val;
            val.type = type;

            printf("> ");
            fflush(stdout);

            // Using the 16-bit type ID
            switch (val.type) {
                case TYPE_INT:   scanf("%d", &val.data.u); break;
                case TYPE_FLOAT: scanf("%f", &val.data.f); break;
                case TYPE_CHAR:  scanf(" %c", &val.data.c); break;
                case TYPE_BYTE: {
                    int temp;
                    scanf("%i", &temp);
                    val.data.b = (int8_t)temp;
                    break;
                }
                default:
                    printf("[VM ERROR] Unknown input type code: %d", val.type);
                    break;
            }

            // Clear buffer to prevent input contamination
            int c; while ((c = getchar()) != '\n' && c != EOF);

            VM_THREAD_PUSH(thread, val);
            break;
        }
    }

    return EXEC_CONTINUE;
}

// TODO: Implement Basic IO using MMIO
