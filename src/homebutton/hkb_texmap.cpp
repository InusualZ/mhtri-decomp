/* homebutton/hkb_texmap.cpp - the keyboard layout engine's texture map (nw4r::lyt::TexMap): GX texture and TLUT
 *   object setup from the packed sampling state, the state read back from a GX texture object, and the image
 *   replaced from a TPL entry.
 * RANGE. .text 0x80533474-0x805337A0 (4 functions: Get(GXTexObj*), Get(GXTlutObj*), Set, ReplaceImage);
 *   .sdata2 0x8079D5D0-0x8079D5E8 (1/256, the unsigned int-to-float bias, 256).
 *   Edges: the left edge is the `.sdata2` pool duplicate 4330000000000000 (0x8079D5C8 read by
 *   homebutton/hkb_material.cpp, 0x8079D5D8 read by 0x80533474).  The right edge: the pool word 0x8079D5E4 is the
 *   zero pad after the float at 0x8079D5E0 (a TU's `.sdata2` contribution ends 8-aligned), and the next float 0.0
 *   at 0x8079D5E8 belongs to homebutton/hkb_drawinfo.cpp.
 * FLAGS. GC/3.0a5.2 + cflags_hkb, the band's flags (evidence: src/homebutton/hbm_kb_event.cpp FLAGS).
 * NAMES. GUESS: nw4r::lyt::TexMap for the class (the GX texture/TLUT and TPL calls and the 0x1C record
 *   homebutton/hkb_material.cpp keeps per texture slot, the layout library's texture map); Get, ReplaceImage keep
 *   the map's names; Set(const GXTexObj&) is a GUESS from GXGetTexObjAll/GXGetTexObjLODAll.
 * RESIDUALS. .text none once the map rows are renamed (request hkb-a#7); .sdata2: the object's pool is 0x14 B
 *   against the claimed 0x18 B (the trailing zero pad word 0x8079D5E4).
 * SHAPES. Get reads the TLUT name into a local before the colour-index init (retail calls GXGetTexObjTlut first);
 *   Set and ReplaceImage store through the inline setters (SetSize, SetTexelFormat, SetWrapMode, SetLOD, ...),
 *   whose argument evaluation gives retail's load order (height before width, maxLOD before minLOD).
 */
#include "homebutton/hkb_texmap.h"

#include "RVLGX/GXTexture.h"
#include "TPL/tpl.h"

namespace nw4r {
namespace lyt {

/* 0x80533474 (0xEC): Initialises a GX texture object (colour-index or direct) and its LOD state from the map. */
void TexMap::Get(GXTexObj* pTexObj) const {
    if (detail::IsCITexelFormat(GetTexelFormat())) {
        u32 tlutName = GXGetTexObjTlut(pTexObj);
        GXInitTexObjCI(pTexObj, GetImage(), GetWidth(), GetHeight(), GetTexelFormat(), GetWrapModeS(),
                       GetWrapModeT(), IsMipMap(), tlutName);
    } else {
        GXInitTexObj(pTexObj, GetImage(), GetWidth(), GetHeight(), GetTexelFormat(), GetWrapModeS(), GetWrapModeT(),
                     IsMipMap());
    }
    GXInitTexObjLOD(pTexObj, GetMinFilter(), GetMagFilter(), GetMinLOD(), GetMaxLOD(), GetLODBias(),
                    IsBiasClampEnable(), IsEdgeLODEnable(), GetAnisotropy());
}

/* 0x80533560 (0x1C): Initialises a GX TLUT object from the map's palette. */
void TexMap::Get(GXTlutObj* pTlutObj) const {
    GXInitTlutObj(pTlutObj, GetPalette(), GetPaletteFormat(), GetPaletteEntryNum());
}

/* 0x8053357C (0x154): Reads the image, size, format, wrap and LOD state back from a GX texture object. */
void TexMap::Set(const GXTexObj& texObj) {
    void* image;
    u16 width;
    u16 height;
    u32 format;
    u32 wrapS;
    u32 wrapT;
    u8 mipmap;
    GXGetTexObjAll(&texObj, &image, &width, &height, &format, &wrapS, &wrapT, &mipmap);
    mpImage = image;
    SetSize(width, height);
    SetTexelFormat(format);
    SetWrapMode(wrapS, wrapT);
    SetMipMap(mipmap != 0);

    u32 minFilter;
    u32 magFilter;
    f32 minLOD;
    f32 maxLOD;
    f32 lodBias;
    u8 biasClamp;
    u8 edgeLOD;
    u32 anisotropy;
    GXGetTexObjLODAll(&texObj, &minFilter, &magFilter, &minLOD, &maxLOD, &lodBias, &biasClamp, &edgeLOD,
                      &anisotropy);
    SetFilter(minFilter, magFilter);
    SetLOD(minLOD, maxLOD);
    SetLODBias(lodBias);
    SetBiasClampEnable(biasClamp != 0);
    SetEdgeLODEnable(edgeLOD != 0);
    SetAnisotropy(anisotropy);
}

/* 0x805336D0 (0xD0): Replaces the image and palette with entry `id` of a TPL file, binding the file first. */
void TexMap::ReplaceImage(TPLPalette* pTplRes, u32 id) {
    if ((u32)pTplRes->descriptorArray < 0x80000000) {
        TPLBind(pTplRes);
    }

    const TPLDescriptor* pTPLDesc = TPLGet(pTplRes, id);
    const TPLImageHeader* pTexHeader = pTPLDesc->textureHeader;
    mpImage = pTexHeader->data;
    SetSize(pTexHeader->width, pTexHeader->height);
    SetTexelFormat(pTexHeader->format);

    const TPLClutHeader* pClutHeader = pTPLDesc->clutHeader;
    if (pClutHeader) {
        mpPalette = pClutHeader->data;
        mBits.paletteFormat = pClutHeader->format;
        mPaletteEntryNum = pClutHeader->numEntries;
    } else {
        mpPalette = NULL;
        mBits.paletteFormat = 0;
        mPaletteEntryNum = 0;
    }
}

}  // namespace lyt
}  // namespace nw4r
