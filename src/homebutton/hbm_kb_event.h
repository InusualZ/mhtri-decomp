/*
 * homebutton/hbm_kb_event.h - declarations of the symbols owned by `homebutton/hbm_kb_event.cpp` that other units call or read.
 */
#ifndef HOMEBUTTON_HBM_KB_EVENT_H
#define HOMEBUTTON_HBM_KB_EVENT_H

#include "types.h"
#include "homebutton/hbm_vu_object.h"
#include "nw4r/math.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Hands (id, arg) to every child in the object's list at +0x04. */
void hbmKbEventBroadcast(VuObject* self, u32 id, u32* arg);

/* The animations' y ranges (start, end) of the sliding parts and of the keyboard panel, and their alpha range
 * (.sdata 0x807946D8, 0x807946E8, 0x807946E0); the states move the panel's end to y 145 or 151. */
extern f32 hkbKbSlideY[2];
extern f32 hkbKbPanelY[2];
extern f32 hkbKbAlpha[2];

/* The label strings per layout type (.data 0x806574D8): thirteen rows of eight layouts. */
extern const wchar_t* hkbLayoutLabels[13][8];

#ifdef __cplusplus
}
#endif

#ifdef __cplusplus
#include "homebutton/hbm_kb_object.h"

/* The views of the keyboard's parts the band's dispatchers call: the slots are named by offset (the retail
 * names are not recoverable); no method is defined and nothing is constructed, so no table is emitted. */

struct MEMAllocator;
namespace nw4r {
namespace lyt {
class TextBox;
class Pane;
class Layout;
}  // namespace lyt
}  // namespace nw4r
class HkbKbPart44;
class HkbKbPart48;
class HkbKbPart50;
class HbmTextBufferSet;
class SysKbdInputView;
class SysKbdPart;
class SysKbdLayoutPart;
class SysKbdSlidePart;
class SysKbdTextPart;
class SysKbdPredictPart;
class SysKbdCursorPart;
class HbmGuiPaneWidget;

/* The wrapped object's sub-object at its +0x118: slot 0x10 links the cursor part.
 * size: 0x04 (approximation: only the compiler-emitted vptr at +0x00 is touched) */
class HkbKbLinkSub {
public:
    /* slot +0x008 */ virtual void v_0x008();
    /* slot +0x00C */ virtual void v_0x00C();
    /* slot +0x010 */ virtual void v_0x010(HkbKbPart48* part);
};

/* The controller handed to HkbInputWidget::setEventSink: it is stored at +0x0C and told its owner through slot 0x1C.
 * size: 0x04 (approximation: only the compiler-emitted vptr at +0x00 are touched) */
class HkbVuController {
public:
    /* slot +0x008 */ virtual void v_0x008();
    /* slot +0x00C */ virtual void v_0x00C();
    /* slot +0x010 */ virtual void v_0x010();
    /* slot +0x014 */ virtual void v_0x014();
    /* slot +0x018 */ virtual void v_0x018();
    /* slot +0x01C */ virtual void v_0x01C(VuObject* owner);
};

/* An element of the owner's child list at +0x04: slot 0x1C receives the two words hbmKbEventBroadcast broadcasts.
 * size: 0x04 (approximation: only the compiler-emitted vptr at +0x00 are touched) */
class HkbVuChild {
public:
    /* slot +0x008 */ virtual void v_0x008();
    /* slot +0x00C */ virtual void v_0x00C();
    /* slot +0x010 */ virtual void v_0x010();
    /* slot +0x014 */ virtual void v_0x014();
    /* slot +0x018 */ virtual void v_0x018();
    /* untyped: opaque handles passed through */
    /* slot +0x01C */ virtual void v_0x01C(u32 id, u32* arg);
};

/* The content object at +0x40: slot 0x0C returns the value fn_80561018 hands on, 0x18..0x28 are forwarded.
 * size: 0x04 (approximation: only the compiler-emitted vptr at +0x00 are touched) */
class HkbVuContent {
public:
    /* slot +0x008 */ virtual void v_0x008();
    /* slot +0x00C */ virtual u32 v_0x00C();
    /* slot +0x010 */ virtual void v_0x010();
    /* slot +0x014 */ virtual void v_0x014();
    /* slot +0x018 */ virtual void v_0x018();
    /* slot +0x01C */ virtual void v_0x01C();
    /* slot +0x020 */ virtual void v_0x020();
    /* slot +0x024 */ virtual void v_0x024();
    /* slot +0x028 */ virtual void v_0x028();
};

/* The widget the state setters run on: its own slots 0xEC, 0x100, 0x114 and 0x128.
 * size: 0x04 (approximation: only the compiler-emitted vptr at +0x00 are touched) */
