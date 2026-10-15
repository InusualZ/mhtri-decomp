/* homebutton/hkb_widget_tail.cpp - the last `.text` unit of the software keyboard: the USB keyboard's input widget
 *   (constructor 0x8056D814, table .data 0x80658458) and the keyboard state HkbKbdState it keeps, the keyboard layout
 *   HkbLayout (a nw4r::lyt::Layout whose CreateAnimTransform builds HkbAnimTransform objects, table .data 0x80658480)
 *   and the animation transform HkbAnimTransform (an AnimTransformBasic that allocates its animation links when it is
 *   bound, table .data 0x806584C0).
 * RANGE. .text 0x8056D814-0x8056F2B4 (17 functions); .rodata 0x8057C760-0x8057C808; .data 0x80658458-0x806584E4;
 *   .bss 0x80790DF8-0x80790E04; .sdata 0x80794750-0x80794754; .sdata2 0x8079D7B0-0x8079D7C8.
 *   Edges: the left edge follows homebutton/tiHKBManager.cpp, whose static initialiser 0x8056D78C (.ctors word
 *   0x8056F428) and destructor 0x8056D7D4 end it.  This unit has no `.ctors` word.
 *   The class tables 0x80658458 -> 0x80658480 -> 0x806584C0 rise, MWCC's reverse class order for the three classes
 *   defined here (input widget, HkbLayout, HkbAnimTransform), so the range stays one unit; the `.bss` record
 *   0x80790DF8 is the destructor-chain entry of hkbGetKbdState's static (our object emits the same 0xC bytes).
 * FLAGS. GC/3.0a5.2 + cflags_hkb, the band's flags (evidence: src/homebutton/hbm_kb_event.cpp FLAGS); no extab, so
 *   `#pragma exceptions off`.  `#pragma auto_inline on` around the animation-count helpers: retail inlines the
 *   recursive pane count two levels into itself and into HkbAnimTransform::Bind(Pane*).
 * NAMES. GUESS: the file name, from its place after tiHKBManager.cpp; GUESS class names HkbLayout, HkbAnimTransform
 *   (from their bases and tables), HkbKbdInput (with its owner and mode views) and helper names hkbAnimCountContents,
 *   hkbAnimCountContentsTree, hkbAnimBindNamedPane, hkbKbdInputActivate, hkbKbdInputSyncShift (read off the bodies).
 *   HkbKbdState and its members follow homebutton/hkb_keyboard.cpp's names (setCapsLock, the three bits).
 * SHAPES. hkbAnimCountContents counts in an `int` and hoists the content count; HkbAnimTransform::Bind(Pane*) counts
 *   the tree with a literal `true` (retail passes `li r5,1` and drops the inner tests) into a `u32`; the links are
 *   pointed through a two-store helper (retail computes the link address once for both stores).
 * SHAPES. The three `.sdata` bytes 0x80794750..0x80794752 are one 3-byte object (hkbKbdShiftDefaults): separate
 *   zero-initialised globals would land in `.sbss`.
 * RESIDUALS. HkbLayout's inline destructor is emitted here (weak) while retail keeps the copy at 0x8055B898 in
 *   homebutton/hbm_value.cpp's range; HkbLayout's table names it, so that map row needs the destructor's mangling.
 * RESIDUALS. hkbKbdInputSyncShift: retail loads hkbKbdShiftDefaults' second and third bytes as `sym+1@sda21` /
 *   `sym+2@sda21`; ours materialises the base (`li r3,sym@sda21`) and loads at +1/+2.
 * RESIDUALS. hkbGetKbdState's guard and object are retail's `.sbss` 0x80794D60/0x80794D64, inside
 *   menu/menu_plsearch.cpp's and menu/menu_sysmsg.cpp's claims (a seam request); ours are this object's own.
 * RESIDUALS. HkbLayout::CreateAnimTransform, hkbAnimCountContents, HkbAnimTransform::Bind(Pane*): register
 *   allocation only (the file-version test's two registers, the counter and the content count).
 * RESIDUALS. Unwritten: the input widget's constructor 0x8056D814 (its table names five other units' functions by
 *   their generated names), its key handler 0x8056DA60 (0xEDC) and 0x8056E93C, and 0x8056EA18 - they call
 *   homebutton/tiHKBManager.cpp's unnamed key-iterator and key-code functions.
 */

