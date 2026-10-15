/*
 * homebutton/hbm_kb_child.cpp - software-keyboard code of the tiHKBManager band: the prediction window's language
 * selector (HbmKbChild): one dictionary key per language of the three regions, the key-top pane that fades them in and
 * out, its pane handler and its key parts.
 *
 * RANGE.     .text 0x80555B0C..0x80556FC4 (36 functions), .ctors 0x8056F408..0x8056F40C, .rodata 0x8057B848..0x8057BAB0,
 *            .data 0x80650500..0x80650B40, .sdata 0x80794620..0x80794628.
 * FLAGS.     GC/3.0a5.2 + cflags_hkb (configure.py); the retail object has no extab/extabindex, so `#pragma exceptions off`.
 * NAMES.     GUESS file name: the band's `hbm_` prefix + the child widget; no `__FILE__` string, no runtime-dump name.
 *            The unit loads `fs_prdicSelWidw_a_PRDC_*.brlan` and binds the N_PRDCkey_JP/US/EU panes, so the widget is
 *            the prediction window's dictionary (language) selector.
 * NAMES.     GUESS classes HbmKbChild (three bases: the class table 0x80650A48 holds the HbmPaneManager table at +0x24
 *            and the HbmKeyListener table at +0x98, as the destructor's stores show), HbmKbChildBase (its primary base,
 *            whose table is another unit's), HbmKeyListener (the band key's +0x18 listener: HbmKey::update calls its
 *            slot 0x08 with (1, key, anim)), HbmKbChildPaneHandler (table 0x80650A28), HbmKbChildPart and its kinds
 *            HbmKbChildPartA/B/C; members named from the panes they bind and the codes they set.
 * NAMES.     GUESS data names hbmKbChildAnimDefs, hbmKbChildKeyDefs, hbmKbChildRegionKeys, hbmKbChildKeyCodes,
 *            hbmKbChildKeyGroup.
 * SHAPES.    The key table's group words and the code table's names are initialised from variables, so the unit's
 *            static initialiser (the .ctors entry) patches them as retail's 0x80556F08 does.
 * SHAPES.    The pane-name copy and the code lookup of the pane handler are inline helpers: retail expands them
 *            (memset/strlen/strncpy, the nine-entry search).
 * SHAPES.    HbmKbChildPartA::setAnim switches on `(u32)mState` (retail's `cmplwi` bound check before the jump table).
 * RESIDUALS. HbmKbChildPartC's listener is stored after the HbmKey constructor (retail stores it inside it): HbmKey has
 *            no (pane, listener) constructor and its +0x18 is typed HbmKeyAnim* in homebutton/hbm_value.h (request
 *            hb-kb-parts#2); create's registers shift by one with it.
 * RESIDUALS. The static initialiser restores r30/r31 after its last store where retail restores them between the stores.
 * RESIDUALS. resetKeys and fadeOtherKeys: the table base and the index take each other's saved registers; the
 *            pane handler's onPaneEvent keeps the code index in a temporary where retail keeps the scaled offset.
 * RESIDUALS. setLanguage: retail schedules the language register copy before the pool base; ours after.
 */

#include "types.h"
#include "homebutton/hbm_kb_child.h"
#include "homebutton/hkb_layout.h"
#include "homebutton/hkb_textbox.h"
#include "homebutton/hbm_kb_event.h"
#include "homebutton/hbm_hermite.h"
#include "MEM/MEMAllocFromAllocator.h"
#include "MSL_C/string.h"
#include "Runtime.PPCEABI.H/memset.h"
#include "MSL/strlen.h"

/* The selector's slot 0x20 as its callers use it: the key code alone (the band's root names the slot onPaneEvent).
 * size: 0x4 (approximation: only the vptr is touched) */
class HbmKbKeyCodeView {
public:
    /* slot +0x008 */ virtual void v_0x008();
    /* slot +0x00C */ virtual void v_0x00C();
    /* slot +0x010 */ virtual void v_0x010();
    /* slot +0x014 */ virtual void v_0x014();
    /* slot +0x018 */ virtual void v_0x018();
    /* slot +0x01C */ virtual void v_0x01C();
    /* slot +0x020 */ virtual void setKeyCode(s32 code);
}; /* size: 0x4 */


