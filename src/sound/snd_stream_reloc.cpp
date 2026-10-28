/* sound/snd_stream_reloc.cpp - the per-stream relocation-table state installers, the audio bring-up, the AI DMA
 * callback and the network voice ring it mixes into the output
 *
 * RANGE. `.text` 0x800E7D34..0x800E8E48, every function written, in address order; `.sbss` 0x807949D8..0x80794A10,
 * `.bss` 0x806987E0..0x80698AF0, `.ctors` 0x8056F2FC..0x8056F300 (MWCC's `__sinit` for `snd_audio_system`).
 * FLAGS. `cflags_main` (Wii/1.3); `#pragma peephole off` for the whole file except `snd_output_mode_set`.
 * NAMES. The unit name is a GUESS (the range opens with the relocation-table state installers). GUESS:
 *   `snd_reloc_state1_set_fixed`/`snd_reloc_state0_set_fixed` (0x800E7EAC/0x800E7EC0, the tail-call targets of
 *   `fn_800E7EA4`/`fn_800E7EBC`; the map rows were split at the `b`), `snd_ai_dma_callback` (registered with
 *   `AIRegisterDMACallback`), `snd_voice_upsample_mix`, `snd_voice_ring_mix` (it drains the ring
 *   `pushReverbSamples` fills and the mediator's `writeVoice` gets the result), `SndAudioSystem`/`snd_audio_system`
 *   (the `__sinit` object whose destructor quits AX and MIX), and the `snd_ai_dma_*` / `snd_voice_ring_*` /
 *   `snd_voice_transfer_on` data words, named from their readers.
 * RESIDUALS. None in `.text`. `.bss` is 0x30C against the claimed 0x310 (the dtor record is 0xC; the 4 bytes are
 *   link padding - a trial `Matching` link hashes to the retail DOL). `snd_voice_ring_mix`'s local block is 0x210
 *   bytes and `snd_voice_transfer_on` is `u8[8]` (APPROXIMATIONS: the 0x220 frame, the 8-byte `.sbss` tail).
 * SHAPES. `snd_output_mode_set` takes a `u16` under `peephole on`: retail's `cmplwi 1` range test with `cmpwi` case
 *   tests is a switch on a promoted u16 whose `clrlwi` the peephole drops. `SndAudioSystem`'s constructor is
 *   `never_inline` (idea 242) so `__sinit` calls it and it follows the destructor. `snd_voice_ring_mix` walks a
 *   `u8*` destination and keeps the read cursor in a register; declaration orders are retail's colouring.
 */

#include "OS/OSInterrupt.h" /* OSDisableInterrupts, OSEnableInterrupts, OSRestoreInterrupts (rule 2: the owner's header) */
#include "SEQ/seq.h" /* SEQInit, SEQRunAudioFrame (rule 2: the owner's header) */
#include "SYN/syn.h" /* SYNInit, SYNRunAudio (rule 2: the owner's header) */
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
#include "MIX/mix.h"
#include "Runtime.PPCEABI.H/memset.h" /* memset (rule 2: its owner's header) */
#include "Runtime.PPCEABI.H/memcpy.h" /* memcpy (rule 2: its owner's header) */
#include "AX/AX.h" /* AXInitEx, AXQuit (rule 2: the owner's header) */
#include "AI_SDK/ai.h" /* AIInit, AIInitDMA, AIGetDMAStartAddr, AIRegisterDMACallback (rule 2: the owner's header) */
#include "OS/DCInvalidateRange.h" /* DCInvalidateRange, DCFlushRange (rule 2: the owner's header) */
#include "Network/NetworkWiiMediator.h" /* NetworkWiiMediator::writeVoice (rule 2: the owner's header) */
#include "sound/fn_800E46E8_types.h"

#pragma peephole off

/* The audio system object: its constructor sets the output level, its destructor shuts AX and MIX down. */
class SndAudioSystem {
public:
    SndAudioSystem() __attribute__((never_inline)) { level = 127; }
    ~SndAudioSystem()
    {
        AXQuit();
        MIXQuit();
    }

