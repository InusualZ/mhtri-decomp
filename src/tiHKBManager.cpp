/*
 * Naming note: the symbol map has only fn_XXXXXXXX for this range (checked with
 * `python tools/symbols/dumpmap.py lookup <addr>` for every fn_8056 address in the range, which answers
 * only a bare `zz_<addr>_` placeholder; the runtime dump carries no real name for any of the 51 symbols).
 *
 * `tiHKBManager.cpp` - a C++ manager.  `.text` 0x8056BBF0-0x8056F2B4 (51 functions, 14020 B) plus the
 * `.ctors` word 0x8056F428-0x8056F42C (dtk assigned it to this unit on the split; it holds fn_8056D78C).
 *
 * Name evidence (docs/plan.md 12, evidence class 1): the unit's own fn_8056BBF0 reaches an OSPanic whose
 * file argument is the bare source name string `"tiHKBManager.cpp"` at .data:0x80658408 (the neighbouring
 * string at 0x8065841C is the panic message `"specified listener not found!"`); the two are referenced
 * nowhere else in the image.  The language is therefore C++ (attribution: `cxx=true`, high confidence,
 * `source-cpp` + the mangled-undefined `__dl__FPv`).  The module directory is not encoded in the file name
 * and no registered sibling exists in the address band, so the file is top-level `src/tiHKBManager.cpp`,
 * beside the other un-moduled game files (`nw_resource.cpp`, `mh3_pad.cpp`), in the `main` lib.
 *
 * Sections claimed: .text 0x8056BBF0-0x8056F2B4 and .ctors 0x8056F428-0x8056F42C.  The unit's `.data`
 * vtables (0x80658440/0x80658458/0x80658480/0x806584C0), the `.rodata` tables (0x8057C760..) and the
 * global manager `lbl_80790D64` all live in OTHER splits, so the target object has only `.text` + `.ctors`
 * and every one of them is referenced here as an extern `lbl_` symbol (rule 10: a table outside our ranges
 * is another TU's - we call through its `lbl_` symbol, never declare the class with virtuals).
 *
 * This is the tail of the game's `.text`: the immediately preceding proposal (0x80569DAC-0x8056BBF0) is the
 * same subsystem and is unclaimed, so the left seam is a guess (brief sec.1).
 *
 * Reconstruction status: this session reconstructed the head of the unit (the listener list unlink, the
 * record/channel accessors and the three observer-notify loops) and measured each against the target
 * object with `recompile.py --measure`.  Of the 51 functions, 5 reached the 80 % bar (fn_8056C148 and
 * fn_8056C5E8 byte-identical, fn_8056BBF0 92.59 %, fn_8056BE2C 87.73 %, fn_8056C108 87.5 %); six more
 * have bodies at 54-74 % (fn_8056BC88 73.98, fn_8056C3D8 71.43, fn_8056BC5C 63.64, fn_8056C090 58.63,
 * fn_8056BEBC/BF58/BFF4 55.77, fn_8056BE84 55.36).  The 37 functions from fn_8056C164 to fn_8056F274 (the
 * channel state machine, the event-bit scanners and the 0x2A8-byte work-item dispatch, including the
 * 3804 B fn_8056DA60) have no body yet.
 *
 * Spelling notes - three bodies were re-measured under a different (typed) spelling of the same logic,
 * because the retail object's own address computation decides the form.  All three improved, none
 * regressed, and the figures are the same `recompile.py --measure` report metric as above:
 *   - fn_8056BC5C reaches the record through the modelled singleton's own record array
 *     (`(&lbl_80790D64.rec0)[rec->index]`) instead of re-deriving it from a cast
 *     (`((HkbRecord*)&lbl_80790D64)[index]`); dropping the two temporaries and the cast is what moves it
 *     from 44.55 to 63.64 %.
 *   - fn_8056BE84 keeps the argument's flag word and value word in locals before the two guards, so the
 *     record index keeps its own register (the retail object loads `lwz r6,4(r4)` and never copies the
 *     argument pointer); 49.29 -> 55.36 %, and the body now reaches the target's 56 B.
 *   - fn_8056C090 indexes the record array by the index the constructor has just written
 *     (`&((HkbRecord*)self)[self->rec0.index]`) rather than by the literal 0; that is what makes MWCC keep
 *     the retail `mulli r4,r6,0x6C; add r4,r3,r4` record-base pair instead of folding it away - 26.73 ->
 *     58.63 % at the target's 120 B.
 *   - fn_8056BD38 hands fn_80529430 the built message word (`*(u32*)&msg`), matching the retail
 *     `lwz r6,8(r1)`; the earlier spelling passed the message's address (`(u32)&msg` -> `addi r6,r1,8`).
 *     The report metric is unchanged for this one row, but the instruction itself is now the target's.
 *
 * Residuals (measured against the target object, symbol by symbol; the per-symbol numbers are in the
 * outbox .pi/outbox/8056bbf0-fn-8056bbf0-8919.json):
 *   - fn_8056C3D8 / fn_8056BE2C / fn_8056C5E8 / fn_8056C108 / fn_8056BC5C / fn_8056C090: sizes exact; the
 *     shortfall is MWCC register colouring and the order the paired loads/`andc` are scheduled, not logic.
 *   - fn_8056BC88 / fn_8056BD38: the interrupt-guarded dispatch path; the target keeps the argument struct
 *     word in r6 and interleaves the small-data materialization with the shift - a source-shape residual.
 *   - fn_8056BEBC/BF58/BFF4: the circular-list notification loops reproduce the target's structure
 *     (walk `prev` to the head, then `next` once around), but the sentinel is coloured r3 where the retail
 *     object uses r0; the three bodies are otherwise identical.  Naming the sentinel twice (a second local
 *     alias for the same node), calling through the vtable slot as an expression, and reordering the loop
 *     were all measured and produce the same object.
 *   - fn_8056C090: the register that holds the record base differs (r6 vs the retail r4); the `li r0,9` is
 *     scheduled next to its store here and early in the retail object.
 *   - the 0x8056C164..0x8056F274 tail has no body (0 %, unpaired).
 */

