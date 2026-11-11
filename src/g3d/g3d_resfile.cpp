/*
 * g3d/g3d_resfile.cpp - nw4r g3d `ResFile`: the root-dictionary lookups per category, the bind/release walks, the
 *   container checks/releases and the out-of-line `g3d_resmat_ac.h` accessors.
 * RANGE. .text 0x80092444-0x8009411C (76 functions); extab 0x80009290-0x80009450, extabindex
 *   0x80021FB4-0x80022254, .data 0x80590A60-0x80590D78 (the ten 32-aligned category names, "g3d_resfile.cpp",
 *   then the `_ac.h` inline tail), .sdata 0x80791270-0x80791280.  The left edge is `ResFile::ref` (0x80092444,
 *   asserts "g3d_resfile_ac.h"); the next body, res_mat_pix_dc_store (0x8009411C), belongs to `g3d/g3d_resmat.cpp`.
 * NAMES. The category accessors keep their map stems.
 * RESIDUALS. none.  The flip links because `tools/elf/objextab.py` gives the object's extab/extabindex entries the
 *   map's `@etb_`/`@eti_` names with global binding (playbook 59).
 * SHAPES. The lookups sit under `#pragma peephole off`; their `_ac.h` accessors are `inline` and
 *   `__declspec(noinline)`, each placed after a dead-stripped one-call referencer (res_file_root,
 *   res_file_anm_*_init), so the weak copies land at retail's addresses and their strings form the tail;
 *   `ResFile::ref` names its member through a local `static char` ("ref" first in `.sdata`).  The handles are
 *   `ResHandleTemp` class temporaries.
 */

#include "types.h"
#include "gx.h"                  /* GXWGFifo (rule 1) */
#include "nw4r/g3d/res_common.h" /* ResHandle (rule 1) */
#include "g3d/g3d_anmshp.h"      /* fn_800695EC (rule 2) */
#include "g3d/g3d_anmtexpat.h"   /* fn_8006993C (rule 2) */
#include "g3d/g3d_anmtexsrt.h"   /* fn_8006CDBC (rule 2) */
#include "g3d/g3d_anmscn.h"      /* fn_8006584C (rule 2) */
#include "g3d/g3d_anmclr.h"      /* fn_80063FD0 (rule 2) */
#include "g3d/g3d_scninline.h" /* the scene inline copies, placement new (rule 2) */
#include "g3d/g3d_calcmaterial.h" /* fn_8006F158/0x8006F298 (rule 2) */
#include "g3d/g3d_anmvis.h"      /* fn_8006EC3C (rule 2) */
#include "g3d/g3d_anmchr.h"      /* nw4r::g3d::ResAnmChr (rule 2) */
#include "g3d/g3d_calcview.h"    /* fn_800700C0 (rule 2) */
#include "g3d/g3d_state.h"
#include "g3d/g3d_resvtx.h"
#include "g3d/g3d_cpu.h"
#include "g3d/g3d_resfile.h"     /* nw4r::g3d::ResFile and the ten category accessors (this unit's own header) */
#include "g3d/g3d_resmat.h"
#include "g3d/g3d_rescommon.h"
#include "g3d/g3d_resshp.h"
#include "fn_8004CAD8.h"
#include "g3d/g3d_resanmamblight.h"
#include "g3d/g3d_resanmscn.h"
#include "g3d/g3d_anmshp.h"

namespace nw4r {
namespace db {

/* `Panic(const char* pFile, int line, const char* pFmt, ...)` - the trailing `e` in the map's mangling
 * is MWCC's vararg marker.  Declared in the namespace (C++ linkage), never as the mangled spelling
 * (rule 9). */
void Panic(const char* pFile, int line, const char* pFmt, ...);

} // namespace db
} // namespace nw4r

/* ------------------------------------------------------------------------------------------------ */
/* The `ResFile` release helpers are `g3d/g3d_restex.cpp`'s (`g3d/g3d_restex.h`); `GXCallDisplayList` is the
 * SDK's (`RVLGX/GXPixel.h`). */
#include "g3d/g3d_restex.h"
#include "RVLGX/GXPixel.h"

/* ------------------------------------------------------------------------------------------------ */
/* The types this unit names.                                                                         */
/* ------------------------------------------------------------------------------------------------ */

/* The revision word each resource-type check reads out of a resolved block: only +0x8 is touched
 * here, so the two leading words are modelled as opaque padding (approximate prefix). */
typedef struct {
    /* +0x0 */ u32 pad_0x0;
    /* +0x4 */ u32 pad_0x4;
    /* +0x8 */ u32 revision;
} ResRevisionWord; /* size: 0xC (approximate - only +0x8 is evidenced) */

/* The global material table `fn_8005A9BC` returns; only +0x4 is read here. */
typedef struct {
    /* +0x0 */ u32 pad_0x0;
    /* +0x4 */ u32 field_0x4;
} ResMaterialTable; /* size: 0x8 (approximate - only +0x4 is evidenced) */

