/*
 * SYN/syn.c - a software synthesizer on top of AX, part one: voice init, the per-frame run, MIDI input and the event
 *    handlers.
 *
 * RANGE. .text 0x804DF0A0-0x804DFC50 (9 functions, 0xBB0 B); .bss 0x80757828-0x8075AFC0; .sbss 0x80795488-0x807954A0;
 *    .sdata2 0x8079D350-0x8079D360.  Cut from the old SC block between `SI/SIBios.c` (0x804DF0A0) and
 *    `SYN/synenv.c` (0x804DFC50).
 * FLAGS. the `OS` lib's group with `cflags_base` (configure.py `cflags=`): every function start of the band is
 *    16-aligned (`-O4,p` default alignment), not the 4-byte packing `cflags_os` gives.
 * NAMES. the library name `SYN` and the file name `syn.c` are GUESSES read off the bodies; no row is named in the map
 *    or the dump.  `SYNInit`, `SYNQuit`, `SYNRunAudio`, `SYNControlChange`, `SYNResetControllers`, `SYNNote`,
 *    `SYNHandleMessage`, `SYNDrainEvents` and the globals `SYNVoicePool`, `SYNVoices`, `SYNInitialized`,
 *    `SYNSynthList`, `SYNVoiceCount` are GUESSES from what the bodies do (MIDI channel messages: note on/off,
 *    controller change, program change, pitch bend).   The MIX functions it calls (`MIXInitChannel`, `MIXReleaseChannel`) are named from their use (MIX unit).
 * EVIDENCE. `.bss` 0x80757828 (0x3798 B of 0x94 B records, initialised by the loop of `SYNInit` that also calls
 *    `AXIsInit` and the AX voice count) and `.sbss` 0x80795488..0x807954A0 (read by the third unit too) are
 *    defined here; `.sdata2` 0x8079D350 and 0x8079D358 are the constants of `SYNHandleMessage`, the second the
 *    `0x4330000080000000` int-to-double constant, held again at 0x8079D378 (`SYN/synenv.c`) and 0x8079D380
 *    (`SYN/synvoice.c`): one TU pools a value once, so the three are separate TUs; the callers are
 *    `sound/snd_stream_reloc.cpp` (init and per frame) and the sequencer (`SYNMIDIInput`).
 * RESIDUALS. `SYNControlChange` 0x804DF200 (97.8 %): the controller-value store is `stbx` where the target adds the
 *    channel row and the controller index first and folds 0x78 into the displacement (one instruction pair);
 *    `SYNHandleMessage` 0x804DFA60 (99.7 %): `status` and `type` swap r0/r6.  flipcheck: `SYNVolumeTable` is defined by
 *    `SYN/synvoice.c` (not flipped); .bss/.sbss shortfalls are link padding before the next unit.
 * SHAPES. the note-off, all-notes-off and bend-range steps are static inline helpers expanded in place (two copies each
 *    in the target); `SYNMIDIInput` and `SYNHandleMessage` take a non-const `u8*` (a const message lets the scheduler hoist
 *    the loads above the queue stores); the controller reset loops index `synth->controller[channel][i]` directly (a
 *    hoisted row pointer changes the unroll).
 */

#include "types.h"

#include "AX/AX.h"
#include "AX/AXAlloc.h"
#include "AX/AXVPB.h"
#include "MIX/mix.h"
#include "SYN/syn.h"
#include "SYN/synenv.h"
#include "SYN/synvoice.h"

SYNVoice SYNVoicePool[96];
u32 SYNVoiceCount;
SYNSynth* SYNSynthList;
s32 SYNInitialized;
SYNVoice* SYNVoices;

void SYNDrainEvents(SYNSynth* synth);
void SYNControlChange(SYNSynth* synth, u8 channel, u8 controller, u8 value);
void SYNResetControllers(SYNSynth* synth, u8 channel);
void SYNNote(SYNSynth* synth, u8 channel, u8 key, u8 velocity);
void SYNHandleMessage(SYNSynth* synth, u8* message);

/* Releases every sounding note of a channel. */
static inline void SYNReleaseChannelNotes(SYNSynth* synth, u8 channel)
{
    s32 key;
    SYNVoice** slot = synth->noteVoice[channel];

    for (key = 0; key < 128; key++, slot++) {
        if (*slot != NULL) {
            SYNVoiceRelease(*slot, synth->releasePriority);
            *slot = NULL;
        }
    }
}

