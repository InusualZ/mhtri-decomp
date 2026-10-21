/*
 * g3d/g3d_resanmtexsrt.cpp - nw4r g3d `ResAnmTexSrt` SRT-animation evaluator and the `ResFile::GetResXxx`
 *   accessor bodies the `g3d_res*_ac.h` headers inline beside it: byte-IO (de)serialisers, the six value-type
 *   constructors, the ten category lookups and their forwarders.
 * RANGE. .text 0x800916FC-0x80093990 (75 functions); extab, extabindex, .data 0x80590928-0x80590D78 (opens on
 *   "g3d_resanmtexsrt.cpp"; the `_ac.h` assert strings share the fragment), .sdata 0x80791270-0x80791280, .sdata2
 *   0x80795F08-0x80795F10.  The right edge is `g3d/g3d_resfile.cpp`'s first body, a proven seam.
 * NAMES. Map stems (the dump answers `zz_` placeholders) except the two `nw4r::g3d::ResFile` members
 *   `GetResPltt`/`GetResTex`.  res_anm_tex_srt_get_tex_result (0x80091B50, nw4r's per-texture
 *   `GetTexSrtAnmResult`) is a GUESS.
 * RESIDUALS. Code: none.
 *   Seam: 0x80092444-0x80093990 (fn_80092444 `ResFile::ref`, whose asserts pass "g3d_resfile_ac.h" at
 *   0x80590D64, and every lookup that reads the 0x80590A60 `ResName` records) is `g3d/g3d_resfile.cpp`'s TU,
 *   and so is `.data` 0x80590A60-0x80590D78 (the records open that TU's run, before "g3d_resfile.cpp").
 *   flipcheck: `.data`/`.sdata`: the strings are literals, the `_ac.h` accessor strings are not the inline tail
 *   yet (retail: weak `@STRING@` copies, one per inline instance, after the TU's own strings, playbook 80);
 *   the `ResName` records stay externs, and the run also holds "g3d_resfile.cpp"'s strings (a data seam with
 *   `g3d/g3d_resfile.cpp`); `.sdata2` is not emitted.
 * SHAPES. File-scope `#pragma peephole off`: the constructor guards keep retail's split `clrlwi` + `cmpwi`.
 *   `GetAnmResult` sits under a scoped `#pragma pool_data off` (each assert string its own `lis`/`addi`); the
 *   per-texture evaluator declares its cell index before the flags word; the two dictionary walkers read the
 *   node's bit index into an `int` local, and fn_80092020 passes the found name by value to fn_80091D78
 *   (`ResName::operator==`), which copies the constructor's result.
 *   The `ResName` record externs are declared `__attribute__((aligned(32)))` (retail's records are 32-aligned):
 *   the lookups then hold the key copy's address in r31 as retail does.  The element lookups, the two
 *   `GetResPltt`/`GetResTex(ResName)` and fn_8009380C build their handles as class temporaries
 *   (`ResHandleTemp`, g3d/g3d_rescommon.h) passed to the out-of-line constructors: retail's handle temporaries
 *   take their stack slots in creation order after the named locals, which named `u32` handles do not.  The byte readers and writers
 *   walk `p++`; fn_80092234 is compiled with `#pragma peephole on` (its base word stays in r0).
 */

#include "types.h"
#include "g3d/g3d_resanmtexsrt.h" /* `nw4r::g3d::ResFile` (this unit's own header, rule 1) */
#include "g3d/g3d_rescommon.h"    /* `nw4r::g3d::ResName` (rule 1) */
#include "g3d/g3d_resshp.h"       /* `nw4r::g3d::ResTex`/`ResPltt` (rule 2) */
#include "nw4r/g3d/res_common.h"   /* ResHandle (rule 1) */
#include "g3d/g3d_anmchr.h"        /* fn_8006268C/fn_80062750/fn_80062914 (rule 2, owner header) */
#include "g3d/g3d_anmshp.h"        /* ResDic::GetNumData's owner (rule 2) */
#include "g3d/g3d_anmtexsrt.h"     /* fn_8006CDBC/fn_8006D9FC (rule 2, owner header) */
#include "g3d/g3d_resanmcamera.h"  /* res_anm_data_get_result (rule 2, owner header) */
#include "g3d/g3d_resmat.h"        /* nw4r::g3d::TexSrt (rule 1) */
#include "MSL_C/alloc.h"           /* __fpclassifyf, owner MSL_C/alloc.cpp (rule 2) */
#include "g3d/g3d_resanmscn.h"     /* fn_8009125C/fn_800912D4/fn_800913A0/fn_80091628
                                    * (rule 2, owner header) */

