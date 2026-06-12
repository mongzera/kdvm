#include "kdvm.h"
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "../util/hashmap/hashmap.h"

#define EMIT(x) vm->program[vm->program_size++] = (x)



struct subroutine_ctx{
    const char* name;
    int program_line;
};

void vm_error(const char* message) {
    printf("[VM ERROR] %s\n", message);
}

void parse_error(const char* message, ...) {
    printf("[PARSE ERROR] ");

    va_list args;
    va_start(args, message);

    vprintf(message, args);

    va_end(args);

    printf("\n");
}

FILE* read_file(const char* filename){
    FILE* file = fopen(filename, "r");
    if (!file) {
        printf("[ERROR] Could not open file: %s\n", filename);
        return 0;
    }

    return file;
}

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

int load_kdm_file(const char* filename, VM* vm) {

    FILE* file = read_file(filename);

    // Initialize
    char line[256];
    int line_num = 0;
    vm->program_size = 0;

    // create hashmap for sub-routines
    struct hashmap *subroutine_map = hashmap_new(64, 0, 0, 0, subroutine_hash, subroutine_cmp, NULL, NULL);

    // main subroutine
    struct subroutine_ctx *subroutine_global = 0;

    // check whether a new subroutine was started without calling RET
    bool defining_subroutine = false;

    while (fgets(line, sizeof(line), file)) {
        line_num++;

        char* cursor = line;
        while (isspace((unsigned char)*cursor)) cursor++;

        if (*cursor == '\0' || *cursor == '#') continue;

        char command[64];
        int read_bytes = 0;
        if (sscanf(cursor, "%63s%n", command, &read_bytes) <= 0) continue;
        cursor += read_bytes;


        // Sub-routine Parsing
        if(strncmp(command, "::", 2) == 0){
            if(defining_subroutine){
                parse_error("Cannot start a new subroutine without calling RET: Line %d", line_num);
                fclose(file);
                return -1;
            }

            struct subroutine_ctx *subroutine = malloc(sizeof(*subroutine));;

            const char* subroutine_name = &command[2];
            subroutine->name = strdup(subroutine_name);
            subroutine->program_line = vm->program_size;

            hashmap_set(subroutine_map, subroutine);

            // check if subroutine is ::_global
            if(strcmp(subroutine_name, "_global") == 0){
                subroutine_global = subroutine;
            }

            printf("[SUBROUTINE]: {%s, %d}\n", subroutine->name, subroutine->program_line);
            defining_subroutine = true;
            continue;
        }

        // Base & Integer
        if (strcmp(command, "HALT") == 0) EMIT(OP_HALT);
        else if (strcmp(command, "POP") == 0)   EMIT(OP_POP);
        else if (strcmp(command, "ADD") == 0)   EMIT(OP_ADD);
        else if (strcmp(command, "SUB") == 0)   EMIT(OP_SUB);
        else if (strcmp(command, "MUL") == 0)   EMIT(OP_MUL);
        else if (strcmp(command, "DIV") == 0)   EMIT(OP_DIV);

        // Memory & Comparisons & Return Callstack
        else if (strcmp(command, "STORE") == 0) EMIT(OP_STORE);
        else if (strcmp(command, "LOAD") == 0)  EMIT(OP_LOAD);
        else if (strcmp(command, "CMPEQ") == 0) EMIT(OP_CMPEQ);
        else if (strcmp(command, "CMPLT") == 0) EMIT(OP_CMPLT);
        else if (strcmp(command, "RET") == 0)  {
            if(!defining_subroutine){
                parse_error("Cannot end with RET without defining a sub-routine! Line %d", line_num);
                fclose(file);
                return -1;
            }
            defining_subroutine = false;
            EMIT(OP_RET);
        }

        // I/O
        else if (strcmp(command, "OUT") == 0)   EMIT(OP_OUT);
        else if (strcmp(command, "FOUT") == 0)  EMIT(OP_FOUT);
        else if (strcmp(command, "IN") == 0)    EMIT(OP_IN);

        // Floats
        else if (strcmp(command, "FADD") == 0)  EMIT(OP_FADD);
        else if (strcmp(command, "FSUB") == 0)  EMIT(OP_FSUB);
        else if (strcmp(command, "FMUL") == 0)  EMIT(OP_FMUL);
        else if (strcmp(command, "FDIV") == 0)  EMIT(OP_FDIV);

        // Parameterized Commands (Require an integer argument)
        else if (strcmp(command, "PUSH") == 0 || strcmp(command, "JUMP") == 0 || strcmp(command, "JIF") == 0) {
            EMIT( (strcmp(command, "PUSH") == 0) ? OP_PUSH :
                  (strcmp(command, "JUMP") == 0) ? OP_JUMP :
                                                   OP_JIF);

            int value;
            if (sscanf(cursor, "%d", &value) != 1) {
                parse_error("Line %d: Expected integer.\n", line_num);
                fclose(file); return -1;
            }

            EMIT((uint32_t)value);
        }

        // Parameterized Command (Require a float argument)
        else if (strcmp(command, "FPUSH") == 0) {
            EMIT(OP_FPUSH);

            float val;
            if (sscanf(cursor, "%f", &val) != 1) {
                parse_error("Line %d: Expected float.\n", line_num);
                fclose(file); return -1;
            }

            FloatCast cast; cast.f = val;
            EMIT(cast.u);
        }

        // Parameterized Commands (Require a string argument)
        else if (strcmp(command, "CALL") == 0) {
            EMIT(OP_CALL);

            char value[64];
            if (sscanf(cursor, "%63s", value) != 1) {
                parse_error("Line %d: Expected string.\n", line_num);
                fclose(file); return -1;
            }

            struct subroutine_ctx key = { .name = value };
            struct subroutine_ctx *subroutine = (struct subroutine_ctx*) hashmap_get(subroutine_map, &key);

            if(subroutine == NULL){
                parse_error("Subroutine %s does not exist!. Line %d\n", value, vm->program_size);
                fclose(file); return -1;
            }

            // if it exists
            EMIT((uint32_t)subroutine->program_line);

        }

        else {
            parse_error("Line %d: Unknown instruction '%s'\n", line_num, command);
            fclose(file); return -1;
        }

        if (vm->program_size + 2 >= PROGRAM_MEM) {
            vm_error("Program size exceeds VM capacity.");
            fclose(file); return -1;
        }
    }

    fclose(file);

    if(subroutine_global == 0) {
        parse_error("No _global subroutine was defined!\n");
        return -1;
    }

    vm->_global_start = subroutine_global->program_line;
    printf("Successfully assembled %s (%d instructions loaded).\n", filename, vm->program_size);
    return 0;
}
