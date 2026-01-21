#include "metrics.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <sys/time.h>
#include <execinfo.h>

#define _GNU_SOURCE
#include <dlfcn.h>

/* Hash function for addresses */
static size_t hash_address(void *addr, size_t capacity) {
    uintptr_t val = (uintptr_t)addr;
    /* Simple hash: multiply by prime and modulo capacity */
    return (val * 2654435761UL) % capacity;
}

ProfileTable* profile_table_create(size_t capacity) {
    ProfileTable *table = calloc(1, sizeof(ProfileTable));
    if (!table) return NULL;

    table->func_profiles = calloc(capacity, sizeof(FunctionProfile));
    table->mem_profiles = calloc(capacity, sizeof(MemoryProfile));

    if (!table->func_profiles || !table->mem_profiles) {
        free(table->func_profiles);
        free(table->mem_profiles);
        free(table);
        return NULL;
    }

    table->capacity = capacity;
    table->count = 0;
    table->total_calls = 0;
    table->total_memory_ops = 0;

    return table;
}

void profile_table_destroy(ProfileTable *table) {
    if (!table) return;
    free(table->func_profiles);
    free(table->mem_profiles);
    free(table);
}

FunctionProfile* profile_get_or_create_function(ProfileTable *table, void *func_addr) {
    if (!table || !func_addr) return NULL;

    size_t hash = hash_address(func_addr, table->capacity);
    size_t index = hash;

    /* Linear probing */
    for (size_t i = 0; i < table->capacity; i++) {
        FunctionProfile *prof = &table->func_profiles[index];

        if (prof->function_addr == NULL) {
            /* Empty slot - create new profile */
            prof->function_addr = func_addr;
            resolve_function_name(func_addr, prof->function_name, sizeof(prof->function_name));
            prof->call_count = 0;
            prof->last_call_timestamp = 0;
            prof->is_hotspot = false;
            prof->is_patched = false;
            prof->patch_timestamp = 0;
            table->count++;
            return prof;
        }

        if (prof->function_addr == func_addr) {
            /* Found existing profile */
            return prof;
        }

        /* Collision - try next slot */
        index = (index + 1) % table->capacity;
    }

    /* Table full */
    return NULL;
}

MemoryProfile* profile_get_or_create_memory(ProfileTable *table, void *func_addr) {
    if (!table || !func_addr) return NULL;

    size_t hash = hash_address(func_addr, table->capacity);
    size_t index = hash;

    /* Linear probing */
    for (size_t i = 0; i < table->capacity; i++) {
        MemoryProfile *prof = &table->mem_profiles[index];

        if (prof->function_addr == NULL) {
            /* Empty slot - create new profile */
            prof->function_addr = func_addr;
            prof->stack_reads = 0;
            prof->stack_writes = 0;
            prof->heap_reads = 0;
            prof->heap_writes = 0;
            prof->total_memory_ops = 0;
            return prof;
        }

        if (prof->function_addr == func_addr) {
            /* Found existing profile */
            return prof;
        }

        /* Collision - try next slot */
        index = (index + 1) % table->capacity;
    }

    return NULL;
}

void profile_record_call(ProfileTable *table, void *func_addr) {
    FunctionProfile *prof = profile_get_or_create_function(table, func_addr);
    if (prof) {
        prof->call_count++;
        prof->last_call_timestamp = get_timestamp_microseconds();
        table->total_calls++;
    }
}

void profile_record_memory(ProfileTable *table, void *func_addr,
                          bool is_read, bool is_stack) {
    MemoryProfile *prof = profile_get_or_create_memory(table, func_addr);
    if (prof) {
        if (is_stack) {
            if (is_read) prof->stack_reads++;
            else prof->stack_writes++;
        } else {
            if (is_read) prof->heap_reads++;
            else prof->heap_writes++;
        }
        prof->total_memory_ops++;
        table->total_memory_ops++;
    }
}

