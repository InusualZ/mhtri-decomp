/*
 * SEQ/seq.c - a MIDI sequencer: its init and quit, the MIDI event dispatch and the per-frame sequence runner.
 *
 * RANGE. .text 0x804DD350-0x804DD9C0 (5 functions, 0x670 B); .data 0x80629F18-0x80629F98; .sbss
 *    0x80795468-0x80795470; .sdata2 0x8079D330-0x8079D350.  Cut from the old SC block between
 *    `SC/SCProductInfo.c` (0x804DD350) and `SI/SIBios.c` (0x804DD9C0).
 * FLAGS. the `OS` lib's group with `cflags_base` (configure.py `cflags=`): every function start of the band is
 *    16-aligned (`-O4,p` default alignment), not the 4-byte packing `cflags_os` gives.
 * NAMES. the library name `SEQ`, the file name `seq.c` and every symbol are GUESSES read off the bodies (`SEQHandleEvent`,
 *    `SEQInit`, `SEQQuit`, `SEQRunAudioFrame`, `SEQSetState`, `SYNMIDIInput`, `MidiMessageLengthTable`, the type and
 *    field names); none of the five rows was named in the map or the dump.
 * EVIDENCE. `.data` 0x80629F18 (0x80 B) is a MIDI message length table indexed by status byte - 0x80, read by the event
 *    decode at 0x804DD350 (which writes events through 0x804DF1B0 of the synthesizer unit); `.sbss`
 *    0x80795468/0x8079546C is the init flag and the sequence list head read by 0x804DD430, 0x804DD450 and
 *    0x804DD460; `.sdata2` 0x8079D330..0x8079D350 are the tempo/tick constants of the per-frame runner; the
 *    callers are `sound/snd_stream_reloc.cpp` (init at 0x804DD430, per frame at 0x804DD460).
 * RESIDUALS. `SEQRunAudioFrame` 0x804DD460 (96.5 %): the .sdata2 pool is created in a different order (target 65536, 96,
 *    signed-int double, 1e6, 32000, unsigned-int double; ours 1e6, 65536, 96, 32000, ...), so seven pool relocations name other
 *    constants; the tempo case's variable-length-quantity value is dead in ours and kept in the target (24 B shorter);
 *    `SEQSetState` 0x804DD870 (98.9 %): track pointer r8 where the target uses r7; `SEQHandleEvent` 0x804DD350 (98.6 %): the
 *    cursor loads use r3/r5/r0 where the target uses r5/r3/r0. flipcheck: .text 0x654 vs 0x670, .sdata2 order.
 * SHAPES. the track cursor lives in the track record and is re-read after every byte (a variable-length quantity
 *    reader expanded in place).
 */

#include "types.h"

#include "OS/OSInterrupt.h"
#include "SEQ/seq.h"
#include "SYN/syn.h"

typedef struct SEQSequence SEQSequence;
typedef struct SEQTrack SEQTrack;

/* Invoked for a MIDI controller change whose controller number has a hook. */
typedef void (*SEQControllerCallback)(SEQTrack* track, u32 controller);

/* size: 0x28 - one track of a loaded sequence. */
struct SEQTrack {
    /* +0x00 */ SEQSequence* sequence;
    /* +0x04 */ u8* start;
    /* +0x08 */ u8* end;
    /* +0x0C */ u8* cursor;
    /* +0x10 */ u8 runningStatus;
    /* +0x11 */ u8 pad_0x11[3];
    /* +0x14 */ f32 quarterNotesPerSecond;
    /* +0x18 */ u32 initialPulsesPerFrame;
    /* +0x1C */ u32 pulsesPerFrame;
    /* +0x20 */ u32 wait;
    /* +0x24 */ u32 active;
};

/* size: 0x3350 + 0x28 * trackCount - a loaded sequence; `tracks` is the first of `trackCount` records (the extent
 * is an approximation: one track is declared). */
struct SEQSequence {
    /* +0x00 */ SEQSequence* next;
    /* +0x04 */ u32 state;
    /* +0x08 */ u16 trackCount;
    /* +0x0A */ s16 division;
    /* +0x0C */ s32 activeTracks;
    /* +0x10 */ s32 finished;
    /* +0x14 */ SYNSynth synth;
    /* +0x3150 */ SEQControllerCallback controllerCallbacks[128];
    /* +0x3350 */ SEQTrack tracks[1];
};

static u8 MidiMessageLengthTable[128] = {
    2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
    2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
    2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
    2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
    0, 0, 2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
};

static SEQSequence* SequenceList;
static s32 SequencerInitialized;

/* Reads a MIDI variable-length quantity at the cursor, leaving the cursor on its last byte. */
static inline u32 SEQReadVariableLength(SEQTrack* track)
{
    u8 byte = *track->cursor;
    u32 value = byte & 0x7F;
    while (byte & 0x80) {
        track->cursor++;
        byte = *track->cursor;
        value = (value << 7) + (byte & 0x7F);
    }
    return value;
}

/* Skips a length-prefixed event body (a system exclusive or an unhandled meta event). */
static inline void SEQSkipEvent(SEQTrack* track)
{
    u32 length = SEQReadVariableLength(track);
    track->cursor++;
    track->cursor += length;
}

