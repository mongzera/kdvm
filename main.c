#include <stdio.h>
#include "kdvm.h"

int main(int argc, char* argv[]) {
    if (argc < 2) {
        printf("Usage: %s <program.kdm>\n", argv[0]);
        return 1;
    }

    // 1. Create a virtual machine instance
    VM my_machine;
    vm_init(&my_machine);

    // 2. Load the program into this specific instance
    if (load_kdm_file(argv[1], &my_machine) != 0) {
        return 1;
    }

    // 3. Execute
    printf("--- Execution Started ---\n");
    vm_execute(&my_machine);
    printf("--- Execution Halted ---\n");

    return 0;
}
