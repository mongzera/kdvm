#ifndef H_MEMORY
#define H_MEMORY

#include <stdio.h>
#define mem_is_out_of_bounds(addr, start, size) ((addr) < (start) || (addr) >= ((start) + (size)))

#include "../core/kdvm.h"
#include <stdlib.h>

typedef enum HeapBlockStats{
    BLOCK_FREE = 0x0,
    BLOCK_INUSE
};

typedef struct HeapBlock{
    uint32_t address;
    uint32_t size;
    uint8_t block_status;
    PrimitiveType type;
    struct HeapBlock *next;
} HeapBlock;

HeapBlock *create_block(uint32_t offset, uint32_t size, PrimitiveType type);

static inline uint32_t store_global(VM* vm, uint32_t address, PrimitiveValue value) {

    if(mem_is_out_of_bounds(address, GLOBAL_RAM_START, GLOBAL_RAM_SIZE)) {
        vm_error("SegFault: Global store out of bounds!");
        exit(-1);
    }
    vm->ram[address] = value;
    return 0;
}

static inline PrimitiveValue load_global(VM* vm, uint32_t address) {

    if(mem_is_out_of_bounds(address, GLOBAL_RAM_START, GLOBAL_RAM_SIZE)) {
        vm_error("SegFault: Global load out of bounds!");
        exit(-1);
    }
    return vm->ram[address];
}

/*
 * Allocates n bytes and returns heap_address
 * @returns heap_address, -1 if error
 */
uint32_t    malloc_heap(VM* vm, uint32_t size);

/*
 * Frees memory block that start at addr
 */
void        free_heap(VM* vm, uint32_t addr);

static inline uint32_t store_stack(VM_Thread* thread, uint32_t fp, uint32_t offset, PrimitiveValue value) {
    uint32_t address = fp + offset;

    if(mem_is_out_of_bounds(address, 0, THREAD_STACK_SIZE)) {
        vm_error("SegFault: Stack store out of bounds!");
        exit(-1);
    }
    thread->ram_stack[address] = value;
    return 0;
}

static inline PrimitiveValue load_stack(VM_Thread* thread, uint32_t fp, uint32_t offset) {
    uint32_t address = fp + offset;
    if(mem_is_out_of_bounds(address, 0, THREAD_STACK_SIZE)) {
        vm_error("SegFault: Stack load out of bounds!");
        exit(-1);
    }
    return thread->ram_stack[address];
}

#endif
