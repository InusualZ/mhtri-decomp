/*
 * g3d/g3d_scnmdl.cpp - nw4r g3d `ScnMdl` scene model: its replaced-material (`mReplacement`) buffers, the node
 *   visibility and option accessors, the constructor and destructor, and the `ScnMdl` name-record cluster.
 * RANGE. .text 0x8007C540-0x8007F0E4 (55 functions); extab, extabindex, .rodata 0x8056F678-0x8056F688 (the "ScnMdl" name record), .data
 *   0x8058EDA0-0x8058F0A0 (opens on "g3d_scnmdl.cpp"), .sdata 0x80791208-0x80791210.  The left edge is `g3d/fn_80075DCC.cpp`'s cap, not a proven
 *   seam; tudiscover proves one TU through 0x8007EF1C, and the 0x8007EF1C-0x8007F0E4 tail is here because its head
 *   GetTypeObj reads the "ScnMdl" name record (scn_typename_ScnMdl), which only the class's own TU registers.
 * NAMES. res_gen_mode_end_edit is a GUESS; res_mat_misc_end_edit is a GUESS; res_mat_pix_end_edit is a GUESS;
 *   res_mat_tex_coord_gen_end_edit is a GUESS; res_tev_end_edit is a GUESS; res_tex_obj_common_copy_ctor is a GUESS;
 *   res_tex_obj_copy_ctor is a GUESS; res_tlut_obj_common_copy_ctor is a GUESS;
 *   res_tlut_obj_copy_ctor is a GUESS (the evidence follows).
 *   res_tev_end_edit, res_mat_tex_coord_gen_end_edit, res_mat_pix_end_edit, res_mat_misc_end_edit,
 *   res_gen_mode_end_edit, Construct, InitBuffer, HandleTemp, scn_mdl_psize_error_msg, res_tex_obj_copy_ctor, res_tlut_obj_copy_ctor (and their *_common_copy_ctor
 *   halves) and scn_mdl_clean_mat_buffer are GUESSES (nw4r's EndEdit hooks, by the DCStore each
 *   tail-calls; the per-material refill that copies each flagged block into its mReplacement buffer).  The ScnMdl members are nw4r's (`src/nw4r/g3d/scnmdl.h`: the vtable gives the virtual order, the asserts name
 *   `mpAnmObjShp` and the `mReplacement.*Array` buffers, the map rows carry the manglings).  G3dProcCalcWorld,
 *   G3dProcCalcMat, G3dProcCalcVtx, G3dProcDrawOpa, G3dProcDrawXlu, IsVisBufferRefreshNeeded, IsVisBufferEnabled,
 *   UpdateVisBuffer, TestMatBufferFlag and GetAnmObjShp are GUESSES (the per-pass helpers G3dProc dispatches to and the
 *   flag and buffer readers); `g3d_root_model_bind` (0x8007F0CC) is a GUESS (it appends an object to the scene
 *   root through ScnGroup::Insert at the child count).
 *   res_mdl_info_ref is a GUESS (0x8007D404: the ResMdlInfo handle's block, asserting the handle).  The ScnMdlSimple
 *   getters, G3dProcUpdateFrame and IsDerivedFrom defined here are its weak copies (members of
 *   `g3d/g3d_scnmdlsmpl.h`), as is AnmObj::IsBound; ScnGroup::PopBack and ScnGroup::Empty (0x8007F05C, 0x8007F0BC)
 *   are `g3d/g3d_scnobj.h`'s.
 * RESIDUALS. Partial: RemoveAnmObj(AnmObj*) (the vertex-position loop's counter and destination swap r27/r28; its
 *   extab saved-register byte differs with it).
 *   flipcheck: `.rodata` (the "ScnMdl" name record, declared not defined), `.data` and `.sdata` are claimed and not
 *   emitted.
 * SHAPES. The handle copies retail calls out of line (res_mat_copy_ctor, res_tex_obj_copy_ctor, ...) are explicit calls
 *   on `ResHandle` locals, declared in Construct/G3dProcCalcMat/InitBuffer order so the stack slots match; a
 *   by-value handle word the callee returns becomes a `HandleTemp` class temporary, whose address is then taken.
 *   The unit does not include `g3d/fn_80063888.h`: its global placement `operator delete` gives Construct's
 *   `new (pBuf) ScnMdl` a landing pad retail lacks (the leaf `g3d/res_mat_copy_ctor.h` carries what is needed).
 *   The unit compiles with `#pragma peephole off` throughout (retail keeps the unfused `clrlwi` + `cmpwi`, `extsh`,
 *   `addi r0` vtable-store and `mr r3` + `lwz r12,0(r3)` virtual-call forms; playbook idea 106).
 */

#include "types.h"
#include "nw4r/g3d/res_common.h" /* ResHandle, IS_VALID_PTR (rule 1) */
#include "nw4r/g3d/scnmdl.h"      /* nw4r::g3d::ScnMdl (rule 1) */
#include "g3d/res_mat_copy_ctor.h" /* res_mat_copy_ctor, apply_clr_anm_result (rule 2: owner g3d/fn_80063888.cpp) */
#include "g3d/g3d_anmchr.h"       /* G3dObj::operator delete, TypeObj::GetTypeName, type_obj_set_name, TypeObj::operator==, G3dObj (rule 2: owner g3d/g3d_anmchr.cpp) */
#include "g3d/fn_800680CC.h"      /* fn_800696E4, fn_800697A4 (rule 2: owner g3d/fn_800680CC.cpp) */
#include "g3d/fn_80075DCC.h"      /* fn_8007B734..g3d_draw_res_mdl_directly (rule 2: owner g3d/fn_80075DCC.cpp) */
#include "g3d/g3d_scnobj.h"       /* nw4r::g3d::ScnObj / ScnLeaf / ScnGroup (rule 1) */
#include "g3d/g3d_anmvis.h"      /* g3d_apply_vis_anm_result/fn_8006ED84 (rule 2: owner g3d/g3d_anmvis.cpp) */
#include "g3d/g3d_calcview.h"     /* fn_8006FFBC/fn_8006FFC8 (rule 2: owner g3d/g3d_calcview.cpp) */
#include "g3d/g3d_calcworld.h"     /* res_mdl_get_info, res_mdl_info_num_view_mtx (rule 2) */
#include "g3d/g3d_calcvtx.h"       /* fn_8007270C (rule 2: g3d_calcvtx.cpp) */
#include "g3d/g3d_resvtx.h"
#include "g3d/g3d_resmat.h"
#include "g3d/g3d_resnode.h"
#include "g3d/g3d_scnmdlsmpl.h"   /* nw4r::g3d::ScnMdlSimple (rule 2) */
#include "g3d/g3d_scnmdl.h"
#include "g3d/g3d_resfile.h"     /* res_mat_*_dc_store, res_mat_*_copy_to (rule 2) */
#include "g3d/g3d_resshp.h"      /* res_tev_dc_store, res_tev_copy_to (rule 2) */
#include "g3d/g3d_calcmaterial.h" /* res_*_end_edit (rule 2) */
#include "g3d/g3d_anmtexsrt.h"   /* nw4r::g3d::AnmObjTexSrt (rule 1) */
#include "g3d/res_mat_chan_copy_ctor.h" /* res_mat_chan_copy_ctor (rule 2: owner fn_80059550.cpp) */
#include "unsplit/g3d.h"

#pragma peephole off

/* `ScnMdl` lives in its real namespace (rule 9's owner spelling); this unit's bodies name it short. */
using nw4r::g3d::ReplacementBlock;
using nw4r::g3d::ScnMdl;
using nw4r::g3d::ResMdl;
using nw4r::g3d::ResMat;

namespace nw4r {
namespace db {

/* `Panic(const char* pFile, int line, const char* pFmt, ...)` - the trailing `e` in the map's mangling
 * is MWCC's vararg marker.  Declared in the namespace (C++ linkage), never as the mangled spelling
 * (rule 9). */
void Panic(const char* pFile, int line, const char* pFmt, ...);

}  // namespace db
}  // namespace nw4r

/* The panic file/format strings and data objects this unit's bodies reference.  They are `extern` map
 * labels, not literals: MWCC's `-str reuse` would pool a repeated literal into one blob addressed
 * through a shared base register, while the target loads each one with its own `lis`/`addi`. */