class HkbVuWidget {
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
    /* slot +0x034 */ virtual s32 v_0x034();
    /* slot +0x038 */ virtual void v_0x038();
    /* slot +0x03C */ virtual void v_0x03C(u32 flags);
    /* slot +0x040 */ virtual void v_0x040(u32 flags);
    /* slot +0x044 */ virtual void v_0x044(u32 flags);
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
    /* slot +0x0A8 */ virtual HbmTextBufferSet* createTextBuffers();
    /* slot +0x0AC */ virtual SysKbdInputView* createInput();
    /* slot +0x0B0 */ virtual SysKbdLayoutPart* createLayoutPart();
    /* slot +0x0B4 */ virtual SysKbdSlidePart* createSlidePart();
    /* slot +0x0B8 */ virtual SysKbdTextPart* createTextPart();
    /* slot +0x0BC */ virtual SysKbdPredictPart* createPredictPart();
    /* slot +0x0C0 */ virtual SysKbdCursorPart* createCursorPart();
    /* slot +0x0C4 */ virtual SysKbdPart* createPart70();
    /* slot +0x0C8 */ virtual SysKbdPart* createPart74();
    /* slot +0x0CC */ virtual void v_0x0CC();
    /* slot +0x0D0 */ virtual void v_0x0D0();
    /* slot +0x0D4 */ virtual void v_0x0D4();
    /* slot +0x0D8 */ virtual void v_0x0D8();
    /* slot +0x0DC */ virtual void v_0x0DC();
    /* slot +0x0E0 */ virtual void v_0x0E0();
    /* slot +0x0E4 */ virtual void v_0x0E4();
    /* slot +0x0E8 */ virtual void v_0x0E8();
    /* slot +0x0EC */ virtual void v_0x0EC(u32 value);
    /* slot +0x0F0 */ virtual void v_0x0F0();
    /* slot +0x0F4 */ virtual void v_0x0F4();
    /* slot +0x0F8 */ virtual void v_0x0F8();
    /* slot +0x0FC */ virtual void v_0x0FC();
    /* slot +0x100 */ virtual void v_0x100();
    /* slot +0x104 */ virtual void v_0x104();
    /* slot +0x108 */ virtual void v_0x108();
    /* slot +0x10C */ virtual void v_0x10C();
    /* slot +0x110 */ virtual void v_0x110();
    /* slot +0x114 */ virtual void v_0x114();
    /* slot +0x118 */ virtual void v_0x118();
    /* slot +0x11C */ virtual void v_0x11C();
    /* slot +0x120 */ virtual void v_0x120();
    /* slot +0x124 */ virtual void v_0x124();
    /* slot +0x128 */ virtual void v_0x128();
    /* slot +0x12C */ virtual void v_0x12C();
    /* slot +0x130 */ virtual void v_0x130();
    /* slot +0x134 */ virtual void v_0x134();
    /* slot +0x138 */ virtual void v_0x138();
    /* slot +0x13C */ virtual void v_0x13C();
    /* slot +0x140 */ virtual void v_0x140();
    /* slot +0x144 */ virtual void v_0x144();
    /* slot +0x148 */ virtual SysKbdTextPart* createAltTextPart();
    /* slot +0x14C */ virtual HbmGuiPaneWidget* createGuiPart();
};

/* A part the state setters switch (the widget's +0x14/+0x18/+0x1C/+0x20 members): the slots take an on/off flag.
 * size: 0x04 (approximation: only the compiler-emitted vptr at +0x00 are touched) */
