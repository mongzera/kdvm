#include "../core/grrvm.h"
#include <stdio.h>

/* Loads a pre-compiled binary `.bin` bytecode file directly into VM RAM. */
int load_kdm_file(const char* filename, VM* vm) {
    FILE *file = fopen(filename, "rb");
    if (!file) {
        printf("[ERROR] Could not open binary file: %s\n", filename);
        return -1;
    }

    // 1. Read the 8-byte header
    uint32_t header[2];
    if (fread(header, sizeof(uint32_t), 2, file) != 2) {
        printf("[ERROR] Failed to read binary header from %s\n", filename);
        fclose(file);
        return -1;
    }

    vm->_global_start = header[0];
    vm->program_size  = header[1];

    // 2. Validate memory bounds against grrvm.h limits
    if (vm->program_size > VM_PROGRAM_MEM) {
        printf("[ERROR] Program size (%u words) exceeds VM_PROGRAM_MEM (%d words).\n",
               vm->program_size, VM_PROGRAM_MEM);
        fclose(file);
        return -1;
    }

    // 3. Read the exact bytecode words directly into vm->program
    size_t words_read = fread(vm->program, sizeof(uint32_t), vm->program_size, file);
    if (words_read != vm->program_size) {
        printf("[ERROR] File truncated. Expected %u words, read %zu.\n",
               vm->program_size, words_read);
        fclose(file);
        return -1;
    }

    fclose(file);
    printf("[VM LOADER] Successfully loaded %s (%u instructions, global start @ %u).\n",
           filename, vm->program_size, vm->_global_start);
    return 0;
}
