/*
 * g3d/g3d_anmtexsrt.cpp - nw4r g3d `g3d_anmtexsrt.cpp`: the texture-SRT animation classes (`AnmObjTexSrt`,
 *   `AnmObjTexSrtNode`, `AnmObjTexSrtOverride`, `AnmObjTexSrtRes`), the texture-SRT result applier, the checked
 *   `ResAnmTexSrt`/`ResTexSrt` getters of the headers it includes and their accessors.
 * RANGE. .text 0x80069C80-0x8006EAC0; extab 0x80007C1C-0x80007E98, extabindex 0x800200E8-0x8002040C, .rodata
 *   0x8056F5C0-0x8056F628, .data 0x8058CE10-0x8058D6C0, .sdata 0x80791198-0x807911A0, .sdata2 0x80795D90-0x80795DA8.
 *   The TU is `g3d_anmtexsrt.cpp`: its one-copy `__FILE__` string (0x8058CE10) is cited from
 *   AnmObjTexSrt::TestExistence (0x80069CF4) through apply_tex_srt_anm_result (0x8006E338); the AnmObjTexSrt
 *   constructor (0x80069C80) stores this TU's vtable (0x8058D5B0), and the previous TU's (`g3d_anmtexpat.cpp`,
 *   0x8058CD40) record is last read at 0x80069C50.  The right edge is `g3d/g3d_anmvis.cpp`.
 * FLAGS. `cflags_g3d` (docs/g3d.md).
 * NAMES. apply_tex_srt_anm_result is a GUESS (nw4r's ApplyTexSrtAnmResult, by the ScnMdl material pass that hands it
 *   the copied handles and the animation's result).  The AnmObjTexSrt, AnmObjTexSrtNode, AnmObjTexSrtOverride and
 *   AnmObjTexSrtRes members are nw4r's (the `AnmObjTexSrt::Attach(...)` / `AnmObjTexSrtNode::G3dProc(...)` warnings
 *   and the four vtables); the stems are defined `extern "C"` (playbook 48).  The classes are declared in
 *   `g3d/g3d_anmtexsrt.h`, with the unit's C-linkage helpers.
 * RESIDUALS. Unwritten (19 functions, objdiff scores them zero): 0x8006C138-0x8006C650, 0x8006C668-0x8006CDBC,
 *   0x8006CE48-0x8006D000, 0x8006D084-0x8006D9A0 (the AnmObjTexSrtRes members), 0x8006DA48-0x8006E2A8,
 *   0x8006E338-0x8006E668 (apply_tex_srt_anm_result).  Partial: fn_8006C650, fn_8006E668.
 *   AnmObjTexSrt, AnmObjTexSrtNode, AnmObjTexSrtOverride, AnmObjTexSrtRes: the destructors' empty bodies are complete
 *   (the compiler emits the base calls and the deleting tail; `#pragma peephole off` keeps retail's `extsh`).
 *   fn_8006C650: retail materialises `~(b - 1)` with an explicit `not` where MWCC folds it into `andc` (one
 *   instruction fewer).
 *   flipcheck: `.text`, extab and extabindex are partial; `.rodata`, `.data` and `.sdata2` are claimed and not
 *   emitted.
 * SHAPES. The unit compiles with `#pragma peephole off` throughout (retail keeps the unfused `clrlwi` + `cmpwi`, `extsh`,
 *   `addi r0` vtable-store and `mr r3` + `lwz r12,0(r3)` virtual-call forms; playbook idea 106) and
 *   `#pragma pool_data off`.
 */

#include "types.h"
#include "g3d/g3d_anmscn.h"
#include "g3d/g3d_anmclr.h"   /* type_obj_set_name_texsrt_res/_node/_override (rule 2) */
#include "g3d/g3d_resvtx.h"
#include "g3d/g3d_resmat.h"   /* nw4r::g3d::ResTexSrt (rule 2) */
#include "g3d/g3d_anmchr.h"  /* the fn_8005Dxx / fn_80062xx helpers the g3d_anmchr.cpp range now owns (rule 2) */
#include "g3d/g3d_anmtexsrt.h" /* the texture-SRT classes this range defines (rule 1) */
#include "nw4r/db_assert.h"   /* nw4r::db::Warning (rule 2) */
#include "MSL_C/alloc.h"      /* __fpclassifyf, owner MSL_C/alloc.cpp (rule 2) */