class HkbVuPart {
public:
    /* slot +0x008 */ virtual void v_0x008();
    /* slot +0x00C */ virtual void v_0x00C();
    /* slot +0x010 */ virtual void v_0x010();
    /* slot +0x014 */ virtual void v_0x014(s32 enable);
    /* slot +0x018 */ virtual void v_0x018();
    /* slot +0x01C */ virtual s32 v_0x01C();
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
    /* slot +0x054 */ virtual void v_0x054(s32 enable);
    /* slot +0x058 */ virtual void v_0x058();
    /* slot +0x05C */ virtual void v_0x05C();
    /* slot +0x060 */ virtual void v_0x060(s32 enable);
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
    /* slot +0x0A8 */ virtual void v_0x0A8();
    /* slot +0x0AC */ virtual void v_0x0AC(s32 enable);
    /* slot +0x0B0 */ virtual void v_0x0B0();
    /* slot +0x0B4 */ virtual void v_0x0B4();
    /* slot +0x0B8 */ virtual void v_0x0B8(s32 enable);
    /* slot +0x0BC */ virtual void v_0x0BC();
    /* slot +0x0C0 */ virtual void v_0x0C0(const wchar_t* label);
    /* slot +0x0C4 */ virtual void v_0x0C4(const wchar_t* label);
    /* slot +0x0C8 */ virtual void v_0x0C8();
    /* slot +0x0CC */ virtual void v_0x0CC();
    /* slot +0x0D0 */ virtual void v_0x0D0();
    /* slot +0x0D4 */ virtual void v_0x0D4();
    /* slot +0x0D8 */ virtual void v_0x0D8();
    /* slot +0x0DC */ virtual void v_0x0DC();
    /* slot +0x0E0 */ virtual void v_0x0E0();
    /* slot +0x0E4 */ virtual void v_0x0E4();
    /* slot +0x0E8 */ virtual void v_0x0E8();
    /* slot +0x0EC */ virtual void v_0x0EC();
    /* slot +0x0F0 */ virtual void v_0x0F0();
    /* slot +0x0F4 */ virtual void v_0x0F4();
    /* slot +0x0F8 */ virtual void v_0x0F8();
    /* slot +0x0FC */ virtual void v_0x0FC(s32 enable);
    /* slot +0x100 */ virtual void v_0x100();
    /* slot +0x104 */ virtual void v_0x104(s32 enable);
    /* slot +0x108 */ virtual void v_0x108(s32 enable);
    /* slot +0x10C */ virtual void v_0x10C(s32 enable);
    /* slot +0x110 */ virtual void v_0x110(s32 enable);
    /* slot +0x114 */ virtual void v_0x114();
    /* slot +0x118 */ virtual void v_0x118();
    /* slot +0x11C */ virtual void v_0x11C();
    /* slot +0x120 */ virtual void v_0x120(s32 enable);
    /* slot +0x124 */ virtual void v_0x124(s32 enable);
    /* slot +0x128 */ virtual void v_0x128(s32 enable);
    /* slot +0x12C */ virtual void v_0x12C();
    /* slot +0x130 */ virtual void v_0x130();
    /* slot +0x134 */ virtual void v_0x134();
    /* slot +0x138 */ virtual void v_0x138(s32 enable);
    /* slot +0x13C */ virtual void v_0x13C();
    /* slot +0x140 */ virtual void v_0x140();
    /* slot +0x144 */ virtual void v_0x144();
    /* slot +0x148 */ virtual void v_0x148();
    /* slot +0x14C */ virtual void v_0x14C();
    /* slot +0x150 */ virtual void v_0x150();
    /* slot +0x154 */ virtual void v_0x154();
    /* slot +0x158 */ virtual void v_0x158();
    /* slot +0x15C */ virtual void v_0x15C();
    /* slot +0x160 */ virtual void v_0x160();
    /* slot +0x164 */ virtual void v_0x164();
    /* slot +0x168 */ virtual void v_0x168();
    /* slot +0x16C */ virtual void v_0x16C();
    /* slot +0x170 */ virtual void v_0x170();
    /* slot +0x174 */ virtual void v_0x174();
    /* slot +0x178 */ virtual void v_0x178();
    /* slot +0x17C */ virtual void v_0x17C();
    /* slot +0x180 */ virtual void v_0x180();
    /* slot +0x184 */ virtual void v_0x184();
    /* slot +0x188 */ virtual void v_0x188();
    /* slot +0x18C */ virtual void v_0x18C();
    /* slot +0x190 */ virtual void v_0x190();
    /* slot +0x194 */ virtual void v_0x194();
    /* slot +0x198 */ virtual void v_0x198();
    /* slot +0x19C */ virtual void v_0x19C();
    /* slot +0x1A0 */ virtual void v_0x1A0();
    /* slot +0x1A4 */ virtual void v_0x1A4();
    /* slot +0x1A8 */ virtual void v_0x1A8();
    /* slot +0x1AC */ virtual void v_0x1AC();
    /* slot +0x1B0 */ virtual void v_0x1B0();
    /* slot +0x1B4 */ virtual void v_0x1B4();
    /* slot +0x1B8 */ virtual void v_0x1B8();
    /* slot +0x1BC */ virtual void v_0x1BC();
    /* slot +0x1C0 */ virtual void v_0x1C0();
    /* slot +0x1C4 */ virtual void v_0x1C4();
    /* slot +0x1C8 */ virtual void v_0x1C8();
    /* slot +0x1CC */ virtual void v_0x1CC();
    /* slot +0x1D0 */ virtual void v_0x1D0();
    /* slot +0x1D4 */ virtual void v_0x1D4();
    /* slot +0x1D8 */ virtual void v_0x1D8();
    /* slot +0x1DC */ virtual void v_0x1DC();
    /* slot +0x1E0 */ virtual void v_0x1E0();
    /* slot +0x1E4 */ virtual void v_0x1E4();
    /* slot +0x1E8 */ virtual void v_0x1E8();
    /* slot +0x1EC */ virtual void v_0x1EC();
    /* slot +0x1F0 */ virtual void v_0x1F0();
    /* slot +0x1F4 */ virtual void v_0x1F4();
    /* slot +0x1F8 */ virtual void v_0x1F8();
    /* slot +0x1FC */ virtual void v_0x1FC();
    /* slot +0x200 */ virtual void v_0x200();
    /* slot +0x204 */ virtual void v_0x204();
    /* slot +0x208 */ virtual void v_0x208();
    /* slot +0x20C */ virtual void v_0x20C();
    /* slot +0x210 */ virtual void v_0x210();
    /* slot +0x214 */ virtual void v_0x214();
    /* slot +0x218 */ virtual void v_0x218();
    /* slot +0x21C */ virtual void v_0x21C();
    /* slot +0x220 */ virtual void v_0x220();
    /* slot +0x224 */ virtual void v_0x224();
    /* slot +0x228 */ virtual void v_0x228();
    /* slot +0x22C */ virtual void v_0x22C();
    /* slot +0x230 */ virtual void v_0x230();
    /* slot +0x234 */ virtual void v_0x234();
    /* slot +0x238 */ virtual void v_0x238();
    /* slot +0x23C */ virtual void v_0x23C(s32 enable);
    /* slot +0x240 */ virtual void v_0x240();
    /* slot +0x244 */ virtual void v_0x244();
    /* slot +0x248 */ virtual void v_0x248();
    /* slot +0x24C */ virtual void v_0x24C();
    /* slot +0x250 */ virtual void v_0x250();
    /* slot +0x254 */ virtual void v_0x254();
    /* slot +0x258 */ virtual void v_0x258();
    /* slot +0x25C */ virtual void v_0x25C();
};

/* A sub-object embedded in the keyboard's parts; slots 0x1C (refresh) and 0x20 (pointer event) are the ones this unit calls.
 * size: 0x04 (approximation: only the compiler-emitted vptr at +0x00 are touched) */
class HkbKbSubPart {
public:
    /* slot +0x008 */ virtual void v_0x008();
    /* slot +0x00C */ virtual void v_0x00C();
    /* slot +0x010 */ virtual void v_0x010();
    /* slot +0x014 */ virtual void v_0x014();
    /* slot +0x018 */ virtual void v_0x018();
    /* slot +0x01C */ virtual void v_0x01C();
    /* slot +0x020 */ virtual s32 v_0x020(s32 key, f32 x, f32 y, s32 arg_b, s32 arg_c, s32 arg_d, KbArgs* args);
    /* untyped: the input record is passed through untouched */
    /* slot +0x024 */ virtual s32 v_0x024(void* input);
    /* slot +0x028 */ virtual nw4r::lyt::Layout* getLayout();
    /* slot +0x02C */ virtual nw4r::lyt::TextBox* findTextBox(const char* name);
    /* slot +0x030 */ virtual void v_0x030();
    /* slot +0x034 */ virtual void setInputActive(u8 active);
    /* slot +0x038 */ virtual void v_0x038();
    /* slot +0x03C */ virtual void v_0x03C();
    /* slot +0x040 */ virtual void v_0x040();
    /* slot +0x044 */ virtual void v_0x044();
    /* slot +0x048 */ virtual void v_0x048();
    /* slot +0x04C */ virtual void v_0x04C();
    /* slot +0x050 */ virtual void setPaneVisible(const char* name, s32 visible);
};

