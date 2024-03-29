#pragma once
#include <string.h>
#include "common.h"

typedef struct Obj Obj;
typedef struct ObjString ObjString;

#ifdef NAN_BOXING // -- With NaN Boxing --
// constants
#define SIGN_BIT ((uint64_t)0x8000000000000000)
#define QNAN     ((uint64_t)0x7ffc000000000000)
#define TAG_NIL   1 // 01.
#define TAG_FALSE 2 // 10.
#define TAG_TRUE  3 // 11.
#define COMPILE_ERROR 4 // 0100. // <-- EP added this in. If sign bit is unset, & we have a qnan, then we can store a lot of numeric values here that are not numbers.
#define RUNTIME_ERROR 8 // 1100.

// value type
typedef uint64_t Value;

// value constructors
#define NIL_VAL             ((Value)(uint64_t)(QNAN | TAG_NIL))
#define FALSE_VAL           ((Value)(uint64_t)(QNAN | TAG_FALSE))
#define TRUE_VAL            ((Value)(uint64_t)(QNAN | TAG_TRUE))
#define BOOL_VAL(b)         ((b) ? TRUE_VAL : FALSE_VAL)
#define NUMBER_VAL(num)     numToValue(num)
#define OBJ_VAL(obj)        ((Value)(SIGN_BIT | QNAN | (uint64_t)(uintptr_t)(obj)))
#define ERROR_VAL(err)      ((Value)(uint64_t)(QNAN | err))

// value properties
#define IS_NIL(value)       ((value) == NIL_VAL)
#define IS_BOOL(value)      (((value) | 1) == TRUE_VAL)
#define IS_NUMBER(value)    (((value) & QNAN) != QNAN)
#define IS_OBJ(value)       (((value) & (QNAN | SIGN_BIT)) == (QNAN | SIGN_BIT))
#define IS_ERROR(value)     (((value) | 8) == (QNAN | RUNTIME_ERROR))

// value casting
#define AS_BOOL(value)      ((value) == TRUE_VAL)
#define AS_NUMBER(value)    valueToNum(value)
#define AS_OBJ(value)       ((Obj*)(uintptr_t)((value) & ~(SIGN_BIT | QNAN)))
#define AS_ERROR(value)     ((int)(((value) & 12) >> 2))

static inline Value numToValue( double num ) { // seems slow, but compiler should convert this function into a simply copy
    Value value;
    memcpy( &value, &num, sizeof(double) );
    return value;
}

static inline double valueToNum( Value value ) { // seems slow, but compiler should convert this function into a simply copy
    double num;
    memcpy( &num, &value, sizeof( Value ) );
    return num;
}

#else // -- Without NaN Boxing --

typedef enum {
    VAL_NIL, // NIL must be the 0th entry, so that a zeroed-out value is NIL (table.c depends on this behavior)
    VAL_BOOL,
    VAL_NUMBER,
    VAL_OBJ,
    VAL_ERROR,
} ValueType;

typedef enum {
    COMPILE_ERROR,
    RUNTIME_ERROR
} Error;

typedef struct {
    ValueType type;
    union {
        bool boolean;
        double number;
        Obj* obj;
        Error error;
    } as;
} Value;

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

#endif

typedef struct {
    size_t capacity, count;
    Value* values;
} ValueArray;

void initValueArray( ValueArray* array );
void writeValueArray( ValueArray* array, Value value );
void freeValueArray( ValueArray* array );
void printValue( Value value );
bool valuesEqual( Value a, Value b );