/* The target's constructor guards keep the split `clrlwi` + `cmpwi` pair (no record-form `clrlwi.`),
 * so this TU carries the same file-scoped `peephole off` as its `g3d/g3d_resanmchr.cpp` sibling. */
#pragma peephole off

namespace nw4r {
namespace db {

/* `Panic(const char* pFile, int line, const char* pFmt, ...)` - the trailing `e` in the map's
 * mangling is MWCC's vararg marker.  Declared in the namespace (C++ linkage), never as the mangled
 * spelling (rule 9). */
void Panic(const char* pFile, int line, const char* pFmt, ...);

} // namespace db
} // namespace nw4r

extern "C" {

/* The `ResName` categories of the `ResFile` root dictionary (length-prefixed records). */
extern const u8 lbl_80590A60[] __attribute__((aligned(32))); /* "3DModels(NW4R)" */
extern const u8 lbl_80590A80[] __attribute__((aligned(32))); /* "Palettes(NW4R)" */
extern const u8 lbl_80590AA0[] __attribute__((aligned(32))); /* "Textures(NW4R)" */
extern const u8 lbl_80590AC0[] __attribute__((aligned(32))); /* "AnmChr(NW4R)" */
extern const u8 lbl_80590AE0[] __attribute__((aligned(32))); /* "AnmVis(NW4R)" */
extern const u8 lbl_80590B00[] __attribute__((aligned(32))); /* "AnmClr(NW4R)" */
extern const u8 lbl_80590B20[] __attribute__((aligned(32))); /* "AnmTexPat(NW4R)" */
extern const u8 lbl_80590B40[] __attribute__((aligned(32))); /* "AnmTexSrt(NW4R)" */
extern const u8 lbl_80590B60[] __attribute__((aligned(32))); /* "AnmShp(NW4R)" */
extern const u8 lbl_80590B80[] __attribute__((aligned(32))); /* "AnmScn(NW4R)" */

} // extern "C"

/* ============================ 0x800916FC-0x80091CEC: the texture-SRT evaluator ================= */

/* One texture's record in a texture-SRT material: the identity/uniform/constant flags, then the value cells the
 * evaluator walks (a constant float or a channel offset each).  size: 0x8 (a lower bound: the cells run on) */
struct ResAnmTexSrtTexData {
    /* +0x00 */ u32 flags;
    /* +0x04 */ u32 anms[1];
};

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

extern "C" u32 fn_80091CEC(u32 self, u32 ofs);
extern "C" u32 fn_80091D20(void* self, void* key);

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

/* ============================ 0x80091CEC-0x80091FD0: the byte (de)serialisers ================= */
/* These are the little big-endian word writers/readers the `ResAnmTexSrt` setter path uses: tag a
 * record, write a big-endian u32, read it back.  All C linkage (bare `fn_` stems). */

extern "C" u32 fn_80091D1C(u32 value);       /* 0x80091D1C - the identity (returns r3 unchanged) */
extern "C" void fn_80091EAC(u8* p, u8 value); /* 0x80091EAC - `*p = value` */
extern "C" u8 fn_80091F68(const u8* p);       /* 0x80091F68 - `return *p` */

extern "C" u32 fn_80091D1C(u32 value) {
    return value;
}

extern "C" void fn_80091EAC(u8* p, u8 value) {
    *p = value;
}

extern "C" u8 fn_80091F68(const u8* p) {
    return *p;
}

extern "C" u32 fn_80091CEC(u32 self, u32 ofs) {
    return fn_80091D1C(self) + ofs;
}

extern "C" void fn_80091E44(u8* p, u32 value) {
    fn_80091EAC(p++, value >> 24);
    fn_80091EAC(p++, (value >> 16) & 0xFF);
    fn_80091EAC(p++, (value >> 8) & 0xFF);
    fn_80091EAC(p, value & 0xFF);
}

/* 0x80091DFC (0x48): writes a BP command: the 0x61 opcode, then the register word. */
void nw4r::g3d::detail::ResWriteBPCmd(u8* p, u32 value) {
    fn_80091EAC(p, 0x61);
    fn_80091E44(p + 1, value);
}

