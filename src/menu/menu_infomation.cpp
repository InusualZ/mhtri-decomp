/* menu/menu_infomation.cpp - the equipment-information screen: the detail panels for each weapon
 * class (sword, bowgun, and the shared basis/level helpers).
 *
 * `.text` 0x8030D338..0x80313E24 (0x6AEC B, 67 symbols), extab 0x80015C24..0x80015DEC (57 framed
 * functions, one 8-byte record each) and extabindex 0x8003489C..0x80034B48 (57 12-byte records).
 *
 * MODULE AND NAME (brief section 2, evidence order).
 *   * class 1 (a `__FILE__` string): `.data` 0x805DCCDC, size 0x14, is the bare source name
 *     `"menu_infomation.cpp"` (the retail misspelling is the original), and 0x805DCCF0 (0x18 B) is
 *     its `"NW4R:Failed assertion 0"` message.  `fn_80312F84` (this range) builds the pair at
 *     0x80313020: `lis r3,0x805E; addi r3,r3,-13092` -> 0x805DCCDC and
 *     `lis r5,0x805E; addi r5,r5,-13072` -> 0x805DCCF0, i.e. an `nw4r::db::Panic(__FILE__,line,msg)`.
 *     Module `menu` (the `menu_*` file family: `menu_item.cpp`, `menu_note.cpp`, `menu_placeinfo.cpp`
 *     all sit in this band's data pool), extension `.cpp` (the range's mangled callees).
 *
 * rule 7 deferred: the symbol map has only fn_XXXXXXXX for most of this range (checked with
 * `tools/units/symedit.py range 0x8030D338 0x80313E24`: only the 4 `Put_equip_dtl_basis_*` rows are
 * named, the other 63 are bare `fn_XXXXXXXX` stems and `dumpmap.py lookup` answers `zz_<addr>_` for
 * every one of them).  The 4 named rows are written as the C++ free functions their manglings spell.
 *
 * SEAM (unproven).  The `__FILE__` string is referenced outside this range too
 * (`auto_fn_8030A328_text.o`, `auto_03_8030BF34_text.o`, `auto_fn_80313E24_text.o` ..
 * `auto_Set_equip_column_arr_text.o` at 0x8031A244), so the real `menu_infomation.cpp` TU extends
 * from at most 0x8030A328 to at least 0x8031A244 and this registration is a fragment of it; the
 * range is worked as one unit and its extent settles as the functions match (brief 8.3).
 *
 * SECTIONS beyond `.text`: extab and extabindex are this unit's own runs.  No `.data` run is claimed:
 * every table/constant the bodies read is shared with the neighbouring fragments of the same TU.
 * `datagap` (target object vs ours): ours-extra is empty in every section - ours emits .text, extab
 * and extabindex only, and the target's extra extab/extabindex bytes are the 39 functions not yet
 * written (target-extra, not a defect).  No `.sdata2`/`.rodata` partial claim is made (playbook 23).
 *
 * STATUS (official metric, this worktree's `build/RMHE08/obj/menu/menu_infomation.o` target vs our
 * `build/RMHE08/src/menu/menu_infomation.o`): **28 of 67 functions written, 27 of 28 above the 80 %
 * bar, 11 byte-identical; unit 11.754055 % fuzzy, 396 / 27372 `.text` bytes matched.**  The lowest
 * written row is `fn_80311540` (46.25: its tail-call argument is masked with `rlwinm r0,r6,0,31,28`
 * + `clrlwi` - a bitfield/`char` conversion whose source shape is not recovered, so the mask is
 * written `(u8)(c & 0x9FFFFFFF)` and MWCC folds it to a different `rlwinm`).  The 39 unwritten rows
 * are the band's larger dispatchers and state machines (`fn_80311C3C` 0xC64, `fn_80312F84` 0xBB8,
 * the three bowgun panels, the `fn_8030F87C`/`fn_8030FDA0`/`fn_80310F30`/`fn_803116D8` switch bodies);
 * they are residuals, in address order, not a reason to stop (brief 5).
 */

