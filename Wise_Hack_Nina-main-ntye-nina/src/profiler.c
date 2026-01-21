#include "profiler.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

ProfilerContext* profiler_init(EngineConfig *config) {
    if (!config) return NULL;

    ProfilerContext *ctx = calloc(1, sizeof(ProfilerContext));
    if (!ctx) return NULL;

    ctx->profiles = profile_table_create(config->hashtable_size);
    if (!ctx->profiles) {
        free(ctx);
        return NULL;
    }

    ctx->config = config;
    ctx->total_instructions = 0;
    ctx->instrumented_calls = 0;
    ctx->instrumented_mem_ops = 0;
    ctx->active = false;

    /* Set start time for learning phase */
    config->start_time_ms = get_timestamp_microseconds() / 1000;

    return ctx;
}

void profiler_cleanup(ProfilerContext *ctx) {
    if (!ctx) return;
    if (ctx->profiles) {
        profile_table_destroy(ctx->profiles);
    }
    free(ctx);
}

void profiler_on_function_call(ProfilerContext *ctx, void *func_addr, void *caller_addr) {
    if (!ctx || !ctx->active) return;

    /* Record the call */
    profile_record_call(ctx->profiles, func_addr);
    ctx->instrumented_calls++;

    if (ctx->config->verbose && ctx->instrumented_calls % 10000 == 0) {
        printf("[Profiler] Instrumented %lu calls, tracking %zu functions\n",
               ctx->instrumented_calls, ctx->profiles->count);
    }
}

bool profiler_is_stack_access(void *addr, void *rsp, void *rbp) {
    if (!addr || !rsp) return false;

    /* Simple heuristic: check if address is near stack pointer
     * Stack typically grows downward, so check if addr is in range
     * This is a simplified check - in production, you'd need more sophisticated detection
     */
    uintptr_t addr_val = (uintptr_t)addr;
    uintptr_t rsp_val = (uintptr_t)rsp;
    uintptr_t rbp_val = rbp ? (uintptr_t)rbp : rsp_val;

    /* Check if within typical stack frame range (conservative: 1MB) */
    uintptr_t stack_min = rsp_val < rbp_val ? rsp_val : rbp_val;
    uintptr_t stack_max = rsp_val > rbp_val ? rsp_val : rbp_val;
    stack_max += 1024 * 1024; /* Add 1MB buffer */

    return (addr_val >= stack_min && addr_val <= stack_max);
}

void profiler_on_memory_access(ProfilerContext *ctx, void *func_addr,
                               void *mem_addr, bool is_read,
                               void *stack_ptr, void *frame_ptr) {
    if (!ctx || !ctx->active) return;

    /* Determine if this is a stack or heap access */
    bool is_stack = profiler_is_stack_access(mem_addr, stack_ptr, frame_ptr);

    /* Record the memory access */
    profile_record_memory(ctx->profiles, func_addr, is_read, is_stack);
    ctx->instrumented_mem_ops++;

    if (ctx->config->verbose && ctx->instrumented_mem_ops % 100000 == 0) {
        printf("[Profiler] Instrumented %lu memory operations\n",
               ctx->instrumented_mem_ops);
    }
}

void profiler_start(ProfilerContext *ctx) {
    if (!ctx) return;
    ctx->active = true;

    if (ctx->config->verbose) {
        printf("[Profiler] Started profiling\n");
        printf("[Profiler] Learning phase: %s (%lu ms)\n",
               ctx->config->learning_phase ? "ACTIVE" : "INACTIVE",
               ctx->config->learning_duration_ms);
    }
}

void profiler_stop(ProfilerContext *ctx) {
    if (!ctx) return;
    ctx->active = false;

    if (ctx->config->verbose) {
        printf("[Profiler] Stopped profiling\n");
        profiler_print_summary(ctx);
    }
}

Statistics profiler_get_statistics(ProfilerContext *ctx) {
    if (!ctx || !ctx->profiles) {
        Statistics empty = {0};
        return empty;
    }

    return profile_compute_statistics(ctx->profiles);
}

void profiler_print_summary(ProfilerContext *ctx) {
    if (!ctx) return;

    printf("\n=== Profiler Summary ===\n");
    printf("Total Instructions:    %lu\n", ctx->total_instructions);
    printf("Instrumented Calls:    %lu\n", ctx->instrumented_calls);
    printf("Instrumented Mem Ops:  %lu\n", ctx->instrumented_mem_ops);
    printf("Functions Tracked:     %zu\n", ctx->profiles->count);
    printf("Total Calls Recorded:  %lu\n", ctx->profiles->total_calls);
    printf("Total Mem Ops:         %lu\n", ctx->profiles->total_memory_ops);

    Statistics stats = profiler_get_statistics(ctx);
    printf("\nStatistics:\n");
    printf("  Call Mean:           %.2f\n", stats.call_mean);
    printf("  Call StdDev:         %.2f\n", stats.call_stddev);
    printf("  Call P95:            %lu\n", stats.call_p95);
    printf("  Call P99:            %lu\n", stats.call_p99);
    printf("  Memory Mean:         %.2f\n", stats.mem_mean);
    printf("  Memory StdDev:       %.2f\n", stats.mem_stddev);
    printf("  Memory P95:          %lu\n", stats.mem_p95);
    printf("  Memory P99:          %lu\n", stats.mem_p99);
    printf("========================\n\n");
}
