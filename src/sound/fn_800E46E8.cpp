/* sound/fn_800E46E8.cpp - the reverb manager and stream-state band of the sound runtime
 *
 * `.text` 0x800E46E8..0x800E5430, 65 functions written (the rest of the range is not decompiled yet).
 * Each function keeps the `#pragma` state it had in its retired source. The retired header's notes follow below.
 */

/* Retired header of `sound/fn_800E46E8.cpp` (kept for its notes and residuals): */
/* sound/fn_800E46E8.cpp - the sound/stream runtime: `fn_800E46E8`, `.text` 0x800E46E8-0x800E8E60.
 * Naming note: the symbol map has only fn_XXXXXXXX for this range (checked with symedit: every fn_ name this file uses is a bare .text entry in config/RMHE08/symbols.txt)
 *
 * 196 functions in one C++ TU.  Two globals carry the state:
 *   * the stream manager `lbl_8069A810` (0xD8 B, `.bss`): a flag word at +0x04, the current slot index at
 *     +0x24 with its three-entry table at +0x26, and three per-stream records at +0x5C, stride 0x28
 *     (`fn_800E4C90` is `base + i*0x28 + 0x5C`, `fn_800E4EA4` bounds the index at 3).  Each record holds
 *     four relocation-table states (`table` + resolved `entries`) at +0x00/+0x08/+0x10/+0x18 and two
 *     scalars at +0x20/+0x24; `snd_reloc_state_rebase` walks one state's table and adds the table base to every entry
 *     whose top bit is clear (`fn_800E7578` is the read side, `fn_800E7E94`..`7FBC` the per-offset
 *     installers).
 *   * the sound object `lbl_80697980` (0xE60 B): a vtable, the level table `levels[49]` at +0x0A, three
 *     level words at +0x7C, two floats at +0x88/+0x8C and the 49 72-byte reverb delay lines at +0x90.
 *     `fn_800E7ABC` is its constructor, `fn_800E7C34`/`dtor_800E5548` the line constructor/destructor.
 *     The reverb units themselves live in `snd_reverb_mgr` (`ReverbMgr`): three `ReverbFx` (0x3F4 B each)
 *     with a 1000-byte effect body, their `effects[3]` pointers at +0xBE0 and a pending-bit word at +0xBEC.
 *
 * The volume block is a separate `.sbss` run (0x807949B0..0x807949D8): per-slot "is bgm" flags, a
 * `{cur, prev}` pair for bgm and one for se, and the set-point each is driven from.
 *
 * Flags: `auto` / `Wii/1.3` / `cflags_main` (`-O3 -inline noauto`), but retail's codegen is
 * **peephole-off** throughout - the tell is the unfused `clrlwi r0,r3,24` before a `u8` use
 * (`set_stream_main_vol_flag`, `fn_800E48E4`, `fn_800E4908`, the `fn_800E46E8` selector) and the
 * unfused `slwi`/`addi r0` elsewhere, which the peephole removes.  The whole-file
 * `#pragma peephole off` below carries that; the outbox asks for a per-region `-opt nopeephole` cflags
 * group so the pragma can go.
 *
 * Linkage: this is a C++ file whose map names are a mix - the named entries (`set_stream_main_vol_flag__
 * FUcUc`, `PlayStream__FUlUl`, ...) are the *mangled* names of C++ functions and are written as such,
 * while every `fn_XXXXXXXX` entry has an unmangled map name, so those definitions and declarations carry
 * `extern "C"` (main.cpp's header records the same convention).
 *
 * Residuals (measured with `python build/tmp/unitreport.py`, the report metric):
 *   * 11 functions are still unwritten (0 %): `fn_800E5720` (1608 B), `fn_800E6314` (728), `fn_800E6A58`
 *     (520), `fn_800E6818` (512), `fn_800E6D8C` (496), `fn_800E7078` (492), `fn_800E8B1C` (428),
 *     `fn_800E8888` (336), `fn_800E89F8` (292), `fn_800E6764` (180) and `fn_800E7EA4` (24 B, 66 %).
 *   * `fn_800E7EA4`/`fn_800E7EBC`/`fn_800E77DC`/`fn_800E6A18`/`fn_800E6FB8`/`fn_800E6FF8`/`fn_800E7038`/
 *     `fn_800E4FB4` are the "materialise the state pointer, then a redundant branch" shape: retail emits
 *     `addi r3,r3,OFF` / `b` where ours folds `OFF` into the displacement (24-64 B, 66-93 %).  Neither
 *     `&block->state[i]`, `block->state + i`, a `for(;;) { ...; break; }`, a `while` nor a `do/while(0)`
 *     reproduces it, and marking `fn_800E7578` `inline` (so the wrappers inline it) does not either - it
 *     only loses `fn_800E4FB4`'s 87 %.  The logic itself is byte-identical.
 *   * `fn_800E46E8`'s jump table is `jumptable_80597D88`, which lives in the unit's **unclaimed** `.data`
 *     range (0x80597D88), so our object emits its own local `@NN` table; the score is unaffected
 *     (reloc-only) and the claim belongs to the measured data pass.
 *
 * Inventory, addresses and sizes: `python tools/units/ledger.py unit sound/fn_800E46E8.cpp`.
 * The name is provisional (`auto/` plus the first symbol's address) because nothing in the object names
 * the original source file.
 */

