/* sound/quest_snd.cpp - the AX voice setup for waves and streams, the stream-format address helpers, the sound heap
 * and the chunk install.
 *
 * RANGE. `.text` 0x800EE014..0x800EEAE0 (12 functions, address order); extab/extabindex for the five functions with a
 *   frame; `.bss` snd_heap_allocator, `.sbss` snd_exp_heap, `.sdata2` 0x80796558..0x80796588 (the pitch-ratio pool).
 * FLAGS. The lib's cflags_main (-O3, C++ exceptions on). `#pragma optimization_level 4` file-wide: the format switches
 *   test their middle case range with `subi`+`cmplwi` (playbook 108). `#pragma peephole off` file-wide.
 * NAMES. The file name is the registration's (no `__FILE__` string). GUESS: snd_voice_prepare, snd_stream_voice_prepare
 *   (the SDK `sp` library's SPPrepareSound/SPPrepareEnd shape: format 0..5 = ADPCM/PCM16/PCM8 one-shot/looped,
 *   gain 0x800/0x100, sync bits), snd_adpcm_copy, snd_adpcm_loop_copy; the callee names snd_whd_install,
 *   snd_tsb_install, snd_chunk_ids_snapshot are registry proposals (GUESS).
 * RESIDUALS. none recorded.
 */

#include "types.h"
#include "sound/quest_snd.h"
#include "sound/sound_work.h"
#include "sound/snd_format_voice_init.h"
#include "sound/snd_heap_alloc.h"
#include "sound/snd_heap_free.h"
#include "sound/snd_exp_heap.h"
#include "sound/snd_chunk_install.h"
#include "sound/snd_wk.h"
#include "sound/snd_whd_install.h"
#include "sound/snd_tsb_install.h"
#include "sound/snd_chunk_ids_snapshot.h"
#include "AX/AXVPB.h"
#include "OS/OSDisableInterrupts.h"
#include "OS/OSRestoreInterrupts.h"
#include "MSL/w_math.h"

#pragma peephole off
#pragma optimization_level 4

/* The AXVPB sync bits the setup raises (`AX/AXVPB.c` names them). */
#define SND_SYNC_STATE_FLAGS 0x00000008
#define SND_SYNC_ADDR_ALL 0x00000400
#define SND_SYNC_ADPCM 0x00008000
#define SND_SYNC_SRC_ALL 0x00010000
#define SND_SYNC_ADPCM_LOOP 0x00040000

/* Stores a DSP address as its two PB half-words. */
static inline void snd_pb_addr_set(AXPBAddr* addr, u32 value)
{
    addr->hi = value >> 16;
    addr->lo = value;
}

