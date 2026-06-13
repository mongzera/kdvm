#include "types.h"
#include <math.h>


bool primitive_is_numeric(PrimitiveValue v) {
    return v.type == TYPE_INT ||
           v.type == TYPE_CHAR ||
           v.type == TYPE_FLOAT ||
           v.type == TYPE_BYTE;
}

uint32_t primitive_equal(PrimitiveValue a, PrimitiveValue b) {
    if (!primitive_is_numeric(a) || !primitive_is_numeric(b)) {
        return 0;
    }

    double da = primitive_as_double(a);
    double db = primitive_as_double(b);
    return fabs(da - db) < 0.000001;
}

uint32_t primitive_not_equal(PrimitiveValue a, PrimitiveValue b) {
    return !primitive_equal(a, b);
}

uint32_t primitive_less_than(PrimitiveValue a, PrimitiveValue b) {
    if (!primitive_is_numeric(a) || !primitive_is_numeric(b)) {
        return 0;
    }

    return primitive_as_double(a) <
           primitive_as_double(b);
}

uint32_t primitive_less_than_equal(PrimitiveValue a, PrimitiveValue b) {
    if (!primitive_is_numeric(a) || !primitive_is_numeric(b)) {
        return 0;
    }

    return primitive_as_double(a) <=
           primitive_as_double(b);
}

uint32_t primitive_greater_than(PrimitiveValue a, PrimitiveValue b) {
    if (!primitive_is_numeric(a) || !primitive_is_numeric(b)) {
        return 0;
    }

    return primitive_as_double(a) >
           primitive_as_double(b);
}

uint32_t primitive_greater_than_equal(PrimitiveValue a, PrimitiveValue b) {
    if (!primitive_is_numeric(a) || !primitive_is_numeric(b)) {
        return 0;
    }

    return primitive_as_double(a) >=
           primitive_as_double(b);
}
