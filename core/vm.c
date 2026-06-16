#include "kdvm.h"
#include <stdio.h>
#include <stdlib.h>
#include "../math/eval_math.h"
#include "eval/eval_memory.h"
#include "eval/eval_control_flow.h"
#include "../io/eval_io.h"

void vm_init(VM* vm){
    vm->program_size = 0;
    *vm->threads = (VM_Thread*)malloc(sizeof(VM_Thread) * MAX_THREADS);

    if(*vm->threads == NULL) vm_error("Cannot allocate threads!"); exit(-1);

    // initialize threads since they have garbage shiz
    for(int i = 0; i < MAX_THREADS; i++){
        VM_Thread *thread = vm->threads[i];
        vm_init_thread(vm, thread);
    }
}

int vm_execute(VM* vm) {
    //vm->pc = vm->_global_start;

    // // push initial stack frame for entry
    // LocalStackFrame stack_frame = {0, 0};
    // VM_SF_PUSH(vm, stack_frame);

    // create main thread
    VM_Thread *main_thread = vm_request_thread(vm);

    while (1) {

        for(int i = 0; i < MAX_THREADS; i++){
            VM_Thread *thread = vm->threads[i];

            if(thread == NULL) continue;
            if(thread->status == THREAD_YIELD) continue;

            uint32_t opcode = vm->program[thread->pc++];
            uint32_t opcode_type = opcode & 0xF0;

            int exec_status = EXEC_NO_ERR;
            //printf("[SP]: %d\n", vm->sp);

            switch(opcode_type){
                case OPT_MEMSTACK:  exec_status = stack_vm_execute(vm, opcode); break;
                case OPT_IMATH:     exec_status = imath_vm_execute(vm, opcode); break;
                case OPT_MEM:       exec_status = mem_vm_execute(vm, opcode); break;
                case OPT_IO:        exec_status = io_vm_execute(vm, opcode); break;
                case OPT_CONTROL:   exec_status = control_flow_vm_execute(vm, opcode); break;
                default:
                    printf("[ERROR] Unknown Opcode at PC %d: %d\n", thread->pc - 1, vm->program[thread->pc-1]);
                    exec_status = EXEC_ERR;
            }

            if(exec_status <= 0) return exec_status;
        }

    }
}