extern const char lbl_8058EDA0[]; /* "g3d_scnmdl.cpp" */
extern const char scn_mdl_psize_error_msg[]; /* "NW4R:Pointer Error\npSize(=%p) is not valid pointer." */
extern const char lbl_8058EDE4[]; /* "NW4R:Failed assertion ((u32)buf & 0x1f) == 0" */
extern const char lbl_8058EE14[]; /* "NW4R:Failed assertion pos.GetSize() == ResVtxPos(rep.vtxPosTable..." */
extern const char lbl_8058EE64[]; /* "...((u32)&mReplacement.pixDLArray[i] & 0x1f) == 0" */
extern const char lbl_8058EEAC[]; /* "...((u32)&mReplacement.tevColorDLArray[i] & 0x1f) == 0" */
extern const char lbl_8058EEF8[]; /* "...((u32)&mReplacement.indMtxAndScaleDLArray[i] & 0x1f) == 0" */
extern const char lbl_8058EF48[]; /* "...((u32)&mReplacement.texCoordGenDLArray[i] & 0x1f) == 0" */
extern const char lbl_8058EF98[]; /* "...((u32)&mReplacement.tevDataArray[i] & 0x1f) == 0" */
extern const char lbl_8058EFE0[]; /* "NW4R:Failed assertion !mpAnmObjShp" */
extern const char lbl_8058F004[]; /* "NW4R:Failed assertion !GetParent()" */
extern const char lbl_8058F070[]; /* "%s::%s: Object not valid." */
extern const char lbl_8058F090[]; /* "g3d_resmdl_ac.h" */
extern const char lbl_80791208[4]; /* the `.sdata` string "ref" */

/* ------------------------------------------------------------------------------------------------ */
/* the object                                                                                        */
/* ------------------------------------------------------------------------------------------------ */

/* The resource block `fn_800730D8`/fn_800732F0/fn_800696E4 hand back: its leading word only.  The
 * layout belongs to g3d/g3d_calcvtx.cpp (rule 1), which defines the full block. */
struct ResVtxBlockHead {
    /* +0x00 */ u32 mSize;
}; /* size: 0x04 */

/* ------------------------------------------------------------------------------------------------ */
/* this unit's own bodies, in address order                                                          */
/* ------------------------------------------------------------------------------------------------ */

extern "C" {

u32 fn_8007D38C(u32 p);
u32 fn_8007D468(const ResHandle* pSelf);
u32 fn_8007D470(u32 p);
ResHandle* res_tlut_obj_copy_ctor(ResHandle* pDst, const ResHandle* pSrc);
void res_tlut_obj_common_copy_ctor(ResHandle* pDst, const ResHandle* pSrc);
ResHandle* res_tex_obj_copy_ctor(ResHandle* pDst, const ResHandle* pSrc);
void res_tex_obj_common_copy_ctor(ResHandle* pDst, const ResHandle* pSrc);

} /* extern "C" */

/* Wraps a copied block's handle word in a class temporary, so the edit hook gets the temporary's address. */
struct HandleTemp {
    HandleTemp(u32 word) { mHandle.mpData = (void*)word; }
    /* +0x00 */ ResHandle mHandle;
}; /* size: 0x4 */

/* 0x8007C540 (0xE4C): sizes a model for `mdl` with `numView` views and the replacement buffers `bufferOption`
 * selects, reports the size through `pSize`, and builds the model and its buffers in one block from `pHeap`. */