#include "types.h"
#include "pl.h"
#include "hud/layout.h"
#include "menu/menu_item.h"
#include "Pl/fn_8027D684.h"
#include "unsplit/menu.h"
/* draw_font_idx is declared in include/hud/layout.h (its owner) */

extern "C" int sprintf(s8*, const char*, ...);   /* 0x8045DECC, the OS Runtime's */

/* The equipment slot index the detail putters walk.  It is shared with the next band's
 * `Set_equip_column_arrangement`; until that unit registers, this file is its only user, so it lives
 * here and moves to `include/unsplit/menu.h` with the second user (rule 1).  Only +0x00/+0x04 are
 * touched by this range.  size: 0x08 (approximate: max touched offset + 1) */
struct _EQUIP_INDEX {
    /* +0x00 */ u32 equip_0x00;   /* the packed equipment word `fn_8027FF88` classifies */
    /* +0x04 */ u32 unused_0x04;
};

/* This unit's own C++-linkage exports (address order). */
void Put_equip_dtl_basis_bowgun_gan1(_PLW*, _EQUIP_INDEX*, u16, u8, _mh_ivec2_*);
void Put_equip_dtl_basis_bowgun_gan2(_PLW*, _EQUIP_INDEX*, u16, u8, _mh_ivec2_*);
void Put_equip_dtl_basis_bowgun_gan_lv(_PLW*, _EQUIP_INDEX*, u16, u8, _mh_ivec2_*);

#ifdef __cplusplus
extern "C" {
#endif

/* This unit's own forward declarations (address order).  They carry C linkage here so the
 * definitions below emit the map's bare `fn_XXXXXXXX` stems (rule 9: an `fn_` stem is not a
 * mangling). */
void fn_8030D6C0(void*, void*, void*, u16, u8);
void fn_8030D6A8(void*, void*, u16, u8);
void fn_8030D808(void*, void*, u16, u8);
void fn_8030D728(void*, void*, void*, void*, u16, u8);
void fn_8030F814(void*, void*, u16, u8);
void fn_8030F7B0(void*, void*, void*, u16, u8);
void fn_8030FCC0(void*, void*, void*, void*, u16, u8);
void fn_8030FD30(void*, void*, void*, void*, void*, void*, void*, u16);
void fn_8030FC58(void*, void*, void*, u16, u8);
void fn_80310560(void*, void*, void*, u16, u8);
void fn_803105C8(void*, void*, void*, void*, u16, u8);
void fn_80310638(void*, void*, void*, void*, void*, void*, void*, u16);
void fn_80310E58(void*, void*, void*, u16, u8);
void fn_80310EC0(void*, void*, void*, void*, u16, u8);
void fn_80310F30(void*, void*, u16, u8);
void fn_80311560(void*, void*, void*, u16, u8, u8);
void fn_803116D8(void*, void*, u16, u8, u8);
void fn_8030F87C(void*, void*, u16, u8);
void fn_8030FDA0(void*, void*, u16, u8);
void fn_803106A8(void*, void*, u16, u8);
void fn_8031077C(void*, void*, u8, void*);
void fn_8030E79C(void*, void*, void*, u16, u8);
void fn_8030E784(void*, void*, u16, u8);
void fn_8030F38C(void*, void*, void*, u16, u8);
void fn_8030F374(void*, void*, u16, u8);
void fn_8030FB78(u16, u8);
void fn_8030FB60(u16);
void fn_8030FB6C(u16);
void fn_8030FC58(void*, void*, void*, u16, u8);
void fn_8030FC40(void*, void*, u16, u8);
void fn_8030FF0C(void*, void*, void*, u16, u8);
void fn_8030FEF4(void*, void*, u16, u8);
void fn_80310560(void*, void*, void*, u16, u8);
void fn_80310548(void*, void*, u16, u8);
void fn_80310E58(void*, void*, void*, u16, u8);
void fn_80310E40(void*, void*, u16, u8);
void fn_80311560(void*, void*, void*, u16, u8, u8);
void fn_80311540(void*, void*, u16, u32, u8);
void fn_80312DF0(u32*);

extern u32 lbl_805DCD48[];          /* .data: the per-colour packed word table */
extern const char lbl_80792BE4[3];  /* .sdata "%d" */
extern const char lbl_80792BE8[3];  /* .sdata "%s" */
extern const char lbl_80792BEC[5];  /* .sdata "%d%s" */

#ifdef __cplusplus
}
#endif

