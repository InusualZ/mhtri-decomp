/*
 * homebutton/hbm_value.cpp - software-keyboard code of the tiHKBManager band: the wide-character text buffer (text,
 * cursor, selection anchor, input mode and KPR composition queue), the pane manager and its animated keys, the Vu
 * object's part plumbing (create, init, refresh, draw, pointer and input dispatch, screen aspect) and the deleting
 * destructors.
 *
 * RANGE.     .text 0x80558EB4..0x8055EAF0 (176 functions; the ones not defined below keep the target object's bytes).
 * FLAGS.     GC/3.0a5.2 + cflags_hkb (configure.py); the retail object has no extab/extabindex, so `#pragma exceptions off`.
 * FLAGS.     Scoped `#pragma auto_inline on` around hbmTextTruncate and hbmTextSetInputMode: retail inlines them into hbmTextTruncateClamped and
 *            hbmTextPutInMode (twice) and also keeps their out-of-line copies; -inline noauto inlines nothing on its own.
 * NAMES.     GUESS file name: the band's `hbm_` prefix + the value/text helpers; no `__FILE__` string, no runtime-dump name.
 * NAMES.     GUESS view classes HbmTextBuffer (the text buffer: its slots 0x1C length, 0x20 append, 0x24 insert, 0x28 delete,
 *            0x2C replace, 0x40 set character, 0x5C put character, 0x98 selection range, 0x9C character before the cursor,
 *            0xA0 set that character), HbmTextLayoutView (vptr at +0x4C, the panel's +0x10 sub-object), HbmVuView,
 *            HbmVuLinkPart, HbmVuNodeView and HbmWidgetView (dispatch views); slots are named by offset.
 * NAMES.     GUESS function names (hbmText*, hbmVu*, hbmDeletingDtorThunk*): read off the bodies; map rows renamed with
 *            them (requests hbm-value#1..#41 and hb-value-event#9).
 * NAMES.     GUESS HbmPaneManager::drawPane / calcDrawMtx / createRoot (slots 0x38, 0x6C, 0x70: draw a named pane, flip
 *            a matrix's y axis behind the view matrix, build the component manager); slots v_0x03C/v_0x040 switch the
 *            16:9 / 4:3 screen and v_0x044 loads the matching projection.
 * NAMES.     GUESS HbmKeyAnim fields (the animation transform, kind, frame, loop and recursive-bind flags) and
 *            HbmKeyListener (the key's +0x18 receiver of animation start/end).
 * NAMES.     GUESS classes HbmPaneManager and HbmKey (homebutton/hbm_value.h) and their members.
 * NAMES.     GUESS class HbmKeyboard (the keyboard the band builds; its table 0x806521A8 and destructor thunk
 *            0x8055EAB8 are this unit's) and HbmCellphoneKeypad (the keypad, table 0x80652074, thunk 0x8055EAC0).
 * SHAPES.    The view classes are polymorphic so MWCC emits the canonical `lwz r12,0(r3); lwz r12,<slot>(r12)` dispatch;
 *            no method is defined and nothing is constructed, so no table is emitted here (rule 10).
 * SHAPES.    hbmTextSetCharAt compares `capacity > index` and hbmTextLastChar tests an `s32` index (`subic.`): the other spellings
 *            swap the compare operands or add an `extsh.`.
 * SHAPES.    fn_8055DCD0 takes an `int` it narrows to u8: retail masks the argument (`clrlwi r0,r4,24`) and compares the
 *            masked copy, which a `u8` parameter does not emit.
 * SHAPES.    HbmKey's loop and recursive flags are `bool` (stored and passed raw); a u8 parameter adds the `neg/or/srwi`
 *            bool conversion before Pane::BindAnimation.
 * SHAPES.    create fetches the layout resource into a local before Build (retail reloads mpResAccessor); hbmAnimRestoreState
 *            runs through an inline helper so `this` and the saved state take retail's registers.
 * SHAPES.    hbmVuDispatchPointer / hbmVuDispatchInput OR the results as `list | panel | wrapped | mode` (the only order of
 *            24 that reproduces retail's two-pair tree); hbmVuApplyAspectRatio's body is an inline create also uses.
 * SHAPES.    hbmTextAppendChar's character buffer is `u16[12]` (retail's 0x30-byte frame).
 * RESIDUALS. fn_8055A888 calls the stale name fn_805041F4 (relocdiff: the target names CharWriter::Print) and returns
 *            u32 where retail passes Print's f32 through; its prototype is in homebutton/hbm_value.h (request
 *            hbm-value#72).
 * RESIDUALS. HbmTextBuffer duplicates the HbmU16Array and HbmTextValue views of homebutton/hbm_widget.h (same object; only
 *            this unit uses them); the merge is a header change outside this unit.
 * RESIDUALS. HbmPaneCursor (the manager's +0x8C) is the five-entry container homebutton/hkb_keyboard.h models as the
 *            non-virtual HkbRecentKeys; retail's is polymorphic with all-inline methods (table 0x80651814 here), so the
 *            manager's constructor (0x8055B710) and calcComponents (its local copy) wait for that class (request
 *            hb-value-event#12).
 * RESIDUALS. getPaneVisible: retail loads mpLyt before the prologue's saves; calcDrawMtx: retail sets r3 for the final
 *            PSMTXCopy at the end of each branch; v_0x03C: retail's 4:3 rectangle sits above the 16:9 one on the stack;
 *            hbmTextPutChar: retail keeps the `(u16)count` mask the u32 count drops.
 * RESIDUALS. Unwritten, need the unowned .bss table 0x80790250 (0x580 bytes, read only by hbmTextWriterInit and hbmTextWriterRefresh)
 *            claimed first; its link slot (between hbm_text_panel's and hbm_anim_record's .bss) leaves the owner
 *            ambiguous (request hbm-value#70): hbmTextWriterInit, hbmTextWriterRefresh.
 * RESIDUALS. Unwritten, store a vtable whose class this unit would have to emit (rule 10): fn_80558F38, fn_8055B710,
 *            fn_8055CAD0, fn_8055CB34 (the Vu object's constructor and destructor, table 0x80651950), createRoot (the gui
 *            manager's tables 0x80658358/0x806582FC) and the eight part factories in 0x8055DFCC..0x8055E778.
 * RESIDUALS. Unwritten: fn_8055EAA8, the unit's static initialiser (its .ctors entry), which stores lbl_8060E748 into the
 *            .sbss word lbl_80795A70; hbmTextWriterSetStandardScreen; the text writer's
 *            0x8055A4BC..0x8055B434 run (hbmTextWriterDraw/DrawLine and five neighbours, the panel's sub-object view).
 * RESIDUALS. 26 functions (7728 bytes) are unwritten, the largest hbmTextWriterDraw (1272), hbmTextWriterDrawLine (724),
 *            calcComponents (668) and fn_8055CB34 (492); the list is `python tools/units/sweepcomments.py --unit
 *            homebutton/hbm_value`.
 * RESIDUALS. .data 0x80651568..0x80652340 spans at least 10 TUs (datagap.py emission-order seams), so the data cannot match
 *            until the range is re-cut.
 */

#include "types.h"
#include "sys_mem.h"

/* The retail object has no extab/extabindex: exceptions are off for this band. */
#pragma exceptions off

#include "homebutton/hbm_widget.h"
#include "homebutton/hbm_vu_object.h"
#include "MEM/mem_allocator.h"
#include "MEM/mem_expheap.h"
#include "homebutton/hkb_layout.h"
#include "homebutton/hkb_keyboard.h"
#include "homebutton/hkb_cellphone.h"
#include "homebutton/hkb_widget_tail.h"
#include "homebutton/hbm_anim_record.h"
#include "homebutton/hbm_kb_child.h"
#include "homebutton/hbm_kb_list.h"
#include "homebutton/hbm_kb_widget.h"
#include "homebutton/hbm_text_panel.h"
#include "homebutton/hbm_value.h"
#include "homebutton/hbm_hermite.h"
#include "homebutton/hbm_kb_event.h"
#include "MSL_C/wstring.h"
#include "SC/SCApi.h"
#include "KPR/kpr.h"
#include "nw4r/fn_805012C4.h"
#include "nw4r/fn_80502828.h"
#include "homebutton/hkb_textbox.h"
#include "MTX/mtx44.h"
#include "RVLGX/GXTransform.h"
#include "RVLGX/GXPixel.h"

extern "C" void wmemset(void* dst, int value, u16 size);

extern f32 lbl_8079D700;

/* ---------------------------------------------------------------------------------------------------
 * the unit's own `.sdata2` float pool and `.sdata` globals: other splits own them, so they are
 * declared, never defined (playbook 29).
 * ------------------------------------------------------------------------------------------------ */

/* The Vu object's part at +0x10 (its slot 0xAC builds it): slot 0x10 initialises it, slot 0x14 links the wrapped
 * keyboard.  size: 0x04 (approximation: only the compiler-emitted vptr at +0x00 is touched) */
class HbmVuLinkPart {
public:
    /* slot +0x008 */ virtual void v_0x008();
    /* slot +0x00C */ virtual void v_0x00C();
    /* slot +0x010 */ virtual void v_0x010();
    /* slot +0x014 */ virtual void v_0x014(HkbKbWrapped* wrapped);
};

/* The dispatch view of the band's Vu objects (VuObject, its +0x1C sub-object and the objects its slots 0x68 and
 * 0xE0 return): the slots are the ones the VuVTable view of homebutton/hbm_vu_object.h names, declared as virtuals so
 * MWCC emits the canonical `lwz r12,0(r3); lwz r12,<slot>(r12)` dispatch.  No method is defined and nothing is
 * constructed here, so no table is emitted (rule 10).
 * size: 0x04 (approximation: only the compiler-emitted vptr at +0x00 is touched) */
