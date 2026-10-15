/*
 * homebutton/hbm_anim_record.cpp - software-keyboard code of the tiHKBManager band: the keyboard's sign panel
 * (HbmAnimPanel): the 20 quick-chat keys with their phrase and gesture pages, the page and close buttons, the key-top
 * pane it slides over its owner key, its pane handler and its key parts.
 *
 * RANGE.     .text 0x80556FC4..0x80558EB4 (61 functions), .ctors 0x8056F40C..0x8056F410, .rodata 0x8057BAB0..0x8057BDB0,
 *            .data 0x80650B40..0x80651568, .bss 0x807907D0..0x80790D10, .sdata 0x80794628..0x80794638.
 * FLAGS.     GC/3.0a5.2 + cflags_hkb (configure.py); the retail object has no extab/extabindex, so `#pragma exceptions off`.
 * FLAGS.     Scoped `#pragma auto_inline on` around the page helpers (resetPage, stepPageDown, stepPageUp): retail inlines
 *            them into their callers and also keeps their out-of-line copies.
 * NAMES.     GUESS file name: the band's `hbm_` prefix + the page record; no `__FILE__` string, no runtime-dump name.
 *            The unit loads `fs_signWindow_a_*.brlan` and binds the N_SGN and P_SGNkey panes: the sign panel.
 * NAMES.     GUESS classes HbmAnimPanel (three bases: the class table 0x80651448 holds the HbmPaneManager table at +0x50
 *            and the HbmKeyListener table at +0xC4, as the destructor's stores show), HbmAnimPanelBase (its primary base:
 *            HkbInputWidget's slots 0x08..0x30, then 0x34..0x4C), HbmAnimPanelPaneHandler (table 0x80651428),
 *            HbmAnimPart and its kinds HbmAnimPartA/B/C; members named from the panes they bind and the events they send.
 * NAMES.     GUESS data names hbmAnimPanelAnimDefs, hbmAnimPanelButtons, hbmAnimPanelPages, hbmAnimPanelEventDefault,
 *            hbmAnimSignKeyNames, hbmAnimPanelKeyGroup, hbmAnimPanelCloseGroup, hbmAnimPanelKeyDefs.
 * SHAPES.    The key table's group words are pointer variables, so the unit's static initialiser (the .ctors entry)
 *            patches them as retail's 0x80558E18 does.
 * SHAPES.    The pane visibility is `(flags & 0xFE) | bit` through hbmAnimSetPaneVisible: retail's `rlwinm 24,30` (+ `ori`)
 *            pair; a bitfield store compiles to `rlwimi` instead.
 * SHAPES.    open hands slot 0x104 `mPage` twice (retail's two `mr` from the parameter register) and declares `source`
 *            before `keytop` (register colouring).
 * RESIDUALS. The parts' listener is stored after the HbmKey constructor (retail stores it inside it): HbmKey has no
 *            (pane, listener) constructor and its +0x18 is typed HbmKeyAnim* in homebutton/hbm_value.h (request
 *            hb-kb-parts#2).
 * RESIDUALS. 0x80556FE4 (a second copy of the page reset) and 0x805571CC (the base's empty slot 0x34) are emitted by
 *            retail for no caller in this unit; they keep their C spelling.
 * RESIDUALS. The two onPaneEvent bodies and updateLabels: the inline lookups' index, page and key-table registers come
 *            out in another order than retail's.
 * RESIDUALS. HbmAnimPanelBase's table is not this unit's: its key function is slot 0x40, kept at 0x80454970 by another
 *            unit, so the member is declared and never defined here.
 */

#include "types.h"
#include "homebutton/hbm_anim_record.h"
#include "homebutton/hbm_text_panel.h"
#include "MSL_C/wstring.h"
#include "MSL_C/wprintf.h"
#include "homebutton/hkb_layout.h"
#include "homebutton/hkb_textbox.h"
#include "homebutton/hbm_kb_event.h"
#include "homebutton/hbm_hermite.h"
#include "MEM/MEMAllocFromAllocator.h"
#include "MSL_C/string.h"
#include "MSL_C/wstring.h"
#include "MSL_C/strtoul.h"
#include "MSL/strlen.h"
#include "Runtime.PPCEABI.H/memset.h"

/* The retail object has no extab/extabindex: exceptions are off for this band. */
#pragma exceptions off

extern "C" void wmemset(void* dst, int value, u16 size);

/* A close or page button of the panel and the code its press stands for.  size: 0x18 */
struct HbmAnimPanelButton {
    /* +0x00 */ char name[20];
    /* +0x14 */ s32 code;
}; /* size: 0x18 */

