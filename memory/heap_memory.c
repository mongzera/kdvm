#include "memory.h"

HeapBitMask heap_alloc_mask;
HeapBitMask heap_split_mask;

uint32_t get_index_level(uint32_t lvl){
    return (1 << lvl) - 1;
}

uint32_t get_level_from_index(uint32_t idx) {
    uint32_t val = idx + 1;
    return 31 - __builtin_clz(val);
}

uint32_t get_left_node(uint32_t idx){
    return (idx << 1) + 1;
}

uint32_t get_right_node(uint32_t idx){
    return (idx << 1) + 2;
}

uint32_t malloc_heap(VM_Thread* thread, uint32_t size){
    uint32_t remaining_size = size;

    uint32_t block_size = HEAP_RAM_SIZE;

    while(remaining_size > 0){
        // TODO: Implement
    }


    return 0;
}

/*
 * Frees memory block that start at addr
 */
void free_heap(VM* vm, uint32_t addr){

}