ScnMdl* ScnMdl::Construct(MEMAllocator* pHeap, u32* pSize, ResMdl mdl, u32 bufferOption, int numView)
{
    if (!mdl.IsValid()) {
        return NULL;
    }
    if (numView == 0) {
        numView = 1;
    } else if (numView > 16) {
        numView = 16;
    }
    ScnMdl* pScnMdl = NULL;
    u32 numPosNrmMtx = res_mdl_info_num_pos_nrm_mtx(&HandleTemp(res_mdl_get_info(&mdl)).mHandle);
    u32 numViewMtx = res_mdl_info_num_view_mtx(&HandleTemp(res_mdl_get_info(&mdl)).mHandle);
    u32 numMat = mdl.GetResMatNumEntries();
    u32 numNode = mdl.GetResNodeNumEntries();

    u32 worldMtxSize = numPosNrmMtx * sizeof(nw4r::math::MTX34);
    u32 worldAttribSize = numPosNrmMtx * sizeof(u32);
    u32 viewMtxSize = numViewMtx * sizeof(nw4r::math::MTX34);
    u32 viewPosSize = numView * fn_8007D470(viewMtxSize);
    u32 viewNrmSize = numViewMtx * sizeof(nw4r::math::MTX33);
    if (((const ScnMdlResMdlInfoData*)res_mdl_info_ref(&HandleTemp(res_mdl_get_info(&mdl)).mHandle))->needNrmMtxArray) {
        viewNrmSize = numView * fn_8007D470(viewNrmSize);
    } else {
        viewNrmSize = 0;
    }
    u32 viewTexSize;
    if (((const ScnMdlResMdlInfoData*)res_mdl_info_ref(&HandleTemp(res_mdl_get_info(&mdl)).mHandle))->needTexMtxArray) {
        viewTexSize = numView * fn_8007D470(viewMtxSize);
    } else {
        viewTexSize = 0;
    }
    u32 matFlagsSize = numMat * sizeof(u32);
    u32 texObjSize = (bufferOption & 0x1) ? numMat * 0x104 : 0;
    u32 tlutObjSize = (bufferOption & 0x2) ? numMat * 0x64 : 0;
    u32 texSrtSize = (bufferOption & 0x4) ? numMat * 0x248 : 0;
    u32 chanSize = (bufferOption & 0x8) ? numMat * 0x28 : 0;
    u32 genModeSize = (bufferOption & 0x10) ? numMat * 0x8 : 0;
    u32 matMiscSize = (bufferOption & 0x20) ? numMat * 0xC : 0;
    u32 visSize = (bufferOption & 0x40) ? numNode : 0;
    u32 pixSize = (bufferOption & 0x80) ? numMat * 0x20 : 0;
    u32 tevColorSize = (bufferOption & 0x100) ? numMat * 0x80 : 0;
    u32 indMtxSize = (bufferOption & 0x200) ? numMat * 0x40 : 0;
    u32 texCoordGenSize = (bufferOption & 0x400) ? numMat * 0xA0 : 0;
    u32 tevSize = (bufferOption & 0x800) ? numMat * 0x200 : 0;

    u32 vtxPosTableSize = 0;
    u32 vtxPosDataSize = 0;
    if (bufferOption & 0x1000) {
        u32 numVtxPos = mdl.GetResVtxPosNumEntries();
        u32 numShp = mdl.GetResShpNumEntries();
        vtxPosTableSize = numVtxPos * sizeof(void*);
        for (u32 i = 0; i < numVtxPos; i++) {
            nw4r::g3d::ResVtxPos pos(&mdl.GetResVtxPos(i));
            u32 j;
            for (j = 0; j < numShp; j++) {
                ResHandle shp;
                res_shp_copy_ctor(&shp, (ResHandle*)&mdl.GetResShp(j));
                if (pos.ptr() == reinterpret_cast<nw4r::g3d::ResVtxPos*>(&HandleTemp(res_shp_get_vtx_pos(&shp)).mHandle)->ptr()) {
                    break;
                }
            }
            if (j != numShp) {
                vtxPosDataSize += fn_8007D470(pos.GetSize());
            }
        }
    }

    u32 vtxNrmTableSize = 0;
    u32 vtxNrmDataSize = 0;
    if (bufferOption & 0x2000) {
        u32 numVtxNrm = mdl.GetResVtxNrmNumEntries();
        u32 numShp = mdl.GetResShpNumEntries();
        vtxNrmTableSize = numVtxNrm * sizeof(void*);
        for (u32 i = 0; i < numVtxNrm; i++) {
            nw4r::g3d::ResVtxNrm nrm(&mdl.GetResVtxNrm(i));
            u32 j;
            for (j = 0; j < numShp; j++) {
                ResHandle shp;
                res_shp_copy_ctor(&shp, (ResHandle*)&mdl.GetResShp(j));
                if (nrm.ptr() == reinterpret_cast<nw4r::g3d::ResVtxNrm*>(&HandleTemp(res_shp_get_vtx_nrm(&shp)).mHandle)->ptr()) {
                    break;
                }
            }
            if (j != numShp) {
                vtxNrmDataSize += fn_8007D470(nrm.GetSize());
            }
        }
    }

    u32 vtxClrTableSize = 0;
    u32 vtxClrDataSize = 0;
    if (bufferOption & 0x4000) {
        u32 numVtxClr = mdl.GetResVtxClrNumEntries();
        u32 numShp = mdl.GetResShpNumEntries();
        vtxClrTableSize = numVtxClr * sizeof(void*);
        for (u32 i = 0; i < numVtxClr; i++) {
            nw4r::g3d::ResVtxClr clr(&mdl.GetResVtxClr(i));
            u32 j;
            for (j = 0; j < numShp; j++) {
                ResHandle shp;
                res_shp_copy_ctor(&shp, (ResHandle*)&mdl.GetResShp(j));
                bool used = clr.ptr() == reinterpret_cast<nw4r::g3d::ResVtxClr*>(
                                &HandleTemp(res_shp_get_vtx_clr(&shp, 0)).mHandle)->ptr() ||
                            clr.ptr() == reinterpret_cast<nw4r::g3d::ResVtxClr*>(
                                &HandleTemp(res_shp_get_vtx_clr(&shp, 1)).mHandle)->ptr();
                if (used) {
                    break;
                }
            }
            if (j != numShp) {
                vtxClrDataSize += fn_8007D470(clr.GetSize());
            }
        }
    }

    u32 worldMtxOffset = fn_8007D470(sizeof(ScnMdl));
    u32 worldAttribOffset = fn_8007D470(worldMtxOffset + worldMtxSize);
    u32 viewPosOffset = fn_8007D470(worldAttribOffset + worldAttribSize);
    u32 viewNrmOffset = fn_8007D470(viewPosOffset + viewPosSize);
    u32 viewTexOffset = fn_8007D470(viewNrmOffset + viewNrmSize);
    u32 matFlagsOffset = fn_8007D38C(viewTexOffset + viewTexSize);
    u32 texObjOffset = fn_8007D470(matFlagsOffset + matFlagsSize);
    u32 tlutObjOffset = fn_8007D38C(texObjOffset + texObjSize);
    u32 texSrtOffset = fn_8007D38C(tlutObjOffset + tlutObjSize);
    u32 chanOffset = fn_8007D38C(texSrtOffset + texSrtSize);
    u32 genModeOffset = fn_8007D38C(chanOffset + chanSize);
    u32 matMiscOffset = fn_8007D38C(genModeOffset + genModeSize);
    u32 visOffset = fn_8007D38C(matMiscOffset + matMiscSize);
    u32 pixOffset = fn_8007D470(visOffset + visSize);
    u32 tevColorOffset = fn_8007D470(pixOffset + pixSize);
    u32 indMtxOffset = fn_8007D470(tevColorOffset + tevColorSize);
    u32 texCoordGenOffset = fn_8007D470(indMtxOffset + indMtxSize);
    u32 tevOffset = fn_8007D470(texCoordGenOffset + texCoordGenSize);
    u32 vtxPosTableOffset = fn_8007D470(tevOffset + tevSize);
    u32 vtxNrmTableOffset = vtxPosTableOffset + vtxPosTableSize;
    u32 vtxClrTableOffset = vtxNrmTableOffset + vtxNrmTableSize;
    u32 vtxPosDataOffset = fn_8007D470(vtxClrTableOffset + vtxClrTableSize);
    u32 vtxNrmDataOffset = fn_8007D470(vtxPosDataOffset + vtxPosDataSize);
    u32 vtxClrDataOffset = fn_8007D470(vtxNrmDataOffset + vtxNrmDataSize);
    u32 size = fn_8007D470(vtxClrDataOffset + vtxClrDataSize);

    if (pSize != NULL) {
        BOOL ok1 = TRUE, ok2 = TRUE, ok3 = TRUE, ok4 = TRUE, ok5 = TRUE, ok6 = TRUE;
        u32 top = (u32)pSize & 0xFF000000u;
        if (!(top == 0x80000000u) && !(((u32)pSize & 0xFF800000u) == 0x81000000u))
            ok6 = FALSE;
        if (!ok6 && !(((u32)pSize & 0xF8000000u) == 0x90000000u))
            ok5 = FALSE;
        if (!ok5 && !(top == 0xC0000000u))
            ok4 = FALSE;
        if (!ok4 && !(((u32)pSize & 0xFF800000u) == 0xC1000000u))
            ok3 = FALSE;
        if (!ok3 && !(((u32)pSize & 0xF8000000u) == 0xD0000000u))
            ok2 = FALSE;
        if (!ok2 && !(((u32)pSize & 0xFFFFC000u) == 0xE0000000u))
            ok1 = FALSE;
        if (!ok1)
            nw4r::db::Panic(lbl_8058EDA0, 802, scn_mdl_psize_error_msg, pSize);
        *pSize = size;
    }

    if (pHeap != NULL) {
        u8* pBuf = (u8*)Alloc(pHeap, size);
        if ((u32)pBuf & 0x1F) {
            nw4r::db::Panic(lbl_8058EDA0, 813, lbl_8058EDE4);
        }
        if (pBuf == NULL) {
            return NULL;
        }

        ReplacementBlock rep;
        rep.mFlag = 0;
        u32 keepVtx = bufferOption & 0x01000000;
        if (keepVtx == 0) {
            rep.mFlag |= 1;
        }
        rep.mpNodeVisible = visSize ? pBuf + visOffset : NULL;
        rep.mpTexObjDataArray = texObjSize ? pBuf + texObjOffset : NULL;
        rep.mpTlutObjDataArray = tlutObjSize ? pBuf + tlutObjOffset : NULL;
        rep.mpTexSrtDataArray = texSrtSize ? pBuf + texSrtOffset : NULL;
        rep.mpChanDataArray = chanSize ? pBuf + chanOffset : NULL;
        rep.mpGenModeDataArray = genModeSize ? pBuf + genModeOffset : NULL;
        rep.mpMatMiscDataArray = matMiscSize ? pBuf + matMiscOffset : NULL;
        rep.mpPixDLArray = pixSize ? pBuf + pixOffset : NULL;
        rep.mpTevColorDLArray = tevColorSize ? pBuf + tevColorOffset : NULL;
        rep.mpIndMtxAndScaleDLArray = indMtxSize ? pBuf + indMtxOffset : NULL;
        rep.mpTexCoordGenDLArray = texCoordGenSize ? pBuf + texCoordGenOffset : NULL;
        rep.mpTevDataArray = tevSize ? pBuf + tevOffset : NULL;
        rep.mpVtxPosTable = vtxPosTableSize ? (void**)(pBuf + vtxPosTableOffset) : NULL;
        rep.mpVtxNrmTable = vtxNrmTableSize ? (void**)(pBuf + vtxNrmTableOffset) : NULL;
        rep.mpVtxClrTable = vtxClrTableSize ? (void**)(pBuf + vtxClrTableOffset) : NULL;

        if (rep.mpVtxPosTable) {
            u32 numVtxPos = mdl.GetResVtxPosNumEntries();
            u32 numShp = mdl.GetResShpNumEntries();
            for (u32 i = 0; i < numVtxPos; i++) {
                nw4r::g3d::ResVtxPos pos(&mdl.GetResVtxPos(i));
                u32 j;
                for (j = 0; j < numShp; j++) {
                    ResHandle shp;
                    res_shp_copy_ctor(&shp, (ResHandle*)&mdl.GetResShp(j));
                    if (pos.ptr() == reinterpret_cast<nw4r::g3d::ResVtxPos*>(&HandleTemp(res_shp_get_vtx_pos(&shp)).mHandle)->ptr()) {
                        break;
                    }
                }
                if (j != numShp) {
                    rep.mpVtxPosTable[i] = pBuf + vtxPosDataOffset;
                    vtxPosDataOffset += fn_8007D470(pos.GetSize());
                    pos.CopyTo(rep.mpVtxPosTable[i]);
                    if (pos.GetSize() != nw4r::g3d::ResVtxPos(rep.mpVtxPosTable[i]).GetSize()) {
                        nw4r::db::Panic(lbl_8058EDA0, 866, lbl_8058EE14);
                    }
                } else {
                    rep.mpVtxPosTable[i] = pos.ptr();
                }
            }
        }
        if (rep.mpVtxNrmTable) {
            u32 numVtxNrm = mdl.GetResVtxNrmNumEntries();
            u32 numShp = mdl.GetResShpNumEntries();
            for (u32 i = 0; i < numVtxNrm; i++) {
                nw4r::g3d::ResVtxNrm nrm(&mdl.GetResVtxNrm(i));
                u32 j;
                for (j = 0; j < numShp; j++) {
                    ResHandle shp;
                    res_shp_copy_ctor(&shp, (ResHandle*)&mdl.GetResShp(j));
                    if (nrm.ptr() == reinterpret_cast<nw4r::g3d::ResVtxNrm*>(&HandleTemp(res_shp_get_vtx_nrm(&shp)).mHandle)->ptr()) {
                        break;
                    }
                }
                if (j != numShp) {
                    rep.mpVtxNrmTable[i] = pBuf + vtxNrmDataOffset;
                    vtxNrmDataOffset += fn_8007D470(nrm.GetSize());
                    nrm.CopyTo(rep.mpVtxNrmTable[i]);
                } else {
                    rep.mpVtxNrmTable[i] = nrm.ptr();
                }
            }
        }
        if (rep.mpVtxClrTable) {
            u32 numVtxClr = mdl.GetResVtxClrNumEntries();
            u32 numShp = mdl.GetResShpNumEntries();
            for (u32 i = 0; i < numVtxClr; i++) {
                nw4r::g3d::ResVtxClr clr(&mdl.GetResVtxClr(i));
                u32 j;
                for (j = 0; j < numShp; j++) {
                    ResHandle shp;
                    res_shp_copy_ctor(&shp, (ResHandle*)&mdl.GetResShp(j));
                    bool used = clr.ptr() == reinterpret_cast<nw4r::g3d::ResVtxClr*>(
                                &HandleTemp(res_shp_get_vtx_clr(&shp, 0)).mHandle)->ptr() ||
                                clr.ptr() == reinterpret_cast<nw4r::g3d::ResVtxClr*>(
                                &HandleTemp(res_shp_get_vtx_clr(&shp, 1)).mHandle)->ptr();
                    if (used) {
                        break;
                    }
                }
                if (j != numShp) {
                    rep.mpVtxClrTable[i] = pBuf + vtxClrDataOffset;
                    vtxClrDataOffset += fn_8007D470(clr.GetSize());
                    clr.CopyTo(rep.mpVtxClrTable[i]);
                } else {
                    rep.mpVtxClrTable[i] = clr.ptr();
                }
            }
        }

        u32 option = 0;
        if (keepVtx) {
            option |= 1;
        }
        pScnMdl = new (pBuf) ScnMdl(pHeap, mdl, (nw4r::math::MTX34*)(pBuf + worldMtxOffset),
                                    (u32*)(pBuf + worldAttribOffset), (nw4r::math::MTX34*)(pBuf + viewPosOffset),
                                    viewNrmSize ? (nw4r::math::MTX33*)(pBuf + viewNrmOffset) : NULL,
                                    viewTexSize ? (nw4r::math::MTX34*)(pBuf + viewTexOffset) : NULL, numView,
                                    numViewMtx, &rep, (u32*)(pBuf + matFlagsOffset), option);
        pScnMdl->InitBuffer();
    }
    return pScnMdl;
}

