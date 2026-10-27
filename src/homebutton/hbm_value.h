/*
 * homebutton/hbm_value.h - declarations of the symbols owned by `homebutton/hbm_value.cpp` that other units call or read.
 */
#ifndef HOMEBUTTON_HBM_VALUE_H
#define HOMEBUTTON_HBM_VALUE_H

#include "types.h"
#include "homebutton/hbm_widget.h"
#include "homebutton/hbm_vu_object.h"

#ifdef __cplusplus
extern "C" {
#endif

struct HbmTextWriter;

/* Stores the writer's standard-screen flag and loads the 4:3 (set) or 16:9 screen rectangle. */
void hbmTextWriterSetStandardScreen(struct HbmTextWriter* writer, s32 standard);

/* Prints `code`, or '*' while the writer masks its input. */
f32 hbmTextWriterPrintChar(struct HbmTextWriter* writer, u16 code);

extern f32 lbl_8079D700;

/* The text writer (the panel's +0x10 sub-object) and the band's text buffer, as the callers name them. */
struct HbmTextWriter;
struct HbmTextCaret;
#ifdef __cplusplus
class HbmTextBuffer;
#endif

/* Hands the text writer the characters [start, end) of `text`. */
void hbmTextWriterSetText(struct HbmTextWriter* writer, const u16* text, u32 start, u32 end);
/* Draws the lines [line, lineEnd) from the character `position` on, reporting the caret through `cursor`. */
void hbmTextWriterDrawLine(struct HbmTextWriter* writer, struct HbmTextCaret* caret, u32 position, s32 line, s32 lineEnd);
/* Draws the writer's text, the caret at `cursor`. */
void hbmTextWriterDraw(struct HbmTextWriter* writer, struct HbmTextCaret* caret);
/* Initialises the text writer. */
void hbmTextWriterInit(struct HbmTextWriter* writer);
#ifdef __cplusplus
/* The text buffer's setters: replace the text, clear it, type `text` under input mode `mode`, set the mode. */
void hbmTextSet(HbmTextBuffer* self, const u16* text);
void hbmTextClear(HbmTextBuffer* self);
void hbmTextPutInMode(HbmTextBuffer* self, const u16* text, s32 mode);
void hbmTextSetInputMode(HbmTextBuffer* self, s32 mode);
#endif

/* Links the Vu object's parts to each other, hands them the layout and the language and initialises them. */
void hbmVuInitParts(VuObject* self);

/* Reads the animation's current node out as an (id, frame) pair, or the empty pair. */
void hbmAnimSaveState(HbmWidgetNodeView* anim, HbmIdValue* out);

/* Puts the animation back on the node a saved (id, frame) pair names. */
void hbmAnimRestoreState(HbmWidgetNodeView* anim, const HbmIdValue* state);

#ifdef __cplusplus
}
#endif

#ifdef __cplusplus
namespace nw4r {
namespace ut {
class Font;
struct Rect;
}  // namespace ut
}  // namespace nw4r

extern "C" {
/* Takes the font, clears the character-width cache and measures the codes 0x20..0x17F into it. */
void hbmTextWriterRefresh(struct HbmTextWriter* writer, const nw4r::ut::Font* font);
/* Saves the viewport and projection and sets up an orthographic draw clipped to `clip`. */
void hbmTextWriterBeginDraw(struct HbmTextWriter* writer, const nw4r::ut::Rect* clip);
}
#endif

#ifdef __cplusplus
#include "nw4r/fn_805012C4.h"
#include "homebutton/hkb_drawinfo.h"

struct MEMAllocator;
struct HkbManager;
class HbmEventHandler;
class HbmSoundPlayer;
struct HbmKeyAnim;
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

class HbmComponent;
class HbmPaneHandler;

/* The component manager at the pane manager's +0x10 (the gui manager class of the band, tables 0x806582FC /
 * 0x80658358), as the slots the band calls.
 * size: 0x4 (approximation: only the vptr is touched) */
class HbmPaneLayout {
public:
    /* slot +0x008 */ virtual ~HbmPaneLayout();
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
    /* slot +0x044 */ virtual void setMaxComponents(u32 count);
    /* slot +0x048 */ virtual void setLayout(nw4r::lyt::Layout* layout);
    /* slot +0x04C */ virtual HbmComponent* findComponent(nw4r::lyt::Pane* pane);
    /* slot +0x050 */ virtual void v_0x050();
    /* slot +0x054 */ virtual void setDrawInfo(nw4r::lyt::DrawInfo* drawInfo);
    /* slot +0x058 */ virtual void setEnable(s32 enable);
}; /* size: 0x4 */

