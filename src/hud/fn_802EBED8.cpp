/* hud/fn_802EBED8.cpp - the cockpit HUD's quest-window band (`.text` 0x802EBED8..0x802F140C, 54
 * functions / 21812 B).
 *
 * Naming note: the symbol map has only fn_XXXXXXXX for this range (checked with
 * `tools/symbols/symedit.py --section .text range 0x802EBED8 0x802F140C`: every row in the range is
 * a bare `fn_` stem, and `tools/symbols/dumpmap.py lookup 0x802EBED8` answers `zz_02ebed8_`), so
 * every body below keeps the map's stem.
 *
 * Home, name and seam, from evidence (brief section 2):
 *   - class 4 (nothing supports a name).  No `.data`/`.sdata` string of the range is a source-file
 *     name: the range's only `.data` references (0x805D6310..0x805D6940) are the mask/table blobs
 *     `fn_802ED588` and friends walk, and `grep -nE '5d08|805d'` over the range's disassembly finds
 *     no `__FILE__` load.  The dump answers `zz_` for every address, so there is no class-2 name
 *     either.  The file therefore keeps its map stem (`hud/fn_802EBED8.cpp`).
 *   - module `hud`: the band is the cockpit HUD's continuation above `hud/cockpit_quest.cpp` - it
 *     reads the same `_PLW` act fields (`field_0x3D8`/`field_0x3DC`, `field_0x309`, `slot_id[26]`)
 *     and the quest-window record (`cockpit_work`, `get_lsp_data`, `draw_sprite_*`), and its
 *     neighbours `layout.cpp` (0x802E0740..0x802E4978) and `cockpit_quest.cpp`
 *     (0x802E7408..0x802EBED8) are both registered in the `hud` lib.
 *   - the seam is unproven.  `tudiscover.py at 0x802EBED8` must-links `quest_marker_draw`/`fn_802EC200`
 *     and offers strong left cuts at 0x802EBBD0, 0x802EBE2C and 0x802EBED8 (all on the `.sdata2`
 *     run jump `lbl_8079A980 -> lbl_8079A988`); this unit claims the brief's cut 0x802EBED8, which
 *     is a clean function boundary (the band below ends at 0x802EBE2C's tail).  The sibling
 *     `cockpit_quest.cpp` header records the same pool break as a candidate seam it did not take.
 *
 * Sections this unit owns: .text 0x802EBED8..0x802F140C, extab 0x8001529C..0x80015424 and
 * extabindex 0x80033A50..0x80033C9C - both runs are 49 records (8 and 12 bytes), one per framed
 * function of the range, and both start exactly where `hud/cockpit_quest.cpp`'s runs end.  The five
 * unframed functions (fn_802ED588, fn_802ED6F4, fn_802EDAF0, fn_802EDB0C, fn_802EF400) carry no
 * record, which is why the runs tile 49 and not 54.  No `.data`/`.sdata`/`.sdata2` range is claimed
 * (a partial pool claim is not linkable - `hud/layout.cpp`'s header records the mwld error); the
 * tables the range reads are declared and never defined (playbook 29).
 *
 * Flags: `cflags_hud` (= `cflags_main` + `-opt nopeephole`, the lib flag the band's siblings settled -
 * see the group's evidence in `configure.py`): this band keeps the unfused `clrlwi` + `cmpwi` pairs
 * (`fn_802ED588` ... `clrlwi r0,r4,24` + `cmpwi r0,0x0` is the smallest witness), and carries the 49
 * extab records `-Cpp_exceptions on` emits.
 *
 * Score at this commit (the official report metric, one `recompile.py --measure` per symbol against
 * this worktree's own split of the same range): 5.96 % fuzzy (504 of 21812 `.text` bytes), **6 of the
 * 54 bodies byte-identical**, 11 of them at or above the 80 % bar:
 *   100.00  fn_802EDAF0 (28 B), fn_802ED7E4 (80 B), fn_802EECA0 (108 B), fn_802EE330 (124 B),
 *           fn_802EF400 (36 B), fn_802EF6B0 (128 B)
 *    98.18  fn_802EF62C (132 B)
 *    96.67  fn_802ED6F4 (240 B)
 *    94.03  fn_802EEC24 (124 B)
 *    91.09  fn_802EF0A0 (92 B)
 *    90.91  fn_802ED834 (88 B)
 * The 41 functions without a body score 0 and dominate the unit percentage.  The whole-object build
 * links: `ninja` ends `build/RMHE08/main.dol: OK`.
 *
 * Residuals of the written bodies:
 *  - `fn_802EC6C4` 72.33 % (60 B target, 60 B ours).  Every instruction is present in the target's
 *    order except that retail folds `1 | ~v` into one `orc` (`li r3,1; orc r3,r3,r4`) where ours
 *    emits `not r0,r4; ori r0,r0,1`.  Tried `(1 | ~v)`, `(~v | 1)`, a `u32` vs `u8` local, and
 *    `#pragma peephole on` around the body: all 72.33.  The fold is an emitter choice this MWCC
 *    revision does not make under the unit's flags (playbook 22).
 *  - `fn_802ED588` 73.47 % (152 B target, 152 B ours).  Structurally complete - one `kind == 0`
 *    test selecting both 4-byte-word mask tables, two bottom-tested loops, the same 38
 *    instructions - but MWCC puts `result`/`bit`/the second table pointer in different registers
 *    than retail (`r5`/`r6`/`r8` vs `r6`/`r5`/`r4`); declaring `bit` before `result` lifted it
 *    72.08 -> 73.47.  Playbook 22 (register allocation, not shape).
 *  - `fn_802ED834` 90.91 %, `fn_802EF0A0` 91.09 %, `fn_802EEC24` 94.03 %, `fn_802ED6F4` 96.67 %:
 *    one or two register/instruction choices off (the `fn_802ED6F4` countdown keeps an `extsb`
 *    retail does not).
 *
 * Not written (41, in address order): the rest of the band.  The largest are `fn_802F09C4` (0x8E0),
 * `fn_802EDCE4` (0x5A4), `fn_802F02EC` (0x4F8), `fn_802ECE28` (0x3E4), `quest_marker_draw` (0x328),
 * `fn_802EC200` (0x2F0), `fn_802EE3AC` (0x2B0), `fn_802EE97C` (0x2A8), `fn_802ED20C` (0x274); all of
 * them drive the same `cockpit_work` / `_PLW` records this unit's written bodies view.  Their m2c
 * shape oracle is kept for the next lane in `build/tmp/m2c/` (throwaway, gitignored).
 */

