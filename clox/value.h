#pragma once
#include "common.h"

typedef enum {
    VAL_NIL, // NIL must be the 0th entry, so that a zeroed-out value is NIL (table.c depends on this behavior)
    VAL_BOOL,
    VAL_NUMBER,
    VAL_OBJ,
    VAL_ERROR,
} ValueType;

typedef struct Obj Obj;
typedef struct ObjString ObjString;

typedef enum {
    COMPILE_ERROR,
    RUNTIME_ERROR
} Error;

#ifdef NAN_BOXING
typedef uint64_t Value;
#else 
typedef struct {
    ValueType type;
    union {
        bool boolean;
        double number;
        Obj* obj;
        Error error;
    } as;
} Value;
#endif

// value constructor macros
#define NIL_VAL           ((Value){VAL_NIL, {.number = 0}})
#define BOOL_VAL(value)   ((Value){VAL_BOOL, {.boolean = value}})
#define NUMBER_VAL(value) ((Value){VAL_NUMBER, {.number = value}})
#define OBJ_VAL(object)   ((Value){VAL_OBJ, {.obj = (Obj*)object}})
#define ERROR_VAL(value)  ((Value){VAL_ERROR, {.error = value}})

// value properties
#define IS_NIL(value)     ((value).type == VAL_NIL)
#define IS_BOOL(value)    ((value).type == VAL_BOOL)
#define IS_NUMBER(value)  ((value).type == VAL_NUMBER)
#define IS_OBJ(value)     ((value).type == VAL_OBJ)
#define IS_ERROR(value)   ((value).type == VAL_ERROR)

// value casting
#define AS_BOOL(value)    ((value).as.boolean)
#define AS_NUMBER(value)  ((value).as.number)
#define AS_OBJ(value)     ((value).as.obj)
#define AS_ERROR(value)   ((value).as.error)

typedef struct {
    size_t capacity, count;
    Value* values;
} ValueArray;

void initValueArray( ValueArray* array );
void writeValueArray( ValueArray* array, Value value );
void freeValueArray( ValueArray* array );
void printValue( Value value );
bool valuesEqual( Value a, Value b );
