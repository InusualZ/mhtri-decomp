/*
 * homebutton/hbm_hermite.cpp - software-keyboard code of the tiHKBManager band: the kana modifier tables and their
 * toggles (voiced, semi-voiced, small, cycle), the name helpers, and the cubic Hermite and rectangle helpers.
 *
 * RANGE.     .text 0x8055EAF0..0x8055F728 (16 functions); .data 0x80652340..0x80652530 (the four kana tables and their
 *            pointer array, emitted by the source); .sbss 0x80795A78..0x80795A90 (the three toggle pairs); .ctors entry.
 * FLAGS.     GC/3.0a5.2 + cflags_hkb (configure.py); the retail object has no extab/extabindex, so `#pragma exceptions off`.
 * FLAGS.     Scoped `#pragma auto_inline on` around hbmKanaToggleHandakuten/hbmKanaToggleSmall: retail inlines both into
 *            hbmKanaToggleDakuten and keeps their out-of-line copies (which call hbmKanaToggleDakuten back).
 * NAMES.     GUESS file name: the band's `hbm_` prefix + the Hermite helper; no `__FILE__` string, no runtime-dump name.
 * NAMES.     GUESS names hbmKana* (the tables hold plain/voiced/semi-voiced/small kana, U+3000 for none) and the record
 *            HbmKanaPair; map rows renamed with them (request hbm-misc).
 * SHAPES.    The pairs are initialised from the non-const table array, so MWCC emits the static initialiser
 *            __sinit_\hbm_hermite_cpp (retail 0x8055F6E8) with its .ctors entry.
 * SHAPES.    hbmKanaTogglePair tests `other != 0x3000` first; the toggles return `toggled` when it differs; hbmKanaCycle
 *            reads the table element twice and keeps the `else return c;` (retail's two return blocks).
 * SHAPES.    hbmVec4Set takes (y, w, z, x): the parameter order that reproduces retail's register assignment.
 * RESIDUALS. hbmKanaToggleDakuten: r30/r31 swapped between the parameter and the inlined toggle's result (281
 *            instructions, 81 words differ); local declaration order, a parameter copy and return forms do not move it.
 * RESIDUALS. fn_8055F650: same 20-instruction sequence, but retail keeps the quotient in f1 (`fdivs f1,f8,f0`) where
 *            ours uses f0, which recolours every temporary after it; operand orders and local spellings do not move it.
 * RESIDUALS. Unwritten: fn_8055EAF0 reads the unclaimed .sdata pair 0x80794638 (8 bytes, only it reads it; request
 *            hbm-misc#93 for the claim); fn_8055EB68 builds its local table pair from .sdata2 0x8079D718, which points at
 *            0x80652310/0x80652328 inside homebutton/hbm_value.cpp's .data claim (seam, request hbm-misc#92).
 * RESIDUALS. Seam: this unit's .sdata2 pair points at 0x80652310/0x80652328, so this TU's .data starts at 0x80652310,
 *            not at the registered 0x80652340 (homebutton/hbm_value.cpp's claim ends there).
 * RESIDUALS. .sdata2 0x8079D718..0x8079D738 is declared, not emitted (the pool constants are extern).
 */

#include "types.h"
#include "sys_mem.h"

/* The retail object has no extab/extabindex: exceptions are off for this band. */
#pragma exceptions off

#include "homebutton/hbm_vu_object.h"
#include "homebutton/hbm_hermite.h"
#include "MSL_C/wstring.h"

/* ---------------------------------------------------------------------------------------------------
 * helpers that live in other splits.  They sit in the unsplit `main` band (fn_80501xxx / fn_8054xxx /
 * fn_8053xxx), so no registered unit owns them - rule 2 leaves them a counted gap (tools/units/stylelint.py,
 * "address band interleaves modules"), the same way the neighbouring `homebutton/gui.cpp` declares them.
 * ------------------------------------------------------------------------------------------------ */
extern "C" {

s32   strlen(const char* s);

s32   strncmp(const char* a, const char* b, u32 n);

char* strncpy(char* dst, const char* src, u32 n);

void* memset(void* dst, s32 c, u32 n);

}

/* ---------------------------------------------------------------------------------------------------
 * the unit's own `.sdata2` float pool and `.sdata` globals: other splits own them, so they are
 * declared, never defined (playbook 29).
 * ------------------------------------------------------------------------------------------------ */
extern "C" {

extern f32 lbl_8079D720;

extern f32 lbl_8079D724;

extern f32 lbl_8079D728;

extern f32 lbl_8079D72C;

extern f32 lbl_8079D730;

extern f32 lbl_8079D734;

}

