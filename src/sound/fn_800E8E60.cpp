/* sound/fn_800E8E60.cpp - the sound voice helpers: pitch, 3D positioning, wave-bank relocation, the voice envelope
 * steps and the 96-node sound command pool.
 *
 * RANGE. `.text` 0x800E8E60..0x800E9D00 (every function, in address order), extab 0x8000B554..0x8000B638, extabindex
 *   0x80024FFC..0x800250F8, `.ctors` 0x8056F300, `.data` 0x80598020..0x80598038 (the two node vtables), `.bss`
 *   0x80698AF0..0x8069A810 (the pool; its last 4 bytes are alignment fill), `.sdata2` 0x807964C0..0x80796520.
 * FLAGS. lib `sound` (`cflags_main`, Wii/1.3); `#pragma peephole off`, `dont_inline on` (retail calls every inline
 *   helper out of line, each copy after its first caller), `no_static_dtors on` (`__sinit` registers no destructor)
 *   and `fp_contract off` over the file.
 * NAMES. GUESS, every name: the file keeps the map's stem (no `__FILE__` string, no dump name; a real name is requested
 *   in the outbox).  The function names come from what each computes and from the callers in `sound/sound_obj.cpp`
 *   (`snd_listener_polar` .. `snd_ref_table_count` are that unit's names); the classes `SndCmdLink`, `SndCmdNode`,
 *   `SndCmdList`, `SndCmdPool` and `SndEnvelopeOwner` and their manglings are GUESSes from the constructors,
 *   destructors and exception actions (the list pair is a member of the pool: retail's cleanup action is
 *   DESTROYMEMBER).
 * RESIDUALS. None in the code.  `lbl_80698AF0` (the pool) keeps its generated name: `sound/sound_obj.cpp` reads it
 *   through this unit's header as a byte array.
 * SHAPES. The pool constructors are inline members defined after the pool object (retail's copies follow `__sinit`);
 *   the wave-table lookup and the four envelope steps are inline functions retail tail-calls, so each has its own map
 *   row; `scale *= ...` keeps the pitch product's operand order; the pan constants split over two statements keep
 *   the pool order.
 */

#include "types.h"
#include "sound/sound_work.h"
#include "sound/snd_listener_polar.h"
#include "sound/snd_deg_to_rad.h"
#include "sound/snd_stream_reloc.h"
#include "AX/AXVPB.h"
#include "MSL/w_math.h"
#include "MSL/mathf.h"
#include "MTX/mtxvec.h"
#include "MTX/vec.h"
#include "ef/ef_torus.h"

#pragma peephole off
#pragma dont_inline on
#pragma no_static_dtors on
#pragma fp_contract off

/* One wave of a bank's wave table.  `snd_wave_bank_relocate` adds the bank's sample base to the four addresses, in
 * the unit the `format` implies (ADPCM nibbles, PCM16 samples, PCM8 bytes); a one-shot format clears `loop_end`. */
typedef struct SndWaveInfo {
    /* +0x00 */ u32 format;     /* 0/1 ADPCM, 2/3 PCM16, 4/5 PCM8; the odd formats loop */
    /* +0x04 */ u32 unused_04;
    /* +0x08 */ u32 loop_start;
    /* +0x0C */ u32 loop_end;
    /* +0x10 */ u32 end;
    /* +0x14 */ u32 start;
} SndWaveInfo; /* size: 0x18 */

/* The header of a bank's wave table file: the wave count, the relocated flag and the sample data at +0x10. */
typedef struct SndWaveTableHeader {
    /* +0x00 */ u32 unused_00;
    /* +0x04 */ u32 count;
    /* +0x08 */ u32 unused_08;
    /* +0x0C */ u32 relocated;  /* set once `snd_wave_bank_relocate` has rebased the waves */
    /* +0x10 */ u8 data[4];     /* the sample data (variable length) */
} SndWaveTableHeader; /* size: 0x14 (variable) */

/* A table of a bank: its header and its resolved entry array. */
struct SndWaveTable {
    /* +0x00 */ SndWaveTableHeader* header;
    /* +0x04 */ SndWaveInfo** items;
}; /* size: 0x8 */

