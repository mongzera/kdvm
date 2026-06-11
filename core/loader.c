#include "kdvm.h"
#include <stdio.h>
#include <string.h>
#include <ctype.h>

void vm_error(const char* message) {
    printf("[VM ERROR] %s\n", message);
}

int load_kdm_file(const char* filename, VM* vm) {
    FILE* file = fopen(filename, "r");
    if (!file) {
        printf("[ERROR] Could not open file: %s\n", filename);
        return -1;
    }

    char line[256];
    int line_num = 0;
    vm->program_size = 0;

    while (fgets(line, sizeof(line), file)) {
        line_num++;

        char* cursor = line;
        while (isspace((unsigned char)*cursor)) cursor++;

        if (*cursor == '\0' || *cursor == '#') continue;

        char command[64];
        int read_bytes = 0;
        if (sscanf(cursor, "%63s%n", command, &read_bytes) <= 0) continue;
        cursor += read_bytes;

        uint32_t* pc = &vm->program[vm->program_size];

        // Base & Integer
        if (strcmp(command, "HALT") == 0)       *pc = OP_HALT;
        else if (strcmp(command, "POP") == 0)   *pc = OP_POP;
        else if (strcmp(command, "ADD") == 0)   *pc = OP_ADD;
        else if (strcmp(command, "SUB") == 0)   *pc = OP_SUB;
        else if (strcmp(command, "MUL") == 0)   *pc = OP_MUL;
        else if (strcmp(command, "DIV") == 0)   *pc = OP_DIV;

        // Memory & Comparisons & Return Callstack
        else if (strcmp(command, "STORE") == 0) *pc = OP_STORE;
        else if (strcmp(command, "LOAD") == 0)  *pc = OP_LOAD;
        else if (strcmp(command, "CMPEQ") == 0) *pc = OP_CMPEQ;
        else if (strcmp(command, "CMPLT") == 0) *pc = OP_CMPLT;
        else if (strcmp(command, "RET") == 0)   *pc = OP_RET;

        // I/O
        else if (strcmp(command, "OUT") == 0)   *pc = OP_OUT;
        else if (strcmp(command, "FOUT") == 0)  *pc = OP_FOUT;
        else if (strcmp(command, "IN") == 0)    *pc = OP_IN;

        // Floats
        else if (strcmp(command, "FADD") == 0)  *pc = OP_FADD;
        else if (strcmp(command, "FSUB") == 0)  *pc = OP_FSUB;
        else if (strcmp(command, "FMUL") == 0)  *pc = OP_FMUL;
        else if (strcmp(command, "FDIV") == 0)  *pc = OP_FDIV;

        // Parameterized Commands (Require an argument)
        else if (strcmp(command, "PUSH") == 0 || strcmp(command, "JUMP") == 0 ||
                 strcmp(command, "JIF") == 0  || strcmp(command, "CALL") == 0) {
            *pc = (strcmp(command, "PUSH") == 0) ? OP_PUSH :
                  (strcmp(command, "JUMP") == 0) ? OP_JUMP :
                  (strcmp(command, "CALL") == 0) ? OP_CALL : OP_JIF;

            vm->program_size++;
            int value;
            if (sscanf(cursor, "%d", &value) != 1) {
                printf("[PARSER ERROR] Line %d: Expected integer.\n", line_num);
                fclose(file); return -1;
            }
            vm->program[vm->program_size] = (uint32_t)value;
        }
        else if (strcmp(command, "FPUSH") == 0) {
            *pc = OP_FPUSH;
            vm->program_size++;
            float val;
            if (sscanf(cursor, "%f", &val) != 1) {
                printf("[PARSER ERROR] Line %d: Expected float.\n", line_num);
                fclose(file); return -1;
            }
            FloatCast cast; cast.f = val;
            vm->program[vm->program_size] = cast.u;
        }
        else {
            printf("[PARSER ERROR] Line %d: Unknown instruction '%s'\n", line_num, command);
            fclose(file); return -1;
        }

        vm->program_size++;
        if (vm->program_size >= PROGRAM_MEM) {
            vm_error("Program size exceeds VM capacity.");
            fclose(file); return -1;
        }
    }

    fclose(file);
    printf("Successfully assembled %s (%d instructions loaded).\n", filename, vm->program_size);
    return 0;
}