extern "C" u32 fn_80091EE8(const u8* p) {
    u32 value = ((u32)fn_80091F68(p++) & 0xFF) << 24;
    value |= ((u32)fn_80091F68(p++) & 0xFF) << 16;
    value |= ((u32)fn_80091F68(p++) & 0xFF) << 8;
    value |= (u32)fn_80091F68(p) & 0xFF;
    return value;
}

/* 0x80091EB4 (0x34): reads a BP command's register word. */
void nw4r::g3d::detail::ResReadBPCmd(const u8* p, u32* out) {
    *out = fn_80091EE8(p + 1);
}

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

/* ============================== 0x80092234-0x800924B8: the ResFile helpers ===================== */

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

extern "C" u32 fn_800924A8(u32* p) {
    return *p;
}

extern "C" void* fn_800924B0(void) {
    return (void*)"ResFile";
}

extern "C" u32 fn_800924B8(u32* p) {
    return *p != 0;
}

/* ============================== 0x80092584-0x80093690: forwarder thunks ======================= */
/* One 4-byte thunk per inline accessor overload; the target's `b <symbol>` is the tail call.  The
 * callee declaration order is the map's address order. */

extern "C" u32 fn_800924CC(void* self, u32 arg);
extern "C" u32 fn_80092790(void* self, u32 arg);
extern "C" u32 fn_80092A54(void* self, u32 arg);
extern "C" u32 fn_80092C08(void* self, u32 arg);
extern "C" u32 fn_80092DBC(void* self, u32 arg);
extern "C" u32 fn_80092F70(void* self, u32 arg);
extern "C" u32 fn_80093124(void* self, u32 arg);
extern "C" u32 fn_800932D8(void* self, u32 arg);
extern "C" u32 fn_8009348C(void* self, u32 arg);
extern "C" u32 fn_800935D4(void* self, u32 arg);

/* Forwards to the category lookup, typed `void*` as `g3d/g3d_resanmtexsrt.h` declares it (it feeds a ResHandle). */
extern "C" void* fn_80092584(void* self, u32 arg) { return (void*)fn_800924CC(self, arg); }
/* Forwards to the category lookup, typed `void*` as `g3d/g3d_resanmtexsrt.h` declares it (it feeds a ResHandle). */
extern "C" void* fn_80092848(void* self, u32 arg) { return (void*)fn_80092790(self, arg); }
/* Forwards to the category lookup, typed `void*` as `g3d/g3d_resanmtexsrt.h` declares it (it feeds a ResHandle). */
extern "C" void* fn_80092B0C(void* self, u32 arg) { return (void*)fn_80092A54(self, arg); }
/* Forwards to the category lookup, typed `void*` as `g3d/g3d_resanmtexsrt.h` declares it (it feeds a ResHandle). */
extern "C" void* fn_80092CC0(void* self, u32 arg) { return (void*)fn_80092C08(self, arg); }
/* Forwards to the category lookup, typed `void*` as `g3d/g3d_resanmtexsrt.h` declares it (it feeds a ResHandle). */
extern "C" void* fn_80092E74(void* self, u32 arg) { return (void*)fn_80092DBC(self, arg); }
/* Forwards to the category lookup, typed `void*` as `g3d/g3d_resanmtexsrt.h` declares it (it feeds a ResHandle). */
extern "C" void* fn_80093028(void* self, u32 arg) { return (void*)fn_80092F70(self, arg); }
/* Forwards to the category lookup, typed `void*` as `g3d/g3d_resanmtexsrt.h` declares it (it feeds a ResHandle). */
extern "C" void* fn_800931DC(void* self, u32 arg) { return (void*)fn_80093124(self, arg); }
/* Forwards to the category lookup, typed `void*` as `g3d/g3d_resanmtexsrt.h` declares it (it feeds a ResHandle). */
extern "C" void* fn_80093390(void* self, u32 arg) { return (void*)fn_800932D8(self, arg); }
/* Forwards to the category lookup, typed `void*` as `g3d/g3d_resanmtexsrt.h` declares it (it feeds a ResHandle). */
extern "C" void* fn_80093544(void* self, u32 arg) { return (void*)fn_8009348C(self, arg); }
/* Forwards to the category lookup, typed `void*` as `g3d/g3d_resanmtexsrt.h` declares it (it feeds a ResHandle). */
extern "C" void* fn_8009368C(void* self, u32 arg) { return (void*)fn_800935D4(self, arg); }

