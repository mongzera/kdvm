#include "types.h"
#include <math.h>
#include <float.h>

// Define a small epsilon for floating point comparison
#define EPSILON 1e-9

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

    // Check if the difference is within the epsilon tolerance
    return (fabs(da - db) < EPSILON) ? 1 : 0;
}

uint32_t primitive_not_equal(PrimitiveValue a, PrimitiveValue b) {
    // Correctly returns 1 if not equal, 0 if equal
    return !primitive_equal(a, b);
}

uint32_t primitive_less_than(PrimitiveValue a, PrimitiveValue b) {
    if (!primitive_is_numeric(a) || !primitive_is_numeric(b)) {
        return 0;
    }

    // Explicitly return 1 or 0
    return (primitive_as_double(a) < primitive_as_double(b)) ? 1 : 0;
}

uint32_t primitive_less_than_equal(PrimitiveValue a, PrimitiveValue b) {
    if (!primitive_is_numeric(a) || !primitive_is_numeric(b)) {
        return 0;
    }

    double da = primitive_as_double(a);
    double db = primitive_as_double(b);

    // For <=, we check if it is less than OR equal (within epsilon)
    return (da < db || fabs(da - db) < EPSILON) ? 1 : 0;
}

uint32_t primitive_greater_than(PrimitiveValue a, PrimitiveValue b) {
    if (!primitive_is_numeric(a) || !primitive_is_numeric(b)) {
        return 0;
    }

    return (primitive_as_double(a) > primitive_as_double(b)) ? 1 : 0;
}

uint32_t primitive_greater_than_equal(PrimitiveValue a, PrimitiveValue b) {
    if (!primitive_is_numeric(a) || !primitive_is_numeric(b)) {
        return 0;
    }

    double da = primitive_as_double(a);
    double db = primitive_as_double(b);

    // For >=, we check if it is greater than OR equal (within epsilon)
    return (da > db || fabs(da - db) < EPSILON) ? 1 : 0;
}