/* The event a sign key press sends (event 5): the text and the mode it is typed in.  size: 0x10 */
struct HbmAnimSignEvent {
    /* +0x00 */ u16 id;
    /* +0x02 */ u16 kind;
    /* +0x04 */ u8 mode;
    /* +0x05 */ u8 pad_0x05[3];
    /* +0x08 */ u32 unused_0x08;
    /* +0x0C */ const wchar_t* text;
}; /* size: 0x10 */

/* The sound player of the panel's pane manager, as the sign keys use it: slot 0x24 plays a gesture, slot 0x28 the
 * phrase sound.  size: 0x04 (approximation: only the vptr is touched) */
class HbmAnimSoundView {
public:
    /* slot +0x008 */ virtual void v_0x008();
    /* slot +0x00C */ virtual void v_0x00C();
    /* slot +0x010 */ virtual void v_0x010();
    /* slot +0x014 */ virtual void play(s32 sound);
    /* slot +0x018 */ virtual void v_0x018();
    /* slot +0x01C */ virtual void v_0x01C();
    /* slot +0x020 */ virtual void v_0x020();
    /* slot +0x024 */ virtual void playGesture(s32 gesture);
    /* slot +0x028 */ virtual void playPhrase();
}; /* size: 0x04 */

/* A panel key: a band key with its state word and its kind; its class table (0x806513FC) and the out-of-line copy of
 * its inline destructor (0x805576EC) are this unit's.  size: 0x34 */
class HbmAnimPart : public HbmKey {
public:
    HbmAnimPart(nw4r::lyt::Pane* pane, HbmKeyListener* listener) : HbmKey(pane, listener) {}

    virtual void reset();
    virtual void setState(u32 state);
    virtual ~HbmAnimPart() {}
    virtual void show();
    virtual s32 getType();

    /* +0x30 */ s32 mType;
}; /* size: 0x34 */

/* The key-top pane (kind 0; class table 0x806513D0): shows and hides itself.  size: 0x34 */
class HbmAnimPartB : public HbmAnimPart {
public:
    HbmAnimPartB(nw4r::lyt::Pane* pane, HbmKeyListener* listener) : HbmAnimPart(pane, listener) { mType = 0; }

    virtual void setAnim(s32 anim);
    virtual void reset();
    virtual ~HbmAnimPartB();
}; /* size: 0x34 */

/* The keys' frame (kind 1; class table 0x806513A4): steps its idle and scrolling states.  size: 0x34 */
class HbmAnimPartC : public HbmAnimPart {
public:
    HbmAnimPartC(nw4r::lyt::Pane* pane, HbmKeyListener* listener) : HbmAnimPart(pane, listener) { mType = 1; }

    virtual void setAnim(s32 anim);
    virtual void reset();
    virtual ~HbmAnimPartC();
}; /* size: 0x34 */

/* A sign key (kind 2; class table 0x80651378): steps a seven-state selector.  size: 0x34 */
class HbmAnimPartA : public HbmAnimPart {
public:
    HbmAnimPartA(nw4r::lyt::Pane* pane, HbmKeyListener* listener) : HbmAnimPart(pane, listener) { mType = 2; }

    virtual void setAnim(s32 anim);
    virtual void reset();
    virtual ~HbmAnimPartA();
}; /* size: 0x34 */

/* The key animations (.rodata 0x8057BAB0). */
const HkbAnimDef hbmAnimPanelAnimDefs[9] = {
    {0, "fs_signWindow_a_SGN_normal.brlan"},
    {1, "fs_signWindow_a_SGN_FADE-IN.brlan"},
    {8, "fs_signWindow_a_Scroll_FADE-OUT.brlan"},
    {2, "fs_signWindow_a_SGN_Focus-IN.brlan"},
    {6, "fs_signWindow_a_SGN_Focus-OUT.brlan"},
    {3, "fs_signWindow_a_SGN_Roll_over.brlan"},
    {4, "fs_signWindow_a_SGN_Pushed.brlan"},
    {7, "fs_signWindow_a_SGN_scroll_next.brlan"},
    {5, "fs_signWindow_a_SGN_scroll_prev.brlan"},
};

/* The close and page buttons (.rodata 0x8057BD18). */
const HbmAnimPanelButton hbmAnimPanelButtons[3] = {
    {"B_SGNkey_close", 24},
    {"B_SGNkey_prev", 26},
    {"B_SGNkey_next", 25},
};

/* The panel's pages (.rodata 0x8057BD60). */
const HbmAnimPanelPages hbmAnimPanelPages = {4, {0}, hkb_sign_keys};