#include "types.h"

/* The target object carries no extab/extabindex, so this unit was built with exceptions off; the `main`
 * lib's cflags_main turns them on, so the deviation is scoped to this file. */
#pragma exceptions off

void operator delete(void* ptr) throw();

extern "C" void OSPanic(const char* file, int line, const char* msg, ...);
extern "C" u32 OSDisableInterrupts(void);
extern "C" void OSRestoreInterrupts(u32 level);

/* the unit's own data lives in other splits; reference it, never redefine it (rule 10 / rule 2 gap). */
extern "C" char lbl_80658408[]; /* "tiHKBManager.cpp" */
extern "C" char lbl_8065841C[]; /* "specified listener not found!" */
extern "C" void* lbl_80658440[]; /* .data vtable */
extern "C" u8 lbl_80794750[];

/* ---- types -------------------------------------------------------------------------------------- */

/* A node on the manager's circular observer list: vtable, prev, next. */
/* size: 0x0C */
struct HkbNode {
    /* +0x00 */ void** vtable;
    /* +0x04 */ HkbNode* prev;
    /* +0x08 */ HkbNode* next;
};

/*
 * A listener record is 0x6C bytes.  Bytes +0x14..+0x6B are its "channel" sub-object (see HkbChannel);
 * the record head holds the index (+0x00), a flag (+0x01) and the state words (+0x04/+0x08/+0x0C/+0x10).
 */
/* size: 0x58 */
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
};

/* size: 0x6C */
struct HkbRecord {
    /* +0x00 */ u8 index;
    /* +0x01 */ u8 flag;
    /* +0x02 */ u8 pad_02[2];
    /* +0x04 */ u32 w04;
    /* +0x08 */ s32 w08; /* signed: retail compares with cmpwi */
    union {
        /* +0x0C */ HkbRecord* records;
        /* +0x0C */ u8* slotFlags;
    };
    /* +0x10 */ u32 w10;
    /* +0x14 */ HkbChannel channel;
};

/* size: 0x94 */
struct HkbManager {
    /* +0x00 */ HkbRecord rec0; /* the manager's own record */
    /* +0x6C */ u8 pad_6C[0x14];
    /* +0x80 */ HkbNode sentinel; /* circular observer list header */
    /* +0x8C */ HkbManager* owner;
    /* +0x90 */ void* field_90;
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
 * fn_80529430: the slot index, three bytes of padding and a value word.  One shared definition for the
 * two bodies (they built the same anonymous local twice).
 */
/* size: 0x08 */
struct HkbSlotMsg {
    /* +0x00 */ u8 index;
    /* +0x01 */ u8 pad_01[3];
    /* +0x04 */ u32 value;
};

/* the global manager singleton (unsplit - an extern declared where the model lives; rule 2 gap) */
extern "C" HkbManager lbl_80790D64;

/* helper entry points in the neighbouring (unsplit) subsystem TUs. */
extern "C" u32 fn_80529430(u32 index, u32 value, void* callback, u32 arg);
extern "C" void fn_80529B50(u8 index, u32 value);
extern "C" void fn_80529430_dummy(void);

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

/* 0x8056BC5C: mark the record at the manager's slot as pending. */
void fn_8056BC5C(HkbRecord* rec)
{
    if (rec->w08 != 7) {
        return;
    }
    (&lbl_80790D64.rec0)[rec->index].channel.b30 = 1;
}

/* 0x8056BC88: clear the slot's bit, dispatch, and re-set it when the dispatch found the record. */
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
        ((HkbManager*)&lbl_80790D64)->rec0.w08 &= ~bit; /* the global's +0x08 word (record head) */
        result = fn_80529430(index, 0, (void*)fn_8056BC88, *(u32*)&msg);
        if ((s32)result == 7) {
            ((HkbManager*)&lbl_80790D64)->rec0.w08 |= bit;
        }
        OSRestoreInterrupts(saved);
    }
}

/* 0x8056BDD8 helper for fn_8056BD38: second half. */