extern "C" {

/* 0x8007D38C - the block pointer aligned up to 4 (the `buf & 0x3` shape the asserts below test). */
u32 fn_8007D38C(u32 p) {
    return (p + 3) & 0xFFFFFFFC;
}

} /* extern "C" */

/* 0x8007D398 (0x24): returns the colour block's size. */
u32 nw4r::g3d::ResVtxClr::GetSize() const {
    return ref().size;
}

/* 0x8007D3BC (0x24): returns the normal block's size. */
u32 nw4r::g3d::ResVtxNrm::GetSize() const {
    return ref().size;
}

/* 0x8007D3E0 (0x24): returns the position block's size. */
u32 nw4r::g3d::ResVtxPos::GetSize() const {
    return ref().size;
}

extern "C" {

/* 0x8007D404 - `ResCommon<ResMdl>::ref()`: the `g3d_resmdl_ac.h` inlined assert, then the handle. */
/* untyped: opaque handle - the info handle */
u32 res_mdl_info_ref(const void* pInfo) {
    ResHandle* pSelf = (ResHandle*)pInfo;

    if (res_mdl_info_is_valid(pSelf) == 0) {
        nw4r::db::Panic(lbl_8058F090, 57, lbl_8058F070, res_mdl_info_get_class_name(), lbl_80791208);
    }
    return fn_8007D468(pSelf);
}

/* 0x8007D468 - the handle's resource pointer. */
u32 fn_8007D468(const ResHandle* pSelf) {
    return (u32)pSelf->mpData;
}

/* 0x8007D470 - the block pointer aligned up to 32 (the `buf & 0x1f` shape). */
u32 fn_8007D470(u32 p) {
    return (p + 31) & 0xFFFFFFE0;
}

} /* extern "C" */

/* 0x8007D47C (0xEC): the world pass: the posture, the node-visibility buffer refresh, the visibility animation (into
 * the buffer when the model keeps one), then the closing callback. */
void ScnMdl::G3dProcCalcWorld(u32 param, const nw4r::math::MTX34* pParent)
{
    CalcPosture(param, pParent);
    if (IsVisBufferEnabled() && IsVisBufferRefreshNeeded()) {
        UpdateVisBuffer();
    }
    if (GetAnmObjVis() != NULL) {
        if (mReplacement.mpNodeVisible != NULL) {
            nw4r::g3d::ResMdl local = GetResMdl();

            fn_8006ED84(mReplacement.mpNodeVisible, &local, (u32*)GetAnmObjVis());
            fn_8007C464(this);
        } else {
            nw4r::g3d::ResMdl local = GetResMdl();

            g3d_apply_vis_anm_result(&local, (void*)GetAnmObjVis());
        }
    }
    CheckCallback_CALC_WORLD(CALLBACK_TIMING_C, param, (void*)pParent);
}

/* 0x8007D568 (0x8): the visibility animation. */
nw4r::g3d::AnmObjVis* nw4r::g3d::ScnMdlSimple::GetAnmObjVis()
{
    return mpAnmObjVis;
}

/* 0x8007D570 (0x18): whether the node-visibility buffer needs a refresh. */
bool ScnMdl::IsVisBufferRefreshNeeded() const
{
    return (mFlags & 1) != 0;
}

/* 0x8007D588 (0x14): whether the node-visibility buffer is in use. */
bool ScnMdl::IsVisBufferEnabled() const
{
    return (mFlags & 2) == 0;
}

/* 0x8007D59C (0x590): the material pass: refreshes the flagged replacement buffers, then applies the
 * texture-pattern, texture-SRT and colour animations to each material's replacement (or resource) blocks. */
