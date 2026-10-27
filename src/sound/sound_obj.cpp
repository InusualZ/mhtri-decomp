/* sound/sound_obj.cpp - the global sound object: its voice setup, 3D positioning and the reverb delay lines
 *
 * RANGE. `.text` 0x800E5430..0x800E7D34, every function written, in address order.
 * FLAGS. lib `sound` (`cflags_main`, Wii/1.3); `#pragma peephole off` over the file.
 * NAMES. GUESS, every name in the unit (no `__FILE__` string, no dump name): the module and file name from the 0xE60 B
 *   global sound object `g_snd_obj`; the function names from the bodies and the object's vtable at 0x80597DA8, whose
 *   slots are snd_obj_destroy, update, release, voice_dropped, stop, pause, resume and start (the same interface as
 *   StreamMgr's table at 0x80598068); `snd_obj_static_init` is the `.ctors` entry; the record names `SndPlayParam`,
 *   `SndRegion`, `SndArtic`, `SndSample`, `VoiceCfg`, `AxVoiceBlock` and their fields (traced from the loads in
 *   snd_voice_setup, snd_obj_play_regions and snd_obj_voice_limit); the registry names of the sound/fn_800E8E60 callees
 *   (`snd_listener_polar`, `snd_distance_atten`, `snd_pan_calc`, `snd_span_calc`, `snd_distance_volume`,
 *   `snd_distance_priority`, `snd_ref_table_count`).
 * RESIDUALS. snd_obj_play_chain: retail passes the chained cue id through `clrlwi r4,r0,16` - the cue id is a u16 from
 *   snd_stream_reloc's wrappers through snd_obj_play_pan/pos/pos_reset and snd_line_cue_get, which this unit cannot
 *   retype alone (its header's callers pass u32); the line/cue registers also swap (r31/r30). The play functions
 *   return the play counter (u32); `sound/sound_obj.h` keeps the `void*` its snd_stream_reloc callers store it into.
 * RESIDUALS. snd_voice_setup: the three aux-level products colour their float temporaries differently (f3/f4 vs
 *   f0/f2; the master level and the 127.0f constant).
 * RESIDUALS. Data: the vtables at 0x80597DA8/DD0/DF8 are stored by hand (snd_obj_construct, snd_player_base_construct,
 *   snd_base_construct) - the class hierarchy SndBase <- SndPlayerBase <- SoundObj/StreamMgr is not modelled yet, so
 *   `.data`, the `.bss` object, the `.ctors` entry and the destructor extab records are not emitted; `.sdata2` and
 *   `.rodata` are byte-identical up to the trailing link fill.
 * SHAPES. The tail-called table lookups (snd_line_*_get) are 4/8-byte wrappers over separate 0x38-byte helpers; the
 *   voice keys are `key = global ? 255 : line` then one expression; snd_voice_volume_calc returns an s16 assigned
 *   from the float product without a cast.
 */

#include "sound/fn_800E8E60.h" /* fn_800E8E60 (rule 2: the owner's header) */
#include "sound/snd_reverb_mgr.h" /* snd_reverb_mgr (rule 2: the owner's header) */
#include "sound/snd_stream_reloc.h" /* snd_stream_reloc (rule 2: the owner's header) */
#include "sound/snd_voice_pool.h" /* snd_voice_pool (rule 2: the owner's header) */
#include "sound/snd_level_tbl.h" /* snd_level_tbl (rule 2: the owner's header) */
#include "sound/snd_stream_mgr.h" /* snd_stream_mgr (rule 2: the owner's header) */
#include "sound/quest_snd.h" /* quest_snd (rule 2: the owner's header) */
#include "sound/snd_bank_table_get.h" /* snd_bank_table_get (rule 2: the owner's header) */
#include "sound/snd_reverb_unit_set.h" /* snd_reverb_unit_set (rule 2: the owner's header) */
#include "sound/snd_voice_alloc.h" /* snd_voice_pool (rule 2: leaf) */
#include "sound/snd_envelope_update.h" /* sound/fn_800E8E60 (rule 2: leaf) */
#include "sound/snd_listener_polar.h" /* sound/fn_800E8E60 (rule 2: leaf) */
#include "AX/AXVPB.h"
#include "AX/AXAlloc.h"
#include "MIX/mix.h"
#include "MTX/mtx.h"
#include "types.h"
#include "sound/fn_800E46E8_types.h"
/* signatures the calls below use, when they differ from the owner header's: a cast call is the same direct call. */
#define fn_800EDB24_c1 ((void* (*)(void*))snd_voice_first)
#define fn_800EDA88_c1 ((void* (*)(void*, void*))snd_voice_free)
#define snd_envelope_update_c1 ((void (*)(Sound2*, SoundSlot*, AxVoiceBlock*))snd_envelope_update)

/* The 49-line bank `snd_obj_destroy` destroys at +0x90. */
typedef struct ReverbBank {
    /* +0x000 */ u8 unused_000[0x90];
    /* +0x090 */ ReverbLine lines[49];
} ReverbBank; /* size: 0xE58 */

/* One slot-list entry's info block (snd_cmd_entry_info returns entry+12). */
typedef struct SlotEntry {
    /* +0x00 */ u32 kind;      /* snd_cmd_list_run dispatches the voice handler on it */
    /* +0x04 */ u32 owner;     /* compared against the voice's owner */
    /* +0x08 */ u8 field_08;
    /* +0x09 */ u8 field_09;
    /* +0x0A */ u8 field_0A;
    /* +0x0B */ u8 field_0B;
    /* +0x0C */ s16 field_0C;  /* snd_obj_cue_volume_set's third argument */
    /* +0x0E */ s16 field_0E;  /* snd_obj_pitch_set's third argument */
} SlotEntry; /* size: 0x10 */

/* The AX handle `fn_800E8E60` reads a word from. */
typedef struct AxHandle {
    /* +0x00 */ u32 unused_00;
    /* +0x04 */ u32 field_04;
} AxHandle; /* size: 0x8 */

/* The per-voice state block of a sound slot (snd_slot_state_get returns it); snd_voice_setup fills it. */
typedef struct AxVoiceBlock {
    /* +0x00 */ s16 volume;        /* the cue's random-scaled volume, 0..127 */
    /* +0x02 */ s16 cue_volume;    /* VoiceCfg::cue_volume, snd_obj_cue_volume_set sets it */
    /* +0x04 */ s16 env_level;     /* starts at the sample's attack level */
    /* +0x06 */ s16 out_volume;    /* the mixed output volume, 0..127 */
    /* +0x08 */ s16 last_volume;   /* out_volume last sent to MIXSetInput, -1 at start */
    /* +0x0A */ u16 line;          /* the play parameters' line, snd_voice_volume_calc's level index */
    /* +0x0C */ f32 max_dist;      /* VoiceCfg::max_dist */
    /* +0x10 */ s16 pitch;         /* cents, clamped to the bend range */
    /* +0x12 */ s16 pan;
    /* +0x14 */ s16 aux_a;
    /* +0x16 */ s16 aux_b;
    /* +0x18 */ s16 aux_c;
    /* +0x1A */ u16 lpf_freq;      /* low-pass cutoff, at most 16000 */
    /* +0x1C */ u8 unused_1C[2];
    /* +0x1E */ u8 lpf_on;         /* the low-pass filter has been set up */
    /* +0x1F */ u8 unused_1F;
    /* +0x20 */ u32 loop_len;      /* frames, (u32)-1 when unset */
    /* +0x24 */ u32 release_count; /* frames, (u32)-1 when unset */
    /* +0x28 */ u8 unused_28[4];
    /* +0x2C */ u32 loop_pos;
    /* +0x30 */ s16 env_attack;
    /* +0x32 */ s16 env_decay;
    /* +0x34 */ s16 env_sustain;
    /* +0x36 */ s16 env_release;
    /* +0x38 */ s16 env_rate;
    /* +0x3A */ u8 state;          /* envelope state, 5 releases */
    /* +0x3B */ u8 unused_3B[5];
    /* +0x40 */ AxHandle* wave;    /* snd_line_wave_get's wave entry */
    /* +0x44 */ AXVPB* voice;
    /* +0x48 */ s16 priority;
    /* +0x4A */ s16 span;
    /* +0x4C */ s16 bend_up;       /* pitch upper limit */
    /* +0x4E */ s16 bend_down;     /* pitch lower limit (negated) */
    /* +0x50 */ s16 key_lo;
    /* +0x52 */ s16 key_hi;
    /* +0x54 */ u8 artic;
    /* +0x55 */ u8 unused_55;
    /* +0x56 */ u16 dist_scale;    /* VoiceCfg::dist_scale */
    /* +0x58 */ u16 flags3d;       /* VoiceCfg::flags: 2 volume, 4 pan, 8 span, 16 priority */
    /* +0x5A */ s16 fader;
    /* +0x5C */ Vec* pos;          /* the followed world position, or 0 */
} AxVoiceBlock; /* size: 0x60 */

