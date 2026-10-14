/*
 * homebutton/tiHKBManager.cpp - the tiHKBManager software keyboard: the gui component/pane managers, the keyboard
 * manager singleton (its channels, LED requests and observer list) and the channel state machine.
 *
 * RANGE.     .text 0x8056A478..0x8056D814 (79 functions), .ctors 0x8056F428..0x8056F42C, .data 0x80658290..0x80658458,
 *            .bss 0x80790D58..0x80790DF8; the functions not defined below keep the target object's bytes.
 * FLAGS.     GC/3.0a5.2 + cflags_hkb (see src/homebutton/hbm_kb_event.cpp); no extab/extabindex in the target, so
 *            `#pragma exceptions off`.
 * NAMES.     File name from the `__FILE__` string "tiHKBManager.cpp" (lbl_80658408) that fn_8056BBF0's OSPanic cites.
 * NAMES.     GUESS view classes HkbGuiComponent, HkbGuiEventTarget and HkbListener (slots named by offset) and the
 *            types HkbManager, HkbChannel, HkbNode, HkbRecord, HkbArg, HkbSlotMsg, HkbKeyIter, HkbStageEventArgs: named
 *            from the bodies that use them.
 * NAMES.     GUESS function names (map rows renamed): hkbGuiUpdateStageFromArgs (builds a stage event and runs the
 *            stage update), hkbKeyIterSeek (scans a channel key word for the next set bit), hkbKeyIterBeginWord0/1/3
 *            (start a cursor on key word 0/1/3), hkbManagerDelete (the singleton's deleting destructor entry).
 * SHAPES.    The view classes are polymorphic so MWCC emits the canonical `lwz r12,0(r3); lwz r12,N(r12)` dispatch;
 *            nothing is defined or constructed through them, so no table is emitted here.
 * SHAPES.    fn_8056BEBC/BF58/BFF4 compare against `&lbl_80790D64.sentinel` itself: a named sentinel local is
 *            coloured r4 where retail keeps it in r0.
 * SHAPES.    Local declaration order decides the callee-saved colouring (the first declared takes the highest
 *            register): fn_8056A638 declares `idx` before `result`, fn_8056BD38 declares `mgr` first.
 * SHAPES.    fn_8056C090 initialises its one channel in a `u8` counter loop: retail scales the zero counter
 *            (`mulli r4,r6,0x6c`), which an `int` counter or a literal index folds away.
 * RESIDUALS. fn_8056A638: retail schedules `lfs f31,4(r4)` before `mr r28,r4`; every declaration order tried keeps the
 *            move first.
 * RESIDUALS. fn_8056B424: retail loads `*b` into r4 and subtracts `*b - *a` first; `*b != *a` matches the loads but
 *            swaps the two `subf` operands, every other spelling loads `*a` first.
 * RESIDUALS. fn_8056BE2C: retail holds the manager in r0 and the scaled index in r6 (ours r6/r0).
 * RESIDUALS. The unit's own `.data` (the vtables lbl_806582FC/lbl_80658358/lbl_80658440, the strings lbl_80658408/
 *            lbl_8065841C) and `.bss` (lbl_80790D64) are referenced as externs: their classes are not reconstructed.
 * RESIDUALS. datagap: the claimed `.data` 0x80658290..0x80658458 spans at least 3 TUs (emission-order seams at
 *            0x806583A0 and in [0x80658408, 0x80658440)) - a seam question for the registration.
 * RESIDUALS. fn_8056C2A4 (0x70) is not written: it calls KBDSetChannelMap, which KBD/kbd.h does not declare yet.
 * RESIDUALS. 24 functions are unwritten: 0x8056A478-0x8056A638, 0x8056AAC4-0x8056AD7C, 0x8056B23C-0x8056B41C,
 *            0x8056B440-0x8056B6C8, 0x8056B7D8-0x8056B9D4, 0x8056B9DC-0x8056BB7C, 0x8056C164-0x8056C3D8,
 *            0x8056C3F4-0x8056C5E8, 0x8056C5F4-0x8056C6C0, 0x8056C788-0x8056D7D4.
 */

#include "types.h"
#include "sys_mem.h"

/* The retail object has no extab/extabindex: exceptions are off for this band. */
#pragma exceptions off

#include "homebutton/gui_manager.h"
#include "NAND/nand.h"
#include "MEM/mem_allocator.h"
#include "MEM/mem_expheap.h"
#include "KBD/kbd.h"
#include "homebutton/hbm_hermite.h"
#include "nw4r/fn_805012C4.h"