#pragma peephole off

#pragma pool_data off

/* The `.rodata` animation name records the helper-backed readers pass by address (claimed, not emitted). */
extern u8 lbl_8056F5C0[];
extern u8 anm_typename_AnmObjTexSrtNode[];     /* "AnmObjTexSrtNode" (this unit's .rodata) */
extern u8 anm_typename_AnmObjTexSrtOverride[]; /* "AnmObjTexSrtOverride" (this unit's .rodata) */
extern u8 anm_typename_AnmObjTexSrtRes[];      /* "AnmObjTexSrtRes" (this unit's .rodata) */

/* The `.data` type-name strings the plain returners hand back by address (claimed, not emitted). */
extern u8 lbl_8058D678[]; /* "ResAnmTexSrt"  */
extern u8 lbl_8058D5F8[]; /* "ResTexSrt"     */

/* --------------------------------------------------------------------------------------------- *
 * The word accessors (0x80068698-0x8006E6B4): read / test / copy the body word at +0x0.
 * --------------------------------------------------------------------------------------------- */

extern "C" u32 fn_8006CE20(void *p)
{
    return *(u32 *)p;
}

/* 0x8006E310 (0x8): returns the texture-SRT block. */
nw4r::g3d::ResTexSrtData* nw4r::g3d::ResTexSrt::ptr()
{
    return static_cast<ResTexSrtData *>(mpData);
}

extern "C" u32 fn_8006CE34(void *p)
{
    return *(u32 *)p != 0;
}

/* 0x8006E324 (0x14): tells whether the handle is set. */
bool nw4r::g3d::ResTexSrt::IsValid() const
{
    return mpData != NULL;
}

/* 0x8006E6B4 (0x14): tells whether the handle is set. */
bool nw4r::g3d::ResMatIndMtxAndScale::IsValid() const
{
    return mpData != NULL;
}

extern "C" void fn_8006D030(u32 *dst, const u32 *src)
{
    *dst = *src;
}

/* --------------------------------------------------------------------------------------------- *
 * The type-name returners (0x800686A0-0x8006E318): hand back the address of the `.data` name record.
 * --------------------------------------------------------------------------------------------- */

extern "C" u32 fn_8006CE28(void)
{
    return (u32)lbl_8058D678;
}

/* 0x8006E318 (0xC): returns the class name. */
const char *nw4r::g3d::ResTexSrt::GetClassName()
{
    return (const char *)lbl_8058D5F8;
}

/* --------------------------------------------------------------------------------------------- *
 * The two argument-less bodies (0x8006D9A0/0x8006E2A8): `blr`, no work.
 * --------------------------------------------------------------------------------------------- */

extern "C" void fn_8006D9A0(void)
{
}

extern "C" void fn_8006E2A8(void)
{
}

/* --------------------------------------------------------------------------------------------- *
 * The float helpers (0x8006E690/0x8006E6A4): a bit reinterpret and an `fmax`.
 * --------------------------------------------------------------------------------------------- */

extern "C" u32 fn_8006E690(f32 x)
{
    return *(u32 *)&x;
}

extern "C" f32 fn_8006E6A4(f32 x, f32 y)
{
    if (x < y)
        return y;
    return x;
}

/* Rounds `a` up to the next multiple of `b`. */
extern "C" u32 fn_8006C650(u32 a, u32 b)
{
    return (a + b - 1) & -b;
}

/* --------------------------------------------------------------------------------------------- *
 * The store-then-return forwarders (0x800686C0-0x8006D000): write through the word copier beside each
 * one and hand the original `self` back in r3.
 * --------------------------------------------------------------------------------------------- */





extern "C" void fn_8006D030(u32 *dst, const u32 *src);

extern "C" void *fn_8006D000(void *self, void *src)
{
    fn_8006D030((u32 *)self, (const u32 *)src);
    return self;
}

/* --------------------------------------------------------------------------------------------- *
 * The helper-backed name readers (0x800693A0-0x8006E734): store the name record through one of the
 * unsplit store helpers and read the word back through the returned address.
 * --------------------------------------------------------------------------------------------- */


