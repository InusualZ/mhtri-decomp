/*
 * g3d/g3d_resanmtexsrt.cpp - nw4r g3d `ResAnmTexSrt` SRT-animation evaluator and the `g3d_rescommon` helpers
 *   beside it: the byte-IO BP-command (de)serialisers and the `ResDic` patricia-tree lookups.
 * RANGE. .text 0x800916FC-0x80092444 (21 functions); extab 0x80009208-0x80009290, extabindex
 *   0x80021EE8-0x80021FB4, .data 0x80590928-0x80590A60 (opens on "g3d_resanmtexsrt.cpp"; the `ofs != 0` assert of
 *   the `g3d_rescommon_ac.h` inline closes it), .sdata2 0x80795F08-0x80795F10.  The right edge is
 *   `g3d/g3d_resfile.cpp`'s `ResFile::ref` (0x80092444), whose asserts pass "g3d_resfile_ac.h".
 * NAMES. Map stems (the dump answers `zz_` placeholders).  res_anm_tex_srt_get_tex_result (0x80091B50,
 *   nw4r's per-texture `GetTexSrtAnmResult`) is a GUESS.
 * RESIDUALS. Code: none.
 *   flipcheck: the 0x4-byte tail of the `.data` claim is fill before `g3d/g3d_resfile.cpp`'s 32-aligned name
 *   records (that unit's `.data` split carries `align:32`, request g3d-res2#7).
 * SHAPES. File-scope `#pragma peephole off`: retail's split `clrlwi` + `cmpwi`; fn_80092234 is compiled with
 *   `#pragma peephole on` (its base word stays in r0).  Definitions in address order.  `GetAnmResult` sits under
 *   a scoped `#pragma pool_data off` (each assert string its own `lis`/`addi`); the per-texture evaluator declares
 *   its cell index before the flags word; the two dictionary walkers read the node's bit index into an `int`
 *   local, and fn_80092020 passes the found name by value to fn_80091D78 (`ResName::operator==`), which copies the
 *   constructor's result.  fn_800922D0 (`g3d_rescommon_ac.h`'s offset resolver) is `inline` and
 *   `__declspec(noinline)`: its weak copy follows its first referencer and its strings are the `.data` tail.
 *   The byte readers and writers walk `p++`.
 */

#include "types.h"
#include "g3d/g3d_resanmtexsrt.h" /* `nw4r::g3d::ResFile` and the detail BP helpers (this unit's own header) */
#include "g3d/g3d_rescommon.h"    /* `nw4r::g3d::ResName`/`ResDic` (rule 1) */
#include "nw4r/g3d/res_common.h"   /* IS_VALID_PTR (rule 1) */
#include "g3d/g3d_anmchr.h"        /* res_dic_is_valid/res_dic_ptr (rule 2, owner header) */
#include "g3d/g3d_anmtexsrt.h"     /* fn_8006CDBC/fn_8006D9FC/res_anm_tex_srt_ptr (rule 2, owner header) */
#include "g3d/g3d_resanmcamera.h"  /* res_anm_data_get_result (rule 2, owner header) */
#include "g3d/g3d_resmat.h"        /* nw4r::g3d::TexSrt (rule 1) */
#include "MSL_C/alloc.h"           /* __fpclassifyf, owner MSL_C/alloc.cpp (rule 2) */
#include "g3d/g3d_resanmscn.h"     /* fn_8009125C/fn_800912D4 (rule 2, owner header) */

/* The target's assert guards keep the split `clrlwi` + `cmpwi` pair (no record-form `clrlwi.`). */
#pragma peephole off

namespace nw4r {
namespace db {

/* `Panic(const char* pFile, int line, const char* pFmt, ...)` - the trailing `e` in the map's
 * mangling is MWCC's vararg marker.  Declared in the namespace (C++ linkage), never as the mangled
 * spelling (rule 9). */
void Panic(const char* pFile, int line, const char* pFmt, ...);

} // namespace db
} // namespace nw4r

/* One texture's record in a texture-SRT material: the identity/uniform/constant flags, then the value cells the
 * evaluator walks (a constant float or a channel offset each).  size: 0x8 (a lower bound: the cells run on) */
struct ResAnmTexSrtTexData {
    /* +0x00 */ u32 flags;
    /* +0x04 */ u32 anms[1];
};

/* The material dictionary word the field reader indexes.  size: 0x14 (lower bound - only +0x10 is evidenced) */
typedef struct {
    /* +0x00 */ u8 pad_0x00[0x10];
    /* +0x10 */ u32 field_0x10;
} ResAnmTexSrtBody;