/* 0x800EE014 (0x4F4): starts `wave` on `voice`: addresses, ADPCM state, the pitch-scaled SRC ratio and the LPF. */
void snd_voice_prepare(AxHandle* wave, AxVoice* voice, s16 pitch, u16 lpf_freq)
{
    SndWaveEntry* entry = (SndWaveEntry*)wave;
    AXVPB* vpb = (AXVPB*)voice;
    SndAdpcm zero = {0};
    AXPBLpf lpf;
    u16 a0;
    u16 b0;
    f64 scale;
    f32 rate;
    u32 ratio;
    BOOL old;

    scale = pow(2.0, pitch / 1200.0);
    rate = entry->sample_rate;
    rate = rate * (f32)scale / 32000.0f;
    ratio = (u32)(65536.0 * rate);
    old = OSDisableInterrupts();
    AXSetVoiceSrcType(vpb, 2);

    switch (entry->format) {
    case 0:
        vpb->pb.addrFormat.first = 0;
        vpb->pb.addrFormat.second = 0;
        snd_pb_addr_set(&vpb->pb.addrLoop, entry->loop_addr);
        snd_pb_addr_set(&vpb->pb.addrEnd, entry->end_addr);
        snd_pb_addr_set(&vpb->pb.addrCurrent, entry->current_addr);
        snd_adpcm_copy((SndAdpcm*)vpb->pb.adpcm, &entry->adpcm.adpcm);
        vpb->pb.srcRatio[0] = ratio >> 16;
        vpb->pb.srcRatio[1] = ratio;
        vpb->pb.srcFraction = 0;
        vpb->pb.srcLast[0] = 0;
        vpb->pb.srcLast[1] = 0;
        vpb->pb.srcLast[2] = 0;
        vpb->pb.srcLast[3] = 0;
        vpb->sync |= SND_SYNC_ADDR_ALL | SND_SYNC_ADPCM | SND_SYNC_SRC_ALL;
        break;
    case 1: {
        SndAdpcmInfo* info = &entry->adpcm;

        vpb->pb.addrFormat.first = 1;
        vpb->pb.addrFormat.second = 0;
        snd_pb_addr_set(&vpb->pb.addrLoop, entry->loop_addr);
        snd_pb_addr_set(&vpb->pb.addrEnd, entry->loop_end_addr);
        snd_pb_addr_set(&vpb->pb.addrCurrent, entry->current_addr);
        snd_adpcm_copy((SndAdpcm*)vpb->pb.adpcm, &info->adpcm);
        vpb->pb.srcRatio[0] = ratio >> 16;
        vpb->pb.srcRatio[1] = ratio;
        vpb->pb.srcFraction = 0;
        vpb->pb.srcLast[0] = 0;
        vpb->pb.srcLast[1] = 0;
        vpb->pb.srcLast[2] = 0;
        vpb->pb.srcLast[3] = 0;
        snd_adpcm_loop_copy(&vpb->pb.adpcmLoop, &info->loop);
        vpb->sync |= SND_SYNC_ADDR_ALL | SND_SYNC_ADPCM | SND_SYNC_SRC_ALL | SND_SYNC_ADPCM_LOOP;
        break;
    }
    case 2:
        vpb->pb.addrFormat.first = 0;
        vpb->pb.addrFormat.second = 10;
        snd_pb_addr_set(&vpb->pb.addrLoop, entry->loop_addr);
        snd_pb_addr_set(&vpb->pb.addrEnd, entry->end_addr);
        snd_pb_addr_set(&vpb->pb.addrCurrent, entry->current_addr);
        snd_adpcm_copy((SndAdpcm*)vpb->pb.adpcm, &zero);
        ((SndAdpcm*)vpb->pb.adpcm)->gain = 0x800;
        vpb->pb.srcRatio[0] = ratio >> 16;
        vpb->pb.srcRatio[1] = ratio;
        vpb->pb.srcFraction = 0;
        vpb->pb.srcLast[0] = 0;
        vpb->pb.srcLast[1] = 0;
        vpb->pb.srcLast[2] = 0;
        vpb->pb.srcLast[3] = 0;
        vpb->sync |= SND_SYNC_ADDR_ALL | SND_SYNC_ADPCM | SND_SYNC_SRC_ALL;
        break;
    case 3:
        vpb->pb.addrFormat.first = 1;
        vpb->pb.addrFormat.second = 10;
        snd_pb_addr_set(&vpb->pb.addrLoop, entry->loop_addr);
        snd_pb_addr_set(&vpb->pb.addrEnd, entry->loop_end_addr);
        snd_pb_addr_set(&vpb->pb.addrCurrent, entry->current_addr);
        snd_adpcm_copy((SndAdpcm*)vpb->pb.adpcm, &zero);
        ((SndAdpcm*)vpb->pb.adpcm)->gain = 0x800;
        vpb->pb.srcRatio[0] = ratio >> 16;
        vpb->pb.srcRatio[1] = ratio;
        vpb->pb.srcFraction = 0;
        vpb->pb.srcLast[0] = 0;
        vpb->pb.srcLast[1] = 0;
        vpb->pb.srcLast[2] = 0;
        vpb->pb.srcLast[3] = 0;
        vpb->sync |= SND_SYNC_ADDR_ALL | SND_SYNC_ADPCM | SND_SYNC_SRC_ALL;
        break;
    case 4:
        vpb->pb.addrFormat.first = 0;
        vpb->pb.addrFormat.second = 25;
        snd_pb_addr_set(&vpb->pb.addrLoop, entry->loop_addr);
        snd_pb_addr_set(&vpb->pb.addrEnd, entry->end_addr);
        snd_pb_addr_set(&vpb->pb.addrCurrent, entry->current_addr);
        snd_adpcm_copy((SndAdpcm*)vpb->pb.adpcm, &zero);
        ((SndAdpcm*)vpb->pb.adpcm)->gain = 0x100;
        vpb->pb.srcRatio[0] = ratio >> 16;
        vpb->pb.srcRatio[1] = ratio;
        vpb->pb.srcFraction = 0;
        vpb->pb.srcLast[0] = 0;
        vpb->pb.srcLast[1] = 0;
        vpb->pb.srcLast[2] = 0;
        vpb->pb.srcLast[3] = 0;
        vpb->sync |= SND_SYNC_ADDR_ALL | SND_SYNC_ADPCM | SND_SYNC_SRC_ALL;
        break;
    case 5:
        vpb->pb.addrFormat.first = 1;
        vpb->pb.addrFormat.second = 25;
        snd_pb_addr_set(&vpb->pb.addrLoop, entry->loop_addr);
        snd_pb_addr_set(&vpb->pb.addrEnd, entry->loop_end_addr);
        snd_pb_addr_set(&vpb->pb.addrCurrent, entry->current_addr);
        snd_adpcm_copy((SndAdpcm*)vpb->pb.adpcm, &zero);
        ((SndAdpcm*)vpb->pb.adpcm)->gain = 0x100;
        vpb->pb.srcRatio[0] = ratio >> 16;
        vpb->pb.srcRatio[1] = ratio;
        vpb->pb.srcFraction = 0;
        vpb->pb.srcLast[0] = 0;
        vpb->pb.srcLast[1] = 0;
        vpb->pb.srcLast[2] = 0;
        vpb->pb.srcLast[3] = 0;
        vpb->sync |= SND_SYNC_ADDR_ALL | SND_SYNC_ADPCM | SND_SYNC_SRC_ALL;
        break;
    }

    if (lpf_freq == 0) {
        lpf.on = 0;
        lpf.yn1 = 0;
        lpf.a0 = 0;
        lpf.b0 = 0;
    } else {
        AXGetLpfCoefs(lpf_freq, &a0, &b0);
        lpf.on = 1;
        lpf.yn1 = 0;
        lpf.a0 = a0;
        lpf.b0 = b0;
    }
    AXSetVoiceLpf(vpb, &lpf);
    OSRestoreInterrupts(old);
}