    /* +0x00 */ s16 level;
}; /* size: 0x2 */

s16 snd_ai_dma_buffers[2][0xC0] __attribute__((aligned(32))); /* the two output blocks the DMA alternates over */
AIDMACallback snd_ai_dma_prev_callback;     /* the DMA callback `snd_audio_init` replaced, chained first */
u32 snd_ai_dma_prev_block;                  /* the block the last DMA started on (cached address) */
u32 snd_ai_dma_buffer_index;                /* which half of `snd_ai_dma_buffers` is queued next */
u32 snd_voice_ring_size;                    /* voice ring size (0x8000) */
u32 snd_voice_ring_base;                    /* voice ring base (0x90003F60) */
u32 snd_voice_ring_write_pos;               /* the byte offset `pushReverbSamples` writes at */
u32 snd_voice_ring_read_pos;                /* the byte offset `snd_voice_ring_mix` reads at */
u32 snd_voice_ring_capacity;                /* the wrap offset of both cursors */
u32 snd_voice_ring_push_pos;                /* the write offset of the last push */
u32 snd_voice_ring_push_frame;              /* `frame_counter` at the last push */
s32 snd_voice_ring_ticks;                   /* blocks the ring may still deliver (`advanceReverbClock` adds 11) */
SndAudioSystem snd_audio_system;            /* the audio system object */
u8 snd_voice_transfer_on[8];                /* whether the network transfer mode is 1 */

extern void* mpMediator__15sNetworkLibrary;
extern u32 frame_counter;


extern "C" {


u32 snd_handle_get(void);


s32 snd_level_volume_get(void);
s16 fn_800E852C(void);


void fn_800E7E94(void* group, RelocTable* table);
void fn_800E7E9C(void* group, RelocTable* table);
void fn_800E7EA4(void* group, RelocTable* table);
void fn_800E7EBC(void* group, RelocTable* table);
void snd_reloc_state1_set_fixed(RelocState* state, RelocTable* table);
void snd_reloc_state0_set_fixed(RelocState* state, RelocTable* table);
ReverbLine* fn_800E7ED0(Sound* snd, u32 idx);
void fn_800E7F9C(void* group, RelocTable* table);
void fn_800E7FA4(void* group, RelocTable* table);
void fn_800E7FAC(void* group, RelocTable* table);
void fn_800E7FB4(void* group, RelocTable* table);
void fn_800E7FBC(void* group, RelocTable* table);
void fn_800E84DC(Sound* p, u32 v, u8 idx);
void snd_se_level_set(s32 v);
void fn_800E8524(Sound* snd, s16 v);
s16 fn_800E8550(Sound* snd);
void snd_fade_level_set(u8 v);
void fn_800E858C(Sound* snd, u8 v);
s16 fn_800E85D8(Sound* snd, u32 idx);
SndAudioSystem* fn_800E8690(void);
void snd_fx_hooks_set(void* alloc, void* free);

s32 fn_800E8880(SndAudioSystem* audio);
NetworkWiiMediator* getInstance(void);
void setStreamTransferMode(u32 v);
void clearReverbWorkArea(void);
void advanceReverbClock(void);

void AXFXSetHooks(void* alloc, void* free);


void fn_800E7D34(void);
void snd_whd_install(u32 idx, RelocDesc* desc);
void snd_tsb_install(u32 idx, RelocDesc* desc);
void* fn_800E7FC4(u32 idx, u32 key);
u32 snd_stream_reloc_busy_ck(u32 key);
void fn_800E8080(u32 key, u16 pitch);
void* fn_800E80DC(u32 a, u32 b, u32 c);
void* fn_800E8150(u32 a, u32 b, u32 c, u32 d);
void* fn_800E81BC(u32 a, u32 b, u32 c, u32 d);
void snd_stream_reloc_stop(u32 owner);
void snd_stream_bank_reset(void);
void snd_stream_reloc_pause(void);
void snd_stream_reloc_resume(void);
void fn_800E8354(u32 owner, s16 arg);
void fn_800E83CC(u32 owner, s16 arg);
s16 fn_800E8444(u32 key);
void fn_800E8498(u32 v, u8 idx);
s16 fn_800E8594(u32 idx);
void snd_reverb_advance(u32 idx);

void snd_wave_bank_relocate(ReverbLine* line);


void snd_reverb_apply(u32 idx, u32 entry_idx);
void fn_800E8698(void);
void snd_master_volume_set(u32 id);
void pushReverbSamples(s16* src, s32 count);


void AXSetMasterVolume(u16 id);

void snd_ai_dma_callback(void);
s32 snd_voice_upsample_mix(s16* dst, s32 dst_size, const s16* src, s32 src_size);
s32 snd_voice_ring_mix(s16* dst);
void snd_audio_init(void);
void snd_output_mode_set(u16 mode);


void AXRegisterCallback(void* cb);
void AXSetCompressor(u32 v);
void AXSetMode(u32 v);


}


