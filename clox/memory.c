#include <stdlib.h>
#include "memory.h"

void* reallocate( void* buffer, size_t oldSize, size_t newSize ) {
    // no bytes requested: free memory
    if( 0 == newSize ) {
        free( buffer );
        return NULL;
    }

    // otherwise: allocate/resize memory
    void* newBuffer = realloc( buffer, newSize );
    if( NULL == newBuffer ) exit( 1 ); // out-of-memory!
    return newBuffer;
}

size_t growCapacity( size_t capacity ) {
    return capacity < 8 ? 8 : capacity << 1;
}

void* growArray( size_t typeSize, void* array, size_t oldCapacity, size_t newCapacity ) {
    size_t oldSize = typeSize * oldCapacity, newSize = typeSize * newCapacity;
    return reallocate( array, oldSize, newSize );
}

void freeArray( size_t typeSize, void* array, size_t capacity ) {
    size_t size = typeSize * capacity;
    reallocate( array, size, 0 );
}
