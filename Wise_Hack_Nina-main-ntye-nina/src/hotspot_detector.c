#include "hotspot_detector.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

HotspotDetector* hotspot_detector_init(EngineConfig *config, ProfileTable *profiles) {
    if (!config || !profiles) return NULL;

    HotspotDetector *detector = calloc(1, sizeof(HotspotDetector));
    if (!detector) return NULL;

    detector->config = config;
    detector->profiles = profiles;
    detector->hotspots_detected = 0;
    detector->last_update_time = get_timestamp_microseconds();
    detector->criteria_initialized = false;

    /* Initialize criteria with conservative defaults */
    detector->criteria.call_threshold = UINT64_MAX;
    detector->criteria.memory_threshold = UINT64_MAX;
    detector->criteria.time_window_sec = config->time_window_sec;
    detector->criteria.statistical_multiplier = 2.0; /* mean + 2*stddev */

    return detector;
}

void hotspot_detector_cleanup(HotspotDetector *detector) {
    if (!detector) return;
    free(detector);
}

void hotspot_update_criteria(HotspotDetector *detector, const Statistics *stats) {
    if (!detector || !stats) return;

    /* Calculate adaptive thresholds based on statistics
     * This is the KEY to generalization - no hardcoded values!
     */

    /* Adjust multiplier based on sensitivity
     * High sensitivity (1.0) -> lower multiplier (1.0) -> more hotspots
     * Low sensitivity (0.0) -> higher multiplier (3.0) -> fewer hotspots
     */
    double sensitivity = detector->config->sensitivity;
    detector->criteria.statistical_multiplier = 3.0 - (2.0 * sensitivity);

    /* Call threshold: mean + k * stddev */
    double call_threshold_d = stats->call_mean +
                             detector->criteria.statistical_multiplier * stats->call_stddev;
    detector->criteria.call_threshold = (uint64_t)call_threshold_d;

    /* Memory threshold: mean + k * stddev */
    double mem_threshold_d = stats->mem_mean +
                            detector->criteria.statistical_multiplier * stats->mem_stddev;
    detector->criteria.memory_threshold = (uint64_t)mem_threshold_d;

    /* Ensure minimum thresholds to avoid false positives on low-activity binaries */
    if (detector->criteria.call_threshold < 100) {
        detector->criteria.call_threshold = 100;
    }
    if (detector->criteria.memory_threshold < 1000) {
        detector->criteria.memory_threshold = 1000;
    }

    /* Alternative approach: Use percentiles for more robust detection */
    if (strcmp(detector->config->strategy, "aggressive") == 0) {
        /* Aggressive: use 95th percentile */
        if (stats->call_p95 > detector->criteria.call_threshold) {
            detector->criteria.call_threshold = stats->call_p95;
        }
    } else if (strcmp(detector->config->strategy, "conservative") == 0) {
        /* Conservative: use 99th percentile */
        if (stats->call_p99 > 0) {
            detector->criteria.call_threshold = stats->call_p99;
        }
    }

    detector->criteria_initialized = true;
    detector->last_update_time = get_timestamp_microseconds();

    if (detector->config->verbose) {
        printf("[HotspotDetector] Updated criteria:\n");
        printf("  Call Threshold:   %lu (%.2f + %.2f * %.2f)\n",
               detector->criteria.call_threshold,
               stats->call_mean,
               detector->criteria.statistical_multiplier,
               stats->call_stddev);
        printf("  Memory Threshold: %lu (%.2f + %.2f * %.2f)\n",
               detector->criteria.memory_threshold,
               stats->mem_mean,
               detector->criteria.statistical_multiplier,
               stats->mem_stddev);
    }
}

