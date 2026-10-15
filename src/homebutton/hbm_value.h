/*
 * homebutton/hbm_value.h - declarations of the symbols owned by `homebutton/hbm_value.cpp` that other units call or read.
 */
#ifndef HOMEBUTTON_HBM_VALUE_H
#define HOMEBUTTON_HBM_VALUE_H

#include "types.h"
#include "homebutton/hbm_widget.h"

#ifdef __cplusplus
extern "C" {
#endif

/* r3 is the address the caller hands over (the +0x1A04 record of the panel at 0x805482CC and the
 * sub-object at its +0x10); `fn_8055A3E0` additionally takes the mode word the caller materialises. */
/* untyped: opaque band object, typed by the callers' views */
void fn_8055A3E0(void* sub, s32 mode);

u32 fn_8055A888(HbmWidget* self, int arg);

/* untyped: opaque band object, typed by the callers' views */
void* fn_8055B174(void* a, void* b);

/* untyped: opaque band object, typed by the callers' views */
void* fn_8055B898(void* self, int flag);

extern f32 lbl_8079D700;

#ifdef __cplusplus
}
#endif

#ifdef __cplusplus
#include "nw4r/fn_805012C4.h"
#include "homebutton/hkb_drawinfo.h"

struct MEMAllocator;
struct HkbManager;
class HbmEventHandler;
class HbmSoundPlayer;
class HbmKeyAnim;
class HbmKey;

namespace nw4r {
namespace lyt {
class Pane;
class Layout;
class ResourceAccessor;
class TextBox;
class AnimTransform;
}  // namespace lyt
}  // namespace nw4r

/* The scene object at the pane manager's +0x0C and +0x10, as the slots the band calls.
 * size: 0x4 (approximation: only the vptr is touched) */
class HbmPaneLayout {
public:
    /* slot +0x008 */ virtual void v_0x008();
    /* slot +0x00C */ virtual void v_0x00C();
    /* slot +0x010 */ virtual void v_0x010();
    /* slot +0x014 */ virtual void v_0x014();
    /* slot +0x018 */ virtual void v_0x018();
    /* slot +0x01C */ virtual void v_0x01C();
    /* slot +0x020 */ virtual void v_0x020();
    /* slot +0x024 */ virtual void v_0x024();
    /* slot +0x028 */ virtual void v_0x028();
    /* slot +0x02C */ virtual void v_0x02C();
    /* slot +0x030 */ virtual u32 v_0x030();
    /* slot +0x034 */ virtual void v_0x034();
    /* slot +0x038 */ virtual void setVisible(s32 visible);
    /* slot +0x03C */ virtual void v_0x03C();
    /* slot +0x040 */ virtual void v_0x040();
    /* slot +0x044 */ virtual void v_0x044();
    /* slot +0x048 */ virtual void v_0x048();
    /* slot +0x04C */ virtual void v_0x04C();
    /* slot +0x050 */ virtual void v_0x050();
    /* slot +0x054 */ virtual void v_0x054();
    /* slot +0x058 */ virtual void setEnable(s32 enable);
}; /* size: 0x4 */

/* The pane manager's cursor (embedded at its +0x8C): the slots the band calls.
 * size: 0x1C (approximation: the gap to the manager's end) */
class HbmPaneCursor {
public:
    /* slot +0x008 */ virtual void reset();
    /* slot +0x00C */ virtual void setTarget(u32 target);
    /* slot +0x010 */ virtual void v_0x010();
    /* slot +0x014 */ virtual u32 getTarget();

    /* +0x04 */ u8 unused_0x04[0x18];
}; /* size: 0x1C */

/* The band's pane manager (class table 0x806517A0 in this unit's .data): a layout, its root, its sound player, its
 * component list and its cursor.  Every keyboard, cellphone keypad and band widget derives from it; the slots
 * named v_0xNNN are not understood yet.  size: 0xA8 */
