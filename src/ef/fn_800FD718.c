/* auto/800FD718_fn_800FD718.c - the state-1/2/3 handlers of the `eft002` effect machine,
 * Naming note: the symbol map has only fn_XXXXXXXX for this range (checked with symedit: every fn_ name this file uses is a bare .text entry in config/RMHE08/symbols.txt)
 * `.text` 0x800FD718..0x800FD864 (3 functions: fn_800FD718, fn_800FD850, fn_800FD860).
 *
 * What it is.  `fn_800FD4E4` is the four-state update hook the `eft002` setters install on the 0x48-byte
 * effect record (`_EFT::dispatch_0x34`); it dispatches on `state_0x05` to `fn_800FD520` (0),
 * `fn_800FD718` (1), `fn_800FD850` (2) and `fn_800FD860` (3).  This unit owns the last three.
 *
 * `fn_800FD718` is the shell family's per-frame body (`type_0x02 == 1`): it counts the frame budget down,
 * and once the shell's byte at +0x00 has cleared it drops the effect; otherwise, while the shell is in
 * the area the effect was spawned for, it re-seats the effect on the shell's position - the shell's own
 * position, offset 20 units up, rotated by the shell's X and Z angles and added back - and then asks the
 * nw4r effect whether it is still alive, either dropping the effect or handing the pool block to
 * `fn_800F93D8`.  `fn_800FD850` is the state-2 "advance one state" step and `fn_800FD860` the state-3
 * release, a one-line forward to `fn_800F886C`.
 *
 * Language.  `langcheck` reads the object as *suggested* C++ - the only evidence is six mangled callees -
 * and a mangled callee does not decide the caller's language, so the unit stays `.c` and the callees are
 * declared with the map's spelling, exactly as `auto/800FD520_fn_800FD520.c` does for the same six
 * (docs/plan.md, "The language comes from the symbol").
 *
 * Result: fn_800FD718 (0x138), fn_800FD850 (0x10) and fn_800FD860 (0x4) 100 %; `.text` (0x14C),
 * `extab` (0x8) and `extabindex` (0xC) byte-identical to the target, and so is every `.rela.text`
 * relocation (offset, type and symbol name).  The only object-level differences are the `.comment`
 * version byte (ours 0x0f, retail 0x0e) and our local extab/symbol-table names, neither of which reaches
 * the linked DOL.
 *
 * Load-bearing source shapes (each measured; the wrong form costs real points):
 *   * `type_0x02` is `s8`, not `u8`.  Retail compares it with `cmpwi`; a `u8` makes MWCC emit `cmplwi`
 *     (99.94 %).  The neighbours agree: `_PLW::flag_0x30` needed `s8` for the same reason, and the
 *     `switch` chains that read a `u8` got `cmpwi` only because a switch chain is lowered signed.
 *   * the area test is an **early return** (`if (area != get_now_areano()) return;`), not an
 *     `if (area == ...) { ... }` block.  Written as a block, MWCC branches to the `effect_move` block
 *     where retail branches straight to the epilogue (99.94 %) - the shell leaving the area skips the
 *     rest of the frame.
 *   * `work` is declared **before** `source`: with `source` first the allocator colours the two
 *     callee-saved webs the other way round (`source` r31 / `work` r30 where retail has r31 / r30).
 *   * `work = self->work_0x38;` is a statement *before* the `VEC3` construction - retail does not hoist
 *     the constructor call above it.
 *   * `fn_800FD850`/`fn_800FD860` are *not* called from `fn_800FD718`: the state bump is written out
 *     twice, and the state-2/3 bodies exist only because `fn_800FD4E4` dispatches to them.
 *
 * Data.  The unit owns no pool section: its two `.sdata2` floats (0.0f and 20.0f, the effect's offset
 * from the shell) live in a shared pool, so they are `extern`-declared by their map names and never
 * defined (playbook 29); the target object carries no such section either.  The `extab`/`extabindex`
 * fragments travel with the code and are already claimed in `splits.txt`.
 *
 * Types.  `_EFT`, `_EFT_WORK` and `_SHELL_W` are reconstructed minimally (only the offsets this unit
 * reads) and are copies of `auto/800FCED4_fn_800FCED4.cpp`'s definitions, which the same `_SHELL_W`
 * extension belongs to; all three belong in one shared header, which does not exist yet (the request is
 * in that unit's outbox).  `VEC3` is `include/nw4r/math.h`'s type, which is C++ and so cannot be
 * included here - the same private copy `auto/80104BD0_fn_80104BD0.c` keeps.
 *
 * Inventory, addresses and sizes: `python tools/units/ledger.py unit auto/800FD718_fn_800FD718.c`.
 * The name is provisional - `auto/` plus the first symbol's address - because nothing in the object
 * names the original source file.
 */

