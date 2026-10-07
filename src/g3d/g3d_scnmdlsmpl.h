/* g3d/g3d_scnmdlsmpl.h - nw4r g3d's `ScnMdlSimple` (a scene leaf drawing one model resource with its world, view
 *   and texture matrix arrays, byte codes and five animation slots) and the helpers it shares.
 *   Evidence: the ScnMdlSimple vtable (.data 0x8058F340: ScnLeaf's slots, then SetAnmObj, the two RemoveAnmObj
 *   and the two GetAnmObj at +0x34..+0x44), the constructor 0x80080C60 (the layout below, the byte-code names
 *   "NodeTree"/"NodeMix"/"DrawOpa"/"DrawXlu") and the asserts naming `mpAnmObjChr` .. `mpAnmObjTexSrt`.  The member
 *   names are nw4r's.  The destructor is declared first so the vtable is emitted with it (IsDerivedFrom's weak copy
 *   sits in `g3d/g3d_scnmdl.cpp`'s range).
 */
#ifndef MHTRI_G3D_G3D_SCNMDLSMPL_H
#define MHTRI_G3D_G3D_SCNMDLSMPL_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x8056F688 - the "ScnMdlSimple" type-name record (`.rodata`: a length word, then the NUL-terminated name) the
 * class's run-time type members read. */
extern u8 scn_typename_ScnMdlSimple[];
/* The callback-timing setter and the world-callback destructor the game's model users call (C linkage while
 * their consumers spell the stems). */
void fn_80080B10(void* arg0, u32 arg1);
void fn_800810DC(void* arg0, s32 arg1);

#ifdef __cplusplus
}

#include "nw4r/fn_805012C4.h" /* nw4r::math::AABB, owner nw4r/fn_805012C4.cpp (rule 2) */
#include "g3d/g3d_scnobj.h"   /* nw4r::g3d::ScnLeaf, owner g3d/g3d_scnobj.cpp (rule 1) */
#include "g3d/g3d_resmat.h"   /* nw4r::g3d::ResMdl (rule 2) */

/* 0x80080F44 - constructs an AABB's two corner records (two VEC3_ctor no-ops) and returns it: the element
 * constructor ScnObj's bounding-box array is built with. */
extern "C" nw4r::math::AABB* AABB_ctor(nw4r::math::AABB* pBox);

/* The fields of a model's `ResMdlInfo` block the ScnMdlSimple and ScnMdl constructors and Constructs read.  size: 0x40 (approximation: only
 * the read fields are named) */
struct ScnMdlResMdlInfoData {
    /* +0x00 */ u8 pad_0x00[0x20];
    /* +0x20 */ u8 needNrmMtxArray;
    /* +0x21 */ u8 needTexMtxArray;
    /* +0x22 */ u8 isValidVolume;
    /* +0x23 */ u8 pad_0x23[0x5];
    /* +0x28 */ nw4r::math::VEC3 volumeMin;
    /* +0x34 */ nw4r::math::VEC3 volumeMax;
};

namespace nw4r {
namespace g3d {

class AnmObjChr;
class AnmObjVis;
class AnmObjMatClr;
class AnmObjTexPat;
class AnmObjTexSrt;

/* The world-matrix callback a model runs for one node (its interface is implemented by the game).  size: 0x4 */
class ICalcWorldCallback {
public:
    virtual ~ICalcWorldCallback();
};

/* The callback, timing mask and node a world-matrix pass hands the calculator.  size: 0x8 */
struct FuncObjCalcWorld {
    FuncObjCalcWorld(ICalcWorldCallback* pCallback, u32 timing, u32 nodeID);

    /* +0x0 */ ICalcWorldCallback* mpCallback;
    /* +0x4 */ u8 mTiming;
    /* +0x5 */ u8 pad_0x05;
    /* +0x6 */ u16 mNodeID;
};

/* size: 0x138 */
class ScnMdlSimple : public ScnLeaf {
public:
    /* The five animation slots (SetAnmObj / RemoveAnmObj / GetAnmObj). */
    enum AnmObjType {
        ANMOBJTYPE_CHR = 0,
        ANMOBJTYPE_VIS = 1,
        ANMOBJTYPE_MATCLR = 2,
        ANMOBJTYPE_TEXPAT = 3,
        ANMOBJTYPE_TEXSRT = 4,
        ANMOBJTYPE_SHP = 5,
        ANMOBJTYPE_NOT_SPECIFIED = 6
    };

    ScnMdlSimple(MEMAllocator* pHeap, ResMdl mdl, math::MTX34* pWorldMtxArray, u32* pWorldMtxAttribArray,
                 math::MTX34* pViewPosMtxArray, math::MTX33* pViewNrmMtxArray, math::MTX34* pViewTexMtxArray,
                 int numView, int numViewMtx);

