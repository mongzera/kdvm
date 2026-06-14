#ifndef H_MEMORY
#define H_MEMORY

#define mem_is_out_of_bounds(addr, start, size) ((addr) < (start) || (addr) >= ((start) + (size)))

#include "../core/kdvm.h"
#include <stdlib.h>

/*
 * Allocates n bytes and returns heap_address
 * @returns heap_address, -1 if error
 */
uint32_t        malloc_heap(VM* vm, uint32_t size);

/*
 * Frees memory block that start at addr
 */
void            free_heap(VM* vm, uint32_t addr);

static inline uint32_t store_stack(VM* vm, uint32_t fp, uint32_t offset, PrimitiveValue value) {
    if(!mem_is_out_of_bounds(fp + offset, STACK_RAM_START, STACK_RAM_SIZE)) {
        vm_error("SegFault: Stack out of bounds!");
        exit(-1);
    }
    vm->ram[STACK_RAM_START + fp + offset] = value;
    return 0;
}

static inline PrimitiveValue load_stack(VM* vm, uint32_t fp, uint32_t offset) {
    if(!mem_is_out_of_bounds(fp + offset, STACK_RAM_START, STACK_RAM_SIZE)) {
        vm_error("SegFault: Stack out of bounds!");
        exit(-1);
    }
    return vm->ram[STACK_RAM_START + fp + offset];
}

#endif
