#include <stdint.h>
#ifndef CONFIG_H
#define CONFIG_H

#include "dr_api.h"

/* Profile modes */
typedef enum {
    PROFILE_MODE_NONE,        // No profiling
    PROFILE_MODE_LIGHT,       // Light profiling (default)
    PROFILE_MODE_FULL,        // Full profiling
    PROFILE_MODE_AGGRESSIVE   // Aggressive profiling
} profile_mode_t;

/* Adaptation strategies */
typedef enum {
    ADAPT_STRATEGY_CONSERVATIVE,  // Conservative adaptation
    ADAPT_STRATEGY_BALANCED,      // Balanced adaptation
    ADAPT_STRATEGY_AGGRESSIVE,    // Aggressive adaptation
    ADAPT_STRATEGY_ADAPTIVE       // Adaptive based on load
} adaptation_strategy_t;

/* Configuration structure */
typedef struct {
    /* Hotspot detection */
    double hotspot_multiplier;          /* Multiplicateur pour seuil dynamique */
    int min_calls;                      /* Appels minimum avant analyse */
    
    /* Analysis settings */
    int analysis_interval;              /* Intervalle d'analyse en ms */
    bool enable_memory_tracking;        /* Activer suivi mémoire */
    bool enable_branch_tracking;        /* Activer suivi branches (bonus) */
    
    /* Logging and debugging */
    bool verbose_logging;               /* Logs détaillés */
    bool log_to_file;                   /* Log to file instead of stderr */
    
    /* Security and limits */
    bool allow_external_patch;          /* Permettre patch externe */
    int max_functions;                  /* Maximum functions to track */
    
    /* Advanced thresholds */
    double memory_threshold_multiplier; /* Multiplier for memory operations */
    int call_window_size;               /* Sliding window size for call counting */
    int hotspot_cooldown_ms;            /* Cooldown period after hotspot detection */
    
    /* Operation modes */
    profile_mode_t profile_mode;        /* Profiling mode */
    adaptation_strategy_t adaptation_strategy; /* Adaptation strategy */
} engine_config_t;

/* Initialization */
void config_init(void);
void config_cleanup(void);

/* Environment configuration */
void config_from_env(void);

/* Configuration access */
engine_config_t* config_get(void);
void config_dump(void);

/* Dynamic threshold calculation */
double config_get_dynamic_threshold(double avg_calls);
uint64_t config_get_memory_threshold(double avg_mem_ops);

/* Adaptation decision */
bool config_should_adapt_function(uint64_t call_count, uint64_t memory_ops,
                                  double avg_calls, double avg_memory);

/* Configuration queries */
bool config_is_verbose(void);
bool config_track_memory(void);
bool config_track_branches(void);
int config_get_analysis_interval(void);
int config_get_max_functions(void);
int config_get_cooldown_ms(void);
bool config_allow_external_patch(void);
double config_get_overhead_factor(void);

/* Dynamic configuration */
bool config_update(const char *key, const char *value);
void config_reset(void);
char* config_export(void);
bool config_import(const char *config_str);

#endif /* CONFIG_H */