/* 0x800EE508 (0x1C): copies an ADPCM loop context. */
void snd_adpcm_loop_copy(AXPBAdpcmLoop* dst, AXPBAdpcmLoop* src)
{
    dst->predScale = src->predScale;
    dst->yn1 = src->yn1;
    dst->yn2 = src->yn2;
}

/* 0x800EE524 (0x64): copies an ADPCM decoder state. */
void snd_adpcm_copy(SndAdpcm* dst, SndAdpcm* src)
{
    *dst = *src;
}

/* 0x800EE588 (0x1F8): sets one channel of a stream up on `vpb` as a looping stream voice at SRC `ratio`. */
void snd_stream_voice_prepare(StreamFormat* format, AXVPB* vpb, StreamChannel* channel, u32 ratio)
{
    vpb->pb.srcRatio[0] = ratio >> 16;
    vpb->pb.srcRatio[1] = ratio;
    vpb->pb.srcFraction = 0;
    vpb->pb.srcLast[0] = 0;
    vpb->pb.srcLast[1] = 0;
    vpb->pb.srcLast[2] = 0;
    vpb->pb.srcLast[3] = 0;
    SndAdpcm zero = {0};

    switch (format->codec) {
    case 0:
        vpb->pb.addrFormat.first = 1;
        vpb->pb.addrFormat.second = 0;
        snd_adpcm_copy((SndAdpcm*)vpb->pb.adpcm, &channel->adpcm.adpcm);
        snd_adpcm_loop_copy(&vpb->pb.adpcmLoop, &channel->adpcm.loop);
        vpb->pb.stateFlags = 1;
        vpb->sync |= SND_SYNC_ADDR_ALL | SND_SYNC_ADPCM | SND_SYNC_SRC_ALL | SND_SYNC_ADPCM_LOOP;
        break;
    case 1:
        vpb->pb.addrFormat.first = 1;
        vpb->pb.addrFormat.second = 0;
        snd_adpcm_copy((SndAdpcm*)vpb->pb.adpcm, &channel->adpcm.adpcm);
        snd_adpcm_loop_copy(&vpb->pb.adpcmLoop, &channel->adpcm.loop);
        vpb->pb.stateFlags = 1;
        vpb->sync |= SND_SYNC_ADDR_ALL | SND_SYNC_ADPCM | SND_SYNC_SRC_ALL | SND_SYNC_ADPCM_LOOP | SND_SYNC_STATE_FLAGS;
        return;
    case 2:
        vpb->pb.addrFormat.first = 1;
        vpb->pb.addrFormat.second = 10;
        snd_adpcm_copy((SndAdpcm*)vpb->pb.adpcm, &zero);
        ((SndAdpcm*)vpb->pb.adpcm)->gain = 0x800;
        vpb->pb.stateFlags = 1;
        break;
    case 3:
        vpb->pb.addrFormat.first = 1;
        vpb->pb.addrFormat.second = 10;
        snd_adpcm_copy((SndAdpcm*)vpb->pb.adpcm, &zero);
        ((SndAdpcm*)vpb->pb.adpcm)->gain = 0x800;
        vpb->pb.stateFlags = 1;
        break;
    case 4:
        vpb->pb.addrFormat.first = 1;
        vpb->pb.addrFormat.second = 25;
        snd_adpcm_copy((SndAdpcm*)vpb->pb.adpcm, &zero);
        ((SndAdpcm*)vpb->pb.adpcm)->gain = 0x100;
        vpb->pb.stateFlags = 1;
        break;
    case 5:
        vpb->pb.addrFormat.first = 1;
        vpb->pb.addrFormat.second = 25;
        snd_adpcm_copy((SndAdpcm*)vpb->pb.adpcm, &zero);
        ((SndAdpcm*)vpb->pb.adpcm)->gain = 0x100;
        vpb->pb.stateFlags = 1;
        break;
    }
    vpb->sync |= SND_SYNC_ADDR_ALL | SND_SYNC_ADPCM | SND_SYNC_SRC_ALL | SND_SYNC_STATE_FLAGS;
}