class HbmPaneManager {
public:
    /* slot +0x008 */ virtual ~HbmPaneManager();
    /* slot +0x00C */ virtual void create(MEMAllocator* allocator, HbmEventHandler* handler);
    /* slot +0x010 */ virtual void init();
    /* slot +0x014 */ virtual void v_0x014();
    /* slot +0x018 */ virtual void calcComponents();
    /* slot +0x01C */ virtual void drawComponents();
    /* slot +0x020 */ virtual u32 updateComponents();
    /* slot +0x024 */ virtual s32 onKeyRelease(HkbManager* manager);
    /* slot +0x028 */ virtual nw4r::lyt::Layout* getLayout();
    /* slot +0x02C */ virtual nw4r::lyt::TextBox* findTextBox(const char* name);
    /* slot +0x030 */ virtual u32 getHoldFrames(u32 channel, const char* name);
    /* slot +0x034 */ virtual void setInputActive(u8 active);
    /* slot +0x038 */ virtual void v_0x038();
    /* slot +0x03C */ virtual void v_0x03C();
    /* slot +0x040 */ virtual void v_0x040();
    /* slot +0x044 */ virtual void v_0x044();
    /* slot +0x048 */ virtual void setCursorTarget(u32 target);
    /* slot +0x04C */ virtual void resetCursor();
    /* slot +0x050 */ virtual void setPaneVisible(const char* name, s32 visible);
    /* slot +0x054 */ virtual s32 getPaneVisible(const char* name, u8* visible);
    /* slot +0x058 */ virtual void setPaneText(const char* name, const wchar_t* text);
    /* slot +0x05C */ virtual HbmKey* getPaneAnim(const char* name);
    /* slot +0x060 */ virtual HbmKey* getKeyAnim(u16 key);
    /* slot +0x064 */ virtual nw4r::ut::List* getComponentList();
    /* slot +0x068 */ virtual HbmPaneLayout* getRoot();
    /* slot +0x06C */ virtual void v_0x06C();
    /* slot +0x070 */ virtual void v_0x070();

    /* +0x04 */ nw4r::lyt::Layout* mpLyt;
    /* +0x08 */ u32 unused_0x08;
    /* +0x0C */ nw4r::lyt::ResourceAccessor* mpResAccessor;
    /* +0x10 */ HbmPaneLayout* mpRoot;
    /* +0x14 */ HbmSoundPlayer* mpSound;
    /* +0x18 */ nw4r::lyt::DrawInfo mDrawInfo;
    /* +0x6C */ nw4r::ut::List mComponents;
    /* +0x78 */ u8 unused_0x78[0x10];
    /* +0x88 */ u8 mbInputActive;
    /* +0x89 */ u8 pad_0x89[3];
    /* +0x8C */ HbmPaneCursor mCursor;
}; /* size: 0xA8 */

/* The band's animated pane component (class table 0x8065184C in this unit's .data): its pane, its animation list
 * and its state.  Keys, buttons and the widgets' parts derive from it.  size: 0x30 */
class HbmKey {
public:
    HbmKey(nw4r::lyt::Pane* pane) : mpPane(pane), mpCurAnim(NULL), mpNextAnim(NULL) {
        nw4r::ut::List_Init(&mAnims, 0x10);
        mState = 0;
    }

    /* slot +0x008 */ virtual void reset();
    /* slot +0x00C */ virtual void update();
    /* slot +0x010 */ virtual void setAnim(s32 anim);
    /* slot +0x014 */ virtual void setState(u32 state);
    /* slot +0x018 */ virtual s32 isAnimating();
    /* slot +0x01C */ virtual HbmKeyAnim* findAnim(u32 kind);
    /* slot +0x020 */ virtual ~HbmKey();

    void destroy(MEMAllocator* allocator);
    void addAnim(MEMAllocator* allocator, u32 kind, nw4r::lyt::AnimTransform* anim, u8 frame, u8 loop);
    void addGroupAnim(MEMAllocator* allocator, u32 kind, nw4r::lyt::AnimTransform* anim, const char* group, u8 frame,
                      u8 loop);

    /* +0x04 */ nw4r::lyt::Pane* mpPane;
    /* +0x08 */ nw4r::ut::List mAnims;
    /* +0x14 */ HbmKeyAnim* mpCurAnim;
    /* +0x18 */ HbmKeyAnim* mpNextAnim;
    /* +0x1C */ u8 unused_0x1C[0x10];
    /* +0x2C */ s32 mState;
}; /* size: 0x30 */
#endif

#endif