/* The wrapped keyboard object at +0x04 (and what the owner's slot 0x34 returns): slot 0xF0 is its state.
 * size: 0x04 (approximation: only the compiler-emitted vptr at +0x00 are touched) */
class HkbKbWrapped {
public:
    /* slot +0x008 */ virtual void v_0x008();
    /* slot +0x00C */ virtual void v_0x00C();
    /* slot +0x010 */ virtual void v_0x010();
    /* slot +0x014 */ virtual void v_0x014();
    /* slot +0x018 */ virtual void v_0x018(s32 mode, s32 arg);
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
    /* slot +0x0A8 */ virtual void v_0x0A8();
    /* slot +0x0AC */ virtual void v_0x0AC();
    /* slot +0x0B0 */ virtual void v_0x0B0();
    /* slot +0x0B4 */ virtual void v_0x0B4();
    /* slot +0x0B8 */ virtual void v_0x0B8();
    /* slot +0x0BC */ virtual void v_0x0BC();
    /* slot +0x0C0 */ virtual void v_0x0C0();
    /* slot +0x0C4 */ virtual void v_0x0C4();
    /* slot +0x0C8 */ virtual void v_0x0C8();
    /* slot +0x0CC */ virtual void v_0x0CC();
    /* slot +0x0D0 */ virtual void v_0x0D0();
    /* slot +0x0D4 */ virtual void v_0x0D4(MEMAllocator* allocator, VuHeader* texts);
    /* slot +0x0D8 */ virtual void v_0x0D8();
    /* slot +0x0DC */ virtual void v_0x0DC();
    /* slot +0x0E0 */ virtual void v_0x0E0();
    /* slot +0x0E4 */ virtual void v_0x0E4(s32 finished);
    /* slot +0x0E8 */ virtual void v_0x0E8();
    /* slot +0x0EC */ virtual void v_0x0EC();
    /* slot +0x0F0 */ virtual s32 v_0x0F0();
    /* slot +0x0F4 */ virtual void v_0x0F4();
    /* slot +0x0F8 */ virtual void v_0x0F8(u32 layout);
    /* slot +0x0FC */ virtual void v_0x0FC();
    /* slot +0x100 */ virtual void v_0x100();
    /* slot +0x104 */ virtual void v_0x104();
    /* slot +0x108 */ virtual void v_0x108();
    /* slot +0x10C */ virtual void v_0x10C();
    /* slot +0x110 */ virtual void v_0x110(u32 language);
    /* slot +0x114 */ virtual void v_0x114(u8 cursor);
    /* slot +0x118 */ virtual u8 v_0x118();
    /* slot +0x11C */ virtual void v_0x11C();
    /* slot +0x120 */ virtual void v_0x120();
    /* slot +0x124 */ virtual void v_0x124();
    /* slot +0x128 */ virtual void v_0x128();
    /* slot +0x12C */ virtual void v_0x12C();
    /* slot +0x130 */ virtual void v_0x130();
    /* slot +0x134 */ virtual void v_0x134();
    /* slot +0x138 */ virtual void v_0x138();
    /* slot +0x13C */ virtual void v_0x13C();
    /* slot +0x140 */ virtual void v_0x140();
    /* slot +0x144 */ virtual void v_0x144();
    /* slot +0x148 */ virtual void v_0x148();
    /* slot +0x14C */ virtual void v_0x14C();
    /* slot +0x150 */ virtual void v_0x150();
    /* slot +0x154 */ virtual void v_0x154();
    /* slot +0x158 */ virtual void v_0x158();
    /* slot +0x15C */ virtual void v_0x15C();
    /* slot +0x160 */ virtual void v_0x160();
    /* slot +0x164 */ virtual void v_0x164();
    /* slot +0x168 */ virtual void v_0x168();
    /* slot +0x16C */ virtual void v_0x16C();
    /* slot +0x170 */ virtual void v_0x170();
    /* slot +0x174 */ virtual void v_0x174();
    /* slot +0x178 */ virtual void v_0x178();
    /* slot +0x17C */ virtual void v_0x17C();
    /* slot +0x180 */ virtual void v_0x180();
    /* slot +0x184 */ virtual void v_0x184();
    /* slot +0x188 */ virtual void v_0x188();
    /* slot +0x18C */ virtual void v_0x18C();
    /* slot +0x190 */ virtual void v_0x190();
    /* slot +0x194 */ virtual void v_0x194();
    /* slot +0x198 */ virtual void v_0x198();
    /* slot +0x19C */ virtual void v_0x19C();
    /* slot +0x1A0 */ virtual void v_0x1A0();
    /* slot +0x1A4 */ virtual void v_0x1A4();
    /* slot +0x1A8 */ virtual void v_0x1A8();
    /* slot +0x1AC */ virtual void v_0x1AC();
    /* slot +0x1B0 */ virtual void v_0x1B0();
    /* slot +0x1B4 */ virtual void v_0x1B4();
    /* slot +0x1B8 */ virtual void v_0x1B8();
    /* slot +0x1BC */ virtual void v_0x1BC();
    /* slot +0x1C0 */ virtual void v_0x1C0();
    /* slot +0x1C4 */ virtual void v_0x1C4();
    /* slot +0x1C8 */ virtual void v_0x1C8();
    /* slot +0x1CC */ virtual void v_0x1CC();
    /* slot +0x1D0 */ virtual void v_0x1D0();
    /* slot +0x1D4 */ virtual void v_0x1D4();
    /* slot +0x1D8 */ virtual void v_0x1D8();
    /* slot +0x1DC */ virtual void v_0x1DC();
    /* slot +0x1E0 */ virtual void v_0x1E0();
    /* slot +0x1E4 */ virtual void v_0x1E4();
    /* slot +0x1E8 */ virtual void v_0x1E8();
    /* slot +0x1EC */ virtual void v_0x1EC();
    /* slot +0x1F0 */ virtual void v_0x1F0();
    /* slot +0x1F4 */ virtual void v_0x1F4();
    /* slot +0x1F8 */ virtual void v_0x1F8();
    /* slot +0x1FC */ virtual void v_0x1FC();
    /* slot +0x200 */ virtual void v_0x200();
    /* slot +0x204 */ virtual void v_0x204();
    /* slot +0x208 */ virtual void v_0x208();
    /* slot +0x20C */ virtual void v_0x20C();
    /* slot +0x210 */ virtual void v_0x210();
    /* slot +0x214 */ virtual void v_0x214();
    /* slot +0x218 */ virtual void v_0x218();
    /* slot +0x21C */ virtual void v_0x21C();
    /* slot +0x220 */ virtual s32 v_0x220(s32 key, s32 arg_b, s32 arg_c, s32 arg_d, KbArgs* args);
    /* slot +0x224 */ virtual s32 v_0x224(s32 key, f32 x, f32 y, s32 arg_b, s32 arg_c, s32 arg_d, KbArgs* args);
    /* untyped: the input record is passed through untouched */
    /* slot +0x228 */ virtual s32 v_0x228(void* input);
    /* slot +0x22C */ virtual void v_0x22C();
    /* slot +0x230 */ virtual void v_0x230();
    /* slot +0x234 */ virtual void v_0x234();
    /* slot +0x238 */ virtual void v_0x238();
    /* +0x004 */ u8 unused_0x004[0x114];
    /* +0x118 */ HkbKbLinkSub sub_0x118;
    /* +0x11C */ u8 unused_0x11C[0x18E8];
    /* +0x1A04 */ HkbKbSubPart child_0x1A04;
    /* slot +0x23C */ virtual void v_0x23C();
    /* slot +0x240 */ virtual void v_0x240();
    /* slot +0x244 */ virtual void v_0x244(f32 value);
    /* slot +0x248 */ virtual void v_0x248();
    /* slot +0x24C */ virtual void v_0x24C();
    /* slot +0x250 */ virtual f32 v_0x250();
    /* slot +0x254 */ virtual f32 v_0x254();
};

