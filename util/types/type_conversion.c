#include "types.h"
#include <stdio.h>

void primitive_print(PrimitiveValue value){
    switch (value.type) {
        case TYPE_INT:      printf("%d\n", value.data.u); break;
        case TYPE_FLOAT:    printf("%f\n", value.data.f); break;
        case TYPE_CHAR:     printf("%c\n", value.data.c); break;
        case TYPE_BYTE:     printf("%i\n", value.data.b); break;
        default:            vm_error("Cannot print value"); break;
    }
}
