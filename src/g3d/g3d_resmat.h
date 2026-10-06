/* g3d/g3d_resmat.h - nw4r g3d's material resources (`ResMat` and its texture, TLUT, texture-SRT, channel, tev
 *   colour, pixel, indirect-matrix, misc, fur and texture-palette parts) and the `ResMdl` model resource whose
 *   members `g3d/g3d_resmat.cpp` defines. */
#ifndef MHTRI_NW4R_G3D_G3D_RESMAT_H
#define MHTRI_NW4R_G3D_G3D_RESMAT_H

#include "types.h"
#include "gx.h"
#include "nw4r/math.h"
#include "g3d/g3d_rescommon.h"

/* The SDK signed colour.  size: 0x8 */
struct GXColorS10 {
    /* +0x0 */ s16 r;
    /* +0x2 */ s16 g;
    /* +0x4 */ s16 b;
    /* +0x6 */ s16 a;

    GXColorS10& operator=(const GXColorS10& rhs);
};

namespace nw4r {
namespace g3d {

class ResFile;
class ResTex;
class ResPltt;
class ResNode;
class ResShp;
class ResTev;
class ResVtxPos;
class ResVtxNrm;
class ResVtxClr;
class ResVtxTexCoord;
class ResVtxFurVec;
class ResVtxFurPos;

/* The eight texture objects of a material and the bitmask of the ones in use.  size: 0x104 */
struct ResTexObjData {
    /* +0x000 */ u32 flagUsedTexMapID;
    /* +0x004 */ GXTexObj texObj[8];
};

/* The eight TLUT objects of a material and the bitmask of the ones in use.  size: 0x64 */
struct ResTlutObjData {
    /* +0x00 */ u32 flagUsedTlutID;
    /* +0x04 */ GXTlutObj tlutObj[8];
};

/* One texture coordinate transform.  size: 0x14 */
struct TexSrt {
    /* +0x00 */ f32 Su;
    /* +0x04 */ f32 Sv;
    /* +0x08 */ f32 R;
    /* +0x0C */ f32 Tu;
    /* +0x10 */ f32 Tv;
};

/* One texture coordinate's effect matrix and how it is mapped.  size: 0x34 */
struct TexMtxEffect {
    /* +0x00 */ s8 ref_camera;  /* the camera the projection follows, -1 = none */
    /* +0x01 */ s8 ref_light;   /* the light the projection follows, -1 = none */
    /* +0x02 */ u8 map_mode;
    /* +0x03 */ u8 misc_flag;   /* bit 0: the effect matrix is the identity */
    /* +0x04 */ math::MTX34 effectMtx;
};

/* The eight texture coordinate transforms of a material.  size: 0x248 */
struct ResTexSrtData {
    /* +0x000 */ u32 flagTexSrt;   /* four bits per coordinate */
    /* +0x004 */ u32 texMtxMode;
    /* +0x008 */ TexSrt texSrt[8];
    /* +0x0A8 */ TexMtxEffect effect[8];
};

/* The material's GX general-mode counts and cull mode.  size: 0x8 */
struct ResGenModeData {
    /* +0x0 */ u8 nTexGens;
    /* +0x1 */ u8 nChans;
    /* +0x2 */ u8 nTevs;
    /* +0x3 */ u8 nInds;
    /* +0x4 */ _GXCullMode cullMode;

    ResGenModeData& operator=(const ResGenModeData& rhs);
};

/* The material's z-compare location, light set, fog and the indirect methods.  size: 0xC */
struct ResMatMiscData {
    /* The indirect texturing methods. */
    enum IndirectMethod {
        WARP,
        NORMAL_MAP,
        NORMAL_MAP_SPEC,
        FUR,
        RESERVE0,
        RESERVE1,
        USER0,
        USER1,
        NUM_OF_INDIRECT_METHOD
    };

    /* +0x0 */ u8 zCompLoc;
    /* +0x1 */ s8 light_set_idx;
    /* +0x2 */ s8 fog_idx;
    /* +0x3 */ u8 pad_0x3;
    union {
        /* +0x4 */ u8 indirect_method[4];   /* indexed by GXIndTexMtxID - GX_ITM_0 */
        /* +0x4 */ u32 indirect_method_word;
    };
    union {
        /* +0x8 */ s8 normal_map_ref[4];    /* the light a normal-mapped indirect stage follows, -1 = none */
        /* +0x8 */ u32 normal_map_ref_word;
    };