/* One node of the voice pool: list links, flags, owner and key, then the voice state block. */
typedef struct SoundSlot {
    /* +0x00 */ u32 kind;            /* 0..4 */
    /* +0x04 */ struct SoundSlot* next; /* snd_voice_next */
    /* +0x08 */ struct SoundSlot* prev; /* snd_voice_prev */
    /* +0x0C */ u32 flags;           /* snd_slot_flag_set/clear/test */
    /* +0x10 */ u32 owner;           /* the sound object's play counter when it started */
    /* +0x14 */ u32 key;             /* line or 0xFF, the global flag, key_bank and key_id */
    /* +0x18 */ u8 priority;
    /* +0x19 */ u8 active;           /* must be 1 for the entry to be handled */
    /* +0x1A */ u8 line;
    /* +0x1B */ u8 unused_1B;
    /* +0x1C */ AxVoiceBlock ax;     /* snd_slot_state_get returns it */
    /* +0x7C */ u8 unused_7C[0x94];
} SoundSlot; /* size: 0x110 */

/* The voice-player interface the sound object implements (its table at 0x80597DA8: destroy, update, release,
 * voice_dropped, stop, pause, resume, start); declared for the virtual calls, its bodies are this unit's C rows. */
class SndPlayer {
public:
    virtual ~SndPlayer();
    virtual void update(SoundSlot* slot);
    virtual u32 release(SoundSlot** link);
    virtual void voice_dropped(AXVPB* vpb);
    virtual void stop(SoundSlot* slot);
    virtual void pause(SoundSlot* slot);
    virtual void resume(SoundSlot* slot);
    virtual void start(SoundSlot* slot);
}; /* size: 0x4 (the interface view: the table pointer at +0x00) */

/* The owner record snd_obj_voice_dropped updates. */
typedef struct SoundOwner {
    /* +0x00 */ u8 unused_00[0x14];
    /* +0x14 */ SoundSlot* slot;
} SoundOwner; /* size: 0x18 */

/* Two untyped words. */
typedef struct Pair32 {
    /* +0x00 */ u32 a;
    /* +0x04 */ u32 b;
} Pair32; /* size: 0x8 */

/* A vtable pointer. */
typedef struct VtblHolder {
    /* +0x00 */ const void* vtable;
} VtblHolder; /* size: 0x4 */

/* One entry of a slot table: a slot id and its weight. */
typedef struct SlotTableEntry {
    /* +0x00 */ u8 id;
    /* +0x01 */ u8 weight;
} SlotTableEntry; /* size: 0x2 */

/* A weighted slot table (`snd_obj_cue_random` rolls against it). */
typedef struct SlotTable {
    /* +0x00 */ u32 count;
    /* +0x04 */ SlotTableEntry entries[1];
} SlotTable; /* size: 0x8 (variable) */

/* One cue of a line (`snd_line_cue_get` returns it): how its voices are keyed, mixed and positioned. */
typedef struct VoiceCfg {
    /* +0x00 */ u8 kind;            /* snd_obj_play_cue dispatches on it */
    /* +0x01 */ u8 volume_random;
    /* +0x02 */ u8 table_idx;       /* snd_obj_cue_random's slot table */
    /* +0x03 */ u8 priority;
    /* +0x04 */ u16 pitch_random;
    /* +0x06 */ u16 flags;          /* 1 global key, 2 volume, 4 pan, 8 span, 16 priority, 32 follow */
    /* +0x08 */ u8 artic;           /* the regions' articulation index */
    /* +0x09 */ u8 note;
    /* +0x0A */ u8 key_id;
    /* +0x0B */ u8 key_bank;
    /* +0x0C */ u8 cue_volume;
    /* +0x0D */ u8 pan;
    /* +0x0E */ s16 fine;
    /* +0x10 */ u32 loop_len;
    /* +0x14 */ f32 max_dist;
    /* +0x18 */ u16 release_time;
    /* +0x1A */ u16 lpf_freq;
    /* +0x1C */ u8 max_voices;      /* 255: no limit */
    /* +0x1D */ u8 next_cue;        /* next cue in the chain, 255 ends it */
    /* +0x1E */ u16 dist_scale;
    /* +0x20 */ u8 fader;
    /* +0x21 */ u8 aux_a;
    /* +0x22 */ u8 aux_b;
    /* +0x23 */ u8 aux_c;
} VoiceCfg; /* size: 0x24 */

/* One region of a line (`snd_line_region_get`, stride 0x14): the key range a sample plays over. */
typedef struct SndRegion {
    /* +0x00 */ u8 priority;
    /* +0x01 */ u8 volume;
    /* +0x02 */ u8 pan;
    /* +0x03 */ u8 unused_03;
    /* +0x04 */ s16 coarse;
    /* +0x06 */ s16 fine;
    /* +0x08 */ s16 key_lo;
    /* +0x0A */ s16 key_hi;
    /* +0x0C */ s16 bend_down;
    /* +0x0E */ s16 bend_up;
    /* +0x10 */ u8 sample;          /* snd_line_sample_get's index */
    /* +0x11 */ u8 artic;           /* snd_line_artic_get's index, matched against VoiceCfg::artic */
    /* +0x12 */ u8 unused_12[2];
} SndRegion; /* size: 0x14 */

/* One articulation of a line (`snd_line_artic_get`, stride 8). */
typedef struct SndArtic {
    /* +0x00 */ u8 volume;
    /* +0x01 */ u8 pan;
    /* +0x02 */ u8 unused_02[2];
    /* +0x04 */ s16 coarse;
    /* +0x06 */ s16 fine;
} SndArtic; /* size: 0x8 */

/* One sample of a line (`snd_line_sample_get`): its wave, tuning, mix and envelope. */
typedef struct SndSample {
    /* +0x00 */ s16 wave;           /* snd_line_wave_get's index */
    /* +0x02 */ s16 coarse;
    /* +0x04 */ s16 fine;
    /* +0x06 */ s16 root;
    /* +0x08 */ u8 volume;
    /* +0x09 */ u8 pan;
    /* +0x0A */ u8 attack;
    /* +0x0B */ u8 decay;
    /* +0x0C */ u8 sustain;
    /* +0x0D */ u8 release;
    /* +0x0E */ u8 rate;
} SndSample; /* size: 0xF (approximate) */

/* The play parameters snd_obj_play_pan/pos/pos_reset build for snd_obj_play_chain. */
typedef struct SndPlayParam {
    /* +0x00 */ u32 line;
    /* +0x04 */ s32 volume;
    /* +0x08 */ s32 pan;
    /* +0x0C */ s32 unused_0C;
    /* +0x10 */ s32 span;
    /* +0x14 */ s32 priority;       /* 0: the region's own */
    /* +0x18 */ Vec* pos;           /* the followed world position, or 0 */
} SndPlayParam; /* size: 0x1C */

extern u8 lbl_80597DF8[0x28]; /* the base vtable snd_base_construct installs */
extern u8 lbl_80597DA8[0x28]; /* the sound object's vtable (snd_obj_construct) */
extern u8 lbl_80597DD0[0x28]; /* the player base vtable (snd_player_base_construct) */
extern u8 g_snd_obj[0xE60];             /* the global sound object (snd_obj_get) */

