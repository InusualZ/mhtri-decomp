/*
 * Naming note: the symbol map has only fn_XXXXXXXX for this range (checked with
 * `python tools/symbols/dumpmap.py lookup <addr>` - the shared runtime dump answers `zz_<addr>_` for every
 * unnamed function here, so there is no better name to take).  The mangled names the range does carry
 * (`drawshape_exec__Fv`, `set_mydata2vs__FUcUc`, `write_wpad_memory__FUcUc`, `rotMatrixX__FUlP...`,
 * `atan2ang__Fff`, ...) are spelled by their real declarations, never as callable identifiers (rule 9).
 * `vec3_scale` (0x80051EE0, `out = in * s`) is a GUESS name from its body and its callers.
 * GUESS: `vec3_len` (0x80050F24): the square root of `vec3_length_sq`, a vector's length.
 *
 * The game-root draw/gallery band.  `.text` 0x8004CAD8..0x8005270C (phase 4 cut: the old range ran to 0x80054C64, 277 functions).
 * The `draw_shape` 2D pipeline's `drawshape_*` half from 0x8005270C on is `draw_shape.cpp`'s now.
 * One maximal unclaimed run (attribute.py `fn_8004CAD8.cpp`): its seam is unproven
 * (`capped at --max-bytes, seam is a guess`) and the body work is plainly several families - the
 * work-block accessors and quest/VS result buffers (0x8004CAD8..0x8004D4xx), the gallery / Wii-message /
 * Wii-remote-memory helpers (0x8004E7xx..0x8004FE88), the nw4r-math wrappers (0x8004FFxx..0x800518xx) and
 * the draw_shape 2D pipeline (0x80051894..0x800547A8) - so the boundary is a hint, not a proven TU edge.
 *
 * Naming - which evidence class decided it.
 *   * Class 1 (`__FILE__` string) FAILS for this range.  Its `.data` pool holds the nw4r/g3d header
 *     asserts its inlined math and ResPltt/ResTex accessors reference (`arithmetic.h` 0x8058175C,
 *     `g3d_respltt_ac.h` 0x80581980/0x805819B0/0x805819E0, `g3d_restex_ac.h` 0x80581A20/0x80581A50) and
 *     the generic `%s::%s: Object not valid.` strings, but no game source-file name.  The
 *     `s_draw_shape.cpp` string at 0x8058178C belongs to the NEXT unit: its only referrers are
 *     0x80055C5C and the `*_tex_load` functions, all above this range's end 0x80054C64.
 *   * Class 2 (runtime-dump name) FAILS: `dumpmap.py lookup 0x8004CAD8` answers `zz_004cad8_`, and the
 *     `join` pass proposes names (`DBClose`, `GXPosition3f32`, `J3DColorBlockLightOff::setColorChanNum`)
 *     that are address coincidences from another build's dump - not evidence.
 *   * Class 3 decides it: the file keeps the map's stem `set_slot_none`, registered in the game-root band -
 *     the `main` lib at the `src/` root with `cflags_main`, exactly where its link neighbours sit
 *     (`fn_80040598.cpp`, `mh3_pad.cpp` 0x800408A8..0x80047398, `fn_80047398.cpp` 0x80047398..0x8004C9A0,
 *     the `8004C9A0` run 0x8004C9A0..0x8004CAD8 - which ends where this unit begins).  Module `main` is a
 *     recorded decision, not an invention; a later pass that finds a `__FILE__` string or a subsystem seam
 *     can re-home it.
 *
 * What the written bodies do: the small work-block accessors and the quest/VS result buffers
 * (`get_qResult_work` / `clear_qResult_work` / `clear_FqResult_work` over the 0x438 B and 0x27C B blocks,
 * `get_vsUser_work` over the two 0x100 B VS slots, `score_add_clamped`'s 0..9999999 counter clamp) and two of
 * the nw4r math helpers the rest of the range is built on (`setVector3`, `copyMat33`).
 *
 * Status: partial (phase B first pass).  9 of the unit's 201 symbols have bodies, measured against the retired
 * per-range targets (the auto_*_text.o objects under build/RMHE08/obj/): 8 at 100 % (`score_add_clamped`,
 * `fn_8004D134`, `get_qResult_work`, `clear_FqResult_work`, `clear_qResult_work`, `fn_8004D1A4`,
 * `setVector3`, `copyMat33`) and `get_vsUser_work` at 81.67 %.  The remaining ~268 symbols are unwritten
 * (0 %), biggest first `drawshape_exec` (0x4BC), `write_wpad_memory` (0x434), `fn_8004CDA4` (0x318),
 * `fn_80051894` (0x2E8), `set_mydata2vs` (0x270), `fn_8004E1C4` (0x260).
 *
 * Measurement path (this unit has no single registered target object, so `recompile.py --measure` cannot
 * run): the source is compiled with MAIN's real main-lib command line
 * (`ninja -t commands build/RMHE08/src/fn_80040598.o`, rewritten for this source by `recompile.rewrite`)
 * and each symbol is scored with `recompile.measure` against the retired per-range object that contains
 * it - `auto_03_8004D0BC_text.o` (the 0x8004D0BC group), `auto_03_8004FFA4_text.o` (`setVector3`),
 * `auto_03_800516F0_text.o` (`copyMat33`), `auto_fn_8004D1A4_text.o` (`fn_8004D1A4`).
 *
 * Residual (recorded, not worked around):
 *   * `get_vsUser_work` stops at 81.67 %: the target orders `cmpwi r3,2` before the `slwi r3,8` slot
 *     offset and conditional-returns with `bltlr`, while MWCC schedules the shift first and uses a
 *     different base register (`addi r0` vs `addi r3`).  The variant applied (`u32 offset = index * 0x100`
 *     hoisted before the range branch) is the best of three tried; the two alternatives score 67.92 %
 *     (plain `if (index < 2) return base + index*0x100;`) and 63.33 % (ternary `return index < 2 ? slot : 0`).
 *   * The accessors that read `system_w` (0x806585E0, `main.cpp`'s SystemWork) are NOT written:
 *     `get_userdata`, `fn_8004D0BC`, `gallery_bit_off`/`gallery_open`/`gallery_def_set` and the whole
 *     0x8004E7xx family reach `system_w`'s fields directly (`+0x888`, `+0x95C`, ...).  Reaching them from a
 *     second unit needs `SystemWork` moved to `include/` first (rule 1) - a cross-unit refactor, recorded
 *     here rather than copied (rule 1 forbids a local copy).
 *   * The nw4r-math wrappers `rotMatrixX/Y/Z`, `rotLocalMat*`, `rotVecX/Y/Z`, `atan2ang`, `calcVecAng*`,
 *     `calcDistanceSqXZ`, `mulVecMat*` are NOT written: they need the unnamed in-range helpers
 *     (`anim_tick_cos`/`fn_8005024C` = Sin/CosFIdx of a bit-cast u16, `sqrt_f32` = FrSqrt) and the
 *     `SinFIdx`/`CosFIdx`/`FrSqrt` declarations, which have no owner header yet - a rule-2 gap, recorded
 *     rather than re-declared here.
 *   * `set_slot_none` (the 16-way `kind` dispatch over the +0x92..+0x9C half-words) is declared but not
 *     written: those six fields have no context name yet (rule 5).
 *
 * Inventory / addresses / sizes: `python tools/units/ledger.py unit fn_8004CAD8.cpp`.
 *   flipcheck: `.bss` claimed, not emitted.
 *   flipcheck: `.rodata` claimed, not emitted.
 *   flipcheck: `.sbss` claimed, not emitted.
 *   flipcheck: `.sdata` claimed, not emitted.
 *   flipcheck: `.sdata2` claimed, not emitted.
 * NAMES. GUESS (from each body and its callers): get_FqResult_work
 *   GUESS (from each body and its callers): vec3_dist_sq
 *   GUESS (from each body and its callers): userdata_progress_flag_ck
 *   GUESS (from its 39 call sites, all `v *= s`): vec3_scale_in_place (0x800513F0)
 */

