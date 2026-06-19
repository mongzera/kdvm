#include "memory.h"
#include <stdlib.h>


HeapBlock *genesis_block = 0;

HeapBlock* create_block(uint32_t offset, uint32_t size, PrimitiveType type){
    HeapBlock *block =  malloc(sizeof(HeapBlock));
    block->type = type;
    block->block_status = BLOCK_INUSE;
    block->address = offset;
    block->size = size;
    block->next = 0;

    return block;
}

void destory_blocks(uint32_t address){

}

uint32_t malloc_heap(VM* vm, uint32_t size){

}

/*
 * Frees memory block that start at addr
 */
void free_heap(VM* vm, uint32_t addr){

}