/* 0x800EE780 (0xE8): sets both channels of a stream up on their voices at the pitch-scaled SRC ratio. */
void snd_format_voice_init(StreamFormat* format, AXVPB* l, AXVPB* r, s16 pitch)
{
    f64 scale;
    f32 rate;
    u32 ratio;
    BOOL old;

    scale = pow(2.0, pitch / 1200.0);
    rate = format->sample_rate;
    rate = rate * (f32)scale / 32000.0f;
    ratio = (u32)(65536.0 * rate);
    old = OSDisableInterrupts();
    if (l != 0) {
        snd_stream_voice_prepare(format, l, &format->channel[0], ratio);
    }
    if (r != 0) {
        snd_stream_voice_prepare(format, r, &format->channel[1], ratio);
    }
    OSRestoreInterrupts(old);
}

/* 0x800EE868 (0x4C): converts a byte count to a DSP address offset for the format's codec. */
u32 snd_format_bytes_to_addr(StreamFormat* format, u32 bytes)
{
    u32 r = 0;

    switch (format->codec) {
    case 0:
    case 1:
        r = bytes << 1;
        break;
    case 2:
    case 3:
        r = bytes >> 1;
        break;
    case 4:
    case 5:
        r = bytes;
        break;
    }
    return r;
}

/* 0x800EE8B4 (0x4C): converts a DSP address offset to a byte count for the format's codec. */
u32 snd_format_addr_to_bytes(StreamFormat* format, u32 addr)
{
    u32 r = 0;

    switch (format->codec) {
    case 0:
    case 1:
        r = addr >> 1;
        break;
    case 2:
    case 3:
        r = addr << 1;
        break;
    case 4:
    case 5:
        r = addr;
        break;
    }
    return r;
}

/* 0x800EE900 (0x78): converts a sample count to a byte count (ADPCM: 14 samples per 8-byte frame). */
u32 snd_format_samples_to_bytes(StreamFormat* format, u32 samples)
{
    u32 r = 0;

    switch (format->codec) {
    case 0:
    case 1:
        r = samples % 14 + (samples / 14) * 16;
        r >>= 1;
        break;
    case 2:
    case 3:
        r = samples << 1;
        break;
    case 4:
    case 5:
        r = samples;
        break;
    }
    return r;
}

/* 0x800EE978 (0x74): converts a sample count to a DSP address offset (ADPCM: 16 nibbles per 14 samples). */
u32 snd_format_samples_to_addr(StreamFormat* format, u32 samples)
{
    u32 r = 0;

    switch (format->codec) {
    case 0:
    case 1:
        r = samples % 14 + (samples / 14) * 16;
        break;
    case 2:
    case 3:
        r = samples << 1;
        break;
    case 4:
    case 5:
        r = samples;
        break;
    }
    return r;
}

/* 0x800EE9EC (0x40): allocates `size` bytes from the sound heap's allocator (null for a zero request). */
void* snd_heap_alloc(u32 size)
{
    void* p = 0;

    if (size != 0) {
        p = MEMAllocFromAllocator(snd_heap_allocator, size);
    }
    return p;
}

/* 0x800EEA2C (0x18): frees a block back to the sound expand heap. */
void snd_heap_free(void* p)
{
    if (p == 0) {
        return;
    }
    MEMFreeToExpHeap((void*)snd_exp_heap, p);
}

/* 0x800EEA44 (0x9C): installs chunk `chunk`'s `whd`/`tsb` pair into stream handle `ctx` and records the handle. */
void snd_chunk_install(u32 ctx, u32 chunk)
{
    snd_whd_install(ctx, (RelocDesc*)snd_wk->whd_ofs[chunk]);
    snd_tsb_install(ctx, (RelocDesc*)snd_wk->tsb_ofs[chunk]);
    snd_wk->whd_installed[chunk] = 1;
    snd_wk->tsb_installed[chunk] = 1;
    snd_wk->chunk_handle[chunk] = ctx;
    snd_chunk_ids_snapshot(ctx);
}

/* The sound heap: the allocator record `snd_heap_alloc` allocates from, and the expand heap `snd_heap_free` frees
 * into. */
u8 snd_heap_allocator[0x10];
u32 snd_exp_heap;
