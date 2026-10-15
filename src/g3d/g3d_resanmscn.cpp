/*
 * g3d/g3d_resanmscn.cpp - nw4r g3d `ResAnmScn` dictionary lookups (by name and by index, with the `*_ac.h`
 *   element constructors), channel getters (`GetResLightSet`, `GetResAnmAmbLight`, `GetResAnmLight`,
 *   `GetResAnmFog`, `GetResAnmCamera`, each with its null-safe offset helper) and the `ResAnmTexPat` accessors,
 *   `Bind(ResFile)` (fn_800913A0) and `Release()` (fn_80091628).
 * RANGE. .text 0x8009001C-0x800916FC (50 functions); extab, extabindex, .data 0x80590660-0x80590928 (the five
 *   `ResName` records, the getters' strings from "g3d_resanmscn.cpp", then the `_ac.h` inline tail), .sdata
 *   0x80791268-0x80791270.  The left seam is the first function that reads this TU's data (fn_8009001C's callers
 *   read the 0x80590660 records; extabindex 0x80021D8C).
 * NAMES. Map stems (the dump answers `zz_` placeholders); the element types come from each out-of-line
 *   constructor's `*_ac.h` assert (fn_800900A4, fn_800901AC, fn_80090380, fn_80090554, fn_80090728).
 *   res_light_set_init, res_anm_amb_light_init, res_anm_light_init, res_anm_fog_init, res_anm_camera_init and
 *   res_anm_tex_pat_ref are GUESSES (dead-stripped referencers of the inline accessors; no symbols.txt row).
 *   res_name_light_set .. res_name_cameras are GUESSES for the map's lbl_80590660..lbl_805906E0.
 * RESIDUALS. fn_80090218/3EC/5C0/794: stack slots only - retail lays the fallback handle, the `ResDic` temporary,
 *   the element handle and the two `ResName` words at 0x8/0xC/0x10/0x14/0x18, ours at 0x14/0x8/0x18/0xC/0x10
 *   (moving the two handle declarations reaches no closer).
 *   fn_800913A0: register colouring only - the two `ResAnmTexPatInfoData` counts in r31/r30 (retail r30/r29), the
 *   texture-name array in r29 (retail r31), the palette counter on r24 (retail reuses r31), and the loop
 *   temporaries at 0x0C/0x1C/0x28/0x2C (retail 0x0C/0x14/0x1C/0x28).
 *   flipcheck: `.sdata` byte-identical; `.data` identical up to the texpat `ref()` format string, which retail
 *   places 4 bytes later (0x80590908 + 8-aligned, ours 4-aligned).  The six dead-stripped referencers are
 *   object-only.
 * SHAPES. The `_ac.h` accessors that carry strings (fn_800912F8, fn_80091280 and the five element constructors
 *   with their word stores) are `inline` under `#pragma dont_inline on`, defined in string-creation order (the
 *   inline tail is reversed); each weak copy follows its first referencer - retail's dead-stripped ones are
 *   frameless one-call wrappers here.  The lookups and constructors are compiled with `#pragma peephole off`
 *   (the kept `clrlwi` + `cmpwi`); the getters keep the default, which holds the eight offset helpers' base in r0.
 *   The dictionary lookups use the copy constructor's return value (fn_8009001C) as the dictionary and declare
 *   `obj` before `resolved`; the `ResName` records are 32-aligned (retail's 0x18 bytes of fill before them, and
 *   the lookups then hold the key copy's address in r31).
 *   The five getters declare the fallback temporary before the result (r1+0xC for the empty path, r1+0x8 for the
 *   element), and the `"ref"` assert argument stays a literal (an extern loads with `lis`/`addi`, not `@sda21`).
 */

#include "types.h"
#include "nw4r/g3d/res_common.h" /* IS_VALID_PTR (rule 1) */
#include "g3d/g3d_rescommon.h"     /* nw4r::g3d::ResName (rule 2) */
#include "g3d/g3d_resanmtexsrt.h"  /* nw4r::g3d::ResFile (rule 2) */
#include "g3d/g3d_resshp.h"       /* nw4r::g3d::ResTex / ResPltt (rule 2) */
#include "nw4r/g3d/res_anm.h"    /* ResAnmAmbLightData, ResAnmFogData (rule 1) */
#include "g3d/g3d_anmscn.h"      /* fn_8006584C, owner g3d/g3d_anmscn.cpp (rule 2) */
#include "g3d/g3d_anmshp.h"      /* ResDic::GetNumData's owner (rule 2) */

#pragma pool_data off

namespace nw4r {
namespace db {

/* `Panic(const char* pFile, int line, const char* pFmt, ...)` - the assert handler.  Called through its
 * namespace owner, never its mangled spelling (rule 9). */
void Panic(const char *pFile, int line, const char *pFmt, ...);

} // namespace db
} // namespace nw4r

/* A `ResName` dictionary key: the name's length, then the name (`.data`, one 0x20 slot each). */
struct ResNameData32 {
    /* +0x00 */ u32 len;
    /* +0x04 */ char str[0x1C];
}; /* size: 0x20 */

/* The five top-level dictionary keys of a `ResAnmScn`. */
static ResNameData32 res_name_light_set __attribute__((aligned(32))) = {14, "LightSet(NW4R)"};
static ResNameData32 res_name_amb_lights __attribute__((aligned(32))) = {15, "AmbLights(NW4R)"};
static ResNameData32 res_name_lights __attribute__((aligned(32))) = {12, "Lights(NW4R)"};
static ResNameData32 res_name_fogs __attribute__((aligned(32))) = {10, "Fogs(NW4R)"};
static ResNameData32 res_name_cameras __attribute__((aligned(32))) = {13, "Cameras(NW4R)"};