class HbmVuView {
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
    /* slot +0x050 */ virtual void v_0x050(s32 arg);
    /* slot +0x054 */ virtual void v_0x054();
    /* slot +0x058 */ virtual void v_0x058(s32 first, s32 second);
    /* slot +0x05C */ virtual void v_0x05C();
    /* slot +0x060 */ virtual void v_0x060();
    /* slot +0x064 */ virtual void v_0x064();
    /* slot +0x068 */ virtual HbmVuView* v_0x068();
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
    /* slot +0x0A8 */ virtual VuHeader* v_0x0A8();
    /* slot +0x0AC */ virtual HbmVuLinkPart* v_0x0AC();
    /* slot +0x0B0 */ virtual HkbKbPart3C* v_0x0B0();
    /* slot +0x0B4 */ virtual HkbKbPart40* v_0x0B4();
    /* slot +0x0B8 */ virtual HkbKbWrapped* v_0x0B8();
    /* slot +0x0BC */ virtual HkbKbPart48* v_0x0BC();
    /* slot +0x0C0 */ virtual HkbKbPart4C* v_0x0C0();
    /* slot +0x0C4 */ virtual HkbKbPart50* v_0x0C4();
    /* slot +0x0C8 */ virtual HkbKbPart44* v_0x0C8();
    /* slot +0x0CC */ virtual void v_0x0CC();
    /* slot +0x0D0 */ virtual void v_0x0D0();
    /* slot +0x0D4 */ virtual void v_0x0D4();
    /* slot +0x0D8 */ virtual void v_0x0D8();
    /* slot +0x0DC */ virtual void v_0x0DC();
    /* slot +0x0E0 */ virtual HbmVuView* v_0x0E0();
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
    /* slot +0x118 */ virtual void v_0x118();
    /* slot +0x11C */ virtual void v_0x11C();
    /* slot +0x120 */ virtual void v_0x120();
    /* slot +0x124 */ virtual void v_0x124();
    /* slot +0x128 */ virtual void v_0x128();
    /* slot +0x12C */ virtual void v_0x12C();
    /* slot +0x130 */ virtual void v_0x130();
    /* slot +0x134 */ virtual void v_0x134();
    /* slot +0x138 */ virtual void v_0x138(u32 first, u32 second);
    /* slot +0x13C */ virtual void v_0x13C(u32 first, u32 second);
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
    /* slot +0x23C */ virtual void v_0x23C();
    /* slot +0x240 */ virtual void v_0x240();
};

/* The dispatch view of HbmWidget's table (the HbmWidgetVtable view of homebutton/hbm_widget.h).
 * size: 0x04 (approximation: only the compiler-emitted vptr at +0x00 is touched) */
class HbmWidgetView {
public:
    /* slot +0x008 */ virtual void v_0x008();
    /* slot +0x00C */ virtual void v_0x00C();
    /* slot +0x010 */ virtual void update();
    /* slot +0x014 */ virtual void hide();
    /* slot +0x018 */ virtual void v_0x018();
    /* slot +0x01C */ virtual void v_0x01C();
    /* slot +0x020 */ virtual void v_0x020();
    /* slot +0x024 */ virtual void v_0x024();
    /* slot +0x028 */ virtual void v_0x028();
    /* slot +0x02C */ virtual void v_0x02C();
    /* slot +0x030 */ virtual s32 isDone();
    /* slot +0x034 */ virtual void v_0x034();
    /* slot +0x038 */ virtual void v_0x038();
    /* slot +0x03C */ virtual void setText(const char* text, int on);
};

/* The wide-character text buffer the band's text-entry functions below work on: the text, its length, a cursor and
 * a selection anchor.  Its slots are named by offset; the retail names are not recoverable.  No method is defined
 * and nothing is constructed here, so no table is emitted (rule 10).
 * size: 0x50 (approximation: the largest offset these functions reach is +0x4C) */
class HbmTextBuffer {
public:
    /* slot +0x008 */ virtual void v_0x008();
    /* slot +0x00C */ virtual void v_0x00C();
    /* slot +0x010 */ virtual void v_0x010();
    /* slot +0x014 */ virtual void v_0x014();
    /* slot +0x018 */ virtual void v_0x018();
    /* slot +0x01C */ virtual u16 v_0x01C();
    /* slot +0x020 */ virtual void v_0x020(const u16* text);
    /* slot +0x024 */ virtual void v_0x024(u16 position, const u16* text);
    /* slot +0x028 */ virtual void v_0x028(u16 position, u16 count);
    /* slot +0x02C */ virtual void v_0x02C(u16 position, u16 count, const u16* text);
    /* slot +0x030 */ virtual void v_0x030();
    /* slot +0x034 */ virtual void v_0x034();
    /* slot +0x038 */ virtual void v_0x038();
    /* slot +0x03C */ virtual void v_0x03C();
    /* slot +0x040 */ virtual void v_0x040(u16 index, u16 ch);
    /* slot +0x044 */ virtual void v_0x044();
    /* slot +0x048 */ virtual void v_0x048();
    /* slot +0x04C */ virtual void v_0x04C(s32 arg);
    /* slot +0x050 */ virtual void v_0x050();
    /* slot +0x054 */ virtual s32 v_0x054();
    /* slot +0x058 */ virtual void v_0x058();
    /* slot +0x05C */ virtual void v_0x05C(u16 ch);
    /* slot +0x060 */ virtual void v_0x060(const u16* text);
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
    /* slot +0x098 */ virtual void v_0x098(u32* first, u32* second);
    /* slot +0x09C */ virtual u16 v_0x09C();
    /* slot +0x0A0 */ virtual void v_0x0A0(u16 ch);
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
    /* slot +0x0CC */ virtual s32 v_0x0CC();
    /* slot +0x0D0 */ virtual void v_0x0D0();
    /* slot +0x0D4 */ virtual void v_0x0D4();

    /* +0x00 is the compiler-emitted vptr */
    /* +0x04 */ u16  capacity;
    /* +0x06 */ u16  length;
    /* +0x08 */ u16  byteLength;
    /* +0x0A */ u16  pad_0x0A;
    /* +0x0C */ u16* text;
    /* +0x10 */ u16* workText;
    /* +0x14 */ u8   unused_0x14[4];
    /* +0x18 */ MEMAllocator* allocator;
    /* +0x1C */ u32  cursor;
    /* +0x20 */ u32  anchor;
    /* +0x24 */ u8   selecting;
    /* +0x25 */ u8   pad_0x25[3];
    /* +0x28 */ s32  inputMode;
    /* +0x2C */ KPRQueue queue;
    /* +0x44 */ u16  lookahead[5];
};

extern "C" void hbmTextTruncate(HbmTextBuffer* self, u16 length);
extern "C" void hbmTextSetInputMode(HbmTextBuffer* self, s32 mode);

/* 0x80558EB4 (0x84): Allocates the text and the work text from an allocator and clears them. */
extern "C" void hbmTextInit(HbmTextBuffer* self, MEMAllocator* allocator) {
    self->allocator = allocator;
    self->text = (u16*)MEMAllocFromAllocator(allocator, self->capacity * 2);
    wmemset(self->text, 0, self->capacity);
    self->length = 0;
    self->byteLength = 0;
    self->workText = (u16*)MEMAllocFromAllocator(allocator, self->capacity * 2);
    wmemset(self->workText, 0, self->capacity);
}

/* 0x80558FAC (0x6C): Appends one character and refreshes the buffer. */
extern "C" void hbmTextAppendChar(HbmTextBuffer* self, u16 ch) {
    u16 chars[12];
    chars[0] = ch;
    chars[1] = 0;
    wcsncat(self->text, chars, wcslen(chars));
    self->length++;
    self->v_0x030();
}

/* 0x80559018 (0x38): Removes the last character and refreshes the buffer. */
extern "C" void hbmTextRemoveLast(HbmTextBuffer* self) {
    if (self->length != 0) {
        self->length--;
    }
    self->text[self->length] = 0;
    self->v_0x030();
}

/* 0x80559050 (0xE4): Appends a string when it fits the capacity; 0 when it does not or is null. */
extern "C" s32 hbmTextAppend(HbmTextBuffer* self, const u16* text) {
    u32 count = wcslen(text);
    if (count + self->v_0x01C() > self->capacity) {
        return 0;
    }
    if (text == 0) {
        return 0;
    }
    while (*text != 0) {
        self->text[self->length] = *text;
        text++;
        self->length++;
    }
    self->text[self->length] = 0;
    self->v_0x030();
    return 1;
}

/* 0x80559134 (0x13C): Inserts a string at `position` when it fits the capacity; 0 when it does not. */
extern "C" s32 hbmTextInsert(HbmTextBuffer* self, u16 position, const u16* text) {
    u32 count = wcslen(text);
    if ((u16)count + self->v_0x01C() > self->capacity) {
        return 0;
    }
    if (text == 0 || position > self->v_0x01C()) {
        return 0;
    }
    wmemset(self->workText, 0, self->capacity);
    wcsncpy(self->workText, self->text + position, self->v_0x01C() - position);
    self->length = position;
    self->text[position] = 0;
    self->v_0x020(text);
    self->v_0x020(self->workText);
    return 1;
}

/* 0x80559270 (0x134): Deletes up to `count` characters at `position`, keeping the text after them. */
extern "C" void hbmTextDelete(HbmTextBuffer* self, u16 position, u16 count) {
    if (self->v_0x01C() == 0) {
        return;
    }
    if (position > self->v_0x01C()) {
        return;
    }
    if (position + count >= self->v_0x01C()) {
        count = self->v_0x01C() - position;
    }
    wmemset(self->workText, 0, self->capacity);
    wcsncpy(self->workText, self->text + position + count, self->v_0x01C() - (position + count));
    self->length = position;
    self->text[position] = 0;
    self->v_0x020(self->workText);
}

/* 0x805593A4 (0xC0): Replaces up to `count` characters at `position` with a string. */
extern "C" void hbmTextReplace(HbmTextBuffer* self, u16 position, u16 count, const u16* text) {
    if (position + count >= self->v_0x01C()) {
        count = self->v_0x01C() - position;
    }
    self->v_0x028(position, count);
    self->v_0x024(position, text);
}

