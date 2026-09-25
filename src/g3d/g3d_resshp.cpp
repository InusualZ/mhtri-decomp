/*
 * nw4r g3d: `g3d_resshp.cpp` - the `ResShp`/`ResShpPrePrim`/`ResTagDL` accessor head,
 * `.text` 0x80099400-0x800997E0 (21 functions).
 *
 * Naming (brief section 2, evidence class 1 - a `__FILE__` string, with the boundary proven from the
 * classes).  The first body, fn_80099400, calls `fn_80077674` on its own `this`: that is `ResShp::ref`
 * (its failing assert names `.sdata` 0x807911F0 = "ResShp" as the `%s::%s: Object not valid.` class and
 * `.data` 0x8058E720 = "g3d_resshp_ac.h" as its file), while the body before it, fn_800993B4, calls
 * `fn_8005D218` = `ResNode::ref` (class-name string `.sdata` 0x80791148 = "ResNode").  Two different
 * classes cannot be methods of one source file, so 0x80099400 is the TU boundary.  The `g3d_resshp.cpp`
 * `.data` fragment starts at 0x80591618 (the exact end of `g3d_resnode.cpp`'s three-string fragment,
 * 0x805915C0 + 0x58) and runs to 0x80591860 (`lbl_80591860` = "g3d_cpu.cpp", the next TU's first
 * string); every `__FILE__` string this unit's asserts name - `g3d_resshp.cpp` (0x80591618),
 * `g3d_resshp_ac.h` (0x80591708/0x80591748) and `g3d_rescommon_ac.h` (0x80591780/0x805917EC) - sits
 * inside it, and so do the class-name strings the assert macro passes ("ResShpPrePrim" 0x80591718,
 * "ResTagDL" 0x80591794).  `langcheck.py` agrees: `.cpp` name plus the `Panic` C++ mangling.
 *
 * **This is the head of a TU whose tail is sibling proposal `800997E0` (0x800997E0-0x8009A748), which
 * the queue already labels `g3d_resshp.cpp`.**  The pooled brief for proposal/80098D5C capped the range
 * at 0x800997E0, but its own range 0x80098D5C-0x800997E0 is not one TU: the last body that references
 * the `g3d_resnode.cpp` `.data` fragment is 0x80099378-0x80099400, and the ResShp strings are already
 * referenced from 0x80099488 on.  This unit therefore registers only its own non-overlapping part and
 * the sibling's left seam must be widened to 0x80099400 (see the unit report and the outbox
 * `config_requests`).
 *
 * Sections: `.text` 0x80099400-0x800997E0, `extab` 0x80009880-0x800098D8 (11 records), `extabindex`
 * 0x8002289C-0x80022920 (11 entries).
 *
 * rule 7 deferred: the symbol map has only `fn_XXXXXXXX` for this range (checked with `grep` over
 * config/RMHE08/symbols.txt: every 0x80099400..0x800997E0 row is a bare `fn_XXXXXXXX`).
 *
 * What the bodies are: fn_80099400/fn_8009943C, fn_800995A0/fn_800995D4/fn_80099638 and
 * fn_800996AC/fn_800996E8/fn_80099724/fn_800997AC are three `ResShp` sub-resource lookups built from
 * the `ResShp` data's own offsets; fn_80099488-fn_80099640 and fn_80099580-fn_80099740 are the
 * out-of-line copies of the two classes' `Ptr`/`ref`/`IsValid` accessors, each with the assert the
 * `_ac.h` header it was written in passes (so fn_80099514 and fn_80099740 are two copies of the same
 * `ResTagDL::ref`).
 *
 * Measurement.  `python tools/units/recompile.py g3d/g3d_resshp.cpp --measure <symbol>` against MAIN's
 * retired per-symbol `auto_*_text.o` targets (MAIN has no split object for this unit until the
 * registration lands).  20 of 21 bodies are byte-identical (100.0 %); fn_80099724 is 98.57 % (28 B both,
 * instruction-identical modulo one register name: the base word loads into r3 where the target uses r0).
 * The object's sections equal the claim exactly: `.text` 0x3E0, `extab` 0x58 (11 records), `extabindex`
 * 0x84 (11 entries).
 *
 * Two codegen levers, both file-scoped:
 * - `#pragma peephole off` - with it the `rlwinm` flag extracts keep the target's explicit `cmpwi`
 *   instead of the folded record form (the same lever `g3d/g3d_resnode.cpp` records).
 * - the `.sdata` `"ref"` member-name strings must be declared as *sized* 4-byte arrays
 *   (`extern char lbl_807912B8[4];`) so MWCC's small-data heuristic emits the target's `li r7, @sda21`
 *   form instead of `lis`/`addi` (an unsized `extern const char[]` is assumed large); the same lever as
 *   `g3d/g3d_resfile.cpp`'s `lbl_80791290`.  Each takes fn_80099514/80099640/80099740 from 93.6 % to
 *   100.0 %.
 *
 * What the bodies are: fn_80099400/fn_8009943C, fn_800995A0/fn_800995D4/fn_80099638 and
 * fn_800996AC/fn_800996E8/fn_80099724/fn_800997AC are three `ResShp` sub-resource lookups built from
 * the `ResShp` data's own offsets; fn_80099488-fn_80099640 and fn_80099580-fn_80099740 are the
 * out-of-line copies of the two classes' `Ptr`/`ref`/`IsValid` accessors, each with the assert the
 * `_ac.h` header it was written in passes (so fn_80099514 and fn_80099740 are two copies of the same
 * `ResTagDL::ref`).
 *
 * Residual (best-scoring variant kept, per the matching policy): fn_80099724's register name (above).
 * The one naming caveat: the field names taken from an offset with no class-name evidence
 * (`ResTagDLData::mPrePrimOfs`) are best-effort; the offsets themselves are the facts.  `mPrePrimOfs` is
 * the +0x08 word of the `ResTagDL` block at `ResShpData::mTag`, resolved from the tag's own address.
 */