/* Releases or defers the note of a key. */
static inline void SYNNoteOff(SYNSynth* synth, u8 channel, u8 key)
{
    SYNVoice** slot = &synth->noteVoice[channel][key];

    if (*slot != NULL) {
        if (synth->controller[channel][64] > 64) {
            (*slot)->releasePending = 1;
            return;
        }
        SYNVoiceRelease(*slot, synth->releasePriority);
        *slot = NULL;
    }
}

/* Sets a channel's pitch-bend range from its data-entry controllers when an RPN 0 is selected. */
static inline void SYNSetBendRange(SYNSynth* synth, u8 channel)
{
    u8* controller = synth->controller[channel];

    if (synth->rpnSelected[channel] != 0 && (u16)((controller[101] << 8) + controller[100]) == 0) {
        synth->pitchBendRange[channel] = (controller[38] + controller[6] * 100) << 16;
    }
}

void SYNInit(void)
{
    u32 i;

    if (AXIsInit() != 0 && SYNInitialized == 0) {
        SYNVoiceCount = AXGetMaxVoices();
        SYNVoices = SYNVoicePool;
        for (i = 0; i < SYNVoiceCount; i++) {
            SYNVoices[i].synth = NULL;
        }
        SYNSynthList = NULL;
        SYNInitialized = 1;
    }
}

void SYNQuit(void)
{
    SYNVoices = NULL;
    SYNInitialized = 0;
}

void SYNRunAudio(void)
{
    u32 i;
    SYNSynth* synth;

    if (SYNInitialized != 0) {
        for (i = 0; i < SYNVoiceCount; i++) {
            SYNVoiceRun(i);
        }
        for (synth = SYNSynthList; synth != NULL; synth = synth->next) {
            SYNDrainEvents(synth);
        }
    }
}

void SYNMIDIInput(SYNSynth* synth, u8* message)
{
    *synth->eventWrite = message[0];
    synth->eventWrite++;
    *synth->eventWrite = message[1];
    synth->eventWrite++;
    *synth->eventWrite = message[2];
    synth->eventWrite++;
    synth->eventCount++;
}

void SYNControlChange(SYNSynth* synth, u8 channel, u8 controller, u8 value)
{
    synth->controller[channel][controller] = value;
    switch (controller) {
    case 6:
        SYNSetBendRange(synth, channel);
        break;
    case 7:
        synth->channelVolume[channel] = SYNVolumeTable[value];
        break;
    case 11:
        synth->expression[channel] = SYNVolumeTable[value];
        break;
    case 38:
        SYNSetBendRange(synth, channel);
        break;
    case 64:
        if (value < 64) {
            s32 key;
            SYNVoice** slot = synth->noteVoice[channel];

            for (key = 0; key < 128; key++, slot++) {
                SYNVoice* voice = *slot;
                if (voice != NULL && voice->releasePending != 0) {
                    SYNVoiceRelease(voice, synth->releasePriority);
                    voice->synth->noteVoice[voice->channel][voice->key] = NULL;
                }
            }
        }
        break;
    case 91:
        synth->reverbSend[channel] = SYNVolumeTable[value];
        break;
    case 92:
        synth->chorusSend[channel] = SYNVolumeTable[value];
        break;
    case 98:
    case 99:
        synth->rpnSelected[channel] = 0;
        break;
    case 100:
    case 101:
        synth->rpnSelected[channel] = 1;
        break;
    case 120:
        SYNReleaseChannelNotes(synth, channel);
        break;
    case 121:
        if (value == 0) {
            SYNResetControllers(synth, channel);
        } else {
            s32 count;

            synth->pitchBendRange[channel] = 0xC80000;
            synth->pitchBend[channel] = 0;
            for (count = 0; count < 128; count++) {
                synth->controller[channel][count] = 0;
            }
            SYNControlChange(synth, channel, 7, 100);
            SYNControlChange(synth, channel, 10, 64);
            SYNControlChange(synth, channel, 11, 127);
            SYNControlChange(synth, channel, 91, 0);
            SYNControlChange(synth, channel, 92, 0);
        }
        break;
    case 123:
    case 124:
    case 125:
    case 126:
    case 127:
        SYNReleaseChannelNotes(synth, channel);
        break;
    }
}

void SYNResetControllers(SYNSynth* synth, u8 channel)
{
    u8 volume;
    u8 pan;
    u8 expression;
    s32 count;

    synth->pitchBendRange[channel] = 0xC80000;
    synth->pitchBend[channel] = 0;
    volume = synth->controller[channel][7];
    pan = synth->controller[channel][10];
    expression = synth->controller[channel][11];
    for (count = 0; count < 128; count++) {
        synth->controller[channel][count] = 0;
    }
    SYNControlChange(synth, channel, 7, volume);
    SYNControlChange(synth, channel, 10, pan);
    SYNControlChange(synth, channel, 11, expression);
    SYNControlChange(synth, channel, 91, 0);
    SYNControlChange(synth, channel, 92, 0);
}

