/*
 * nw4r g3d: g3d_resanmscn.cpp - the `ResAnmScn` channel getters and the `ResAnmTexPat` accessor/bind
 * cluster, `.text` 0x800908FC-0x800916FC (24 functions, 0xE00 B).
 *
 * Registered once, at its final home (docs/plan.md 12), from the pooled proposal
 * `proposal/800908FC_fn_800908FC`.  Naming - evidence class 1, a `__FILE__` string: the five channel
 * getters (fn_800908FC..fn_8009107C, lines 122/162/202/241/281 and 132/172/212/251/291) pass
 * `.data` 0x80590700 = "g3d_resanmscn.cpp" to `nw4r::db::Panic`, and the next function outside this
 * range (fn_800916FC, line 0x800916FC) passes 0x80590928 = "g3d_resanmtexsrt.cpp".  So the module is
 * `g3d` (its registered neighbours g3d_resanmfog/g3d_resanmchr are the same nw4r band), the extension
 * is `.cpp` (the `__FILE__` suffix) and the file takes the evidenced TU name; class 2 fails
 * (`dumpmap.py lookup 0x800908FC` answers the `zz_00908fc_` placeholder).
 *
 * The range spans TWO original TUs; the seam is inside it, not at its edge (recorded, not guessed
 * away):
 *
 *  * 0x800908FC-0x8009125C - `ResAnmScn`: five getters (`GetResLightSet`/`GetResAnmAmbLight`/
 *    `GetResAnmLight`/`GetResAnmFog`/`GetResAnmCamera`) plus each one's null-safe offset helper.  Every
 *    getter asserts `IsValid()`, reads one `ResAnmScnInfoData` count at +0x3C/+0x3E/+0x40/+0x42/+0x44,
 *    resolves one of the five `toResAnm*DataArray` offsets at +0x14..+0x24 and returns the element
 *    (stride 0x4C/0x1C/0x5C/0x28/0x5C - the element struct sizes) through that element type's
 *    out-of-line constructor (0x800900A4 = `g3d_reslightset_ac.h`, 0x800901AC =
 *    `g3d_resanmamblight_ac.h`, 0x80090380 = `g3d_resanmlight_ac.h`, 0x80090554 =
 *    `g3d_resanmfog_ac.h`, 0x80090728 = `g3d_resanmcamera_ac.h` - each constructor's own assert names
 *    its class header, which is what identifies the five element types).
 *  * 0x8009125C-0x800916FC - `ResAnmTexPat`: the checked `ref()` (fn_800912F8, whose assert names
 *    `g3d_resanmtexpat_ac.h` and the class string "ResAnmTexPat"), the raw data accessors, `Bind(ResFile)`
 *    (fn_800913A0) and `Release()` (fn_80091628).  Both read `ResAnmTexPatData`'s `toTexNameArray`
 *    (+0x14), `toPlttNameArray` (+0x18), `toResTexArray` (+0x1C), `toResPlttArray` (+0x20) and
 *    `info.numTexture`/`info.numPalette` (+0x34/+0x36) - the nw4r `ResAnmTexPatData` layout, which does
 *    not overlap `ResAnmScnData`'s array offsets.  The seam at 0x8009125C is where the first `ResAnmTexPat`
 *    accessor starts; the orchestrator's re-split may want to cut this unit in two there (the file name
 *    stays the evidence-backed `g3d_resanmscn.cpp` for the lower half).
 *
 * Sections: `.text` 0x800908FC-0x800916FC, `extab` 0x800091A0-0x80009208 (13 unwind-only records - one
 * per function that calls, i.e. every function but the seven leaf helpers) and `extabindex`
 * 0x80021E4C-0x80021EE8 (13 records).  Both are claimed in splits.txt; the `.data`/`.sdata` strings and
 * the one `.sdata` word ("ref") are declared here as externs (they belong to no registered unit).
 *
 * Language: C++ - `langcheck.py` says so (`__FILE__` `g3d_resanmscn.cpp` plus the mangled
 * `Panic__Q24nw4r2dbFPCciPCce` relocation), and every body is the nw4r g3d resource pattern.  The unit
 * builds with the g3d lib's flags (`cflags_g3d`: -O3, `-inline noauto`, `-Cpp_exceptions on`) - the
 * `-inline noauto` is what makes the `*_ac.h` inline constructors call out-of-line copies
 * (fn_800900A4/... and fn_80062D58/...), so those are declared and called by their map stems.
 *
 * Naming note: the symbol map has only `fn_XXXXXXXX` for this range (checked with
 * `python tools/symbols/dumpmap.py lookup 0x800908FC` -> `zz_00908fc_`, and with
 * config/RMHE08/symbols.txt, whose every `.text` entry in 0x800908FC..0x800916FC is a bare
 * `fn_XXXXXXXX`), so the map's stems stand and are the identifiers.  The `*_ac.h` file strings prove the
 * *classes*, not a better symbol name.
 *
 * Measurement: this unit is registered here for the first time, so MAIN has no split object for the
 * range; `tools/units/recompile.py g3d/g3d_resanmscn.cpp --measure <symbol>` compiles with the g3d lib's
 * real command line and scores each symbol against the retired per-function objects under
 * build/RMHE08/obj/ (`auto_fn_800908FC_text.o`, `auto_03_80090AC0_text.o`, ...).  Every one of the 24
 * symbols is measured; the per-symbol numbers are in `.pi/outbox/800908fc-fn-800908fc-ab7b.json`.
 *
 * Reconstruction status (official report metric `fuzzy_match_percent`, 2026-09-25): 23 of the 24
 * symbols are at **100.00 %** - all five getters, all eight null-safe offset helpers, and every
 * `ResAnmTexPat` accessor (fn_8009125C, fn_80091280, fn_800912D4, fn_800912F8, fn_8009135C, fn_80091364,
 * fn_80091394, fn_80091584, fn_80091614, fn_80091628) - and fn_800913A0 is at 99.30 %.  The section sizes
 * land exactly (`.text` 0xE00, extab 0x68, extabindex 0x9C), so the object is size-identical to the
 * retail split; what is left is one function's register colouring, recorded here rather than worked
 * around.  No `#pragma peephole off` / `fp_contract off`: the retail object's split `li`+`blr` tails and
 * its `lwz r0` base words are what the *default* pass produces here, and turning the peephole off moves
 * the eight offset helpers' base from r0 to r3 (98.57 %) without fixing anything else - measured, and
 * the opposite of what `g3d/g3d_resanm.c`/`g3d_resanmchr.cpp` need, so this unit carries no pragma.
 *
 * Residual - fn_800913A0 (99.30 %, 484 B): the instruction sequence is identical, the colouring is not.
 * MWCC's allocator puts the two `ResAnmTexPatInfoData` counts in r31/r30 where the retail keeps them in
 * r30/r29, the texture-name array in r29 where the retail has r31, and it coalesces the palette loop's
 * counter onto the texture loop's r24 where the retail reuses the dead `numTexture` register r31; the
 * loop-local name/handle temporaries follow at 0x0C/0x1C/0x28/0x2C against the retail's
 * 0x0C/0x14/0x1C/0x28.  Declaration and counter-type permutations were swept - counts before pointers
 * and a signed palette counter took it 97.52 % -> 98.43 % -> 99.30 % (with the peephole pragma still in
 * place), and it has held 99.30 % since the pragma came out - but this is the allocator's choice, not a
 * source shape, so it is recorded rather than chased.
 *
 * Residual - nothing else.  Two source facts were needed to reach 100 % and are worth recording: the five
 * getters' two class temporaries must be declared fallback-then-result for MWCC to put them at r1+0xC (the
 * empty path) and r1+0x8 (the element path), and the `%s::%s: Object not valid.` message's `"ref"`
 * argument must stay a *literal* (an extern would be loaded with `lis`/`addi` instead of the retail's
 * `li r7, ...@sda21`).
 */

