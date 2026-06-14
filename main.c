#include <stdio.h>
#include "core/kdvm.h"
#include "assembler/assember.h"
#include <time.h>

int main(int argc, char* argv[]) {
    if (argc < 2) {
        printf("Usage: %s <program.kdm>\n", argv[0]);
        return 1;
    }

    // Initialize a Single VM Instance for now
    VM my_machine;
    vm_init(&my_machine);

    // Load the program into this specific instance
    if (load_kdm_file(argv[1], &my_machine) != 0) {
        return 1;
    }

    struct timespec start, end;

    // Execute
    printf("--- Execution Started ---\n");
    clock_gettime(CLOCK_MONOTONIC, &start);
    vm_execute(&my_machine);
    clock_gettime(CLOCK_MONOTONIC, &end);
    printf("--- Execution Halted ---\n");

    // Calculate elapsed time
    long long elapsed_ns = (end.tv_sec - start.tv_sec) * 1000000000LL + (end.tv_nsec - start.tv_nsec);

    printf("Execution time: %lld nanoseconds\n", elapsed_ns);
    return 0;
}
