/* auto/800FD520_fn_800FD520.c - the state-0 handler of the eft002 effect machine,
 * rule 7 deferred: the symbol map has only fn_XXXXXXXX for this range (checked with symedit: every fn_ name this file uses is a bare .text entry in config/RMHE08/symbols.txt)
 * `.text` 0x800FD520..0x800FD718 (one function, `fn_800FD520`).
 *
 * What it is.  `fn_800FD4E4` (the last function of the previous unit, 0x800FCED4..0x800FD520) is the
 * four-state update hook `eft002_set` installs on the 0x48-byte effect record; it dispatches on
 * `state_0x05` to `fn_800FD520` (0), `fn_800FD718` (1), `fn_800FD850` (2) and `fn_800FD860` (3).  This
 * function creates the real nw4r effect for the actor's weapon class and, for the classes whose model
 * exists, places it at the weapon joint: it queries the joint's world matrix through the player's
 * `MHchar` (`_PLW::physics_0x13C + 4`), transforms a per-class local muzzle offset by it and adds the
 * matrix translation, then hands the result to `SetRootMtxTrans`.  The sibling `fn_8027C064`
 * (`Pl/pl_act.cpp`) is the same shape and was the template for the tail.
 *
 * The two "empty" callees are real: `fn_80043EA8` and `fn_8005050C` are one-instruction `blr` stubs in
 * the DOL, so their calls must still be written out to reproduce the target's bytes.
 *
 * Result: 100 %; `.text` (0x1F8), `extab` (0x8), `extabindex` (0xC) and all 26 `.rela.text` relocation
 * names byte-identical to the target.  The only object-level differences are the `.comment` version
 * byte (ours 0x0f, retail 0x0e) and our local extab/symbol-table names, neither of which reaches the DOL.
 *
 * Load-bearing source shapes (each measured; the wrong form costs real points):
 *   * `#pragma peephole off` is required for the pool-block store.  With the pass on MWCC forwards
 *     `work->effect = res_eft_create(...)` into the test, dropping the target's `lwz r0,4(work)` - 4 B
 *     short, 99.68 %; the pragma keeps the reload (100 %).  The assignment has to be its own statement
 *     too: `if ((work->effect = ...) == 0)` does not reload even with the pragma (99.68 %).
 *   * `source` is declared **before** `work`.  With `work` first the allocator colours the two webs the
 *     other way round (`work` r31 / `source` r30 where retail has r30 / r31); the declaration order
 *     flips the pair and is the whole 99.68 -> 100 % step.
 *
 * Data.  The unit owns no pool section: its 12 `.sdata2` floats (the per-class offsets) and the two
 * `.sdata` u16 tables (effect id and parameter id per class) live in a shared pool, so they are
 * `extern`-declared by their map names and never defined (playbook 29); the target object carries no
 * such section either.  The map names are the target's - the same trick
 * `auto/800FCED4_fn_800FCED4.cpp` uses for its own pool.
 *
 * Types.  `_EFT`/`_EFT_WORK`/`_PLW` are reconstructed minimally (only the offsets this function reads)
 * and are copies of the neighbours' definitions (`auto/800FCED4_fn_800FCED4.cpp`'s `_EFT`,
 * `Pl/pl_act.cpp`'s `_PLW`); all three belong in one shared header, which does not exist yet.
 *
 * Language.  The unit's own symbol is plain (`fn_800FD520`), so the file stays C and the mangled
 * callees are declared by their map spelling, as `auto/803066F0_fn_803066F0.c` does.
 *
 * Inventory, addresses and sizes: `python tools/units/ledger.py unit auto/800FD520_fn_800FD520.c`.
 */

#include "types.h"
#include "nw4r/math.h"
#include "ef/fn_800FD718.h"
#include "unsplit/sound.h"

/* ---------------------------------------------------------------------------------------------------
 * the nw4r math types the mangled callees take
 * ------------------------------------------------------------------------------------------------- */

/* `VEC3` / `MTX34` come from `nw4r/math.h` - one definition, in the owner's header (rule 1). */

/* ---------------------------------------------------------------------------------------------------
 * the 0x48-byte effect record and its pool block
 * ------------------------------------------------------------------------------------------------- */

struct _EFT_WORK;

/* The effect object `fn_800F8788(16)` hands out and `fn_800FD4E4` dispatches. size: 0x48 */
struct _EFT {
    /* +0x00 */ u8 unused_0x00;
    /* +0x01 */ u8 flag_0x01;   /* 1 while the effect is live, 0 when it has been dropped */
    /* +0x02 */ u8 type_0x02;   /* the effect type the pool tables are indexed by */
    /* +0x03 */ u8 unused_0x03;
    /* +0x04 */ u8 unused_0x04;
    /* +0x05 */ u8 state_0x05;  /* the `fn_800FD4E4` state index this handler advances */
    /* +0x06 */ u8 unused_0x06[0x18 - 0x06];
    /* +0x18 */ VEC3 pos_0x18;  /* the world position handed to SetRootMtxTrans */
    /* +0x24 */ u8 unused_0x24[0x30 - 0x24];
    /* +0x30 */ void* source_0x30; /* the actor the effect was spawned for (a `_PLW*` for type 0) */
    /* +0x34 */ u8 unused_0x34[0x38 - 0x34];
    /* +0x38 */ struct _EFT_WORK* work_0x38;
};

