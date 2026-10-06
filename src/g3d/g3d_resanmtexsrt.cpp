/*
 * g3d/g3d_resanmtexsrt.cpp - nw4r g3d `ResAnmTexSrt` SRT-animation evaluator and the `ResFile::GetResXxx`
 *   accessor bodies the `g3d_res*_ac.h` headers inline beside it: byte-IO (de)serialisers, the six value-type
 *   constructors, the ten category lookups and their forwarders.
 * RANGE. .text 0x800916FC-0x80093990 (75 functions); extab, extabindex, .data 0x80590928-0x80590D78 (opens on
 *   "g3d_resanmtexsrt.cpp"; the `_ac.h` assert strings share the fragment), .sdata 0x80791270-0x80791280, .sdata2
 *   0x80795F08-0x80795F10.  The right edge is `g3d/g3d_resfile.cpp`'s first body, a proven seam.
 * NAMES. Map stems (the dump answers `zz_` placeholders) except the two `nw4r::g3d::ResFile` members
 *   `GetResPltt`/`GetResTex`.
 * RESIDUALS. Unwritten (objdiff scores them zero): 0x800916FC-0x80091CEC (fn_800916FC, the `GetAnmResult`
 *   dispatcher, and fn_80091B50), 0x80092020-0x80092234 (fn_80092020, fn_8009213C).
 *   Partial (32): fn_80091D78, fn_80091E44, fn_80091EE8, fn_80091FD0, fn_80092234, fn_800924CC,
 *   0x80092588-0x80092848 and 0x8009284C-0x80092B0C (four each), fn_80092B10, fn_80092C08, fn_80092CC4,
 *   fn_80092DBC, fn_80092E78, fn_80092F70, fn_8009302C, fn_80093124, fn_800931E0, fn_800932D8, fn_80093394,
 *   fn_8009348C, 0x80093548-0x8009368C (two), 0x80093690-0x800938EC (four).
 *   flipcheck: `.text` 0x19A0 of 0x2294; `.data`, `.sdata` and `.sdata2` are claimed and not emitted.
 * SHAPES. File-scope `#pragma peephole off`: the constructor guards keep retail's split `clrlwi` + `cmpwi`.
 */

#include "types.h"
#include "g3d/g3d_resanmtexsrt.h" /* `nw4r::g3d::ResFile` (this unit's own header, rule 1) */
#include "g3d/g3d_rescommon.h"    /* `nw4r::g3d::ResName` (rule 1) */
#include "g3d/g3d_resshp.h"       /* `nw4r::g3d::ResTex`/`ResPltt` (rule 2) */
#include "nw4r/g3d/res_common.h"   /* ResHandle (rule 1) */
#include "g3d/g3d_anmchr.h"        /* fn_8006268C/fn_80062750/fn_80062914 (rule 2, owner header) */
#include "g3d/fn_800680CC.h"       /* fn_80069664/fn_8006CDBC/fn_8006D9FC (rule 2, owner header) */
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

