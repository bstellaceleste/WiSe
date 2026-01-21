#ifndef HOTPATCH_H
#define HOTPATCH_H

#include "dr_api.h"

typedef enum {
    PATCH_JUMP_DIRECT,
    PATCH_TRAMPOLINE,
    PATCH_CALL,
    PATCH_CALL_WRAPPER,
    PATCH_CODE_REPLACE
} patch_type_t;

typedef struct {
    app_pc original_addr;
    app_pc target_addr;
    patch_type_t type;
    byte *patch_code;
    size_t patch_size;
    byte original_bytes[16];
    bool is_applied;
} hotpatch_t;

void hotpatch_init(void);
void hotpatch_exit(void);
bool hotpatch_redirect(app_pc from, app_pc to);
bool hotpatch_apply(hotpatch_t *patch);
bool hotpatch_remove(hotpatch_t *patch);
void hotpatch_destroy(hotpatch_t *patch);
void hotpatch_list_patches(void);
bool hotpatch_rollback_all(void);
size_t hotpatch_get_patch_size(patch_type_t type);

#endif
