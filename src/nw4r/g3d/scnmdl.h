/*
 * nw4r/g3d/scnmdl.h - nw4r g3d's `ScnMdl`: a ScnMdlSimple with a shape animation, a node-visibility buffer and the
 *   replacement buffers that let a model draw copied materials (`ScnMdl::CopiedMatAccess`).
 *
 * Evidence: the ScnMdl vtable (.data 0x8058F028: ScnMdlSimple's slots with ScnMdl's IsDerivedFrom, G3dProc,
 * destructor, type members, options, SetAnmObj, the two RemoveAnmObj and the two GetAnmObj), the constructor
 * 0x8007ED58 (the shape animation at +0x138, the flag word at +0x13C, the material-buffer flags at +0x140, the
 * 0x40-byte replacement record at +0x144 and the buffer option at +0x184) and the `mReplacement.*Array` asserts.
 * `__ct__Q44nw4r3g3d6ScnMdl15CopiedMatAccessFPQ34nw4r3g3d6ScnMdlUl` (the map's mangling) is
 * `nw4r::g3d::ScnMdl::CopiedMatAccess::CopiedMatAccess(ScnMdl*, unsigned long)`.  The destructor is declared first so
 * the vtable is emitted with it.
 */
#ifndef MHTRI_NW4R_G3D_SCNMDL_H
#define MHTRI_NW4R_G3D_SCNMDL_H

#include "types.h"
#include "g3d/g3d_scnmdlsmpl.h" /* nw4r::g3d::ScnMdlSimple, owner g3d/g3d_scnmdlsmpl.cpp (rule 1) */

namespace nw4r {
namespace g3d {

/*
 * The 0x40-byte replacement record the ScnMdl constructor copies in, word by word, from its caller's argument.  The
 * range reads its flag word (bit 0: the vertex tables need refreshing), the node-visibility byte array at +0x04
 * (UpdateVisBuffer writes one byte per node, the world pass hands it to the visibility walker), the eleven
 * per-material replacement arrays (the asserts name pixDLArray .. tevDataArray; the per-material sizes come from
 * Construct 0x8007C540) and the three per-array vertex tables at +0x34..+0x3C, which the vertex pass hands the
 * shape-blend driver and RemoveAnmObj refills from the resource.
 */
struct ReplacementBlock {
    /* +0x00 */ u32 mFlag;
    /* +0x04 */ u8* mpNodeVisible;
    /* +0x08 */ void* mpTexObjDataArray;       /* 0x104 bytes per material */
    /* +0x0C */ void* mpTlutObjDataArray;      /* 0x64 bytes per material */
    /* +0x10 */ void* mpTexSrtDataArray;       /* 0x248 bytes per material */
    /* +0x14 */ void* mpChanDataArray;         /* 0x28 bytes per material */
    /* +0x18 */ void* mpGenModeDataArray;      /* 8 bytes per material */
    /* +0x1C */ void* mpMatMiscDataArray;      /* 0xC bytes per material */
    /* +0x20 */ void* mpPixDLArray;            /* 0x20 bytes per material */
    /* +0x24 */ void* mpTevColorDLArray;       /* 0x80 bytes per material */
    /* +0x28 */ void* mpIndMtxAndScaleDLArray; /* 0x40 bytes per material */
    /* +0x2C */ void* mpTexCoordGenDLArray;    /* 0xA0 bytes per material */
    /* +0x30 */ void* mpTevDataArray;          /* 0x200 bytes per material */
    /* +0x34 */ void** mpVtxPosTable;
    /* +0x38 */ void** mpVtxNrmTable;
    /* +0x3C */ void** mpVtxClrTable;
}; /* size: 0x40 */

/* size: 0x188 */
class ScnMdl : public ScnMdlSimple {
public:
    /* The material accessor the game's effect code builds on the stack.  size: 0x34 (approximation) */
    struct CopiedMatAccess {
        /* +0x00 */ u32 handle_0x00;
        /* +0x04 */ u8 pad_0x04[0x30];

        CopiedMatAccess(ScnMdl* mdl, u32 idx);

        /* 0x8007BBAC - the material handle accessor, defined by `g3d/fn_80075DCC.cpp` (rule 2 owner). */
        u32 GetResTexSrt(bool arg1);
    };

    ScnMdl(MEMAllocator* pHeap, ResMdl mdl, math::MTX34* pWorldMtxArray, u32* pWorldMtxAttribArray,
           math::MTX34* pViewPosMtxArray, math::MTX33* pViewNrmMtxArray, math::MTX34* pViewTexMtxArray, int numView,
           int numViewMtx, const ReplacementBlock* pReplacement, u32* pMatBufferFlags, u32 bufferOption);

    virtual ~ScnMdl();
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

    static const TypeObj GetTypeObjStatic();

    AnmObjShp* GetAnmObjShp();
    bool IsVisBufferRefreshNeeded() const;
    bool IsVisBufferEnabled() const;
    void UpdateVisBuffer();
    bool TestMatBufferFlag(u32 idx, u32 mask) const;
    void G3dProcCalcWorld(u32 param, const math::MTX34* pParent);
    /* untyped: caller-owned payload - the pass's info block */
    void G3dProcCalcVtx(u32 param, void* pInfo);
    /* untyped: caller-owned payload - the pass's info block */
    void G3dProcCalcMat(u32 param, void* pInfo);
    void G3dProcDrawOpa(u32 param, const u32* pDrawMode);
    void G3dProcDrawXlu(u32 param, const u32* pDrawMode);

    /* +0x138 */ AnmObjShp* mpAnmObjShp;
    /* +0x13C */ u32 mFlags;            /* bit 0: the visibility buffer needs a refresh; bit 1: the buffer is off */
    /* +0x140 */ u32* mpDLBuffer;       /* the per-material buffer flag words TestMatBufferFlag reads */
    /* +0x144 */ ReplacementBlock mReplacement;
    /* +0x184 */ u32 mBufferOption;     /* the constructor's last argument; never read by this unit */
};

}  // namespace g3d
}  // namespace nw4r

#endif /* MHTRI_NW4R_G3D_SCNMDL_H */