/* 0x8030D338 - the sword's colour panel.  Classify the slot (`fn_8027FF88`), then draw the sprite
 * run `get_menu_lsp_tbl(0x8E|0x8F)` names (each frame recoloured from `lbl_805DCD48`) and, for the
 * colour forms, up to two label rows from the string table. */
void Put_equip_dtl_basis_sword_colorX(_PLW* plw, _EQUIP_INDEX* idx, u16 a, u16 b,
                                      _mh_ivec2_* pos, u8 c) {    u8 n = fn_8027FF88(idx->equip_0x00);
    if (n == 0) {
        return;
    }
    int sel = n - 1;
    u32 color = lbl_805DCD48[(u8)sel];
    u16* tbl;
    _mh_ivec2_ vec;
    u32 srcA;
    u32 srcB;
    s8 buf[8];
    _SPR_DATA_ spr;
    if ((u8)sel < 3) {
        tbl = (u16*)get_menu_lsp_tbl(0x8E);
        if (a == 0) {
            srcA = *(u32*)pos;
            fn_802E06B0(0x80E, &vec, &srcA);
        } else if (pos == NULL) {
            get_lsp_data(0, &vec);
        } else {
            return;
        }
    } else {
        tbl = (u16*)get_menu_lsp_tbl(0x8F);
        if (b == 0) {
            srcB = *(u32*)pos;
            fn_802E06B0(0x819, &vec, &srcB);
        } else if (pos == NULL) {
            get_lsp_data(0, &vec);
        } else {
            return;
        }
    }
    while (*tbl != 0xFFFF) {
        fn_801E6850(&spr, get_lsp_data(*tbl, NULL));
        spr.color = color;
        draw_sprite(spr, &vec);
        tbl++;
    }
    draw_sprite_ary(((u16**)get_menu_lsp_tbl(0x95))[(u8)sel], &vec);
    switch ((u8)sel) {
    case 1:
        sprintf(buf, lbl_80792BE8, get_str_tbl(0x32)[2]);
        draw_font_idx(0x811, buf, 4, &vec);
        sprintf(buf, lbl_80792BE4, 1);
        draw_font_idx(0x812, buf, 4, &vec);
        break;
    case 2:
        sprintf(buf, lbl_80792BE8, get_str_tbl(0x32)[2]);
        draw_font_idx(0x814, buf, 4, &vec);
        sprintf(buf, lbl_80792BEC, 0xF, get_str_tbl(0x32)[3]);
        draw_font_idx(0x815, buf, 4, &vec);
        break;
    case 3:
        sprintf(buf, lbl_80792BE8, get_str_tbl(0x32)[2]);
        draw_font_idx(0x81C, buf, 4, &vec);
        sprintf(buf, lbl_80792BE4, 1);
        draw_font_idx(0x81D, buf, 4, &vec);
        break;
    case 4:
        sprintf(buf, lbl_80792BE8, get_str_tbl(0x32)[2]);
        draw_font_idx(0x81F, buf, 4, &vec);
        sprintf(buf, lbl_80792BEC, 0xF, get_str_tbl(0x32)[3]);
        draw_font_idx(0x820, buf, 4, &vec);
        sprintf(buf, lbl_80792BE8, get_str_tbl(0x32)[2]);
        draw_font_idx(0x81C, buf, 4, &vec);
        sprintf(buf, lbl_80792BE4, 1);
        draw_font_idx(0x81D, buf, 4, &vec);
        break;
    }
}

