/* WPAD/WPADHIDParser.cpp - the Wii remote report parser (the SDK's WPADHIDParser.c): the extension and calibration
 *   handshakes, the input-report dispatch table and the report decoders.
 * RANGE. .text 0x804F2FF0-0x804F72C0 (25 functions); .rodata 0x80573CC0-0x80573CD8 (`wpadExtZero`); .data 0x8062C358-0x8062C8C8
 *   (the report handler table, then the parser's strings and jump tables); .bss 0x8075EA00-0x8075EA68; .sbss
 *   0x80795700-0x80795718; .sdata2 0x8079D3D0-0x8079D430.  Seam with WPAD/wpad.cpp at 0x804F2FF0 / 0x8062C358: .data has a
 *   4-byte hole 0x8062C354-0x8062C358 after `WPADiSendSetReportType`'s jump table, the handler table starts 8-aligned at
 *   0x8062C358 and the first string after it ("detaching extension during initialization.") is read by `wpadExtInitCallback`
 *   0x804F2FF0; `wpadExtZero` follows `WPADControlDpd`'s .rodata; and the parser calls the 8-byte WPAD.c functions
 *   `WPADiGetReserved0..3` out of line.  Seam with WPAD/WPADEncrypt.cpp at 0x804F72C0 / 0x8062C8C8: .data has a 4-byte hole
 *   0x8062C8C4-0x8062C8C8 after "invalid ack!", and the key tables and debug strings from 0x8062C8C8 are read only by the
 *   key generators.  The .sdata2 pool repeats constants WPAD/WPADClamp.cpp's pool holds again (the double 4330000080000000 at
 *   0x8079D418 and 0x8079D460, 0.0f at 0x8079D3E0 and 0x8079D46C, -1.0f at 0x8079D408 and 0x8079D470), so the two are
 *   separate TUs.  The .bss arrays at 0x8075EA00, `wpadExtRaw`, the .sbss rows and `wpadExtZero` are read only by this unit.
 * FLAGS. `cflags_base` plus `-fp_contract off` (configure.py): the target has no fused multiply-add and keeps unfused
 *   fmuls->fadds pairs in `wpadAccCalibrationReadDone` (2) and `wpadDecodeDpd` (2); `-fp_contract on` fuses two in
 *   `wpadDecodeDpd` (an fmsubs and an fmadds), the flag leaves none and `wpadDecodeDpd` gains.
 * NAMES. GUESS: the unit name (SDK spelling); `wpadExtInitCallback`, `wpadDpdCalibrationReadDone`, `wpadDispatchReport`,
 *   `wpadReportStatus/ReadData/Ack/Ignored`, `wpadReportButtons`, `wpadReportButtonsAccel`, `wpadReportButtonsExt8`,
 *   `wpadReportButtonsAccelDpd12`, `wpadReportExt19`, `wpadReportButtonsAccelExt16`, `wpadReportButtonsDpd10Ext9`,
 *   `wpadReportButtonsAccelDpd10Ext6`, `wpadReportExt21`, `wpadReportInterleavedA/B` (the report ids are 0x20..0x3F; the names
 *   read the payload layout), `wpadDecodeDpd`, `wpadDecodeDpdInterleaved`, `wpadDecodeExtDualStick`, `wpadDecodeExtBoard` (the
 *   extension layouts are GUESSes from the decode: Nunchuk-style, Classic-style, a byte-stick type and a balance-board-like
 *   21-byte type); named but unwritten: `wpadAccCalibrationReadDone` (0x804F3150), `wpadExtCalibrationReadDone` (0x804F3C40),
 *   `wpadExtIdReadDone` (0x804F4180); GUESS data names `wpadExtType`, `wpadExtSubType`; GUESS field `WPADCB::extStatus`.
 *   Statics are named from use; the .bss rows at 0x8075EA00.. are not understood yet.
 * RESIDUALS. Written: 22 of 25 rows, 1 of them matches; 3 rows are not attempted (0x804F3150 (0xAE8), 0x804F3C40 (0x53C),
 *   0x804F4180 (0x438)).  The report decoders re-read the sample pointer per statement in the original (`wpadDecodeExtBoard`,
 *   `wpadDecodeDpdInterleaved`: the source's statement order differs and the original keeps no stack frame);
 *   `wpadReportReadData` and `wpadReportStatus` differ in register numbering and one reload; `wpadReportButtonsAccel` (the
 *   original reloads the control block pointer) and `wpadReportButtons` differ in scheduling.  Flip blockers: .text (3 unwritten
 *   rows), .data (the strings and jump tables of the unwritten bodies; the handler table is emitted), .rodata, .sbss
 *   (`wpadExtSubType`/`wpadExtType` are read only by the unwritten bodies) and .sdata2 (the pool the unwritten bodies and
 *   `wpadDecodeDpd` load).
 */

#include "types.h"
#include "WPAD/wpad.h"
#include "WPAD/WPADHIDParser.h"
#include "WPAD/WPADEncrypt.h"
#include "WPAD/debug_msg.h"
#include "OS/OSDisableInterrupts.h"
#include "OS/OSRestoreInterrupts.h"
#include "Runtime.PPCEABI.H/memcpy.h"
#include "Runtime.PPCEABI.H/memset.h"
#include "MSL_C/alloc.h"
#include "MSL/s_cos.h"
#include "MSL/s_sin.h"

/* .bss */
static f32 wpadDpdCenterX[4];
static f32 wpadDpdCenterY[4];
static f32 wpadDpdPivotX[4];
static f32 wpadDpdPivotY[4];
static f32 wpadDpdRoll[4];
static u8 wpadExtRaw[0x18];

/* .sbss */
static u8 wpadInterleavedA[4];
static u8 wpadInterleavedB[4];
static u8 wpadExtInitRetries[4];
static u8 wpadSticksZeroed[4];

/* .rodata */
static const u8 wpadExtZero[0x18] = { 0 };

/* The report handlers, indexed by report id - 0x20. */
typedef void (*WPADReportHandler)(u8 chan, u8* report, WPADSample* sample);
static WPADReportHandler wpadReportHandlers[32] = {
    wpadReportStatus, wpadReportReadData, wpadReportAck, wpadReportIgnored,
    wpadReportIgnored, wpadReportIgnored, wpadReportIgnored, wpadReportIgnored,
    wpadReportIgnored, wpadReportIgnored, wpadReportIgnored, wpadReportIgnored,
    wpadReportIgnored, wpadReportIgnored, wpadReportIgnored, wpadReportIgnored,
    wpadReportButtons, wpadReportButtonsAccel, wpadReportButtonsExt8, wpadReportButtonsAccelDpd12,
    wpadReportExt19, wpadReportButtonsAccelExt16, wpadReportButtonsDpd10Ext9, wpadReportButtonsAccelDpd10Ext6,
    wpadReportIgnored, wpadReportIgnored, wpadReportIgnored, wpadReportIgnored,
    wpadReportIgnored, wpadReportExt21, wpadReportInterleavedA, wpadReportInterleavedB,
};