extern "C" {
u32 res_anm_tex_srt_get_tex_result(nw4r::g3d::TexSrt* srt, const ResAnmTexSrtTexData* texData, f32 frame);
u32 fn_80091CEC(u32 self, u32 ofs);
u32 fn_80091D1C(u32 value);
u32 fn_80091D20(void* self, void* key);
u32 fn_80091D78(void* a, nw4r::g3d::ResName b);
void fn_80091E44(u8* p, u32 value);
void fn_80091EAC(u8* p, u8 value);
u32 fn_80091EE8(const u8* p);
u8 fn_80091F68(const u8* p);
u32 fn_80092020(void* self, void* name);
u32 fn_8009213C(void* self, const char* name, u32 len);
u32* fn_80092234(u32* self, u32 ofs);
u32 fn_800922D0(u32* self, u32 ofs);
u32 strlen(const char* p);
int strcmp(const char* a, const char* b);
}

#pragma pool_data off
/* 0x800916FC (0x454): evaluates material `id`'s texture and indirect SRT animations at `frame` into `pResult`. */
void nw4r::g3d::ResAnmTexSrt::GetAnmResult(TexSrtAnmResult* pResult, u32 id, f32 frame) const
{
    u32 valid;
    bool finite;
    const ResAnmTexSrtMatData* matData;
    const s32* toTexData;
    u32 texFlags;
    u32 indFlags;
    int i;

    valid = IS_VALID_PTR(this);
    if (!valid) {
        nw4r::db::Panic("g3d_resanmtexsrt.cpp", 0x76, "NW4R:Pointer Error\nthis(=%p) is not valid pointer.", this);
    }
    valid = IS_VALID_PTR(pResult);
    if (!valid) {
        nw4r::db::Panic("g3d_resanmtexsrt.cpp", 0x77, "NW4R:Pointer Error\npResult(=%p) is not valid pointer.", pResult);
    }
    finite = false;
    if (__fpclassifyf(frame) > 2 && __fpclassifyf(frame) != 1) {
        finite = true;
    }
    if (!finite) {
        nw4r::db::Panic("g3d_resanmtexsrt.cpp", 0x78, "NW4R:Floating Point Value Error(%f)\nframe is infinite or nan.", frame);
    }
    matData = (const ResAnmTexSrtMatData*)fn_80091D20((void*)this, (void*)id);
    valid = IS_VALID_PTR(matData);
    if (!valid) {
        nw4r::db::Panic("g3d_resanmtexsrt.cpp", 0x7B, "NW4R:Pointer Error\nmatData(=%p) is not valid pointer.", matData);
    }
    toTexData = matData->toResAnmTexSrtTexData;
    texFlags = matData->flags;
    indFlags = matData->indFlags;
    pResult->flags = 0;
    pResult->indFlags = 0;
    pResult->texMtxMode = ((const ResAnmTexSrtData*)res_anm_tex_srt_ptr((void*)this))->texMtxMode;
    for (i = 0; texFlags != 0; texFlags >>= 1, i++) {
        if (texFlags & 1) {
            const ResAnmTexSrtTexData* texData = (const ResAnmTexSrtTexData*)fn_80091CEC((u32)matData, *toTexData++);
            u32 srtFlags = res_anm_tex_srt_get_tex_result(reinterpret_cast<TexSrt*>(pResult->srt[i]), texData, frame);
            pResult->flags |= srtFlags << (i * 4);
        }
    }
    for (i = 0; indFlags != 0; indFlags >>= 1, i++) {
        if (indFlags & 1) {
            const ResAnmTexSrtTexData* texData = (const ResAnmTexSrtTexData*)fn_80091CEC((u32)matData, *toTexData++);
            u32 srtFlags = res_anm_tex_srt_get_tex_result(reinterpret_cast<TexSrt*>(pResult->srt[i + 8]), texData, frame);
            pResult->indFlags |= srtFlags << (i * 4);
        }
    }
}
#pragma pool_data reset

/* 0x80091B50 (0x19C): evaluates one texture's scale/rotation/translation channels at `frame` into `srt`; returns
 * the identity bits (scale one, rotation zero, translation zero). */
