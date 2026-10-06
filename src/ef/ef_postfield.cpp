/*
 * ef/ef_postfield.cpp - nw4r::ef::PostField (the per-frame field step over a manager's particles: its collision,
 *   speed-limit, random and convergence sub-steps) and the res_drawparam_ac.h / res_emitterparam_ac.h inline
 *   accessors its TU emits, with their users.
 * RANGE. .text 0x800AEE48-0x800B2878 (29 functions); extab 0x80009F74-0x8000A044, extabindex 0x800232D4-0x8002340C,
 *   .data 0x80593580-0x80593690 (the `__FILE__` string "ef_postfield.cpp" first, then the header strings
 *   "res_drawparam_ac.h" 0x805935E4 and "res_emitterparam_ac.h" 0x8059362C/0x80593678), .sdata2
 *   0x807960E8-0x80796120 (read only by 0x800AEE48-0x800B1C80).  Right edge 0x800B2878: the three inline accessors
 *   that cite the header strings (0x800B23A4, 0x800B2598, 0x800B26F8) are each followed by their users (0x800B24BC..
 *   0x800B2590, 0x800B26B0/0x800B26D4, 0x800B2810/0x800B2840), and `ef/ef_resource.cpp` opens with the resource
 *   singleton's getter 0x800B2878 (it reads `.bss` 0x80694598, the `.ctors` entry's object).
 * FLAGS. `cflags_main`; file-wide `#pragma peephole off`.
 * NAMES. The file name is the TU's own `__FILE__` string (0x80593580), not a guess.  The helpers keep the map's
 *   `fn_` stems.
 * RESIDUALS. 19 rows unwritten (declared, never defined): 0x800AEE48-0x800B0B90, 0x800B0BC8-0x800B24BC,
 *   0x800B2598-0x800B26B0, 0x800B26F8-0x800B2810.  The 10 written rows are byte-identical; the source order differs
 *   from retail's, so our `.text` and the extab and extabindex records run in another order.
 *   flipcheck: `.data` and `.sdata2` claimed, not emitted (declared by their map names, playbook 29).
 */

#include "types.h"
#include "gx.h"
#include "ef.h"
#include "ef/ef_drawstrategyimpl.h"
#include "ef/fn_800AEE48.h"
#include "ef/ef_postfield.h"
#include "sys_mem.h"
#include "unsplit/ef.h"
#include "mh3_pad.h" /* VEC3_ctor / copyVec3 / setVec3 (rule 2) */

#ifdef __cplusplus
extern "C" {
#endif

#pragma peephole off

/* This TU's `__FILE__`/assert strings (its claimed `.data`), declared, never defined. */
extern char lbl_80593580[];  /* "ef_postfield.cpp"                                        .data 0x80593580 */
extern char lbl_80593598[];  /* "NW4R:Failed assertion 0"                                  .data 0x80593598 */
extern char lbl_805935E4[];  /* "res_drawparam_ac.h"                                       .data 0x805935E4 */
extern char lbl_8059362C[];  /* "res_emitterparam_ac.h"                                    .data 0x8059362C */
extern char lbl_80593678[];  /* "res_emitterparam_ac.h"                                    .data 0x80593678 */

/* Helpers owned by other units. */
extern void  PSVECSubtract(Vec* dst, const Vec* a, const Vec* b);

/* --------------------------------------------------------------------------------------------- *
 * The resource-parameter accessor family (res_drawparam_ac.h / res_emitterparam_ac.h, 0x800B23A4..0x800B2878).
 * --------------------------------------------------------------------------------------------- */

/* The `RES_ACCESS` record: its first word is the checked `mData` pointer, and `fn_800B23A4` asserts
 * and returns it (the out-of-line `IsValidPointer` instantiation - `ef.h`). */
typedef struct EfResDrawParam {
    u8 pad_0x00[0x94];   /* +0x00 */
    u8 field_0x94[0x4C]; /* +0x94  the parameter payload the accessors return */
} EfResDrawParam; /* size: 0xE0 */

/* The state word `fn_800B23A4` returns (a `Res*` block whose flags sit at +0). */
typedef struct EfResState {
    u16 flags_0x00; /* +0x00 */
} EfResState; /* size: 0x02 */

/* A record whose first field holds a pointer (the two `sts`-style setters store one). */
typedef struct EfResSlot {
    void* field_0x00; /* +0x00 */
} EfResSlot; /* size: 0x04 */

extern EfResState* fn_800B23A4(void* self);
extern EfResDrawParam* fn_800A5484(void* arg);
extern EfResDrawParam* fn_800A4864(EfResDrawParam* arg);

/* Sets or clears the 0x400 flag on the accessor's state word. */
void fn_800B24BC(void* self, int enable) {
    if (enable != 0) {
        fn_800B23A4(self)->flags_0x00 |= 0x400;
    } else {
        fn_800B23A4(self)->flags_0x00 &= ~0x400;
    }
}

/* Stores a pointer into the record's first field. */
void fn_800B2544(EfResSlot* self, void* value) {
    self->field_0x00 = value;
}

/* Stores a pointer into the record's first field. */
void fn_800B2590(EfResSlot* self, void* value) {
    self->field_0x00 = value;
}

/* Resolves the parameter through the two accessors and stores it. */
void fn_800B2504(EfResSlot* self, void* arg) {
    fn_800B2544(self, fn_800A4864(fn_800A5484(arg)));
}

/* Resolves the parameter and stores the second block's +0x94 field. */
void fn_800B254C(EfResSlot* self, void* arg) {
    fn_800B2590(self, &fn_800A4864(fn_800A5484(arg))->field_0x94);
}

/* The resolved parameter block the +0x4C/+0x54 accessors read. */
typedef struct EfResParams {
    u8 pad_0x00[0x4C]; /* +0x00 */
    f32 field_0x4C;    /* +0x4C */
    u8 pad_0x50[0x04]; /* +0x50 */
    VEC3 field_0x54;   /* +0x54 */
    u8 pad_0x60[0x34]; /* +0x60 */
} EfResParams; /* size: 0x94 */

extern EfResParams* fn_800B2598(void* self);
extern EfResParams* fn_800B26F8(void* self);

/* The resolved parameter's scale. */
f32 fn_800B26B0(void* self) {
    return fn_800B2598(self)->field_0x4C;
}

/* The resolved parameter's second block. */
void* fn_800B26D4(void* self) {
    return &fn_800B2598(self)->field_0x54;
}

/* Sets the resolved parameter's scale. */
void fn_800B2810(void* self, f32 value) {
    fn_800B26F8(self)->field_0x4C = value;
}

/* Copies a source block over the resolved parameter's second block. */
void fn_800B2840(void* self, const nw4r::math::VEC3* src) {
    copyVec3(&fn_800B26F8(self)->field_0x54, src);
}

/* Subtracts `b` from `self` in place and returns `self`. */
Vec* fn_800B0B90(Vec* self, Vec* b) {
    PSVECSubtract(self, self, b);
    return self;
}

#ifdef __cplusplus
}
#endif
