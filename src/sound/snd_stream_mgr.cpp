/* sound/snd_stream_mgr.cpp - the disc stream player: the stream manager class, its voices and its DVD reads
 *
 * RANGE. `.text` 0x800E9D00..0x800ED780 (63 rows); `.data` 0x80598038..0x80598090 (the command switch table, then
 *   `__vt__9StreamMgr`); `.bss` `g_stream_mgr`; `.sdata` the PRNG seed; `.sbss` the buffer sizes; `.sdata2` the
 *   float pool (literals, byte-identical); `.ctors` the static initialiser.
 * FLAGS. lib `sound` (`cflags_main`, Wii/1.3). `#pragma peephole off` and `#pragma fp_contract off` over the file;
 *   `#pragma optimization_level 4` around `cmd_start` (its channel switch is the level-4 `subi`/`cmplwi` range
 *   lowering, playbook 108); the manager's constructor is `inline` and `never_inline` (idea 242), so `__sinit` calls it,
 *   it follows `__sinit` and the base constructor is inlined into it.
 * NAMES. GUESS (from the bodies, the AX/DVD/MIX calls and the callers), every name in the unit: the manager streams
 *   ADPCM/PCM files from the disc into a buffer split in thirds (`DVDReadAsyncPrio`, `stream_read_*_cb`), plays them
 *   through one or two AX voices, and runs the commands of three stream banks (`StreamRelocSet`: format, group,
 *   command and track tables plus a path list). The callees of other units carry registry names (lane
 *   sound-stream-mgr); the record type names (`StreamVoice`, `StreamCmd`, `StreamTrack`, ...) are GUESSes.
 * RESIDUALS. `voice_limit`: the key and the counters take r30/r28 where retail uses r28/r30 (declaration orders and
 *   variable reuse tried). `stream_voice_setup`: the pitch sum's first load is r5 where retail loads it into r26, and
 *   the pan add keeps swapped operands (a one-expression pan reassociates). `queue_read`: the per-channel limit and
 *   `read_pos` swap r4/r5. `stream_voice_rewind`: the `addis` lands in r0 where retail reuses r4.
 * RESIDUALS. The class shape (blocks the flip): retail's slot 0 is a real destructor (`__destroy_arr` over the
 *   records, then the base destructor `snd_player_base_destruct(this, 0)`), the records are a class (constructor
 *   `stream_rec_init`, copy constructor `stream_rec_copy` over `stream_reloc_copy`, destructor `stream_rec_destroy`)
 *   and `cmd_start` passes `*rec` by value to `stream_rec_format` (copy, call, destroy). Those three compiler-managed
 *   objects are retail's three `extab` cleanup records (0x40 bytes, relocations to `stream_rec_destroy` x2 and
 *   `snd_player_base_destruct`), which the explicit calls here cannot emit. Writing them needs sound/sound_obj's
 *   player base constructor/destructor as mangled C++ members (a foreign rename), and `stream_play`'s record copy has
 *   no destructor call in retail, which the single class model does not yet explain.
 * RESIDUALS. Data: `.sdata` is 2 of the claimed 8 bytes (the rest is link fill). flipcheck: `extab` -0x40 (the three
 *   records) and the extab index relocations that follow it; `.text` is the target's size.
 * SHAPES. The DVD read callbacks test the result with three separate early returns (a `&&`/`||` chain, nested ifs or a
 *   switch fold to one `addi`/`cmplwi` range test). The bank table getters are separate functions right after their
 *   wrappers (retail's `addi`/`b`), every index widened to u32 at the call; the id key is
 *   `owner = global ? 0xFF : slot` then one expression; the `MIXInitChannel` calls take their five `snd_level_to_db`
 *   arguments inline (evaluated right to left). `run_chain` reads the chained index through a `u16` inline getter
 *   (retail's `clrlwi` at the call); `stream_read_cb` rounds the read size in its own statement (retail reloads
 *   `file_pos` after the call); `stream_read_loop_cb` sums buffer, slot offset and result left to right; the pitch is
 *   one left-to-right sum.
 */

#include "types.h"
#include "AX/AXVPB.h"
#include "AX/AXAlloc.h"
#include "DVD/dvd.h"
#include "DVD/dvdfs.h"
#include "MIX/mix.h"
#include "Runtime.PPCEABI.H/CPlusLibPPC.h"
#include "Runtime.PPCEABI.H/__destroy_arr.h"
#include "sound/snd_bank_table_get.h"
#include "sound/sound_obj.h"
#include "sound/snd_stream_reloc.h"
#include "sound/snd_stream_mgr_types.h" /* StreamCmd (rule 1) */
#include "sound/snd_level_tbl.h"
#include "sound/snd_voice_pool.h"
#include "sound/stream_mgr_get.h"
#include "sound/snd_slot_flag_set.h"
#include "sound/snd_envelope_update.h"
#include "sound/snd_voice_alloc.h"
#include "sound/snd_format_voice_init.h"

class StreamMgr;
struct SndPlayerBase;
struct StreamNode;
struct StreamFormat;

/* A relocation state of a stream bank: a table and its resolved entry array. */
struct StreamRelocState {
    /* +0x00 */ u32* table;
    /* +0x04 */ void** entries;
}; /* size: 0x8 */

/* One stream bank record (three live in the manager): the format, group, command and track tables, and the
 * file path list. */
struct StreamRelocSet {
    /* +0x00 */ StreamRelocState formats;
    /* +0x08 */ StreamRelocState groups;
    /* +0x10 */ StreamRelocState cmds;
    /* +0x18 */ StreamRelocState tracks;
    /* +0x20 */ const char** paths;
    /* +0x24 */ u32 path_count;
}; /* size: 0x28 */

/* One weighted choice of a random command. */
struct StreamGroupEntry {
    /* +0x00 */ u8 cmd;
    /* +0x01 */ u8 weight;
}; /* size: 0x2 */

/* A random command's choices. */
struct StreamGroup {
    /* +0x00 */ u32 count;
    /* +0x04 */ StreamGroupEntry entries[1];
}; /* size: 0x8 (variable) */

/* A stream file's format header. */
struct StreamFormat {
    /* +0x00 */ s32 mono;        /* zero for a two-channel stream */
    /* +0x04 */ u32 codec;       /* 0/1 are the ADPCM formats */
    /* +0x08 */ u8 pad_0x08[8];
    /* +0x10 */ u32 loop_start;
    /* +0x14 */ u32 loop_end;    /* zero when the stream does not loop */
}; /* size: 0x18 */

/* A stream track: the format it plays and its mix parameters. */
struct StreamTrack {
    /* +0x00 */ s16 format;
    /* +0x02 */ s16 pitch_coarse;  /* semitones, x100 cents */
    /* +0x04 */ s16 pitch_fine;
    /* +0x06 */ u8 volume;
    /* +0x07 */ u8 pan;
    /* +0x08 */ u8 env_attack;
    /* +0x09 */ u8 env_decay;
    /* +0x0A */ u8 env_sustain;
    /* +0x0B */ s8 env_hold;
    /* +0x0C */ u8 env_release;
    /* +0x0D */ u8 aux_a;
    /* +0x0E */ u8 aux_b;
    /* +0x0F */ u8 aux_c;
}; /* size: 0x10 */

/* The streaming state of one voice node. */
struct StreamVoice {
    /* +0x00 */ StreamFormat* format;
    /* +0x04 */ AXVPB* vpb_l;
    /* +0x08 */ AXVPB* vpb_r;
    /* +0x0C */ DVDFileInfo file;
    /* +0x48 */ s16 level;
    /* +0x4A */ u8 pad_0x4A[2];
    /* +0x4C */ s16 env_level;
    /* +0x4E */ s16 out_volume;
    /* +0x50 */ u8 pad_0x50[8];
    /* +0x58 */ s16 pitch;
    /* +0x5A */ s16 pan;
    /* +0x5C */ s16 aux_a;
    /* +0x5E */ s16 aux_b;
    /* +0x60 */ s16 aux_c;
    /* +0x62 */ u8 pad_0x62[6];
    /* +0x68 */ u32 duration;
    /* +0x6C */ u32 hold;
    /* +0x70 */ u32 fade_frames;
    /* +0x74 */ u32 fade_count;
    /* +0x78 */ s16 env_attack;
    /* +0x7A */ s16 env_decay;
    /* +0x7C */ s16 env_sustain;
    /* +0x7E */ s16 env_hold;
    /* +0x80 */ s16 env_release;
    /* +0x82 */ u8 env_state;
    /* +0x83 */ u8 pad_0x83[5];
    /* +0x88 */ u8 cmd_flags;
    /* +0x89 */ u8 play_buf;
    /* +0x8A */ s16 fader;
    /* +0x8C */ s16 ramp_target;
    /* +0x8E */ u8 pad_0x8E[2];
    /* +0x90 */ u8* buffer;
    /* +0x94 */ u32 chunk_size;
    /* +0x98 */ f32 fade;
    /* +0x9C */ s16 ramp_volume;
    /* +0x9E */ s16 ramp_base;
    /* +0xA0 */ u32 ramp_count;
    /* +0xA4 */ u8 pad_0xA4[4];
    /* +0xA8 */ u8 active;
    /* +0xA9 */ u8 read_buf;
    /* +0xAA */ u8 queued_buf;
    /* +0xAB */ u8 cancelled;
    /* +0xAC */ u32 read_pos;
    /* +0xB0 */ u32 file_pos;
    /* +0xB4 */ s32 entrynum;
    /* +0xB8 */ u32 unused_0xB8;
    /* +0xBC */ u32 ramp_frames;
    /* +0xC0 */ u32 play_addr;
    /* +0xC4 */ u32 prev_addr;
    /* +0xC8 */ u32 played;
    /* +0xCC */ u32 loop_end;
    /* +0xD0 */ u8 wrapping;
    /* +0xD1 */ u8 at_end;
    /* +0xD2 */ u8 unused_0xD2;
    /* +0xD3 */ s8 channels;
    /* +0xD4 */ u16 id;
    /* +0xD6 */ u8 loading;
    /* +0xD7 */ u8 pad_0xD7;
    /* +0xD8 */ u8 loop_seen;
    /* +0xD9 */ u8 pad_0xD9[7];
    /* +0xE0 */ u32 wraps;
    /* +0xE4 */ u8 pad_0xE4[0x10];
}; /* size: 0xF4 */

