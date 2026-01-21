#ifndef HOTSPOT_DETECTOR_H
#define HOTSPOT_DETECTOR_H

#include "metrics.h"
#include "config.h"
#include <stdbool.h>

/* Hotspot detection criteria - computed adaptively */
typedef struct {
    uint64_t call_threshold;          /* Adaptive threshold for calls */
    uint64_t memory_threshold;        /* Adaptive threshold for memory */
    float time_window_sec;            /* Time window for recency */
    double statistical_multiplier;    /* Multiplier for mean + k*stddev */
} HotspotCriteria;

/* Hotspot detector context */
typedef struct {
    HotspotCriteria criteria;         /* Current criteria */
    EngineConfig *config;             /* Configuration reference */
    ProfileTable *profiles;           /* Profile data reference */
    uint64_t hotspots_detected;       /* Total hotspots detected */
    uint64_t last_update_time;        /* Last criteria update */
    bool criteria_initialized;        /* Are criteria initialized? */
} HotspotDetector;

/* Initialize hotspot detector */
HotspotDetector* hotspot_detector_init(EngineConfig *config, ProfileTable *profiles);

/* Cleanup detector */
void hotspot_detector_cleanup(HotspotDetector *detector);

/* Update adaptive criteria based on current statistics */
void hotspot_update_criteria(HotspotDetector *detector, const Statistics *stats);

/* Check if a function is a hotspot */
bool hotspot_is_hotspot(HotspotDetector *detector,
                       const FunctionProfile *func_prof,
                       const MemoryProfile *mem_prof);

/* Detect all hotspots in profile table */
void hotspot_detect_all(HotspotDetector *detector);

/* Get hotspot detection reason (for logging) */
const char* hotspot_get_reason(HotspotDetector *detector,
                               const FunctionProfile *func_prof,
                               const MemoryProfile *mem_prof);

/* Print hotspot detection summary */
void hotspot_print_summary(HotspotDetector *detector);

#endif /* HOTSPOT_DETECTOR_H */