#include "types.h"
#include "pl.h"
#include "nw4r/math.h"
#include "hud/layout.h"
#include "hud/fn_802EBED8.h"
#include "menu/menu_item.h"

/* The band needs the unfused `clrlwi` + `cmpwi` pairs (see the header); that is the lib flag
 * `-opt nopeephole`, which `configure.py` applies to `hud` as `cflags_hud`. */

/* 0x802EC6C4 (0x3C).  A one-bit predicate over `fn_803311A0`'s byte: the branch-free
 * `subfic`/`orc`/`subf` sequence is the target's own shape. */
u32 fn_802EC6C4(void)
{
    u32 v = fn_803311A0();
    return ((~v | 1) - ((1 - v) >> 1)) >> 31;
}

/* 0x802EDAF0 (0x1C).  Zero a 4-byte run and the second byte of a following flag record - the
 * caller (`fn_802E4978`) hands the two pointers into its 0x194 record. */
void fn_802EDAF0(_PLW* plw, u8* a, u8* b)
{
    (void)plw;
    a[0] = 0;
    a[1] = 0;
    a[2] = 0;
    a[3] = 0;
    b[1] = 0;
}

/* 0x802ED588 (0x98).  Fold the two 0x3D8/0x3DC act bitfields through the two 4-byte mask tables
 * (the `kind` selects the table pair) into one bitmask: bit `n` is raised when the n-th non-zero
 * mask word is set in the field. */
s32 fn_802ED588(_PLW* plw, u8 kind)
{
    s32 bit = 1;
    s32 result = 0;
    const u32* a;
    const u32* b;

    if (kind == 0) {
        a = lbl_805D6338;
        b = lbl_805D6360;
    } else {
        a = lbl_805D638C;
        b = lbl_805D63B0;
    }
    while (*a != 0) {
        if ((plw->field_0x3D8 & *a) != 0) {
            result |= bit;
        }
        bit <<= 1;
        a++;
    }
    while (*b != 0) {
        if ((plw->field_0x3DC & *b) != 0) {
            result |= bit;
        }
        bit <<= 1;
        b++;
    }
    return result;
}

/* 0x802ED6F4 (0xF0).  Advance the two-byte cursor over the caller's bit table: while `timer` is
 * negative, arm on the lowest set bit; while it is non-negative, keep counting down on the same
 * bit and, once it runs out, arm on the next set bit (wrapping at `count`). */
void fn_802ED6F4(u32 mask, s32 count, Cursor2* cur)
{
    if (mask == 0) {
        cur->timer = -1;
        return;
    }
    if (cur->timer < 0) {
        for (s32 i = 0; i < count; i++) {
            if ((mask & (1u << i)) != 0) {
                cur->index = i;
                cur->timer = 20;
                return;
            }
        }
        return;
    }
    if ((mask & (1u << cur->index)) != 0 && cur->timer > 0) {
        cur->timer -= 1;
        return;
    }
    cur->timer = -1;
    for (s32 i = 0; i < count; i++) {
        cur->index = cur->index + 1;
        if (cur->index >= count) {
            cur->index = 0;
        }
        if ((mask & (1u << cur->index)) != 0) {
            cur->timer = 20;
            return;
        }
    }
}