/* 0x80559464 (0x5C): Clears the buffer, then appends a string through slot 0x20. */
extern "C" void hbmTextSet(HbmTextBuffer* self, const u16* text) {
    self->v_0x018();
    self->v_0x020(text);
}

/* Bounds-checked store of one character. */
extern "C" void hbmTextSetCharAt(HbmTextBuffer* self, u16 index, u16 ch) {
    if (self->capacity > index) {
        self->text[index] = ch;
    }
}

#pragma push
#pragma auto_inline on
/* 0x805594DC (0x8C): Clears every character from `length` on through slot 0x40 and truncates the text there. */
extern "C" void hbmTextTruncate(HbmTextBuffer* self, u16 length) {
    u16 i;
    for (i = length; i < self->length; i++) {
        self->v_0x040(i, 0);
    }
    self->length = length;
    self->v_0x030();
}
#pragma pop

/* Clears the text and its counters. */
extern "C" void hbmTextClear(HbmTextBuffer* self) {
    wmemset(self->text, 0, self->capacity);
    self->length = 0;
    self->byteLength = 0;
}

/* The last character of the text, or 0 when it is empty. */
extern "C" u16 hbmTextLastChar(HbmTextBuffer* self) {
    s32 index = self->length - 1;
    if (index >= 0) {
        return self->text[index];
    }
    return 0;
}

/* Recomputes the text's length in bytes. */
extern "C" void hbmTextCountBytes(HbmTextBuffer* self) {
    u16 i;
    self->byteLength = 0;
    for (i = 0; i < self->length; i++) {
        self->byteLength += 2;
    }
}

/* 0x80559604 (0x34): The index of the character that holds a byte offset, or 0 past the end. */
extern "C" u32 hbmTextIndexAtByte(HbmTextBuffer* self, u32 byteOffset) {
    u32 bytes = 0;
    u32 i;
    for (i = 0; i < self->length; i++) {
        bytes += 2;
        if (bytes > byteOffset) {
            return i;
        }
    }
    return 0;
}

/* 0x80559638 (0x1DC): Types one character: under an input mode it goes through the composition queue (a line break
 * flushes it), then the result replaces the selection or is inserted at the cursor. */
extern "C" void hbmTextPutChar(HbmTextBuffer* self, u16 ch) {
    u16 chars[5];
    u32 first;
    u32 second;
    u32 count;
    u8 i;

    if (ch == 0xFFFE) {
        return;
    }
    chars[4] = 0;
    chars[3] = 0;
    chars[2] = 0;
    chars[1] = 0;
    chars[0] = 0;
    if (self->inputMode != 0) {
        self->lookahead[4] = 0;
        self->lookahead[3] = 0;
        self->lookahead[2] = 0;
        self->lookahead[1] = 0;
        self->lookahead[0] = 0;
        if (ch == 10 && (u8)KPRLookAhead(&self->queue, 0, 0) != 0) {
            count = KPRPutChar(&self->queue, 0xFFFF);
        } else {
            count = KPRPutChar(&self->queue, hbmToWideChar(ch));
        }
        for (i = 0; i < count; i++) {
            chars[i] = KPRGetChar(&self->queue);
        }
        KPRLookAhead(&self->queue, (u32)self->lookahead, 5);
    } else {
        chars[0] = ch;
        count = 1;
        chars[1] = 0;
    }
    self->v_0x04C(0);
    self->v_0x098(&first, &second);
    if (first == second) {
        self->v_0x024((u16)self->cursor, chars);
        self->cursor += (u16)count;
    } else {
        self->v_0x02C((u16)first, (u16)(second - first), chars);
        self->cursor = first + (u16)count;
    }
    self->anchor = self->cursor;
}

/* Appends a line break through slot 0x5C. */
extern "C" void hbmTextPutNewline(HbmTextBuffer* self) {
    self->v_0x05C(10);
}

/* 0x80559828 (0x64): Appends a zero-terminated string one character at a time through slot 0x5C. */
extern "C" void hbmTextPutString(HbmTextBuffer* self, const u16* text) {
    if (text != 0) {
        const u16* p;
        for (p = text; *p != 0; p++) {
            self->v_0x05C(*p);
        }
    }
}

/* 0x8055988C (0x144): Puts a string through slot 0x60 under a temporary input mode. */
extern "C" void hbmTextPutInMode(HbmTextBuffer* self, const u16* text, s32 mode) {
    s32 previous = self->inputMode;
    hbmTextSetInputMode(self, mode);
    self->v_0x060(text);
    hbmTextSetInputMode(self, previous);
}

/* 0x805599D0 (0x168): Deletes backwards: the newest composing character, else the selection, else the character
 * before the cursor (or hands slot 0x4C a 0 when slot 0x54 asks for it). */
extern "C" void hbmTextBackspace(HbmTextBuffer* self) {
    u32 first;
    u32 second;
    u16 empty;

    if (self->inputMode != 0 && (u8)KPRLookAhead(&self->queue, 0, 0) != 0) {
        u8 index;
        self->queue.pending--;
        index = KPRLookAhead(&self->queue, 0, 0);
        self->lookahead[index] = 0;
        return;
    }
    self->v_0x098(&first, &second);
    if (first == second && first == 0) {
        return;
    }
    if (first == second) {
        if (self->v_0x054() != 0) {
            self->v_0x04C(0);
        } else {
            if (self->cursor != 0) {
                self->cursor--;
            }
            self->v_0x028((u16)self->cursor, 1);
        }
    } else {
        empty = 0;
        self->v_0x02C((u16)first, (u16)(second - first), &empty);
        self->cursor = first;
    }
    self->anchor = self->cursor;
}

/* 0x80559B38 (0x70): Inserts a string at the cursor and moves the cursor and the anchor past it. */
extern "C" void hbmTextInsertAtCursor(HbmTextBuffer* self, const u16* text) {
    if (text != 0) {
        self->v_0x024((u16)self->cursor, text);
        self->anchor = self->cursor += wcslen(text);
    }
}

/* 0x80559BA8 (0x74): Moves the cursor one character right; at the end, reports whether no selection is open. */
extern "C" s32 hbmTextCursorRight(HbmTextBuffer* self) {
    u16 length = self->v_0x01C();
    if (self->cursor >= length) {
        return self->selecting == 0;
    }
    self->cursor++;
    if (self->selecting == 0) {
        self->anchor = self->cursor;
    }
    return 0;
}

/* Moves the cursor one character left; the anchor follows while no selection is open. */
extern "C" u32 hbmTextCursorLeft(HbmTextBuffer* self) {
    if (self->cursor == 0) {
        return 0;
    }
    self->cursor--;
    if (self->selecting == 0) {
        self->anchor = self->cursor;
    }
    return 1;
}

/* Places the cursor; the anchor follows while no selection is open. */
extern "C" void hbmTextSetCursor(HbmTextBuffer* self, u32 cursor) {
    u8 selecting = self->selecting;
    self->cursor = cursor;
    if (selecting == 0) {
        self->anchor = cursor;
    }
}

/* Opens a selection at the cursor. */
extern "C" void hbmTextBeginSelection(HbmTextBuffer* self) {
    self->selecting = 1;
    self->anchor = self->cursor;
}

/* Closes the selection. */
extern "C" void hbmTextEndSelection(HbmTextBuffer* self) {
    self->selecting = 0;
}

/* Reads the cursor and the anchor out. */
extern "C" void hbmTextGetCursor(HbmTextBuffer* self, u32* cursor, u32* anchor) {
    *cursor = self->cursor;
    *anchor = self->anchor;
}

/* 0x80559C9C (0x64): Whether the cursor is off the start or a selection is open; otherwise whether slot 0xCC reports 0. */
extern "C" s32 hbmTextCanMoveLeft(HbmTextBuffer* self) {
    if (self->cursor != 0 || (self->cursor == 0 && self->cursor != self->anchor)) {
        return 1;
    }
    if (self->v_0x0CC() == 0) {
        return 1;
    }
    return 0;
}

/* 0x80559D00 (0x78): Deletes the character under the cursor; 0 at the end of the text. */
extern "C" s32 hbmTextDeleteAtCursor(HbmTextBuffer* self) {
    u32 cursor = self->cursor;
    if (cursor >= self->v_0x01C()) {
        return 0;
    }
    self->v_0x028((u16)cursor, 1);
    return 1;
}

/* Writes the selection's two ends ordered, smaller first. */
extern "C" void hbmTextGetSelection(HbmTextBuffer* self, u32* first, u32* second) {
    if (self->cursor > self->anchor) {
        *first = self->anchor;
        *second = self->cursor;
    } else {
        *first = self->cursor;
        *second = self->anchor;
    }
}

/* 0x80559DA8 (0x64): The character before the selection's far end, or 0 at the start. */
extern "C" u16 hbmTextCharBeforeSelection(HbmTextBuffer* self) {
    u32 first;
    u32 second;
    self->v_0x098(&first, &second);
    if (second == 0) {
        return 0;
    }
    return self->text[second - 1];
}

/* 0x80559E0C (0x68): Replaces the character before the selection's far end. */
extern "C" void hbmTextSetCharBeforeSelection(HbmTextBuffer* self, u16 ch) {
    u32 first;
    u32 second;
    self->v_0x098(&first, &second);
    if (second != 0) {
        self->text[second - 1] = ch;
    }
}

/* 0x80559E74 (0x40): Whether the character before the selection is a voiced kana. */
extern "C" s32 hbmTextIsDakutenAtCursor(HbmTextBuffer* self) {
    return hbmKanaIsDakuten(self->v_0x09C()) != 0;
}

/* 0x80559EB4 (0x5C): Toggles the voiced mark of the character before the selection. */
extern "C" void hbmTextToggleDakutenAtCursor(HbmTextBuffer* self) {
    self->v_0x0A0(hbmKanaToggleDakuten(self->v_0x09C()));
}