/* A sound pool node playing a stream. */
struct StreamNode {
    /* +0x00 */ u8 pool_links[0xC];
    /* +0x0C */ u32 flags;
    /* +0x10 */ u32 unused_0x10;
    /* +0x14 */ u32 key;
    /* +0x18 */ u8 priority;
    /* +0x19 */ u8 kind;          /* 2 for a stream */
    /* +0x1A */ u8 slot;
    /* +0x1B */ u8 pad_0x1B;
    /* +0x1C */ StreamVoice voice;
}; /* size: 0x110 */

struct SndPool2;
struct AxVoiceBlock;
struct ReverbMgr;

/* Call views (the `_c1` convention): owner declarations whose types differ from the ones these calls pass. */
#define snd_bank_table_get_c1 ((SndPool2* (*)(void))snd_bank_table_get)
#define snd_voice_free_c1 ((StreamNode* (*)(SndPool2*, StreamNode*))snd_voice_free)
#define snd_voice_first_c1 ((StreamNode* (*)(SndPool2*))snd_voice_first)
#define snd_level_volume_get_c1 ((s16 (*)(void))snd_level_volume_get)

/* The sound player base (sound/sound_obj): its constructor is that unit's C-linkage function. */
struct SndPlayerBase {
    SndPlayerBase()
    {
        snd_player_base_construct(this);
    }
}; /* size: 0x0 (the base's vtable and flag word are laid out in StreamMgr) */

/* The stream manager. */
class StreamMgr : public SndPlayerBase {
public:
    StreamMgr() __attribute__((never_inline));
    virtual StreamMgr* destroy(s16 flags);
    virtual void update(StreamNode* node);
    virtual BOOL release(StreamNode** link);
    virtual void voice_dropped(AXVPB* vpb);
    virtual void stop(StreamNode* node);
    virtual void pause(StreamNode* node);
    virtual void resume(StreamNode* node);
    virtual void start(StreamNode* node);

    void track_position(StreamNode* node);
    void update_volume(StreamNode* node);
    u8* buffer_alloc();
    void buffer_free(u8* buffer);
    void run_chain(StreamRelocSet* rec, StreamCmd* cmd, s32 slot, u32 paused);
    void run_cmd(StreamRelocSet* rec, StreamCmd* cmd, s32 slot, const char* path, u32 paused);
    void cmd_start(StreamRelocSet* rec, StreamCmd* cmd, s32 slot, const char* path, u32 fade_in, u32 paused);
    BOOL voice_limit(StreamCmd* cmd, s32 slot);
    void cmd_stop(StreamCmd* cmd, s32 slot);
    void cmd_pause(StreamCmd* cmd, s32 slot);
    void cmd_resume(StreamCmd* cmd, s32 slot);
    void cmd_random(StreamRelocSet* rec, StreamCmd* cmd, s32 slot, u32 paused);
    void cmd_ramp(StreamCmd* cmd, s32 slot);
    void cmd_ramp_up(StreamCmd* cmd, s32 slot);
    void cmd_ramp_down(StreamCmd* cmd, s32 slot);
    void cmd_fade_out(StreamCmd* cmd, s32 slot);
    void cmd_fade_in(StreamCmd* cmd, s32 slot);
    void poll_read(StreamNode* node);
    void queue_read(StreamNode* node);
    void fade_out_set(StreamNode* node, u32 ms);
    void fade_in_set(StreamNode* node, u32 ms);
    void ramp_set(StreamNode* node, StreamCmd* cmd);
    void ramp_up_set(StreamNode* node, StreamCmd* cmd);
    void ramp_down_set(StreamNode* node, StreamCmd* cmd);

    /* +0x04 */ u32 flags;
    /* +0x08 */ u32 buffer_count;
    /* +0x0C */ u32 buffer_used;
    /* +0x10 */ s32 active;
    /* +0x14 */ u8* buffers[4];
    /* +0x24 */ s16 master_volume;
    /* +0x26 */ s16 bank_volume[3];
    /* +0x2C */ StreamCmd cmd;
    /* +0x5C */ StreamRelocSet recs[3];
    /* +0xD4 */ u32 unused_0xD4;
}; /* size: 0xD8 */


extern "C" {
u8 stream_rand(void);
void* stream_rec_destroy(StreamRelocSet* rec, s16 flags);
void stream_voice_drop_cb(AXVPB* vpb);
void stream_voice_setup(StreamVoice* voice, StreamCmd* cmd, StreamTrack* track, u32 id);
void stream_voice_loop_set(StreamVoice* voice, AXVPB* vpb, u8* buffer);
void stream_voice_rewind(StreamVoice* voice);
void stream_read_cb(s32 result, DVDFileInfo* file);
u32 stream_round_up(u32 value, u32 align);
StreamVoice* stream_node_voice(StreamNode* node);
void stream_read_loop_cb(s32 result, DVDFileInfo* file);
void stream_read_wrap_cb(s32 result, DVDFileInfo* file);
void stream_read_tail_cb(s32 result, DVDFileInfo* file);
void stream_cancel_cb(s32 result, DVDCommandBlock* block);
void stream_buffers_setup(StreamMgr* mgr, u8* base, u32 size, u32 count);
void stream_play(StreamMgr* mgr, s32 slot, u32 cmd_id, u32 paused);
const char* stream_rec_path(StreamRelocSet* rec, u32 idx);
StreamCmd* stream_rec_cmd(StreamRelocSet* rec, u32 idx);
StreamCmd* stream_cmd_table_get(StreamRelocState* state, u32 idx);
StreamRelocSet* stream_rec_copy(StreamRelocSet* dst, StreamRelocSet* src);
void stream_reloc_copy(StreamRelocState* dst, StreamRelocState* src);
void stream_play_cmd(StreamMgr* mgr, s32 slot, StreamCmd* cmd, u32 paused);
void stream_cmd_copy(StreamCmd* dst, StreamCmd* src);
StreamFormat* stream_rec_format(StreamRelocSet* rec, u32 idx);
StreamFormat* stream_format_table_get(StreamRelocState* state, u32 idx);
StreamTrack* stream_rec_track(StreamRelocSet* rec, u32 idx);
StreamTrack* stream_track_table_get(StreamRelocState* state, u32 idx);
StreamGroup* stream_rec_group(StreamRelocSet* rec, u32 idx);
u32 stream_status(StreamMgr* mgr, u32 id);
StreamRelocSet* stream_rec_init(StreamRelocSet* rec);
}

#pragma peephole off
#pragma fp_contract off

static u16 stream_rand_seed = 0x0D37;

static u32 stream_buffer_size;
static u32 stream_half_size;
static u32 stream_chunk_size;
static u32 stream_end_margin;

/* Advances the 16-bit generator: the low byte gains the high byte. */
u8 stream_rand(void)
{
    union {
        u16 word;
        u8 bytes[2];
    } next, half;

    next.word = stream_rand_seed;
    half.word = stream_rand_seed >> 1;
    next.bytes[0] += half.bytes[1];
    next.bytes[1] = half.bytes[1];
    if (next.word == stream_rand_seed) {
        next.word += 0x101;
    }
    stream_rand_seed = next.word;
    return next.bytes[0];
}

/* Destroys the manager's three records, then the base. */
StreamMgr* StreamMgr::destroy(s16 flags)
{
    if (this != 0) {
        __destroy_arr(recs, (void*)stream_rec_destroy, sizeof(StreamRelocSet), 3);
        snd_player_base_destruct(this, 0);
        if (flags > 0) {
            operator delete(this);
        }
    }
    return this;
}