/* The `ResFile` accessors' assert strings, this unit's own `.data` fragment. */
extern const char lbl_80590928[]; /* "g3d_resanmtexsrt.cpp" */
extern const char lbl_80590940[]; /* "this(=%p) is not valid pointer." */
extern const char lbl_80590974[]; /* "pResult(=%p) is not valid pointer." */
extern const char lbl_805909AC[]; /* "frame is infinite or nan." */
extern const char lbl_805909EC[]; /* "matData(=%p) is not valid pointer." */
extern const char lbl_80590A28[]; /* "ofs != 0" */
extern const char lbl_80590A48[]; /* "g3d_rescommon_ac.h" */
/* The `ResName` categories of the `ResFile` root dictionary (length-prefixed records). */
extern const u8 lbl_80590A60[]; /* "3DModels(NW4R)" */
extern const u8 lbl_80590A80[]; /* "Palettes(NW4R)" */
extern const u8 lbl_80590AA0[]; /* "Textures(NW4R)" */
extern const u8 lbl_80590AC0[]; /* "AnmChr(NW4R)" */
extern const u8 lbl_80590AE0[]; /* "AnmVis(NW4R)" */
extern const u8 lbl_80590B00[]; /* "AnmClr(NW4R)" */
extern const u8 lbl_80590B20[]; /* "AnmTexPat(NW4R)" */
extern const u8 lbl_80590B40[]; /* "AnmTexSrt(NW4R)" */
extern const u8 lbl_80590B60[]; /* "AnmShp(NW4R)" */
extern const u8 lbl_80590B80[]; /* "AnmScn(NW4R)" */
extern const char lbl_80590BD8[]; /* "NW4R:Failed assertion !((u32)p & 0x3)" */
extern const char lbl_80590C00[]; /* "g3d_resanmshp_ac.h" */
extern const char lbl_80590C14[]; /* the same assert string */
extern const char lbl_80590C3C[]; /* "g3d_resanmtexsrt_ac.h" */
extern const char lbl_80590C54[]; /* the same assert string */
extern const char lbl_80590C7C[]; /* "g3d_resanmtexpat_ac.h" */
extern const char lbl_80590C94[]; /* the same assert string */
extern const char lbl_80590CBC[]; /* "g3d_resanmclr_ac.h" */
extern const char lbl_80590CD0[]; /* the same assert string */
extern const char lbl_80590CF8[]; /* "g3d_resanmvis_ac.h" */
extern const char lbl_80590D0C[]; /* the same assert string */
extern const char lbl_80590D34[]; /* "g3d_resanmchr_ac.h" */
extern const char lbl_80590D48[]; /* "%s::%s: Object not valid." */
extern const char lbl_80590D64[]; /* "g3d_resfile_ac.h" */

/* The `.sdata` words the `ResFile` default-construct path hands back the address of (the queue's
 * lbl_80791270/lbl_80791278). */
extern u32 lbl_80791270;
extern u32 lbl_80791278;

} // extern "C"

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
    fn_80091EAC(p, value >> 24);
    fn_80091EAC(p + 1, (value >> 16) & 0xFF);
    fn_80091EAC(p + 2, (value >> 8) & 0xFF);
    fn_80091EAC(p + 3, value & 0xFF);
}

/* 0x80091DFC (0x48): writes a BP command: the 0x61 opcode, then the register word. */
void nw4r::g3d::detail::ResWriteBPCmd(u8* p, u32 value) {
    fn_80091EAC(p, 0x61);
    fn_80091E44(p + 1, value);
}

extern "C" u32 fn_80091EE8(const u8* p) {
    u32 value = ((u32)fn_80091F68(p) & 0xFF) << 24;
    value |= ((u32)fn_80091F68(p + 1) & 0xFF) << 16;
    value |= ((u32)fn_80091F68(p + 2) & 0xFF) << 8;
    value |= (u32)fn_80091F68(p + 3) & 0xFF;
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
    u32 word = fn_80091EE8(p + 1) | value;
    ResWriteBPCmd(p, word | 0xFE000000);
}

/* ============================== 0x80092234-0x800924B8: the ResFile helpers ===================== */

extern "C" u32* fn_80092234(u32* self, u32 ofs) {
    u32 base = *self;
    if (ofs == 0) {
        return 0;
    }
    return (u32*)(base + ofs);
}

extern "C" u32 fn_800924A8(u32* p) {
    return *p;
}

extern "C" void* fn_800924B0(void) {
    return &lbl_80791278;
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
        nw4r::db::Panic(lbl_80590D34, 39, lbl_80590D0C);
    }
    return self;
}

extern "C" void* fn_80092D50(u32* self, u32 ptr) {
    fn_80092DB4(self, ptr);
    if ((ptr & 3) != 0) {
        nw4r::db::Panic(lbl_80590CF8, 39, lbl_80590CD0);
    }
    return self;
}

extern "C" void* fn_80092F04(u32* self, u32 ptr) {
    fn_80092F68(self, ptr);
    if ((ptr & 3) != 0) {
        nw4r::db::Panic(lbl_80590CBC, 39, lbl_80590C94);
    }
    return self;
}

extern "C" void* fn_800930B8(u32* self, u32 ptr) {
    fn_8009311C(self, ptr);
    if ((ptr & 3) != 0) {
        nw4r::db::Panic(lbl_80590C7C, 92, lbl_80590C54);
    }
    return self;
}