/* --------------------------------------------------------------------------------------------- *
 * The list-insert chain steps (0x80069334-0x8006EA24): resolve a name record, compare the caller's key
 * against it through the `fn_800639D0` helper, and on a miss run the previous step's insertion with a
 * copy of the key word (the shape `g3d/g3d_anmchr.cpp`'s `fn_800638F8` uses beside them).
 * --------------------------------------------------------------------------------------------- */

/* The three name readers this cluster's own insert steps chain into, defined in the helper section
 * beside their `fn_80063898`-family siblings. */


/* --------------------------------------------------------------------------------------------- *
 * The teardown destructors and the checked word reader (0x8006AA4C-0x8006E9C8).
 * --------------------------------------------------------------------------------------------- */


/* --------------------------------------------------------------------------------------------- *
 * The float-packed field reader (0x8006E668) and the indexed resource reader (0x8006D9FC).
 * --------------------------------------------------------------------------------------------- */

extern "C" s32 fn_8006E668(f32 x)
{
    return (s32)(((fn_8006E690(x) >> 15) & 0xFF) - 127);
}

extern "C" u32 fn_8006D9FC(void *self, u32 idx)
{
    u32 base = *(u32 *)self;

    if (idx != 0)
        return (u32)nw4r::g3d::ResDic((void *)(base + idx)).mpData;
    return (u32)nw4r::g3d::ResDic((void *)0).mpData;
}

/* --------------------------------------------------------------------------------------------- *
 * The seven checked getters (0x80068634-0x8006E2AC): resolve the object through the validity test,
 * panic through `nw4r::db::Panic` out of the owning `_ac.h` when it is not valid, then read the body
 * word through the resolver beside it.  The shape is nw4r's `NW4R_ASSERT` (file/line/message and the
 * `"ref"` reference-name argument).
 * --------------------------------------------------------------------------------------------- */
namespace nw4r {
namespace db {

void Panic(const char *file, int line, const char *message, ...);

} /* namespace db */
} /* namespace nw4r */

extern u8 lbl_8058D6A4[];
extern u8 lbl_8058D688[];
extern u8 lbl_8058D620[];
extern u8 lbl_8058D604[];

extern "C" u32 fn_8006CDBC(void *p)
{
    if (fn_8006CE34(p) == 0)
        nw4r::db::Panic((const char *)lbl_8058D6A4, 38, (const char *)lbl_8058D688,
                        (const char *)fn_8006CE28(), "ref");
    return fn_8006CE20(p);
}

/* 0x8006E2AC (0x64): returns the texture-SRT block, panicking on a NULL handle. */
nw4r::g3d::ResTexSrtData& nw4r::g3d::ResTexSrt::ref()
{
    if (!IsValid())
        nw4r::db::Panic((const char *)lbl_8058D620, 107, (const char *)lbl_8058D604, GetClassName(), "ref");
    return *ptr();
}

/* --------------------------------------------------------------------------------------------- *
 * The three field readers of the resolved `ResAnmTexSrt` record (0x8006D03C/0x8006D060/0x8006D9A4).
 * --------------------------------------------------------------------------------------------- */
typedef struct {
    /* +0x00 */ u8 pad_0x00[0x10];
    /* +0x10 */ u32 field_0x10;
    /* +0x14 */ u8 pad_0x14[0xC];
    /* +0x20 */ u16 field_0x20;
    /* +0x22 */ u8 pad_0x22[0x6];
    /* +0x28 */ u32 field_0x28;
} ResAnmTexSrtData; /* size: 0x2C (approximate - only the three fields read here are evidenced) */

extern "C" u16 fn_8006D060(void *p)
{
    return ((ResAnmTexSrtData *)(void *)fn_8006CDBC(p))->field_0x20;
}

extern "C" u32 fn_8006D03C(void *p)
{
    return ((ResAnmTexSrtData *)(void *)fn_8006CDBC(p))->field_0x28;
}

extern "C" u32 fn_8006D9A4(void *self, void *key)
{
    u32 tmp = fn_8006D9FC(self, ((ResAnmTexSrtData *)(void *)fn_8006CDBC(self))->field_0x10);

    return (u32)(*reinterpret_cast<nw4r::g3d::ResDic *>(&tmp))[(int)key];
}