/*
 * The `ResAnmScn` resource (`g3d_resanmscn.h`).  Only the fields this unit reads are named; the tail of
 * each array element is padding (the element size comes from the target's own index stride, quoted on
 * every type below).
 */
struct ResAnmScnInfoData {
    /* +0x00 */ u16 numFrame;
    /* +0x02 */ u16 numSpecularLight;
    /* +0x04 */ u32 policy;
    /* +0x08 */ u16 numResLightSetData;
    /* +0x0A */ u16 numResAnmAmbLightData;
    /* +0x0C */ u16 numResAnmLightData;
    /* +0x0E */ u16 numResAnmFogData;
    /* +0x10 */ u16 numResAnmCameraData;
}; /* size: 0x14 */

struct ResAnmScnData {
    /* +0x00 */ u32 size;
    /* +0x04 */ u32 type;
    /* +0x08 */ u32 revision;
    /* +0x0C */ s32 toResFileData;
    /* +0x10 */ s32 toScnTopLevelDic;
    /* +0x14 */ s32 toResLightSetDataArray;
    /* +0x18 */ s32 toResAnmAmbLightDataArray;
    /* +0x1C */ s32 toResAnmLightDataArray;
    /* +0x20 */ s32 toResAnmFogDataArray;
    /* +0x24 */ s32 toResAnmCameraDataArray;
    /* +0x28 */ s32 name;
    /* +0x2C */ s32 originalPath;
    /* +0x30 */ u32 pad_0x30;
    /* +0x34 */ ResAnmScnInfoData info;
}; /* size: 0x48 */

/* The three channel element types only this unit names.  Each one's head is the nw4r resource header
 * (`size`/`toResAnmScnData`/`name`/`id`/`refNumber`/...); this unit only reads `id` at +0x0C, and the
 * size is the target's index stride (`mulli` immediate) for that channel's array. */
struct ResLightSetData {
    /* +0x00 */ u32 size;
    /* +0x04 */ s32 toResAnmScnData;
    /* +0x08 */ s32 name;
    /* +0x0C */ u32 id;
    /* +0x10 */ u8 pad_0x10[0x4C - 0x10];
}; /* size: 0x4C - the stride of the `toResLightSetDataArray` index in fn_800908FC */

struct ResAnmLightData {
    /* +0x00 */ u32 size;
    /* +0x04 */ s32 toResAnmScnData;
    /* +0x08 */ s32 name;
    /* +0x0C */ u32 id;
    /* +0x10 */ u8 pad_0x10[0x5C - 0x10];
}; /* size: 0x5C - the stride in fn_80090CBC */

struct ResAnmCameraData {
    /* +0x00 */ u32 size;
    /* +0x04 */ s32 toResAnmScnData;
    /* +0x08 */ s32 name;
    /* +0x0C */ u32 id;
    /* +0x10 */ u8 pad_0x10[0x5C - 0x10];
}; /* size: 0x5C - the stride in fn_8009107C */

/* The five one-word `ResCommon<T>` handles a getter returns. */
struct ResLightSet {
    /* +0x00 */ void *mpData;
}; /* size: 0x4 */

struct ResAnmAmbLight {
    /* +0x00 */ void *mpData;
}; /* size: 0x4 */

struct ResAnmLight {
    /* +0x00 */ void *mpData;
}; /* size: 0x4 */

struct ResAnmFog {
    /* +0x00 */ void *mpData;
}; /* size: 0x4 */

struct ResAnmCamera {
    /* +0x00 */ void *mpData;
}; /* size: 0x4 */

/*
 * The `ResAnmTexPat` resource (`g3d_resanmtexpat.h`): the four arrays Bind/Release walk and the two
 * `info` counts they loop over.  The offsets and the array element types are the ones the two bodies
 * read; the element strides are 4 (`ResTex`/`ResPltt` handles, one word each).
 */
struct ResAnmTexPatInfoData {
    /* +0x00 */ u16 numFrame;
    /* +0x02 */ u16 numMaterial;
    /* +0x04 */ u16 numTexture;
    /* +0x06 */ u16 numPalette;
}; /* size: 0x8 */

struct ResAnmTexPatData {
    /* +0x00 */ u32 size;
    /* +0x04 */ u32 type;
    /* +0x08 */ u32 revision;
    /* +0x0C */ s32 toResFileData;
    /* +0x10 */ s32 toTexPatDataDic;
    /* +0x14 */ s32 toTexNameArray;
    /* +0x18 */ s32 toPlttNameArray;
    /* +0x1C */ s32 toResTexArray;
    /* +0x20 */ s32 toResPlttArray;
    /* +0x24 */ s32 toResUserData;
    /* +0x28 */ s32 name;
    /* +0x2C */ s32 originalPath;
    /* +0x30 */ ResAnmTexPatInfoData info;
}; /* size: 0x38 */

/* The one-word `ResCommon<T>` handles of the texture/palette cluster (owner: the texture TU in the
 * 0x80052xxx band, not reconstructed yet). */
struct ResTex {
    /* +0x00 */ void *mpData;
}; /* size: 0x4 */

struct ResPltt {
    /* +0x00 */ void *mpData;
}; /* size: 0x4 */

/* A resolved resource name: a one-word handle over a `{s32 length; char name[];}` record.  The next
 * unit's comparator reads its first word and `+0x4` as a length/string pair. */
struct ResName {
    /* +0x00 */ void *mpData;
}; /* size: 0x4 */

