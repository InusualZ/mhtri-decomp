/*
 * homebutton/hbm_anim_record.h - the classes `homebutton/hbm_anim_record.cpp` defines (the keyboard's sign panel: the
 * quick-chat phrase and gesture keys in pages) and the symbols other units call or read.
 */
#ifndef HOMEBUTTON_HBM_ANIM_RECORD_H
#define HOMEBUTTON_HBM_ANIM_RECORD_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* The panel's destructor (HbmAnimPanel::~HbmAnimPanel below) as homebutton/hbm_value.cpp spells its call. */
/* The sign keys' editable texts: 12 phrases of 45 wide characters and 12 gestures of 11. */
extern wchar_t hkb_sign_phrase_text[12][45];
extern wchar_t hkb_sign_gesture_text[12][11];

#ifdef __cplusplus
}
#endif

#ifdef __cplusplus
#include "homebutton/hkb_keyboard.h"
#include "homebutton/hbm_value.h"
#include "homebutton/hbm_kb_child.h"
#include "homebutton/hkb_sign_keys.h"

struct HbmTextPanel;

/* The panel's pages: their count and the sign keys whose texts they show.  size: 0x40 (approximation: the words
 * after the key table are zero and never read) */
struct HbmAnimPanelPages {
    /* +0x00 */ u8 count;
    /* +0x01 */ u8 pad_0x01[3];
    /* +0x04 */ const HkbSignKey* keys;
    /* +0x08 */ u8 unused_0x08[0x38];
}; /* size: 0x40 */

/* The object the panel asks for the keyboard's text panel (its +0x20): slot 0x68 returns it.
 * size: 0x04 (approximation: only the vptr is touched) */
class HbmAnimPanelTextOwnerView {
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
}; /* size: 0x04 */

/* A pane translation.  size: 0xC */
struct HbmAnimVec3 {
    /* +0x00 */ f32 x;
    /* +0x04 */ f32 y;
    /* +0x08 */ f32 z;
}; /* size: 0xC */

/* The pane record the owner's pane holder points at: its translation at +0x2C and, in bit 0 of the +0xBB flag byte,
 * its visibility.  size: 0xBC */
struct HbmAnimPane {
    /* +0x00 */ u8 unused_0x00[0x2C];
    /* +0x2C */ HbmAnimVec3 translate;
    /* +0x38 */ u8 unused_0x38[0x83];
    /* +0xBB */ u8 flags;
}; /* size: 0xBC */

/* What the owner's +0x4C base and the panel's pane manager hand back (slot 0x28): the pane record at +0x10.
 * size: 0x14 */
struct HbmAnimPaneHolder {
    /* +0x00 */ u8 unused_0x00[0x10];
    /* +0x10 */ HbmAnimPane* pane;
}; /* size: 0x14 */

/* The owner's +0x4C base: slot 0x28 returns the pane holder.
 * size: 0x04 (approximation: only the vptr is touched) */
class HbmAnimOwnerBaseView {
public:
    /* slot +0x008 */ virtual void v_0x008();
    /* slot +0x00C */ virtual void v_0x00C();
    /* slot +0x010 */ virtual void v_0x010();
    /* slot +0x014 */ virtual void v_0x014();
    /* slot +0x018 */ virtual void v_0x018();
    /* slot +0x01C */ virtual void v_0x01C();
    /* slot +0x020 */ virtual void v_0x020();
    /* slot +0x024 */ virtual void v_0x024();
    /* slot +0x028 */ virtual HbmAnimPaneHolder* getPaneHolder();
}; /* size: 0x04 */

/* The key the panel opens over (its +0xD8): slot 0x24 says whether it is busy, slot 0x30 closes it; its +0x4C base
 * holds the pane.  size: 0x50 (approximation: only the vptr and the +0x4C base are touched) */
class HbmAnimOwnerView {
public:
    /* slot +0x008 */ virtual void v_0x008();
    /* slot +0x00C */ virtual void v_0x00C();
    /* slot +0x010 */ virtual void v_0x010();
    /* slot +0x014 */ virtual void v_0x014();
    /* slot +0x018 */ virtual void v_0x018();
    /* slot +0x01C */ virtual void v_0x01C();
    /* slot +0x020 */ virtual void v_0x020();
    /* slot +0x024 */ virtual s32 isBusy();
    /* slot +0x028 */ virtual void v_0x028();
    /* slot +0x02C */ virtual void v_0x02C();
    /* slot +0x030 */ virtual void close();

    /* +0x04 */ u8 unused_0x04[0x48];
    /* +0x4C */ u32 base_4C;    /* the vptr of its HbmAnimOwnerBaseView base */
}; /* size: 0x50 */

/* The sign panel's own part: its page state and the page table, its allocator and the object holding the text panel;
 * slots 0x34..0x4C are its own (its table is not this unit's: slot 0x40 is kept at 0x80454970).  size: 0x24 */