/* Destroys the four relocation states of a record. */
void* stream_rec_destroy(StreamRelocSet* rec, s16 flags)
{
    if (rec != 0) {
        snd_reloc_state_release(&rec->tracks, -1);
        snd_reloc_state_release(&rec->cmds, -1);
        snd_reloc_state_release(&rec->groups, -1);
        snd_reloc_state_release(&rec->formats, -1);
        if (flags > 0) {
            operator delete(rec);
        }
    }
    return rec;
}

/* The AX voice-drop callback: hands the voice to the stream manager. */
void stream_voice_drop_cb(AXVPB* vpb)
{
    stream_mgr_get()->voice_dropped(vpb);
}

/* Initialises a voice's mix, envelope and streaming state from its command and track. */
void stream_voice_setup(StreamVoice* voice, StreamCmd* cmd, StreamTrack* track, u32 id)
{
    u32 frames;
    f32 env;
    f32 volume;
    f32 track_volume;
    f32 bus;
    f32 master;
    f32 bank;
    f32 scale;
    s16 random;
    u8 range;
    s16 pitch_range;
    s32 pitch;
    s32 pan;
    u32 limit;

    voice->cmd_flags = cmd->flags;
    voice->env_level = voice->env_attack = track->env_attack;
    voice->env_decay = track->env_decay;
    voice->env_sustain = track->env_sustain;
    voice->env_hold = track->env_hold;
    voice->env_release = 0.47f * (0.6f * (0.14f * track->env_release));
    if (voice->env_release <= 0) {
        voice->env_release = 1;
    }

    frames = cmd->duration;
    if (frames != 0xFFFFFFFF) {
        frames = 5.5555558f * frames;
    }
    voice->duration = frames;
    frames = cmd->hold;
    if (frames != 0xFFFFFFFF) {
        frames = 5.5555558f * frames;
    } else {
        frames = 0;
    }
    voice->hold = frames;
    frames = cmd->fade;
    if (frames != 0xFFFFFFFF) {
        frames = 5.5555558f * frames;
    }
    voice->fade_frames = frames;
    voice->fade_count = 0;
    voice->env_state = 0;

    env = voice->env_level / 127.0f;
    volume = cmd->volume / 127.0f;
    track_volume = track->volume / 127.0f;
    bus = snd_level_volume_get_c1() / 127.0f;
    master = stream_master_volume_get() / 127.0f;
    bank = stream_bank_volume_get(id) / 127.0f;
    scale = bank * (master * (bus * (track_volume * (env * volume))));

    random = 0;
    range = cmd->volume_random;
    if ((s8)range > 0) {
        random = (stream_rand() % (range * 2)) - range;
    }
    voice->level = (random + 127) * scale;
    voice->out_volume = voice->level * voice->fade;
    if (voice->out_volume > 127) {
        voice->out_volume = 127;
    } else if (voice->out_volume < 0) {
        voice->out_volume = 0;
    }
    voice->ramp_base = voice->level;

    pitch = cmd->pitch + track->pitch_fine + track->pitch_coarse * 100;
    pitch_range = cmd->pitch_random;
    if (pitch_range > 0) {
        pitch += ((stream_rand() << 5) % (pitch_range * 2)) - pitch_range;
    }
    if (pitch > 0x2000) {
        pitch = 0x2000;
    } else if (pitch < -0x2000) {
        pitch = -0x2000;
    }
    voice->pitch = pitch;

    pan = cmd->pan - 64;
    pan = track->pan + pan;
    if (pan > 127) {
        pan = 127;
    } else if (pan < 0) {
        pan = 0;
    }
    voice->pan = pan;

    voice->aux_a = track->aux_a;
    if (voice->aux_a > 127) {
        voice->aux_a = 127;
    } else if (voice->aux_a < 0) {
        voice->aux_a = 0;
    }
    voice->aux_b = track->aux_b;
    if (voice->aux_b > 127) {
        voice->aux_b = 127;
    } else if (voice->aux_b < 0) {
        voice->aux_b = 0;
    }
    voice->aux_c = track->aux_c;
    if (voice->aux_c > 127) {
        voice->aux_c = 127;
    } else if (voice->aux_c < 0) {
        voice->aux_c = 0;
    }

    voice->fader = cmd->fader;
    voice->active = 1;
    voice->queued_buf = 0xFF;
    voice->read_buf = 0;
    voice->read_pos = 0;
    voice->file_pos = voice->format->mono == 0 ? voice->file.length >> 1 : 0;
    voice->cancelled = 0;
    voice->unused_0xB8 = 0;
    voice->ramp_target = 127;
    voice->play_buf = 2;
    voice->channels = (voice->format->mono == 0) + 1;
    if (voice->buffer != 0) {
        voice->prev_addr = voice->play_addr = (u32)voice->buffer - 0x80000000;
    }
    voice->played = 0;
    voice->loop_seen = 0;
    if (voice->format->loop_end != 0) {
        if (voice->channels == 1) {
            voice->loop_end = snd_format_addr_to_bytes(voice->format, voice->format->loop_end);
        } else {
            voice->loop_end = snd_format_samples_to_bytes(voice->format, voice->format->loop_end);
        }
        limit = voice->file.length / voice->channels;
        if (voice->loop_end > limit) {
            voice->loop_end = limit;
        }
    } else {
        voice->loop_end = 0;
    }
    voice->wrapping = 0;
    voice->at_end = 0;
    voice->unused_0xD2 = 0;
    voice->id = id;
    voice->loading = 1;
    voice->wraps = 0;
}

/* Programs a voice's loop, end and current addresses for the buffer it plays. */
void stream_voice_loop_set(StreamVoice* voice, AXVPB* vpb, u8* buffer)
{
    u32 start;
    u32 loop;
    u32 end;

    if (buffer != 0) {
        start = snd_format_bytes_to_addr(voice->format, (u32)buffer - 0x80000000);
        if (voice->file.length / voice->channels <= stream_half_size) {
            if (voice->loop_end != 0) {
                vpb->pb.addrFormat.first = 1;
                loop = start + snd_format_samples_to_addr(voice->format, voice->format->loop_start);
                end = start + snd_format_samples_to_addr(voice->format, voice->format->loop_end) - 1;
            } else {
                vpb->pb.addrFormat.first = 0;
                loop = start;
                end = start + snd_format_bytes_to_addr(voice->format, voice->file.length / voice->channels) - 1;
            }
        } else {
            loop = start;
            end = start + snd_format_bytes_to_addr(voice->format, stream_half_size) - 1;
            vpb->pb.addrFormat.first = 1;
        }
        if (voice->format->codec <= 1) {
            loop += 2;
        }
        vpb->pb.addrLoop.hi = loop >> 16;
        vpb->pb.addrLoop.lo = loop;
        vpb->pb.addrEnd.hi = end >> 16;
        vpb->pb.addrEnd.lo = end;
        vpb->pb.addrCurrent.hi = start >> 16;
        vpb->pb.addrCurrent.lo = start;
    }
}

/* Points the voices at the start of the buffer third after the one playing. */
void stream_voice_rewind(StreamVoice* voice)
{
    u32 next;
    u8* left;
    u8* right;
    u32 left_addr;
    u32 right_addr;
    u32 addr;

    if (voice->buffer != 0) {
        next = voice->play_buf == 2 ? 0 : voice->play_buf + 1;
        if (voice->vpb_r != 0) {
            left = voice->buffer + next * stream_chunk_size;
            right = left + stream_half_size;
            left_addr = snd_format_bytes_to_addr(voice->format, (u32)left - 0x80000000);
            right_addr = snd_format_bytes_to_addr(voice->format, (u32)right - 0x80000000);
            if (voice->vpb_l != 0) {
                AXSetVoiceCurrentAddr(voice->vpb_l, left_addr);
            }
            if (voice->vpb_r != 0) {
                AXSetVoiceCurrentAddr(voice->vpb_r, right_addr);
            }
        } else {
            addr = snd_format_bytes_to_addr(voice->format, (u32)(voice->buffer + (next * voice->chunk_size - 0x80000000)));
            if (voice->vpb_l != 0) {
                AXSetVoiceCurrentAddr(voice->vpb_l, addr);
            }
        }
    }
}

/* The first chunk read finished: queue the right channel's chunk. */
void stream_read_cb(s32 result, DVDFileInfo* file)
{
    StreamVoice* voice;
    AXPBAdpcmLoop loop;
    u32 size;

    if (result == -1) {
        return;
    }
    if (result == -2) {
        return;
    }
    if (result == -3) {
        return;
    }
    {
        voice = stream_node_voice((StreamNode*)file->cb.userData);
        if (voice->buffer != 0) {
            if (voice->read_buf == 0 && voice->format->codec <= 1) {
                loop.predScale = *voice->buffer;
                if (voice->vpb_l != 0) {
                    AXSetVoiceAdpcmLoop(voice->vpb_l, &loop);
                }
            }
            size = stream_chunk_size;
            if (voice->wrapping == 1 || voice->at_end == 1 || voice->file.length / voice->channels <= stream_half_size) {
                size = result;
            }
            voice->read_pos += result;
            if (voice->file_pos + size > voice->file.length) {
                size = voice->file.length - voice->file_pos;
            }
            size = stream_round_up(size, 32);
            DVDReadAsyncPrio(&voice->file, voice->buffer + stream_half_size + voice->read_buf * stream_chunk_size,
                             size, voice->file_pos, stream_read_loop_cb, 0);
        }
    }
}

