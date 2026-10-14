/*
 * homebutton/hkb_keyboard.h - declarations of the symbols owned by `homebutton/hkb_keyboard.cpp` that other units call or read.
 */
#ifndef HOMEBUTTON_HKB_KEYBOARD_H
#define HOMEBUTTON_HKB_KEYBOARD_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* The 7 base destructors the range's deleting destructors dispatch into (each in the unsplit band). */
/* untyped: opaque band object, typed by the callers' views */
void fn_805385D0(void* self, s32 flag);

#ifdef __cplusplus
}
#endif

#ifdef __cplusplus
#include "nw4r/fn_805012C4.h"
#include "homebutton/hkb_material.h"

namespace nw4r {
namespace lyt {

class Group;
class AnimTransform;

namespace detail {

/* The texture coordinates of one quad: four corners. size: 0x20 */
struct TexCoordQuad {
    /* +0x00 */ math::VEC2 coord[4];
};

/* A pane's texture-coordinate quads, allocated from the layout allocator. size: 0x8 */
class TexCoordAry {
public:
    TexCoordAry();
    void Free();
    void Reserve(u8 num);
    void SetSize(u8 num);
    /* untyped: byte range - the resource's packed TexCoordQuad array */
    void Copy(const void* pResTexCoord, u8 texCoordNum);

    u8 GetSize() const { return mNum; }
    const TexCoordQuad* GetArray() const { return mpData; }
    bool IsEmpty() const { return mCap == 0; }

    /* +0x0 */ u8 mCap;
    /* +0x1 */ u8 mNum;
    /* +0x2 */ u8 pad_0x2[2];
    /* +0x4 */ TexCoordQuad* mpData;
};

/* Returns whether two pane or group names agree in their first 16 characters. */
bool EqualsResName(const char* name1, const char* name2);

/* Returns whether two material names agree in their first 20 characters. */
bool EqualsMaterialName(const char* name1, const char* name2);
/* Returns whether a binary resource header carries `signature` and the big-endian byte-order mark. */
bool TestFileHeader(const ut::BinaryFileHeader& fileHeader, u32 signature);
/* Returns whether the vertex colours or the alpha are not plain white. */
bool IsModulateVertexColor(ut::Color* vtxColors, u8 glbAlpha);
/* Returns a colour with its alpha scaled by `alpha`. */
const ut::Color MultipleAlpha(const ut::Color col, u8 alpha);
/* Sets the GX vertex format: position, optional colour and `texCoordNum` texture coordinates. */
void SetVertexFormat(bool bModulate, u8 texCoordNum);
/* Draws one quad of `size` from `basePt` with its texture coordinates and corner colours. */
void DrawQuad(const math::VEC2& basePt, const Size& size, u8 num, const TexCoordQuad* texCoords,
              const ut::Color* vtxColors, u8 alpha);
/* Binds an animation to every pane of a group. */
void BindAnimation(Group* pGroup, AnimTransform* pAnimTrans, bool bRecursive, bool bDisable);

}  // namespace detail
}  // namespace lyt
}  // namespace nw4r
#endif

#endif
