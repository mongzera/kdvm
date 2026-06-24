#ifndef H_MEMORY
#define H_MEMORY
#include "../math/math.h"
#include "../core/kdvm.h"
#include "clz_fallback.h"
#include <stdlib.h>

#define mem_is_out_of_bounds(addr, start, size) ((addr) < (start) || (addr) >= ((start) + (size)))
#define HEAP_BITMASK_LENGTH LOG2_32(HEAP_RAM_SIZE)

typedef struct {
    uint32_t bitmask[HEAP_BITMASK_LENGTH];
} HeapBitMask;

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

typedef struct {
    uint32_t levels[8]; //hard code levels for now, use dynamic level compute later...
} HeapBitmask;

/*
 * Allocates n bytes and returns heap_address
 * @returns heap_address, -1 if error
 */
uint32_t    malloc_heap(VM_Thread* thread, uint32_t size);

/*
 * Frees memory block that start at addr
 */
void        free_heap(VM* vm, uint32_t addr);

uint32_t get_index_level(uint32_t lvl);
uint32_t get_level_from_index(uint32_t idx);
uint32_t get_left_node(uint32_t idx);
uint32_t get_right_node(uint32_t idx);

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
