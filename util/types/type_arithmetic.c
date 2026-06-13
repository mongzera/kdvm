#include "types.h"
#include <math.h>
#include <stdlib.h>

double primitive_as_double(PrimitiveValue v) {
    switch (v.type) {
        case TYPE_INT:   return (double)v.data.u;
        case TYPE_FLOAT: return (double)v.data.f;
        case TYPE_CHAR:  return (double)v.data.c;
        default:         return 0.0;
    }
}

int32_t primitive_as_int(PrimitiveValue v) { // Change to int32_t
    switch (v.type) {
        case TYPE_INT:   return v.data.u;
        case TYPE_CHAR:  return (int32_t)v.data.c;
        case TYPE_FLOAT: return (int32_t)v.data.f;
        case TYPE_BYTE:  return (int32_t)v.data.b;
        default:         return 0;
    }
}

PrimitiveValue make_int(int32_t v) {
    PrimitiveValue r;
    r.type = TYPE_INT;
    r.data.u = v;
    return r;
}

PrimitiveValue make_float(float v) {
    PrimitiveValue r;
    r.type = TYPE_FLOAT;
    r.data.f = v;
    return r;
}

PrimitiveValue primitive_add(PrimitiveValue a, PrimitiveValue b) {
    if (!primitive_is_numeric(a) || !primitive_is_numeric(b))
        return make_int(0);

    if (a.type == TYPE_FLOAT || b.type == TYPE_FLOAT) {
        return make_float(
            (float)(primitive_as_double(a) +
                    primitive_as_double(b)));
    }

    return make_int(
        primitive_as_int(a) +
        primitive_as_int(b));
}

PrimitiveValue primitive_mul(PrimitiveValue a, PrimitiveValue b) {
    if (!primitive_is_numeric(a) || !primitive_is_numeric(b))
        return make_int(0);

    if (a.type == TYPE_FLOAT || b.type == TYPE_FLOAT) {
        return make_float(
            (float)(primitive_as_double(a) *
                    primitive_as_double(b)));
    }

    return make_int(
        primitive_as_int(a) *
        primitive_as_int(b));
}

PrimitiveValue primitive_sub(PrimitiveValue a, PrimitiveValue b) {
    if (!primitive_is_numeric(a) || !primitive_is_numeric(b))
        return make_int(0);

    if (a.type == TYPE_FLOAT || b.type == TYPE_FLOAT) {
        return make_float(
            (float)(primitive_as_double(a) -
                    primitive_as_double(b)));
    }

    return make_int(
        primitive_as_int(a) -
        primitive_as_int(b));
}

PrimitiveValue primitive_div(PrimitiveValue a, PrimitiveValue b) {
    if (!primitive_is_numeric(a) || !primitive_is_numeric(b)) return make_int(0);

    if (a.type == TYPE_FLOAT || b.type == TYPE_FLOAT) {
        // Java allows float division by zero (results in Infinity or NaN)
        return make_float((float)(primitive_as_double(a) / primitive_as_double(b)));
    }

    int32_t divisor = primitive_as_int(b);
    if (divisor == 0) {
        vm_error("ArithmeticException: / by zero");
        exit(-1);
    }

    return make_int(primitive_as_int(a) / divisor);
}

PrimitiveValue primitive_mod(PrimitiveValue a, PrimitiveValue b) {
    if (!primitive_is_numeric(a) || !primitive_is_numeric(b)) return make_int(0);

    // Java allows floating point modulo!
    if (a.type == TYPE_FLOAT || b.type == TYPE_FLOAT) {
        return make_float(fmodf((float)primitive_as_double(a), (float)primitive_as_double(b)));
    }

    // Add a check to prevent a hard VM crash (SIGFPE) from divide-by-zero
    int32_t divisor = primitive_as_int(b);
    if (divisor == 0) {
        vm_error("ArithmeticException: / by zero");
        exit(-1);
    }

    return make_int(primitive_as_int(a) % divisor);
}
