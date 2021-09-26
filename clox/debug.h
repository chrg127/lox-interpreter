#ifndef DEBUG_H_INCLUDED
#define DEBUG_H_INCLUDED

#ifndef NDEBUG

#define DEBUG_PRINT_CODE 1
#define DEBUG_TRACE_EXECUTION
// #define DEBUG_STRESS_GC
// #define DEBUG_LOC_GC
#define DEBUG_GC_OFF1

#else

#define DEBUG_PRINT_CODE 0

#endif

#endif