extern "C" {
ReverbBank* snd_obj_destroy(ReverbBank* bank, s32 flags);
void* snd_player_base_destruct(void* obj, s32 flags);
void* snd_base_destruct(void* obj, s32 flags);
void* snd_line_destruct(void* obj, s32 flags);
void* snd_artic_table_release(void* obj, s32 flags);
void* snd_region_table_release(void* obj, s32 flags);
void* snd_reloc_state_release(void* obj, s32 flags);
void snd_voice_drop_cb(AXVPB* vpb);
Sound* snd_obj_get(void);
void snd_voice_setup(AxVoiceBlock* ax, SndArtic* artic, SndRegion* region, SndSample* sample, VoiceCfg* cfg,
                 SndPlayParam* param, Sound* snd);
s16 snd_voice_volume_calc(AxVoiceBlock* ax);

void __destroy_arr(void* array, void* dtor, int size, int count);
void __dl__FPv(void* p);

void snd_obj_voice_dropped(void* unused, SoundOwner* owner);
void snd_slot_flag_set(SoundSlot* slot, u32 mask);
void snd_slot_flag_clear(SoundSlot* slot, u32 mask);
AxVoiceBlock* snd_slot_state_get(SoundSlot* slot);
void snd_cmd_list_run(Sound2* self);
u32 snd_cmd_list_end_ck(u8* list, u8* entry);
u32 snd_cmd_iter_eq(u8* a, u8* b);
u8* snd_cmd_list_end(u8* list);
u32 snd_voice_list_end_ck(u8* list, u8* entry);
u32 snd_voice_iter_eq(u8* a, u8* b);
u8* snd_voice_list_end(u8* list);
SoundSlot* snd_voice_next(SoundSlot* voice);
SlotEntry* snd_cmd_entry_info(u8* entry);
u8* snd_cmd_list_get(void);
u32 snd_obj_release(Sound* snd, SoundSlot** link);
u32 snd_slot_flag_test(SoundSlot* slot, u32 mask);
void snd_obj_update(Sound2* self, SoundSlot* slot);
void snd_voice_3d_update(Sound* snd, AxVoiceBlock* ax);
void* fn_800E9B5C(u8* list, void* entry);
void* fn_800E9BE4(u8* list);

void snd_obj_start(void* unused, SoundSlot* slot);
u32 snd_reverb_flags_get(ReverbMgr* mgr);
ReverbMgr* snd_reverb_mgr_get(void);
void snd_cmd_post(void* unused, u32 owner, u32 kind, u8 a, u8 b, u8 c, u8 d, s16 e, s16 f);
SoundSlot* snd_voice_prev(SoundSlot* p);
u32 snd_voice_list_head_ck(u8* list, u8* entry);
u32 snd_voice_head_eq(u8* a, u8* b);
void snd_obj_cue_release(void* unused, VoiceCfg* cfg, u32 arg);
void snd_obj_cue_stop(Sound2* self, VoiceCfg* cfg, u32 arg);
void snd_obj_cue_random(Sound2* self, ReverbLine* line, VoiceCfg* cfg, u32 line_idx, SndPlayParam* param);
void* snd_line_slot_table_get(void* p, u32 idx);
void* snd_reloc_state_entry(RelocState* state, u32 idx);
Sound* snd_obj_static_init(void);
void snd_reloc_state_clear(Pair32* p);
void snd_region_table_clear(Pair32* p);
void snd_artic_table_clear(Pair32* p);
f32 snd_deg_to_rad(f32 v);
void snd_base_construct(VtblHolder* p);

void* fn_800E9960(u8* list);

void snd_obj_stop(void* unused, SoundSlot* slot);
void snd_obj_pause(void* unused, SoundSlot* slot);
void snd_obj_resume(void* unused, SoundSlot* slot);
u32 snd_obj_owner_voice_count(void* unused, u32 key);
void snd_obj_reverb_apply(Sound* snd, u32 idx, u32 entry_idx);
void* snd_line_reverb_get(void* p, u32 idx);
void snd_obj_cue_volume_set(void* unused, SoundSlot* slot, s16 volume);
void snd_obj_pitch_set(void* unused, SoundSlot* slot, s16 pitch);
s16 snd_obj_owner_pitch_get(void* unused, u32 key);
void snd_obj_owner_lpf_set(void* unused, u32 key, u16 pitch);
Sound* snd_obj_construct(Sound* snd);
ReverbLine* snd_line_construct(ReverbLine* line);
Sound* snd_player_base_construct(Sound* snd);

void fn_800E8E60(AxVoice* voice, u32 a, s16 b);

void __construct_array(void* array, void* ctor, void* dtor, int size, int count);

void* snd_line_cue_get(void* p, u32 idx);
VoiceCfg* snd_reloc_cue_get(RelocState* state, u32 idx);
SndArtic* snd_reloc_artic_get(RelocState* state, u32 idx);
AxHandle* snd_reloc_wave_get(RelocState* state, u32 idx);
SndSample* snd_reloc_sample_get(RelocState* state, u32 idx);
SndRegion* snd_reloc_region_get(RelocState* state, u32 idx);
ReverbCfg* snd_reloc_reverb_get(RelocState* state, u32 idx);
void snd_obj_play_cue(Sound2* self, ReverbLine* line, VoiceCfg* cfg, u32 line_idx, SndPlayParam* param);
void* snd_line_artic_get(void* p, u32 idx);
void* snd_line_wave_get(void* p, u32 idx);
void* snd_line_sample_get(void* p, u32 idx);
void* snd_line_region_get(void* p, u32 idx);

void snd_obj_play_regions(Sound* snd, ReverbLine* line, VoiceCfg* cfg, u32 line_idx, SndPlayParam* param);
u32 snd_obj_voice_limit(Sound* snd, VoiceCfg* cfg, u32 line_idx);

u32 snd_obj_play_chain(Sound* snd, u32 line_idx, u32 id, SndPlayParam* param);

}


#pragma peephole off

/* Destroys a reverb bank's 49 lines, then the bank itself. */
extern "C" ReverbBank* snd_obj_destroy(ReverbBank* bank, s32 flags)
{
    if (bank != 0) {
        __destroy_arr(&bank->lines, (void*)snd_line_destruct, sizeof(ReverbLine), 49);
        snd_player_base_destruct(bank, 0);
        if ((s16)flags > 0) {
            __dl__FPv(bank);
        }
    }
    return bank;
}

/* Destroys one delay line's own members, then frees it. */
extern "C" void* snd_player_base_destruct(void* obj, s32 flags)
{
    if (obj != 0) {
        snd_base_destruct(obj, 0);
        if ((s16)flags > 0) {
            __dl__FPv(obj);
        }
    }
    return obj;
}

/* Frees an object if the delete flag asks for it. */
extern "C" void* snd_base_destruct(void* obj, s32 flags)
{
    if (obj != 0 && (s16)flags > 0) {
        __dl__FPv(obj);
    }
    return obj;
}

/* Destroys the nine parts of one delay line, then frees the line. */
extern "C" void* snd_line_destruct(void* obj, s32 flags)
{
    ReverbLine* line = (ReverbLine*)obj;

    if (line != 0) {
        snd_reloc_state_release(&line->part[8], -1);
        snd_reloc_state_release(&line->part[7], -1);
        snd_reloc_state_release(&line->part[6], -1);
        snd_reloc_state_release(&line->part[5], -1);
        snd_reloc_state_release(&line->part[4], -1);
        snd_reloc_state_release(&line->part[3], -1);
        snd_reloc_state_release(&line->part[2], -1);
        snd_region_table_release(&line->part[1], -1);
        snd_artic_table_release(&line->part[0], -1);
        if ((s16)flags > 0) {
            __dl__FPv(line);
        }
    }
    return line;
}

/* Frees an object if the delete flag asks for it. */
extern "C" void* snd_artic_table_release(void* obj, s32 flags)
{
    if (obj != 0 && (s16)flags > 0) {
        __dl__FPv(obj);
    }
    return obj;
}