/* Runs after an extension handshake step: retries the handshake or reports its failure. */
void wpadExtInitCallback(s32 chan, s32 result)
{
    WPADCB* cb = wpadCBTable[chan];
    s8 error;
    WPADCallback callback;

    if (result == 0) {
        return;
    }
    WPADiClearQueue(&cb->auxQueue);
    cb->extInitState = 0;
    if (result == -1) {
        error = -3;
    } else if (cb->flags.extensionAttached != 0) {
        u8 tries = wpadExtInitRetries[chan];
        wpadExtInitRetries[chan] = tries + 1;
        if (tries < 0x20) {
            WPADCB* again = wpadCBTable[chan];
            WPADiClearQueue(&again->auxQueue);
            WPADiSendSetReportType(&again->auxQueue, again->dataFormat, again->reportOnChange, wpadExtInitCallback);
            again->extInitState = 1;
            wpadSendWriteByte(&again->auxQueue, 0x55, 0x04A400F0, wpadExtInitCallback);
            wpadSendWriteByte(&again->auxQueue, 0, 0x04A400FB, wpadExtInitCallback);
            wpadSendReadMemory(&again->auxQueue, again->extId, 6, 0x04A400FA, wpadExtInitCallback);
            return;
        }
        error = -4;
    } else {
        WPADiDebugPrint("detaching extension during initialization.\n");
        WPADiSendSetReportType(&cb->auxQueue, cb->dataFormat, cb->reportOnChange, NULL);
        return;
    }
    cb->deviceType = error;
    cb->extensionResult = error;
    cb->extStatus = 0;
    callback = cb->extensionCallback;
    if (callback != NULL) {
        callback(chan, error);
    }
}

/* Checks the checksum of the pointer calibration block just read and stores the block when it is intact. */
void wpadDpdCalibrationReadDone(s32 chan, s32 error, s32 which)
{
    WPADCB* cb = wpadCBTable[chan];
    u8* data = (u8*)cb->replyA;

    if (error == 0) {
        s32 sum = 0;
        s32 i;
        for (i = 0; i < 0x2F; i++) {
            sum += data[i];
        }
        if (data[0x2F] == (u8)(sum + 0x55)) {
            memcpy(cb->calBytes, data, sizeof(cb->calBytes));
            (&cb->dpdStatusA)[(u8)which] = 0;
        } else {
            (&cb->dpdStatusA)[(u8)which] = -4;
        }
    } else {
        (&cb->dpdStatusA)[(u8)which] = -4;
    }
}

/* Handles the input report of the dispatch table that carries nothing the driver reads. */
void wpadReportIgnored(u8 chan, u8* report, WPADSample* sample)
{
}

/* Handles the status report: the battery, LED and extension flags, and starts the extension handshake. */
void wpadReportStatus(u8 chan, u8* report, WPADSample* sample)
{
    WPADCB* cb = wpadCBTable[chan];
    s32 wasAttached = cb->flags.extensionAttached;
    u32 level;
    WPADCallback callback;

    WPADiDebugPrint("Received report 20\n");
    level = OSDisableInterrupts();
    if (cb->ready == 0) {
        OSRestoreInterrupts(level);
        return;
    }
    cb->flags.extensionAttached = (report[3] & 2) >> 1;
    cb->flags.batteryLow = report[3] & 1;
    cb->flags.ledMask = (report[3] >> 4) & 0xF;
    cb->flags.protect = 0;
    cb->flags.statusFlags = report[5] & 0xF0;
    cb->flags.buttonByteTopBit = (report[1] >> 7) & 1;
    cb->flags.dpdEnabled = (report[3] >> 3) & 1;
    cb->flags.speakerEnabled = (report[3] >> 2) & 1;
    if (cb->deviceType == 3) {
        cb->flags.batteryLevel = cb->extBatteryLevel;
    } else {
        u8 level7 = report[6];
        if (level7 >= 0x55) {
            cb->flags.batteryLevel = 4;
        } else if (level7 >= 0x44) {
            cb->flags.batteryLevel = 3;
        } else if (level7 >= 0x33) {
            cb->flags.batteryLevel = 2;
        } else if (level7 >= 3) {
            cb->flags.batteryLevel = 1;
        } else {
            cb->flags.batteryLevel = 0;
        }
    }
    if (cb->flags.extensionAttached != 0) {
        if (wasAttached == 0) {
            WPADCB* again;
            WPADiDebugPrint("initialize attachment\n");
            wpadExtInitRetries[chan] = 0;
            again = wpadCBTable[chan];
            callback = cb->extensionCallback;
            WPADiClearQueue(&again->auxQueue);
            WPADiSendSetReportType(&again->auxQueue, again->dataFormat, again->reportOnChange, wpadExtInitCallback);
            again->extInitState = 1;
            wpadSendWriteByte(&again->auxQueue, 0x55, 0x04A400F0, wpadExtInitCallback);
            wpadSendWriteByte(&again->auxQueue, 0, 0x04A400FB, wpadExtInitCallback);
            wpadSendReadMemory(&again->auxQueue, again->extId, 6, 0x04A400FA, wpadExtInitCallback);
            cb->deviceType = 0xFF;
            cb->extensionSubType = 0;
            if (callback != NULL) {
                callback(chan, 0xFF);
            }
        }
    } else {
        cb->deviceType = 0;
        cb->extensionSubType = 0;
        WPADiClearQueue(&cb->auxQueue);
        WPADiSendSetReportType(&cb->auxQueue, cb->dataFormat, cb->reportOnChange, NULL);
        if (wasAttached != 0) {
            cb->extStatus = 1;
            cb->unused_0x992 = 0x12C;
            callback = cb->extensionCallback;
            if (callback != NULL) {
                callback(chan, 0);
            }
        }
    }
    if (cb->infoOut != NULL) {
        memcpy(cb->infoOut, &cb->flags, sizeof(WPADFlags));
        cb->infoOut = NULL;
    }
    memcpy(sample, wpadSpareSample(wpadCBTable[chan]), sizeof(WPADSample));
    sample->buttons = ((((report[1] & ~0xFF00) | ((report[2] << 8) & 0xFF00)) & 0x9F1F) & ~0x6000) | (sample->buttons & 0x6000);
    if (sample->deviceType != cb->deviceType) {
        sample->deviceType = cb->deviceType;
        sample->extensionError = -4;
    }
    callback = cb->cmdCallback;
    if (callback != NULL && cb->statusRequested != 0) {
        callback(chan, 0);
        cb->cmdCallback = NULL;
    }
    cb->statusRequested = 0;
    OSRestoreInterrupts(level);
}