/* The part the owner's slot 0x38 returns: slot 0x20 receives the input record.
 * size: 0x04 (approximation: only the compiler-emitted vptr at +0x00 are touched) */
class HkbKbPart38 {
public:
    /* slot +0x008 */ virtual void v_0x008();
    /* slot +0x00C */ virtual void v_0x00C();
    /* slot +0x010 */ virtual void v_0x010();
    /* slot +0x014 */ virtual void v_0x014();
    /* slot +0x018 */ virtual void v_0x018();
    /* slot +0x01C */ virtual void v_0x01C();
    /* untyped: the input record is passed through untouched */
    /* slot +0x020 */ virtual void v_0x020(void* input);
};

/* The part the owner's slot 0x3C returns (used while the mode flag of slot 0x4C is set).
 * size: 0x04 (approximation: only the compiler-emitted vptr at +0x00 are touched) */
class HkbKbPart3C {
public:
    /* slot +0x008 */ virtual void v_0x008();
    /* slot +0x00C */ virtual void v_0x00C(MEMAllocator* allocator);
    /* slot +0x010 */ virtual void v_0x010();
    /* slot +0x014 */ virtual void v_0x014(HkbKbWrapped* wrapped);
    /* slot +0x018 */ virtual void v_0x018();
    /* slot +0x01C */ virtual void v_0x01C();
    /* slot +0x020 */ virtual void v_0x020();
    /* slot +0x024 */ virtual void v_0x024();
    /* slot +0x028 */ virtual void v_0x028(u32 language);
    /* slot +0x02C */ virtual void v_0x02C();
    /* slot +0x030 */ virtual void v_0x030();
    /* slot +0x034 */ virtual void v_0x034();
    /* slot +0x038 */ virtual void v_0x038();
    /* slot +0x03C */ virtual void v_0x03C();
    /* slot +0x040 */ virtual void v_0x040();
    /* slot +0x044 */ virtual void v_0x044();
    /* slot +0x048 */ virtual u8 v_0x048();
    /* slot +0x04C */ virtual void v_0x04C();
    /* slot +0x050 */ virtual void v_0x050();
    /* slot +0x054 */ virtual void v_0x054();
    /* slot +0x058 */ virtual void v_0x058();
    /* slot +0x05C */ virtual void v_0x05C();
    /* slot +0x060 */ virtual void v_0x060();
    /* slot +0x064 */ virtual void v_0x064(bool caps, u32 column, u32 row);
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
    /* slot +0x0A8 */ virtual void v_0x0A8();
    /* slot +0x0AC */ virtual void v_0x0AC();
    /* slot +0x0B0 */ virtual void v_0x0B0();
    /* slot +0x0B4 */ virtual void v_0x0B4();
    /* slot +0x0B8 */ virtual void v_0x0B8();
    /* slot +0x0BC */ virtual void v_0x0BC();
    /* slot +0x0C0 */ virtual void v_0x0C0();
    /* slot +0x0C4 */ virtual void v_0x0C4();
    /* slot +0x0C8 */ virtual void v_0x0C8();
    /* slot +0x0CC */ virtual void v_0x0CC();
    /* slot +0x0D0 */ virtual void v_0x0D0();
    /* slot +0x0D4 */ virtual void v_0x0D4();
    /* slot +0x0D8 */ virtual void v_0x0D8();
    /* slot +0x0DC */ virtual void v_0x0DC();
    /* slot +0x0E0 */ virtual void v_0x0E0();
    /* slot +0x0E4 */ virtual void v_0x0E4();
    /* slot +0x0E8 */ virtual void v_0x0E8();
    /* slot +0x0EC */ virtual void v_0x0EC();
    /* slot +0x0F0 */ virtual void v_0x0F0();
    /* slot +0x0F4 */ virtual void v_0x0F4();
    /* slot +0x0F8 */ virtual void v_0x0F8();
    /* slot +0x0FC */ virtual void v_0x0FC();
    /* slot +0x100 */ virtual void v_0x100();
    /* slot +0x104 */ virtual void v_0x104();
    /* slot +0x108 */ virtual void v_0x108();
    /* slot +0x10C */ virtual void v_0x10C();
    /* slot +0x110 */ virtual void v_0x110();
    /* slot +0x114 */ virtual void v_0x114();
    /* slot +0x118 */ virtual void v_0x118(HkbKbPart50* part);
    /* slot +0x11C */ virtual void v_0x11C(HkbKbPart44* part);
    /* slot +0x120 */ virtual void v_0x120();
    /* slot +0x124 */ virtual void v_0x124(s32 enable);
    /* slot +0x128 */ virtual void v_0x128();
    /* slot +0x12C */ virtual s32 v_0x12C(s32 key, f32 x, f32 y, s32 arg_b, s32 arg_c, s32 arg_d, KbArgs* args);
    /* untyped: the input record is passed through untouched */
    /* slot +0x130 */ virtual s32 v_0x130(void* input);
    /* +0x04 */ u8 unused_0x04[0x10];
    /* +0x14 */ u32 kanaFlags;
    /* +0x18 */ s32 kanaMode;
    /* +0x1C */ u8 unused_0x1C[0x0C];
    /* +0x28 */ s32 cursorColumn;
    /* +0x2C */ u8 unused_0x2C[4];
    /* +0x30 */ s32 cursorRow;
    /* +0x34 */ u8 unused_0x34[0x18];
    /* +0x4C */ HkbKbSubPart child_0x4C;
    /* slot +0x134 */ virtual void v_0x134(u8 state);
};

