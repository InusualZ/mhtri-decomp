/* sound/snd_stream_reloc.cpp - the per-stream relocation-table state installers and stream flag words
 *
 * `.text` 0x800E7D34..0x800E8E48, 55 functions written (the rest of the range is not decompiled yet).
 * Name is a GUESS: the range holds `fn_800E7E94`..`fn_800E7FBC` (the per-offset installers of the stream records' relocation-table states) and the `.sbss` stream flag words.
 * Each function keeps the `#pragma` state it had in its retired source.
 */

#include "OS/OSDisableInterrupts.h" /* OSDisableInterrupts (rule 2: the owner's header) */
#include "SEQ/seq.h" /* SEQInit, SEQRunAudioFrame (rule 2: the owner's header) */
#include "OS/OSRestoreInterrupts.h" /* OSRestoreInterrupts (rule 2: the owner's header) */
#include "sound/snd_reloc_state_rebase.h" /* snd_reloc_state_rebase (rule 2: the owner's header) */
#include "sound/snd_reverb_state_advance.h" /* snd_reverb_state_advance (rule 2: the owner's header) */
#include "sound/sound_obj.h" /* sound_obj (rule 2: the owner's header) */
#include "sound/snd_voice_pool.h" /* snd_voice_pool (rule 2: the owner's header) */
#include "sound/snd_bank_pending_ck.h" /* snd_bank_pending_ck (rule 2: the owner's header) */
#include "sound/snd_bank_pending_clear.h" /* snd_bank_pending_clear (rule 2: the owner's header) */
#include "sound/snd_bank_pending_set.h" /* snd_bank_pending_set (rule 2: the owner's header) */
#include "sound/snd_bank_table_get.h" /* snd_bank_table_get (rule 2: the owner's header) */
#include "sound/snd_bank_select_pending.h" /* snd_bank_select_pending (rule 2: the owner's header) */
#include "sound/snd_handle_store.h" /* snd_handle_store (rule 2: the owner's header) */
#include "types.h"
#include "Runtime.PPCEABI.H/memset.h" /* memset (rule 2: its owner's header) */
#include "sound/fn_800E46E8_types.h"

/* The global level setting (`fn_800E8690`). */
typedef struct LevelSetting {
    /* +0x00 */ s16 level;
    /* +0x02 */ u16 unused_02;
} LevelSetting; /* size: 0x4 */

extern LevelSetting lbl_80794A04;   /* the global level setting */
extern u8 lbl_80794A08[8];          /* the "state off" flag */
extern u32 lbl_807949E4;            /* reverb work-area size */
extern u32 lbl_807949E8;            /* reverb work-area base */
extern u32 lbl_807949EC;
extern u32 lbl_807949F0;
extern u32 lbl_807949F4;
extern u32 lbl_80794A00;            /* reverb time counter */
extern void* mpMediator__15sNetworkLibrary;
extern u32 lbl_807949F8;
extern u32 lbl_807949FC;
extern u32 frame_counter;
extern u8 lbl_80698AE0[0x10];
extern void* lbl_807949D8;