extern "C" void* fn_8009326C(u32* self, u32 ptr) {
    fn_800932D0(self, ptr);
    if ((ptr & 3) != 0) {
        nw4r::db::Panic(lbl_80590C3C, 38, lbl_80590C14);
    }
    return self;
}

extern "C" void* fn_80093420(u32* self, u32 ptr) {
    fn_80093484(self, ptr);
    if ((ptr & 3) != 0) {
        nw4r::db::Panic(lbl_80590C00, 166, lbl_80590BD8);
    }
    return self;
}

/* ==================== 0x80091D20: the checked `ResAnmTexSrt` field reader ===================== */
/* The same shape as `g3d/fn_800680CC.cpp`'s fn_8006D9A4: resolve the resource through
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
    u32 obj;
    u32 entry = (u32)nw4r::g3d::ResDic((void*)(fn_80092444(self) + 24))[nw4r::g3d::ResName((void*)lbl_80590AE0)];
    if (entry != 0) {
        u32 key = (u32)nw4r::g3d::ResDic((void*)entry)[(int)arg];
        fn_80092D50(&obj, key);
        return obj;
    }
    fn_80092D50(&obj, 0);
    return obj;
}

extern "C" u32 fn_80092F70(void* self, u32 arg) {
    u32 obj;
    u32 entry = (u32)nw4r::g3d::ResDic((void*)(fn_80092444(self) + 24))[nw4r::g3d::ResName((void*)lbl_80590B00)];
    if (entry != 0) {
        u32 key = (u32)nw4r::g3d::ResDic((void*)entry)[(int)arg];
        fn_80092F04(&obj, key);
        return obj;
    }
    fn_80092F04(&obj, 0);
    return obj;
}

extern "C" u32 fn_80093124(void* self, u32 arg) {
    u32 obj;
    u32 entry = (u32)nw4r::g3d::ResDic((void*)(fn_80092444(self) + 24))[nw4r::g3d::ResName((void*)lbl_80590B20)];
    if (entry != 0) {
        u32 key = (u32)nw4r::g3d::ResDic((void*)entry)[(int)arg];
        fn_800930B8(&obj, key);
        return obj;
    }
    fn_800930B8(&obj, 0);
    return obj;
}

extern "C" u32 fn_800932D8(void* self, u32 arg) {
    u32 obj;
    u32 entry = (u32)nw4r::g3d::ResDic((void*)(fn_80092444(self) + 24))[nw4r::g3d::ResName((void*)lbl_80590B40)];
    if (entry != 0) {
        u32 key = (u32)nw4r::g3d::ResDic((void*)entry)[(int)arg];
        fn_8009326C(&obj, key);
        return obj;
    }
    fn_8009326C(&obj, 0);
    return obj;
}

extern "C" u32 fn_8009348C(void* self, u32 arg) {
    u32 obj;
    u32 entry = (u32)nw4r::g3d::ResDic((void*)(fn_80092444(self) + 24))[nw4r::g3d::ResName((void*)lbl_80590B60)];
    if (entry != 0) {
        u32 key = (u32)nw4r::g3d::ResDic((void*)entry)[(int)arg];
        fn_80093420(&obj, key);
        return obj;
    }
    fn_80093420(&obj, 0);
    return obj;
}

/* ==================== the Mdl/Pltt/Tex/Scn keyed accessors (0xB8) ==================== */
/* The same chain, but the ctor differs per category and two of them key through the `ResDic`
 * string lookup (fn_80092250) rather than the raw table (fn_80062750). */

#include "fn_8004CAD8.h"           /* res_tex_ctor/res_pltt_ctor (rule 2, owner header) */
#include "g3d/fn_80075DCC.h"       /* fn_8007B878 (rule 2, owner header) */
#include "g3d/g3d_resanmamblight.h"/* fn_80089F94 (rule 2, owner header) */
#include "g3d/g3d_resmat.h"

extern "C" void* fn_80092B9C(u32* self, u32 ptr);