#include "types.h"
#include "nw4r/g3d/res_common.h" /* IS_VALID_PTR (rule 1) */
#include "nw4r/g3d/res_anm.h"    /* ResAnmAmbLightData, ResAnmFogData (rule 1) */

namespace nw4r {
namespace db {

/* `Panic(const char* pFile, int line, const char* pFmt, ...)` - the assert handler.  Called through its
 * namespace owner, never its mangled spelling (rule 9). */
void Panic(const char *pFile, int line, const char *pFmt, ...);

} // namespace db
} // namespace nw4r

/* The assert file/format strings the bodies pass by address.  They are declared as externs rather than
 * written as literals: the target loads each one with its own `lis`/`addi`, and `-str reuse` would pool
 * repeated literals.  The one string the target reaches through small data - the `"ref"` argument of the
 * `%s::%s: Object not valid.` message - stays a literal, because only a literal gets the `@sda21` form
 * of `li r7, ...` that fn_800912F8 has. */
extern const char lbl_80590700[]; /* "g3d_resanmscn.cpp" */
extern const char lbl_80590718[]; /* "NW4R:Failed assertion IsValid()" */
extern const char lbl_80590738[]; /* "NW4R:Pointer Error\npArray(=%p) is not valid pointer." */
extern const char lbl_805908B0[]; /* "NW4R:Pointer must not be NULL (mpData)" */
extern const char lbl_805908D8[]; /* "g3d_rescommon_ac.h" */
extern const char lbl_805908F0[]; /* "%s::%s: Object not valid." */
extern const char lbl_8059090C[]; /* "g3d_resanmtexpat_ac.h" */

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
 * owned by other translation units (their map names are plain stems, so they stay C linkage):
 * 0x800658C4/0x800658B0 and the five 0x800900A4..0x80090728 constructors are in the unclaimed
 * 0x8009xxxx/g3d band; 0x80052984..0x80053A90 belong to the texture units in the 0x80052xxx band;
 * 0x8006268C/0x80062D58 are owned by g3d/g3d_anmchr.cpp, 0x800699B4/0x800699A8/0x80069BD8/0x80069C14 by
 * g3d/fn_800680CC.cpp, and 0x800926CC/0x80092990 by the unclaimed ResFile unit.
 */