#include "sound/snd_stream_mgr.h" /* snd_stream_mgr (rule 2: the owner's header) */
#include "sound/snd_voice_pool.h" /* snd_voice_pool (rule 2: the owner's header) */
#include "sound/snd_stream_reloc.h" /* snd_stream_reloc (rule 2: the owner's header) */
#include "types.h"
#include "sound/fn_800E46E8_types.h"

/* A 4x4 matrix, 0x40 B, the form the GX projection path exchanges.  Only copied here, so it is an
 * opaque word block. */
typedef struct Mtx44 {
    /* +0x00 */ u32 w[16];
} Mtx44; /* size: 0x40 */

/* The flag word snd_bank_pending_ck/49E0/4ABC reach at +4 (both the stream manager and the sound object
 * carry it there). */
typedef struct FlagWord {
    /* +0x00 */ u32 unused_00;
    /* +0x04 */ u32 flags;
} FlagWord; /* size: 0x8 */

/* A volume that is smoothed from `prev` to `cur`. */
typedef struct StreamVol {
    /* +0x00 */ f32 cur;
    /* +0x04 */ f32 prev;
} StreamVol; /* size: 0x8 */

/* The same body for the second effect class: its live parameters sit 0x30 lower. */
typedef struct ReverbStdData {
    /* +0x0000 */ u8 state_0000[0x148];
    /* +0x0148 */ f32 damping;
    /* +0x014C */ f32 mix;
    /* +0x0150 */ f32 time;
    /* +0x0154 */ f32 coloration;
    /* +0x0158 */ f32 pre_delay;
    /* +0x015C */ f32 crosstalk;
    /* +0x0160 */ u8 state_0160[0x288];
} ReverbStdData; /* size: 0x3E8 */

/* The unit's `.sbss` volume block.  Declared with its map names - the target object relocates against
 * exactly these. */
extern u8 lbl_807949B0[8];          /* per-slot "this stream is bgm" flags */
extern StreamVol lbl_807949B8;      /* bgm smoothed volume */
extern StreamVol lbl_807949C0;      /* bgm set-point (set_str_bgm_main_vol) */
extern StreamVol lbl_807949C8;      /* se smoothed volume */
extern StreamVol lbl_807949D0;      /* se set-point (set_str_se_main_vol) */

extern f32 lbl_80796478;            /* 1.0f: the initial volume */