/* ====================== 0x80092B9C-0x80093484: the six value-type constructors ================= */
/* `ResAnmChr`/`ResAnmVis`/`ResAnmClr`/`ResAnmTexPat`/`ResAnmTexSrt`/`ResAnmShp` share the shape:
 * store the data pointer through the object, assert the pointer is 4-byte aligned, return the
 * object.  The object is a single pointer (`ResCommon<...>`), so its address arrives in r3. */

extern "C" void fn_80092C00(u32* self, u32 ptr);
extern "C" void fn_80092DB4(u32* self, u32 ptr);
extern "C" void fn_80092F68(u32* self, u32 ptr);
extern "C" void fn_8009311C(u32* self, u32 ptr);
extern "C" void fn_800932D0(u32* self, u32 ptr);
extern "C" void fn_80093484(u32* self, u32 ptr);

extern "C" void fn_80092C00(u32* self, u32 ptr) { *self = ptr; }
extern "C" void fn_80092DB4(u32* self, u32 ptr) { *self = ptr; }
extern "C" void fn_80092F68(u32* self, u32 ptr) { *self = ptr; }
extern "C" void fn_8009311C(u32* self, u32 ptr) { *self = ptr; }
extern "C" void fn_800932D0(u32* self, u32 ptr) { *self = ptr; }
extern "C" void fn_80093484(u32* self, u32 ptr) { *self = ptr; }

extern "C" void* fn_80092B9C(u32* self, u32 ptr) {
    fn_80092C00(self, ptr);
    if ((ptr & 3) != 0) {
        nw4r::db::Panic("g3d_resanmchr_ac.h", 39, "NW4R:Failed assertion !((u32)p & 0x3)");
    }
    return self;
}

extern "C" void* fn_80092D50(u32* self, u32 ptr) {
    fn_80092DB4(self, ptr);
    if ((ptr & 3) != 0) {
        nw4r::db::Panic("g3d_resanmvis_ac.h", 39, "NW4R:Failed assertion !((u32)p & 0x3)");
    }
    return self;
}

extern "C" void* fn_80092F04(u32* self, u32 ptr) {
    fn_80092F68(self, ptr);
    if ((ptr & 3) != 0) {
        nw4r::db::Panic("g3d_resanmclr_ac.h", 39, "NW4R:Failed assertion !((u32)p & 0x3)");
    }
    return self;
}

extern "C" void* fn_800930B8(u32* self, u32 ptr) {
    fn_8009311C(self, ptr);
    if ((ptr & 3) != 0) {
        nw4r::db::Panic("g3d_resanmtexpat_ac.h", 92, "NW4R:Failed assertion !((u32)p & 0x3)");
    }
    return self;
}

extern "C" void* fn_8009326C(u32* self, u32 ptr) {
    fn_800932D0(self, ptr);
    if ((ptr & 3) != 0) {
        nw4r::db::Panic("g3d_resanmtexsrt_ac.h", 38, "NW4R:Failed assertion !((u32)p & 0x3)");
    }
    return self;
}

extern "C" void* fn_80093420(u32* self, u32 ptr) {
    fn_80093484(self, ptr);
    if ((ptr & 3) != 0) {
        nw4r::db::Panic("g3d_resanmshp_ac.h", 166, "NW4R:Failed assertion !((u32)p & 0x3)");
    }
    return self;
}

/* ==================== 0x80091D20: the checked `ResAnmTexSrt` field reader ===================== */
/* The same shape as `g3d/g3d_anmtexsrt.cpp`'s fn_8006D9A4: resolve the resource through
 * fn_8006CDBC, read the body word at +0x10, index the handle, then look the key up in the table. */

typedef struct {
    /* +0x00 */ u8 pad_0x00[0x10];
    /* +0x10 */ u32 field_0x10;
} ResAnmTexSrtBody; /* size: 0x14 (lower bound - only +0x10 is evidenced) */

extern "C" u32 fn_80091D20(void* self, void* key) {
    ResAnmTexSrtBody* body = (ResAnmTexSrtBody*)fn_8006CDBC(self);
    u32 tmp = fn_8006D9FC(self, body->field_0x10);

    return (u32)(*reinterpret_cast<nw4r::g3d::ResDic*>(&tmp))[(int)key];
}

/* ==================== the `ResFile` dictionary chain (rule 2 declarations) ==================== */