/* 0x802ED7E4 (0x50).  The 18/19-entry cursor's `kind == 0` lumping: mask the act fields, then arm
 * the cursor with the count the kind selects. */
void fn_802ED7E4(_PLW* plw, Cursor2* cur, u8 kind)
{
    u8 k = kind;
    s32 count = (k == 0) ? 19 : 18;
    fn_802ED6F4((u32)fn_802ED588(plw, k), count, cur);
}

/* 0x802ED834 (0x58).  The second selector's 4/6-entry cursor. */
void fn_802ED834(_PLW* plw, Cursor2* cur, u8 kind)
{
    s32 count = 4;
    if (kind == 0) {
        count = 6;
    }
    fn_802ED6F4((u32)fn_802ED620(plw, kind), count, cur);
}

/* 0x802EECA0 (0x6C).  Count the live equipment slots (0..25) `fn_802EEC24` accepts. */
s16 fn_802EECA0(_PLW* plw)
{
    s16 count = 0;
    u32 i = 0;
    do {
        if (fn_802EEC24(plw, i) != 0) {
            count = count + 1;
        }
        i = i + 1;
    } while (i < 26);
    return count;
}

/* 0x802EEC24 (0x7C).  A slot is "live" when its value is positive, it holds an item, the item
 * passes `item_category_ck`'s flag test and its record's first byte is not 1. */
s32 fn_802EEC24(_PLW* plw, u16 idx)
{
    _SLOTENT* slot = &plw->slot_id[idx];
    if (slot->value > 0) {
        if (slot->item_id != 0) {
            if (item_category_ck(slot->item_id, 8) != 0) {
                if (GetItemData(slot->item_id)->kind_0x00 != 1) {
                    return 1;
                }
            }
        }
    }
    return 0;
}

/* 0x802EE330 (0x7C).  One cockpit icon: when the act's status byte overlaps the caller's mask,
 * draw the position record through the 2D library in the fixed 0xFFE300FF colour, else play the
 * icon's animation. */
void fn_802EE330(_PLW* plw, u16 id, u8 anim, u8 mask, const _mh_ivec2_* pos)
{
    if ((plw->field_0x309 & mask) != 0) {
        _SPR_DATA_ spr;
        sprite_frame_apply(&spr, id, anim, 0);
        spr.color = 0xFFE300FF;
        draw_sprite(spr, pos);
    } else {
        draw_sprite_anim_idx(id, anim, pos);
    }
}

/* 0x802EF0A0 (0x5C).  Project the caller's position through `eft053_shell_pos_project` and hand the
 * 12-byte result to the same-file `fn_802EEDE4` (the three word copies are the target's own shape). */
void fn_802EF0A0(_PLW* plw, void* out)
{
    Pos12 scratch;
    Pos12 pos;

    (void)out;
    VEC3_ctor((VEC3*)&scratch);
    eft053_shell_pos_project(plw, &scratch);
    pos = scratch;
    fn_802EEDE4(&pos);
}

/* 0x802EF62C (0x84).  The quest HUD's gate: off while `menu_item_frame_update(0)` or `fn_802BE39C()` hold,
 * on only in area 1 with the act's state byte at 0 or 0xE. */
s32 fn_802EF62C(_PLW* plw)
{
    if (menu_item_frame_update(0) != 0) {
        return 0;
    }
    if (fn_802BE39C() == 1) {
        return 0;
    }

    u8 kind = plw->field_0x3E4;
    if ((kind == 0 || kind == 0xE) && get_now_areano() == 1) {
        return 1;
    }
    return 0;
}

/* 0x802EF6B0 (0x80).  Arm/decay the blink record after the two quest work records: zero it when
 * the quest gate `fn_802EF62C` is closed, else reload a 10-frame countdown from `fn_8028E4F0` and
 * tick it down. */
void fn_802EF6B0(void)
{
    QuestBlink* blink = &cockpit_state;

    if (fn_802EF62C(cockpit_work[0]) == 0) {
        blink->timer = 0;
        return;
    }

    u8 v = fn_8028E4F0();
    if (v != 0) {
        blink->timer = 10;
        blink->flag = v;
        return;
    }
    if (blink->timer != 0) {
        blink->timer -= 1;
    }
}

/* 0x802EF400 (0x24).  Copy one 5-byte selection record field by field (the widths are the target's
 * `lbz`/`lbz`/`lhz`/`lbz`). */
void fn_802EF400(QuestSel* dst, const QuestSel* src)
{
    dst->field_0x0 = src->field_0x0;
    dst->field_0x1 = src->field_0x1;
    dst->field_0x2 = src->field_0x2;
    dst->field_0x4 = src->field_0x4;
}
