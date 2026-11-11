/* homebutton/hkb_picture.cpp - the software keyboard layout engine's picture pane (nw4r::lyt::Picture): a textured,
 *   vertex-coloured quad.
 * RANGE. .text 0x8052C330-0x8052C880 (10 functions, the last is the static initialiser that sets
 *   Picture::typeInfo); .ctors 0x8056F3E4-0x8056F3E8; .data 0x8064A638-0x8064A6B0 (Picture's class table);
 *   .sbss 0x80795A28-0x80795A30 (Picture::typeInfo); .sdata2 0x8079D578-0x8079D580.
 *   Edges: the left edge is homebutton/hkb_layout.cpp's last function (BuildPaneObj, ends 0x8052C330); the right edge
 *   follows into homebutton/hkb_textbox.cpp at 0x8052C880. The seam evidence is in homebutton/hkb_group.cpp.
 * FLAGS. GC/3.0a5.2 + cflags_hkb, the band's flags (evidence: src/homebutton/hbm_kb_event.cpp FLAGS).
 * NAMES. GUESS: nw4r::lyt::Picture and its members, from the runtime dump (nw4hbm::lyt::Picture::DrawSelf at its
 *   address). SetMaterialTexCoordGen is a GUESS name for the file's static helper.
 * SHAPES. Picture::Append sets the coordinate generator through a by-value helper (retail's
 *   Material::SetTexCoordGen, which homebutton/hkb_material.h does not declare: request hkb-b#4).
 */
#include "homebutton/hkb_layout.h"

#include "homebutton/hkb_bounding.h"
#include "homebutton/hkb_window.h"

namespace nw4r {
namespace lyt {

namespace {

/* Sets texture coordinate generator `idx` of a material. */
inline void SetMaterialTexCoordGen(Material* pMaterial, u32 idx, TexCoordGen value) {
    pMaterial->GetTexCoordGenAry()[idx] = value;
}

}  // namespace

const ut::detail::RuntimeTypeInfo Picture::typeInfo(&Pane::typeInfo);

/* 0x8052C330 (0x14C): Builds the picture from its `pic1` block: the corner colours, the texture coordinates and
 * the material. */
Picture::Picture(const res::Picture* pResPic, const ResBlockSet& resBlockSet) : Pane(pResPic) {
    u8 texCoordNum = ut::Min<u8>(pResPic->texCoordNum, 8);

    if (texCoordNum > 0) {
        mTexCoordAry.Reserve(texCoordNum);
    }

    for (int i = 0; i < 4; ++i) {
        *reinterpret_cast<u32*>(&mVtxColors[i]) = pResPic->vtxCols[i];
    }

    if (texCoordNum > 0 && !mTexCoordAry.IsEmpty()) {
        mTexCoordAry.Copy(&pResPic[1], texCoordNum);
    }

    const u32* matOffsTbl = detail::ConvertOffsToPtr<u32>(resBlockSet.pMaterialList, 0xC);
    const res::Material* pResMaterial =
        detail::ConvertOffsToPtr<res::Material>(resBlockSet.pMaterialList, matOffsTbl[pResPic->materialIdx]);
    mpMaterial = Layout::NewObj<Material>(pResMaterial, resBlockSet);
}

/* 0x8052C47C (0xD8): Destroys the material and the texture coordinates. */
Picture::~Picture() {
    if (mpMaterial && !mpMaterial->mbUserAllocated) {
        Layout::DeleteObj(mpMaterial);
        mpMaterial = NULL;
    }

    mTexCoordAry.Free();
}

/* 0x8052C554 (0x1B4): Adds a texture to the material with its own coordinate generator; the first texture also
 * sizes a picture that has no size. */
void Picture::Append(const TexMap& texMap) {
    if (mpMaterial->GetTextureNum() >= mpMaterial->mGXMemCap.texMap ||
        mpMaterial->GetTextureNum() >= mpMaterial->mGXMemCap.texCoordGen) {
        return;
    }

    const u8 texMapIdx = mpMaterial->GetTextureNum();
    mpMaterial->SetTextureNum((u8)(texMapIdx + 1));
    mpMaterial->GetTexMapAry()[texMapIdx] = texMap;
    mpMaterial->SetTexCoordGenNum(mpMaterial->GetTextureNum());
    SetMaterialTexCoordGen(mpMaterial, texMapIdx, TexCoordGen());
    mTexCoordAry.SetSize(mpMaterial->GetTextureNum());

    const Size zeroSize(0.0f, 0.0f);
    bool bZeroSize = false;
    if (mSize.width == zeroSize.width && mSize.height == zeroSize.height) {
        bZeroSize = true;
    }

    if (bZeroSize && mpMaterial->GetTextureNum() == 1) {
        mSize = detail::GetTextureSize(mpMaterial, 0);
    }
}

/* 0x8052C708 (0x2C): Returns the colour of corner `idx`. */
ut::Color Picture::GetVtxColor(u32 idx) const {
    return mVtxColors[idx];
}

/* 0x8052C734 (0x2C): Sets the colour of corner `idx`. */
void Picture::SetVtxColor(u32 idx, ut::Color value) {
    mVtxColors[idx] = value;
}

/* 0x8052C760 (0x18): Returns one channel of a corner colour. */
u8 Picture::GetVtxColorElement(u32 idx) const {
    return reinterpret_cast<const u8*>(&mVtxColors[idx / 4])[idx % 4];
}

/* 0x8052C778 (0x18): Sets one channel of a corner colour. */
void Picture::SetVtxColorElement(u32 idx, u8 value) {
    reinterpret_cast<u8*>(&mVtxColors[idx / 4])[idx % 4] = value;
}

/* 0x8052C790 (0xDC): Draws the picture's quad with its material and corner colours. */
void Picture::DrawSelf(const DrawInfo& drawInfo) {
    if (!mpMaterial) {
        return;
    }

    LoadMtx(drawInfo);

    bool bUseVtxCol = mpMaterial->SetupGX(detail::IsModulateVertexColor(mVtxColors, mGlbAlpha), mGlbAlpha);
    detail::SetVertexFormat(bUseVtxCol, mTexCoordAry.GetSize());
    detail::DrawQuad(AsVEC2(GetVtxPos()), mSize, mTexCoordAry.GetSize(), mTexCoordAry.GetArray(),
                     bUseVtxCol ? mVtxColors : NULL, mGlbAlpha);
}

/* 0x8052C86C (0x8): Returns the class's run-time type record. */
const ut::detail::RuntimeTypeInfo* Picture::GetRuntimeTypeInfo() const {
    return &typeInfo;
}

}  // namespace lyt
}  // namespace nw4r