/* The vtables this unit's managers store (the unit's own `.data`; their classes are not reconstructed). */
extern "C" {

extern char lbl_806582FC[];

extern char lbl_80658358[];

}

/* ---- types -------------------------------------------------------------------------------------- */

struct HkbManager;

/* A node on the manager's circular observer list: vtable, prev, next and the manager it reports to (the
 * manager's own sentinel node points back at the manager, fn_8056C090). */
/* size: 0x10 */
struct HkbNode {
    /* +0x00 */ void** vtable;
    /* +0x04 */ HkbNode* prev;
    /* +0x08 */ HkbNode* next;
    /* +0x0C */ HkbManager* manager;
};

/* One keyboard channel of the manager: its state words, the LED request flag (+0x30) and the packed key words
 * (+0x20/+0x24/+0x28) the blend helper reads. */
/* size: 0x6C */
struct HkbChannel {
    /* +0x00 */ u8 b00;
    /* +0x01 */ u8 b01;
    /* +0x02 */ u8 pad_02[2];
    /* +0x04 */ u32 w04;
    /* +0x08 */ u32 w08;
    /* +0x0C */ u32 w0C;
    /* +0x10 */ u32 w10;
    /* +0x14 */ u32 w14;
    /* +0x18 */ u32 w18;
    /* +0x1C */ u32 w1C;
    /* +0x20 */ u32 w20;
    /* +0x24 */ u32 w24;
    /* +0x28 */ u32 w28;
    /* +0x2C */ u32 w2C;
    /* +0x30 */ u8 b30;
    /* +0x31 */ u8 b31[8];
    /* +0x39 */ u8 b39[8];
    /* +0x41 */ u8 b41[8];
    /* +0x49 */ u8 pad_49[3];
    /* +0x4C */ u32 w4C;
    /* +0x50 */ u32 w50;
    /* +0x54 */ u32 w54;
    /* +0x58 */ u8 pad_58[0x14];
};

/* The status record KBDSetLedsAsync hands its callback (fn_8056BC88) and fn_8056BC5C reads: the channel index and
 * the result code (7 = busy).
 * size: 0x0C (approximation: only +0x00 and +0x08 are touched) */
struct HkbRecord {
    /* +0x00 */ u8 index;
    /* +0x01 */ u8 pad_01[3];
    /* +0x04 */ u32 pad_04;
    /* +0x08 */ s32 w08; /* signed: retail compares with cmpwi */
};

/* The keyboard manager singleton: per-channel active flags, the pending-LED bit mask, one channel and the
 * sentinel of the observer list. */
/* size: 0x94 */
struct HkbManager {
    /* +0x00 */ u8 b00;
    /* +0x01 */ u8 active[1];
    /* +0x02 */ u8 pad_02[2];
    /* +0x04 */ u32 w04;
    /* +0x08 */ u32 pendingMask;
    /* +0x0C */ u32 w0C;
    /* +0x10 */ u32 w10;
    /* +0x14 */ HkbChannel channels[1];
    /* +0x80 */ HkbNode sentinel; /* circular observer list header */
    /* +0x90 */ u32 field_90;
};

/* size: 0x08 */
struct HkbArg {
    /* +0x00 */ u8 index;
    /* +0x01 */ u8 value;
    /* +0x02 */ u16 pad_02;
    /* +0x04 */ u32 flags;
};

/*
 * The 8-byte slot message the two slot-dispatch bodies (fn_8056BC88, fn_8056BD38) build and hand to
 * KBDSetLedsAsync: the slot index, three bytes of padding and a value word.  One shared definition for the
 * two bodies (they built the same anonymous local twice).
 */
/* size: 0x08 */
struct HkbSlotMsg {
    /* +0x00 */ u8 index;
    /* +0x01 */ u8 pad_01[3];
    /* +0x04 */ u32 value;
};

/* The polymorphic view of an observer on the manager's list (an HkbNode): slots +0x0C/+0x10/+0x14 receive the
 * notification argument, so MWCC emits the canonical dispatch; nothing is defined here, so no table is emitted.
 * size: 0x04 (approximation: only the compiler-emitted vptr at +0x00 is touched) */
class HkbListener {
public:
    /* slot +0x008 */ virtual void v_0x008();
    /* untyped: opaque handle passed through */
    /* slot +0x00C */ virtual void v_0x00C(void* arg);
    /* untyped: opaque handle passed through */
    /* slot +0x010 */ virtual void v_0x010(void* arg);
    /* untyped: opaque handle passed through */
    /* slot +0x014 */ virtual void v_0x014(void* arg);
};