extern "C" u32 res_anm_tex_srt_get_tex_result(nw4r::g3d::TexSrt* srt, const ResAnmTexSrtTexData* texData, f32 frame)
{
    int index = 0;
    u32 flags = texData->flags;

    if (!(flags & 0x2)) {
        srt->Su = res_anm_data_get_result((u32*)&texData->anms[index++], frame, (flags & 0x20) != 0);
        if (flags & 0x10) {
            srt->Sv = srt->Su;
        } else {
            srt->Sv = res_anm_data_get_result((u32*)&texData->anms[index++], frame, (flags & 0x40) != 0);
        }
    } else {
        srt->Su = 1.0f;
        srt->Sv = 1.0f;
    }
    if (!(flags & 0x4)) {
        srt->R = res_anm_data_get_result((u32*)&texData->anms[index++], frame, (flags & 0x80) != 0);
    } else {
        srt->R = 0.0f;
    }
    if (!(flags & 0x8)) {
        srt->Tu = res_anm_data_get_result((u32*)&texData->anms[index++], frame, (flags & 0x100) != 0);
        srt->Tv = res_anm_data_get_result((u32*)&texData->anms[index], frame, (flags & 0x200) != 0);
    } else {
        srt->Tu = 0.0f;
        srt->Tv = 0.0f;
    }
    return flags & 0xF;
}

/* Resolves a record-relative offset against the record. */
extern "C" u32 fn_80091CEC(u32 self, u32 ofs) {
    return fn_80091D1C(self) + ofs;
}

/* The identity the offset resolver folds in. */
extern "C" u32 fn_80091D1C(u32 value) {
    return value;
}

/* Material `key`'s animation record, from the resource's material dictionary. */
extern "C" u32 fn_80091D20(void* self, void* key) {
    ResAnmTexSrtBody* body = (ResAnmTexSrtBody*)fn_8006CDBC(self);
    u32 tmp = fn_8006D9FC(self, body->field_0x10);

    return (u32)(*reinterpret_cast<nw4r::g3d::ResDic*>(&tmp))[(int)key];
}

/* `ResName::operator==`: whether the two names have the same length and characters. */
extern "C" u32 fn_80091D78(void* a, nw4r::g3d::ResName b) {
    if (fn_800912D4(a) == fn_800912D4(&b)) {
        return strcmp(fn_8009125C(a), fn_8009125C(&b)) == 0;
    }
    return 0;
}

/* 0x80091DFC (0x48): writes a BP command: the 0x61 opcode, then the register word. */
void nw4r::g3d::detail::ResWriteBPCmd(u8* p, u32 value) {
    fn_80091EAC(p, 0x61);
    fn_80091E44(p + 1, value);
}

/* Writes a big-endian word byte by byte. */
extern "C" void fn_80091E44(u8* p, u32 value) {
    fn_80091EAC(p++, value >> 24);
    fn_80091EAC(p++, (value >> 16) & 0xFF);
    fn_80091EAC(p++, (value >> 8) & 0xFF);
    fn_80091EAC(p, value & 0xFF);
}

/* Stores one byte. */
extern "C" void fn_80091EAC(u8* p, u8 value) {
    *p = value;
}

/* 0x80091EB4 (0x34): reads a BP command's register word. */
void nw4r::g3d::detail::ResReadBPCmd(const u8* p, u32* out) {
    *out = fn_80091EE8(p + 1);
}

/* Reads a big-endian word byte by byte. */
extern "C" u32 fn_80091EE8(const u8* p) {
    u32 value = ((u32)fn_80091F68(p++) & 0xFF) << 24;
    value |= ((u32)fn_80091F68(p++) & 0xFF) << 16;
    value |= ((u32)fn_80091F68(p++) & 0xFF) << 8;
    value |= (u32)fn_80091F68(p) & 0xFF;
    return value;
}

/* Loads one byte. */
extern "C" u8 fn_80091F68(const u8* p) {
    return *p;
}

/* Writes a tagged command: the 0x08 opcode, the tag byte, then the register word. */
extern "C" void fn_80091F70(u8* p, u32 tag, u32 value) {
    fn_80091EAC(p, 8);
    fn_80091EAC(p + 1, tag);
    fn_80091E44(p + 2, value);
}

/* 0x80091FD0 (0x50): ORs `value` into a BP command's word and rewrites it as a mask command. */
void nw4r::g3d::detail::ResWriteSSMask(u8* p, u32 value) {
    u32 word = fn_80091EE8(p + 1);
    ResWriteBPCmd(p, (word | value) | 0xFE000000);
}

/* 0x80092020 (0x11C): walks the dictionary's patricia tree by the bits of `name`; returns the entry whose name
 * equals `name`, or NULL. */