/* ------------------------------------------------------------------------------------------------ */
/* 0x80092444-0x80093990: the `ResFile` root-dictionary lookups (`GetResXxx`/`GetResXxxNumEntries` per   */
/* category), the `_ac.h` accessors they keep out of line and the bind/release walks.                    */
/* ------------------------------------------------------------------------------------------------ */

/* The moved block keeps retail's split `clrlwi` + `cmpwi` (no record-form `clrlwi.`). */
#pragma peephole off

/* A `ResName` dictionary key: the name's length, then the name (`.data`, one 0x20 slot each). */
struct ResFileNameData {
    /* +0x00 */ u32 len;
    /* +0x04 */ char str[0x1C];
}; /* size: 0x20 */

/* The ten category keys of the `ResFile` root dictionary. */
static ResFileNameData res_file_name_mdl __attribute__((aligned(32))) = {14, "3DModels(NW4R)"};
static ResFileNameData res_file_name_pltt __attribute__((aligned(32))) = {14, "Palettes(NW4R)"};
static ResFileNameData res_file_name_tex __attribute__((aligned(32))) = {14, "Textures(NW4R)"};
static ResFileNameData res_file_name_anm_chr __attribute__((aligned(32))) = {12, "AnmChr(NW4R)"};
static ResFileNameData res_file_name_anm_vis __attribute__((aligned(32))) = {12, "AnmVis(NW4R)"};
static ResFileNameData res_file_name_anm_clr __attribute__((aligned(32))) = {12, "AnmClr(NW4R)"};
static ResFileNameData res_file_name_anm_tex_pat __attribute__((aligned(32))) = {15, "AnmTexPat(NW4R)"};
static ResFileNameData res_file_name_anm_tex_srt __attribute__((aligned(32))) = {15, "AnmTexSrt(NW4R)"};
static ResFileNameData res_file_name_anm_shp __attribute__((aligned(32))) = {12, "AnmShp(NW4R)"};
static ResFileNameData res_file_name_anm_scn __attribute__((aligned(32))) = {12, "AnmScn(NW4R)"};

extern "C" {
u32 fn_80092444(void* self);
u32 fn_800924A8(u32* p);
const char* fn_800924B0(void);
u32 fn_800924B8(u32* p);
u32 res_file_get_res_mdl(void* self, u32 arg);
u32 res_file_pltt_by_index(void* self, u32 arg);
u32 res_file_tex_by_index(void* self, u32 arg);
u32 res_file_get_res_anm_chr(void* self, u32 arg);
u32 fn_80092DBC(void* self, u32 arg);
u32 fn_80092F70(void* self, u32 arg);
u32 fn_80093124(void* self, u32 arg);
u32 fn_800932D8(void* self, u32 arg);
u32 fn_8009348C(void* self, u32 arg);
u32 fn_800935D4(void* self, u32 arg);
void* res_anm_chr_ctor(u32* self, u32 ptr);
void* fn_80092D50(u32* self, u32 ptr);
void* fn_80092F04(u32* self, u32 ptr);
void* fn_800930B8(u32* self, u32 ptr);
void* fn_8009326C(u32* self, u32 ptr);
void* fn_80093420(u32* self, u32 ptr);
void fn_80092C00(u32* self, u32 ptr);
void fn_80092DB4(u32* self, u32 ptr);
void fn_80092F68(u32* self, u32 ptr);
void fn_8009311C(u32* self, u32 ptr);
void fn_800932D0(u32* self, u32 ptr);
void fn_80093484(u32* self, u32 ptr);
}

/* The file's root block word; dead-stripped, it places `ResFile::ref`'s weak copy here. */
extern "C" u32 res_file_root(void* self) /* untyped: opaque handle - the ResFile handle */
{
    return fn_80092444(self);
}

/* `ResFile::ref()`: the checked root block word. */
extern "C" __declspec(noinline) inline u32 fn_80092444(void* self) {
    static char sMember[] = "ref";

    if (fn_800924B8((u32*)self) == 0) {
        nw4r::db::Panic("g3d_resfile_ac.h", 60, "%s::%s: Object not valid.", fn_800924B0(), sMember);
    }
    return fn_800924A8((u32*)self);
}

/* `ResFile::ptr()`. */
extern "C" __declspec(noinline) inline u32 fn_800924A8(u32* p) {
    return *p;
}

/* `ResFile::GetClassName()`. */
extern "C" __declspec(noinline) inline const char* fn_800924B0(void) {
    return "ResFile";
}

/* `ResFile::IsValid()`. */
extern "C" __declspec(noinline) inline u32 fn_800924B8(u32* p) {
    return *p != 0;
}






/* `ResFile::GetResMdl(int)`. */
extern "C" u32 res_file_get_res_mdl(void* self, u32 arg) {
    u32 entry = (u32)nw4r::g3d::ResDic((void*)(fn_80092444(self) + 24))[nw4r::g3d::ResName((void*)&res_file_name_mdl)];
    if (entry != 0) {
        return *(u32*)&nw4r::g3d::ResMdl(nw4r::g3d::ResDic((void*)entry)[(int)arg]);
    }
    return *(u32*)&nw4r::g3d::ResMdl((void*)0);
}

