/*
 * stage/stg_w.cpp - the stage-work block and its accessors.
 *
 * `.text` 0x802AD9C0-0x802B2978 (98 functions, 20408 B).  Registered from
 * `proposal/802AD9C0_get_stg_w__Fv.cpp`.
 *
 * Module `stage`.  The lib and the sibling below are `stage/` (`stage/fn_802B2978.c`), and the
 * range's own entry points name the subsystem: `get_stg_w` hands back the stage-work block `stage_w`
 * at 0x806B87C0, and `get_stg_weapon_work` / `get_stg_eft_col` / `get_now_mapno` / `get_now_areano`
 * are its accessors.  Language C++: the map defines six real manglings (`get_stg_w__Fv`,
 * `get_now_areano__Fv`, ...) and the range calls out through mangled names (`Pl_master_ck__FP4_PLW`,
 * `body_set__FP7_BODY_WP10_BODY_DATAUcUlUc`).
 *
 * Name.  No `__FILE__` string covers this range (the `menu_item.cpp` string at 0x805CDFC8 is
 * referenced only by the preceding range, at 0x802A54C0/0x802A5900/0x802A64DC) and the shared
 * runtime dump answers only `zz_XXXXXXXX_` for the range's unnamed functions, so the file name is
 * class 3 of brief section 2: what the code does plus the neighbours' scheme - the block the range
 * is built around is the stage (`stg`) work record.
 *
 * Naming note: the symbol map has only fn_XXXXXXXX for 92 of this range's 98 symbols (checked
 * with `python tools/symbols/symedit.py range 0x802AD9C0 0x802B2978` and
 * `python tools/symbols/dumpmap.py lookup` over the inventory: every unnamed entry is a bare
 * `fn_XXXXXXXX` in config/RMHE08/symbols.txt and the runtime dump answers `zz_XXXXXXXX_`); the six
 * named symbols are get_stg_w / get_stg_weapon_work / get_stg_eft_col / get_now_mapno /
 * get_now_areano / get_worldworld_pos.
 *
 * The stage-work block.  `stage_w` (map symbol, .bss 0x806B87C0, 0x2FE0 B) is a flat arena: a
 * pointer table at +0x04, the current map/area bytes at +0xBC5/+0xBC6, one 476-byte area record per
 * area from +0xC20, and scalar tails past the record array.  `StageWork`/`StageArea` mirror the
 * offsets the range touches; the two table-indexing shapes (`areas[areano].rows[i]`, stride 0x1DC /
 * 0x30) are what the disassembly computes, so they are kept verbatim.
 *
 * Inventory and evidence: `python tools/units/ledger.py unit stage/stg_w.cpp`.
 */

#include "types.h"
#include "nw4r/math.h"
#include "gx.h"

#include "ef.h"
#include "g3d/g3d_calcview.h"

/* The stage band is peephole-off: the sibling `stage/fn_802B2978.c` proves it for the TU below, and
 * this range shows the same fingerprint - retail keeps the unfused `clrlwi`+`slwi`/`mulli` narrow
 * before a table index where `-O3`'s peephole folds it into one `rlwinm`, and keeps the redundant
 * byte mask on an already-byte value.  A lib/region flag is the durable home; until that lands the
 * pragma pair carries the deviation in this unit's own source (playbook 39). */
#pragma peephole off

/* The stage-work block (map symbol, 0x806B87C0).  Never defined here (playbook 29): the object
 * emits only the references and `get_stg_w` is its accessor. */
extern u8 stage_w[];

/* The unit's pooled data the map already names (playbook 29). */
extern const u8 lbl_805CED40[];  /* id remap table, 0x18 B */
extern const u8 lbl_805CF038[];  /* id pair table, 0x18 B */
extern const f32 lbl_8079A480;
extern const f32 lbl_8079A47C;
extern const f32 lbl_8079A484;
extern const f32 lbl_8079A498;

/* ------------------------------------------------------------------------------------------------ */
/* types                                                                                             */
/* ------------------------------------------------------------------------------------------------ */

/* The 48-byte row chunked as three VEC3s from +0x0C (fn_802B02C4's view). */
typedef struct StageAreaVecs {
    /* +0x00 */ u32 pad_0x00[3];
    /* +0x0C */ VEC3 vecs[3];
} StageAreaVecs; /* size: 0x30 */