extern ReverbMgr snd_reverb_mgr;      /* the global reverb manager (fn_800E5398) */

extern "C" {
void fn_80075390(Mtx44* mtx);
void fn_802BDC90(Mtx44* mtx);
void* fn_80075E98(Mtx44* mtx);
void GXSetProjection(void* mtx, u32 type);
u32 OSDisableInterrupts(void);
void OSRestoreInterrupts(u32 level);

/* This unit's own entries (defined below; declared up here so the address order below can be read in
 * call order). */
Mtx44* fn_800E479C(Mtx44* dst, const Mtx44* src);
void fn_800E47CC(Mtx44* dst, const Mtx44* src);
void fn_800E4E00(StreamRec* rec);
void fn_800E4E10(StreamRec* rec);
void fn_800E4E18(RelocState* state);
void fn_800E4E28(StreamRec* rec);
void fn_800E4E30(StreamRec* rec);
void fn_800E4E38(StreamRec* rec);
void fn_800E4E70(StreamWork* work, s16 slot);
s16 fn_800E4E9C(StreamWork* work);
s16 fn_800E4EE8(StreamWork* work, u32 idx);
void fn_800E4C0C(StreamRec* rec, u32 a, u32 b);
void fn_800E4C18(StreamRec* rec, RelocTable* table);
void snd_reloc_state_rebase(RelocState* state, RelocTable* table);
void fn_800E4C78(StreamRec* rec, RelocTable* table);
void fn_800E4C80(StreamRec* rec, RelocTable* table);
void fn_800E4C88(StreamRec* rec, RelocTable* table);
void* fn_800E4FB4(StreamRec* rec, u32 entry_idx);

void snd_reverb_state_advance(ReverbMgr* mgr, u32 idx);
ReverbHiData* fn_800E5184(ReverbFx* fx);
ReverbStdData* fn_800E517C(ReverbFx* fx);
ReverbStdData* fn_800E518C(ReverbFx* fx);
ReverbHiData* fn_800E5194(ReverbFx* fx);
ReverbStdData* fn_800E519C(ReverbFx* fx);
ReverbHiData* fn_800E51A4(ReverbFx* fx);
void fn_800E522C(ReverbFx* fx);
void fn_800E523C(ReverbFx* fx);
void fn_800E52A0(ReverbFx* fx);
void fn_800E5304(ReverbFx* fx);
ReverbMgr* fn_800E53A4(ReverbMgr* mgr);
void fn_800E5420(ReverbFx* fx, ReverbSetFn handler);

void fn_80474DD0(void);
void fn_80474EB0(ReverbHiData* data);
void fn_80474F20(void);
void fn_80474D50(ReverbStdData* data);
void fn_80474E80(ReverbHiData* data);
void fn_80476DF0(ReverbStdData* data);
void fn_80476E80(ReverbHiData* data);
void fn_80476F10(ReverbStdData* data);
void fn_80476F40(ReverbHiData* data);
void AXFXReverbHiInit(ReverbStdData* data);
void AXFXReverbHiShutdown(ReverbStdData* data);
void AXFXReverbHiCallback(void);
void AXRegisterAuxACallback(void);
void fn_8046F210(void);
void fn_8046F280(void);
void* memset(void* dst, int val, u32 n);

StreamWork* fn_800E4A18(void);
/* untyped: the stream manager and the sound objects both keep their flag word at +4: a caller-owned payload */
u32 snd_bank_pending_ck(void* work, u32 mask);
/* untyped: the stream manager and the sound objects both keep their flag word at +4: a caller-owned payload */
void snd_bank_pending_clear(void* work, s32 mask);
/* untyped: the stream manager and the sound objects both keep their flag word at +4: a caller-owned payload */
void snd_bank_pending_set(void* work, u32 mask);
StreamWork* snd_bank_table_get(void);
StreamRec* fn_800E4C90(StreamWork* work, u32 idx);
void fn_800E48B0(void);
}

