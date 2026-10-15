/*
 * homebutton/hbm_value.cpp - software-keyboard code of the tiHKBManager band: the wide-character text buffer (text,
 * cursor, selection anchor, input mode and KPR composition queue), the Vu object forwarders and the deleting destructors.
 *
 * RANGE.     .text 0x80558EB4..0x8055EAF0 (176 functions; the ones not defined below keep the target object's bytes).
 * FLAGS.     GC/3.0a5.2 + cflags_hkb (configure.py); the retail object has no extab/extabindex, so `#pragma exceptions off`.
 * FLAGS.     Scoped `#pragma auto_inline on` around hbmTextTruncate and hbmTextSetInputMode: retail inlines them into hbmTextTruncateClamped and
 *            hbmTextPutInMode (twice) and also keeps their out-of-line copies; -inline noauto inlines nothing on its own.
 * NAMES.     GUESS file name: the band's `hbm_` prefix + the value/text helpers; no `__FILE__` string, no runtime-dump name.
 * NAMES.     GUESS view classes HbmTextBuffer (the text buffer: its slots 0x1C length, 0x20 append, 0x24 insert, 0x28 delete,
 *            0x2C replace, 0x40 set character, 0x5C put character, 0x98 selection range, 0x9C character before the cursor
 *            are read off the bodies that call them), HbmTextLayoutView (vptr at +0x4C, the panel's +0x10 sub-object),
 *            HbmVuView and HbmWidgetView (the dispatch views of VuVTable and HbmWidgetVtable); slots are named by offset.
 * NAMES.     GUESS function names (hbmText*, hbmVuApplyAspectRatio, hbmDeletingDtorThunk*): read off the bodies; map rows
 *            renamed with them (requests hbm-value#1..#41).
 * SHAPES.    The view classes are polymorphic so MWCC emits the canonical `lwz r12,0(r3); lwz r12,<slot>(r12)` dispatch;
 *            no method is defined and nothing is constructed, so no table is emitted here (rule 10).
 * SHAPES.    hbmTextSetCharAt compares `capacity > index` and hbmTextLastChar tests an `s32` index (`subic.`): the other spellings
 *            swap the compare operands or add an `extsh.`.
 * SHAPES.    fn_8055DCD0 takes an `int` it narrows to u8: retail masks the argument (`clrlwi r0,r4,24`) and compares the
 *            masked copy, which a `u8` parameter does not emit.
 * RESIDUALS. fn_8055A888 calls the stale name fn_805041F4 (relocdiff: the target names CharWriter::Print) and returns
 *            u32 where retail passes Print's f32 through; its prototype is in homebutton/hbm_value.h (request
 *            hbm-value#72).
 * RESIDUALS. HbmTextBuffer duplicates the HbmU16Array and HbmTextValue views of homebutton/hbm_widget.h (same object; only
 *            this unit uses them); the merge is a header change outside this unit.
 * RESIDUALS. Unwritten, need a declaration first: fn_80558FAC (wcsncat, not declared in MSL_C/wstring.h), fn_80559638 and
 *            fn_8055A224 (KPRPutChar/KPRGetChar/KPRInitQueue/KPRSetQueueMode, not declared in KPR/kpr.h), fn_80559E74,
 *            fn_80559EB4, fn_80559F10, fn_80559F50, fn_80559FAC, fn_8055A008, fn_8055A048 and fn_8055A3E0 (character
 *            converters of homebutton/hbm_hermite.cpp not declared in its header).
 * RESIDUALS. Unwritten, need the unowned .bss table 0x80790250 (0x580 bytes, read only by fn_8055A338 and fn_8055AE40)
 *            claimed first; its link slot (between hbm_text_panel's and hbm_anim_record's .bss) leaves the owner
 *            ambiguous (request hbm-value#70): fn_8055A338, fn_8055AE40.
 * RESIDUALS. Unwritten, store a vtable whose class this unit would have to emit (rule 10): fn_80558F38, fn_8055CAD0.
 * RESIDUALS. Unwritten: fn_8055EAA8, the unit's static initialiser (its .ctors entry), which stores lbl_8060E748 into the
 *            .sbss word lbl_80795A70.
 * RESIDUALS. 69 functions (17560 bytes) are unwritten in 28 runs, the largest fn_8055A8A0 (1272), fn_8055D594 (888),
 *            fn_8055CFC8 (824) and fn_8055A59C (724); the list is `python tools/units/sweepcomments.py --unit
 *            homebutton/hbm_value`.
 * RESIDUALS. .data 0x80651568..0x80652340 spans at least 10 TUs (datagap.py emission-order seams), so the data cannot match
 *            until the range is re-cut.
 * NAMES.     GUESS classes HbmPaneManager and HbmKey (homebutton/hbm_value.h) and their members; the slots named
 *            v_0xNNN are not understood.
 * NAMES.     GUESS class HbmKeyboard (the keyboard the band builds; its table 0x806521A8 and destructor thunk
 *            0x8055EAB8 are this unit's).
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
#include "MSL_C/wstring.h"
#include "SC/SCApi.h"
#include "KPR/kpr.h"
#include "nw4r/fn_805012C4.h"
#include "nw4r/fn_80502828.h"

extern "C" void wmemset(void* dst, int value, u16 size);

extern "C" void* fn_8055B898(void* self, int flag);

extern f32 lbl_8079D700;

/* ---------------------------------------------------------------------------------------------------
 * the unit's own `.sdata2` float pool and `.sdata` globals: other splits own them, so they are
 * declared, never defined (playbook 29).
 * ------------------------------------------------------------------------------------------------ */

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
    /* slot +0x050 */ virtual void v_0x050();
    /* slot +0x054 */ virtual void v_0x054();
    /* slot +0x058 */ virtual void v_0x058();
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
    /* slot +0x060 */ virtual void v_0x060(u16 ch);
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

