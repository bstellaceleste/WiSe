#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>

void config_init(EngineConfig *config) {
    if (!config) return;

    /* Initialize with safe defaults */
    config->call_threshold = 0;           /* Will be computed adaptively */
    config->memory_threshold = 0;         /* Will be computed adaptively */
    config->time_window_sec = 5.0;        /* 5 second window */

    /* Statistical parameters */
    config->sensitivity = 0.5;            /* Balanced sensitivity */
    config->aggressiveness = 0.5;         /* Balanced aggressiveness */

    /* Learning phase - observe first 10 seconds */
    config->learning_phase = true;
    config->learning_duration_ms = 10000; /* 10 seconds */
    config->start_time_ms = 0;

    /* Performance */
    config->max_functions = 10000;        /* Track up to 10k functions */
    config->hashtable_size = 16384;       /* Hash table size */

    /* Output */
    strcpy(config->output_file, "results.json");
    config->verbose = false;

    /* Strategy */
    strcpy(config->strategy, "balanced");
}

bool config_parse_args(EngineConfig *config, int argc, char **argv) {
    static struct option long_options[] = {
        {"target",      required_argument, 0, 't'},
        {"output",      required_argument, 0, 'o'},
        {"threshold",   required_argument, 0, 'T'},
        {"metrics",     required_argument, 0, 'm'},
        {"verbose",     no_argument,       0, 'v'},
        {"sensitivity", required_argument, 0, 's'},
        {"strategy",    required_argument, 0, 'S'},
        {"help",        no_argument,       0, 'h'},
        {0, 0, 0, 0}
    };

    int opt;
    int option_index = 0;

    while ((opt = getopt_long(argc, argv, "t:o:T:m:vs:S:h", long_options, &option_index)) != -1) {
        switch (opt) {
            case 't':
                /* Target binary handled in main */
                break;
            case 'o':
                strncpy(config->output_file, optarg, sizeof(config->output_file) - 1);
                break;
            case 'T':
                if (strcmp(optarg, "auto") == 0) {
                    /* Keep adaptive thresholds */
                } else {
                    config->call_threshold = atoll(optarg);
                }
                break;
            case 's':
                config->sensitivity = atof(optarg);
                if (config->sensitivity < 0.0) config->sensitivity = 0.0;
                if (config->sensitivity > 1.0) config->sensitivity = 1.0;
                break;
            case 'S':
                strncpy(config->strategy, optarg, sizeof(config->strategy) - 1);
                break;
            case 'v':
                config->verbose = true;
                break;
            case 'h':
                printf("Adaptive Binary Instrumentation Engine\n");
                printf("Usage: %s --target <binary> [options]\n", argv[0]);
                printf("\nOptions:\n");
                printf("  --target <binary>       Target binary to instrument\n");
                printf("  --output <file>         Output JSON file (default: results.json)\n");
                printf("  --threshold <N|auto>    Call threshold (default: auto)\n");
                printf("  --sensitivity <0-1>     Detection sensitivity (default: 0.5)\n");
                printf("  --strategy <name>       Strategy: conservative/balanced/aggressive\n");
                printf("  --verbose               Verbose output\n");
                printf("  --help                  Show this help\n");
                return false;
            default:
                return false;
        }
    }

    return true;
}

void config_print(const EngineConfig *config) {
    printf("=== Engine Configuration ===\n");
    printf("Call Threshold:     %lu %s\n", config->call_threshold,
           config->call_threshold == 0 ? "(adaptive)" : "");
    printf("Memory Threshold:   %lu %s\n", config->memory_threshold,
           config->memory_threshold == 0 ? "(adaptive)" : "");
    printf("Time Window:        %.1f sec\n", config->time_window_sec);
    printf("Sensitivity:        %.2f\n", config->sensitivity);
    printf("Strategy:           %s\n", config->strategy);
    printf("Max Functions:      %zu\n", config->max_functions);
    printf("Output File:        %s\n", config->output_file);
    printf("Verbose:            %s\n", config->verbose ? "yes" : "no");
    printf("============================\n\n");
}