/* Frees an object if the delete flag asks for it. */
extern "C" void* snd_region_table_release(void* obj, s32 flags)
{
    if (obj != 0 && (s16)flags > 0) {
        __dl__FPv(obj);
    }
    return obj;
}

/* Frees an object if the delete flag asks for it. */
extern "C" void* snd_reloc_state_release(void* obj, s32 flags)
{
    if (obj != 0 && (s16)flags > 0) {
        __dl__FPv(obj);
    }
    return obj;
}

/* Passes an argument to the global sound object's fifth virtual method. */
extern "C" void snd_voice_drop_cb(AXVPB* vpb)
{
    Sound* snd = snd_obj_get();

    ((SndPlayer*)snd)->voice_dropped(vpb);
}

/* The global sound object. */
extern "C" Sound* snd_obj_get(void)
{
    return (Sound*)g_snd_obj;
}

/* Initialises a new voice's state block from its articulation, region, sample, cue and play parameters. */
extern "C" void snd_voice_setup(AxVoiceBlock* ax, SndArtic* artic, SndRegion* region, SndSample* sample, VoiceCfg* cfg,
                            SndPlayParam* param, Sound* snd)
{
    f32 master;
    f32 level;
    f32 artic_volume;
    f32 region_volume;
    f32 sample_volume;
    s16 random;
    s32 pitch;
    s32 pan;
    u16 bend;

    ax->pos = param->pos;
    ax->key_lo = region->key_lo;
    ax->key_hi = region->key_hi;
    ax->artic = region->artic;
    ax->loop_len = (cfg->loop_len != (u32)-1) ? (u32)(5.555556f * cfg->loop_len) : cfg->loop_len;
    ax->release_count = (cfg->release_time != 0xFFFF) ? (u32)(5.555556f * cfg->release_time) : 0;
    ax->dist_scale = cfg->dist_scale;
    ax->flags3d = cfg->flags;
    ax->loop_pos = 0;
    ax->env_level = ax->env_attack = sample->attack;
    ax->env_decay = sample->decay;
    ax->env_sustain = sample->sustain;
    ax->env_release = (s8)sample->release;
    ax->env_rate = (s16)(0.47f * (0.6f * (0.14f * sample->rate)));
    if (ax->env_rate <= 0) {
        ax->env_rate = 1;
    }
    ax->state = 0;
    artic_volume = artic->volume / 127.0f;
    region_volume = region->volume / 127.0f;
    sample_volume = sample->volume / 127.0f;
    random = 0;
    ax->cue_volume = cfg->cue_volume;
    ax->last_volume = -1;
    ax->line = param->line;
    if ((s8)cfg->volume_random > 0) {
        random = (u8)stream_rand() % (cfg->volume_random * 2) - cfg->volume_random;
    }
    ax->volume = (s16)((f32)(random + 127) * (sample_volume * (artic_volume * region_volume)));
    if (ax->volume > 127) {
        ax->volume = 127;
    } else if (ax->volume < 0) {
        ax->volume = 0;
    }
    ax->out_volume = (s16)(param->volume / 127.0f * (f32)(s16)snd_voice_volume_calc(ax));
    if (ax->out_volume > 127) {
        ax->out_volume = 127;
    } else if (ax->out_volume < 0) {
        ax->out_volume = 0;
    }
    ax->bend_up = region->bend_up;
    ax->bend_down = region->bend_down;
    pitch = cfg->fine;
    pitch += artic->fine + artic->coarse * 100;
    pitch += region->fine + region->coarse * 100;
    pitch += sample->fine + sample->coarse * 100;
    pitch += (cfg->note - sample->root) * 100;
    bend = cfg->pitch_random;
    if ((s16)bend > 0) {
        pitch += ((u8)stream_rand() << 5) % (bend * 2) - bend;
    }
    if (pitch > ax->bend_up) {
        pitch = ax->bend_up;
    } else if (pitch < -ax->bend_down) {
        pitch = -ax->bend_down;
    }
    ax->pitch = pitch;
    pan = cfg->pan - 64;
    pan += artic->pan - 64;
    pan += region->pan - 64;
    pan += sample->pan - 64;
    pan += param->pan;
    if (pan > 127) {
        pan = 127;
    } else if (pan < 0) {
        pan = 0;
    }
    ax->pan = pan;
    ax->span = param->span;
    if (ax->span > 127) {
        ax->span = 127;
    } else if (ax->span < 0) {
        ax->span = 0;
    }
    master = snd->field_71 / 127.0f;
    level = cfg->aux_a / 127.0f;
    ax->aux_a = (s16)(127.0f * (master * (level * (snd->field_6E / 127.0f))));
    if (ax->aux_a > 127) {
        ax->aux_a = 127;
    } else if (ax->aux_a < 0) {
        ax->aux_a = 0;
    }
    level = cfg->aux_b / 127.0f;
    ax->aux_b = (s16)(127.0f * (master * (level * (snd->field_6F / 127.0f))));
    if (ax->aux_b > 127) {
        ax->aux_b = 127;
    } else if (ax->aux_b < 0) {
        ax->aux_b = 0;
    }
    level = cfg->aux_c / 127.0f;
    ax->aux_c = (s16)(127.0f * (master * (level * (snd->field_70 / 127.0f))));
    if (ax->aux_c > 127) {
        ax->aux_c = 127;
    } else if (ax->aux_c < 0) {
        ax->aux_c = 0;
    }
    ax->fader = cfg->fader;
    if (ax->fader > 127) {
        ax->fader = 127;
    } else if (ax->fader < 0) {
        ax->fader = 0;
    }
    ax->lpf_freq = cfg->lpf_freq;
    if (ax->lpf_freq == 0) {
        ax->lpf_on = 0;
    } else {
        ax->lpf_on = 1;
    }
    if (ax->lpf_freq > 16000) {
        ax->lpf_freq = 16000;
    }
    ax->max_dist = cfg->max_dist;
}

/* Scales the three global level factors and the config's own levels into a 0..127 voice volume. */
extern "C" s16 snd_voice_volume_calc(AxVoiceBlock* ax)
{
    f32 a = (f32)(s16)snd_level_volume_get() / 127.0f;
    f32 b = (f32)(s16)fn_800E8594(ax->line) / 127.0f;
    f32 c = (f32)(s16)fn_800E852C() / 127.0f;
    s16 v = a * c * b * ((f32)ax->cue_volume / 127.0f) * ((f32)ax->env_level / 127.0f) * ((f32)ax->volume / 127.0f) *
            127.0f;

    if (v > 127) {
        v = 127;
    } else if (v < 0) {
        v = 0;
    }
    return v;
}

/* Clears one sound slot's voice block and marks it unowned. */
extern "C" void snd_obj_voice_dropped(void* unused, SoundOwner* owner)
{
    SoundSlot* slot = owner->slot;
    AxVoiceBlock* ax = snd_slot_state_get(slot);

    ax->voice = 0;
    ax->wave = 0;
    snd_slot_flag_clear(slot, 1);
    snd_slot_flag_set(slot, 1024);
}

/* Sets bits in a sound slot's flag word. */
extern "C" void snd_slot_flag_set(SoundSlot* slot, u32 mask)
{
    slot->flags |= mask;
}

/* Clears bits in a sound slot's flag word. */
extern "C" void snd_slot_flag_clear(SoundSlot* slot, u32 mask)
{
    slot->flags &= ~mask;
}

/* The AX voice block of a sound slot. */
extern "C" AxVoiceBlock* snd_slot_state_get(SoundSlot* slot)
{
    return &slot->ax;
}

/* Dispatches every active voice of a slot to the handler its kind selects. */
extern "C" void snd_cmd_list_run(Sound2* self)
{
    SoundSlot* iter = (SoundSlot*)fn_800E9BE4(snd_cmd_list_get());

    while (snd_cmd_list_end_ck((u8*)snd_cmd_list_get(), (u8*)iter) != 1) {
        SlotEntry* info = snd_cmd_entry_info((u8*)iter);
        SoundSlot* voice = (SoundSlot*)fn_800EDB24_c1(snd_bank_table_get());

        while (snd_voice_list_end_ck((u8*)snd_bank_table_get(), (u8*)voice) != 1) {
            if (voice->active == 1 && voice->owner == info->owner) {
                switch (info->kind) {
                case 0: ((SndPlayer*)self)->stop(voice); break;
                case 1: ((SndPlayer*)self)->pause(voice); break;
                case 2: ((SndPlayer*)self)->resume(voice); break;
                case 3: snd_obj_cue_volume_set(self, voice, info->field_0C); break;
                case 4: snd_obj_pitch_set(self, voice, info->field_0E); break;
                }
            }
            voice = snd_voice_next(voice);
        }
        iter = (SoundSlot*)fn_800E9B5C(snd_cmd_list_get(), iter);
    }
}