void operator delete(void* ptr) throw();

/* The unit's own `.data` strings and vtable and its `.bss` singleton (see RESIDUALS). */
extern "C" char lbl_80658408[]; /* "tiHKBManager.cpp" */

extern "C" char lbl_8065841C[]; /* "specified listener not found!" */

extern "C" void* lbl_80658440[]; /* .data vtable */

extern "C" HkbManager lbl_80790D64;

/* The polymorphic view of a gui component the manager bodies call through: the slots are named by their offset
 * in the table (+0x08 is the deleting destructor), so MWCC emits the canonical `lwz r12,0(r3); lwz r12,N(r12)`
 * dispatch. Nothing is defined or constructed here, so no table is emitted (rule 10).
 * size: 0x04 (approximation: only the compiler-emitted vptr at +0x00 is touched) */
class HkbGuiComponent {
public:
    /* slot +0x008 */ virtual void v_0x008(int flag);
    /* untyped: opaque handles passed through */
    /* slot +0x00C */ virtual void v_0x00C(void* a, void* b, void* c);
    /* slot +0x010 */ virtual void v_0x010();
    /* slot +0x014 */ virtual void v_0x014();
    /* slot +0x018 */ virtual void v_0x018();
    /* slot +0x01C */ virtual void v_0x01C();
    /* slot +0x020 */ virtual Pane* v_0x020(); /* the pane the manager keys the component by */
    /* slot +0x024 */ virtual u32 v_0x024(u32 idx);
    /* slot +0x028 */ virtual void v_0x028();
    /* slot +0x02C */ virtual void v_0x02C(u32 idx, u32 on);
    /* slot +0x030 */ virtual void v_0x030(u32 idx);
    /* slot +0x034 */ virtual void v_0x034(u32 idx);
    /* slot +0x038 */ virtual void v_0x038(f32 dx, f32 dy);
    /* slot +0x03C */ virtual void v_0x03C(u32 idx, f32 x, f32 y);
    /* slot +0x040 */ virtual void v_0x040();
    /* untyped: opaque handle passed through */
    /* slot +0x044 */ virtual void v_0x044(void* arg);
    /* slot +0x048 */ virtual void v_0x048();
    /* slot +0x04C */ virtual void v_0x04C();
    /* slot +0x050 */ virtual void v_0x050();
    /* untyped: opaque handle passed through */
    /* slot +0x054 */ virtual void v_0x054(void* arg);
    /* slot +0x058 */ virtual u32 v_0x058(StageEvent* event);
    /* slot +0x05C */ virtual void v_0x05C();
    /* slot +0x060 */ virtual void v_0x060();
    /* slot +0x064 */ virtual u32 v_0x064(f32 x, f32 y);
};

/* The event target a component forwards to (its +0x04 and +0x98 members): slot +0x10 takes the event's third
 * word, slot +0x0C the whole event.
 * size: 0x04 (approximation: only the compiler-emitted vptr at +0x00 is touched) */
class HkbGuiEventTarget {
public:
    /* slot +0x008 */ virtual void v_0x008();
    /* untyped: opaque handles passed through */
    /* slot +0x00C */ virtual void v_0x00C(Component* source, void* b, void* d);
    /* untyped: opaque handle passed through */
    /* slot +0x010 */ virtual void v_0x010(void* c);
};

/* --- 0x8056A638: per-stage update dispatcher - returns 0/1/2/3 by which transition fired. --- */
extern "C" u32 fn_8056A638(Component* self, StageEvent* p) {
    HkbGuiComponent* view = (HkbGuiComponent*)self;
    u32 idx;
    f32 x = p->x;
    u32 result = 0;
    f32 y = p->y;
    idx = p->idx;
    if (view->v_0x058(p) != 0) {
        if (view->v_0x064(x, y) != 0) {
            if (view->v_0x024(idx) != 0) {
                view->v_0x03C(idx, x, y);
                result = 3;
            } else {
                view->v_0x02C(idx, 1);
                view->v_0x030(idx);
                result = 1;
            }
        } else if (view->v_0x024(idx) != 0) {
            view->v_0x02C(idx, 0);
            view->v_0x034(idx);
            result = 2;
        }
        if (self->flags_0x0D[idx] != 0) {
            StageRec* r = &self->recs_0x18[idx];
            view->v_0x038(x - r->x, y - r->y);
            r->x = x;
            r->y = y;
        }
    }
    if (self->flags_0x0D[idx] != 0 && (p->field_0x10 & self->field_0x78) == 0) {
        self->flags_0x0D[idx] = 0;
    }
    return result;
}