/* 0x80559F10 (0x40): Whether the character before the selection is a semi-voiced kana. */
extern "C" s32 hbmTextIsHandakutenAtCursor(HbmTextBuffer* self) {
    return hbmKanaIsHandakuten(self->v_0x09C()) != 0;
}

/* 0x80559F50 (0x5C): Toggles the semi-voiced mark of the character before the selection. */
extern "C" void hbmTextToggleHandakutenAtCursor(HbmTextBuffer* self) {
    self->v_0x0A0(hbmKanaToggleHandakuten(self->v_0x09C()));
}

/* 0x80559FAC (0x5C): Cycles the kana forms of the character before the selection. */
extern "C" void hbmTextCycleKanaAtCursor(HbmTextBuffer* self) {
    self->v_0x0A0(hbmKanaCycle(self->v_0x09C()));
}

/* 0x8055A008 (0x40): Whether the character before the selection is a small kana. */
extern "C" s32 hbmTextIsSmallAtCursor(HbmTextBuffer* self) {
    return hbmKanaIsSmall(self->v_0x09C()) != 0;
}

/* 0x8055A048 (0x5C): Toggles the small form of the character before the selection. */
extern "C" void hbmTextToggleSmallAtCursor(HbmTextBuffer* self) {
    self->v_0x0A0(hbmKanaToggleSmall(self->v_0x09C()));
}

/* The text-layout sub-object whose vptr sits at its own +0x4C (the panel's +0x10 sub-object, `Panel805482CC_Sub`
 * in fn_805482CC.h): slot 0x90 re-lays the text out.
 * size: 0xDC (approximation: the largest offset reached is +0xD8) */
class HbmTextLayoutView {
    /* +0x00 */ u8 unused_0x00[0x4C];
    /* +0x4C is the compiler-emitted vptr */
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
    /* slot +0x08C */ virtual void v_0x08C();
    /* slot +0x090 */ virtual void v_0x090();

    /* +0x50 */ u8 unused_0x50[0x84];
    /* +0xD4 */ s32 line;
    /* +0xD8 */ u32 lineMode;
};

/* 0x8055A0A4 (0xDC): Whether the cursor sits, with no selection open, right after a sentence end ('.', '!' or '?'). */
extern "C" s32 hbmTextIsAfterSentenceEnd(HbmTextBuffer* self) {
    u32 first;
    u32 second;

    if (self->cursor == 0) {
        return 0;
    }
    if (self->v_0x01C() < 1) {
        return 0;
    }
    self->v_0x098(&first, &second);
    if (first != second) {
        return 0;
    }
    switch (self->v_0x09C()) {
    case '.':
    case '!':
    case '?':
        return 1;
    }
    return 0;
}

#pragma push
#pragma auto_inline on
/* 0x8055A180 (0xA4): Switches the input mode, running slot 0xD4 when the mode drops to 0 and slot 0xCC reports 0, and
 * sets the queue's composition flags for modes 1 and 2. */
extern "C" void hbmTextSetInputMode(HbmTextBuffer* self, s32 mode) {
    if (self->inputMode != mode) {
        if (self->v_0x0CC() == 0 && mode == 0) {
            self->v_0x0D4();
        }
        self->inputMode = mode;
        switch (mode) {
        case 1:
            self->queue.mode = 4;
            break;
        case 2:
            self->queue.mode = 8;
            break;
        }
    }
}
#pragma pop

/* 0x8055A224 (0x74): Empties the composition queue and sets its flags for the current input mode. */
extern "C" void hbmTextResetQueue(HbmTextBuffer* self) {
    KPRInitQueue(&self->queue);
    KPRSetQueueMode(&self->queue, 0);
    self->lookahead[0] = 0;
    switch (self->inputMode) {
    case 1:
        self->queue.mode = 4;
        break;
    case 2:
        self->queue.mode = 8;
        break;
    }
}

/* 0x8055A298 (0xA0): Truncates the text and pulls the cursor and the anchor back inside it. */
extern "C" void hbmTextTruncateClamped(HbmTextBuffer* self, u16 length) {
    hbmTextTruncate(self, length);
    if (self->cursor > length) {
        self->anchor = length;
        self->cursor = length;
    }
}

/* Hands the text writer the characters [start, end) of `text`. */
extern "C" void hbmTextWriterSetText(HbmTextWriter* writer, const u16* text, u32 start, u32 end) {
    HbmWidget* self = (HbmWidget*)writer;

    self->arg_50 = (u32)text;
    self->ptr_BC = (void*)start;
    self->ptr_C0 = (void*)end;
}

/* Loads a timer from its two key positions. */
extern "C" void fn_8055A40C(HbmTimer* self, u32 id, u32 kind, f32 a, f32 b, f32 c, f32 d) {
    f32 base = self->base;
    self->id = id;
    self->kind = kind;
    self->a = a;
    self->b = b - base;
    self->start = c;
    self->end = d - base;
    self->active = 1;
}

/* Stores the two callback pointers of the band's observer interface. */
extern "C" void fn_8055A43C(HbmWidget* self, HbmAllocator* allocator, u32 tail) {
    self->allocator = allocator;
    self->tail.word_D0 = tail;
}

/* 0x8055A448 (0x74): Sets the shown line (clamped at 0) and its mode, re-laying the text out when the line changes. */
extern "C" void hbmTextLayoutSetLine(HbmTextLayoutView* self, s32 line, u32 lineMode) {
    if (line < 0) {
        line = 0;
    }
    if (self->line != line) {
        self->v_0x090();
    }
    self->line = line;
    self->lineMode = lineMode;
}

/* A null slot of the band's observer interface. */
extern "C" u32 fn_8055A594(void) {
    return 0;
}

/* Counts one child of the widget. */
extern "C" void fn_8055A870(HbmWidget* self) {
    self->count_A0++;
}

/* Empty virtual slot. */
extern "C" void fn_8055A880(void) {
}

/* Empty virtual slot. */
extern "C" void fn_8055A884(void) {
}

/* Reports the widget's own completion code. */
extern "C" u32 fn_8055A888(HbmWidget* self, int arg) {
    if (self->flag_C8 == 0) {
        return fn_805041F4(self, arg);
    }
    return fn_805041F4(self, 0x2A);
}

/* Empty virtual slot. */
extern "C" void fn_8055AD98(void) {
}

/* Empty virtual slot. */
extern "C" void fn_8055AD9C(void) {
}

/* Empty virtual slot. */
extern "C" void fn_8055B434(void) {
}

/* Empty virtual slot. */
extern "C" void fn_8055B438(void) {
}

/* 0x8055B43C (0x6C): Hands a manager event on as a pane event, then lets the sound player sound the pane. */
/* untyped: caller-owned payload - the argument record of the event */
void HbmPaneHandler::onEvent(u32 id, u32 event, void* data) {
    HbmComponent* component = (HbmComponent*)id;
    onPaneEvent(component, event, (HbmPaneTrigger*)data);
    mpSound->playPaneSound(component->mpPane, event);
}

/* 0x8055B4A8 (0x8C): Returns the component whose pane is called `name`, or NULL. */
HbmKey* HbmPaneManager::getPaneAnim(const char* name) {
    HbmKey* key;
    for (key = (HbmKey*)nw4r::ut::List_GetNext(&mComponents, NULL); key != NULL;
         key = (HbmKey*)nw4r::ut::List_GetNext(&mComponents, key)) {
        if (hbmNameMatches(name, key->mpPane->mName)) {
            return key;
        }
    }
    return NULL;
}

/* 0x8055B534 (0xEC): Returns the component whose first child is a text box holding just the character `key`. */
HbmKey* HbmPaneManager::getKeyAnim(u16 key) {
    HbmKey* component;
    for (component = (HbmKey*)nw4r::ut::List_GetNext(&mComponents, NULL); component != NULL;
         component = (HbmKey*)nw4r::ut::List_GetNext(&mComponents, component)) {
        nw4r::lyt::TextBox* textBox =
            nw4r::ut::DynamicCast<nw4r::lyt::TextBox>(&*component->mpPane->mChildList.GetBeginIter());
        if (textBox != NULL && textBox->mTextBuf[0] == key && textBox->mTextBuf[1] == 0) {
            return component;
        }
    }
    return NULL;
}

/* An element of a Vu object's node list at +0x08: slot 0x0C updates it, slot 0x14 hands it an event.
 * size: 0x04 (approximation: only the compiler-emitted vptr at +0x00 is touched) */
class HbmVuNodeView {
public:
    /* slot +0x008 */ virtual void v_0x008();
    /* slot +0x00C */ virtual void update();
    /* slot +0x010 */ virtual void v_0x010();
    /* slot +0x014 */ virtual void notify(u32 event);
};

/* 0x8055B620 (0x70): Updates every node of the list at +0x08. */
extern "C" void hbmVuNodesUpdate(VuObject* self) {
    HbmVuNodeView* node;
    for (node = (HbmVuNodeView*)nw4r::ut::List_GetNext((nw4r::ut::List*)&self->list_0x08, NULL); node != NULL;
         node = (HbmVuNodeView*)nw4r::ut::List_GetNext((nw4r::ut::List*)&self->list_0x08, node)) {
        node->update();
    }
}

/* 0x8055B690 (0x80): Hands `event` to every node of the list at +0x08. */
extern "C" void hbmVuNodesNotify(VuObject* self, u32 event) {
    HbmVuNodeView* node;
    for (node = (HbmVuNodeView*)nw4r::ut::List_GetNext((nw4r::ut::List*)&self->list_0x08, NULL); node != NULL;
         node = (HbmVuNodeView*)nw4r::ut::List_GetNext((nw4r::ut::List*)&self->list_0x08, node)) {
        node->notify(event);
    }
}