extern "C" u32 fn_800924CC(void* self, u32 arg) {
    u32 obj;
    u32 entry = (u32)nw4r::g3d::ResDic((void*)(fn_80092444(self) + 24))[nw4r::g3d::ResName((void*)lbl_80590A60)];
    if (entry != 0) {
        u32 key = (u32)nw4r::g3d::ResDic((void*)entry)[(int)arg];
        fn_8007B878((s32)&obj, (s32)key);
        return obj;
    }
    fn_8007B878((s32)&obj, 0);
    return obj;
}

extern "C" u32 fn_80092790(void* self, u32 arg) {
    u32 obj;
    u32 entry = (u32)nw4r::g3d::ResDic((void*)(fn_80092444(self) + 24))[nw4r::g3d::ResName((void*)lbl_80590A80)];
    if (entry != 0) {
        u32 key = (u32)nw4r::g3d::ResDic((void*)entry)[(int)arg];
        res_pltt_ctor(&obj, key);
        return obj;
    }
    res_pltt_ctor(&obj, 0);
    return obj;
}

extern "C" u32 fn_80092A54(void* self, u32 arg) {
    u32 obj;
    u32 entry = (u32)nw4r::g3d::ResDic((void*)(fn_80092444(self) + 24))[nw4r::g3d::ResName((void*)lbl_80590AA0)];
    if (entry != 0) {
        u32 key = (u32)nw4r::g3d::ResDic((void*)entry)[(int)arg];
        res_tex_ctor(&obj, key);
        return obj;
    }
    res_tex_ctor(&obj, 0);
    return obj;
}

extern "C" u32 fn_80092C08(void* self, u32 arg) {
    u32 obj;
    u32 entry = (u32)nw4r::g3d::ResDic((void*)(fn_80092444(self) + 24))[nw4r::g3d::ResName((void*)lbl_80590AC0)];
    if (entry != 0) {
        u32 key = (u32)nw4r::g3d::ResDic((void*)entry)[(int)arg];
        fn_80092B9C(&obj, key);
        return obj;
    }
    fn_80092B9C(&obj, 0);
    return obj;
}

extern "C" u32 fn_800935D4(void* self, u32 arg) {
    u32 obj;
    u32 entry = (u32)nw4r::g3d::ResDic((void*)(fn_80092444(self) + 24))[nw4r::g3d::ResName((void*)lbl_80590B80)];
    if (entry != 0) {
        u32 key = (u32)nw4r::g3d::ResDic((void*)entry)[(int)arg];
        fn_80089F94(&obj, key);
        return obj;
    }
    fn_80089F94(&obj, 0);
    return obj;
}

/* ==================== the two real-named `ResFile` accessors ==================== */
/* `GetResPltt(const char*)`/`GetResTex(const char*)` are the only functions in this range the
 * runtime dump names (evidence class 2).  Rule 9 requires them defined through their owner so MWCC
 * emits the map's mangled symbols (`GetResPltt__Q34nw4r3g3d7ResFileCFPCc`), and their bodies key the
 * entry through the string lookup rather than the raw table. */

namespace nw4r {
namespace g3d {

u32 ResFile::GetResPltt(const char* pName) const {
    u32 obj;
    u32 entry = (u32)nw4r::g3d::ResDic((void*)(fn_80092444((void*)this) + 24))[nw4r::g3d::ResName((void*)lbl_80590A80)];
    if (entry != 0) {
        u32 key = (u32)nw4r::g3d::ResDic((void*)entry)[pName];
        res_pltt_ctor(&obj, key);
        return obj;
    }
    res_pltt_ctor(&obj, 0);
    return obj;
}

u32 ResFile::GetResTex(const char* pName) const {
    u32 obj;
    u32 entry = (u32)nw4r::g3d::ResDic((void*)(fn_80092444((void*)this) + 24))[nw4r::g3d::ResName((void*)lbl_80590AA0)];
    if (entry != 0) {
        u32 key = (u32)nw4r::g3d::ResDic((void*)entry)[pName];
        res_tex_ctor(&obj, key);
        return obj;
    }
    res_tex_ctor(&obj, 0);
    return obj;
}

} // namespace g3d
} // namespace nw4r

/* ==================== 0x80092250-0x80092444: the ResFile/ResDic chain ==================== */

extern "C" u32 strlen(const char* p);
extern "C" int strcmp(const char* a, const char* b);
extern "C" u32 fn_8009213C(void* self, const char* name, u32 len);
extern "C" u32 fn_80092020(void* self, void* name);


