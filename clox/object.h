#pragma once
#include "common.h"
#include "chunk.h"
#include "value.h"

#define OBJ_TYPE(value)         (AS_OBJ(value)->type)
#define IS_STRING(value)        isObjType(value, OBJ_STRING)
#define IS_FUNCTION(value)      isObjType(value, OBJ_FUNCTION)
#define AS_STRING(value)        ((ObjString*)AS_OBJ(value))
#define AS_FUNCTION(value)      ((ObjFunction*)AS_OBJ(value))
#define HASH_SEED 2166136261u
#define HASH_PRIME 16777619

typedef enum {
    OBJ_STRING,
    OBJ_FUNCTION,
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

typedef struct { // TODO: why do we ahve to make this a typedef? otherwise compiler is not happy...
    Obj obj;
    int arity;
    Chunk chunk;
    ObjString* name;
} ObjFunction;

// objects
void printObject( Obj* obj );
static inline bool isObjType( Value value, ObjType type ) { return IS_OBJ(value) && AS_OBJ(value)->type == type; }

// strings
void printString( ObjString* s );
ObjString* makeString( const char* s, size_t len );
ObjString* concatStrings( const char* s1, size_t len1, const char* s2, size_t len2 );

// functions
void printFunction( ObjFunction* f );
ObjFunction* newFunction();