/* 0x8055B7D4 (0xC4): Destroys the layout and the component manager. */
HbmPaneManager::~HbmPaneManager() {
    if (mpLyt != NULL) {
        mpLyt->~Layout();
        MEMFreeToAllocator(mpPaneAllocator, mpLyt);
    }
    if (mpRoot != NULL) {
        mpRoot->~HbmPaneLayout();
        MEMFreeToAllocator(mpPaneAllocator, mpRoot);
    }
}

/* Destroys the keyboard layout. */
HkbLayout::~HkbLayout() {
}

/* 0x8055B8F0 (0x1F4): Builds the layout from its resource, the component manager when there is a handler, and the
 * draw information. */
void HbmPaneManager::create(MEMAllocator* allocator, HbmEventHandler* handler) {
    mpPaneAllocator = allocator;
    mpLyt = new (MEMAllocFromAllocator(allocator, sizeof(HkbLayout))) HkbLayout();
    /* untyped: the RLYT resource image */
    void* resource = mpResAccessor->GetResource(0, mpLayoutName, NULL);
    mpLyt->Build(resource, mpResAccessor);
    if (handler != NULL) {
        static_cast<HbmPaneHandler*>(handler)->setSound(mpSound);
        mpRoot = createRoot(allocator, handler);
        mpRoot->setLayout(mpLyt);
        mpRoot->setMaxComponents(16);
        mpRoot->setDrawInfo(&mDrawInfo);
    }
    mDrawInfo.mViewRect = mpLyt->GetLayoutRect();
    nw4r::math::MTX34 viewMtx;
    PSMTXIdentity(viewMtx.m);
    mDrawInfo.mViewMtx = viewMtx;
    mpLyt->CalculateMtx(mDrawInfo);
    mDrawInfo.mFlag.receiveAlpha = 1;
    mDrawInfo.mFlag.invisiblePaneCalculateMtx = 0;
}


/* Stores the widget's layout-name string. */
extern "C" void fn_8055BAE4(HbmWidget* self, const void* name) {
    self->owner = (HbmWidget*)name;
}

/* Runs the manager's own slot 0x14. */
void HbmPaneManager::init() {
    v_0x014();
}

/* Runs the scene's slot 0x10. */
void HbmPaneManager::v_0x014() {
    mpRoot->v_0x010();
}

/* The widget's collection entry at an index. */
extern "C" void* fn_8055BE48(HbmWidget* self, u32 index) {
    return self->items[index];
}

/* The widget's type tag. */
extern "C" u32 fn_8055BE58(void) {
    return 5;
}

/* 0x8055BE60 (0x7C): Updates every component while the manager takes input. */
void HbmPaneManager::drawComponents() {
    if (mbInputActive) {
        HbmKey* key;
        for (key = (HbmKey*)nw4r::ut::List_GetNext(&mComponents, NULL); key != NULL;
             key = (HbmKey*)nw4r::ut::List_GetNext(&mComponents, key)) {
            key->update();
        }
    }
}

/* Returns the scene's slot 0x30 result. */
u32 HbmPaneManager::updateComponents() {
    return mpRoot->v_0x030();
}

/* Takes no key release: returns 0. */
s32 HbmPaneManager::onKeyRelease(HkbManager* manager) {
    (void)manager;
    return 0;
}

/* 0x8055BEF8 (0xC0): Shows or hides the pane called `name`; a 'P' or 'W' pane takes its 'B' twin along. */
void HbmPaneManager::setPaneVisible(const char* name, s32 visible) {
    nw4r::lyt::Pane* pane = mpLyt->mpRootPane->FindPaneByName(name, true);
    if (pane != NULL) {
        pane->mFlag = (pane->mFlag & (u8)~1) | visible;
        if (name[0] == 'P' || name[0] == 'W') {
            char twin[17];
            hbmNameCopyWithChar(twin, sizeof(twin), name, 0, 'B');
            setPaneVisible(twin, visible);
        }
    }
}

/* 0x8055BFB8 (0x64): Reads whether the pane called `name` is shown; 0 when there is no such pane. */
s32 HbmPaneManager::getPaneVisible(const char* name, u8* visible) {
    nw4r::lyt::Pane* pane = mpLyt->mpRootPane->FindPaneByName(name, true);
    if (pane == NULL) {
        return 0;
    }
    *visible = pane->mFlag & 1;
    return 1;
}

/* 0x8055C01C (0xCC): Sets the text of the text box called `name`. */
void HbmPaneManager::setPaneText(const char* name, const wchar_t* text) {
    nw4r::lyt::TextBox* textBox =
        nw4r::ut::DynamicCast<nw4r::lyt::TextBox>(mpLyt->mpRootPane->FindPaneByName(name, true));
    if (textBox != NULL) {
        textBox->SetString(text, 0);
    }
}

/* Returns the layout's pane called `name`, searching the whole tree. */
nw4r::lyt::TextBox* HbmPaneManager::findTextBox(const char* name) {
    return (nw4r::lyt::TextBox*)mpLyt->mpRootPane->FindPaneByName(name, true);
}

/* 0x8055C104 (0x78): Returns how many frames channel `channel` has held the component of the pane called `name`. */
u32 HbmPaneManager::getHoldFrames(u32 channel, const char* name) {
    return mpRoot->findComponent(findTextBox(name))->getHoldFrames(channel);
}

/* 0x8055C17C (0x58): Draws the pane called `name` with the manager's draw information. */
void HbmPaneManager::drawPane(const char* name) {
    mpLyt->mpRootPane->FindPaneByName(name, true)->Draw(mDrawInfo);
}

/* 0x8055C1D4 (0xF8): Switches to the 16:9 screen: the root pane is stretched and the locations are adjusted back. */
void HbmPaneManager::v_0x03C() {
    mbStandardAspect = 0;
    nw4r::ut::Rect standard;
    hbmGetScreenRect4x3((VuVec4*)&standard);
    nw4r::ut::Rect wide;
    hbmGetScreenRect16x9((VuVec4*)&wide);
    nw4r::math::VEC2 scale;
    scale.x = (wide.right - wide.left) / (standard.right - standard.left);
    scale.y = 1.0f;
    mpLyt->mpRootPane->mScale = scale;
    nw4r::math::VEC2 adjust;
    adjust.x = (standard.right - standard.left) / (wide.right - wide.left);
    adjust.y = 1.0f;
    mDrawInfo.mLocationAdjustScale = adjust;
    mDrawInfo.mFlag.locationAdjust = 1;
    mScreenRect = wide;
}

/* 0x8055C2CC (0x70): Switches to the 4:3 screen: no stretch and no location adjustment. */
void HbmPaneManager::v_0x040() {
    mbStandardAspect = 1;
    hbmGetScreenRect4x3((VuVec4*)&mScreenRect);
    nw4r::math::VEC2 scale;
    scale.x = 1.0f;
    scale.y = 1.0f;
    mpLyt->mpRootPane->mScale = scale;
    nw4r::math::VEC2 adjust;
    adjust.x = 1.0f;
    adjust.y = 1.0f;
    mDrawInfo.mLocationAdjustScale = adjust;
    mDrawInfo.mFlag.locationAdjust = 1;
}

/* 0x8055C33C (0xAC): Loads the orthographic projection of the layout (4:3) or of the 16:9 screen. */
void HbmPaneManager::v_0x044() {
    Mtx44 projection;
    nw4r::ut::Rect layoutRect = mpLyt->GetLayoutRect();
    if (!mbStandardAspect) {
        nw4r::ut::Rect wide;
        hbmGetScreenRect16x9((VuVec4*)&wide);
        C_MTXOrtho(projection, wide.bottom, wide.top, wide.left, wide.right, -1.0f, 1.0f);
    } else {
        C_MTXOrtho(projection, layoutRect.top, layoutRect.bottom, layoutRect.left, layoutRect.right, -1.0f, 1.0f);
    }
    GXSetProjection(projection, 1);
}

/* 0x8055C3E8 (0xAC): Writes `mtx` (behind the view matrix when the draw information asks for it) with its y axis
 * flipped. */
void HbmPaneManager::calcDrawMtx(nw4r::math::MTX34* out, const nw4r::lyt::DrawInfo& drawInfo,
                                 const nw4r::math::MTX34* mtx) {
    nw4r::math::MTX34 flipped;
    if (drawInfo.mFlag.mulViewDraw) {
        PSMTXConcat(drawInfo.mViewMtx.m, mtx->m, flipped.m);
        flipped.m[0][1] = -flipped.m[0][1];
        flipped.m[1][1] = -flipped.m[1][1];
        flipped.m[2][1] = -flipped.m[2][1];
    } else {
        PSMTXCopy(mtx->m, flipped.m);
        flipped.m[0][1] = -flipped.m[0][1];
        flipped.m[1][1] = -flipped.m[1][1];
        flipped.m[2][1] = -flipped.m[2][1];
    }
    PSMTXCopy(flipped.m, out->m);
}

/* Destroys the band key (its animations are released by destroy). */
HbmKey::~HbmKey() {
}

/* 0x8055C4D4 (0xB8): Adds an animation of kind `kind` to the key and binds it to the pane, disabled. */
void HbmKey::addAnim(MEMAllocator* allocator, u32 kind, nw4r::lyt::AnimTransform* anim, bool loop, bool recursive) {
    HbmKeyAnim* entry = (HbmKeyAnim*)MEMAllocFromAllocator(allocator, sizeof(HbmKeyAnim));
    if (entry != NULL) {
        entry->mpAnim = anim;
        entry->kind = kind;
        entry->frame = 0.0f;
        entry->bLoop = loop;
        entry->bRecursive = recursive;
    }
    nw4r::ut::List_Append(&mAnims, entry);
    mpPane->BindAnimation(anim, recursive, false);
    mpPane->SetAnimationEnable(anim, false, recursive);
}

