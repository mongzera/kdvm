#include "types.h"
#include <math.h>

double primitive_numeric_value(PrimitiveValue v) {
    switch (v.type) {
        case TYPE_INT:
            return (double)v.data.u;

        case TYPE_CHAR:
            return (double)v.data.c;

        case TYPE_FLOAT:
            return (double)v.data.f;

        default:
            return 0.0;
    }
}

bool primitive_is_numeric(PrimitiveValue v) {
    return v.type == TYPE_INT ||
           v.type == TYPE_CHAR ||
           v.type == TYPE_FLOAT;
}
uint32_t primitive_equal(PrimitiveValue a, PrimitiveValue b) {
    if (!primitive_is_numeric(a) || !primitive_is_numeric(b)) {
        return 0;
    }

    double da = primitive_numeric_value(a);
    double db = primitive_numeric_value(b);
    return fabs(da - db) < 0.000001;
}

uint32_t primitive_not_equal(PrimitiveValue a, PrimitiveValue b) {
    return !primitive_equal(a, b);
}

uint32_t primitive_less_than(PrimitiveValue a, PrimitiveValue b) {
    if (!primitive_is_numeric(a) || !primitive_is_numeric(b)) {
        return 0;
    }

    return primitive_numeric_value(a) <
           primitive_numeric_value(b);
}

uint32_t primitive_less_than_equal(PrimitiveValue a, PrimitiveValue b) {
    if (!primitive_is_numeric(a) || !primitive_is_numeric(b)) {
        return 0;
    }

    return primitive_numeric_value(a) <=
           primitive_numeric_value(b);
}

uint32_t primitive_greater_than(PrimitiveValue a, PrimitiveValue b) {
    if (!primitive_is_numeric(a) || !primitive_is_numeric(b)) {
        return 0;
    }

    return primitive_numeric_value(a) >
           primitive_numeric_value(b);
}

uint32_t primitive_greater_than_equal(PrimitiveValue a, PrimitiveValue b) {
    if (!primitive_is_numeric(a) || !primitive_is_numeric(b)) {
        return 0;
    }

    return primitive_numeric_value(a) >=
           primitive_numeric_value(b);
}