/* The event a sign key press starts from (.rodata 0x8057BDA0). */
const HbmAnimSignEvent hbmAnimPanelEventDefault = {0x2A, 3, 0, {0}, 0, NULL};

/* The sign keys' part names in key order (.data 0x80650C30). */
const char* hbmAnimSignKeyNames[20] = {
    "P_SGNkey_12", "P_SGNkey_01", "P_SGNkey_02", "P_SGNkey_03", "P_SGNkey_04",
    "P_SGNkey_05", "P_SGNkey_06", "P_SGNkey_07", "P_SGNkey_08", "P_SGNkey_09",
    "P_SGNkey_10", "P_SGNkey_11", "P_SGNkey_00", "P_SGNkey_13", "P_SGNkey_14",
    "P_SGNkey_15", "P_SGNkey_16", "P_SGNkey_17", "P_SGNkey_18", "P_SGNkey_19",
};

/* The keys whose animations the sign keys and the page buttons share. */
const char* hbmAnimPanelKeyGroup = "P_SGNkey_00";
const char* hbmAnimPanelCloseGroup = "P_SGNkey_close";

#define HBM_ANIM_PANEL_KEY_ANIMS                                                                                       \
    {&hbmAnimPanelAnimDefs[0], &hbmAnimPanelAnimDefs[1], &hbmAnimPanelAnimDefs[2], &hbmAnimPanelAnimDefs[3],           \
     &hbmAnimPanelAnimDefs[4], &hbmAnimPanelAnimDefs[5], &hbmAnimPanelAnimDefs[6]}
#define HBM_ANIM_PANEL_SIGN_KEY(name) {2, name, 7, hbmAnimPanelKeyGroup, HBM_ANIM_PANEL_KEY_ANIMS}

/* The key-top pane, the keys' frame, the sign keys and the buttons (.data 0x80650C90). */
HbmKbChildKeyDef hbmAnimPanelKeyDefs[25] = {
    {0, "N_SGNkeytop_all", 3, NULL, {&hbmAnimPanelAnimDefs[0], &hbmAnimPanelAnimDefs[1], &hbmAnimPanelAnimDefs[2]}},
    {1, "N_SGNkeyall", 3, NULL, {&hbmAnimPanelAnimDefs[0], &hbmAnimPanelAnimDefs[7], &hbmAnimPanelAnimDefs[8]}},
    HBM_ANIM_PANEL_SIGN_KEY("P_SGNkey_12"),
    HBM_ANIM_PANEL_SIGN_KEY("P_SGNkey_01"),
    HBM_ANIM_PANEL_SIGN_KEY("P_SGNkey_02"),
    HBM_ANIM_PANEL_SIGN_KEY("P_SGNkey_03"),
    HBM_ANIM_PANEL_SIGN_KEY("P_SGNkey_04"),
    HBM_ANIM_PANEL_SIGN_KEY("P_SGNkey_05"),
    HBM_ANIM_PANEL_SIGN_KEY("P_SGNkey_06"),
    HBM_ANIM_PANEL_SIGN_KEY("P_SGNkey_07"),
    HBM_ANIM_PANEL_SIGN_KEY("P_SGNkey_08"),
    HBM_ANIM_PANEL_SIGN_KEY("P_SGNkey_09"),
    HBM_ANIM_PANEL_SIGN_KEY("P_SGNkey_10"),
    HBM_ANIM_PANEL_SIGN_KEY("P_SGNkey_11"),
    HBM_ANIM_PANEL_SIGN_KEY("P_SGNkey_00"),
    HBM_ANIM_PANEL_SIGN_KEY("P_SGNkey_13"),
    HBM_ANIM_PANEL_SIGN_KEY("P_SGNkey_14"),
    HBM_ANIM_PANEL_SIGN_KEY("P_SGNkey_15"),
    HBM_ANIM_PANEL_SIGN_KEY("P_SGNkey_16"),
    HBM_ANIM_PANEL_SIGN_KEY("P_SGNkey_17"),
    HBM_ANIM_PANEL_SIGN_KEY("P_SGNkey_18"),
    HBM_ANIM_PANEL_SIGN_KEY("P_SGNkey_19"),
    {2, "P_SGNkey_prev", 7, hbmAnimPanelCloseGroup, HBM_ANIM_PANEL_KEY_ANIMS},
    {2, "P_SGNkey_next", 7, hbmAnimPanelCloseGroup, HBM_ANIM_PANEL_KEY_ANIMS},
    {2, "P_SGNkey_close", 7, NULL, HBM_ANIM_PANEL_KEY_ANIMS},
};

/* The sign keys' editable texts. */
wchar_t hkb_sign_phrase_text[12][45];
wchar_t hkb_sign_gesture_text[12][11];