/* Rounds `value` up to a multiple of `align` (zero when `align` is zero). */
u32 stream_round_up(u32 value, u32 align)
{
    return align != 0 ? (value + align - 1) / align * align : 0;
}

/* The streaming state of a node. */
StreamVoice* stream_node_voice(StreamNode* node)
{
    return &node->voice;
}

/* The right channel's chunk read finished: hand the chunk over, or read the loop's head on a wrap. */
void stream_read_loop_cb(s32 result, DVDFileInfo* file)
{
    StreamVoice* voice;
    AXPBAdpcmLoop loop;
    u32 pos;
    u8 buf;
    u8* dst;
    StreamNode* node;
    s32 size;

    if (result == -1) {
        return;
    }
    if (result == -2) {
        return;
    }
    if (result == -3) {
        return;
    }
    {
        node = (StreamNode*)file->cb.userData;
        voice = stream_node_voice(node);
        if (voice->buffer != 0) {
            if (voice->read_buf == 0) {
                loop.predScale = *(voice->buffer + stream_half_size);
                if (voice->vpb_r != 0) {
                    AXSetVoiceAdpcmLoop(voice->vpb_r, &loop);
                }
            }
            if (voice->wrapping == 1) {
                size = stream_chunk_size - result;
                if (voice->channels == 1) {
                    pos = snd_format_addr_to_bytes(voice->format, voice->format->loop_start);
                } else {
                    pos = snd_format_samples_to_bytes(voice->format, voice->format->loop_start);
                }
                dst = voice->buffer + voice->read_buf * stream_chunk_size + result;
                voice->read_pos = pos & ~7;
                DVDReadAsyncPrio(&voice->file, dst, size, voice->read_pos, stream_read_wrap_cb, 0);
                return;
            }
            voice->file_pos += result;
            buf = voice->read_buf;
            snd_slot_flag_set(node, 0x10000 << buf);
            snd_slot_flag_clear(node, 0x2000 << buf);
            if (voice->file.length > stream_half_size) {
                if (++voice->read_buf > 2) {
                    voice->read_buf = 0;
                }
                voice->loading = 0;
            }
        }
    }
}

/* The loop head of the left channel was read: read the right channel's loop head. */
void stream_read_wrap_cb(s32 result, DVDFileInfo* file)
{
    StreamVoice* voice;
    u32 offset;
    u8* dst;
    u32 pos;

    if (result == -1) {
        return;
    }
    if (result == -2) {
        return;
    }
    if (result == -3) {
        return;
    }
    {
        voice = stream_node_voice((StreamNode*)file->cb.userData);
        if (voice->buffer != 0) {
            offset = voice->read_buf * stream_chunk_size;
            dst = voice->buffer + stream_half_size + offset + (stream_chunk_size - result);
            pos = snd_format_samples_to_bytes(voice->format, voice->format->loop_start);
            voice->read_pos += result;
            if (voice->channels == 1) {
                voice->file_pos = 0;
            } else {
                voice->file_pos = (pos & ~7) + (voice->file.length >> 1);
            }
            DVDReadAsyncPrio(&voice->file, dst, result, voice->file_pos, stream_read_tail_cb, 0);
        }
    }
}

/* The wrapped chunk is complete: hand it over. */
void stream_read_tail_cb(s32 result, DVDFileInfo* file)
{
    StreamVoice* voice;
    u8 buf;
    StreamNode* node;

    if (result == -1) {
        return;
    }
    if (result == -2) {
        return;
    }
    if (result == -3) {
        return;
    }
    {
        node = (StreamNode*)file->cb.userData;
        voice = stream_node_voice(node);
        voice->file_pos += result;
        voice->wrapping = 0;
        buf = voice->read_buf;
        snd_slot_flag_set(node, 0x10000 << buf);
        snd_slot_flag_clear(node, 0x2000 << buf);
        if (++voice->read_buf > 2) {
            voice->read_buf = 0;
        }
        voice->loading = 0;
    }
}

/* The cancelled read finished: mark the voice cancelled. */
void stream_cancel_cb(s32 result, DVDCommandBlock* block)
{
    StreamVoice* voice = stream_node_voice((StreamNode*)block->userData);

    voice->cancelled = 1;
    voice->id = 0xFFFF;
    voice->loading = 0;
}

/* Tracks the play position and switches the buffer third the voice plays. */
void StreamMgr::track_position(StreamNode* node)
{
    StreamVoice* voice = stream_node_voice(node);
    s32 crossed = 0;
    u32 base;
    u32 second;
    u32 third;
    s32 delta;
    s32 next;

    if (voice->buffer != 0 && voice->vpb_l != 0) {
        base = (u32)voice->buffer - 0x80000000;
        second = base + stream_chunk_size;
        third = base + stream_chunk_size * 2;
        voice->play_addr = snd_format_addr_to_bytes(voice->format,
                                       (voice->vpb_l->pb.addrCurrent.hi << 16) | voice->vpb_l->pb.addrCurrent.lo);
        delta = voice->play_addr - voice->prev_addr;
        if (voice->prev_addr < second && voice->play_addr > second) {
            crossed = 1;
        } else if (voice->prev_addr < third && voice->play_addr > third) {
            crossed = 1;
        } else if (voice->prev_addr > voice->play_addr) {
            crossed = 1;
            delta += stream_half_size;
        }
        if (voice->loop_end == 0) {
            voice->played += delta;
        } else if (voice->loop_seen == 1) {
            voice->played += delta;
        } else if (voice->played + delta >= voice->loop_end) {
            voice->played = voice->played + delta - voice->loop_end + snd_format_samples_to_bytes(voice->format, voice->format->loop_start);
        } else {
            voice->played += delta;
        }
        if (voice->at_end == 1 && voice->played > voice->file.length / voice->channels - stream_end_margin) {
            stop(node);
            return;
        }
        voice->prev_addr = voice->play_addr;
        if (crossed != 0 && voice->file.length / voice->channels > stream_half_size) {
            next = voice->play_buf + 1;
            if (snd_slot_flag_test(node, 0x10000 << (next > 2 ? 0 : next)) != 0) {
                if (++voice->play_buf > 2) {
                    voice->play_buf = 0;
                }
            } else {
                stream_voice_rewind(voice);
            }
        }
    }
}

/* Per-frame update of one stream voice. */
void StreamMgr::update(StreamNode* node)
{
    StreamVoice* voice = stream_node_voice(node);

    if (snd_slot_flag_test(node, 0x1000000) != 0) {
        if (active >= buffer_count) {
            return;
        }
        voice->buffer = buffer_alloc();
        if (voice->buffer == 0) {
            return;
        }
        snd_slot_flag_clear(node, 0x1000000);
    }
    if (voice->env_state != 0) {
        track_position(node);
    }
    if (voice->hold != 0xFFFFFFFF && voice->hold != 0) {
        voice->hold--;
    }
    if (voice->env_state != 0 && voice->duration != 0xFFFFFFFF) {
        if (--voice->duration == 0) {
            voice->env_state = 5;
            voice->duration = 0xFFFFFFFF;
        }
    }
    snd_envelope_update(this, node, (AxVoiceBlock*)&voice->level);
    poll_read(node);
    update_volume(node);
}

/* An AX voice was dropped: forget it and cancel the node's read. */
void StreamMgr::voice_dropped(AXVPB* vpb)
{
    StreamNode* node = (StreamNode*)vpb->userContext;
    StreamVoice* voice = stream_node_voice(node);

    if ((AXVPB*)voice->vpb_l == vpb) {
        voice->vpb_l = 0;
    }
    if ((AXVPB*)voice->vpb_r == vpb) {
        voice->vpb_r = 0;
    }
    snd_slot_flag_clear(node, 1);
    snd_slot_flag_set(node, 0x400);
    voice->file.cb.userData = node;
    DVDCancelAsync(&voice->file.cb, stream_cancel_cb);
}

/* Frees a finished node; returns TRUE when it was released. */
BOOL StreamMgr::release(StreamNode** link)
{
    StreamVoice* voice = stream_node_voice(*link);

    if (snd_slot_flag_test(*link, 0x400) != 0) {
        if (voice->cancelled != 0) {
            if (voice->vpb_l != 0) {
                AXFreeVoice(voice->vpb_l);
                voice->vpb_l = 0;
            }
            if (voice->vpb_r != 0) {
                AXFreeVoice(voice->vpb_r);
                voice->vpb_r = 0;
            }
            buffer_free(voice->buffer);
            DVDClose(&voice->file);
            *link = snd_voice_free_c1(snd_bank_table_get_c1(), *link);
            active--;
            return TRUE;
        }
    } else if (snd_slot_flag_test(*link, 0x200) != 1 && snd_slot_flag_test(*link, 0x100) != 0) {
        if (voice->vpb_l->pb.running == 0 && voice->vpb_l->pb.addrFormat.first != 1) {
            if (voice->vpb_l != 0) {
                AXFreeVoice(voice->vpb_l);
                voice->vpb_l = 0;
            }
            if (voice->vpb_r != 0) {
                AXFreeVoice(voice->vpb_r);
                voice->vpb_r = 0;
            }
            buffer_free(voice->buffer);
            DVDClose(&voice->file);
            *link = snd_voice_free_c1(snd_bank_table_get_c1(), *link);
            active--;
            return TRUE;
        }
    }
    return FALSE;
}