extern "C" {



u32 snd_handle_get(void);


void __dl__FPv(void* p);


s32 fn_800E885C(void);
s16 fn_800E852C(void);


void fn_800E7E94(void* group, RelocTable* table);
void fn_800E7E9C(void* group, RelocTable* table);
void fn_800E7EA4(void* group, RelocTable* table);
void fn_800E7EBC(void* group, RelocTable* table);
ReverbLine* fn_800E7ED0(Sound* snd, u32 idx);
void fn_800E7F9C(void* group, RelocTable* table);
void fn_800E7FA4(void* group, RelocTable* table);
void fn_800E7FAC(void* group, RelocTable* table);
void fn_800E7FB4(void* group, RelocTable* table);
void fn_800E7FBC(void* group, RelocTable* table);
void fn_800E84DC(Sound* p, u32 v, u8 idx);
void fn_800E84F0(s32 v);
void fn_800E8524(Sound* snd, s16 v);
s16 fn_800E8550(Sound* snd);
void fn_800E8558(u8 v);
void fn_800E858C(Sound* snd, u8 v);
s16 fn_800E85D8(Sound* snd, u32 idx);
LevelSetting* fn_800E8690(void);
void fn_800E87E0(void* alloc, void* free);

s16 fn_800E8880(LevelSetting* setting);
void* getInstance(void);
void setStreamTransferMode(u32 v);
void clearReverbWorkArea(void);
void advanceReverbClock(void);
void fn_800E8E3C(s16* p);

u32 fn_804C2830(void);
void AXFXSetHooks(void* alloc, void* free);



void fn_800E7D34(void);
void fn_800E7DE4(u32 idx, RelocDesc* desc);
void fn_800E7EE0(u32 idx, RelocDesc* desc);
void* fn_800E7FC4(u32 idx, u32 key);
u32 fn_800E802C(u32 key);
void fn_800E8080(u32 key, u16 pitch);
void* fn_800E80DC(u32 a, u32 b, u32 c);
void* fn_800E8150(u32 a, u32 b, u32 c, u32 d);
void* fn_800E81BC(u32 a, u32 b, u32 c, u32 d);
void fn_800E8228(u32 owner);
void fn_800E8294(void);
void fn_800E82D4(void);
void fn_800E8314(void);
void fn_800E8354(u32 owner, s16 arg);
void fn_800E83CC(u32 owner, s16 arg);
s16 fn_800E8444(u32 key);
void fn_800E8498(u32 v, u8 idx);
s16 fn_800E8594(u32 idx);
void fn_800E85E8(u32 idx);

void fn_800E93E0(ReverbLine* line);


void fn_800E8634(u32 idx, u32 entry_idx);
void fn_800E8698(void);
void fn_800E86E8(u32 id);
void pushReverbSamples(s16* src, s32 count);
void fn_800E8DA4(void);
void* fn_800E8DE0(void* obj, s32 flags);


void fn_804DF140(void);
void fn_804C40D0(void);
void AXSetMasterVolume(u16 id);
u32 OSEnableInterrupts(void);
void AXQuit(void* obj);
void fn_804C2800(void);
void __register_global_object(void* obj, void* dtor, void* ref);

void* fn_800E8888(void);
void fn_800E8730(void);
void fn_800E87E4(u32 mode);

void AIInit(u32 mode);
void AXInitEx(u32 v);
void fn_804C26E0(void);
void fn_804DF0A0(void);

void AXRegisterCallback(void* cb);
void AXSetCompressor(u32 v);
void* AIRegisterDMACallback(void* cb);
void AXSetMode(u32 v);
void fn_804C2820(u32 mode);


}


#pragma peephole off

/* Installs a relocation table into one state of the group. */
extern "C" void fn_800E7E94(void* group, RelocTable* table)
{
    snd_reloc_state_rebase(&((RelocGroup*)group)->state[3], table);
}

/* Installs a relocation table into one state of the group. */
extern "C" void fn_800E7E9C(void* group, RelocTable* table)
{
    snd_reloc_state_rebase(&((RelocGroup*)group)->state[2], table);
}

/* Installs a table and its fixed entry array into one state of the group. */
extern "C" void fn_800E7EA4(void* group, RelocTable* table)
{
    RelocState* state = &((RelocGroup*)group)->state[1];

    state->table = table;
    state->entries = (void**)&table->entries_16;
}

/* Installs a table and its fixed entry array into one state of the group. */
extern "C" void fn_800E7EBC(void* group, RelocTable* table)
{
    RelocState* state = &((RelocGroup*)group)->state[0];

    state->table = table;
    state->entries = (void**)&table->entries_16;
}

/* One delay line of the bank. */
extern "C" ReverbLine* fn_800E7ED0(Sound* snd, u32 idx)
{
    return &snd->lines[idx];
}

/* Installs a relocation table into one state of the group. */
extern "C" void fn_800E7F9C(void* group, RelocTable* table)
{
    snd_reloc_state_rebase(&((RelocGroup*)group)->state[8], table);
}

/* Installs a relocation table into one state of the group. */
extern "C" void fn_800E7FA4(void* group, RelocTable* table)
{
    snd_reloc_state_rebase(&((RelocGroup*)group)->state[7], table);
}

/* Installs a relocation table into one state of the group. */
extern "C" void fn_800E7FAC(void* group, RelocTable* table)
{
    snd_reloc_state_rebase(&((RelocGroup*)group)->state[6], table);
}

/* Installs a relocation table into one state of the group. */
extern "C" void fn_800E7FB4(void* group, RelocTable* table)
{
    snd_reloc_state_rebase(&((RelocGroup*)group)->state[5], table);
}

/* Installs a relocation table into one state of the group. */
extern "C" void fn_800E7FBC(void* group, RelocTable* table)
{
    snd_reloc_state_rebase(&((RelocGroup*)group)->state[4], table);
}

/* Stores one level in a sound object's level array. */
extern "C" void fn_800E84DC(Sound* p, u32 v, u8 idx)
{
    p->levels2[idx] = v;
}

