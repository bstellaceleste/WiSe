#ifndef CONFIG_H
#define CONFIG_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/* Engine Configuration */
typedef struct {
    /* Adaptive thresholds - calculated dynamically */
    uint64_t call_threshold;          /* Adaptive: mean + k*stddev */
    uint64_t memory_threshold;        /* Adaptive: mean + k*stddev */
    float time_window_sec;            /* Time window for hotspot detection */

    /* Statistical parameters */
    float sensitivity;                /* 0.0-1.0: affects threshold calculation */
    float aggressiveness;             /* 0.0-1.0: patching aggressiveness */

    /* Learning phase */
    bool learning_phase;              /* Is engine in learning mode? */
    uint64_t learning_duration_ms;    /* Duration of learning phase */
    uint64_t start_time_ms;           /* Engine start timestamp */

    /* Performance */
    size_t max_functions;             /* Maximum functions to track */
    size_t hashtable_size;            /* Hash table capacity */

    /* Output */
    char output_file[256];            /* JSON output file */
    bool verbose;                     /* Verbose logging */

    /* Strategy */
    char strategy[32];                /* "conservative", "balanced", "aggressive" */

} EngineConfig;

/* Default configuration */
void config_init(EngineConfig *config);

/* Update configuration from command line */
bool config_parse_args(EngineConfig *config, int argc, char **argv);

/* Print configuration */
void config_print(const EngineConfig *config);

#endif /* CONFIG_H */