extern "C" u32 fn_80092444(void* self);             /* this unit (0x80092444) */

/* The ten "no-key" accessors: build the dictionary name from the category label, resolve the
 * `ResFile` root dictionary at +0x18, look the entry up, then read the body word through the
 * indexed resolver.  They share one shape (the target bodies are instruction-identical modulo the
 * label), so they are written once each with the label that differs. */

extern "C" u32 fn_80092588(void* self) {
    u32 entry = (u32)nw4r::g3d::ResDic((void*)(fn_80092444(self) + 24))[nw4r::g3d::ResName((void*)lbl_80590A60)];
    if (entry != 0) {
        return nw4r::g3d::ResDic((void*)entry).GetNumData();
    }
    return 0;
}

extern "C" u32 fn_8009284C(void* self) {
    u32 entry = (u32)nw4r::g3d::ResDic((void*)(fn_80092444(self) + 24))[nw4r::g3d::ResName((void*)lbl_80590A80)];
    if (entry != 0) {
        return nw4r::g3d::ResDic((void*)entry).GetNumData();
    }
    return 0;
}

extern "C" u32 fn_80092B10(void* self) {
    u32 entry = (u32)nw4r::g3d::ResDic((void*)(fn_80092444(self) + 24))[nw4r::g3d::ResName((void*)lbl_80590AA0)];
    if (entry != 0) {
        return nw4r::g3d::ResDic((void*)entry).GetNumData();
    }
    return 0;
}

extern "C" u32 fn_80092CC4(void* self) {
    u32 entry = (u32)nw4r::g3d::ResDic((void*)(fn_80092444(self) + 24))[nw4r::g3d::ResName((void*)lbl_80590AC0)];
    if (entry != 0) {
        return nw4r::g3d::ResDic((void*)entry).GetNumData();
    }
    return 0;
}

extern "C" u32 fn_80092E78(void* self) {
    u32 entry = (u32)nw4r::g3d::ResDic((void*)(fn_80092444(self) + 24))[nw4r::g3d::ResName((void*)lbl_80590AE0)];
    if (entry != 0) {
        return nw4r::g3d::ResDic((void*)entry).GetNumData();
    }
    return 0;
}

extern "C" u32 fn_8009302C(void* self) {
    u32 entry = (u32)nw4r::g3d::ResDic((void*)(fn_80092444(self) + 24))[nw4r::g3d::ResName((void*)lbl_80590B00)];
    if (entry != 0) {
        return nw4r::g3d::ResDic((void*)entry).GetNumData();
    }
    return 0;
}

extern "C" u32 fn_800931E0(void* self) {
    u32 entry = (u32)nw4r::g3d::ResDic((void*)(fn_80092444(self) + 24))[nw4r::g3d::ResName((void*)lbl_80590B20)];
    if (entry != 0) {
        return nw4r::g3d::ResDic((void*)entry).GetNumData();
    }
    return 0;
}

extern "C" u32 fn_80093394(void* self) {
    u32 entry = (u32)nw4r::g3d::ResDic((void*)(fn_80092444(self) + 24))[nw4r::g3d::ResName((void*)lbl_80590B40)];
    if (entry != 0) {
        return nw4r::g3d::ResDic((void*)entry).GetNumData();
    }
    return 0;
}

extern "C" u32 fn_80093548(void* self) {
    u32 entry = (u32)nw4r::g3d::ResDic((void*)(fn_80092444(self) + 24))[nw4r::g3d::ResName((void*)lbl_80590B60)];
    if (entry != 0) {
        return nw4r::g3d::ResDic((void*)entry).GetNumData();
    }
    return 0;
}

extern "C" u32 fn_80093690(void* self) {
    u32 entry = (u32)nw4r::g3d::ResDic((void*)(fn_80092444(self) + 24))[nw4r::g3d::ResName((void*)lbl_80590B80)];
    if (entry != 0) {
        return nw4r::g3d::ResDic((void*)entry).GetNumData();
    }
    return 0;
}

/* ==================== the keyed "category + name" accessors (0xB8) ==================== */
/* Same dictionary chain as the no-key accessors, then the key is resolved through fn_80062750 and
 * wrapped in the value-type constructor for that category. */