/* Sets the global sound object's level word. */
extern "C" void fn_800E84F0(s32 v)
{
    fn_800E8524(fn_800E5714(), (s16)v);
}

/* Stores the sound object's level word. */
extern "C" void fn_800E8524(Sound* snd, s16 v)
{
    snd->field_08 = v;
}

/* The global sound object's level word. */
extern "C" s16 fn_800E852C(void)
{
    return fn_800E8550(fn_800E5714());
}

/* The sound object's level word. */
extern "C" s16 fn_800E8550(Sound* snd)
{
    return snd->field_08;
}

/* Stores the global sound object's mode byte. */
extern "C" void fn_800E8558(u8 v)
{
    fn_800E858C(fn_800E5714(), v);
}

/* Stores the sound object's mode byte. */
extern "C" void fn_800E858C(Sound* snd, u8 v)
{
    snd->field_71 = v;
}

/* One element of the sound object's level table. */
extern "C" s16 fn_800E85D8(Sound* snd, u32 idx)
{
    return snd->levels[idx];
}

/* The global level setting. */
extern "C" LevelSetting* fn_800E8690(void)
{
    return &lbl_80794A04;
}

/* Installs the AX reverb hooks. */
extern "C" void fn_800E87E0(void* alloc, void* free)
{
    AXFXSetHooks(alloc, free);
}

/* The current sound handle. */
extern "C" u32 snd_handle_get(void)
{
    return fn_804C2830();
}

/* The global level setting's level word. */
extern "C" s32 fn_800E885C(void)
{
    return fn_800E8880(fn_800E8690());
}

/* The level word of a setting. */
extern "C" s16 fn_800E8880(LevelSetting* setting)
{
    return setting->level;
}

/* The network library mediator. */
extern "C" void* getInstance(void)
{
    return mpMediator__15sNetworkLibrary;
}

/* Records whether the given state is the "off" one. */
extern "C" void setStreamTransferMode(u32 v)
{
    lbl_80794A08[0] = (u8)(v == 1);
}

/* Clears the reverb work area. */
extern "C" void clearReverbWorkArea(void)
{
    u32 size = 0x8000;
    void* base = (void*)0x90003F60;

    lbl_807949E4 = size;
    lbl_807949E8 = (u32)base;
    lbl_807949EC = 0;
    lbl_807949F0 = 0;
    lbl_807949F4 = size;
    lbl_80794A00 = 0;
    memset(base, 0, size);
}

/* Advances the reverb time counter with interrupts off. */
extern "C" void advanceReverbClock(void)
{
    u32 level = OSDisableInterrupts();

    lbl_80794A00 += 11;
    OSRestoreInterrupts(level);
}

/* Stores the maximum voice volume. */
extern "C" void fn_800E8E3C(s16* p)
{
    *p = 127;
}

/* Snapshots the voice state and installs whichever sound bank is pending. */
extern "C" void fn_800E7D34(void)
{
    fn_800E5F40((Sound2*)fn_800E5714());
    if (snd_bank_pending_ck(fn_800E5714(), 1) != 0) {
        snd_bank1_install(snd_bank_table_get(), 1);
        snd_bank_pending_clear(fn_800E5714(), 1);
        return;
    }
    if (snd_bank_pending_ck(fn_800E5714(), 2) != 0) {
        snd_bank2_install(snd_bank_table_get(), 1);
        snd_bank_pending_clear(fn_800E5714(), 2);
        return;
    }
    if (snd_bank_pending_ck(fn_800E5714(), 4) != 0) {
        snd_bank3_install(snd_bank_table_get(), 1);
        snd_bank_pending_clear(fn_800E5714(), 4);
    }
}

/* Installs one line's four relocation tables. */
extern "C" void fn_800E7DE4(u32 idx, RelocDesc* desc)
{
    RelocTable* t4 = (RelocTable*)(desc->reloc_04 + (u32)desc);
    RelocTable* t8 = (RelocTable*)(desc->reloc_08 + (u32)desc);
    RelocTable* tC = (RelocTable*)(desc->reloc_0C + (u32)desc);

    fn_800E7EBC(fn_800E7ED0(fn_800E5714(), idx), (RelocTable*)(desc->reloc_00 + (u32)desc));
    fn_800E7EA4(fn_800E7ED0(fn_800E5714(), idx), t4);
    fn_800E7E9C(fn_800E7ED0(fn_800E5714(), idx), t8);
    fn_800E7E94(fn_800E7ED0(fn_800E5714(), idx), tC);
    fn_800E93E0(fn_800E7ED0(fn_800E5714(), idx));
}