/* Whether a slot list has reached its end marker. */
extern "C" u32 snd_cmd_list_end_ck(u8* list, u8* entry)
{
    return snd_cmd_iter_eq(snd_cmd_list_end(list), entry);
}

/* Whether two addresses are equal. */
extern "C" u32 snd_cmd_iter_eq(u8* a, u8* b)
{
    return b == a;
}

/* The end marker of a slot list. */
extern "C" u8* snd_cmd_list_end(u8* list)
{
    return list + 76;
}

/* Whether a voice list has reached its end marker. */
extern "C" u32 snd_voice_list_end_ck(u8* list, u8* entry)
{
    return snd_voice_iter_eq(snd_voice_list_end(list), entry);
}

/* Whether two addresses are equal. */
extern "C" u32 snd_voice_iter_eq(u8* a, u8* b)
{
    return b == a;
}

/* The end marker of a voice list. */
extern "C" u8* snd_voice_list_end(u8* list)
{
    return list + 272;
}

/* The next voice in a list. */
extern "C" SoundSlot* snd_voice_next(SoundSlot* voice)
{
    return voice->next;
}

/* The slot info block of one slot list entry. */
extern "C" SlotEntry* snd_cmd_entry_info(u8* entry)
{
    return (SlotEntry*)(entry + 12);
}

/* The global slot list. */
extern "C" u8* snd_cmd_list_get(void)
{
    return lbl_80698AF0;
}

/* Frees the voice a slot still holds when its flags say it is finished, unlinking the slot. */
extern "C" u32 snd_obj_release(Sound* snd, SoundSlot** link)
{
    AxVoiceBlock* ax = snd_slot_state_get(*link);

    if (snd_slot_flag_test(*link, 1024)) {
        if (ax->voice != 0) {
            AXFreeVoice(ax->voice);
        }
        *link = (SoundSlot*)fn_800EDA88_c1(snd_bank_table_get(), *link);
        return 1;
    }
    if (snd_slot_flag_test(*link, 512) != 1 && snd_slot_flag_test(*link, 256) != 0 && ax->voice->pb.running == 0) {
        AXFreeVoice(ax->voice);
        *link = (SoundSlot*)fn_800EDA88_c1(snd_bank_table_get(), *link);
        return 1;
    }
    return 0;
}

/* Tests whether any of a sound slot's flag bits is set. */
extern "C" u32 snd_slot_flag_test(SoundSlot* slot, u32 mask)
{
    return (slot->flags & mask) != 0;
}

/* Advances a slot's voice timers and re-dispatches it. */
extern "C" void snd_obj_update(Sound2* self, SoundSlot* slot)
{
    AxVoiceBlock* ax = snd_slot_state_get(slot);

    if (ax->release_count != (u32)-1 && ax->release_count != 0) {
        ax->release_count--;
    }
    if (ax->loop_len != (u32)-1) {
        if (++ax->loop_pos >= ax->loop_len && ax->state != 0) {
            ax->state = 5;
            ax->loop_len = (u32)-1;
        }
    }
    snd_envelope_update_c1(self, slot, ax);
    snd_voice_3d_update((Sound*)self, ax);
}

/* Recomputes a positioned voice's distance volume, pan, span and priority, then its output volume. */
extern "C" void snd_voice_3d_update(Sound* snd, AxVoiceBlock* ax)
{
    s32 volume = 127;
    s32 dual;
    Vec rel;
    Vec rel2;
    f32 angle;
    f32 dist;
    f32 atten;
    f32 angle2;
    f32 dist2;
    f32 atten2;

    if (ax->pos != 0) {
        atten = 0.0f;
        dual = 0;
        atten2 = 0.0f;
        snd_listener_polar((const f32(*)[4])snd->levels2[0], ax->pos, &rel, &angle, &dist);
        snd_distance_atten(&ax->max_dist, &dist, &atten);
        if (snd->levels2[1] != 0) {
            dual = 1;
            snd_listener_polar((const f32(*)[4])snd->levels2[1], ax->pos, &rel2, &angle2, &dist2);
            snd_distance_atten(&ax->max_dist, &dist2, &atten2);
        }
        if (ax->flags3d & 2) {
            volume = snd_distance_volume(127, dist, 0.001f * ax->dist_scale);
        }
        if (ax->flags3d & 4) {
            if (dual == 0) {
                ax->pan = snd_pan_calc(&rel, dist, angle, snd->field_88, snd->field_8C, atten);
                MIXSetPan(ax->voice, ax->pan);
            } else {
                s16 pan = snd_pan_calc(&rel, dist, angle, snd->field_88, snd->field_8C, atten);
                ax->pan = pan + ((s16)snd_pan_calc(&rel2, dist2, angle2, snd->field_88, snd->field_8C, atten2) - pan) / 2;
                MIXSetPan(ax->voice, ax->pan);
            }
        }
        if (ax->flags3d & 8) {
            if (dual == 0) {
                ax->span = snd_span_calc(&rel, dist, snd->field_88, atten);
                MIXSetSPan(ax->voice, ax->span);
            } else {
                ax->span = 127;
                MIXSetSPan(ax->voice, 127);
            }
        }
        if (ax->flags3d & 16) {
            ax->priority = snd_distance_priority(dist, 0.001f * ax->dist_scale);
            AXSetVoicePriority(ax->voice, ax->priority);
        }
    }
    ax->out_volume = (s16)(volume / 127.0f * (f32)(s16)snd_voice_volume_calc(ax));
    if (ax->out_volume != ax->last_volume) {
        if (ax->out_volume > 127) {
            ax->out_volume = 127;
        } else if (ax->out_volume < 0) {
            ax->out_volume = 0;
        }
        MIXSetInput(ax->voice, snd_level_to_db(ax->out_volume));
    }
    ax->last_volume = ax->out_volume;
}

/* Starts a slot's voice from its AX block. */
extern "C" void snd_obj_start(void* unused, SoundSlot* slot)
{
    AxVoiceBlock* ax = snd_slot_state_get(slot);

    snd_voice_prepare(ax->wave, (AxVoice*)ax->voice, ax->pitch, ax->lpf_freq);
    MIXInitChannel(ax->voice, snd_reverb_flags_get(snd_reverb_mgr_get()), snd_level_to_db(ax->out_volume),
                   snd_level_to_db(ax->aux_a), snd_level_to_db(ax->aux_b), snd_level_to_db(ax->aux_c), ax->pan, ax->span,
                   snd_level_to_db(ax->fader));
    AXSetVoiceState(ax->voice, 1);
    snd_slot_flag_set(slot, 256);
    ax->state = 1;
}

/* The manager's pending-bit word. */
extern "C" u32 snd_reverb_flags_get(ReverbMgr* mgr)
{
    return mgr->flags;
}

/* The global reverb manager. */
extern "C" ReverbMgr* snd_reverb_mgr_get(void)
{
    return &snd_reverb_mgr;
}

/* Fills one slot-list entry's info block. */
extern "C" void snd_cmd_post(void* unused, u32 owner, u32 kind, u8 a, u8 b, u8 c, u8 d, s16 e, s16 f)
{
    u8* entry;
    SlotEntry* info;

    if (owner == 0) {
        return;
    }
    entry = (u8*)fn_800E9960(snd_cmd_list_get());
    if (entry == 0) {
        return;
    }
    info = snd_cmd_entry_info(entry);
    info->owner = owner;
    info->kind = kind;
    info->field_08 = a;
    info->field_09 = b;
    info->field_0A = c;
    info->field_0B = d;
    info->field_0C = e;
    info->field_0E = f;
}