/* Starts the voices once the first buffer third is loaded. */
void StreamMgr::start(StreamNode* node)
{
    StreamVoice* voice = stream_node_voice(node);
    s32 next = voice->play_buf + 1;
    s16 pan;

    if (snd_slot_flag_test(node, 0x10000 << (next > 2 ? 0 : next)) != 0) {
        snd_format_voice_init(voice->format, voice->vpb_l, voice->vpb_r, voice->pitch);
        if (voice->vpb_l != 0) {
            stream_voice_loop_set(voice, voice->vpb_l, voice->buffer);
        }
        if (voice->vpb_r != 0) {
            stream_voice_loop_set(voice, voice->vpb_r, voice->buffer + stream_half_size);
        }
        if (voice->format->mono == 0) {
            pan = 0;
        } else {
            pan = voice->pan;
        }
        if (voice->vpb_l != 0) {
            MIXInitChannel(voice->vpb_l, snd_reverb_flags_get(snd_reverb_mgr_get()), snd_level_to_db(voice->out_volume),
                           snd_level_to_db(voice->aux_a), snd_level_to_db(voice->aux_b), snd_level_to_db(voice->aux_c), pan, 127,
                           snd_level_to_db(voice->fader));
            if (snd_slot_flag_test(node, 0x200) != 0) {
                AXSetVoiceState(voice->vpb_l, 0);
            } else {
                AXSetVoiceState(voice->vpb_l, 1);
            }
        }
        if (voice->vpb_r != 0) {
            MIXInitChannel(voice->vpb_r, snd_reverb_flags_get(snd_reverb_mgr_get()), snd_level_to_db(voice->out_volume),
                           snd_level_to_db(voice->aux_a), snd_level_to_db(voice->aux_b), snd_level_to_db(voice->aux_c), 127, 127,
                           snd_level_to_db(voice->fader));
            if (snd_slot_flag_test(node, 0x200) != 0) {
                AXSetVoiceState(voice->vpb_r, 0);
            } else {
                AXSetVoiceState(voice->vpb_r, 1);
            }
        }
        active++;
        snd_slot_flag_set(node, 0x100);
        voice->env_state = 1;
    }
}

/* Steps the fade and ramp state machines and pushes the resulting volume to the mixer. */
void StreamMgr::update_volume(StreamNode* node)
{
    StreamVoice* voice = stream_node_voice(node);
    f32 env = 1.0f;
    BOOL changed = FALSE;
    f32 step;
    u16 volume;

    if (voice->env_state != 0 && voice->env_state != 4) {
        env = voice->env_level / 127.0f;
        changed = TRUE;
    }
    if (snd_slot_flag_test(node, 0x80000) != 0) {
        voice->fade_count++;
        if (voice->fade_count >= voice->fade_frames) {
            voice->fade = 1.0f;
            voice->out_volume = voice->ramp_base;
            snd_slot_flag_clear(node, 0x80000);
        } else {
            voice->fade = (f32)voice->fade_count / (f32)(voice->fade_frames + 1);
            voice->out_volume = voice->ramp_base;
        }
        changed = TRUE;
    }
    if (snd_slot_flag_test(node, 0x100000) != 0) {
        if (++voice->fade_count >= voice->fade_frames) {
            voice->fade = 0.0f;
            snd_slot_flag_clear(node, 0x100000);
        } else {
            voice->fade = 1.0f - (f32)voice->fade_count / (f32)(voice->fade_frames + 1);
            voice->out_volume = voice->ramp_base;
        }
        changed = TRUE;
    }
    if (snd_slot_flag_test(node, 0x200000) != 0) {
        if (++voice->ramp_count >= voice->ramp_frames) {
            voice->ramp_target = voice->ramp_volume;
            if (voice->ramp_target > 127) {
                voice->ramp_target = 127;
            } else if (voice->ramp_target < 0) {
                voice->ramp_target = 0;
            }
            voice->out_volume = voice->ramp_target * env;
            voice->ramp_base = voice->out_volume;
            snd_slot_flag_clear(node, 0x200000);
        } else {
            voice->ramp_target = voice->ramp_volume;
            if (voice->ramp_target > 127) {
                voice->ramp_target = 127;
            } else if (voice->ramp_target < 0) {
                voice->ramp_target = 0;
            }
            step = (f32)voice->ramp_count * ((voice->ramp_target - voice->ramp_base) / (1.0f + (f32)voice->ramp_frames));
            voice->out_volume = env * (voice->ramp_base + step);
        }
        changed = TRUE;
    }
    if (snd_slot_flag_test(node, 0x400000) != 0) {
        if (++voice->ramp_count >= voice->ramp_frames) {
            voice->ramp_target = voice->ramp_base + voice->ramp_volume;
            if (voice->ramp_target > 127) {
                voice->ramp_target = 127;
            } else if (voice->ramp_target < 0) {
                voice->ramp_target = 0;
            }
            voice->out_volume = voice->ramp_target * env;
            voice->ramp_base = voice->out_volume;
            snd_slot_flag_clear(node, 0x400000);
        } else {
            voice->ramp_target = voice->ramp_base + voice->ramp_volume;
            if (voice->ramp_target > 127) {
                voice->ramp_target = 127;
            } else if (voice->ramp_target < 0) {
                voice->ramp_target = 0;
            }
            step = (f32)voice->ramp_count * ((voice->ramp_target - voice->ramp_base) / (1.0f + (f32)voice->ramp_frames));
            voice->out_volume = env * (voice->ramp_base + step);
        }
        changed = TRUE;
    }
    if (snd_slot_flag_test(node, 0x800000) != 0) {
        if (++voice->ramp_count >= voice->ramp_frames) {
            voice->ramp_target = voice->level * (0.01f * voice->ramp_volume);
            if (voice->ramp_target > 127) {
                voice->ramp_target = 127;
            } else if (voice->ramp_target < 0) {
                voice->ramp_target = 0;
            }
            voice->out_volume = voice->ramp_target * env;
            voice->ramp_base = voice->out_volume;
            snd_slot_flag_clear(node, 0x800000);
        } else {
            voice->ramp_target = voice->level * (voice->ramp_volume / 100.0f);
            if (voice->ramp_target > 127) {
                voice->ramp_target = 127;
            } else if (voice->ramp_target < 0) {
                voice->ramp_target = 0;
            }
            step = (f32)voice->ramp_count * ((voice->ramp_target - voice->ramp_base) / (1.0f + (f32)voice->ramp_frames));
            voice->out_volume = env * (voice->ramp_base + step);
        }
        changed = TRUE;
    }
    if (changed != FALSE) {
        voice->out_volume = voice->out_volume * voice->fade;
        if (voice->out_volume > 127) {
            voice->out_volume = 127;
        } else if (voice->out_volume < 0) {
            voice->out_volume = 0;
        }
    }
    volume = voice->out_volume;
    if (stream_bus_dirty_ck(voice->id) == 1 || changed != FALSE) {
        volume = volume * stream_bus_volume_get(voice->id);
        if (volume > 127) {
            volume = 127;
        }
        if (voice->vpb_l != 0) {
            MIXSetInput(voice->vpb_l, snd_level_to_db(volume));
        }
        if (voice->vpb_r != 0) {
            MIXSetInput(voice->vpb_r, snd_level_to_db(volume));
        }
    }
}

/* Takes the first free stream buffer, or returns zero. */
u8* StreamMgr::buffer_alloc()
{
    u32 i = 0;
    u32 count = buffer_count;
    u8* buffer;

    for (; i < count; i++) {
        if ((buffer_used & (1 << i)) == 0) {
            buffer = buffers[i];
            buffer_used |= (1 << i);
            return buffer;
        }
    }
    return 0;
}

/* Returns a stream buffer to the free set. */
void StreamMgr::buffer_free(u8* buffer)
{
    u32 i;

    for (i = 0; i < buffer_count; i++) {
        if (buffers[i] == buffer) {
            buffer_used &= ~(1 << i);
        }
    }
}

/* Splits `base` into `count` stream buffers of `size` bytes and sets the chunk sizes. */
void stream_buffers_setup(StreamMgr* mgr, u8* base, u32 size, u32 count)
{
    u32 i;

    stream_buffer_size = size;
    stream_half_size = size >> 1;
    stream_chunk_size = size / 6;
    stream_end_margin = 0x10000;
    for (i = 0; i < count; i++) {
        mgr->buffers[i] = base + stream_buffer_size * i;
    }
    mgr->buffer_count = count;
}