/* Sets or clears the pane's visibility bit (bit 0 of +0xBB), keeping the other flags. */
static inline void hbmAnimSetPaneVisible(HbmAnimPane* pane, BOOL visible) {
    pane->flags = (u8)((pane->flags & 0xFE) | (visible ? 1 : 0));
}

/* Returns the code of the close or page button `name`, or 33 (a sign key). */
static inline s32 hbmAnimFindButton(const char* name) {
    for (u16 i = 0; i < 3; i++) {
        if (hbmNameMatches(hbmAnimPanelButtons[i].name, name)) {
            return hbmAnimPanelButtons[i].code;
        }
    }
    return 33;
}

/* Returns the text the sign key `name` shows on `page`, or NULL. */
static inline const wchar_t* hbmAnimFindSignText(const HbmAnimPanelPages* pages, u16 page, const char* name) {
    u16 i;
    const HkbSignKey* keys = pages->keys;
    for (i = 0; i < 20; i++) {
        if (hbmNameMatches(keys[i].button_name, name)) {
            return (&keys[i].phrase)[page];
        }
    }
    return NULL;
}

#pragma push
#pragma auto_inline on
/* 0x80556FC4 (0x20): Puts the panel back on its first page. */
void HbmAnimPanelBase::resetPage() {
    mPage = 0;
    mbAnimating = 0;
    mMode = 0;
    mpPages = &hbmAnimPanelPages;
}

/* 0x80556FE4 (0x20): Puts a panel record back on its first page. */
extern "C" void fn_80556FE4(HbmAnimRecord* self) {
    self->flag_17 = 0;
    self->flag_15 = 0;
    self->flag_16 = 0;
    self->table = (const u8*)&hbmAnimPanelPages;
}
#pragma pop

/* 0x80557004 (0x1C8): Runs a pressed button, or sends the pressed sign key's text in the current page (event 5). */
void HbmAnimPanelBase::onPaneEvent(u32 event, const char* paneName) {
    if (event != 4) {
        return;
    }
    s32 code = hbmAnimFindButton(paneName);
    if (code != 33) {
        switch (code) {
        case 24:
            close();
            break;
        case 26:
            prevPage();
            break;
        case 25:
            nextPage();
            break;
        }
    } else {
        const wchar_t* text = hbmAnimFindSignText(mpPages, mPage, paneName);
        if (text != NULL) {
            HbmAnimSignEvent ev = hbmAnimPanelEventDefault;
            ev.mode = mMode;
            ev.text = text;
            if (hbmPanelIsSingleLine((struct HbmTextPanel*)mpTextOwner->getTextPanel())) {
                ev.mode = 1;
            }
            if (mPage == 0) {
                sendEvent(5, &ev);
            }
        }
    }
}

/* 0x805571CC (0x4): Does nothing. */
extern "C" void fn_805571CC(void) {}

#pragma push
#pragma auto_inline on
/* 0x805571D0 (0x2C): Steps the page back, wrapping to the last page. */
void HbmAnimPanelBase::stepPageDown() {
    if (mPage == 0) {
        mPage = mpPages->count - 1;
    } else {
        mPage = mPage - 1;
    }
}

/* 0x805571FC (0x2C): Steps the page forward, wrapping to the first page. */
void HbmAnimPanelBase::stepPageUp() {
    u8 next = mPage + 1;
    mPage = next;
    if (next >= mpPages->count) {
        mPage = 0;
    }
}
#pragma pop

/* 0x80557228 (0xE8): Destroys the pane handler and the keys. */
HbmAnimPanel::~HbmAnimPanel() {
    mpPaneHandler->~HbmEventHandler();
    MEMFreeToAllocator(mpAllocator, mpPaneHandler);
    for (HbmKey* pKey = (HbmKey*)nw4r::ut::List_GetNext(&mComponents, NULL); pKey != NULL;
         pKey = (HbmKey*)nw4r::ut::List_GetNext(&mComponents, NULL)) {
        nw4r::ut::List_Remove(&mComponents, pKey);
        pKey->destroy(mpAllocator);
    }
}

/* 0x80557310 (0x40): Releases the object's storage and returns the pointer. */
extern "C" void* fn_80557310(void* self, int flag) {
    if (self != 0) {
        if (flag > 0) {
            operator delete(self);
        }
    }
    return self;
}

/* 0x80557350 (0x40): Destroys the pane handler. */
HbmAnimPanelPaneHandler::~HbmAnimPanelPaneHandler() {}

/* 0x80557390 (0x35C): Creates the pane handler and the pane manager, hides the root, and creates one part per key with
 * its animations. */