    ResMatMiscData& operator=(const ResMatMiscData& rhs);
};

/* One colour channel's colours and control words.  size: 0x14 */
struct ResChanEntry {
    /* +0x00 */ u32 flag;
    /* +0x04 */ GXColor matColor;
    /* +0x08 */ GXColor ambColor;
    /* +0x0C */ u32 paramChanCtrlC;   /* the XF channel-control word of the colour half */
    /* +0x10 */ u32 paramChanCtrlA;   /* the XF channel-control word of the alpha half */
};

/* The material's two colour channels.  size: 0x28 */
struct ResMatChanData {
    /* +0x00 */ ResChanEntry chan[2];

    ResMatChanData& operator=(const ResMatChanData& rhs);
};

/* The fur parameters of a material.  size: 0xC (a lower bound: only the words below are reached) */
struct ResMatFurData {
    /* +0x0 */ u32 pad_0x0;
    /* +0x4 */ u32 numLayer;
    /* +0x8 */ u32 layerRatioMode;   /* 0: linear, 1: a power curve */
};

/* One texture of a material and how it is sampled.  size: 0x34 */
struct ResTexPlttInfoData {
    /* +0x00 */ s32 nameTex;        /* offset to the texture's name, from this field */
    /* +0x04 */ s32 namePltt;       /* offset to the palette's name, from this field */
    /* +0x08 */ void* pTexData;     /* the bound texture block, NULL while unbound */ /* untyped: opaque handle */
    /* +0x0C */ void* pPlttData;    /* the bound palette block, NULL while unbound */ /* untyped: opaque handle */
    /* +0x10 */ u32 mapID;          /* GXTexMapID */
    /* +0x14 */ u32 tlutID;         /* GXTlut */
    /* +0x18 */ u32 wrap_s;
    /* +0x1C */ u32 wrap_t;
    /* +0x20 */ u32 min_filt;
    /* +0x24 */ u32 mag_filt;
    /* +0x28 */ f32 lod_bias;
    /* +0x2C */ u32 max_aniso;
    /* +0x30 */ u8 bias_clamp;
    /* +0x31 */ u8 do_edge_lod;
    /* +0x32 */ u8 pad_0x32[2];
};

/* The material block.  size: 0x418 */
struct ResMatData {
    /* +0x000 */ u32 size;
    /* +0x004 */ s32 toResMdlData;
    /* +0x008 */ s32 name;
    /* +0x00C */ u32 id;
    /* +0x010 */ u32 flag;
    /* +0x014 */ ResGenModeData genMode;
    /* +0x01C */ ResMatMiscData misc;
    /* +0x028 */ s32 toResTevData;
    /* +0x02C */ u32 numResTexPlttInfo;
    /* +0x030 */ s32 toResTexPlttInfo;
    /* +0x034 */ s32 toResMatFurData;
    /* +0x038 */ s32 toResUserData;
    /* +0x03C */ s32 toResMatDLData;
    /* +0x040 */ ResTexObjData texObjData;
    /* +0x144 */ ResTlutObjData tlutObjData;
    /* +0x1A8 */ ResTexSrtData texSrtData;
    /* +0x3F0 */ ResMatChanData chan;
};

/* The model block: the dictionaries of its sub-resources.  size: 0x48 (a lower bound) */
struct ResMdlData {
    /* +0x00 */ u32 signature;
    /* +0x04 */ u32 size;
    /* +0x08 */ u32 revision;
    /* +0x0C */ s32 toResFileData;
    /* +0x10 */ s32 toResByteCodeDic;
    /* +0x14 */ s32 toResNodeDic;
    /* +0x18 */ s32 toResVtxPosDic;
    /* +0x1C */ s32 toResVtxNrmDic;
    /* +0x20 */ s32 toResVtxClrDic;
    /* +0x24 */ s32 toResVtxTexCoordDic;
    /* +0x28 */ s32 toResVtxFurVecDic;
    /* +0x2C */ s32 toResVtxFurPosDic;
    /* +0x30 */ s32 toResMatDic;
    /* +0x34 */ s32 toResTevDic;
    /* +0x38 */ s32 toResShpDic;
    /* +0x3C */ s32 toResTexNameToTexPlttInfoDic;
    /* +0x40 */ s32 toResPlttNameToTexPlttInfoDic;
    /* +0x44 */ s32 toResUserData;
};

/* The display-list block of a material's pixel state (alpha compare, z mode, blend).  size: 0x20 (a lower bound) */
struct ResMatPixData;
/* The display-list block of a material's tev colours.  size: 0x60 (a lower bound) */
struct ResMatTevColorData;
/* The display-list block of a material's indirect matrices.  size: 0x40 (a lower bound) */
struct ResMatIndMtxAndScaleData;
/* The display-list block of a material's texture-coordinate generators.  size: 0xA0 (a lower bound) */
struct ResMatTexCoordGenData;

/* size: 0x4 */
class ResTexObj : public ResCommon<ResTexObjData> {
public:
    NW4R_G3D_RESOURCE_FUNC_DECL(ResTexObj)
    ResTexObj& operator=(const ResTexObj& rhs);