/* --------------------------------------------------------------------------------------------- *
 * The vtable-dispatch wrappers (0x8006A38C/0x8006A3F4/0x8006E794/0x8006E8F4): the body word 0 is its
 * vtable, the entry at +0x14 runs with the object and a word is read back through the `TypeObj::GetTypeName`
 * helper (the shape `g3d/g3d_anmscn.cpp` uses for its two wrappers).
 * --------------------------------------------------------------------------------------------- */


/* ------------------------------------------------------------------------------------------------ */
/* AnmObjTexSrt and its node, override and resource forms (the g3d_anmtexsrt.cpp part)              */
/* ------------------------------------------------------------------------------------------------ */

/* The nw4r resource pointer assert: `ptr` must fall in one of the seven mapped Wii memory ranges. */
#define TEXSRT_POINTER_ASSERT(ptr, line, msg)                                                  \
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
            nw4r::db::Panic("g3d_anmtexsrt.cpp", line, msg, (ptr));                              \
    }

#define TEXSRT_THIS_MSG "NW4R:Pointer Error\nthis(=%p) is not valid pointer."

/* 0x80069C80 (0x74): constructs the texture-SRT animation over a binding buffer of `numBinding` words, all
 * released. */
nw4r::g3d::AnmObjTexSrt::AnmObjTexSrt(MEMAllocator* pHeap, u16* pBindingBuf, int numBinding)
    : AnmObj(pHeap, NULL), mNumBinding(numBinding), mpBinding(pBindingBuf)
{
    Release();
}

/* 0x80069CF4 (0x168): whether material `idx` has an animation bound. */
bool nw4r::g3d::AnmObjTexSrt::TestExistence(u32 idx) const
{
    TEXSRT_POINTER_ASSERT(this, 0x36, TEXSRT_THIS_MSG);
    if (!((int)idx <= mNumBinding - 1)) {
        nw4r::db::Panic("g3d_anmtexsrt.cpp", 0x37, "int(matId) is out of bounds(%d)\nint(matId) <= %d not satisfied.",
                        idx, mNumBinding - 1);
    }
    return (mpBinding[idx] & 0xC000) == 0;
}

/* 0x80069E5C (0x168): whether material `idx`'s binding is defined. */
bool nw4r::g3d::AnmObjTexSrt::TestDefined(u32 idx) const
{
    TEXSRT_POINTER_ASSERT(this, 0x3F, TEXSRT_THIS_MSG);
    if (!((int)idx <= mNumBinding - 1)) {
        nw4r::db::Panic("g3d_anmtexsrt.cpp", 0x40, "int(matId) is out of bounds(%d)\nint(matId) <= %d not satisfied.",
                        idx, mNumBinding - 1);
    }
    return (mpBinding[idx] & 0x8000) == 0;
}

/* 0x80069FC4 (0x154): unbinds every material. */
void nw4r::g3d::AnmObjTexSrt::Release()
{
    TEXSRT_POINTER_ASSERT(this, 0x48, TEXSRT_THIS_MSG);
    for (int i = 0; i < mNumBinding; i++) {
        mpBinding[i] = 0x8000;
    }
    SetAnmFlag(ANMFLAG_ISBOUND, false);
}

/* 0x8006A118 (0x274): the base cannot attach: warns and returns NULL. */
nw4r::g3d::AnmObjTexSrtRes* nw4r::g3d::AnmObjTexSrt::Attach(int idx, AnmObjTexSrtRes* pRes)
{
    TEXSRT_POINTER_ASSERT(this, 0x59, TEXSRT_THIS_MSG);
    TEXSRT_POINTER_ASSERT(pRes, 0x5A, "NW4R:Pointer Error\npObj(=%p) is not valid pointer.");
    nw4r::db::Warning("g3d_anmtexsrt.cpp", 0x60,
                      "AnmObjTexSrt::Attach(%d, %p(%s)) called with %s. Maybe uninteded call.", idx, pRes,
                      reinterpret_cast<G3dObj*>(pRes)->GetTypeName(), GetTypeName());
    return NULL;
}