/* `ResFile::GetResMdl(u32)`, forwarding to the `int` overload. */
extern "C" void* fn_80092584(void* self, u32 arg) { return (void*)res_file_get_res_mdl(self, arg); }

/* `ResFile::GetResMdlNumEntries()`. */
extern "C" u32 fn_80092588(void* self) {
    u32 entry = (u32)nw4r::g3d::ResDic((void*)(fn_80092444(self) + 24))[nw4r::g3d::ResName((void*)&res_file_name_mdl)];
    if (entry != 0) {
        return nw4r::g3d::ResDic((void*)entry).GetNumData();
    }
    return 0;
}

namespace nw4r {
namespace g3d {

u32 ResFile::GetResPltt(const char* pName) const {
    u32 entry = (u32)nw4r::g3d::ResDic((void*)(fn_80092444((void*)this) + 24))[nw4r::g3d::ResName((void*)&res_file_name_pltt)];
    if (entry != 0) {
        return *(u32*)res_pltt_ctor(&ResHandleTemp().self()->word, (u32)nw4r::g3d::ResDic((void*)entry)[pName]);
    }
    return *(u32*)res_pltt_ctor(&ResHandleTemp().self()->word, 0);
}

/* 0x800926CC (0xC4): looks `name` up in the file's palette dictionary. */
ResPltt ResFile::GetResPltt(const ResName name) const {
    u32 entry = (u32)nw4r::g3d::ResDic((void*)(fn_80092444(const_cast<ResFile*>(this)) + 24))[nw4r::g3d::ResName((void*)&res_file_name_pltt)];
    if (entry != 0) {
        return *reinterpret_cast<ResPltt*>(res_pltt_ctor(&ResHandleTemp().self()->word, (u32)nw4r::g3d::ResDic((void*)entry)[name]));
    }
    return *reinterpret_cast<ResPltt*>(res_pltt_ctor(&ResHandleTemp().self()->word, 0));
}

}  // namespace g3d
}  // namespace nw4r

/* `ResFile::GetResPltt(int)`. */
extern "C" u32 res_file_pltt_by_index(void* self, u32 arg) {
    u32 entry = (u32)nw4r::g3d::ResDic((void*)(fn_80092444(self) + 24))[nw4r::g3d::ResName((void*)&res_file_name_pltt)];
    if (entry != 0) {
        return *(u32*)res_pltt_ctor(&ResHandleTemp().self()->word, (u32)nw4r::g3d::ResDic((void*)entry)[(int)arg]);
    }
    return *(u32*)res_pltt_ctor(&ResHandleTemp().self()->word, 0);
}

/* `ResFile::GetResPltt(u32)`. */
extern "C" void* fn_80092848(void* self, u32 arg) { return (void*)res_file_pltt_by_index(self, arg); }

/* `ResFile::GetResPlttNumEntries()`. */
extern "C" u32 fn_8009284C(void* self) {
    u32 entry = (u32)nw4r::g3d::ResDic((void*)(fn_80092444(self) + 24))[nw4r::g3d::ResName((void*)&res_file_name_pltt)];
    if (entry != 0) {
        return nw4r::g3d::ResDic((void*)entry).GetNumData();
    }
    return 0;
}

namespace nw4r {
namespace g3d {

u32 ResFile::GetResTex(const char* pName) const {
    u32 entry = (u32)nw4r::g3d::ResDic((void*)(fn_80092444((void*)this) + 24))[nw4r::g3d::ResName((void*)&res_file_name_tex)];
    if (entry != 0) {
        return *(u32*)res_tex_ctor(&ResHandleTemp().self()->word, (u32)nw4r::g3d::ResDic((void*)entry)[pName]);
    }
    return *(u32*)res_tex_ctor(&ResHandleTemp().self()->word, 0);
}

/* 0x80092990 (0xC4): looks `name` up in the file's texture dictionary. */
ResTex ResFile::GetResTex(const ResName name) const {
    u32 entry = (u32)nw4r::g3d::ResDic((void*)(fn_80092444(const_cast<ResFile*>(this)) + 24))[nw4r::g3d::ResName((void*)&res_file_name_tex)];
    if (entry != 0) {
        return *reinterpret_cast<ResTex*>(res_tex_ctor(&ResHandleTemp().self()->word, (u32)nw4r::g3d::ResDic((void*)entry)[name]));
    }
    return *reinterpret_cast<ResTex*>(res_tex_ctor(&ResHandleTemp().self()->word, 0));
}

}  // namespace g3d
}  // namespace nw4r