/* 0x8055C58C (0xAC): Adds an animation of kind `kind` bound to the contents named `group`, disabled. */
void HbmKey::addGroupAnim(MEMAllocator* allocator, u32 kind, nw4r::lyt::AnimTransform* anim, const char* group,
                          bool loop, bool recursive) {
    HbmKeyAnim* entry = (HbmKeyAnim*)MEMAllocFromAllocator(allocator, sizeof(HbmKeyAnim));
    if (entry != NULL) {
        entry->mpAnim = anim;
        entry->kind = kind;
        entry->frame = 0.0f;
        entry->bLoop = loop;
        entry->bRecursive = recursive;
    }
    nw4r::ut::List_Append(&mAnims, entry);
    hkbAnimBindNamedPane((HkbAnimTransform*)anim, mpPane, group);
    mpPane->SetAnimationEnable(anim, false, recursive);
}

/* 0x8055C638 (0xFC): Stops the current animation and starts the one of kind `state` from its first frame. */
void HbmKey::setState(u32 state) {
    if (mpCurAnim != NULL) {
        mpPane->SetAnimationEnable(mpCurAnim->mpAnim, false, mpCurAnim->bRecursive);
    }
    mpCurAnim = findAnim(state);
    if (mpCurAnim != NULL) {
        mpCurAnim->frame = 0.0f;
        mpCurAnim->mpAnim->mFrame = 0.0f;
        mpPane->SetAnimationEnable(mpCurAnim->mpAnim, true, mpCurAnim->bRecursive);
        setAnim(3);
        if (mpListener != NULL) {
            mpListener->onAnimEvent(0, this, mpCurAnim);
        }
    }
}

/* 0x8055C734 (0x128): Advances the current animation one frame; at its end it loops or stops and reports it. */
void HbmKey::update() {
    if (mpCurAnim != NULL) {
        mpCurAnim->frame += 1.0f;
        HbmKeyAnim* anim = mpCurAnim;
        if (anim->frame >= anim->mpAnim->GetFrameSize()) {
            if (anim->bLoop) {
                anim->frame = 0.0f;
                mpCurAnim->mpAnim->mFrame = mpCurAnim->frame;
            } else {
                mpPane->SetAnimationEnable(mpCurAnim->mpAnim, false, mpCurAnim->bRecursive);
                if (mpListener != NULL) {
                    mpListener->onAnimEvent(1, this, mpCurAnim);
                }
                mpCurAnim = NULL;
                setAnim(4);
            }
        } else {
            mpCurAnim->mpAnim->mFrame = anim->frame;
        }
    }
}

/* Reads the list node's id and value out, or the empty pair. */
extern "C" void hbmAnimSaveState(HbmWidgetNodeView* self, HbmIdValue* out) {
    if (self->node != 0) {
        out->value = self->node->value;
        out->id = self->node->id;
    } else {
        f32 value = lbl_8079D700;
        out->id = -1;
        out->value = value;
    }
}

/* Stops the key's current animation and restarts the one a saved (kind, frame) pair names. */
static inline void hbmKeyRestoreState(HbmKey* key, const HbmIdValue* state) {
    if (key->mpCurAnim != NULL) {
        key->mpPane->SetAnimationEnable(key->mpCurAnim->mpAnim, false, key->mpCurAnim->bRecursive);
    }
    if (state->id < 0) {
        key->mpCurAnim = NULL;
    } else {
        key->mpCurAnim = key->findAnim(state->id);
    }
    if (key->mpCurAnim != NULL) {
        f32 frame = state->value;
        key->mpCurAnim->frame = frame;
        key->mpCurAnim->mpAnim->mFrame = frame;
        key->mpPane->SetAnimationEnable(key->mpCurAnim->mpAnim, true, key->mpCurAnim->bRecursive);
    }
}

/* 0x8055C894 (0xD4): Stops the current animation and restarts the one a saved (kind, frame) pair names. */
extern "C" void hbmAnimRestoreState(HbmWidgetNodeView* anim, const HbmIdValue* state) {
    hbmKeyRestoreState((HbmKey*)anim, state);
}

/* 0x8055C968 (0x148): Unbinds and frees every animation of the key, then destroys the key; without an allocator
 * both go back through operator delete. */
void HbmKey::destroy(MEMAllocator* allocator) {
    HbmKeyAnim* anim;
    if (allocator != NULL) {
        for (anim = (HbmKeyAnim*)nw4r::ut::List_GetNext(&mAnims, NULL); anim != NULL;
             anim = (HbmKeyAnim*)nw4r::ut::List_GetNext(&mAnims, NULL)) {
            mpPane->UnbindAnimation(anim->mpAnim, true);
            nw4r::ut::List_Remove(&mAnims, anim);
            MEMFreeToAllocator(allocator, anim);
        }
        this->~HbmKey();
        MEMFreeToAllocator(allocator, this);
    } else {
        for (anim = (HbmKeyAnim*)nw4r::ut::List_GetNext(&mAnims, NULL); anim != NULL;
             anim = (HbmKeyAnim*)nw4r::ut::List_GetNext(&mAnims, NULL)) {
            mpPane->UnbindAnimation(anim->mpAnim, true);
            nw4r::ut::List_Remove(&mAnims, anim);
            delete anim;
        }
        delete this;
    }
}

