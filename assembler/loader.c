#include "assember.h"
#include <stdio.h>

#define EMIT(x)   (vm->program[vm->program_size++] = (x))
#define MAX_NAME  64
#define MAX_LINE  256

/* ── error helpers ──────────────────────────────────────────────────────── */

void parse_error(const char *message, ...) {
    printf("[PARSE ERROR] ");
    va_list args;
    va_start(args, message);
    vprintf(message, args);
    va_end(args);
    printf("\n");
}

static FILE *read_file(const char *filename) {
    FILE *file = fopen(filename, "r");
    if (!file)
        printf("[ERROR] Could not open file: %s\n", filename);
    return file;
}

/* ── typed-store helpers ────────────────────────────────────────────────── */

/* Emit opcode + value (as int32_t bits) + absolute RAM address. */
static int emit_store(VM *vm, uint32_t opcode, const char *cursor, int line_num) {
    int      n = 0;
    uint32_t addr;

    if (opcode == OP_ISTORE || opcode == OP_BSTORE) {
        int value;
        if (sscanf(cursor, "%i%n", &value, &n) != 1) {
            parse_error("Line %d: Expected integer/byte value.", line_num); return -1;
        }
        cursor += n;
        if (sscanf(cursor, "%i", &addr) != 1) {
            parse_error("Line %d: Expected address.", line_num); return -1;
        }
        EMIT(opcode); EMIT((int32_t)value); EMIT(addr);

    } else if (opcode == OP_FSTORE) {
        float value;
        if (sscanf(cursor, "%f%n", &value, &n) != 1) {
            parse_error("Line %d: Expected float value.", line_num); return -1;
        }
        cursor += n;
        if (sscanf(cursor, "%i", &addr) != 1) {
            parse_error("Line %d: Expected address.", line_num); return -1;
        }
        union { float f; int32_t i; } pun = { .f = value };
        EMIT(opcode); EMIT(pun.i); EMIT(addr);

    } else if (opcode == OP_CSTORE) {
        char value;
        if (sscanf(cursor, " '%c'%n", &value, &n) != 1) {
            parse_error("Line %d: Expected char literal 'X'.", line_num); return -1;
        }
        cursor += n;
        if (sscanf(cursor, "%i", &addr) != 1) {
            parse_error("Line %d: Expected address.", line_num); return -1;
        }
        EMIT(opcode); EMIT((int32_t)value); EMIT(addr);
    }
    return 0;
}

/* Emit opcode + value (as int32_t bits) + local-variable slot offset.
   BUG FIX: var_name was declared as `char` — a single byte used as a
   string buffer, causing a stack smash on any real variable name. */
static int emit_store_load_local(VM *vm, uint32_t opcode, const char *cursor,
                             struct subroutine_ctx *sub, int line_num) {
    int  n = 0;
    char var_name[MAX_NAME];   /* fixed: was `char var_name;` */

    if (opcode == OP_ISTORE_L || opcode == OP_BSTORE_L) {
        int value;
        if (sscanf(cursor, "%i%n", &value, &n) != 1) {
            parse_error("Line %d: Expected integer/byte value.", line_num); return -1;
        }
        cursor += n;
        if (sscanf(cursor, "%63s", var_name) != 1) {
            parse_error("Line %d: Expected variable name.", line_num); return -1;
        }
        uint32_t offset = get_create_local_var(sub, var_name);
        EMIT(opcode); EMIT((int32_t)value); EMIT(offset);

    } else if (opcode == OP_FSTORE_L) {
        float value;
        if (sscanf(cursor, "%f%n", &value, &n) != 1) {
            parse_error("Line %d: Expected float value.", line_num); return -1;
        }
        cursor += n;
        if (sscanf(cursor, "%63s", var_name) != 1) {
            parse_error("Line %d: Expected variable name.", line_num); return -1;
        }
        uint32_t offset = get_create_local_var(sub, var_name);
        union { float f; int32_t i; } pun = { .f = value };
        EMIT(opcode); EMIT(pun.i); EMIT(offset);

    } else if (opcode == OP_CSTORE_L) {
        char value;
        if (sscanf(cursor, " '%c'%n", &value, &n) != 1) {
            parse_error("Line %d: Expected char literal 'X'.", line_num); return -1;
        }
        cursor += n;
        if (sscanf(cursor, "%63s", var_name) != 1) {
            parse_error("Line %d: Expected variable name.", line_num); return -1;
        }
        uint32_t offset = get_create_local_var(sub, var_name);
        EMIT(opcode); EMIT((int32_t)value); EMIT(offset);
    } else if (opcode == OP_STORE_L){
        // STORE_L
        if (sscanf(cursor, "%63s", var_name) != 1) {
            parse_error("Line %d: Expected variable name.", line_num); return -1;
        }
        uint32_t offset = get_create_local_var(sub, var_name);
        EMIT(opcode); EMIT(offset);
    }
    else{
        // LOAD_L
        if (sscanf(cursor, "%63s", var_name) != 1) {
            parse_error("Line %d: Expected variable name.", line_num); return -1;
        }
        uint32_t offset = get_create_local_var(sub, var_name);
        EMIT(opcode); EMIT(offset);
    }
    return 0;
}