/* 0x8006A38C (0x38): returns the type's name. */
const char* nw4r::g3d::AnmObjTexSrt::GetTypeName() const
{
    return GetTypeObj().GetTypeName();
}

/* 0x8006A3C4 (0x30): returns the type. */
const nw4r::g3d::G3dObj::TypeObj nw4r::g3d::AnmObjTexSrt::GetTypeObj() const
{
    void* pName;
    return *reinterpret_cast<const TypeObj*>(type_obj_set_name_anmchr(&pName, lbl_8056F5C0));
}

/* 0x8006A3F4 (0x38): returns the type's name. */
const char* nw4r::g3d::AnmObjTexSrtRes::GetTypeName() const
{
    return GetTypeObj().GetTypeName();
}

/* 0x8006A42C (0x30): returns the type. */
const nw4r::g3d::G3dObj::TypeObj nw4r::g3d::AnmObjTexSrtRes::GetTypeObj() const
{
    const u8* pName;
    return *reinterpret_cast<const TypeObj*>(type_obj_set_name_texsrt_res(&pName, anm_typename_AnmObjTexSrtRes));
}

/* 0x8006A45C (0x15C): the base cannot detach: warns and returns NULL. */
nw4r::g3d::AnmObjTexSrtRes* nw4r::g3d::AnmObjTexSrt::Detach(int idx)
{
    TEXSRT_POINTER_ASSERT(this, 0x69, TEXSRT_THIS_MSG);
    nw4r::db::Warning("g3d_anmtexsrt.cpp", 0x6F, "AnmObjTexSrt::Detach(%d) called with %s. Maybe uninteded call.", idx,
                      GetTypeName());
    return NULL;
}

/* 0x8006A5B8 (0x148): the base cannot detach: warns. */
void nw4r::g3d::AnmObjTexSrt::DetachAll()
{
    TEXSRT_POINTER_ASSERT(this, 0x77, TEXSRT_THIS_MSG);
    nw4r::db::Warning("g3d_anmtexsrt.cpp", 0x7D, "AnmObjTexSrt::DetachAll() called with %s. Maybe uninteded call.",
                      GetTypeName());
}

/* Whether `idx` names one of the node's children. */
static inline bool texsrt_node_idx_valid(const nw4r::g3d::AnmObjTexSrtNode* pNode, int idx)
{
    bool valid = false;
    if (idx >= 0 && idx <= pNode->mChildrenArraySize - 1) {
        valid = true;
    }
    return valid;
}

/* 0x8006A700 (0x34C): constructs the node over a binding buffer and an emptied child array. */
nw4r::g3d::AnmObjTexSrtNode::AnmObjTexSrtNode(MEMAllocator* pHeap, u16* pBindingBuf, int numBinding,
                                              AnmObjTexSrtRes** ppChildrenBuf, int numChildren)
    : AnmObjTexSrt(pHeap, pBindingBuf, numBinding), mChildrenArraySize(numChildren), mpChildrenArray(ppChildrenBuf)
{
    TEXSRT_POINTER_ASSERT(pHeap, 0x90, "NW4R:Pointer Error\npHeap(=%p) is not valid pointer.");
    TEXSRT_POINTER_ASSERT(pBindingBuf, 0x91, "NW4R:Pointer Error\npBindingBuffer(=%p) is not valid pointer.");
    TEXSRT_POINTER_ASSERT(ppChildrenBuf, 0x92, "NW4R:Pointer Error\npChildrenArray(=%p) is not valid pointer.");
    for (int i = 0; i < mChildrenArraySize; i++) {
        mpChildrenArray[i] = NULL;
    }
}

/* 0x8006AA4C (0x5C): destroys the texture-SRT animation. */
nw4r::g3d::AnmObjTexSrt::~AnmObjTexSrt()
{
}

/* 0x8006AAA8 (0x16C): detaches every child and destroys the node. */
nw4r::g3d::AnmObjTexSrtNode::~AnmObjTexSrtNode()
{
    TEXSRT_POINTER_ASSERT(this, 0x9D, TEXSRT_THIS_MSG);
    DetachAll();
}

/* 0x8006AC14 (0x314): attaches `pRes` as child `idx`, binding the materials it defines, and returns the child it
 * replaced. */