/* Handles the memory-read reply: copies the payload and runs the read's follow-up when the last block arrives. */
void wpadReportReadData(u8 chan, u8* report, WPADSample* sample)
{
    WPADCB* cb = wpadCBTable[chan];
    u32 level = OSDisableInterrupts();
    u32 base;
    u16 offset;
    u32 high;
    u16 low;
    u8 size;
    s32 result;
    WPADCallback callback;

    if ((report[3] & 0xF) != 0) {
        WPADiDebugPrint("read error happens!\n");
        cb->replyC = -1;
        callback = cb->cmdCallback;
        if (callback != NULL) {
            if (cb->extensionCallback == NULL || cb->extensionCallback != callback) {
                callback(chan, -3);
            }
            cb->cmdCallback = NULL;
        }
        cb->status = 0;
    }
    base = cb->replyB;
    low = base;
    offset = (report[5] & ~0xFF00) | ((report[4] << 8) & 0xFF00);
    high = base >> 16;
    size = ((s32)report[3] >> 4) + 1;
    if (offset >= low && offset <= low + cb->replyD) {
        memcpy((u8*)cb->replyA + (s16)(offset - low), report + 6, size);
        if (low + cb->replyD == offset + size) {
            WPADiDebugPrint("base addr: %08x\n", cb->replyB);
            WPADiDebugPrint("length   : %d\n", (u32)cb->replyD);
            result = (-3) & ((s32)cb->replyC >> 31);
            WPADiDebugPrint("i2c = %04x\n", base >> 16);
            WPADiDebugPrint("enc = %d\n", (u32)cb->extInitState);
            if (high == 0x4A4) {
                WPADiDebugPrint("Access to extension register.\n");
                if ((u8)(cb->extInitState - 2) <= 1) {
                    WPADiDebugPrint("Decode!!!!\n");
                    WPADiDebugPrint("    len = %d, addr = %04x\n", (u32)size, (s32)(u16)base);
                    wpadDecryptExtension(chan, (u8*)cb->replyA, size, (u16)base);
                }
            }
            if ((cb->replyB == 0 && cb->calibrationPhase == 0) || (cb->replyB == 0x176C && cb->calibrationPhase == 1)) {
                wpadAccCalibrationReadDone(chan, result);
            }
            if (cb->replyB + 0xFB5C0000 == 0x20) {
                wpadExtCalibrationReadDone(chan, result);
            }
            if (cb->replyB + 0xFB5C0000 == 0xFA) {
                wpadExtIdReadDone(chan, result);
            }
            if (cb->replyB == 0x2A) {
                wpadDpdCalibrationReadDone(chan, result, 0);
            }
            if (cb->replyB == 0x62) {
                wpadDpdCalibrationReadDone(chan, result, 1);
            }
            callback = cb->cmdCallback;
            if (callback != NULL) {
                callback(chan, result);
                cb->cmdCallback = NULL;
            }
            cb->status = 0;
        }
    } else {
        WPADiDebugPrint("received data is out of range!\n");
    }
    memcpy(sample, wpadSpareSample(wpadCBTable[chan]), sizeof(WPADSample));
    sample->buttons = ((((report[1] & ~0xFF00) | ((report[2] << 8) & 0xFF00)) & 0x9F1F) & ~0x6000) | (sample->buttons & 0x6000);
    if (sample->deviceType != cb->deviceType) {
        sample->deviceType = cb->deviceType;
        sample->extensionError = -4;
    }
    OSRestoreInterrupts(level);
}

/* Handles the acknowledge report: completes the pending command when the acknowledged id is the one in flight. */
void wpadReportAck(u8 chan, u8* report, WPADSample* sample)
{
    WPADCB* cb = wpadCBTable[chan];
    u32 level = OSDisableInterrupts();
    u8 ackedId;
    u8 error;
    s32 result;
    WPADCallback callback;

    WPADiDebugPrint("Received ack!\n");
    memcpy(sample, wpadSpareSample(wpadCBTable[chan]), sizeof(WPADSample));
    sample->buttons = ((((report[1] & ~0xFF00) | ((report[2] << 8) & 0xFF00)) & 0x9F1F) & ~0x6000) | (sample->buttons & 0x6000);
    if (sample->deviceType != cb->deviceType) {
        sample->deviceType = cb->deviceType;
        sample->extensionError = -4;
    }
    ackedId = report[3];
    error = report[4];
    result = (error == 0) ? 0 : -3;
    WPADiDebugPrint("ack --> report ID = %02x, error code = %d\n", ackedId, error);
    if (cb->lastReportId == ackedId) {
        callback = cb->cmdCallback;
        if (callback != NULL) {
            callback(chan, result);
            cb->cmdCallback = NULL;
        }
        cb->status = 0;
    } else {
        WPADiDebugPrint("invalid ack!\n");
    }
    OSRestoreInterrupts(level);
}

/* Dispatches an input report to its handler and publishes the decoded sample; returns -1 for an unknown report id. */
s32 wpadDispatchReport(s32 chan, u8* report)
{
    WPADCB* cb = wpadCBTable[(u8)chan];
    s32 result = 0;

    if ((u8)(report[0] - 0x20) < 0x20) {
        WPADCB* current = wpadCBTable[(u8)chan];
        u32 level = OSDisableInterrupts();
        WPADSample* sample = &current->samples[current->sampleIndex];

        if (report[0] != 0x3E && report[0] != 0x3F) {
            memset(sample, 0, sizeof(WPADSample));
        }
        wpadReportHandlers[report[0] - 0x20]((u8)chan, report, sample);
        if (cb->ready == 0) {
            sample->extensionError = -4;
        }
        if (report[0] != 0x3E && report[0] != 0x3F) {
            WPADCB* next = wpadCBTable[(u8)chan];
            next->sampleIndex = (next->sampleIndex == 0);
        }
        OSRestoreInterrupts(level);
        wpadFilterButtons(chan);
        wpadStoreSample(chan);
    } else {
        result = -1;
    }
    return result;
}

