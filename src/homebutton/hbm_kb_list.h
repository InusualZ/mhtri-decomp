/*
 * homebutton/hbm_kb_list.h - the classes `homebutton/hbm_kb_list.cpp` defines (the keyboard's toolbar: its confirm and
 * cancel buttons and its key-layout switch) and the symbols other units call.
 */
#ifndef HOMEBUTTON_HBM_KB_LIST_H
#define HOMEBUTTON_HBM_KB_LIST_H

#include "types.h"
#include "homebutton/hbm_widget.h"

#ifdef __cplusplus
extern "C" {
#endif

/* The toolbar's destructor (HbmKbList::~HbmKbList below) as homebutton/hbm_value.cpp spells its call. */
#ifdef __cplusplus
}
#endif

#ifdef __cplusplus
#include "homebutton/hkb_keyboard.h"
#include "homebutton/hbm_value.h"
#include "homebutton/hbm_text_panel.h"

struct HbmTextPanel;

/* A sub-widget of the keyboard the toolbar resets when it closes (returned by the keyboard's slots 0x78, 0x80 and
 * 0x90): the toolbar calls its slots 0x38, 0x68 and 0x74.
 * size: 0x04 (approximation: only the vptr is touched) */
class HbmKbListSubWidgetView {
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
    /* slot +0x030 */ virtual void v_0x030();
    /* slot +0x034 */ virtual void v_0x034();
    /* slot +0x038 */ virtual void v_0x038();
    /* slot +0x03C */ virtual void v_0x03C();
    /* slot +0x040 */ virtual void v_0x040();
    /* slot +0x044 */ virtual void v_0x044();
    /* slot +0x048 */ virtual void v_0x048();
    /* slot +0x04C */ virtual void v_0x04C();
    /* slot +0x050 */ virtual void v_0x050();
    /* slot +0x054 */ virtual void v_0x054();
    /* slot +0x058 */ virtual void v_0x058();
    /* slot +0x05C */ virtual void v_0x05C();
    /* slot +0x060 */ virtual void v_0x060();
    /* slot +0x064 */ virtual void v_0x064();
    /* slot +0x068 */ virtual void v_0x068();
    /* slot +0x06C */ virtual void v_0x06C();
    /* slot +0x070 */ virtual void v_0x070();
    /* slot +0x074 */ virtual void v_0x074();
}; /* size: 0x04 */

/* The keyboard object the toolbar reports to (its +0x10): slot 0x68 returns the text panel whose candidates a
 * layout change closes; 0x78, 0x80 and 0x90 return the sub-widgets the toolbar resets when it closes.
 * size: 0x04 (approximation: only the vptr is touched) */
class HbmKbListOwnerView {
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
    /* slot +0x030 */ virtual void v_0x030();
    /* slot +0x034 */ virtual void v_0x034();
    /* slot +0x038 */ virtual void v_0x038();
    /* slot +0x03C */ virtual void v_0x03C();
    /* slot +0x040 */ virtual void v_0x040();
    /* slot +0x044 */ virtual void v_0x044();
    /* slot +0x048 */ virtual void v_0x048();
    /* slot +0x04C */ virtual void v_0x04C();
    /* slot +0x050 */ virtual void v_0x050();
    /* slot +0x054 */ virtual void v_0x054();
    /* slot +0x058 */ virtual void v_0x058();
    /* slot +0x05C */ virtual void v_0x05C();
    /* slot +0x060 */ virtual void v_0x060();
    /* slot +0x064 */ virtual void v_0x064();
    /* slot +0x068 */ virtual HbmTextPanel* getTextPanel();
    /* slot +0x06C */ virtual void v_0x06C();
    /* slot +0x070 */ virtual void v_0x070();
    /* slot +0x074 */ virtual void v_0x074();
    /* slot +0x078 */ virtual HbmKbListSubWidgetView* v_0x078();
    /* slot +0x07C */ virtual void v_0x07C();
    /* slot +0x080 */ virtual HbmKbListSubWidgetView* v_0x080();
    /* slot +0x084 */ virtual void v_0x084();
    /* slot +0x088 */ virtual void v_0x088();
    /* slot +0x08C */ virtual void v_0x08C();
    /* slot +0x090 */ virtual HbmKbListSubWidgetView* v_0x090();
}; /* size: 0x04 */

