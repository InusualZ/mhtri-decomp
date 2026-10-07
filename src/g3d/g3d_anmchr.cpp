/*
 * g3d/g3d_anmchr.cpp - nw4r g3d character animation (`g3d_anmchr.cpp`), led by the g3d work-memory accessors,
 *   with small constant/forwarder helpers.
 * RANGE. .text 0x8005CED0-0x80063E60 (149 functions); extab 0x800073C8-0x8000776C, extabindex
 *   0x8001F71C-0x8001FBFC, .rodata 0x8056F500-0x8056F550 (the "AnmObjChr", "AnmObjChrNode", "AnmObjChrBlend" and "AnmObjChrRes" name records, read only here), .data 0x8058B410-0x8058C118, .bss 0x8066AE80-0x80682E80, .sdata 0x80791138-0x80791158,
 *   .sdata2 0x80795D48-0x80795D68.  The asserts from 0x8005CF10 on pass "g3d_anmchr.cpp" (.data 0x8058B410).
 * RANGE. Left edge 0x8005CED0: the font/debug-print console before it is `font/flfnt.cpp` (its header has the
 *   seam evidence).  fn_8005CED0-fn_8005CF04 return the 0x18000-byte `.bss` buffer 0x8066AE80-0x80682E80, read
 *   only by g3d calc units; they are nw4r g3d work memory and may be a TU of their own (unproven).  Right edge
 *   0x80063E60: the five vtables (.data 0x8058BCA8-0x8058BE24, inside this claim) point at 13 functions of
 *   0x80063888-0x80063E60 (the weak type-info members MWCC emits at the end of the TU that defines the vtables),
 *   the other 8 of the run are called only from inside it or from other g3d units (fn_80063964, fn_80063AC4), and
 *   0x80063E60 onward is called by g3d_calcmaterial, g3d_scnmdl and g3d_resfile; the extabindex records of the
 *   run end at 0x8001FBFC, the last record (0x80063E30) with its extab 0x80007764-0x8000776C.
 * NAMES. The map's own names and stems, the stems defined `extern "C"`.  The members of G3dObj, G3dObj::TypeObj,
 *   AnmObj, AnmObjChr, AnmObjChrNode, AnmObjChrBlend and AnmObjChrRes are nw4r's: the assert and warning strings name
 *   the classes and methods (`AnmObjChr::Attach(%d, %p(%s))`, `AnmObjChrNode::G3dProc(...)`), the five vtables give
 *   the slot order, and the map rows carry the compiler's manglings.
 *   GUESS: `type_obj_set_name` (0x800638B8: stores a type-name record pointer through `out`, the weak type-info
 *   helper) and `anm_typename_AnmObj`/`_AnmObjChrNode`/`_AnmObjChrBlend`/`_G3dObj` (the `.rodata` records by their strings).
 *   GUESS: `Alloc__Q34nw4r3g3d6G3dObjFP12MEMAllocatorUl` (0x800604D4, `b` to the next word: G3dObj's allocation entry the
 *   callers pass (heap, size) to) and `g3d_obj_alloc_tail` (0x800604D8, the 4-byte local that tail-calls
 *   MEMAllocFromAllocator); the map row was one 8-byte `fn_800604D4` before.  g3d_round_up (0x800604DC) is a GUESS
 *   (the Construct functions' align-up of an offset: nw4r's `ut::RoundUp`); G3dObj's placement operator new
 *   (0x800604CC) is the member the Construct functions build their objects with.  g3d_obj_alloc_tail is a GUESS
 *   (the allocation forwarder above); math_reciprocal is a GUESS (0x800610AC: one Newton-Raphson step on `fres`,
 *   nw4r's `math::FInv`).
 *   ResAnmChr (0x80061898..0x80061D08: GetNumNode, ref, ptr, GetClassName, IsValid, GetAnmPolicy, GetNumFrame), the
 *   AnmObjChrRes constructor, Construct and its vtable members are nw4r's (the vtable at .data 0x8058BCA8 orders
 *   them; ResAnmChrData's field names are GUESSES).  res_anm_chr_copy_ctor is a GUESS; res_anm_chr_common_copy_ctor
 *   is a GUESS; anmchr_resanmchr_class_name is a GUESS; anmchr_resanmchr_ac_file is a GUESS; anmchr_ref_invalid_fmt
 *   is a GUESS; anmchr_ref_name is a GUESS (the strings ResAnmChr::ref passes to Panic).
 *   type_obj_set_name_anmchr is a GUESS (0x8005DCD0: the type-name store copy the AnmObjChr, AnmObjMatClr,
 *   AnmObjTexPat and ScnMdlSimple type members call).
 * RESIDUALS. 30 functions unwritten (objdiff scores them zero; `python tools/objdiff/unitscore.py g3d/g3d_anmchr`
 *   lists them), the largest GetResult__Q34nw4r3g3d14AnmObjChrBlendFPQ34nw4r3g3d12ChrAnmResultUl (0x8D0), the
 *   AnmObjChrRes members Bind(ResMdl, u32, BindOption) (0x384) and Release(ResMdl, u32, BindOption) (0x320).
 *   Partial: fn_8005D27C, g3d_round_up (retail materialises `~(align - 1)` with `nor` + `and` where MWCC folds it
 *   into `andc`), fn_8006244C, fn_80062824, fn_800628AC, and the members whose only difference is a relocation name
 *   (the strings).
 *   G3dObj::operator delete, AnmObj, AnmObjChr, AnmObjChrBlend, AnmObjChrRes: the empty bodies are complete (retail's
 *   operator delete is a bare `blr`; the compiler emits the constructors' base calls, vtable stores and member
 *   initialisers and the destructors' base calls).  The G3dObj vtable is emitted here (its first virtual,
 *   IsDerivedFrom, is defined here) while retail keeps it in `g3d/fn_80075DCC.cpp`'s `.data`.
 *   The AnmObj type-name record (0x8056F568, read only by AnmObj::GetTypeObj and GetTypeObjStatic here) sits in
 *   `g3d/fn_80063888.cpp`'s `.rodata` claim (0x8056F550-0x8056F578) beside the AnmObjMatClr record (0x8056F550, read
 *   only by that unit's fn_80064868): the two owners interleave there, which one seam cannot express.
 *   flipcheck: `.text` 0xB38 of 0x6F90; extab 0x110 of 0x3A4; extabindex 0x198 of 0x4E0; `.rodata`, `.data`, `.bss`, `.sdata` and `.sdata2` are claimed and not emitted.
 *   Relocation names that differ from retail (pool constants, save helpers, statics): `lbl_80791148`,
 *     `lbl_807911E8`, `lbl_80791168`, `lbl_80791150`.
 * SHAPES. The ResName constructor is complete: its work is the base initialiser.  The unit includes the leaf
 *   `g3d/anm_typename_AnmObj.h` instead of `g3d/fn_80063888.h`: a visible global placement `operator delete` gives
 *   AnmObjChrBlend::Construct a landing pad (`__dl__FPvPv`) retail does not have.
 *   The unit compiles with `#pragma peephole off` throughout (retail keeps the unfused `clrlwi` + `cmpwi`, `extsh`,
 *   `addi r0` vtable-store and `mr r3` + `lwz r12,0(r3)` virtual-call forms; playbook idea 106).
 */