/* Decodes the pointer objects of a basic (format 1) or extended (format 3) report and rotates them by the channel's roll. */
void wpadDecodeDpd(s32 chan, WPADSample** sample, u32 format, const u8* data, s32 length)
{
    u32 i;
    u8 slot;

    if (wpadCBTable[chan]->dpdBlocked != 0) {
        (*sample)->extensionError = -4;
        return;
    }
    if (format == 3) {
        for (slot = 0; slot < 4; slot++) {
            s32 offset = slot * 3;
            if (offset + 2 < length) {
                const u8* object = data + offset;
                u8 high = object[2];
                (*sample)->dpd[slot].x = object[0] | ((high << 4) & 0x300);
                (*sample)->dpd[slot].y = 0x2FF - (object[1] | ((high << 2) & 0x300));
                (*sample)->dpd[slot].size = high & 0xF;
                (*sample)->dpd[slot].size = (u8)(3.141592f * ((f32)(*sample)->dpd[slot].size * (f32)(*sample)->dpd[slot].size));
                if ((*sample)->dpd[slot].size == 0 || (*sample)->dpd[slot].x == 0x3FF || (*sample)->dpd[slot].y == 0x2FF) {
                    (*sample)->dpd[slot].x = 0;
                    (*sample)->dpd[slot].y = 0x2FF;
                    (*sample)->dpd[slot].size = 0;
                }
                (*sample)->dpd[slot].slot = slot;
            } else {
                (*sample)->dpd[slot].x = 0;
                (*sample)->dpd[slot].y = 0x2FF;
                (*sample)->dpd[slot].size = 0;
                (*sample)->dpd[slot].slot = slot;
            }
        }
    } else if (format == 1) {
        for (slot = 0; slot < 4; slot++) {
            s32 pair = slot >> 1;
            const u8* object = data + slot * 3 - pair;
            u8 x = object[0];
            u8 y = object[1];
            u8 high = data[pair * 5 + 2];
            if (slot % 2 == 0) {
                (*sample)->dpd[slot].x = x | ((high << 4) & 0x300);
                (*sample)->dpd[slot].y = 0x2FF - (y | ((high << 2) & 0x300));
            } else {
                (*sample)->dpd[slot].x = x | ((high << 8) & 0x300);
                (*sample)->dpd[slot].y = 0x2FF - (y | ((high << 6) & 0x300));
            }
            if ((*sample)->dpd[slot].x == 0x3FF || (*sample)->dpd[slot].y == 0x2FF) {
                (*sample)->dpd[slot].x = 0;
                (*sample)->dpd[slot].y = 0x2FF;
                (*sample)->dpd[slot].size = 0;
            } else {
                (*sample)->dpd[slot].size = 0xC;
            }
            (*sample)->dpd[slot].slot = slot;
        }
    }
    for (i = 0; i < 4; i++) {
        WPADDpdObject* object = &(*sample)->dpd[i];
        if (object->x != 0 || object->y != 0x2FF) {
            f32 px = (f32)object->x + wpadDpdCenterX[chan];
            f32 py = (f32)object->y + wpadDpdCenterY[chan];
            f32 dx = px - wpadDpdPivotX[chan];
            f32 dy = py - wpadDpdPivotY[chan];
            f32 sine = (f32)sin(-1.0f * wpadDpdRoll[chan]);
            object->x = wpadDpdPivotX[chan] + ((dx * (f32)cos(-1.0f * wpadDpdRoll[chan])) - (dy * sine));
            sine = (f32)cos(-1.0f * wpadDpdRoll[chan]);
            (*sample)->dpd[i].y = wpadDpdPivotY[chan] + ((dx * (f32)sin(-1.0f * wpadDpdRoll[chan])) + (dy * sine));
        }
    }
}

/* Decodes one 9-byte pointer record of an interleaved report into `slot` of the sample, plus its bounding box. */
void wpadDecodeDpdInterleaved(s32 chan, WPADSample** sample, u8 slot, const u8* data, s32 reserved)
{
    (*sample)->dpd[slot].x = data[0] | ((data[2] << 4) & 0x300);
    (*sample)->dpd[slot].y = 0x2FF - (data[1] | ((data[2] << 2) & 0x300));
    (*sample)->dpdExt[slot].intensity = (((s16)((data[7] << 8) & 0xFF00) | data[8]) << 6) & 0xFFC0;
    (*sample)->dpdExt[slot].sizeExtra = data[2] & 0xF;
    (*sample)->dpdExt[slot].left = data[3] != 0xFF ? data[3] : 0;
    (*sample)->dpdExt[slot].top = data[4] != 0xFF ? data[4] : 0;
    (*sample)->dpdExt[slot].right = data[5] != 0xFF ? data[5] : 0;
    (*sample)->dpdExt[slot].bottom = data[6] != 0xFF ? data[6] : 0;
    (*sample)->dpdExt[slot].left = (*sample)->dpdExt[slot].left * 8;
    (*sample)->dpdExt[slot].top = 0x2FF - (*sample)->dpdExt[slot].top * 8;
    (*sample)->dpdExt[slot].right = (*sample)->dpdExt[slot].right * 8;
    (*sample)->dpdExt[slot].bottom = 0x2FF - (*sample)->dpdExt[slot].bottom * 8;
    (*sample)->dpd[slot].size = 3.141592f * ((f32)(s8)(*sample)->dpdExt[slot].sizeExtra * (f32)(s8)(*sample)->dpdExt[slot].sizeExtra);
    if ((*sample)->dpd[slot].size == 0 || (*sample)->dpd[slot].x == 0x3FF || (*sample)->dpd[slot].y == 0x2FF || (*sample)->dpdExt[slot].sizeExtra == 0xF) {
        (*sample)->dpd[slot].x = 0;
        (*sample)->dpd[slot].y = 0x2FF;
        (*sample)->dpd[slot].size = 0;
        (*sample)->dpdExt[slot].intensity = 0;
        (*sample)->dpdExt[slot].sizeExtra = 0;
    }
    (*sample)->dpd[slot].slot = slot;
}