#include "types.h"
#include "homebutton/hkb_widget_tail.h"
#include "homebutton/tiHKBManager.h"
#include "homebutton/hkb_layout.h"
#include "MSL_C/strstr.h"
#include "Runtime.PPCEABI.H/memset.h"

/* The retail object has no extab/extabindex: exceptions are off for this band. */
#pragma exceptions off

namespace {

/* Returns whether a binary resource has the layout format's major version 0 and a minor version 8 to 10. */
inline bool TestFileVersion(const nw4r::ut::BinaryFileHeader& fileHeader) {
    u8 majorVer = (u8)((fileHeader.version >> 8) & 0xFF);
    u8 minorVer = (u8)(fileHeader.version & 0xFF);
    bool bValid = false;

    if (majorVer == 0 && minorVer >= 8 && minorVer <= 10) {
        bValid = true;
    }
    return bValid;
}

}  // namespace

/* The text-entry mode object the input widget asks (its owner's slot 0x98): slot 0x10 whether it is busy, 0xA8
 * whether it takes keys.  size: 0x4 (approximation: only the vptr is touched) */
class HkbKbdInputMode {
public:
    /* slot +0x008 */ virtual void v_0x008();
    /* slot +0x00C */ virtual void v_0x00C();
    /* slot +0x010 */ virtual s32 isBusy();
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
    /* slot +0x078 */ virtual void v_0x078();
    /* slot +0x07C */ virtual void v_0x07C();
    /* slot +0x080 */ virtual void v_0x080();
    /* slot +0x084 */ virtual void v_0x084();
    /* slot +0x088 */ virtual void v_0x088();
    /* slot +0x08C */ virtual void v_0x08C();
    /* slot +0x090 */ virtual void v_0x090();
    /* slot +0x094 */ virtual void v_0x094();
    /* slot +0x098 */ virtual void v_0x098();
    /* slot +0x09C */ virtual void v_0x09C();
    /* slot +0x0A0 */ virtual void v_0x0A0();
    /* slot +0x0A4 */ virtual void v_0x0A4();
    /* slot +0x0A8 */ virtual s32 takesKeys();
}; /* size: 0x04 */

/* The owner of the input widget (its +0x24): slot 0x78 returns the software keyboard, 0x98 the text-entry mode.
 * size: 0x4 (approximation: only the vptr is touched) */
class HkbKbdInputOwner {
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
    /* slot +0x078 */ virtual HkbKeyboard* getKeyboard();
    /* slot +0x07C */ virtual void v_0x07C();
    /* slot +0x080 */ virtual void v_0x080();
    /* slot +0x084 */ virtual void v_0x084();
    /* slot +0x088 */ virtual void v_0x088();
    /* slot +0x08C */ virtual void v_0x08C();
    /* slot +0x090 */ virtual void v_0x090();
    /* slot +0x094 */ virtual void v_0x094();
    /* slot +0x098 */ virtual HkbKbdInputMode* getInputMode();
}; /* size: 0x04 */

/* The USB keyboard's input widget (table .data 0x80658458, constructor 0x8056D814): the keys queued for the text
 * at +0x10, their count at +0x20, the first-activation flag at +0x21 and the owner it reports to at +0x24.
 * size: 0x28 */
struct HkbKbdInput {
    /* +0x00 */ void* vtable;
    /* +0x04 */ u8 unused_0x04[8];
    /* +0x0C */ u32 value_0C;
    /* +0x10 */ u16 queue[8];
    /* +0x20 */ s8 queued;
    /* +0x21 */ s8 activated;
    /* +0x22 */ u8 unused_0x22[2];
    /* +0x24 */ HkbKbdInputOwner* owner;
}; /* size: 0x28 */

/* The shift states the keyboard state takes: when the widget starts, when shift is reported down and up.
 * size: 0x3 */
struct HkbKbdShiftDefaults {
    /* +0x0 */ u8 onStart;
    /* +0x1 */ u8 onDown;
    /* +0x2 */ u8 onUp;
}; /* size: 0x3 */

HkbKbdShiftDefaults hkbKbdShiftDefaults = {0, 1, 0};

