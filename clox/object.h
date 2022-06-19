#pragma once
#include "common.h"
#include "chunk.h"
#include "value.h"

#define OBJ_TYPE(value)     (AS_OBJ(value)->type)
#define IS_STRING(value)    isObjType(value, OBJ_STRING)
#define AS_STRING(value)    ((ObjString*)AS_OBJ(value))
#define HASH_SEED 2166136261u
#define HASH_PRIME 16777619

typedef enum {
    OBJ_STRING,
} ObjType;

struct Obj {
    ObjType type;
    struct Obj* next;
};

struct ObjString {
    Obj obj;
    size_t len;
    uint32_t hash; // upgrade to 64-bit at some point
    char buf[]; // flexible array member
};

struct ObjFunction {
    Obj obj;
    int arity;
    Chunk chunk;
    ObjString* name;
};

void printObject( Obj* obj );
void printString( ObjString* str );
ObjString* makeString( const char* s, size_t len );
ObjString* concatStrings( const char* s1, size_t len1, const char* s2, size_t len2 );

static inline bool isObjType( Value value, ObjType type ) {
    return IS_OBJ(value) && AS_OBJ(value)->type == type;
}