Statistics profile_compute_statistics(const ProfileTable *table) {
    Statistics stats = {0};

    if (!table || table->count == 0) return stats;

    /* Compute call statistics */
    uint64_t call_sum = 0;
    uint64_t mem_sum = 0;
    uint64_t *call_counts = malloc(table->count * sizeof(uint64_t));
    uint64_t *mem_counts = malloc(table->count * sizeof(uint64_t));
    size_t valid_count = 0;

    for (size_t i = 0; i < table->capacity && valid_count < table->count; i++) {
        if (table->func_profiles[i].function_addr != NULL) {
            call_counts[valid_count] = table->func_profiles[i].call_count;
            call_sum += call_counts[valid_count];

            if (table->mem_profiles[i].function_addr != NULL) {
                mem_counts[valid_count] = table->mem_profiles[i].total_memory_ops;
                mem_sum += mem_counts[valid_count];
            } else {
                mem_counts[valid_count] = 0;
            }

            valid_count++;
        }
    }

    /* Mean */
    stats.call_mean = (double)call_sum / valid_count;
    stats.mem_mean = (double)mem_sum / valid_count;

    /* Standard deviation */
    double call_variance = 0;
    double mem_variance = 0;

    for (size_t i = 0; i < valid_count; i++) {
        double call_diff = call_counts[i] - stats.call_mean;
        call_variance += call_diff * call_diff;

        double mem_diff = mem_counts[i] - stats.mem_mean;
        mem_variance += mem_diff * mem_diff;
    }

    stats.call_stddev = sqrt(call_variance / valid_count);
    stats.mem_stddev = sqrt(mem_variance / valid_count);

    /* Percentiles - simple sorting approach */
    /* Sort call_counts for percentiles */
    for (size_t i = 0; i < valid_count - 1; i++) {
        for (size_t j = i + 1; j < valid_count; j++) {
            if (call_counts[i] > call_counts[j]) {
                uint64_t temp = call_counts[i];
                call_counts[i] = call_counts[j];
                call_counts[j] = temp;
            }
        }
    }

    size_t p95_idx = (size_t)(valid_count * 0.95);
    size_t p99_idx = (size_t)(valid_count * 0.99);
    stats.call_p95 = call_counts[p95_idx < valid_count ? p95_idx : valid_count - 1];
    stats.call_p99 = call_counts[p99_idx < valid_count ? p99_idx : valid_count - 1];

    /* Sort mem_counts for percentiles */
    for (size_t i = 0; i < valid_count - 1; i++) {
        for (size_t j = i + 1; j < valid_count; j++) {
            if (mem_counts[i] > mem_counts[j]) {
                uint64_t temp = mem_counts[i];
                mem_counts[i] = mem_counts[j];
                mem_counts[j] = temp;
            }
        }
    }

    stats.mem_p95 = mem_counts[p95_idx < valid_count ? p95_idx : valid_count - 1];
    stats.mem_p99 = mem_counts[p99_idx < valid_count ? p99_idx : valid_count - 1];

    free(call_counts);
    free(mem_counts);

    return stats;
}

bool profile_export_json(const ProfileTable *table, const char *filename,
                        const Statistics *stats) {
    if (!table || !filename) return false;

    FILE *f = fopen(filename, "w");
    if (!f) return false;

    fprintf(f, "{\n");
    fprintf(f, "  \"engine_version\": \"1.0\",\n");
    fprintf(f, "  \"total_functions_analyzed\": %zu,\n", table->count);
    fprintf(f, "  \"total_calls\": %lu,\n", table->total_calls);
    fprintf(f, "  \"total_memory_ops\": %lu,\n", table->total_memory_ops);

    if (stats) {
        fprintf(f, "  \"statistics\": {\n");
        fprintf(f, "    \"call_mean\": %.2f,\n", stats->call_mean);
        fprintf(f, "    \"call_stddev\": %.2f,\n", stats->call_stddev);
        fprintf(f, "    \"call_p95\": %lu,\n", stats->call_p95);
        fprintf(f, "    \"call_p99\": %lu,\n", stats->call_p99);
        fprintf(f, "    \"mem_mean\": %.2f,\n", stats->mem_mean);
        fprintf(f, "    \"mem_stddev\": %.2f,\n", stats->mem_stddev);
        fprintf(f, "    \"mem_p95\": %lu,\n", stats->mem_p95);
        fprintf(f, "    \"mem_p99\": %lu\n", stats->mem_p99);
        fprintf(f, "  },\n");
    }

    fprintf(f, "  \"function_profiles\": [\n");

    size_t written = 0;
    for (size_t i = 0; i < table->capacity; i++) {
        if (table->func_profiles[i].function_addr != NULL) {
            FunctionProfile *fp = &table->func_profiles[i];
            MemoryProfile *mp = &table->mem_profiles[i];

            if (written > 0) fprintf(f, ",\n");

            fprintf(f, "    {\n");
            fprintf(f, "      \"address\": \"%p\",\n", fp->function_addr);
            fprintf(f, "      \"name\": \"%s\",\n", fp->function_name);
            fprintf(f, "      \"call_count\": %lu,\n", fp->call_count);
            fprintf(f, "      \"stack_reads\": %lu,\n", mp->stack_reads);
            fprintf(f, "      \"stack_writes\": %lu,\n", mp->stack_writes);
            fprintf(f, "      \"heap_reads\": %lu,\n", mp->heap_reads);
            fprintf(f, "      \"heap_writes\": %lu,\n", mp->heap_writes);
            fprintf(f, "      \"is_hotspot\": %s,\n", fp->is_hotspot ? "true" : "false");
            fprintf(f, "      \"is_patched\": %s\n", fp->is_patched ? "true" : "false");
            fprintf(f, "    }");

            written++;
        }
    }

    fprintf(f, "\n  ]\n");
    fprintf(f, "}\n");

    fclose(f);
    return true;
}

uint64_t get_timestamp_microseconds(void) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (uint64_t)tv.tv_sec * 1000000 + tv.tv_usec;
}

const char* resolve_function_name(void *addr, char *buffer, size_t size) {
    if (!buffer || size == 0) return NULL;

    /* Try to resolve symbol name using dladdr */
    Dl_info info;
    if (dladdr(addr, &info) && info.dli_sname) {
        snprintf(buffer, size, "%s", info.dli_sname);
    } else {
        /* No symbol - use address */
        snprintf(buffer, size, "func_%p", addr);
    }

    return buffer;
}