    GXTexObj* GetTexObj(_GXTexMapID id);
    const GXTexObj* GetTexObj(_GXTexMapID id) const;
    bool IsValidTexObj(_GXTexMapID id) const;
    void Validate(_GXTexMapID id);
    void Invalidate(_GXTexMapID id);
    ResTexObj CopyTo(void* pDst) const; /* untyped: byte range */
};

/* size: 0x4 */
class ResTlutObj : public ResCommon<ResTlutObjData> {
public:
    NW4R_G3D_RESOURCE_FUNC_DECL(ResTlutObj)
    ResTlutObj& operator=(const ResTlutObj& rhs);

    GXTlutObj* GetTlut(_GXTlut id);
    const GXTlutObj* GetTlut(_GXTlut id) const;
    bool IsValidTlut(_GXTlut id) const;
    void Validate(_GXTlut id);
    void Invalidate(_GXTlut id);
    ResTlutObj CopyTo(void* pDst) const; /* untyped: byte range */
};

/* A one-word handle on a `ResTexSrtData`; default-constructible because the material accessors fill it.
 * size: 0x4 */
class ResTexSrt {
public:
    ResTexSrt() {}
    explicit ResTexSrt(void* pData); /* untyped: opaque handle */

    /* The `id`-th effect matrix slot: copy `pMtx` in (or clear it when null) and set the present bit.
     * `id` outside [0, 8) is a no-op that returns false. */
    bool SetEffectMtx(u32 id, const nw4r::math::MTX34* pMtx);
    /* The const twin: copy the `id`-th slot out. */
    bool GetEffectMtx(u32 id, nw4r::math::MTX34* pMtx) const;
    bool SetMapMode(u32 id, u32 mapMode, int refCamera, int refLight);
    bool GetMapMode(u32 id, u32* pMapMode, int* pRefCamera, int* pRefLight) const;
    ResTexSrt CopyTo(void* pDst) const; /* untyped: byte range */
    ResTexSrt& operator=(const ResTexSrt& rhs);

    static const char* GetClassName();
    bool IsValid() const;
    ResTexSrtData* ptr();
    ResTexSrtData& ref();
    const ResTexSrtData& ref() const;

    /* +0x00 */ void* mpData; /* untyped: opaque handle */
};

/* size: 0x4 */
class ResGenMode : public ResCommon<ResGenModeData> {
public:
    NW4R_G3D_RESOURCE_FUNC_DECL(ResGenMode)
    ResGenMode& operator=(const ResGenMode& rhs);

    void GXSetNumTevStages(u8 nStages);
    void GXSetCullMode(_GXCullMode cullMode);
    ResGenMode CopyTo(void* pDst) const; /* untyped: byte range */
    u8 GXGetNumTexGens() const;
    u8 GXGetNumChans() const;
    u8 GXGetNumTevStages() const;
    u8 GXGetNumIndStages() const;
    _GXCullMode GXGetCullMode() const;
};

/* size: 0x4 */
class ResMatMisc : public ResCommon<ResMatMiscData> {
public:
    NW4R_G3D_RESOURCE_FUNC_DECL(ResMatMisc)
    ResMatMisc& operator=(const ResMatMisc& rhs);