/* The part the owner's slot 0x40 returns (used while the mode flag of slot 0x4C is clear); slot 0x40 is its kind.
 * size: 0x04 (approximation: only the compiler-emitted vptr at +0x00 are touched) */
class HkbKbPart40 {
public:
    /* slot +0x008 */ virtual void v_0x008();
    /* slot +0x00C */ virtual void v_0x00C(MEMAllocator* allocator);
    /* slot +0x010 */ virtual void v_0x010();
    /* slot +0x014 */ virtual void v_0x014(HkbKbWrapped* wrapped);
    /* slot +0x018 */ virtual void v_0x018();
    /* slot +0x01C */ virtual void v_0x01C();
    /* slot +0x020 */ virtual void v_0x020();
    /* slot +0x024 */ virtual void v_0x024();
    /* slot +0x028 */ virtual void v_0x028(u32 language);
    /* slot +0x02C */ virtual void v_0x02C();
    /* slot +0x030 */ virtual void v_0x030();
    /* slot +0x034 */ virtual void v_0x034();
    /* slot +0x038 */ virtual void v_0x038();
    /* slot +0x03C */ virtual void v_0x03C();
    /* slot +0x040 */ virtual s32 v_0x040();
    /* slot +0x044 */ virtual void v_0x044();
    /* slot +0x048 */ virtual void v_0x048();
    /* slot +0x04C */ virtual void v_0x04C();
    /* slot +0x050 */ virtual void v_0x050();
    /* slot +0x054 */ virtual void v_0x054();
    /* slot +0x058 */ virtual void v_0x058();
    /* slot +0x05C */ virtual u8 v_0x05C();
    /* slot +0x060 */ virtual void v_0x060(u8 layout);
    /* slot +0x064 */ virtual void v_0x064(bool shift);
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
    /* slot +0x0A8 */ virtual void v_0x0A8();
    /* slot +0x0AC */ virtual void v_0x0AC();
    /* slot +0x0B0 */ virtual void v_0x0B0();
    /* slot +0x0B4 */ virtual void v_0x0B4();
    /* slot +0x0B8 */ virtual void v_0x0B8();
    /* slot +0x0BC */ virtual void v_0x0BC();
    /* slot +0x0C0 */ virtual void v_0x0C0();
    /* slot +0x0C4 */ virtual void v_0x0C4();
    /* slot +0x0C8 */ virtual void v_0x0C8();
    /* slot +0x0CC */ virtual void v_0x0CC();
    /* slot +0x0D0 */ virtual void v_0x0D0();
    /* slot +0x0D4 */ virtual void v_0x0D4();
    /* slot +0x0D8 */ virtual void v_0x0D8();
    /* slot +0x0DC */ virtual void v_0x0DC();
    /* slot +0x0E0 */ virtual void v_0x0E0();
    /* slot +0x0E4 */ virtual void v_0x0E4();
    /* slot +0x0E8 */ virtual void v_0x0E8();
    /* slot +0x0EC */ virtual void v_0x0EC();
    /* slot +0x0F0 */ virtual void v_0x0F0();
    /* slot +0x0F4 */ virtual void v_0x0F4();
    /* slot +0x0F8 */ virtual void v_0x0F8();
    /* slot +0x0FC */ virtual void v_0x0FC(HkbKbPart50* part);
    /* slot +0x100 */ virtual void v_0x100(HkbKbPart44* part);
    /* slot +0x104 */ virtual void v_0x104();
    /* slot +0x108 */ virtual void v_0x108();
    /* slot +0x10C */ virtual void v_0x10C();
    /* slot +0x110 */ virtual void v_0x110();
    /* slot +0x114 */ virtual s32 v_0x114(s32 key, f32 x, f32 y, s32 arg_b, s32 arg_c, s32 arg_d, KbArgs* args);
    /* untyped: the input record is passed through untouched */
    /* slot +0x118 */ virtual s32 v_0x118(void* input);
    /* +0x04 */ u8 unused_0x04[0x30];
    /* +0x34 */ HkbKbSubPart child_0x34;
};