/* A sound bank as `snd_wave_bank_relocate` sees it: nine tables of 8 bytes, the wave table at +0x18. */
typedef struct SndWaveBank {
    /* +0x00 */ u8 pad_0x00[0x18];
    /* +0x18 */ SndWaveTable waves;
    /* +0x20 */ u8 pad_0x20[0x28];
} SndWaveBank; /* size: 0x48 */

/* A view of the voice-player interface the envelope steps dispatch through (`sound/sound_obj.cpp` declares it as
 * `SndPlayer`; `sound/snd_stream_mgr.cpp` derives the stream manager from it).  Only the slots are declared, so no
 * table is emitted into this object. */
class SndEnvelopeOwner {
public:
    virtual SndEnvelopeOwner* destroy(s16 flags); /* +0x08 */
    virtual void update(SoundSlot* slot);         /* +0x0C */
    virtual BOOL release(SoundSlot** link);       /* +0x10 */
    virtual void voice_dropped(AXVPB* vpb);       /* +0x14 */
    virtual void stop(SoundSlot* slot);           /* +0x18 */
    virtual void pause(SoundSlot* slot);          /* +0x1C */
    virtual void resume(SoundSlot* slot);         /* +0x20 */
    virtual void start(SoundSlot* slot);          /* +0x24 */
}; /* size: 0x4 (the vtable pointer; the fields are the derived classes') */

/* A link of the sound command lists: the base of every node (its constructor makes it a one-node ring). */
class SndCmdLink {
public:
    SndCmdLink();
    virtual ~SndCmdLink() {}

    /* +0x00: the vtable pointer */
    /* +0x04 */ SndCmdLink* next;
    /* +0x08 */ SndCmdLink* prev;
}; /* size: 0xC */

/* A sound command node: a link and the command record `sound/sound_obj.cpp` fills in. */
class SndCmdNode : public SndCmdLink {
public:
    SndCmdNode();
    virtual ~SndCmdNode() {}

    /* +0x0C */ u8 record[0x40];
}; /* size: 0x4C */

/* The free and active list sentinels of the command pool. */
class SndCmdList {
public:
    SndCmdList();
    ~SndCmdList();

    /* +0x00 */ SndCmdNode free_list;
    /* +0x4C */ SndCmdNode active_list;
}; /* size: 0x98 */

/* The 96-node command pool `sound/sound_obj.cpp` allocates its voice commands from. */
class SndCmdPool {
public:
    SndCmdPool();
    ~SndCmdPool();

    /* +0x0000 */ SndCmdList lists;
    /* +0x0098 */ SndCmdNode nodes[96];
    /* +0x1D18 */ u8 initialised;
}; /* size: 0x1D1C */

/* --- the unit's own functions, declared so each has one signature ---------------------------------- */

extern "C" SndWaveInfo* snd_wave_bank_wave_get(SndWaveBank* self, u32 idx);
extern "C" inline SndWaveInfo* snd_wave_table_entry(SndWaveTable* table, u32 idx);
extern "C" SndWaveTableHeader* snd_wave_table_header_get(SndWaveTable* table);
extern "C" SndRef* snd_bank_table1_header_get(SndRef** slot);
extern "C" SndRef* snd_bank_table0_header_get(SndRef** slot);
extern "C" SndRef* snd_bank_table_header_get(SndRef** slot);
extern "C" void snd_envelope_update(SndEnvelopeOwner* self, SoundSlot* slot, AxVoiceBlock* ax);
extern "C" inline void snd_envelope_attack(SndEnvelopeOwner* self, AxVoiceBlock* ax);
extern "C" inline void snd_envelope_decay(SndEnvelopeOwner* self, AxVoiceBlock* ax);
extern "C" inline void snd_envelope_sustain(SndEnvelopeOwner* self, AxVoiceBlock* ax);
extern "C" inline void snd_envelope_fade(SndEnvelopeOwner* self, SoundSlot* slot, AxVoiceBlock* ax);
extern "C" void snd_envelope_release(SndEnvelopeOwner* self, AxVoiceBlock* ax);
extern "C" SndCmdLink* snd_cmd_pool_alloc(SndCmdPool* self);
extern "C" SndCmdLink* snd_cmd_pool_take(SndCmdPool* self);
extern "C" void snd_cmd_link_push_front(SndCmdLink* list, SndCmdLink* node);
extern "C" SndCmdLink* snd_cmd_link_pop_front(SndCmdLink* self);
extern "C" u32 snd_cmd_link_empty_ck(SndCmdLink* self);
extern "C" void snd_cmd_pool_reset(SndCmdPool* self, SndCmdNode* src, u32 count);
extern "C" void snd_cmd_link_push_nodes(SndCmdLink* list, SndCmdNode* src, u32 count);
extern "C" void snd_cmd_link_clear(SndCmdLink* self);
extern "C" SndCmdLink* snd_cmd_pool_free(SndCmdLink* list, SndCmdLink* node);
extern "C" SndCmdLink* snd_cmd_link_move_front(SndCmdLink* list, SndCmdLink* node);
extern "C" void snd_cmd_link_unlink(SndCmdLink* node);
extern "C" SndCmdLink* snd_cmd_link_next(SndCmdLink* self);
extern "C" SndCmdLink* snd_cmd_pool_first(SndCmdPool* self);
extern "C" SndCmdLink* snd_cmd_pool_active(SndCmdPool* self);