/* Plays a cue on one line at a fixed pan position. */
extern "C" u32 snd_obj_play_pan(Sound* snd, u32 line, u32 id, u32 pan)
{
    SndPlayParam param = { line, 127, 0, 0, 127, 0, 0 };

    param.pan = (s32)(127.0f * ((f32)(s32)pan / (f32)(s32)snd->field_74)) + 64;
    return snd_obj_play_chain(snd, line, id, &param);
}

/* Plays a cue on one line at a world position, deriving its volume, pan, span and priority from the listener. */
extern "C" u32 snd_obj_play_pos(Sound* snd, u32 line, u32 id, u32 pos, u32 follow)
{
    SndPlayParam param = { line, 127, 0, 0, 127, 0, 0 };
    Vec rel;
    f32 angle;
    f32 dist;
    f32 atten = 0.0f;
    VoiceCfg* cfg;

    snd_listener_polar((const f32(*)[4])snd->levels2[0], (Vec*)pos, &rel, &angle, &dist);
    cfg = (VoiceCfg*)snd_line_cue_get(&snd->lines[line], id);
    if (cfg == 0) {
        return 0;
    }
    snd_distance_atten(&cfg->max_dist, &dist, &atten);
    param.volume = (cfg->flags & 2) ? snd_distance_volume(127, dist, 0.001f * cfg->dist_scale) : 127;
    param.pan = (cfg->flags & 4) ? snd_pan_calc(&rel, dist, angle, snd->field_88, snd->field_8C, atten) : 64;
    param.span = (cfg->flags & 8) ? snd_span_calc(&rel, dist, snd->field_88, atten) : 127;
    param.priority = (cfg->flags & 16) ? snd_distance_priority(dist, 0.001f * cfg->dist_scale) : 0;
    if (follow == 1 && (cfg->flags & 32)) {
        param.pos = (Vec*)pos;
    }
    return snd_obj_play_chain(snd, line, id, &param);
}

/* Looks up one cue of a line. */
extern "C" void* snd_line_cue_get(void* p, u32 idx)
{
    return snd_reloc_cue_get(&((RelocGroup*)p)->state[8], idx);
}

/* One cue of a cue table, or 0 past its end. */
extern "C" VoiceCfg* snd_reloc_cue_get(RelocState* state, u32 idx)
{
    if (state->table == 0) {
        return 0;
    }
    if (state->table->count <= idx) {
        return 0;
    }
    return (VoiceCfg*)state->entries[idx];
}

/* Plays a cue on one line at a world position with its cue volume cleared. */
extern "C" u32 snd_obj_play_pos_reset(Sound* snd, u32 line, u32 id, u32 pos, u32 follow)
{
    SndPlayParam param = { line, 127, 0, 0, 127, 0, 0 };
    Vec rel;
    f32 angle;
    f32 dist;
    f32 atten = 0.0f;
    VoiceCfg* cfg;

    snd_listener_polar((const f32(*)[4])snd->levels2[0], (Vec*)pos, &rel, &angle, &dist);
    cfg = (VoiceCfg*)snd_line_cue_get(&snd->lines[line], id);
    if (cfg == 0) {
        return 0;
    }
    cfg->cue_volume = 0;
    snd_distance_atten(&cfg->max_dist, &dist, &atten);
    param.volume = (cfg->flags & 2) ? snd_distance_volume(127, dist, 0.001f * cfg->dist_scale) : 127;
    param.pan = (cfg->flags & 4) ? snd_pan_calc(&rel, dist, angle, snd->field_88, snd->field_8C, atten) : 64;
    param.span = (cfg->flags & 8) ? snd_span_calc(&rel, dist, snd->field_88, atten) : 127;
    param.priority = (cfg->flags & 16) ? snd_distance_priority(dist, 0.001f * cfg->dist_scale) : 0;
    if (follow == 1 && (cfg->flags & 32)) {
        param.pos = (Vec*)pos;
    }
    return snd_obj_play_chain(snd, line, id, &param);
}

/* Plays a slot chain through one of the object's delay lines. */
extern "C" u32 snd_obj_play_chain(Sound* snd, u32 a, u32 b, SndPlayParam* c)
{
    VoiceCfg* cfg;
    ReverbLine* line = &snd->lines[a];

    cfg = (VoiceCfg*)snd_line_cue_get(line, b);

    if (cfg == 0) {
        return 0;
    }
    snd->field_78++;
    snd_obj_play_cue((Sound2*)snd, line, cfg, a, c);
    if (cfg->next_cue == 255) {
        return snd->field_78;
    }
    do {
        cfg = (VoiceCfg*)snd_line_cue_get(line, cfg->next_cue);
        if (cfg == 0) {
            return snd->field_78;
        }
        snd_obj_play_cue((Sound2*)snd, line, cfg, a, c);
    } while (cfg->next_cue != 255);
    return snd->field_78;
}

/* Dispatches a voice to the handler its kind selects. */
extern "C" void snd_obj_play_cue(Sound2* self, ReverbLine* a, VoiceCfg* cfg, u32 c, SndPlayParam* d)
{
    u8 kind = cfg->kind;

    if (kind == 1) {
        snd_obj_play_regions((Sound*)self, a, cfg, c, d);
        return;
    }
    if (kind == 2) {
        snd_obj_cue_release(self, cfg, c);
        return;
    }
    if (kind == 3) {
        snd_obj_cue_stop(self, cfg, c);
        return;
    }
    if (kind == 4) {
        snd_obj_cue_random(self, a, cfg, c, d);
    }
}

/* Starts one voice for every region of a line whose key group and key range cover the cue. */
extern "C" void snd_obj_play_regions(Sound* snd, ReverbLine* line, VoiceCfg* cfg, u32 line_idx, SndPlayParam* param)
{
    u32 i;

    if (snd_obj_voice_limit(snd, cfg, line_idx) == 0) {
        return;
    }
    for (i = 0; i < snd_ref_table_count((SndRefTable*)line, 3); i++) {
        SndRegion* region = (SndRegion*)snd_line_region_get(line, i);
        SndSample* sample;
        SoundSlot* slot;
        AxVoiceBlock* ax;
        u32 priority;

        if (region->artic > cfg->artic) {
            return;
        }
        if (region->key_lo > cfg->note || cfg->note > region->key_hi || region->artic != cfg->artic) {
            continue;
        }
        sample = (SndSample*)snd_line_sample_get(line, region->sample);
        if (sample == 0) {
            continue;
        }
        slot = (SoundSlot*)snd_voice_alloc((SndPool2*)snd_bank_table_get());
        if (slot == 0) {
            continue;
        }
        slot->active = 1;
        snd_slot_flag_set(slot, 1);
        slot->owner = snd->field_78;
        slot->priority = cfg->priority;
        slot->line = line_idx;
        if (cfg->flags & 1) {
            slot->key = ((u32)cfg->key_bank << 8) | (cfg->key_id | 0xFF010000);
        } else {
            u32 key = cfg->key_id | (line_idx << 24);

            slot->key = key | ((u32)cfg->key_bank << 8);
        }
        ax = snd_slot_state_get(slot);
        ax->wave = (AxHandle*)snd_line_wave_get(line, sample->wave);
        priority = param->priority;
        if ((s32)priority <= 0) {
            priority = region->priority;
        }
        ax->voice = AXAcquireVoice(priority, snd_voice_drop_cb, (u32)slot);
        if (ax->voice != 0 && ax->wave != 0) {
            snd_voice_setup(ax, (SndArtic*)snd_line_artic_get(line, region->artic), region, sample, cfg, param, snd);
        } else {
            if (ax->voice != 0) {
                AXFreeVoice(ax->voice);
            }
            snd_voice_free((SndNode2*)snd_bank_table_get(), (SndNode2*)slot);
        }
    }
}

/* Looks up one articulation of a line. */
extern "C" void* snd_line_artic_get(void* p, u32 idx)
{
    return snd_reloc_artic_get(&((RelocGroup*)p)->state[0], idx);
}