/* --- 0x8056A824: constant virtual (always-true predicate). --- */
extern "C" u32 fn_8056A824(void) { return 1; }

/* --- 0x8056A82C: per-index colour-selector byte. --- */
extern "C" u8 fn_8056A82C(ColorBlock* self, u32 idx) { return self->colors_0x05[idx]; }

/* --- 0x8056A838: return the old per-index counter, then increment it. --- */
extern "C" u16 fn_8056A838(ColorBlock* self, u32 idx) {
    u16 v = self->counts_0x80[idx];
    self->counts_0x80[idx] = v + 1;
    return v;
}

/* --- 0x8056A850: store a per-index colour-selector byte. --- */
extern "C" void fn_8056A850(ColorBlock* self, u32 idx, u8 v) { self->colors_0x05[idx] = v; }

/* --- 0x8056A85C / 0x8056A870: clear a per-index counter. --- */
extern "C" void fn_8056A85C(ColorBlock* self, u32 idx) {
    u16* p = &self->counts_0x80[idx];
    *p = 0;
}

extern "C" void fn_8056A870(ColorBlock* self, u32 idx) {
    u16* p = &self->counts_0x80[idx];
    *p = 0;
}

/* --- 0x8056A884: empty virtual. --- */
extern "C" void fn_8056A884(void) {}

/* --- 0x8056A888: Manager deleting destructor - free the component list, then the manager. --- */
extern "C" Manager* fn_8056A888(Manager* self, int flag) {
    if (self != 0) {
        self->vtable = (VTable*)lbl_80658358;
        Node* n = (Node*)nw4r::ut::List_GetNext((nw4r::ut::List*)&self->list_0x0C, 0);
        while (n != 0) {
            nw4r::ut::List_Remove((nw4r::ut::List*)&self->list_0x0C, n);
            if (self->allocator != 0) {
                MEMFreeToAllocator((MEMAllocator*)self->allocator, n);
            } else {
                operator delete(n);
            }
            n = (Node*)nw4r::ut::List_GetNext((nw4r::ut::List*)&self->list_0x0C, 0);
        }
        if (flag > 0) {
            operator delete(self);
        }
    }
    return self;
}

/* --- 0x8056A944: run virtual slot 4 over every component. --- */
extern "C" void fn_8056A944(Manager* self) {
    Node* n = (Node*)nw4r::ut::List_GetNext((nw4r::ut::List*)&self->list_0x0C, 0);
    while (n != 0) {
        HkbGuiComponent* c = (HkbGuiComponent*)n->mValue;
        c->v_0x010();
        n = (Node*)nw4r::ut::List_GetNext((nw4r::ut::List*)&self->list_0x0C, n);
    }
}

/* --- 0x8056A9B4: append `comp` to the manager's list, keyed by its virtual slot 8 result. --- */
extern "C" void fn_8056A9B4(Manager* self, Component* comp) {
    Pane* key = ((HkbGuiComponent*)comp)->v_0x020();
    comp->field_0x94 = (Component*)self;
    if (self->allocator != 0) {
        Node* node = (Node*)MEMAllocFromAllocator((MEMAllocator*)self->allocator, sizeof(Node));
        if (node != 0) {
            node->mKey = key;
            node->mValue = comp;
        }
        nw4r::ut::List_Append((nw4r::ut::List*)&self->list_0x0C, node);
    } else {
        Node* node = (Node*)operator new(sizeof(Node));
        if (node != 0) {
            node->mKey = key;
            node->mValue = comp;
        }
        nw4r::ut::List_Append((nw4r::ut::List*)&self->list_0x0C, node);
    }
}

/* --- 0x8056AA64: component id. --- */
extern "C" u32 fn_8056AA64(Component* self) { return self->field_0x7C; }

/* --- 0x8056AA6C / 0x8056AA98: indexed component lookup (Manager::getComponent). --- */
extern "C" Component* fn_8056AA6C(Manager* self, u32 index) {
    return (Component*)((Node*)nw4r::ut::List_GetNth((nw4r::ut::List*)&self->list_0x0C, (u16)index))->mValue;
}

extern "C" Component* fn_8056AA98(Manager* self, u32 index) {
    return (Component*)((Node*)nw4r::ut::List_GetNth((nw4r::ut::List*)&self->list_0x0C, (u16)index))->mValue;
}

/* The argument block hkbGuiUpdateStageFromArgs builds for the stage update: the event and one trailing word.
 * size: 0x18 */