/* ---- the sword/bowgun "basis" dispatch helpers, 0x8030D6A8..0x8030D980 ---- */

/* 0x8030D6A8 - the sword-detail colour entry: force the sub-index to 0 and tail into the shared
 * `fn_8030D6C0`. */
void fn_8030D6A8(void* s0, void* s1, u16 a, u8 b) {
    fn_8030D6C0(s0, s1, 0, a, b);
}

/* 0x8030F374 - the same zero-sub-index entry for the `fn_8030F38C` band. */
void fn_8030F374(void* s0, void* s1, u16 a, u8 b) {
    fn_8030F38C(s0, s1, 0, a, b);
}

/* 0x8030E784 - the `fn_8030E79C` band's zero-sub-index entry. */
void fn_8030E784(void* s0, void* s1, u16 a, u8 b) {
    fn_8030E79C(s0, s1, 0, a, b);
}

/* 0x8030FC40 - the `fn_8030FC58` band's zero-sub-index entry. */
void fn_8030FC40(void* s0, void* s1, u16 a, u8 b) {
    fn_8030FC58(s0, s1, 0, a, b);
}

/* 0x8030FEF4 - the `fn_8030FF0C` band's zero-sub-index entry. */
void fn_8030FEF4(void* s0, void* s1, u16 a, u8 b) {
    fn_8030FF0C(s0, s1, 0, a, b);
}

/* 0x80310548 - the `fn_80310560` band's zero-sub-index entry. */
void fn_80310548(void* s0, void* s1, u16 a, u8 b) {
    fn_80310560(s0, s1, 0, a, b);
}

/* 0x80310E40 - the `fn_80310E58` band's zero-sub-index entry. */
void fn_80310E40(void* s0, void* s1, u16 a, u8 b) {
    fn_80310E58(s0, s1, 0, a, b);
}

/* 0x80311540 - the `fn_80311560` band's zero-sub-index entry (its third argument's top two bits
 * are cleared on the way through). */
void fn_80311540(void* s0, void* s1, u16 a, u32 c, u8 d) {
    fn_80311560(s0, s1, 0, a, (u8)(c & 0x9FFFFFFF), d);
}

/* 0x8030FB60 / 0x8030FB6C - the two fixed-kind entries into `fn_8030FB78`. */
void fn_8030FB60(u16 a) {
    fn_8030FB78(a, 6);
}

void fn_8030FB6C(u16 a) {
    fn_8030FB78(a, 7);
}

/* 0x80312DF0 - draw the sprite run `get_menu_lsp_tbl(0xCD)` names at the row `fn_802E06B0`
 * positions from the caller's value. */
void fn_80312DF0(u32* p) {
    u32 v = *p;
    _mh_ivec2_ pos;
    fn_802E06B0(0x9A5, &pos, &v);
    draw_sprite_ary((u16*)get_menu_lsp_tbl(0xCD), &pos);
}

/* 0x8030D6C0 - the shared sword/small-blade branch: resolve the caller's variant into a stack
 * record with `fn_80315440` and hand off to the dispatcher (unless it reports "done"). */
void fn_8030D6C0(void* s0, void* s1, void* s2, u16 a, u8 b) {
    u8 out[24];
    u8 spb;
    spb = b;
    if (fn_80315440(s0, s1, s2, &spb, out) == 1) {
        return;
    }
    fn_8030D808(s0, out, a, spb);
}

/* 0x8030FB78 - the bowgun/ammo row: derive the offset from the 0x8A7 anchor (or zero), then draw
 * the `get_str_tbl(0x2E)[kind]` label at entry 0x8A8. */
void fn_8030FB78(u16 a, u8 b) {
    _mh_ivec2_ pos;
    _mh_ivec2_ d;
    if (a != 0) {
        get_lsp_data(0x8A7, &pos);
        get_lsp_data(a, &d);
        d.x = d.x - pos.x;
        d.y = d.y - pos.y;
    } else {
        d.x = 0;
        d.y = 0;
    }
    u32 tmp = *(u32*)&d;
    fn_802E06B0(0x8A7, &pos, &tmp);
    draw_font_idx(0x8A8, (s8*)((u8**)get_str_tbl(0x2E))[b], 0, &pos);
}