extern "C" {

/* ---------------------------------------------------------------------------------------------------
 * tail-call thunks: the retail body is `...; b <victim>` (the arguments pass through untouched)
 * ------------------------------------------------------------------------------------------------ */
void fn_8055CAB0(VuObject* self, void* node) { nw4r::ut::List_Append((nw4r::ut::List*)&self->list_0x08, node); }

/* ---------------------------------------------------------------------------------------------------
 * no-op virtuals: the retail body is a bare `blr`
 * ------------------------------------------------------------------------------------------------ */
void fn_8055CAB8(VuObject* self) { (void)self; }

void fn_8055CABC(VuObject* self) { (void)self; }

/* Resets nothing. */
void HbmKey::reset() {}

void fn_8055CAC4(VuObject* self) { (void)self; }

void fn_8055CAC8(VuObject* self) { (void)self; }

void fn_8055CACC(VuObject* self) { (void)self; }

void* fn_8055CD20(void* self, s32 flag)
{
    if (self != 0) {
        if (flag > 0)
            operator delete(self);
    }
    return self;
}

/* Runs slot 0xCC on a 16:9 screen and slot 0xD0 otherwise, recording which in +0x3C. */
static inline void hbmVuApplyAspect(VuObject* self)
{
    if (SCGetAspectRatio() == 1) {
        ((HbmVuView*)self)->v_0x0CC();
        self->field_0x3C = 0;
    } else {
        ((HbmVuView*)self)->v_0x0D0();
        self->field_0x3C = 1;
    }
}

/* 0x8055CD60 (0x1F4): Builds the text buffers and the eight parts through the factory slots, creates each part
 * from the allocator, applies the screen's aspect ratio and initialises the object. */
void hbmVuCreateParts(VuObject* self)
{
    HbmVuView* vu = (HbmVuView*)self;

    self->field_0x0C = vu->v_0x0A8();
    hbmTextBuffersInit((HbmTextBufferSet*)(VuHeader*)self->field_0x0C, (MEMAllocator*)self->list_0x04);
    self->field_0x10 = vu->v_0x0AC();
    self->field_0x14 = vu->v_0x0B0();
    ((HkbKbPart3C*)self->field_0x14)->v_0x00C((MEMAllocator*)self->list_0x04);
    self->field_0x18 = vu->v_0x0B4();
    ((HkbKbPart40*)self->field_0x18)->v_0x00C((MEMAllocator*)self->list_0x04);
    self->sub_0x1C = (VuSub*)vu->v_0x0B8();
    ((HkbKbWrapped*)self->sub_0x1C)->v_0x0D4((MEMAllocator*)self->list_0x04, (VuHeader*)self->field_0x0C);
    self->field_0x20 = vu->v_0x0BC();
    ((HkbKbPart48*)self->field_0x20)->v_0x00C((MEMAllocator*)self->list_0x04);
    self->field_0x24 = vu->v_0x0C0();
    ((HkbKbPart4C*)self->field_0x24)->v_0x008((MEMAllocator*)self->list_0x04);
    self->field_0x28 = vu->v_0x0C4();
    ((HkbKbPart50*)self->field_0x28)->v_0x00C((MEMAllocator*)self->list_0x04);
    self->field_0x2C = vu->v_0x0C8();
    ((HkbKbPart44*)self->field_0x2C)->v_0x00C((MEMAllocator*)self->list_0x04);
    hbmVuApplyAspect(self);
    vu->v_0x010();
}

/* 0x8055CF54 (0x74): Runs slot 0xCC on a 16:9 screen and slot 0xD0 otherwise, recording which in +0x3C. */
void hbmVuApplyAspectRatio(VuObject* self)
{
    hbmVuApplyAspect(self);
}

/* 0x8055CFC8 (0x338): Links the parts to each other and to the wrapped keyboard, hands them the layout and the
 * language, initialises each of them and resets the object's state. */
void hbmVuInitParts(VuObject* self)
{
    HkbKbWrapped* wrapped = (HkbKbWrapped*)self->sub_0x1C;

    wrapped->v_0x014();
    ((HbmVuLinkPart*)self->field_0x10)->v_0x014((HkbKbWrapped*)self->sub_0x1C);
    ((HkbKbPart3C*)self->field_0x14)->v_0x014((HkbKbWrapped*)self->sub_0x1C);
    ((HkbKbPart40*)self->field_0x18)->v_0x014((HkbKbWrapped*)self->sub_0x1C);
    ((HkbKbPart48*)self->field_0x20)->v_0x014((HkbKbWrapped*)self->sub_0x1C);
    ((HkbKbPart50*)self->field_0x28)->v_0x014((HkbKbWrapped*)self->sub_0x1C);
    ((HkbKbPart44*)self->field_0x2C)->v_0x014((HkbKbWrapped*)self->sub_0x1C);
    ((HkbKbWrapped*)self->sub_0x1C)->sub_0x118.v_0x010((HkbKbPart48*)self->field_0x20);
    ((HkbKbPart3C*)self->field_0x14)->v_0x118((HkbKbPart50*)self->field_0x28);
    ((HkbKbPart3C*)self->field_0x14)->v_0x11C((HkbKbPart44*)self->field_0x2C);
    ((HkbKbPart40*)self->field_0x18)->v_0x0FC((HkbKbPart50*)self->field_0x28);
    ((HkbKbPart40*)self->field_0x18)->v_0x100((HkbKbPart44*)self->field_0x2C);
    if (self->field_0x3C == 1) {
        ((HbmVuView*)self)->v_0x0D0();
    } else {
        ((HbmVuView*)self)->v_0x0CC();
    }
    ((HkbKbPart50*)self->field_0x28)->v_0x0CC((u32)self->field_0x34);
    ((HkbKbWrapped*)self->sub_0x1C)->v_0x0F8((u32)self->field_0x34);
    ((HkbKbWrapped*)self->sub_0x1C)->v_0x110((u32)self->field_0x38);
    ((HkbKbPart3C*)self->field_0x14)->v_0x028((u32)self->field_0x38);
    ((HkbKbPart40*)self->field_0x18)->v_0x028((u32)self->field_0x38);
    ((HkbKbPart44*)self->field_0x2C)->v_0x028((u32)self->field_0x38);
    ((HkbKbPart48*)self->field_0x20)->v_0x048((u32)self->field_0x38);
    ((HkbKbPart4C*)self->field_0x24)->v_0x018((u32)self->field_0x38);
    ((HkbKbPart50*)self->field_0x28)->v_0x0D0((u32)self->field_0x38);
    hkbKbdInputSetKeyMap((HkbKbdInput*)(HbmVuLinkPart*)self->field_0x10, (u32)self->field_0x34, (u32)self->field_0x38);
    ((HbmVuLinkPart*)self->field_0x10)->v_0x010();
    ((HkbKbPart3C*)self->field_0x14)->v_0x010();
    ((HkbKbPart40*)self->field_0x18)->v_0x010();
    ((HkbKbWrapped*)self->sub_0x1C)->v_0x010();
    ((HkbKbPart48*)self->field_0x20)->v_0x010();
    ((HkbKbPart4C*)self->field_0x24)->v_0x00C();
    ((HkbKbPart50*)self->field_0x28)->v_0x010();
    ((HkbKbPart44*)self->field_0x2C)->v_0x010();
    ((HbmVuView*)self)->v_0x058(0, 0);
    ((HbmVuView*)self)->v_0x050(0);
}

/* The same three-call chain through slot 0x138 instead of 0x13C. */
void fn_8055D300(VuObject* self, u32 a, u32 b)
{
    ((HbmVuView*)self)->v_0x068()->v_0x0E0()->v_0x138(a, b);
}

/* 0x8055D36C (0x10C): Refreshes the parts the current mode shows, the list, the cursor part and the wrapped
 * keyboard, then the shown pop-ups. */
void hbmVuRefreshParts(VuObject* self)
{
    if (((HkbKbPart4C*)self->field_0x24)->v_0x010()) {
        ((HkbKbPart3C*)self->field_0x14)->v_0x114();
    } else {
        ((HkbKbPart40*)self->field_0x18)->v_0x038();
    }
    ((HkbKbPart4C*)self->field_0x24)->child_0x14.v_0x01C();
    ((HkbKbPart48*)self->field_0x20)->v_0x0CC();
    ((HkbKbWrapped*)self->sub_0x1C)->v_0x14C();
    if (((HkbKbPart50*)self->field_0x28)->v_0x0B8()) {
        ((HkbKbPart50*)self->field_0x28)->child_0x1C.v_0x01C();
    }
    if (((HkbKbPart44*)self->field_0x2C)->v_0x0D8()) {
        ((HkbKbPart44*)self->field_0x2C)->child_0x24.v_0x01C();
    }
}

/* 0x8055D478 (0x11C): Sets the z mode to less-or-equal and draws the parts the current mode shows, then the shown
 * pop-ups. */
void hbmVuDrawParts(VuObject* self)
{
    GXSetZMode(1, 3, 1);
    ((HkbKbPart4C*)self->field_0x24)->v_0x098();
    ((HkbKbWrapped*)self->sub_0x1C)->v_0x148();
    if (((HkbKbPart4C*)self->field_0x24)->v_0x010()) {
        ((HkbKbPart3C*)self->field_0x14)->v_0x110();
    } else {
        ((HkbKbPart40*)self->field_0x18)->v_0x03C();
    }
    ((HkbKbPart48*)self->field_0x20)->v_0x024();
    if (((HkbKbPart50*)self->field_0x28)->v_0x0B8()) {
        ((HkbKbPart50*)self->field_0x28)->v_0x0A4();
    }
    if (((HkbKbPart44*)self->field_0x2C)->v_0x0D8()) {
        ((HkbKbPart44*)self->field_0x2C)->v_0x0D0();
    }
}

/* Re-arms the wrapped keyboard and the parts after the mode flag changed. */
static inline void hbmVuRearmParts(VuObject* self)
{
    ((HkbKbWrapped*)((HbmVuView*)self)->v_0x068())->v_0x018(6, 0);
    if (!((HkbKbPart4C*)self->field_0x24)->v_0x010() && ((HkbKbPart40*)self->field_0x18)->v_0x040() == 3) {
        ((HkbKbPart48*)self->field_0x20)->v_0x040(1);
    } else {
        ((HkbKbPart48*)self->field_0x20)->v_0x040(0);
    }
    if (((HkbKbPart4C*)self->field_0x24)->v_0x010()) {
        ((HkbKbPart3C*)self->field_0x14)->v_0x034();
    } else {
        ((HkbKbPart40*)self->field_0x18)->v_0x034();
    }
}

/* 0x8055D594 (0x378): Hands a pointer event to the shown pop-up, or else to the parts the current mode shows, and
 * reports whether any took it; re-arms the parts when the mode flag changed underneath. */
s32 hbmVuDispatchPointer(VuObject* self, s32 key, f32 x, f32 y, s32 arg_b, s32 arg_c, s32 arg_d)
{
    KbArgs args;
    u32 mode = ((HkbKbPart4C*)self->field_0x24)->v_0x010();
    s32 handled_mode;
    s32 handled_list;
    s32 handled_panel;
    s32 handled_wrapped;
    s32 result;

    args.a = key;
    args.x = x;
    args.y = y;
    args.b = arg_b;
    args.c = arg_c;
    args.d = arg_d;
    if (((HkbKbPart50*)self->field_0x28)->v_0x0B8()) {
        return ((HkbKbPart50*)self->field_0x28)->child_0x1C.v_0x020(key, x, y, arg_b, arg_c, arg_d, &args);
    }
    ((HkbKbWrapped*)self->sub_0x1C)->v_0x220(key, arg_b, arg_c, arg_d, &args);
    if (((HkbKbPart44*)self->field_0x2C)->v_0x0D8()) {
        return ((HkbKbPart44*)self->field_0x2C)->v_0x0E0(key, x, y, arg_b, arg_c, arg_d, &args);
    }
    if (mode) {
        handled_mode = ((HkbKbPart3C*)self->field_0x14)->v_0x12C(key, x, y, arg_b, arg_c, arg_d, &args);
    } else {
        handled_mode = ((HkbKbPart40*)self->field_0x18)->v_0x114(key, x, y, arg_b, arg_c, arg_d, &args);
    }
    handled_list = ((HkbKbPart4C*)self->field_0x24)->child_0x14.v_0x020(key, x, y, arg_b, arg_c, arg_d, &args);
    handled_panel = ((HkbKbPart48*)self->field_0x20)->child_0x17F4.v_0x020(key, x, y, arg_b, arg_c, arg_d, &args);
    handled_wrapped = ((HkbKbWrapped*)self->sub_0x1C)->v_0x224(key, x, y, arg_b, arg_c, arg_d, &args);
    result = (handled_list | handled_panel | handled_wrapped | handled_mode) != 0;
    if (mode != ((HkbKbPart4C*)self->field_0x24)->v_0x010()) {
        hbmVuRearmParts(self);
    }
    return result;
}

/* 0x8055D90C (0x26C): Hands an input record to the shown pop-up, or else to the parts the current mode shows, and
 * reports whether any took it; re-arms the parts when the mode flag changed underneath. */
/* untyped: the input record is passed through untouched */
s32 hbmVuDispatchInput(VuObject* self, void* input)
{
    u32 mode = ((HkbKbPart4C*)self->field_0x24)->v_0x010();
    s32 handled_mode;
    s32 handled_list;
    s32 handled_panel;
    s32 handled_wrapped;
    s32 result;

    if (((HkbKbPart50*)self->field_0x28)->v_0x0B8()) {
        return ((HkbKbPart50*)self->field_0x28)->child_0x1C.v_0x024(input);
    }
    if (((HkbKbPart44*)self->field_0x2C)->v_0x0D8()) {
        return ((HkbKbPart44*)self->field_0x2C)->v_0x0E4(input);
    }
    if (mode) {
        handled_mode = ((HkbKbPart3C*)self->field_0x14)->v_0x130(input);
    } else {
        handled_mode = ((HkbKbPart40*)self->field_0x18)->v_0x118(input);
    }
    handled_list = ((HkbKbPart4C*)self->field_0x24)->child_0x14.v_0x024(input);
    handled_panel = ((HkbKbPart48*)self->field_0x20)->child_0x17F4.v_0x024(input);
    handled_wrapped = ((HkbKbWrapped*)self->sub_0x1C)->v_0x228(input);
    result = (handled_list | handled_panel | handled_wrapped | handled_mode) != 0;
    if (mode != ((HkbKbPart4C*)self->field_0x24)->v_0x010()) {
        hbmVuRearmParts(self);
    }
    return result;
}
/* ---------------------------------------------------------------------------------------------------
 * small virtual forwarders: `self->sub->slot(...)` through HbmVuView
 * ------------------------------------------------------------------------------------------------ */
void fn_8055DB78(VuObject* self) { ((HbmVuView*)self->sub_0x1C)->v_0x0DC(); }

void fn_8055DB8C(VuObject* self) { ((HbmVuView*)self->sub_0x1C)->v_0x240(); }

void fn_8055DBA0(VuObject* self) { hbmPanelClearText(self->sub_0x1C); }

void fn_8055DBA8(VuObject* self, void* value) { self->field_0x38 = value; }

void fn_8055DBB0(VuObject* self, void* value) { self->field_0x34 = value; }

void fn_8055DBB8(VuObject* self) { ((HbmVuView*)self->sub_0x1C)->v_0x0FC(); }

void fn_8055DBCC(VuObject* self) { ((HbmVuView*)self->sub_0x1C)->v_0x100(); }

void fn_8055DBE0(VuObject* self) { ((HbmVuView*)self->sub_0x1C)->v_0x104(); }

/* 0x8055DBF4 (0xDC): Lets the pane managers of every part take input, or not. */
void hbmVuSetInputActive(VuObject* self, u8 active)
{
    ((HkbKbPart3C*)self->field_0x14)->child_0x4C.setInputActive(active);
    ((HkbKbPart40*)self->field_0x18)->child_0x34.setInputActive(active);
    ((HkbKbWrapped*)self->sub_0x1C)->child_0x1A04.setInputActive(active);
    ((HkbKbPart48*)self->field_0x20)->child_0x17F4.setInputActive(active);
    ((HkbKbPart4C*)self->field_0x24)->child_0x14.setInputActive(active);
    ((HkbKbPart50*)self->field_0x28)->child_0x1C.setInputActive(active);
    ((HkbKbPart44*)self->field_0x2C)->child_0x24.setInputActive(active);
}

/* A byte-flag setter that dispatches on the new value. */
void fn_8055DCD0(VuObject* self, int flag)
{
    self->field_0x3C = flag;
    if ((u8)flag == 1)
        ((HbmVuView*)self)->v_0x0D0();
    else
        ((HbmVuView*)self)->v_0x0CC();
}

/* 0x8055DD00 (0xCC): Sets the text of the wrapped keyboard's title text box. */
void hbmVuSetTitleText(VuObject* self, const wchar_t* text)
{
    nw4r::lyt::TextBox* textBox = nw4r::ut::DynamicCast<nw4r::lyt::TextBox>(
        ((HkbKbWrapped*)self->sub_0x1C)->child_0x1A04.findTextBox("T_title_text"));
    if (textBox != NULL) {
        textBox->SetString(text, 0);
    }
}

/* 0x8055DDCC (0xB8): Switches every part to the 16:9 screen. */
void hbmVuSetWideScreen(VuObject* self)
{
    ((HkbKbPart3C*)self->field_0x14)->child_0x4C.v_0x03C();
    ((HkbKbPart40*)self->field_0x18)->child_0x34.v_0x03C();
    ((HkbKbWrapped*)self->sub_0x1C)->v_0x234();
    ((HkbKbPart48*)self->field_0x20)->v_0x114();
    ((HkbKbPart4C*)self->field_0x24)->child_0x14.v_0x03C();
    ((HkbKbPart50*)self->field_0x28)->child_0x1C.v_0x03C();
    ((HkbKbPart44*)self->field_0x2C)->child_0x24.v_0x03C();
}

/* 0x8055DE84 (0xB8): Switches every part to the 4:3 screen. */
void hbmVuSetStandardScreen(VuObject* self)
{
    ((HkbKbPart3C*)self->field_0x14)->child_0x4C.v_0x040();
    ((HkbKbPart40*)self->field_0x18)->child_0x34.v_0x040();
    ((HkbKbWrapped*)self->sub_0x1C)->v_0x238();
    ((HkbKbPart48*)self->field_0x20)->v_0x118();
    ((HkbKbPart4C*)self->field_0x24)->child_0x14.v_0x040();
    ((HkbKbPart50*)self->field_0x28)->child_0x1C.v_0x040();
    ((HkbKbPart44*)self->field_0x2C)->child_0x24.v_0x040();
}

/* Two factories: allocate a sub-object from the owner's allocator, then build or construct it. */
VuHeader* fn_8055DF3C(VuOwner* self)
{
    VuHeader* obj = (VuHeader*)MEMAllocFromAllocator((MEMAllocator*)self->allocator, 0x14);

    if (obj != 0) {
        obj->vt = (void*)lbl_8064F0B0;
        obj->field_0x04 = 0;
        obj->field_0x08 = 0;
        obj->field_0x0C = 0;
    }
    return obj;
}

void fn_8055DF88(VuOwner* self)
{
    void* obj = MEMAllocFromAllocator((MEMAllocator*)self->allocator, 0x28);

    if (obj != 0)
        hkbKbdInputConstruct(obj, self);
}

/* A plain deleting destructor with no base-destructor call. */
void* fn_8055E59C(void* self, s32 flag)
{
    if (self != 0) {
        if (flag > 0)
            operator delete(self);
    }
    return self;
}

/* ---------------------------------------------------------------------------------------------------
 * accessors
 * ------------------------------------------------------------------------------------------------ */
void* fn_8055E694(VuObject* self) { return self->field_0x20; }

void* fn_8055E778(VuObject* self) { return self->sub_0x1C; }

void* fn_8055E780(VuObject* self) { return self->field_0x24; }

void* fn_8055E788(VuObject* self) { return self->field_0x18; }

void* fn_8055E790(VuObject* self) { return self->field_0x28; }

void* fn_8055E798(VuObject* self) { return self->field_0x28; }

void* fn_8055E7A0(VuObject* self) { return self->field_0x2C; }

void* fn_8055E7A8(VuObject* self) { return self->field_0x14; }

void* fn_8055E7B0(VuObject* self) { return self->field_0x10; }

void fn_8055E7B8(VuObject* self) { (void)self; }

void fn_8055E7BC(VuObject* self) { (void)self; }

/* Three chained virtual calls: slot 0x68 -> slot 0xE0 -> slot 0x13C(a, b). */
void fn_8055E7C0(VuObject* self, u32 a, u32 b)
{
    ((HbmVuView*)self)->v_0x068()->v_0x0E0()->v_0x13C(a, b);
}

void* fn_8055E82C(VuObject* self) { return self->field_0x38; }

void fn_8055E834(VuObject* self) { (void)self; }

void fn_8055E838(VuObject* self, void* value) { self->field_0x14 = value; }

/* ---------------------------------------------------------------------------------------------------
 * deleting destructors: `if (self && flag > 0) delete self;` returns the object (the compiler's own
 * deleting-destructor shape, dispatching into the class's non-deleting destructor first)
 * ------------------------------------------------------------------------------------------------ */
/* The software keyboard as the band builds it: only its destructor is its own, so its class table
 * (0x806521A8) and the this-adjusting destructor thunk come out of this unit.  size: 0x150 */
class HbmKeyboard : public HkbKeyboard {
public:
    virtual ~HbmKeyboard();
};

/* Destroys the keyboard. */
HbmKeyboard::~HbmKeyboard()
{
}

/* The cellphone keypad as the band builds it: only its destructor is its own, so its class table
 * (0x80652074) and the this-adjusting destructor thunk come out of this unit.  size: 0xEC */
class HbmCellphoneKeypad : public HkbCellphoneKeypad {
public:
    virtual ~HbmCellphoneKeypad();
};

/* Destroys the keypad. */
HbmCellphoneKeypad::~HbmCellphoneKeypad()
{
}

void* fn_8055E8F0(void* self, s32 flag)
{
    if (self != 0) {
        fn_8054F90C(self, 0);
        if (flag > 0)
            operator delete(self);
    }
    return self;
}

void* fn_8055E948(void* self, s32 flag)
{
    if (self != 0) {
        fn_8054CF38(self, 0);
        if (flag > 0)
            operator delete(self);
    }
    return self;
}

/* The HbmKbList as the band builds it: only its destructor is its own, so its class table (0x80651C40) and the
 * this-adjusting destructor thunk come out of this unit. */
class HbmKbToolbar : public HbmKbList {
public:
    virtual ~HbmKbToolbar();
};

/* Destroys the HbmKbList. */
HbmKbToolbar::~HbmKbToolbar()
{
}

/* The HbmKbChild as the band builds it: only its destructor is its own, so its class table (0x80651B40) and the
 * this-adjusting destructor thunk come out of this unit. */
class HbmKbSelector : public HbmKbChild {
public:
    virtual ~HbmKbSelector();
};

/* Destroys the HbmKbChild. */
HbmKbSelector::~HbmKbSelector()
{
}

/* The HbmAnimPanel as the band builds it: only its destructor is its own, so its class table (0x80651A24) and the
 * this-adjusting destructor thunk come out of this unit. */
class HbmKbSignPanel : public HbmAnimPanel {
public:
    virtual ~HbmKbSignPanel();
};

/* Destroys the HbmAnimPanel. */
HbmKbSignPanel::~HbmKbSignPanel()
{
}
/* ---------------------------------------------------------------------------------------------------
 * adjustor thunks: the retail body is `subi r3,r3,N; b <deleting destructor>` (the flag passes through)
 * ------------------------------------------------------------------------------------------------ */

/* 0x8055EAC8 (0x8): Adjusts a base pointer by -0x17F4 and runs the deleting destructor. */
u8* hbmDeletingDtorThunk17F4(u8* base, s32 flag) { return (u8*)fn_8055E8F0(base - 0x17F4, flag); }

/* 0x8055EAD0 (0x8): Adjusts a base pointer by -0x1A04 and runs the deleting destructor. */
u8* hbmDeletingDtorThunk1A04(u8* base, s32 flag) { return (u8*)fn_8055E948(base - 0x1A04, flag); }

}