/* 0x8056D838 (0xA4): Takes the start shift state and sets the manager's modifiers, reporting the first
 * activation (0x100) once. */
extern "C" void hkbKbdInputActivate(HkbKbdInput* self) {
    HkbKbdState& kbd = hkbGetKbdState();
    kbd.mbLeftShift = hkbKbdShiftDefaults.onStart;
    u32 state = hkbGetModifierState(&hkbManager);
    if (!self->activated) {
        state |= 0x100;
        self->activated = 1;
    }
    hkbSetModifierState(&hkbManager, state, 0x700);
}

/* 0x8056D8DC (0x184): Follows the manager's shift modifier (bit 2) into the keyboard state and the software
 * keyboard's shift, while the text-entry mode is busy or takes keys. */
extern "C" void hkbKbdInputSyncShift(HkbKbdInput* self, HkbManager* manager) {
    HkbKbdState& kbd = hkbGetKbdState();
    u32 state = hkbGetModifierState(manager);
    if (self->owner->getInputMode()->isBusy() || self->owner->getInputMode()->takesKeys()) {
        bool shift = kbd.isShiftHeld();
        if (state & 2) {
            kbd.mbLeftShift = hkbKbdShiftDefaults.onDown;
            if (!shift) {
                self->owner->getKeyboard()->setShiftOn(0);
            }
        } else {
            kbd.mbLeftShift = hkbKbdShiftDefaults.onUp;
            if (shift && !kbd.isShiftHeld()) {
                self->owner->getKeyboard()->setShiftOff();
            }
        }
    }
}

#pragma push
#pragma auto_inline on

/* 0x8056EB10 (0xBC): The number of the animation's contents (pane or material) named `name`. */
extern "C" int hkbAnimCountContents(const nw4r::lyt::AnimTransform* anim, const char* name) {
    const nw4r::lyt::res::AnimationBlock* pRes = anim->mpRes;
    if (pRes == NULL) {
        return 0;
    }
    int num = 0;
    const u32* animContOffsets = nw4r::lyt::detail::ConvertOffsToPtr<u32>(pRes, pRes->animContOffsetsOffset);
    const u16 animContNum = pRes->animContNum;
    for (u16 i = 0; i < animContNum; ++i) {
        const nw4r::lyt::res::AnimationContent& animCont =
            *nw4r::lyt::detail::ConvertOffsToPtr<nw4r::lyt::res::AnimationContent>(pRes, animContOffsets[i]);
        if (animCont.type == 0) {
            if (strncmp(animCont.name, name, 17) == 0) {
                ++num;
            }
        } else {
            if (strncmp(animCont.name, name, 17) == 0) {
                ++num;
            }
        }
    }
    return num;
}

/* 0x8056EBCC (0xE4): The number of the animation's contents named by `pane` and, when `bRecursive`, by its
 * descendants. */
extern "C" int hkbAnimCountContentsTree(const nw4r::lyt::AnimTransform* anim, nw4r::lyt::Pane* pane, bool bRecursive) {
    int num = hkbAnimCountContents(anim, pane->mName);
    if (bRecursive) {
        for (nw4r::ut::LinkList<nw4r::lyt::Pane, 4>::Iterator it = pane->mChildList.GetBeginIter();
             it != pane->mChildList.GetEndIter(); ++it) {
            num += hkbAnimCountContentsTree(anim, &*it, bRecursive);
        }
    }
    return num;
}

#pragma pop

/* Points an animation link at a transform's content `idx`. */
static inline void hkbAnimLinkSet(nw4r::lyt::AnimationLink* link, nw4r::lyt::AnimTransform* anim, u16 idx) {
    link->mAnimTrans = anim;
    link->mIdx = idx;
}

/* An animation transform that allocates its animation links when it is bound, one per content it finds there.
 * size: 0x20 */
class HkbAnimTransform : public nw4r::lyt::AnimTransformBasic {
public:
    virtual void SetResource(const nw4r::lyt::res::AnimationBlock* pRes, nw4r::lyt::ResourceAccessor* pResAccessor);
    virtual void Bind(nw4r::lyt::Pane* pPane, bool bRecursive, bool bDisable);
    virtual void Bind(nw4r::lyt::Material* pMaterial, bool bDisable);
}; /* size: 0x20 */

