#include "memory.h"

// We use the first slot of every allocation as a "metadata header".
// No struct needed, just utilizing the PrimitiveValue's fields.
#define HEADER_SLOTS 1

HeapBitMask heap_alloc_mask;
HeapBitMask heap_split_mask;

// BUDDY_ARENA_SIZE now represents the total number of PrimitiveValue slots available
#define BUDDY_ARENA_SIZE (1U << (31 - __builtin_clz(HEAP_RAM_SIZE)))

/* --- Bitset Helpers (Unchanged) --- */
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

/* --- Tree Navigation (Unchanged) --- */
uint32_t get_index_level(uint32_t lvl) { return (1 << lvl) - 1; }
uint32_t get_left_node(uint32_t idx)   { return (idx << 1) + 1; }
uint32_t get_right_node(uint32_t idx)  { return (idx << 1) + 2; }

/* --- Internal Allocator Logic --- */
static uint32_t buddy_find_free(uint32_t idx, uint32_t curr_lvl, uint32_t target_lvl) {
    if (bit_test(&heap_alloc_mask, idx)) return (uint32_t)-1;

    if (curr_lvl == target_lvl) {
        if (bit_test(&heap_split_mask, idx)) return (uint32_t)-1;
        return idx;
    }

    bit_set(&heap_split_mask, idx);

    uint32_t left = get_left_node(idx);
    uint32_t res = buddy_find_free(left, curr_lvl + 1, target_lvl);
    if (res != (uint32_t)-1) return res;

    uint32_t right = get_right_node(idx);
    return buddy_find_free(right, curr_lvl + 1, target_lvl);
}

static void buddy_mark_allocated(uint32_t idx) {
    bit_set(&heap_alloc_mask, idx);
    while (idx > 0) {
        uint32_t sibling = ((idx - 1) ^ 1) + 1;
        uint32_t parent  = (idx - 1) >> 1;
        if (bit_test(&heap_alloc_mask, sibling)) {
            bit_set(&heap_alloc_mask, parent);
            idx = parent;
        } else { break; }
    }
}

/* --- Public API --- */

/**
 * Allocates 'num_slots' in the shared heap.
 * Ownership: The allocation is associated with the calling thread's VM context.
 */
uint32_t malloc_heap(VM_Thread* thread, uint32_t num_slots) {
    VM* vm = thread->vm; // Access global VM via the thread context

    // 1. Reserve 1 slot for header (Level/Metadata)
    uint32_t total_slots = num_slots + HEADER_SLOTS;
    if (total_slots == 0 || total_slots > BUDDY_ARENA_SIZE) return (uint32_t)-1;

    uint32_t needed_pow2 = next_pow2(total_slots);

    // 2. Derive target tree depth
    uint32_t target_level = 0;
    uint32_t capacity = BUDDY_ARENA_SIZE;
    while (capacity > needed_pow2) {
        capacity >>= 1;
        target_level++;
    }

    // 3. Search for free block in shared masks
    uint32_t node_idx = buddy_find_free(0, 0, target_level);
    if (node_idx == (uint32_t)-1) return (uint32_t)-1;

    buddy_mark_allocated(node_idx);

    // 4. Calculate starting array index (PrimitiveValue slot index)
    uint32_t lvl_start_idx = get_index_level(target_level);
    uint32_t offset_in_lvl = node_idx - lvl_start_idx;
    uint32_t slots_per_block = BUDDY_ARENA_SIZE >> target_level;
    uint32_t start_slot = HEAP_RAM_START + (offset_in_lvl * slots_per_block);

    // 5. Initialize the Header
    // We treat the first slot as metadata.
    vm->ram[start_slot].type = TYPE_INT;
    vm->ram[start_slot].data.u = (int32_t)target_level;
    vm->ram[start_slot].word_state = WORD_CONSTANT; // Read-only for safety

    // 6. Return the index of the DATA slot (the VM handles the abstraction)
    return start_slot + 1;
}

/**
 * Frees memory.
 * Ownership: Thread provides context, but the heap is global.
 */
void free_heap(VM* vm, uint32_t data_slot) {
    uint32_t header_slot = data_slot - 1;

    // Boundary check
    if (header_slot < HEAP_RAM_START || header_slot >= HEAP_RAM_START + BUDDY_ARENA_SIZE) {
        vm_error("HeapFree: Pointer out of bounds!");
        return;
    }

    // 1. Recover Metadata
    int32_t target_level = vm->ram[header_slot].data.u;

    // Safety check: Prevents using corrupted metadata to wreak havoc on bitmasks
    if (target_level < 0 || target_level > 31) {
        vm_error("HeapFree: Metadata corruption or invalid target level!");
        return;
    }

    // 2. Mathematical index recovery
    uint32_t rel_addr = header_slot - HEAP_RAM_START;
    uint32_t slots_per_block = BUDDY_ARENA_SIZE >> target_level;
    uint32_t offset_in_lvl = rel_addr / slots_per_block;
    uint32_t idx = get_index_level(target_level) + offset_in_lvl;

    // 3. Clear bitmask (De-allocate)
    bit_clear(&heap_alloc_mask, idx);

    // 4. Upward propagation (Clear parents if siblings are free)
    uint32_t curr = idx;
    while (curr > 0) {
        uint32_t parent = (curr - 1) >> 1;
        if (bit_test(&heap_alloc_mask, parent)) {
            bit_clear(&heap_alloc_mask, parent);
            curr = parent;
        } else { break; }
    }

    // 5. Coalesce buddies
    while (idx > 0) {
        uint32_t sibling = ((idx - 1) ^ 1) + 1;
        uint32_t parent  = (idx - 1) >> 1;

        // Check if buddy is truly free
        bool sib_is_pure_free = !bit_test(&heap_alloc_mask, sibling) &&
                                !bit_test(&heap_split_mask, sibling);

        if (sib_is_pure_free) {
            bit_clear(&heap_split_mask, parent);
            idx = parent;
        } else { break; }
    }
}
