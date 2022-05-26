#pragma once
#include "common.h"

void* allocate( size_t size );
void* zallocate( size_t size );
void deallocate( void* buffer, size_t size );
size_t growCapacity( size_t capacity );
void* growArray( size_t typeSize, void* array, size_t oldCapacity, size_t newCapacity );
void freeArray( size_t typeSize, void* array, size_t capacity );
void freeObjects();