#include "types.h"
#include "g3d/anm_typename_AnmObj.h" /* anm_typename_AnmObj, owned by g3d/fn_80063888.cpp (leaf header, rule 2) */
#include "g3d/fn_80075DCC.h" /* anm_typename_G3dObj, owned by g3d/fn_80075DCC.cpp (rule 2) */
#include "g3d/g3d_anmchr.h" /* this unit's own declarations, and the G3dObj dispatch record (rule 1) */
#include "g3d/g3d_resmat.h" /* nw4r::g3d::ResMdl (rule 2) */
#include "g3d/g3d_resnode.h" /* nw4r::g3d::ResNode (rule 2) */
#include "fn_8004CAD8.h"   /* mtx34_const_ptr's owner header (docs/plan.md 6.5, rule 2) */
#include "nw4r/math_arithmetic.h"   /* nw4r::math::detail::FExp, owner nw4r/math_arithmetic.cpp (rule 2) */
#include "nw4r/db_assert.h"  /* nw4r::db::Warning (rule 2) */
#include "MSL_C/alloc.h"     /* __fpclassifyf, owner MSL_C/alloc.cpp (rule 2) */
#include "OS/MEMAllocFromAllocator.h" /* MEMAllocFromAllocator, owner OS/FindContainHeap_.c (rule 2) */

#pragma peephole off

#pragma pool_data off


/* --- the SDK entry points this unit tail-calls (owners are unsplit; declared, never defined) --------- */

extern "C" void fn_804C6F40(u32, u32, void*);
extern "C" void fn_804C6D70(void*, u32);
extern "C" void fn_80061EC4(void*, f32);

/* The g3d math constant fn_8006244C scales by (`.sdata` 0x807911E8). */
extern f32 lbl_807911E8;

/* The `.sdata` strings fn_8005D27C / fn_800628AC hand back (`_SDA_BASE_`-relative), and the two
 * absolute constants the blocked-return helpers hand back (`.bss` / `.data`). */
extern u8 lbl_80791148[];
extern u8 lbl_80791150[];
extern u8 lbl_8066AE80[];
extern u8 anmchr_resanmchr_class_name[];    /* "ResAnmChr" */
extern const char anmchr_resanmchr_ac_file[]; /* "g3d_resanmchr_ac.h" */
extern const char anmchr_ref_invalid_fmt[];   /* "%s::%s: Object not valid." */
extern const char anmchr_ref_name[4];         /* "ref" (sized: an SDA21 address) */
extern "C" void res_anm_chr_common_copy_ctor(nw4r::g3d::ResAnmChr* pDst, const nw4r::g3d::ResAnmChr* pSrc);
extern const char anmchr_resnode_align_assert_msg[]; /* the ResNode alignment assertion */
extern const char anmchr_resnode_ac_file[]; /* "g3d_resnode_ac.h" */
extern const char anmchr_resdic_idx_bounds_msg[]; /* the ResDic index assertion */
extern const char anmchr_resdic_ac_file[]; /* "g3d_resdict.h" */
extern const char anmchr_resdic_align_assert_msg[]; /* the ResDic alignment assertion */
extern const char anmchr_resdic_ac_file_ctor[]; /* "g3d_resdict.h" */
extern u8 lbl_8056F500[];
extern u8 lbl_8056F538[];

/* --- bodies: the g3d work-memory accessors and tiny constant/identity helpers ---------------------- */

extern "C" u8* fn_8005CED0(void)
{
    return lbl_8066AE80;
}

extern "C" u8* fn_8005CEDC(void)
{
    return lbl_8066AE80 + 0x6000;
}

extern "C" u8* fn_8005CEEC(void)
{
    return lbl_8066AE80;
}

extern "C" u8* fn_8005CEF8(void)
{
    return lbl_8066AE80;
}

extern "C" u8* fn_8005CF04(void)
{
    return lbl_8066AE80;
}

extern "C" u32* fn_8005D0C4(u32** pp)
{
    return *pp;
}

extern "C" void fn_8005D114(u32* a, u32* b)
{
    (void)a;
    (void)b;
}

extern "C" void fn_8005D118(u32* dst, u32* src)
{
    *dst = *src;
}

/* 0x8005D210 (0x8): stores the block address. */
NW4R_G3D_RESCOMMON_CTOR(ResNodeData)

/* 0x8005D1AC (0x64): wraps `pData`, asserting its alignment. */
/* untyped: opaque handle - the node block */
nw4r::g3d::ResNode::ResNode(void* pData) : ResCommon<ResNodeData>(pData) {
    if ((u32)pData & 0x3) {
        nw4r::db::Panic((const char*)anmchr_resnode_ac_file, 44, (const char*)anmchr_resnode_align_assert_msg);
    }
}

extern "C" u8* fn_8005D27C(void)
{
    return lbl_80791148;
}

extern "C" void fn_8005D2F0(u32* dst, u32* src)
{
    *dst = *src;
}

/* 0x8005D2FC (0x14): tells whether the handle is set. */
bool nw4r::g3d::ResMdl::IsValid() const
{
    return mpData != NULL;
}

/* 0x8005D3E0 (0x4): releases nothing - the objects are freed through their heap. */
void nw4r::g3d::G3dObj::operator delete(void* pBlock) /* untyped: byte range */
{
}

/* Returns the type's name (the record's string, after its length word). */
const char* nw4r::g3d::G3dObj::TypeObj::GetTypeName() const
{
    return mName->str;
}

extern "C" void **fn_8005DC60(void **out, void *v)
{
    *out = v;
    return out;
}

extern "C" void **type_obj_set_name_anmchr(void **out, void *v)
{
    *out = v;
    return out;
}

/* The two ``current chunk`` getters: stash a fixed descriptor address in a local through the setter,
 * then hand the local back (the setter is opaque, so the reload is real). */


/* The two-word copy pair the character-animation constructor uses. */
extern "C" void* fn_8005D0CC(u32* dst, u32* src)
{
    fn_8005D118(dst, src);
    fn_8005D114(dst + 1, src + 1);
    return dst;
}

/* 0x800600B8 (0x8): records the parent. */
void nw4r::g3d::G3dObj::SetParent(G3dObj* pParent)
{
    mpParent = pParent;
}

/* 0x800600C0 (0x8): returns the parent. */
nw4r::g3d::G3dObj* nw4r::g3d::G3dObj::GetParent() const
{
    return mpParent;
}

/* 0x800604CC (0x8): the placement form: builds the object in the caller's block. */
/* untyped: byte range - the placement address the Construct functions build the object in */
void* nw4r::g3d::G3dObj::operator new(unsigned long size, void* pBlock)
{
    (void)size;
    return pBlock;
}

extern "C" void fn_80060FEC(void)
{
}

/* 0x80061898 (0x24): the animated node count. */
u32 nw4r::g3d::ResAnmChr::GetNumNode() const
{
    return ref().numNode;
}

/* 0x800618BC (0x64): the resource block, asserting the handle is valid. */
const nw4r::g3d::ResAnmChrData& nw4r::g3d::ResAnmChr::ref() const
{
    if (!IsValid()) {
        nw4r::db::Panic(anmchr_resanmchr_ac_file, 39, anmchr_ref_invalid_fmt, GetClassName(), anmchr_ref_name);
    }
    return *ptr();
}

/* 0x80061920 (0x8): the resource block, unchecked. */
const nw4r::g3d::ResAnmChrData* nw4r::g3d::ResAnmChr::ptr() const
{
    return mpData;
}

/* 0x80061928 (0xC): the class name the asserts print. */
const char* nw4r::g3d::ResAnmChr::GetClassName()
{
    return (const char*)anmchr_resanmchr_class_name;
}

/* 0x80061B30 (0xC): copies the base handle's data pointer. */
extern "C" void res_anm_chr_common_copy_ctor(nw4r::g3d::ResAnmChr* pDst, const nw4r::g3d::ResAnmChr* pSrc)
{
    pDst->mpData = pSrc->mpData;
}

extern "C" f32 fn_80062024(f32* p)
{
    return *p;
}

extern "C" void fn_800621DC(f32* p, f32 v)
{
    p[1] = v;
}

extern "C" f32 fn_80062300(f32* p)
{
    return p[1];
}

extern "C" {
/* untyped: byte range - the allocated block */
static void* g3d_obj_alloc_tail(MEMAllocator* pHeap, u32 size);
}