/* Installs one line's five relocation tables. */
extern "C" void fn_800E7EE0(u32 idx, RelocDesc* desc)
{
    RelocTable* t4 = (RelocTable*)(desc->reloc_04 + (u32)desc);
    RelocTable* t8 = (RelocTable*)(desc->reloc_08 + (u32)desc);
    RelocTable* tC = (RelocTable*)(desc->reloc_0C + (u32)desc);
    RelocTable* t10 = (RelocTable*)(desc->reloc_10 + (u32)desc);

    fn_800E7FBC(fn_800E7ED0(fn_800E5714(), idx), (RelocTable*)(desc->reloc_00 + (u32)desc));
    fn_800E7FB4(fn_800E7ED0(fn_800E5714(), idx), t4);
    fn_800E7FAC(fn_800E7ED0(fn_800E5714(), idx), t8);
    fn_800E7FA4(fn_800E7ED0(fn_800E5714(), idx), tC);
    fn_800E7F9C(fn_800E7ED0(fn_800E5714(), idx), t10);
}

/* One line's slot for a voice id, with interrupts off. */
extern "C" void* fn_800E7FC4(u32 idx, u32 key)
{
    u32 level = OSDisableInterrupts();
    void* slot = fn_800E6A18(fn_800E7ED0(fn_800E5714(), idx), key);

    OSRestoreInterrupts(level);
    return slot;
}

/* How many voices one owner holds, with interrupts off. */
extern "C" u32 fn_800E802C(u32 key)
{
    u32 level = OSDisableInterrupts();
    u32 count = fn_800E7700(fn_800E5714(), key);

    OSRestoreInterrupts(level);
    return count;
}

/* Retunes one owner's voices, with interrupts off. */
extern "C" void fn_800E8080(u32 key, u16 pitch)
{
    u32 level = OSDisableInterrupts();

    fn_800E7990(fn_800E5714(), key, pitch);
    OSRestoreInterrupts(level);
}

/* Plays a sound through the global object, with interrupts off. */
extern "C" void* fn_800E80DC(u32 a, u32 b, u32 c)
{
    u32 level = OSDisableInterrupts();
    void* r = fn_800E6764(fn_800E5714(), a, b, c);

    OSRestoreInterrupts(level);
    return r;
}

/* Plays a sound through the global object, with interrupts off. */
extern "C" void* fn_800E8150(u32 a, u32 b, u32 c, u32 d)
{
    u32 level = OSDisableInterrupts();
    void* r = fn_800E6818(fn_800E5714(), a, b, c, d);

    OSRestoreInterrupts(level);
    return r;
}

/* Plays a sound through the global object, with interrupts off. */
extern "C" void* fn_800E81BC(u32 a, u32 b, u32 c, u32 d)
{
    u32 level = OSDisableInterrupts();
    void* r = fn_800E6A58(fn_800E5714(), a, b, c, d);

    OSRestoreInterrupts(level);
    return r;
}

/* Registers one slot-list entry with no extra data, with interrupts off. */
extern "C" void fn_800E8228(u32 owner)
{
    u32 level = OSDisableInterrupts();

    fn_800E66DC(fn_800E5714(), owner, 0, 0, 0, 0, 0, 0, 0);
    OSRestoreInterrupts(level);
}

/* Marks sound bank 1 pending, with interrupts off. */
extern "C" void fn_800E8294(void)
{
    u32 level = OSDisableInterrupts();

    snd_bank_pending_set(fn_800E5714(), 1);
    OSRestoreInterrupts(level);
}

/* Marks sound bank 2 pending, with interrupts off. */
extern "C" void fn_800E82D4(void)
{
    u32 level = OSDisableInterrupts();

    snd_bank_pending_set(fn_800E5714(), 2);
    OSRestoreInterrupts(level);
}

/* Marks sound bank 4 pending, with interrupts off. */
extern "C" void fn_800E8314(void)
{
    u32 level = OSDisableInterrupts();

    snd_bank_pending_set(fn_800E5714(), 4);
    OSRestoreInterrupts(level);
}

/* Registers a slot-list entry of kind 3, with interrupts off. */
extern "C" void fn_800E8354(u32 owner, s16 arg)
{
    u32 level = OSDisableInterrupts();

    fn_800E66DC(fn_800E5714(), owner, 3, 0, 0, 0, 0, arg, 0);
    OSRestoreInterrupts(level);
}

/* Registers a slot-list entry of kind 4, with interrupts off. */
extern "C" void fn_800E83CC(u32 owner, s16 arg)
{
    u32 level = OSDisableInterrupts();

    fn_800E66DC(fn_800E5714(), owner, 4, 0, 0, 0, 0, 0, arg);
    OSRestoreInterrupts(level);
}