#pragma peephole off

/* Maps a sound-kind selector to the volume-slot it plays through. */
extern "C" s32 fn_800E46E8(u32 kind)
{
    switch ((u8)kind) {
    case 0: return 0;
    case 1: return 1;
    case 2: return 3;
    case 3: return 2;
    case 4: return 5;
    case 5: return 6;
    case 6: return 4;
    case 7: return 7;
    default: return 3;
    }
}

/* Builds a projection matrix from the current screen setup and hands it to GX. */
extern "C" void fn_800E4754(void)
{
    Mtx44 mtx;
    Mtx44 src;

    fn_80075390(&mtx);
    fn_802BDC90(&src);
    fn_800E479C(&mtx, &src);
    GXSetProjection(fn_80075E98(&mtx), 0);
}

/* Assigns one matrix over another and returns the destination. */
extern "C" Mtx44* fn_800E479C(Mtx44* dst, const Mtx44* src)
{
    fn_800E47CC(dst, src);
    return dst;
}

/* Copies a 4x4 matrix. */
extern "C" void fn_800E47CC(Mtx44* dst, const Mtx44* src)
{
    *dst = *src;
}

/* Resets every volume to 1.0 and clears the per-slot flags. */
extern "C" void fn_800E4850(void)
{
    lbl_807949B0[0] = 0;
    lbl_807949B0[1] = 0;
    lbl_807949B0[2] = 0;
    lbl_807949B0[3] = 0;
    lbl_807949B8.prev = lbl_80796478;
    lbl_807949B8.cur = lbl_80796478;
    lbl_807949C0.cur = lbl_80796478;
    lbl_807949C8.prev = lbl_80796478;
    lbl_807949C8.cur = lbl_80796478;
    lbl_807949D0.cur = lbl_80796478;
}

/* Sets one slot's "this stream is bgm" flag. */
void set_stream_main_vol_flag(u8 slot, u8 is_bgm)
{
    lbl_807949B0[slot] = is_bgm;
}

/* Sets the bgm set-point volume. */
void set_str_bgm_main_vol(f32 vol)
{
    lbl_807949C0.cur = vol;
}

/* Sets the se set-point volume. */
void set_str_se_main_vol(f32 vol)
{
    lbl_807949D0.cur = vol;
}

/* Snapshots both set-points into the smoothed volumes (the previous value keeps the interpolation start). */
extern "C" void fn_800E48B0(void)
{
    lbl_807949B8.prev = lbl_807949B8.cur;
    lbl_807949B8.cur = lbl_807949C0.cur;
    lbl_807949C8.prev = lbl_807949C8.cur;
    lbl_807949C8.cur = lbl_807949D0.cur;
}

/* The stream subsystem is always ready. */
extern "C" s32 fn_800E48DC(void)
{
    return 1;
}

/* Picks the smoothed volume of the bus one slot is assigned to. */
extern "C" f32 fn_800E48E4(u8 slot)
{
    if (lbl_807949B0[slot] == 0) {
        return lbl_807949B8.cur;
    }
    return lbl_807949C8.cur;
}

/* Sets the set-point of the bus selected by `is_bgm`. */
extern "C" void fn_800E4908(u8 is_bgm, f32 vol)
{
    if (is_bgm == 0) {
        lbl_807949C0.cur = vol;
    }
    if (is_bgm != 0) {
        lbl_807949D0.cur = vol;
    }
}

/* The current bgm volume. */
extern "C" f32 fn_800E492C(void)
{
    return lbl_807949B8.cur;
}