/* Decodes the sticks and triggers of a Classic-style extension payload and centres them on the first reading. */
void wpadDecodeExtDualStick(s32 chan, WPADSample** sample, u8 variant, const u8* data, u32 length)
{
    WPADCB* cb = wpadCBTable[chan];

    switch (variant) {
    case 2:
        (*sample)->classic.leftX = (s16)((s16)(data[0] * 4) & ~3) | (data[4] & 3);
        (*sample)->classic.rightX = (s16)((s16)(data[1] * 4) & ~3) | ((data[4] >> 2) & 3);
        (*sample)->classic.leftY = (s16)((s16)(data[2] * 4) & ~3) | ((data[4] >> 4) & 3);
        (*sample)->classic.rightY = (s16)((s16)(data[3] * 4) & ~3) | (s16)((s32)data[4] >> 6);
        (*sample)->classic.leftTrigger = data[5];
        (*sample)->classic.rightTrigger = (length < 9) ? 0 : data[6];
        (*sample)->classic.buttons = (length < 9) ? 0 : (((data[8] & ~0xFF00) | ((data[7] << 8) & 0xFF00)) ^ 0xFFFF);
        break;
    case 3:
        (*sample)->classic.leftX = (s16)data[0] * 4;
        (*sample)->classic.rightX = (s16)data[1] * 4;
        (*sample)->classic.leftY = (s16)data[2] * 4;
        (*sample)->classic.rightY = (s16)data[3] * 4;
        (*sample)->classic.leftTrigger = data[4];
        (*sample)->classic.rightTrigger = data[5];
        (*sample)->classic.buttons = (length < 8) ? 0 : (((data[7] & ~0xFF00) | ((data[6] << 8) & 0xFF00)) ^ 0xFFFF);
        break;
    default:
        (*sample)->classic.leftX = (data[0] * 0x10) & 0x3F0;
        (*sample)->classic.leftY = (data[1] * 0x10) & 0x3F0;
        (*sample)->classic.rightX = (s16)((s16)((s32)data[2] >> 7) | ((((data[1] >> 5) & 6) & ~0x18) | ((data[0] >> 3) & 0x18))) << 5;
        (*sample)->classic.rightY = (data[2] << 5) & 0x3E0;
        (*sample)->classic.leftTrigger = (u8)((((((s32)data[3] >> 5) & ~0x18) | (((s32)data[2] >> 2) & 0x18)) * 8) & 0xF8);
        (*sample)->classic.rightTrigger = (u8)((data[3] * 8) & 0xF8);
        (*sample)->classic.buttons = ((data[5] & ~0xFF00) | ((data[4] << 8) & 0xFF00)) ^ 0xFFFF;
        break;
    }
    if (cb->deviceType == 2) {
        (*sample)->classic.leftX = (*sample)->classic.leftX - 0x200;
        (*sample)->classic.leftY = (*sample)->classic.leftY - 0x200;
        (*sample)->classic.rightX = (*sample)->classic.rightX - 0x200;
        (*sample)->classic.rightY = (*sample)->classic.rightY - 0x200;
    } else {
        (*sample)->classic.leftX = (*sample)->classic.leftX - 0x200;
        (*sample)->classic.leftY = (*sample)->classic.leftY - 0x200;
    }
    if (cb->centerValid == 0) {
        cb->centerValid = 1;
        cb->calB.classic.leftX[0] = (*sample)->classic.leftX;
        cb->calB.classic.leftY[0] = (*sample)->classic.leftY;
        if (cb->deviceType == 2) {
            cb->calB.classic.rightX[0] = (*sample)->classic.rightX;
            cb->calB.classic.rightY[0] = (*sample)->classic.rightY;
            cb->calB.classic.leftTrigger = (*sample)->classic.leftTrigger;
            cb->calB.classic.rightTrigger = (*sample)->classic.rightTrigger;
        } else {
            cb->calB.classic.rightX[0] = 0;
            cb->calB.classic.rightY[0] = 0;
            cb->calB.classic.leftTrigger = 0;
            cb->calB.classic.rightTrigger = 0;
        }
    }
    {
        s16 value = (*sample)->classic.leftX - cb->calB.classic.leftX[0];
        (*sample)->classic.leftX = value < -0x200 ? -0x200 : (value > 0x1FF ? 0x1FF : value);
        value = (*sample)->classic.leftY - cb->calB.classic.leftY[0];
        (*sample)->classic.leftY = value < -0x200 ? -0x200 : (value > 0x1FF ? 0x1FF : value);
    }
    if (cb->deviceType == 2) {
        s16 value = (*sample)->classic.rightX - cb->calB.classic.rightX[0];
        (*sample)->classic.rightX = value < -0x200 ? -0x200 : (value > 0x1FF ? 0x1FF : value);
        value = (*sample)->classic.rightY - cb->calB.classic.rightY[0];
        (*sample)->classic.rightY = value < -0x200 ? -0x200 : (value > 0x1FF ? 0x1FF : value);
        value = (*sample)->classic.leftTrigger - cb->calB.classic.leftTrigger;
        (*sample)->classic.leftTrigger = value < 0 ? 0 : (value > 0xFF ? 0xFF : value);
        value = (*sample)->classic.rightTrigger - cb->calB.classic.rightTrigger;
        (*sample)->classic.rightTrigger = value < 0 ? 0 : (value > 0xFF ? 0xFF : value);
    }
    if (wpadSticksZeroed[chan] != 0) {
        (*sample)->classic.rightX = 0;
        (*sample)->classic.rightY = 0;
        (*sample)->classic.leftTrigger = 0;
        (*sample)->classic.rightTrigger = 0;
    }
}

/* Decodes the 21-byte payload of the board-style extension into the sample and its calibration. */
void wpadDecodeExtBoard(s32 chan, WPADSample** sample, u8 format, const u8* data, u32 length)
{
    (*sample)->board.batteryRef = data[0];
    (*sample)->board.weightRef[0] = (data[1] << 2) | ((s32)data[6] >> 6);
    (*sample)->board.weightRef[1] = (data[2] << 2) | ((s32)data[6] >> 4 & 3);
    (*sample)->board.weightRef[2] = (data[3] << 2) | ((s32)data[6] >> 2 & 3);
    (*sample)->board.weightRef[3] = (data[4] << 2) | (data[6] & 3);
    (*sample)->board.weightRef[4] = (data[5] << 2) | ((s32)data[7] >> 6);
    (*sample)->board.temperature = data[8];
    (*sample)->board.weight[0] = (data[9] << 2) | ((s32)data[14] >> 6);
    (*sample)->board.weight[1] = (data[10] << 2) | ((s32)data[14] >> 4 & 3);
    (*sample)->board.weight[2] = (data[11] << 2) | ((s32)data[14] >> 2 & 3);
    (*sample)->board.weight[3] = (data[12] << 2) | (data[14] & 3);
    (*sample)->board.weight[4] = (data[13] << 2) | ((s32)data[15] >> 6);
    {
        s32 reference = (data[7] << 4) & 0xF3F0;
        (*sample)->board.refA = (reference & ~0xF) | (((s32)data[15] >> 2) & 0xF);
        (*sample)->board.refC = data[15] & 3;
        if (reference < 0) {
            (*sample)->board.refB = 0;
            (*sample)->board.refD = 0;
            (*sample)->board.refE = 0;
            (*sample)->board.refF = 0;
            return;
        }
    }
    (*sample)->board.refB = (data[16] << 2) | ((s32)data[17] >> 6);
    (*sample)->board.refD = data[18] & 7;
    (*sample)->board.refE = data[19];
    (*sample)->board.refF = data[20];
}