/* The pitch of one owner's first voice, with interrupts off. */
extern "C" s16 fn_800E8444(u32 key)
{
    u32 level = OSDisableInterrupts();
    s16 pitch = fn_800E78F8(fn_800E5714(), key);

    OSRestoreInterrupts(level);
    return pitch;
}

/* Stores one of the global object's level words. */
extern "C" void fn_800E8498(u32 v, u8 idx)
{
    fn_800E84DC(fn_800E5714(), v, idx);
}

/* One of the global object's level words, or 0 for an out-of-range index. */
extern "C" s16 fn_800E8594(u32 idx)
{
    if (idx >= 49) {
        return 0;
    }
    return fn_800E85D8(fn_800E5714(), idx);
}

/* Releases one reverb unit, with interrupts off. */
extern "C" void fn_800E85E8(u32 idx)
{
    u32 level = OSDisableInterrupts();

    snd_reverb_state_advance(fn_800E66D0(), idx);
    OSRestoreInterrupts(level);
}

/* Applies one reverb config from the global object's bank, with interrupts off. */
extern "C" void fn_800E8634(u32 idx, u32 entry_idx)
{
    u32 level = OSDisableInterrupts();

    fn_800E778C(fn_800E5714(), idx, entry_idx);
    OSRestoreInterrupts(level);
}

/* The per-frame sound update. */
extern "C" void fn_800E8698(void)
{
    u32 level = OSEnableInterrupts();

    SEQRunAudioFrame();
    fn_804DF140();
    fn_804C40D0();
    fn_800EDB74(snd_bank_table_get());
    fn_800E7D34();
    snd_bank_select_pending();
    OSRestoreInterrupts(level);
}

/* Sets the global voice count with interrupts off. */
extern "C" void fn_800E86E8(u32 id)
{
    u32 level = OSDisableInterrupts();

    AXSetMasterVolume((u16)id);
    OSRestoreInterrupts(level);
}

/* Copies one buffer into the ring the reverb time counter walks. */
extern "C" void pushReverbSamples(s16* src, s32 count)
{
    u32 pos = lbl_807949EC;
    s16* dst;
    s32 n;

    lbl_807949F8 = pos;
    lbl_807949FC = frame_counter;
    dst = (s16*)(pos + lbl_807949E8);
    n = count / 2;
    while (n-- > 0) {
        *dst++ = *src++;
        pos = lbl_807949EC + 2;
        lbl_807949EC = pos;
        if (pos % lbl_807949F4 == 0) {
            lbl_807949EC = 0;
            dst = (s16*)lbl_807949E8;
        }
    }
}

/* Initialises the global level setting and registers its destructor. */
extern "C" void fn_800E8DA4(void)
{
    fn_800E8E3C(&lbl_80794A04.level);
    __register_global_object(&lbl_80794A04, (void*)fn_800E8DE0, lbl_80698AE0);
}

/* Destroys the global level setting. */
extern "C" void* fn_800E8DE0(void* obj, s32 flags)
{
    if (obj != 0) {
        AXQuit(obj);
        fn_804C2800();
        if ((s16)flags > 0) {
            __dl__FPv(obj);
        }
    }
    return obj;
}

/* Brings up the audio subsystem. */
extern "C" void fn_800E8730(void)
{
    u32 level;

    AIInit(0);
    AXInitEx(1);
    fn_804C26E0();
    fn_804DF0A0();
    SEQInit();
    AXRegisterCallback((void*)fn_800E8698);
    AXSetCompressor(0);
    fn_800E87E4(1);
    level = OSDisableInterrupts();
    lbl_807949D8 = AIRegisterDMACallback((void*)fn_800E8888);
    OSRestoreInterrupts(level);
    /* The same reverb work-area clear `clearReverbWorkArea` performs; retail has the body here too. */
    lbl_807949E4 = 0x8000;
    lbl_807949E8 = 0x90003F60;
    lbl_807949EC = 0;
    lbl_807949F0 = 0;
    lbl_807949F4 = 0x8000;
    lbl_80794A00 = 0;
    memset((void*)0x90003F60, 0, 0x8000);
}

/* Selects the output mode. */
extern "C" void fn_800E87E4(u32 mode)
{
    switch (mode) {
    case 0:
    case 1: AXSetMode(0); break;
    case 2: AXSetMode(1); break;
    case 3: AXSetMode(2); break;
    }
    fn_804C2820(mode);
    snd_handle_store((u32*)fn_800E66D0());
}