/* 0x800604D4 (0x4): allocates `size` bytes from the object heap. */
/* untyped: byte range - the allocated block */
void* nw4r::g3d::G3dObj::Alloc(MEMAllocator* pHeap, u32 size)
{
    return g3d_obj_alloc_tail(pHeap, size);
}

extern "C" {
/* 0x800604D8 (0x4): forwards the allocation to the allocator. */
/* untyped: byte range - the allocated block */
static void* g3d_obj_alloc_tail(MEMAllocator* pHeap, u32 size)
{
    return MEMAllocFromAllocator(pHeap, size);
}

/* 0x800610AC (0x18): 1 / value by one Newton-Raphson step from the hardware estimate. */
asm f32 math_reciprocal(register f32 value)
{
    nofralloc
    fres f0, value
    ps_add f2, f0, f0
    ps_mul f0, f0, f0
    ps_nmsub f0, value, f0, f2
    fmr f1, f0
    blr
}
}

/* 0x800626BC (0x8): stores the block address. */
NW4R_G3D_RESCOMMON_CTOR(nw4r::g3d::ResNameData)

/* untyped: opaque handle - the dictionary block */
extern "C" void* fn_800628A4(void* self)
{
    return *(void**)self;
}

extern "C" u8* fn_800628AC(void)
{
    return lbl_80791150;
}

/* 0x80062978 (0x8): stores the block address. */
NW4R_G3D_RESCOMMON_CTOR(nw4r::g3d::ResDicData)

extern "C" void fn_80062D88(u32* dst, u32* src)
{
    *dst = *src;
}

/* --- assignment helpers that return the object (nw4r's fluent setters) ------------------------------ */

/* 0x8006268C (0x30): wraps `pData`. */
/* untyped: opaque handle - the name block */
nw4r::g3d::ResName::ResName(void* pData) : ResCommon<ResNameData>(pData) {}

/* 0x80061B00 (0x30): copy-constructs a character-animation handle and returns the destination. */
extern "C" nw4r::g3d::ResAnmChr* res_anm_chr_copy_ctor(nw4r::g3d::ResAnmChr* pDst, const nw4r::g3d::ResAnmChr* pSrc)
{
    res_anm_chr_common_copy_ctor(pDst, pSrc);
    return pDst;
}

extern "C" void* fn_80062D58(void* self, u32* src)
{
    fn_80062D88((u32*)self, src);
    return self;
}

extern "C" void* fn_8005D2C0(void* self, u32* src)
{
    fn_8005D2F0((u32*)self, src);
    return self;
}

/* --- more one-instruction accessors ----------------------------------------------------------------- */

/* 0x80061934 (0x14): whether the handle names a block. */
bool nw4r::g3d::ResAnmChr::IsValid() const
{
    return mpData != NULL;
}

/* 0x80061CE4 (0x24): the play policy. */
nw4r::g3d::AnmPolicy nw4r::g3d::ResAnmChr::GetAnmPolicy() const
{
    return ref().policy;
}

/* 0x80061D08 (0x24): the frame count. */
int nw4r::g3d::ResAnmChr::GetNumFrame() const
{
    return ref().numFrame;
}


/* untyped: opaque handle - the dictionary block */
extern "C" u32 fn_800628B4(void* self)
{
    return *(u32*)self != 0;
}

/* Returns the word at +0 plus `offset`, or 0 when `offset` is 0 (nw4r's null-safe offset helper). */
extern "C" u32 fn_80062824(u32* p, u32 offset)
{
    u32 base = *p;

    if (offset != 0) {
        return base + offset;
    }
    return 0;
}

/* The align-up helper: (~(align - 1)) & (offset + align - 1). */
extern "C" u32 g3d_round_up(u32 offset, u32 align)
{
    return ~(align - 1) & (align + offset - 1);
}

/* nw4r's four-float vector setter. */
extern "C" void dVector4Set(f32* dst, f32 x, f32 y, f32 z, f32 w)
{
    dst[0] = x;
    dst[1] = y;
    dst[2] = z;
    dst[3] = w;
}

/* --- forwarders into the SDK allocator / matrix helpers --------------------------------------------- */

extern "C" void* fn_80060F70(void* self, u32 a, u32 b)
{
    fn_804C6F40(a, b, self);
    return self;
}

extern "C" void* fn_80060FAC(void* self, u32 a)
{
    fn_804C6D70(self, mtx34_const_ptr(a));
    return self;
}

/* The animated-float advance: offset[0] += offset[1] * scale, then the virtual step. */
extern "C" void fn_8006244C(f32* p)
{
    f32 scale = lbl_807911E8;
    f32 value = p[1] * scale;

    fn_80061EC4(p, p[0] + value);
}

/* --- nw4r's ``fn_800626F4``-returns-this pair ------------------------------------------------------- */

extern "C" u32 fn_800626F4(u32 value)
{
    return value;
}

extern "C" u32 fn_800626C4(u32 a, u32 b)
{
    return b + fn_800626F4(a);
}

/* --- forwarders into nw4r math ---------------------------------------------------------------------- */

/* The exp forwarder (nw4r's inline `math::FExp`): a tail call with the argument untouched. */
extern "C" f32 fn_80060F6C(f32 x)
{
    return nw4r::math::detail::FExp(x);
}

/* 0x80062914 (0x64): wraps `pData`, asserting its alignment. */
/* untyped: opaque handle - the dictionary block */
nw4r::g3d::ResDic::ResDic(void* pData) : ResCommon<ResDicData>(pData) {
    if ((u32)pData & 0x3) {
        nw4r::db::Panic((const char*)anmchr_resdic_ac_file_ctor, 84, (const char*)anmchr_resdic_align_assert_msg);
    }
}

/* 0x80062750 (0xD4): returns the `idx`-th entry's data, asserting the index is in range. */
/* untyped: opaque handle - the entry's data block */
void* nw4r::g3d::ResDic::operator[](int idx) const {
    if (fn_800628B4((u32*)this)) {
        bool inRange = false;
        if (idx >= 0 && idx <= (s32)ref().numData - 1) {
            inRange = true;
        }
        if (!inRange) {
            nw4r::db::Panic((const char*)anmchr_resdic_ac_file, 42, (const char*)anmchr_resdic_idx_bounds_msg, idx, 0,
                            ref().numData - 1);
        }
        return (void*)fn_80062824((u32*)this, ref().entry[idx + 1].ofsData);
    }
    return NULL;
}

/* --------------------------------------------------------------------------------------------- *
 * The pointer-holder accessors (0x80063888-0x80063E60): a local pointer is filled in through one of
 * the two store helpers and the destination address is returned.
 * --------------------------------------------------------------------------------------------- */

/* Stores `v` through `out` and hands `out` back so the caller keeps its address in r3. */
extern "C" const u8 **type_obj_set_name(const u8 **out, const u8 *v)
{
    *out = v;
    return out;
}


extern "C" void **fn_80063B24(void **out, void *v)
{
    *out = v;
    return out;
}


extern "C" void **fn_80063C8C(void **out, void *v)
{
    *out = v;
    return out;
}


/* --------------------------------------------------------------------------------------------- *
 * The vtable-dispatch wrappers (0x800638C0/0x80063B2C/0x80063C94): the object's word 0 is its vtable,
 * the entry at +0x14 runs with the object as its argument and a word is read back through the
 * `TypeObj::GetTypeName` helper.
 * --------------------------------------------------------------------------------------------- */


/* The two equality readers: compare the words reached through each argument. */


/* The five list-insert steps (0x800638F8-0x80063DC4): resolve a type name, compare the caller's key
 * against it through fn_800639D0, and on a miss run the previous step's insertion with a copy of the
 * key word.  They chain in address order. */


/* The two deleting destructors (0x80063C00/0x80063D68): clear the base, and free only when the signed
 * flag is positive; the object is handed back either way. */


/* ------------------------------------------------------------------------------------------------ */
/* G3dObj, AnmObj and the AnmObjChr base                                                              */
/* ------------------------------------------------------------------------------------------------ */