/* `ResFile::GetResTex(int)`. */
extern "C" u32 res_file_tex_by_index(void* self, u32 arg) {
    u32 entry = (u32)nw4r::g3d::ResDic((void*)(fn_80092444(self) + 24))[nw4r::g3d::ResName((void*)&res_file_name_tex)];
    if (entry != 0) {
        return *(u32*)res_tex_ctor(&ResHandleTemp().self()->word, (u32)nw4r::g3d::ResDic((void*)entry)[(int)arg]);
    }
    return *(u32*)res_tex_ctor(&ResHandleTemp().self()->word, 0);
}

/* `ResFile::GetResTex(u32)`. */
extern "C" void* fn_80092B0C(void* self, u32 arg) { return (void*)res_file_tex_by_index(self, arg); }

/* `ResFile::GetResTexNumEntries()`. */
extern "C" u32 fn_80092B10(void* self) {
    u32 entry = (u32)nw4r::g3d::ResDic((void*)(fn_80092444(self) + 24))[nw4r::g3d::ResName((void*)&res_file_name_tex)];
    if (entry != 0) {
        return nw4r::g3d::ResDic((void*)entry).GetNumData();
    }
    return 0;
}

/* Constructs a `ResAnmChr` handle; dead-stripped, it places the inline constructor's weak copy here. */
extern "C" void* res_file_anm_chr_init(u32* self, u32 ptr) /* untyped: opaque handle - the constructed resource handle */
{
    return res_anm_chr_ctor(self, ptr);
}

/* `ResAnmChr(void*)`: stores the block pointer and asserts its 4-byte alignment. */
extern "C" __declspec(noinline) inline void* res_anm_chr_ctor(u32* self, u32 ptr) {
    fn_80092C00(self, ptr);
    if ((ptr & 3) != 0) {
        nw4r::db::Panic("g3d_resanmchr_ac.h", 39, "NW4R:Failed assertion !((u32)p & 0x3)");
    }
    return self;
}

/* `ResCommon<ResAnmChrData>(void*)`. */
extern "C" __declspec(noinline) inline void fn_80092C00(u32* self, u32 ptr) { *self = ptr; }




/* `ResFile::GetResAnmChr(int)`. */
extern "C" u32 res_file_get_res_anm_chr(void* self, u32 arg) {
    u32 entry = (u32)nw4r::g3d::ResDic((void*)(fn_80092444(self) + 24))[nw4r::g3d::ResName((void*)&res_file_name_anm_chr)];
    if (entry != 0) {
        return *(u32*)res_anm_chr_ctor(&ResHandleTemp().self()->word, (u32)nw4r::g3d::ResDic((void*)entry)[(int)arg]);
    }
    return *(u32*)res_anm_chr_ctor(&ResHandleTemp().self()->word, 0);
}

/* `ResFile::GetResAnmChr(u32)`. */
extern "C" void* fn_80092CC0(void* self, u32 arg) { return (void*)res_file_get_res_anm_chr(self, arg); }

/* `ResFile::GetResAnmChrNumEntries()`. */
extern "C" u32 res_file_get_res_anm_chr_num(void* self) {
    u32 entry = (u32)nw4r::g3d::ResDic((void*)(fn_80092444(self) + 24))[nw4r::g3d::ResName((void*)&res_file_name_anm_chr)];
    if (entry != 0) {
        return nw4r::g3d::ResDic((void*)entry).GetNumData();
    }
    return 0;
}

/* Constructs a `ResAnmVis` handle; dead-stripped, it places the inline constructor's weak copy here. */
extern "C" void* res_file_anm_vis_init(u32* self, u32 ptr) /* untyped: opaque handle - the constructed resource handle */
{
    return fn_80092D50(self, ptr);
}

/* `ResAnmVis(void*)`. */
extern "C" __declspec(noinline) inline void* fn_80092D50(u32* self, u32 ptr) {
    fn_80092DB4(self, ptr);
    if ((ptr & 3) != 0) {
        nw4r::db::Panic("g3d_resanmvis_ac.h", 39, "NW4R:Failed assertion !((u32)p & 0x3)");
    }
    return self;
}

/* `ResCommon<ResAnmVisData>(void*)`. */
extern "C" __declspec(noinline) inline void fn_80092DB4(u32* self, u32 ptr) { *self = ptr; }




/* `ResFile::GetResAnmVis(int)`. */
extern "C" u32 fn_80092DBC(void* self, u32 arg) {
    u32 entry = (u32)nw4r::g3d::ResDic((void*)(fn_80092444(self) + 24))[nw4r::g3d::ResName((void*)&res_file_name_anm_vis)];
    if (entry != 0) {
        return *(u32*)fn_80092D50(&ResHandleTemp().self()->word, (u32)nw4r::g3d::ResDic((void*)entry)[(int)arg]);
    }
    return *(u32*)fn_80092D50(&ResHandleTemp().self()->word, 0);
}

/* `ResFile::GetResAnmVis(u32)`. */
extern "C" void* fn_80092E74(void* self, u32 arg) { return (void*)fn_80092DBC(self, arg); }

