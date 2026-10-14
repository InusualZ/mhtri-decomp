/* homebutton/hkb_window.cpp - the software keyboard layout engine's window pane (nw4r::lyt::Window): a content quad
 *   framed by one, four or eight frame quads, each with its own material; the class table (.data 0x8064A774,
 *   33 slots) adds six slots to the 27 of the pane table, and the frame draws read the texture-flip table
 *   (.data 0x8064A738) and call the quad helpers of homebutton/hkb_keyboard.cpp.
 * RANGE. .text 0x8052E0CC-0x80530680 (21 functions); .ctors 0x8056F3EC-0x8056F3F0; .data 0x8064A738-0x8064A800;
 *   .sbss 0x80795A38-0x80795A40; .sdata2 0x8079D598-0x8079D5B0.
 *   Edges: the left edge follows homebutton/hkb_textbox.cpp's static initialiser 0x8052E0C0; the right edge is this
 *   unit's static initialiser 0x80530674 (.ctors word 0x8056F3EC).
 * FLAGS. GC/3.0a5.2 + cflags_hkb, the band's flags (evidence: src/homebutton/hbm_kb_event.cpp FLAGS).
 * NAMES. GUESS: nw4r::lyt::Window and its members, from the runtime dump (nw4hbm::lyt::Window::DrawSelf,
 *   AnimateSelf, UnbindAnimationSelf, GetFrameSize at their addresses; the dump's namespace is the HOME Button copy
 *   of NintendoWare's layout library, spelt nw4r::lyt in this band after homebutton/hkb_material.cpp). The
 *   texture-flip table and its helpers (GetLTTexCoord ..) are GUESS names for the file's static data and code.
 * NAMES. The map rows keep their generated names until the hkb-b#1 rename batch lands (the source spells the
 *   manglings).
 * SHAPES. Point (a VEC2 with constructors and a member-wise assignment) stands in for nw4r::math::VEC2's own
 *   constructors, which src/nw4r/math.h does not declare: retail builds the quad origins in place and copies
 *   points as floats. DrawFrame / DrawFrame4 place their pieces through the Get*FrameSize helpers, DrawFrame8 inline.
 * RESIDUALS. constructor: retail loads the material list for each material pointer and its ResBlockSet copy into one
 *   register; ours copies it (`mr r8,r5`) and the frame loop's registers differ.
 * RESIDUALS. DrawSelf: retail keeps GetVtxPos's temporary at 0x8 and basePt at 0x10, ours the other way round.
 * RESIDUALS. DrawFrame: float register colouring (retail keeps the texture width in f31, ours the 0.0 constant).
 * RESIDUALS. DrawFrame4 / DrawFrame8: two `lfs` of the texture size load into swapped registers.
 * RESIDUALS. our object emits weak copies the target lacks (ut::Color ctor/dtor, Window::Content dtor); the target
 *   calls the folded Color constructor fn_80064834.
 */
#include "homebutton/hkb_window.h"

