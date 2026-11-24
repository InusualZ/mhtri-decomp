/* sound/snd_voice_pool.cpp - the 96-voice pool, its node classes and the list helpers over it
 *
 * RANGE. `.text` 0x800ED780..0x800EE014 (27 functions, all written), extab 0x8000B7E8..0x8000B8BC, extabindex
 *        0x80025320..0x80025404, `.ctors` 0x8056F308 (the TU's `__sinit`), `.data` 0x80598090..0x805980A8 (the two
 *        node vtables), `.bss` 0x8069A8E8..0x806A1110 (`g_snd_voice_pool`).
 * FLAGS. The library's flags (`-O3 -inline noauto -Cpp_exceptions on`); `#pragma peephole off` for the whole file.
 * NAMES. GUESS: the unit (it owns the pool `snd_voice_alloc`/`snd_voice_free` draw voices from), `g_snd_voice_pool`
 *        (the pool `snd_bank_table_get` returns), `SndVoiceLink`/`SndVoiceNode`/`SndVoiceLists`/`SndVoicePool` (the
 *        classes behind the destructor, constructor and vtable shapes below).
 * SHAPES. The three destructors are out-of-line definitions at the top of the file (retail's address order);
 *        `SndVoiceLink`'s constructor is defined `inline` outside its class, which makes `SndVoiceLists`'s constructor
 *        carry the member-destroy unwind action retail has; the pool is a member of nothing and has no declared
 *        destructor, so the `__sinit` only calls its constructor; the global sits after `#pragma dont_inline on`, so
 *        the `__sinit` calls the constructors out of line and MWCC emits them after it, in retail's order.
 *        `SndVoicePool`'s implicit destructor is emitted (weak, unreferenced) before the `__sinit` and the linker
 *        drops it, as it did in retail. `SndVoiceLists`'s destructor is kept by the pool constructor's extab action.
 * RESIDUALS. The object carries the weak `__dt__12SndVoicePoolFv` (0x78 `.text`, 0x24 extab, 0xC extabindex) that the
 *        target lacks; the link drops it. `.bss` is 0x6824 against the 0x6828 range: the next object's 8-byte
 *        alignment fills the gap. The list helpers keep `fn_` names and take `SndNode2`/`SndPool2`, the C views of
 *        these classes in `sound/sound_work.h` that other sound units share; unifying them is a cross-unit change.
 */

#include "types.h"
#include "sound/sound_work.h"
#include "sound/sound_obj.h"
#include "sound/snd_voice_list_end_ck.h"
#include "sound/snd_voice_pool.h"

#pragma peephole off

/* The list link every pool node starts with: its own vtable and the two links (`+0x04` next, `+0x08` prev). */
class SndVoiceLink {
public:
    SndVoiceLink();
    virtual ~SndVoiceLink();

    /* +0x00 */ /* the vtable pointer */
    /* +0x04 */ SndVoiceLink* next;
    /* +0x08 */ SndVoiceLink* prev;
}; /* size: 0xC */

/* One 0x110-byte voice record of the pool (the `SndNode2` view in `sound/sound_work.h`). */
class SndVoiceNode : public SndVoiceLink {
public:
    SndVoiceNode()
    {
        field_0x0C = 0;
        field_0x10 = 0;
        field_0x14 = 255;
    }
    virtual ~SndVoiceNode();

    /* +0x0C */ u32 field_0x0C;
    /* +0x10 */ u32 field_0x10;
    /* +0x14 */ u32 field_0x14;
    /* +0x18 */ u8 pad_0x18[0xF8];
}; /* size: 0x110 */

/* The pool's free and active list sentinels. */
class SndVoiceLists {
public:
    SndVoiceLists() {}
    ~SndVoiceLists();

    /* +0x000 */ SndVoiceNode free_list;
    /* +0x110 */ SndVoiceNode active_list;
}; /* size: 0x220 */

/* The 96-voice pool: both lists, the node storage and the "initialised" flag `snd_voice_alloc` tests. */
class SndVoicePool {
public:
    SndVoicePool() { initialised = 0; }

    /* +0x0000 */ SndVoiceLists lists;
    /* +0x0220 */ SndVoiceNode nodes[96];
    /* +0x6820 */ u8 initialised;
}; /* size: 0x6824 */

/* Makes the link an empty ring. */
inline SndVoiceLink::SndVoiceLink()
{
    next = this;
    prev = this;
}

