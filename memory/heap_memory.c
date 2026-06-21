#include "memory.h"

uint32_t get_level_index(uint32_t lvl){
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

uint32_t malloc_heap(VM* vm, uint32_t size){
    return 0;
}

/*
 * Frees memory block that start at addr
 */
void free_heap(VM* vm, uint32_t addr){

}
