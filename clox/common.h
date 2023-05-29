#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#define UINT8_COUNT (UINT8_MAX + 1)

// optimization
#define NAN_BOXING

// compilation
//#define DEBUG_PRINT_SCAN
//#define DEBUG_PRINT_CODE

// VM
//#define DEBUG_TRACE_EXECUTION

// garbage collection
#define DEBUG_STRESS_GC // keep this on for now, since it's the best way to find GC bugs
//#define DEBUG_LOG_GC // only log if we notice problems in execution