struct HkbStageEventArgs {
    /* +0x00 */ StageEvent event;
    /* +0x14 */ u32 extra_14;
};

/* --- 0x8056AD7C: build a stage event from the arguments and run the stage update on it. --- */
extern "C" u32 hkbGuiUpdateStageFromArgs(Component* self, u32 idx, f32 x, f32 y, u32 word_0C, u32 mask, u32 extra) {
    HkbStageEventArgs args;
    args.event.idx = idx;
    args.event.x = x;
    args.event.y = y;
    args.event.field_0x0C = word_0C;
    args.event.field_0x10 = mask;
    args.extra_14 = extra;
    return fn_8056A638(self, &args.event);
}

/* --- 0x8056ADB8: forward an event from `source` to the +4 handler and to the source's +0x98 handler. --- */
/* untyped: opaque handles passed through */
extern "C" void fn_8056ADB8(Component* self, Component* source, void* b, void* c, void* d) {
    HkbGuiEventTarget* h = (HkbGuiEventTarget*)self->field_0x04;
    if (h != 0) {
        h->v_0x010(c);
        h = (HkbGuiEventTarget*)self->field_0x04;
        h->v_0x00C(source, b, d);
    }
    h = (HkbGuiEventTarget*)source->field_0x98;
    if (h != 0) {
        h->v_0x00C(source, b, d);
    }
}

/* --- 0x8056AE70: per-component flag byte. --- */
extern "C" u8 fn_8056AE70(Component* self) { return self->field_0x90; }

/* --- 0x8056AE78: latch a stage position and arm its flag when the mask matches. --- */
extern "C" void fn_8056AE78(Component* self, u32 idx, u32 mask, StageRec* v) {
    if ((mask & self->field_0x78) != 0) {
        self->recs_0x18[idx] = *v;
        self->flags_0x0D[idx] = 1;
        self->counts_0x80[idx] = 0;
    }
}

/* --- 0x8056AEC4 / 0x8056AF34: run virtual slot 5 / slot 7 over every component. --- */
extern "C" void fn_8056AEC4(Manager* self) {
    Node* n = (Node*)nw4r::ut::List_GetNext((nw4r::ut::List*)&self->list_0x0C, 0);
    while (n != 0) {
        HkbGuiComponent* c = (HkbGuiComponent*)n->mValue;
        c->v_0x014();
        n = (Node*)nw4r::ut::List_GetNext((nw4r::ut::List*)&self->list_0x0C, n);
    }
}

extern "C" void fn_8056AF34(Manager* self) {
    Node* n = (Node*)nw4r::ut::List_GetNext((nw4r::ut::List*)&self->list_0x0C, 0);
    while (n != 0) {
        HkbGuiComponent* c = (HkbGuiComponent*)n->mValue;
        c->v_0x01C();
        n = (Node*)nw4r::ut::List_GetNext((nw4r::ut::List*)&self->list_0x0C, n);
    }
}

/* --- 0x8056AFA4: forward an argument to virtual slot 21 of every component. --- */
extern "C" void fn_8056AFA4(Manager* self, void* arg) {
    Node* n = (Node*)nw4r::ut::List_GetNext((nw4r::ut::List*)&self->list_0x0C, 0);
    while (n != 0) {
        HkbGuiComponent* c = (HkbGuiComponent*)n->mValue;
        c->v_0x054(arg);
        n = (Node*)nw4r::ut::List_GetNext((nw4r::ut::List*)&self->list_0x0C, n);
    }
}

/* --- 0x8056B024: forward an argument to virtual slot 17 of every component. --- */
extern "C" void fn_8056B024(Manager* self, void* arg) {
    Node* n = (Node*)nw4r::ut::List_GetNext((nw4r::ut::List*)&self->list_0x0C, 0);
    while (n != 0) {
        HkbGuiComponent* c = (HkbGuiComponent*)n->mValue;
        c->v_0x044(arg);
        n = (Node*)nw4r::ut::List_GetNext((nw4r::ut::List*)&self->list_0x0C, n);
    }
}

/* --- 0x8056B0A4: store the trigger mask. --- */
extern "C" void fn_8056B0A4(Component* self, u32 v) { self->field_0x78 = v; }

