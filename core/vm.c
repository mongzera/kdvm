#include "kdvm.h"
#include <stdio.h>
#include <stdlib.h>
#include "../math/eval_math.h"
#include "eval/eval_memory.h"
#include "eval/eval_control_flow.h"
#include "../io/eval_io.h"

void vm_init(VM* vm){
    vm->program_size = 0;

    bool thread_init_success = true;
    for(int i = 0; i < MAX_THREADS; i++){
        vm->threads[i] = (VM_Thread*) malloc(sizeof(VM_Thread));

        if(vm->threads[i] == NULL) {

            thread_init_success = false;
            continue;
        }

        vm_init_thread(vm, vm->threads[i]);
    }

    if(!thread_init_success) {
        vm_error("Cannot allocate threads!");
        exit(-1);
    }
}

int vm_execute(VM* vm) {
    // create main thread
    vm_request_thread(vm);

    int exec_status = EXEC_CONTINUE;
    while (1) {
        int free_threads = 0;
        for(int i = 0; i < MAX_THREADS; i++){
            VM_Thread *thread = vm->threads[i];

            if(thread == NULL) continue;
            if(thread->status == THREAD_YIELD || thread->status == THREAD_IO_BLOCKED) continue;
            if(thread->status == THREAD_FREE){
                free_threads++;
                continue;
            }

            uint32_t opcode = vm->program[thread->pc++];
            uint32_t opcode_type = opcode & 0xF0;

            switch(opcode_type){
                case OPT_MEMSTACK:  exec_status = stack_vm_execute(vm, thread, opcode); break;
                case OPT_IMATH:     exec_status = imath_vm_execute(vm, thread, opcode); break;
                case OPT_MEM:       exec_status = mem_vm_execute(vm,thread, opcode); break;
                case OPT_IO:        exec_status = io_vm_execute(vm, thread, opcode); break;
                case OPT_CONTROL:   exec_status = control_flow_vm_execute(vm, thread, opcode); break;
                default:
                    printf("[ERROR] Unknown Opcode at PC %d: %d\n", thread->pc - 1, vm->program[thread->pc-1]);
                    exec_status = EXEC_ERR;
            }

            if(exec_status <= 0) {
                thread->status = THREAD_FREE;
            }
        }

        // if all threads are free, end VM.
        if(free_threads >= MAX_THREADS){
            exec_status = EXEC_ERR;
            break;
        }

    }

    return exec_status;
}