/* The retail object has no extab/extabindex: exceptions are off for this band. */
#pragma exceptions off

/* A language code and the dictionary key that selects it.  size: 0x8 */
struct HbmKbChildKeyCode {
    /* +0x00 */ s32 code;
    /* +0x04 */ const char* name;
}; /* size: 0x8 */

/* A selector key: a band key with its state word and its kind; its class table (0x80650A00) and the out-of-line copy
 * of its inline destructor (0x80555F5C) are this unit's.  size: 0x34 */
class HbmKbChildPart : public HbmKey {
public:
    HbmKbChildPart(nw4r::lyt::Pane* pane) : HbmKey(pane) {}
    HbmKbChildPart(nw4r::lyt::Pane* pane, HbmKeyListener* listener) : HbmKey(pane, listener) {}

    virtual void reset();
    virtual void setState(u32 state);
    virtual ~HbmKbChildPart() {}
    virtual u32 getType();

    /* +0x30 */ u32 mType;
}; /* size: 0x34 */

/* The key-top part (kind 2; class table 0x806509D8): reports its animation ends to the selector.  size: 0x34 */
class HbmKbChildPartC : public HbmKbChildPart {
public:
    HbmKbChildPartC(nw4r::lyt::Pane* pane, HbmKeyListener* listener) : HbmKbChildPart(pane, listener) {
        mType = 2;
    }

    virtual void setAnim(s32 anim);
    virtual ~HbmKbChildPartC();
}; /* size: 0x34 */

/* A plain key (kind 0; class table 0x806509B0): steps a five-state selector.  size: 0x34 */
class HbmKbChildPartB : public HbmKbChildPart {
public:
    HbmKbChildPartB(nw4r::lyt::Pane* pane) : HbmKbChildPart(pane) { mType = 0; }

    virtual void setAnim(s32 anim);
    virtual ~HbmKbChildPartB();
}; /* size: 0x34 */

/* A dictionary key (kind 1; class table 0x80650988): steps a nine-state selector and fades the other keys when it is
 * chosen.  size: 0x38 */
class HbmKbChildPartA : public HbmKbChildPart {
public:
    HbmKbChildPartA(nw4r::lyt::Pane* pane, HbmKbChild* owner) : HbmKbChildPart(pane), mpOwner(owner) { mType = 1; }

    virtual void setAnim(s32 anim);
    virtual ~HbmKbChildPartA();

    /* +0x34 */ HbmKbChild* mpOwner;
}; /* size: 0x38 */

/* The key animations (.rodata 0x8057B848). */
const HkbAnimDef hbmKbChildAnimDefs[9] = {
    {0, "fs_prdicSelWidw_a_PRDC_normal.brlan"},
    {1, "fs_prdicSelWidw_a_PRDC_Focus-IN.brlan"},
    {2, "fs_prdicSelWidw_a_PRDC_Focus-OUT.brlan"},
    {4, "fs_prdicSelWidw_a_PRDC_Pushed.brlan"},
    {5, "fs_prdicSelWidw_a_PRDC_FADE-IN.brlan"},
    {6, "fs_prdicSelWidw_a_PRDC_FADE-OUT.brlan"},
    {3, "fs_prdicSelWidw_a_PRDC_Roll_over.brlan"},
    {8, "fs_prdicSelWidw_a_PRDC_toggle-OFF.brlan"},
    {7, "fs_prdicSelWidw_a_PRDC_toggle-ON.brlan"},
};

/* The key whose animations the dictionary keys share. */
const char* hbmKbChildKeyGroup = "P_PRDC_US_US";

#define HBM_KB_CHILD_KEY_ANIMS                                                                                         \
    {&hbmKbChildAnimDefs[0], &hbmKbChildAnimDefs[1], &hbmKbChildAnimDefs[2], &hbmKbChildAnimDefs[3],                   \
     &hbmKbChildAnimDefs[6], &hbmKbChildAnimDefs[7], &hbmKbChildAnimDefs[8]}