#include "types.h"
#include "nw4r/math.h"
#include "mh3_pad.h" /* VEC3_ctor / copyVec3 / setVec3 (rule 2) */

/* ---------------------------------------------------------------------------------------------------
 * the nw4r math type the mangled callees take
 * ------------------------------------------------------------------------------------------------- */

/* `VEC3` comes from `nw4r/math.h` - one definition, in the owner's header (rule 1). */

/* ---------------------------------------------------------------------------------------------------
 * the 0x48-byte effect record, its pool block, and the two actors it follows
 * ------------------------------------------------------------------------------------------------- */

struct _EFT_WORK;

/* The shell work the effect follows.  Size from `push_shell_work`'s `memset(self, 0, 0x10C)`. */
struct _SHELL_W {
    /* +0x000 */ u8 field_0x00;  /* 0 marks the shell as finished: the effect is dropped */
    /* +0x001 */ u8 unused_0x001[0x03 - 0x01];
    /* +0x003 */ u8 type_0x03;    /* the shell type, 0..29 (fn_800FD2B0's switch operand) */
    /* +0x004 */ u8 unused_0x004[0x08 - 0x04];
    /* +0x008 */ u8 area_0x08;    /* the area the shell is in, copied into _EFT::area_0x44 */
    /* +0x009 */ u8 unused_0x009[0x18 - 0x09];
    /* +0x018 */ VEC3 pos_0x18;   /* world position; the effect is re-seated on it */
    /* +0x024 */ u32 rot_x_0x24;  /* X rotation, rotVecX's operand */
    /* +0x028 */ u8 unused_0x028[0x2C - 0x28];
    /* +0x02C */ u32 rot_z_0x2C;  /* Z rotation, rotVecZ's operand */
    /* +0x030 */ u8 unused_0x030[0x10C - 0x30];
}; /* size: 0x10C */

/* The effect record `fn_800F8788` hands out and `fn_800FD4E4` dispatches on. */
struct _EFT {
    /* +0x00 */ u8 unused_0x00;
    /* +0x01 */ u8 flag_0x01;      /* 1 while the effect is live, 0 once it has been dropped */
    /* +0x02 */ s8 type_0x02;      /* the effect type; 0 is the player family, 1 the shell family; s8 is
                                    * this unit's codegen (u8: 100.0 -> 99.23), reported vs `_EFT`'s u8 */
    /* +0x03 */ u8 field_0x03;
    /* +0x04 */ u8 field_0x04;
    /* +0x05 */ u8 state_0x05;     /* the `fn_800FD4E4` state index the handlers advance */
    /* +0x06 */ u8 unused_0x06[0x0C - 0x06];
    /* +0x0C */ s32 timer_0x0C;    /* the frame budget the handlers count down */
    /* +0x10 */ u8 unused_0x10[0x18 - 0x10];
    /* +0x18 */ VEC3 pos_0x18;     /* the position handed to SetRootMtxTrans */
    /* +0x24 */ u8 unused_0x24[0x30 - 0x24];
    /* +0x30 */ void* source_0x30; /* the actor the effect was spawned for (_PLW or _SHELL_W) */
    /* +0x34 */ void (*dispatch_0x34)(struct _EFT*); /* the state dispatcher (fn_800FD4E4) */
    /* +0x38 */ struct _EFT_WORK* work_0x38;
    /* +0x3C */ u8 unused_0x3C[0x40 - 0x3C];
    /* +0x40 */ void (*release_0x40)(struct _EFT*); /* the pool release handler (fn_800FD4A8) */
    /* +0x44 */ u8 area_0x44;      /* the area the effect is legal in */
    /* +0x45 */ u8 unused_0x45[0x48 - 0x45];
}; /* size: 0x48 */

/* The pool block at `_EFT::work_0x38`: a count and the effect-pointer array it counts.  Size 0x10 is a
 * lower bound - the block is only ever reached through its first two words. */