extern "C" void* fn_80092D50(u32* self, u32 ptr);
extern "C" void* fn_80092F04(u32* self, u32 ptr);
extern "C" void* fn_800930B8(u32* self, u32 ptr);
extern "C" void* fn_8009326C(u32* self, u32 ptr);
extern "C" void* fn_80093420(u32* self, u32 ptr);

extern "C" u32 fn_80092DBC(void* self, u32 arg) {
    u32 entry = (u32)nw4r::g3d::ResDic((void*)(fn_80092444(self) + 24))[nw4r::g3d::ResName((void*)lbl_80590AE0)];
    if (entry != 0) {
        return *(u32*)fn_80092D50(&ResHandleTemp().self()->word, (u32)nw4r::g3d::ResDic((void*)entry)[(int)arg]);
    }
    return *(u32*)fn_80092D50(&ResHandleTemp().self()->word, 0);
}

extern "C" u32 fn_80092F70(void* self, u32 arg) {
    u32 entry = (u32)nw4r::g3d::ResDic((void*)(fn_80092444(self) + 24))[nw4r::g3d::ResName((void*)lbl_80590B00)];
    if (entry != 0) {
        return *(u32*)fn_80092F04(&ResHandleTemp().self()->word, (u32)nw4r::g3d::ResDic((void*)entry)[(int)arg]);
    }
    return *(u32*)fn_80092F04(&ResHandleTemp().self()->word, 0);
}

extern "C" u32 fn_80093124(void* self, u32 arg) {
    u32 entry = (u32)nw4r::g3d::ResDic((void*)(fn_80092444(self) + 24))[nw4r::g3d::ResName((void*)lbl_80590B20)];
    if (entry != 0) {
        return *(u32*)fn_800930B8(&ResHandleTemp().self()->word, (u32)nw4r::g3d::ResDic((void*)entry)[(int)arg]);
    }
    return *(u32*)fn_800930B8(&ResHandleTemp().self()->word, 0);
}

extern "C" u32 fn_800932D8(void* self, u32 arg) {
    u32 entry = (u32)nw4r::g3d::ResDic((void*)(fn_80092444(self) + 24))[nw4r::g3d::ResName((void*)lbl_80590B40)];
    if (entry != 0) {
        return *(u32*)fn_8009326C(&ResHandleTemp().self()->word, (u32)nw4r::g3d::ResDic((void*)entry)[(int)arg]);
    }
    return *(u32*)fn_8009326C(&ResHandleTemp().self()->word, 0);
}

extern "C" u32 fn_8009348C(void* self, u32 arg) {
    u32 entry = (u32)nw4r::g3d::ResDic((void*)(fn_80092444(self) + 24))[nw4r::g3d::ResName((void*)lbl_80590B60)];
    if (entry != 0) {
        return *(u32*)fn_80093420(&ResHandleTemp().self()->word, (u32)nw4r::g3d::ResDic((void*)entry)[(int)arg]);
    }
    return *(u32*)fn_80093420(&ResHandleTemp().self()->word, 0);
}

/* ==================== the Mdl/Pltt/Tex/Scn keyed accessors (0xB8) ==================== */
/* The same chain, but the ctor differs per category and two of them key through the `ResDic`
 * string lookup (fn_80092250) rather than the raw table (fn_80062750). */

#include "fn_8004CAD8.h"           /* res_tex_ctor/res_pltt_ctor (rule 2, owner header) */
#include "g3d/g3d_scninline.h" /* the scene inline copies, placement new (rule 2) */
#include "g3d/g3d_resanmamblight.h"/* fn_80089F94 (rule 2, owner header) */
#include "g3d/g3d_resmat.h"

#pragma pool_data off

extern "C" void* fn_80092B9C(u32* self, u32 ptr);

extern "C" u32 fn_800924CC(void* self, u32 arg) {
    u32 entry = (u32)nw4r::g3d::ResDic((void*)(fn_80092444(self) + 24))[nw4r::g3d::ResName((void*)lbl_80590A60)];
    if (entry != 0) {
        return *(u32*)fn_8007B878((s32)&ResHandleTemp().self()->word, (s32)(u32)nw4r::g3d::ResDic((void*)entry)[(int)arg]);
    }
    return *(u32*)fn_8007B878((s32)&ResHandleTemp().self()->word, 0);
}