/* `ResDic::GetOffset` guarded by the `g3d_rescommon_ac.h` assert. */
extern "C" u32 fn_800922D0(u32* self, u32 ofs) {
    if (ofs == 0) {
        nw4r::db::Panic(lbl_80590A48, 123, lbl_80590A28);
    }
    return *self + ofs;
}

/* `ResDic::Get(const char*)` - validate, bound, look the name up, then resolve its offset. */
/* 0x80092250 (0x80): returns the data of the entry named `name`, or NULL. */
/* untyped: opaque handle - the entry's data block */
void* nw4r::g3d::ResDic::operator[](const char* name) const {
    if (fn_800628B4(const_cast<ResDic*>(this)) != 0) {
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
        nw4r::db::Panic(lbl_80590D64, 60, lbl_80590D48, fn_800924B0(), &lbl_80791270);
    }
    return fn_800924A8((u32*)self);
}

/* `ResDic::Get(const ResName&)` - validate both the dictionary and the name, then resolve. */
/* 0x80092330 (0x84): returns the data of the entry named by `name`, or NULL. */
/* untyped: opaque handle - the entry's data block */
void* nw4r::g3d::ResDic::operator[](const ResName name) const {
    if (fn_800628B4(const_cast<ResDic*>(this)) != 0) {
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
extern "C" s32 fn_800923B4(void* self, void* arg) {
    if (fn_800628B4(self) != 0) {
        if (reinterpret_cast<const nw4r::g3d::ResName*>(arg)->IsValid() != 0) {
            u32 local = *(u32*)arg;
            u32 e = fn_80092020(self, &local);
            if (e != 0) {
                u32 base = (u32)fn_800628A4(self);
                return (s32)(e - (base + 24)) / 16;
            }
        }
    }
    return -1;
}

/* ==================== 0x80091D78: the ResName equality test ==================== */

extern "C" u32 fn_80091D78(void* a, void* b) {
    if (fn_800912D4(a) != fn_800912D4(b)) {
        return 0;
    }
    return strcmp(fn_8009125C(a), fn_8009125C(b)) == 0;
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
    u32 obj;
    u32 entry = (u32)nw4r::g3d::ResDic((void*)(fn_80092444(const_cast<ResFile*>(this)) + 24))[nw4r::g3d::ResName((void*)lbl_80590A80)];
    if (entry != 0) {
        u32 e2 = (u32)nw4r::g3d::ResDic((void*)entry)[name];
        res_pltt_ctor(&obj, e2);
        return *reinterpret_cast<ResPltt*>(&obj);
    }
    res_pltt_ctor(&obj, 0);
    return *reinterpret_cast<ResPltt*>(&obj);
}

/* 0x80092990 (0xC4): looks `name` up in the file's texture dictionary. */
nw4r::g3d::ResTex nw4r::g3d::ResFile::GetResTex(const ResName name) const {
    u32 obj;
    u32 entry = (u32)nw4r::g3d::ResDic((void*)(fn_80092444(const_cast<ResFile*>(this)) + 24))[nw4r::g3d::ResName((void*)lbl_80590AA0)];
    if (entry != 0) {
        u32 e2 = (u32)nw4r::g3d::ResDic((void*)entry)[name];
        res_tex_ctor(&obj, e2);
        return *reinterpret_cast<ResTex*>(&obj);
    }
    res_tex_ctor(&obj, 0);
    return *reinterpret_cast<ResTex*>(&obj);
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
    u32 present = 1;
    u32 count = fn_80092588(self);
    for (u32 i = 0; i < count; i++) {
        u32 key = *arg;
        void* value = fn_80092584(self, i);
        u32 flag = 0;
        if (reinterpret_cast<nw4r::g3d::ResMdl*>(&value)->Bind(reinterpret_cast<const nw4r::g3d::ResFile&>(key)) &&
            present != 0) {
            flag = 1;
        }
        present = flag;
    }

    u32 count2 = fn_800931E0(self);
    for (u32 i = 0; i < count2; i++) {
        u32 key = *arg;
        void* value = fn_800931DC(self, i);
        present = fn_800913A0(&value, &key);
    }
    return present;
}