/* The part the owner's slot 0x44 returns; slot 0xD8 says whether it is shown.
 * size: 0x28 (approximation: only the compiler-emitted vptr at +0x00 and the embedded sub-object are touched) */
class HkbKbPart44 {
public:
    /* slot +0x008 */ virtual void v_0x008();
    /* slot +0x00C */ virtual void v_0x00C(MEMAllocator* allocator);
    /* slot +0x010 */ virtual void v_0x010();
    /* slot +0x014 */ virtual void v_0x014(HkbKbWrapped* wrapped);
    /* slot +0x018 */ virtual void v_0x018();
    /* slot +0x01C */ virtual void v_0x01C();
    /* slot +0x020 */ virtual void v_0x020();
    /* slot +0x024 */ virtual void v_0x024();
    /* slot +0x028 */ virtual void v_0x028(u32 language);
    /* slot +0x02C */ virtual void v_0x02C();
    /* slot +0x030 */ virtual void v_0x030();
    /* slot +0x034 */ virtual void v_0x034();
    /* slot +0x038 */ virtual void v_0x038();
    /* slot +0x03C */ virtual void v_0x03C(u8 state);
    /* slot +0x040 */ virtual u8 v_0x040();
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
    /* slot +0x0A8 */ virtual void v_0x0A8();
    /* slot +0x0AC */ virtual void v_0x0AC();
    /* slot +0x0B0 */ virtual void v_0x0B0();
    /* slot +0x0B4 */ virtual void v_0x0B4();
    /* slot +0x0B8 */ virtual void v_0x0B8();
    /* slot +0x0BC */ virtual void v_0x0BC();
    /* slot +0x0C0 */ virtual void v_0x0C0();
    /* slot +0x0C4 */ virtual void v_0x0C4();
    /* slot +0x0C8 */ virtual void v_0x0C8();
    /* slot +0x0CC */ virtual void v_0x0CC();
    /* slot +0x0D0 */ virtual void v_0x0D0();
    /* slot +0x0D4 */ virtual void v_0x0D4();
    /* slot +0x0D8 */ virtual s32 v_0x0D8();
    /* slot +0x0DC */ virtual void v_0x0DC();
    /* slot +0x0E0 */ virtual s32 v_0x0E0(s32 key, f32 x, f32 y, s32 arg_b, s32 arg_c, s32 arg_d, KbArgs* args);
    /* untyped: the input record is passed through untouched */
    /* slot +0x0E4 */ virtual s32 v_0x0E4(void* input);
    /* +0x04 */ u8 pad_0x04[0x20];
    /* +0x24 */ HkbKbSubPart child_0x24; /* an embedded sub-object the dispatchers call */
};

/* The part the owner's slot 0x48 returns; slot 0x100 says whether it is shown, slot 0x40 takes a flag.
 * size: 0x17F8 (approximation: only the compiler-emitted vptr at +0x00 and the embedded sub-object are touched) */
class HkbKbPart48 {
public:
    /* slot +0x008 */ virtual void v_0x008();
    /* slot +0x00C */ virtual void v_0x00C(MEMAllocator* allocator);
    /* slot +0x010 */ virtual void v_0x010();
    /* slot +0x014 */ virtual void v_0x014(HkbKbWrapped* wrapped);
    /* slot +0x018 */ virtual void v_0x018();
    /* slot +0x01C */ virtual void v_0x01C();
    /* slot +0x020 */ virtual void v_0x020();
    /* slot +0x024 */ virtual void v_0x024();
    /* slot +0x028 */ virtual void v_0x028();
    /* slot +0x02C */ virtual void v_0x02C();
    /* slot +0x030 */ virtual void v_0x030();
    /* slot +0x034 */ virtual u8 v_0x034();
    /* slot +0x038 */ virtual void v_0x038();
    /* slot +0x03C */ virtual void v_0x03C();
    /* slot +0x040 */ virtual void v_0x040(s32 enable);
    /* slot +0x044 */ virtual void v_0x044();
    /* slot +0x048 */ virtual void v_0x048(u32 language);
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
    /* slot +0x0A8 */ virtual void v_0x0A8();
    /* slot +0x0AC */ virtual void v_0x0AC();
    /* slot +0x0B0 */ virtual void v_0x0B0();
    /* slot +0x0B4 */ virtual void v_0x0B4();
    /* slot +0x0B8 */ virtual void v_0x0B8();
    /* slot +0x0BC */ virtual void v_0x0BC();
    /* slot +0x0C0 */ virtual void v_0x0C0();
    /* slot +0x0C4 */ virtual void v_0x0C4();
    /* slot +0x0C8 */ virtual void v_0x0C8();
    /* slot +0x0CC */ virtual void v_0x0CC();
    /* slot +0x0D0 */ virtual void v_0x0D0();
    /* slot +0x0D4 */ virtual void v_0x0D4();
    /* slot +0x0D8 */ virtual void v_0x0D8();
    /* slot +0x0DC */ virtual void v_0x0DC();
    /* slot +0x0E0 */ virtual void v_0x0E0();
    /* slot +0x0E4 */ virtual void v_0x0E4();
    /* slot +0x0E8 */ virtual void v_0x0E8();
    /* slot +0x0EC */ virtual void v_0x0EC();
    /* slot +0x0F0 */ virtual void v_0x0F0();
    /* slot +0x0F4 */ virtual void v_0x0F4();
    /* slot +0x0F8 */ virtual void v_0x0F8();
    /* slot +0x0FC */ virtual void v_0x0FC();
    /* slot +0x100 */ virtual s32 v_0x100();
    /* +0x04 */ u8 pad_0x04[0x17F0];
    /* +0x17F4 */ HkbKbSubPart child_0x17F4; /* an embedded sub-object the dispatchers call */
    /* slot +0x104 */ virtual void v_0x104();
    /* slot +0x108 */ virtual void v_0x108();
    /* slot +0x10C */ virtual void v_0x10C();
    /* slot +0x110 */ virtual void v_0x110();
    /* slot +0x114 */ virtual void v_0x114();
    /* slot +0x118 */ virtual void v_0x118();
};