/* 0x8030D808 - the weapon detail's shared body: position the panel from the 0x84E anchor (or the
 * caller's offset), then dispatch on `fn_80315A60`'s row - one label, the caller's value, or the
 * three bowgun panels plus the trailing text row. */
void fn_8030D808(void* s0, void* s1, u16 a, u8 b) {
    _mh_ivec2_ pos;
    _mh_ivec2_ off;
    _mh_ivec2_ src;
    if (a != 0) {
        get_lsp_data(0x84E, &pos);
        get_lsp_data(a, &off);
        off.x = off.x - pos.x;
        off.y = off.y - pos.y;
    } else {
        off.x = 0;
        off.y = 0;
    }
    src = off;
    fn_802E06B0(0x84E, &pos, &src);
    u8 r = fn_80315A60(s0, s1, b);
    switch (r) {
    case 1:
        return;
    case 2:
        draw_font_idx(0x8D2, (s8*)((u8**)get_str_tbl(0x2E))[1], 0, &pos);
        return;
    default:
        fn_80315274(s0, s1, b);
        Put_equip_dtl_basis_bowgun_gan1((_PLW*)s0, (_EQUIP_INDEX*)s1, 0, b, &off);
        Put_equip_dtl_basis_bowgun_gan2((_PLW*)s0, (_EQUIP_INDEX*)s1, 0, b, &off);
        Put_equip_dtl_basis_bowgun_gan_lv((_PLW*)s0, (_EQUIP_INDEX*)s1, 0, b, &off);
        fn_803153F8(s0, b);
        return;
    }
}

/* 0x8030D728 - the second small-weapon branch entry (resolves through `fn_803155BC`). */
void fn_8030D728(void* s0, void* p1, void* p2, void* p3, u16 a, u8 b) {
    u8 out[24];
    if (fn_803155BC(s0, p1, p2, p3, b, out) == 1) {
        return;
    }
    fn_8030D808(s0, out, a, b);
}

/* ---- the same resolve-then-dispatch shape, one family per weapon sub-band ----
 * Each `fn_8030F7B0`-style entry resolves the caller's variant into a stack record with
 * `fn_80315440` and hands it to its band's dispatcher; `fn_8030FCC0`-style resolves through
 * `fn_803155BC` (six registers plus a slot); `fn_8030FD30`-style through `fn_80315730`, whose
 * ninth (stack) argument is the record. */

/* 0x8030F7B0 - the 0x8030F87C band's `fn_80315440` entry. */
void fn_8030F7B0(void* s0, void* s1, void* s2, u16 a, u8 b) {
    u8 out[24];
    u8 spb;
    spb = b;
    if (fn_80315440(s0, s1, s2, &spb, out) == 1) {
        return;
    }
    fn_8030F87C(s0, out, a, spb);
}

/* 0x8030F814 - the `fn_8030F87C` band's variant whose variant word is forced to 0. */
void fn_8030F814(void* s0, void* s1, u16 a, u8 b) {
    u8 out[24];
    u8 spb;
    spb = b;
    if (fn_80315440(s0, s1, 0, &spb, out) == 1) {
        return;
    }
    fn_8030F87C(s0, out, a, spb);
}

/* 0x8030FC58 - the 0x8030FDA0 band's `fn_80315440` entry. */
void fn_8030FC58(void* s0, void* s1, void* s2, u16 a, u8 b) {
    u8 out[24];
    u8 spb;
    spb = b;
    if (fn_80315440(s0, s1, s2, &spb, out) == 1) {
        return;
    }
    fn_8030FDA0(s0, out, a, spb);
}

