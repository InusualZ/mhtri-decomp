/*
 * homebutton/hbm_kb_list.cpp - software-keyboard code of the tiHKBManager band: the keyboard's toolbar (HbmKbList), its
 * confirm and cancel buttons and its QWERTY/cellphone key-layout switch, its pane handler and its button parts.
 *
 * RANGE.     .text 0x805546B4..0x80555B0C (37 functions), .ctors 0x8056F404..0x8056F408, .rodata 0x8057B668..0x8057B848,
 *            .data 0x80650160..0x80650500, .sdata 0x80794608..0x80794620.
 * FLAGS.     GC/3.0a5.2 + cflags_hkb (configure.py); the retail object has no extab/extabindex, so `#pragma exceptions off`.
 * NAMES.     GUESS file name: the band's `hbm_` prefix + the list widget; no `__FILE__` string, no runtime-dump name.
 *            The animation resources the unit loads are `fs_VK_toolbar_a_*.brlan`, so the widget is the toolbar.
 * NAMES.     GUESS classes HbmKbList (the toolbar: two bases, the class table 0x806503F8 holds the HbmPaneManager table
 *            at +0x20 as its constructor-less destructor stores show), HbmKbListBase (its primary base, table 0x806504C8),
 *            HbmKbListPaneHandler (table 0x806503D8), HbmKbListPart and its kinds HbmKbListPartA/B; members named from
 *            the panes they bind (P_BT_confirm/cancel, T_BT_*, P_kyChng_*) and the states they set.
 * NAMES.     GUESS data names hbmKbListAnimDefs, hbmKbListKeyDefs, hbmKbListConfirmGroup, hbmKbListCellphoneGroup.
 * SHAPES.    The key table's group words are pointer variables, so the table is patched by the unit's static
 *            initialiser (the .ctors entry) exactly as retail's 0x80555AD8 does.
 * SHAPES.    The parts set their kind after their class pointer (retail's store order), so the kind is assigned in the
 *            kinds' constructor bodies.
 * SHAPES.    HbmKbListPartB::setAnim switches on `(s32)mState` (retail's signed compare tree); HbmKbListPartA::setAnim's
 *            guard compares `(s32)mState` too.
 * SHAPES.    HbmKbList's constructor is defined (first, as retail's emission order implies) though nothing calls it: it
 *            makes MWCC emit HbmKbListBase's table and inline members, which retail keeps; the linker strips its body.
 * SHAPES.    The layout-mode getter and setters take and return `u32` (retail compares and forwards the words with no
 *            `clrlwi`); the classes are declared in the order that reproduces retail's .data (tables in reverse order).
 * RESIDUALS. HbmKbListBase::getLayoutMode (0x80536DB8) and HbmKbList::getKeyChangeMode (0x80541B28) are inline members
 *            whose kept copies sit in homebutton/hkb_keyboard.cpp's and homebutton/hkb_cellphone.cpp's ranges; this
 *            object emits its own weak copies (and HbmEventHandler's/HbmPaneHandler's inline destructors) besides.
 * RESIDUALS. HbmKbList::create: `this` and the allocator take r18/r19 where retail has r19/r20 (retail keeps the
 *            animation count in r18); the declaration orders tried move the count above them.
 */

#include "types.h"
#include "homebutton/hbm_kb_list.h"
#include "homebutton/hkb_layout.h"
#include "homebutton/hkb_textbox.h"
#include "homebutton/hbm_kb_event.h"
#include "homebutton/hbm_hermite.h"
#include "MEM/MEMAllocFromAllocator.h"

/* The retail object has no extab/extabindex: exceptions are off for this band. */
#pragma exceptions off

/* A toolbar button: a band key with its state word and its kind; its class table (0x806503A8) and the out-of-line
 * copy of its inline destructor (0x80554AE0) are this unit's.  size: 0x34 */
class HbmKbListPart : public HbmKey {
public:
    HbmKbListPart(nw4r::lyt::Pane* pane) : HbmKey(pane) {}

    virtual void reset();
    virtual void setState(u32 state);
    virtual ~HbmKbListPart() {}
    virtual u32 getType();
    virtual u32 getState();

    /* +0x30 */ u32 mType;
}; /* size: 0x34 */

/* A confirm or cancel button (kind 0; class table 0x8065037C): steps a four-state selector.  size: 0x34 */
class HbmKbListPartB : public HbmKbListPart {
public:
    HbmKbListPartB(nw4r::lyt::Pane* pane) : HbmKbListPart(pane) { mType = 0; }

    virtual void setAnim(s32 anim);
    virtual ~HbmKbListPartB();
}; /* size: 0x34 */