/* One 48-byte row of an area record: 12 words, also read as three 12-byte VEC3s at +0x0C. */
typedef union StageAreaRow {
    u32 words[12];
    StageAreaVecs v;
} StageAreaRow; /* size: 0x30 */

/* One 476-byte per-area record from stage_w+0xC20.  Only the head is touched by this range. */
typedef struct StageArea {
    /* +0x000 */ GXColor color;
    /* +0x004 */ StageAreaRow rows[9];
    /* +0x1B4 */ u8 pad_0x1B4[0x28];
} StageArea; /* size: 0x1DC */

/* stage_w: the offsets this range touches. */
typedef struct StageWork {
    /* +0x0000 */ u8 pad_0x0000[0x0004];
    /* +0x0004 */ void* entries[0x2F0];         /* lookup by a byte id */
    /* +0x0BC4 */ u8 pad_0x0BC4[0x0001];
    /* +0x0BC5 */ u8 mapno;
    /* +0x0BC6 */ u8 areano;
    /* +0x0BC7 */ u8 pad_0x0BC7[0x0BD0 - 0x0BC7];
    /* +0x0BD0 */ u8 field_0xBD0;
    /* +0x0BD1 */ u8 pad_0x0BD1[0x0BD4 - 0x0BD1];
    /* +0x0BD4 */ f32 field_0xBD4;
    /* +0x0BD8 */ u32 field_0xBD8[12];
    /* +0x0C08 */ VEC3 field_0xC08;
    /* +0x0C14 */ u8 flags;
    /* +0x0C15 */ u8 pad_0x0C15[0x0C20 - 0x0C15];
    /* +0x0C20 */ StageArea areas[15];
    /* +0x2804 */ u8 pad_0x2804[0x2818 - 0x2804];
    /* +0x2818 */ f32 field_0x2818;
    /* +0x281C */ u8 bitmask[0x2FD4 - 0x281C];
    /* +0x2FD4 */ void* field_0x2FD4;
    /* +0x2FD8 */ void* field_0x2FD8;
    /* +0x2FDC */ u8 pad_0x2FDC[0x0004];
} StageWork; /* size: 0x2FE0 */

#define SW ((StageWork*)stage_w)

/* A record `fn_802AE564` selects: one loadable effect/resource descriptor per id.  Only the fields
 * this range reads are named; the size is traced from the 0x24/0x28 bytes the accessors touch. */
typedef struct StageElem {
    /* +0x00 */ u8 pad_0x00[0x24];
    /* +0x24 */ u32 field_0x24;
    /* +0x28 */ u8 field_0x28;
    /* +0x29 */ u8 pad_0x29[0x0B];
    /* +0x34 */ f32 field_0x34;
    /* +0x38 */ u8 field_0x38;
    /* +0x39 */ u8 pad_0x39[0x03];
} StageElem; /* size: 0x3C */

/* The head `fn_802AE564` returns: `kind` at +0, a word at +4 that indexes an array of descriptors,
 * an inline word table at +0x0C, a byte-indexed word array at +0x18 and a 16-byte-stride base at
 * +0x14. */
typedef struct StageGroup {
    /* +0x00 */ u8 kind;
    /* +0x01 */ u8 pad_0x01[0x03];
    /* +0x04 */ StageElem** elems;
    /* +0x08 */ u32 field_0x08;   /* screen_projection_get reads it */
    /* +0x0C */ u32* field_0x0C;
    /* +0x10 */ u8 pad_0x10[0x04];
    /* +0x14 */ u32* field_0x14;    /* base of a j*16-byte table */
    /* +0x18 */ u32* field_0x18;
} StageGroup; /* size: 0x1C */

/* ------------------------------------------------------------------------------------------------ */
/* foreign declarations                                                                              */
/* ------------------------------------------------------------------------------------------------ */

