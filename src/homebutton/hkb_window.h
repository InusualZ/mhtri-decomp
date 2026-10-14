/*
 * homebutton/hkb_window.h - the keyboard layout engine's window pane (nw4r::lyt::Window), which
 *   homebutton/hkb_window.cpp defines: a content quad framed by one, four or eight frame quads, each with its own
 *   material.
 */
#ifndef HOMEBUTTON_HKB_WINDOW_H
#define HOMEBUTTON_HKB_WINDOW_H

#include "homebutton/hkb_layout.h"

namespace nw4r {
namespace lyt {

/* How far the content quad reaches past (or into) the frame on each side. size: 0x10 */
struct InflationLRTB {
    /* +0x0 */ f32 l;
    /* +0x4 */ f32 r;
    /* +0x8 */ f32 t;
    /* +0xC */ f32 b;
};

/* The widths of the frame on each side. size: 0x10 */
struct WindowFrameSize {
    /* +0x0 */ f32 l;
    /* +0x4 */ f32 r;
    /* +0x8 */ f32 t;
    /* +0xC */ f32 b;
};

namespace res {

/* The `wnd1` block: a pane, the content inflation and the offsets of the content and frame records. size: 0x68 */
struct Window : public Pane {
    /* +0x4C */ InflationLRTB inflation;
    /* +0x5C */ u8 frameNum;
    /* +0x5D */ u8 pad_0x5D[3];
    /* +0x60 */ u32 contentOffset;
    /* +0x64 */ u32 frameOffsetTableOffset;
};

/* The window content record: corner colours, material and the texture coordinates that follow. size: 0x14 */
struct WindowContent {
    /* +0x00 */ u32 vtxCols[4];
    /* +0x10 */ u16 materialIdx;
    /* +0x12 */ u8 texCoordNum;
    /* +0x13 */ u8 pad_0x13;
};

/* One frame record: its material and how its texture is flipped. size: 0x4 */
struct WindowFrame {
    /* +0x0 */ u16 materialIdx;
    /* +0x2 */ u8 textureFlip;
    /* +0x3 */ u8 pad_0x3;
};

}  // namespace res

/* A pane drawn as a content quad inside a frame of one, four or eight quads. size: 0x108 */
class Window : public Pane {
public:
    /* One frame quad: its texture flip and material. size: 0x8 */
    struct Frame {
        Frame() : textureFlip(0), pMaterial(NULL) {}
        ~Frame();

        /* +0x0 */ u8 textureFlip;
        /* +0x4 */ Material* pMaterial;
    };

    /* The content quad: its corner colours and texture coordinates. size: 0x10 */
    struct Content {
        /* +0x0 */ ut::Color vtxColors[4];
        /* +0x10 */ detail::TexCoordAry texCoordAry;
    };

    Window(const res::Window* pBlock, const ResBlockSet& resBlockSet);
    virtual ~Window();
    virtual const ut::detail::RuntimeTypeInfo* GetRuntimeTypeInfo() const;
    virtual void DrawSelf(const DrawInfo& drawInfo);
    virtual void AnimateSelf(u32 option);
    virtual ut::Color GetVtxColor(u32 idx) const;
    virtual void SetVtxColor(u32 idx, ut::Color value);
    virtual u8 GetVtxColorElement(u32 idx) const;
    virtual void SetVtxColorElement(u32 idx, u8 value);
    virtual Material* FindMaterialByName(const char* findName, bool bRecursive);
    virtual void UnbindAnimationSelf(AnimTransform* pAnimTrans);
    virtual u8 GetMaterialNum() const;
    using Pane::GetMaterial;
    virtual Material* GetMaterial(u32 idx) const;
    virtual Material* GetContentMaterial() const;
    virtual Material* GetFrameMaterial(u32 frameIdx) const;
    virtual void DrawContent(const math::VEC2& basePt, const WindowFrameSize& frameSize, u8 alpha);
    virtual void DrawFrame(const math::VEC2& basePt, const Frame& frame, const WindowFrameSize& frameSize, u8 alpha);
    virtual void DrawFrame4(const math::VEC2& basePt, const Frame* frames, const WindowFrameSize& frameSize,
                            u8 alpha);
    virtual void DrawFrame8(const math::VEC2& basePt, const Frame* frames, const WindowFrameSize& frameSize,
                            u8 alpha);

    WindowFrameSize GetFrameSize(u8 frameNum, const Frame* frames);

    /* The class's run-time type record (0x80795A38); its parent is Pane's. */
    static const ut::detail::RuntimeTypeInfo typeInfo;

    /* +0xD8 */ InflationLRTB mContentInflation;
    /* +0xE8 */ Content mContent;
    /* +0x100 */ Frame* mFrames;
    /* +0x104 */ u8 mFrameNum;
    /* +0x105 */ u8 pad_0x105[3];
};

}  // namespace lyt
}  // namespace nw4r

#endif