/* Snapshots the volumes, then selects whichever of the three sound banks is flagged as pending. */
extern "C" void snd_bank_select_pending(void)
{
    fn_800E48B0();
    if (snd_bank_pending_ck(fn_800E4A18(), 1) != 0) {
        snd_bank1_install(snd_bank_table_get(), 2);
        snd_bank_pending_clear(fn_800E4A18(), 1);
        return;
    }
    if (snd_bank_pending_ck(fn_800E4A18(), 2) != 0) {
        snd_bank2_install(snd_bank_table_get(), 2);
        snd_bank_pending_clear(fn_800E4A18(), 2);
        return;
    }
    if (snd_bank_pending_ck(fn_800E4A18(), 4) != 0) {
        snd_bank3_install(snd_bank_table_get(), 2);
        snd_bank_pending_clear(fn_800E4A18(), 4);
    }
}

/* Clears the given bank's pending bits. */
/* untyped: the stream manager and the sound objects both keep their flag word at +4: a caller-owned payload */
extern "C" void snd_bank_pending_clear(void* work, s32 mask)
{
    ((FlagWord*)work)->flags = ((FlagWord*)work)->flags & ~mask;
}

/* The sound table the bank installers write into. */
extern "C" StreamWork* snd_bank_table_get(void)
{
    return (StreamWork*)lbl_8069A8E8;
}

/* Tests whether any of the given bank's pending bits is set. */
/* untyped: the stream manager and the sound objects both keep their flag word at +4: a caller-owned payload */
extern "C" u32 snd_bank_pending_ck(void* work, u32 mask)
{
    return (((FlagWord*)work)->flags & mask) != 0;
}

/* The global stream manager. */
extern "C" StreamWork* fn_800E4A18(void)
{
    return &lbl_8069A810;
}

/* Resets the volume block and hands the three sound-bank parameters to the bank installer. */
extern "C" void fn_800E4A24(u32 a, u32 b, u32 c)
{
    fn_800E4850();
    fn_800EBAE8(fn_800E4A18(), a, b, c);
}

/* Marks bank 1 pending, with interrupts off. */
extern "C" void resumeSoundEngine(void)
{
    u32 level = OSDisableInterrupts();
    snd_bank_pending_set(fn_800E4A18(), 1);
    OSRestoreInterrupts(level);
}

/* Sets the given bank's pending bits. */
/* untyped: the stream manager and the sound objects both keep their flag word at +4: a caller-owned payload */
extern "C" void snd_bank_pending_set(void* work, u32 mask)
{
    ((FlagWord*)work)->flags |= mask;
}

/* Marks bank 2 pending, with interrupts off. */
extern "C" void fn_800E4ACC(void)
{
    u32 level = OSDisableInterrupts();
    snd_bank_pending_set(fn_800E4A18(), 2);
    OSRestoreInterrupts(level);
}

/* Marks bank 4 pending, with interrupts off. */
extern "C" void fn_800E4B0C(void)
{
    u32 level = OSDisableInterrupts();
    snd_bank_pending_set(fn_800E4A18(), 4);
    OSRestoreInterrupts(level);
}

/* Installs one stream's four relocation tables and its two scalar words. */
extern "C" void fn_800E4B4C(u32 idx, RelocDesc* desc, u32 a, u32 b)
{
    RelocTable* t4 = (RelocTable*)(desc->reloc_04 + (u32)desc);
    RelocTable* t8 = (RelocTable*)(desc->reloc_08 + (u32)desc);
    RelocTable* tC = (RelocTable*)(desc->reloc_0C + (u32)desc);
    fn_800E4C88(fn_800E4C90(fn_800E4A18(), idx), (RelocTable*)(desc->reloc_00 + (u32)desc));
    fn_800E4C80(fn_800E4C90(fn_800E4A18(), idx), t4);
    fn_800E4C78(fn_800E4C90(fn_800E4A18(), idx), t8);
    fn_800E4C18(fn_800E4C90(fn_800E4A18(), idx), tC);
    fn_800E4C0C(fn_800E4C90(fn_800E4A18(), idx), a, b);
}

/* Stores a stream's two scalar words. */
extern "C" void fn_800E4C0C(StreamRec* rec, u32 a, u32 b)
{
    rec->scalar_20 = a;
    rec->scalar_24 = b;
}