/* Decodes the button word of the 0x32 report, 8 extension bytes follow. */
void wpadReportButtonsExt8(u8 chan, u8* report, WPADSample* sample)
{
    WPADCB* cb = wpadCBTable[chan];

    sample->buttons = ((report[1] & ~0xFF00) | ((report[2] << 8) & 0xFF00)) & 0x9F1F;
    if (cb->dataFormat == 0 || cb->dataFormat == 3 || cb->dataFormat == 6 || cb->dataFormat == 10) {
        sample->extensionError = 0;
    } else {
        sample->extensionError = -4;
    }
    sample->deviceType = cb->deviceType;
    cb->flags.buttonByteTopBit = (report[1] >> 7) & 1;
    memcpy(wpadExtRaw, report + 3, 8);
    wpadDecryptExtension(chan, report + 3, 8, 0);
    if (cb->flags.extensionAttached != 0) {
        switch (cb->deviceType) {
        case 1: {
            WPADCB* again = wpadCBTable[chan];
            s16 value;
            sample->nunchuk.stickX = report[3];
            sample->nunchuk.stickY = report[4];
            sample->nunchuk.accX = (s16)(((s16)(report[5] * 4) & ~3) | ((report[8] >> 2) & 3)) - again->calB.nunchuk.accZero[0];
            sample->nunchuk.accY = (s16)(((s16)(report[6] * 4) & ~3) | ((report[8] >> 4) & 3)) - again->calB.nunchuk.accZero[1];
            sample->nunchuk.accZ = (s16)(((s16)(report[7] * 4) & ~3) | (s16)((s32)report[8] >> 6)) - again->calB.nunchuk.accZero[2];
            sample->buttons = sample->buttons | ((~report[8] << 13) & 0x6000);
            if (again->centerValid == 0) {
                again->centerValid = 1;
                again->calB.nunchuk.stickX[0] = sample->nunchuk.stickX;
                again->calB.nunchuk.stickY[0] = sample->nunchuk.stickY;
            }
            value = sample->nunchuk.stickX - (u8)again->calB.nunchuk.stickX[0];
            sample->nunchuk.stickX = value < -0x80 ? -0x80 : (value > 0x7F ? 0x7F : value);
            value = sample->nunchuk.stickY - (u8)again->calB.nunchuk.stickY[0];
            sample->nunchuk.stickY = value < -0x80 ? -0x80 : (value > 0x7F ? 0x7F : value);
            break;
        }
        case 2:
            wpadDecodeExtDualStick(chan, &sample, cb->extensionSubType, report + 3, 8);
            break;
        case 16:
            sample->simple.stickX = report[5];
            sample->simple.stickY = report[6];
            sample->simple.buttons = ((report[10] & ~0xFF00) | ((report[9] << 8) & 0xFF00)) ^ 0xFFFF;
            break;
        }
        if (memcmp(wpadExtRaw, wpadExtZero, 8) == 0 && sample->extensionError == 0) {
            sample->extensionError = -7;
        }
    }
}

/* Decodes the 0x33 report: buttons, accelerometer and 12 bytes of pointer data. */
void wpadReportButtonsAccelDpd12(u8 chan, u8* report, WPADSample* sample)
{
    WPADCB* cb = wpadCBTable[chan];
    WPADCB* again;

    sample->buttons = ((report[1] & ~0xFF00) | ((report[2] << 8) & 0xFF00)) & 0x9F1F;
    sample->deviceType = cb->deviceType;
    if (cb->dataFormat <= 2) {
        sample->extensionError = 0;
    } else {
        sample->extensionError = -4;
    }
    cb->flags.buttonByteTopBit = (report[1] >> 7) & 1;
    again = wpadCBTable[chan];
    sample->accX = (s16)(((s16)(report[3] * 4) & ~3) | ((report[1] >> 5) & 3)) - again->calA.accZero[0];
    sample->accY = (s16)(((s16)(report[4] * 4) & ~3) | (s16)((report[2] >> 4) & 2)) - again->calA.accZero[1];
    sample->accZ = (s16)(((s16)(report[5] * 4) & ~3) | (s16)((report[2] >> 5) & 2)) - again->calA.accZero[2];
    wpadDecodeDpd(chan, &sample, cb->dpdFormat, report + 6, 12);
}

/* Decodes the 0x34 report: buttons and 19 extension bytes. */
void wpadReportExt19(u8 chan, u8* report, WPADSample* sample)
{
    WPADCB* cb = wpadCBTable[chan];

    sample->buttons = ((report[1] & ~0xFF00) | ((report[2] << 8) & 0xFF00)) & 0x9F1F;
    if (cb->dataFormat == 12) {
        sample->extensionError = 0;
    } else {
        sample->extensionError = -4;
    }
    sample->deviceType = cb->deviceType;
    cb->flags.buttonByteTopBit = (report[1] >> 7) & 1;
    memcpy(wpadExtRaw, report + 3, 0x13);
    wpadDecryptExtension(chan, report + 3, 0x13, 0);
    if (cb->flags.extensionAttached != 0) {
        if (cb->deviceType == 3) {
            s32 doubled;
            sample->board.weight[0] = (report[4] & ~0xFF00) | ((report[3] << 8) & 0xFF00);
            sample->board.weight[1] = (report[6] & ~0xFF00) | ((report[5] << 8) & 0xFF00);
            sample->board.weight[2] = (report[8] & ~0xFF00) | ((report[7] << 8) & 0xFF00);
            sample->board.weight[3] = (report[10] & ~0xFF00) | ((report[9] << 8) & 0xFF00);
            sample->board.weight[4] = report[11];
            sample->board.temperature = report[13];
            doubled = sample->board.temperature * 2;
            if (doubled >= 0x104) {
                cb->extBatteryLevel = 4;
            } else if ((u32)(doubled - 0xFA) <= 9) {
                cb->extBatteryLevel = 3;
            } else if ((u32)(doubled - 0xF0) <= 9) {
                cb->extBatteryLevel = 2;
            } else if ((u32)(doubled - 0xD4) <= 0x1B) {
                cb->extBatteryLevel = 1;
            } else {
                cb->extBatteryLevel = 0;
            }
            if (cb->centerValid == 0) {
                cb->centerValid = 1;
            }
        }
        if (memcmp(wpadExtRaw, wpadExtZero, 0x13) == 0 && sample->extensionError == 0) {
            sample->extensionError = -7;
        }
    }
}