/* `ResFile::GetResAnmVisNumEntries()`. */
extern "C" u32 fn_80092E78(void* self) {
    u32 entry = (u32)nw4r::g3d::ResDic((void*)(fn_80092444(self) + 24))[nw4r::g3d::ResName((void*)&res_file_name_anm_vis)];
    if (entry != 0) {
        return nw4r::g3d::ResDic((void*)entry).GetNumData();
    }
    return 0;
}

/* Constructs a `ResAnmClr` handle; dead-stripped, it places the inline constructor's weak copy here. */
extern "C" void* res_file_anm_clr_init(u32* self, u32 ptr) /* untyped: opaque handle - the constructed resource handle */
{
    return fn_80092F04(self, ptr);
}

/* `ResAnmClr(void*)`. */
extern "C" __declspec(noinline) inline void* fn_80092F04(u32* self, u32 ptr) {
    fn_80092F68(self, ptr);
    if ((ptr & 3) != 0) {
        nw4r::db::Panic("g3d_resanmclr_ac.h", 39, "NW4R:Failed assertion !((u32)p & 0x3)");
    }
    return self;
}

/* `ResCommon<ResAnmClrData>(void*)`. */
extern "C" __declspec(noinline) inline void fn_80092F68(u32* self, u32 ptr) { *self = ptr; }




/* `ResFile::GetResAnmClr(int)`. */
extern "C" u32 fn_80092F70(void* self, u32 arg) {
    u32 entry = (u32)nw4r::g3d::ResDic((void*)(fn_80092444(self) + 24))[nw4r::g3d::ResName((void*)&res_file_name_anm_clr)];
    if (entry != 0) {
        return *(u32*)fn_80092F04(&ResHandleTemp().self()->word, (u32)nw4r::g3d::ResDic((void*)entry)[(int)arg]);
    }
    return *(u32*)fn_80092F04(&ResHandleTemp().self()->word, 0);
}

/* `ResFile::GetResAnmClr(u32)`. */
extern "C" void* fn_80093028(void* self, u32 arg) { return (void*)fn_80092F70(self, arg); }

/* `ResFile::GetResAnmClrNumEntries()`. */
extern "C" u32 fn_8009302C(void* self) {
    u32 entry = (u32)nw4r::g3d::ResDic((void*)(fn_80092444(self) + 24))[nw4r::g3d::ResName((void*)&res_file_name_anm_clr)];
    if (entry != 0) {
        return nw4r::g3d::ResDic((void*)entry).GetNumData();
    }
    return 0;
}

/* Constructs a `ResAnmTexPat` handle; dead-stripped, it places the inline constructor's weak copy here. */
extern "C" void* res_file_anm_tex_pat_init(u32* self, u32 ptr) /* untyped: opaque handle - the constructed resource handle */
{
    return fn_800930B8(self, ptr);
}

/* `ResAnmTexPat(void*)`. */
extern "C" __declspec(noinline) inline void* fn_800930B8(u32* self, u32 ptr) {
    fn_8009311C(self, ptr);
    if ((ptr & 3) != 0) {
        nw4r::db::Panic("g3d_resanmtexpat_ac.h", 92, "NW4R:Failed assertion !((u32)p & 0x3)");
    }
    return self;
}

/* `ResCommon<ResAnmTexPatData>(void*)`. */
extern "C" __declspec(noinline) inline void fn_8009311C(u32* self, u32 ptr) { *self = ptr; }




/* `ResFile::GetResAnmTexPat(int)`. */
extern "C" u32 fn_80093124(void* self, u32 arg) {
    u32 entry = (u32)nw4r::g3d::ResDic((void*)(fn_80092444(self) + 24))[nw4r::g3d::ResName((void*)&res_file_name_anm_tex_pat)];
    if (entry != 0) {
        return *(u32*)fn_800930B8(&ResHandleTemp().self()->word, (u32)nw4r::g3d::ResDic((void*)entry)[(int)arg]);
    }
    return *(u32*)fn_800930B8(&ResHandleTemp().self()->word, 0);
}

/* `ResFile::GetResAnmTexPat(u32)`. */
extern "C" void* fn_800931DC(void* self, u32 arg) { return (void*)fn_80093124(self, arg); }

/* `ResFile::GetResAnmTexPatNumEntries()`. */
extern "C" u32 fn_800931E0(void* self) {
    u32 entry = (u32)nw4r::g3d::ResDic((void*)(fn_80092444(self) + 24))[nw4r::g3d::ResName((void*)&res_file_name_anm_tex_pat)];
    if (entry != 0) {
        return nw4r::g3d::ResDic((void*)entry).GetNumData();
    }
    return 0;
}

/* Constructs a `ResAnmTexSrt` handle; dead-stripped, it places the inline constructor's weak copy here. */
extern "C" void* res_file_anm_tex_srt_init(u32* self, u32 ptr) /* untyped: opaque handle - the constructed resource handle */
{
    return fn_8009326C(self, ptr);
}