/* Snapshots the voice state and installs whichever sound bank is pending. */
extern "C" void fn_800E7D34(void)
{
    snd_cmd_list_run((Sound2*)snd_obj_get());
    if (snd_bank_pending_ck(snd_obj_get(), 1) != 0) {
        snd_bank1_install(snd_bank_table_get(), 1);
        snd_bank_pending_clear(snd_obj_get(), 1);
        return;
    }
    if (snd_bank_pending_ck(snd_obj_get(), 2) != 0) {
        snd_bank2_install(snd_bank_table_get(), 1);
        snd_bank_pending_clear(snd_obj_get(), 2);
        return;
    }
    if (snd_bank_pending_ck(snd_obj_get(), 4) != 0) {
        snd_bank3_install(snd_bank_table_get(), 1);
        snd_bank_pending_clear(snd_obj_get(), 4);
    }
}

/* Installs one line's four relocation tables. */
extern "C" void snd_whd_install(u32 idx, RelocDesc* desc)
{
    RelocTable* t4 = (RelocTable*)(desc->reloc_04 + (u32)desc);
    RelocTable* t8 = (RelocTable*)(desc->reloc_08 + (u32)desc);
    RelocTable* tC = (RelocTable*)(desc->reloc_0C + (u32)desc);

    fn_800E7EBC(fn_800E7ED0(snd_obj_get(), idx), (RelocTable*)(desc->reloc_00 + (u32)desc));
    fn_800E7EA4(fn_800E7ED0(snd_obj_get(), idx), t4);
    fn_800E7E9C(fn_800E7ED0(snd_obj_get(), idx), t8);
    fn_800E7E94(fn_800E7ED0(snd_obj_get(), idx), tC);
    snd_wave_bank_relocate(fn_800E7ED0(snd_obj_get(), idx));
}

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
    snd_reloc_state1_set_fixed(&((RelocGroup*)group)->state[1], table);
}

/* Points a state at a table and the table's fixed entry array. */
extern "C" void snd_reloc_state1_set_fixed(RelocState* state, RelocTable* table)
{
    state->table = table;
    state->entries = (void**)&table->entries_16;
}

/* Installs a table and its fixed entry array into one state of the group. */
extern "C" void fn_800E7EBC(void* group, RelocTable* table)
{
    snd_reloc_state0_set_fixed(&((RelocGroup*)group)->state[0], table);
}

/* Points a state at a table and the table's fixed entry array. */
extern "C" void snd_reloc_state0_set_fixed(RelocState* state, RelocTable* table)
{
    state->table = table;
    state->entries = (void**)&table->entries_16;
}

/* One delay line of the bank. */
extern "C" ReverbLine* fn_800E7ED0(Sound* snd, u32 idx)
{
    return &snd->lines[idx];
}