/* 0x8056EA48 (0xC8): Sets the resource and loads its texture files through `pResAccessor`; the links wait for Bind. */
void HkbAnimTransform::SetResource(const nw4r::lyt::res::AnimationBlock* pRes,
                                   nw4r::lyt::ResourceAccessor* pResAccessor) {
    mpRes = pRes;
    mpFileResAry = NULL;

    if (pRes->fileNum > 0) {
        mpFileResAry = static_cast<void**>(nw4r::lyt::Layout::AllocMemory(sizeof(void*) * pRes->fileNum));
        if (mpFileResAry) {
            const u32* fileNameOffsets = nw4r::lyt::detail::ConvertOffsToPtr<u32>(mpRes, sizeof(*mpRes));
            for (int i = 0; i < mpRes->fileNum; ++i) {
                const char* fileName = nw4r::lyt::detail::ConvertOffsToPtr<char>(fileNameOffsets, fileNameOffsets[i]);
                mpFileResAry[i] = pResAccessor->GetResource('timg', fileName, NULL);
            }
        }
    }
}

/* 0x8056ECB0 (0x210): Allocates one link per content naming `pPane`'s panes and materials (below it when
 * `bRecursive`) and binds each of them. */
void HkbAnimTransform::Bind(nw4r::lyt::Pane* pPane, bool bRecursive, bool bDisable) {
    u32 animNum;
    (void)bDisable;
    if (!bRecursive) {
        animNum = hkbAnimCountContents(this, pPane->mName);
    } else {
        animNum = hkbAnimCountContentsTree(this, pPane, true);
    }

    mAnimLinkAry = static_cast<nw4r::lyt::AnimationLink*>(
        nw4r::lyt::Layout::AllocMemory(sizeof(nw4r::lyt::AnimationLink) * animNum));
    mAnimLinkNum = animNum;
    memset(mAnimLinkAry, 0, sizeof(nw4r::lyt::AnimationLink) * animNum);
    for (u16 i = 0; i < animNum; ++i) {
        new (&mAnimLinkAry[i]) nw4r::lyt::AnimationLink();
    }

    u32 linkIdx = 0;
    const u32* animContOffsets = nw4r::lyt::detail::ConvertOffsToPtr<u32>(mpRes, mpRes->animContOffsetsOffset);
    for (u16 i = 0; i < mpRes->animContNum; ++i) {
        const nw4r::lyt::res::AnimationContent& animCont =
            *nw4r::lyt::detail::ConvertOffsToPtr<nw4r::lyt::res::AnimationContent>(mpRes, animContOffsets[i]);
        if (animCont.type == 0) {
            nw4r::lyt::Pane* pFindPane = pPane->FindPaneByName(animCont.name, bRecursive);
            if (pFindPane) {
                hkbAnimLinkSet(&mAnimLinkAry[linkIdx], this, i);
                pFindPane->AddAnimationLink(&mAnimLinkAry[linkIdx]);
                ++linkIdx;
            }
        } else {
            nw4r::lyt::Material* pFindMat = pPane->FindMaterialByName(animCont.name, bRecursive);
            if (pFindMat) {
                hkbAnimLinkSet(&mAnimLinkAry[linkIdx], this, i);
                pFindMat->AddAnimationLink(&mAnimLinkAry[linkIdx]);
                ++linkIdx;
            }
        }
    }
}

/* 0x8056EEC0 (0x4): Materials are bound through their panes. */
void HkbAnimTransform::Bind(nw4r::lyt::Material* pMaterial, bool bDisable) {
    (void)pMaterial;
    (void)bDisable;
}

/* 0x8056EEC4 (0x168): Allocates one link per content named `name` and binds each to `pane` (pane contents) or to
 * its material (material contents). */