/* --- bodies, in address order --------------------------------------------------------------------- */

/* 0x800E8E60 (0x98): sets a voice's sample-rate ratio from its rate and a pitch offset in cents. */
extern "C" void snd_voice_pitch_set(AXVPB* vpb, u32 rate, s16 cents)
{
    f64 scale = pow(2.0, cents / 1200.0);

    scale *= rate / 32000.0f;
    AXSetVoiceSrcRatio(vpb, scale);
}

/* 0x800E8EF8 (0x80): transforms a position into listener space and returns its bearing and distance. */
extern "C" void snd_listener_polar(const Mtx listener, const Vec* pos, Vec* rel, f32* angle, f32* dist)
{
    PSMTXMultVec(listener, pos, rel);
    *angle = atan2f(rel->x, -rel->z);
    *dist = sqrtf(rel->z * rel->z + (rel->x * rel->x + rel->y * rel->y));
}

/* 0x800E8F78 (0x3C): the attenuation of `dist` against `max_dist`, clamped to 0 past the maximum. */
extern "C" void snd_distance_atten(f32* max_dist, f32* dist, f32* atten)
{
    if (0.0f == *max_dist || *dist > *max_dist) {
        *atten = 0.0f;
    } else {
        *atten = 1.0f - *dist / *max_dist;
    }
}

/* 0x800E8FB4 (0x210): the stereo pan of a listener-space position from its bearing. */
extern "C" s32 snd_pan_calc(Vec* rel, f32 dist, f32 angle, f32 pan_dist, f32 pan_angle, f32 atten)
{
    f32 a = ef_fabsf(angle);
    s32 pan;

    if (a >= 1.5707964f) {
        a = (3.1415927f - a) / 1.5707964f;
        a = 64.0f * a;
    } else if (a < pan_angle) {
        a = 64.0f * (a / 1.5707964f * (snd_deg_to_rad(45.0f) / pan_angle));
    } else if (0.0f == snd_deg_to_rad(90.0f) - pan_angle) {
        a = 64.0f;
    } else {
        a = 32.0f + (a - pan_angle) * (32.0f / (snd_deg_to_rad(90.0f) - pan_angle));
    }
    if (dist < pan_dist) {
        a *= 0.01f * ef_fabsf(rel->x);
    }
    if (angle >= 0.0f) {
        pan = 64.0f + a;
    } else {
        pan = 64.0f - a;
    }
    pan += (64 - pan) * atten;
    if (pan > 127) {
        pan = 127;
    } else if (pan < 0) {
        pan = 0;
    }
    return pan;
}