extern "C" u32 fn_80092790(void* self, u32 arg) {
    u32 entry = (u32)nw4r::g3d::ResDic((void*)(fn_80092444(self) + 24))[nw4r::g3d::ResName((void*)lbl_80590A80)];
    if (entry != 0) {
        return *(u32*)res_pltt_ctor(&ResHandleTemp().self()->word, (u32)nw4r::g3d::ResDic((void*)entry)[(int)arg]);
    }
    return *(u32*)res_pltt_ctor(&ResHandleTemp().self()->word, 0);
}

extern "C" u32 fn_80092A54(void* self, u32 arg) {
    u32 entry = (u32)nw4r::g3d::ResDic((void*)(fn_80092444(self) + 24))[nw4r::g3d::ResName((void*)lbl_80590AA0)];
    if (entry != 0) {
        return *(u32*)res_tex_ctor(&ResHandleTemp().self()->word, (u32)nw4r::g3d::ResDic((void*)entry)[(int)arg]);
    }
    return *(u32*)res_tex_ctor(&ResHandleTemp().self()->word, 0);
}

extern "C" u32 fn_80092C08(void* self, u32 arg) {
    u32 entry = (u32)nw4r::g3d::ResDic((void*)(fn_80092444(self) + 24))[nw4r::g3d::ResName((void*)lbl_80590AC0)];
    if (entry != 0) {
        return *(u32*)fn_80092B9C(&ResHandleTemp().self()->word, (u32)nw4r::g3d::ResDic((void*)entry)[(int)arg]);
    }
    return *(u32*)fn_80092B9C(&ResHandleTemp().self()->word, 0);
}

extern "C" u32 fn_800935D4(void* self, u32 arg) {
    u32 entry = (u32)nw4r::g3d::ResDic((void*)(fn_80092444(self) + 24))[nw4r::g3d::ResName((void*)lbl_80590B80)];
    if (entry != 0) {
        return *(u32*)fn_80089F94(&ResHandleTemp().self()->word, (u32)nw4r::g3d::ResDic((void*)entry)[(int)arg]);
    }
    return *(u32*)fn_80089F94(&ResHandleTemp().self()->word, 0);
}

/* ==================== the two real-named `ResFile` accessors ==================== */
/* `GetResPltt(const char*)`/`GetResTex(const char*)` are the only functions in this range the
 * runtime dump names (evidence class 2).  Rule 9 requires them defined through their owner so MWCC
 * emits the map's mangled symbols (`GetResPltt__Q34nw4r3g3d7ResFileCFPCc`), and their bodies key the
 * entry through the string lookup rather than the raw table. */

namespace nw4r {
namespace g3d {

u32 ResFile::GetResPltt(const char* pName) const {
    u32 entry = (u32)nw4r::g3d::ResDic((void*)(fn_80092444((void*)this) + 24))[nw4r::g3d::ResName((void*)lbl_80590A80)];
    if (entry != 0) {
        return *(u32*)res_pltt_ctor(&ResHandleTemp().self()->word, (u32)nw4r::g3d::ResDic((void*)entry)[pName]);
    }
    return *(u32*)res_pltt_ctor(&ResHandleTemp().self()->word, 0);
}

u32 ResFile::GetResTex(const char* pName) const {
    u32 entry = (u32)nw4r::g3d::ResDic((void*)(fn_80092444((void*)this) + 24))[nw4r::g3d::ResName((void*)lbl_80590AA0)];
    if (entry != 0) {
        return *(u32*)res_tex_ctor(&ResHandleTemp().self()->word, (u32)nw4r::g3d::ResDic((void*)entry)[pName]);
    }
    return *(u32*)res_tex_ctor(&ResHandleTemp().self()->word, 0);
}

} // namespace g3d
} // namespace nw4r

/* ==================== 0x80092250-0x80092444: the ResFile/ResDic chain ==================== */

extern "C" u32 strlen(const char* p);
extern "C" int strcmp(const char* a, const char* b);
extern "C" u32 fn_80091D78(void* a, nw4r::g3d::ResName b);

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


/* `ResDic::GetOffset` guarded by the `g3d_rescommon_ac.h` assert. */
extern "C" u32 fn_800922D0(u32* self, u32 ofs) {
    if (ofs == 0) {
        nw4r::db::Panic("g3d_rescommon_ac.h", 123, "NW4R:Failed assertion ofs != 0");
    }
    return *self + ofs;
}

/* `ResDic::Get(const char*)` - validate, bound, look the name up, then resolve its offset. */
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