nw4r::g3d::AnmObjTexSrtRes* nw4r::g3d::AnmObjTexSrtNode::Attach(int idx, AnmObjTexSrtRes* pRes)
{
    TEXSRT_POINTER_ASSERT(this, 0xA4, TEXSRT_THIS_MSG);
    if (!texsrt_node_idx_valid(this, idx)) {
        nw4r::db::Panic("g3d_anmtexsrt.cpp", 0xA5, "idx is out of bounds(%d)\n%d <= idx <= %d not satisfied.", idx, 0,
                        mChildrenArraySize - 1);
    }
    TEXSRT_POINTER_ASSERT(pRes, 0xA6, "NW4R:Pointer Error\npObj(=%p) is not valid pointer.");
    AnmObjTexSrtRes* pOld = Detach(idx);
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

/* 0x8006AF28 (0x33C): detaches child `idx`, rebuilding the binding words from the remaining children, and returns
 * it. */
nw4r::g3d::AnmObjTexSrtRes* nw4r::g3d::AnmObjTexSrtNode::Detach(int idx)
{
    TEXSRT_POINTER_ASSERT(this, 0xC7, TEXSRT_THIS_MSG);
    if (!texsrt_node_idx_valid(this, idx)) {
        nw4r::db::Panic("g3d_anmtexsrt.cpp", 0xC8, "idx is out of bounds(%d)\n%d <= idx <= %d not satisfied.", idx, 0,
                        mChildrenArraySize - 1);
    }
    AnmObjTexSrtRes* pOld = mpChildrenArray[idx];
    if (pOld != NULL) {
        TEXSRT_POINTER_ASSERT(pOld, 0xCE, "NW4R:Pointer Error\npOld(=%p) is not valid pointer.");
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

/* 0x8006B264 (0x14C): detaches every child. */
void nw4r::g3d::AnmObjTexSrtNode::DetachAll()
{
    TEXSRT_POINTER_ASSERT(this, 0xF6, TEXSRT_THIS_MSG);
    for (int i = 0; i < mChildrenArraySize; i++) {
        Detach(i);
    }
}

/* 0x8006B3B0 (0x164): advances every child's frame. */
void nw4r::g3d::AnmObjTexSrtNode::UpdateFrame()
{
    TEXSRT_POINTER_ASSERT(this, 0x101, TEXSRT_THIS_MSG);
    for (int i = 0; i < mChildrenArraySize; i++) {
        AnmObjTexSrtRes* pChild = mpChildrenArray[i];
        if (pChild != NULL) {
            pChild->UpdateFrame();
        }
    }
}

/* Whether `value` is a finite number (not infinite, not NaN): the frame and rate setters' assertion. */
static inline bool texsrt_value_valid(f32 value)
{
    bool valid = false;
    if (__fpclassifyf(value) > 2 && __fpclassifyf(value) != 1) {
        valid = true;
    }
    return valid;
}
/* 0x8006B514 (0x1C4): sets every child's frame. */
void nw4r::g3d::AnmObjTexSrtNode::SetFrame(f32 frame)
{
    TEXSRT_POINTER_ASSERT(this, 0x10F, TEXSRT_THIS_MSG);
    if (!texsrt_value_valid(frame)) {
        nw4r::db::Panic("g3d_anmtexsrt.cpp", 0x110, "NW4R:Floating Point Value Error(%f)\nframe is infinite or nan.", frame);
    }
    for (int i = 0; i < mChildrenArraySize; i++) {
        AnmObjTexSrtRes* pChild = mpChildrenArray[i];
        if (pChild != NULL) {
            pChild->SetFrame(frame);
        }
    }
}

/* 0x8006B840 (0x1C4): sets every child's update rate. */
void nw4r::g3d::AnmObjTexSrtNode::SetUpdateRate(f32 rate)
{
    TEXSRT_POINTER_ASSERT(this, 0x12E, TEXSRT_THIS_MSG);
    if (!texsrt_value_valid(rate)) {
        nw4r::db::Panic("g3d_anmtexsrt.cpp", 0x12F, "NW4R:Floating Point Value Error(%f)\nrate is infinite or nan.", rate);
    }
    for (int i = 0; i < mChildrenArraySize; i++) {
        AnmObjTexSrtRes* pChild = mpChildrenArray[i];
        if (pChild != NULL) {
            pChild->SetUpdateRate(rate);
        }
    }
}

/* 0x8006B6D8 (0x168): returns the first child's frame, 0 without children. */
f32 nw4r::g3d::AnmObjTexSrtNode::GetFrame() const
{
    TEXSRT_POINTER_ASSERT(this, 0x11E, TEXSRT_THIS_MSG);
    for (int i = 0; i < mChildrenArraySize; i++) {
        if (mpChildrenArray[i] != NULL) {
            return mpChildrenArray[i]->GetFrame();
        }
    }
    return 0.0f;
}

/* 0x8006BA04 (0x168): returns the first child's update rate, 1 without children. */
f32 nw4r::g3d::AnmObjTexSrtNode::GetUpdateRate() const
{
    TEXSRT_POINTER_ASSERT(this, 0x13D, TEXSRT_THIS_MSG);
    for (int i = 0; i < mChildrenArraySize; i++) {
        if (mpChildrenArray[i] != NULL) {
            return mpChildrenArray[i]->GetUpdateRate();
        }
    }
    return 1.0f;
}

/* 0x8006BB6C (0x210): binds every child to `mdl` and marks the materials they define bound. */
bool nw4r::g3d::AnmObjTexSrtNode::Bind(ResMdl mdl)
{
    TEXSRT_POINTER_ASSERT(this, 0x14D, TEXSRT_THIS_MSG);
    if (!mdl.IsValid()) {
        nw4r::db::Panic("g3d_anmtexsrt.cpp", 0x14E, "NW4R:Failed assertion resMdl.IsValid()");
    }
    bool result = false;
    for (int i = 0; i < mChildrenArraySize; i++) {
        AnmObjTexSrtRes* pChild = mpChildrenArray[i];
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

/* 0x8006BD7C (0x16C): releases every child and the node's bindings. */
void nw4r::g3d::AnmObjTexSrtNode::Release()
{
    TEXSRT_POINTER_ASSERT(this, 0x172, TEXSRT_THIS_MSG);
    for (int i = 0; i < mChildrenArraySize; i++) {
        AnmObjTexSrtRes* pChild = mpChildrenArray[i];
        if (pChild != NULL) {
            pChild->Release();
        }
    }
    AnmObjTexSrt::Release();
}

/* 0x8006BEE8 (0x250): handles a child detaching itself and the node being attached to or detached from a
 * parent. */
void nw4r::g3d::AnmObjTexSrtNode::G3dProc(u32 task, u32 param, void* pInfo) /* untyped: caller-owned payload */
{
    TEXSRT_POINTER_ASSERT(this, 0x185, TEXSRT_THIS_MSG);
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
            nw4r::db::Warning("g3d_anmtexsrt.cpp", 0x19A,
                              "AnmObjTexSrtNode::G3dProc(G3DPROC_CHILD_DETACHED,%p,%p)\nThe child not found.", param,
                              pInfo);
        }
        break;
    }
    case G3DPROC_DETACH_PARENT:
        if (!GetParent()) {
            nw4r::db::Panic("g3d_anmtexsrt.cpp", 0x1A0, "NW4R:Failed assertion GetParent()");
        }
        SetParent(NULL);
        break;
    case G3DPROC_ATTACH_PARENT:
        if (GetParent()) {
            nw4r::db::Panic("g3d_anmtexsrt.cpp", 0x1A7, "NW4R:Failed assertion !GetParent()");
        }
        SetParent(static_cast<G3dObj*>(pInfo));
        break;
    }
}

/* ------------------------------------------------------------------------------------------------ */
/* The run-time type members of the texture-SRT classes                                             */
/* ------------------------------------------------------------------------------------------------ */

/* 0x8006E6C8 (0x6C): whether the object is an AnmObjTexSrt or derives from `type`. */
bool nw4r::g3d::AnmObjTexSrt::IsDerivedFrom(TypeObj type) const
{
    if (type == GetTypeObjStatic()) {
        return true;
    }
    return AnmObj::IsDerivedFrom(type);
}

/* 0x8006E734 (0x30): returns the AnmObjTexSrt type. */
const nw4r::g3d::G3dObj::TypeObj nw4r::g3d::AnmObjTexSrt::GetTypeObjStatic()
{
    void* pName;
    return *reinterpret_cast<const TypeObj*>(type_obj_set_name_anmchr(&pName, lbl_8056F5C0));
}

/* 0x8006E764 (0x30): returns the type. */
const nw4r::g3d::G3dObj::TypeObj nw4r::g3d::AnmObjTexSrtNode::GetTypeObj() const
{
    const u8* pName;
    return *reinterpret_cast<const TypeObj*>(type_obj_set_name_texsrt_node(&pName, anm_typename_AnmObjTexSrtNode));
}

/* 0x8006E794 (0x38): returns the type's name. */
const char* nw4r::g3d::AnmObjTexSrtNode::GetTypeName() const
{
    return GetTypeObj().GetTypeName();
}

/* 0x8006E7CC (0x6C): whether the object is an AnmObjTexSrtNode or derives from `type`. */
bool nw4r::g3d::AnmObjTexSrtNode::IsDerivedFrom(TypeObj type) const
{
    if (type == GetTypeObjStatic()) {
        return true;
    }
    return AnmObjTexSrt::IsDerivedFrom(type);
}

/* 0x8006E838 (0x30): returns the AnmObjTexSrtNode type. */
const nw4r::g3d::G3dObj::TypeObj nw4r::g3d::AnmObjTexSrtNode::GetTypeObjStatic()
{
    const u8* pName;
    return *reinterpret_cast<const TypeObj*>(type_obj_set_name_texsrt_node(&pName, anm_typename_AnmObjTexSrtNode));
}

/* 0x8006E868 (0x5C): destroys the override node. */
nw4r::g3d::AnmObjTexSrtOverride::~AnmObjTexSrtOverride()
{
}

/* 0x8006E8C4 (0x30): returns the type. */
const nw4r::g3d::G3dObj::TypeObj nw4r::g3d::AnmObjTexSrtOverride::GetTypeObj() const
{
    const u8* pName;
    return *reinterpret_cast<const TypeObj*>(
        type_obj_set_name_texsrt_override(&pName, anm_typename_AnmObjTexSrtOverride));
}

/* 0x8006E8F4 (0x38): returns the type's name. */
const char* nw4r::g3d::AnmObjTexSrtOverride::GetTypeName() const
{
    return GetTypeObj().GetTypeName();
}

/* 0x8006E92C (0x6C): whether the object is an AnmObjTexSrtOverride or derives from `type`. */
bool nw4r::g3d::AnmObjTexSrtOverride::IsDerivedFrom(TypeObj type) const
{
    if (type == GetTypeObjStatic()) {
        return true;
    }
    return AnmObjTexSrtNode::IsDerivedFrom(type);
}

/* 0x8006E998 (0x30): returns the AnmObjTexSrtOverride type. */
const nw4r::g3d::G3dObj::TypeObj nw4r::g3d::AnmObjTexSrtOverride::GetTypeObjStatic()
{
    const u8* pName;
    return *reinterpret_cast<const TypeObj*>(
        type_obj_set_name_texsrt_override(&pName, anm_typename_AnmObjTexSrtOverride));
}

/* 0x8006E9C8 (0x5C): destroys the resource animation. */
nw4r::g3d::AnmObjTexSrtRes::~AnmObjTexSrtRes()
{
}

/* 0x8006EA24 (0x6C): whether the object is an AnmObjTexSrtRes or derives from `type`. */
bool nw4r::g3d::AnmObjTexSrtRes::IsDerivedFrom(TypeObj type) const
{
    if (type == GetTypeObjStatic()) {
        return true;
    }
    return AnmObjTexSrt::IsDerivedFrom(type);
}

/* 0x8006EA90 (0x30): returns the AnmObjTexSrtRes type. */
const nw4r::g3d::G3dObj::TypeObj nw4r::g3d::AnmObjTexSrtRes::GetTypeObjStatic()
{
    const u8* pName;
    return *reinterpret_cast<const TypeObj*>(type_obj_set_name_texsrt_res(&pName, anm_typename_AnmObjTexSrtRes));
}