#include "types.h"
#include "nw4r/math.h"                /* nw4r::math::VEC3 / MTX34 + the free-function declarations (rule 9) */
#include "Runtime.PPCEABI.H/memset.h" /* owned by Runtime.PPCEABI.H/memset.c (rule 2) */
#include "fn_8004CAD8/get_qResult_work.h" /* get_qResult_work (its own leaf header) */
#include "quest/quest_result_work.h"        /* Q_ResultWork, the record the accessor hands out */

/* --- the unit's own work blocks (declared, never defined - the split object defines them) ---------- */

/* The 0x27C B `FqResult` buffer and the 0x438 B `qResult` buffer the accessors hand out.  Opaque here:
 * no written body reads a field, only the base and the memset length are used. */
extern u8 lbl_80669F68[0x27C];
extern u8 lbl_8066A1E8[0x438];
/* The two 0x100 B VS user slots get_vsUser_work indexes (0/1). */
extern u8 lbl_8066A620[0x200];

/* --- the unit's own functions, declared so each has one signature --------------------------------- */

extern "C" void score_add_clamped(s32 delta, s32* value);
extern "C" void* get_FqResult_work(void);
extern "C" void fn_8004D1A4(void);
/* These four carry a C++ mangling in the map (`__Fv`/`__Fl`), so they are real C++ functions: the
 * compiler produces the map name from the plain spelling (rule 9 - the mangled form is never written). */
