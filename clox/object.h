#pragma once
#include "common.h"
#include "chunk.h"
#include "value.h"

#define OBJ_TYPE(value)         (AS_OBJ(value)->type)
#define IS_STRING(value)        isObjType(value, OBJ_STRING)
#define IS_FUNCTION(value)      isObjType(value, OBJ_FUNCTION)
#define IS_NATIVE(value)        isObjType(value, OBJ_NATIVE)
#define IS_CLOSURE(value)       isObjType(value, OBJ_CLOSURE)
#define AS_STRING(value)        ((ObjString*)AS_OBJ(value))
#define AS_FUNCTION(value)      ((ObjFunction*)AS_OBJ(value))
#define AS_NATIVE(value)        (((ObjNative*)AS_OBJ(value))->function)
#define AS_CLOSURE(value)       ((ObjClosure*)AS_OBJ(value))
#define HASH_SEED 2166136261u
#define HASH_PRIME 16777619

typedef enum {
    OBJ_STRING,
    OBJ_UPVALUE,
    OBJ_FUNCTION,
    OBJ_NATIVE,
    OBJ_CLOSURE,
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

typedef struct ObjUpvalue {
    Obj obj;
    Value* location;
} ObjUpvalue;

typedef struct {
    Obj obj;
    int arity, upvalueCount;
    Chunk chunk;
    ObjString* name;
} ObjFunction;

// native function object
typedef Value (*NativeFn)(int argCount, Value* args);

typedef struct {
    Obj obj;
    NativeFn function;
} ObjNative;

// closure object
typedef struct {
    Obj obj;
    ObjFunction* function;
    ObjUpvalue** upvalues;
    int upvalueCount;
} ObjClosure;

// objects
void printObject( Obj* obj );
static inline bool isObjType( Value value, ObjType type ) { return IS_OBJ(value) && AS_OBJ(value)->type == type; }

// strings
void printString( ObjString* s );
void printStringToErr( ObjString* s );
ObjString* makeString( const char* s, size_t len );
ObjString* concatStrings( const char* s1, size_t len1, const char* s2, size_t len2 );

// upvalues
void printUpvalue( ObjUpvalue* upvalue );
ObjUpvalue* newUpvalue( Value* slot );

// functions
void printFunction( ObjFunction* f );
ObjFunction* newFunction();

// native functions
ObjNative* newNative( NativeFn function );

// closures
ObjClosure* newClosure( ObjFunction* function );