/* The nw4r resource pointer assert: `ptr` must fall in one of the seven mapped Wii memory ranges. */
#define ANMCHR_POINTER_ASSERT(file, ptr, line, msg)                                            \
    {                                                                                          \
        BOOL ok1_ = TRUE, ok2_ = TRUE, ok3_ = TRUE, ok4_ = TRUE, ok5_ = TRUE, ok6_ = TRUE;      \
        u32 top_ = (u32)(ptr) & 0xFF000000u;                                                    \
        if (!(top_ == 0x80000000u) && !(((u32)(ptr) & 0xFF800000u) == 0x81000000u))             \
            ok6_ = FALSE;                                                                        \
        if (!ok6_ && !(((u32)(ptr) & 0xF8000000u) == 0x90000000u))                              \
            ok5_ = FALSE;                                                                        \
        if (!ok5_ && !(top_ == 0xC0000000u))                                                    \
            ok4_ = FALSE;                                                                        \
        if (!ok4_ && !(((u32)(ptr) & 0xFF800000u) == 0xC1000000u))                              \
            ok3_ = FALSE;                                                                        \
        if (!ok3_ && !(((u32)(ptr) & 0xF8000000u) == 0xD0000000u))                              \
            ok2_ = FALSE;                                                                        \
        if (!ok2_ && !(((u32)(ptr) & 0xFFFFC000u) == 0xE0000000u))                              \
            ok1_ = FALSE;                                                                        \
        if (!ok1_)                                                                              \
            nw4r::db::Panic(file, line, msg, (ptr));                                             \
    }

extern "C" u32 g3d_round_up(u32 offset, u32 align);

/* 0x80061948 (0x1B8): builds a resource-played character animation over `res`, with an optional result cache. */
nw4r::g3d::AnmObjChrRes::AnmObjChrRes(MEMAllocator* pHeap, ResAnmChr res, u16* pBindingBuf, int numBinding,
                                      ChrAnmResult* pCacheBuf)
    : AnmObjChr(pHeap, pBindingBuf, numBinding), FrameCtrl(0.0f, res.GetNumFrame(), GetAnmPlayPolicy(res.GetAnmPolicy()))
{
    res_anm_chr_copy_ctor(&mRes, &res);
    mpResultCache = pCacheBuf;
    ANMCHR_POINTER_ASSERT("g3d_anmchr.cpp", this, 0x40F, "NW4R:Pointer Error\nthis(=%p) is not valid pointer.");
    if (mpResultCache) {
        UpdateCache();
    }
}

/* 0x80061424 (0x474): sizes a resource-played animation of `res` for `mdl` (with a per-node result cache when
 * `bHasCache`), reports the size through `pSize`, and builds it in one block from `pHeap`. */
nw4r::g3d::AnmObjChrRes* nw4r::g3d::AnmObjChrRes::Construct(MEMAllocator* pHeap, u32* pSize, ResAnmChr res,
                                                            ResMdl mdl, bool bHasCache)
{
    if (!res.IsValid() || !mdl.IsValid()) {
        return NULL;
    }
    int numNode = res.GetNumNode();
    int numBinding = mdl.GetResNodeNumEntries();
    u32 cacheSize = (bHasCache ? numNode : 0) * 0x4C; /* sizeof(ChrAnmResult) */
    u32 size = sizeof(AnmObjChrRes) + cacheSize + numBinding * sizeof(u16);
    if (pSize != NULL) {
        ANMCHR_POINTER_ASSERT("g3d_anmchr.cpp", pSize, 0x3DB, "NW4R:Pointer Error\npSize(=%p) is not valid pointer.");
        *pSize = size;
    }
    if (pHeap == NULL) {
        return NULL;
    }
    ANMCHR_POINTER_ASSERT("g3d_anmchr.cpp", pHeap, 0x3E4, "NW4R:Pointer Error\npHeap(=%p) is not valid pointer.");
    u8* pBuf = (u8*)Alloc(pHeap, size);
    if (pBuf == NULL) {
        return NULL;
    }
    ANMCHR_POINTER_ASSERT("g3d_anmchr.cpp", pBuf, 0x3EE, "NW4R:Pointer Error\nbuf(=%p) is not valid pointer.");
    ChrAnmResult* pCacheBuf = bHasCache ? (ChrAnmResult*)(pBuf + sizeof(AnmObjChrRes)) : NULL;
    u16* pBindingBuf = (u16*)(pBuf + sizeof(AnmObjChrRes) + cacheSize);
    if ((u32)pBuf & 3) {
        nw4r::db::Panic("g3d_anmchr.cpp", 0x3F2, "NW4R:Alignment Error(0x%x)\nbuf must be aligned to 4 bytes boundary.",
                        pBuf);
    }
    if ((u32)pCacheBuf & 3) {
        nw4r::db::Panic("g3d_anmchr.cpp", 0x3F3,
                        "NW4R:Alignment Error(0x%x)\npCacheBuf must be aligned to 4 bytes boundary.", pCacheBuf);
    }
    if ((u32)pBindingBuf & 1) {
        nw4r::db::Panic("g3d_anmchr.cpp", 0x3F4,
                        "NW4R:Alignment Error(0x%x)\npBindBuf must be aligned to 2 bytes boundary.", pBindingBuf);
    }
    return new (pBuf) AnmObjChrRes(pHeap, res, pBindingBuf, numBinding, pCacheBuf);
}

/* 0x800600C8 (0x404): sizes a blend node for `mdl` with `numChildren` weighted children, reports the size through
 * `pSize`, and builds it in one block from `pHeap` (NULL without a heap or memory). */
nw4r::g3d::AnmObjChrBlend* nw4r::g3d::AnmObjChrBlend::Construct(MEMAllocator* pHeap, u32* pSize, ResMdl mdl,
                                                                int numChildren)
{
    if (!mdl.IsValid()) {
        return NULL;
    }
    int numNode = mdl.GetResNodeNumEntries();
    u32 bindingOffset = g3d_round_up(sizeof(AnmObjChrBlend), 2);
    u32 childrenOffset = g3d_round_up(bindingOffset + numNode * sizeof(u16), 4);
    u32 childrenSize = numChildren * sizeof(AnmObjChrRes*);
    u32 weightOffset = g3d_round_up(childrenOffset + childrenSize, 4);
    u32 size = g3d_round_up(weightOffset + childrenSize, 4);
    if (pSize != NULL) {
        ANMCHR_POINTER_ASSERT("g3d_anmchr.cpp", pSize, 0x267, "NW4R:Pointer Error\npSize(=%p) is not valid pointer.");
        *pSize = size;
    }
    if (pHeap == NULL) {
        return NULL;
    }
    ANMCHR_POINTER_ASSERT("g3d_anmchr.cpp", pHeap, 0x270, "NW4R:Pointer Error\npHeap(=%p) is not valid pointer.");
    u8* pBuf = (u8*)Alloc(pHeap, size);
    if (pBuf == NULL) {
        return NULL;
    }
    ANMCHR_POINTER_ASSERT("g3d_anmchr.cpp", pBuf, 0x27A, "NW4R:Pointer Error\nbuf(=%p) is not valid pointer.");
    if ((u32)pBuf & 3) {
        nw4r::db::Panic("g3d_anmchr.cpp", 0x27B, "NW4R:Alignment Error(0x%x)\nbuf must be aligned to 4 bytes boundary.",
                        pBuf);
    }
    return new (pBuf) AnmObjChrBlend(pHeap, (u16*)(pBuf + bindingOffset), numNode,
                                     (AnmObjChrRes**)(pBuf + childrenOffset), numChildren,
                                     (f32*)(pBuf + weightOffset));
}


/* 0x8005D384 (0x5C): destroys the animation object. */
nw4r::g3d::AnmObj::~AnmObj()
{
}