/* untyped: caller-owned payload - the pass's info block */
void ScnMdl::G3dProcCalcMat(u32 param, void* pInfo)
{
    CheckCallback_CALC_MAT(CALLBACK_TIMING_A, param, pInfo);

    u32 view;
    ResHandle matHandle;
    ResHandle texObj;
    ResHandle tlutObj;
    ResHandle texSrt;
    ResHandle indMtx;
    ResHandle tevColor;
    ResHandle chan;
    nw4r::g3d::TexPatAnmResult texPatResult;
    nw4r::g3d::ClrAnmResult clrResult;
    nw4r::g3d::TexSrtAnmResult texSrtResult;

    fn_80077E34((s32)&view, &GetResMdl());
    u32 numMat = reinterpret_cast<ResMdl*>(&view)->GetResMatNumEntries();

    for (u32 i = 0; i < numMat; i++) {
        res_mat_copy_ctor(&matHandle, (ResHandle*)&reinterpret_cast<ResMdl*>(&view)->GetResMat(i));

        if (TestMatBufferFlag(i, 0x1)) {
            scn_mdl_clean_mat_buffer(this, i, 0x1);
        }
        if (TestMatBufferFlag(i, 0x2)) {
            scn_mdl_clean_mat_buffer(this, i, 0x2);
        }
        if (TestMatBufferFlag(i, 0x200)) {
            scn_mdl_clean_mat_buffer(this, i, 0x200);
        }
        if (TestMatBufferFlag(i, 0x4)) {
            scn_mdl_clean_mat_buffer(this, i, 0x4);
        }
        if (TestMatBufferFlag(i, 0x8)) {
            scn_mdl_clean_mat_buffer(this, i, 0x8);
        }
        if (TestMatBufferFlag(i, 0x100)) {
            scn_mdl_clean_mat_buffer(this, i, 0x100);
        }
        if (TestMatBufferFlag(i, 0x10)) {
            scn_mdl_clean_mat_buffer(this, i, 0x10);
        }
        if (TestMatBufferFlag(i, 0x20)) {
            scn_mdl_clean_mat_buffer(this, i, 0x20);
        }
        if (TestMatBufferFlag(i, 0x80)) {
            scn_mdl_clean_mat_buffer(this, i, 0x80);
        }
        if (TestMatBufferFlag(i, 0x400)) {
            scn_mdl_clean_mat_buffer(this, i, 0x400);
        }
        if (TestMatBufferFlag(i, 0x800)) {
            scn_mdl_clean_mat_buffer(this, i, 0x800);
        }

        if (GetAnmObjTexPat() && GetAnmObjTexPat()->TestExistence(i)) {
            tex_pat_anm_result_ctor(&texPatResult);
            const nw4r::g3d::TexPatAnmResult* pResult = GetAnmObjTexPat()->GetResult(&texPatResult, i);

            res_tex_obj_copy_ctor(&texObj, mReplacement.mpTexObjDataArray
                ? (ResHandle*)&nw4r::g3d::ResTexObj((u8*)mReplacement.mpTexObjDataArray + i * 0x104)
                : (ResHandle*)&reinterpret_cast<ResMat&>(matHandle).GetResTexObj());
            res_tlut_obj_copy_ctor(&tlutObj, mReplacement.mpTlutObjDataArray
                ? (ResHandle*)&nw4r::g3d::ResTlutObj((u8*)mReplacement.mpTlutObjDataArray + i * 0x64)
                : (ResHandle*)&reinterpret_cast<ResMat&>(matHandle).GetResTlutObj());
            apply_tex_pat_anm_result(texObj, tlutObj, pResult);
            res_tex_obj_end_edit(&texObj);
            res_tlut_obj_end_edit(&tlutObj);
            scn_mdl_set_mat_buffer_flag(this, i, 0x3);
        }

        if (GetAnmObjTexSrt() && GetAnmObjTexSrt()->TestExistence(i)) {
            const nw4r::g3d::TexSrtAnmResult* pResult = GetAnmObjTexSrt()->GetResult(&texSrtResult, i);

            res_tex_srt_copy_ctor(&texSrt, *(const u32*)(mReplacement.mpTexSrtDataArray
                ? (ResHandle*)&nw4r::g3d::ResTexSrt((u8*)mReplacement.mpTexSrtDataArray + i * 0x248)
                : (ResHandle*)&reinterpret_cast<ResMat&>(matHandle).GetResTexSrt()));
            res_mat_ind_mtx_copy_ctor(&indMtx, mReplacement.mpIndMtxAndScaleDLArray
                ? (ResHandle*)&nw4r::g3d::ResMatIndMtxAndScale((u8*)mReplacement.mpIndMtxAndScaleDLArray + i * 0x40)
                : (ResHandle*)&reinterpret_cast<ResMat&>(matHandle).GetResMatIndMtxAndScale());
            apply_tex_srt_anm_result(texSrt, indMtx, pResult);
            res_mat_ind_mtx_end_edit(&indMtx);
            res_tex_srt_end_edit(&texSrt);
            scn_mdl_set_mat_buffer_flag(this, i, 0x204);
        }

        if (GetAnmObjMatClr() && GetAnmObjMatClr()->TestExistence(i)) {
            res_mat_tev_color_copy_ctor(&tevColor, mReplacement.mpTevColorDLArray
                ? (ResHandle*)&nw4r::g3d::ResMatTevColor((u8*)mReplacement.mpTevColorDLArray + i * 0x80)
                : (ResHandle*)&reinterpret_cast<ResMat&>(matHandle).GetResMatTevColor());
            res_mat_chan_copy_ctor(&chan, mReplacement.mpChanDataArray
                ? (ResHandle*)&nw4r::g3d::ResMatChan((u8*)mReplacement.mpChanDataArray + i * 0x28)
                : (ResHandle*)&reinterpret_cast<ResMat&>(matHandle).GetResMatChan());
            const nw4r::g3d::ClrAnmResult* pResult = GetAnmObjMatClr()->GetResult(&clrResult, i);
            apply_clr_anm_result(chan, tevColor, pResult);
            res_mat_chan_end_edit(&chan);
            res_mat_tev_color_end_edit(&tevColor);
            scn_mdl_set_mat_buffer_flag(this, i, 0x108);
        }
    }

    CheckCallback_CALC_MAT(CALLBACK_TIMING_C, param, pInfo);
}

/* 0x8007DB2C (0x8): the material-colour animation. */
nw4r::g3d::AnmObjMatClr* nw4r::g3d::ScnMdlSimple::GetAnmObjMatClr()
{
    return mpAnmObjMatClr;
}

/* 0x8007DB34 (0x8): the texture-SRT animation. */
nw4r::g3d::AnmObjTexSrt* nw4r::g3d::ScnMdlSimple::GetAnmObjTexSrt()
{
    return mpAnmObjTexSrt;
}

extern "C" {

/* 0x8007DB3C (0x30): copy-constructs a palette-object handle and returns the destination. */
ResHandle* res_tlut_obj_copy_ctor(ResHandle* pDst, const ResHandle* pSrc) {
    res_tlut_obj_common_copy_ctor(pDst, pSrc);
    return pDst;
}

/* 0x8007DB6C (0xC): copies the base handle's data pointer. */
void res_tlut_obj_common_copy_ctor(ResHandle* pDst, const ResHandle* pSrc) {
    pDst->mpData = pSrc->mpData;
}

/* 0x8007DB78 (0x30): copy-constructs a texture-object handle and returns the destination. */
ResHandle* res_tex_obj_copy_ctor(ResHandle* pDst, const ResHandle* pSrc) {
    res_tex_obj_common_copy_ctor(pDst, pSrc);
    return pDst;
}

/* 0x8007DBA8 (0xC): copies the base handle's data pointer. */
void res_tex_obj_common_copy_ctor(ResHandle* pDst, const ResHandle* pSrc) {
    pDst->mpData = pSrc->mpData;
}

} /* extern "C" */

/* 0x8007DBB4 (0x8): the texture-pattern animation. */
nw4r::g3d::AnmObjTexPat* nw4r::g3d::ScnMdlSimple::GetAnmObjTexPat()
{
    return mpAnmObjTexPat;
}

/* 0x8007DBBC (0x20): whether material `idx`'s buffer flag word has any of `mask`'s bits. */
bool ScnMdl::TestMatBufferFlag(u32 idx, u32 mask) const
{
    return (mpDLBuffer[idx] & mask) != 0;
}

/* 0x8007DBDC (0xD4): the opaque draw pass with the model's replacement buffers. */
void ScnMdl::G3dProcDrawOpa(u32 param, const u32* pDrawMode)
{
    u32 key;

    CheckCallback_DRAW_OPA(CALLBACK_TIMING_A, param, (void*)pDrawMode);
    if (pDrawMode != 0) {
        key = *pDrawMode;
    } else {
        key = GetDrawMode();
    }
    nw4r::g3d::ResMdl handle = GetResMdl();
    g3d_draw_res_mdl_directly(&handle, GetViewPosMtxArray(), GetViewNrmMtxArray(), GetViewTexMtxArray(),
                              GetByteCodeDrawOpa(), NULL, &mReplacement, key);
    CheckCallback_DRAW_OPA(CALLBACK_TIMING_C, param, (void*)pDrawMode);
}

/* 0x8007DCB0 (0x8): the opaque draw byte code. */
const u8* nw4r::g3d::ScnMdlSimple::GetByteCodeDrawOpa()
{
    return mpByteCodeDrawOpa;
}