struct _EFT_WORK {
    /* +0x00 */ s32 count;
    /* +0x04 */ void* effect;
    /* +0x08 */ f32 scale;
    /* +0x0C */ u32 param_id;
}; /* size: 0x10 */

/* ---------------------------------------------------------------------------------------------------
 * externs - the plain-named callees and the shared pool
 * ------------------------------------------------------------------------------------------------- */

      /* a `blr` stub in the DOL: the VEC3 ctor */
extern void* fn_80073F68(VEC3* dst, const VEC3* src);
extern void fn_800F93D8(void* self, void* effects, u32 mode, s32 count, u32 arg);
extern void fn_800F886C(void* self);
extern void fn_800FD718(struct _EFT* self);
extern void fn_800FD850(struct _EFT* self);
extern void fn_800FD860(struct _EFT* self);

/* The mangled callees, declared with the map's spelling (docs/plan.md, "The language comes from the
 * symbol"): `setVector3(nw4r::math::VEC3*, f32, f32, f32)`, `rotVecX/rotVecZ(..., u32)`,
 * `SetRootMtxTrans(nw4r::ef::Effect*, nw4r::math::VEC3*)`, `effect_move(nw4r::ef::Effect*)`,
 * `get_now_areano()`. */
extern u32 get_now_areano__Fv(void);
extern void setVector3__FPQ34nw4r4math4VEC3fff(VEC3* v, f32 x, f32 y, f32 z);
extern void rotVecX__FPQ34nw4r4math4VEC3Ul(VEC3* v, u32 angle);
extern void rotVecZ__FPQ34nw4r4math4VEC3Ul(VEC3* v, u32 angle);
extern void SetRootMtxTrans__FPQ34nw4r2ef6EffectPQ34nw4r4math4VEC3(void* effect, VEC3* pos);
extern u32 effect_move__FPQ34nw4r2ef6Effect(void* effect);

/* The shared pool: the effect's offset from the shell.  Referenced but not emitted (see the header). */
extern f32 lbl_80796688; /* 0.0f  */
extern f32 lbl_8079668C; /* 20.0f */

/* ---------------------------------------------------------------------------------------------------
 * bodies
 * ------------------------------------------------------------------------------------------------- */

/* Per-frame body of the shell family's effect: counts the frame budget down, drops the effect once the
 * shell is gone, and re-seats it on the shell's position while the shell is in the effect's area. */
void fn_800FD718(struct _EFT* self)
{
    struct _EFT_WORK* work;
    struct _SHELL_W* source;
    VEC3 v;

    work = self->work_0x38;
    VEC3_ctor(&v);

    if (self->type_0x02 == 1) {
        source = (struct _SHELL_W*)self->source_0x30;
        self->timer_0x0C--;
        if (source->field_0x00 == 0 && self->timer_0x0C < 0) {
            self->flag_0x01 = 0;
            self->state_0x05++;
            return;
        }
        self->area_0x44 = source->area_0x08;
        if (self->area_0x44 != (u8)get_now_areano__Fv()) {
            return;
        }
        copyVec3(&self->pos_0x18, &source->pos_0x18);
        setVector3__FPQ34nw4r4math4VEC3fff(&v, lbl_80796688, lbl_8079668C, lbl_80796688);
        rotVecX__FPQ34nw4r4math4VEC3Ul(&v, source->rot_x_0x24);
        rotVecZ__FPQ34nw4r4math4VEC3Ul(&v, source->rot_z_0x2C);
        fn_80073F68(&self->pos_0x18, &v);
        SetRootMtxTrans__FPQ34nw4r2ef6EffectPQ34nw4r4math4VEC3(work->effect, &self->pos_0x18);
    }

    if (effect_move__FPQ34nw4r2ef6Effect(work->effect) == 0) {
        self->flag_0x01 = 0;
        self->state_0x05++;
        return;
    }
    fn_800F93D8(self, &work->effect, 1, work->count, 0);
}

/* Advances the effect one state: the state-2 "nothing left to do" step. */
void fn_800FD850(struct _EFT* self)
{
    self->state_0x05++;
}

/* Releases the effect: the state-3 step, the release `fn_800F886C` performs. */
void fn_800FD860(struct _EFT* self)
{
    fn_800F886C(self);
}