/* 0x8005D3E4 (0x44): constructs the animation object with no flags set. */
nw4r::g3d::AnmObj::AnmObj(MEMAllocator* pHeap, G3dObj* pParent) : G3dObj(pHeap, pParent), mFlags(0)
{
}

/* 0x8005D428 (0x12C): records the parent and the heap the object was allocated from. */
nw4r::g3d::G3dObj::G3dObj(MEMAllocator* pHeap, G3dObj* pParent) : mpParent(pParent), mpHeap(pHeap)
{
    ANMCHR_POINTER_ASSERT("g3d_obj.h", pHeap, 0x9C, "NW4R:Pointer Error\npHeap(=%p) is not valid pointer.");
}

/* 0x8005D310 (0x74): constructs the character animation over a binding buffer of `numBinding` words, all
 * released. */
nw4r::g3d::AnmObjChr::AnmObjChr(MEMAllocator* pHeap, u16* pBindingBuf, int numBinding)
    : AnmObj(pHeap, NULL), mNumBinding(numBinding), mpBinding(pBindingBuf)
{
    Release();
}

/* 0x8005D554 (0x168): whether node `idx` has an animation bound. */
bool nw4r::g3d::AnmObjChr::TestExistence(u32 idx) const
{
    ANMCHR_POINTER_ASSERT("g3d_anmchr.cpp", this, 0x5C, "NW4R:Pointer Error\nthis(=%p) is not valid pointer.");
    if (!((int)idx <= mNumBinding - 1)) {
        nw4r::db::Panic("g3d_anmchr.cpp", 0x5D, "int(nodeId) is out of bounds(%d)\nint(nodeId) <= %d not satisfied.",
                        idx, mNumBinding - 1);
    }
    return (mpBinding[idx] & 0xC000) == 0;
}

/* 0x8005D6BC (0x168): whether node `idx`'s binding is defined. */
bool nw4r::g3d::AnmObjChr::TestDefined(u32 idx) const
{
    ANMCHR_POINTER_ASSERT("g3d_anmchr.cpp", this, 0x65, "NW4R:Pointer Error\nthis(=%p) is not valid pointer.");
    if (!((int)idx <= mNumBinding - 1)) {
        nw4r::db::Panic("g3d_anmchr.cpp", 0x66, "int(nodeId) is out of bounds(%d)\nint(nodeId) <= %d not satisfied.",
                        idx, mNumBinding - 1);
    }
    return (mpBinding[idx] & 0x8000) == 0;
}

/* 0x8005D824 (0x154): unbinds every node. */
void nw4r::g3d::AnmObjChr::Release()
{
    ANMCHR_POINTER_ASSERT("g3d_anmchr.cpp", this, 0x6E, "NW4R:Pointer Error\nthis(=%p) is not valid pointer.");
    for (int i = 0; i < mNumBinding; i++) {
        mpBinding[i] = 0x8000;
    }
    SetAnmFlag(ANMFLAG_ISBOUND, false);
}

/* 0x8005D978 (0x274): the base cannot attach: warns and returns NULL. */
nw4r::g3d::AnmObjChrRes* nw4r::g3d::AnmObjChr::Attach(int idx, AnmObjChrRes* pRes)
{
    ANMCHR_POINTER_ASSERT("g3d_anmchr.cpp", this, 0x80, "NW4R:Pointer Error\nthis(=%p) is not valid pointer.");
    ANMCHR_POINTER_ASSERT("g3d_anmchr.cpp", pRes, 0x81, "NW4R:Pointer Error\npObj(=%p) is not valid pointer.");
    nw4r::db::Warning("g3d_anmchr.cpp", 0x87, "AnmObjChr::Attach(%d, %p(%s)) called with %s. Maybe uninteded call.",
                      idx, pRes, reinterpret_cast<G3dObj*>(pRes)->GetTypeName(), GetTypeName());
    return NULL;
}

/* 0x8005DBEC (0x38): returns the type's name. */
const char* nw4r::g3d::AnmObjChr::GetTypeName() const
{
    return GetTypeObj().GetTypeName();
}

/* 0x8005DC30 (0x30): returns the type. */
const nw4r::g3d::G3dObj::TypeObj nw4r::g3d::AnmObjChr::GetTypeObj() const
{
    const TypeObj::TypeName* pName;
    return *reinterpret_cast<const TypeObj*>(fn_8005DC60((void**)&pName, lbl_8056F500));
}

/* 0x8005DCD8 (0x15C): the base cannot detach: warns and returns NULL. */
nw4r::g3d::AnmObjChrRes* nw4r::g3d::AnmObjChr::Detach(int idx)
{
    ANMCHR_POINTER_ASSERT("g3d_anmchr.cpp", this, 0x90, "NW4R:Pointer Error\nthis(=%p) is not valid pointer.");
    nw4r::db::Warning("g3d_anmchr.cpp", 0x96, "AnmObjChr::Detach(%d) called with %s. Maybe uninteded call.", idx,
                      GetTypeName());
    return NULL;
}

/* 0x8005DE34 (0x168): the base has no weights: warns. */
void nw4r::g3d::AnmObjChr::SetWeight(int idx, f32 weight)
{
    ANMCHR_POINTER_ASSERT("g3d_anmchr.cpp", this, 0xA0, "NW4R:Pointer Error\nthis(=%p) is not valid pointer.");
    nw4r::db::Warning("g3d_anmchr.cpp", 0xA6, "AnmObjChr::Attach(%d, %f) called with %s. Maybe uninteded call.", idx,
                      weight, GetTypeName());
}

/* 0x8005DF9C (0x15C): the base has no weights: warns and returns 0. */
f32 nw4r::g3d::AnmObjChr::GetWeight(int idx) const
{
    ANMCHR_POINTER_ASSERT("g3d_anmchr.cpp", this, 0xAD, "NW4R:Pointer Error\nthis(=%p) is not valid pointer.");
    nw4r::db::Warning("g3d_anmchr.cpp", 0xB3, "AnmObjChr::GetWeight(%d) called with %s. Maybe uninteded call.", idx,
                      GetTypeName());
    return 0.0f;
}

/* 0x8005E0F8 (0x148): the base cannot detach: warns. */
void nw4r::g3d::AnmObjChr::DetachAll()
{
    ANMCHR_POINTER_ASSERT("g3d_anmchr.cpp", this, 0xBB, "NW4R:Pointer Error\nthis(=%p) is not valid pointer.");
    nw4r::db::Warning("g3d_anmchr.cpp", 0xC1, "AnmObjChr::DetachAll() called with %s. Maybe uninteded call.",
                      GetTypeName());
}

/* 0x8005E58C (0x5C): destroys the character animation. */
nw4r::g3d::AnmObjChr::~AnmObjChr()
{
}

/* ------------------------------------------------------------------------------------------------ */
/* The run-time type members of G3dObj, AnmObj and AnmObjChr                                        */
/* ------------------------------------------------------------------------------------------------ */

/* 0x80063888 (0x30): returns the type. */
const nw4r::g3d::G3dObj::TypeObj nw4r::g3d::AnmObj::GetTypeObj() const
{
    const u8* pName;
    return *reinterpret_cast<const TypeObj*>(type_obj_set_name(&pName, anm_typename_AnmObj));
}

/* 0x800638C0 (0x38): returns the type's name. */
const char* nw4r::g3d::AnmObj::GetTypeName() const
{
    return GetTypeObj().GetTypeName();
}

/* 0x800638F8 (0x6C): whether the object is an AnmObj or derives from `type`. */
bool nw4r::g3d::AnmObj::IsDerivedFrom(TypeObj type) const
{
    if (type == GetTypeObjStatic()) {
        return true;
    }
    return G3dObj::IsDerivedFrom(type);
}

/* 0x80063964 (0x3C): whether `type` is G3dObj. */
bool nw4r::g3d::G3dObj::IsDerivedFrom(TypeObj type) const
{
    return type == GetTypeObjStatic();
}