/* 0x8007DCB8 (0xD4): the translucent draw pass with the model's replacement buffers. */
void ScnMdl::G3dProcDrawXlu(u32 param, const u32* pDrawMode)
{
    u32 key;

    CheckCallback_DRAW_XLU(CALLBACK_TIMING_A, param, (void*)pDrawMode);
    if (pDrawMode != 0) {
        key = *pDrawMode;
    } else {
        key = GetDrawMode();
    }
    nw4r::g3d::ResMdl handle = GetResMdl();
    g3d_draw_res_mdl_directly(&handle, GetViewPosMtxArray(), GetViewNrmMtxArray(), GetViewTexMtxArray(), NULL,
                              GetByteCodeDrawXlu(), &mReplacement, key);
    CheckCallback_DRAW_XLU(CALLBACK_TIMING_C, param, (void*)pDrawMode);
}

/* 0x8007DD8C (0x8): the translucent draw byte code. */
const u8* nw4r::g3d::ScnMdlSimple::GetByteCodeDrawXlu()
{
    return mpByteCodeDrawXlu;
}

/* 0x8007DD94 (0x60): the vertex pass: blends the shape animation into the replacement vertex tables. */
/* untyped: caller-owned payload - the pass's info block */
void ScnMdl::G3dProcCalcVtx(u32 param, void* pInfo)
{
    if (GetAnmObjShp() != NULL) {
        nw4r::g3d::ResMdl local = GetResMdl();

        fn_8007270C(&local, (void*)GetAnmObjShp(), (const void**)mReplacement.mpVtxPosTable,
                    (const void**)mReplacement.mpVtxNrmTable, (const void**)mReplacement.mpVtxClrTable);
    }
}

/* 0x8007DDF4 (0x8): the shape animation. */
nw4r::g3d::AnmObjShp* ScnMdl::GetAnmObjShp()
{
    return mpAnmObjShp;
}

/* 0x8007DDFC (0x19C): runs the model's work for a pass unless the pass is disabled. */
/* untyped: caller-owned payload - the pass's info block */
void ScnMdl::G3dProc(u32 task, u32 param, void* pInfo)
{
    if (IsG3dProcDisabled(task)) {
        return;
    }
    switch (task) {
    case G3DPROC_GATHER_SCNOBJ:
        G3dProcGatherScnObj(param, static_cast<nw4r::g3d::IScnObjGather*>(pInfo));
        break;
    case G3DPROC_CALC_WORLD:
        G3dProcCalcWorld(param, static_cast<const nw4r::math::MTX34*>(pInfo));
        break;
    case G3DPROC_CALC_MAT:
        G3dProcCalcMat(param, pInfo);
        break;
    case G3DPROC_CALC_VIEW:
        G3dProcCalcView(param, static_cast<const nw4r::math::MTX34*>(pInfo));
        break;
    case G3DPROC_DRAW_OPA:
        G3dProcDrawOpa(param, static_cast<const u32*>(pInfo));
        break;
    case G3DPROC_DRAW_XLU:
        G3dProcDrawXlu(param, static_cast<const u32*>(pInfo));
        break;
    case G3DPROC_UPDATEFRAME:
        G3dProcUpdateFrame(param, pInfo);
        if (mpAnmObjShp != NULL) {
            mpAnmObjShp->UpdateFrame();
        }
        break;
    case G3DPROC_CHILD_DETACHED:
        RemoveAnmObj(static_cast<AnmObj*>(pInfo));
        break;
    case G3DPROC_CALC_VTX:
        G3dProcCalcVtx(param, pInfo);
        break;
    default:
        DefG3dProcScnLeaf(task, param, pInfo);
        break;
    }
}

/* 0x8007DF98 (0x4): the frame-update pass: advances the attached animations. */
/* untyped: caller-owned payload - the pass's info block */
void nw4r::g3d::ScnMdlSimple::G3dProcUpdateFrame(u32 param, void* pInfo)
{
    UpdateFrame();
}

/* 0x8007DF9C (0x40): the visibility-buffer option clears or sets its disable bit; the rest are ScnMdlSimple's. */
bool ScnMdl::SetScnObjOption(u32 option, u32 value)
{
    if (option == 0x30001) {
        if (value != 0) {
            mFlags &= ~2u;
        } else {
            mFlags |= 2u;
        }
    } else {
        return ScnMdlSimple::SetScnObjOption(option, value);
    }
    return true;
}

/* 0x8007DFDC (0x40): the visibility-buffer option reads its bit; the rest are ScnMdlSimple's. */
bool ScnMdl::GetScnObjOption(u32 option, u32* pValue) const
{
    u32 flag;

    if (pValue == 0) {
        return false;
    }
    if (option == 0x30001) {
        flag = mFlags & 2;
        *pValue = !flag;
    } else {
        return ScnMdlSimple::GetScnObjOption(option, pValue);
    }
    return true;
}

extern "C" {

/* 0x8007E478 (0x8): stores the tev block's display list back without waiting. */
void res_tev_end_edit(struct ResHandle* pSelf) {
    res_tev_dc_store(pSelf, 0);
}

/* 0x8007E480 (0x8): stores the texture-coordinate-generator block back without waiting. */
void res_mat_tex_coord_gen_end_edit(struct ResHandle* pSelf) {
    res_mat_tex_coord_gen_dc_store(pSelf, 0);
}

/* 0x8007E488 (0x8): stores the pixel-engine block back without waiting. */
void res_mat_pix_end_edit(struct ResHandle* pSelf) {
    res_mat_pix_dc_store(pSelf, 0);
}

/* 0x8007E490 (0x4): the misc block's edit hook, empty. */
void res_mat_misc_end_edit(struct ResHandle* pSelf) {
    (void)pSelf;
}

/* 0x8007E494 (0x4): the gen-mode block's edit hook, empty. */
void res_gen_mode_end_edit(struct ResHandle* pSelf) {
    (void)pSelf;
}

} /* extern "C" */

/* 0x8007E01C (0x45C): fills the node-visibility buffer from the nodes' flags, then copies every material's blocks
 * into the replacement buffers that exist and clears each material's buffer-flag word. */