namespace nw4r {
namespace lyt {

namespace {

/* The corners of a frame quad, in TexCoordQuad order. */
enum {
    VERTEX_LT,
    VERTEX_RT,
    VERTEX_LB,
    VERTEX_RB
};

/* The texture flips a frame record can ask for. */
enum {
    TEXTUREFLIP_NONE,
    TEXTUREFLIP_H,
    TEXTUREFLIP_V,
    TEXTUREFLIP_90,
    TEXTUREFLIP_180,
    TEXTUREFLIP_270
};

/* A flip as unit texture coordinates per corner and the order the s/t axes are walked in. size: 0xA */
struct TextureFlipInfo {
    /* +0x0 */ u8 coords[4][2];
    /* +0x8 */ u8 idx[2];
};

/* Returns component `i` (0 = x, 1 = y) of a vector. */
inline f32& Component(math::VEC2& vec, int i) {
    return (&vec.x)[i];
}

/* A VEC2 built in place from its components or copied member by member. size: 0x8 */
struct Point : public math::VEC2 {
    Point() {}
    Point(f32 px, f32 py) {
        x = px;
        y = py;
    }
    Point& operator=(const math::VEC2& v) {
        x = v.x;
        y = v.y;
        return *this;
    }
};

/* Places the left-top piece of a one- or four-piece frame. */
inline void GetLTFrameSize(Point* pPt, Size* pSize, const math::VEC2& basePt, const Size& windowSize,
                           const WindowFrameSize& frameSize) {
    *pPt = basePt;
    pSize->width = windowSize.width - frameSize.r;
    pSize->height = frameSize.t;
}

/* Places the right-top piece of a one- or four-piece frame. */
inline void GetRTFrameSize(Point* pPt, Size* pSize, const math::VEC2& basePt, const Size& windowSize,
                           const WindowFrameSize& frameSize) {
    *pPt = Point(basePt.x + windowSize.width - frameSize.r, basePt.y);
    pSize->width = frameSize.r;
    pSize->height = windowSize.height - frameSize.b;
}

/* Places the left-bottom piece of a one- or four-piece frame. */
inline void GetLBFrameSize(Point* pPt, Size* pSize, const math::VEC2& basePt, const Size& windowSize,
                           const WindowFrameSize& frameSize) {
    *pPt = Point(basePt.x, basePt.y - frameSize.t);
    pSize->width = frameSize.l;
    pSize->height = windowSize.height - frameSize.t;
}

/* Places the right-bottom piece of a one- or four-piece frame. */
inline void GetRBFrameSize(Point* pPt, Size* pSize, const math::VEC2& basePt, const Size& windowSize,
                           const WindowFrameSize& frameSize) {
    *pPt = Point(basePt.x + frameSize.l, basePt.y - windowSize.height + frameSize.b);
    pSize->width = windowSize.width - frameSize.l;
    pSize->height = frameSize.b;
}

/* Returns the corner coordinates and axis order of a texture flip. */
inline TextureFlipInfo& GetTextureFlipInfo(u8 textureFlip) {
    static TextureFlipInfo flipInfos[] = {
        {{{0, 0}, {1, 0}, {0, 1}, {1, 1}}, {0, 1}},
        {{{1, 0}, {0, 0}, {1, 1}, {0, 1}}, {0, 1}},
        {{{0, 1}, {1, 1}, {0, 0}, {1, 0}}, {0, 1}},
        {{{0, 1}, {0, 0}, {1, 1}, {1, 0}}, {1, 0}},
        {{{1, 1}, {0, 1}, {1, 0}, {0, 0}}, {0, 1}},
        {{{1, 0}, {1, 1}, {0, 0}, {0, 1}}, {1, 0}},
    };
    return flipInfos[textureFlip];
}

/* Computes the texture coordinates of a quad anchored at its left-top corner. */
inline void GetLTTexCoord(math::VEC2* texCds, const Size& polSize, const Size& texSize, u8 textureFlip) {
    TextureFlipInfo& flipInfo = GetTextureFlipInfo(textureFlip);
    int ix = flipInfo.idx[0];
    int iy = flipInfo.idx[1];
    math::VEC2 tSz;
    tSz.x = texSize.width;
    tSz.y = texSize.height;

    Component(texCds[VERTEX_LT], ix) = Component(texCds[VERTEX_LB], ix) = flipInfo.coords[VERTEX_LT][ix];
    Component(texCds[VERTEX_LT], iy) = Component(texCds[VERTEX_RT], iy) = flipInfo.coords[VERTEX_LT][iy];
    Component(texCds[VERTEX_RB], ix) = Component(texCds[VERTEX_RT], ix) =
        flipInfo.coords[VERTEX_LT][ix] +
        polSize.width / ((flipInfo.coords[VERTEX_RT][ix] - flipInfo.coords[VERTEX_LT][ix]) * Component(tSz, ix));
    Component(texCds[VERTEX_RB], iy) = Component(texCds[VERTEX_LB], iy) =
        flipInfo.coords[VERTEX_LT][iy] +
        polSize.height / ((flipInfo.coords[VERTEX_LB][iy] - flipInfo.coords[VERTEX_LT][iy]) * Component(tSz, iy));
}

/* Computes the texture coordinates of a quad anchored at its right-top corner. */
inline void GetRTTexCoord(math::VEC2* texCds, const Size& polSize, const Size& texSize, u8 textureFlip) {
    TextureFlipInfo& flipInfo = GetTextureFlipInfo(textureFlip);
    int ix = flipInfo.idx[0];
    int iy = flipInfo.idx[1];
    math::VEC2 tSz;
    tSz.x = texSize.width;
    tSz.y = texSize.height;

    Component(texCds[VERTEX_RT], ix) = Component(texCds[VERTEX_RB], ix) = flipInfo.coords[VERTEX_RT][ix];
    Component(texCds[VERTEX_RT], iy) = Component(texCds[VERTEX_LT], iy) = flipInfo.coords[VERTEX_RT][iy];
    Component(texCds[VERTEX_LB], ix) = Component(texCds[VERTEX_LT], ix) =
        flipInfo.coords[VERTEX_RT][ix] +
        polSize.width / ((flipInfo.coords[VERTEX_LT][ix] - flipInfo.coords[VERTEX_RT][ix]) * Component(tSz, ix));
    Component(texCds[VERTEX_LB], iy) = Component(texCds[VERTEX_RB], iy) =
        flipInfo.coords[VERTEX_RT][iy] +
        polSize.height / ((flipInfo.coords[VERTEX_RB][iy] - flipInfo.coords[VERTEX_RT][iy]) * Component(tSz, iy));
}

/* Computes the texture coordinates of a quad anchored at its right-bottom corner. */
inline void GetRBTexCoord(math::VEC2* texCds, const Size& polSize, const Size& texSize, u8 textureFlip) {
    TextureFlipInfo& flipInfo = GetTextureFlipInfo(textureFlip);
    int ix = flipInfo.idx[0];
    int iy = flipInfo.idx[1];
    math::VEC2 tSz;
    tSz.x = texSize.width;
    tSz.y = texSize.height;

    Component(texCds[VERTEX_RB], ix) = Component(texCds[VERTEX_RT], ix) = flipInfo.coords[VERTEX_RB][ix];
    Component(texCds[VERTEX_RB], iy) = Component(texCds[VERTEX_LB], iy) = flipInfo.coords[VERTEX_RB][iy];
    Component(texCds[VERTEX_LT], ix) = Component(texCds[VERTEX_LB], ix) =
        flipInfo.coords[VERTEX_RB][ix] +
        polSize.width / ((flipInfo.coords[VERTEX_LB][ix] - flipInfo.coords[VERTEX_RB][ix]) * Component(tSz, ix));
    Component(texCds[VERTEX_LT], iy) = Component(texCds[VERTEX_RT], iy) =
        flipInfo.coords[VERTEX_RB][iy] +
        polSize.height / ((flipInfo.coords[VERTEX_RT][iy] - flipInfo.coords[VERTEX_RB][iy]) * Component(tSz, iy));
}

/* Computes the texture coordinates of a quad anchored at its left-bottom corner. */
inline void GetLBTexCoord(math::VEC2* texCds, const Size& polSize, const Size& texSize, u8 textureFlip) {
    TextureFlipInfo& flipInfo = GetTextureFlipInfo(textureFlip);
    int ix = flipInfo.idx[0];
    int iy = flipInfo.idx[1];
    math::VEC2 tSz;
    tSz.x = texSize.width;
    tSz.y = texSize.height;

    Component(texCds[VERTEX_LB], ix) = Component(texCds[VERTEX_LT], ix) = flipInfo.coords[VERTEX_LB][ix];
    Component(texCds[VERTEX_LB], iy) = Component(texCds[VERTEX_RB], iy) = flipInfo.coords[VERTEX_LB][iy];
    Component(texCds[VERTEX_RT], ix) = Component(texCds[VERTEX_RB], ix) =
        flipInfo.coords[VERTEX_LB][ix] +
        polSize.width / ((flipInfo.coords[VERTEX_RB][ix] - flipInfo.coords[VERTEX_LB][ix]) * Component(tSz, ix));
    Component(texCds[VERTEX_RT], iy) = Component(texCds[VERTEX_LT], iy) =
        flipInfo.coords[VERTEX_LB][iy] +
        polSize.height / ((flipInfo.coords[VERTEX_LT][iy] - flipInfo.coords[VERTEX_LB][iy]) * Component(tSz, iy));
}

}  // namespace

const ut::detail::RuntimeTypeInfo Window::typeInfo(&Pane::typeInfo);

/* 0x8052E0CC (0x284): Builds the window from its `wnd1` block: the content's colours, texture coordinates and
 * material, then one material per frame record. */
Window::Window(const res::Window* pBlock, const ResBlockSet& resBlockSet) : Pane(pBlock) {
    const res::WindowContent* pResContent =
        detail::ConvertOffsToPtr<res::WindowContent>(pBlock, pBlock->contentOffset);
    u8 texCoordNum = ut::Min<u8>(pResContent->texCoordNum, 8);

    if (texCoordNum > 0) {
        mContent.texCoordAry.Reserve(texCoordNum);
    }

    mContentInflation = pBlock->inflation;
    const u32* matOffsTbl = detail::ConvertOffsToPtr<u32>(resBlockSet.pMaterialList, 0xC);
    for (int i = 0; i < 4; ++i) {
        *reinterpret_cast<u32*>(&mContent.vtxColors[i]) = pResContent->vtxCols[i];
    }

    if (texCoordNum > 0 && !mContent.texCoordAry.IsEmpty()) {
        mContent.texCoordAry.Copy(&pResContent[1], texCoordNum);
    }

    const res::Material* pResMaterial =
        detail::ConvertOffsToPtr<res::Material>(resBlockSet.pMaterialList, matOffsTbl[pResContent->materialIdx]);
    mpMaterial = Layout::NewObj<Material>(pResMaterial, resBlockSet);

    mFrameNum = 0;
    mFrames = NULL;
    if (pBlock->frameNum > 0) {
        u8 frameNum = pBlock->frameNum;
        mFrameNum = 0;
        mFrames = Layout::NewArray<Frame>(frameNum);
        if (mFrames) {
            mFrameNum = frameNum;
        }

        const u32* frameOffsetTable = detail::ConvertOffsToPtr<u32>(pBlock, pBlock->frameOffsetTableOffset);
        for (int i = 0; i < mFrameNum; ++i) {
            const res::WindowFrame* pResWindowFrame =
                detail::ConvertOffsToPtr<res::WindowFrame>(pBlock, frameOffsetTable[i]);
            mFrames[i].textureFlip = pResWindowFrame->textureFlip;
            const res::Material* pResFrameMaterial = detail::ConvertOffsToPtr<res::Material>(
                resBlockSet.pMaterialList, matOffsTbl[pResWindowFrame->materialIdx]);
            mFrames[i].pMaterial = Layout::NewObj<Material>(pResFrameMaterial, resBlockSet);
        }
    }
}

/* Destroys the frame's material. */
inline Window::Frame::~Frame() {
    Layout::DeleteObj(pMaterial);
}

/* 0x8052E350 (0x144): Destroys the frame materials, the content material and the content texture coordinates. */
Window::~Window() {
    Layout::DeleteArray(mFrames, mFrameNum);

    if (mpMaterial && !mpMaterial->mbUserAllocated) {
        Layout::DeleteObj(mpMaterial);
        mpMaterial = NULL;
    }

    mContent.texCoordAry.Free();
}

/* 0x8052E494 (0xFC): Returns the content or frame material named `findName`, searching the children when asked. */
Material* Window::FindMaterialByName(const char* findName, bool bRecursive) {
    if (mpMaterial && detail::EqualsMaterialName(mpMaterial->mName, findName)) {
        return mpMaterial;
    }

    for (int i = 0; i < mFrameNum; ++i) {
        if (detail::EqualsMaterialName(mFrames[i].pMaterial->mName, findName)) {
            return mFrames[i].pMaterial;
        }
    }

    if (bRecursive) {
        for (ut::LinkList<Pane, 4>::Iterator it = mChildList.GetBeginIter(); it != mChildList.GetEndIter(); ++it) {
            if (Material* pMat = it->FindMaterialByName(findName, bRecursive)) {
                return pMat;
            }
        }
    }

    return NULL;
}

/* 0x8052E590 (0x2C): Returns the colour of content corner `idx`. */
ut::Color Window::GetVtxColor(u32 idx) const {
    return mContent.vtxColors[idx];
}

/* 0x8052E5BC (0x2C): Sets the colour of content corner `idx`. */
void Window::SetVtxColor(u32 idx, ut::Color value) {
    mContent.vtxColors[idx] = value;
}

/* 0x8052E5E8 (0x18): Returns one channel of a content corner colour. */
u8 Window::GetVtxColorElement(u32 idx) const {
    return reinterpret_cast<const u8*>(&mContent.vtxColors[idx / 4])[idx % 4];
}

/* 0x8052E600 (0x18): Sets one channel of a content corner colour. */
void Window::SetVtxColorElement(u32 idx, u8 value) {
    reinterpret_cast<u8*>(&mContent.vtxColors[idx / 4])[idx % 4] = value;
}

/* 0x8052E618 (0x140): Draws the content quad and then the frame quads. */
void Window::DrawSelf(const DrawInfo& drawInfo) {
    LoadMtx(drawInfo);

    const WindowFrameSize frameSize = GetFrameSize(mFrameNum, mFrames);
    const math::VEC2 vtxPos = GetVtxPos();
    const math::VEC2 basePt = vtxPos;

    DrawContent(basePt, frameSize, mGlbAlpha);

    switch (mFrameNum) {
    case 1:
        DrawFrame(basePt, *mFrames, frameSize, mGlbAlpha);
        break;
    case 4:
        DrawFrame4(basePt, mFrames, frameSize, mGlbAlpha);
        break;
    case 8:
        DrawFrame8(basePt, mFrames, frameSize, mGlbAlpha);
        break;
    }
}

/* 0x8052E758 (0x90): Animates the pane, then the frame materials unless an invisible pane is skipped. */
void Window::AnimateSelf(u32 option) {
    Pane::AnimateSelf(option);

    if (IsVisible() || !(option & 1)) {
        for (int i = 0; i < mFrameNum; ++i) {
            mFrames[i].pMaterial->Animate();
        }
    }
}

/* 0x8052E7E8 (0x90): Unbinds an animation from the frame materials and the pane. */
void Window::UnbindAnimationSelf(AnimTransform* pAnimTrans) {
    for (int i = 0; i < mFrameNum; ++i) {
        mFrames[i].pMaterial->UnbindAnimation(pAnimTrans);
    }

    Pane::UnbindAnimationSelf(pAnimTrans);
}

/* 0x8052E878 (0x114): Draws the content quad inside the frame, grown by the content inflation. */
void Window::DrawContent(const math::VEC2& basePt, const WindowFrameSize& frameSize, u8 alpha) {
    bool bUseVtxCol = mpMaterial->SetupGX(detail::IsModulateVertexColor(mContent.vtxColors, alpha), alpha);

    detail::SetVertexFormat(bUseVtxCol, mContent.texCoordAry.GetSize());

    detail::DrawQuad(Point(basePt.x + frameSize.l - mContentInflation.l, basePt.y - frameSize.t + mContentInflation.t),
                     Size(mSize.width - frameSize.l + mContentInflation.l - frameSize.r + mContentInflation.r,
                          mSize.height - frameSize.t + mContentInflation.t - frameSize.b + mContentInflation.b),
                     mContent.texCoordAry.GetSize(), mContent.texCoordAry.GetArray(),
                     bUseVtxCol ? mContent.vtxColors : NULL, alpha);
}

/* 0x8052E98C (0x5E0): Draws a one-material frame: the four corner pieces, the material's texture flipped into each
 * corner. */
void Window::DrawFrame(const math::VEC2& basePt, const Frame& frame, const WindowFrameSize& frameSize, u8 alpha) {
    if (frame.pMaterial->GetTextureNum() == 0) {
        return;
    }

    bool bUseVtxCol = frame.pMaterial->SetupGX(detail::IsModulateVertexColor(NULL, alpha), alpha);
    detail::SetVertexFormat(bUseVtxCol, 1);

    const Size texSize = detail::GetTextureSize(frame.pMaterial, 0);
    ut::Color vtxColors[4];
    detail::TexCoordQuad texCds;
    Point polPt;
    Size polSize(0.0f, 0.0f);

    GetLTFrameSize(&polPt, &polSize, basePt, mSize, frameSize);
    GetLTTexCoord(texCds.coord, polSize, texSize, TEXTUREFLIP_NONE);
    detail::DrawQuad(polPt, polSize, 1, &texCds, bUseVtxCol ? vtxColors : NULL, alpha);

    GetRTFrameSize(&polPt, &polSize, basePt, mSize, frameSize);
    GetRTTexCoord(texCds.coord, polSize, texSize, TEXTUREFLIP_H);
    detail::DrawQuad(polPt, polSize, 1, &texCds, bUseVtxCol ? vtxColors : NULL, alpha);

    GetRBFrameSize(&polPt, &polSize, basePt, mSize, frameSize);
    GetRBTexCoord(texCds.coord, polSize, texSize, TEXTUREFLIP_180);
    detail::DrawQuad(polPt, polSize, 1, &texCds, bUseVtxCol ? vtxColors : NULL, alpha);

    GetLBFrameSize(&polPt, &polSize, basePt, mSize, frameSize);
    GetLBTexCoord(texCds.coord, polSize, texSize, TEXTUREFLIP_V);
    detail::DrawQuad(polPt, polSize, 1, &texCds, bUseVtxCol ? vtxColors : NULL, alpha);
}

/* 0x8052EF6C (0x744): Draws a four-material frame: one corner piece per frame record, each with its own flip. */
void Window::DrawFrame4(const math::VEC2& basePt, const Frame* frames, const WindowFrameSize& frameSize,
                        u8 alpha) {
    ut::Color vtxColors[4];
    detail::TexCoordQuad texCds;
    Point polPt;
    Size polSize(0.0f, 0.0f);
    bool bModVtxCol = detail::IsModulateVertexColor(NULL, alpha);

    if (frames[0].pMaterial->GetTextureNum() > 0) {
        bool bUseVtxCol = frames[0].pMaterial->SetupGX(bModVtxCol, alpha);
        GetLTFrameSize(&polPt, &polSize, basePt, mSize, frameSize);
        GetLTTexCoord(texCds.coord, polSize, detail::GetTextureSize(frames[0].pMaterial, 0), frames[0].textureFlip);
        detail::SetVertexFormat(bUseVtxCol, 1);
        detail::DrawQuad(polPt, polSize, 1, &texCds, bUseVtxCol ? vtxColors : NULL, alpha);
    }
    if (frames[1].pMaterial->GetTextureNum() > 0) {
        bool bUseVtxCol = frames[1].pMaterial->SetupGX(bModVtxCol, alpha);
        GetRTFrameSize(&polPt, &polSize, basePt, mSize, frameSize);
        GetRTTexCoord(texCds.coord, polSize, detail::GetTextureSize(frames[1].pMaterial, 0), frames[1].textureFlip);
        detail::SetVertexFormat(bUseVtxCol, 1);
        detail::DrawQuad(polPt, polSize, 1, &texCds, bUseVtxCol ? vtxColors : NULL, alpha);
    }
    if (frames[3].pMaterial->GetTextureNum() > 0) {
        bool bUseVtxCol = frames[3].pMaterial->SetupGX(bModVtxCol, alpha);
        GetRBFrameSize(&polPt, &polSize, basePt, mSize, frameSize);
        GetRBTexCoord(texCds.coord, polSize, detail::GetTextureSize(frames[3].pMaterial, 0), frames[3].textureFlip);
        detail::SetVertexFormat(bUseVtxCol, 1);
        detail::DrawQuad(polPt, polSize, 1, &texCds, bUseVtxCol ? vtxColors : NULL, alpha);
    }
    if (frames[2].pMaterial->GetTextureNum() > 0) {
        bool bUseVtxCol = frames[2].pMaterial->SetupGX(bModVtxCol, alpha);
        GetLBFrameSize(&polPt, &polSize, basePt, mSize, frameSize);
        GetLBTexCoord(texCds.coord, polSize, detail::GetTextureSize(frames[2].pMaterial, 0), frames[2].textureFlip);
        detail::SetVertexFormat(bUseVtxCol, 1);
        detail::DrawQuad(polPt, polSize, 1, &texCds, bUseVtxCol ? vtxColors : NULL, alpha);
    }
}

/* 0x8052F6B0 (0xE48): Draws an eight-material frame: the four corners and the four edges between them. */
void Window::DrawFrame8(const math::VEC2& basePt, const Frame* frames, const WindowFrameSize& frameSize,
                        u8 alpha) {
    ut::Color vtxColors[4];
    detail::TexCoordQuad texCds;
    Size polSize(0.0f, 0.0f);
    bool bModVtxCol = detail::IsModulateVertexColor(NULL, alpha);

    if (frames[0].pMaterial->GetTextureNum() > 0) {
        bool bUseVtxCol = frames[0].pMaterial->SetupGX(bModVtxCol, alpha);
        polSize = Size(frameSize.l, frameSize.t);
        GetLTTexCoord(texCds.coord, polSize, detail::GetTextureSize(frames[0].pMaterial, 0), frames[0].textureFlip);
        detail::SetVertexFormat(bUseVtxCol, 1);
        detail::DrawQuad(basePt, polSize, 1, &texCds, bUseVtxCol ? vtxColors : NULL, alpha);
    }
    if (frames[6].pMaterial->GetTextureNum() > 0) {
        bool bUseVtxCol = frames[6].pMaterial->SetupGX(bModVtxCol, alpha);
        polSize = Size(mSize.width - frameSize.l - frameSize.r, frameSize.t);
        GetLTTexCoord(texCds.coord, polSize, detail::GetTextureSize(frames[6].pMaterial, 0), frames[6].textureFlip);
        detail::SetVertexFormat(bUseVtxCol, 1);
        detail::DrawQuad(Point(basePt.x + frameSize.l, basePt.y),
                         polSize, 1, &texCds, bUseVtxCol ? vtxColors : NULL, alpha);
    }
    if (frames[1].pMaterial->GetTextureNum() > 0) {
        bool bUseVtxCol = frames[1].pMaterial->SetupGX(bModVtxCol, alpha);
        polSize = Size(frameSize.r, frameSize.t);
        GetRTTexCoord(texCds.coord, polSize, detail::GetTextureSize(frames[1].pMaterial, 0), frames[1].textureFlip);
        detail::SetVertexFormat(bUseVtxCol, 1);
        detail::DrawQuad(Point(basePt.x + mSize.width - frameSize.r, basePt.y),
                         polSize, 1, &texCds, bUseVtxCol ? vtxColors : NULL, alpha);
    }
    if (frames[5].pMaterial->GetTextureNum() > 0) {
        bool bUseVtxCol = frames[5].pMaterial->SetupGX(bModVtxCol, alpha);
        polSize = Size(frameSize.r, mSize.height - frameSize.t - frameSize.b);
        GetRTTexCoord(texCds.coord, polSize, detail::GetTextureSize(frames[5].pMaterial, 0), frames[5].textureFlip);
        detail::SetVertexFormat(bUseVtxCol, 1);
        detail::DrawQuad(Point(basePt.x + mSize.width - frameSize.r, basePt.y - frameSize.t),
                         polSize, 1, &texCds, bUseVtxCol ? vtxColors : NULL, alpha);
    }
    if (frames[3].pMaterial->GetTextureNum() > 0) {
        bool bUseVtxCol = frames[3].pMaterial->SetupGX(bModVtxCol, alpha);
        polSize = Size(frameSize.r, frameSize.b);
        GetRBTexCoord(texCds.coord, polSize, detail::GetTextureSize(frames[3].pMaterial, 0), frames[3].textureFlip);
        detail::SetVertexFormat(bUseVtxCol, 1);
        detail::DrawQuad(Point(basePt.x + mSize.width - frameSize.r, basePt.y - mSize.height + frameSize.b),
                         polSize, 1, &texCds, bUseVtxCol ? vtxColors : NULL, alpha);
    }
    if (frames[7].pMaterial->GetTextureNum() > 0) {
        bool bUseVtxCol = frames[7].pMaterial->SetupGX(bModVtxCol, alpha);
        polSize = Size(mSize.width - frameSize.l - frameSize.r, frameSize.b);
        GetRBTexCoord(texCds.coord, polSize, detail::GetTextureSize(frames[7].pMaterial, 0), frames[7].textureFlip);
        detail::SetVertexFormat(bUseVtxCol, 1);
        detail::DrawQuad(Point(basePt.x + frameSize.l, basePt.y - mSize.height + frameSize.b),
                         polSize, 1, &texCds, bUseVtxCol ? vtxColors : NULL, alpha);
    }
    if (frames[2].pMaterial->GetTextureNum() > 0) {
        bool bUseVtxCol = frames[2].pMaterial->SetupGX(bModVtxCol, alpha);
        polSize = Size(frameSize.l, frameSize.b);
        GetLBTexCoord(texCds.coord, polSize, detail::GetTextureSize(frames[2].pMaterial, 0), frames[2].textureFlip);
        detail::SetVertexFormat(bUseVtxCol, 1);
        detail::DrawQuad(Point(basePt.x, basePt.y - mSize.height + frameSize.b),
                         polSize, 1, &texCds, bUseVtxCol ? vtxColors : NULL, alpha);
    }
    if (frames[4].pMaterial->GetTextureNum() > 0) {
        bool bUseVtxCol = frames[4].pMaterial->SetupGX(bModVtxCol, alpha);
        polSize = Size(frameSize.l, mSize.height - frameSize.t - frameSize.b);
        GetLBTexCoord(texCds.coord, polSize, detail::GetTextureSize(frames[4].pMaterial, 0), frames[4].textureFlip);
        detail::SetVertexFormat(bUseVtxCol, 1);
        detail::DrawQuad(Point(basePt.x, basePt.y - frameSize.t),
                         polSize, 1, &texCds, bUseVtxCol ? vtxColors : NULL, alpha);
    }
}

/* 0x805304F8 (0x100): Returns the frame widths: the first frame's texture size, and for four or eight frames the
 * right-bottom frame's texture size on the right and bottom. */
WindowFrameSize Window::GetFrameSize(u8 frameNum, const Frame* frames) {
    WindowFrameSize ret = {0};

    switch (frameNum) {
    case 1: {
        const Size texSize = detail::GetTextureSize(frames->pMaterial, 0);
        ret.l = texSize.width;
        ret.t = texSize.height;
        ret.r = texSize.width;
        ret.b = texSize.height;
    } break;
    case 4:
    case 8: {
        Size texSize = detail::GetTextureSize(frames[0].pMaterial, 0);
        ret.l = texSize.width;
        ret.t = texSize.height;
        texSize = detail::GetTextureSize(frames[3].pMaterial, 0);
        ret.r = texSize.width;
        ret.b = texSize.height;
    } break;
    }

    return ret;
}

/* 0x805305F8 (0x10): Returns the material count: the content's and one per frame. */
u8 Window::GetMaterialNum() const {
    return (u8)(mFrameNum + 1);
}

/* 0x80530608 (0x2C): Returns material `idx`: 0 is the content's, the rest are the frames'. */
Material* Window::GetMaterial(u32 idx) const {
    if (idx == 0) {
        return GetContentMaterial();
    }
    return GetFrameMaterial(idx - 1);
}

/* 0x80530634 (0x28): Returns the material of frame `frameIdx`, or NULL past the last frame. */
Material* Window::GetFrameMaterial(u32 frameIdx) const {
    if (frameIdx >= mFrameNum) {
        return NULL;
    }
    return mFrames[frameIdx].pMaterial;
}

/* 0x8053065C (0x10): Returns the content material. */
Material* Window::GetContentMaterial() const {
    return GetMaterial();
}

/* 0x8053066C (0x8): Returns the class's run-time type record. */
const ut::detail::RuntimeTypeInfo* Window::GetRuntimeTypeInfo() const {
    return &typeInfo;
}

}  // namespace lyt
}  // namespace nw4r
