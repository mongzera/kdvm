#include "kdvm.h"
#include <stdio.h>

VM_Thread *vm_request_thread(VM* vm) {

    VM_Thread *thread = NULL;

    // find available thread
    for(int i = 0; i < MAX_THREADS; i++){
        VM_Thread *th = vm->threads[i];

        // we expect no threads are null at this point, since the first block is initialized.
        // if there is, then something is wrong with the vm_init_thread.

        if(th->status == THREAD_FREE) {
            thread = th;
            break;
        }
    }

    if(thread == NULL) {vm_error("Cannot request thread, all are in-use."); exit(-1);}

    // if no more threads are available, request new block with double the MAX_THREADS, if cannot, then wala eh.
    // but for now, we focus on pool.

    vm_init_thread(vm, thread);

    // set the thread to active
    thread->status = THREAD_ACTIVE;
    LocalStackFrame stack_frame = {0, 0};
    VM_THREAD_SF_PUSH(thread, stack_frame);
    printf("[THREAD] Requested Thread.\n");
    return thread;
}


VM_Thread *vm_get_thread(VM* vm, int thread_id){
    if(thread_id < 0 || thread_id >= MAX_THREADS) {
        printf("[THREAD ERROR] Cannot allocate thread ID: %d [MAX_THREADS: %d]", thread_id, MAX_THREADS);
        return NULL;
    }

    return vm->threads[thread_id];
}


void vm_init_thread(VM* vm, VM_Thread *thread){
    if(thread == NULL) {vm_error("Cannot initialize thread! Thread is null."); exit(-1);}
    thread->pc = vm->_global_start;
    thread->sp = -1;
    thread->csp = -1;
    thread->sfp = -1;
    thread->vm = (VM*) vm;
    thread->status = THREAD_FREE;
}