void SYNNote(SYNSynth* synth, u8 channel, u8 key, u8 velocity)
{
    SYNVoice** slot;
    SYNVoice** exclusiveSlot;
    AXVPB* axVoice;
    SYNVoice* voice;
    SYNVoice* other;

    if (velocity != 0) {
        slot = &synth->noteVoice[channel][key];
        if (*slot != NULL) {
            SYNVoiceRelease(*slot, synth->releasePriority);
            *slot = NULL;
        }
        axVoice = AXAcquireVoice(synth->acquirePriority, (AXVPBCallback)SYNVoiceFreedCallback, (u32)synth);
        if (axVoice != NULL) {
            voice = &SYNVoices[axVoice->index];
            voice->axVoice = axVoice;
            voice->synth = synth;
            voice->channel = channel;
            voice->key = key;
            voice->velocity = velocity;
            voice->releasePending = 0;
            if (SYNVoiceBindLayer(voice) != 0) {
                *slot = voice;
                synth->voiceCount++;
                voice->exclusiveClass = voice->layer->exclusiveClass;
                if (voice->exclusiveClass != 0) {
                    exclusiveSlot = &synth->exclusiveVoice[channel][voice->exclusiveClass];
                    other = *exclusiveSlot;
                    if (other != NULL) {
                        other->synth = NULL;
                        MIXReleaseChannel(other->axVoice);
                        AXFreeVoice(other->axVoice);
                        synth->noteVoice[channel][other->key] = NULL;
                        synth->voiceCount--;
                    }
                    synth->exclusiveVoice[channel][voice->exclusiveClass] = voice;
                }
                SYNVoiceInitPitch(voice);
                SYNVoiceInitBaseVolume(voice);
                SYNVoiceInitPan(voice);
                SYNVoiceInitLfo(voice);
                SYNSetupVolumeEnvelope(voice);
                SYNSetupModulationEnvelope(voice);
                if (channel == 9) {
                    MIXInitChannel(axVoice, 0, SYNVoiceVolume(voice), synth->reverbSend[channel] >> 16,
                                synth->chorusSend[channel] >> 16, -960, voice->pan, 127,
                                SYNVoiceChannelVolume(voice));
                } else {
                    MIXInitChannel(axVoice, 0, SYNVoiceVolume(voice), synth->reverbSend[channel] >> 16,
                                synth->chorusSend[channel] >> 16, -960, synth->controller[channel][10], 127,
                                SYNVoiceChannelVolume(voice));
                }
                if (synth->noteCallback != NULL) {
                    synth->noteCallback(axVoice, synth, channel);
                }
                SYNVoiceSetupSample(voice);
                SYNVoiceStartPitch(voice);
                axVoice->pb.running = 1;
                axVoice->sync |= 4;
                AXSetVoicePriority(axVoice, synth->voicePriority);
            } else {
                voice->synth = NULL;
                MIXReleaseChannel(axVoice);
                AXFreeVoice(axVoice);
            }
        }
    } else {
        SYNNoteOff(synth, channel, key);
    }
}

void SYNHandleMessage(SYNSynth* synth, u8* message)
{
    u8 status = message[0];
    u8 channel = status & 0xF;
    s32 type = status >> 4;
    u8 data1;
    SYNKeyMap** program;
    s32 bend;

    data1 = message[1];
    switch ((u8)type) {
    case 8:
        SYNNoteOff(synth, channel, data1);
        break;
    case 9:
        SYNNote(synth, channel, data1, message[2]);
        break;
    case 11:
        SYNControlChange(synth, channel, data1, message[2]);
        break;
    case 12:
        if (channel == 9) {
            program = &synth->keyMap[channel];
            *program = synth->drumKeyMaps;
        } else {
            program = &synth->keyMap[channel];
            *program = synth->melodicKeyMaps;
        }
        *program += data1;
        break;
    case 14:
        bend = (data1 + (message[2] << 7)) - 0x2000;
        synth->pitchBend[channel] = (s32)((f32)bend / 8192.0f * (f32)synth->pitchBendRange[channel]);
        break;
    }
}

void SYNDrainEvents(SYNSynth* synth)
{
    u8* message = synth->eventQueue;

    while (synth->eventCount != 0) {
        SYNHandleMessage(synth, message);
        message += 3;
        synth->eventCount--;
    }
    synth->eventWrite = synth->eventQueue;
}