/* `ResAnmTexSrt(void*)`. */
extern "C" __declspec(noinline) inline void* fn_8009326C(u32* self, u32 ptr) {
    fn_800932D0(self, ptr);
    if ((ptr & 3) != 0) {
        nw4r::db::Panic("g3d_resanmtexsrt_ac.h", 38, "NW4R:Failed assertion !((u32)p & 0x3)");
    }
    return self;
}

/* `ResCommon<ResAnmTexSrtData>(void*)`. */
extern "C" __declspec(noinline) inline void fn_800932D0(u32* self, u32 ptr) { *self = ptr; }




/* `ResFile::GetResAnmTexSrt(int)`. */
extern "C" u32 fn_800932D8(void* self, u32 arg) {
    u32 entry = (u32)nw4r::g3d::ResDic((void*)(fn_80092444(self) + 24))[nw4r::g3d::ResName((void*)&res_file_name_anm_tex_srt)];
    if (entry != 0) {
        return *(u32*)fn_8009326C(&ResHandleTemp().self()->word, (u32)nw4r::g3d::ResDic((void*)entry)[(int)arg]);
    }
    return *(u32*)fn_8009326C(&ResHandleTemp().self()->word, 0);
}

/* `ResFile::GetResAnmTexSrt(u32)`. */
extern "C" void* res_file_get_res_anm_tex_srt(void* self, u32 arg) { return (void*)fn_800932D8(self, arg); }

/* `ResFile::GetResAnmTexSrtNumEntries()`. */
extern "C" u32 res_file_get_res_anm_tex_srt_num(void* self) {
    u32 entry = (u32)nw4r::g3d::ResDic((void*)(fn_80092444(self) + 24))[nw4r::g3d::ResName((void*)&res_file_name_anm_tex_srt)];
    if (entry != 0) {
        return nw4r::g3d::ResDic((void*)entry).GetNumData();
    }
    return 0;
}

/* Constructs a `ResAnmShp` handle; dead-stripped, it places the inline constructor's weak copy here. */
extern "C" void* res_file_anm_shp_init(u32* self, u32 ptr) /* untyped: opaque handle - the constructed resource handle */
{
    return fn_80093420(self, ptr);
}

/* `ResAnmShp(void*)`. */
extern "C" __declspec(noinline) inline void* fn_80093420(u32* self, u32 ptr) {
    fn_80093484(self, ptr);
    if ((ptr & 3) != 0) {
        nw4r::db::Panic("g3d_resanmshp_ac.h", 166, "NW4R:Failed assertion !((u32)p & 0x3)");
    }
    return self;
}

/* `ResCommon<ResAnmShpData>(void*)`. */
extern "C" __declspec(noinline) inline void fn_80093484(u32* self, u32 ptr) { *self = ptr; }




/* `ResFile::GetResAnmShp(int)`. */
extern "C" u32 fn_8009348C(void* self, u32 arg) {
    u32 entry = (u32)nw4r::g3d::ResDic((void*)(fn_80092444(self) + 24))[nw4r::g3d::ResName((void*)&res_file_name_anm_shp)];
    if (entry != 0) {
        return *(u32*)fn_80093420(&ResHandleTemp().self()->word, (u32)nw4r::g3d::ResDic((void*)entry)[(int)arg]);
    }
    return *(u32*)fn_80093420(&ResHandleTemp().self()->word, 0);
}

/* `ResFile::GetResAnmShp(u32)`. */
extern "C" void* fn_80093544(void* self, u32 arg) { return (void*)fn_8009348C(self, arg); }

/* `ResFile::GetResAnmShpNumEntries()`. */
extern "C" u32 fn_80093548(void* self) {
    u32 entry = (u32)nw4r::g3d::ResDic((void*)(fn_80092444(self) + 24))[nw4r::g3d::ResName((void*)&res_file_name_anm_shp)];
    if (entry != 0) {
        return nw4r::g3d::ResDic((void*)entry).GetNumData();
    }
    return 0;
}

/* `ResFile::GetResAnmScn(int)`. */
extern "C" u32 fn_800935D4(void* self, u32 arg) {
    u32 entry = (u32)nw4r::g3d::ResDic((void*)(fn_80092444(self) + 24))[nw4r::g3d::ResName((void*)&res_file_name_anm_scn)];
    if (entry != 0) {
        return *(u32*)fn_80089F94(&ResHandleTemp().self()->word, (u32)nw4r::g3d::ResDic((void*)entry)[(int)arg]);
    }
    return *(u32*)fn_80089F94(&ResHandleTemp().self()->word, 0);
}

/* `ResFile::GetResAnmScn(u32)`. */
extern "C" void* fn_8009368C(void* self, u32 arg) { return (void*)fn_800935D4(self, arg); }

/* `ResFile::GetResAnmScnNumEntries()`. */
extern "C" u32 fn_80093690(void* self) {
    u32 entry = (u32)nw4r::g3d::ResDic((void*)(fn_80092444(self) + 24))[nw4r::g3d::ResName((void*)&res_file_name_anm_scn)];
    if (entry != 0) {
        return nw4r::g3d::ResDic((void*)entry).GetNumData();
    }
    return 0;
}

