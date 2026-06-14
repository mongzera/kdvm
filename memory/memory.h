#ifndef H_MEMORY
#define H_MEMORY

#include <stdio.h>
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
    uint32_t address = STACK_RAM_START + fp + offset;

    if(mem_is_out_of_bounds(address, STACK_RAM_START, STACK_RAM_SIZE)) {
        vm_error("SegFault: Stack store out of bounds!");
        exit(-1);
    }
    vm->ram[address] = value;
    return 0;
}

static inline PrimitiveValue load_stack(VM* vm, uint32_t fp, uint32_t offset) {
    uint32_t address = STACK_RAM_START + fp + offset;
    if(mem_is_out_of_bounds(address, STACK_RAM_START, STACK_RAM_SIZE)) {
        vm_error("SegFault: Stack load out of bounds!");
        exit(-1);
    }
    return vm->ram[address];
}

#endif
