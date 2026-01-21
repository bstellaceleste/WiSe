#ifndef PATCHER_H
#define PATCHER_H

#include "metrics.h"
#include "config.h"
#include <stdint.h>
#include <stdbool.h>

/* Patch information */
typedef struct {
    void *original_func;              /* Original function address */
    void *optimized_func;             /* Optimized function address */
    uint8_t original_bytes[16];       /* Saved original bytes */
    size_t patch_size;                /* Size of patch in bytes */
    bool installed;                   /* Is patch installed? */
    uint64_t install_timestamp;       /* When patch was installed */
} PatchInfo;

/* Patcher context */
typedef struct {
    PatchInfo *patches;               /* Array of patches */
    size_t capacity;                  /* Maximum patches */
    size_t count;                     /* Current patch count */
    EngineConfig *config;             /* Configuration */
    uint64_t successful_patches;      /* Successful patches */
    uint64_t failed_patches;          /* Failed patches */
} PatcherContext;

/* Initialize patcher */
PatcherContext* patcher_init(EngineConfig *config);

/* Cleanup patcher */
void patcher_cleanup(PatcherContext *ctx);

/* Install a trampoline patch */
bool patcher_install_trampoline(PatcherContext *ctx,
                               void *target_func,
                               void *new_func);

/* Uninstall a patch (restore original code) */
bool patcher_uninstall(PatcherContext *ctx, void *target_func);

/* Check if function is already patched */
bool patcher_is_patched(PatcherContext *ctx, void *func_addr);

/* Get patch info for a function */
PatchInfo* patcher_get_info(PatcherContext *ctx, void *func_addr);

/* Helper: Make memory writable */
bool patcher_make_writable(void *addr, size_t size);

/* Helper: Restore memory protection */
bool patcher_restore_protection(void *addr, size_t size);

/* Helper: Flush instruction cache */
void patcher_flush_cache(void *addr, size_t size);

/* Generate optimized version of function (placeholder for actual optimization) */
void* patcher_generate_optimized_version(void *original_func);

/* Print patcher summary */
void patcher_print_summary(PatcherContext *ctx);

#endif /* PATCHER_H */