extern "C" void hkbAnimBindNamedPane(HkbAnimTransform* anim, nw4r::lyt::Pane* pane, const char* name) {
    u32 animNum = hkbAnimCountContents(anim, name);

    anim->mAnimLinkAry = static_cast<nw4r::lyt::AnimationLink*>(
        nw4r::lyt::Layout::AllocMemory(sizeof(nw4r::lyt::AnimationLink) * animNum));
    anim->mAnimLinkNum = animNum;
    memset(anim->mAnimLinkAry, 0, sizeof(nw4r::lyt::AnimationLink) * animNum);
    for (u16 j = 0; j < animNum; ++j) {
        for (u16 i = 0; i < animNum; ++i) {
            new (&anim->mAnimLinkAry[i]) nw4r::lyt::AnimationLink();
        }
    }

    u32 linkIdx = 0;
    const u32* animContOffsets =
        nw4r::lyt::detail::ConvertOffsToPtr<u32>(anim->mpRes, anim->mpRes->animContOffsetsOffset);
    for (u16 i = 0; i < anim->mpRes->animContNum; ++i) {
        const nw4r::lyt::res::AnimationContent& animCont =
            *nw4r::lyt::detail::ConvertOffsToPtr<nw4r::lyt::res::AnimationContent>(anim->mpRes, animContOffsets[i]);
        if (strncmp(animCont.name, name, 17) == 0) {
            hkbAnimLinkSet(&anim->mAnimLinkAry[linkIdx], anim, i);
            if (animCont.type == 0) {
                pane->AddAnimationLink(&anim->mAnimLinkAry[linkIdx]);
            } else {
                pane->GetMaterial()->AddAnimationLink(&anim->mAnimLinkAry[linkIdx]);
            }
            ++linkIdx;
        }
    }
}

/* 0x8056F02C (0x1A0): Creates one HkbAnimTransform per `pai1` block of an RLAN image of a known animation kind and
 * returns the last; NULL for an unknown file version. */
/* untyped: byte range - the RLAN resource image */
nw4r::lyt::AnimTransform* HkbLayout::CreateAnimTransform(const void* animResBuf, nw4r::lyt::ResourceAccessor* pResAcsr) {
    const nw4r::ut::BinaryFileHeader* pFileHead = static_cast<const nw4r::ut::BinaryFileHeader*>(animResBuf);

    if (!TestFileVersion(*pFileHead)) {
        return NULL;
    }

    const nw4r::ut::BinaryBlockHeader* pDataBlockHead =
        nw4r::lyt::detail::ConvertOffsToPtr<nw4r::ut::BinaryBlockHeader>(pFileHead, pFileHead->headerSize);
    nw4r::lyt::AnimTransform* pAnimTrans = NULL;

    for (int i = 0; i < pFileHead->dataBlocks; ++i) {
        switch (pDataBlockHead->kind) {
        case 'pai1':
            switch (pFileHead->signature) {
            case 'RLPA':
            case 'RLTS':
            case 'RLVC':
            case 'RLMC':
            case 'RLVI':
            case 'RLTP':
            case 'RLAN': {
                HkbAnimTransform* pAnimTransBasic =
                    new (nw4r::lyt::Layout::AllocMemory(sizeof(HkbAnimTransform))) HkbAnimTransform;
                if (pAnimTransBasic) {
                    pAnimTransBasic->SetResource(
                        reinterpret_cast<const nw4r::lyt::res::AnimationBlock*>(pDataBlockHead), pResAcsr);
                    pAnimTrans = pAnimTransBasic;
                }
            } break;
            }
            if (pAnimTrans) {
                mAnimTransList.PushBack(pAnimTrans);
            }
            break;
        }
        pDataBlockHead = nw4r::lyt::detail::ConvertOffsToPtr<nw4r::ut::BinaryBlockHeader>(pDataBlockHead,
                                                                                          pDataBlockHead->size);
    }
    return pAnimTrans;
}


/* 0x8056F224 (0x10): Starts with the caps lock off and no shift key held. */
HkbKbdState::HkbKbdState() {
    mbCapsLock = 0;
    mbLeftShift = 0;
    mbRightShift = 0;
}

/* 0x8056F234 (0x40): Nothing to release. */
HkbKbdState::~HkbKbdState() {
}

/* 0x8056F274 (0x40): Stores the caps lock and sets the manager's caps-lock modifier (0x200) to match. */
void HkbKbdState::setCapsLock(const u8& bCapsLock) {
    mbCapsLock = bCapsLock;
    if (mbCapsLock) {
        hkbSetModifierState(&hkbManager, 0x200, 0x200);
    } else {
        hkbSetModifierState(&hkbManager, 0, 0x200);
    }
}