/* 0x800639A0 (0x30): returns the G3dObj type. */
const nw4r::g3d::G3dObj::TypeObj nw4r::g3d::G3dObj::GetTypeObjStatic()
{
    const u8* pName;
    return *reinterpret_cast<const TypeObj*>(type_obj_set_name(&pName, anm_typename_G3dObj));
}

/* 0x800639D0 (0x50): whether the two types are the same. */
bool nw4r::g3d::G3dObj::TypeObj::operator==(const TypeObj& rhs) const
{
    return GetTypeID() == rhs.GetTypeID();
}

/* 0x80063A20 (0x8): returns the type's identity (its name record's address). */
u32 nw4r::g3d::G3dObj::TypeObj::GetTypeID() const
{
    return (u32)mName;
}

/* 0x80063A28 (0x30): returns the AnmObj type. */
const nw4r::g3d::G3dObj::TypeObj nw4r::g3d::AnmObj::GetTypeObjStatic()
{
    const u8* pName;
    return *reinterpret_cast<const TypeObj*>(type_obj_set_name(&pName, anm_typename_AnmObj));
}

/* 0x80063A58 (0x6C): whether the object is an AnmObjChr or derives from `type`. */
bool nw4r::g3d::AnmObjChr::IsDerivedFrom(TypeObj type) const
{
    if (type == GetTypeObjStatic()) {
        return true;
    }
    return AnmObj::IsDerivedFrom(type);
}

/* 0x80063AC4 (0x30): returns the AnmObjChr type. */
const nw4r::g3d::G3dObj::TypeObj nw4r::g3d::AnmObjChr::GetTypeObjStatic()
{
    const TypeObj::TypeName* pName;
    return *reinterpret_cast<const TypeObj*>(fn_8005DC60((void**)&pName, lbl_8056F500));
}

/* ------------------------------------------------------------------------------------------------ */
/* AnmObjChrNode                                                                                    */
/* ------------------------------------------------------------------------------------------------ */

/* Whether `idx` names one of the node's children. */
static inline bool anmchr_node_idx_valid(const nw4r::g3d::AnmObjChrNode* pNode, int idx)
{
    bool valid = false;
    if (idx >= 0 && idx <= pNode->mChildrenArraySize - 1) {
        valid = true;
    }
    return valid;
}

/* 0x8005E240 (0x34C): constructs the node over a binding buffer and an emptied child array. */
nw4r::g3d::AnmObjChrNode::AnmObjChrNode(MEMAllocator* pHeap, u16* pBindingBuf, int numBinding,
                                        AnmObjChrRes** ppChildrenBuf, int numChildren)
    : AnmObjChr(pHeap, pBindingBuf, numBinding), mChildrenArraySize(numChildren), mpChildrenArray(ppChildrenBuf)
{
    ANMCHR_POINTER_ASSERT("g3d_anmchr.cpp", pHeap, 0xD4, "NW4R:Pointer Error\npHeap(=%p) is not valid pointer.");
    ANMCHR_POINTER_ASSERT("g3d_anmchr.cpp", pBindingBuf, 0xD5,
                          "NW4R:Pointer Error\npBindingBuffer(=%p) is not valid pointer.");
    ANMCHR_POINTER_ASSERT("g3d_anmchr.cpp", ppChildrenBuf, 0xD6,
                          "NW4R:Pointer Error\npChildrenArray(=%p) is not valid pointer.");
    for (int i = 0; i < mChildrenArraySize; i++) {
        mpChildrenArray[i] = NULL;
    }
}

/* 0x8005E5E8 (0x16C): detaches every child and destroys the node. */
nw4r::g3d::AnmObjChrNode::~AnmObjChrNode()
{
    ANMCHR_POINTER_ASSERT("g3d_anmchr.cpp", this, 0xE1, "NW4R:Pointer Error\nthis(=%p) is not valid pointer.");
    DetachAll();
}

/* 0x8005E754 (0x314): attaches `pRes` as child `idx`, binding the nodes it defines, and returns the child it
 * replaced. */
nw4r::g3d::AnmObjChrRes* nw4r::g3d::AnmObjChrNode::Attach(int idx, AnmObjChrRes* pRes)
{
    ANMCHR_POINTER_ASSERT("g3d_anmchr.cpp", this, 0xE8, "NW4R:Pointer Error\nthis(=%p) is not valid pointer.");
    if (!anmchr_node_idx_valid(this, idx)) {
        nw4r::db::Panic("g3d_anmchr.cpp", 0xE9, "idx is out of bounds(%d)\n%d <= idx <= %d not satisfied.", idx, 0,
                        mChildrenArraySize - 1);
    }
    ANMCHR_POINTER_ASSERT("g3d_anmchr.cpp", pRes, 0xEA, "NW4R:Pointer Error\npObj(=%p) is not valid pointer.");
    AnmObjChrRes* pOld = Detach(idx);
    bool bound = false;
    for (u32 i = 0; i < mNumBinding; i++) {
        if (pRes->TestDefined(i)) {
            bound = true;
            mpBinding[i] = 0;
        }
    }
    if (bound) {
        SetAnmFlag(ANMFLAG_ISBOUND, true);
    }
    mpChildrenArray[idx] = pRes;
    pRes->G3dProc(G3DPROC_ATTACH_PARENT, 0, this);
    return pOld;
}

/* 0x8005EA68 (0x33C): detaches child `idx`, rebuilding the binding words from the remaining children, and returns
 * it. */
nw4r::g3d::AnmObjChrRes* nw4r::g3d::AnmObjChrNode::Detach(int idx)
{
    ANMCHR_POINTER_ASSERT("g3d_anmchr.cpp", this, 0x107, "NW4R:Pointer Error\nthis(=%p) is not valid pointer.");
    if (!anmchr_node_idx_valid(this, idx)) {
        nw4r::db::Panic("g3d_anmchr.cpp", 0x108, "idx is out of bounds(%d)\n%d <= idx <= %d not satisfied.", idx, 0,
                        mChildrenArraySize - 1);
    }
    AnmObjChrRes* pOld = mpChildrenArray[idx];
    if (pOld != NULL) {
        ANMCHR_POINTER_ASSERT("g3d_anmchr.cpp", pOld, 0x10F, "NW4R:Pointer Error\npOld(=%p) is not valid pointer.");
        pOld->G3dProc(G3DPROC_DETACH_PARENT, 0, this);
        mpChildrenArray[idx] = NULL;
        bool bound = false;
        for (u32 i = 0; i < mNumBinding; i++) {
            u16 binding = 0x8000;
            for (int j = 0; j < mChildrenArraySize; j++) {
                if (mpChildrenArray[j] != NULL && mpChildrenArray[j]->TestDefined(i)) {
                    bound = true;
                    binding = 0;
                    break;
                }
            }
            mpBinding[i] = binding;
        }
        if (!bound) {
            SetAnmFlag(ANMFLAG_ISBOUND, false);
        }
    }
    return pOld;
}

/* 0x8005EDA4 (0x14C): detaches every child. */
void nw4r::g3d::AnmObjChrNode::DetachAll()
{
    ANMCHR_POINTER_ASSERT("g3d_anmchr.cpp", this, 0x137, "NW4R:Pointer Error\nthis(=%p) is not valid pointer.");
    for (int i = 0; i < mChildrenArraySize; i++) {
        Detach(i);
    }
}

/* 0x8005EEF0 (0x164): advances every child's frame. */
void nw4r::g3d::AnmObjChrNode::UpdateFrame()
{
    ANMCHR_POINTER_ASSERT("g3d_anmchr.cpp", this, 0x142, "NW4R:Pointer Error\nthis(=%p) is not valid pointer.");
    for (int i = 0; i < mChildrenArraySize; i++) {
        AnmObjChrRes* pChild = mpChildrenArray[i];
        if (pChild != NULL) {
            pChild->UpdateFrame();
        }
    }
}

