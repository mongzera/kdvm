#include "memory.h"

HeapBitMask heap_alloc_mask; // 1 = fully allocated
HeapBitMask heap_split_mask; // 1 = split into two smaller buddies

// Clamps HEAP_RAM_SIZE down to the nearest power of 2 (e.g. 3072 -> 2048)
#define BUDDY_ARENA_SIZE (1U << (31 - __builtin_clz(HEAP_RAM_SIZE)))

/* --- Bitset Helpers --- */

static inline bool bit_test(const HeapBitMask* m, uint32_t bit) {
    return (m->bitmask[bit >> 5] & (1U << (bit & 31))) != 0;
}

static inline void bit_set(HeapBitMask* m, uint32_t bit) {
    m->bitmask[bit >> 5] |= (1U << (bit & 31));
}

static inline void bit_clear(HeapBitMask* m, uint32_t bit) {
    m->bitmask[bit >> 5] &= ~(1U << (bit & 31));
}

static inline uint32_t next_pow2(uint32_t v) {
    if (v <= 1) return 1;
    return 1U << (32 - __builtin_clz(v - 1));
}

/* --- Tree Navigation --- */

uint32_t get_index_level(uint32_t lvl) {
    return (1 << lvl) - 1;
}

uint32_t get_level_from_index(uint32_t idx) {
    return 31 - __builtin_clz(idx + 1);
}

uint32_t get_left_node(uint32_t idx) {
    return (idx << 1) + 1;
}

uint32_t get_right_node(uint32_t idx) {
    return (idx << 1) + 2;
}

/* --- Internal Allocator Logic --- */

static uint32_t buddy_find_free(uint32_t idx, uint32_t curr_lvl, uint32_t target_lvl) {
    if (bit_test(&heap_alloc_mask, idx)) return (uint32_t)-1; // Branch fully occupied

    if (curr_lvl == target_lvl) {
        if (bit_test(&heap_split_mask, idx)) return (uint32_t)-1; // Broken into smaller parts
        return idx; // Claimed!
    }

    // We are above the target level. Mark this node as split and descend.
    bit_set(&heap_split_mask, idx);

    uint32_t left = get_left_node(idx);
    uint32_t res = buddy_find_free(left, curr_lvl + 1, target_lvl);
    if (res != (uint32_t)-1) return res;

    uint32_t right = get_right_node(idx);
    return buddy_find_free(right, curr_lvl + 1, target_lvl);
}

static void buddy_mark_allocated(uint32_t idx) {
    bit_set(&heap_alloc_mask, idx);

    // Propagate 'fully allocated' state upward to parents
    while (idx > 0) {
        uint32_t sibling = ((idx - 1) ^ 1) + 1;
        uint32_t parent  = (idx - 1) >> 1;

        if (bit_test(&heap_alloc_mask, sibling)) {
            bit_set(&heap_alloc_mask, parent);
            idx = parent;
        } else {
            break;
        }
    }
}

/* --- Public API --- */

uint32_t malloc_heap(VM_Thread* thread, uint32_t size) {
    if (size == 0 || size > BUDDY_ARENA_SIZE) return (uint32_t)-1;

    uint32_t needed_pow2 = next_pow2(size);

    // Derive target tree depth (Root level 0 = BUDDY_ARENA_SIZE)
    uint32_t target_level = 0;
    uint32_t capacity = BUDDY_ARENA_SIZE;
    while (capacity > needed_pow2) {
        capacity >>= 1;
        target_level++;
    }

    uint32_t node_idx = buddy_find_free(0, 0, target_level);
    if (node_idx == (uint32_t)-1) return (uint32_t)-1; // Out of Memory / Fragmented

    buddy_mark_allocated(node_idx);

    // Translate node index to absolute VM->ram address
    uint32_t lvl_start_idx = get_index_level(target_level);
    uint32_t offset_in_lvl = node_idx - lvl_start_idx;
    uint32_t words_per_block = BUDDY_ARENA_SIZE >> target_level;

    return HEAP_RAM_START + (offset_in_lvl * words_per_block);
}

void free_heap(VM* vm, uint32_t addr) {
    if (addr < HEAP_RAM_START || addr >= HEAP_RAM_START + BUDDY_ARENA_SIZE) {
        vm_error("HeapFree: Pointer out of bounds!");
        return;
    }

    uint32_t rel_addr = addr - HEAP_RAM_START;

    // 1. Walk down the tree to locate the exact node allocated for this pointer
    uint32_t idx = 0;
    uint32_t span = BUDDY_ARENA_SIZE;

    while (1) {
        if (bit_test(&heap_alloc_mask, idx)) break; // Found the exact allocated node!

        if (!bit_test(&heap_split_mask, idx)) {
            vm_error("HeapFree: Double free or invalid pointer!");
            return;
        }

        span >>= 1;
        idx = ((rel_addr & span) == 0) ? get_left_node(idx) : get_right_node(idx);
    }

    // 2. Clear allocation bit & un-propagate parent allocations
    bit_clear(&heap_alloc_mask, idx);
    uint32_t curr = idx;
    while (curr > 0) {
        uint32_t parent = (curr - 1) >> 1;
        if (bit_test(&heap_alloc_mask, parent)) {
            bit_clear(&heap_alloc_mask, parent);
            curr = parent;
        } else {
            break;
        }
    }

    // 3. Coalesce free buddies back up toward the root
    while (idx > 0) {
        uint32_t sibling = ((idx - 1) ^ 1) + 1;
        uint32_t parent  = (idx - 1) >> 1;

        bool sib_is_pure_free = !bit_test(&heap_alloc_mask, sibling) &&
                                !bit_test(&heap_split_mask, sibling);

        if (sib_is_pure_free) {
            bit_clear(&heap_split_mask, parent); // Undo the split
            idx = parent;
        } else {
            break;
        }
    }
}
