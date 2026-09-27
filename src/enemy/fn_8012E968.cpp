/* enemy/fn_8012E968.cpp - the enemy area/group wait set, 3 function(s), 0x8012E968..0x8012EC74.
 * Naming note: the symbol map has only fn_XXXXXXXX for this range (checked with symedit: every fn_
 * name this file uses is a bare .text entry in config/RMHE08/symbols.txt)
 *
 * What it is.  `fn_8012E968` is a mode-dispatched predicate over the enemy work record `_ENEMY_WORK`:
 *   * modes 0..5 - `fn_8012E8F4` (the neighbouring unit's frame-stamp comparison) is handed
 *     `60.0f * Screen_w.frame_scale * {3.0f, 5.0f, 10.0f, 15.0f, 25.0f, 35.0f}`, i.e. the wait the
 *     mode selects, scaled by the frame rate.  Those six constants are the `.sdata2` run
 *     0x80796C9C..0x80796CB8 this range references (values read out of `build/RMHE08/main.elf`);
 *   * mode 6 - "is this enemy's area and group already taken": it walks the live `_ENEMY_WORK` records
 *     `get_move_work_adrs(3)` returns (0xB18 apart, `get_move_work_max(3)` of them) and then the
 *     area/group table at `lbl_806A54E0` (32 groups of four 0x44-byte entries, 0x2200 B), and returns
 *     0 as soon as one record matches `field_0x00A` and `group`;
 *   * any other mode falls out of the switch to `return 0`.
 * `fn_8012EC3C` and `fn_8012EC60` are the two one-byte predicates `enemy/fn_8013BE60.c`'s handlers
 * call on the same record: "`field_0x89F` is 2 or 3" and "`mode_0x8AA` is 1".
 *
 * Registration.  This is the run discovery proposed (`proposal/8012E968_fn_8012E968`), registered once
 * at its final home.  Module `enemy` by naming class 3 (both bracketing units are `enemy/`, and the
 * code is enemy work-record code: `_ENEMY_WORK`, `get_move_work_adrs(3)`); the file keeps the map's own
 * stem by naming class 4 - no `__FILE__` string survives in the range (the closest `.data` names belong
 * to other units) and the runtime dump answers only `zz_` placeholders - see the module note in the
 * `enemy` block of `configure.py`.
 *
 * Seam (checked, not assumed).  This is not a continuation of `enemy/fn_8012BDF4.cpp`
 * (0x8012BDF4..0x8012E968, the unit that ends where this range starts).  `tudiscover at 0x8012E968`:
 *   * `expand`'s must-link closure around `fn_8012E968` is that function alone - no anchor reaches
 *     across 0x8012E968, so nothing ties this range to the unit below;
 *   * the one strong observation in the band is the `.sdata2` pool-run jump
 *     `lbl_80796CB4 -> lbl_80796CB8`, whose four legal cuts are this range's own edges
 *     (0x8012E968, 0x8012EC74) and its two interior function starts (0x8012EC3C, 0x8012EC60), so the
 *     extent is the evidence's widest cut and settles as the three bodies match (`tu.verdict:
 *     unproven`; the header says so, docs/plan.md 8.3).
 *
 * Sections.  `.text` plus the `extab`/`extabindex` pair the target object carries, read out of the
 * retired `auto_fn_8012E968_text.o` and its `.note.split` `VIRTSplit` record: `extab`
 * 0x8000CD2C..0x8000CD34 (8 B) and `extabindex` 0x800272AC..0x800272B8 (12 B - one entry, whose own
 * relocations name `fn_8012E968` and its `@etb_8000CD2C`), plus the `.text` line.  The `.sdata2` run
 * 0x80796C9C..0x80796CB8 and the `.bss` objects `Screen_w`/`lbl_806A54E0` are *read*, never defined:
 * the pool literals stay with the data pass (playbook 29), so nothing but `.text`/`extab`/`extabindex`
 * is claimed.
 *
 * Types.  `_ENEMY_WORK` is the shared 0xB18 record in `include/enemy/ENEMY_WORK.h` (rule 1); this unit
 * named the byte it measures that the header still had as padding (`+0x8AA` as `mode_0x8AA`) and gave
 * `+0x00A`'s measured meaning a comment - its *name* stays the header's `field_0x00A`, because
 * `src/enemy/fn_80170600.cpp` already reads that byte under it.  The area/group table entry and the
 * `Screen_w`/pool views are local
 * (rules 1/3/4/5): `Screen_w` is unsplit `.bss` whose bracketing units name different modules, so its
 * view stays private - the same read `src/fn_80056F24.cpp` and `src/draw_shape.cpp` keep.
 * `get_move_work_adrs`/`get_move_work_max` are owned by `src/ef/fn_800CDB2C.cpp`, which does not
 * declare them in its header yet; they are declared here at C++ scope with their real signatures, as
 * `enemy/fn_8012BDF4.cpp` does, so the target's `get_move_work_adrs__FUc` relocations pair (rule 9).
 * `fn_8012E8F4` is declared in its owner's header, `enemy/fn_8012BDF4.h` (rule 2), which this file
 * includes.
 *
 * Flags.  The unit's command line is the `enemy` lib's (`configure.py`), and the file carries the same
 * scoped `#pragma peephole off` as its neighbour `enemy/fn_8012BDF4.cpp`.  Measured on this unit:
 * with the pragma `fn_8012E968` is 99.03 % (724 B, the target's size); without it the `-O3` peephole
 * fuses retail's `clrlwi` + `cmpwi` on the mode argument into `clrlwi.` and the body is 716 B,
 * 97.71 %.  `fn_8012EC3C`/`fn_8012EC60` are 100.00 % either way.
 *
 * Status and residual.
 *   * `fn_8012EC60` 100.0 %, `fn_8012EC3C` 100.0 % (both byte-identical);
 *   * `fn_8012E968` 99.03 % - every instruction is the target's, in the target's order, at the
 *     target's size (724 B); the residual is *register naming* in the mode-6 table walk: retail keeps
 *     the walk pointer in r5 and the four blocks' load scratch in r3, this build has that pair the
 *     other way round (r3/r5).  Both registers are named by the allocator, not by the source.
 *   * The walk's counter is retail's own and is written the only way this compiler keeps it: the
 *     second argument (`mode`) is reused as the table index, so its home stays r4 and the
 *     `addi r4,r4,3` survives next to the four-record `entry` walk.  A fresh local does not reproduce
 *     it - with `return 1` MWCC drops the counter (98.3 %), and observing a fresh local after the loop
 *     (`return (idx == 0x60)`) makes it emit a three-instruction `cntlzw` tail where retail has
 *     `li r3,1` (97.5 %).  This is the target's shape, not a tidier one.
 */