/* ── main assembler ─────────────────────────────────────────────────────── */

int load_kdm_file(const char *filename, VM *vm) {
    FILE *file = read_file(filename);
    if (!file) return -1;

    char line[MAX_LINE];
    int  line_num        = 0;
    vm->program_size     = 0;

    struct hashmap        *subroutine_map      = hashmap_new(64, 0, 0, 0, subroutine_hash, subroutine_cmp, NULL, NULL);
    struct subroutine_ctx *current_subroutine  = NULL;
    struct subroutine_ctx *subroutine_global   = NULL;
    bool                   defining_subroutine = false;

/* Close the file, emit an error, and bail. */
#define FAIL(...) do { parse_error(__VA_ARGS__); fclose(file); return -1; } while (0)

    while (fgets(line, sizeof(line), file)) {
        line_num++;

        /* Skip leading whitespace, blank lines, and comments. */
        char *cursor = line;
        while (isspace((unsigned char)*cursor)) cursor++;
        if (*cursor == '\0' || *cursor == '#') continue;

        char command[MAX_NAME];
        int  read_bytes = 0;
        if (sscanf(cursor, "%63s%n", command, &read_bytes) <= 0) continue;
        cursor += read_bytes;

        /* ── subroutine label ─────────────────────────────────────────── */
        if (strncmp(command, "::", 2) == 0) {
            if (defining_subroutine)
                FAIL("Cannot start a new subroutine without calling RET: Line %d", line_num);

            struct subroutine_ctx *sub = malloc(sizeof(*sub));
            const char *name = &command[2];

            sub->name          = strdup(name);
            sub->program_line  = vm->program_size;
            sub->local_var_map = hashmap_new(64, 0, 0, 0, local_var_hash, local_var_cmp, NULL, NULL);

            hashmap_set(subroutine_map, sub);
            current_subroutine = sub;

            if (strcmp(name, "_global") == 0)
                subroutine_global = sub;

            printf("[SUBROUTINE]: {%s, %d}\n", sub->name, sub->program_line);
            defining_subroutine = true;
            continue;
        }

        /* ── zero-operand instructions ────────────────────────────────── */
        if      (strcmp(command, "HALT")  == 0) EMIT(OP_HALT);
        else if (strcmp(command, "POP")   == 0) EMIT(OP_POP);
        else if (strcmp(command, "DUP")   == 0) EMIT(OP_DUP);
        else if (strcmp(command, "SWAP")  == 0) EMIT(OP_SWAP);
        else if (strcmp(command, "ROT")   == 0) EMIT(OP_ROT);
        else if (strcmp(command, "PEEK")  == 0) EMIT(OP_PEEK);
        else if (strcmp(command, "ADD")   == 0) EMIT(OP_ADD);
        else if (strcmp(command, "SUB")   == 0) EMIT(OP_SUB);
        else if (strcmp(command, "MUL")   == 0) EMIT(OP_MUL);
        else if (strcmp(command, "DIV")   == 0) EMIT(OP_DIV);
        else if (strcmp(command, "MOD")   == 0) EMIT(OP_MOD);
        else if (strcmp(command, "STORE") == 0) EMIT(OP_STORE);
        else if (strcmp(command, "LOAD")  == 0) EMIT(OP_LOAD);
        else if (strcmp(command, "CMPEQ") == 0) EMIT(OP_CMPEQ);
        else if (strcmp(command, "CMPNEQ") == 0) EMIT(OP_CMPNEQ);
        else if (strcmp(command, "CMPLT") == 0) EMIT(OP_CMPLT);
        else if (strcmp(command, "CMPLE") == 0) EMIT(OP_CMPLE);
        else if (strcmp(command, "CMPGT") == 0) EMIT(OP_CMPGT);
        else if (strcmp(command, "CMPGE") == 0) EMIT(OP_CMPGE);
        else if (strcmp(command, "OUT")   == 0) EMIT(OP_OUT);
        else if (strcmp(command, "OUT_LN")   == 0) EMIT(OP_OUT_LN);
        else if (strcmp(command, "FOUT")  == 0) EMIT(OP_FOUT);
        else if (strcmp(command, "FOUT_LN")  == 0) EMIT(OP_FOUT_LN);
        else if (strcmp(command, "RET")   == 0) { defining_subroutine = false; EMIT(OP_RET); }

        /* ── IN <int> ─────────────────────────────────────────────────── */
        else if (strcmp(command, "IN") == 0) {
            int value;
            if (sscanf(cursor, "%i", &value) != 1)
                FAIL("Line %d: Expected integer.", line_num);
            EMIT(OP_IN); EMIT(value);
        }

        /* ── PUSH / FPUSH / CPUSH / BPUSH <value> ────────────────────── */
        else if (strcmp(command, "PUSH")  == 0 || strcmp(command, "FPUSH") == 0 ||
                 strcmp(command, "CPUSH") == 0 || strcmp(command, "BPUSH") == 0) {

            uint32_t opcode = (strcmp(command, "PUSH")  == 0) ? OP_PUSH  :
                              (strcmp(command, "FPUSH") == 0) ? OP_FPUSH :
                              (strcmp(command, "CPUSH") == 0) ? OP_CPUSH : OP_BPUSH;
            EMIT(opcode);

            if (opcode == OP_PUSH) {
                int32_t v;
                if (sscanf(cursor, "%i", &v) != 1) FAIL("Line %d: Expected integer.", line_num);
                EMIT(v);
            } else if (opcode == OP_FPUSH) {
                float v;
                if (sscanf(cursor, "%f", &v) != 1) FAIL("Line %d: Expected float.", line_num);
                union { float f; int32_t i; } pun = { .f = v };
                EMIT(pun.i);
            } else if (opcode == OP_CPUSH) {

                //TODO:: encapsulate this code segment, too big!
                // 1. Skip leading whitespace
                const char *p = cursor;
                while (*p == ' ') p++;

                // 2. Expect an opening quote
                if (*p != '\'') FAIL("Line %d: Expected opening quote.", line_num);
                p++;

                // 3. Determine if it is empty ('') or contains a char
                char v;
                if (*p == '\'') {
                    // Empty: ''
                    v = 0;
                } else {
                    // Contains a char (e.g., 'A')
                    v = *p;
                    p++;
                }

                // 4. Expect a closing quote
                if (*p != '\'') FAIL("Line %d: Expected closing quote.", line_num);
                p++;

                // 5. Success
                EMIT((int32_t)v);
                // Optional: Update 'cursor' to point after this token
                cursor = (char *)p;
            } else { /* OP_BPUSH */
                int v;
                if (sscanf(cursor, "%i", &v) != 1) FAIL("Line %d: Expected byte.", line_num);
                EMIT((int32_t)(int8_t)v);
            }
        }

        /* ── JUMP / JZ / JNZ <relative-offset> ───────────────────────── */
        else if (strcmp(command, "JUMP") == 0 || strcmp(command, "JZ") == 0 || strcmp(command, "JNZ") == 0) {
            if (!current_subroutine)   /* BUG FIX: was an unchecked NULL deref */
                FAIL("Line %d: Jump instruction outside of subroutine.", line_num);

            uint32_t opcode = (strcmp(command, "JUMP") == 0) ? OP_JUMP :
                              (strcmp(command, "JZ")   == 0) ? OP_JZ   : OP_JNZ;
            int value;
            if (sscanf(cursor, "%d", &value) != 1)
                FAIL("Line %d: Expected integer.", line_num);
            EMIT(opcode); EMIT((uint32_t)value + current_subroutine->program_line);
        }

        /* ── CALL <name> ──────────────────────────────────────────────── */
        else if (strcmp(command, "CALL") == 0) {
            char name[MAX_NAME];
            if (sscanf(cursor, "%63s", name) != 1)
                FAIL("Line %d: Expected subroutine name.", line_num);

            struct subroutine_ctx  key = { .name = name };
            struct subroutine_ctx *sub = (struct subroutine_ctx*)hashmap_get(subroutine_map, &key);
            if (!sub)
                FAIL("Line %d: Unknown subroutine '%s'.", line_num, name); /* BUG FIX: was vm->program_size */

            EMIT(OP_CALL); EMIT((uint32_t)sub->program_line);
        }

        /* ── typed STORE (global RAM) ─────────────────────────────────── */
        else if (strcmp(command, "ISTORE") == 0 || strcmp(command, "FSTORE") == 0 ||
                 strcmp(command, "CSTORE") == 0 || strcmp(command, "BSTORE") == 0) {

            uint32_t opcode = (strcmp(command, "ISTORE") == 0) ? OP_ISTORE :
                              (strcmp(command, "FSTORE") == 0) ? OP_FSTORE :
                              (strcmp(command, "CSTORE") == 0) ? OP_CSTORE : OP_BSTORE;

            if (emit_store(vm, opcode, cursor, line_num) < 0) { fclose(file); return -1; }
        }

        /* ── typed STORE_L (local variable) ──────────────────────────── */
        else if (strcmp(command, "ISTORE_L") == 0 || strcmp(command, "FSTORE_L") == 0 ||
                 strcmp(command, "CSTORE_L") == 0 || strcmp(command, "BSTORE_L") == 0 ||
                 strcmp(command, "STORE_L") == 0 || strcmp(command, "LOAD_L") == 0) {

            if (!current_subroutine)   /* BUG FIX: was an unchecked NULL deref */
                FAIL("Line %d: Local store outside of subroutine.", line_num);

            uint32_t opcode = (strcmp(command, "ISTORE_L") == 0) ? OP_ISTORE_L :
                              (strcmp(command, "FSTORE_L") == 0) ? OP_FSTORE_L :
                              (strcmp(command, "CSTORE_L") == 0) ? OP_CSTORE_L :
                              (strcmp(command, "BSTORE_L") == 0) ? OP_BSTORE_L :
                              (strcmp(command, "STORE_L") == 0)  ? OP_STORE_L : OP_LOAD_L;

            if (emit_store_load_local(vm, opcode, cursor, current_subroutine, line_num) < 0) { fclose(file); return -1; }
        }

        /* ── unknown instruction ──────────────────────────────────────── */
        else {
            FAIL("Line %d: Unknown instruction '%s'.", line_num, command);
        }

        if (vm->program_size + 2 >= VM_PROGRAM_MEM)
            FAIL("Program size exceeds VM capacity.");
    }

#undef FAIL

    fclose(file);

    if (!subroutine_global) {
        parse_error("No _global subroutine was defined.");
        return -1;
    }

    vm->_global_start = subroutine_global->program_line;
    printf("Successfully assembled %s (%d instructions loaded).\n", filename, vm->program_size);
    return 0;
}