/* Installs a relocation table into a stream's first state. */
extern "C" void fn_800E4C18(StreamRec* rec, RelocTable* table)
{
    snd_reloc_state_rebase(&rec->reloc[0], table);
}

/* Installs a relocation table into the given state and resolves every entry that is not yet fixed. */
extern "C" void snd_reloc_state_rebase(RelocState* state, RelocTable* table)
{
    u32 i;

    state->table = table;
    state->entries = (void**)((u8*)table + table->entries_offset);
    for (i = 0; i < state->table->count; i++) {
        u32 entry = (u32)state->entries[i];
        if ((entry & 0x80000000) == 0 && entry != 0) {
            state->entries[i] = (void*)((u32)state->entries + entry);
        }
    }
}

/* Installs a relocation table into a stream's third state. */
extern "C" void fn_800E4C78(StreamRec* rec, RelocTable* table)
{
    snd_reloc_state_rebase(&rec->reloc[2], table);
}

/* Installs a relocation table into a stream's fourth state. */
extern "C" void fn_800E4C80(StreamRec* rec, RelocTable* table)
{
    snd_reloc_state_rebase(&rec->reloc[3], table);
}

/* Installs a relocation table into a stream's second state. */
extern "C" void fn_800E4C88(StreamRec* rec, RelocTable* table)
{
    snd_reloc_state_rebase(&rec->reloc[1], table);
}

/* The stream record of one slot. */
extern "C" StreamRec* fn_800E4C90(StreamWork* work, u32 idx)
{
    return &work->recs[idx];
}

/* Starts a stream on the given bank, with interrupts off. */
void PlayStream(u32 a, u32 b)
{
    u32 level = OSDisableInterrupts();
    fn_800EBC24(fn_800E4A18(), a, b, 0);
    OSRestoreInterrupts(level);
}

/* Stops the stream on the given bank, with interrupts off. */
extern "C" void fn_800E4D00(u32 a, u32 b)
{
    u32 level = OSDisableInterrupts();
    fn_800EBDDC(fn_800E4A18(), a, b, 0);
    OSRestoreInterrupts(level);
}

/* Releases one slot's sound table and clears its record. */
extern "C" void fn_800E4D60(u32 idx)
{
    u32 level = OSDisableInterrupts();

    fn_800EDD00(snd_bank_table_get(), 2, idx);
    fn_800E4E38(fn_800E4C90(fn_800E4A18(), idx));
    fn_800E4E30(fn_800E4C90(fn_800E4A18(), idx));
    fn_800E4E28(fn_800E4C90(fn_800E4A18(), idx));
    fn_800E4E10(fn_800E4C90(fn_800E4A18(), idx));
    fn_800E4E00(fn_800E4C90(fn_800E4A18(), idx));
    OSRestoreInterrupts(level);
}

/* Clears a stream record's two scalar words. */
extern "C" void fn_800E4E00(StreamRec* rec)
{
    rec->scalar_20 = 0;
    rec->scalar_24 = 0;
}

/* Clears a stream record's fourth relocation state. */
extern "C" void fn_800E4E10(StreamRec* rec)
{
    fn_800E4E18(&rec->reloc[3]);
}

/* Clears a relocation state. */
extern "C" void fn_800E4E18(RelocState* state)
{
    state->table = 0;
    state->entries = 0;
}

/* Clears a stream record's third relocation state. */
extern "C" void fn_800E4E28(StreamRec* rec)
{
    fn_800E4E18(&rec->reloc[2]);
}

/* Clears a stream record's second relocation state. */
extern "C" void fn_800E4E30(StreamRec* rec)
{
    fn_800E4E18(&rec->reloc[1]);
}

/* Clears a stream record's first relocation state. */
extern "C" void fn_800E4E38(StreamRec* rec)
{
    fn_800E4E18(&rec->reloc[0]);
}