/* Runs command `cmd_id` of bank `slot` and its chain. */
void stream_play(StreamMgr* mgr, s32 slot, u32 cmd_id, u32 paused)
{
    StreamRelocSet rec;
    StreamCmd* cmd;
    const char* path;

    stream_rec_copy(&rec, &mgr->recs[slot]);
    cmd = stream_rec_cmd(&rec, cmd_id);
    if (cmd != 0) {
        path = 0;
        if (cmd->type == 1) {
            path = stream_rec_path(&rec, cmd->track);
            if (path == 0) {
                return;
            }
        }
        mgr->run_cmd(&rec, cmd, slot, path, paused);
        mgr->run_chain(&rec, cmd, slot, paused);
    }
}

/* The file path `idx` of a record, or zero. */
const char* stream_rec_path(StreamRelocSet* rec, u32 idx)
{
    if (rec->path_count <= idx) {
        return 0;
    }
    return rec->paths[idx];
}

/* The command `idx` of a record, or zero. */
StreamCmd* stream_rec_cmd(StreamRelocSet* rec, u32 idx)
{
    return stream_cmd_table_get(&rec->cmds, idx);
}

/* Entry `idx` of a command table, or zero when the table is missing or `idx` is out of range. */
StreamCmd* stream_cmd_table_get(StreamRelocState* state, u32 idx)
{
    if (state->table == 0) {
        return 0;
    }
    if (state->table[1] <= idx) {
        return 0;
    }
    return (StreamCmd*)state->entries[idx];
}

/* Copies a record. */
StreamRelocSet* stream_rec_copy(StreamRelocSet* dst, StreamRelocSet* src)
{
    stream_reloc_copy(&dst->formats, &src->formats);
    stream_reloc_copy(&dst->groups, &src->groups);
    stream_reloc_copy(&dst->cmds, &src->cmds);
    stream_reloc_copy(&dst->tracks, &src->tracks);
    dst->paths = src->paths;
    dst->path_count = src->path_count;
    return dst;
}

/* Copies a relocation state. */
void stream_reloc_copy(StreamRelocState* dst, StreamRelocState* src)
{
    dst->table = src->table;
    dst->entries = src->entries;
}

/* Runs an immediate command on bank `slot`. */
void stream_play_cmd(StreamMgr* mgr, s32 slot, StreamCmd* cmd, u32 paused)
{
    StreamRelocSet rec;
    StreamCmd* copy;

    stream_rec_copy(&rec, &mgr->recs[slot]);
    copy = &mgr->cmd;
    stream_cmd_copy(copy, cmd);
    if (copy != 0) {
        mgr->run_cmd(&rec, copy, slot, 0, paused);
    }
}

/* Copies a stream command field by field. */
void stream_cmd_copy(StreamCmd* dst, StreamCmd* src)
{
    dst->type = src->type;
    dst->volume_random = src->volume_random;
    dst->group = src->group;
    dst->priority = src->priority;
    dst->flags = src->flags;
    dst->track = src->track;
    dst->id_lo = src->id_lo;
    dst->id_hi = src->id_hi;
    dst->pan = src->pan;
    dst->max_voices = src->max_voices;
    dst->next = src->next;
    dst->fader = src->fader;
    dst->pitch_random = src->pitch_random;
    dst->volume = src->volume;
    dst->pitch = src->pitch;
    dst->unused_0x12 = src->unused_0x12;
    dst->duration = src->duration;
    dst->hold = src->hold;
    dst->fade = src->fade;
    dst->param_0x20 = src->param_0x20;
    dst->param_0x22 = src->param_0x22;
    dst->param_0x24 = src->param_0x24;
    dst->param_0x26 = src->param_0x26;
    dst->user = src->user;
}

/* Returns the index of the command chained after `cmd` (0xFF ends the chain). */
inline u16 stream_cmd_next(StreamCmd* cmd)
{
    return cmd->next;
}

/* Runs the commands chained after `cmd`. */
void StreamMgr::run_chain(StreamRelocSet* rec, StreamCmd* cmd, s32 slot, u32 paused)
{
    StreamCmd* next;
    const char* path;
    u16 idx = stream_cmd_next(cmd);

    if (idx != 0xFF) {
        do {
            next = stream_rec_cmd(rec, idx);
            if (next == 0) {
                return;
            }
            path = 0;
            if (next->type == 1 || next->type == 6) {
                path = stream_rec_path(rec, next->track);
                if (path == 0) {
                    return;
                }
            }
            run_cmd(rec, next, slot, path, paused);
            idx = stream_cmd_next(next);
        } while (idx != 0xFF);
    }
}

/* Dispatches one command. */
void StreamMgr::run_cmd(StreamRelocSet* rec, StreamCmd* cmd, s32 slot, const char* path, u32 paused)
{
    switch (cmd->type) {
    case 1:
        cmd_start(rec, cmd, slot, path, 0, paused);
        break;
    case 2:
        cmd_stop(cmd, slot);
        break;
    case 3:
        cmd_pause(cmd, slot);
        break;
    case 4:
        cmd_resume(cmd, slot);
        break;
    case 5:
        cmd_random(rec, cmd, slot, paused);
        break;
    case 6:
        cmd_fade_in(cmd, slot);
        break;
    case 7:
        cmd_fade_out(cmd, slot);
        break;
    case 8:
        cmd_ramp(cmd, slot);
        break;
    case 9:
        cmd_ramp_up(cmd, slot);
        break;
    case 10:
        cmd_ramp_down(cmd, slot);
        break;
    }
}

#pragma optimization_level 4

/* Opens the file of a start command, takes a node, voices and a buffer, and initialises the voice. */
void StreamMgr::cmd_start(StreamRelocSet* rec, StreamCmd* cmd, s32 slot, const char* path, u32 fade_in, u32 paused)
{
    StreamTrack* track;
    StreamNode* node;
    StreamVoice* voice;
    StreamRelocSet copy;
    u32 key;

    if (voice_limit(cmd, slot) == 0) {
        return;
    }
    track = stream_rec_track(rec, cmd->track);
    if (track == 0) {
        return;
    }
    node = snd_voice_alloc(snd_bank_table_get_c1());
    if (node == 0) {
        return;
    }
    node->kind = 2;
    voice = stream_node_voice(node);
    voice->entrynum = DVDConvertPathToEntrynum(path);
    if (voice->entrynum < 0) {
        snd_voice_free_c1(snd_bank_table_get_c1(), node);
        return;
    }
    if (DVDFastOpen(voice->entrynum, &voice->file) != 1) {
        snd_voice_free_c1(snd_bank_table_get_c1(), node);
        return;
    }
    snd_slot_flag_set(node, 1);
    node->priority = cmd->priority;
    node->slot = slot;
    if ((cmd->flags & 1) != 0) {
        node->key = (cmd->id_hi << 8) | (cmd->id_lo | 0xFF010000);
    } else {
        key = cmd->id_lo | (slot << 24);
        node->key = key | (cmd->id_hi << 8);
    }
    stream_rec_copy(&copy, rec);
    voice->format = stream_rec_format(&copy, track->format);
    stream_rec_destroy(&copy, -1);
    if (voice->format == 0) {
        snd_voice_free_c1(snd_bank_table_get_c1(), node);
        return;
    }
    if (active >= 3) {
        snd_slot_flag_set(node, 0x1000000);
    } else {
        voice->buffer = buffer_alloc();
        if (voice->buffer == 0) {
            snd_voice_free_c1(snd_bank_table_get_c1(), node);
            return;
        }
    }
    voice->chunk_size = voice->format->mono == 0 ? stream_chunk_size >> 1 : stream_chunk_size;
    switch (voice->format->mono) {
    case 0:
        voice->vpb_l = AXAcquireVoice(31, stream_voice_drop_cb, (u32)node);
        voice->vpb_r = AXAcquireVoice(31, stream_voice_drop_cb, (u32)node);
        break;
    case 1:
    case 2:
        voice->vpb_l = AXAcquireVoice(31, stream_voice_drop_cb, (u32)node);
        voice->vpb_r = 0;
        break;
    }
    if (voice->vpb_l == 0) {
        snd_voice_free_c1(snd_bank_table_get_c1(), node);
        return;
    }
    if (fade_in == 1) {
        voice->fade = 0.0f;
        voice->fade_count = 0;
        snd_slot_flag_set(node, 0x80000);
    } else {
        voice->fade = 1.0f;
    }
    if (paused == 1) {
        snd_slot_flag_set(node, 0x200);
    }
    stream_voice_setup(voice, cmd, track, slot);
}

#pragma optimization_level reset

/* The format `idx` of a record, or zero. */
StreamFormat* stream_rec_format(StreamRelocSet* rec, u32 idx)
{
    return stream_format_table_get(&rec->formats, idx);
}

/* Entry `idx` of a format table, or zero when the table is missing or `idx` is out of range. */
StreamFormat* stream_format_table_get(StreamRelocState* state, u32 idx)
{
    if (state->table == 0) {
        return 0;
    }
    if (state->table[1] <= idx) {
        return 0;
    }
    return (StreamFormat*)state->entries[idx];
}