/* 0x8055988C (0x144): Puts one character through slot 0x60 under a temporary input mode. */
extern "C" void hbmTextPutInMode(HbmTextBuffer* self, u16 ch, s32 mode) {
    s32 previous = self->inputMode;
    hbmTextSetInputMode(self, mode);
    self->v_0x060(ch);
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

/* 0x8055A298 (0xA0): Truncates the text and pulls the cursor and the anchor back inside it. */
extern "C" void hbmTextTruncateClamped(HbmTextBuffer* self, u16 length) {
    hbmTextTruncate(self, length);
    if (self->cursor > length) {
        self->anchor = length;
        self->cursor = length;
    }
}

/* Stores the callback cluster of the band's observer interface. */
extern "C" void fn_8055A3FC(HbmWidget* self, u32 a, void* b, void* c) {
    self->arg_50 = a;
    self->ptr_BC = b;
    self->ptr_C0 = c;
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

/* Runs the widget's own destructor, then releases its storage. */
extern "C" void* fn_8055B898(void* self, int flag) {
    if (self != 0) {
        fn_8052B5F8(self, 0);
        if (flag > 0) {
            operator delete(self);
        }
    }
    return self;
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

/* Returns the scene's slot 0x30 result. */
u32 HbmPaneManager::updateComponents() {
    return mpRoot->v_0x030();
}

/* Takes no key release: returns 0. */
s32 HbmPaneManager::onKeyRelease(HkbManager* manager) {
    (void)manager;
    return 0;
}

/* Returns the layout's pane called `name`, searching the whole tree. */
nw4r::lyt::TextBox* HbmPaneManager::findTextBox(const char* name) {
    return (nw4r::lyt::TextBox*)mpLyt->mpRootPane->FindPaneByName(name, true);
}

/* Destroys the band key (its animations are released by destroy). */
HbmKey::~HbmKey() {
}

/* Reads the list node's id and value out, or the empty pair. */
extern "C" void fn_8055C85C(HbmWidgetNodeView* self, HbmIdValue* out) {
    if (self->node != 0) {
        out->value = self->node->value;
        out->id = self->node->id;
    } else {
        f32 value = lbl_8079D700;
        out->id = -1;
        out->value = value;
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

/* 0x8055CF54 (0x74): Runs slot 0xCC on a 16:9 screen and slot 0xD0 otherwise, recording which in +0x3C. */
void hbmVuApplyAspectRatio(VuObject* self)
{
    if (SCGetAspectRatio() == 1) {
        ((HbmVuView*)self)->v_0x0CC();
        self->field_0x3C = 0;
    } else {
        ((HbmVuView*)self)->v_0x0D0();
        self->field_0x3C = 1;
    }
}

/* The same three-call chain through slot 0x138 instead of 0x13C. */
void fn_8055D300(VuObject* self, u32 a, u32 b)
{
    ((HbmVuView*)self)->v_0x068()->v_0x0E0()->v_0x138(a, b);
}

/* ---------------------------------------------------------------------------------------------------
 * small virtual forwarders: `self->sub->slot(...)` through HbmVuView
 * ------------------------------------------------------------------------------------------------ */
void fn_8055DB78(VuObject* self) { ((HbmVuView*)self->sub_0x1C)->v_0x0DC(); }

void fn_8055DB8C(VuObject* self) { ((HbmVuView*)self->sub_0x1C)->v_0x240(); }

void fn_8055DBA0(VuObject* self) { fn_80546660(self->sub_0x1C); }

void fn_8055DBA8(VuObject* self, void* value) { self->field_0x38 = value; }

void fn_8055DBB0(VuObject* self, void* value) { self->field_0x34 = value; }

void fn_8055DBB8(VuObject* self) { ((HbmVuView*)self->sub_0x1C)->v_0x0FC(); }

void fn_8055DBCC(VuObject* self) { ((HbmVuView*)self->sub_0x1C)->v_0x100(); }

void fn_8055DBE0(VuObject* self) { ((HbmVuView*)self->sub_0x1C)->v_0x104(); }

/* A byte-flag setter that dispatches on the new value. */
void fn_8055DCD0(VuObject* self, int flag)
{
    self->field_0x3C = flag;
    if ((u8)flag == 1)
        ((HbmVuView*)self)->v_0x0D0();
    else
        ((HbmVuView*)self)->v_0x0CC();
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
        fn_8056D814(obj, self);
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

void* fn_8055E898(void* self, s32 flag)
{
    if (self != 0) {
        fn_8053F65C(self, 0);
        if (flag > 0)
            operator delete(self);
    }
    return self;
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

void* fn_8055E9A0(void* self, s32 flag)
{
    if (self != 0) {
        fn_80554714(self, 0);
        if (flag > 0)
            operator delete(self);
    }
    return self;
}

void* fn_8055E9F8(void* self, s32 flag)
{
    if (self != 0) {
        fn_80555B0C(self, 0);
        if (flag > 0)
            operator delete(self);
    }
    return self;
}

void* fn_8055EA50(void* self, s32 flag)
{
    if (self != 0) {
        fn_80557228(self, 0);
        if (flag > 0)
            operator delete(self);
    }
    return self;
}
/* ---------------------------------------------------------------------------------------------------
 * adjustor thunks: the retail body is `subi r3,r3,N; b <deleting destructor>` (the flag passes through)
 * ------------------------------------------------------------------------------------------------ */

/* 0x8055EAC0 (0x8): Adjusts a base pointer by -0x34 and runs the deleting destructor. */
u8* hbmDeletingDtorThunk34(u8* base, s32 flag) { return (u8*)fn_8055E898(base - 0x34, flag); }

/* 0x8055EAC8 (0x8): Adjusts a base pointer by -0x17F4 and runs the deleting destructor. */
u8* hbmDeletingDtorThunk17F4(u8* base, s32 flag) { return (u8*)fn_8055E8F0(base - 0x17F4, flag); }

/* 0x8055EAD0 (0x8): Adjusts a base pointer by -0x1A04 and runs the deleting destructor. */
u8* hbmDeletingDtorThunk1A04(u8* base, s32 flag) { return (u8*)fn_8055E948(base - 0x1A04, flag); }

/* 0x8055EAD8 (0x8): Adjusts a base pointer by -0x14 and runs the deleting destructor. */
u8* hbmDeletingDtorThunk14(u8* base, s32 flag) { return (u8*)fn_8055E9A0(base - 0x14, flag); }

/* 0x8055EAE0 (0x8): Adjusts a base pointer by -0x1C and runs the deleting destructor. */
u8* hbmDeletingDtorThunk1C(u8* base, s32 flag) { return (u8*)fn_8055E9F8(base - 0x1C, flag); }

/* 0x8055EAE8 (0x8): Adjusts a base pointer by -0x24 and runs the deleting destructor. */
u8* hbmDeletingDtorThunk24(u8* base, s32 flag) { return (u8*)fn_8055EA50(base - 0x24, flag); }
}