    virtual ~ScnMdlSimple();
    virtual bool IsDerivedFrom(TypeObj type) const;
    virtual void G3dProc(u32 task, u32 param, void* pInfo); /* untyped: caller-owned payload */
    virtual const TypeObj GetTypeObj() const;
    virtual const char* GetTypeName() const;
    virtual bool SetScnObjOption(u32 option, u32 value);
    virtual bool GetScnObjOption(u32 option, u32* pValue) const;
    virtual bool SetAnmObj(AnmObj* pObj, AnmObjType type);
    virtual bool RemoveAnmObj(AnmObj* pObj);
    virtual AnmObj* RemoveAnmObj(AnmObjType type);
    virtual AnmObj* GetAnmObj(AnmObjType type);
    virtual const AnmObj* GetAnmObj(AnmObjType type) const;

    static ScnMdlSimple* Construct(MEMAllocator* pHeap, u32* pSize, ResMdl mdl, int numView);
    static const TypeObj GetTypeObjStatic();

    ResMdl GetResMdl();
    math::MTX34* GetWldMtxArray();
    u32* GetWldMtxAttribArray();
    math::MTX34* GetViewPosMtxArray();
    math::MTX33* GetViewNrmMtxArray();
    math::MTX34* GetViewTexMtxArray();
    u32 GetNumViewMtx() const;
    const u8* GetByteCodeCalc();
    const u8* GetByteCodeMix();
    const u8* GetByteCodeDrawOpa();
    const u8* GetByteCodeDrawXlu();
    u32 GetDrawMode() const;
    ICalcWorldCallback* GetCalcWorldCallback();
    u8 GetCalcWorldCallbackTiming();
    u32 GetCalcWorldNodeID();
    AnmObjChr* GetAnmObjChr();
    AnmObjVis* GetAnmObjVis();
    AnmObjMatClr* GetAnmObjMatClr();
    AnmObjTexPat* GetAnmObjTexPat();
    AnmObjTexSrt* GetAnmObjTexSrt();
    void UpdateFrame();
    /* untyped: caller-owned payload - the pass's info block */
    void G3dProcUpdateFrame(u32 param, void* pInfo);
    void CalcPosture(u32 param, const math::MTX34* pParent);
    void CalcSkinning();
    void G3dProcGatherScnObj(u32 param, IScnObjGather* pGather);
    void G3dProcCalcWorld(u32 param, const math::MTX34* pParent);
    /* untyped: caller-owned payload - the pass's info block */
    void G3dProcCalcMat(u32 param, void* pInfo);
    void G3dProcCalcView(u32 param, const math::MTX34* pCamera);
    void G3dProcDrawOpa(u32 param, const u32* pDrawMode);
    void G3dProcDrawXlu(u32 param, const u32* pDrawMode);

    /* +0xE8 */ ResMdl mResMdl;
    /* +0xEC */ math::MTX34* mpWorldMtxArray;
    /* +0xF0 */ u32* mpWorldMtxAttribArray;
    /* +0xF4 */ math::MTX34* mpViewPosMtxArray;
    /* +0xF8 */ math::MTX33* mpViewNrmMtxArray;
    /* +0xFC */ math::MTX34* mpViewTexMtxArray;
    /* +0x100 */ u8 mNumView;
    /* +0x101 */ u8 mCurView;
    /* +0x102 */ u16 mNumViewMtx;
    /* +0x104 */ u32 mFlagScnMdlSimple;              /* bit 0: the world matrices are calculated in the locked cache */
    /* +0x108 */ const u8* mpByteCodeCalc;
    /* +0x10C */ const u8* mpByteCodeMix;
    /* +0x110 */ const u8* mpByteCodeDrawOpa;
    /* +0x114 */ const u8* mpByteCodeDrawXlu;
    /* +0x118 */ u32 mDrawMode;
    /* +0x11C */ ICalcWorldCallback* mpCalcWorldCallback;
    /* +0x120 */ u8 mCalcWorldCallbackTiming;
    /* +0x121 */ u8 mCalcWorldCallbackDeleteOption;
    /* +0x122 */ u16 mCalcWorldNodeID;
    /* +0x124 */ AnmObjChr* mpAnmObjChr;
    /* +0x128 */ AnmObjVis* mpAnmObjVis;
    /* +0x12C */ AnmObjMatClr* mpAnmObjMatClr;
    /* +0x130 */ AnmObjTexPat* mpAnmObjTexPat;
    /* +0x134 */ AnmObjTexSrt* mpAnmObjTexSrt;
};

}  // namespace g3d
}  // namespace nw4r
#endif

#endif /* MHTRI_G3D_G3D_SCNMDLSMPL_H */
