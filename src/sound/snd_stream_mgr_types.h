/* sound/snd_stream_mgr_types.h - the stream command record `sound/snd_stream_mgr.cpp` defines and
 * `sound/fn_800F2A94.cpp` builds (docs/plan.md 6.5 rule 1: a type two units use lives in one header). */
#ifndef MHTRI_SOUND_SND_STREAM_MGR_TYPES_H
#define MHTRI_SOUND_SND_STREAM_MGR_TYPES_H

#include "types.h"

/* Two caller words a stream command carries through. */
struct StreamCmdUser {
    /* +0x00 */ u32 words[2];
}; /* size: 0x8 */

/* A stream command (the bank's command table entries and the manager's immediate command). */
struct StreamCmd {
    /* +0x00 */ u8 type;          /* 1 start, 2 stop, 3 pause, 4 resume, 5 random, 6..10 the ramps */
    /* +0x01 */ u8 volume_random;  /* signed: a positive range randomises the level */
    /* +0x02 */ u8 group;
    /* +0x03 */ u8 priority;
    /* +0x04 */ u8 flags;         /* bit 0: a global (slot-less) id */
    /* +0x05 */ u8 track;
    /* +0x06 */ u8 id_lo;
    /* +0x07 */ u8 id_hi;
    /* +0x08 */ u8 pan;
    /* +0x09 */ u8 max_voices;    /* 0xFF: unlimited */
    /* +0x0A */ u8 next;          /* the chained command, 0xFF ends the chain */
    /* +0x0B */ u8 fader;
    /* +0x0C */ s16 pitch_random;
    /* +0x0E */ s16 volume;
    /* +0x10 */ s16 pitch;
    /* +0x12 */ s16 unused_0x12;
    /* +0x14 */ u32 duration;     /* milliseconds, (u32)-1 for none */
    /* +0x18 */ u32 hold;
    /* +0x1C */ u32 fade;
    /* +0x20 */ u16 param_0x20;
    /* +0x22 */ u16 param_0x22;
    /* +0x24 */ u16 param_0x24;
    /* +0x26 */ u16 param_0x26;
    /* +0x28 */ StreamCmdUser user;
}; /* size: 0x30 */

#endif /* MHTRI_SOUND_SND_STREAM_MGR_TYPES_H */