void ScnMdl::InitBuffer()
{
    u32 view;
    ResHandle matHandle;
    fn_80077E34((s32)&view, &GetResMdl());
    u32 numMat = reinterpret_cast<ResMdl*>(&view)->GetResMatNumEntries();
    u32 numNode = reinterpret_cast<ResMdl*>(&view)->GetResNodeNumEntries();
    u32 i;

    if (mReplacement.mpNodeVisible != 0) {
        for (i = 0; i < numNode; i++) {
            if (fn_80078904((s32)&reinterpret_cast<ResMdl*>(&view)->GetResNode(i)) != 0) {
                mReplacement.mpNodeVisible[i] = 1;
            } else {
                mReplacement.mpNodeVisible[i] = 0;
            }
        }
    }

    for (i = 0; i < numMat; i++) {
        res_mat_copy_ctor(&matHandle, (ResHandle*)&reinterpret_cast<ResMdl*>(&view)->GetResMat(i));
        mpDLBuffer[i] = 0;

        if (mReplacement.mpTexObjDataArray) {
            res_tex_obj_end_edit((ResHandle*)&reinterpret_cast<ResMat&>(matHandle).GetResTexObj().CopyTo(
                (u8*)mReplacement.mpTexObjDataArray + i * 0x104));
        }
        if (mReplacement.mpTlutObjDataArray) {
            res_tlut_obj_end_edit((ResHandle*)&reinterpret_cast<ResMat&>(matHandle).GetResTlutObj().CopyTo(
                (u8*)mReplacement.mpTlutObjDataArray + i * 0x64));
        }
        if (mReplacement.mpTexSrtDataArray) {
            res_tex_srt_end_edit((ResHandle*)&reinterpret_cast<ResMat&>(matHandle).GetResTexSrt().CopyTo(
                (u8*)mReplacement.mpTexSrtDataArray + i * 0x248));
        }
        if (mReplacement.mpChanDataArray) {
            res_mat_chan_end_edit((ResHandle*)&reinterpret_cast<ResMat&>(matHandle).GetResMatChan().CopyTo(
                (u8*)mReplacement.mpChanDataArray + i * 0x28));
        }
        if (mReplacement.mpGenModeDataArray) {
            res_gen_mode_end_edit((ResHandle*)&reinterpret_cast<ResMat&>(matHandle).GetResGenMode().CopyTo(
                (u8*)mReplacement.mpGenModeDataArray + i * 0x8));
        }
        if (mReplacement.mpMatMiscDataArray) {
            res_mat_misc_end_edit((ResHandle*)&reinterpret_cast<ResMat&>(matHandle).GetResMatMisc().CopyTo(
                (u8*)mReplacement.mpMatMiscDataArray + i * 0xC));
        }
        if (mReplacement.mpPixDLArray) {
            if (((u32)mReplacement.mpPixDLArray + i * 0x20) & 0x1F) {
                nw4r::db::Panic(lbl_8058EDA0, 1312, lbl_8058EE64);
            }
            res_mat_pix_end_edit(&HandleTemp(res_mat_pix_copy_to(
                (ResHandle*)&reinterpret_cast<ResMat&>(matHandle).GetResMatPix(),
                (u32)mReplacement.mpPixDLArray + i * 0x20)).mHandle);
        }
        if (mReplacement.mpTevColorDLArray) {
            if (((u32)mReplacement.mpTevColorDLArray + i * 0x80) & 0x1F) {
                nw4r::db::Panic(lbl_8058EDA0, 1318, lbl_8058EEAC);
            }
            res_mat_tev_color_end_edit(&HandleTemp(res_mat_tev_color_copy_to(
                (ResHandle*)&reinterpret_cast<ResMat&>(matHandle).GetResMatTevColor(),
                (u32)mReplacement.mpTevColorDLArray + i * 0x80)).mHandle);
        }
        if (mReplacement.mpIndMtxAndScaleDLArray) {
            if (((u32)mReplacement.mpIndMtxAndScaleDLArray + i * 0x40) & 0x1F) {
                nw4r::db::Panic(lbl_8058EDA0, 1324, lbl_8058EEF8);
            }
            res_mat_ind_mtx_end_edit(&HandleTemp(res_mat_ind_mtx_copy_to(
                (ResHandle*)&reinterpret_cast<ResMat&>(matHandle).GetResMatIndMtxAndScale(),
                (u32)mReplacement.mpIndMtxAndScaleDLArray + i * 0x40)).mHandle);
        }
        if (mReplacement.mpTexCoordGenDLArray) {
            if (((u32)mReplacement.mpTexCoordGenDLArray + i * 0xA0) & 0x1F) {
                nw4r::db::Panic(lbl_8058EDA0, 1330, lbl_8058EF48);
            }
            res_mat_tex_coord_gen_end_edit(&HandleTemp(res_mat_tex_coord_gen_copy_to(
                (ResHandle*)&reinterpret_cast<ResMat&>(matHandle).GetResMatTexCoordGen(),
                (u32)mReplacement.mpTexCoordGenDLArray + i * 0xA0)).mHandle);
        }
        if (mReplacement.mpTevDataArray) {
            if (((u32)mReplacement.mpTevDataArray + i * 0x200) & 0x1F) {
                nw4r::db::Panic(lbl_8058EDA0, 1336, lbl_8058EF98);
            }
            res_tev_end_edit(&HandleTemp(res_tev_copy_to(
                (ResHandle*)&reinterpret_cast<ResMat&>(matHandle).GetResTev(),
                (u8*)mReplacement.mpTevDataArray + i * 0x200)).mHandle);
        }
    }
}

/* 0x8007E498 (0x364): copies each block `option` flags of material `matID` into its replacement buffer, closes each
 * copy's edit, and clears those flags in the material's buffer-flag word. */
extern "C" void scn_mdl_clean_mat_buffer(ScnMdl* pMdl, u32 matID, u32 option)
{
    ResHandle matHandle;
    res_mat_copy_ctor(&matHandle, (ResHandle*)&pMdl->GetResMdl().GetResMat(matID));
    ReplacementBlock& rep = pMdl->mReplacement;

    if ((option & 0x1) && rep.mpTexObjDataArray) {
        res_tex_obj_end_edit((ResHandle*)&reinterpret_cast<ResMat&>(matHandle).GetResTexObj().CopyTo((u8*)rep.mpTexObjDataArray + matID * 0x104));
    }
    if ((option & 0x2) && rep.mpTlutObjDataArray) {
        res_tlut_obj_end_edit((ResHandle*)&reinterpret_cast<ResMat&>(matHandle).GetResTlutObj().CopyTo((u8*)rep.mpTlutObjDataArray + matID * 0x64));
    }
    if ((option & 0x4) && rep.mpTexSrtDataArray) {
        res_tex_srt_end_edit((ResHandle*)&reinterpret_cast<ResMat&>(matHandle).GetResTexSrt().CopyTo((u8*)rep.mpTexSrtDataArray + matID * 0x248));
    }
    if ((option & 0x8) && rep.mpChanDataArray) {
        res_mat_chan_end_edit((ResHandle*)&reinterpret_cast<ResMat&>(matHandle).GetResMatChan().CopyTo((u8*)rep.mpChanDataArray + matID * 0x28));
    }
    if ((option & 0x10) && rep.mpGenModeDataArray) {
        res_gen_mode_end_edit((ResHandle*)&reinterpret_cast<ResMat&>(matHandle).GetResGenMode().CopyTo((u8*)rep.mpGenModeDataArray + matID * 0x8));
    }
    if ((option & 0x20) && rep.mpMatMiscDataArray) {
        res_mat_misc_end_edit((ResHandle*)&reinterpret_cast<ResMat&>(matHandle).GetResMatMisc().CopyTo((u8*)rep.mpMatMiscDataArray + matID * 0xC));
    }
    if ((option & 0x80) && rep.mpPixDLArray) {
        res_mat_pix_end_edit(&HandleTemp(res_mat_pix_copy_to((ResHandle*)&reinterpret_cast<ResMat&>(matHandle).GetResMatPix(), (u32)rep.mpPixDLArray + matID * 0x20)).mHandle);
    }
    if ((option & 0x100) && rep.mpTevColorDLArray) {
        res_mat_tev_color_end_edit(&HandleTemp(res_mat_tev_color_copy_to((ResHandle*)&reinterpret_cast<ResMat&>(matHandle).GetResMatTevColor(),
                                                 (u32)rep.mpTevColorDLArray + matID * 0x80)).mHandle);
    }
    if ((option & 0x200) && rep.mpIndMtxAndScaleDLArray) {
        res_mat_ind_mtx_end_edit(&HandleTemp(res_mat_ind_mtx_copy_to((ResHandle*)&reinterpret_cast<ResMat&>(matHandle).GetResMatIndMtxAndScale(),
                                             (u32)rep.mpIndMtxAndScaleDLArray + matID * 0x40)).mHandle);
    }
    if ((option & 0x400) && rep.mpTexCoordGenDLArray) {
        res_mat_tex_coord_gen_end_edit(&HandleTemp(res_mat_tex_coord_gen_copy_to((ResHandle*)&reinterpret_cast<ResMat&>(matHandle).GetResMatTexCoordGen(),
                                                        (u32)rep.mpTexCoordGenDLArray + matID * 0xA0)).mHandle);
    }
    if ((option & 0x800) && rep.mpTevDataArray) {
        res_tev_end_edit(&HandleTemp(res_tev_copy_to((ResHandle*)&reinterpret_cast<ResMat&>(matHandle).GetResTev(), (u8*)rep.mpTevDataArray + matID * 0x200)).mHandle);
    }

    pMdl->mpDLBuffer[matID] &= ~option;
}

/* 0x8007E7FC (0xB8): writes one byte per node into the visibility buffer (1 where the node is visible) and clears
 * the refresh bit. */
void ScnMdl::UpdateVisBuffer()
{
    u32 view;
    nw4r::g3d::ResMdl handle = GetResMdl();
    s32 numNodes;
    u32 i;

    fn_80077E34((s32)&view, &handle);
    numNodes = reinterpret_cast<nw4r::g3d::ResMdl*>(&view)->GetResNodeNumEntries();
    if (mReplacement.mpNodeVisible != 0) {
        for (i = 0; i < (u32)numNodes; i++) {
            nw4r::g3d::ResNode node = reinterpret_cast<nw4r::g3d::ResMdl*>(&view)->GetResNode(i);

            if (fn_80078904((s32)&node) != 0) {
                mReplacement.mpNodeVisible[i] = 1;
            } else {
                mReplacement.mpNodeVisible[i] = 0;
            }
        }
    }
    mFlags &= ~1u;
}

/* 0x8007E8B4 (0x154): attaches a bound shape animation (given or found) to its slot and marks the vertex tables for
 * refreshing; any other animation goes to ScnMdlSimple. */