/* --- 0x8056B0AC: PaneManager deleting destructor - free the pane list, then the base Manager. --- */
extern "C" PaneManager* fn_8056B0AC(PaneManager* self, int flag) {
    if (self != 0) {
        self->vtable = (VTable*)lbl_806582FC;
        Node* n = (Node*)nw4r::ut::List_GetNext((nw4r::ut::List*)&self->list_0x24, 0);
        while (n != 0) {
            nw4r::ut::List_Remove((nw4r::ut::List*)&self->list_0x24, n);
            if (self->allocator != 0) {
                HkbGuiComponent* c = (HkbGuiComponent*)n->mValue;
                c->v_0x008(-1);
                MEMFreeToAllocator((MEMAllocator*)self->allocator, n->mValue);
                MEMFreeToAllocator((MEMAllocator*)self->allocator, n);
            } else {
                HkbGuiComponent* c = (HkbGuiComponent*)n->mValue;
                if (c != 0) {
                    c->v_0x008(1);
                }
                operator delete(n);
            }
            n = (Node*)nw4r::ut::List_GetNext((nw4r::ut::List*)&self->list_0x24, 0);
        }
        fn_8056A888(self, 0);
        if (flag > 0) {
            operator delete(self);
        }
    }
    return self;
}

/* --- 0x8056B1BC / 0x8056B1FC: conditional-delete helpers. --- */
extern "C" void* fn_8056B1BC(void* self, int flag) {
    if (self != 0 && flag > 0) {
        operator delete(self);
    }
    return self;
}

extern "C" void* fn_8056B1FC(void* self, int flag) {
    if (self != 0 && flag > 0) {
        operator delete(self);
    }
    return self;
}

/* --- 0x8056B41C: the component of a list node. --- */
extern "C" void* fn_8056B41C(Node* n) { return n->mValue; }

/* --- 0x8056B424: `*a != *b`. --- */
extern "C" u32 fn_8056B424(u32* a, u32* b) { return *b != *a; }

/* --- 0x8056B6C8: PaneComponent::setPane. --- */
extern "C" void fn_8056B6C8(Component* self, void* pane) { self->field_0x9C = pane; }

/* --- 0x8056B6D0: getPaneComponentByName - find the component whose pane's name matches `name`. --- */
extern "C" Component* fn_8056B6D0(PaneManager* self, const char* name) {
    for (u32 i = 0; i < self->list_0x24.mSize; i++) {
        Node* n = (Node*)nw4r::ut::List_GetNth((nw4r::ut::List*)&self->list_0x24, (u16)i);
        Pane* pane = (Pane*)n->mKey;
        if (fn_8055F580(pane->name_0xBC, name)) {
            return (Component*)n->mValue;
        }
    }
    return 0;
}

/* --- 0x8056B760: PaneManager::getPaneComponentByPane - find the component keyed by `pane`. --- */
extern "C" Component* fn_8056B760(PaneManager* self, void* pane) {
    for (u32 i = 0; i < self->list_0x24.mSize; i++) {
        Node* n = (Node*)nw4r::ut::List_GetNth((nw4r::ut::List*)&self->list_0x24, (u16)i);
        if (n->mKey == pane) {
            return (Component*)n->mValue;
        }
    }
    return 0;
}

/* --- 0x8056B9D4: the manager's +0x18 word. --- */
extern "C" void* fn_8056B9D4(Manager* self) { return self->field_0x18; }

/* --- 0x8056BB7C: every node on the +0xC chain must carry the low visibility bit. --- */
extern "C" u32 fn_8056BB7C(Component* self) {
    VisNode* p = (VisNode*)self->field_0x9C;
    if (p == 0) {
        return 0;
    }
    while (p != 0) {
        if ((p->flags_0xBB & 1) == 0) {
            return 0;
        }
        p = p->next_0x0C;
    }
    return 1;
}

/* --- 0x8056BBBC: tail-call forward to fn_8056A638 (stage update with the point record). --- */
extern "C" u32 fn_8056BBBC(Component* self, StageEvent* p) { return fn_8056A638(self, p); }

/* --- 0x8056BBC0: store the +4 word. --- */
extern "C" void fn_8056BBC0(Manager* self, u32 v) { self->field_0x04 = v; }

/* --- 0x8056BBC8: constant virtual (always-false predicate). --- */
extern "C" u32 fn_8056BBC8(void) { return 0; }

/* --- 0x8056BBD0: append `node` at the tail of the +8/+4 chain rooted at `head`. --- */
extern "C" void fn_8056BBD0(CNode* head, CNode* node) {
    CNode* p = head;
    while (p->next_0x08 != 0) {
        p = p->next_0x08;
    }
    p->next_0x08 = node;
    node->prev_0x04 = p;
}

/* ---- functions ---------------------------------------------------------------------------------- */