/* Installs one line's five relocation tables. */
extern "C" void snd_tsb_install(u32 idx, RelocDesc* desc)
{
    RelocTable* t4 = (RelocTable*)(desc->reloc_04 + (u32)desc);
    RelocTable* t8 = (RelocTable*)(desc->reloc_08 + (u32)desc);
    RelocTable* tC = (RelocTable*)(desc->reloc_0C + (u32)desc);
    RelocTable* t10 = (RelocTable*)(desc->reloc_10 + (u32)desc);

    fn_800E7FBC(fn_800E7ED0(snd_obj_get(), idx), (RelocTable*)(desc->reloc_00 + (u32)desc));
    fn_800E7FB4(fn_800E7ED0(snd_obj_get(), idx), t4);
    fn_800E7FAC(fn_800E7ED0(snd_obj_get(), idx), t8);
    fn_800E7FA4(fn_800E7ED0(snd_obj_get(), idx), tC);
    fn_800E7F9C(fn_800E7ED0(snd_obj_get(), idx), t10);
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

/* One line's slot for a voice id, with interrupts off. */
extern "C" void* fn_800E7FC4(u32 idx, u32 key)
{
    u32 level = OSDisableInterrupts();
    void* slot = snd_line_cue_get(fn_800E7ED0(snd_obj_get(), idx), key);

    OSRestoreInterrupts(level);
    return slot;
}

/* How many voices one owner holds, with interrupts off. */
extern "C" u32 snd_stream_reloc_busy_ck(u32 key)
{
    u32 level = OSDisableInterrupts();
    u32 count = snd_obj_owner_voice_count(snd_obj_get(), key);

    OSRestoreInterrupts(level);
    return count;
}

/* Retunes one owner's voices, with interrupts off. */
extern "C" void fn_800E8080(u32 key, u16 pitch)
{
    u32 level = OSDisableInterrupts();

    snd_obj_owner_lpf_set(snd_obj_get(), key, pitch);
    OSRestoreInterrupts(level);
}

/* Plays a sound through the global object, with interrupts off. */
extern "C" void* fn_800E80DC(u32 a, u32 b, u32 c)
{
    u32 level = OSDisableInterrupts();
    void* r = snd_obj_play_pan(snd_obj_get(), a, b, c);

    OSRestoreInterrupts(level);
    return r;
}

/* Plays a sound through the global object, with interrupts off. */
extern "C" void* fn_800E8150(u32 a, u32 b, u32 c, u32 d)
{
    u32 level = OSDisableInterrupts();
    void* r = snd_obj_play_pos(snd_obj_get(), a, b, c, d);

    OSRestoreInterrupts(level);
    return r;
}

/* Plays a sound through the global object, with interrupts off. */
extern "C" void* fn_800E81BC(u32 a, u32 b, u32 c, u32 d)
{
    u32 level = OSDisableInterrupts();
    void* r = snd_obj_play_pos_reset(snd_obj_get(), a, b, c, d);

    OSRestoreInterrupts(level);
    return r;
}

/* Registers one slot-list entry with no extra data, with interrupts off. */
extern "C" void snd_stream_reloc_stop(u32 owner)
{
    u32 level = OSDisableInterrupts();

    snd_cmd_post(snd_obj_get(), owner, 0, 0, 0, 0, 0, 0, 0);
    OSRestoreInterrupts(level);
}

/* Marks sound bank 1 pending, with interrupts off. */
extern "C" void snd_stream_bank_reset(void)
{
    u32 level = OSDisableInterrupts();

    snd_bank_pending_set(snd_obj_get(), 1);
    OSRestoreInterrupts(level);
}

/* Marks sound bank 2 pending, with interrupts off. */
extern "C" void snd_stream_reloc_pause(void)
{
    u32 level = OSDisableInterrupts();

    snd_bank_pending_set(snd_obj_get(), 2);
    OSRestoreInterrupts(level);
}

/* Marks sound bank 4 pending, with interrupts off. */
extern "C" void snd_stream_reloc_resume(void)
{
    u32 level = OSDisableInterrupts();

    snd_bank_pending_set(snd_obj_get(), 4);
    OSRestoreInterrupts(level);
}

/* Registers a slot-list entry of kind 3, with interrupts off. */
extern "C" void fn_800E8354(u32 owner, s16 arg)
{
    u32 level = OSDisableInterrupts();

    snd_cmd_post(snd_obj_get(), owner, 3, 0, 0, 0, 0, arg, 0);
    OSRestoreInterrupts(level);
}

/* Registers a slot-list entry of kind 4, with interrupts off. */
extern "C" void fn_800E83CC(u32 owner, s16 arg)
{
    u32 level = OSDisableInterrupts();

    snd_cmd_post(snd_obj_get(), owner, 4, 0, 0, 0, 0, 0, arg);
    OSRestoreInterrupts(level);
}

/* The pitch of one owner's first voice, with interrupts off. */
extern "C" s16 fn_800E8444(u32 key)
{
    u32 level = OSDisableInterrupts();
    s16 pitch = snd_obj_owner_pitch_get(snd_obj_get(), key);

    OSRestoreInterrupts(level);
    return pitch;
}

/* Stores one of the global object's level words. */
extern "C" void fn_800E8498(u32 v, u8 idx)
{
    fn_800E84DC(snd_obj_get(), v, idx);
}

/* Stores one level in a sound object's level array. */
extern "C" void fn_800E84DC(Sound* p, u32 v, u8 idx)
{
    p->levels2[idx] = v;
}

/* Sets the global sound object's level word. */
extern "C" void snd_se_level_set(s32 v)
{
    fn_800E8524(snd_obj_get(), (s16)v);
}

/* Stores the sound object's level word. */
extern "C" void fn_800E8524(Sound* snd, s16 v)
{
    snd->field_08 = v;
}

/* The global sound object's level word. */
extern "C" s16 fn_800E852C(void)
{
    return fn_800E8550(snd_obj_get());
}

/* The sound object's level word. */
extern "C" s16 fn_800E8550(Sound* snd)
{
    return snd->field_08;
}

/* Stores the global sound object's mode byte. */
extern "C" void snd_fade_level_set(u8 v)
{
    fn_800E858C(snd_obj_get(), v);
}

/* Stores the sound object's mode byte. */
extern "C" void fn_800E858C(Sound* snd, u8 v)
{
    snd->field_71 = v;
}

/* One of the global object's level words, or 0 for an out-of-range index. */
extern "C" s16 fn_800E8594(u32 idx)
{
    if (idx >= 49) {
        return 0;
    }
    return fn_800E85D8(snd_obj_get(), idx);
}

/* One element of the sound object's level table. */
extern "C" s16 fn_800E85D8(Sound* snd, u32 idx)
{
    return snd->levels[idx];
}

/* Releases one reverb unit, with interrupts off. */
extern "C" void snd_reverb_advance(u32 idx)
{
    u32 level = OSDisableInterrupts();

    snd_reverb_state_advance(snd_reverb_mgr_get(), idx);
    OSRestoreInterrupts(level);
}

/* Applies one reverb config from the global object's bank, with interrupts off. */
extern "C" void snd_reverb_apply(u32 idx, u32 entry_idx)
{
    u32 level = OSDisableInterrupts();

    snd_obj_reverb_apply(snd_obj_get(), idx, entry_idx);
    OSRestoreInterrupts(level);
}

/* The audio system object. */
extern "C" SndAudioSystem* fn_800E8690(void)
{
    return &snd_audio_system;
}

/* The per-frame sound update. */
extern "C" void fn_800E8698(void)
{
    u32 level = OSEnableInterrupts();

    SEQRunAudioFrame();
    SYNRunAudio();
    MIXUpdateSettings();
    fn_800EDB74(snd_bank_table_get());
    fn_800E7D34();
    snd_bank_select_pending();
    OSRestoreInterrupts(level);
}

/* Sets the global voice count with interrupts off. */
extern "C" void snd_master_volume_set(u32 id)
{
    u32 level = OSDisableInterrupts();

    AXSetMasterVolume((u16)id);
    OSRestoreInterrupts(level);
}

/* Brings up the audio subsystem. */
extern "C" void snd_audio_init(void)
{
    u32 level;

    AIInit(0);
    AXInitEx(1);
    MIXInit();
    SYNInit();
    SEQInit();
    AXRegisterCallback((void*)fn_800E8698);
    AXSetCompressor(0);
    snd_output_mode_set(1);
    level = OSDisableInterrupts();
    snd_ai_dma_prev_callback = AIRegisterDMACallback(snd_ai_dma_callback);
    OSRestoreInterrupts(level);
    /* The same reverb work-area clear `clearReverbWorkArea` performs; retail has the body here too. */
    snd_voice_ring_size = 0x8000;
    snd_voice_ring_base = 0x90003F60;
    snd_voice_ring_write_pos = 0;
    snd_voice_ring_read_pos = 0;
    snd_voice_ring_capacity = 0x8000;
    snd_voice_ring_ticks = 0;
    memset((void*)0x90003F60, 0, 0x8000);
}

/* Installs the AX reverb hooks. */
extern "C" void snd_fx_hooks_set(void* alloc, void* free)
{
    AXFXSetHooks(alloc, free);
}

#pragma peephole on

/* Selects the output mode. */
extern "C" void snd_output_mode_set(u16 mode)
{
    switch (mode) {
    case 0:
    case 1: AXSetMode(0); break;
    case 2: AXSetMode(1); break;
    case 3: AXSetMode(2); break;
    }
    MIXSetSoundMode(mode);
    snd_handle_store((u32*)snd_reverb_mgr_get());
}

#pragma peephole off

/* The current sound handle. */
extern "C" u32 snd_handle_get(void)
{
    return MIXGetSoundMode();
}

/* The global level setting's level word. */
extern "C" s32 snd_level_volume_get(void)
{
    return fn_800E8880(fn_800E8690());
}

/* The output level of the audio system object. */
extern "C" s32 fn_800E8880(SndAudioSystem* audio)
{
    return audio->level;
}

/* Queues the next output block: chains the previous callback, copies the block the DMA just finished into the
 * double buffer, mixes the voice ring into it while the transfer mode is on, and hands it to the voice output. */
extern "C" void snd_ai_dma_callback(void)
{
    u32 prev = snd_ai_dma_prev_block;

    if (snd_ai_dma_prev_callback != 0) {
        snd_ai_dma_prev_callback();
    }
    snd_ai_dma_prev_block = AIGetDMAStartAddr() + 0x80000000;
    AIInitDMA((u32)snd_ai_dma_buffers[snd_ai_dma_buffer_index], 0x180);
    if (prev != 0) {
        DCInvalidateRange((void*)prev, 0x180);
        memcpy(snd_ai_dma_buffers[snd_ai_dma_buffer_index], (void*)prev, 0x180);
    } else {
        memset(snd_ai_dma_buffers[snd_ai_dma_buffer_index], 0, 0x180);
    }
    if (snd_voice_transfer_on[0] == 1) {
        if (snd_voice_ring_write_pos != snd_voice_ring_read_pos) {
            snd_voice_ring_mix(snd_ai_dma_buffers[snd_ai_dma_buffer_index]);
        }
        if (getInstance() != 0) {
            getInstance()->writeVoice(snd_ai_dma_buffers[snd_ai_dma_buffer_index], 0x180);
        }
    } else {
        snd_voice_ring_read_pos = snd_voice_ring_write_pos;
        snd_voice_ring_ticks = 0;
    }
    DCFlushRange(snd_ai_dma_buffers[snd_ai_dma_buffer_index], 0x180);
    snd_ai_dma_buffer_index ^= 1;
}

/* The network library mediator. */
extern "C" NetworkWiiMediator* getInstance(void)
{
    return (NetworkWiiMediator*)mpMediator__15sNetworkLibrary;
}

/* Records whether the given state is the "off" one. */
extern "C" void setStreamTransferMode(u32 v)
{
    snd_voice_transfer_on[0] = (u8)(v == 1);
}

/* Upsamples `src` by eight (linear steps of a quarter) into the scratch block after the work area and adds it
 * to `dst` with saturation; returns the number of samples mixed. */
extern "C" s32 snd_voice_upsample_mix(s16* dst, s32 dst_size, const s16* src, s32 src_size)
{
    s32 prev;
    s16* out;
    s32 dst_count;
    s32 src_count;
    s32 cur;
    s32 count;
    s32 i;
    s16* mix;
    s32 k;

    dst_count = dst_size / 2;
    src_count = src_size / 2;
    count = 0;
    prev = *src;
    i = 0;
    out = (s16*)0x9000BF60;
    while (i < src_count || count + 7 < dst_count) {
        cur = *src;
        out[0] = (prev * 6 + cur * 2) / 8;
        out[2] = ((prev + cur) * 4) / 8;
        out[4] = (prev * 2 + cur * 6) / 8;
        out[6] = cur;
        out[1] = out[0];
        out[3] = out[2];
        out[5] = out[4];
        out[7] = out[6];
        prev = cur;
        src++;
        i++;
        out += 8;
        count += 8;
    }
    mix = (s16*)0x9000BF60;
    for (k = 0; k < count; k++) {
        s32 sum = *dst + *mix;

        if (sum > 32767) {
            sum = 32767;
        }
        if (sum < -32768) {
            sum = -32768;
        }
        *dst = sum;
        dst++;
        mix++;
    }
    return count;
}

/* Takes one 48-byte block out of the voice ring (stopping at the write cursor after a wrap) and mixes it,
 * upsampled, into `dst`; returns the number of samples mixed, 0 while the clock has no tick left. */
extern "C" s32 snd_voice_ring_mix(s16* dst)
{
    u8 block[0x210];
    u8* p;
    u8* src;
    s32 i;
    u8 wrapped;

    if (snd_voice_ring_ticks <= 0) {
        return 0;
    }
    snd_voice_ring_ticks--;
    memset(block, 0, 0x30);
    p = block;
    src = (u8*)(snd_voice_ring_read_pos + snd_voice_ring_base);
    wrapped = 0;
    for (i = 0; i < 0x30; i++) {
        *p++ = *src++;
        snd_voice_ring_read_pos++;
        if (snd_voice_ring_read_pos >= snd_voice_ring_capacity) {
            wrapped = 1;
            snd_voice_ring_read_pos = 0;
            src = (u8*)snd_voice_ring_base;
        } else if (wrapped == 1 && snd_voice_ring_read_pos > snd_voice_ring_write_pos) {
            break;
        }
    }
    snd_voice_ring_read_pos = (u32)src - snd_voice_ring_base;
    return snd_voice_upsample_mix(dst, 0x180, (s16*)block, 0x30);
}

/* Copies one buffer into the ring the reverb time counter walks. */
extern "C" void pushReverbSamples(s16* src, s32 count)
{
    u32 pos = snd_voice_ring_write_pos;
    s16* dst;
    s32 n;

    snd_voice_ring_push_pos = pos;
    snd_voice_ring_push_frame = frame_counter;
    dst = (s16*)(pos + snd_voice_ring_base);
    n = count / 2;
    while (n-- > 0) {
        *dst++ = *src++;
        pos = snd_voice_ring_write_pos + 2;
        snd_voice_ring_write_pos = pos;
        if (pos % snd_voice_ring_capacity == 0) {
            snd_voice_ring_write_pos = 0;
            dst = (s16*)snd_voice_ring_base;
        }
    }
}

/* Clears the reverb work area. */
extern "C" void clearReverbWorkArea(void)
{
    u32 size = 0x8000;
    void* base = (void*)0x90003F60;

    snd_voice_ring_size = size;
    snd_voice_ring_base = (u32)base;
    snd_voice_ring_write_pos = 0;
    snd_voice_ring_read_pos = 0;
    snd_voice_ring_capacity = size;
    snd_voice_ring_ticks = 0;
    memset(base, 0, size);
}

/* Advances the reverb time counter with interrupts off. */
extern "C" void advanceReverbClock(void)
{
    u32 level = OSDisableInterrupts();

    snd_voice_ring_ticks += 11;
    OSRestoreInterrupts(level);
}