bool hotspot_is_hotspot(HotspotDetector *detector,
                       const FunctionProfile *func_prof,
                       const MemoryProfile *mem_prof) {
    if (!detector || !func_prof) return false;

    /* Don't detect hotspots if criteria not initialized */
    if (!detector->criteria_initialized) return false;

    /* Criterion 1: High call frequency */
    bool high_call_frequency = (func_prof->call_count >= detector->criteria.call_threshold);

    /* Criterion 2: High memory activity */
    bool high_memory_activity = false;
    if (mem_prof) {
        high_memory_activity = (mem_prof->total_memory_ops >= detector->criteria.memory_threshold);
    }

    /* Criterion 3: Recent activity (within time window) */
    uint64_t current_time = get_timestamp_microseconds();
    uint64_t time_since_last_call = current_time - func_prof->last_call_timestamp;
    double time_window_us = detector->criteria.time_window_sec * 1000000.0;
    bool recent_activity = (time_since_last_call < time_window_us);

    /* A hotspot is detected if:
     * - (High call frequency OR high memory activity) AND recent activity
     */
    bool is_hotspot = (high_call_frequency || high_memory_activity) && recent_activity;

    /* Additional heuristic: very high call count is always a hotspot */
    if (func_prof->call_count > detector->criteria.call_threshold * 2) {
        is_hotspot = true;
    }

    return is_hotspot;
}

void hotspot_detect_all(HotspotDetector *detector) {
    if (!detector || !detector->profiles) return;

    ProfileTable *table = detector->profiles;

    for (size_t i = 0; i < table->capacity; i++) {
        FunctionProfile *fp = &table->func_profiles[i];
        if (fp->function_addr == NULL) continue;

        MemoryProfile *mp = &table->mem_profiles[i];

        /* Check if this is a hotspot */
        bool is_hotspot = hotspot_is_hotspot(detector, fp, mp);

        if (is_hotspot && !fp->is_hotspot) {
            /* Newly detected hotspot */
            fp->is_hotspot = true;
            detector->hotspots_detected++;

            if (detector->config->verbose) {
                printf("[HotspotDetector] HOTSPOT detected: %s @ %p\n",
                       fp->function_name, fp->function_addr);
                printf("  Calls: %lu (threshold: %lu)\n",
                       fp->call_count, detector->criteria.call_threshold);
                if (mp->function_addr) {
                    printf("  Memory ops: %lu (threshold: %lu)\n",
                           mp->total_memory_ops, detector->criteria.memory_threshold);
                }
            }
        }
    }
}

const char* hotspot_get_reason(HotspotDetector *detector,
                               const FunctionProfile *func_prof,
                               const MemoryProfile *mem_prof) {
    if (!detector || !func_prof) return "unknown";

    static char reason_buffer[256];
    reason_buffer[0] = '\0';

    bool high_calls = (func_prof->call_count >= detector->criteria.call_threshold);
    bool high_mem = mem_prof && (mem_prof->total_memory_ops >= detector->criteria.memory_threshold);

    if (high_calls && high_mem) {
        snprintf(reason_buffer, sizeof(reason_buffer),
                 "high_call_frequency+high_memory_activity");
    } else if (high_calls) {
        snprintf(reason_buffer, sizeof(reason_buffer),
                 "high_call_frequency (count=%lu, threshold=%lu)",
                 func_prof->call_count, detector->criteria.call_threshold);
    } else if (high_mem) {
        snprintf(reason_buffer, sizeof(reason_buffer),
                 "high_memory_activity (ops=%lu, threshold=%lu)",
                 mem_prof->total_memory_ops, detector->criteria.memory_threshold);
    } else {
        snprintf(reason_buffer, sizeof(reason_buffer), "statistical_outlier");
    }

    return reason_buffer;
}

void hotspot_print_summary(HotspotDetector *detector) {
    if (!detector) return;

    printf("\n=== Hotspot Detection Summary ===\n");
    printf("Total Hotspots Detected: %lu\n", detector->hotspots_detected);
    printf("Criteria Initialized:    %s\n", detector->criteria_initialized ? "YES" : "NO");
    printf("Call Threshold:          %lu\n", detector->criteria.call_threshold);
    printf("Memory Threshold:        %lu\n", detector->criteria.memory_threshold);
    printf("Statistical Multiplier:  %.2f\n", detector->criteria.statistical_multiplier);
    printf("=================================\n\n");
}