void HbmAnimPanel::create(MEMAllocator* allocator) {
    const char* group;

    HbmAnimPanelBase::create(allocator);
    HbmAnimPanelPaneHandler* pHandler =
        new (MEMAllocFromAllocator(allocator, sizeof(HbmAnimPanelPaneHandler))) HbmAnimPanelPaneHandler(this);
    mpPaneHandler = pHandler;
    HbmPaneManager::create(allocator, pHandler);
    mpRoot->setVisible(0);
    mpRoot->setEnable(1);
    for (u16 i = 0; i < 25; i++) {
        const HbmKbChildKeyDef* def = &hbmAnimPanelKeyDefs[i];
        HbmAnimPart* pPart = NULL;
        switch (def->type) {
        case 2:
            pPart = new (MEMAllocFromAllocator(allocator, sizeof(HbmAnimPartA)))
                HbmAnimPartA(findTextBox(def->name), this);
            break;
        case 1:
            pPart = new (MEMAllocFromAllocator(allocator, sizeof(HbmAnimPartC)))
                HbmAnimPartC(findTextBox(def->name), this);
            break;
        case 0:
            pPart = new (MEMAllocFromAllocator(allocator, sizeof(HbmAnimPartB)))
                HbmAnimPartB(findTextBox(def->name), this);
            break;
        }
        nw4r::ut::List_Append(&mComponents, pPart);
        group = def->group;
        u16 j = 0;
        u32 numAnims = def->numAnims;
        for (; j < numAnims; j++) {
            const void* res = mpResAccessor->GetResource(0, def->anims[j]->name, NULL);
            nw4r::lyt::AnimTransform* anim = getLayout()->CreateAnimTransform(res, mpResAccessor);
            if (group != NULL) {
                pPart->addGroupAnim(allocator, def->anims[j]->kind, anim, group, 0, 1);
            } else {
                pPart->addAnim(allocator, def->anims[j]->kind, anim, 0, 1);
            }
        }
    }
    init();
}

/* 0x80557744 (0x1CC): Resets the page, the key-top pane and the page buttons, restores the default sign texts, labels
 * the keys and lays the panes out. */
void HbmAnimPanel::init() {
    HbmAnimPanelBase::init();
    getPaneAnim("N_SGNkeytop_all")->setState(0);
    setPaneVisible("N_SGNkeytop_all", 1);
    getPaneAnim("P_SGNkey_prev")->setState(0);
    getPaneAnim("P_SGNkey_next")->setState(0);
    for (int i = 0; i < 12; i++) {
        wcscpy((u16*)hkb_sign_phrase_text[i], (const u16*)hkb_sign_phrase_defaults[i]);
    }
    for (int i = 0; i < 12; i++) {
        wcscpy((u16*)hkb_sign_gesture_text[i], (const u16*)hkb_sign_gesture_defaults[i]);
    }
    updateLabels(mPage, mPage);
    setPageNumber(mPage);
    mpLyt->Animate(0);
    mpLyt->CalculateMtx(mDrawInfo);
}

/* 0x80557910 (0x198): Runs the base's handling of a press, then plays the close sound, or the sign key's gesture or
 * phrase. */
void HbmAnimPanel::onPaneEvent(u32 event, const char* paneName) {
    HbmAnimPanelBase::onPaneEvent(event, paneName);
    if (event != 4) {
        return;
    }
    s32 code = hbmAnimFindButton(paneName);
    if (code != 33) {
        switch (code) {
        case 24:
            mpSound->play(9);
            break;
        }
    } else {
        const wchar_t* text = hbmAnimFindSignText(mpPages, mPage, paneName);
        if (text != NULL) {
            if (mpOwner->isBusy() == 0 && mPage == 0) {
                sendEvent(39, NULL);
            }
            if (mPage != 0) {
                ((HbmAnimSoundView*)mpSound)->playGesture(atoi(paneName + 9));
            } else {
                ((HbmAnimSoundView*)mpSound)->playPhrase();
            }
        }
    }
}

/* 0x80557AA8 (0x1AC): Opens the panel over a key at a mode and page: resets the keys, shows the page, moves the
 * key-top pane onto the key's pane and fades it in. */
