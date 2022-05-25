#pragma once
#include "common.h"
#include "value.h"

#define OBJ_TYPE(value)     (AS_OBJ(value)->type)
#define IS_STRING(value)    isObjType(value, OBJ_STRING)
#define AS_STRING(value)    ((ObjString*)AS_OBJ(value))

typedef enum {
    OBJ_STRING,
} ObjType;

struct Obj {
    ObjType type;
};

struct ObjString {
    Obj obj;
    size_t len;
    char buf[]; // flexible array member
};

void printObject( Obj* obj );
bool objectsEqual( Obj* a, Obj* b );
ObjString* makeString( const char* chars, size_t len );
ObjString* concatStrings( ObjString* a, ObjString* b );

static inline bool isObjType( Value value, ObjType type ) {
    return IS_OBJ(value) && AS_OBJ(value)->type == type;
}