/* The track `idx` of a record, or zero. */
StreamTrack* stream_rec_track(StreamRelocSet* rec, u32 idx)
{
    return stream_track_table_get(&rec->tracks, idx);
}

/* Entry `idx` of a track table, or zero when the table is missing or `idx` is out of range. */
StreamTrack* stream_track_table_get(StreamRelocState* state, u32 idx)
{
    if (state->table == 0) {
        return 0;
    }
    if (state->table[1] <= idx) {
        return 0;
    }
    return (StreamTrack*)state->entries[idx];
}

/* Whether a start command may take a voice; stops lower-priority voices of the same id to make room. */
BOOL StreamMgr::voice_limit(StreamCmd* cmd, s32 slot)
{
    StreamNode* node;
    StreamNode* victim;
    u32 global;
    u32 owner;
    u32 key;
    s32 count;
    s32 weaker;
    u32 hold;

    node = snd_voice_last(snd_bank_table_get_c1());
    if (snd_voice_list_head_ck(snd_bank_table_get_c1(), node) == 1) {
        return TRUE;
    }
    weaker = 0;
    count = 0;
    global = cmd->flags & 1;
    owner = global != 0 ? 0xFF : slot;
    key =cmd->id_lo | ((cmd->id_hi << 8) | ((owner << 24) | (global << 16)));
    for (; snd_voice_list_head_ck(snd_bank_table_get_c1(), node) != 1; node = snd_voice_prev(node)) {
        if (node->kind == 2 && cmd->id_lo != 0xFF && key == node->key) {
            hold = stream_node_voice(node)->hold;
            if (hold != 0xFFFFFFFF && hold != 0) {
                return FALSE;
            }
            count++;
            if (cmd->priority >= node->priority) {
                weaker++;
            }
        }
    }
    if (cmd->max_voices == 0xFF || cmd->max_voices > count) {
        return TRUE;
    }
    if (weaker == 0) {
        return FALSE;
    }
    weaker = count - (cmd->max_voices - 1);
    for (victim = snd_voice_last(snd_bank_table_get_c1()); snd_voice_list_head_ck(snd_bank_table_get_c1(), victim) != 1;
         victim = snd_voice_prev(victim)) {
        if (victim->kind == 2 && cmd->id_lo != 0xFF && key == victim->key && cmd->priority >= victim->priority) {
            snd_envelope_release(this, (AxVoiceBlock*)&stream_node_voice(victim)->level);
            if (--weaker <= 0) {
                return TRUE;
            }
        }
    }
    return FALSE;
}

/* Stops every voice of the command's id. */
void StreamMgr::cmd_stop(StreamCmd* cmd, s32 slot)
{
    StreamNode* node = snd_voice_first_c1(snd_bank_table_get_c1());
    u32 global = cmd->flags & 1;
    u32 owner = global != 0 ? 0xFF : slot;
    u32 key = cmd->id_lo | ((cmd->id_hi << 8) | ((owner << 24) | (global << 16)));
    for (; snd_voice_list_end_ck(snd_bank_table_get_c1(), node) != 1; node = snd_voice_next(node)) {
        if (node->kind == 2 && key == node->key) {
            stop(node);
        }
    }
}

/* Pauses every voice of the command's id. */
void StreamMgr::cmd_pause(StreamCmd* cmd, s32 slot)
{
    StreamNode* node = snd_voice_first_c1(snd_bank_table_get_c1());
    u32 global = cmd->flags & 1;
    u32 owner = global != 0 ? 0xFF : slot;
    u32 key = cmd->id_lo | ((cmd->id_hi << 8) | ((owner << 24) | (global << 16)));
    for (; snd_voice_list_end_ck(snd_bank_table_get_c1(), node) != 1; node = snd_voice_next(node)) {
        if (node->kind == 2 && key == node->key) {
            pause(node);
        }
    }
}

/* Resumes every voice of the command's id. */
void StreamMgr::cmd_resume(StreamCmd* cmd, s32 slot)
{
    StreamNode* node = snd_voice_first_c1(snd_bank_table_get_c1());
    u32 global = cmd->flags & 1;
    u32 owner = global != 0 ? 0xFF : slot;
    u32 key = cmd->id_lo | ((cmd->id_hi << 8) | ((owner << 24) | (global << 16)));
    for (; snd_voice_list_end_ck(snd_bank_table_get_c1(), node) != 1; node = snd_voice_next(node)) {
        if (node->kind == 2 && key == node->key) {
            resume(node);
        }
    }
}

/* Picks one command of the command's group by weight and plays it. */
void StreamMgr::cmd_random(StreamRelocSet* rec, StreamCmd* cmd, s32 slot, u32 paused)
{
    StreamGroup* group = stream_rec_group(rec, cmd->group);
    s32 roll;
    s32 sum;
    u32 i;

    if (group != 0 && group->count != 0) {
        roll = stream_rand() % 100;
        sum = 0;
        for (i = 0; i < group->count; i++) {
            sum += group->entries[i].weight;
            if (roll < sum) {
                stream_play(this, slot, group->entries[i].cmd, paused);
                return;
            }
        }
    }
}

/* The group `idx` of a record. */
StreamGroup* stream_rec_group(StreamRelocSet* rec, u32 idx)
{
    return snd_reloc_state_entry(&rec->groups, idx);
}

/* Starts a volume ramp on every voice of the command's id. */
void StreamMgr::cmd_ramp(StreamCmd* cmd, s32 slot)
{
    StreamNode* node = snd_voice_first_c1(snd_bank_table_get_c1());
    u32 global = cmd->flags & 1;
    u32 owner = global != 0 ? 0xFF : slot;
    u32 key = cmd->id_lo | ((cmd->id_hi << 8) | ((owner << 24) | (global << 16)));
    for (; snd_voice_list_end_ck(snd_bank_table_get_c1(), node) != 1; node = snd_voice_next(node)) {
        if (node->kind == 2 && key == node->key) {
            ramp_set(node, cmd);
        }
    }
}

/* Starts a relative volume ramp on every voice of the command's id. */
void StreamMgr::cmd_ramp_up(StreamCmd* cmd, s32 slot)
{
    StreamNode* node = snd_voice_first_c1(snd_bank_table_get_c1());
    u32 global = cmd->flags & 1;
    u32 owner = global != 0 ? 0xFF : slot;
    u32 key = cmd->id_lo | ((cmd->id_hi << 8) | ((owner << 24) | (global << 16)));
    for (; snd_voice_list_end_ck(snd_bank_table_get_c1(), node) != 1; node = snd_voice_next(node)) {
        if (node->kind == 2 && key == node->key) {
            ramp_up_set(node, cmd);
        }
    }
}

/* Starts a percentage volume ramp on every voice of the command's id. */
void StreamMgr::cmd_ramp_down(StreamCmd* cmd, s32 slot)
{
    StreamNode* node = snd_voice_first_c1(snd_bank_table_get_c1());
    u32 global = cmd->flags & 1;
    u32 owner = global != 0 ? 0xFF : slot;
    u32 key = cmd->id_lo | ((cmd->id_hi << 8) | ((owner << 24) | (global << 16)));
    for (; snd_voice_list_end_ck(snd_bank_table_get_c1(), node) != 1; node = snd_voice_next(node)) {
        if (node->kind == 2 && key == node->key) {
            ramp_down_set(node, cmd);
        }
    }
}

/* Fades out every voice of the command's id. */
void StreamMgr::cmd_fade_out(StreamCmd* cmd, s32 slot)
{
    StreamNode* node = snd_voice_first_c1(snd_bank_table_get_c1());
    u32 global = cmd->flags & 1;
    u32 owner = global != 0 ? 0xFF : slot;
    u32 key = cmd->id_lo | ((cmd->id_hi << 8) | ((owner << 24) | (global << 16)));
    for (; snd_voice_list_end_ck(snd_bank_table_get_c1(), node) != 1; node = snd_voice_next(node)) {
        if (node->kind == 2 && key == node->key) {
            fade_out_set(node, cmd->fade);
        }
    }
}

/* Fades in every voice of the command's id. */
void StreamMgr::cmd_fade_in(StreamCmd* cmd, s32 slot)
{
    StreamNode* node = snd_voice_first_c1(snd_bank_table_get_c1());
    u32 global = cmd->flags & 1;
    u32 owner = global != 0 ? 0xFF : slot;
    u32 key = cmd->id_lo | ((cmd->id_hi << 8) | ((owner << 24) | (global << 16)));
    for (; snd_voice_list_end_ck(snd_bank_table_get_c1(), node) != 1; node = snd_voice_next(node)) {
        if (node->kind == 2 && key == node->key) {
            fade_in_set(node, cmd->fade);
        }
    }
}

