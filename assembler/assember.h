#ifndef H_ASSEMBLER
#define H_ASSEMBLER

#include "../core/kdvm.h"
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "../util/hashmap/hashmap.h"

struct subroutine_local_var{
    const char* name;
    int offset;
};

struct subroutine_ctx{
    const char* name;
    int program_line;

    struct hashmap *local_var_map;
    int local_var_count;
};

uint64_t subroutine_hash(const void* item, uint64_t seed0, uint64_t seed1);
int subroutine_cmp(const void* a, const void* b, void *udata);

uint64_t local_var_hash(const void* item, uint64_t seed0, uint64_t seed1);
int local_var_cmp(const void* a, const void* b, void *udata);
uint32_t get_create_local_var(struct subroutine_ctx* subroutine, const char* var_name);

int load_kdm_file(const char* filename, VM* vm);

#endif