void HbmAnimPanel::open(HbmAnimOwnerView* owner, u8 mode, u8 page) {
    HbmAnimPane* source;
    HbmKey* keytop;
    HbmAnimPane* target;

    resetKeys();
    fadeKeys();
    getRoot()->v_0x010();
    mbOpen = 1;
    mbShown = 0;
    mpOwner = owner;
    resetCursor();
    mMode = mode;
    mPage = page;
    updateLabels(mPage, mPage);
    setPageNumber(mPage);
    keytop = getPaneAnim("N_SGNkeytop_all");
    source = ((HbmAnimOwnerBaseView*)&mpOwner->base_4C)->getPaneHolder()->pane;
    target = (HbmAnimPane*)getLayout()->mpRootPane;
    target->translate = source->translate;
    keytop->setAnim(6);
    mpSound->play(14);
    mpSound->setEnable(0);
}

/* 0x80557C54 (0xC8): Starts closing the panel: shows the owner key's pane again while the key is idle and fades the
 * key-top pane out. */
void HbmAnimPanel::close() {
    if (mpOwner != NULL) {
        if (mpOwner->isBusy() == 0) {
            hbmAnimSetPaneVisible(((HbmAnimOwnerBaseView*)&mpOwner->base_4C)->getPaneHolder()->pane, TRUE);
        }
        getPaneAnim("N_SGNkeytop_all")->setAnim(7);
        mbShown = 0;
        mpSound->setEnable(0);
    }
}

/* 0x80557D1C (0x60): Arms the keys, starts closing and finishes the close. */
void HbmAnimPanel::refresh() {
    armKeys();
    close();
    onClosed();
}

/* 0x80557D7C (0x88): Closes the owner key, clears the open flag and hides the key-top pane. */
void HbmAnimPanel::onClosed() {
    if (mpOwner != NULL) {
        mpOwner->close();
    }
    mbOpen = 0;
    setPaneVisible("N_SGNkeytop_all", 0);
    mpSound->setEnable(1);
}

/* 0x80557E04 (0x88): Marks the panel shown, hides the owner key's pane while the key is idle and enables the panel's
 * sound. */
void HbmAnimPanel::show() {
    mbShown = 1;
    if (mpOwner->isBusy() == 0) {
        hbmAnimSetPaneVisible(((HbmAnimOwnerBaseView*)&mpOwner->base_4C)->getPaneHolder()->pane, FALSE);
    }
    mpSound->setEnable(2);
}

/* 0x80557E8C (0x140): Follows the key-top and frame animations: relabels the keys when a scroll ends, finishes the
 * close or the open, and tracks whether a scroll is running. */
void HbmAnimPanel::onAnimEvent(u32 code, HbmKey* key, HbmKeyAnim* anim) {
    s32 event = (s32)code;
    if (hbmNameMatches("N_SGNkeyall", key->mpPane->mName)) {
        if ((anim->kind == 7 || anim->kind == 5) && event == 1) {
            updateLabels(mPage, mPage);
        }
    }
    if (hbmNameMatches("N_SGNkeytop_all", key->mpPane->mName)) {
        if (anim->kind == 8 && event == 1) {
            onClosed();
        }
        if (anim->kind == 1 && event == 1) {
            show();
        }
    }
    if (anim->kind == 7 || anim->kind == 5) {
        if (event == 0) {
            mbAnimating = 1;
        }
        if (event == 1) {
            mbAnimating = 0;
        }
    }
}

/* 0x80557FCC (0x90): Arms every sign key (kind 2). */
void HbmAnimPanel::armKeys() {
    HbmAnimPart* pPart;

    for (pPart = (HbmAnimPart*)nw4r::ut::List_GetNext(&mComponents, NULL); pPart != NULL;
         pPart = (HbmAnimPart*)nw4r::ut::List_GetNext(&mComponents, pPart)) {
        if (pPart->getType() == 2) {
            pPart->setAnim(2);
        }
    }
}

/* Returns the part's kind. */
s32 HbmAnimPart::getType() {
    return mType;
}

/* 0x80558064 (0x80): Puts every key back to its rest state. */
void HbmAnimPanel::resetKeys() {
    for (u16 i = 0; i < 25; i++) {
        getPaneAnim(hbmAnimPanelKeyDefs[i].name)->reset();
    }
}

/* Clears the part's state. */
void HbmAnimPart::reset() {
    mState = 0;
}

/* 0x805580F0 (0xC4): Puts the sign keys and buttons into their showing state and fades every key in. */
void HbmAnimPanel::fadeKeys() {
    for (u16 i = 2; i < 25; i++) {
        ((HbmAnimPart*)getPaneAnim(hbmAnimPanelKeyDefs[i].name))->show();
    }
    for (HbmKey* pKey = (HbmKey*)nw4r::ut::List_GetNext(&mComponents, NULL); pKey != NULL;
         pKey = (HbmKey*)nw4r::ut::List_GetNext(&mComponents, pKey)) {
        pKey->setAnim(4);
    }
}