/* A key-layout switch (kind 1; class table 0x80650350): steps a seven-state selector.  size: 0x34 */
class HbmKbListPartA : public HbmKbListPart {
public:
    HbmKbListPartA(nw4r::lyt::Pane* pane) : HbmKbListPart(pane) { mType = 1; }

    virtual void setAnim(s32 anim);
    virtual ~HbmKbListPartA();
}; /* size: 0x34 */

/* The toolbar buttons' animations (.rodata 0x8057B668). */
const HkbAnimDef hbmKbListAnimDefs[7] = {
    {0, "fs_VK_toolbar_a_normal.brlan"},
    {1, "fs_VK_toolbar_a_Focus-IN.brlan"},
    {2, "fs_VK_toolbar_a_Focus-OUT.brlan"},
    {3, "fs_VK_toolbar_a_Roll_over.brlan"},
    {4, "fs_VK_toolbar_a_Pushed.brlan"},
    {5, "fs_VK_toolbar_a_toggle-ON.brlan"},
    {6, "fs_VK_toolbar_a_toggle-OFF.brlan"},
};

/* The buttons whose animations the confirm button and the cellphone switch share. */
const char* hbmKbListConfirmGroup = "P_BT_cancel";
const char* hbmKbListCellphoneGroup = "P_kyChng_QWERTY";

/* The toolbar's buttons (.data 0x806501A0). */
HkbKeyDef hbmKbListKeyDefs[4] = {
    {0, "P_BT_cancel", 5, NULL,
     {&hbmKbListAnimDefs[0], &hbmKbListAnimDefs[1], &hbmKbListAnimDefs[2], &hbmKbListAnimDefs[3],
      &hbmKbListAnimDefs[4]}},
    {0, "P_BT_confirm", 5, hbmKbListConfirmGroup,
     {&hbmKbListAnimDefs[0], &hbmKbListAnimDefs[1], &hbmKbListAnimDefs[2], &hbmKbListAnimDefs[3],
      &hbmKbListAnimDefs[4]}},
    {1, "P_kyChng_QWERTY", 7, NULL,
     {&hbmKbListAnimDefs[0], &hbmKbListAnimDefs[1], &hbmKbListAnimDefs[2], &hbmKbListAnimDefs[3],
      &hbmKbListAnimDefs[4], &hbmKbListAnimDefs[5], &hbmKbListAnimDefs[6], &hbmKbListAnimDefs[7]}},
    {1, "P_kyChng_CP", 7, hbmKbListCellphoneGroup,
     {&hbmKbListAnimDefs[0], &hbmKbListAnimDefs[1], &hbmKbListAnimDefs[2], &hbmKbListAnimDefs[3],
      &hbmKbListAnimDefs[4], &hbmKbListAnimDefs[5], &hbmKbListAnimDefs[6], &hbmKbListAnimDefs[7]}},
};

/* Builds the toolbar; nothing in the DOL constructs one, so the linker strips the body and keeps the class tables and
 * the base's inline members it emits. */
HbmKbList::HbmKbList() {}

/* 0x80554714 (0xE0): Destroys the pane handler and the button parts. */
HbmKbList::~HbmKbList() {
    mpPaneHandler->~HbmEventHandler();
    MEMFreeToAllocator(mpAllocator, mpPaneHandler);
    for (HbmKey* pKey = (HbmKey*)nw4r::ut::List_GetNext(&mComponents, NULL); pKey != NULL;
         pKey = (HbmKey*)nw4r::ut::List_GetNext(&mComponents, NULL)) {
        nw4r::ut::List_Remove(&mComponents, pKey);
        pKey->destroy(mpAllocator);
    }
}

/* 0x805547F4 (0x40): Destroys the pane handler. */
HbmKbListPaneHandler::~HbmKbListPaneHandler() {}

/* 0x80554834 (0x2A4): Creates the pane handler and the pane manager, hides the root, and creates one part per toolbar
 * button with its animations. */