/* 0x800E91C4 (0x160): the front/back span of a listener-space position. */
extern "C" s32 snd_span_calc(Vec* rel, f32 dist, f32 pan_dist, f32 atten)
{
    Vec unit;
    s32 span;

    if (snd_handle_get() == 0 || snd_handle_get() == 1) {
        return 127;
    }
    if (rel->z > 1.0f || rel->z < -1.0f) {
        PSVECNormalize(rel, &unit);
    } else {
        unit.z = rel->z;
    }
    span = 64.0f * unit.z;
    if (dist < pan_dist) {
        span = 0.64f * rel->z;
    }
    span = 64 - span;
    span += (64 - span) * atten;
    if (span > 127) {
        span = 127;
    } else if (span < 0) {
        span = 0;
    }
    return span;
}

/* 0x800E9324 (0x6C): scales a volume down with distance and clamps it to [0, volume]. */
extern "C" s32 snd_distance_volume(s16 volume, f32 dist, f32 scale)
{
    s32 v = (f32)volume / (1.0f + 0.14159292f * (dist * scale));

    if (v > volume) {
        v = volume;
    } else if (v < 0) {
        v = 0;
    }
    return v;
}

/* 0x800E9390 (0x50): a priority that falls with distance, clamped to [1, 30]. */
extern "C" s32 snd_distance_priority(f32 dist, f32 scale)
{
    s32 v = 31.0f / (1.0f + 0.034292035f * (dist * scale));

    if (v > 30) {
        v = 30;
    } else if (v < 1) {
        v = 1;
    }
    return v;
}

/* 0x800E93E0 (0x1E4): rebases every wave of the bank onto the physical address of its sample data, once. */
extern "C" void snd_wave_bank_relocate(SndWaveBank* self)
{
    SndWaveTableHeader* header = snd_wave_table_header_get(&self->waves);

    if (header->relocated == 0) {
        u32 bytes = (u32)header->data - 0x80000000;
        u32 nibbles = bytes << 1;
        u32 samples = bytes >> 1;
        u32 i;

        for (i = 0; i < header->count; i++) {
            SndWaveInfo* wave = snd_wave_bank_wave_get(self, i);

            if (wave != 0) {
                switch (wave->format) {
                case 0:
                    wave->loop_start = nibbles + wave->start;
                    wave->loop_end = 0;
                    wave->end += nibbles;
                    wave->start += nibbles;
                    break;
                case 1:
                    wave->loop_start += nibbles;
                    wave->loop_end += nibbles;
                    wave->end += nibbles;
                    wave->start += nibbles;
                    break;
                case 2:
                    wave->loop_start = samples + wave->start;
                    wave->loop_end = 0;
                    wave->end += samples;
                    wave->start += samples;
                    break;
                case 3:
                    wave->loop_start += samples;
                    wave->loop_end += samples;
                    wave->end += samples;
                    wave->start += samples;
                    break;
                case 4:
                    wave->loop_start = bytes + wave->start;
                    wave->loop_end = 0;
                    wave->end += bytes;
                    wave->start += bytes;
                    break;
                case 5:
                    wave->loop_start += bytes;
                    wave->loop_end += bytes;
                    wave->end += bytes;
                    wave->start += bytes;
                    break;
                }
            }
        }
        header->relocated = 1;
    }
}

/* 0x800E95C4 (0x8): returns wave `idx` of the bank, or null. */
extern "C" SndWaveInfo* snd_wave_bank_wave_get(SndWaveBank* self, u32 idx)
{
    return snd_wave_table_entry(&self->waves, idx);
}

/* 0x800E95CC (0x38): returns entry `idx` of the table, or null when it is unset or the index is out of range. */
extern "C" inline SndWaveInfo* snd_wave_table_entry(SndWaveTable* table, u32 idx)
{
    if (table->header == 0) {
        return 0;
    }
    if (table->header->count <= idx) {
        return 0;
    }
    return table->items[idx];
}

/* 0x800E9604 (0x8): returns the table's header. */
extern "C" SndWaveTableHeader* snd_wave_table_header_get(SndWaveTable* table)
{
    return table->header;
}