/* Puts the part into its showing state. */
void HbmAnimPart::show() {
    mState = 6;
}

/* 0x805581C0 (0x1F4): Labels the sign keys with the texts of `page` (long phrases cut to nine characters and an
 * ellipsis) and the buttons in the current language. */
void HbmAnimPanel::updateLabels(u16 page, u16 newPage) {
    for (u16 i = 0; i < 20; i++) {
        const char* buttonName = mpPages->keys[i].button_name;
        char boxName[17];
        memset(boxName, 0, 17);
        strncpy(boxName, buttonName, strlen(buttonName));
        boxName[0] = 'T';
        const wchar_t* text = (&mpPages->keys[i].phrase)[page];
        nw4r::lyt::TextBox* box = findTextBox(boxName);
        wchar_t label[12];
        wmemset(label, 0, 11);
        if (wcslen((const u16*)text) > 10 && page == 0) {
            wcsncpy((u16*)label, (const u16*)text, 9);
            wcsncat((u16*)&label[10], (const u16*)L"\x2026", 1);
            box->SetString(label, 0);
        } else {
            box->SetString(text, 0);
        }
    }
    setPaneText("T_SGNkey_prev", hkbLayoutLabels[4][getLayoutType()]);
    setPaneText("T_SGNkey_next", hkbLayoutLabels[3][getLayoutType()]);
    setPaneText("T_SGNkey_close", hkbLayoutLabels[6][getLayoutType()]);
}

/* 0x805583B4 (0x80): Shows "page/count" in the page number text box. */
void HbmAnimPanel::setPageNumber(s32 page) {
    wchar_t text[17];
    swprintf((u16*)text, 17, (const u16*)L"%d/%d", page + 1, mpPages->count);
    findTextBox("T_SGN_pageNumber")->SetString(text, 0);
}

/* 0x80558434 (0xBC): Steps the page back (stopping at the first page) and scrolls the keys. */
void HbmAnimPanel::prevPage() {
    u8 previous = mPage;
    u8 current;

    if (previous == 0) {
        return;
    }
    stepPageDown();
    current = mPage;
    updateLabels(previous, current);
    setPageNumber(current);
    scrollKeysBack();
    mpSound->play(15);
}

/* 0x805584F0 (0xB8): Steps the page forward (wrapping to the first page) and scrolls the keys. */
void HbmAnimPanel::nextPage() {
    u8 previous = mPage;
    u8 current;

    stepPageUp();
    current = mPage;
    updateLabels(previous, current);
    setPageNumber(current);
    scrollKeysForward();
    mpSound->play(15);
}

/* 0x805585A8 (0x48): Scrolls the keys' frame back. */
void HbmAnimPanel::scrollKeysBack() {
    getPaneAnim("N_SGNkeyall")->setAnim(8);
}

/* 0x805585F0 (0x48): Scrolls the keys' frame forward. */
void HbmAnimPanel::scrollKeysForward() {
    getPaneAnim("N_SGNkeyall")->setAnim(9);
}

/* 0x80558638 (0x1C): Updates the components while the panel is shown. */
u32 HbmAnimPanel::updateComponents() {
    if (mbShown != 0) {
        return HbmPaneManager::updateComponents();
    }
    return 0;
}

/* 0x80558654 (0x1C): Hands a key release to the pane manager while the panel is shown. */
s32 HbmAnimPanel::onKeyRelease(HkbManager* manager) {
    if (mbShown != 0) {
        return HbmPaneManager::onKeyRelease(manager);
    }
    return 0;
}

/* 0x80558670 (0x64): Replaces the twelve editable phrases. */
void HbmAnimPanel::setPhraseTexts(const wchar_t** texts) {
    for (int i = 0; i < 12; i++) {
        wcscpy((u16*)hkb_sign_phrase_text[i], (const u16*)texts[i]);
    }
}

/* 0x805586D4 (0x64): Replaces the twelve editable gestures. */
void HbmAnimPanel::setGestureTexts(const wchar_t** texts) {
    for (int i = 0; i < 12; i++) {
        wcscpy((u16*)hkb_sign_gesture_text[i], (const u16*)texts[i]);
    }
}

/* 0x80558738 (0xBC): Steps the page back (stopping at the first page) and scrolls the keys. */
void HbmAnimPanel::prevPageWrap() {
    u8 previous = mPage;
    u8 current;

    if (previous == 0) {
        return;
    }
    stepPageDown();
    current = mPage;
    updateLabels(previous, current);
    setPageNumber(current);
    scrollKeysBack();
    mpSound->play(15);
}

/* 0x805587F4 (0x1AC): Reports presses to the panel and, for a "B..." button, drives its "P..." part (press, release,
 * focus). */