/* Whether the file has an `AnmChr` category. */
extern "C" u32 res_file_has_anm_chr(void* self) {
    return (u32)nw4r::g3d::ResDic((void*)(fn_80092444(self) + 24))[nw4r::g3d::ResName((void*)&res_file_name_anm_chr)] != 0;
}

/* Whether the file has an `AnmTexSrt` category. */
extern "C" u32 res_file_has_anm_tex_srt(void* self) {
    return (u32)nw4r::g3d::ResDic((void*)(fn_80092444(self) + 24))[nw4r::g3d::ResName((void*)&res_file_name_anm_tex_srt)] != 0;
}

/* Binds every model and scene animation of the file against `arg`; false when one fails. */
extern "C" u32 res_file_bind(void* self, u32* arg) {
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

/* Releases every model and scene animation of the file. */
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

#pragma peephole reset

/* ------------------------------------------------------------------------------------------------ */
/* The unit's own functions, in address order.                                                        */
/* ------------------------------------------------------------------------------------------------ */

extern "C" u32 fn_80093B0C(ResHandle* pSelf);
extern "C" void fn_80093A98(void* p);
extern "C" u32 fn_80093E14(ResHandle* p);
extern "C" u32 fn_80093E40(ResHandle* p);
extern "C" u32 fn_80093E64(ResHandle* p);
extern "C" u32 fn_80093E90(ResHandle* p);
extern "C" u32 fn_80093EB4(ResHandle* p);
extern "C" u32 fn_80093EE0(ResHandle* p);
extern "C" u32 fn_80093F04(ResHandle* p);
extern "C" u32 fn_80093F30(ResHandle* p);
extern "C" u32 fn_80093F54(ResHandle* p);
extern "C" u32 fn_80093F80(ResHandle* p);
extern "C" u32 fn_80093FA4(ResHandle* p);
extern "C" u32 fn_80093FD0(ResHandle* p);
extern "C" u32 fn_80093FF4(ResHandle* p);
extern "C" u32 fn_80094020(ResHandle* p);
extern "C" u32 fn_80094044(ResHandle* p);
extern "C" u32 fn_80094070(ResHandle* p);
extern "C" u32 fn_800940D0(ResHandle* pSelf, u32 offset);

/* 0x80093990 - the file-checked destructor walk: assert `CheckRevision()`, then release each of the
 * three object categories through its own helper. */
extern "C" void res_file_init(ResHandle* pSelf) {
    if (fn_80093B0C(pSelf) == 0) {
        nw4r::db::Panic("g3d_resfile.cpp", 0xDF, "NW4R:Failed assertion CheckRevision()");
    }

    u32 count = fn_80092588(pSelf);
    for (u32 i = 0; i < count; i++) {
        ResHandle item;
        item.mpData = fn_80092584(pSelf, i);
        reinterpret_cast<nw4r::g3d::ResMdl*>(&item)->Init();
    }

    count = fn_80092B10(pSelf);
    for (u32 i = 0; i < count; i++) {
        ResHandle item;
        item.mpData = fn_80092B0C(pSelf, i);
        res_tex_dc_flush(&item);
    }

    count = fn_8009284C(pSelf);
    for (u32 i = 0; i < count; i++) {
        ResHandle item;
        item.mpData = fn_80092848(pSelf, i);
        fn_80093A98(&item);
    }
}

/* 0x80093A98 - release one entry of the third category (tail call). */
extern "C" void fn_80093A98(void* p) {
    res_pltt_dc_store((ResHandle*)p, 0);
}

/* 0x80093AA0 - release every entry of the first category through `ResMdl::Terminate`. */
extern "C" void res_file_terminate(ResHandle* pSelf) {
    u32 count = fn_80092588(pSelf);
    for (u32 i = 0; i < count; i++) {
        ResHandle item;
        item.mpData = fn_80092584(pSelf, i);
        reinterpret_cast<nw4r::g3d::ResMdl*>(&item)->Terminate();
    }
}

/* 0x80093B0C - `ResFile::CheckRevision()`: walk all ten categories and report whether every entry
 * passes its own revision check. */
extern "C" u32 fn_80093B0C(ResHandle* pSelf) {
    u32 count = fn_80092588(pSelf);
    for (u32 i = 0; i < count; i++) {
        ResHandle item;
        item.mpData = fn_80092584(pSelf, i);
        if (fn_80094044(&item) == 0) {
            return 0;
        }
    }

    count = fn_80092B10(pSelf);
    for (u32 i = 0; i < count; i++) {
        ResHandle item;
        item.mpData = fn_80092B0C(pSelf, i);
        if (res_tex_check_revision(&item) == 0) {
            return 0;
        }
    }

    count = fn_8009284C(pSelf);
    for (u32 i = 0; i < count; i++) {
        ResHandle item;
        item.mpData = fn_80092848(pSelf, i);
        if (res_pltt_check_revision(&item) == 0) {
            return 0;
        }
    }

    count = res_file_get_res_anm_chr_num(pSelf);
    for (u32 i = 0; i < count; i++) {
        ResHandle item;
        item.mpData = fn_80092CC0(pSelf, i);
        if (fn_80093FF4(&item) == 0) {
            return 0;
        }
    }

    count = fn_80092E78(pSelf);
    for (u32 i = 0; i < count; i++) {
        ResHandle item;
        item.mpData = fn_80092E74(pSelf, i);
        if (fn_80093FA4(&item) == 0) {
            return 0;
        }
    }

    count = fn_8009302C(pSelf);
    for (u32 i = 0; i < count; i++) {
        ResHandle item;
        item.mpData = fn_80093028(pSelf, i);
        if (fn_80093F54(&item) == 0) {
            return 0;
        }
    }

    count = fn_800931E0(pSelf);
    for (u32 i = 0; i < count; i++) {
        ResHandle item;
        item.mpData = fn_800931DC(pSelf, i);
        if (fn_80093F04(&item) == 0) {
            return 0;
        }
    }

    count = res_file_get_res_anm_tex_srt_num(pSelf);
    for (u32 i = 0; i < count; i++) {
        ResHandle item;
        item.mpData = res_file_get_res_anm_tex_srt(pSelf, i);
        if (fn_80093EB4(&item) == 0) {
            return 0;
        }
    }

    count = fn_80093548(pSelf);
    for (u32 i = 0; i < count; i++) {
        ResHandle item;
        item.mpData = fn_80093544(pSelf, i);
        if (fn_80093E64(&item) == 0) {
            return 0;
        }
    }

    count = fn_80093690(pSelf);
    for (u32 i = 0; i < count; i++) {
        ResHandle item;
        item.mpData = fn_8009368C(pSelf, i);
        if (fn_80093E14(&item) == 0) {
            return 0;
        }
    }

    return 1;
}

/* The ten per-category revision checks: each compares the category's revision word against the
 * revision the loader accepts. */
extern "C" u32 fn_80093E14(ResHandle* p) {
    return fn_80093E40(p) == 5;
}

extern "C" u32 fn_80093E40(ResHandle* p) {
    return ((ResRevisionWord*)fn_8006584C(p))->revision;
}

extern "C" u32 fn_80093E64(ResHandle* p) {
    return fn_80093E90(p) == 4;
}

extern "C" u32 fn_80093E90(ResHandle* p) {
    return ((ResRevisionWord*)(u32)fn_800695EC(p))->revision;
}

extern "C" u32 fn_80093EB4(ResHandle* p) {
    return fn_80093EE0(p) == 5;
}

extern "C" u32 fn_80093EE0(ResHandle* p) {
    return ((ResRevisionWord*)(u32)fn_8006CDBC(p))->revision;
}

extern "C" u32 fn_80093F04(ResHandle* p) {
    return fn_80093F30(p) == 4;
}

extern "C" u32 fn_80093F30(ResHandle* p) {
    return ((ResRevisionWord*)(u32)fn_8006993C(p))->revision;
}

extern "C" u32 fn_80093F54(ResHandle* p) {
    return fn_80093F80(p) == 4;
}

extern "C" u32 fn_80093F80(ResHandle* p) {
    return ((ResRevisionWord*)(u32)fn_80063FD0(p))->revision;
}

extern "C" u32 fn_80093FA4(ResHandle* p) {
    return fn_80093FD0(p) == 4;
}

extern "C" u32 fn_80093FD0(ResHandle* p) {
    return ((ResRevisionWord*)(u32)fn_8006EC3C(p))->revision;
}

extern "C" u32 fn_80093FF4(ResHandle* p) {
    return fn_80094020(p) == 5;
}

extern "C" u32 fn_80094020(ResHandle* p) {
    return reinterpret_cast<const nw4r::g3d::ResAnmChr*>(p)->ref().revision;
}

extern "C" u32 fn_80094044(ResHandle* p) {
    return fn_80094070(p) == 11;
}

extern "C" u32 fn_80094070(ResHandle* p) {
    return ((ResRevisionWord*)(u32)(u32)&reinterpret_cast<const nw4r::g3d::ResMdl*>(p)->ref())->revision;
}

/* 0x80094094 - resolve the global material table and read the resource at its +0x4 offset. */
extern "C" u32 fn_80094094(ResHandle* pSelf) {
    return fn_800940D0(pSelf, ((ResMaterialTable*)(u32)(u32)&reinterpret_cast<nw4r::g3d::ResMat*>(pSelf)->ref())->field_0x4);
}

/* 0x800940D0 - return the handle stored at `self->mpData + offset` (or the null handle at offset 0). */
extern "C" u32 fn_800940D0(ResHandle* pSelf, u32 offset) {
    u32 data = (u32)pSelf->mpData;

    if (offset != 0) {
        return *(u32*)&nw4r::g3d::ResMdl((void*)(data + offset));
    } else {
        return *(u32*)&nw4r::g3d::ResMdl((void*)0);
    }
}
