#include "patcher.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>
#include <errno.h>

#define MAX_PATCHES 1024

PatcherContext* patcher_init(EngineConfig *config) {
    if (!config) return NULL;

    PatcherContext *ctx = calloc(1, sizeof(PatcherContext));
    if (!ctx) return NULL;

    ctx->patches = calloc(MAX_PATCHES, sizeof(PatchInfo));
    if (!ctx->patches) {
        free(ctx);
        return NULL;
    }

    ctx->capacity = MAX_PATCHES;
    ctx->count = 0;
    ctx->config = config;
    ctx->successful_patches = 0;
    ctx->failed_patches = 0;

    return ctx;
}

void patcher_cleanup(PatcherContext *ctx) {
    if (!ctx) return;

    /* Uninstall all patches before cleanup */
    for (size_t i = 0; i < ctx->count; i++) {
        if (ctx->patches[i].installed) {
            patcher_uninstall(ctx, ctx->patches[i].original_func);
        }
    }

    free(ctx->patches);
    free(ctx);
}

bool patcher_make_writable(void *addr, size_t size) {
    /* Get page-aligned address */
    long page_size = sysconf(_SC_PAGESIZE);
    uintptr_t page_start = ((uintptr_t)addr) & ~(page_size - 1);

    /* Make memory writable */
    if (mprotect((void*)page_start, page_size, PROT_READ | PROT_WRITE | PROT_EXEC) != 0) {
        perror("mprotect (make writable) failed");
        return false;
    }

    return true;
}

bool patcher_restore_protection(void *addr, size_t size) {
    /* Get page-aligned address */
    long page_size = sysconf(_SC_PAGESIZE);
    uintptr_t page_start = ((uintptr_t)addr) & ~(page_size - 1);

    /* Restore to read-execute only */
    if (mprotect((void*)page_start, page_size, PROT_READ | PROT_EXEC) != 0) {
        perror("mprotect (restore protection) failed");
        return false;
    }

    return true;
}

void patcher_flush_cache(void *addr, size_t size) {
    /* Flush instruction cache to ensure CPU sees new instructions */
    __builtin___clear_cache((char*)addr, (char*)addr + size);
}

void* patcher_generate_optimized_version(void *original_func) {
    /* PLACEHOLDER: In a real implementation, this would:
     * 1. Disassemble the original function
     * 2. Apply optimizations (loop unrolling, inlining, etc.)
     * 3. Generate optimized machine code
     * 4. Allocate executable memory and copy the code
     *
     * For this challenge, we simulate by returning a simple stub
     * that just calls the original function (no-op optimization)
     */

    /* In a real scenario, you'd use:
     * - LLVM for JIT compilation
     * - Capstone for disassembly
     * - Keystone for assembly
     *
     * For now, we just return NULL to indicate "no optimization available"
     */

    return NULL; /* No optimization implemented yet */
}

bool patcher_install_trampoline(PatcherContext *ctx, void *target_func, void *new_func) {
    if (!ctx || !target_func || !new_func) return false;

    /* Check if already patched */
    if (patcher_is_patched(ctx, target_func)) {
        if (ctx->config->verbose) {
            printf("[Patcher] Function %p already patched\n", target_func);
        }
        return false;
    }

    /* Check capacity */
    if (ctx->count >= ctx->capacity) {
        fprintf(stderr, "[Patcher] ERROR: Maximum patch capacity reached\n");
        ctx->failed_patches++;
        return false;
    }

    /* Create patch info */
    PatchInfo *patch = &ctx->patches[ctx->count];
    patch->original_func = target_func;
    patch->optimized_func = new_func;
    patch->installed = false;
    patch->install_timestamp = get_timestamp_microseconds();

    /* Step 1: Make memory writable */
    if (!patcher_make_writable(target_func, 16)) {
        ctx->failed_patches++;
        return false;
    }

    /* Step 2: Save original bytes (first 16 bytes to be safe) */
    memcpy(patch->original_bytes, target_func, 16);

    /* Step 3: Calculate jump offset
     * x86_64 JMP instruction: E9 [32-bit relative offset]
     * Offset = target - (source + 5)
     */
    uint8_t jmp_instruction[5];
    jmp_instruction[0] = 0xE9; /* JMP opcode */

    int64_t offset = (uint8_t*)new_func - ((uint8_t*)target_func + 5);

    /* Check if offset fits in 32-bit signed integer */
    if (offset > INT32_MAX || offset < INT32_MIN) {
        fprintf(stderr, "[Patcher] ERROR: Jump offset too large (%ld)\n", offset);
        patcher_restore_protection(target_func, 16);
        ctx->failed_patches++;
        return false;
    }

    memcpy(&jmp_instruction[1], &offset, 4);
    patch->patch_size = 5;

    /* Step 4: Install the trampoline atomically */
    memcpy(target_func, jmp_instruction, 5);

    /* Step 5: Flush instruction cache */
    patcher_flush_cache(target_func, 5);

    /* Step 6: Restore memory protection */
    if (!patcher_restore_protection(target_func, 16)) {
        /* Warning: protection restore failed, but patch is installed */
        fprintf(stderr, "[Patcher] WARNING: Failed to restore memory protection\n");
    }

    patch->installed = true;
    ctx->count++;
    ctx->successful_patches++;

    if (ctx->config->verbose) {
        printf("[Patcher] Successfully patched %p -> %p\n", target_func, new_func);
    }

    return true;
}

bool patcher_uninstall(PatcherContext *ctx, void *target_func) {
    if (!ctx || !target_func) return false;

    /* Find the patch */
    PatchInfo *patch = patcher_get_info(ctx, target_func);
    if (!patch || !patch->installed) {
        return false;
    }

    /* Make memory writable */
    if (!patcher_make_writable(target_func, patch->patch_size)) {
        return false;
    }

    /* Restore original bytes */
    memcpy(target_func, patch->original_bytes, patch->patch_size);

    /* Flush cache */
    patcher_flush_cache(target_func, patch->patch_size);

    /* Restore protection */
    patcher_restore_protection(target_func, patch->patch_size);

    patch->installed = false;

    if (ctx->config->verbose) {
        printf("[Patcher] Uninstalled patch for %p\n", target_func);
    }

    return true;
}

bool patcher_is_patched(PatcherContext *ctx, void *func_addr) {
    if (!ctx || !func_addr) return false;

    for (size_t i = 0; i < ctx->count; i++) {
        if (ctx->patches[i].original_func == func_addr && ctx->patches[i].installed) {
            return true;
        }
    }

    return false;
}

PatchInfo* patcher_get_info(PatcherContext *ctx, void *func_addr) {
    if (!ctx || !func_addr) return NULL;

    for (size_t i = 0; i < ctx->count; i++) {
        if (ctx->patches[i].original_func == func_addr) {
            return &ctx->patches[i];
        }
    }

    return NULL;
}

void patcher_print_summary(PatcherContext *ctx) {
    if (!ctx) return;

    printf("\n=== Patcher Summary ===\n");
    printf("Total Patches:       %zu\n", ctx->count);
    printf("Successful Patches:  %lu\n", ctx->successful_patches);
    printf("Failed Patches:      %lu\n", ctx->failed_patches);
    printf("=======================\n\n");
}