/*
 * The declarations this unit needs.  The `*_ac.h` inline constructors/resolvers are out-of-line copies
 * owned by other units (plain map stems, so C linkage): 0x800658C4/0x800658B0 by `g3d/g3d_anmscn.cpp`, the
 * five 0x800900A4..0x80090728 constructors by `g3d/g3d_resanmlight.cpp`, 0x80052984..0x80053A90 by the texture
 * units of the 0x80052xxx band, 0x8006268C/0x80062D58 by `g3d/g3d_anmchr.cpp`, 0x800699B4/0x800699A8/0x80069BD8/
 * 0x80069C14 by `g3d/g3d_anmtexpat.cpp`, and 0x800926CC/0x80092990 by `g3d/g3d_resanmtexsrt.cpp`.
 */
extern "C" {

/* This unit's own bodies, forward-declared so a callee that sits later in address order can be called
 * (rule 2: the owner of these symbols is this file).  Definitions follow in address order. */
u32 *fn_8009001C(u32 *self, u32 *src);
void fn_8009004C(u32 *self, u32 *src);
u32 fn_80090058(u32 *self, s32 offset);
void fn_80090108(u32 *self, u32 value);
void fn_80090210(u32 *self, u32 value);
u32 fn_80090218(u32 *self, u32 key);
s32 fn_800902E4(u32 *self);
void fn_800903E4(u32 *self, u32 value);
u32 fn_800903EC(u32 *self, u32 key);
s32 fn_800904B8(u32 *self);
void fn_800905B8(u32 *self, u32 value);
u32 fn_800905C0(u32 *self, u32 key);
s32 fn_8009068C(u32 *self);
void fn_8009078C(u32 *self, u32 value);
u32 fn_80090794(u32 *self, u32 key);
s32 fn_80090860(u32 *self);

ResLightSet res_anm_scn_get_res_light_set_by_ref_number(void *self, u32 idx);
void *fn_80090AC0(void *self, s32 offset);
ResAnmAmbLight res_anm_scn_get_res_anm_amb_light_by_ref_number(void *self, u32 idx);
void *fn_80090CA0(void *self, s32 offset);
ResAnmLight res_anm_scn_get_res_anm_light_by_ref_number(void *self, u32 idx);
void *fn_80090E80(void *self, s32 offset);
ResAnmFog res_anm_scn_get_res_anm_fog_by_ref_number(void *self, u32 idx);
void *fn_80091060(void *self, s32 offset);
ResAnmCamera res_anm_scn_get_res_anm_camera_by_ref_number(void *self, u32 idx);
void *fn_80091240(void *self, s32 offset);
char *fn_8009125C(void *self);
u32 *fn_80091280(void *self);
u32 fn_800912D4(void *self);
u32 fn_800912F8(void *self);
u32 fn_8009135C(void *self);
void *fn_80091364(void *self, const void *src);
void fn_80091394(void *dst, const void *src);
u32 fn_800913A0(void *self, void *file);
u32 fn_80091584(void *self, u32 idx);
void *fn_800915C0(void *self, s32 offset);
void *fn_800915DC(void *self, s32 offset);
void *fn_800915F8(void *self, s32 offset);
void fn_80091628(void *self);

/* The `ResAnmScn` handle's out-of-line `IsValid()`/`ref()` copies (0x800658C4 / 0x800658B0). */
u32 fn_800658C4(void *self);
void *fn_800658B0(void *self);

/* The five element constructors (`*_ac.h`).  Each stores `pData` in the handle and asserts its 4-byte
 * alignment; MWCC's constructor convention returns the object address in r3. */
void *fn_800900A4(void *self, const void *pData); /* ResLightSet */
void *fn_800901AC(void *self, const void *pData); /* ResAnmAmbLight */
void *fn_80090380(void *self, const void *pData); /* ResAnmLight */
void *fn_80090554(void *self, const void *pData); /* ResAnmFog */
void *fn_80090728(void *self, const void *pData); /* ResAnmCamera */

/* The texture/palette handle helpers (0x80052xxx band). */
void *res_tex_assign(void *dst, const void *src);             /* store a ResTex handle */
void *res_pltt_assign(void *dst, const void *src);             /* store a ResPltt handle */
void *res_tex_ctor(void *self, u32 value);                  /* construct a ResTex handle */
void *res_pltt_ctor(void *self, u32 value);                  /* construct a ResPltt handle */

/* The name/handle store helpers: store the source word at +0x0 of `self` and return `self`. */
void *res_name_copy_ctor(void *self, void *src); /* g3d/g3d_anmchr.cpp */
void *fn_80069BD8(void *self, void *src); /* g3d/g3d_anmtexpat.cpp */
void *fn_80069C14(void *self, void *src); /* g3d/g3d_anmtexpat.cpp */

/* The `ResAnmTexPat` ref()-check helpers (g3d/g3d_anmtexpat.cpp): `fn_800699B4` is `*(u32*)p != 0` and
 * `fn_800699A8` returns the class-name string the message is built from. */
u32 fn_800699B4(void *p);
u32 fn_800699A8(void);

/* The `ResFile` lookups (`g3d/g3d_resanmtexsrt.cpp`): resolve a name to a texture/palette handle. */

#pragma peephole off

/* The `*_ac.h` inline accessors this TU keeps, called out of line (`dont_inline`).  They are defined in
 * the order their strings are created: the inline tail is emitted in reverse, after every out-of-line
 * string. */
#pragma dont_inline on

/* `ResAnmTexPat::ref()`: the checked data pointer.  The message's class-name argument identifies the
 * class the `g3d_resanmtexpat_ac.h` inline belongs to. */
inline u32 fn_800912F8(void *self)
{
    if (fn_800699B4(self) == 0) {
        nw4r::db::Panic("g3d_resanmtexpat_ac.h", 92, "%s::%s: Object not valid.", (const char *)fn_800699A8(), "ref");
    }
    return fn_8009135C(self);
}

/* The checked data pointer of a one-word handle (`ResCommon<T>::ref()`); panics on a NULL `mpData`. */
inline u32 *fn_80091280(void *self)
{
    if (*(u32 **)self == NULL) {
        nw4r::db::Panic("g3d_rescommon_ac.h", 143, "NW4R:Pointer must not be NULL (mpData)");
    }
    return *(u32 **)self;
}

/* Stores the resolved ambient-light pointer and asserts its 4-byte alignment. */
inline void *fn_800901AC(void *self, const void *pData)
{
    u32 value = (u32)pData;

    fn_80090210((u32 *)self, value);
    if (value & 3) {
        nw4r::db::Panic("g3d_resanmamblight_ac.h", 0x25, "NW4R:Failed assertion !((u32)p & 0x3)");
    }
    return self;
}

/* Stores a word. */
inline void fn_80090210(u32 *self, u32 value)
{
    *self = value;
}

/* Stores the resolved light pointer and asserts its 4-byte alignment. */
inline void *fn_80090380(void *self, const void *pData)
{
    u32 value = (u32)pData;

    fn_800903E4((u32 *)self, value);
    if (value & 3) {
        nw4r::db::Panic("g3d_resanmlight_ac.h", 0x8F, "NW4R:Failed assertion !((u32)p & 0x3)");
    }
    return self;
}

/* Stores a word. */
inline void fn_800903E4(u32 *self, u32 value)
{
    *self = value;
}

/* Stores the resolved light-set pointer and asserts its 4-byte alignment. */
inline void *fn_800900A4(void *self, const void *pData)
{
    u32 value = (u32)pData;

    fn_80090108((u32 *)self, value);
    if (value & 3) {
        nw4r::db::Panic("g3d_reslightset_ac.h", 0x27, "NW4R:Failed assertion !((u32)p & 0x3)");
    }
    return self;
}

/* Stores a word. */
inline void fn_80090108(u32 *self, u32 value)
{
    *self = value;
}

/* Stores the resolved fog pointer and asserts its 4-byte alignment. */
inline void *fn_80090554(void *self, const void *pData)
{
    u32 value = (u32)pData;

    fn_800905B8((u32 *)self, value);
    if (value & 3) {
        nw4r::db::Panic("g3d_resanmfog_ac.h", 0x3E, "NW4R:Failed assertion !((u32)p & 0x3)");
    }
    return self;
}

/* Stores a word. */
inline void fn_800905B8(u32 *self, u32 value)
{
    *self = value;
}

/* Stores the resolved camera pointer and asserts its 4-byte alignment. */
inline void *fn_80090728(void *self, const void *pData)
{
    u32 value = (u32)pData;

    fn_8009078C((u32 *)self, value);
    if (value & 3) {
        nw4r::db::Panic("g3d_resanmcamera_ac.h", 0x7D, "NW4R:Failed assertion !((u32)p & 0x3)");
    }
    return self;
}

/* Stores a word. */
inline void fn_8009078C(u32 *self, u32 value)
{
    *self = value;
}


/* ------------------------------------------------------------------------------------------------ *
 * 0x8009001C-0x800908FC - the `ResAnmScn` dictionary lookups by name and the `*_ac.h` element constructors.
 * ------------------------------------------------------------------------------------------------ */

#pragma peephole off

/* Copies one word through `self` and returns `self` (nw4r's fluent setter). */
u32 *fn_8009001C(u32 *self, u32 *src)
{
    fn_8009004C(self, src);
    return self;
}

/* Copies one word. */
void fn_8009004C(u32 *self, u32 *src)
{
    *self = *src;
}

/* Resolves a sub-resource offset and dereferences it. */
u32 fn_80090058(u32 *self, s32 offset)
{
    u32 base = *self;

    if (offset != 0) {
        return (u32)nw4r::g3d::ResDic((void*)(base + offset)).mpData;
    }
    return (u32)nw4r::g3d::ResDic((void*)(0)).mpData;
}

/* Constructs a `ResLightSet` handle; dead-stripped, it places the inline constructor's weak copy here. */
ResLightSet *res_light_set_init(ResLightSet *pHandle, const ResLightSetData *pData)
{
    return (ResLightSet *)fn_800900A4(pHandle, pData);
}

/* Resolves the "LightSet(NW4R)" dictionary entry through the scene dictionary. */
s32 fn_80090110(u32 *self)
{
    u32 obj;
    u32 resolved;
    s32 found;

    found = (s32)(*reinterpret_cast<nw4r::g3d::ResDic*>(
                  fn_8009001C(&obj, &(resolved = fn_80090058(self, fn_8006584C(self)->field_0x10)))))
                  [nw4r::g3d::ResName(&res_name_light_set)];
    if (found != 0) {
        return nw4r::g3d::ResDic((void*)found).GetNumData();
    }
    return 0;
}

/* Constructs a `ResAnmAmbLight` handle; dead-stripped, it places the inline constructor's weak copy here. */
ResAnmAmbLight *res_anm_amb_light_init(ResAnmAmbLight *pHandle, const ResAnmAmbLightData *pData)
{
    return (ResAnmAmbLight *)fn_800901AC(pHandle, pData);
}

/* Resolves the "AmbLights(NW4R)" dictionary entry and reads the entry for `key`. */
u32 fn_80090218(u32 *self, u32 key)
{
    u32 obj;
    u32 resolved;
    s32 found;

    found = (s32)(*reinterpret_cast<nw4r::g3d::ResDic*>(
                  fn_8009001C(&obj, &(resolved = fn_80090058(self, fn_8006584C(self)->field_0x10)))))
                  [nw4r::g3d::ResName(&res_name_amb_lights)];
    if (found != 0) {
        u32 light;
        return *(u32 *)fn_800901AC(&light, (const void *)nw4r::g3d::ResDic((void*)found)[(int)key]);
    }
    u32 none;
    return *(u32 *)fn_800901AC(&none, 0);
}

/* Tail thunk to fn_80090218. */
u32 res_anm_scn_get_res_anm_amb_light(u32 *self, u32 key)
{
    return fn_80090218(self, key);
}

/* Resolves the "AmbLights(NW4R)" dictionary entry through the scene dictionary. */
s32 fn_800902E4(u32 *self)
{
    u32 obj;
    u32 resolved;
    s32 found;

    found = (s32)(*reinterpret_cast<nw4r::g3d::ResDic*>(
                  fn_8009001C(&obj, &(resolved = fn_80090058(self, fn_8006584C(self)->field_0x10)))))
                  [nw4r::g3d::ResName(&res_name_amb_lights)];
    if (found != 0) {
        return nw4r::g3d::ResDic((void*)found).GetNumData();
    }
    return 0;
}

/* Constructs a `ResAnmLight` handle; dead-stripped, it places the inline constructor's weak copy here. */
ResAnmLight *res_anm_light_init(ResAnmLight *pHandle, const ResAnmLightData *pData)
{
    return (ResAnmLight *)fn_80090380(pHandle, pData);
}

/* Resolves the "Lights(NW4R)" dictionary entry and reads the entry for `key`. */
u32 fn_800903EC(u32 *self, u32 key)
{
    u32 obj;
    u32 resolved;
    s32 found;

    found = (s32)(*reinterpret_cast<nw4r::g3d::ResDic*>(
                  fn_8009001C(&obj, &(resolved = fn_80090058(self, fn_8006584C(self)->field_0x10)))))
                  [nw4r::g3d::ResName(&res_name_lights)];
    if (found != 0) {
        u32 light;
        return *(u32 *)fn_80090380(&light, (const void *)nw4r::g3d::ResDic((void*)found)[(int)key]);
    }
    u32 none;
    return *(u32 *)fn_80090380(&none, 0);
}

/* Tail thunk to fn_800903EC. */
u32 res_anm_scn_get_res_anm_light(u32 *self, u32 key)
{
    return fn_800903EC(self, key);
}

/* Resolves the "Lights(NW4R)" dictionary entry through the scene dictionary. */
s32 fn_800904B8(u32 *self)
{
    u32 obj;
    u32 resolved;
    s32 found;

    found = (s32)(*reinterpret_cast<nw4r::g3d::ResDic*>(
                  fn_8009001C(&obj, &(resolved = fn_80090058(self, fn_8006584C(self)->field_0x10)))))
                  [nw4r::g3d::ResName(&res_name_lights)];
    if (found != 0) {
        return nw4r::g3d::ResDic((void*)found).GetNumData();
    }
    return 0;
}

/* Constructs a `ResAnmFog` handle; dead-stripped, it places the inline constructor's weak copy here. */
ResAnmFog *res_anm_fog_init(ResAnmFog *pHandle, const ResAnmFogData *pData)
{
    return (ResAnmFog *)fn_80090554(pHandle, pData);
}

/* Resolves the "Fogs(NW4R)" dictionary entry and reads the entry for `key`. */
u32 fn_800905C0(u32 *self, u32 key)
{
    u32 obj;
    u32 resolved;
    s32 found;

    found = (s32)(*reinterpret_cast<nw4r::g3d::ResDic*>(
                  fn_8009001C(&obj, &(resolved = fn_80090058(self, fn_8006584C(self)->field_0x10)))))
                  [nw4r::g3d::ResName(&res_name_fogs)];
    if (found != 0) {
        u32 light;
        return *(u32 *)fn_80090554(&light, (const void *)nw4r::g3d::ResDic((void*)found)[(int)key]);
    }
    u32 none;
    return *(u32 *)fn_80090554(&none, 0);
}

/* Tail thunk to fn_800905C0. */
u32 res_anm_scn_get_res_anm_fog(u32 *self, u32 key)
{
    return fn_800905C0(self, key);
}

/* Resolves the "Fogs(NW4R)" dictionary entry through the scene dictionary. */
s32 fn_8009068C(u32 *self)
{
    u32 obj;
    u32 resolved;
    s32 found;

    found = (s32)(*reinterpret_cast<nw4r::g3d::ResDic*>(
                  fn_8009001C(&obj, &(resolved = fn_80090058(self, fn_8006584C(self)->field_0x10)))))
                  [nw4r::g3d::ResName(&res_name_fogs)];
    if (found != 0) {
        return nw4r::g3d::ResDic((void*)found).GetNumData();
    }
    return 0;
}

/* Constructs a `ResAnmCamera` handle; dead-stripped, it places the inline constructor's weak copy here. */
ResAnmCamera *res_anm_camera_init(ResAnmCamera *pHandle, const ResAnmCameraData *pData)
{
    return (ResAnmCamera *)fn_80090728(pHandle, pData);
}

/* Resolves the "Cameras(NW4R)" dictionary entry and reads the entry for `key`. */
u32 fn_80090794(u32 *self, u32 key)
{
    u32 obj;
    u32 resolved;
    s32 found;

    found = (s32)(*reinterpret_cast<nw4r::g3d::ResDic*>(
                  fn_8009001C(&obj, &(resolved = fn_80090058(self, fn_8006584C(self)->field_0x10)))))
                  [nw4r::g3d::ResName(&res_name_cameras)];
    if (found != 0) {
        u32 light;
        return *(u32 *)fn_80090728(&light, (const void *)nw4r::g3d::ResDic((void*)found)[(int)key]);
    }
    u32 none;
    return *(u32 *)fn_80090728(&none, 0);
}

/* Tail thunk to fn_80090794. */
u32 res_anm_scn_get_res_anm_camera(u32 *self, u32 key)
{
    return fn_80090794(self, key);
}

/* Resolves the "Cameras(NW4R)" dictionary entry through the scene dictionary. */
s32 fn_80090860(u32 *self)
{
    u32 obj;
    u32 resolved;
    s32 found;

    found = (s32)(*reinterpret_cast<nw4r::g3d::ResDic*>(
                  fn_8009001C(&obj, &(resolved = fn_80090058(self, fn_8006584C(self)->field_0x10)))))
                  [nw4r::g3d::ResName(&res_name_cameras)];
    if (found != 0) {
        return nw4r::g3d::ResDic((void*)found).GetNumData();
    }
    return 0;
}

#pragma peephole on

/* ------------------------------------------------------------------------------------------------ *
 * 0x800908FC-0x8009125C - `ResAnmScn`'s five channel getters and their offset helpers.
 * ------------------------------------------------------------------------------------------------ */

/* `ResAnmScn::GetResLightSet(u32 idx)`: the light-set channel at `idx`. */
ResLightSet res_anm_scn_get_res_light_set_by_ref_number(void *self, u32 idx)
{
    const ResAnmScnData *pScn;
    const ResLightSetData *pArray;
    const ResLightSetData *pElem;
    ResLightSet fallback;
    ResLightSet result;
    u32 valid;

    if (fn_800658C4(self) == 0) {
        nw4r::db::Panic("g3d_resanmscn.cpp", 122, "NW4R:Failed assertion IsValid()");
    }
    pScn = (const ResAnmScnData *)fn_800658B0(self);
    if (pScn->info.numResLightSetData <= idx) {
        return *(ResLightSet *)fn_800900A4(&fallback, NULL);
    }
    pArray = (const ResLightSetData *)fn_80090AC0(
        self, ((const ResAnmScnData *)fn_800658B0(self))->toResLightSetDataArray);
    pElem = &pArray[idx];
    valid = IS_VALID_PTR(pArray);
    if (!valid) {
        nw4r::db::Panic("g3d_resanmscn.cpp", 132, "NW4R:Pointer Error\npArray(=%p) is not valid pointer.", pArray);
    }
    return *(ResLightSet *)fn_800900A4(
        &result, pElem->id < pScn->info.numResLightSetData ? pElem : NULL);
}

/* Resolves a self-relative offset against the handle's data base; a zero offset means "no sub-resource". */
void *fn_80090AC0(void *self, s32 offset)
{
    u32 base = *(u32 *)self;

    if (offset != 0) {
        base += offset;
        return (void *)base;
    }
    return 0;
}

/* `ResAnmScn::GetResAnmAmbLight(u32 idx)`: the ambient-light channel at `idx`. */
ResAnmAmbLight res_anm_scn_get_res_anm_amb_light_by_ref_number(void *self, u32 idx)
{
    const ResAnmScnData *pScn;
    const ResAnmAmbLightData *pArray;
    const ResAnmAmbLightData *pElem;
    ResAnmAmbLight fallback;
    ResAnmAmbLight result;
    u32 valid;

    if (fn_800658C4(self) == 0) {
        nw4r::db::Panic("g3d_resanmscn.cpp", 162, "NW4R:Failed assertion IsValid()");
    }
    pScn = (const ResAnmScnData *)fn_800658B0(self);
    if (pScn->info.numResAnmAmbLightData <= idx) {
        return *(ResAnmAmbLight *)fn_800901AC(&fallback, NULL);
    }
    pArray = (const ResAnmAmbLightData *)fn_80090CA0(
        self, ((const ResAnmScnData *)fn_800658B0(self))->toResAnmAmbLightDataArray);
    pElem = &pArray[idx];
    valid = IS_VALID_PTR(pArray);
    if (!valid) {
        nw4r::db::Panic("g3d_resanmscn.cpp", 172, "NW4R:Pointer Error\npArray(=%p) is not valid pointer.", pArray);
    }
    return *(ResAnmAmbLight *)fn_800901AC(
        &result, pElem->id < pScn->info.numResAnmAmbLightData ? pElem : NULL);
}

/* Resolves a self-relative offset against the handle's data base; a zero offset means "no sub-resource". */
void *fn_80090CA0(void *self, s32 offset)
{
    u32 base = *(u32 *)self;

    if (offset != 0) {
        base += offset;
        return (void *)base;
    }
    return 0;
}

/* `ResAnmScn::GetResAnmLight(u32 idx)`: the light channel at `idx`. */
ResAnmLight res_anm_scn_get_res_anm_light_by_ref_number(void *self, u32 idx)
{
    const ResAnmScnData *pScn;
    const ResAnmLightData *pArray;
    const ResAnmLightData *pElem;
    ResAnmLight fallback;
    ResAnmLight result;
    u32 valid;

    if (fn_800658C4(self) == 0) {
        nw4r::db::Panic("g3d_resanmscn.cpp", 202, "NW4R:Failed assertion IsValid()");
    }
    pScn = (const ResAnmScnData *)fn_800658B0(self);
    if (pScn->info.numResAnmLightData <= idx) {
        return *(ResAnmLight *)fn_80090380(&fallback, NULL);
    }
    pArray = (const ResAnmLightData *)fn_80090E80(
        self, ((const ResAnmScnData *)fn_800658B0(self))->toResAnmLightDataArray);
    pElem = &pArray[idx];
    valid = IS_VALID_PTR(pArray);
    if (!valid) {
        nw4r::db::Panic("g3d_resanmscn.cpp", 212, "NW4R:Pointer Error\npArray(=%p) is not valid pointer.", pArray);
    }
    return *(ResAnmLight *)fn_80090380(
        &result, pElem->id < pScn->info.numResAnmLightData ? pElem : NULL);
}

/* Resolves a self-relative offset against the handle's data base; a zero offset means "no sub-resource". */
void *fn_80090E80(void *self, s32 offset)
{
    u32 base = *(u32 *)self;

    if (offset != 0) {
        base += offset;
        return (void *)base;
    }
    return 0;
}

/* `ResAnmScn::GetResAnmFog(u32 idx)`: the fog channel at `idx`. */
ResAnmFog res_anm_scn_get_res_anm_fog_by_ref_number(void *self, u32 idx)
{
    const ResAnmScnData *pScn;
    const ResAnmFogData *pArray;
    const ResAnmFogData *pElem;
    ResAnmFog fallback;
    ResAnmFog result;
    u32 valid;

    if (fn_800658C4(self) == 0) {
        nw4r::db::Panic("g3d_resanmscn.cpp", 241, "NW4R:Failed assertion IsValid()");
    }
    pScn = (const ResAnmScnData *)fn_800658B0(self);
    if (pScn->info.numResAnmFogData <= idx) {
        return *(ResAnmFog *)fn_80090554(&fallback, NULL);
    }
    pArray = (const ResAnmFogData *)fn_80091060(
        self, ((const ResAnmScnData *)fn_800658B0(self))->toResAnmFogDataArray);
    pElem = &pArray[idx];
    valid = IS_VALID_PTR(pArray);
    if (!valid) {
        nw4r::db::Panic("g3d_resanmscn.cpp", 251, "NW4R:Pointer Error\npArray(=%p) is not valid pointer.", pArray);
    }
    return *(ResAnmFog *)fn_80090554(
        &result, pElem->id < pScn->info.numResAnmFogData ? pElem : NULL);
}

/* Resolves a self-relative offset against the handle's data base; a zero offset means "no sub-resource". */
void *fn_80091060(void *self, s32 offset)
{
    u32 base = *(u32 *)self;

    if (offset != 0) {
        base += offset;
        return (void *)base;
    }
    return 0;
}

/* `ResAnmScn::GetResAnmCamera(u32 idx)`: the camera channel at `idx`. */
ResAnmCamera res_anm_scn_get_res_anm_camera_by_ref_number(void *self, u32 idx)
{
    const ResAnmScnData *pScn;
    const ResAnmCameraData *pArray;
    const ResAnmCameraData *pElem;
    ResAnmCamera fallback;
    ResAnmCamera result;
    u32 valid;

    if (fn_800658C4(self) == 0) {
        nw4r::db::Panic("g3d_resanmscn.cpp", 281, "NW4R:Failed assertion IsValid()");
    }
    pScn = (const ResAnmScnData *)fn_800658B0(self);
    if (pScn->info.numResAnmCameraData <= idx) {
        return *(ResAnmCamera *)fn_80090728(&fallback, NULL);
    }
    pArray = (const ResAnmCameraData *)fn_80091240(
        self, ((const ResAnmScnData *)fn_800658B0(self))->toResAnmCameraDataArray);
    pElem = &pArray[idx];
    valid = IS_VALID_PTR(pArray);
    if (!valid) {
        nw4r::db::Panic("g3d_resanmscn.cpp", 291, "NW4R:Pointer Error\npArray(=%p) is not valid pointer.", pArray);
    }
    return *(ResAnmCamera *)fn_80090728(
        &result, pElem->id < pScn->info.numResAnmCameraData ? pElem : NULL);
}

/* Resolves a self-relative offset against the handle's data base; a zero offset means "no sub-resource". */
void *fn_80091240(void *self, s32 offset)
{
    u32 base = *(u32 *)self;

    if (offset != 0) {
        base += offset;
        return (void *)base;
    }
    return 0;
}

/* The byte array that follows the data head (the next unit's comparator walks it). */
char *fn_8009125C(void *self)
{
    return (char *)fn_80091280(self) + 4;
}

/* The word at +0x0 of the data. */
u32 fn_800912D4(void *self)
{
    return *fn_80091280(self);
}

/* Returns the `ResAnmTexPat` data; dead-stripped, it places the inline accessor's weak copy here. */
u32 res_anm_tex_pat_ref(ResHandle *pHandle)
{
    return fn_800912F8(pHandle);
}

/* The raw data word of the handle (no check). */
u32 fn_8009135C(void *self)
{
    return *(u32 *)self;
}

/* The store-and-return wrapper of `fn_80091394`. */
void *fn_80091364(void *self, const void *src)
{
    fn_80091394(self, src);
    return self;
}

/* Stores one word through `dst` and hands `self` back (the fluent store shape). */
void fn_80091394(void *dst, const void *src)
{
    *(u32 *)dst = *(const u32 *)src;
}

/* `ResAnmTexPat::Bind(ResFile file)`: resolve every texture and palette name against `file` and store
 * the handles in the two arrays; returns whether every entry resolved. */
u32 fn_800913A0(void *self, void *file)
{
    s32 numTex;
    s32 numPltt;
    u32 *pTexNameArray;
    u32 *pPlttNameArray;
    ResTex *pTexArray;
    ResPltt *pPlttArray;
    u32 numBound;
    ResAnmTexPatData *pData;
    s32 i;

    pData = (ResAnmTexPatData *)fn_800912F8(self);
    numTex = pData->info.numTexture;
    numPltt = pData->info.numPalette;

    pTexNameArray = (u32 *)fn_800915F8(
        self, ((ResAnmTexPatData *)fn_8009135C(self))->toTexNameArray);
    pPlttNameArray = (u32 *)fn_800915F8(
        self, ((ResAnmTexPatData *)fn_8009135C(self))->toPlttNameArray);
    pTexArray = (ResTex *)fn_800915DC(
        self, ((ResAnmTexPatData *)fn_8009135C(self))->toResTexArray);
    pPlttArray = (ResPltt *)fn_800915C0(
        self, ((ResAnmTexPatData *)fn_8009135C(self))->toResPlttArray);

    numBound = 0;
    for (i = 0; i < numTex; i++) {
        if (reinterpret_cast<const nw4r::g3d::ResTex*>(pTexArray)->IsValid() != 0) {
            numBound++;
        } else {
            u32 nameWord = fn_80091584(pTexNameArray, i);
            ResName name;
            ResTex tex;
            u32 handle;

            res_name_copy_ctor(&name, &nameWord);
            handle = (u32)reinterpret_cast<nw4r::g3d::ResFile*>(file)->GetResTex(*reinterpret_cast<nw4r::g3d::ResName*>(&name)).mpData;
            fn_80069C14(&tex, &handle);
            if (reinterpret_cast<const nw4r::g3d::ResTex*>(&tex)->IsValid() != 0) {
                res_tex_assign(pTexArray, &tex);
                numBound++;
            }
        }
        pTexArray++;
    }

    for (i = 0; i < numPltt; i++) {
        if (reinterpret_cast<const nw4r::g3d::ResPltt*>(pPlttArray)->IsValid() != 0) {
            numBound++;
        } else {
            u32 nameWord = fn_80091584(pPlttNameArray, i);
            ResName name;
            ResPltt pltt;
            u32 handle;

            res_name_copy_ctor(&name, &nameWord);
            handle = (u32)reinterpret_cast<nw4r::g3d::ResFile*>(file)->GetResPltt(*reinterpret_cast<nw4r::g3d::ResName*>(&name)).mpData;
            fn_80069BD8(&pltt, &handle);
            if (reinterpret_cast<const nw4r::g3d::ResPltt*>(&pltt)->IsValid() != 0) {
                res_pltt_assign(pPlttArray, &pltt);
                numBound++;
            }
        }
        pPlttArray++;
    }

    return numTex + numPltt - numBound == 0;
}

/* The name-array element at `idx`: the entry is an offset to a name record, whose four-byte head the
 * wrapper starts at. */
u32 fn_80091584(void *self, u32 idx)
{
    u32 offset = *(u32 *)((u8 *)self + idx * 4);
    return (u32)nw4r::g3d::ResName((void *)((u8 *)self + offset - 4)).mpData;
}

/* Resolves a self-relative offset against the handle's data base; a zero offset means "no sub-resource". */
void *fn_800915C0(void *self, s32 offset)
{
    u32 base = *(u32 *)self;

    if (offset != 0) {
        base += offset;
        return (void *)base;
    }
    return 0;
}

/* Resolves a self-relative offset against the handle's data base; a zero offset means "no sub-resource". */
void *fn_800915DC(void *self, s32 offset)
{
    u32 base = *(u32 *)self;

    if (offset != 0) {
        base += offset;
        return (void *)base;
    }
    return 0;
}

/* Resolves a self-relative offset against the handle's data base; a zero offset means "no sub-resource". */
void *fn_800915F8(void *self, s32 offset)
{
    u32 base = *(u32 *)self;

    if (offset != 0) {
        base += offset;
        return (void *)base;
    }
    return 0;
}

} /* extern "C" */

