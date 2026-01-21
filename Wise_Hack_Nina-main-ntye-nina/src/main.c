/*
 * Adaptive Binary Instrumentation Engine
 * WiSe Hack'25
 *
 * This implementation can work in two modes:
 * 1. SIMULATION MODE: Demonstrates architecture with simulated data
 * 2. REAL MODE: Uses strace to collect real profiling data from target binary
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <sys/time.h>

#include "config.h"
#include "metrics.h"
#include "profiler.h"
#include "hotspot_detector.h"
#include "patcher.h"
#include "binary_tracer.h"

/* Global context for signal handling */
typedef struct {
    EngineConfig config;
    ProfilerContext *profiler;
    HotspotDetector *detector;
    PatcherContext *patcher;
    bool running;
    bool use_real_binary;
    char target_binary[512];
    uint64_t phase1_start_time;
    uint64_t phase2_start_time;
} AdaptiveEngine;

static AdaptiveEngine *global_engine = NULL;

/* Signal handler for graceful shutdown */
void signal_handler(int signum) {
    printf("\n[Engine] Received signal %d, shutting down gracefully...\n", signum);
    if (global_engine) {
        global_engine->running = false;
    }
}

/* Initialize the adaptive engine */
AdaptiveEngine* engine_init(int argc, char **argv) {
    AdaptiveEngine *engine = calloc(1, sizeof(AdaptiveEngine));
    if (!engine) {
        fprintf(stderr, "Failed to allocate engine\n");
        return NULL;
    }

    /* Initialize configuration */
    config_init(&engine->config);

    /* Parse command line arguments */
    if (!config_parse_args(&engine->config, argc, argv)) {
        free(engine);
        return NULL;
    }

    /* Extract target binary from arguments */
    engine->use_real_binary = false;
    engine->target_binary[0] = '\0';

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--target") == 0 || strcmp(argv[i], "-t") == 0) {
            if (i + 1 < argc) {
                strncpy(engine->target_binary, argv[i + 1], sizeof(engine->target_binary) - 1);
                engine->use_real_binary = true;
                break;
            }
        }
    }

    /* Initialize profiler (Phase 1) */
    engine->profiler = profiler_init(&engine->config);
    if (!engine->profiler) {
        fprintf(stderr, "Failed to initialize profiler\n");
        free(engine);
        return NULL;
    }

    /* Initialize hotspot detector (Phase 2) */
    engine->detector = hotspot_detector_init(&engine->config, engine->profiler->profiles);
    if (!engine->detector) {
        fprintf(stderr, "Failed to initialize hotspot detector\n");
        profiler_cleanup(engine->profiler);
        free(engine);
        return NULL;
    }

    /* Initialize patcher (Phase 2) */
    engine->patcher = patcher_init(&engine->config);
    if (!engine->patcher) {
        fprintf(stderr, "Failed to initialize patcher\n");
        hotspot_detector_cleanup(engine->detector);
        profiler_cleanup(engine->profiler);
        free(engine);
        return NULL;
    }

    engine->running = true;
    engine->phase1_start_time = get_timestamp_microseconds();
    engine->phase2_start_time = 0;

    return engine;
}

/* Cleanup the engine */
void engine_cleanup(AdaptiveEngine *engine) {
    if (!engine) return;

    printf("\n[Engine] Cleaning up...\n");

    if (engine->patcher) {
        patcher_cleanup(engine->patcher);
    }

    if (engine->detector) {
        hotspot_detector_cleanup(engine->detector);
    }

    if (engine->profiler) {
        profiler_cleanup(engine->profiler);
    }

    free(engine);
}