/* The key-top pane and the dictionary keys (.data 0x80650510). */
HbmKbChildKeyDef hbmKbChildKeyDefs[10] = {
    {2, "N_PRDCkeytop_all", 3, NULL, {&hbmKbChildAnimDefs[0], &hbmKbChildAnimDefs[4], &hbmKbChildAnimDefs[5]}},
    {1, "P_PRDC_EU_UK", 7, hbmKbChildKeyGroup, HBM_KB_CHILD_KEY_ANIMS},
    {1, "P_PRDC_EU_Ger", 7, hbmKbChildKeyGroup, HBM_KB_CHILD_KEY_ANIMS},
    {1, "P_PRDC_EU_Fre", 7, hbmKbChildKeyGroup, HBM_KB_CHILD_KEY_ANIMS},
    {1, "P_PRDC_EU_Spa", 7, hbmKbChildKeyGroup, HBM_KB_CHILD_KEY_ANIMS},
    {1, "P_PRDC_EU_Ita", 7, hbmKbChildKeyGroup, HBM_KB_CHILD_KEY_ANIMS},
    {1, "P_PRDC_EU_Dut", 7, hbmKbChildKeyGroup, HBM_KB_CHILD_KEY_ANIMS},
    {1, "P_PRDC_US_US", 7, NULL, HBM_KB_CHILD_KEY_ANIMS},
    {1, "P_PRDC_US_Fre", 7, hbmKbChildKeyGroup, HBM_KB_CHILD_KEY_ANIMS},
    {1, "P_PRDC_US_Spa", 7, hbmKbChildKeyGroup, HBM_KB_CHILD_KEY_ANIMS},
};

/* The dictionary keys of each region (0 JP, 1 US, 2 EU), each list ending in NULL (.data 0x80650810). */
const char* hbmKbChildRegionKeys[3][8] = {
    {NULL},
    {"P_PRDC_US_US", "P_PRDC_US_Fre", "P_PRDC_US_Spa", NULL},
    {"P_PRDC_EU_UK", "P_PRDC_EU_Ger", "P_PRDC_EU_Fre", "P_PRDC_EU_Spa", "P_PRDC_EU_Ita", "P_PRDC_EU_Dut", NULL},
};

/* The language code each dictionary key selects (.data 0x80650870). */
HbmKbChildKeyCode hbmKbChildKeyCodes[9] = {
    {2, hbmKbChildRegionKeys[1][0]}, {3, hbmKbChildRegionKeys[1][1]}, {4, hbmKbChildRegionKeys[1][2]},
    {5, hbmKbChildRegionKeys[2][0]}, {6, hbmKbChildRegionKeys[2][1]}, {7, hbmKbChildRegionKeys[2][2]},
    {8, hbmKbChildRegionKeys[2][3]}, {9, hbmKbChildRegionKeys[2][4]}, {10, hbmKbChildRegionKeys[2][5]},
};

/* 0x80555B0C (0xE8): Destroys the pane handler and the keys. */
HbmKbChild::~HbmKbChild() {
    mpPaneHandler->~HbmEventHandler();
    MEMFreeToAllocator(mpAllocator, mpPaneHandler);
    for (HbmKey* pKey = (HbmKey*)nw4r::ut::List_GetNext(&mComponents, NULL); pKey != NULL;
         pKey = (HbmKey*)nw4r::ut::List_GetNext(&mComponents, NULL)) {
        nw4r::ut::List_Remove(&mComponents, pKey);
        pKey->destroy(mpAllocator);
    }
}

/* 0x80555BF4 (0x40): Destroys the pane handler. */
HbmKbChildPaneHandler::~HbmKbChildPaneHandler() {}

/* 0x80555C34 (0x328): Creates the pane handler and the pane manager, hides the root, and creates one part per key with
 * its animations. */