extern "C" void* fn_802AE564(u8 id);
extern "C" u32 stage_map_kind_get(u32 kind);
extern "C" bool fn_802B0B04(u8 index, u8 bit);
extern "C" u32 stage_map_area_count_get(u8 id);
extern "C" void fn_802B0FF0(void* self, u8 index);
extern "C" void fn_802B11A0(void* self, u8 index);
extern "C" void fn_802AFA7C(u8 index);
extern "C" void fn_802B2278(u8 a, u8 b);
extern "C" void* fn_802B1558(void* work, long which);
extern "C" void* fn_802B057C(void* base, s32 off);
extern "C" void* fn_802B0540(void* self);
/* fn_80051490 comes from include/fn_8004CAD8.h and fn_800700C0 from include/g3d/g3d_calcview.h
 * (rule 2: the owner's header). */

/* ------------------------------------------------------------------------------------------------ */
/* functions                                                                                         */
/* ------------------------------------------------------------------------------------------------ */

/* The record `fn_802B050C` returns the body of. */
typedef struct StageObject {
    /* +0x000 */ u8 head[0x114];
    /* +0x114 */ u8 body[];
} StageObject; /* size: 0x114 (flexible body tail) */

/* The header of the resource `fn_800700C0` resolves (`fn_802B0540` reads its size at +0x48). */
typedef struct ResolverView {
    /* +0x00 */ u8 pad_0x00[0x48];
    /* +0x48 */ s32 size;
} ResolverView; /* size: 0x4C */

/* Returns the stage-work block. */
void* get_stg_w()
{
    return stage_w;
}

/* Returns the current map number. */
u8 get_now_mapno()
{
    return SW->mapno;
}

/* Returns the current area number. */
u8 get_now_areano()
{
    return SW->areano;
}

/* Copies the 4-byte word at `src` over the one at `dst`. */
extern "C" void fn_802AE280(void* dst, const void* src)
{
    *(u32*)dst = *(const u32*)src;
}

/* Copies `src`'s 4-byte word over `dst` and returns `dst`. */
extern "C" void* fn_802AE250(void* dst, const void* src)
{
    fn_802AE280(dst, src);
    return dst;
}

/* Narrows both arguments to a byte and forwards to fn_802B2278. */
extern "C" void fn_802B230C(u32 a, u32 b)
{
    fn_802B2278((u8)a, (u8)b);
}

/* Returns the stage's secondary block at stage_w + 0x2EC0. */
extern "C" void* fn_802B0420()
{
    return stage_w + 0x2EC0;
}

/* Tail-calls fn_802B1558(stage_w, 1). */
extern "C" void fn_802B1FDC()
{
    fn_802B1558(stage_w, 1);
}

/* Copies six bytes. */
extern "C" void fn_802AE980(u8* dst, const u8* src)
{
    dst[0] = src[0];
    dst[1] = src[1];
    dst[2] = src[2];
    dst[3] = src[3];
    dst[4] = src[4];
    dst[5] = src[5];
}

/* Returns the id-remapped byte at stage_w.bitmask[index]. */
extern "C" u8 fn_802B0AEC(u8 index)
{
    const u8* p = &SW->bitmask[0];
    return p[(u8)index];
}

/* Tests bit 0 of the stage flag byte. */
extern "C" bool fn_802AFF38()
{
    return SW->flags & 1;
}

/* Tests bit 1 of the stage flag byte. */
extern "C" bool fn_802AFF58()
{
    return SW->flags & 2;
}

/* Tests bit (4 + bit) of stage_w.bitmask[index]. */
extern "C" bool fn_802B0B04(u8 index, u8 bit)
{
    return SW->bitmask[(u8)index] & (0x10 << (u8)bit);
}

/* Remaps an id through lbl_805CED40; an unmapped id (0xFF) is returned unchanged.  The parameter is
 * wider than a byte: retail masks it (`clrlwi`) before the table index, so the original signature
 * was not the byte one `include/unsplit/unknown.h` guessed. */
extern "C" u32 stage_map_kind_get(u32 kind)
{
    u8 mapped = lbl_805CED40[(u8)kind];
    if (mapped == 0xFF)
        return kind;
    return mapped;
}

/* Tail-calls fn_802B0B04 with the byte pair lbl_805CF038[2*index], [2*index+1]. */
extern "C" u32 fn_802B0998(u8 index)
{
    const u8* p = &lbl_805CF038[(u8)index * 2];
    return fn_802B0B04(p[0], p[1]);
}