class HbmAnimPanelBase : public HkbLayoutInputWidget {
public:
    /* slot +0x034 */ virtual void v_0x034() {}
    /* slot +0x038 */ virtual s32 isAnimating() { return mbAnimating; }
    /* slot +0x03C */ virtual void setPage(int page) {
        mPage = page;
        if ((u8)page < mpPages->count) {
            return;
        }
        mPage = 0;
    }
    /* slot +0x040 */ virtual void v_0x040();
    /* slot +0x044 */ virtual void prevPage();
    /* slot +0x048 */ virtual void nextPage();
    /* slot +0x04C */ virtual void close();
    /* slot +0x00C */ virtual void create(MEMAllocator* allocator) {
        mpAllocator = allocator;
        resetPage();
    }
    /* slot +0x010 */ virtual void init() { resetPage(); }
    /* slot +0x020 */ virtual void onPaneEvent(u32 event, const char* paneName);

    void resetPage();
    void stepPageDown();
    void stepPageUp();

    /* +0x14 */ u8 unused_0x14;
    /* +0x15 */ u8 mbAnimating;
    /* +0x16 */ u8 mMode;
    /* +0x17 */ u8 mPage;
    /* +0x18 */ const HbmAnimPanelPages* mpPages;
    /* +0x1C */ MEMAllocator* mpAllocator;
    /* +0x20 */ HbmAnimPanelTextOwnerView* mpTextOwner;
}; /* size: 0x24 */

/* The keyboard's sign panel (class table 0x80651448): the 20 quick-chat keys, their phrase and gesture pages, the
 * page and close buttons and the key-top pane it slides over its owner key; drawn through its pane manager, and the
 * listener of its parts' animations.  size: 0xDC */
class HbmAnimPanel : public HbmAnimPanelBase, public HbmPaneManager, public HbmKeyListener {
public:
    /* slot +0x008 */ virtual ~HbmAnimPanel();
    /* slot +0x00C */ virtual void create(MEMAllocator* allocator);
    /* slot +0x010 */ virtual void init();
    /* slot +0x020 */ virtual void onPaneEvent(u32 event, const char* paneName);
    /* slot +0x024 */ virtual s32 isBusy();
    /* slot +0x028 */ virtual void setLayoutType(s32 type);
    /* slot +0x034 */ virtual void v_0x034();
    /* slot +0x044 */ virtual void prevPage();
    /* slot +0x048 */ virtual void nextPage();
    /* slot +0x04C */ virtual void close();
    /* slot +0x0D0 */ virtual void calcComponents();
    /* slot +0x0D4 */ virtual void open(HbmAnimOwnerView* owner, u8 mode, u8 page);
    /* slot +0x0D8 */ virtual u8 isOpen();
    /* slot +0x0DC */ virtual void onAnimEvent(u32 event, HbmKey* key, HbmKeyAnim* anim);
    /* slot +0x0E0 */ virtual u32 updateComponents();
    /* slot +0x0E4 */ virtual s32 onKeyRelease(HkbManager* manager);
    /* slot +0x0E8 */ virtual void refresh();
    /* slot +0x0EC */ virtual void setPhraseTexts(const wchar_t** texts);
    /* slot +0x0F0 */ virtual void setGestureTexts(const wchar_t** texts);
    /* slot +0x0F4 */ virtual void prevPageWrap();
    /* slot +0x0F8 */ virtual void armKeys();
    /* slot +0x0FC */ virtual void resetKeys();
    /* slot +0x100 */ virtual void fadeKeys();
    /* slot +0x104 */ virtual void updateLabels(u16 page, u16 newPage);
    /* slot +0x108 */ virtual void setPageNumber(s32 page);
    /* slot +0x10C */ virtual void onClosed();
    /* slot +0x110 */ virtual void show();
    /* slot +0x114 */ virtual void scrollKeysBack();
    /* slot +0x118 */ virtual void scrollKeysForward();

    /* +0xD0 */ u8 mbOpen;
    /* +0xD1 */ u8 mbShown;
    /* +0xD2 */ u8 pad_0xD2[2];
    /* +0xD4 */ HbmEventHandler* mpPaneHandler;
    /* +0xD8 */ HbmAnimOwnerView* mpOwner;
}; /* size: 0xDC */

/* The panel's pane handler (class table 0x80651428): presses on the panel's keys reach the panel.  size: 0x10 */
class HbmAnimPanelPaneHandler : public HbmPaneHandler {
public:
    HbmAnimPanelPaneHandler(HbmAnimPanel* panel) : mpPanel(panel) {}

    /* slot +0x008 */ virtual ~HbmAnimPanelPaneHandler();
    /* slot +0x018 */ virtual void onPaneEvent(HbmComponent* pComponent, u32 event, HbmPaneTrigger* pTrigger);

    /* +0x0C */ HbmAnimPanel* mpPanel;
}; /* size: 0x10 */
#endif

#endif