void HbmKbChild::create(MEMAllocator* allocator) {
    const char* group;

    HbmKbChildBase::create(allocator);
    HbmKbChildPaneHandler* pHandler =
        new (MEMAllocFromAllocator(allocator, sizeof(HbmKbChildPaneHandler))) HbmKbChildPaneHandler(this);
    mpPaneHandler = pHandler;
    HbmPaneManager::create(allocator, pHandler);
    mpRoot->setVisible(0);
    mpRoot->setEnable(1);
    for (u16 i = 0; i < 10; i++) {
        const HbmKbChildKeyDef* def = &hbmKbChildKeyDefs[i];
        HbmKbChildPart* pPart = NULL;
        switch (def->type) {
        case 2:
            pPart = new (MEMAllocFromAllocator(allocator, sizeof(HbmKbChildPartC)))
                HbmKbChildPartC(findTextBox(def->name), this);
            break;
        case 0:
            pPart = new (MEMAllocFromAllocator(allocator, sizeof(HbmKbChildPartB)))
                HbmKbChildPartB(findTextBox(def->name));
            break;
        case 1:
            pPart = new (MEMAllocFromAllocator(allocator, sizeof(HbmKbChildPartA)))
                HbmKbChildPartA(findTextBox(def->name), this);
            break;
        }
        nw4r::ut::List_Append(&mComponents, pPart);
        group = def->group;
        u16 j = 0;
        u32 numAnims = def->numAnims;
        for (; j < numAnims; j++) {
            const void* res = mpResAccessor->GetResource(0, def->anims[j]->name, NULL);
            nw4r::lyt::AnimTransform* anim = getLayout()->CreateAnimTransform(res, mpResAccessor);
            if (group == NULL) {
                pPart->addAnim(allocator, def->anims[j]->kind, anim, 0, 1);
            } else {
                pPart->addGroupAnim(allocator, def->anims[j]->kind, anim, group, 0, 1);
            }
        }
    }
    init();
}

/* 0x80555FB4 (0xAC): Puts the key-top pane back, hides it and lays the panes out. */
void HbmKbChild::init() {
    getPaneAnim("N_PRDCkeytop_all")->setState(0);
    setPaneVisible("N_PRDCkeytop_all", 0);
    mpLyt->Animate(0);
    mpLyt->CalculateMtx(mDrawInfo);
}

/* 0x80556060 (0xEC): Opens the selector on a language code: shows the region's keys and fades the key-top pane in. */
void HbmKbChild::open(s32 code, HbmKbChildPopupView* popup) {
    mpPopup = popup;
    setRegion(mRegion);
    ((HbmKbKeyCodeView*)this)->setKeyCode(code);
    mbOpen = 1;
    mbActive = 0;
    setPaneVisible("N_PRDCkeytop_all", 1);
    resetKeys();
    getPaneAnim("N_PRDCkeytop_all")->setState(5);
    mpSound->play(19);
}

/* 0x8055614C (0x8): Stores the language code. */
void HbmKbChildBase::onPaneEvent(u32 code, const char* name) {
    (void)name;
    mKeyCode = code;
}

/* 0x80556154 (0x1C): Lets the keys take input and refreshes the root. */
void HbmKbChild::activate() {
    mbActive = 1;
    mpRoot->v_0x010();
}

/* 0x80556170 (0x60): Fades the key-top pane out and stops taking input. */
void HbmKbChild::deactivate() {
    getPaneAnim("N_PRDCkeytop_all")->setState(6);
    mbActive = 0;
}

/* 0x805561D0 (0xA8): Reports the language (events 31 and 29), closes the window and hides the key-top pane. */
void HbmKbChild::close() {
    s32 code;

    sendEvent(31, &code);
    code = mKeyCode;
    sendEvent(29, &code);
    if (mpPopup != NULL) {
        mpPopup->close();
    }
    mbOpen = 0;
    setPaneVisible("N_PRDCkeytop_all", 0);
}

/* 0x80556278 (0x158): Stores the region (0 JP, 1 US, 2 EU) and shows that region's keys only. */
void HbmKbChild::setRegion(s32 region) {
    mRegion = region;
    switch (region) {
    case 0:
        setPaneVisible("N_PRDCkey_JP", 1);
        setPaneVisible("N_PRDCkey_US", 0);
        setPaneVisible("N_PRDCkey_EU", 0);
        break;
    case 1:
        setPaneVisible("N_PRDCkey_JP", 0);
        setPaneVisible("N_PRDCkey_US", 1);
        setPaneVisible("N_PRDCkey_EU", 0);
        break;
    case 2:
        setPaneVisible("N_PRDCkey_JP", 0);
        setPaneVisible("N_PRDCkey_US", 0);
        setPaneVisible("N_PRDCkey_EU", 1);
        break;
    }
}

