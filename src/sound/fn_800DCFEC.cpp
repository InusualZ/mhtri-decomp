/* auto/800DCFEC_fn_800DCFEC.cpp - one function, 0x800DCFEC..0x800DD1F0.
 *
 * Language: the target object references the two shared getters under their C++ manglings
 * (`get_now_areano__Fv`/`get_now_mapno__Fv`, read from the target's own `.symtab`), so the original TU
 * was C++.  This file was `fn_800DCFEC.c`; a `.c` cannot spell those names, which is what the
 * relocation audit caught.  Renamed to `.cpp` (the registered path follows in configure.py) so an
 * ordinary declaration mangles; the plain `fn_*` callees and the definition keep C linkage with
 * `extern "C"`, exactly like `g3d/g3d_resanmcamera.cpp` did on 2026-09-24.
 * rule 7 deferred: the symbol map has only fn_XXXXXXXX for this range (checked with symedit: every fn_ name this file uses is a bare .text entry in config/RMHE08/symbols.txt)
 *
 * Requests SE work 46 at the caller's position, with the SE code picked from the current map region
 * (`fn_802B0668` of `get_now_mapno`) and area number (`get_now_areano`).  The range was claimed in bulk
 * from the DOL's own layout (docs/plan.md 12 item 5); the seam rests on a pinned .data pool run - the
 * unit's own 11-entry switch table, `jumptable_80597B50`.
 *
 * Data: the object now emits that switch table itself, so `.data 0x80597B50..0x80597B7C` is this unit's
 * and is requested for claiming below (today it is only a splits.txt comment).  The three `.sdata2`
 * floats at 0x80796410 are `extern`-declared and never defined (playbook 29), so this unit emits only
 * references to them and does not claim them.
 *
 * Load-bearing source shape: `code` is deliberately left unassigned on the default path (cases 0, 6 and
 * `> 10`), which is what leaves the target's `r4` untouched at the tail call.
 *
 * The name stays the map's `fn_800DCFEC`: the shared runtime dump has only its `zz_00dcfec_` placeholder.
 *
 * One codegen deviation, and it has a proven flag behind it.  The target carries the non-record
 * `clrlwi rX,rY,24` + `cmpwi rX,0` for the three zero tests where the committed cflags fuse them into
 * `clrlwi.` + `bne`, leaving the function three instructions short (97.52 %); `-opt nopeephole` on the
 * real command line, with the pragma below removed, reproduces the object instruction for instruction,
 * so the original TU was built peephole-off.  The pragma carries the deviation until a cflags group does
 * (playbook 33: the flag is the preferred home).  ~15 source shapes for those tests (`== 0`, `!= 0`,
 * `<= 0`, `< 1`, `& 0xFF`, a `switch`, `u8`/`u32` locals, `(u8)`/`(s32)` casts, C and C++, and the
 * `-O2`/`-O4,p` levels) were measured first and none produces the non-fused pair.
 *
 * Inventory, addresses and sizes: `python tools/units/ledger.py unit auto/800DCFEC_fn_800DCFEC.c`.
 */

#include "types.h"
#include "mh3_pad.h" /* VEC3_ctor / copyVec3 / setVec3 (rule 2) */

/* A 3-float engine vector.  `Pl/pl_act.cpp` carries the same type as `nw4r::math::VEC3`; the two belong
 * in one header (rules 1-2 are not linted, see tools/units/stylelint.py).  size: 0xC */
typedef struct VEC3 {
    /* 0x0 */ f32 x;
    /* 0x4 */ f32 y;
    /* 0x8 */ f32 z;
} VEC3;

/* The two getters are C++-mangled in the target (`get_now_mapno__Fv`/`get_now_areano__Fv`); they are
 * still unsplit, so the declarations live here as ordinary C++ functions.  The `fn_*` callees are
 * plain (C linkage) in the target, so their declarations are wrapped in `extern "C"`. */
u8 get_now_mapno(void);
u8 get_now_areano(void);
extern "C" {
u32 fn_802B0668(u8 mapno);
u32 fn_800DA72C(u32 se_work, u32 se_code, VEC3* pos);
}

/* This unit's private .sdata2 position pool (0x80796410..0x8079641C), referenced but not defined here. */
extern const f32 lbl_80796410;
extern const f32 lbl_80796414;
extern const f32 lbl_80796418;

/* This unit's single codegen deviation (see the file header).  The file is one function, so the region
 * is file-wide and the only thing it changes is the zero tests' record form. */
#pragma peephole off

/* Requests SE work 46 at the caller's position, with the SE code the current map region and area number
 * select. */
extern "C" void fn_800DCFEC(u32 arg0, VEC3* pos)
{
    u32 m = fn_802B0668((u8)get_now_mapno());
    u8 areano = get_now_areano();
    u32 se_code;

    switch ((s32)(u8)m) {
    case 1:
        se_code = (u8)arg0 % 3;
        break;
    case 2:
        if (areano == 6) {
            se_code = (u8)arg0 % 2 + 3;
        } else {
            se_code = 10;
        }
        break;
    case 3:
        switch (areano) {
        case 5:
            se_code = 4;
            break;
        case 9:
            se_code = 5;
            break;
        case 6:
        case 10:
            se_code = (u8)arg0 + 3;
            break;
        default:
            return;
        }
        break;
    case 4:
        if (areano != 1) {
            return;
        }
        se_code = 3;
        break;
    case 5:
        switch (areano) {
        case 2:
        case 4:
            se_code = (u8)arg0 + 3;
            break;
        default:
            return;
        }
        break;
    case 7:
        switch (areano) {
        case 0:
            se_code = (u8)arg0 + 3;
            break;
        case 1:
            se_code = 5;
            break;
        case 2:
            if ((u8)arg0 == 0) {
                VEC3 fixed_pos;

                setVec3(&fixed_pos, lbl_80796410, lbl_80796414, lbl_80796418);
                fn_800DA72C(46, 8, &fixed_pos);
                return;
            }
            se_code = (u8)arg0 + 5;
            break;
        default:
            return;
        }
        break;
    case 8:
        if (areano != 1) {
            return;
        }
        se_code = 3;
        break;
    case 9:
        if (areano != 0) {
            return;
        }
        se_code = 3;
        break;
    case 10:
        if (areano != 1) {
            return;
        }
        se_code = (u8)arg0 + 3;
    }
    fn_800DA72C(46, se_code, pos);
}

#pragma peephole reset