/* Selects one of five reference slots by index and returns its value. */
extern "C" u32 snd_ref_table_count(SndRefTable* self, s32 index)
{
    u32 value = 0;

    switch (index) {
    case 0:
        value = snd_bank_table_header_get(&self->ref_40)->value;
        break;
    case 1:
        value = snd_bank_table_header_get(&self->ref_18)->value;
        break;
    case 2:
        value = snd_bank_table0_header_get(&self->ref_00)->value;
        break;
    case 3:
        value = snd_bank_table1_header_get(&self->ref_08)->value;
        break;
    case 4:
        value = snd_bank_table_header_get(&self->ref_10)->value;
        break;
    }
    return value;
}

/* Returns the reference the slot holds. */
extern "C" SndRef* snd_bank_table1_header_get(SndRef** slot)
{
    return *slot;
}

/* Returns the reference the slot holds. */
extern "C" SndRef* snd_bank_table0_header_get(SndRef** slot)
{
    return *slot;
}

/* Returns the reference the slot holds. */
extern "C" SndRef* snd_bank_table_header_get(SndRef** slot)
{
    return *slot;
}

/* Advances a voice's envelope one step; state 0 starts the voice through the owner's `start` slot. */
extern "C" void snd_envelope_update(SndEnvelopeOwner* self, SoundSlot* slot, AxVoiceBlock* ax)
{
    switch (ax->field_3A) {
    case 0:
        self->start(slot);
        break;
    case 1:
        snd_envelope_attack(self, ax);
        break;
    case 2:
        snd_envelope_decay(self, ax);
        break;
    case 3:
        snd_envelope_sustain(self, ax);
        break;
    case 5:
        snd_envelope_fade(self, slot, ax);
        break;
    }
}

/* 0x800E971C (0x60): raises the level by the attack step; at the top it moves to decay, sustain or hold. */
extern "C" inline void snd_envelope_attack(SndEnvelopeOwner* self, AxVoiceBlock* ax)
{
    ax->loop_value += ax->loop_rise_step;
    if (ax->loop_value >= 127) {
        ax->loop_value = 127;
        if (ax->loop_fall_base != 0) {
            ax->field_3A = 2;
        } else if (ax->loop_hold_step != 0) {
            ax->field_3A = 3;
        } else {
            ax->field_3A = 4;
        }
    }
}

/* 0x800E977C (0x48): lowers the level by the decay step to the sustain level, then moves to sustain or hold. */
extern "C" inline void snd_envelope_decay(SndEnvelopeOwner* self, AxVoiceBlock* ax)
{
    ax->loop_value -= ax->loop_fall_base;
    if (ax->loop_value <= ax->loop_fall_min) {
        ax->loop_value = ax->loop_fall_min;
        if (ax->loop_hold_step != 0) {
            ax->field_3A = 3;
        } else {
            ax->field_3A = 4;
        }
    }
}

/* 0x800E97C4 (0x4C): moves the level by the sustain step until it reaches the top (hold) or zero (release). */
extern "C" inline void snd_envelope_sustain(SndEnvelopeOwner* self, AxVoiceBlock* ax)
{
    ax->loop_value += ax->loop_hold_step;
    if (ax->loop_value >= 127) {
        ax->loop_value = 127;
        ax->field_3A = 4;
    } else if (ax->loop_value <= 0) {
        ax->loop_value = 0;
        ax->field_3A = 5;
    }
}

/* 0x800E9810 (0x38): lowers the level by the release step and stops the voice at zero. */
extern "C" inline void snd_envelope_fade(SndEnvelopeOwner* self, SoundSlot* slot, AxVoiceBlock* ax)
{
    ax->loop_value -= ax->loop_release_step;
    if (ax->loop_value <= 0) {
        ax->loop_value = 0;
        self->stop(slot);
    }
}

/* Forces a voice's loop state to the wrap state. */
extern "C" void snd_envelope_release(SndEnvelopeOwner* self, AxVoiceBlock* ax)
{
    ax->field_3A = 5;
}

/* Destroys both list sentinels. */
__declspec(export) SndCmdList::~SndCmdList() {}

/* Initialises the pool on first use and allocates one node from it. */
extern "C" SndCmdLink* snd_cmd_pool_alloc(SndCmdPool* self)
{
    if (self->initialised == 0) {
        snd_cmd_pool_reset(self, &self->nodes[0], 96);
        self->initialised = 1;
    }
    return snd_cmd_pool_take(self);
}