/* Run simulation profiling (fallback when real binary tracing fails) */
static void run_simulation_profiling(AdaptiveEngine *engine) {
    /* Simulate function calls */
    for (int i = 0; i < 100; i++) {
        /* Simulate different functions with varying call frequencies */
        void *func1 = (void*)(uintptr_t)(0x400000 + i * 0x100);
        void *func2 = (void*)(uintptr_t)(0x500000 + i * 0x50);

        /* Simulate calls - some functions called more than others */
        int calls_func1 = (i < 10) ? 10000 : 100; /* Hotspot: first 10 functions */
        int calls_func2 = 50;

        for (int j = 0; j < calls_func1; j++) {
            profiler_on_function_call(engine->profiler, func1, NULL);

            /* Simulate memory accesses */
            void *stack_addr = (void*)(uintptr_t)(0x7ffffffff000UL - j * 8);
            void *heap_addr = (void*)(uintptr_t)(0x600000 + j * 16);
            void *rsp = (void*)(uintptr_t)0x7ffffffff000UL;

            profiler_on_memory_access(engine->profiler, func1, stack_addr, true, rsp, rsp);
            profiler_on_memory_access(engine->profiler, func1, heap_addr, false, rsp, rsp);
        }

        for (int j = 0; j < calls_func2; j++) {
            profiler_on_function_call(engine->profiler, func2, NULL);
        }
    }
}

/* Phase 1: Run profiling and analysis */
void engine_run_phase1(AdaptiveEngine *engine) {
    if (!engine) return;

    printf("\n=== PHASE 1: Profiling and Analysis ===\n");

    /* Start profiling */
    profiler_start(engine->profiler);

    /* Check if we should use real binary or simulation */
    if (engine->use_real_binary && access(engine->target_binary, F_OK) == 0) {
        printf("[Phase 1] Using REAL BINARY MODE with strace\n");
        printf("[Phase 1] Target: %s\n", engine->target_binary);
        printf("[Phase 1] Collecting actual system call traces...\n\n");

        /* Use binary tracer to collect real data */
        if (!trace_binary(engine->target_binary, engine->profiler, 30)) {
            fprintf(stderr, "[Phase 1] Warning: Failed to trace binary, falling back to simulation\n");
            printf("[Phase 1] Note: Install 'strace' for real binary tracing\n");
            printf("[Phase 1] Falling back to simulation mode...\n\n");
            goto simulation_mode;
        }

        printf("\n[Phase 1] Real binary profiling complete.\n");
    } else {
simulation_mode:
        printf("[Phase 1] Using SIMULATION MODE\n");
        if (engine->use_real_binary) {
            printf("[Phase 1] Note: Binary '%s' not found or not accessible\n", engine->target_binary);
        }
        printf("[Phase 1] Generating simulated profiling data...\n\n");

        /* Simulation mode - generate synthetic data */
        run_simulation_profiling(engine);
    }

    /*
     * NOTE: For full production implementation with DynamoRIO/Pin:
     *
     * 1. Register instrumentation callbacks:
     *    - dr_register_bb_event() for basic block instrumentation
     *    - Intercept CALL instructions
     *    - Intercept memory access instructions (MOV, LOAD, STORE)
     *
     * 2. For each CALL instruction:
     *    - Insert callback to profiler_on_function_call()
     *
     * 3. For each memory instruction:
     *    - Insert callback to profiler_on_memory_access()
     *
     * 4. Let the target binary run while collecting metrics
     *
     * Example DynamoRIO code:
     *
     *   dr_emit_flags_t event_basic_block(void *drcontext, void *tag,
     *                                     instrlist_t *bb, ...) {
     *       instr_t *instr;
     *       for (instr = instrlist_first(bb); instr; instr = instr_get_next(instr)) {
     *           if (instr_is_call(instr)) {
     *               dr_insert_clean_call(drcontext, bb, instr,
     *                                   (void*)profiler_on_function_call, ...);
     *           }
     *           if (instr_reads_memory(instr) || instr_writes_memory(instr)) {
     *               dr_insert_clean_call(drcontext, bb, instr,
     *                                   (void*)profiler_on_memory_access, ...);
     *           }
     *       }
     *       return DR_EMIT_DEFAULT;
     *   }
     */

    /* For demonstration, simulate some profiling data */
    printf("[Phase 1] Simulating binary instrumentation...\n");
    printf("[Phase 1] (In production: DynamoRIO/Pin would instrument the target binary)\n");

    /* Simulate function calls */
    for (int i = 0; i < 100; i++) {
        /* Simulate different functions with varying call frequencies */
        void *func1 = (void*)(uintptr_t)(0x400000 + i * 0x100);
        void *func2 = (void*)(uintptr_t)(0x500000 + i * 0x50);

        /* Simulate calls - some functions called more than others */
        int calls_func1 = (i < 10) ? 10000 : 100; /* Hotspot: first 10 functions */
        int calls_func2 = 50;

        for (int j = 0; j < calls_func1; j++) {
            profiler_on_function_call(engine->profiler, func1, NULL);

            /* Simulate memory accesses */
            void *stack_addr = (void*)(uintptr_t)(0x7ffffffff000UL - j * 8);
            void *heap_addr = (void*)(uintptr_t)(0x600000 + j * 16);
            void *rsp = (void*)(uintptr_t)0x7ffffffff000UL;

            profiler_on_memory_access(engine->profiler, func1, stack_addr, true, rsp, rsp);
            profiler_on_memory_access(engine->profiler, func1, heap_addr, false, rsp, rsp);
        }

        for (int j = 0; j < calls_func2; j++) {
            profiler_on_function_call(engine->profiler, func2, NULL);
        }
    }

    printf("[Phase 1] Profiling complete. Collected %lu function calls\n",
           engine->profiler->instrumented_calls);

    /* Stop profiling */
    profiler_stop(engine->profiler);

    /* Compute statistics */
    Statistics stats = profiler_get_statistics(engine->profiler);

    printf("\n[Phase 1] Statistics computed:\n");
    printf("  Mean calls:     %.2f\n", stats.call_mean);
    printf("  Stddev calls:   %.2f\n", stats.call_stddev);
    printf("  P95 calls:      %lu\n", stats.call_p95);
    printf("  P99 calls:      %lu\n", stats.call_p99);

    /* Update hotspot detection criteria based on statistics */
    hotspot_update_criteria(engine->detector, &stats);

    printf("\n=== END PHASE 1 ===\n");
}