/* 0x805563D0 (0xE4): When the key-top pane has faded in, plays the chosen language's key and takes input; when it has
 * faded out, closes the selector. */
void HbmKbChild::onAnimEvent(u32 code, HbmKey* key, HbmKeyAnim* anim) {
    s32 event = (s32)code;
    if (hbmNameMatches("N_PRDCkeytop_all", key->mpPane->mName)) {
        switch (anim->kind) {
        case 5:
            if (event == 1) {
                findKey(mRegion, mKeyCode)->setState(4);
                activate();
            }
            break;
        case 6:
            if (event == 1) {
                close();
            }
            break;
        }
    }
}

/* 0x805564B4 (0xAC): Fades out every key of the region except `name`. */
void HbmKbChild::fadeOtherKeys(const char* name) {
    const char* key;
    u32 i = 0;

    for (key = hbmKbChildRegionKeys[mRegion][i]; key != NULL; key = hbmKbChildRegionKeys[mRegion][++i]) {
        if (!hbmNameMatches(key, name)) {
            getPaneAnim(key)->setAnim(5);
        }
    }
}

/* 0x80556560 (0x98): Puts every key of the region back to its rest state. */
void HbmKbChild::resetKeys() {
    u32 i = 0;
    const char* key = hbmKbChildRegionKeys[mRegion][i];

    while (key != NULL) {
        getPaneAnim(key)->setState(0);
        key = hbmKbChildRegionKeys[mRegion][++i];
    }
}

/* 0x805565F8 (0x54): Returns the key that selects language `code`, or NULL. */
HbmKey* HbmKbChild::findKey(s32 region, s32 code) {
    for (u16 i = 0; i < 9; i++) {
        const HbmKbChildKeyCode* entry = &hbmKbChildKeyCodes[i];
        if (code == entry->code) {
            const char* name = entry->name;
            if (code != 0) {
                return getPaneAnim(name);
            }
        }
    }
    return NULL;
}

/* 0x8055664C (0x1F4): Titles the window in `language` and selects that language's dictionary code. */
void HbmKbChild::setLanguage(u32 language) {
    setPaneText("T_PRDC_title", hkbLayoutLabels[2][language]);
    switch (language) {
    case 0:
        ((HbmKbKeyCodeView*)this)->setKeyCode(1);
        break;
    case 1:
        ((HbmKbKeyCodeView*)this)->setKeyCode(2);
        break;
    case 2:
        ((HbmKbKeyCodeView*)this)->setKeyCode(5);
        break;
    case 4:
        ((HbmKbKeyCodeView*)this)->setKeyCode(6);
        break;
    case 5:
        ((HbmKbKeyCodeView*)this)->setKeyCode(9);
        break;
    case 7:
        ((HbmKbKeyCodeView*)this)->setKeyCode(10);
        break;
    case 6:
        if (mRegion == 2) {
            ((HbmKbKeyCodeView*)this)->setKeyCode(8);
        } else {
            ((HbmKbKeyCodeView*)this)->setKeyCode(4);
            setPaneText("T_PRDC_title", L"Diccionario");
        }
        break;
    case 3:
        if (mRegion == 2) {
            ((HbmKbKeyCodeView*)this)->setKeyCode(7);
        } else {
            ((HbmKbKeyCodeView*)this)->setKeyCode(3);
            setPaneText("T_PRDC_title", L"Dictionnaire");
        }
        break;
    }
}

/* Copies a pane name and replaces its first character with `c` (the button pane's B -> the part's P). */
static inline void hbmKbChildMakePartName(char* dst, const char* src, char c) {
    memset(dst, 0, 17);
    strncpy(dst, src, strlen(src));
    dst[0] = c;
}

/* Returns the language code of the dictionary key `name`, or 11. */
static inline s32 hbmKbChildFindCode(const char* name) {
    for (u16 i = 0; i < 9; i++) {
        if (hbmNameMatches(hbmKbChildKeyCodes[i].name, name)) {
            return hbmKbChildKeyCodes[i].code;
        }
    }
    return 11;
}