/* Allocates one node: pops the free list and pushes it onto the active list. */
extern "C" SndCmdLink* snd_cmd_pool_take(SndCmdPool* self)
{
    if (snd_cmd_link_empty_ck(&self->lists.free_list)) {
        return 0;
    }
    {
        SndCmdLink* node = snd_cmd_link_pop_front(&self->lists.free_list);

        snd_cmd_link_push_front(&self->lists.active_list, node);
        return node;
    }
}

/* Inserts `node` at the head of `list` (the sentinel). */
extern "C" void snd_cmd_link_push_front(SndCmdLink* list, SndCmdLink* node)
{
    SndCmdLink* first = list->next;

    node->next = first;
    first->prev = node;
    node->prev = list;
    list->next = node;
}

/* Removes and returns the head node, or null when the list is empty. */
extern "C" SndCmdLink* snd_cmd_link_pop_front(SndCmdLink* self)
{
    if (snd_cmd_link_empty_ck(self) == 1) {
        return 0;
    }
    {
        SndCmdLink* first = self->next;
        SndCmdLink* second = first->next;

        self->next = second;
        second->prev = self;
        return first;
    }
}

/* Whether the list is empty (its sentinel is its only node). */
extern "C" u32 snd_cmd_link_empty_ck(SndCmdLink* self)
{
    return self->next == self->prev;
}

/* Empties the active list and fills the free list with `count` nodes from `src`. */
extern "C" void snd_cmd_pool_reset(SndCmdPool* self, SndCmdNode* src, u32 count)
{
    snd_cmd_link_clear(&self->lists.active_list);
    snd_cmd_link_push_nodes(&self->lists.free_list, src, count);
}

/* Inserts `count` consecutive nodes from `src` at the head of `list`. */
extern "C" void snd_cmd_link_push_nodes(SndCmdLink* list, SndCmdNode* src, u32 count)
{
    u32 i;

    for (i = 0; i < count; i++) {
        snd_cmd_link_push_front(list, &src[i]);
    }
}

/* Makes `self` an empty list. */
extern "C" void snd_cmd_link_clear(SndCmdLink* self)
{
    self->prev = self;
    self->next = self;
}

/* Moves `node` to the head of `list` and returns its former successor. */
extern "C" SndCmdLink* snd_cmd_pool_free(SndCmdLink* list, SndCmdLink* node)
{
    return snd_cmd_link_move_front(list, node);
}

/* Moves `node` to the head of `list` and returns its former successor. */
extern "C" SndCmdLink* snd_cmd_link_move_front(SndCmdLink* list, SndCmdLink* node)
{
    SndCmdLink* next = snd_cmd_link_next(node);

    snd_cmd_link_unlink(node);
    snd_cmd_link_push_front(list, node);
    return next;
}

/* Unlinks `node` from whatever list holds it. */
extern "C" void snd_cmd_link_unlink(SndCmdLink* node)
{
    node->prev->next = node->next;
    node->prev->next->prev = node->prev;
}

/* The node after `self`. */
extern "C" SndCmdLink* snd_cmd_link_next(SndCmdLink* self)
{
    return self->next;
}

/* The first node of the pool's active list. */
extern "C" SndCmdLink* snd_cmd_pool_first(SndCmdPool* self)
{
    return snd_cmd_link_next(snd_cmd_pool_active(self));
}

/* The pool's active-list sentinel. */
extern "C" SndCmdLink* snd_cmd_pool_active(SndCmdPool* self)
{
    return &self->lists.active_list;
}

/* The sound command pool. */
SndCmdPool lbl_80698AF0;

/* Constructs the pool: both list sentinels, the 96 nodes, and a cleared first-use flag. */
inline SndCmdPool::SndCmdPool()
{
    initialised = 0;
}

/* Constructs a command node. */
inline SndCmdNode::SndCmdNode() {}

/* Constructs a link as a one-node ring. */
inline SndCmdLink::SndCmdLink()
{
    next = this;
    prev = this;
}

/* Constructs both list sentinels. */
inline SndCmdList::SndCmdList() {}