void* get_vsUser_work(s32 index);
void clear_FqResult_work(void);
void clear_qResult_work(void);

/* --- bodies, in address order --------------------------------------------------------------------- */

/* Clamp `*value += delta` into [0, 9999999].  The map gives no mangling, so it is C linkage. */
extern "C" void score_add_clamped(s32 delta, s32* value)
{
    *value += delta;
    if (*value > 9999999) {
        *value = 9999999;
    } else if (*value < 0) {
        *value = 0;
    }
}

/* The 0x27C B FqResult buffer. */
extern "C" void* get_FqResult_work(void)
{
    return lbl_80669F68;
}

/* The 0x438 B qResult buffer. */
Q_ResultWork* get_qResult_work(void)
{
    return (Q_ResultWork*)lbl_8066A1E8;
}

/* One of the two 0x100 B VS user slots, or null when the index is out of range. */
void* get_vsUser_work(s32 index)
{
    if (index < 0) {
        return 0;
    }
    /* The target hoists the `index << 8` before the range branch and conditional-returns (`bltlr`). */
    u32 offset = (u32)index * 0x100;
    if (index >= 2) {
        return 0;
    }
    return lbl_8066A620 + offset;
}

void clear_FqResult_work(void)
{
    memset(lbl_80669F68, 0, sizeof(lbl_80669F68));
}

void clear_qResult_work(void)
{
    memset(lbl_8066A1E8, 0, sizeof(lbl_8066A1E8));
}

/* Clears both 0x100 B VS slots (fn_8004D1A4 clears 0x8066A620 and 0x8066A720). */
extern "C" void fn_8004D1A4(void)
{
    memset(lbl_8066A620, 0, 0x100);
    memset(lbl_8066A620 + 0x100, 0, 0x100);
}

/* --- the nw4r math free functions this unit defines (declared in nw4r/math.h, rule 9) ------- */

void setVector3(VEC3* v, f32 x, f32 y, f32 z)
{
    v->x = x;
    v->y = y;
    v->z = z;
}

/* Copies the 3x3 rotation of `src` into `dst` (the translation column at +0x0C/+0x1C/+0x2C is left). */
void copyMat33(MTX34* dst, MTX34* src)
{
    for (int row = 0; row < 3; row++) {
        dst->m[row][0] = src->m[row][0];
        dst->m[row][1] = src->m[row][1];
        dst->m[row][2] = src->m[row][2];
    }
}