/* One articulation of an articulation table, or 0 past its end. */
extern "C" SndArtic* snd_reloc_artic_get(RelocState* state, u32 idx)
{
    if (state->table == 0) {
        return 0;
    }
    if (state->table->count <= idx) {
        return 0;
    }
    return &((SndArtic*)state->entries)[idx];
}

/* Looks up one wave of a line. */
extern "C" void* snd_line_wave_get(void* p, u32 idx)
{
    return snd_reloc_wave_get(&((RelocGroup*)p)->state[3], idx);
}

/* One wave of a wave table, or 0 past its end. */
extern "C" AxHandle* snd_reloc_wave_get(RelocState* state, u32 idx)
{
    if (state->table == 0) {
        return 0;
    }
    if (state->table->count <= idx) {
        return 0;
    }
    return (AxHandle*)state->entries[idx];
}

/* Looks up one sample of a line. */
extern "C" void* snd_line_sample_get(void* p, u32 idx)
{
    return snd_reloc_sample_get(&((RelocGroup*)p)->state[2], idx);
}

/* One sample of a sample table, or 0 past its end. */
extern "C" SndSample* snd_reloc_sample_get(RelocState* state, u32 idx)
{
    if (state->table == 0) {
        return 0;
    }
    if (state->table->count <= idx) {
        return 0;
    }
    return (SndSample*)state->entries[idx];
}

/* Looks up one region of a line. */
extern "C" void* snd_line_region_get(void* p, u32 idx)
{
    return snd_reloc_region_get(&((RelocGroup*)p)->state[1], idx);
}

/* One region of a region table, or 0 past its end. */
extern "C" SndRegion* snd_reloc_region_get(RelocState* state, u32 idx)
{
    if (state->table == 0) {
        return 0;
    }
    if (state->table->count <= idx) {
        return 0;
    }
    return &((SndRegion*)state->entries)[idx];
}

/* Whether a cue may start another voice; releases the oldest same-key voices when it is over its limit. */
extern "C" u32 snd_obj_voice_limit(Sound* snd, VoiceCfg* cfg, u32 line_idx)
{
    SoundSlot* slot = (SoundSlot*)snd_voice_last((SndPool2*)snd_bank_table_get());
    u32 lower;
    s32 count;
    u32 flag;
    u32 key;
    s32 excess;

    if (snd_voice_list_head_ck((u8*)snd_bank_table_get(), (u8*)slot) == 1) {
        return 1;
    }
    lower = 0;
    count = 0;
    flag = cfg->flags & 1;
    key = flag == 0 ? line_idx : 255;
    key = (key << 24) | (flag << 16) | ((u32)cfg->key_bank << 8) | cfg->key_id;
    while (snd_voice_list_head_ck((u8*)snd_bank_table_get(), (u8*)slot) != 1) {
        if (slot->active == 1 && cfg->key_id != 255 && key == slot->key) {
            u32 release = snd_slot_state_get(slot)->release_count;
            if (release != (u32)-1 && release != 0) {
                return 0;
            }
            count++;
            if (cfg->priority >= slot->priority) {
                lower++;
            }
        }
        slot = snd_voice_prev(slot);
    }
    if (cfg->max_voices == 255 || cfg->max_voices > count) {
        return 1;
    }
    if (lower == 0) {
        return 0;
    }
    excess = count - (cfg->max_voices - 1);
    slot = (SoundSlot*)snd_voice_last((SndPool2*)snd_bank_table_get());
    while (snd_voice_list_head_ck((u8*)snd_bank_table_get(), (u8*)slot) != 1) {
        if (slot->active == 1 && cfg->key_id != 255 && key == slot->key && cfg->priority >= slot->priority) {
            snd_envelope_release((StreamMgr*)snd, snd_slot_state_get(slot));
            if (--excess <= 0) {
                return 1;
            }
        }
        slot = snd_voice_prev(slot);
    }
    return 0;
}

/* The previous voice in a list. */
extern "C" SoundSlot* snd_voice_prev(SoundSlot* p)
{
    return p->prev;
}

/* Whether a voice list has reached its end marker. */
extern "C" u32 snd_voice_list_head_ck(u8* list, u8* entry)
{
    return snd_voice_head_eq(snd_voice_list_end(list), entry);
}

/* Whether two addresses are equal. */
extern "C" u32 snd_voice_head_eq(u8* a, u8* b)
{
    return b == a;
}

/* Silences every voice whose key matches the one the config selects. */
extern "C" void snd_obj_cue_release(void* unused, VoiceCfg* cfg, u32 arg)
{
    SoundSlot* voice = (SoundSlot*)fn_800EDB24_c1(snd_bank_table_get());
    u32 flag = cfg->flags & 1;
    u32 key = flag == 0 ? arg : 255;

    key = (key << 24) | (flag << 16) | ((u32)cfg->key_bank << 8) | cfg->key_id;
    while (snd_voice_list_end_ck((u8*)snd_bank_table_get(), (u8*)voice) != 1) {
        if (voice->active == 1 && key == voice->key) {
            AxVoiceBlock* ax = snd_slot_state_get(voice);
            ax->state = 5;
        }
        voice = snd_voice_next(voice);
    }
}

/* Dispatches every voice whose key matches the one the config selects. */
extern "C" void snd_obj_cue_stop(Sound2* self, VoiceCfg* cfg, u32 arg)
{
    SoundSlot* voice = (SoundSlot*)fn_800EDB24_c1(snd_bank_table_get());
    u32 flag = cfg->flags & 1;
    u32 key = flag == 0 ? arg : 255;

    key = (key << 24) | (flag << 16) | ((u32)cfg->key_bank << 8) | cfg->key_id;
    while (snd_voice_list_end_ck((u8*)snd_bank_table_get(), (u8*)voice) != 1) {
        if (voice->active == 1 && key == voice->key) {
            ((SndPlayer*)self)->stop(voice);
        }
        voice = snd_voice_next(voice);
    }
}

/* Plays one of a bank's slots, picked by a 0..99 roll. */
extern "C" void snd_obj_cue_random(Sound2* self, ReverbLine* p1, VoiceCfg* p2, u32 p3, SndPlayParam* p4)
{
    SlotTable* table = (SlotTable*)snd_line_slot_table_get(p1, p2->table_idx);
    s32 roll;
    u32 total;
    u32 i;

    if (table == 0) {
        return;
    }
    if (table->count == 0) {
        return;
    }
    roll = (s32)(u8)stream_rand() % 100;
    total = 0;
    for (i = 0; i < table->count; i++) {
        total += table->entries[i].weight;
        if (roll < (s32)total) {
            VoiceCfg* slot = (VoiceCfg*)snd_line_cue_get(p1, table->entries[i].id);
            if (slot == 0) {
                return;
            }
            slot->aux_a = p2->aux_a;
            slot->aux_b = p2->aux_b;
            slot->aux_c = p2->aux_c;
            snd_obj_play_cue(self, p1, slot, p3, p4);
            return;
        }
    }
}

/* A relocation-table state inside a group. */
extern "C" void* snd_line_slot_table_get(void* p, u32 idx)
{
    return snd_reloc_state_entry(&((RelocGroup*)p)->state[7], idx);
}

/* Looks up one entry of a relocation table. */
extern "C" void* snd_reloc_state_entry(RelocState* state, u32 idx)
{
    if (state->table == 0) {
        return 0;
    }
    if (state->table->count <= idx) {
        return 0;
    }
    return state->entries[idx];
}

/* Stops a slot's voice and clears its flags. */
extern "C" void snd_obj_stop(void* unused, SoundSlot* slot)
{
    AxVoiceBlock* ax;

    if (slot == 0) {
        return;
    }
    snd_slot_flag_clear(slot, 512);
    snd_slot_flag_clear(slot, 1);
    snd_slot_flag_set(slot, 1024);
    ax = snd_slot_state_get(slot);
    if (ax->voice != 0) {
        AXSetVoiceState(ax->voice, 0);
    }
}

