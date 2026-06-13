#include "kdvm.h"
#include <stdio.h>

void vm_error(const char* message) {
    printf("[VM ERROR] %s\n", message);
}

void vm_depr(const char* message) {
    printf("[VM DEPRECATED] %s\n", message);
}