extern "C" {

/* ---------------------------------------------------------------------------------------------------
 * the kana modifier tables (.data) and the three toggle pairs built from them (.sbss, filled by the unit's static
 * initialiser because they read the table array)
 * ------------------------------------------------------------------------------------------------ */
/* 0x80652340: the plain kana (hiragana then katakana) every other table is indexed by (59 characters and the terminator). */
u16 hbmKanaBase[60] = {
    0x3042, 0x3044, 0x3046, 0x3048, 0x304A, 0x304B, 0x304D, 0x304F, 0x3051, 0x3053,
    0x3055, 0x3057, 0x3059, 0x305B, 0x305D, 0x305F, 0x3061, 0x3064, 0x3066, 0x3068,
    0x306F, 0x3072, 0x3075, 0x3078, 0x307B, 0x3084, 0x3086, 0x3088, 0x308F, 0x30A2,
    0x30A4, 0x30A6, 0x30A8, 0x30AA, 0x30AB, 0x30AD, 0x30AF, 0x30B1, 0x30B3, 0x30B5,
    0x30B7, 0x30B9, 0x30BB, 0x30BD, 0x30BF, 0x30C1, 0x30C4, 0x30C6, 0x30C8, 0x30CF,
    0x30D2, 0x30D5, 0x30D8, 0x30DB, 0x30E4, 0x30E6, 0x30E8, 0x30EF, 0x0000, 0x0000,
};

/* 0x806523B8: the voiced (dakuten) form of each plain kana, U+3000 where there is none (59 characters and the terminator). */
u16 hbmKanaDakutenForms[60] = {
    0x3000, 0x3000, 0x3000, 0x3000, 0x3000, 0x304C, 0x304E, 0x3050, 0x3052, 0x3054,
    0x3056, 0x3058, 0x305A, 0x305C, 0x305E, 0x3060, 0x3062, 0x3065, 0x3067, 0x3069,
    0x3070, 0x3073, 0x3076, 0x3079, 0x307C, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000,
    0x3000, 0x30F4, 0x3000, 0x3000, 0x30AC, 0x30AE, 0x30B0, 0x30B2, 0x30B4, 0x30B6,
    0x30B8, 0x30BA, 0x30BC, 0x30BE, 0x30C0, 0x30C2, 0x30C5, 0x30C7, 0x30C9, 0x30D0,
    0x30D3, 0x30D6, 0x30D9, 0x30DC, 0x3000, 0x3000, 0x3000, 0x3000, 0x0000, 0x0000,
};

/* 0x80652430: the semi-voiced (handakuten) form of each plain kana, U+3000 where there is none (59 characters and the terminator). */
u16 hbmKanaHandakutenForms[60] = {
    0x3000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000,
    0x3000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000,
    0x3071, 0x3074, 0x3077, 0x307A, 0x307D, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000,
    0x3000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000,
    0x3000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000, 0x30D1,
    0x30D4, 0x30D7, 0x30DA, 0x30DD, 0x3000, 0x3000, 0x3000, 0x3000, 0x0000, 0x0000,
};

/* 0x806524A8: the small form of each plain kana, U+3000 where there is none (59 characters and the terminator). */
u16 hbmKanaSmallForms[60] = {
    0x3041, 0x3043, 0x3045, 0x3047, 0x3049, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000,
    0x3000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3063, 0x3000, 0x3000,
    0x3000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3083, 0x3085, 0x3087, 0x308E, 0x30A1,
    0x30A3, 0x30A5, 0x30A7, 0x30A9, 0x30F5, 0x3000, 0x3000, 0x30F6, 0x3000, 0x3000,
    0x3000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000, 0x30C3, 0x3000, 0x3000, 0x3000,
    0x3000, 0x3000, 0x3000, 0x3000, 0x30E3, 0x30E5, 0x30E7, 0x30EE, 0x0000, 0x0000,
};

/* 0x80652520: the four kana tables, in the order the cycle walks them. */
const u16* hbmKanaTables[4] = {
    hbmKanaBase,
    hbmKanaDakutenForms,
    hbmKanaHandakutenForms,
    hbmKanaSmallForms,
};

HbmKanaPair hbmKanaDakuten = { hbmKanaTables[0], hbmKanaTables[1] };
HbmKanaPair hbmKanaHandakuten = { hbmKanaTables[0], hbmKanaTables[2] };
HbmKanaPair hbmKanaSmall = { hbmKanaTables[0], hbmKanaTables[3] };

/* Toggles `c` between the two tables of `pair`; a character in neither, or one whose counterpart is U+3000, is kept. */
static inline u16 hbmKanaTogglePair(const HbmKanaPair* pair, u16 c)
{
    s32 i;

    for (i = 0; i < 2; i++) {
        const u16* found = wcschr(pair->table[i], c);

        if (found != 0) {
            u16 other = pair->table[1 - i][found - pair->table[i]];

            if (other != 0x3000)
                return other;
            return c;
        }
    }
    return c;
}

/* 0x8055EC3C (0x464): toggles the voiced mark of a kana, first undoing a semi-voiced or small form. */
u16 hbmKanaToggleDakuten(u16 c)
{
    u16 plain;
    u16 toggled;

    if (c == 0x3000)
        return c;
    if (wcschr(hbmKanaHandakuten.table[1], c) != 0) {
        plain = hbmKanaToggleHandakuten(c);
        toggled = hbmKanaTogglePair(&hbmKanaDakuten, plain);
        if (toggled != plain)
            c = toggled;
        return c;
    }
    if (wcschr(hbmKanaSmall.table[1], c) != 0) {
        plain = hbmKanaToggleSmall(c);
        toggled = hbmKanaTogglePair(&hbmKanaDakuten, plain);
        if (toggled != plain)
            c = toggled;
        return c;
    }
    return hbmKanaTogglePair(&hbmKanaDakuten, c);
}

#pragma push
#pragma auto_inline on

/* 0x8055F0A0 (0x16C): toggles the semi-voiced mark of a kana, first undoing a voiced form. */
u16 hbmKanaToggleHandakuten(u16 c)
{
    u16 plain;
    u16 toggled;

    if (c == 0x3000)
        return c;
    if (wcschr(hbmKanaDakuten.table[1], c) != 0) {
        plain = hbmKanaToggleDakuten(c);
        toggled = hbmKanaTogglePair(&hbmKanaHandakuten, plain);
        if (toggled != plain)
            return toggled;
        return c;
    }
    return hbmKanaTogglePair(&hbmKanaHandakuten, c);
}

/* 0x8055F20C (0x16C): toggles the small form of a kana, first undoing a voiced form. */
u16 hbmKanaToggleSmall(u16 c)
{
    u16 plain;
    u16 toggled;

    if (c == 0x3000)
        return c;
    if (wcschr(hbmKanaDakuten.table[1], c) != 0) {
        plain = hbmKanaToggleDakuten(c);
        toggled = hbmKanaTogglePair(&hbmKanaSmall, plain);
        if (toggled != plain)
            return toggled;
        return c;
    }
    return hbmKanaTogglePair(&hbmKanaSmall, c);
}

#pragma pop

/* 0x8055F378 (0x160): cycles a kana through its plain, voiced, semi-voiced and small forms (skipping the missing ones). */
u16 hbmKanaCycle(u16 c)
{
    s32 table;
    s32 index;
    s32 step;

    if (c == 0x3000)
        return c;
    for (table = 0; table < 4; table++) {
        const u16* found = wcschr(hbmKanaTables[table], c);

        if (found != 0) {
            index = found - hbmKanaTables[table];
            break;
        }
    }
    if (table < 4) {
        for (step = 1; step < 4; step++) {
            if (hbmKanaTables[(table + step) % 4][index] != 0x3000)
                return hbmKanaTables[(table + step) % 4][index];
        }
    } else {
        return c;
    }
    return c;
}

/* ---------------------------------------------------------------------------------------------------
 * string/predicate helpers of the band's tail (the pool +0x8055F4D8..0x8055F5E8)
 * ------------------------------------------------------------------------------------------------ */
/* 0x8055F4D8 (0x38): whether `c` is a voiced kana. */
BOOL hbmKanaIsDakuten(u16 c)
{
    return wcschr(hbmKanaDakuten.table[1], c) != 0;
}

/* 0x8055F510 (0x38): whether `c` is a semi-voiced kana. */
BOOL hbmKanaIsHandakuten(u16 c)
{
    return wcschr(hbmKanaHandakuten.table[1], c) != 0;
}

/* 0x8055F548 (0x38): whether `c` is a small kana. */
BOOL hbmKanaIsSmall(u16 c)
{
    return wcschr(hbmKanaSmall.table[1], c) != 0;
}

s32 hbmNameMatches(const char* a, const char* b)
{
    u32 n = (u32)strlen(a);

    if (n >= 0x10)
        n = 0x10;
    if (strncmp(a, b, n) == 0)
        return 1;
    return 0;
}

void hbmNameCopyWithChar(char* dst, u32 n, const char* src, u32 idx, char ch)
{
    memset(dst, 0, n);
    strncpy(dst, src, n);
    dst[idx] = ch;
}

/* ---------------------------------------------------------------------------------------------------
 * constant returns
 * ------------------------------------------------------------------------------------------------ */
s32 fn_8055F648(VuObject* self) { (void)self; return 1; }

/* Cubic Hermite interpolation of the band's shaped-value helpers. */
f32 fn_8055F650(f32 a, f32 b, f32 c, f32 d, f32 e, f32 f, f32 g)
{
    f32 dt = a - b;
    f32 de = e - b;
    f32 dv = c - f;
    f32 t = dt / de;
    f32 t2 = t * t;
    f32 two_t = t + t;
    f32 u = t2 - t;
    return (c + dv * (two_t * u - t2)) - dt * (t * d - ((d + d * u) + g * u));
}

}

/* Stores the four components; the parameter order (y, w, z, x) is the one that reproduces retail's register
 * assignment (see SHAPES in the unit header). */
static inline void hbmVec4Set(VuVec4* self, f32 y, f32 w, f32 z, f32 x)
{
    self->x = x;
    self->z = z;
    self->w = w;
    self->y = y;
}

extern "C" {

void fn_8055F6A0(VuVec4* self)
{
    hbmVec4Set(self, lbl_8079D72C, lbl_8079D728, lbl_8079D724, lbl_8079D720);
}

void fn_8055F6C4(VuVec4* self)
{
    hbmVec4Set(self, lbl_8079D72C, lbl_8079D728, lbl_8079D734, lbl_8079D730);
}
}