/* The pool block `fn_800F8788` attaches: the effect handle, its scale and its parameter id.
 * size: 0x10 - lower bound, an approximation (the pool block the handlers walk). */
struct _EFT_WORK {
    /* +0x00 */ s32 count;
    /* +0x04 */ void* effect;
    /* +0x08 */ f32 scale;
    /* +0x0C */ u32 param_id;  /* the joint index the query is made with */
};

/* The player actor the effect was spawned for.  Only the two fields this function reads are named; the
 * full type is `Pl/pl_act.cpp`'s `_PLW`. size: 0x668 */
struct _PLW {
    /* +0x000 */ u8 unused_0x000[0x02];
    /* +0x002 */ u8 weaponClass;      /* the per-class muzzle-offset switch's operand */
    /* +0x003 */ u8 unused_0x003[0x13C - 0x03];
    /* +0x13C */ u8* physics_0x13C;   /* the body sub-object; its MHchar sits at +4 */
    /* +0x140 */ u8 unused_0x140[0x668 - 0x140];
};

/* ---------------------------------------------------------------------------------------------------
 * externs - the callees and the shared pool
 * ------------------------------------------------------------------------------------------------- */

extern void fn_80043EA8(VEC3* out);      /* a `blr` stub in the DOL: a no-op, but the call is in the bytes */
extern void fn_8005050C(MTX34* mtx);     /* likewise */
extern u32 fn_800F92F4(struct _EFT* self, u32 mode);
/* fn_800FD718 / fn_800FD860 come from their owner's header (rule 2). */

extern void* res_eft_create__FUsUsUl(u16 id, u16 param, u32 idx);
extern void mulVecMat__FPQ34nw4r4math4VEC3PQ34nw4r4math5MTX34(VEC3* out, MTX34* mtx);
extern void SetRootMtxTrans__FPQ34nw4r2ef6EffectPQ34nw4r4math4VEC3(void* effect, VEC3* pos);

/* The shared pool: the effect/parameter id tables, then the twelve per-class muzzle offsets.  The two
 * u16 tables are sized (2) on purpose: an unsized `extern` is too big for MWCC's small-data heuristic
 * and it emits a `lis`/`addi` pair where the target uses `lbl_...@sda21`. */
extern u16 lbl_807916E0[2];
extern u16 lbl_807916E4[2];
extern f32 lbl_80796658; /* 10.0f   */
extern f32 lbl_8079665C; /* -2.0f   */
extern f32 lbl_80796660; /* 190.0f  */
extern f32 lbl_80796664; /* -4.9f   */
extern f32 lbl_80796668; /* -0.7f   */
extern f32 lbl_8079666C; /* 90.0f   */
extern f32 lbl_80796670; /* -7.8f   */
extern f32 lbl_80796674; /* -10.0f  */
extern f32 lbl_80796678; /* 195.0f  */
extern f32 lbl_8079667C; /* -5.0f   */
extern f32 lbl_80796680; /* -20.0f  */
extern f32 lbl_80796684; /* 155.0f  */

/* ---------------------------------------------------------------------------------------------------
 * body
 * ------------------------------------------------------------------------------------------------- */

#pragma peephole off /* the pool-block store keeps the target's reload - see the unit header */

/* Creates the actor's effect and places it at the weapon joint the pool block names. */
void fn_800FD520(struct _EFT* self)
{
    struct _PLW* source;
    VEC3 v;
    MTX34 mtx;
    struct _EFT_WORK* work;

    work = self->work_0x38;
    fn_80043EA8(&v);
    fn_8005050C(&mtx);

    self->state_0x05++;
    work->effect = res_eft_create__FUsUsUl(lbl_807916E0[self->type_0x02],
                                           lbl_807916E4[self->type_0x02], 0);
    if (work->effect == 0) {
        fn_800FD860(self);
        return;
    }
    if (self->type_0x02 == 0) {
        source = self->source_0x30;
        if (fn_800F92F4(self, 0) == 0) {
            self->flag_0x01 = 0;
            self->state_0x05 = 3;
            return;
        }
        fn_800E0A14(source->physics_0x13C + 4, work->param_id, &mtx);
        switch (source->weaponClass) {
        default:
            break;
        case 0:
        case 7:
            v.x = lbl_80796658;
            v.y = lbl_8079665C;
            v.z = lbl_80796660;
            break;
        case 1:
            v.x = lbl_80796664;
            v.y = lbl_80796668;
            v.z = lbl_8079666C;
            break;
        case 3:
            v.x = lbl_80796670;
            v.y = lbl_80796674;
            v.z = lbl_80796678;
            break;
        case 2:
            v.x = lbl_8079667C;
            v.y = lbl_80796680;
            v.z = lbl_80796684;
            break;
        case 8:
            v.x = lbl_80796658;
            v.y = lbl_8079665C;
            v.z = lbl_80796660;
            break;
        }
        mulVecMat__FPQ34nw4r4math4VEC3PQ34nw4r4math5MTX34(&v, &mtx);
        self->pos_0x18.x = mtx.m[0][3] + v.x;
        self->pos_0x18.y = mtx.m[1][3] + v.y;
        self->pos_0x18.z = mtx.m[2][3] + v.z;
        SetRootMtxTrans__FPQ34nw4r2ef6EffectPQ34nw4r4math4VEC3(work->effect, &self->pos_0x18);
    }
    self->flag_0x01 = 1;
    fn_800FD718(self);
}

#pragma peephole reset
