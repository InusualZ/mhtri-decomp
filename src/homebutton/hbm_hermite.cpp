/*
 * homebutton/hbm_hermite.cpp - HOME-button (HBM) software-keyboard code, `.text` 0x8055EAF0..0x8055F728 (16 functions, 9 reconstructed).
 *
 * Phase 4 recut: 9 function(s) of 0x8055EAF0..0x8055F728 from the former registered unit `homebutton/keyboard_ui.cpp`.
 *
 * Name: GUESS - `hbm_` (the HOME-button menu library, keyboard part) + the cubic Hermite interpolation of the shaped-value helpers, from the dominant function descriptions
 * below; the band has no `__FILE__` string and the runtime dump names no function, so every function keeps its `fn_<addr>`
 * placeholder.  The band was built with C++ exceptions off (no extab/extabindex in the target objects), so the scoped
 * `#pragma exceptions off` below is carried from the absorbed sources and applied to every piece.
 *
 * Residuals: partial reconstruction - the functions not defined below keep the target object's bytes; the evidence, type views and
 * per-function residuals of the absorbed sources are in docs/splits/phase4/homebutton-carried-notes.md; the type views are in
 * `homebutton/fn_8054E894.h`, `homebutton/hbm_widget.h`, `homebutton/hbm_vu_object.h`, `homebutton/hbm_kb_object.h` and `homebutton/gui_manager.h`.
 */

#include "types.h"
#include "sys_mem.h"

/* The retail object has no extab/extabindex: exceptions are off for this band. */
#pragma exceptions off

#include "homebutton/hbm_vu_object.h"
#include "homebutton/hbm_hermite.h"

/* ---------------------------------------------------------------------------------------------------
 * helpers that live in other splits.  They sit in the unsplit `main` band (fn_80501xxx / fn_8054xxx /
 * fn_8053xxx), so no registered unit owns them - rule 2 leaves them a counted gap (tools/units/stylelint.py,
 * "address band interleaves modules"), the same way the neighbouring `homebutton/gui.cpp` declares them.
 * ------------------------------------------------------------------------------------------------ */
extern "C" {

s32   wcschr(void* base, void* key);

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

extern u32 lbl_80795A78[2];  /* small-data records whose word at +4 is the search base */

extern u32 lbl_80795A80[2];

extern u32 lbl_80795A88[2];

}

extern "C" {

/* ---------------------------------------------------------------------------------------------------
 * string/predicate helpers of the band's tail (the pool +0x8055F4D8..0x8055F5E8)
 * ------------------------------------------------------------------------------------------------ */
s32 fn_8055F4D8(void* self)
{
    u32* base = lbl_80795A78;
    return wcschr((void*)base[1], self) != 0;
}

s32 fn_8055F510(void* self)
{
    u32* base = lbl_80795A80;
    return wcschr((void*)base[1], self) != 0;
}

s32 fn_8055F548(void* self)
{
    u32* base = lbl_80795A88;
    return wcschr((void*)base[1], self) != 0;
}

s32 fn_8055F580(const char* a, const char* b)
{
    u32 n = (u32)strlen(a);

    if (n >= 0x10)
        n = 0x10;
    if (strncmp(a, b, n) == 0)
        return 1;
    return 0;
}

void fn_8055F5E8(char* dst, u32 n, const char* src, u32 idx, char ch)
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
    f32 t = (a - b) / (e - b);
    f32 t2 = t * t;
    f32 two_t = t + t;
    f32 base = t * d;
    f32 u = t2 - t;
    f32 v = d * u;
    f32 w = two_t * u;
    f32 x = g * u;

    v = d + v;
    w = w - t2;
    v = v + x;
    x = w * f;
    base = base - v;
    x = c + x;
    base = (a - b) * base;
    x = x - base;
    return x;
}

void fn_8055F6A0(VuVec4* self)
{
    f32 a = lbl_8079D720;
    f32 b = lbl_8079D724;
    f32 c = lbl_8079D728;
    f32 d = lbl_8079D72C;

    self->x = a;
    self->z = b;
    self->w = c;
    self->y = d;
}

void fn_8055F6C4(VuVec4* self)
{
    f32 a = lbl_8079D730;
    f32 b = lbl_8079D734;
    f32 c = lbl_8079D728;
    f32 d = lbl_8079D72C;

    self->x = a;
    self->z = b;
    self->w = c;
    self->y = d;
}
}