#include "types.h"

#include "enemy/ENEMY_WORK.h"
#include "enemy/fn_8012BDF4.h"

#pragma peephole off

/* `Screen_w` (0x8065903C, .bss, unsplit): only the frame scale at +0x14, the one `main.cpp` writes
 * (`sw->f20 = 60.0f / vcount`).  A private view - `main.cpp`'s `ScreenWork` documents the rest.
 * size: 0x54 */
typedef struct ScreenFrameScale {
    /* +0x00 */ u8 pad_0x00[0x14];
    /* +0x14 */ f32 frame_scale;
    /* +0x18 */ u8 pad_0x18[0x54 - 0x18];
} ScreenFrameScale; /* size: 0x54 */

extern "C" ScreenFrameScale Screen_w;

/* The `.sdata2` pool literals this range reads, in address order (values from `main.elf`). */
extern "C" f32 lbl_80796C9C;   /* 3.0f - mode 0 */
extern "C" f32 lbl_80796CA0;   /* 60.0f - the frame scale every mode is multiplied by */
extern "C" f32 lbl_80796CA4;   /* 5.0f - mode 1 */
extern "C" f32 lbl_80796CA8;   /* 10.0f - mode 2 */
extern "C" f32 lbl_80796CAC;   /* 15.0f - mode 3 */
extern "C" f32 lbl_80796CB0;   /* 25.0f - mode 4 */
extern "C" f32 lbl_80796CB4;   /* 35.0f - mode 5 */

/* One entry of the area/group table `lbl_806A54E0` (0x806A54E0, .bss, unsplit): four of them per
 * group, 32 groups (the `.bss` run 0x806A54E0..0x806A76E0 the retired object's data run gives).
 * Only the four bytes `fn_8012E968` tests are named.
 * size: 0x44 */