/* Decodes the 0x35 report: buttons, accelerometer and 16 extension bytes. */
void wpadReportButtonsAccelExt16(u8 chan, u8* report, WPADSample* sample)
{
    WPADCB* cb = wpadCBTable[chan];
    WPADCB* again;

    sample->buttons = ((report[1] & ~0xFF00) | ((report[2] << 8) & 0xFF00)) & 0x9F1F;
    if (cb->dataFormat <= 1 || (u32)(cb->dataFormat - 3) <= 1 || (u32)(cb->dataFormat - 6) <= 1 || cb->dataFormat == 13) {
        sample->extensionError = 0;
    } else {
        sample->extensionError = -4;
    }
    sample->deviceType = cb->deviceType;
    cb->flags.buttonByteTopBit = (report[1] >> 7) & 1;
    again = wpadCBTable[chan];
    sample->accX = (s16)(((s16)(report[3] * 4) & ~3) | ((report[1] >> 5) & 3)) - again->calA.accZero[0];
    sample->accY = (s16)(((s16)(report[4] * 4) & ~3) | (s16)((report[2] >> 4) & 2)) - again->calA.accZero[1];
    sample->accZ = (s16)(((s16)(report[5] * 4) & ~3) | (s16)((report[2] >> 5) & 2)) - again->calA.accZero[2];
    memcpy(wpadExtRaw, report + 6, 0x10);
    wpadDecryptExtension(chan, report + 6, 0x10, 0);
    if (cb->flags.extensionAttached != 0) {
        switch (cb->deviceType) {
        case 1: {
            WPADCB* current = wpadCBTable[chan];
            s16 value;
            sample->nunchuk.stickX = report[6];
            sample->nunchuk.stickY = report[7];
            sample->nunchuk.accX = (s16)(((s16)(report[8] * 4) & ~3) | ((report[11] >> 2) & 3)) - current->calB.nunchuk.accZero[0];
            sample->nunchuk.accY = (s16)(((s16)(report[9] * 4) & ~3) | ((report[11] >> 4) & 3)) - current->calB.nunchuk.accZero[1];
            sample->nunchuk.accZ = (s16)(((s16)(report[10] * 4) & ~3) | (s16)((s32)report[11] >> 6)) - current->calB.nunchuk.accZero[2];
            sample->buttons = sample->buttons | ((~report[11] << 13) & 0x6000);
            if (current->centerValid == 0) {
                current->centerValid = 1;
                current->calB.nunchuk.stickX[0] = sample->nunchuk.stickX;
                current->calB.nunchuk.stickY[0] = sample->nunchuk.stickY;
            }
            value = sample->nunchuk.stickX - (u8)current->calB.nunchuk.stickX[0];
            sample->nunchuk.stickX = value < -0x80 ? -0x80 : (value > 0x7F ? 0x7F : value);
            value = sample->nunchuk.stickY - (u8)current->calB.nunchuk.stickY[0];
            sample->nunchuk.stickY = value < -0x80 ? -0x80 : (value > 0x7F ? 0x7F : value);
            break;
        }
        case 2:
            wpadDecodeExtDualStick(chan, &sample, cb->extensionSubType, report + 6, 0x10);
            break;
        case 4:
            wpadDecodeExtBoard(chan, &sample, cb->dataFormat, report + 6, 0x10);
            break;
        }
        if (memcmp(wpadExtRaw, wpadExtZero, 0x10) == 0 && sample->extensionError == 0) {
            sample->extensionError = -7;
        }
    }
}

/* Decodes the 0x36 report: buttons, 10 bytes of pointer data and 9 extension bytes. */
void wpadReportButtonsDpd10Ext9(u8 chan, u8* report, WPADSample* sample)
{
    WPADCB* cb = wpadCBTable[chan];

    sample->buttons = ((report[1] & ~0xFF00) | ((report[2] << 8) & 0xFF00)) & 0x9F1F;
    sample->extensionError = -4;
    sample->deviceType = cb->deviceType;
    cb->flags.buttonByteTopBit = (report[1] >> 7) & 1;
    wpadDecodeDpd(chan, &sample, cb->dpdFormat, report + 3, 10);
    memcpy(wpadExtRaw, report + 13, 9);
    wpadDecryptExtension(chan, report + 13, 9, 0);
    if (cb->flags.extensionAttached != 0 && memcmp(wpadExtRaw, wpadExtZero, 9) == 0 && sample->extensionError == 0) {
        sample->extensionError = -7;
    }
}

/* Decodes the 0x37 report: buttons, accelerometer, 10 bytes of pointer data and 6 extension bytes. */
void wpadReportButtonsAccelDpd10Ext6(u8 chan, u8* report, WPADSample* sample)
{
    WPADCB* cb = wpadCBTable[chan];
    WPADCB* again;

    sample->buttons = ((report[1] & ~0xFF00) | ((report[2] << 8) & 0xFF00)) & 0x9F1F;
    if (cb->dataFormat <= 8 || cb->dataFormat == 11 || cb->dataFormat == 15) {
        sample->extensionError = 0;
    } else {
        sample->extensionError = -4;
    }
    sample->deviceType = cb->deviceType;
    cb->flags.buttonByteTopBit = (report[1] >> 7) & 1;
    again = wpadCBTable[chan];
    sample->accX = (s16)(((s16)(report[3] * 4) & ~3) | ((report[1] >> 5) & 3)) - again->calA.accZero[0];
    sample->accY = (s16)(((s16)(report[4] * 4) & ~3) | (s16)((report[2] >> 4) & 2)) - again->calA.accZero[1];
    sample->accZ = (s16)(((s16)(report[5] * 4) & ~3) | (s16)((report[2] >> 5) & 2)) - again->calA.accZero[2];
    wpadDecodeDpd(chan, &sample, cb->dpdFormat, report + 6, 10);
    memcpy(wpadExtRaw, report + 16, 6);
    wpadDecryptExtension(chan, report + 16, 6, 0);
    if (cb->flags.extensionAttached != 0) {
        if (cb->deviceType == 1) {
            WPADCB* current = wpadCBTable[chan];
            s16 value;
            sample->nunchuk.stickX = report[16];
            sample->nunchuk.stickY = report[17];
            sample->nunchuk.accX = (s16)(((s16)(report[18] * 4) & ~3) | ((report[21] >> 2) & 3)) - current->calB.nunchuk.accZero[0];
            sample->nunchuk.accY = (s16)(((s16)(report[19] * 4) & ~3) | ((report[21] >> 4) & 3)) - current->calB.nunchuk.accZero[1];
            sample->nunchuk.accZ = (s16)(((s16)(report[20] * 4) & ~3) | (s16)((s32)report[21] >> 6)) - current->calB.nunchuk.accZero[2];
            sample->buttons = sample->buttons | ((~report[21] << 13) & 0x6000);
            if (current->centerValid == 0) {
                current->centerValid = 1;
                current->calB.nunchuk.stickX[0] = sample->nunchuk.stickX;
                current->calB.nunchuk.stickY[0] = sample->nunchuk.stickY;
            }
            value = sample->nunchuk.stickX - (u8)current->calB.nunchuk.stickX[0];
            sample->nunchuk.stickX = value < -0x80 ? -0x80 : (value > 0x7F ? 0x7F : value);
            value = sample->nunchuk.stickY - (u8)current->calB.nunchuk.stickY[0];
            sample->nunchuk.stickY = value < -0x80 ? -0x80 : (value > 0x7F ? 0x7F : value);
        } else if (cb->deviceType == 2 || (u8)(cb->deviceType - 0x11) <= 1) {
            wpadDecodeExtDualStick(chan, &sample, cb->extensionSubType, report + 16, 6);
        }
        if (memcmp(wpadExtRaw, wpadExtZero, 6) == 0 && sample->extensionError == 0) {
            sample->extensionError = -7;
        }
    }
}

