#include <stdio.h>
#include <stdint.h>
#include "../core/grrvm.h"

// Packed representation matching PrimitiveValueBin in assembler.py
typedef struct {
    uint8_t  type;
    uint8_t  word_state;
    uint16_t reserved;
    int32_t  data;
} __attribute__((packed)) BinaryPrimitive;

int load_kdm_file(const char* filename, VM* vm) {
    FILE *file = fopen(filename, "rb");
    if (!file) {
        printf("[ERROR] Could not open binary file: %s\n", filename);
        return -1;
    }

    // 1. Read 12-byte header
    uint32_t header[3]; // [_global_start, program_size, data_size]
    if (fread(header, sizeof(uint32_t), 3, file) != 3) {
        printf("[ERROR] Corrupt file header in %s\n", filename);
        fclose(file);
        return -1;
    }

    vm->_global_start = header[0];
    vm->program_size  = header[1];
    uint32_t data_size = header[2];

    // 2. Read ONLY instructions into vm->program
    if (fread(vm->program, sizeof(uint32_t), vm->program_size, file) != vm->program_size) {
        printf("[ERROR] Failed to read instruction stream.\n");
        fclose(file);
        return -1;
    }

    // 3. Read remaining data section directly into vm->ram
    BinaryPrimitive raw_prim;
    for (uint32_t i = 0; i < data_size; i++) {
        if (fread(&raw_prim, sizeof(BinaryPrimitive), 1, file) != 1) {
            printf("[ERROR] Corrupt data payload at RAM index %u\n", i);
            fclose(file);
            return -1;
        }

        vm->ram[i].type       = (PrimitiveType)raw_prim.type;
        vm->ram[i].word_state = raw_prim.word_state;
        vm->ram[i].data.u     = raw_prim.data;
    }

    fclose(file);
    return 0; // Success
}