/* Watches the node's read: a finished read queues the next, a disc error pauses the voice. */
void StreamMgr::poll_read(StreamNode* node)
{
    switch (DVDGetCommandBlockStatus(&stream_node_voice(node)->file.cb)) {
    case 0:
        queue_read(node);
        break;
    case -1:
    case 4:
    case 6:
    case 7:
        snd_slot_flag_set(node, 0x2000000);
        pause(node);
        return;
    }
    if (snd_slot_flag_test(node, 0x2000000) != 0) {
        resume(node);
        snd_slot_flag_clear(node, 0x2000000);
    }
}

/* Queues the read of the next buffer third. */
void StreamMgr::queue_read(StreamNode* node)
{
    StreamVoice* voice = stream_node_voice(node);
    u32 size = stream_chunk_size;
    u8 buf = voice->read_buf;
    u8* dst;
    u32 per_channel;
    u32 end;
    u32 limit;
    u32 rounded;

    if (voice->queued_buf != buf && buf != voice->play_buf) {
        if (voice->file.length != 0 && (voice->loop_end == 0 || voice->file.length > voice->file_pos)) {
            dst = voice->buffer + buf * size;
            voice->file.cb.userData = node;
            per_channel = voice->file.length / voice->channels;
            if (per_channel <= stream_half_size) {
                size = per_channel;
                if (voice->loop_end == 0) {
                    voice->at_end = 1;
                }
            } else if (voice->loop_end != 0) {
                if (voice->read_pos + stream_chunk_size > voice->loop_end) {
                    size = voice->loop_end - voice->read_pos;
                    voice->wrapping = 1;
                    voice->wraps++;
                }
            } else {
                end = voice->file_pos + stream_chunk_size;
                if (end > voice->file.length) {
                    size = end - voice->file.length;
                }
                if (voice->at_end == 0 && voice->file.length - stream_end_margin < end) {
                    voice->at_end = 1;
                }
            }
            limit = voice->file.length / voice->channels;
            if (voice->read_pos + size > limit) {
                size = limit - voice->read_pos;
            }
            rounded = stream_round_up(size, 32);
            buf = voice->read_buf;
            snd_slot_flag_set(node, 0x2000 << buf);
            snd_slot_flag_clear(node, 0x10000 << buf);
            voice->queued_buf = voice->read_buf;
            DVDReadAsyncPrio(&voice->file, dst, rounded, voice->read_pos, stream_read_cb, 0);
        }
    }
}

/* Stops a node: halts its voices and cancels its read. */
void StreamMgr::stop(StreamNode* node)
{
    StreamVoice* voice;

    if (node != 0) {
        snd_slot_flag_clear(node, 0x200);
        snd_slot_flag_clear(node, 1);
        snd_slot_flag_set(node, 0x400);
        voice = stream_node_voice(node);
        if (voice->vpb_l != 0) {
            AXSetVoiceState(voice->vpb_l, 0);
        }
        if (voice->vpb_r != 0) {
            AXSetVoiceState(voice->vpb_r, 0);
        }
        voice->file.cb.userData = node;
        DVDCancelAsync(&voice->file.cb, stream_cancel_cb);
    }
}

/* Pauses a node's voices. */
void StreamMgr::pause(StreamNode* node)
{
    StreamVoice* voice;

    if (node != 0) {
        voice = stream_node_voice(node);
        if (voice->vpb_l != 0) {
            AXSetVoiceState(voice->vpb_l, 0);
        }
        if (voice->vpb_r != 0) {
            AXSetVoiceState(voice->vpb_r, 0);
        }
        snd_slot_flag_set(node, 0x200);
    }
}

/* Resumes a paused node's voices. */
void StreamMgr::resume(StreamNode* node)
{
    StreamVoice* voice;

    if (node != 0 && snd_slot_flag_test(node, 0x200) == 1) {
        snd_slot_flag_clear(node, 0x200);
        voice = stream_node_voice(node);
        if (voice->vpb_l != 0) {
            AXSetVoiceState(voice->vpb_l, 1);
        }
        if (voice->vpb_r != 0) {
            AXSetVoiceState(voice->vpb_r, 1);
        }
    }
}

/* Starts a fade-out of `ms` milliseconds. */
void StreamMgr::fade_out_set(StreamNode* node, u32 ms)
{
    StreamVoice* voice = stream_node_voice(node);

    voice->fade_count = 0;
    voice->fade_frames = 5.5555558f * ms;
    snd_slot_flag_clear(node, 0x80000);
    snd_slot_flag_set(node, 0x100000);
}

/* Starts a fade-in of `ms` milliseconds. */
void StreamMgr::fade_in_set(StreamNode* node, u32 ms)
{
    StreamVoice* voice = stream_node_voice(node);

    voice->fade_count = 0;
    voice->fade_frames = 5.5555558f * ms;
    snd_slot_flag_clear(node, 0x100000);
    snd_slot_flag_set(node, 0x80000);
}

/* Starts an absolute volume ramp. */
void StreamMgr::ramp_set(StreamNode* node, StreamCmd* cmd)
{
    StreamVoice* voice = stream_node_voice(node);
    u32 frames;

    voice->ramp_base = voice->out_volume;
    voice->ramp_volume = cmd->volume;
    voice->ramp_count = 0;
    frames = cmd->fade;
    if (frames != 0xFFFFFFFF) {
        frames = 5.5555558f * frames;
    } else {
        frames = 0;
    }
    voice->ramp_frames = frames;
    snd_slot_flag_clear(node, 0x800000);
    snd_slot_flag_clear(node, 0x400000);
    snd_slot_flag_set(node, 0x200000);
}

/* Starts a relative volume ramp. */
void StreamMgr::ramp_up_set(StreamNode* node, StreamCmd* cmd)
{
    StreamVoice* voice = stream_node_voice(node);
    u32 frames;

    if (snd_slot_flag_test(node, 0x80000) != 0) {
        voice->ramp_base = voice->level;
    } else if (snd_slot_flag_test(node, 0x800000) == 0) {
        if (snd_slot_flag_test(node, 0x200000) != 0) {
            return;
        }
        if (snd_slot_flag_test(node, 0x100000) != 0) {
            return;
        }
        voice->ramp_base = voice->out_volume;
    }
    voice->ramp_volume = cmd->volume;
    voice->ramp_count = frames = 0;
    if (cmd->fade != 0xFFFFFFFF) {
        frames = 5.5555558f * cmd->fade;
    }
    voice->ramp_frames = frames;
    snd_slot_flag_clear(node, 0x800000);
    snd_slot_flag_set(node, 0x400000);
}

/* Starts a percentage volume ramp. */
void StreamMgr::ramp_down_set(StreamNode* node, StreamCmd* cmd)
{
    StreamVoice* voice = stream_node_voice(node);
    u32 frames;

    if (snd_slot_flag_test(node, 0x80000) != 0) {
        voice->ramp_base = voice->level;
    } else if (snd_slot_flag_test(node, 0x400000) == 0) {
        if (snd_slot_flag_test(node, 0x200000) != 0) {
            return;
        }
        if (snd_slot_flag_test(node, 0x100000) != 0) {
            return;
        }
        voice->ramp_base = voice->out_volume;
    }
    voice->ramp_volume = cmd->volume;
    voice->ramp_count = frames = 0;
    if (cmd->fade != 0xFFFFFFFF) {
        frames = 5.5555558f * cmd->fade;
    }
    voice->ramp_frames = frames;
    snd_slot_flag_clear(node, 0x400000);
    snd_slot_flag_set(node, 0x800000);
}

/* The play state of stream `id`: 1 loading, 3 paused, 2 playing, 0 stopped or absent. */
u32 stream_status(StreamMgr* mgr, u32 id)
{
    u32 status = 0;
    StreamNode* node;
    StreamVoice* voice;

    for (node = snd_voice_first_c1(snd_bank_table_get_c1()); snd_voice_list_end_ck(snd_bank_table_get_c1(), node) != 1;
         node = snd_voice_next(node)) {
        voice = stream_node_voice(node);
        if (node->kind == 2 && voice->id == id) {
            if (voice->loading == 1) {
                status = 1;
            } else if (snd_slot_flag_test(node, 0x200) != 0) {
                status = 3;
            } else if (snd_slot_flag_test(node, 0x100) != 0) {
                status = 2;
            } else {
                status = 0;
            }
            break;
        }
    }
    return status;
}

/* Constructs the four relocation states of a record and clears its path list. */
inline StreamRelocSet* stream_rec_init(StreamRelocSet* rec)
{
    snd_reloc_state_clear(&rec->formats);
    snd_reloc_state_clear(&rec->groups);
    snd_reloc_state_clear(&rec->cmds);
    snd_reloc_state_clear(&rec->tracks);
    rec->paths = 0;
    rec->path_count = 0;
    return rec;
}

/* Clears the buffer set, the volumes and the three records. */
inline StreamMgr::StreamMgr() : buffer_count(0), buffer_used(0), active(0), master_volume(127)
{
    __construct_array(recs, (void*)stream_rec_init, (void*)stream_rec_destroy, sizeof(StreamRelocSet), 3);
    bank_volume[0] = 127;
    bank_volume[1] = 127;
    bank_volume[2] = 127;
}

StreamMgr g_stream_mgr;
