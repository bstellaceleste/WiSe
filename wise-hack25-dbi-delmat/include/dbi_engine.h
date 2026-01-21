#ifndef DBI_ENGINE_H
#define DBI_ENGINE_H

#include <stdint.h>
#include "dr_api.h"      /* Provides 'bool' and 'app_pc' */
#include "drmgr.h"
#include "drwrap.h"
#include "drutil.h"

/* ========== CONFIGURATION CONSTANTS ========== */
#define MAX_FUNCTIONS 64

/* ========== DATA STRUCTURES ========== */

/* Function statistics */
typedef struct {
    const char *name;           /* Function name */
    app_pc start_addr;          /* Function start address */
    app_pc optimized_addr;      /* Optimized version address */
    uint64_t call_count;        /* Number of calls */
    uint64_t memory_ops;        /* Memory operations count */
    uint64_t last_call_time;    /* Last call timestamp (microseconds) */
    bool is_redirected;         /* Has been redirected */
    bool is_candidate;          /* Is hotspot candidate */
} function_stats_t;

/* Global statistics */
typedef struct {
    function_stats_t functions[MAX_FUNCTIONS]; /* Tracked functions */
    int func_count;             /* Number of tracked functions */
    uint64_t total_calls;       /* Total function calls */
    uint64_t total_memory_ops;  /* Total memory operations */
    uint64_t analysis_cycles;   /* Number of analysis cycles */
    void *lock;                 /* Statistics lock */
} global_stats_t;

/* ========== PROFILING API (PHASE 1) ========== */

/* Statistics management */
function_stats_t* profile_get_or_create_function(app_pc addr, const char *name);
void profile_count_call(app_pc func_addr);
void profile_count_memory(app_pc func_addr, uint num_ops);
void profile_dump_stats(void);

/* ========== ADAPTATION API (PHASE 2) ========== */

/* Hotspot detection and adaptation */
bool adaptation_is_hotspot(function_stats_t *func);
app_pc adaptation_find_optimized(const char *original_name);
bool adaptation_redirect_function(function_stats_t *func);

/* ========== INSTRUMENTATION CALLBACKS ========== */

/* DynamoRIO callbacks */
void module_load_callback(void *drcontext, const module_data_t *mod, bool loaded);
dr_emit_flags_t instrumentation_callback(void *drcontext, void *tag, 
    instrlist_t *bb, instr_t *inst, 
    bool for_trace, bool translating, 
    void *user_data);

/* ========== ENGINE MANAGEMENT ========== */

/* Engine lifecycle */
void engine_init(void);
void engine_exit(void);
bool engine_is_running(void);

/* Statistics access */
global_stats_t* engine_get_stats(void);

/* Control functions */
bool engine_force_adaptation(const char *func_name);
bool engine_rollback_all(void);
function_stats_t* engine_get_function(const char *func_name);
function_stats_t* engine_get_function_by_addr(app_pc addr);

/* ========== DYNAMORIO ENTRY POINT ========== */
DR_EXPORT void dr_client_main(client_id_t id, int argc, const char *argv[]);

#endif /* DBI_ENGINE_H */