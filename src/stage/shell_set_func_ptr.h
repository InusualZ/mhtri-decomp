/* `shell_set_func_ptr` (`.sbss` 0x80794B60) - the pointer to the shell-set job table, and the table's type.
 *
 * The owner is `stage/fn_802B2AA0.cpp` (it defines the pointer; the evidence and its confidence are in
 * that unit's header comment).  Nothing in the DOL stores the pointer: an RSO does, so the slot
 * signatures below are the call sites' own argument shapes (a GUESS per slot) and every table call
 * passes the table itself as its last integer argument.  Only the slots some unit reaches are typed.
 */
#ifndef MHTRI_STAGE_SHELL_SET_FUNC_PTR_H
#define MHTRI_STAGE_SHELL_SET_FUNC_PTR_H

#include "types.h"
#include "nw4r/math.h"

/* size: 0x98 (approximation: evidenced to +0x94) */
struct ShellSetFuncs {
    /* +0x00 */ void (*method_0x00)(struct _PLW* self, u32 kind, u32 mode, ShellSetFuncs* table);  /* the item-use shell (`Pl/pl_act_step.cpp`) */
    /* +0x04 */ void (*method_0x04)(struct _AINPC_W* self, u32 kind, u32 a, u32 b, u32 c, ShellSetFuncs* table);
    /* +0x08 */ void (*method_0x08)(nw4r::math::VEC3* pos, u32 kind, u32 area, u32 a);
    /* +0x0C */ u8 pad_0x0C[0x10 - 0x0C];
    /* +0x10 */ void (*method_0x10)(struct _AINPC_W* self, nw4r::math::VEC3* pos, u32 kind, u32 area,
                                    ShellSetFuncs* table);
    /* +0x14 */ void (*method_0x14)(struct _ENEMY_WORK* self, nw4r::math::VEC3* pos, u32 a, u32 b, u16 c,
                                    ShellSetFuncs* table);
    /* +0x18 */ void (*method_0x18)(struct _ENEMY_WORK* self, nw4r::math::VEC3* pos, u32* angle, u32 a, u32 b,
                                    u16 flags, ShellSetFuncs* table);
    /* +0x1C */ void (*method_0x1C)(struct _PLW* self, s32 n, ShellSetFuncs* table);
    /* +0x20 */ void (*method_0x20)(struct _PLW* self, s32 n, ShellSetFuncs* table);
    /* +0x24 */ void (*set_target)(struct _AINPC_W* self, ShellSetFuncs* table);
    /* +0x28 */ void (*method_0x28)(struct _ENEMY_WORK* self, nw4r::math::VEC3* pos, s32 mode, s32 id,
                                    ShellSetFuncs* table, f32 scale);
    /* +0x2C */ void (*method_0x2C)(struct _ENEMY_WORK* self, u32 a, u32 b, nw4r::math::VEC3* pos, f32 scale,
                                    u16 flags, ShellSetFuncs* table);
    /* +0x30 */ void (*method_0x30)(struct _ENEMY_WORK* self, u32 a, nw4r::math::VEC3* from,
                                    nw4r::math::VEC3* to, f32 scale, u16 flags, ShellSetFuncs* table);
    /* +0x34 */ void (*method_0x34)(struct _PLW* self, s32 n, ShellSetFuncs* table);
    /* +0x38 */ void (*request)(struct _AINPC_W* self, s32 id, ShellSetFuncs* table);
    /* untyped: caller-owned payload, each call site passes its own job record */
    /* +0x3C */ void (*method_0x3C)(struct _ENEMY_WORK* self, void* rec, u32 mode, ShellSetFuncs* table);
    /* untyped: caller-owned payload, the spawn record `enemy/em003_prog.cpp`'s swipe steps build */
    /* +0x40 */ void (*method_0x40)(struct _ENEMY_WORK* self, void* rec, u32 mode, u32 hit_id, ShellSetFuncs* table);
    /* +0x44 */ u8 pad_0x44[0x50 - 0x44];
    /* +0x50 */ void (*method_0x50)(struct _ENEMY_WORK* self, u32 a, u32 b, ShellSetFuncs* table);
    /* +0x54 */ void (*method_0x54)(struct _ENEMY_WORK* self, u32 a, u32 b, nw4r::math::VEC3* pos,
                                    ShellSetFuncs* table);
    /* +0x58 */ void (*method_0x58)(struct _ENEMY_WORK* self, u32 id);
    /* +0x5C */ void (*method_0x5C)(struct _AINPC_W* self, u32 id, ShellSetFuncs* table);
    /* +0x60 */ u8 pad_0x60[0x64 - 0x60];
    /* +0x64 */ void (*method_0x64)(struct _ENEMY_WORK* self, u32 kind, u32 joint, f32 scale, u16 id,
                                    ShellSetFuncs* table);
    /* +0x68 */ void (*method_0x68)(struct _ENEMY_WORK* self, u32 kind, u32 id, f32 scale, nw4r::math::VEC3* pos);
    /* +0x6C */ u8 pad_0x6C[0x70 - 0x6C];
    /* +0x70 */ void (*method_0x70)(struct _ENEMY_WORK* self, ShellSetFuncs* table);
    /* +0x74 */ void (*method_0x74)(struct _ENEMY_WORK* self, u32 kind, u32 joint, nw4r::math::VEC3* pos, f32 scale,
                                    u32 a, u16 b, ShellSetFuncs* table);
    /* +0x78 */ void (*method_0x78)(struct _PLW* self, u32 kind, ShellSetFuncs* table);  /* the slinger shell (`Pl/pl_act_step.cpp`) */
    /* +0x7C */ u8 pad_0x7C[0x80 - 0x7C];
    /* +0x80 */ void (*method_0x80)(nw4r::math::VEC3* pos, u32 kind, s32 id, ShellSetFuncs* table);
    /* +0x84 */ void (*method_0x84)(struct _ENEMY_WORK* self, u32 kind, nw4r::math::VEC3* pos, f32 scale, u32 id,
                                    ShellSetFuncs* table);
    /* +0x88 */ void (*method_0x88)(nw4r::math::VEC3* pos, u32 kind, s32 id, ShellSetFuncs* table);
    /* +0x8C */ void (*method_0x8C)(struct _ENEMY_WORK* self, u32 kind, nw4r::math::VEC3* pos, f32 scale, u32 id,
                                    ShellSetFuncs* table);
    /* +0x90 */ void (*method_0x90)(struct _AINPC_W* self, u32 kind, u8 count, u16 spin, u16 angle,
                                    ShellSetFuncs* table);
    /* +0x94 */ void (*method_0x94)(struct _AINPC_W* self, u32 kind, nw4r::math::VEC3* target, ShellSetFuncs* table);
};

extern ShellSetFuncs* shell_set_func_ptr;

#endif /* MHTRI_STAGE_SHELL_SET_FUNC_PTR_H */
