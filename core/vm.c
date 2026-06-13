#include "kdvm.h"
#include <stdio.h>
#include "../math/eval_math.h"
#include "eval/eval_memory.h"
#include "eval/eval_control_flow.h"
#include "../io/eval_io.h"

void vm_init(VM* vm) {
    vm->pc = 0;
    vm->sp = -1;
    vm->csp = -1;
    vm->program_size = 0;
}

int vm_execute(VM* vm) {
    vm->pc = vm->_global_start;
    while (1) {

        uint32_t opcode = vm->program[vm->pc++];
        uint32_t opcode_type = opcode & 0xF0;

        int exec_status = EXEC_NO_ERR;

        //printf("[INS EXEC LINE: %d]: 0x%X\n", vm->pc, opcode);

        switch(opcode_type){
            case OPT_MEMSTACK:  exec_status = stack_vm_execute(vm, opcode); break;
            case OPT_IMATH:     exec_status = imath_vm_execute(vm, opcode); break;
            case OPT_FMATH:     exec_status = fmath_vm_execute(vm, opcode); break;
            case OPT_MEM:       exec_status = mem_vm_execute(vm, opcode); break;
            case OPT_IO:        exec_status = io_vm_execute(vm, opcode); break;
            case OPT_CONTROL:   exec_status = control_flow_vm_execute(vm, opcode); break;
            default:
                printf("[ERROR] Unknown Opcode at PC %d: %d\n", vm->pc - 1, vm->program[vm->pc-1]);
                exec_status = EXEC_ERR;
        }

        if(exec_status <= 0) return exec_status;

    }
}