/* The part the owner's slot 0x4C returns; slot 0x10 is the mode flag the dispatchers branch on.
 * size: 0x18 (approximation: only the compiler-emitted vptr at +0x00 and the embedded sub-object are touched) */
class HkbKbPart4C {
public:
    /* slot +0x008 */ virtual void v_0x008(MEMAllocator* allocator);
    /* slot +0x00C */ virtual void v_0x00C();
    /* slot +0x010 */ virtual u32 v_0x010();
    /* slot +0x014 */ virtual void v_0x014(s32 enable);
    /* slot +0x018 */ virtual void v_0x018(u32 language);
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
    /* +0x04 */ u8 pad_0x04[0x10];
    /* +0x14 */ HkbKbSubPart child_0x14; /* an embedded sub-object the dispatchers call */
    /* slot +0x09C */ virtual void v_0x09C();
    /* slot +0x0A0 */ virtual void v_0x0A0();
    /* slot +0x0A4 */ virtual void v_0x0A4();
    /* slot +0x0A8 */ virtual void v_0x0A8();
    /* slot +0x0AC */ virtual void v_0x0AC();
    /* slot +0x0B0 */ virtual nw4r::lyt::Pane* v_0x0B0();
    /* slot +0x0B4 */ virtual nw4r::lyt::Pane* v_0x0B4();
};

/* The part the owner's slot 0x50 returns; slot 0xB8 says whether it is shown.
 * size: 0x20 (approximation: only the compiler-emitted vptr at +0x00 and the embedded sub-object are touched) */
class HkbKbPart50 {
public:
    /* slot +0x008 */ virtual void v_0x008();
    /* slot +0x00C */ virtual void v_0x00C(MEMAllocator* allocator);
    /* slot +0x010 */ virtual void v_0x010();
    /* slot +0x014 */ virtual void v_0x014(HkbKbWrapped* wrapped);
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
    /* slot +0x0A8 */ virtual void v_0x0A8();
    /* slot +0x0AC */ virtual void v_0x0AC();
    /* slot +0x0B0 */ virtual void v_0x0B0();
    /* slot +0x0B4 */ virtual void v_0x0B4();
    /* slot +0x0B8 */ virtual s32 v_0x0B8();
    /* +0x04 */ u8 pad_0x04[0x18];
    /* +0x1C */ HkbKbSubPart child_0x1C; /* an embedded sub-object the dispatchers call */
    /* slot +0x0BC */ virtual void v_0x0BC();
    /* slot +0x0C0 */ virtual void v_0x0C0();
    /* slot +0x0C4 */ virtual void v_0x0C4();
    /* slot +0x0C8 */ virtual void v_0x0C8();
    /* slot +0x0CC */ virtual void v_0x0CC(u32 layout);
    /* slot +0x0D0 */ virtual void v_0x0D0(u32 language);
};

/* The part the owner's slot 0x54 returns.
 * size: 0x08 (approximation: only the compiler-emitted vptr at +0x00 and the embedded sub-object are touched) */
class HkbKbPart54 {
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
    /* slot +0x078 */ virtual void v_0x078();
    /* slot +0x07C */ virtual void v_0x07C();
    /* slot +0x080 */ virtual void v_0x080();
    /* slot +0x084 */ virtual void v_0x084();
    /* slot +0x088 */ virtual void v_0x088();
    /* +0x04 */ HkbKbSubPart child_0x04; /* an embedded sub-object the dispatchers call */
};

/* The keyboard object the dispatchers run on: slots 0x34..0x54 return the wrapped object and its parts.
 * size: 0x04 (approximation: only the compiler-emitted vptr at +0x00 are touched) */
class HkbKbOwner {
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
    /* slot +0x034 */ virtual HkbKbWrapped* v_0x034();
    /* slot +0x038 */ virtual HkbKbPart38* v_0x038();
    /* slot +0x03C */ virtual HkbKbPart3C* v_0x03C();
    /* slot +0x040 */ virtual HkbKbPart40* v_0x040();
    /* slot +0x044 */ virtual HkbKbPart44* v_0x044();
    /* slot +0x048 */ virtual HkbKbPart48* v_0x048();
    /* slot +0x04C */ virtual HkbKbPart4C* v_0x04C();
    /* slot +0x050 */ virtual HkbKbPart50* v_0x050();
    /* slot +0x054 */ virtual HkbKbPart54* v_0x054();
};
#endif

#endif