/* Whether the float at `self`+8 is below the pooled constant. */
extern "C" u32 fn_802B0688(void* self)
{
    return ((f32*)self)[2] < lbl_8079A480;
}

/* Returns `base`'s word pointer plus `off`; a zero offset yields NULL. */
extern "C" void* fn_802B057C(void* base, s32 off)
{
    u32 p = *(u32*)base;
    if (off == 0)
        return 0;
    return (u8*)(p + off);
}

/* Returns the record at `self` plus the resource size from fn_800700C0(). */
extern "C" void* fn_802B0540(void* self)
{
    return fn_802B057C(self, ((ResolverView*)fn_800700C0(0))->size);
}

/* Returns the entry `index` points at, biased by its 0x114-byte header, or NULL. */
extern "C" void* fn_802B050C(u8 index)
{
    void** entries = &SW->entries[0];
    StageObject* p = (StageObject*)entries[(u8)index];
    if (p == 0)
        return 0;
    return fn_802B0540(p->body);
}

/* Returns the word at +8 of the current map/area group. */
extern "C" u32 screen_projection_get()
{
    return ((StageGroup*)fn_802AE564((u8)get_now_mapno()))->field_0x08;
}

/* Stores `f` into the current area's record when the map/area pair matches. */
extern "C" void fn_802B08DC(f32 f)
{
    if ((SW->mapno == 6 || SW->mapno == 17) && SW->areano == 1)
        SW->field_0x2818 = f;
}

/* Whether `self`'s map id remaps to a "loadable" category. */
extern "C" u32 fn_802B0B38(void* self)
{
    s32 mapped = (u8)stage_map_kind_get(((StageWork*)self)->mapno);
    return mapped == 4 || mapped == 10;
}

/* Remaps the stage's current map id and returns the j-th element of the group's 16-byte-stride
 * table. */
extern "C" void* fn_802B0B7C(void* work, u8 j)
{
    StageGroup* g = (StageGroup*)fn_802AE564(((StageWork*)work)->mapno);
    return &g->field_0x14[(u8)j * 4];
}

/* Whether the j-th element of the group's 16-byte-stride table is non-zero. */
extern "C" bool fn_802B0BC0(u8 id, u8 j)
{
    StageGroup* g = (StageGroup*)fn_802AE564((u8)id);
    return g->field_0x14[(u8)j * 4];
}

/* Returns the j-th word of the group's inline table, or NULL for the unmapped id. */
extern "C" u32* fn_802AFE5C(u8 id, u8 j)
{
    StageGroup* g;
    if ((u8)id == 0xFF)
        return 0;
    g = (StageGroup*)fn_802AE564((u8)id);
    return &g->field_0x0C[(u8)j];
}

/* Returns the record's +0x24 word, or 0 for the unmapped id. */
extern "C" u32 fn_802AFE08(u8 id, u8 j)
{
    StageGroup* g;
    if ((u8)id == 0xFF)
        return 0;
    g = (StageGroup*)fn_802AE564((u8)id);
    return g->elems[(u8)j]->field_0x24;
}

/* Returns the record's +0x28 byte, or 0xFF for the unmapped id. */
extern "C" u32 fn_802AFEAC(u8 id, u8 j)
{
    StageGroup* g;
    if ((u8)id == 0xFF)
        return 0xFF;
    g = (StageGroup*)fn_802AE564((u8)id);
    return g->elems[(u8)j]->field_0x28;
}

/* Selects one of the two record groups by whether the id was remapped. */
extern "C" void* fn_802AE564(u8 id)
{
    u8 mapped = stage_map_kind_get((u8)id);
    return mapped == (u8)id ? SW->field_0x2FD4 : SW->field_0x2FD8;
}

/* Returns the selected group's head byte, or 0 for the unmapped id. */
extern "C" u32 stage_map_area_count_get(u8 id)
{
    StageGroup* g;
    if ((u8)id == 0xFF)
        return 0;
    g = (StageGroup*)fn_802AE564((u8)id);
    return g->kind;
}

/* Returns the current area's word [i][j] lookup. */
extern "C" u32 fn_802B025C(u8 i, u8 j)
{
    return SW->areas[SW->areano].rows[(u8)i].words[(u8)j];
}