/* The toolbar's own part (class table 0x806504C8): the key-layout mode, the language, the allocator and the
 * keyboard it reports to.  size: 0x14 */
class HbmKbListBase {
public:
    /* slot +0x008 */ virtual void create(MEMAllocator* allocator) { mpAllocator = allocator; }
    /* slot +0x00C */ virtual void init() {}
    /* slot +0x010 */ virtual u32 getLayoutMode() { return mLayoutMode; }
    /* slot +0x014 */ virtual void setLayoutMode(u32 mode) {
        mLayoutMode = mode;
        if (mpOwner->getTextPanel() != NULL) {
            hbmPanelCloseCandidates(mpOwner->getTextPanel());
        }
    }
    /* slot +0x018 */ virtual void setLayoutType(s32 type) { mLayoutType = type; }
    /* slot +0x01C */ virtual s32 getLayoutType() { return mLayoutType; }

    /* +0x04 */ u8 mLayoutMode;
    /* +0x05 */ u8 pad_0x05[3];
    /* +0x08 */ s32 mLayoutType;
    /* +0x0C */ MEMAllocator* mpAllocator;
    /* +0x10 */ HbmKbListOwnerView* mpOwner;
}; /* size: 0x14 */

/* The keyboard's toolbar (class table 0x806503F8): the confirm and cancel buttons and the switch between the QWERTY
 * and the cellphone key layouts, drawn through its pane manager.  size: 0xC4 */
class HbmKbList : public HbmKbListBase, public HbmPaneManager {
public:
    HbmKbList();

    /* slot +0x008 */ virtual void create(MEMAllocator* allocator);
    /* slot +0x00C */ virtual void init();
    /* slot +0x014 */ virtual void setLayoutMode(u32 mode);
    /* slot +0x018 */ virtual void setLayoutType(s32 type);
    /* slot +0x094 */ virtual ~HbmKbList();
    /* slot +0x098 */ virtual void calcComponents();
    /* slot +0x09C */ virtual void confirm();
    /* slot +0x0A0 */ virtual void cancel();
    /* slot +0x0A4 */ virtual void switchLayoutMode(u32 mode);
    /* slot +0x0A8 */ virtual u8 getKeyChangeMode() { return mKeyChangeMode; }
    /* slot +0x0AC */ virtual void setKeyChangeMode(u8 mode);
    /* slot +0x0B0 */ virtual nw4r::lyt::Pane* findUpNode();
    /* slot +0x0B4 */ virtual nw4r::lyt::Pane* findDownNode();
    /* slot +0x0B8 */ virtual void setConfirmVisible(s32 visible);
    /* slot +0x0BC */ virtual void setCancelVisible(s32 visible);
    /* slot +0x0C0 */ virtual void setConfirmText(const wchar_t* text);
    /* slot +0x0C4 */ virtual void setCancelText(const wchar_t* text);
    /* slot +0x0C8 */ virtual void refreshKeys();

    /* +0xBC */ HbmEventHandler* mpPaneHandler;
    /* +0xC0 */ u8 mKeyChangeMode;
    /* +0xC1 */ u8 pad_0xC1[3];
}; /* size: 0xC4 */

/* The toolbar's pane handler (class table 0x806503D8): presses on the toolbar's buttons reach the toolbar.
 * size: 0x10 */
class HbmKbListPaneHandler : public HbmPaneHandler {
public:
    HbmKbListPaneHandler(HbmKbList* list) : mpList(list) {}

    /* slot +0x008 */ virtual ~HbmKbListPaneHandler();
    /* slot +0x018 */ virtual void onPaneEvent(HbmComponent* pComponent, u32 event, HbmPaneTrigger* pTrigger);

    /* +0x0C */ HbmKbList* mpList;
}; /* size: 0x10 */
#endif

#endif