void HbmAnimPanelPaneHandler::onPaneEvent(HbmComponent* pComponent, u32 event, HbmPaneTrigger* pTrigger) {
    const char* name = pComponent->mpPane->mName;
    char partName[17];
    HbmKey* pPart;

    if (mpPanel->isAnimating() != 0 && event == 4) {
        return;
    }
    if (event == 4 && (pTrigger->trigger & 0x10)) {
        mpPanel->onPaneEvent(4, name);
    }
    if (name[0] != 'B') {
        return;
    }
    memset(partName, 0, sizeof(partName));
    strncpy(partName, name, strlen(name));
    partName[0] = 'P';
    pPart = mpPanel->getPaneAnim(partName);
    if (pPart == NULL) {
        return;
    }
    switch ((s32)event) {
    case 4:
        if (pTrigger->trigger & 0x10) {
            pPart->setAnim(0);
        }
        break;
    case 1:
        pPart->setAnim(2);
        break;
    case 0:
        mpSound->play(4);
        mpPanel->setCursorTarget((u32)pPart->mpPane);
        pPart->setAnim(1);
        break;
    }
}

/* 0x805589A8 (0x9C): Event 6 shows the pane and selects state 1, event 7 selects state 8; event 4 leaves state 1 for
 * 0 and hides the pane in state 8. */
void HbmAnimPartB::setAnim(s32 event) {
    if (event == 6) {
        hbmAnimSetPaneVisible((HbmAnimPane*)mpPane, TRUE);
        setState(1);
        return;
    }
    if (event == 7) {
        setState(8);
        return;
    }
    switch (mState) {
    case 1:
        if (event == 4) {
            setState(0);
        }
        break;
    case 8:
        if (event == 4) {
            hbmAnimSetPaneVisible((HbmAnimPane*)mpPane, FALSE);
        }
        break;
    }
}

/* Stores the state, then hands it to the band key. */
void HbmAnimPart::setState(u32 state) {
    mState = state;
    HbmKey::setState(state);
}

/* 0x80558A4C (0xBC): Steps the idle (0) and scrolling (5, 7) states on an input event. */
void HbmAnimPartC::setAnim(s32 event) {
    switch (mState) {
    case 0:
        if (event == 9) {
            setState(5);
        }
        if (event == 8) {
            setState(7);
        }
        break;
    case 5:
    case 7:
        if (event == 4) {
            setState(0);
        }
        break;
    }
}

/* 0x80558B08 (0x184): Steps the seven-state selector on an input event (0 also forces state 4). */
void HbmAnimPartA::setAnim(s32 event) {
    if (event == 0) {
        setState(4);
    }
    switch ((u32)mState) {
    case 0:
        if (event == 1) {
            setState(2);
        }
        break;
    case 2:
        if (event == 4) {
            setState(3);
        }
        if (event == 2) {
            setState(6);
        }
        break;
    case 3:
        if (event == 2) {
            setState(6);
        }
        break;
    case 6:
        if (event == 4) {
            setState(0);
        }
        if (event == 1) {
            setState(2);
        }
        break;
    case 4:
        if (event == 4) {
            setState(3);
        }
        if (event == 2) {
            setState(6);
        }
        break;
    }
}

/* 0x80558C8C (0x8): Updates the pane manager's components. */
void HbmAnimPanel::calcComponents() {
    HbmPaneManager::calcComponents();
}

/* 0x80558C94 (0x4): Does nothing. */
void HbmAnimPanel::v_0x034() {}

/* 0x80558CBC (0x14): Stores the language, then re-runs init. */
void HbmAnimPanel::setLayoutType(s32 type) {
    mLayoutType = type;
    init();
}

/* 0x80558CD0 (0x8): Reports the panel as busy in its own way (2). */
s32 HbmAnimPanel::isBusy() {
    return 2;
}

/* 0x80558CD8 (0x8): Whether the panel is open. */
u8 HbmAnimPanel::isOpen() {
    return mbOpen;
}

/* Destroys the part (the inline base destructors run here). */
HbmAnimPartA::~HbmAnimPartA() {}

/* Clears the part's state. */
void HbmAnimPartA::reset() {
    mState = 0;
}

/* Clears the part's state. */
void HbmAnimPartC::reset() {
    mState = 0;
}

/* Destroys the part (the inline base destructors run here). */
HbmAnimPartB::~HbmAnimPartB() {}

/* Clears the part's state. */
void HbmAnimPartB::reset() {
    mState = 0;
}

/* Destroys the part (the inline base destructors run here). */
HbmAnimPartC::~HbmAnimPartC() {}