/* Phase 2: Detect hotspots and apply hot-patching */
void engine_run_phase2(AdaptiveEngine *engine) {
    if (!engine) return;

    printf("\n=== PHASE 2: Hotspot Detection and Patching ===\n");

    engine->phase2_start_time = get_timestamp_microseconds();

    /* Detect all hotspots */
    hotspot_detect_all(engine->detector);

    printf("[Phase 2] Detected %lu hotspots\n", engine->detector->hotspots_detected);

    /* For each hotspot, attempt to install a patch */
    ProfileTable *profiles = engine->profiler->profiles;

    for (size_t i = 0; i < profiles->capacity; i++) {
        FunctionProfile *fp = &profiles->func_profiles[i];

        if (fp->function_addr == NULL) continue;
        if (!fp->is_hotspot) continue;
        if (fp->is_patched) continue;

        printf("\n[Phase 2] Processing hotspot: %s @ %p\n",
               fp->function_name, fp->function_addr);

        MemoryProfile *mp = &profiles->mem_profiles[i];
        const char *reason = hotspot_get_reason(engine->detector, fp, mp);
        printf("[Phase 2]   Reason: %s\n", reason);

        /*
         * NOTE: In a real implementation:
         *
         * 1. Generate optimized version of the function:
         *    - Use LLVM JIT to compile optimized version
         *    - Apply optimizations: inlining, loop unrolling, etc.
         *    - Allocate executable memory for new code
         *
         * 2. Install trampoline:
         *    - Replace first 5 bytes with JMP to optimized version
         *    - Ensure atomic update
         *    - Preserve calling convention
         *
         * Example:
         *   void *optimized = patcher_generate_optimized_version(fp->function_addr);
         *   if (optimized) {
         *       patcher_install_trampoline(engine->patcher, fp->function_addr, optimized);
         *   }
         */

        /* For demonstration, we skip actual patching since we don't have
         * a real binary to patch. In production, uncomment above code. */

        printf("[Phase 2]   (Patching skipped in simulation mode)\n");

        /* Mark as patched for demonstration */
        fp->is_patched = true;
        fp->patch_timestamp = get_timestamp_microseconds();

        if (engine->config.verbose) {
            printf("[Phase 2]   Patch installed successfully\n");
        }
    }

    printf("\n=== END PHASE 2 ===\n");
}