bool ScnMdl::SetAnmObj(AnmObj* pObj, AnmObjType type)
{
    if (pObj != NULL && pObj->GetParent() == NULL) {
        if (type == ANMOBJTYPE_SHP || type == ANMOBJTYPE_NOT_SPECIFIED) {
            AnmObjShp* pShp = nw4r::g3d::DynamicCast<AnmObjShp>(pObj);
            if (pShp != NULL) {
                if (!pShp->IsBound()) {
                    return false;
                }
                if (mpAnmObjShp != NULL) {
                    RemoveAnmObj(mpAnmObjShp);
                }
                if (mpAnmObjShp != NULL) {
                    nw4r::db::Panic(lbl_8058EDA0, 0x5B2, lbl_8058EFE0);
                }
                mpAnmObjShp = pShp;
                pShp->G3dProc(G3DPROC_ATTACH_PARENT, 0, this);
                mReplacement.mFlag &= ~1;
                return true;
            }
            if (type == ANMOBJTYPE_NOT_SPECIFIED) {
                return ScnMdlSimple::SetAnmObj(pObj, type);
            }
            return false;
        } else {
            return ScnMdlSimple::SetAnmObj(pObj, type);
        }
    }
    return false;
}

/* 0x8007EA08 (0x8): whether the animation is bound to a model. */
bool nw4r::g3d::AnmObj::IsBound() const
{
    return TestAnmFlag(ANMFLAG_ISBOUND);
}

/* 0x8007EA10 (0x7C): the checked cast to AnmObjShp. */
template nw4r::g3d::AnmObjShp* nw4r::g3d::DynamicCast<nw4r::g3d::AnmObjShp, nw4r::g3d::AnmObj>(
    nw4r::g3d::AnmObj* pObj);

/* 0x8007EA8C (0x24C): detaches the shape animation, refilling the replacement vertex tables from the resource (or
 * marking them for refreshing); any other animation goes to ScnMdlSimple. */
bool ScnMdl::RemoveAnmObj(AnmObj* pObj)
{
    if (pObj == NULL) {
        return false;
    }
    if (pObj == mpAnmObjShp) {
        mpAnmObjShp->G3dProc(G3DPROC_DETACH_PARENT, 0, this);
        mpAnmObjShp = NULL;
        if ((mBufferOption & 1) == 0) {
            mReplacement.mFlag |= 1;
            return true;
        }
        if (mReplacement.mpVtxPosTable != NULL) {
            u32 num = GetResMdl().GetResVtxPosNumEntries();
            for (u32 i = 0; i < num; i++) {
                nw4r::g3d::ResVtxPos pos(&GetResMdl().GetResVtxPos(i));
                void* pDst = mReplacement.mpVtxPosTable[i];
                if ((void*)pos.ptr() != pDst) {
                    pos.CopyTo(pDst);
                }
            }
        }
        if (mReplacement.mpVtxNrmTable != NULL) {
            u32 num = GetResMdl().GetResVtxNrmNumEntries();
            for (u32 i = 0; i < num; i++) {
                nw4r::g3d::ResVtxNrm nrm(&GetResMdl().GetResVtxNrm(i));
                void* pDst = mReplacement.mpVtxNrmTable[i];
                if ((void*)nrm.ptr() != pDst) {
                    nrm.CopyTo(pDst);
                }
            }
        }
        if (mReplacement.mpVtxClrTable != NULL) {
            u32 num = GetResMdl().GetResVtxClrNumEntries();
            for (u32 i = 0; i < num; i++) {
                nw4r::g3d::ResVtxClr clr(&GetResMdl().GetResVtxClr(i));
                void* pDst = mReplacement.mpVtxClrTable[i];
                if ((void*)clr.ptr() != pDst) {
                    clr.CopyTo(pDst);
                }
            }
        }
        return true;
    }
    return ScnMdlSimple::RemoveAnmObj(pObj);
}

/* 0x8007ECD8 (0x50): detaches and returns the animation in slot `type` (the shape slot here, the rest ScnMdlSimple's). */
nw4r::g3d::AnmObj* ScnMdl::RemoveAnmObj(AnmObjType type)
{
    if (type == ANMOBJTYPE_SHP) {
        AnmObj* pObj = mpAnmObjShp;

        RemoveAnmObj(pObj);
        return pObj;
    }
    return ScnMdlSimple::RemoveAnmObj(type);
}

/* 0x8007ED28 (0x18): the animation in slot `type`. */
nw4r::g3d::AnmObj* ScnMdl::GetAnmObj(AnmObjType type)
{
    if (type == ANMOBJTYPE_SHP) {
        return mpAnmObjShp;
    }
    return ScnMdlSimple::GetAnmObj(type);
}

/* 0x8007ED40 (0x18): the animation in slot `type`. */
const nw4r::g3d::AnmObj* ScnMdl::GetAnmObj(AnmObjType type) const
{
    if (type == ANMOBJTYPE_SHP) {
        return mpAnmObjShp;
    }
    return ScnMdlSimple::GetAnmObj(type);
}

/* 0x8007ED58 (0x110): constructs the model over its matrix arrays, with no shape animation, its material-buffer flags
 * and a copy of the caller's replacement record. */
ScnMdl::ScnMdl(MEMAllocator* pHeap, ResMdl mdl, nw4r::math::MTX34* pWorldMtxArray, u32* pWorldMtxAttribArray,
               nw4r::math::MTX34* pViewPosMtxArray, nw4r::math::MTX33* pViewNrmMtxArray,
               nw4r::math::MTX34* pViewTexMtxArray, int numView, int numViewMtx, const ReplacementBlock* pReplacement,
               u32* pMatBufferFlags, u32 bufferOption)
    : ScnMdlSimple(pHeap, mdl, pWorldMtxArray, pWorldMtxAttribArray, pViewPosMtxArray, pViewNrmMtxArray,
                   pViewTexMtxArray, numView, numViewMtx),
      mpAnmObjShp(NULL),
      mFlags(0),
      mpDLBuffer(pMatBufferFlags),
      mReplacement(*pReplacement),
      mBufferOption(bufferOption)
{
}

/* 0x8007EE68 (0xB4): asserts the model is detached and detaches its shape animation. */
ScnMdl::~ScnMdl()
{
    if (GetParent() != 0) {
        nw4r::db::Panic(lbl_8058EDA0, 1627, lbl_8058F004);
    }
    if (mpAnmObjShp != 0) {
        RemoveAnmObj(mpAnmObjShp);
    }
}

/* 0x8007EF1C (0x30): returns the type. */
const nw4r::g3d::G3dObj::TypeObj ScnMdl::GetTypeObj() const
{
    const u8* local;

    return *reinterpret_cast<const TypeObj*>(type_obj_set_name(&local, scn_typename_ScnMdl));
}

/* 0x8007EF4C (0x38): returns the type's name. */
const char* ScnMdl::GetTypeName() const
{
    return GetTypeObj().GetTypeName();
}

/* 0x8007EF84 (0x6C): whether the object is a ScnMdl or derives from `type`. */
bool ScnMdl::IsDerivedFrom(TypeObj type) const
{
    if (type == GetTypeObjStatic()) {
        return true;
    }
    return ScnMdlSimple::IsDerivedFrom(type);
}

/* 0x8007EFF0 (0x6C): whether the object is a ScnMdlSimple or derives from `type`. */
bool nw4r::g3d::ScnMdlSimple::IsDerivedFrom(TypeObj type) const
{
    if (type == GetTypeObjStatic()) {
        return true;
    }
    return ScnLeaf::IsDerivedFrom(type);
}

/* 0x8007F05C (0x60): removes and returns the last child; NULL when the group is empty. */
nw4r::g3d::ScnObj* nw4r::g3d::ScnGroup::PopBack()
{
    if (!Empty()) {
        return Remove(Size() - 1);
    }
    return NULL;
}

/* 0x8007F0BC (0x10): whether the group has no child. */
bool nw4r::g3d::ScnGroup::Empty() const
{
    return mNumScnObj == 0;
}

extern "C" {

/* 0x8007F0CC - appends `id` to the scene root's children (ScnGroup::Insert at the child count, through slot
 * +0x34). */
void g3d_root_model_bind(s32 root, u32 id) {
    nw4r::g3d::ScnGroup* pGroup = reinterpret_cast<nw4r::g3d::ScnGroup*>(root);

    pGroup->Insert(pGroup->mNumScnObj, reinterpret_cast<nw4r::g3d::ScnObj*>(id));
}

} /* extern "C" */