/* 0x80556840 (0x21C): Animates the dictionary key under a pane event and selects its language when it is pressed. */
void HbmKbChildPaneHandler::onPaneEvent(HbmComponent* pComponent, u32 event, HbmPaneTrigger* pTrigger) {
    HbmKey* pPart;

    if (mpChild->isActive() == 0) {
        return;
    }
    const char* name = pComponent->mpPane->mName;
    if (name[0] != 'B') {
        return;
    }
    char partName[17];
    hbmKbChildMakePartName(partName, name, 'P');
    pPart = mpChild->getPaneAnim(partName);
    if (pPart != NULL) {
        switch (event) {
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
            pPart->setAnim(1);
            break;
        }
    }
    if (event == 4 && (pTrigger->trigger & 0x10)) {
        char keyName[17];
        hbmKbChildMakePartName(keyName, name, 'P');
        s32 code = hbmKbChildFindCode(keyName);
        if (code != 11) {
            ((HbmKbKeyCodeView*)mpChild)->setKeyCode(code);
            mpChild->deactivate();
            mpSound->play(3);
        }
    }
}

/* 0x80556A5C (0x8): Whether the keys take input. */
u32 HbmKbChild::isActive() {
    return mbActive;
}

/* 0x80556A64 (0x2C): Leaves state 5 for state 0 on event 4. */
void HbmKbChildPartC::setAnim(s32 event) {
    if (mState == 5 && event == 4) {
        setState(0);
    }
}

/* Stores the state, then hands it to the band key. */
void HbmKbChildPart::setState(u32 state) {
    mState = state;
    HbmKey::setState(state);
}

/* 0x80556A98 (0x170): Steps the five-state selector on an input event (0 also forces state 4). */
void HbmKbChildPartB::setAnim(s32 event) {
    if (event == 0) {
        setState(4);
    }
    switch (mState) {
    case 0:
        if (event == 1) {
            setState(1);
        }
        break;
    case 1:
        if (event == 4) {
            setState(3);
        }
        if (event == 2) {
            setState(2);
        }
        break;
    case 3:
        if (event == 2) {
            setState(2);
        }
        break;
    case 2:
        if (event == 4) {
            setState(0);
        }
        break;
    case 4:
        if (event == 4) {
            setState(3);
        }
        if (event == 2) {
            setState(2);
        }
        break;
    }
}

/* 0x80556C08 (0x1B4): Steps the nine-state selector on an input event; event 5 outside state 0 forces state 8, and a
 * press in state 4 fades the other keys. */
void HbmKbChildPartA::setAnim(s32 event) {
    if (event == 0) {
        setState(4);
    }
    if (event == 5 && mState != 0) {
        setState(8);
    }
    switch ((u32)mState) {
    case 0:
        if (event == 1) {
            setState(1);
        }
        break;
    case 1:
        if (event == 4) {
            setState(3);
        }
        if (event == 2) {
            setState(2);
        }
        break;
    case 3:
        if (event == 2) {
            setState(2);
        }
        break;
    case 2:
    case 8:
        if (event == 4) {
            setState(0);
        }
        if (event == 1) {
            setState(1);
        }
        break;
    case 4:
        if (event == 3) {
            mpOwner->fadeOtherKeys(mpPane->mName);
        }
        if (event == 4) {
            setState(7);
        }
        break;
    }
}

/* 0x80556DC0 (0x8): Updates the pane manager's components. */
void HbmKbChild::calcComponents() {
    HbmPaneManager::calcComponents();
}

/* 0x80556DC8 (0x10): Selects a region. */
void HbmKbChild::selectRegion(s32 region) {
    setRegion(region);
}

/* 0x80556DD8 (0x8): Whether the selector is open. */
u32 HbmKbChild::isOpen() {
    return mbOpen;
}

/* Returns the part's kind. */
u32 HbmKbChildPart::getType() {
    return mType;
}

/* Clears the part's state. */
void HbmKbChildPart::reset() {
    mState = 0;
}

/* Destroys the part (the inline base destructors run here). */
HbmKbChildPartA::~HbmKbChildPartA() {}

/* Destroys the part (the inline base destructors run here). */
HbmKbChildPartB::~HbmKbChildPartB() {}

/* Destroys the part (the inline base destructors run here). */
HbmKbChildPartC::~HbmKbChildPartC() {}