/* Marks a track finished and flags its sequence when it was the last running track. */
static inline void SEQFinishTrack(SEQTrack* track)
{
    SEQSequence* sequence = track->sequence;
    sequence->activeTracks--;
    track->active = 0;
    if (sequence->activeTracks == 0) {
        sequence->finished = 1;
    }
}

#pragma dont_inline on
void SEQHandleEvent(SYNSynth* synth, SEQTrack* track)
{
    u8 message[3];
    message[0] = track->runningStatus;

    switch ((u32)MidiMessageLengthTable[message[0] - 0x80]) {
    case 1:
        message[1] = *track->cursor++;
        break;
    case 2:
        message[1] = *track->cursor++;
        message[2] = *track->cursor++;
        break;
    }

    if ((message[0] & 0xF0) == 0xB0) {
        SEQControllerCallback callback = track->sequence->controllerCallbacks[message[1]];
        if (callback != NULL) {
            callback(track, message[1]);
        }
    }
    SYNMIDIInput(synth, message);
}
#pragma dont_inline reset

void SEQInit(void)
{
    if (SequencerInitialized != 0) {
        return;
    }
    SequenceList = NULL;
    SequencerInitialized = 1;
}

void SEQQuit(void)
{
    SequenceList = NULL;
    SequencerInitialized = 0;
}

void SEQSetState(SEQSequence* sequence, u32 state);

void SEQRunAudioFrame(void)
{
    SEQTrack* track;
    SEQSequence* sequence = SequenceList;

    if (SequencerInitialized == 0) {
        return;
    }
    for (; sequence != NULL; sequence = sequence->next) {
        if (sequence->state - 1 <= 1) {
            u32 i;
            u32 delay;
            track = sequence->tracks;
            for (i = 0; i < sequence->trackCount; track++, i++) {
                u32 pulses;
                if (track->active - 1 > 1) {
                    continue;
                }
                pulses = track->pulsesPerFrame;
                if (track->wait > pulses) {
                    track->wait -= pulses;
                    continue;
                }
                while (pulses >= track->wait) {
                    pulses -= track->wait;
                    if (*track->cursor >= 0x80) {
                        track->runningStatus = *track->cursor;
                        track->cursor++;
                    }
                    switch (track->runningStatus) {
                    case 0xF0:
                    case 0xF7:
                        SEQSkipEvent(track);
                        break;
                    case 0xFF: {
                        s32 type = *track->cursor;
                        track->cursor++;
                        switch (type) {
                        case 0x2F:
                            SEQFinishTrack(track);
                            break;
                        case 0x51: {
                            u32 microseconds;
                            f32 rate;
                            SEQReadVariableLength(track);
                            microseconds = *++track->cursor;
                            microseconds = (microseconds << 8) + *++track->cursor;
                            microseconds = (microseconds << 8) + *++track->cursor;
                            track->cursor++;
                            rate = 1000000.0f / (f32)microseconds;
                            track->quarterNotesPerSecond = rate;
                            track->pulsesPerFrame =
                                (u32)(65536.0f * (96.0f / (32000.0f / rate / (f32)track->sequence->division)));
                            break;
                        }
                        default:
                            SEQSkipEvent(track);
                            break;
                        }
                        break;
                    }
                    default:
                        SEQHandleEvent(&sequence->synth, track);
                        break;
                    }
                    if (track->cursor >= track->end) {
                        SEQFinishTrack(track);
                    }
                    if (track->active == 0) {
                        break;
                    }
                    delay = SEQReadVariableLength(track);
                    track->cursor++;
                    track->wait = delay << 16;
                }
                track->wait -= pulses;
            }
        }
        if (sequence->finished != 0) {
            if (sequence->state == 2) {
                SEQSetState(sequence, 0);
                SEQSetState(sequence, 2);
            } else {
                SEQSetState(sequence, 0);
            }
        }
    }
}

void SEQSetState(SEQSequence* sequence, u32 state)
{
    switch (state) {
    case 1:
    case 2:
        if (sequence->state == 0) {
            u32 delay;
            BOOL level = OSDisableInterrupts();
            SEQTrack* track = sequence->tracks;
            s32 i;
            for (i = 0; i < sequence->trackCount; i++, track++) {
                track->cursor = track->start;
                track->pulsesPerFrame = track->initialPulsesPerFrame;
                delay = SEQReadVariableLength(track);
                track->cursor++;
                track->wait = delay << 16;
                track->active = 1;
            }
            sequence->activeTracks = sequence->trackCount;
            OSRestoreInterrupts(level);
        }
        sequence->finished = 0;
        break;
    case 0:
    case 3: {
        s32 channel;
        for (channel = 0; channel < 16; channel++) {
            u8 message[3];
            BOOL level = OSDisableInterrupts();
            message[0] = channel | 0xB0;
            message[1] = 123;
            message[2] = 0;
            SYNMIDIInput(&sequence->synth, message);
            OSRestoreInterrupts(level);
        }
        break;
    }
    }
    sequence->state = state;
}