/* `ResCommon<T>::ref()` guarded by the `g3d_resfile_ac.h` inlined assert; returns the body word. */
extern "C" u32 fn_80092444(void* self) {
    if (fn_800924B8((u32*)self) == 0) {
        nw4r::db::Panic("g3d_resfile_ac.h", 60, "%s::%s: Object not valid.", fn_800924B0(), "ref");
    }
    return fn_800924A8((u32*)self);
}

/* `ResDic::Get(const ResName&)` - validate both the dictionary and the name, then resolve. */
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

/* `ResDic::GetIndex(const ResName&)` - the same lookup, returning the entry index or -1. */
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

/* ==================== 0x80091D78: the ResName equality test ==================== */

extern "C" u32 fn_80091D78(void* a, nw4r::g3d::ResName b) {
    if (fn_800912D4(a) == fn_800912D4(&b)) {
        return strcmp(fn_8009125C(a), fn_8009125C(&b)) == 0;
    }
    return 0;
}

/* ==================== 0x8009371C/0x80093794: the category-present predicates ==================== */

extern "C" u32 fn_8009371C(void* self) {
    return (u32)nw4r::g3d::ResDic((void*)(fn_80092444(self) + 24))[nw4r::g3d::ResName((void*)lbl_80590AC0)] != 0;
}

extern "C" u32 fn_80093794(void* self) {
    return (u32)nw4r::g3d::ResDic((void*)(fn_80092444(self) + 24))[nw4r::g3d::ResName((void*)lbl_80590B40)] != 0;
}

/* ==================== 0x800926CC/0x80092990: the two-level (ResName key) accessors ==================== */

/* 0x800926CC (0xC4): looks `name` up in the file's palette dictionary. */
nw4r::g3d::ResPltt nw4r::g3d::ResFile::GetResPltt(const ResName name) const {
    u32 entry = (u32)nw4r::g3d::ResDic((void*)(fn_80092444(const_cast<ResFile*>(this)) + 24))[nw4r::g3d::ResName((void*)lbl_80590A80)];
    if (entry != 0) {
        return *reinterpret_cast<ResPltt*>(res_pltt_ctor(&ResHandleTemp().self()->word, (u32)nw4r::g3d::ResDic((void*)entry)[name]));
    }
    return *reinterpret_cast<ResPltt*>(res_pltt_ctor(&ResHandleTemp().self()->word, 0));
}

/* 0x80092990 (0xC4): looks `name` up in the file's texture dictionary. */
nw4r::g3d::ResTex nw4r::g3d::ResFile::GetResTex(const ResName name) const {
    u32 entry = (u32)nw4r::g3d::ResDic((void*)(fn_80092444(const_cast<ResFile*>(this)) + 24))[nw4r::g3d::ResName((void*)lbl_80590AA0)];
    if (entry != 0) {
        return *reinterpret_cast<ResTex*>(res_tex_ctor(&ResHandleTemp().self()->word, (u32)nw4r::g3d::ResDic((void*)entry)[name]));
    }
    return *reinterpret_cast<ResTex*>(res_tex_ctor(&ResHandleTemp().self()->word, 0));
}

/* ==================== 0x8009380C/CleanUpTracks: the track walks ==================== */


extern "C" void CleanUpTracks(void* self) {
    u32 count = fn_80092588(self);
    for (u32 i = 0; i < count; i++) {
        void* value = fn_80092584(self, i);
        reinterpret_cast<nw4r::g3d::ResMdl*>(&value)->Release();
    }

    u32 count2 = fn_800931E0(self);
    for (u32 i = 0; i < count2; i++) {
        void* value = fn_800931DC(self, i);
        fn_80091628(&value);
    }
}

extern "C" u32 fn_8009380C(void* self, u32* arg) {
    u32 i;
    u32 present = 1;
    u32 count = fn_80092588(self);
    for (i = 0; i < count; i++) {
        present = reinterpret_cast<nw4r::g3d::ResMdl*>(ResHandleTemp().set((u32)fn_80092584(self, i)))
                      ->Bind(*reinterpret_cast<const nw4r::g3d::ResFile*>(ResHandleTemp().set(*arg))) &&
                  present;
    }

    u32 count2 = fn_800931E0(self);
    for (i = 0; i < count2; i++) {
        present = fn_800913A0(ResHandleTemp().set((u32)fn_800931DC(self, i)), ResHandleTemp().set(*arg));
    }
    return present;
}