extern "C" SndNode2* fn_800ED8DC(SndPool2* self);
extern "C" void fn_800ED93C(SndNode2* list, SndNode2* node);
extern "C" SndNode2* fn_800ED954(SndNode2* self);
extern "C" u32 fn_800ED9A0(SndNode2* self);
extern "C" void fn_800ED9B8(SndPool2* self, SndNode2* src, u32 count);
extern "C" void fn_800EDA10(SndNode2* list, SndNode2* src, u32 count);
extern "C" void fn_800EDA7C(SndNode2* self);
extern "C" void* fn_800EDAA8(SndNode2* list, SndNode2* node);
extern "C" void fn_800EDB08(SndNode2* node);
extern "C" SndListNode* fn_800EDB48(SndVoiceMgr* self);

/* The voice-player interface the stream manager and the sound object share (table slots 1 and 2 after the
 * destructor); declared for the virtual calls only, so this unit emits no table. */
class SndVoicePlayer {
public:
    virtual ~SndVoicePlayer();
    virtual void update(SndNode2* node);
    virtual u32 release(SndNode2** link);
    virtual void bankInstall0(SndNode2* node);
    virtual void bankInstall1(SndNode2* node);
    virtual void bankInstall2(SndNode2* node);
    virtual void bankInstall3(SndNode2* node);
}; /* size: 0x4 (the interface view: the table pointer at +0x00) */

/* 0x800ED780 (0x6C): destroys the active list's sentinel, then the free list's. */
SndVoiceLists::~SndVoiceLists() {}

/* 0x800ED7EC (0x5C): destroys one voice record. */
SndVoiceNode::~SndVoiceNode() {}

/* 0x800ED848 (0x44): destroys one list link. */
SndVoiceLink::~SndVoiceLink() {}

/* Initialises the second pool on first use and allocates one node. */
extern "C" void snd_voice_alloc(SndPool2* self)
{
    if (self->initialised == 0) {
        fn_800ED9B8(self, &self->nodes[0], 96);
        self->initialised = 1;
    }
    fn_800ED8DC(self);
}

/* Allocates one 0x110 node: pops the free list and pushes it onto the active list. */
extern "C" SndNode2* fn_800ED8DC(SndPool2* self)
{
    if (fn_800ED9A0(&self->free_list) != 0) {
        return 0;
    }
    {
        SndNode2* node = fn_800ED954(&self->free_list);

        fn_800ED93C(&self->active_list, node);
        return node;
    }
}

/* Inserts `node` at the head of `list` (the 0x110 pool's lists). */
extern "C" void fn_800ED93C(SndNode2* list, SndNode2* node)
{
    SndNode2* first = list->next;

    node->next = first;
    first->prev = node;
    node->prev = list;
    list->next = node;
}

/* Removes and returns the head node, or null when the list is empty. */
extern "C" SndNode2* fn_800ED954(SndNode2* self)
{
    if (fn_800ED9A0(self) == 1) {
        return 0;
    }
    {
        SndNode2* first = self->next;
        SndNode2* second = first->next;

        self->next = second;
        second->prev = self;
        return first;
    }
}

/* Whether the list is empty. */
extern "C" u32 fn_800ED9A0(SndNode2* self)
{
    return self->next == self->prev;
}

/* Empties the active list and fills the free list with `count` nodes from `src`. */
extern "C" void fn_800ED9B8(SndPool2* self, SndNode2* src, u32 count)
{
    fn_800EDA7C(&self->active_list);
    fn_800EDA10(&self->free_list, src, count);
}

/* Inserts `count` consecutive 0x110 nodes from `src` at the head of `list`. */
extern "C" void fn_800EDA10(SndNode2* list, SndNode2* src, u32 count)
{
    SndNode2* node;
    u32 i;

    i = 0;
    node = src;
    while (i < count) {
        fn_800ED93C(list, node);
        node++;
        i++;
    }
}

/* Makes `self` an empty list. */
extern "C" void fn_800EDA7C(SndNode2* self)
{
    self->prev = self;
    self->next = self;
}

/* Resets a voice record and hands it to the state machine. */
extern "C" void snd_voice_free(SndNode2* list, SndNode2* node)
{
    node->field_0x0C = 0;
    node->field_0x10 = 0;
    node->field_0x18 = 255;
    node->field_0x14 = 0;
    node->field_0x19 = 0;
    fn_800EDAA8(list, node);
}

/* Moves `node` to the head of `list` and returns its former successor. */
extern "C" void* fn_800EDAA8(SndNode2* list, SndNode2* node)
{
    SndNode2* next = (SndNode2*)snd_voice_next(node);

    fn_800EDB08(node);
    fn_800ED93C(list, node);
    return next;
}

/* Unlinks `node` from whatever list holds it. */
extern "C" void fn_800EDB08(SndNode2* node)
{
    node->prev->next = node->next;
    node->prev->next->prev = node->prev;
}