/* Selects the volume slot a stream plays through. */
extern "C" void fn_800E4E3C(s32 slot)
{
    fn_800E4E70(fn_800E4A18(), (s16)slot);
}

/* Stores the manager's current slot index. */
extern "C" void fn_800E4E70(StreamWork* work, s16 slot)
{
    work->cur_slot = slot;
}

/* The manager's current slot index. */
extern "C" s16 fn_800E4E78(void)
{
    return fn_800E4E9C(fn_800E4A18());
}

/* The manager's current slot index. */
extern "C" s16 fn_800E4E9C(StreamWork* work)
{
    return work->cur_slot;
}

/* The volume slot of one of the three banks, or 0 for an out-of-range bank. */
extern "C" s16 fn_800E4EA4(u32 idx)
{
    if (idx >= 3) {
        return 0;
    }
    return fn_800E4EE8(fn_800E4A18(), idx);
}

/* The volume slot stored for one bank. */
extern "C" s16 fn_800E4EE8(StreamWork* work, u32 idx)
{
    return work->slot_tab[idx];
}

/* The status of one stream, with interrupts off. */
u32 GetStreamStatus(u32 id)
{
    u32 level = OSDisableInterrupts();
    u32 status = fn_800ED5B4(fn_800E4A18(), id);

    OSRestoreInterrupts(level);
    return status;
}

/* One stream's entry in a relocation table, with interrupts off. */
extern "C" void* fn_800E4F4C(u32 idx, u32 entry_idx)
{
    u32 level = OSDisableInterrupts();
    void* entry = fn_800E4FB4(fn_800E4C90(fn_800E4A18(), idx), entry_idx);

    OSRestoreInterrupts(level);
    return entry;
}

/* Looks up one entry of a stream's third relocation table. */
extern "C" void* fn_800E4FB4(StreamRec* rec, u32 entry_idx)
{
    RelocState* state = rec->reloc + 2;

    if (state->table == 0) {
        return 0;
    }
    if (state->table->count <= entry_idx) {
        return 0;
    }
    return state->entries[entry_idx];
}

/* Installs one reverb unit's parameters and marks it pending. */
extern "C" void snd_reverb_unit_set(ReverbMgr* mgr, ReverbCfg* cfg)
{
    u32 kind = cfg->kind;

    if (kind >= 3) {
        return;
    }
    snd_reverb_state_advance(mgr, kind);
    if (cfg->enable != 0) {
        mgr->flags |= 1 << cfg->kind;
    } else {
        mgr->flags &= ~(1 << cfg->kind);
    }
    ReverbFx* fx = mgr->effects[cfg->kind];
    if (mgr->mode == 3) {
        ReverbHiData* data = fn_800E5184(fx);
        data->damping = cfg->damping;
        data->mix = cfg->mix;
        data->time = cfg->time;
        data->coloration = cfg->coloration;
        data->pre_delay = cfg->pre_delay;
        data->crosstalk = cfg->crosstalk;
        fn_80474DD0();
        fn_80474EB0(data);
        fx->set_handler((ReverbSetFn)(void*)fn_80474F20, data);
        fx->active = 1;
    } else {
        ReverbStdData* data = fn_800E517C(fx);
        data->damping = cfg->damping;
        data->mix = cfg->mix;
        data->time = cfg->time;
        data->coloration = cfg->coloration;
        data->pre_delay = cfg->pre_delay;
        data->crosstalk = cfg->crosstalk;
        AXFXReverbHiInit(data);
        fn_80474D50(data);
        fx->set_handler((ReverbSetFn)(void*)AXFXReverbHiCallback, data);
        fx->active = 0;
    }
    fx->state = 0;
}

/* The effect body of the second class. */
extern "C" ReverbStdData* fn_800E517C(ReverbFx* fx)
{
    return (ReverbStdData*)&fx->data;
}