/* Whether `value` is a finite number (not infinite, not NaN): the frame and rate setters' assertion. */
static inline bool anmchr_value_valid(f32 value)
{
    bool valid = false;
    if (__fpclassifyf(value) > 2 && __fpclassifyf(value) != 1) {
        valid = true;
    }
    return valid;
}
/* 0x8005F054 (0x1C4): sets every child's frame. */
void nw4r::g3d::AnmObjChrNode::SetFrame(f32 frame)
{
    ANMCHR_POINTER_ASSERT("g3d_anmchr.cpp", this, 0x150, "NW4R:Pointer Error\nthis(=%p) is not valid pointer.");
    if (!anmchr_value_valid(frame)) {
        nw4r::db::Panic("g3d_anmchr.cpp", 0x151, "NW4R:Floating Point Value Error(%f)\nframe is infinite or nan.", frame);
    }
    for (int i = 0; i < mChildrenArraySize; i++) {
        AnmObjChrRes* pChild = mpChildrenArray[i];
        if (pChild != NULL) {
            pChild->SetFrame(frame);
        }
    }
}

/* 0x8005F380 (0x1C4): sets every child's update rate. */
void nw4r::g3d::AnmObjChrNode::SetUpdateRate(f32 rate)
{
    ANMCHR_POINTER_ASSERT("g3d_anmchr.cpp", this, 0x16F, "NW4R:Pointer Error\nthis(=%p) is not valid pointer.");
    if (!anmchr_value_valid(rate)) {
        nw4r::db::Panic("g3d_anmchr.cpp", 0x170, "NW4R:Floating Point Value Error(%f)\nrate is infinite or nan.", rate);
    }
    for (int i = 0; i < mChildrenArraySize; i++) {
        AnmObjChrRes* pChild = mpChildrenArray[i];
        if (pChild != NULL) {
            pChild->SetUpdateRate(rate);
        }
    }
}

/* 0x8005F218 (0x168): returns the first child's frame, 0 without children. */
f32 nw4r::g3d::AnmObjChrNode::GetFrame() const
{
    ANMCHR_POINTER_ASSERT("g3d_anmchr.cpp", this, 0x15F, "NW4R:Pointer Error\nthis(=%p) is not valid pointer.");
    for (int i = 0; i < mChildrenArraySize; i++) {
        if (mpChildrenArray[i] != NULL) {
            return mpChildrenArray[i]->GetFrame();
        }
    }
    return 0.0f;
}

/* 0x8005F544 (0x168): returns the first child's update rate, 1 without children. */
f32 nw4r::g3d::AnmObjChrNode::GetUpdateRate() const
{
    ANMCHR_POINTER_ASSERT("g3d_anmchr.cpp", this, 0x17E, "NW4R:Pointer Error\nthis(=%p) is not valid pointer.");
    for (int i = 0; i < mChildrenArraySize; i++) {
        if (mpChildrenArray[i] != NULL) {
            return mpChildrenArray[i]->GetUpdateRate();
        }
    }
    return 1.0f;
}

/* 0x8005F6AC (0x210): binds every child to `mdl` and marks the nodes they define bound. */
bool nw4r::g3d::AnmObjChrNode::Bind(ResMdl mdl)
{
    ANMCHR_POINTER_ASSERT("g3d_anmchr.cpp", this, 0x18E, "NW4R:Pointer Error\nthis(=%p) is not valid pointer.");
    if (!mdl.IsValid()) {
        nw4r::db::Panic("g3d_anmchr.cpp", 0x18F, "NW4R:Failed assertion resMdl.IsValid()");
    }
    bool result = false;
    for (int i = 0; i < mChildrenArraySize; i++) {
        AnmObjChrRes* pChild = mpChildrenArray[i];
        if (pChild != NULL) {
            bool childResult = pChild->Bind(mdl);
            result = result || childResult;
            for (u32 j = 0; j < mNumBinding; j++) {
                if (pChild->TestDefined(j)) {
                    mpBinding[j] = 0;
                }
            }
        }
    }
    SetAnmFlag(ANMFLAG_ISBOUND, true);
    return result;
}

/* 0x8005F8BC (0x220): binds every child to `target` of `mdl` and marks the nodes they define bound. */
bool nw4r::g3d::AnmObjChrNode::Bind(ResMdl mdl, u32 target, BindOption option)
{
    ANMCHR_POINTER_ASSERT("g3d_anmchr.cpp", this, 0x1B6, "NW4R:Pointer Error\nthis(=%p) is not valid pointer.");
    if (!mdl.IsValid()) {
        nw4r::db::Panic("g3d_anmchr.cpp", 0x1B7, "NW4R:Failed assertion resMdl.IsValid()");
    }
    bool result = false;
    for (int i = 0; i < mChildrenArraySize; i++) {
        AnmObjChrRes* pChild = mpChildrenArray[i];
        if (pChild != NULL) {
            bool childResult = pChild->Bind(mdl, target, option);
            result = result || childResult;
            for (u32 j = 0; j < mNumBinding; j++) {
                if (pChild->TestDefined(j)) {
                    mpBinding[j] = 0;
                }
            }
        }
    }
    SetAnmFlag(ANMFLAG_ISBOUND, true);
    return result;
}

/* 0x8005FADC (0x16C): releases every child and the node's bindings. */
void nw4r::g3d::AnmObjChrNode::Release()
{
    ANMCHR_POINTER_ASSERT("g3d_anmchr.cpp", this, 0x1DB, "NW4R:Pointer Error\nthis(=%p) is not valid pointer.");
    for (int i = 0; i < mChildrenArraySize; i++) {
        AnmObjChrRes* pChild = mpChildrenArray[i];
        if (pChild != NULL) {
            pChild->Release();
        }
    }
    AnmObjChr::Release();
}

/* 0x8005FC48 (0x220): releases `target` of `mdl` from every child and rebinds the nodes the children still
 * define. */
void nw4r::g3d::AnmObjChrNode::Release(ResMdl mdl, u32 target, BindOption option)
{
    ANMCHR_POINTER_ASSERT("g3d_anmchr.cpp", this, 0x1F0, "NW4R:Pointer Error\nthis(=%p) is not valid pointer.");
    if (!mdl.IsValid()) {
        nw4r::db::Panic("g3d_anmchr.cpp", 0x1F1, "NW4R:Failed assertion resMdl.IsValid()");
    }
    for (int i = 0; i < mChildrenArraySize; i++) {
        AnmObjChrRes* pChild = mpChildrenArray[i];
        if (pChild != NULL) {
            pChild->Release(mdl, target, option);
        }
    }
    AnmObjChr::Release();
    for (int i = 0; i < mChildrenArraySize; i++) {
        AnmObjChrRes* pChild = mpChildrenArray[i];
        if (pChild != NULL) {
            for (u32 j = 0; j < mNumBinding; j++) {
                if (pChild->TestDefined(j)) {
                    mpBinding[j] = 0;
                }
            }
        }
    }
}

/* 0x8005FE68 (0x250): handles a child detaching itself and the node being attached to or detached from a
 * parent. */
void nw4r::g3d::AnmObjChrNode::G3dProc(u32 task, u32 param, void* pInfo) /* untyped: caller-owned payload */
{
    ANMCHR_POINTER_ASSERT("g3d_anmchr.cpp", this, 0x218, "NW4R:Pointer Error\nthis(=%p) is not valid pointer.");
    switch (task) {
    case G3DPROC_CHILD_DETACHED: {
        int i;
        for (i = 0; i < mChildrenArraySize; i++) {
            if (mpChildrenArray[i] == pInfo) {
                Detach(i);
                break;
            }
        }
        if (i >= mChildrenArraySize) {
            nw4r::db::Warning("g3d_anmchr.cpp", 0x22D,
                              "AnmObjChrNode::G3dProc(G3DPROC_CHILD_DETACHED,%p,%p)\nThe child not found.", param, pInfo);
        }
        break;
    }
    case G3DPROC_DETACH_PARENT:
        if (!GetParent()) {
            nw4r::db::Panic("g3d_anmchr.cpp", 0x233, "NW4R:Failed assertion GetParent()");
        }
        SetParent(NULL);
        break;
    case G3DPROC_ATTACH_PARENT:
        if (GetParent()) {
            nw4r::db::Panic("g3d_anmchr.cpp", 0x23A, "NW4R:Failed assertion !GetParent()");
        }
        SetParent(static_cast<G3dObj*>(pInfo));
        break;
    }
}