#include "types.h"
#include "nw4r/math.h"
#include "nw4r/g3d/res_common.h"

/* The target's `-O3` schedule is retail only with the peephole pass off: every flag extract keeps an
 * explicit `cmpwi` after the `rlwinm` instead of the folded record form `rlwinm.`. */
#pragma peephole off
#pragma fp_contract off

/* nw4r::db::Panic(const char*, int, const char*, ...) - the owner's real C++ declaration, called
 * through its signature, never the mangled spelling (docs/plan.md 6.5 rule 9). */
namespace nw4r {
namespace db {

void Panic(const char* pFile, int line, const char* pFmt, ...);

}  // namespace db
}  // namespace nw4r

/* The `ResTagDL` block embedded in the `ResShp` data at +0x18 (fn_80099740's assert names the class
 * through `.sdata` 0x80591794).  Only its +0x08 word is reached: the offset fn_800996E8 resolves from
 * the tag's own address. */
struct ResTagDLData {
    /* +0x00 */ u8 pad_0x00[0x8];
    /* +0x08 */ u32 mPrePrimOfs;
}; /* size: 0xC (a lower bound: only +0x08 is reached) */

/* The `ResShp` resource block `ResShp::ref` hands back.  +0x04 is the offset fn_80099400 turns into a
 * `ResMdl` handle (fn_8007B878's assert header is `g3d_resmdl_ac.h`); +0x18 is the embedded tag. */
struct ResShpData {
    /* +0x00 */ u8 pad_0x00[0x4];
    /* +0x04 */ u32 mToResMdlData;
    /* +0x08 */ u8 pad_0x08[0x18 - 0x8];
    /* +0x18 */ ResTagDLData mTag;
}; /* size: 0x24 (a lower bound: only the fields above are reached) */

/* The pooled constants this unit reads (all still unsplit in the map, so they have no registered
 * owner to move to). */
extern "C" {
extern const char lbl_805916E0[]; /* "NW4R:Failed assertion !((u32)p & 0x3)"   .data */
extern const char lbl_80591708[]; /* "g3d_resshp_ac.h"                        .data */
extern const char lbl_80591718[]; /* "ResShpPrePrim"                          .data */
extern const char lbl_80591728[]; /* "%s::%s: Object not valid."              .data */
extern const char lbl_80591748[]; /* "g3d_resshp_ac.h"                        .data */
extern const char lbl_80591758[]; /* "NW4R:Failed assertion !((u32)p & 0x3)"   .data */
extern const char lbl_80591780[]; /* "g3d_rescommon_ac.h"                     .data */
extern const char lbl_80591794[]; /* "ResTagDL"                               .data */
extern const char lbl_805917A0[]; /* "%s::%s: Object not valid."              .data */
extern const char lbl_805917BC[]; /* "g3d_rescommon_ac.h"                     .data */
extern const char lbl_805917D0[]; /* "%s::%s: Object not valid."              .data */
extern const char lbl_805917EC[]; /* "g3d_rescommon_ac.h"                     .data */
}

