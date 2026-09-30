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

/* size: 0x8C */
struct ShellSetFuncs {
    /* +0x00 */ u8 pad_0x00[0x14];
    /* +0x14 */ void (*method_0x14)(struct _ENEMY_WORK* self, nw4r::math::VEC3* pos, u32 a, u32 b, u16 c,
                                    ShellSetFuncs* table);
    /* +0x18 */ void (*method_0x18)(struct _ENEMY_WORK* self, nw4r::math::VEC3* pos, u32* angle, u32 a, u32 b,
                                    u16 flags, ShellSetFuncs* table);
    /* +0x1C */ u8 pad_0x1C[0x24 - 0x1C];
    /* +0x24 */ void (*set_target)(struct _AINPC_W* self, ShellSetFuncs* table);
    /* +0x28 */ void (*method_0x28)(struct _ENEMY_WORK* self, nw4r::math::VEC3* pos, s32 mode, s32 id,
                                    ShellSetFuncs* table, f32 scale);
    /* +0x2C */ void (*method_0x2C)(struct _ENEMY_WORK* self, u32 a, u32 b, nw4r::math::VEC3* pos, f32 scale,
                                    u16 flags, ShellSetFuncs* table);
    /* +0x30 */ void (*method_0x30)(struct _ENEMY_WORK* self, u32 a, nw4r::math::VEC3* from,
                                    nw4r::math::VEC3* to, f32 scale, u16 flags, ShellSetFuncs* table);
    /* +0x34 */ u8 pad_0x34[0x38 - 0x34];
    /* +0x38 */ void (*request)(struct _AINPC_W* self, s32 id, ShellSetFuncs* table);
    /* untyped: caller-owned payload, each call site passes its own job record */
    /* +0x3C */ void (*method_0x3C)(struct _ENEMY_WORK* self, void* rec, u32 mode, ShellSetFuncs* table);
    /* +0x40 */ u8 pad_0x40[0x54 - 0x40];
    /* +0x54 */ void (*method_0x54)(struct _ENEMY_WORK* self, u32 a, u32 b, nw4r::math::VEC3* pos,
                                    ShellSetFuncs* table);
    /* +0x58 */ void (*method_0x58)(struct _ENEMY_WORK* self, u32 id);
    /* +0x5C */ u8 pad_0x5C[0x80 - 0x5C];
    /* +0x80 */ void (*method_0x80)(nw4r::math::VEC3* pos, u32 kind, s32 id, ShellSetFuncs* table);
    /* +0x84 */ u8 pad_0x84[0x88 - 0x84];
    /* +0x88 */ void (*method_0x88)(nw4r::math::VEC3* pos, u32 kind, s32 id, ShellSetFuncs* table);
};

extern ShellSetFuncs* shell_set_func_ptr;

#endif /* MHTRI_STAGE_SHELL_SET_FUNC_PTR_H */
