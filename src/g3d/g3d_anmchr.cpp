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
 * NAMES. g3d_billboard_work_mtx is a GUESS (the evidence follows).
 *   res_anm_chr_get_node_anm is a GUESS (the evidence follows).
 *   res_dic_is_valid is a GUESS; res_dic_ptr is a GUESS; res_dic_get_class_name is a GUESS;
 *   res_dic_ofs_to_ptr is a GUESS; res_anm_chr_ofs_to_dic is a GUESS; quat_slerp is a GUESS;
 *   quat_from_mtx34 is a GUESS; quat_empty_ctor is a GUESS; mtx34_from_quat is a GUESS; math_fexp is a GUESS;
 *   math_flog_checked is a GUESS; chr_anm_result_init is a GUESS; MTX33_ctor is a GUESS; PSMTXQuat is a GUESS;
 *   QUATMtx is a GUESS; QUATSlerp is a GUESS; anmchr_resnode_class_name is a GUESS;
 *   anmchr_resdic_class_name is a GUESS (the evidence follows).
 *   The quaternion/matrix helpers are nw4r's blend support: the SDK names follow the neighbours (0x804C6430 sits among
 *   the PSMTX scale/rotation builders), the class-name strings are the `.sdata` records 'ResNode'/'ResDic', and the
 *   ResDic accessors (0x800628A4..0x80062914) mirror the ResNode ones.  The blend flag names (ANMFLAG_BLEND_*, the
 *   ChrAnmResult FLAG_*) follow the asserts' text and the branches they select.
 *   anmchr_resnode_ref_name is a GUESS; res_node_assign is a GUESS; res_node_common_assign is a GUESS;
 *   res_node_empty_assign is a GUESS; res_node_get_id is a GUESS; res_node_get_next_sibling is a GUESS;
 *   res_node_get_parent is a GUESS; res_node_ofs_to_node is a GUESS;
 *   res_node_ref_nonconst is a GUESS (the evidence follows).
 *   res_mdl_node_subtree_end is a GUESS (the evidence follows).
 *   res_name_common_copy_ctor is a GUESS; res_node_ref is a GUESS; res_node_get_res_name is a GUESS;
 *   res_anm_chr_get_node_index is a GUESS (the evidence follows).
 *   res_name_copy_ctor is a GUESS; res_node_get_class_name is a GUESS;
 *   res_node_ptr is a GUESS (the evidence follows).
 *   res_node_common_copy_ctor is a GUESS; res_node_copy_ctor is a GUESS; ut_add_offset_to_ptr is a GUESS;
 *   ut_get_int_ptr is a GUESS (the evidence follows).
 *   The map's own names and stems, the stems defined `extern "C"`.  The members of G3dObj, G3dObj::TypeObj,
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
 * RESIDUALS. 1 function unwritten: GetAnmPlayPolicy (0x80061C70, 0x74).
 *   Partial: AnmObjChrBlend::GetResult (0x8D0: the loop's index and the result-flag word take r24/r23 swapped; every
 *   other instruction matches, with `#pragma fp_contract off` around it), AnmObjChrRes::Release(ResMdl, u32,
 *   BindOption) (the unrolled clear loop schedules its `subf` one slot early).
 *   GetAnmPlayPolicy (0x80061C70) is unwritten: its policy table (.sdata 0x80790E70) sits in `mh3_pad.cpp`'s claim.
 *   FrameCtrl::smBaseUpdateRate (.sdata 0x80791168) is `g3d/fn_80063888.cpp`'s claim, declared in the class only.
 *   Partial: g3d_round_up (retail materialises `~(align - 1)` with `nor` + `and` where MWCC folds it into `andc`;
 *   tried: a named mask local, swapped operands), and the members whose only difference is a relocation name (the
 *   strings).
 *   G3dObj::operator delete, AnmObj, AnmObjChr, AnmObjChrBlend, AnmObjChrRes: the empty bodies are complete (retail's
 *   operator delete is a bare `blr`; the compiler emits the constructors' base calls, vtable stores and member
 *   initialisers and the destructors' base calls).  The G3dObj vtable is emitted here (its first virtual,
 *   IsDerivedFrom, is defined here) while retail keeps it in `g3d/fn_80075DCC.cpp`'s `.data`.
 *   The AnmObj type-name record (0x8056F568, read only by AnmObj::GetTypeObj and GetTypeObjStatic here) sits in
 *   `g3d/fn_80063888.cpp`'s `.rodata` claim (0x8056F550-0x8056F578) beside the AnmObjMatClr record (0x8056F550, read
 *   only by that unit's fn_80064868): the two owners interleave there, which one seam cannot express.
 *   flipcheck: `.text` 0x6F18 of 0x6F90; extab 0x3EC of 0x3A4 (__dt__AnmObjChrNode's record is 108 B against 28 B);
 *   extabindex 0x4D4 of 0x4E0; `.rodata`, `.data` (0xA74 of 0xD08), `.bss`, `.sdata` and `.sdata2` are claimed and not emitted.
 *   Relocation names that differ from retail: the string literals (ours are `@NNN` pool names, the target's are the
 *   named `.data` rows) and the float pool constants.
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
#include "g3d/g3d_pointer_assert.h" /* G3D_POINTER_ASSERT (rule 1) */
#include "g3d/fn_8005AA28.h" /* res_node_is_valid (rule 2) */
#include "nw4r/g3d/res_common.h" /* ResHandle (rule 1) */
#include "fn_8004CAD8.h"   /* mtx34_const_ptr's owner header (docs/plan.md 6.5, rule 2) */
#include "OS/PSMTXQuat.h"      /* PSMTXQuat, owner OS/FindContainHeap_.c (leaf header, rule 2) */
#include "NAND/QUATMtx.h"      /* QUATMtx, QUATSlerp, owner NAND/nand.c (leaf header, rule 2) */
#include "nw4r/fn_805012C4.h"   /* nw4r::math::MTX33Identity, MTX34Zero (rule 2) */
#include "mh3_pad/vec3.h"   /* VEC3_ctor (rule 2) */
#include "nw4r/math_arithmetic.h"   /* nw4r::math::detail::FExp, owner nw4r/math_arithmetic.cpp (rule 2) */
#include "nw4r/db_assert.h"  /* nw4r::db::Warning (rule 2) */
#include "MSL_C/alloc.h"     /* __fpclassifyf, owner MSL_C/alloc.cpp (rule 2) */
#include "OS/MEMAllocFromAllocator.h" /* MEMAllocFromAllocator, owner OS/FindContainHeap_.c (rule 2) */

#pragma peephole off

#pragma pool_data off



/* The `.sdata` strings fn_8005D27C / fn_800628AC hand back (`_SDA_BASE_`-relative), and the two
 * absolute constants the blocked-return helpers hand back (`.bss` / `.data`). */
extern u8 anmchr_resnode_class_name[8];
extern u8 anmchr_resdic_class_name[7];
extern const char anmchr_resdic_ref_invalid_fmt[];     /* "%s::%s: Object not valid." */
extern const char anmchr_resdic_ac_file_ref[];         /* "g3d_resdict.h" */
extern const char anmchr_arithmetic_h_file[];          /* "arithmetic.h" */
extern const char anmchr_flog_domain_msg[];            /* "FLog: Input is out of the domain." */
extern const char anmchr_resnode_ref_const_invalid_fmt[]; /* "%s::%s: Object not valid." */
extern const char anmchr_resnode_ac_file_ref_const[];  /* "g3d_resnode_ac.h" */
extern const char anmchr_resdic_ref_name[4];           /* "ref" (sized: an SDA21 address) */
extern const char anmchr_resnode_ref_const_name[4];    /* "ref" (the const ResNode accessor's copy) */
extern u8 lbl_8066AE80[];
extern u8 anmchr_resanmchr_class_name[];    /* "ResAnmChr" */
extern const char anmchr_resanmchr_ac_file[]; /* "g3d_resanmchr_ac.h" */
extern const char anmchr_ref_invalid_fmt[];   /* "%s::%s: Object not valid." */
extern const char anmchr_ref_name[4];         /* "ref" (sized: an SDA21 address) */
extern const char anmchr_resnode_ref_name[4]; /* "ref" (ResNode's copy, sized: an SDA21 address) */
/* The strings the ResNode accessors and the subtree walk pass to Panic, declared so the unit's `.data` keeps its
 * size (literals would grow it and shift the vtables behind them). */
extern const char anmchr_target_node_valid_msg[]; /* "NW4R:Failed assertion targetNode.IsValid()" */
extern const char anmchr_resnode_ac_file_ref[];   /* "g3d_resnode_ac.h" */
extern const char anmchr_resnode_ref_invalid_fmt[]; /* "%s::%s: Object not valid." */
extern const char anmchr_resnode_ac_file_id[];    /* "g3d_resnode_ac.h" */
extern const char anmchr_resnode_is_valid_msg[];  /* "NW4R:Failed assertion IsValid()" */
extern "C" void res_anm_chr_common_copy_ctor(nw4r::g3d::ResAnmChr* pDst, const nw4r::g3d::ResAnmChr* pSrc);

extern "C" {
/* 0x800626F8 (0x58): animated node `idx`'s record, from the resource's node dictionary. */
const nw4r::g3d::ResAnmChrNodeData* res_anm_chr_get_node_anm(const nw4r::g3d::ResAnmChr* pSelf, u32 idx);
/* 0x8005D050 (0x74): the node's index in its model (0 for an invalid handle). */
u32 res_node_get_id(const ResHandle* pSelf);
void* res_node_copy_ctor(void* self, u32* src);
u32 ut_add_offset_to_ptr(u32 a, u32 b);
/* 0x80062D94 (0x58): the node's name. */
nw4r::g3d::ResName res_node_get_res_name(const ResHandle* pNode);
/* 0x80062D04 (0x54): the index of the animated node called `name`, -1 when the resource has none. */
int res_anm_chr_get_node_index(const nw4r::g3d::ResAnmChr* pSelf, nw4r::g3d::ResName name);
/* 0x8005CF10 (0x140): one past the last node of node `idx`'s subtree, in the model's node order. */
u32 res_mdl_node_subtree_end(nw4r::g3d::ResMdl mdl, u32 idx);
}
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

extern "C" u8* g3d_billboard_work_mtx(void)
{
    return lbl_8066AE80;
}

extern "C" u32* res_node_ptr(u32** pp)
{
    return *pp;
}

extern "C" void res_node_empty_assign(u32* a, u32* b)
{
    (void)a;
    (void)b;
}

extern "C" void res_node_common_assign(u32* dst, u32* src)
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

extern "C" u8* res_node_get_class_name(void)
{
    return anmchr_resnode_class_name;
}

extern "C" void res_node_common_copy_ctor(u32* dst, u32* src)
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
extern "C" void* res_node_assign(u32* dst, u32* src)
{
    res_node_common_assign(dst, src);
    res_node_empty_assign(dst + 1, src + 1);
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

/* 0x80060FEC (0x4): constructs a quaternion (leaves it unset). */
extern "C" void quat_empty_ctor(nw4r::math::QUAT*)
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

/* 0x80062024 (0x8): the current frame. */
f32 nw4r::g3d::FrameCtrl::GetFrm() const
{
    return mFrame;
}

/* 0x800621DC (0x8): sets the update rate. */
void nw4r::g3d::FrameCtrl::SetRate(f32 rate)
{
    mUpdateRate = rate;
}

/* 0x80062300 (0x8): the update rate. */
f32 nw4r::g3d::FrameCtrl::GetRate() const
{
    return mUpdateRate;
}

/* 0x80061EC4 (0x44): sets the frame, folded into the frame range by the play policy. */
void nw4r::g3d::FrameCtrl::SetFrm(f32 frame)
{
    mFrame = mpPlayPolicy(mStartFrame, mEndFrame, frame);
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
extern "C" void* res_dic_ptr(void* self)
{
    return *(void**)self;
}

extern "C" u8* res_dic_get_class_name(void)
{
    return anmchr_resdic_class_name;
}

/* 0x80062978 (0x8): stores the block address. */
NW4R_G3D_RESCOMMON_CTOR(nw4r::g3d::ResDicData)

extern "C" void res_name_common_copy_ctor(u32* dst, u32* src)
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

extern "C" void* res_name_copy_ctor(void* self, u32* src)
{
    res_name_common_copy_ctor((u32*)self, src);
    return self;
}

extern "C" void* res_node_copy_ctor(void* self, u32* src)
{
    res_node_common_copy_ctor((u32*)self, src);
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
extern "C" u32 res_dic_is_valid(void* self)
{
    return *(u32*)self != 0;
}

/* Returns the word at +0 plus `offset`, or 0 when `offset` is 0 (nw4r's null-safe offset helper). */
extern "C" u8* res_dic_ofs_to_ptr(u8** p, s32 offset)
{
    u8* base = *p;

    if (offset != 0) {
        return base + offset;
    }
    return NULL;
}

/* The align-up helper: (~(align - 1)) & (offset + align - 1). */
extern "C" u32 g3d_round_up(u32 offset, u32 align)
{
    return (align + offset - 1) & ~(align - 1);
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

/* 0x80060F70 (0x3C): the quaternion interpolation of `pFrom` towards `pTo` by `t` into `pOut`. */
extern "C" nw4r::math::QUAT* quat_slerp(nw4r::math::QUAT* pOut, const nw4r::math::QUAT* pFrom,
                                        const nw4r::math::QUAT* pTo, f32 t)
{
    QUATSlerp(pFrom, pTo, pOut, t);
    return pOut;
}

/* 0x80060FAC (0x40): the quaternion of the matrix at `pMtx`. */
extern "C" nw4r::math::QUAT* quat_from_mtx34(nw4r::math::QUAT* pOut, const nw4r::math::MTX34* pMtx)
{
    QUATMtx(pOut, mtx34_const_ptr((u32)pMtx));
    return pOut;
}

/* 0x8006244C (0x18): advances the frame by the update rate times the base rate. */
void nw4r::g3d::FrameCtrl::UpdateFrm()
{
    f32 step = mUpdateRate * smBaseUpdateRate;

    SetFrm(mFrame + step);
}

/* --- nw4r's ``fn_800626F4``-returns-this pair ------------------------------------------------------- */

extern "C" u32 ut_get_int_ptr(u32 value)
{
    return value;
}

extern "C" u32 ut_add_offset_to_ptr(u32 a, u32 b)
{
    return b + ut_get_int_ptr(a);
}

/* --- forwarders into nw4r math ---------------------------------------------------------------------- */

/* The exp forwarder (nw4r's inline `math::FExp`): a tail call with the argument untouched. */
extern "C" f32 math_fexp(f32 x)
{
    return nw4r::math::detail::FExp(x);
}

/* 0x80060F28 (0x44): builds the rotation matrix of the quaternion into `pOut` and returns it. */
extern "C" nw4r::math::MTX34* mtx34_from_quat(nw4r::math::MTX34* pOut, const nw4r::math::QUAT* pQuat)
{
    PSMTXQuat((nw4r::math::MTX34*)mtx34_get_ptr(pOut), pQuat);
    return pOut;
}

/* 0x80060FF0 (0x78): the natural log, NaN (with a warning) outside the domain. */
extern "C" f32 math_flog_checked(f32 x)
{
    if (!(x > 0.0f)) {
        nw4r::db::Warning(anmchr_arithmetic_h_file, 295, anmchr_flog_domain_msg);
    }
    if (x > 0.0f) {
        return nw4r::math::detail::FLog(x);
    }
    return -(0.0f / 0.0f);
}

/* 0x80061068 (0x44): constructs a character animation result (scale, rotation vector and matrix). */
extern "C" nw4r::g3d::ChrAnmResult* chr_anm_result_init(nw4r::g3d::ChrAnmResult* pSelf)
{
    VEC3_ctor(&pSelf->s);
    VEC3_ctor(&pSelf->rawR);
    MTX34_ctor(&pSelf->rt);
    return pSelf;
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
    if (res_dic_is_valid((u32*)this)) {
        bool inRange = false;
        if (idx >= 0 && idx <= (s32)ref().numData - 1) {
            inRange = true;
        }
        if (!inRange) {
            nw4r::db::Panic((const char*)anmchr_resdic_ac_file, 42, (const char*)anmchr_resdic_idx_bounds_msg, idx, 0,
                            ref().numData - 1);
        }
        return (void*)res_dic_ofs_to_ptr((u8**)this, ref().entry[idx + 1].ofsData);
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

#define ANMCHR_POINTER_ASSERT G3D_POINTER_ASSERT

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

/* 0x80061B3C (0x134): starts at frame 0 with rate 1 over [startFrame, endFrame], folding with `pPolicy`. */
nw4r::g3d::FrameCtrl::FrameCtrl(f32 startFrame, f32 endFrame, PlayPolicyFunc pPolicy)
    : mFrame(0.0f), mUpdateRate(1.0f), mStartFrame(startFrame), mEndFrame(endFrame), mpPlayPolicy(pPolicy)
{
    ANMCHR_POINTER_ASSERT("g3d_anmobj.h", pPolicy, 0x6A, "NW4R:Pointer Error\npolicy(=%p) is not valid pointer.");
}

/* 0x80061D2C (0x198): sets the frame and refreshes the result cache. */
void nw4r::g3d::AnmObjChrRes::SetFrame(f32 frame)
{
    ANMCHR_POINTER_ASSERT("g3d_anmchr.cpp", this, 0x419, "NW4R:Pointer Error\nthis(=%p) is not valid pointer.");
    if (!anmchr_value_valid(frame)) {
        nw4r::db::Panic("g3d_anmchr.cpp", 0x41A, "NW4R:Floating Point Value Error(%f)\nframe is infinite or nan.", frame);
    }
    SetFrm(frame);
    if (mpResultCache) {
        UpdateCache();
    }
}

/* 0x80061F08 (0x11C): the current frame. */
f32 nw4r::g3d::AnmObjChrRes::GetFrame() const
{
    ANMCHR_POINTER_ASSERT("g3d_anmchr.cpp", this, 0x428, "NW4R:Pointer Error\nthis(=%p) is not valid pointer.");
    return GetFrm();
}

/* 0x8006202C (0x1B0): sets the update rate; a rate of 0 refreshes the result cache. */
void nw4r::g3d::AnmObjChrRes::SetUpdateRate(f32 rate)
{
    ANMCHR_POINTER_ASSERT("g3d_anmchr.cpp", this, 0x430, "NW4R:Pointer Error\nthis(=%p) is not valid pointer.");
    if (!anmchr_value_valid(rate)) {
        nw4r::db::Panic("g3d_anmchr.cpp", 0x431, "NW4R:Floating Point Value Error(%f)\nrate is infinite or nan.", rate);
    }
    SetRate(rate);
    if (rate == 0.0f && mpResultCache) {
        UpdateCache();
    }
}

/* 0x800621E4 (0x11C): the update rate. */
f32 nw4r::g3d::AnmObjChrRes::GetUpdateRate() const
{
    ANMCHR_POINTER_ASSERT("g3d_anmchr.cpp", this, 0x440, "NW4R:Pointer Error\nthis(=%p) is not valid pointer.");
    return GetRate();
}

/* 0x80062308 (0x144): advances the frame unless the rate is 0, refreshing the result cache. */
void nw4r::g3d::AnmObjChrRes::UpdateFrame()
{
    ANMCHR_POINTER_ASSERT("g3d_anmchr.cpp", this, 0x448, "NW4R:Pointer Error\nthis(=%p) is not valid pointer.");
    if (GetRate() != 0.0f) {
        UpdateFrm();
        if (mpResultCache) {
            UpdateCache();
        }
    }
}

/* 0x800636B0 (0x1D8): advances the frame on the update pass and handles being attached to or detached from a
 * parent. */
void nw4r::g3d::AnmObjChrRes::G3dProc(u32 task, u32 param, void* pInfo) /* untyped: caller-owned payload */
{
    ANMCHR_POINTER_ASSERT("g3d_anmchr.cpp", this, 0x543, "NW4R:Pointer Error\nthis(=%p) is not valid pointer.");
    switch (task) {
    case G3DPROC_UPDATEFRAME:
        UpdateFrame();
        break;
    case G3DPROC_DETACH_PARENT:
        if (!GetParent()) {
            nw4r::db::Panic("g3d_anmchr.cpp", 0x54F, "NW4R:Failed assertion GetParent()");
        }
        SetParent(NULL);
        break;
    case G3DPROC_ATTACH_PARENT:
        if (GetParent()) {
            nw4r::db::Panic("g3d_anmchr.cpp", 0x556, "NW4R:Failed assertion !GetParent()");
        }
        SetParent(static_cast<G3dObj*>(pInfo));
        break;
    }
}

/* 0x80063170 (0x2CC): node `idx`'s result: nothing for an unbound node, the cached result, or the resource evaluated
 * at the current frame into `pResult`. */
const nw4r::g3d::ChrAnmResult* nw4r::g3d::AnmObjChrRes::GetResult(ChrAnmResult* pResult, u32 idx)
{
    ANMCHR_POINTER_ASSERT("g3d_anmchr.cpp", this, 0x511, "NW4R:Pointer Error\nthis(=%p) is not valid pointer.");
    if (!((int)idx <= mNumBinding - 1)) {
        nw4r::db::Panic("g3d_anmchr.cpp", 0x512, "int(nodeId) is out of bounds(%d)\nint(nodeId) <= %d not satisfied.",
                        idx, mNumBinding - 1);
    }
    ANMCHR_POINTER_ASSERT("g3d_anmchr.cpp", pResult, 0x513, "NW4R:Pointer Error\npResult(=%p) is not valid pointer.");
    u32 anmId = mpBinding[idx];
    if (anmId & 0xC000) {
        pResult->flags = 0;
        return pResult;
    }
    if (anmId != (anmId & 0x3FFF)) {
        nw4r::db::Panic("g3d_anmchr.cpp", 0x51D, "NW4R:Failed assertion anmId == (anmId & BINDING_ID_MASK)");
    }
    if (mpResultCache) {
        return &mpResultCache[anmId];
    }
    mRes.GetAnmResult(pResult, anmId, GetFrm());
    return pResult;
}

/* 0x8006343C (0x274): evaluates every bound node's result at the current frame into the result cache. */
void nw4r::g3d::AnmObjChrRes::UpdateCache()
{
    ANMCHR_POINTER_ASSERT("g3d_anmchr.cpp", this, 0x52E, "NW4R:Pointer Error\nthis(=%p) is not valid pointer.");
    ANMCHR_POINTER_ASSERT("g3d_anmchr.cpp", mpResultCache, 0x52F,
                          "NW4R:Pointer Error\nmpResultCache(=%p) is not valid pointer.");
    f32 frame = GetFrm();
    for (u32 i = 0; i < mNumBinding; i++) {
        u16 binding = mpBinding[i];
        if (!(binding & 0x8000)) {
            u32 anmId = binding & 0x3FFF;
            mRes.GetAnmResult(&mpResultCache[anmId], anmId, frame);
        }
    }
}

/* 0x80062464 (0x228): binds every animated node to the model node of the same name; returns whether any matched. */
bool nw4r::g3d::AnmObjChrRes::Bind(ResMdl mdl)
{
    ANMCHR_POINTER_ASSERT("g3d_anmchr.cpp", this, 0x45B, "NW4R:Pointer Error\nthis(=%p) is not valid pointer.");
    if (!mdl.IsValid()) {
        nw4r::db::Panic("g3d_anmchr.cpp", 0x45C, "NW4R:Failed assertion resMdl.IsValid()");
    }
    if ((int)mdl.GetResNodeNumEntries() > mNumBinding) {
        nw4r::db::Panic("g3d_anmchr.cpp", 0x45D,
                        "NW4R:Failed assertion int(resMdl.GetResNodeNumEntries()) <= mNumBinding");
    }
    int numNode = mRes.GetNumNode();
    bool bSuccess = false;
    for (u16 i = 0; i < numNode; i++) {
        const nw4r::g3d::ResAnmChrNodeData* pNode = res_anm_chr_get_node_anm(&mRes, i);
        ResName name((void*)ut_add_offset_to_ptr((u32)pNode, pNode->toResName - 4));
        ResHandle node;
        res_node_copy_ctor(&node, (u32*)&mdl.GetResNode(name));
        if (res_node_is_valid(&node)) {
            u32 id = res_node_get_id(&node);
            mpBinding[id] = i;
            bSuccess = true;
        }
    }
    SetAnmFlag(ANMFLAG_ISBOUND, true);
    return bSuccess;
}

/* 0x80062980 (0x384): binds node `target` (BIND_ONE) or its whole subtree (BIND_PARTIAL) to the animated nodes of
 * the same names; returns whether any matched. */
bool nw4r::g3d::AnmObjChrRes::Bind(ResMdl mdl, u32 target, BindOption option)
{
    ANMCHR_POINTER_ASSERT("g3d_anmchr.cpp", this, 0x47F, "NW4R:Pointer Error\nthis(=%p) is not valid pointer.");
    if (!mdl.IsValid()) {
        nw4r::db::Panic("g3d_anmchr.cpp", 0x480, "NW4R:Failed assertion resMdl.IsValid()");
    }
    if ((int)mdl.GetResNodeNumEntries() > mNumBinding) {
        nw4r::db::Panic("g3d_anmchr.cpp", 0x481,
                        "NW4R:Failed assertion int(resMdl.GetResNodeNumEntries()) <= mNumBinding");
    }
    bool bSuccess = false;
    switch (option) {
    case BIND_PARTIAL: {
        u32 bindIdEnd = res_mdl_node_subtree_end(mdl, target);
        if ((int)bindIdEnd > mNumBinding) {
            nw4r::db::Panic("g3d_anmchr.cpp", 0x48F, "NW4R:Failed assertion int(bindIdEnd) <= mNumBinding");
        }
        for (u32 i = target; i < bindIdEnd; i++) {
            ResHandle node;
            ResHandle name;
            res_node_copy_ctor(&node, (u32*)&mdl.GetResNode(i));
            res_name_copy_ctor(&name, (u32*)&res_node_get_res_name(&node));
            int animId = res_anm_chr_get_node_index(&mRes, *reinterpret_cast<ResName*>(&name));
            if (animId != -1) {
                bool inRange = false;
                if (animId >= 0 && animId <= 0xFFFF) {
                    inRange = true;
                }
                if (!inRange) {
                    nw4r::db::Panic("g3d_anmchr.cpp", 0x49D,
                                    "animId is out of bounds(%d)\n%d <= animId <= %d not satisfied.", animId, 0,
                                    0xFFFF);
                }
                mpBinding[i] = animId;
                bSuccess = true;
            }
        }
        break;
    }
    case BIND_ONE: {
        ResHandle node;
        ResHandle name;
        res_node_copy_ctor(&node, (u32*)&mdl.GetResNode(target));
        res_name_copy_ctor(&name, (u32*)&res_node_get_res_name(&node));
        int animId = res_anm_chr_get_node_index(&mRes, *reinterpret_cast<ResName*>(&name));
        if (animId != -1) {
            bool inRange = false;
            if (animId >= 0 && animId <= 0xFFFF) {
                inRange = true;
            }
            if (!inRange) {
                nw4r::db::Panic("g3d_anmchr.cpp", 0x4B2,
                                "animId is out of bounds(%d)\n%d <= animId <= %d not satisfied.", animId, 0, 0xFFFF);
            }
            mpBinding[target] = animId;
            bSuccess = true;
        }
        break;
    }
    default:
        nw4r::db::Panic("g3d_anmchr.cpp", 0x4BC, "NW4R:Fatal Error\nUnknown bind-option (=%d)", option);
        break;
    }
    SetAnmFlag(ANMFLAG_ISBOUND, true);
    return bSuccess;
}

/* 0x80062E50 (0x320): unbinds node `target` (BIND_ONE) or its whole subtree (BIND_PARTIAL). */
void nw4r::g3d::AnmObjChrRes::Release(ResMdl mdl, u32 target, BindOption option)
{
    ANMCHR_POINTER_ASSERT("g3d_anmchr.cpp", this, 0x4CD, "NW4R:Pointer Error\nthis(=%p) is not valid pointer.");
    if (!mdl.IsValid()) {
        nw4r::db::Panic("g3d_anmchr.cpp", 0x4CE, "NW4R:Failed assertion resMdl.IsValid()");
    }
    if ((int)mdl.GetResNodeNumEntries() > mNumBinding) {
        nw4r::db::Panic("g3d_anmchr.cpp", 0x4CF,
                        "NW4R:Failed assertion int(resMdl.GetResNodeNumEntries()) <= mNumBinding");
    }
    switch (option) {
    case BIND_PARTIAL: {
        u32 bindIdEnd = res_mdl_node_subtree_end(mdl, target);
        if ((int)bindIdEnd > mNumBinding) {
            nw4r::db::Panic("g3d_anmchr.cpp", 0x4D9, "NW4R:Failed assertion int(bindIdEnd) <= mNumBinding");
        }
        for (u32 i = target; i < bindIdEnd; i++) {
            mpBinding[i] = 0x8000;
        }
        break;
    }
    case BIND_ONE:
        if ((int)target >= mNumBinding) {
            nw4r::db::Panic("g3d_anmchr.cpp", 0x4E8, "NW4R:Failed assertion int(target) < mNumBinding");
        }
        mpBinding[target] = 0x8000;
        break;
    default:
        nw4r::db::Panic("g3d_anmchr.cpp", 0x4F3, "NW4R:Fatal Error\nUnknown bind-option (=%d)", option);
        break;
    }
}

/* Wraps a returned handle word in a class temporary, so a copy helper gets the temporary's address. */
struct HandleWord {
    HandleWord(u32 word) { mWord = word; }
    /* +0x00 */ u32 mWord;
}; /* size: 0x4 */

extern "C" void* res_node_assign(u32* dst, u32* src);

/* 0x8005D050 (0x74): the node's index in its model (0 for an invalid handle). */
extern "C" u32 res_node_get_id(const ResHandle* pSelf)
{
    if (!res_node_is_valid(pSelf)) {
        nw4r::db::Panic(anmchr_resnode_ac_file_id, 56, anmchr_resnode_is_valid_msg);
    }
    if (res_node_is_valid(pSelf)) {
        return ((ResNodeData*)res_node_ptr((u32**)pSelf))->mNodeID;
    }
    return 0;
}

/* 0x8005D218 (0x64): the node's block, asserting the handle is valid. */
extern "C" ResNodeData* res_node_ref_nonconst(ResHandle* pSelf)
{
    if (!res_node_is_valid(pSelf)) {
        nw4r::db::Panic(anmchr_resnode_ac_file_ref, 44, anmchr_resnode_ref_invalid_fmt, res_node_get_class_name(),
                        anmchr_resnode_ref_name);
    }
    return (ResNodeData*)res_handle_ptr(pSelf);
}

/* 0x80062840 (0x64): the dictionary block, asserting the handle is valid. */
const nw4r::g3d::ResDicData& nw4r::g3d::ResDic::ref() const
{
    if (!res_dic_is_valid(const_cast<ResDic*>(this))) {
        nw4r::db::Panic(anmchr_resdic_ac_file_ref, 84, anmchr_resdic_ref_invalid_fmt, res_dic_get_class_name(),
                        anmchr_resdic_ref_name);
    }
    return *static_cast<const ResDicData*>(res_dic_ptr(const_cast<ResDic*>(this)));
}

/* 0x800628C8 (0x4C): the dictionary the offset `ofs` bytes into the resource names (a null dictionary for 0). */
extern "C" s32 res_anm_chr_ofs_to_dic(const nw4r::g3d::ResAnmChr* pSelf, s32 key)
{
    u8* pBase = (u8*)pSelf->mpData;
    if (key) {
        return (s32)nw4r::g3d::ResDic(pBase + key).mpData;
    }
    return (s32)nw4r::g3d::ResDic(NULL).mpData;
}

/* 0x800626F8 (0x58): animated node `idx`'s record, from the resource's node dictionary. */
extern "C" const nw4r::g3d::ResAnmChrNodeData* res_anm_chr_get_node_anm(const nw4r::g3d::ResAnmChr* pSelf, u32 idx)
{
    u32 dic = res_anm_chr_ofs_to_dic(pSelf, pSelf->ref().toChrDataDic);
    return (const nw4r::g3d::ResAnmChrNodeData*)(*reinterpret_cast<nw4r::g3d::ResDic*>(&dic))[(int)idx];
}

/* 0x80062D04 (0x54): the index of the animated node called `name`, -1 when the resource has none. */
extern "C" int res_anm_chr_get_node_index(const nw4r::g3d::ResAnmChr* pSelf, nw4r::g3d::ResName name)
{
    return reinterpret_cast<nw4r::g3d::ResDic*>(
               &HandleWord(res_anm_chr_ofs_to_dic(pSelf, pSelf->ref().toChrDataDic)).mWord)
        ->GetIndex(name);
}

/* 0x80062DEC (0x64): the node's block, asserting the handle is valid. */
extern "C" const ResNodeData* res_node_ref(const ResHandle* pSelf)
{
    if (!res_node_is_valid(pSelf)) {
        nw4r::db::Panic(anmchr_resnode_ac_file_ref_const, 44, anmchr_resnode_ref_const_invalid_fmt,
                        res_node_get_class_name(), anmchr_resnode_ref_const_name);
    }
    return (const ResNodeData*)res_node_ptr((u32**)pSelf);
}

/* 0x80062D94 (0x58): the node's name. */
extern "C" nw4r::g3d::ResName res_node_get_res_name(const ResHandle* pNode)
{
    const ResNodeData* pData = res_node_ref(pNode);
    s32 ofs = pData->mToResName;
    if (ofs) {
        return nw4r::g3d::ResName((u8*)pData + ofs - 4);
    }
    return nw4r::g3d::ResName(NULL);
}

/* 0x8005D160 (0x4C): the node `ofs` bytes from this one (a null node for offset 0). */
extern "C" u32 res_node_ofs_to_node(const ResHandle* pSelf, s32 ofs)
{
    u8* pBase = (u8*)pSelf->mpData;
    if (ofs) {
        return (u32)nw4r::g3d::ResNode(pBase + ofs).mpData;
    }
    return (u32)nw4r::g3d::ResNode(NULL).mpData;
}

/* 0x8005D124 (0x3C): the parent node. */
extern "C" u32 res_node_get_parent(ResHandle* pSelf)
{
    return res_node_ofs_to_node(pSelf, res_node_ref_nonconst(pSelf)->mToParentNode);
}

/* 0x8005D284 (0x3C): the next sibling node. */
extern "C" u32 res_node_get_next_sibling(ResHandle* pSelf)
{
    return res_node_ofs_to_node(pSelf, res_node_ref_nonconst(pSelf)->mToNextSibling);
}

/* 0x8005CF10 (0x140): one past the last node of node `idx`'s subtree: the next sibling of the node or of its
 * nearest ancestor that has one, else the node count. */
extern "C" u32 res_mdl_node_subtree_end(nw4r::g3d::ResMdl mdl, u32 idx)
{
    if (!mdl.IsValid()) {
        nw4r::db::Panic("g3d_anmchr.cpp", 42, "NW4R:Failed assertion resMdl.IsValid()");
    }
    ResHandle target;
    res_node_copy_ctor(&target, (u32*)&mdl.GetResNode(idx));
    if (!res_node_is_valid(&target)) {
        nw4r::db::Panic("g3d_anmchr.cpp", 45, anmchr_target_node_valid_msg);
    }
    ResHandle node;
    ResHandle next;
    res_node_copy_ctor(&node, (u32*)&target);
    res_node_copy_ctor(&next, &HandleWord(res_node_get_next_sibling(&node)).mWord);
    while (!res_node_is_valid(&next)) {
        res_node_assign((u32*)&node, &HandleWord(res_node_get_parent(&node)).mWord);
        if (!res_node_is_valid(&node)) {
            break;
        }
        res_node_assign((u32*)&next, &HandleWord(res_node_get_next_sibling(&node)).mWord);
    }
    if (res_node_is_valid(&next)) {
        return res_node_get_id(&next);
    }
    return mdl.GetResNodeNumEntries();
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

#pragma fp_contract off

/* 0x80060658 (0x8D0): node `idx`'s blended result: the weighted sum of the bound children's results (scale, matrix
 * and translation), blending rotation through quaternions or matrix rows as the animation's flags choose. */
const nw4r::g3d::ChrAnmResult* nw4r::g3d::AnmObjChrBlend::GetResult(ChrAnmResult* pResult, u32 idx)
{
    ANMCHR_POINTER_ASSERT("g3d_anmchr.cpp", this, 0x29D, "NW4R:Pointer Error\nthis(=%p) is not valid pointer.");
    ANMCHR_POINTER_ASSERT("g3d_anmchr.cpp", pResult, 0x29E, "NW4R:Pointer Error\npResult(=%p) is not valid pointer.");
    if (!((int)idx <= mNumBinding - 1)) {
        nw4r::db::Panic("g3d_anmchr.cpp", 0x29F, "int(nodeId) is out of bounds(%d)\nint(nodeId) <= %d not satisfied.",
                        idx, mNumBinding - 1);
    }
    AnmObjChrRes* pOnlyChild = NULL;
    int numAnm = 0;
    f32 sumWeight = 0.0f;
    for (int i = 0; i < mChildrenArraySize; i++) {
        f32 weight = mpWeightArray[i];
        AnmObjChrRes* pChild = mpChildrenArray[i];
        if (pChild != NULL && weight != 0.0f && pChild->TestExistence(idx)) {
            if (!(weight > 0.0f)) {
                nw4r::db::Panic("g3d_anmchr.cpp", 0x2AD, "NW4R:Failed assertion weight > 0.0f");
            }
            numAnm++;
            sumWeight += weight;
            if (pOnlyChild == NULL) {
                pOnlyChild = pChild;
            }
        }
    }
    if (numAnm == 0) {
        pResult->flags = 0;
        return pResult;
    }
    if (numAnm == 1) {
        return pOnlyChild->GetResult(pResult, idx);
    }
    bool bQuatRot = TestAnmFlag(ANMFLAG_BLEND_QUATERNION_ROT);
    bool bGeoScale = TestAnmFlag(ANMFLAG_BLEND_GEOMETRIC_SCALE);
    f32 logSx = 0.0f;
    f32 logSy = 0.0f;
    f32 logSz = 0.0f;
    nw4r::math::QUAT blendQuat;
    dVector4Set(&blendQuat.x, 0.0f, 0.0f, 0.0f, 1.0f);
    f32 quatWeight = 0.0f;
    nw4r::math::MTX33 firstRot;
    MTX33_ctor(&firstRot);
    f32 invSum = math_reciprocal(sumWeight);
    pResult->flags = 0xFFFFFFFF;
    pResult->s.x = 0.0f;
    pResult->s.y = 0.0f;
    pResult->s.z = 0.0f;
    nw4r::math::MTX34Zero(&pResult->rt);
    for (int i = 0; i < mChildrenArraySize; i++) {
        ChrAnmResult childResult;
        chr_anm_result_init(&childResult);
        AnmObjChrRes* pChild = mpChildrenArray[i];
        f32 weight = mpWeightArray[i];
        if (pChild != NULL && weight != 0.0f && pChild->TestExistence(idx)) {
            const ChrAnmResult* pMy = pChild->GetResult(&childResult, idx);
            f32 w = weight * invSum;
            u32 myFlags = pMy->flags;
            if (!(myFlags & ChrAnmResult::FLAG_ANM_EXISTS)) {
                nw4r::db::Panic("g3d_anmchr.cpp", 0x2EC,
                                "NW4R:Failed assertion pMyResult->flags & ChrAnmResult::FLAG_ANM_EXISTS");
            }
            if (myFlags & (ChrAnmResult::FLAG_SSC_APPLY | ChrAnmResult::FLAG_SSC_PARENT | ChrAnmResult::FLAG_XSI_SCALING)) {
                nw4r::db::Panic("g3d_anmchr.cpp", 0x2F3,
                                "NW4R Internal error\n"
                                "モデルデータを使用したキャラクタアニメーションのブレンド処理は未実装です。");
            }
            if (bGeoScale) {
                if (!(myFlags & ChrAnmResult::FLAG_SCALE_ONE)) {
                    logSx += w * math_flog_checked(pMy->s.x);
                    logSy += w * math_flog_checked(pMy->s.y);
                    logSz += w * math_flog_checked(pMy->s.z);
                }
            } else if (!(myFlags & ChrAnmResult::FLAG_SCALE_ONE)) {
                pResult->s.x += pMy->s.x * w;
                pResult->s.y += pMy->s.y * w;
                pResult->s.z += pMy->s.z * w;
            } else {
                pResult->s.x += w;
                pResult->s.y += w;
                pResult->s.z += w;
            }
            if (bQuatRot) {
                nw4r::math::QUAT childQuat;
                quat_empty_ctor(&childQuat);
                if (!(myFlags & ChrAnmResult::FLAG_ROT_ZERO)) {
                    quat_from_mtx34(&childQuat, &pMy->rt);
                } else {
                    childQuat.x = 0.0f;
                    childQuat.y = 0.0f;
                    childQuat.z = 0.0f;
                    childQuat.w = 1.0f;
                }
                quatWeight += weight;
                quat_slerp(&blendQuat, &blendQuat, &childQuat, weight * math_reciprocal(quatWeight));
            } else if (!(myFlags & ChrAnmResult::FLAG_ROT_ZERO)) {
                if (i == 0) {
                    firstRot.m[0][0] = pMy->rt.m[0][0];
                    firstRot.m[0][1] = pMy->rt.m[0][1];
                    firstRot.m[0][2] = pMy->rt.m[0][2];
                    firstRot.m[1][0] = pMy->rt.m[1][0];
                    firstRot.m[1][1] = pMy->rt.m[1][1];
                    firstRot.m[1][2] = pMy->rt.m[1][2];
                    firstRot.m[2][0] = pMy->rt.m[2][0];
                    firstRot.m[2][1] = pMy->rt.m[2][1];
                    firstRot.m[2][2] = pMy->rt.m[2][2];
                }
                pResult->rt.m[0][0] += pMy->rt.m[0][0] * w;
                pResult->rt.m[0][1] += pMy->rt.m[0][1] * w;
                pResult->rt.m[0][2] += pMy->rt.m[0][2] * w;
                pResult->rt.m[1][0] += pMy->rt.m[1][0] * w;
                pResult->rt.m[1][1] += pMy->rt.m[1][1] * w;
                pResult->rt.m[1][2] += pMy->rt.m[1][2] * w;
            } else {
                if (i == 0) {
                    nw4r::math::MTX33Identity(&firstRot);
                }
                pResult->rt.m[0][0] += w;
                pResult->rt.m[1][1] += w;
            }
            if (!(myFlags & ChrAnmResult::FLAG_TRANS_ZERO)) {
                pResult->rt.m[0][3] += pMy->rt.m[0][3] * w;
                pResult->rt.m[1][3] += pMy->rt.m[1][3] * w;
                pResult->rt.m[2][3] += pMy->rt.m[2][3] * w;
            }
            pResult->flags &= myFlags;
        }
    }
    if (bGeoScale) {
        pResult->s.x = math_fexp(logSx);
        pResult->s.y = math_fexp(logSy);
        pResult->s.z = math_fexp(logSz);
    }
    if (bQuatRot) {
        nw4r::math::VEC3 trans;
        trans.x = pResult->rt.m[0][3];
        trans.y = pResult->rt.m[1][3];
        trans.z = pResult->rt.m[2][3];
        mtx34_from_quat(&pResult->rt, &blendQuat);
        pResult->rt.m[0][3] = trans.x;
        pResult->rt.m[1][3] = trans.y;
        pResult->rt.m[2][3] = trans.z;
    } else {
        f32* pRow0 = pResult->rt.m[0];
        f32* pRow1 = pResult->rt.m[1];
        f32* pRow2 = pResult->rt.m[2];
        vec3_cross(pRow2, pRow0, pRow1);
        if (0.0f == vec3_length_sq(pRow0) || 0.0f == vec3_length_sq(pRow2)) {
            pResult->rt.m[0][0] = firstRot.m[0][0];
            pResult->rt.m[0][1] = firstRot.m[0][1];
            pResult->rt.m[0][2] = firstRot.m[0][2];
            pResult->rt.m[1][0] = firstRot.m[1][0];
            pResult->rt.m[1][1] = firstRot.m[1][1];
            pResult->rt.m[1][2] = firstRot.m[1][2];
            pResult->rt.m[2][0] = firstRot.m[2][0];
            pResult->rt.m[2][1] = firstRot.m[2][1];
            pResult->rt.m[2][2] = firstRot.m[2][2];
        } else {
            vec3_normalize_into(pRow0, pRow0);
            vec3_normalize_into(pRow2, pRow2);
            vec3_cross(pRow1, pRow2, pRow0);
        }
    }
    pResult->flags &= 0x7FFFFFFF;
    return pResult;
}

#pragma fp_contract on

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