/* The `"ref"` member-name strings of the accessor asserts (unsplit `.sdata`).  Declared as *sized*
 * 4-byte arrays so MWCC's small-data heuristic emits the target's `li rN, @sda21` address form (an
 * unsized `extern const char[]` is assumed large and gets `lis`/`addi` - the same lever as
 * `g3d/g3d_resfile.cpp`'s `lbl_80791290`). */
extern char lbl_807912B8[4]; /* "ref" */
extern char lbl_807912BC[4]; /* "ref" */
extern char lbl_807912C0[4]; /* "ref" */

/* The neighbours this unit calls; the registered owner is named beside each.  The prototypes sit in
 * this `extern "C"` block (the shape g3d/fn_8005AA28.cpp uses) so the rule-2 conformance pass can lift
 * them into the owners' headers. */
extern "C" {
ResShpData* fn_80077674(const ResHandle* pSelf);         /* owner: src/g3d/fn_80075DCC.cpp */
ResShpData* fn_80077398(const ResHandle* pSelf);         /* owner: src/g3d/fn_80075DCC.cpp */
void* fn_8007B878(void* pOut, u32 value);                /* owner: src/g3d/fn_80075DCC.cpp */
}

/* The unit's own functions, in address order (the forward declarations keep the source order free).
 * They are C linkage: the map spells every one `fn_XXXXXXXX`. */