typedef struct EmAreaEntry {
    /* +0x00 */ u8 active;           /* nonzero while the entry is in use */
    /* +0x01 */ u8 unused_0x01;
    /* +0x02 */ u8 group;            /* matched against `_ENEMY_WORK::group` */
    /* +0x03 */ u8 unused_0x03[0x0A - 0x03];
    /* +0x0A */ u8 area_no;          /* matched against `_ENEMY_WORK::field_0x00A` */
    /* +0x0B */ u8 unused_0x0B;
    /* +0x0C */ u8 field_0x0C;       /* must be 1 for the entry to count */
    /* +0x0D */ u8 unused_0x0D[0x44 - 0x0D];
} EmAreaEntry; /* size: 0x44 */

extern "C" EmAreaEntry lbl_806A54E0[32][4];

/* The work-record accessors `src/ef/fn_800CDB2C.cpp` owns (see the file header). */
_ENEMY_WORK* get_move_work_adrs(u8 kind);
u16 get_move_work_max(u8 kind);

/* The mode-dispatched predicate (see the file header). */
extern "C" s32 fn_8012E968(_ENEMY_WORK* self, u8 mode)
{
    _ENEMY_WORK* work;
    EmAreaEntry* entry;
    u16 max;
    s32 idx;

    switch (mode) {
    case 0:
        return fn_8012E8F4(lbl_80796CA0 * Screen_w.frame_scale * lbl_80796C9C);
    case 1:
        return fn_8012E8F4(lbl_80796CA0 * Screen_w.frame_scale * lbl_80796CA4);
    case 2:
        return fn_8012E8F4(lbl_80796CA0 * Screen_w.frame_scale * lbl_80796CA8);
    case 3:
        return fn_8012E8F4(lbl_80796CA0 * Screen_w.frame_scale * lbl_80796CAC);
    case 4:
        return fn_8012E8F4(lbl_80796CA0 * Screen_w.frame_scale * lbl_80796CB0);
    case 5:
        return fn_8012E8F4(lbl_80796CA0 * Screen_w.frame_scale * lbl_80796CB4);
    case 6:
        work = (_ENEMY_WORK*)get_move_work_adrs(3);
        max = get_move_work_max(3);
        for (idx = 0; idx < max; idx++) {
            if (work->active != 0 && work->area_no == self->field_0x00A &&
                work->team == self->group) {
                return 0;
            }
            work++;
        }
        /* The table walk: 0x60 / 3 = the 32 groups the table holds, four records per group.  `mode`
         * is the target's own counter (see the file header, residual). */
        for (entry = &lbl_806A54E0[0][0], mode = 0; mode < 0x60; mode += 3, entry += 4) {
            if (entry[0].active != 0 && entry[0].area_no == self->field_0x00A &&
                entry[0].group == self->group && entry[0].field_0x0C == 1) {
                return 0;
            }
            if (entry[1].active != 0 && entry[1].area_no == self->field_0x00A &&
                entry[1].group == self->group && entry[1].field_0x0C == 1) {
                return 0;
            }
            if (entry[2].active != 0 && entry[2].area_no == self->field_0x00A &&
                entry[2].group == self->group && entry[2].field_0x0C == 1) {
                return 0;
            }
            if (entry[3].active != 0 && entry[3].area_no == self->field_0x00A &&
                entry[3].group == self->group && entry[3].field_0x0C == 1) {
                return 0;
            }
        }
        return 1;
    }
    return 0;
}

/* Whether the record's `field_0x89F` is 2 or 3 - the two-value range test `enemy/fn_8013BE60.c`'s
 * handlers gate on.  Retail's branchless `subfic`/`orc`/`srwi`/`subf`/`srwi` run is this compiler's
 * spelling of the unsigned range test, so the comparison is written as one. */
extern "C" s32 fn_8012EC3C(_ENEMY_WORK* self)
{
    return (u32)(self->field_0x89F - 2) <= 1;
}

/* Whether the record's latched mode is 1 (`fn_80130A10` latches it, 0/1). */
extern "C" s32 fn_8012EC60(_ENEMY_WORK* self)
{
    return self->mode_0x8AA == 1;
}
