#ifndef H_TYPES
#define H_TYPES

#include "../../core/kdvm.h"

bool primitive_is_numeric(PrimitiveValue v);

double primitive_as_double(PrimitiveValue v);
int32_t primitive_as_int(PrimitiveValue v);
PrimitiveValue make_int(int32_t v);
PrimitiveValue make_float(float v);

// comparison
uint32_t primitive_equal(PrimitiveValue a, PrimitiveValue b);
uint32_t primitive_not_equal(PrimitiveValue a, PrimitiveValue b);
uint32_t primitive_less_than(PrimitiveValue a, PrimitiveValue b);
uint32_t primitive_less_than_equal(PrimitiveValue a, PrimitiveValue b);
uint32_t primitive_greater_than(PrimitiveValue a, PrimitiveValue b);
uint32_t primitive_greater_than_equal(PrimitiveValue a, PrimitiveValue b);

// arithmetic
PrimitiveValue primitive_add(PrimitiveValue a, PrimitiveValue b);
PrimitiveValue primitive_mul(PrimitiveValue a, PrimitiveValue b);
PrimitiveValue primitive_sub(PrimitiveValue a, PrimitiveValue b);
PrimitiveValue primitive_div(PrimitiveValue a, PrimitiveValue b);
PrimitiveValue primitive_mod(PrimitiveValue a, PrimitiveValue b);

// conversion
void primitive_print(PrimitiveValue value);

#endif
