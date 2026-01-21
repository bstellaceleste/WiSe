#ifndef PROFILER_H
#define PROFILER_H

#include "metrics.h"
#include "config.h"
#include <stdbool.h>

/* Profiler context */
typedef struct {
    ProfileTable *profiles;           /* Profile data */
    EngineConfig *config;             /* Configuration */
    uint64_t total_instructions;      /* Total instructions executed */
    uint64_t instrumented_calls;      /* Number of instrumented calls */
    uint64_t instrumented_mem_ops;    /* Number of instrumented memory ops */
    bool active;                      /* Is profiler active? */
} ProfilerContext;

/* Initialize profiler */
ProfilerContext* profiler_init(EngineConfig *config);

/* Cleanup profiler */
void profiler_cleanup(ProfilerContext *ctx);

/* Callbacks for instrumentation */
void profiler_on_function_call(ProfilerContext *ctx, void *func_addr, void *caller_addr);
void profiler_on_memory_access(ProfilerContext *ctx, void *func_addr,
                               void *mem_addr, bool is_read,
                               void *stack_ptr, void *frame_ptr);

/* Check if address is on stack */
bool profiler_is_stack_access(void *addr, void *rsp, void *rbp);

/* Start/stop profiling */
void profiler_start(ProfilerContext *ctx);
void profiler_stop(ProfilerContext *ctx);

/* Get current statistics */
Statistics profiler_get_statistics(ProfilerContext *ctx);

/* Print profiler summary */
void profiler_print_summary(ProfilerContext *ctx);

#endif /* PROFILER_H */