/* 0x80091614 (0x14): tells whether the handle is set. */
bool nw4r::g3d::ResName::IsValid() const
{
    return mpData != NULL;
}

extern "C" {

/* `ResAnmTexPat::Release()`: reset every texture and palette handle in the two arrays. */
void fn_80091628(void *self)
{
    u32 numTex;
    u16 numPltt;
    ResTex *pTexArray;
    ResPltt *pPlttArray;
    ResAnmTexPatData *pData;
    s32 i;
    s32 j;

    pData = (ResAnmTexPatData *)fn_800912F8(self);
    numTex = pData->info.numTexture;
    numPltt = pData->info.numPalette;

    pTexArray = (ResTex *)fn_800915DC(
        self, ((ResAnmTexPatData *)fn_8009135C(self))->toResTexArray);
    pPlttArray = (ResPltt *)fn_800915C0(
        self, ((ResAnmTexPatData *)fn_8009135C(self))->toResPlttArray);

    for (i = 0; i < (s32)(u32)numTex; i++) {
        ResTex tex;

        res_tex_assign(pTexArray, res_tex_ctor(&tex, 0));
        pTexArray++;
    }

    for (j = 0; j < numPltt; j++) {
        ResPltt pltt;

        res_pltt_assign(pPlttArray, res_pltt_ctor(&pltt, 0));
        pPlttArray++;
    }
}

} /* extern "C" */