extern "C" {

/* This unit's own bodies, forward-declared so a callee that sits later in address order can be called
 * (rule 2: the owner of these symbols is this file).  Definitions follow in address order. */
ResLightSet fn_800908FC(void *self, u32 idx);
void *fn_80090AC0(void *self, s32 offset);
ResAnmAmbLight fn_80090ADC(void *self, u32 idx);
void *fn_80090CA0(void *self, s32 offset);
ResAnmLight fn_80090CBC(void *self, u32 idx);
void *fn_80090E80(void *self, s32 offset);
ResAnmFog fn_80090E9C(void *self, u32 idx);
void *fn_80091060(void *self, s32 offset);
ResAnmCamera fn_8009107C(void *self, u32 idx);
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
u32 fn_80091614(void *self);
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
u32 fn_80052984(const void *handle);                       /* is the ResTex handle valid */
u32 fn_80052EF0(const void *handle);                       /* is the ResPltt handle valid */
void *res_tex_assign(void *dst, const void *src);             /* store a ResTex handle */
void *res_pltt_assign(void *dst, const void *src);             /* store a ResPltt handle */
void *res_tex_ctor(void *self, u32 value);                  /* construct a ResTex handle */
void *res_pltt_ctor(void *self, u32 value);                  /* construct a ResPltt handle */

/* The name/handle store helpers: store the source word at +0x0 of `self` and return `self`. */
void *fn_8006268C(void *self, u32 value); /* g3d/g3d_anmchr.cpp */
void *fn_80062D58(void *self, void *src); /* g3d/g3d_anmchr.cpp */
void *fn_80069BD8(void *self, void *src); /* g3d/fn_800680CC.cpp */
void *fn_80069C14(void *self, void *src); /* g3d/fn_800680CC.cpp */

/* The `ResAnmTexPat` ref()-check helpers (g3d/fn_800680CC.cpp): `fn_800699B4` is `*(u32*)p != 0` and
 * `fn_800699A8` returns the class-name string the message is built from. */
u32 fn_800699B4(void *p);
u32 fn_800699A8(void);

/* The `ResFile` lookups (unclaimed ResFile unit, 0x80092xxx band): resolve a name to a texture/palette
 * handle. */
u32 fn_80092990(void *file, ResName name);
u32 fn_800926CC(void *file, ResName name);

/* ------------------------------------------------------------------------------------------------ *
 * 0x800908FC-0x8009125C - `ResAnmScn`'s five channel getters and their offset helpers.
 * ------------------------------------------------------------------------------------------------ */

/* `ResAnmScn::GetResLightSet(u32 idx)`: the light-set channel at `idx`. */
ResLightSet fn_800908FC(void *self, u32 idx)
{
    const ResAnmScnData *pScn;
    const ResLightSetData *pArray;
    const ResLightSetData *pElem;
    ResLightSet fallback;
    ResLightSet result;
    u32 valid;

    if (fn_800658C4(self) == 0) {
        nw4r::db::Panic(lbl_80590700, 122, lbl_80590718);
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
        nw4r::db::Panic(lbl_80590700, 132, lbl_80590738, pArray);
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
ResAnmAmbLight fn_80090ADC(void *self, u32 idx)
{
    const ResAnmScnData *pScn;
    const ResAnmAmbLightData *pArray;
    const ResAnmAmbLightData *pElem;
    ResAnmAmbLight fallback;
    ResAnmAmbLight result;
    u32 valid;

    if (fn_800658C4(self) == 0) {
        nw4r::db::Panic(lbl_80590700, 162, lbl_80590718);
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
        nw4r::db::Panic(lbl_80590700, 172, lbl_80590738, pArray);
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
ResAnmLight fn_80090CBC(void *self, u32 idx)
{
    const ResAnmScnData *pScn;
    const ResAnmLightData *pArray;
    const ResAnmLightData *pElem;
    ResAnmLight fallback;
    ResAnmLight result;
    u32 valid;

    if (fn_800658C4(self) == 0) {
        nw4r::db::Panic(lbl_80590700, 202, lbl_80590718);
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
        nw4r::db::Panic(lbl_80590700, 212, lbl_80590738, pArray);
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
ResAnmFog fn_80090E9C(void *self, u32 idx)
{
    const ResAnmScnData *pScn;
    const ResAnmFogData *pArray;
    const ResAnmFogData *pElem;
    ResAnmFog fallback;
    ResAnmFog result;
    u32 valid;

    if (fn_800658C4(self) == 0) {
        nw4r::db::Panic(lbl_80590700, 241, lbl_80590718);
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
        nw4r::db::Panic(lbl_80590700, 251, lbl_80590738, pArray);
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
ResAnmCamera fn_8009107C(void *self, u32 idx)
{
    const ResAnmScnData *pScn;
    const ResAnmCameraData *pArray;
    const ResAnmCameraData *pElem;
    ResAnmCamera fallback;
    ResAnmCamera result;
    u32 valid;

    if (fn_800658C4(self) == 0) {
        nw4r::db::Panic(lbl_80590700, 281, lbl_80590718);
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
        nw4r::db::Panic(lbl_80590700, 291, lbl_80590738, pArray);
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

/* The checked data pointer of a one-word handle (`ResCommon<T>::ref()`); panics on a NULL `mpData`. */
u32 *fn_80091280(void *self)
{
    if (*(u32 **)self == NULL) {
        nw4r::db::Panic(lbl_805908D8, 143, lbl_805908B0);
    }
    return *(u32 **)self;
}

/* The word at +0x0 of the data. */
u32 fn_800912D4(void *self)
{
    return *fn_80091280(self);
}

/* `ResAnmTexPat::ref()`: the checked data pointer.  The message's class-name argument identifies the
 * class the `g3d_resanmtexpat_ac.h` inline belongs to. */
u32 fn_800912F8(void *self)
{
    if (fn_800699B4(self) == 0) {
        nw4r::db::Panic(lbl_8059090C, 92, lbl_805908F0, (const char *)fn_800699A8(), "ref");
    }
    return fn_8009135C(self);
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
        if (fn_80052984(pTexArray) != 0) {
            numBound++;
        } else {
            u32 nameWord = fn_80091584(pTexNameArray, i);
            ResName name;
            ResTex tex;
            u32 handle;

            fn_80062D58(&name, &nameWord);
            handle = fn_80092990(file, name);
            fn_80069C14(&tex, &handle);
            if (fn_80052984(&tex) != 0) {
                res_tex_assign(pTexArray, &tex);
                numBound++;
            }
        }
        pTexArray++;
    }

    for (i = 0; i < numPltt; i++) {
        if (fn_80052EF0(pPlttArray) != 0) {
            numBound++;
        } else {
            u32 nameWord = fn_80091584(pPlttNameArray, i);
            ResName name;
            ResPltt pltt;
            u32 handle;

            fn_80062D58(&name, &nameWord);
            handle = fn_800926CC(file, name);
            fn_80069BD8(&pltt, &handle);
            if (fn_80052EF0(&pltt) != 0) {
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
    ResName name;

    return *(u32 *)fn_8006268C(&name, (u32)((u8 *)self + offset - 4));
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

/* Whether the handle holds a data pointer. */
u32 fn_80091614(void *self)
{
    return *(u32 *)self != 0;
}

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
