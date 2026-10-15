/*
 * homebutton/hbm_kb_child.h - the classes `homebutton/hbm_kb_child.cpp` defines (the prediction window's language
 * selector: the regional dictionary keys) and the symbols other units call.
 */
#ifndef HOMEBUTTON_HBM_KB_CHILD_H
#define HOMEBUTTON_HBM_KB_CHILD_H

#include "types.h"
#include "homebutton/hbm_widget.h"

#ifdef __cplusplus
extern "C" {
#endif

/* The selector's destructor (HbmKbChild::~HbmKbChild below) as homebutton/hbm_value.cpp spells its call. */
#ifdef __cplusplus
}
#endif

#ifdef __cplusplus
#include "homebutton/hkb_keyboard.h"
#include "homebutton/hbm_value.h"

/* One key of a band panel's key table (the selector's and the sign panel's): its kind, its pane name, its animations
 * and the key whose animations it shares.  size: 0x40 */
struct HbmKbChildKeyDef {
    /* +0x00 */ u32 type;
    /* +0x04 */ char name[20];
    /* +0x18 */ u32 numAnims;
    /* +0x1C */ const char* group;
    /* +0x20 */ const HkbAnimDef* anims[8];
}; /* size: 0x40 */

/* The window the selector closes with it (its +0xD4): slot 0x30 closes it.
 * size: 0x04 (approximation: only the vptr is touched) */
class HbmKbChildPopupView {
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
    /* slot +0x030 */ virtual void close();
}; /* size: 0x04 */

/* The selector's own part: the band's input widget with its allocator; slot 0x20 stores the key code in the
 * selector's +0x10 key code (its callers reach it through the one-argument view HbmKbKeyCodeView).  size: 0x1C */
class HbmKbChildBase : public HkbInputWidget {
public:
    /* slot +0x008 */ virtual ~HbmKbChildBase() {}
    /* slot +0x00C */ virtual void create(MEMAllocator* allocator) { mpAllocator = allocator; }
    /* slot +0x010 */ virtual void init();
    /* slot +0x020 */ virtual void onPaneEvent(u32 code, const char* name);

    /* +0x10 */ s32 mKeyCode;
    /* +0x14 */ MEMAllocator* mpAllocator;
    /* +0x18 */ u32 unused_0x18;
}; /* size: 0x1C */

/* The prediction window's language selector (class table 0x80650A48): the dictionary key of each language of the
 * three regions, the key-top pane that fades them in and out, and the window it closes; drawn through its pane
 * manager, and the listener of its key-top part's animations.  size: 0xD8 */
class HbmKbChild : public HbmKbChildBase, public HbmPaneManager, public HbmKeyListener {
public:
    /* slot +0x008 */ virtual ~HbmKbChild();
    /* slot +0x00C */ virtual void create(MEMAllocator* allocator);
    /* slot +0x010 */ virtual void init();
    /* slot +0x0A4 */ virtual void calcComponents();
    /* slot +0x0A8 */ virtual void open(s32 code, HbmKbChildPopupView* popup);
    /* slot +0x0AC */ virtual void activate();
    /* slot +0x0B0 */ virtual void deactivate();
    /* slot +0x0B4 */ virtual void close();
    /* slot +0x0B8 */ virtual u32 isOpen();
    /* slot +0x0BC */ virtual u32 isActive();
    /* slot +0x0C0 */ virtual void onAnimEvent(u32 event, HbmKey* key, HbmKeyAnim* anim);
    /* slot +0x0C4 */ virtual void fadeOtherKeys(const char* name);
    /* slot +0x0C8 */ virtual void resetKeys();
    /* slot +0x0CC */ virtual void selectRegion(s32 region);
    /* slot +0x0D0 */ virtual void setLanguage(u32 language);
    /* slot +0x0D4 */ virtual void setRegion(s32 region);
    /* slot +0x0D8 */ virtual HbmKey* findKey(s32 region, s32 code);

    /* +0xC8 */ u8 mbOpen;
    /* +0xC9 */ u8 mbActive;
    /* +0xCA */ u8 pad_0xCA[2];
    /* +0xCC */ HbmEventHandler* mpPaneHandler;
    /* +0xD0 */ s32 mRegion;
    /* +0xD4 */ HbmKbChildPopupView* mpPopup;
}; /* size: 0xD8 */

/* The selector's pane handler (class table 0x80650A28): presses on the dictionary keys reach the selector.
 * size: 0x10 */
class HbmKbChildPaneHandler : public HbmPaneHandler {
public:
    HbmKbChildPaneHandler(HbmKbChild* child) : mpChild(child) {}

    /* slot +0x008 */ virtual ~HbmKbChildPaneHandler();
    /* slot +0x018 */ virtual void onPaneEvent(HbmComponent* pComponent, u32 event, HbmPaneTrigger* pTrigger);

    /* +0x0C */ HbmKbChild* mpChild;
}; /* size: 0x10 */
#endif

#endif