/* Same lookup, but the area is an argument. */
extern "C" u32 fn_802B0290(u8 i, u8 j, u8 area)
{
    StageArea* a = &SW->areas[(u8)area];
    StageAreaRow* r = &a->rows[(u8)i];
    return r->words[(u8)j];
}

/* Copies the current area's row [i] 12-byte element [k] into `out`. */
extern "C" void fn_802B02C4(void* out, u8 i, u8 k)
{
    fn_80051490((Vec*)out, (Vec*)&SW->areas[SW->areano].rows[(u8)i].v.vecs[(u8)k]);
}

/* Classifies the current map number into a load category. */
extern "C" u32 fn_802B0608()
{
    s32 m = SW->mapno;
    switch (m) {
    case 2:
    case 5:
    case 16:
    case 10:
        return 1;
    case 13:
    case 4:
    case 15:
        return 2;
    }
    return 0;
}

/* Resets the stage's per-area cursor block and re-seats the world vector. */
extern "C" void fn_802B2918()
{
    s32 i;
    SW->field_0xBD0 = 0;
    SW->field_0xBD4 = lbl_8079A484;
    for (i = 0; i < 12; i++)
        SW->field_0xBD8[i] = 0xFFFFFF00;
    setVector3(&SW->field_0xC08, lbl_8079A498, lbl_8079A498, lbl_8079A498);
}

/* Calls fn_802AFA7C for the two side indices. */
extern "C" void fn_802AFA40()
{
    u32 i;
    for (i = 0; i < 2; i++)
        fn_802AFA7C((u8)i);
}

/* Calls fn_802B0FF0 for each index below the current map's count. */
extern "C" void fn_802B1138(void* self)
{
    u8 n = (u8)stage_map_area_count_get(((StageWork*)self)->mapno);
    u8 i;
    for (i = 0; (u8)i < n; i++)
        fn_802B0FF0(self, i);
}

/* Calls fn_802B11A0 for each index below the current map's count. */
extern "C" void fn_802B13A8(void* self)
{
    u8 n = (u8)stage_map_area_count_get(((StageWork*)self)->mapno);
    u8 i;
    for (i = 0; (u8)i < n; i++)
        fn_802B11A0(self, i);
}

/* Returns the current map's descriptor field +0x34 for `id`, or a default float. */
extern "C" f32 fn_802B0430(u8 id)
{
    StageWork* w = SW;
    StageElem* e;
    if ((u8)id == 0xFF)
        return lbl_8079A47C;
    e = ((StageGroup*)fn_802AE564(w->mapno))->elems[(u8)id];
    if (e == 0)
        return lbl_8079A47C;
    return e->field_0x34;
}

/* Returns the current map's +0x18 word array entry for `id`, or 0. */
extern "C" u32 fn_802B04A0(u8 id)
{
    StageWork* w = SW;
    u32* arr;
    if ((u8)id == 0xFF)
        return 0;
    arr = ((StageGroup*)fn_802AE564(w->mapno))->field_0x18;
    if (arr == 0)
        return 0;
    return arr[(u8)id];
}

/* Returns the current map's descriptor field +0x38 for `id`, or 0. */
extern "C" u32 pl_act_kind_get(u8 id)
{
    StageWork* w = SW;
    StageElem* e;
    if ((u8)id == 0xFF)
        return 0;
    e = ((StageGroup*)fn_802AE564(w->mapno))->elems[(u8)id];
    if (e == 0)
        return 0;
    return e->field_0x38;
}

/* Sets bit (4 + j) of bitmask[id] when the current map's j-th table word is set. */
extern "C" void fn_802B09B8(u8 id, u8 j)
{
    u32* p = (u32*)fn_802B0B7C(stage_w, id);
    if (p[(u8)j] != 0)
        SW->bitmask[(u8)id] |= (u8)(0x10 << (u8)j);
}

/* Whether at least two of the three map flags (ids 3, 11, 4) are set. */
extern "C" u32 fn_802B20A4()
{
    u8 count = 0;
    if (fn_802B0998(3) == 1)
        count = 1;
    if (fn_802B0998(11) == 1)
        count++;
    if (fn_802B0998(4) == 1)
        count++;
    if (count >= 2)
        return 1;
    return 0;
}

#pragma peephole reset