/* The effect body of the first class. */
extern "C" ReverbHiData* fn_800E5184(ReverbFx* fx)
{
    return (ReverbHiData*)&fx->data;
}

/* The effect body of the second class. */
extern "C" ReverbStdData* fn_800E518C(ReverbFx* fx)
{
    return (ReverbStdData*)&fx->data;
}

/* The effect body of the first class. */
extern "C" ReverbHiData* fn_800E5194(ReverbFx* fx)
{
    return (ReverbHiData*)&fx->data;
}

/* The effect body of the second class. */
extern "C" ReverbStdData* fn_800E519C(ReverbFx* fx)
{
    return (ReverbStdData*)&fx->data;
}

/* The effect body of the first class. */
extern "C" ReverbHiData* fn_800E51A4(ReverbFx* fx)
{
    return (ReverbHiData*)&fx->data;
}

/* Releases one reverb unit: its shutdown routine for the current state, then its callback and body. */
extern "C" void snd_reverb_state_advance(ReverbMgr* mgr, u32 idx)
{
    ReverbFx* fx = mgr->effects[idx];
    s32 state = fx->state;

    if (state == 3) {
        return;
    }
    switch (state) {
    case 0: fn_800E523C(fx); break;
    case 1: fn_800E52A0(fx); break;
    case 2: fn_800E5304(fx); break;
    }
    fx->state = 3;
}

/* Clears an effect body. */
extern "C" void fn_800E522C(ReverbFx* fx)
{
    memset(&fx->data, 0, sizeof(fx->data));
}

/* Releases the first effect class: its AXFX shutdown, then the callback and the body. */
extern "C" void fn_800E523C(ReverbFx* fx)
{
    if (fx->active == 1) {
        fn_80474E80(fn_800E5184(fx));
    } else {
        AXFXReverbHiShutdown(fn_800E517C(fx));
    }
    fx->set_handler(0, 0);
    fn_800E522C(fx);
}

/* Releases the second effect class. */
extern "C" void fn_800E52A0(ReverbFx* fx)
{
    if (fx->active == 3) {
        fn_80476E80(fn_800E5194(fx));
    } else {
        fn_80476DF0(fn_800E518C(fx));
    }
    fx->set_handler(0, 0);
    fn_800E522C(fx);
}

/* Releases the third effect class. */
extern "C" void fn_800E5304(ReverbFx* fx)
{
    if (fx->active == 3) {
        fn_80476F40(fn_800E51A4(fx));
    } else {
        fn_80476F10(fn_800E519C(fx));
    }
    fx->set_handler(0, 0);
    fn_800E522C(fx);
}

/* Stores the current sound handle in the caller's word. */
extern "C" void snd_handle_store(u32* out)
{
    *out = snd_handle_get();
}

/* Constructs the global reverb manager. */
extern "C" ReverbMgr* fn_800E5398(void)
{
    return fn_800E53A4(&snd_reverb_mgr);
}

/* Wires up the three reverb units and their effect pointers. */
extern "C" ReverbMgr* fn_800E53A4(ReverbMgr* mgr)
{
    fn_800E5420(&mgr->fx[0], (ReverbSetFn)(void*)AXRegisterAuxACallback);
    fn_800E5420(&mgr->fx[1], (ReverbSetFn)(void*)fn_8046F210);
    fn_800E5420(&mgr->fx[2], (ReverbSetFn)(void*)fn_8046F280);
    mgr->flags = 0;
    mgr->effects[0] = &mgr->fx[0];
    mgr->effects[1] = &mgr->fx[1];
    mgr->effects[2] = &mgr->fx[2];
    return mgr;
}

/* Sets one unit's callback and marks it constructed. */
extern "C" void fn_800E5420(ReverbFx* fx, ReverbSetFn handler)
{
    fx->set_handler = handler;
    fx->state = 3;
}