/* Decodes the 0x3D report: 21 extension bytes and no buttons. */
void wpadReportExt21(u8 chan, u8* report, WPADSample* sample)
{
    WPADCB* cb = wpadCBTable[chan];

    if (cb->dataFormat == 14) {
        sample->extensionError = 0;
    } else {
        sample->extensionError = -4;
    }
    sample->deviceType = cb->deviceType;
    memcpy(wpadExtRaw, report + 1, 0x15);
    wpadDecryptExtension(chan, report + 1, 0x15, 0);
    if (cb->flags.extensionAttached != 0) {
        if (cb->deviceType == 4) {
            wpadDecodeExtBoard(chan, &sample, cb->dataFormat, report + 1, 0x15);
        }
        if (memcmp(wpadExtRaw, wpadExtZero, 0x15) == 0 && sample->extensionError == 0) {
            sample->extensionError = -7;
        }
    }
}

/* Decodes the first half (slots 0 and 1) of an interleaved pointer report. */
void wpadReportInterleavedA(u8 chan, u8* report, WPADSample* sample)
{
    WPADCB* cb = wpadCBTable[chan];
    u32 level;

    if (wpadInterleavedA[chan] == 0 && wpadInterleavedB[chan] == 0) {
        memset(sample, 0, sizeof(WPADSample));
    }
    sample->buttons = ((report[1] & ~0xFF00) | ((report[2] << 8) & 0xFF00)) & 0x9F1F;
    if (cb->dataFormat <= 1 || cb->dataFormat == 9) {
        sample->extensionError = 0;
    } else {
        sample->extensionError = -4;
    }
    sample->deviceType = cb->deviceType;
    cb->flags.buttonByteTopBit = 0;
    sample->accX = (s16)(((s16)(report[3] * 4) & ~3) | (s16)((report[1] >> 6) & 2)) - wpadCBTable[chan]->calA.accZero[0];
    sample->accZ = sample->accZ | (s16)(((s16)(report[2] * 8) & ~0xFF) | ((report[1] * 2) & 0xC0));
    wpadDecodeDpdInterleaved(chan, &sample, 0, report + 4, 0);
    wpadDecodeDpdInterleaved(chan, &sample, 1, report + 13, 0);
    level = OSDisableInterrupts();
    wpadInterleavedA[chan] = 1;
    if (wpadInterleavedB[chan] != 0) {
        sample->accZ = sample->accZ - cb->calA.accZero[2];
        cb->sampleIndex = (cb->sampleIndex == 0);
        wpadInterleavedB[chan] = 0;
        wpadInterleavedA[chan] = 0;
    }
    OSRestoreInterrupts(level);
}

/* Decodes the second half (slots 2 and 3) of an interleaved pointer report. */
void wpadReportInterleavedB(u8 chan, u8* report, WPADSample* sample)
{
    WPADCB* cb = wpadCBTable[chan];
    u32 level;

    if (wpadInterleavedA[chan] == 0 && wpadInterleavedB[chan] == 0) {
        memset(sample, 0, sizeof(WPADSample));
    }
    sample->buttons = ((report[1] & ~0xFF00) | ((report[2] << 8) & 0xFF00)) & 0x9F1F;
    if (cb->dataFormat <= 1 || cb->dataFormat == 9) {
        sample->extensionError = 0;
    } else {
        sample->extensionError = -4;
    }
    sample->deviceType = cb->deviceType;
    cb->flags.buttonByteTopBit = 0;
    sample->accY = (s16)(((s16)(report[3] * 4) & ~3) | (s16)((report[1] >> 6) & 2)) - wpadCBTable[chan]->calA.accZero[1];
    sample->accZ = sample->accZ | (s16)(((report[1] >> 3) & 0xC) | ((report[2] >> 1) & 0x30));
    wpadDecodeDpdInterleaved(chan, &sample, 2, report + 4, 0);
    wpadDecodeDpdInterleaved(chan, &sample, 3, report + 13, 0);
    level = OSDisableInterrupts();
    wpadInterleavedB[chan] = 1;
    if (wpadInterleavedA[chan] != 0 && wpadInterleavedB[chan] != 0) {
        sample->accZ = sample->accZ - cb->calA.accZero[2];
        cb->sampleIndex = (cb->sampleIndex == 0);
        wpadInterleavedB[chan] = 0;
        wpadInterleavedA[chan] = 0;
    }
    OSRestoreInterrupts(level);
}

/* Decodes the report-0x30 payload: the button word only. */
void wpadReportButtons(u8 chan, u8* report, WPADSample* sample)
{
    WPADCB* cb = wpadCBTable[chan];

    sample->buttons = ((report[1] & ~0xFF00) | ((report[2] << 8) & 0xFF00)) & 0x9F1F;
    sample->deviceType = cb->deviceType;
    if (cb->dataFormat == 0) {
        sample->extensionError = 0;
    } else {
        sample->extensionError = -4;
    }
    cb->flags.buttonByteTopBit = (report[1] >> 7) & 1;
}

/* Decodes the report-0x31 payload: the button word and the remote's accelerometer, relative to its zero point. */
void wpadReportButtonsAccel(u8 chan, u8* report, WPADSample* sample)
{
    WPADCB* cb = wpadCBTable[chan];
    WPADCB* again;

    sample->buttons = ((report[1] & ~0xFF00) | ((report[2] << 8) & 0xFF00)) & 0x9F1F;
    sample->deviceType = cb->deviceType;
    if (cb->dataFormat <= 1) {
        sample->extensionError = 0;
    } else {
        sample->extensionError = -4;
    }
    cb->flags.buttonByteTopBit = (report[1] >> 7) & 1;
    again = wpadCBTable[chan];
    sample->accX = (s16)(((s16)(report[3] * 4) & ~3) | ((report[1] >> 5) & 3)) - again->calA.accZero[0];
    sample->accY = (s16)(((s16)(report[4] * 4) & ~3) | (s16)((report[2] >> 4) & 2)) - again->calA.accZero[1];
    sample->accZ = (s16)(((s16)(report[5] * 4) & ~3) | (s16)((report[2] >> 5) & 2)) - again->calA.accZero[2];
}
