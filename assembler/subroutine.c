#include "assember.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

uint64_t subroutine_hash(const void* item, uint64_t seed0, uint64_t seed1){
    const struct subroutine_ctx *subroutine = item;

    const char* str_routine_name = subroutine->name;
    return hashmap_sip(str_routine_name, strlen(str_routine_name), seed0, seed1);
}

int subroutine_cmp(const void* a, const void* b, void *udata){

    const struct subroutine_ctx * item_a = a;
    const struct subroutine_ctx * item_b = b;

    const char* str_a = item_a->name;
    const char* str_b = item_b->name;
    return strcmp(str_a, str_b);
}

uint64_t local_var_hash(const void* item, uint64_t seed0, uint64_t seed1){
    const struct subroutine_local_var *local_var = item;

    const char* str_local_var_name = local_var->name;
    return hashmap_sip(str_local_var_name, strlen(str_local_var_name), seed0, seed1);
}

int local_var_cmp(const void* a, const void* b, void *udata){

    const struct subroutine_local_var *item_a = a;
    const struct subroutine_local_var *item_b = b;

    const char* str_a = item_a->name;
    const char* str_b = item_b->name;
    return strcmp(str_a, str_b);
}

struct subroutine_local_var *get_local_var(struct subroutine_ctx *subroutine, const char* var_name){
    struct subroutine_local_var key = {
        .name = (char *)var_name
    };

    return (struct subroutine_local_var*)hashmap_get(subroutine->local_var_map, &key);
}


uint32_t get_create_local_var(struct subroutine_ctx* subroutine, const char* var_name){
    struct subroutine_local_var *local_var = get_local_var(subroutine, var_name);
    if(local_var == NULL){
        // create one, since it doesn't exist
        local_var = malloc(sizeof(*local_var));
        local_var->name = strdup(var_name);
        local_var->offset = subroutine->local_var_count++;
        hashmap_set(subroutine->local_var_map, local_var);
        printf("[DEBUG] Creating Varname: %s -> %d\n", var_name, local_var->offset);
        return local_var->offset;
    }
    printf("[DEBUG] Re-using Varname: %s -> %d\n", var_name, local_var->offset);
    return local_var->offset;
}