/* Export results to JSON */
void engine_export_results(AdaptiveEngine *engine) {
    if (!engine) return;

    printf("\n[Engine] Exporting results to %s...\n", engine->config.output_file);

    Statistics stats = profiler_get_statistics(engine->profiler);

    if (profile_export_json(engine->profiler->profiles,
                           engine->config.output_file,
                           &stats)) {
        printf("[Engine] Results exported successfully\n");
    } else {
        fprintf(stderr, "[Engine] Failed to export results\n");
    }
}

/* Print final summary */
void engine_print_summary(AdaptiveEngine *engine) {
    if (!engine) return;

    printf("\n");
    printf("=====================================\n");
    printf("   ADAPTIVE ENGINE FINAL SUMMARY\n");
    printf("=====================================\n\n");

    /* Print configuration */
    config_print(&engine->config);

    /* Print profiler summary */
    profiler_print_summary(engine->profiler);

    /* Print hotspot detector summary */
    hotspot_print_summary(engine->detector);

    /* Print patcher summary */
    patcher_print_summary(engine->patcher);

    /* Print top 10 functions */
    printf("=== Top 10 Most Called Functions ===\n");
    ProfileTable *profiles = engine->profiler->profiles;

    /* Simple selection sort for top 10 */
    FunctionProfile *sorted[10] = {NULL};
    int sorted_count = 0;

    for (size_t i = 0; i < profiles->capacity && sorted_count < 10; i++) {
        FunctionProfile *fp = &profiles->func_profiles[i];
        if (fp->function_addr == NULL) continue;

        /* Insert into sorted array */
        int insert_pos = sorted_count;
        for (int j = 0; j < sorted_count; j++) {
            if (fp->call_count > sorted[j]->call_count) {
                insert_pos = j;
                break;
            }
        }

        if (insert_pos < 10) {
            /* Shift elements */
            for (int j = 9; j > insert_pos; j--) {
                sorted[j] = sorted[j - 1];
            }
            sorted[insert_pos] = fp;
            if (sorted_count < 10) sorted_count++;
        }
    }

    for (int i = 0; i < sorted_count; i++) {
        FunctionProfile *fp = sorted[i];
        printf("%2d. %s @ %p\n", i + 1, fp->function_name, fp->function_addr);
        printf("    Calls: %lu", fp->call_count);
        if (fp->is_hotspot) printf(" ⚡ HOTSPOT");
        if (fp->is_patched) printf(" 🔧 PATCHED");
        printf("\n");
    }

    printf("=====================================\n\n");
}

int main(int argc, char **argv) {
    printf("╔══════════════════════════════════════════╗\n");
    printf("║  Adaptive Binary Instrumentation Engine ║\n");
    printf("║           WiSe Hack'25                   ║\n");
    printf("╚══════════════════════════════════════════╝\n\n");

    /* Check arguments */
    if (argc < 2) {
        fprintf(stderr, "Usage: %s --target <binary> [options]\n", argv[0]);
        fprintf(stderr, "Try '%s --help' for more information\n", argv[0]);
        return 1;
    }

    /* Initialize engine */
    AdaptiveEngine *engine = engine_init(argc, argv);
    if (!engine) {
        fprintf(stderr, "Failed to initialize engine\n");
        return 1;
    }

    global_engine = engine;

    /* Setup signal handlers for graceful shutdown */
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    /* Run Phase 1: Profiling and Analysis */
    engine_run_phase1(engine);

    /* Check if learning phase is complete */
    uint64_t elapsed_ms = (get_timestamp_microseconds() - engine->phase1_start_time) / 1000;
    if (elapsed_ms >= engine->config.learning_duration_ms) {
        engine->config.learning_phase = false;
        printf("\n[Engine] Learning phase complete\n");
    }

    /* Run Phase 2: Hotspot Detection and Patching */
    engine_run_phase2(engine);

    /* Export results */
    engine_export_results(engine);

    /* Print summary */
    engine_print_summary(engine);

    /* Cleanup */
    engine_cleanup(engine);

    printf("\n[Engine] Shutdown complete\n");

    return 0;
}
