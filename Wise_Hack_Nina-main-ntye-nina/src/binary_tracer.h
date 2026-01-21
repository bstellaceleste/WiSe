#ifndef BINARY_TRACER_H
#define BINARY_TRACER_H

#include "profiler.h"
#include <stdbool.h>
#include <stdint.h>

/* Trace a binary and collect profiling data using strace */
bool trace_binary(const char *binary_path, ProfilerContext *profiler, int max_seconds);

/* Alternative: trace library calls using ltrace */
bool trace_library_calls(const char *binary_path, ProfilerContext *profiler, int max_seconds);

/* Hash a string to a consistent address */
uintptr_t hash_string(const char *str);

#endif /* BINARY_TRACER_H */