/* ------------------------------------------------------------------------------------------------ */
/* AnmObjChrBlend                                                                                   */
/* ------------------------------------------------------------------------------------------------ */

/* Whether `idx` names one of the blend's children. */
static inline bool anmchr_blend_idx_valid(const nw4r::g3d::AnmObjChrBlend* pBlend, int idx)
{
    bool valid = false;
    if (idx >= 0 && idx <= pBlend->mChildrenArraySize - 1) {
        valid = true;
    }
    return valid;
}

/* 0x800604F4 (0x164): constructs the blend with every child weight 1. */
nw4r::g3d::AnmObjChrBlend::AnmObjChrBlend(MEMAllocator* pHeap, u16* pBindingBuf, int numBinding,
                                          AnmObjChrRes** ppChildrenBuf, int numChildren, f32* pWeightBuf)
    : AnmObjChrNode(pHeap, pBindingBuf, numBinding, ppChildrenBuf, numChildren), mpWeightArray(pWeightBuf)
{
    ANMCHR_POINTER_ASSERT("g3d_anmchr.cpp", this, 0x292, "NW4R:Pointer Error\nthis(=%p) is not valid pointer.");
    for (int i = 0; i < mChildrenArraySize; i++) {
        mpWeightArray[i] = 1.0f;
    }
}

/* 0x800610D8 (0x1CC): sets child `idx`'s weight. */
void nw4r::g3d::AnmObjChrBlend::SetWeight(int idx, f32 weight)
{
    ANMCHR_POINTER_ASSERT("g3d_anmchr.cpp", this, 0x3AE, "NW4R:Pointer Error\nthis(=%p) is not valid pointer.");
    if (!anmchr_blend_idx_valid(this, idx)) {
        nw4r::db::Panic("g3d_anmchr.cpp", 0x3AF, "idx is out of bounds(%d)\n%d <= idx <= %d not satisfied.", idx, 0,
                        mChildrenArraySize - 1);
    }
    if (!(weight >= 0.0f)) {
        nw4r::db::Panic("g3d_anmchr.cpp", 0x3B0, "weight is out of bounds(%f)\n%f <= weight not satisfied.", weight,
                        0.0);
    }
    mpWeightArray[idx] = weight;
}

/* 0x800612A4 (0x180): returns child `idx`'s weight. */
f32 nw4r::g3d::AnmObjChrBlend::GetWeight(int idx) const
{
    ANMCHR_POINTER_ASSERT("g3d_anmchr.cpp", this, 0x3B7, "NW4R:Pointer Error\nthis(=%p) is not valid pointer.");
    if (!anmchr_blend_idx_valid(this, idx)) {
        nw4r::db::Panic("g3d_anmchr.cpp", 0x3B8, "idx is out of bounds(%d)\n%d <= idx <= %d not satisfied.", idx, 0,
                        mChildrenArraySize - 1);
    }
    return mpWeightArray[idx];
}

/* 0x80063C00 (0x5C): destroys the blend. */
nw4r::g3d::AnmObjChrBlend::~AnmObjChrBlend()
{
}

/* 0x80063C5C (0x30): returns the type. */
const nw4r::g3d::G3dObj::TypeObj nw4r::g3d::AnmObjChrBlend::GetTypeObj() const
{
    void* pName;
    return *reinterpret_cast<const TypeObj*>(fn_80063C8C(&pName, anm_typename_AnmObjChrBlend));
}

/* 0x80063C94 (0x38): returns the type's name. */
const char* nw4r::g3d::AnmObjChrBlend::GetTypeName() const
{
    return GetTypeObj().GetTypeName();
}

/* 0x80063CCC (0x6C): whether the object is an AnmObjChrBlend or derives from `type`. */
bool nw4r::g3d::AnmObjChrBlend::IsDerivedFrom(TypeObj type) const
{
    if (type == GetTypeObjStatic()) {
        return true;
    }
    return AnmObjChrNode::IsDerivedFrom(type);
}

/* 0x80063D38 (0x30): returns the AnmObjChrBlend type. */
const nw4r::g3d::G3dObj::TypeObj nw4r::g3d::AnmObjChrBlend::GetTypeObjStatic()
{
    void* pName;
    return *reinterpret_cast<const TypeObj*>(fn_80063C8C(&pName, anm_typename_AnmObjChrBlend));
}

/* ------------------------------------------------------------------------------------------------ */
/* The run-time type members of AnmObjChrNode                                                       */
/* ------------------------------------------------------------------------------------------------ */

/* 0x80063AF4 (0x30): returns the type. */
const nw4r::g3d::G3dObj::TypeObj nw4r::g3d::AnmObjChrNode::GetTypeObj() const
{
    void* pName;
    return *reinterpret_cast<const TypeObj*>(fn_80063B24(&pName, anm_typename_AnmObjChrNode));
}

/* 0x80063B2C (0x38): returns the type's name. */
const char* nw4r::g3d::AnmObjChrNode::GetTypeName() const
{
    return GetTypeObj().GetTypeName();
}

/* 0x80063B64 (0x6C): whether the object is an AnmObjChrNode or derives from `type`. */
bool nw4r::g3d::AnmObjChrNode::IsDerivedFrom(TypeObj type) const
{
    if (type == GetTypeObjStatic()) {
        return true;
    }
    return AnmObjChr::IsDerivedFrom(type);
}

/* 0x80063BD0 (0x30): returns the AnmObjChrNode type. */
const nw4r::g3d::G3dObj::TypeObj nw4r::g3d::AnmObjChrNode::GetTypeObjStatic()
{
    void* pName;
    return *reinterpret_cast<const TypeObj*>(fn_80063B24(&pName, anm_typename_AnmObjChrNode));
}

/* ------------------------------------------------------------------------------------------------ */
/* The run-time type members of AnmObjChrRes                                                        */
/* ------------------------------------------------------------------------------------------------ */

/* 0x8005DC68 (0x38): returns the type's name. */
const char* nw4r::g3d::AnmObjChrRes::GetTypeName() const
{
    return GetTypeObj().GetTypeName();
}

/* 0x8005DCA0 (0x30): returns the type. */
const nw4r::g3d::G3dObj::TypeObj nw4r::g3d::AnmObjChrRes::GetTypeObj() const
{
    void* pName;
    return *reinterpret_cast<const TypeObj*>(type_obj_set_name_anmchr(&pName, lbl_8056F538));
}

/* 0x80063D68 (0x5C): destroys the resource animation. */
nw4r::g3d::AnmObjChrRes::~AnmObjChrRes()
{
}

/* 0x80063DC4 (0x6C): whether the object is an AnmObjChrRes or derives from `type`. */
bool nw4r::g3d::AnmObjChrRes::IsDerivedFrom(TypeObj type) const
{
    if (type == GetTypeObjStatic()) {
        return true;
    }
    return AnmObjChr::IsDerivedFrom(type);
}

/* 0x80063E30 (0x30): returns the AnmObjChrRes type. */
const nw4r::g3d::G3dObj::TypeObj nw4r::g3d::AnmObjChrRes::GetTypeObjStatic()
{
    void* pName;
    return *reinterpret_cast<const TypeObj*>(type_obj_set_name_anmchr(&pName, lbl_8056F538));
}