    u8 GXGetZCompLoc() const;
    void SetLightSetIdx(int idx);
    int GetLightSetIdx() const;
    int GetFogIdx() const;
    void SetIndirectMethod(_GXIndTexMtxID id, ResMatMiscData::IndirectMethod method, s8 normalMapRef);
    void GetIndirectMethod(_GXIndTexMtxID id, ResMatMiscData::IndirectMethod* pMethod, s8* pNormalMapRef) const;
    ResMatMisc CopyTo(void* pDst) const; /* untyped: byte range */
};

/* size: 0x4 */
class ResMatFur : public ResCommon<ResMatFurData> {
public:
    NW4R_G3D_RESOURCE_FUNC_DECL(ResMatFur)

    f32 GetLayerRatio(u32 layer) const;
};

/* size: 0x4 */
class ResMatPix : public ResCommon<ResMatPixData> {
public:
    NW4R_G3D_RESOURCE_FUNC_DECL(ResMatPix)
    ResMatPix& operator=(const ResMatPix& rhs);

    bool GXGetAlphaCompare(_GXCompare* pComp0, u8* pRef0, _GXAlphaOp* pOp, _GXCompare* pComp1, u8* pRef1) const;
    void GXSetAlphaCompare(_GXCompare comp0, u8 ref0, _GXAlphaOp op, _GXCompare comp1, u8 ref1);
    bool GXGetBlendMode(_GXBlendMode* pType, _GXBlendFactor* pSrcFactor, _GXBlendFactor* pDstFactor,
                        _GXLogicOp* pOp) const;
    void GXSetBlendMode(_GXBlendMode type, _GXBlendFactor srcFactor, _GXBlendFactor dstFactor, _GXLogicOp op);
    void CallDisplayList(bool bSync) const;
};

/* size: 0x4 */
class ResMatTevColor : public ResCommon<ResMatTevColorData> {
public:
    NW4R_G3D_RESOURCE_FUNC_DECL(ResMatTevColor)
    ResMatTevColor& operator=(const ResMatTevColor& rhs);

    bool GXGetTevColor(_GXTevRegID id, GXColor* pColor) const;
    void GXSetTevColor(_GXTevRegID id, GXColor color);
    bool GXGetTevKColor(_GXTevKColorID id, GXColor* pColor) const;
    void GXSetTevKColor(_GXTevKColorID id, GXColor color);
};

/* size: 0x4 */
class ResMatIndMtxAndScale : public ResCommon<ResMatIndMtxAndScaleData> {
public:
    NW4R_G3D_RESOURCE_FUNC_DECL(ResMatIndMtxAndScale)
    ResMatIndMtxAndScale& operator=(const ResMatIndMtxAndScale& rhs);

    bool GXGetIndTexMtx(_GXIndTexMtxID id, math::MTX34* pMtx) const;
    bool GXGetIndTexMtx(_GXIndTexMtxID id, math::MTX34* pMtx, s8* pScaleExp) const;
    void GXSetIndTexMtx(_GXIndTexMtxID id, const math::MTX34& mtx, s8 scaleExp);
    void CallDisplayList(u8 indNum, bool bSync) const;
};

/* size: 0x4 */
class ResMatChan : public ResCommon<ResMatChanData> {
public:
    NW4R_G3D_RESOURCE_FUNC_DECL(ResMatChan)
    ResMatChan& operator=(const ResMatChan& rhs);

    void GXSetChanMatColor(u32 chan, GXColor color);
    void GXSetChanAmbColor(u32 chan, GXColor color);
    bool GXGetChanMatColor(u32 chan, GXColor* pColor) const;
    bool GXGetChanAmbColor(u32 chan, GXColor* pColor) const;
    bool GXGetChanCtrl(u32 chan, u8* pEnable, u32* pAmbSrc, u32* pMatSrc, u32* pLightMask, u32* pDiffFn,
                       u32* pAttnFn) const;
    ResMatChan CopyTo(void* pDst) const; /* untyped: byte range */
};

/* size: 0x4 */
class ResMatTexCoordGen : public ResCommon<ResMatTexCoordGenData> {
public:
    NW4R_G3D_RESOURCE_FUNC_DECL(ResMatTexCoordGen)
    ResMatTexCoordGen& operator=(const ResMatTexCoordGen& rhs);