/* The first voice of the manager's voice list. */
extern "C" void* snd_voice_first(SndVoiceMgr* self)
{
    return snd_voice_next(fn_800EDB48(self));
}

/* The manager's voice-list sentinel. */
extern "C" SndListNode* fn_800EDB48(SndVoiceMgr* self)
{
    return &self->voice_list;
}

/* The manager's voice list walked through the other list helper. */
extern "C" void* snd_voice_last(SndVoiceMgr* self)
{
    return snd_voice_prev(fn_800EDB48(self));
}

/* Runs one frame over the voice list: the node's player (the stream manager for kind 2, else the sound object)
 * releases it when it is finished, and a flagged live node is updated. */
extern "C" void snd_voice_pool_frame_update(struct StreamWork* table)
{
    SndNode2* node;
    SndVoicePlayer* player;

    for (node = (SndNode2*)snd_voice_first((SndVoiceMgr*)table);
         snd_voice_list_end_ck((SndPool2*)table, node) != 1;) {
        if (node->field_0x19 == 2) {
            player = (SndVoicePlayer*)stream_mgr_get();
        } else {
            player = (SndVoicePlayer*)snd_obj_get();
        }
        if (player->release(&node) == 1) {
            continue;
        }
        if (snd_slot_flag_test(node, 1) != 1) {
            node = (SndNode2*)snd_voice_next(node);
        } else {
            player->update(node);
            node = (SndNode2*)snd_voice_next(node);
        }
    }
}

/* Hands every voice of `kind` to the player's bank-1 slot (the stream manager for kind 2, else the sound
 * object). */
extern "C" void snd_bank1_install(struct StreamWork* table, s32 kind)
{
    SndVoicePlayer* player;
    SndNode2* node;

    if (kind == 2) {
        player = (SndVoicePlayer*)stream_mgr_get();
    } else {
        player = (SndVoicePlayer*)snd_obj_get();
    }
    for (node = (SndNode2*)snd_voice_first((SndVoiceMgr*)table);
         snd_voice_list_end_ck((SndPool2*)table, node) != 1;
         node = (SndNode2*)snd_voice_next(node)) {
        if (node->field_0x19 == kind) {
            player->bankInstall1(node);
        }
    }
}

/* Hands the voices of `kind` whose index byte is `idx` to the player's bank-1 slot. */
extern "C" void snd_bank1_install_idx(struct StreamWork* table, s32 kind, u32 idx)
{
    SndVoicePlayer* player;
    SndNode2* node;

    if (kind == 2) {
        player = (SndVoicePlayer*)stream_mgr_get();
    } else {
        player = (SndVoicePlayer*)snd_obj_get();
    }
    for (node = (SndNode2*)snd_voice_first((SndVoiceMgr*)table);
         snd_voice_list_end_ck((SndPool2*)table, node) != 1;
         node = (SndNode2*)snd_voice_next(node)) {
        if (node->field_0x19 == kind && node->field_0x1A == idx) {
            player->bankInstall1(node);
        }
    }
}

/* Hands every voice of `kind` to the player's bank-2 slot. */
extern "C" void snd_bank2_install(struct StreamWork* table, s32 kind)
{
    SndVoicePlayer* player;
    SndNode2* node;

    if (kind == 2) {
        player = (SndVoicePlayer*)stream_mgr_get();
    } else {
        player = (SndVoicePlayer*)snd_obj_get();
    }
    for (node = (SndNode2*)snd_voice_first((SndVoiceMgr*)table);
         snd_voice_list_end_ck((SndPool2*)table, node) != 1;
         node = (SndNode2*)snd_voice_next(node)) {
        if (node->field_0x19 == kind) {
            player->bankInstall2(node);
        }
    }
}

/* Hands every voice of `kind` to the player's bank-3 slot. */
extern "C" void snd_bank3_install(struct StreamWork* table, s32 kind)
{
    SndVoicePlayer* player;
    SndNode2* node;

    if (kind == 2) {
        player = (SndVoicePlayer*)stream_mgr_get();
    } else {
        player = (SndVoicePlayer*)snd_obj_get();
    }
    for (node = (SndNode2*)snd_voice_first((SndVoiceMgr*)table);
         snd_voice_list_end_ck((SndPool2*)table, node) != 1;
         node = (SndNode2*)snd_voice_next(node)) {
        if (node->field_0x19 == kind) {
            player->bankInstall3(node);
        }
    }
}

/* The pool is built by the TU's `__sinit` (0x800EDF10), which calls the constructors out of line; MWCC then emits
 * them after it: the pool's (0x800EDF1C), the voice record's (0x800EDF74), the link's (0x800EDFC4) and the lists'
 * (0x800EDFDC). */
#pragma dont_inline on

SndVoicePool g_snd_voice_pool;