extern "C" u32 fn_80092020(void* self, void* name)
{
    u32 len = fn_800912D4(name);
    const char* str = fn_8009125C(name);
    const nw4r::g3d::ResDicData& dic = reinterpret_cast<const nw4r::g3d::ResDic*>(self)->ref();
    const nw4r::g3d::ResDicEntry* parent = &dic.entry[0];
    const nw4r::g3d::ResDicEntry* child = &dic.entry[parent->idxLeft];

    while (parent->ref > child->ref) {
        int bit = child->ref;
        u32 wd;
        u32 pos;

        parent = child;
        wd = bit >> 3;
        pos = bit & 7;
        if (wd < len && ((str[wd] >> pos) & 1)) {
            child = &dic.entry[child->idxRight];
        } else {
            child = &dic.entry[child->idxLeft];
        }
    }
    if (fn_80091D78(name, nw4r::g3d::ResName((void*)((const u8*)&dic + child->ofsString - 4)))) {
        return (u32)child;
    }
    return 0;
}

/* 0x8009213C (0xF8): walks the dictionary's patricia tree by the bits of the `len`-character `name`; returns the
 * entry whose string equals `name`, or NULL. */
extern "C" u32 fn_8009213C(void* self, const char* name, u32 len)
{
    const nw4r::g3d::ResDicData& dic = reinterpret_cast<const nw4r::g3d::ResDic*>(self)->ref();
    const nw4r::g3d::ResDicEntry* parent = &dic.entry[0];
    const nw4r::g3d::ResDicEntry* child = &dic.entry[parent->idxLeft];

    while (parent->ref > child->ref) {
        int bit = child->ref;
        u32 wd;
        u32 pos;

        parent = child;
        wd = bit >> 3;
        pos = bit & 7;
        if (wd < len && ((name[wd] >> pos) & 1)) {
            child = &dic.entry[child->idxRight];
        } else {
            child = &dic.entry[child->idxLeft];
        }
    }
    if (child->ofsString != 0 && strcmp(name, (const char*)fn_80092234((u32*)self, child->ofsString)) == 0) {
        return (u32)child;
    }
    return 0;
}

/* `ResCommon<ResDicData>::ofs_to_ptr`: the dictionary-relative address, NULL for a zero offset. */
#pragma peephole on
extern "C" u32* fn_80092234(u32* self, u32 ofs) {
    u32 base = *self;

    if (ofs != 0) {
        base += ofs;
        return (u32*)base;
    }
    return 0;
}
#pragma peephole off

/* 0x80092250 (0x80): returns the data of the entry named `name`, or NULL. */
/* untyped: opaque handle - the entry's data block */
void* nw4r::g3d::ResDic::operator[](const char* name) const {
    if (res_dic_is_valid(const_cast<ResDic*>(this)) != 0) {
        if (name != 0) {
            u32 e = fn_8009213C(const_cast<ResDic*>(this), name, strlen(name));
            if (e != 0) {
                return (void*)fn_800922D0((u32*)const_cast<ResDic*>(this), ((const nw4r::g3d::ResDicEntry*)e)->ofsData);
            }
        }
    }
    return 0;
}

/* `ResDic`'s `g3d_rescommon_ac.h` offset resolver: the dictionary-relative address of a nonzero offset. */
extern "C" __declspec(noinline) inline u32 fn_800922D0(u32* self, u32 ofs) {
    if (ofs == 0) {
        nw4r::db::Panic("g3d_rescommon_ac.h", 123, "NW4R:Failed assertion ofs != 0");
    }
    return *self + ofs;
}

/* 0x80092330 (0x84): returns the data of the entry named by `name`, or NULL. */
/* untyped: opaque handle - the entry's data block */
void* nw4r::g3d::ResDic::operator[](const ResName name) const {
    if (res_dic_is_valid(const_cast<ResDic*>(this)) != 0) {
        if (name.IsValid() != 0) {
            u32 local = (u32)name.mpData;
            u32 e = fn_80092020(const_cast<ResDic*>(this), &local);
            if (e != 0) {
                return (void*)fn_800922D0((u32*)const_cast<ResDic*>(this), ((const nw4r::g3d::ResDicEntry*)e)->ofsData);
            }
        }
    }
    return 0;
}

/* 0x800923B4 (0x90): returns the index of the entry named by `name`, or -1. */
int nw4r::g3d::ResDic::GetIndex(const ResName name) const {
    if (res_dic_is_valid(const_cast<ResDic*>(this)) != 0) {
        if (name.IsValid() != 0) {
            u32 local = (u32)name.mpData;
            u32 e = fn_80092020(const_cast<ResDic*>(this), &local);
            if (e != 0) {
                u32 base = (u32)res_dic_ptr(const_cast<ResDic*>(this));
                return (s32)(e - (base + 24)) / 16;
            }
        }
    }
    return -1;
}