    void CallDisplayList(u8 numGens, bool bSync) const;
};

/* size: 0x4 */
class ResTexPlttInfo : public ResCommon<ResTexPlttInfoData> {
public:
    NW4R_G3D_RESOURCE_FUNC_DECL(ResTexPlttInfo)

    ResName GetTexName() const;
    ResName GetPlttName() const;
    bool IsCIFmt() const;
    void BindTex(ResTex tex, ResTexObj texObj);
    void BindPltt(ResPltt pltt, ResTlutObj tlutObj);
    bool Bind(ResFile file, ResTexObj texObj, ResTlutObj tlutObj);
    void Release(ResTexObj texObj, ResTlutObj tlutObj);
};

/* size: 0x4 */
class ResMat : public ResCommon<ResMatData> {
public:
    NW4R_G3D_RESOURCE_FUNC_DECL(ResMat)

    bool Bind(ResFile file);
    ResTexObj GetResTexObj();
    ResTlutObj GetResTlutObj();
    void* GetResMatDLData(); /* untyped: byte range */
    u32 GetNumResTexPlttInfo() const;
    ResTexPlttInfo GetResTexPlttInfo(u32 idx);
    void Release();
    void Init();
    ResTev GetResTev();
    ResMatFur GetResMatFur();
    ResMatChan GetResMatChan();
    ResGenMode GetResGenMode();
    ResMatMisc GetResMatMisc();
    ResTexSrt GetResTexSrt();
    ResMatPix GetResMatPix();
    ResMatTevColor GetResMatTevColor();
    ResMatIndMtxAndScale GetResMatIndMtxAndScale();
    ResMatTexCoordGen GetResMatTexCoordGen();
};

/* size: 0x4 */
class ResMdl : public ResCommon<ResMdlData> {
public:
    NW4R_G3D_RESOURCE_FUNC_DECL(ResMdl)

    const u8* GetResByteCode(const char* pName) const;
    ResNode GetResNode(const char* pName) const;
    ResNode GetResNode(ResName name) const;
    ResNode GetResNode(int idx) const;
    ResNode GetResNode(u32 idx) const;
    u32 GetResNodeNumEntries() const;
    ResVtxPos GetResVtxPos(int idx) const;
    ResVtxPos GetResVtxPos(u32 idx) const;
    u32 GetResVtxPosNumEntries() const;
    ResVtxNrm GetResVtxNrm(int idx) const;
    ResVtxNrm GetResVtxNrm(u32 idx) const;
    u32 GetResVtxNrmNumEntries() const;
    ResVtxClr GetResVtxClr(int idx) const;
    ResVtxClr GetResVtxClr(u32 idx) const;
    u32 GetResVtxClrNumEntries() const;
    ResVtxTexCoord GetResVtxTexCoord(int idx) const;
    ResVtxTexCoord GetResVtxTexCoord(u32 idx) const;
    u32 GetResVtxTexCoordNumEntries() const;
    ResVtxFurVec GetResVtxFurVec(u32 idx) const;
    u32 GetResVtxFurVecNumEntries() const;
    ResVtxFurPos GetResVtxFurPos(int idx) const;
    ResVtxFurPos GetResVtxFurPos(u32 idx) const;
    u32 GetResVtxFurPosNumEntries() const;
    ResMat GetResMat(ResName name) const;
    ResMat GetResMat(int idx) const;
    ResMat GetResMat(u32 idx) const;
    u32 GetResMatNumEntries() const;
    ResShp GetResShp(int idx) const;
    ResShp GetResShp(u32 idx) const;
    u32 GetResShpNumEntries() const;
    bool Bind(const ResFile& file);
    void Release();
    void Init();
    void Terminate();
};

}  // namespace g3d
}  // namespace nw4r

#endif /* MHTRI_NW4R_G3D_G3D_RESMAT_H */