/* 0x8056BD38: (de)register a slot; the branch depends on the record index. */
void fn_8056BD38(HkbManager* self, HkbArg* arg)
{
    u32 index = arg->index;
    if (index >= 1) {
        u32 saved;
        u32 bit;
        u32 result;
        HkbRecord* records;
        u32 value;
        HkbSlotMsg msg;

        fn_80529B50((u8)index, 0);
        msg.value = 0;
        msg.index = (u8)index;
        records = self->rec0.records;
        saved = OSDisableInterrupts();
        bit = 1u << index;
        value = records->w08 & ~bit;
        records->w08 = value;
        result = fn_80529430((u8)index, 0, (void*)fn_8056BC88, *(u32*)&msg);
        if (result == 7) {
            records->w08 |= bit;
        }
        OSRestoreInterrupts(saved);
    } else {
        HkbRecord* records = self->rec0.records;
        records[index].flag = 1;
        fn_80529B50((u8)index, 0);
        records[index].channel.b30 = 1;
    }
}

/* 0x8056BE2C: reset one record's channel state. */
void fn_8056BE2C(HkbManager* self, HkbArg* arg)
{
    u32 index = arg->index;
    HkbRecord* records;
    HkbRecord* rec;
    if (index >= 1) {
        return;
    }
    records = self->rec0.records;
    rec = &records[index];
    self->rec0.slotFlags[index + 1] = 0;
    rec->channel.w04 = 0;
    rec->channel.w08 = 0;
    rec->channel.w0C = 0;
    rec->channel.w10 = 0;
    rec->channel.w18 = 0;
    rec->channel.w1C = 0;
    rec->channel.w20 &= 0x00000700;
    rec->channel.w28 = 0;
    rec->channel.w24 = 0;
    rec->channel.b30 = 0;
}

/* 0x8056BE84: forward a slot update into the channel. */
void fn_8056C788(HkbChannel* channel, u32 mode, u32 value);

void fn_8056BE84(HkbManager* self, HkbArg* arg)
{
    u32 index = arg->index;
    u32 flags;
    u32 value;
    if (index >= 1) {
        return;
    }
    flags = arg->flags;
    value = arg->value;
    if (flags & 2) {
        return;
    }
    fn_8056C788(&self->rec0.records[index].channel, flags & 1, value);
}

/* 0x8056BEBC / 0x8056BF58 / 0x8056BFF4: notify every observer on the global manager's list by calling
 * the vtable slot +0x0C / +0x10 / +0x14.  Each is a separate body (the retail object has three copies). */
#define HKB_NOTIFY(slot)                                                       \
    HkbManager* mgr = (HkbManager*)&lbl_80790D64;                              \
    HkbNode* sentinel = &mgr->sentinel;                                        \
    HkbNode* first = sentinel;                                                 \
    HkbNode* n = sentinel->prev;                                               \
    while (n != 0) {                                                           \
        first = n;                                                             \
        if (n->prev == sentinel) {                                             \
            break;                                                             \
        }                                                                      \
        n = first->prev;                                                       \
    }                                                                          \
    n = first;                                                                 \
    while (n != 0) {                                                           \
        void (*fn)(HkbNode*, void*) = (void (*)(HkbNode*, void*))n->vtable[slot]; \
        fn(n, arg);                                                            \
        n = n->next;                                                           \
        if (n == first) {                                                      \
            break;                                                             \
        }                                                                      \
    }

void fn_8056BEBC(void* arg)
{
    HKB_NOTIFY(3)
}

void fn_8056BF58(void* arg)
{
    HKB_NOTIFY(4)
}

void fn_8056BFF4(void* arg)
{
    HKB_NOTIFY(5)
}

/* 0x8056C090: constructor of the manager singleton. */
void fn_8056C090(HkbManager* self)
{
    HkbRecord* rec;
    self->rec0.index = 0;
    rec = &((HkbRecord*)self)[self->rec0.index];
    self->rec0.w04 = 0;
    self->rec0.w08 = 0;
    self->rec0.records = 0;
    self->rec0.w10 = 0;
    self->sentinel.prev = 0;
    self->sentinel.next = 0;
    self->sentinel.vtable = lbl_80658440;
    self->owner = self;
    self->field_90 = 0;
    self->rec0.flag = 0;
    rec->channel.w04 = 0;
    rec->channel.w08 = 0;
    rec->channel.w0C = 0;
    rec->channel.w10 = 0;
    rec->channel.w18 = 0;
    rec->channel.w1C = 0;
    rec->channel.w28 = 0;
    rec->channel.w24 = 0;
    rec->channel.b30 = 0;
    rec->channel.b00 = 0;
    rec->channel.w2C = 9;
    rec->channel.w20 = 0;
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

/* 0x8056C3D8: blend the two packed words +0x34 / +0x38 through the mask +0x3C. */
u32 fn_8056C3D8(HkbRecord* rec)
{
    u32 v = rec->channel.w20;
    u32 b = rec->channel.w28;
    v &= ~b;
    v |= rec->channel.w24 & b;
    return v;
}

/* 0x8056C5E8: set the two words +0x38 / +0x3C. */
void fn_8056C5E8(HkbRecord* rec, u32 a, u32 b)
{
    rec->channel.w24 = a;
    rec->channel.w28 = b;
}

} /* extern "C" */