/* Stops a slot's voice. */
extern "C" void snd_obj_pause(void* unused, SoundSlot* slot)
{
    AxVoiceBlock* ax;

    if (slot == 0) {
        return;
    }
    ax = snd_slot_state_get(slot);
    if (ax->voice == 0) {
        return;
    }
    snd_slot_flag_set(slot, 512);
    AXSetVoiceState(ax->voice, 0);
}

/* Releases a slot's voice once its release flag is set. */
extern "C" void snd_obj_resume(void* unused, SoundSlot* slot)
{
    AxVoiceBlock* ax;

    if (slot == 0) {
        return;
    }
    if (snd_slot_flag_test(slot, 512) != 1) {
        return;
    }
    snd_slot_flag_clear(slot, 512);
    ax = snd_slot_state_get(slot);
    if (ax->voice == 0) {
        return;
    }
    if (ax->voice->pb.running != 0) {
        return;
    }
    AXSetVoiceState(ax->voice, 1);
}

/* Counts the voices one owner holds. */
extern "C" u32 snd_obj_owner_voice_count(void* unused, u32 key)
{
    SoundSlot* voice = (SoundSlot*)fn_800EDB24_c1(snd_bank_table_get());
    u32 count = 0;

    while (snd_voice_list_end_ck((u8*)snd_bank_table_get(), (u8*)voice) != 1) {
        if (voice->active == 1 && key == voice->owner) {
            count++;
        }
        voice = snd_voice_next(voice);
    }
    return count;
}

/* Applies the reverb config stored in one of the bank's lines. */
extern "C" void snd_obj_reverb_apply(Sound* snd, u32 idx, u32 entry_idx)
{
    ReverbCfg* cfg = (ReverbCfg*)snd_line_reverb_get(&snd->lines[idx], entry_idx);

    if (cfg != 0) {
        snd_reverb_unit_set(snd_reverb_mgr_get(), cfg);
    }
}

/* Looks up one reverb setting of a line. */
extern "C" void* snd_line_reverb_get(void* p, u32 idx)
{
    return snd_reloc_reverb_get(&((RelocGroup*)p)->state[4], idx);
}

/* One reverb setting of a reverb table, or 0 past its end. */
extern "C" ReverbCfg* snd_reloc_reverb_get(RelocState* state, u32 idx)
{
    if (state->table == 0) {
        return 0;
    }
    if (state->table->count <= idx) {
        return 0;
    }
    return (ReverbCfg*)state->entries[idx];
}

/* Clamps and stores a slot's cue volume. */
extern "C" void snd_obj_cue_volume_set(void* unused, SoundSlot* slot, s16 volume)
{
    AxVoiceBlock* ax;
    s16 v;

    if (slot == 0) {
        return;
    }
    ax = snd_slot_state_get(slot);
    v = volume;
    if (v > 127) {
        v = 127;
    } else if (v < 0) {
        v = 0;
    }
    ax->cue_volume = v;
}

/* Clamps a slot's pitch to its limits and applies it to the voice. */
extern "C" void snd_obj_pitch_set(void* unused, SoundSlot* slot, s16 pitch)
{
    AxVoiceBlock* ax;
    s32 v = pitch;

    if (slot == 0) {
        return;
    }
    ax = snd_slot_state_get(slot);
    if (v > ax->bend_up) {
        v = ax->bend_up;
    } else if (v < -ax->bend_down) {
        v = -ax->bend_down;
    }
    ax->pitch = v;
    fn_800E8E60((AxVoice*)ax->voice, ax->wave->field_04, ax->pitch);
}

/* The pitch of the first voice one owner holds. */
extern "C" s16 snd_obj_owner_pitch_get(void* unused, u32 key)
{
    s16 pitch = 0;
    SoundSlot* voice = (SoundSlot*)fn_800EDB24_c1(snd_bank_table_get());

    while (snd_voice_list_end_ck((u8*)snd_bank_table_get(), (u8*)voice) != 1) {
        if (voice->active == 1 && key == voice->owner) {
            pitch = snd_slot_state_get(voice)->pitch;
            break;
        }
        voice = snd_voice_next(voice);
    }
    return pitch;
}

/* Retunes every voice one owner holds. */
extern "C" void snd_obj_owner_lpf_set(void* unused, u32 key, u16 pitch)
{
    SoundSlot* voice = (SoundSlot*)fn_800EDB24_c1(snd_bank_table_get());

    while (snd_voice_list_end_ck((u8*)snd_bank_table_get(), (u8*)voice) != 1) {
        if (voice->active == 1 && key == voice->owner) {
            AxVoiceBlock* ax;

            if (pitch > 16000) {
                pitch = 16000;
            }
            ax = snd_slot_state_get(voice);
            if (pitch != ax->lpf_freq) {
                u16 a;
                u16 b;

                if (ax->lpf_on == 0) {
                    AXPBLpf args;

                    AXGetLpfCoefs(pitch, &a, &b);
                    args.on = 1;
                    args.yn1 = 0;
                    args.a0 = a;
                    args.b0 = b;
                    AXSetVoiceLpf(ax->voice, &args);
                    ax->lpf_on = 1;
                    ax->lpf_freq = pitch;
                } else {
                    AXGetLpfCoefs(pitch, &a, &b);
                    AXSetVoiceLpfCoefs(ax->voice, a, b);
                    ax->lpf_freq = pitch;
                }
            }
        }
        voice = snd_voice_next(voice);
    }
}

/* Constructs the global sound object. */
extern "C" Sound* snd_obj_static_init(void)
{
    return snd_obj_construct((Sound*)g_snd_obj);
}

/* Constructs the global sound object. */
extern "C" Sound* snd_obj_construct(Sound* snd)
{
    u32 i;

    snd_player_base_construct(snd);
    snd->vtable = (const SoundVtbl*)lbl_80597DA8;
    snd->field_08 = 127;
    snd->field_6E = 127;
    snd->field_6F = 127;
    snd->field_70 = 127;
    snd->field_71 = 127;
    snd->field_74 = 512;
    snd->field_78 = 0;
    snd->field_88 = 100.0f;
    snd->field_8C = snd_deg_to_rad(45.0f);
    __construct_array(&snd->lines[0], (void*)snd_line_construct, (void*)snd_line_destruct, sizeof(ReverbLine), 49);
    for (i = 0; i < 49; i++) {
        snd->levels[i] = 127;
    }
    snd->levels2[0] = 0;
    snd->levels2[1] = 0;
    snd->levels2[2] = 0;
    return snd;
}

/* Constructs one delay line. */
extern "C" ReverbLine* snd_line_construct(ReverbLine* line)
{
    snd_artic_table_clear((Pair32*)&line->part[0]);
    snd_region_table_clear((Pair32*)&line->part[1]);
    snd_reloc_state_clear((Pair32*)&line->part[2]);
    snd_reloc_state_clear((Pair32*)&line->part[3]);
    snd_reloc_state_clear((Pair32*)&line->part[4]);
    snd_reloc_state_clear((Pair32*)&line->part[5]);
    snd_reloc_state_clear((Pair32*)&line->part[6]);
    snd_reloc_state_clear((Pair32*)&line->part[7]);
    snd_reloc_state_clear((Pair32*)&line->part[8]);
    return line;
}

/* Clears a two-word block. */
extern "C" void snd_reloc_state_clear(Pair32* p)
{
    p->a = 0;
    p->b = 0;
}

/* Clears a two-word block. */
extern "C" void snd_region_table_clear(Pair32* p)
{
    p->a = 0;
    p->b = 0;
}

/* Clears a two-word block. */
extern "C" void snd_artic_table_clear(Pair32* p)
{
    p->a = 0;
    p->b = 0;
}

/* Scales a level by the unit's factor. */
extern "C" f32 snd_deg_to_rad(f32 v)
{
    return v * 0.017453292f;
}

/* The base constructor of the sound object. */
extern "C" Sound* snd_player_base_construct(Sound* snd)
{
    snd_base_construct((VtblHolder*)snd);
    snd->vtable = (const SoundVtbl*)lbl_80597DD0;
    snd->unused_04 = 0;
    return snd;
}

/* Installs a vtable. */
extern "C" void snd_base_construct(VtblHolder* p)
{
    p->vtable = lbl_80597DF8;
}