extern "C" {

u32 fn_80099400(ResHandle* pSelf);
u32 fn_8009943C(ResHandle* pSelf, u32 ofs);
u32 fn_80099488(void);
s32 fn_80099494(const ResHandle* pSelf);
ResHandle* fn_800994A8(ResHandle* pSelf, u32 value);
void fn_8009950C(ResHandle* pSelf, u32 value);
ResTagDLData* fn_80099514(ResHandle* pSelf);
ResTagDLData* fn_80099578(const ResHandle* pSelf);
u32 fn_80099580(void);
s32 fn_8009958C(const ResHandle* pSelf);
u32 fn_800995A0(ResHandle* pSelf);
ResHandle* fn_800995D4(ResHandle* pSelf, u32 value);
void fn_80099638(ResHandle* pSelf, u32 value);
u32 fn_80099640(ResHandle* pSelf);
u32 fn_800996A4(const ResHandle* pSelf);
u32 fn_800996AC(ResHandle* pSelf);
u32 fn_800996E8(ResHandle* pSelf);
u32 fn_80099724(const ResHandle* pSelf, u32 ofs);
ResTagDLData* fn_80099740(ResHandle* pSelf);
ResTagDLData* fn_800997A4(const ResHandle* pSelf);
u32 fn_800997AC(ResHandle* pSelf);

/* -------------------------------------------------------------------------------------------------- */

/* The `ResShp` body before the range's ResShpPrePrim cluster: build the model handle from the shape's
 * +0x04 offset. */
u32 fn_80099400(ResHandle* pSelf) {
    ResShpData* pData = fn_80077674(pSelf);
    return fn_8009943C(pSelf, pData->mToResMdlData);
}

/* Box `ResShp::ref + ofs` (0 for a null offset) and return the boxed word. */
u32 fn_8009943C(ResHandle* pSelf, u32 ofs) {
    u32 base = (u32)pSelf->mpData;
    if (ofs != 0) {
        u32 present;
        return *(u32*)fn_8007B878(&present, base + ofs);
    }
    u32 absent;
    return *(u32*)fn_8007B878(&absent, 0);
}

/* The class-name string the `%s::%s: Object not valid.` assert passes for ResShpPrePrim. */
u32 fn_80099488(void) {
    return (u32)lbl_80591718;
}

/* ResShpPrePrim's validity word. */
s32 fn_80099494(const ResHandle* pSelf) {
    return (u32)pSelf->mpData != 0;
}

/* Box a ResShpPrePrim pointer and assert it is 4-byte aligned (the `g3d_resshp_ac.h` `Ptr` setter). */
ResHandle* fn_800994A8(ResHandle* pSelf, u32 value) {
    fn_8009950C(pSelf, value);
    if ((value & 0x3) != 0) {
        nw4r::db::Panic(lbl_80591708, 0x2C, lbl_805916E0);
    }
    return pSelf;
}

/* The raw handle store the Ptr setter expands to. */
void fn_8009950C(ResHandle* pSelf, u32 value) {
    pSelf->mpData = (void*)value;
}

/* `ResTagDL::ref` with the `g3d_rescommon_ac.h` assert. */
ResTagDLData* fn_80099514(ResHandle* pSelf) {
    if (fn_8009958C(pSelf) == 0) {
        nw4r::db::Panic(lbl_805917EC, 0x151, lbl_805917D0, (const char*)fn_80099580(), lbl_807912B8);
    }
    return fn_80099578(pSelf);
}

/* The raw ref getter. */
ResTagDLData* fn_80099578(const ResHandle* pSelf) {
    return (ResTagDLData*)pSelf->mpData;
}

/* The class-name string the `%s::%s: Object not valid.` assert passes for ResTagDL. */
u32 fn_80099580(void) {
    return (u32)lbl_80591794;
}

/* ResTagDL's validity word. */
s32 fn_8009958C(const ResHandle* pSelf) {
    return (u32)pSelf->mpData != 0;
}

/* Box the ResShp data's embedded tag (its +0x18 block). */
u32 fn_800995A0(ResHandle* pSelf) {
    ResShpData* pData = fn_80077674(pSelf);
    ResHandle tag;
    return (u32)fn_800995D4(&tag, (u32)&pData->mTag)->mpData;
}

/* Box a pointer and assert it is 4-byte aligned (the `g3d_rescommon_ac.h` `Ptr` setter). */
ResHandle* fn_800995D4(ResHandle* pSelf, u32 value) {
    fn_80099638(pSelf, value);
    if ((value & 0x3) != 0) {
        nw4r::db::Panic(lbl_80591780, 0x151, lbl_80591758);
    }
    return pSelf;
}

/* The raw handle store the ResTagDL Ptr setter expands to. */
void fn_80099638(ResHandle* pSelf, u32 value) {
    pSelf->mpData = (void*)value;
}

/* Assert ResShpPrePrim validity, then hand back the pointer (the `g3d_resshp_ac.h` ref form). */
u32 fn_80099640(ResHandle* pSelf) {
    if (fn_80099494(pSelf) == 0) {
        nw4r::db::Panic(lbl_80591748, 0x2C, lbl_80591728, (const char*)fn_80099488(), lbl_807912C0);
    }
    return fn_800996A4(pSelf);
}

/* The raw pre-prim-array pointer getter. */
u32 fn_800996A4(const ResHandle* pSelf) {
    return (u32)pSelf->mpData;
}

/* Resolve the shape's pre-prim array: the tag's +0x08 offset, from the tag's own address. */
u32 fn_800996AC(ResHandle* pSelf) {
    ResHandle array;
    ResHandle tag;
    tag.mpData = (void*)fn_800997AC(pSelf);
    return (u32)fn_800994A8(&array, fn_800996E8(&tag))->mpData;
}

/* The tag's stored offset, resolved from the tag itself. */
u32 fn_800996E8(ResHandle* pSelf) {
    ResTagDLData* pTag = fn_80099740(pSelf);
    return fn_80099724(pSelf, pTag->mPrePrimOfs);
}

/* `base + ofs` for a boxed base, 0 for a null offset. */
u32 fn_80099724(const ResHandle* pSelf, u32 ofs) {
    u32 base = (u32)pSelf->mpData;
    if (ofs) {
        base += ofs;
        return base;
    }
    return 0;
}

/* `ResTagDL::ref` with the `g3d_rescommon_ac.h` assert (the second out-of-line copy). */
ResTagDLData* fn_80099740(ResHandle* pSelf) {
    if (fn_8009958C(pSelf) == 0) {
        nw4r::db::Panic(lbl_805917BC, 0x151, lbl_805917A0, (const char*)fn_80099580(), lbl_807912BC);
    }
    return fn_800997A4(pSelf);
}

/* The raw ref getter (the second out-of-line copy). */
ResTagDLData* fn_800997A4(const ResHandle* pSelf) {
    return (ResTagDLData*)pSelf->mpData;
}

/* Box the ResShp data's embedded tag through the other `ResShp::ref` copy. */
u32 fn_800997AC(ResHandle* pSelf) {
    ResShpData* pData = fn_80077398(pSelf);
    ResHandle tag;
    return (u32)fn_800995D4(&tag, (u32)&pData->mTag)->mpData;
}

}  /* extern "C" */
