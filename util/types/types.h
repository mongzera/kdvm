#ifndef H_TYPES
#define H_TYPES

#include "../../core/kdvm.h"
double primitive_numeric_value(PrimitiveValue v);
bool primitive_is_numeric(PrimitiveValue v);

uint32_t primitive_equal(PrimitiveValue a, PrimitiveValue b);
uint32_t primitive_not_equal(PrimitiveValue a, PrimitiveValue b);
uint32_t primitive_less_than(PrimitiveValue a, PrimitiveValue b);
uint32_t primitive_less_than_equal(PrimitiveValue a, PrimitiveValue b);
uint32_t primitive_greater_than(PrimitiveValue a, PrimitiveValue b);
uint32_t primitive_greater_than_equal(PrimitiveValue a, PrimitiveValue b);


#endif
