#pragma once
#include "common.h"

size_t growCapacity( size_t capacity );
void* growArray( size_t typeSize, void* array, size_t oldCapacity, size_t newCapacity );
void freeArray( size_t typeSize, void* array, size_t capacity );