/* The pane manager's cursor (embedded at its +0x8C): the slots the band calls.
 * size: 0x18 (the vptr and five entries; the manager's allocator follows at +0xA4) */
class HbmPaneCursor {
public:
    /* slot +0x008 */ virtual void reset();
    /* slot +0x00C */ virtual void setTarget(u32 target);
    /* slot +0x010 */ virtual void v_0x010();
    /* slot +0x014 */ virtual u32 getTarget();

    /* +0x04 */ u32 unused_0x04[5];
}; /* size: 0x18 */

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
    /* slot +0x038 */ virtual void drawPane(const char* name);
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
    /* slot +0x06C */ virtual void calcDrawMtx(nw4r::math::MTX34* out, const nw4r::lyt::DrawInfo& drawInfo,
                                             const nw4r::math::MTX34* mtx);
    /* slot +0x070 */ virtual HbmPaneLayout* createRoot(MEMAllocator* allocator, HbmEventHandler* handler);

    /* +0x04 */ nw4r::lyt::Layout* mpLyt;
    /* +0x08 */ const char* mpLayoutName;
    /* +0x0C */ nw4r::lyt::ResourceAccessor* mpResAccessor;
    /* +0x10 */ HbmPaneLayout* mpRoot;
    /* +0x14 */ HbmSoundPlayer* mpSound;
    /* +0x18 */ nw4r::lyt::DrawInfo mDrawInfo;
    /* +0x6C */ nw4r::ut::List mComponents;
    /* +0x78 */ nw4r::ut::Rect mScreenRect;
    /* +0x88 */ u8 mbInputActive;
    /* +0x89 */ u8 mbStandardAspect;
    /* +0x8A */ u8 pad_0x8A[2];
    /* +0x8C */ HbmPaneCursor mCursor;
    /* +0xA4 */ MEMAllocator* mpPaneAllocator;
}; /* size: 0xA8 */

/* An animation of a key's animation list: its transform, its kind, its frame and how it plays.  size: 0x18 */
struct HbmKeyAnim {
    /* +0x00 */ nw4r::lyt::AnimTransform* mpAnim;
    /* +0x04 */ u32 kind;
    /* +0x08 */ f32 frame;
    /* +0x0C */ bool bLoop;
    /* +0x0D */ bool bRecursive;
    /* +0x0E */ u8 pad_0x0E[2];
    /* +0x10 */ nw4r::ut::Link mLink;
}; /* size: 0x18 */

/* The receiver of a key's animation events (the key's +0x18): slot 0x08 hears an animation start (0) or end (1).
 * size: 0x4 (approximation: only the vptr is touched) */
class HbmKeyListener {
public:
    /* slot +0x008 */ virtual void onAnimEvent(u32 event, HbmKey* key, HbmKeyAnim* anim);
}; /* size: 0x4 */

/* The band's animated pane component (class table 0x8065184C in this unit's .data): its pane, its animation list
 * and its state.  Keys, buttons and the widgets' parts derive from it.  size: 0x30 */
class HbmKey {
public:
    HbmKey(nw4r::lyt::Pane* pane) : mpPane(pane), mpCurAnim(NULL), mpListener(NULL) {
        nw4r::ut::List_Init(&mAnims, 0x10);
        mState = 0;
    }

    HbmKey(nw4r::lyt::Pane* pane, HbmKeyListener* listener) : mpPane(pane), mpCurAnim(NULL), mpListener(listener) {
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
    void addAnim(MEMAllocator* allocator, u32 kind, nw4r::lyt::AnimTransform* anim, bool loop, bool recursive);
    void addGroupAnim(MEMAllocator* allocator, u32 kind, nw4r::lyt::AnimTransform* anim, const char* group, bool loop,
                      bool recursive);

    /* +0x04 */ nw4r::lyt::Pane* mpPane;
    /* +0x08 */ nw4r::ut::List mAnims;
    /* +0x14 */ HbmKeyAnim* mpCurAnim;
    /* +0x18 */ HbmKeyListener* mpListener;
    /* +0x1C */ nw4r::ut::Link mLink;
    /* +0x24 */ u8 unused_0x24[8];
    /* +0x2C */ s32 mState;
}; /* size: 0x30 */
#endif

#endif
