#ifndef METRICS_H
#define METRICS_H

#include <stdint.h>
#include <stdbool.h>
#include <time.h>

/* Function Profile - tracks function call metrics */
typedef struct {
    void *function_addr;              /* Function address */
    char function_name[256];          /* Function name (if symbols available) */
    uint64_t call_count;              /* Number of calls */
    uint64_t last_call_timestamp;     /* Last call time (microseconds) */
    bool is_hotspot;                  /* Detected as hotspot? */
    bool is_patched;                  /* Has been patched? */
    uint64_t patch_timestamp;         /* When it was patched */
} FunctionProfile;

/* Memory Profile - tracks memory access patterns */
typedef struct {
    void *function_addr;              /* Associated function */
    uint64_t stack_reads;             /* Stack read operations */
    uint64_t stack_writes;            /* Stack write operations */
    uint64_t heap_reads;              /* Heap read operations */
    uint64_t heap_writes;             /* Heap write operations */
    uint64_t total_memory_ops;        /* Total memory operations */
} MemoryProfile;

/* Profile Table - efficient storage using hash table */
typedef struct {
    FunctionProfile *func_profiles;   /* Function profiles array */
    MemoryProfile *mem_profiles;      /* Memory profiles array */
    size_t capacity;                  /* Total capacity */
    size_t count;                     /* Current count */
    uint64_t total_calls;             /* Sum of all calls */
    uint64_t total_memory_ops;        /* Sum of all memory ops */
} ProfileTable;

/* Statistics for adaptive thresholds */
typedef struct {
    double call_mean;                 /* Mean of call counts */
    double call_stddev;               /* Standard deviation of calls */
    double mem_mean;                  /* Mean of memory operations */
    double mem_stddev;                /* Standard deviation of memory ops */
    uint64_t call_p95;                /* 95th percentile of calls */
    uint64_t call_p99;                /* 99th percentile of calls */
    uint64_t mem_p95;                 /* 95th percentile of memory ops */
    uint64_t mem_p99;                 /* 99th percentile of memory ops */
} Statistics;

/* Profile Table operations */
ProfileTable* profile_table_create(size_t capacity);
void profile_table_destroy(ProfileTable *table);

/* Get or create profile for a function */
FunctionProfile* profile_get_or_create_function(ProfileTable *table, void *func_addr);
MemoryProfile* profile_get_or_create_memory(ProfileTable *table, void *func_addr);

/* Update profiles */
void profile_record_call(ProfileTable *table, void *func_addr);
void profile_record_memory(ProfileTable *table, void *func_addr,
                          bool is_read, bool is_stack);

/* Compute statistics for adaptive thresholds */
Statistics profile_compute_statistics(const ProfileTable *table);

/* Export profiles to JSON */
bool profile_export_json(const ProfileTable *table, const char *filename,
                        const Statistics *stats);

/* Helper functions */
uint64_t get_timestamp_microseconds(void);
const char* resolve_function_name(void *addr, char *buffer, size_t size);

#endif /* METRICS_H */
