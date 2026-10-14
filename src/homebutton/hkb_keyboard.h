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

#ifdef __cplusplus
struct MEMAllocator;
class HbmPaneManager;
class HkbInputWidget;

/* The band's event handler: the pane manager it reports to (class table 0x8064DE70 in this unit's .data).
 * size: 0x8 */
class HbmEventHandler {
public:
    /* slot +0x008 */ virtual ~HbmEventHandler();
    /* untyped: caller-owned payload - the argument record of the event */
    /* slot +0x00C */ virtual void onEvent(u32 id, u32 event, void* data);
    /* slot +0x010 */ virtual void setManager(HbmPaneManager* manager);
    /* slot +0x014 */ virtual HbmPaneManager* getManager();

    /* +0x04 */ HbmPaneManager* mpManager;
}; /* size: 0x8 */

/* The receiver of an input widget's notifications (the widget's +0x0C): the slots the band calls.
 * size: 0x4 (approximation: only the vptr is touched) */
class HkbEventSink {
public:
    /* slot +0x008 */ virtual void v_0x008();
    /* slot +0x00C */ virtual void v_0x00C();
    /* slot +0x010 */ virtual void v_0x010();
    /* slot +0x014 */ virtual void v_0x014();
    /* untyped: caller-owned payload - the argument record of event `id` */
    /* slot +0x018 */ virtual void onEvent(u32 id, void* data);
    /* slot +0x01C */ virtual void setOwner(HkbInputWidget* owner);
}; /* size: 0x4 */

/* The root of the band's input widgets (the software keyboard, the cellphone keypad input and the band's widgets
 * derive from it): its notification sink and its layout type; the slots 0x08..0x30 are the ones every derived
 * class table shares.  size: 0x14 (approximation: the fields the shared members read) */
class HkbInputWidget {
public:
    /* slot +0x008 */ virtual ~HkbInputWidget();
    /* slot +0x00C */ virtual void create(MEMAllocator* allocator);
    /* slot +0x010 */ virtual void init();
    /* slot +0x014 */ virtual void setEventSink(HkbEventSink* sink);
    /* untyped: caller-owned payload - the argument record of event `id` */
    /* slot +0x018 */ virtual void sendEvent(u32 id, void* data);
    /* slot +0x01C */ virtual void onCommand(u32 command, u32 arg);
    /* slot +0x020 */ virtual void onPaneEvent(u32 event, const char* paneName);
    /* slot +0x024 */ virtual s32 isBusy();
    /* slot +0x028 */ virtual void setLayoutType(s32 type);
    /* slot +0x02C */ virtual s32 getLayoutType();
    /* slot +0x030 */ virtual void calc();

    /* +0x04 */ u32 unused_0x04;
    /* +0x08 */ u32 unused_0x08;
    /* +0x0C */ HkbEventSink* mpEventSink;
    /* +0x10 */ s32 mLayoutType;
}; /* size: 0x14 */
#endif

#endif