/* 0x8030FCC0 - the 0x8030FDA0 band's `fn_803155BC` entry. */
void fn_8030FCC0(void* s0, void* p1, void* p2, void* p3, u16 a, u8 b) {
    u8 out[24];
    if (fn_803155BC(s0, p1, p2, p3, b, out) == 1) {
        return;
    }
    fn_8030FDA0(s0, out, a, b);
}

/* 0x8030FD30 - the 0x8030FDA0 band's `fn_80315730` entry. */
void fn_8030FD30(void* s0, void* p1, void* p2, void* p3, void* p4, void* p5, void* p6, u16 a) {
    u8 out[59];
    u8 tmp;
    if (fn_80315730(s0, p1, p2, p3, p4, p5, p6, &tmp, out) == 1) {
        return;
    }
    fn_8030FDA0(s0, out, a, tmp);
}

/* 0x80310560 - the 0x803106A8 band's `fn_80315440` entry. */
void fn_80310560(void* s0, void* s1, void* s2, u16 a, u8 b) {
    u8 out[24];
    u8 spb;
    spb = b;
    if (fn_80315440(s0, s1, s2, &spb, out) == 1) {
        return;
    }
    fn_803106A8(s0, out, a, spb);
}

/* 0x803105C8 - the 0x803106A8 band's `fn_803155BC` entry. */
void fn_803105C8(void* s0, void* p1, void* p2, void* p3, u16 a, u8 b) {
    u8 out[24];
    if (fn_803155BC(s0, p1, p2, p3, b, out) == 1) {
        return;
    }
    fn_803106A8(s0, out, a, b);
}

/* 0x80310638 - the 0x803106A8 band's `fn_80315730` entry. */
void fn_80310638(void* s0, void* p1, void* p2, void* p3, void* p4, void* p5, void* p6, u16 a) {
    u8 out[59];
    u8 tmp;
    if (fn_80315730(s0, p1, p2, p3, p4, p5, p6, &tmp, out) == 1) {
        return;
    }
    fn_803106A8(s0, out, a, tmp);
}

/* 0x80310E58 - the 0x80310F30 band's `fn_80315440` entry. */
void fn_80310E58(void* s0, void* s1, void* s2, u16 a, u8 b) {
    u8 out[24];
    u8 spb;
    spb = b;
    if (fn_80315440(s0, s1, s2, &spb, out) == 1) {
        return;
    }
    fn_80310F30(s0, out, a, spb);
}

/* 0x80310EC0 - the 0x80310F30 band's `fn_803155BC` entry. */
void fn_80310EC0(void* s0, void* p1, void* p2, void* p3, u16 a, u8 b) {
    u8 out[24];
    if (fn_803155BC(s0, p1, p2, p3, b, out) == 1) {
        return;
    }
    fn_80310F30(s0, out, a, b);
}

/* 0x80311560 - the 0x803116D8 band's `fn_80315440` entry (its resolved byte is masked before it
 * is handed on). */
void fn_80311560(void* s0, void* s1, void* s2, u16 a, u8 b, u8 c) {
    u8 out[36];
    u8 spb;
    spb = b;
    if (fn_80315440(s0, s1, s2, &spb, out) == 1) {
        return;
    }
    spb = (u8)(spb & 0x9FFFFFFF);
    fn_803116D8(s0, out, a, spb, c);
}

/* 0x803106A8 - the 0x8031077C band's body: position the panel from the 0x8F0 anchor (or the
 * caller's offset) and hand the resulting anchor to `fn_8031077C`. */
void fn_803106A8(void* s0, void* s1, u16 a, u8 b) {
    _mh_ivec2_ pos;
    _mh_ivec2_ off;
    _mh_ivec2_ src;
    _mh_ivec2_ copy;
    if (a != 0) {
        get_lsp_data(0x8F0, &pos);
        get_lsp_data(a, &off);
        off.x = off.x - pos.x;
        off.y = off.y - pos.y;
    } else {
        off.x = 0;
        off.y = 0;
    }
    src = off;
    fn_802E06B0(0x8F0, &pos, &src);
    copy = pos;
    fn_8031077C(s0, s1, b, &copy);
}