extern "C" {

/* 0x8056BBF0: unlink `node` from `list`, panicking if it is not on the list. */
void fn_8056BBF0(HkbNode* list, HkbNode* node)
{
    HkbNode* p = list->next;
    while (p != 0) {
        if (p == node) {
            HkbNode* prev = p->prev;
            HkbNode* next = p->next;
            if (prev != 0) {
                prev->next = next;
            }
            if (next != 0) {
                next->prev = prev;
            }
            node->prev = 0;
            node->next = 0;
            return;
        }
        p = p->next;
    }
    OSPanic(lbl_80658408, 0xB3, lbl_8065841C);
}

/* 0x8056BC5C: mark the channel of a busy LED request as pending. */
void fn_8056BC5C(HkbRecord* rec)
{
    if (rec->w08 != 7) {
        return;
    }
    lbl_80790D64.channels[rec->index].b30 = 1;
}

/* 0x8056BC88: clear the channel's pending bit, re-issue the LED request, and re-set the bit while it stays busy. */
void fn_8056BC88(HkbRecord* rec)
{
    if (rec->w08 != 7) {
        return;
    }
    {
        u8 index = rec->index;
        u32 saved;
        u32 bit;
        u32 result;
        HkbSlotMsg msg;

        msg.value = 0;
        msg.index = index;

        saved = OSDisableInterrupts();
        bit = 1u << index;
        lbl_80790D64.pendingMask &= ~bit;
        result = KBDSetLedsAsync(index, 0, (void*)fn_8056BC88, *(u32*)&msg);
        if ((s32)result == 7) {
            lbl_80790D64.pendingMask |= bit;
        }
        OSRestoreInterrupts(saved);
    }
}

/* 0x8056BD38: (de)activate a channel; channels past the first issue an LED request instead. */
void fn_8056BD38(HkbNode* self, HkbArg* arg)
{
    if (arg->index >= 1) {
        HkbManager* mgr;
        u8 index;
        u32 saved;
        u32 bit;
        u32 result;
        HkbSlotMsg msg;

        KBDSetChannelValue(arg->index, 0);
        index = arg->index;
        msg.value = 0;
        mgr = self->manager;
        msg.index = index;
        saved = OSDisableInterrupts();
        bit = 1u << index;
        mgr->pendingMask &= ~bit;
        result = KBDSetLedsAsync(index, 0, (void*)fn_8056BC88, *(u32*)&msg);
        if ((s32)result == 7) {
            mgr->pendingMask |= bit;
        }
        OSRestoreInterrupts(saved);
    } else {
        self->manager->active[arg->index] = 1;
        KBDSetChannelValue(arg->index, 0);
        self->manager->channels[arg->index].b30 = 1;
    }
}

/* 0x8056BE2C: reset one channel's state. */
void fn_8056BE2C(HkbNode* self, HkbArg* arg)
{
    u32 index = arg->index;
    HkbManager* mgr;
    HkbChannel* ch;
    if (index >= 1) {
        return;
    }
    mgr = self->manager;
    mgr->active[index] = 0;
    ch = &mgr->channels[index];
    ch->w04 = 0;
    ch->w08 = 0;
    ch->w0C = 0;
    ch->w10 = 0;
    ch->w18 = 0;
    ch->w1C = 0;
    ch->w20 &= 0x00000700;
    ch->w28 = 0;
    ch->w24 = 0;
    ch->b30 = 0;
}

/* 0x8056BE84: forward a channel update. */
void fn_8056C788(HkbChannel* channel, u32 mode, u32 value);

void fn_8056BE84(HkbNode* self, HkbArg* arg)
{
    u32 index = arg->index;
    u32 flags;
    if (index >= 1) {
        return;
    }
    flags = arg->flags;
    if (flags & 2) {
        return;
    }
    fn_8056C788(&self->manager->channels[index], flags & 1, arg->value);
}

/* 0x8056BEBC / 0x8056BF58 / 0x8056BFF4: notify every observer on the global manager's list by calling
 * the vtable slot +0x0C / +0x10 / +0x14.  Each is a separate body (the retail object has three copies). */
#define HKB_NOTIFY(method)                                                     \
    HkbNode* first;                                                            \
    HkbNode* n = &lbl_80790D64.sentinel;                                       \
    while (n->prev != 0) {                                                     \
        n = n->prev;                                                           \
        if (n->prev == &lbl_80790D64.sentinel) {                               \
            break;                                                             \
        }                                                                      \
    }                                                                          \
    first = n;                                                                 \
    while (n != 0) {                                                           \
        ((HkbListener*)n)->method(arg);                                        \
        n = n->next;                                                           \
        if (n == first) {                                                      \
            break;                                                             \
        }                                                                      \
    }

void fn_8056BEBC(void* arg)
{
    HKB_NOTIFY(v_0x00C)
}

void fn_8056BF58(void* arg)
{
    HKB_NOTIFY(v_0x010)
}

void fn_8056BFF4(void* arg)
{
    HKB_NOTIFY(v_0x014)
}

/* 0x8056C090: constructor of the manager singleton. */
void fn_8056C090(HkbManager* self)
{
    u8 i;
    self->b00 = 0;
    self->w04 = 0;
    self->pendingMask = 0;
    self->w0C = 0;
    self->w10 = 0;
    self->sentinel.prev = 0;
    self->sentinel.next = 0;
    self->sentinel.vtable = lbl_80658440;
    self->sentinel.manager = self;
    self->field_90 = 0;
    for (i = 0; i < 1; i++) {
        HkbChannel* ch;
        self->active[i] = 0;
        ch = &self->channels[i];
        ch->w04 = 0;
        ch->w08 = 0;
        ch->w0C = 0;
        ch->w10 = 0;
        ch->w18 = 0;
        ch->w1C = 0;
        ch->w28 = 0;
        ch->w24 = 0;
        ch->b30 = 0;
        ch->b00 = 0;
        ch->w2C = 9;
        ch->w20 = 0;
    }
}

/* 0x8056C108: operator delete wrapper. */
void* fn_8056C108(void* ptr, int count)
{
    void* result = ptr;
    if (ptr != 0 && count > 0) {
        operator delete(ptr);
    }
    return result;
}

/* 0x8056C148: atomic load-or-one helper. */
void fn_8056C148(u32 value, u32* out)
{
    if (value == 0) {
        *out = 1;
        return;
    }
    *out = value;
}

/* A cursor over the set key bits of the manager's channels: the word it scans (`kind` 0..3 selects channel
 * word +0x04..+0x10), the bit and channel it stands on and the number of bits it has passed.
 * size: 0x0C */
struct HkbKeyIter {
    /* +0x00 */ HkbManager* manager;
    /* +0x04 */ u8 kind;
    /* +0x05 */ s8 bit;
    /* +0x06 */ u8 channel;
    /* +0x07 */ u8 pad_07;
    /* +0x08 */ u16 count;
    /* +0x0A */ u8 pad_0A[2];
};

HkbKeyIter hkbKeyIterSeek(const HkbKeyIter* start);

/* 0x8056C3D8: blend the two packed words +0x34 / +0x38 through the mask +0x3C. */
u32 fn_8056C3D8(HkbManager* self)
{
    u32 v = self->channels[0].w20;
    u32 b = self->channels[0].w28;
    v &= ~b;
    v |= self->channels[0].w24 & b;
    return v;
}

/* 0x8056C6C0: the first set bit of the manager's key word 0. */
HkbKeyIter hkbKeyIterBeginWord0(HkbManager* manager)
{
    HkbKeyIter it;
    it.manager = manager;
    it.kind = 0;
    it.bit = -1;
    it.channel = 0;
    it.count = 0;
    return hkbKeyIterSeek(&it);
}

/* 0x8056C700: the first set bit of the manager's key word 1. */
HkbKeyIter hkbKeyIterBeginWord1(HkbManager* manager)
{
    HkbKeyIter it;
    it.manager = manager;
    it.kind = 1;
    it.bit = -1;
    it.channel = 0;
    it.count = 0;
    return hkbKeyIterSeek(&it);
}

/* 0x8056C744: the first set bit of the manager's key word 3. */
HkbKeyIter hkbKeyIterBeginWord3(HkbManager* manager)
{
    HkbKeyIter it;
    it.manager = manager;
    it.kind = 3;
    it.bit = -1;
    it.channel = 0;
    it.count = 0;
    return hkbKeyIterSeek(&it);
}

/* 0x8056C5E8: set the two words +0x38 / +0x3C. */
void fn_8056C5E8(HkbManager* self, u32 a, u32 b)
{
    self->channels[0].w24 = a;
    self->channels[0].w28 = b;
}

/* 0x8056D7D4: deleting destructor of the manager singleton (registered by the unit's static initializer). */
HkbManager* hkbManagerDelete(HkbManager* self, int flag)
{
    if (self != 0 && flag > 0) {
        operator delete(self);
    }
    return self;
}
}