void HbmKbList::create(MEMAllocator* allocator) {
    const char* group;

    HbmKbListBase::create(allocator);
    HbmKbListPaneHandler* pHandler =
        new (MEMAllocFromAllocator(allocator, sizeof(HbmKbListPaneHandler))) HbmKbListPaneHandler(this);
    mpPaneHandler = pHandler;
    HbmPaneManager::create(allocator, pHandler);
    mpRoot->setVisible(0);
    mpRoot->setEnable(1);
    for (u16 i = 0; i < 4; i++) {
        const HkbKeyDef* def = &hbmKbListKeyDefs[i];
        HbmKbListPart* pPart = NULL;
        switch (def->type) {
        case 0:
            pPart = new (MEMAllocFromAllocator(allocator, sizeof(HbmKbListPartB)))
                HbmKbListPartB(findTextBox(def->name));
            break;
        case 1:
            pPart = new (MEMAllocFromAllocator(allocator, sizeof(HbmKbListPartA)))
                HbmKbListPartA(findTextBox(def->name));
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

/* 0x80554B38 (0x1C): Shows or hides the confirm button. */
void HbmKbList::setConfirmVisible(s32 visible) {
    setPaneVisible("P_BT_confirm", visible);
}

/* 0x80554B54 (0x1C): Shows or hides the cancel button. */
void HbmKbList::setCancelVisible(s32 visible) {
    setPaneVisible("P_BT_cancel", visible);
}

/* 0x80554B70 (0x1C): Sets the confirm button's label. */
void HbmKbList::setConfirmText(const wchar_t* text) {
    setPaneText("T_BT_confirm", text);
}

/* 0x80554B8C (0x1C): Sets the cancel button's label. */
void HbmKbList::setCancelText(const wchar_t* text) {
    setPaneText("T_BT_cancel", text);
}

/* 0x80554BA8 (0x214): Resets the pane manager and the buttons, selects the current layout mode, labels the buttons in
 * the current language and lays the panes out. */
void HbmKbList::init() {
    HbmPaneManager::init();
    getPaneAnim("P_kyChng_CP")->setState(0);
    getPaneAnim("P_kyChng_QWERTY")->setState(0);
    setKeyChangeMode(1);
    setLayoutMode(getLayoutMode());
    getPaneAnim("P_BT_confirm")->setState(0);
    getPaneAnim("P_BT_cancel")->setState(0);
    setConfirmText(hkbLayoutLabels[0][getLayoutType()]);
    setCancelText(hkbLayoutLabels[1][getLayoutType()]);
    setCancelVisible(1);
    setConfirmVisible(1);
    resetCursor();
    mpLyt->Animate(0);
    mpLyt->CalculateMtx(mDrawInfo);
}

/* 0x80554DC8 (0x138): Puts every toolbar button back to its rest state from its state and kind. */
void HbmKbList::refreshKeys() {
    const HkbKeyDef* def = hbmKbListKeyDefs;
    u32 i;

    for (i = 0; i < 4; i++, def++) {
        HbmKbListPart* pPart = (HbmKbListPart*)getPaneAnim(def->name);

        if (pPart == NULL) {
            continue;
        }
        switch (pPart->getState()) {
        case 1:
        case 3:
            pPart->setState(2);
            break;
        case 4:
            switch (pPart->getType()) {
            case 0:
                pPart->setState(2);
                break;
            case 1:
                pPart->setState(5);
                break;
            }
            break;
        case 8:
        case 11:
            pPart->setState(9);
            break;
        }
    }
}

/* Returns the part's state. */
u32 HbmKbListPart::getState() {
    return mState;
}

/* Stores the state, then hands it to the band key. */
void HbmKbListPart::setState(u32 state) {
    mState = state;
    HbmKey::setState(state);
}

/* Returns the part's kind. */
u32 HbmKbListPart::getType() {
    return mType;
}

/* 0x80554F18 (0xFC): Closes the toolbar on the confirm button: plays the confirm sound and resets the buttons and the
 * keyboard's sub-widgets. */
void HbmKbList::confirm() {
    mpSound->v_0x018();
    mpSound->play(9);
    refreshKeys();
    mpRoot->v_0x010();
    resetCursor();
    mpOwner->v_0x078()->v_0x074();
    mpOwner->v_0x080()->v_0x068();
    mpOwner->v_0x090()->v_0x038();
}

/* 0x80555014 (0xFC): Closes the toolbar on the cancel button: plays the cancel sound and resets the buttons and the
 * keyboard's sub-widgets. */
void HbmKbList::cancel() {
    mpSound->v_0x01C();
    mpSound->play(9);
    refreshKeys();
    mpRoot->v_0x010();
    resetCursor();
    mpOwner->v_0x078()->v_0x074();
    mpOwner->v_0x080()->v_0x068();
    mpOwner->v_0x090()->v_0x038();
}

/* 0x80555110 (0x14): Stores the language, then re-runs init. */
void HbmKbList::setLayoutType(s32 type) {
    mLayoutType = type;
    init();
}

/* 0x8055512C (0xA0): Plays the switch sound when the layout mode changes, then selects it. */
void HbmKbList::switchLayoutMode(u32 mode) {
    if (mode != getLayoutMode()) {
        if (mode != 0) {
            mpSound->play(22);
        } else {
            mpSound->play(23);
        }
    }
    setLayoutMode(mode);
}

/* 0x805551CC (0x1A8): Switches the key-change panes to the cellphone (non-zero) or QWERTY layout, stores the mode
 * and closes the text panel's candidates. */
void HbmKbList::setLayoutMode(u32 mode) {
    if (mode != 0) {
        s32 state = ((HbmKbListPart*)getPaneAnim("P_kyChng_CP"))->getState();

        if (state != 0 && state != 6) {
            getPaneAnim("P_kyChng_CP")->setState(6);
        }
        getPaneAnim("P_kyChng_QWERTY")->setAnim(0);
    } else {
        s32 state = ((HbmKbListPart*)getPaneAnim("P_kyChng_QWERTY"))->getState();

        if (state != 0 && state != 6) {
            getPaneAnim("P_kyChng_QWERTY")->setState(6);
        }
        getPaneAnim("P_kyChng_CP")->setAnim(0);
    }
    HbmKbListBase::setLayoutMode(mode);
}

/* 0x80555374 (0x98): Stores the key-change mode and shows or hides the three key-change panes with it. */
void HbmKbList::setKeyChangeMode(u8 mode) {
    mKeyChangeMode = mode;
    setPaneVisible("P_kyChng_QWERTY", mode);
    setPaneVisible("P_kyChng_CP", mode);
    setPaneVisible("P_keyChange", mode);
}

/* 0x8055540C (0x20): Finds the upward navigation node of the layout. */
nw4r::lyt::Pane* HbmKbList::findUpNode() {
    return mpLyt->mpRootPane->FindPaneByName("N_UP", true);
}

/* 0x8055542C (0x20): Finds the downward navigation node of the layout. */
nw4r::lyt::Pane* HbmKbList::findDownNode() {
    return mpLyt->mpRootPane->FindPaneByName("N_DOWN", true);
}

/* 0x8055544C (0x190): Steps the four-state selector on an input event (0 also forces state 4). */
void HbmKbListPartB::setAnim(s32 event) {
    if (event == 0) {
        setState(4);
    }
    switch ((s32)mState) {
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
        if (event == 1) {
            setState(1);
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

/* 0x805555DC (0x1BC): Steps the seven-state selector on an input event (0 forces state 4 outside states 4 and 5). */
void HbmKbListPartA::setAnim(s32 event) {
    if (event == 0 && (s32)mState != 5 && (s32)mState != 4) {
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
        if (event == 1) {
            setState(1);
        }
        break;
    case 4:
        if (event == 4) {
            setState(5);
        }
        break;
    case 6:
        if (event == 4) {
            setState(0);
        }
        if (event == 1) {
            setState(1);
        }
        break;
    }
}

/* 0x80555798 (0x274): Animates the toolbar button under a pane event and runs the confirm button and the
 * key-layout switches when they are pressed. */
void HbmKbListPaneHandler::onPaneEvent(HbmComponent* pComponent, u32 event, HbmPaneTrigger* pTrigger) {
    const char* name = pComponent->mpPane->mName;

    if (name[0] == 'B') {
        char partName[17];
        hbmNameCopyWithChar(partName, 17, name, 0, 'P');
        HbmKey* pPart = mpList->getPaneAnim(partName);
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
                mpList->setCursorTarget((u32)pPart->mpPane);
                pPart->setAnim(1);
                break;
            }
        }
        if (event == 4 && (pTrigger->trigger & 0x10)) {
            if (hbmNameMatches(partName, "P_BT_confirm")) {
                mpList->confirm();
            } else if (hbmNameMatches(partName, "P_BT_cancel")) {
            } else if (hbmNameMatches(partName, "P_kyChng_QWERTY")) {
                if (mpList->getLayoutMode() == 0) {
                    mpList->getPaneAnim("P_kyChng_CP")->setState(6);
                }
                mpList->switchLayoutMode(1);
            } else if (hbmNameMatches(partName, "P_kyChng_CP")) {
                if (mpList->getLayoutMode() != 0) {
                    mpList->getPaneAnim("P_kyChng_QWERTY")->setState(6);
                }
                mpList->switchLayoutMode(0);
            }
        }
    }
}

/* 0x80555A0C (0x8): Updates the pane manager's components. */
void HbmKbList::calcComponents() {
    HbmPaneManager::calcComponents();
}

/* Clears the part's state. */
void HbmKbListPart::reset() {
    mState = 0;
}

/* Destroys the part (the inline base destructors run here). */
HbmKbListPartA::~HbmKbListPartA() {}

/* Destroys the part (the inline base destructors run here). */
HbmKbListPartB::~HbmKbListPartB() {}
