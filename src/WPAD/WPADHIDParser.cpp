/* WPAD/WPADHIDParser.cpp - the Wii remote report parser (the SDK's WPADHIDParser.c): the extension and calibration
 *   handshakes, the input-report dispatch table and the report decoders.
 * RANGE. .text 0x804F2FF0-0x804F72C0 (25 functions); .rodata 0x80573CC0-0x80573CD8 (`wpadExtNone`); .data 0x8062C358-0x8062C8C8
 *   (the report handler table, then the parser's strings and jump tables); .bss 0x8075EA00-0x8075EA68; .sbss
 *   0x80795700-0x80795718; .sdata2 0x8079D3D0-0x8079D430.  Seam with WPAD/wpad.cpp at 0x804F2FF0 / 0x8062C358: .data has a
 *   4-byte hole 0x8062C354-0x8062C358 after `WPADiSendSetReportType`'s jump table, the handler table starts 8-aligned at
 *   0x8062C358 and the first string after it ("detaching extension during initialization.") is read by `wpadExtInitCallback`
 *   0x804F2FF0; `wpadExtNone` follows `WPADControlDpd`'s .rodata; and the parser calls the 8-byte WPAD.c functions
 *   `WPADiGetReserved0..3` out of line.  Seam with WPAD/WPADEncrypt.cpp at 0x804F72C0 / 0x8062C8C8: .data has a 4-byte hole
 *   0x8062C8C4-0x8062C8C8 after "invalid ack!", and the key tables and debug strings from 0x8062C8C8 are read only by the
 *   key generators.  The .sdata2 pool repeats constants WPAD/WPADClamp.cpp's pool holds again (the double 4330000080000000 at
 *   0x8079D418 and 0x8079D460, 0.0f at 0x8079D3E0 and 0x8079D46C, -1.0f at 0x8079D408 and 0x8079D470), so the two are
 *   separate TUs.  The .bss arrays at 0x8075EA00, `wpadExtRaw`, the .sbss rows and `wpadExtNone` are read only by this unit.
 * FLAGS. `cflags_base` plus `-fp_contract off` (configure.py): the target has no fused multiply-add and keeps unfused
 *   fmuls->fadds pairs in `wpadAccCalibrationReadDone` (2) and `wpadDecodeDpd` (2); `-fp_contract on` fuses two in
 *   `wpadDecodeDpd` (an fmsubs and an fmadds), the flag leaves none and `wpadDecodeDpd` gains.
 * NAMES. GUESS: the unit name (SDK spelling); `wpadExtInitCallback`, `wpadDpdCalibrationReadDone`, `wpadDispatchReport`,
 *   `wpadReportStatus/ReadData/Ack/Ignored`, `wpadReportButtons`, `wpadReportButtonsAccel`, `wpadReportButtonsExt8`,
 *   `wpadReportButtonsAccelDpd12`, `wpadReportExt19`, `wpadReportButtonsAccelExt16`, `wpadReportButtonsDpd10Ext9`,
 *   `wpadReportButtonsAccelDpd10Ext6`, `wpadReportExt21`, `wpadReportInterleavedA/B` (the report ids are 0x20..0x3F; the names
 *   read the payload layout), `wpadDecodeDpd`, `wpadDecodeDpdInterleaved`, `wpadDecodeExtDualStick`, `wpadDecodeExtBoard` (the
 *   extension layouts are GUESSes from the decode: Nunchuk-style, Classic-style, a byte-stick type and a balance-board-like
 *   21-byte type), `wpadAccCalibrationReadDone`, `wpadExtCalibrationReadDone`, `wpadExtIdReadDone`,
 *   `wpadIsFirstPartyExtension`; GUESS data names `wpadExtType`, `wpadExtSubType`, `wpadExtNone` (the 21 0xFF bytes an
 *   absent extension reads as); GUESS field `WPADCB::extStatus`.  Statics are named from use; the .bss rows at
 *   0x8075EA00.. are not understood yet.
 * RESIDUALS. `wpadAccCalibrationReadDone`: the ten-bit pointer coordinates of the four calibration points are loaded and
 *   combined in a different order (register numbering follows).  `wpadExtCalibrationReadDone` adds the block offset to the
 *   buffer in the other operand order.  `wpadDecodeDpd` numbers the rotation loop's array bases and two float temporaries
 *   differently.  `wpadDecodeExtDualStick` and `wpadDecodeExtBoard` differ in register numbering; `wpadDecodeExtBoard`
 *   also builds the middle weight words with rlwimi from the shifted low byte where the original shifts it with srawi
 *   first (no cast spelling found).  `wpadDispatchReport`, `wpadReportReadData`, `wpadReportStatus` and
 *   `wpadReportButtonsAccelExt16` differ in register numbering (no declaration order moves it); `wpadReportStatus` also
 *   keeps an extsh on the extension bit.  `wpadReportButtonsAccel` and `wpadReportButtons` load the control block one slot
 *   later.  `wpadExtNone` is the map's `wpadExtZero` until request wpad-slice1#23's map half is applied.
 *   Flip blockers: .text (the partial rows) and .sdata2 (`wpadDecodeDpd` converts the unsigned pointer size, which needs
 *   the 4330000000000000 constant).
 * SHAPES. A pointer coordinate's low byte is `(s16)(b & 0xFF)` (the mask keeps the extsh a bare cast drops); an extension
 *   axis's low bits are `(s16)((s16)((s32)b >> n) & m)` (retail keeps the dead srawi); stick clamps are two independent
 *   `if`s over a copy.  The accelerometer decode is an inline (`wpadDecodeAccel`, or `wpadDecodeAccelAt` through the
 *   sample pointer; `wpadInterleavedAxis` for the interleaved reports; `wpadDecodeNunchukAt` for the Nunchuk payload): the
 *   original looks the control block up again inside it, which a plain statement sequence CSEs away.  The extension
 *   handlers hand the decoders a copy of the sample pointer (`decoded`); `wpadReportButtonsAccelDpd10Ext6` passes the
 *   copy to its own stores and `&sample` to the Nunchuk and Classic decoders.
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
#include "MSL/s_atan.h"
#include "MSL/s_tan.h"
#include "OS/OS.h"
#include "WUD/wud.h"

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
static u8 wpadExtSubType[4];
static u8 wpadExtType[4];

/* .rodata */
static const u8 wpadExtNone[0x18] = {
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
};

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

/* Decodes the ten-bit accelerometer axes of a core report and subtracts the remote's zero-G calibration. */
static inline void wpadDecodeAccel(s32 chan, u8* report, WPADSample* sample)
{
    WPADCB* cb = wpadCBTable[chan];

    sample->accX = (s16)((s16)((s16)(report[3] << 2) & ~3) | ((report[1] >> 5) & 3)) - cb->calA.accZero[0];
    sample->accY = (s16)((s16)((s16)(report[4] << 2) & ~3) | (s16)((report[2] >> 4) & 2)) - cb->calA.accZero[1];
    sample->accZ = (s16)((s16)((s16)(report[5] << 2) & ~3) | (s16)((report[2] >> 5) & 2)) - cb->calA.accZero[2];
}

/* Decodes the accelerometer axes into the sample `*sample` points at (the report handlers that hand the sample to a
 * decoder keep it in memory). */
static inline void wpadDecodeAccelAt(s32 chan, u8* report, WPADSample** sample)
{
    WPADCB* cb = wpadCBTable[chan];

    (*sample)->accX = (s16)((s16)((s16)(report[3] << 2) & ~3) | ((report[1] >> 5) & 3)) - cb->calA.accZero[0];
    (*sample)->accY = (s16)((s16)((s16)(report[4] << 2) & ~3) | (s16)((report[2] >> 4) & 2)) - cb->calA.accZero[1];
    (*sample)->accZ = (s16)((s16)((s16)(report[5] << 2) & ~3) | (s16)((report[2] >> 5) & 2)) - cb->calA.accZero[2];
}

/* Decodes one accelerometer axis of an interleaved report (ten bits from the low payload byte and the button bits)
 * relative to the remote's zero-G calibration of `axis`. */
static inline s16 wpadInterleavedAxis(s32 chan, u8 high, u8 low, s32 axis)
{
    return (s16)((s16)((s16)(high << 2) & ~3) | (s16)((low >> 6) & 2)) - wpadCBTable[chan]->calA.accZero[axis];
}

/* Decodes a Nunchuk-style extension payload into the sample `*sample` points at. */
static inline void wpadDecodeNunchukAt(s32 chan, u8* ext, WPADSample** sample)
{
    WPADCB* cb = wpadCBTable[chan];
    s16 value;
    s16 clamped;
    s16 valueY;
    s16 clampedY;

    (*sample)->nunchuk.stickX = ext[0];
    (*sample)->nunchuk.stickY = ext[1];
    (*sample)->nunchuk.accX = (s16)((s16)((s16)(ext[2] << 2) & ~3) | (s16)((s16)((s32)ext[5] >> 2) & 3)) - cb->calB.nunchuk.accZero[0];
    (*sample)->nunchuk.accY = (s16)((s16)((s16)(ext[3] << 2) & ~3) | (s16)((s16)((s32)ext[5] >> 4) & 3)) - cb->calB.nunchuk.accZero[1];
    (*sample)->nunchuk.accZ = (s16)((s16)((s16)(ext[4] << 2) & ~3) | (s16)((s32)ext[5] >> 6)) - cb->calB.nunchuk.accZero[2];
    (*sample)->buttons = (*sample)->buttons | ((~ext[5] << 13) & 0x6000);
    if (cb->centerValid == 0) {
        cb->centerValid = 1;
        cb->calB.nunchuk.stickX[0] = (*sample)->nunchuk.stickX;
        cb->calB.nunchuk.stickY[0] = (*sample)->nunchuk.stickY;
    }
    value = (u8)(*sample)->nunchuk.stickX - (u8)cb->calB.nunchuk.stickX[0];
    clamped = value;
    if (value < -0x80) {
        clamped = -0x80;
    }
    if (value > 0x7F) {
        clamped = 0x7F;
    }
    (*sample)->nunchuk.stickX = clamped;
    valueY = (u8)(*sample)->nunchuk.stickY - (u8)cb->calB.nunchuk.stickY[0];
    clampedY = valueY;
    if (valueY < -0x80) {
        clampedY = -0x80;
    }
    if (valueY > 0x7F) {
        clampedY = 0x7F;
    }
    (*sample)->nunchuk.stickY = clampedY;
}

/* Starts the extension handshake on the auxiliary queue: the report type, the two initialisation writes and the
 * identification read. */
static inline void wpadStartExtInit(s32 chan)
{
    WPADCB* cb = wpadCBTable[chan];

    WPADiClearQueue(&cb->auxQueue);
    WPADiSendSetReportType(&cb->auxQueue, cb->dataFormat, cb->reportOnChange, wpadExtInitCallback);
    cb->extInitState = 1;
    wpadSendWriteByte(&cb->auxQueue, 0x55, 0x04A400F0, wpadExtInitCallback);
    wpadSendWriteByte(&cb->auxQueue, 0, 0x04A400FB, wpadExtInitCallback);
    wpadSendReadMemory(&cb->auxQueue, cb->extId, 6, 0x04A400FA, wpadExtInitCallback);
}

/* Runs after an extension handshake step: retries the handshake or reports its failure. */
void wpadExtInitCallback(s32 chan, s32 result)
{
    WPADCB* cb = wpadCBTable[chan];
    u8 error;
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
            wpadStartExtInit(chan);
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

/* Checks the remote's pointer and accelerometer calibration blocks, falls back to the defaults for a broken or
 * missing block, and derives the pointer centre, pivot and roll of the channel. */
void wpadAccCalibrationReadDone(s32 chan, s32 result)
{
    u8 block;
    f32 sumY;
    WPADCB* cb = wpadCBTable[chan];
    f32 angleX;
    f32 sumX;
    f32 px[4];
    s16 defaultX[4] = { 127, 896, 896, 127 };
    u8 i;
    f32 py[4];
    WPADDpdCalPoint read[4];
    s16 defaultY[4] = { 93, 93, 674, 674 };
    f32 measured[4];
    f32 angleY;
    u8* data;
    f32 expected[4];

    data = (u8*)cb->replyA;
    cb->calA.dpdPoint[0].x = defaultX[0];
    cb->calA.dpdPoint[0].y = defaultY[0];
    cb->calA.dpdPoint[0].size = cb->reportRate;
    cb->calA.dpdPoint[0].index = 0;
    cb->calA.dpdPoint[1].x = defaultX[1];
    cb->calA.dpdPoint[1].y = defaultY[1];
    cb->calA.dpdPoint[1].size = cb->reportRate;
    cb->calA.dpdPoint[1].index = 1;
    cb->calA.dpdPoint[2].x = defaultX[2];
    cb->calA.dpdPoint[2].y = defaultY[2];
    cb->calA.dpdPoint[2].size = cb->reportRate;
    cb->calA.dpdPoint[2].index = 2;
    cb->calA.dpdPoint[3].x = defaultX[3];
    cb->calA.dpdPoint[3].y = defaultY[3];
    cb->calA.dpdPoint[3].size = cb->reportRate;
    cb->calA.dpdPoint[3].index = 3;
    cb->calA.accZero[0] = 0x212;
    cb->calA.accZero[1] = 0x212;
    cb->calA.accZero[2] = 0x212;
    cb->calA.accOne[0] = 0x27C;
    cb->calA.accOne[1] = 0x27C;
    cb->calA.accOne[2] = 0x27C;
    cb->calA.volume = 0;
    cb->calA.motor = 0;
    if (result != 0) {
        return;
    }
    if (cb->calibrationPhase == 0) {
        for (block = 0; block < 2; block++) {
            s32 offset = block * 11;
            u32 sum = 0;
            s32 j;

            for (j = offset; j < offset + 10; j++) {
                sum += data[j];
            }
            if ((u8)(sum + 0x55) == data[offset + 10]) {
                u8* raw = &data[offset];
                WPADDpdCalPoint* point;

                WPADiDebugPrint("Dpd Setting is ok.\n");
                read[0].x = (s16)(raw[0] & 0xFF) | ((raw[2] << 4) & 0x300);
                read[0].y = 767 - (s16)((s16)(raw[1] & 0xFF) | ((raw[2] << 2) & 0x300));
                read[0].size = cb->reportRate;
                read[0].index = 0;
                read[1].x = (s16)(raw[3] & 0xFF) | ((raw[2] << 8) & 0x300);
                read[1].y = 767 - (s16)((s16)(raw[4] & 0xFF) | ((raw[2] << 6) & 0x300));
                read[1].size = cb->reportRate;
                read[1].index = 1;
                read[2].x = (s16)(raw[5] & 0xFF) | ((raw[7] << 4) & 0x300);
                read[2].y = 767 - (s16)((s16)(raw[6] & 0xFF) | ((raw[7] << 2) & 0x300));
                read[2].size = cb->reportRate;
                read[2].index = 2;
                read[3].x = (s16)(raw[8] & 0xFF) | ((raw[7] << 8) & 0x300);
                read[3].y = 767 - (s16)((s16)(raw[9] & 0xFF) | ((raw[7] << 6) & 0x300));
                read[3].size = cb->reportRate;
                read[3].index = 3;
                point = read;
                for (j = 0; j < 4; j++, point++) {
                    if (point->x < 512 && point->y < 384) {
                        cb->calA.dpdPoint[0].x = point->x;
                        cb->calA.dpdPoint[0].y = point->y;
                        cb->calA.dpdPoint[0].size = point->size;
                        cb->calA.dpdPoint[0].index = point->index;
                    }
                    if (point->x > 512 && point->y < 384) {
                        cb->calA.dpdPoint[1].x = point->x;
                        cb->calA.dpdPoint[1].y = point->y;
                        cb->calA.dpdPoint[1].size = point->size;
                        cb->calA.dpdPoint[1].index = point->index;
                    }
                    if (point->x > 512 && point->y > 384) {
                        cb->calA.dpdPoint[2].x = point->x;
                        cb->calA.dpdPoint[2].y = point->y;
                        cb->calA.dpdPoint[2].size = point->size;
                        cb->calA.dpdPoint[2].index = point->index;
                    }
                    if (point->x < 512 && point->y > 384) {
                        cb->calA.dpdPoint[3].x = point->x;
                        cb->calA.dpdPoint[3].y = point->y;
                        cb->calA.dpdPoint[3].size = point->size;
                        cb->calA.dpdPoint[3].index = point->index;
                    }
                }
                break;
            }
            WPADiDebugPrint("Dpd Setting is broken.\n");
        }
    } else {
        WPADiDebugPrint("Old Firmware uses default values because it has no Dpd Setting.\n");
    }
    sumX = 0.0f;
    sumY = sumX;
    for (i = 0; i < 4; i++) {
        px[i] = cb->calA.dpdPoint[i].x;
        py[i] = cb->calA.dpdPoint[i].y;
        sumX += (f32)(cb->calA.dpdPoint[i].x - defaultX[i]);
        sumY += (f32)(cb->calA.dpdPoint[i].y - defaultY[i]);
        WPADiDebugPrint("x = %lf, y = %lf\n", px[i], py[i]);
    }
    sumX *= 0.25f;
    sumY *= 0.25f;
    angleX = (f32)atan((126.5f + sumX) / 1332.5f) - (f32)atan(126.5f / 1332.5f);
    angleY = (f32)atan((93.0f + sumY) / 1337.4f) - (f32)atan(93.0f / 1337.4f);
    wpadDpdCenterX[chan] = 1332.5f * (-1.0f * (f32)tan(angleX));
    wpadDpdCenterY[chan] = 1337.4f * (-1.0f * (f32)tan(angleY));
    WPADiDebugPrint("difaveX = %lf, difaveY = %lf\n", sumX, sumY);
    WPADiDebugPrint("deltaX  = %lf, deltaY  = %lf\n", angleX, angleY);
    WPADiDebugPrint("_wpadCalibrationX = %lf, _wpadCalibrationY = %lf\n", wpadDpdCenterX[chan], wpadDpdCenterY[chan]);
    wpadDpdPivotY[chan] = 0.0f;
    wpadDpdPivotX[chan] = 0.0f;
    for (i = 0; i < 4; i++) {
        px[i] += wpadDpdCenterX[chan];
        wpadDpdPivotX[chan] += px[i];
        py[i] += wpadDpdCenterY[chan];
        wpadDpdPivotY[chan] += py[i];
    }
    wpadDpdPivotX[chan] *= 0.25f;
    wpadDpdPivotY[chan] *= 0.25f;
    WPADiDebugPrint("centX  = %lf, centY  = %lf\n", wpadDpdPivotX[chan], wpadDpdPivotY[chan]);
    wpadDpdRoll[chan] = 0.0f;
    for (i = 0; i < 4; i++) {
        measured[i] = (f32)atan((py[i] - wpadDpdPivotY[chan]) / (px[i] - wpadDpdPivotX[chan]));
        expected[i] = (f32)atan(((f32)defaultY[i] - 383.5f) / ((f32)defaultX[i] - 511.5f));
        wpadDpdRoll[chan] += measured[i] - expected[i];
    }
    wpadDpdRoll[chan] *= 0.25f;
    WPADiDebugPrint("_wpadRolag  = %lf\n", wpadDpdRoll[chan]);
    for (block = 0; block < 2; block++) {
        s32 offset = block * 10;
        u32 sum = 0;
        s32 j;

        if (cb->calibrationPhase == 0) {
            offset += 22;
        }
        for (j = offset; j < offset + 9; j++) {
            sum += data[j];
        }
        if ((u8)(sum + 0x55) == data[offset + 9]) {
            WPADiDebugPrint("Acc is ok.\n");
            cb->calA.accZero[0] = (data[offset + 0] << 2) | ((data[offset + 3] >> 4) & 3);
            cb->calA.accZero[1] = (data[offset + 1] << 2) | ((data[offset + 3] >> 2) & 3);
            cb->calA.accZero[2] = (data[offset + 2] << 2) | (data[offset + 3] & 3);
            cb->calA.accOne[0] = (data[offset + 4] << 2) | ((data[offset + 7] >> 4) & 3);
            cb->calA.accOne[1] = (data[offset + 5] << 2) | ((data[offset + 7] >> 2) & 3);
            cb->calA.accOne[2] = (data[offset + 6] << 2) | (data[offset + 7] & 3);
            cb->calA.volume = data[offset + 8] & 0x7F;
            cb->calA.motor = data[offset + 8] & 0x80;
            break;
        }
        WPADiDebugPrint("Acc is broken.\n");
    }
    WPADiDebugPrint("0G:  accX = %d,  accY = %d,  accZ = %d\n", cb->calA.accZero[0], cb->calA.accZero[1], cb->calA.accZero[2]);
    WPADiDebugPrint("1G:  accX = %d,  accY = %d,  accZ = %d\n", cb->calA.accOne[0], cb->calA.accOne[1], cb->calA.accOne[2]);
    WPADiDebugPrint("Volume: %d,   Motor: %d\n", cb->calA.volume, cb->calA.motor);
}

/* Stores the extension's calibration block (the first of its two copies whose checksums hold) and reports the
 * finished extension handshake. */
void wpadExtCalibrationReadDone(s32 chan, s32 result)
{
    WPADCB* cb = wpadCBTable[chan];
    s32 valid = -1;
    u8* data = (u8*)cb->replyA;
    s32 offset;
    s32 block;

    switch (result) {
    case -1:
        cb->deviceType = 0xFD;
        cb->extensionResult = 0xFD;
        cb->extStatus = 0;
        break;
    case 0:
        cb->deviceType = wpadExtType[chan];
        break;
    default:
        cb->deviceType = 0xFC;
        cb->extensionResult = 0xFC;
        cb->extStatus = 0;
        break;
    }
    cb->extensionSubType = wpadExtSubType[chan];
    if (cb->deviceType >= 1 && cb->deviceType <= 2) {
        for (block = 0, offset = 0; block < 2; block++, offset += 0x10) {
            s32 i;
            u32 sum = 0;
            for (i = offset; i < offset + 14; i++) {
                sum += data[i];
            }
            if ((u8)(sum + 0x55) == data[offset + 14] && (u8)(sum + 0xAA) == data[offset + 15]) {
                valid = offset;
            }
        }
        switch (cb->deviceType) {
        case 1:
            if (valid < 0) {
                WPADiDebugPrint("check sum error.\n");
                cb->calB.nunchuk.accZero[0] = 0x200;
                cb->calB.nunchuk.accZero[1] = 0x200;
                cb->calB.nunchuk.accZero[2] = 0x200;
                cb->calB.nunchuk.accOne[0] = 0x2CC;
                cb->calB.nunchuk.accOne[1] = 0x2CC;
                cb->calB.nunchuk.accOne[2] = 0x2CC;
            } else {
                cb->calB.nunchuk.accZero[0] = (data[valid] << 2) | ((data[valid + 3] >> 4) & 3);
                cb->calB.nunchuk.accZero[1] = (data[valid + 1] << 2) | ((data[valid + 3] >> 2) & 3);
                cb->calB.nunchuk.accZero[2] = (data[valid + 2] << 2) | (data[valid + 3] & 3);
                cb->calB.nunchuk.accOne[0] = (data[valid + 4] << 2) | ((data[valid + 7] >> 4) & 3);
                cb->calB.nunchuk.accOne[1] = (data[valid + 5] << 2) | ((data[valid + 7] >> 2) & 3);
                cb->calB.nunchuk.accOne[2] = (data[valid + 6] << 2) | (data[valid + 7] & 3);
                cb->calB.nunchuk.stickX[2] = (s8)data[valid + 8];
                cb->calB.nunchuk.stickX[1] = (s8)data[valid + 9];
                cb->calB.nunchuk.stickY[2] = (s8)data[valid + 11];
                cb->calB.nunchuk.stickY[1] = (s8)data[valid + 12];
                if (cb->centerValid == 0) {
                    cb->calB.nunchuk.stickX[0] = (s8)data[valid + 10];
                    cb->calB.nunchuk.stickY[0] = (s8)data[valid + 13];
                }
                WPADiDebugPrint("0G:  fsaccX = %d,  fsaccY = %d,  fsaccZ = %d\n", cb->calB.nunchuk.accZero[0],
                                cb->calB.nunchuk.accZero[1], cb->calB.nunchuk.accZero[2]);
                WPADiDebugPrint("1G:  fsaccX = %d,  fsaccY = %d,  fsaccZ = %d\n", cb->calB.nunchuk.accOne[0],
                                cb->calB.nunchuk.accOne[1], cb->calB.nunchuk.accOne[2]);
                WPADiDebugPrint("FS:  X = %d,  X max = %d,  X min = %d\n", cb->calB.nunchuk.stickX[0], cb->calB.nunchuk.stickX[2],
                                cb->calB.nunchuk.stickX[1]);
                WPADiDebugPrint("FS:  Y = %d,  Y max = %d,  Y min = %d\n", cb->calB.nunchuk.stickY[0], cb->calB.nunchuk.stickY[2],
                                cb->calB.nunchuk.stickY[1]);
            }
            break;
        case 2: {
            cb->calB.classic.leftX[2] = (s8)data[valid];
            cb->calB.classic.leftX[1] = (s8)data[valid + 1];
            cb->calB.classic.leftY[2] = (s8)data[valid + 3];
            cb->calB.classic.leftY[1] = (s8)data[valid + 4];
            cb->calB.classic.rightX[2] = (s8)data[valid + 6];
            cb->calB.classic.rightX[1] = (s8)data[valid + 7];
            cb->calB.classic.rightY[2] = (s8)data[valid + 9];
            cb->calB.classic.rightY[1] = (s8)data[valid + 10];
            if (cb->centerValid == 0) {
                cb->calB.classic.leftX[0] = (s8)data[valid + 2];
                cb->calB.classic.leftY[0] = (s8)data[valid + 5];
                cb->calB.classic.rightX[0] = (s8)data[valid + 8];
                cb->calB.classic.rightY[0] = (s8)data[valid + 11];
                cb->calB.classic.leftTrigger = data[valid + 12];
                cb->calB.classic.rightTrigger = data[valid + 13];
            }
            WPADiDebugPrint("CL:  X = %d,  X max = %d,  X min = %d\n", cb->calB.classic.leftX[0], cb->calB.classic.leftX[2],
                            cb->calB.classic.leftX[1]);
            WPADiDebugPrint("CL:  Y = %d,  Y max = %d,  Y min = %d\n", cb->calB.classic.leftY[0], cb->calB.classic.leftY[2],
                            cb->calB.classic.leftY[1]);
            WPADiDebugPrint("CR:  X = %d,  X max = %d,  X min = %d\n", cb->calB.classic.rightX[0], cb->calB.classic.rightX[2],
                            cb->calB.classic.rightX[1]);
            WPADiDebugPrint("CR:  Y = %d,  Y max = %d,  Y min = %d\n", cb->calB.classic.rightY[0], cb->calB.classic.rightY[2],
                            cb->calB.classic.rightY[1]);
            WPADiDebugPrint("LR:  L = %d,  R = %d\n", cb->calB.classic.leftTrigger, cb->calB.classic.rightTrigger);
            break;
        }
        }
    }
    if (cb->deviceType != 3 || cb->replyD != 1) {
        WPADCallback callback = cb->cmdCallback;
        if (callback != NULL && callback == cb->extensionCallback) {
            callback(chan, cb->deviceType);
            cb->cmdCallback = NULL;
        }
    }
}

/* TRUE for the extension types the first-party key generator serves. */
static inline BOOL wpadIsFirstPartyExtension(u8 type)
{
    if ((type >= 1 && type <= 4) || (type >= 17 && type <= 18)) {
        return TRUE;
    }
    return FALSE;
}

/* Classifies the extension from its identification block, keeps or drops its calibration, and starts the encrypted
 * handshake (or reports an unsupported extension). */
void wpadExtIdReadDone(s32 chan, s32 result)
{
    WPADCB* cb = wpadCBTable[chan];
    u8* id = (u8*)cb->replyA;
    WPADCmdQueue* queue = &cb->auxQueue;

    if (result == 0) {
        WPADiDebugPrint("devId: %d, subId: %d\n", id[5], id[0]);
        wpadSticksZeroed[chan] = 0;
        wpadExtSubType[chan] = id[4];
        switch (id[5]) {
        case 0:
            wpadExtType[chan] = 1;
            break;
        case 1:
            wpadExtType[chan] = 2;
            break;
        case 2:
            wpadExtType[chan] = WUDIsLinkedWBC() != 0 ? 3 : 0xFB;
            break;
        case 4:
            wpadExtType[chan] = WPADiGetReserved0() != 0 ? 4 : 0xFB;
            break;
        case 16:
            wpadExtType[chan] = WPADiGetReserved1() != 0 ? 16 : 0xFB;
            break;
        case 3:
            if (__OSInIPL != 0) {
                wpadExtType[chan] = 2;
                wpadExtSubType[chan] = 1;
                wpadSticksZeroed[chan] = 1;
            } else {
                switch (id[0]) {
                case 0:
                    wpadExtType[chan] = WPADiGetReserved2() != 0 ? 17 : 0xFB;
                    break;
                case 1:
                    wpadExtType[chan] = WPADiGetReserved3() != 0 ? 18 : 0xFB;
                    break;
                default:
                    wpadExtType[chan] = 0xFB;
                    break;
                }
            }
            break;
        default:
            wpadExtType[chan] = 0xFB;
            break;
        }
        if (wpadExtType[chan] == 2 && !(wpadExtSubType[chan] >= 1 && wpadExtSubType[chan] <= 3)) {
            wpadExtType[chan] = 0xFC;
        }
        WPADiDebugPrint("type : %d\n", wpadExtType[chan]);
        WPADiDebugPrint("mode : %d\n", wpadExtSubType[chan]);
        WPADiDebugPrint("last : %d\n", cb->extensionResult);
        if (cb->extStatus != 0 && cb->extensionResult == wpadExtType[chan]) {
            if (cb->centerValid != 0) {
                WPADiDebugPrint("Keep calibration!\n");
                cb->centerValid = 1;
            }
        } else {
            WPADiDebugPrint("Recalibration!\n");
            cb->centerValid = 0;
        }
        cb->extensionResult = wpadExtType[chan];
        cb->extStatus = 0;
        if ((u8)(wpadExtType[chan] + 5) <= 1) {
            cb->deviceType = wpadExtType[chan];
            cb->extensionSubType = wpadExtSubType[chan];
            if (cb->extensionCallback != NULL) {
                cb->extensionCallback(chan, cb->deviceType);
            }
        } else {
            if (wpadIsFirstPartyExtension(wpadExtType[chan])) {
                cb->extInitState = 2;
                WPADiCreateKey(chan);
            } else {
                cb->extInitState = 3;
                WPADiCreateKeyFor3rd(chan);
            }
            wpadSendWriteByte(queue, 0xAA, 0x04A400F0, wpadExtInitCallback);
            WPADiSendWriteData(queue, &cb->key[0], 6, 0x04A40040, wpadExtInitCallback);
            WPADiSendWriteData(queue, &cb->key[6], 6, 0x04A40046, wpadExtInitCallback);
            WPADiSendWriteData(queue, &cb->key[12], 4, 0x04A4004C, wpadExtInitCallback);
            if (wpadExtType[chan] == 3) {
                wpadSendWriteByte(queue, 0xAA, 0x04A400F1, wpadExtInitCallback);
                wpadSendWriteByte(queue, 0xAA, 0x04A400F1, wpadExtInitCallback);
                wpadSendWriteByte(queue, 0xAA, 0x04A400F1, wpadExtInitCallback);
            }
            wpadSendReadMemory(queue, cb->extId, 0x20, 0x04A40020, cb->extensionCallback);
        }
    } else {
        cb->deviceType = 0xFC;
        cb->extensionSubType = 0;
        cb->extensionResult = 0xFC;
        cb->extStatus = 0;
    }
}

/* Checks the checksum of the pointer calibration block just read and stores the block when it is intact. */
void wpadDpdCalibrationReadDone(s32 chan, s32 error, s32 which)
{
    WPADCB* cb = wpadCBTable[chan];
    s32 i;
    s32 sum = 0;
    u8* data = (u8*)cb->replyA;

    if (error == 0) {
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
    u32 level;
    s32 wasAttached;
    WPADCallback callback;

    wasAttached = cb->flags.extensionAttached;
    WPADiDebugPrint("Received report 20\n");
    level = OSDisableInterrupts();
    if (cb->ready == 0) {
        OSRestoreInterrupts(level);
        return;
    }
    cb->flags.extensionAttached = (s16)(report[3] & 2) >> 1;
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
            WPADiDebugPrint("initialize attachment\n");
            wpadExtInitRetries[chan] = 0;
            callback = cb->extensionCallback;
            wpadStartExtInit(chan);
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
            cb->extStatusTimer = 0x12C;
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
    s16 delta;
    u32 base;
    s32 result;
    u32 level;
    u32 high;
    WPADCB* cb = wpadCBTable[chan];
    u16 low;
    WPADCallback callback;
    u16 offset;
    u8 size;

    level = OSDisableInterrupts();
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
    delta = offset - low;
    if (offset >= low && offset <= low + cb->replyD) {
        memcpy((u8*)cb->replyA + delta, report + 6, size);
        if (low + cb->replyD == offset + size) {
            WPADiDebugPrint("base addr: %08x\n", cb->replyB);
            WPADiDebugPrint("length   : %d\n", (u32)cb->replyD);
            result = (s32)cb->replyC < 0 ? -3 : 0;
            WPADiDebugPrint("i2c = %04x\n", base >> 16);
            WPADiDebugPrint("enc = %d\n", (u32)cb->extInitState);
            if (high == 0x4A4) {
                WPADiDebugPrint("Access to extension register.\n");
                if (cb->extInitState >= 2 && cb->extInitState <= 3) {
                    WPADiDebugPrint("Decode!!!!\n");
                    WPADiDebugPrint("    len = %d, addr = %04x\n", (u32)size, (s32)low);
                    wpadDecryptExtension(chan, (u8*)cb->replyA, size, low);
                }
            }
            if ((cb->replyB == 0 && cb->calibrationPhase == 0) || (cb->replyB == 0x176C && (s32)cb->calibrationPhase == 1)) {
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
s32 wpadDispatchReport(u8 chan, u8* report)
{
    WPADCB* cb = wpadCBTable[chan];
    s32 result = 0;

    if ((u8)(report[0] + 0xE0) <= 0x1F) {
        u32 level = OSDisableInterrupts();
        WPADCB* current = wpadCBTable[chan];
        WPADSample* sample = &current->samples[current->sampleIndex];

        if (report[0] != 0x3E && report[0] != 0x3F) {
            memset(sample, 0, sizeof(WPADSample));
        }
        wpadReportHandlers[report[0] - 0x20](chan, report, sample);
        if (cb->ready == 0) {
            sample->extensionError = -4;
        }
        if (report[0] != 0x3E && report[0] != 0x3F) {
            WPADCB* next = wpadCBTable[chan];
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
void wpadDecodeDpd(s32 chan, WPADSample** sample, u32 format, u8* data, s32 length)
{
    u8 i;
    u8 slot;

    if (wpadCBTable[chan]->dpdBlocked != 0) {
        (*sample)->extensionError = -4;
        return;
    }
    if (format == 3) {
        for (slot = 0; slot < 4; slot++) {
            s32 offset = slot * 3;
            if (offset + 2 < length) {
                u8* object = data + offset;
                u8 x = object[0];
                u8 high = object[2];
                u8 y = object[1];
                (*sample)->dpd[slot].x = (s16)(x & 0xFF) | ((high << 4) & 0x300);
                (*sample)->dpd[slot].y = 0x2FF - (s16)((s16)(y & 0xFF) | ((high << 2) & 0x300));
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
            u8* object = data + slot * 3 - pair;
            u8 x = object[0];
            u8 y = object[1];
            u8 high = data[pair * 5 + 2];
            if (slot % 2 == 0) {
                (*sample)->dpd[slot].x = (s16)(x & 0xFF) | ((high << 4) & 0x300);
                (*sample)->dpd[slot].y = 0x2FF - (s16)((s16)(y & 0xFF) | ((high << 2) & 0x300));
            } else {
                (*sample)->dpd[slot].x = (s16)(x & 0xFF) | ((high << 8) & 0x300);
                (*sample)->dpd[slot].y = 0x2FF - (s16)((s16)(y & 0xFF) | ((high << 6) & 0x300));
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
        if ((*sample)->dpd[i].x != 0 || (*sample)->dpd[i].y != 0x2FF) {
            WPADDpdObject* object = &(*sample)->dpd[i];
            f32 dx = ((f32)(*sample)->dpd[i].x + wpadDpdCenterX[chan]) - wpadDpdPivotX[chan];
            f32 dy = ((f32)object->y + wpadDpdCenterY[chan]) - wpadDpdPivotY[chan];
            f32 term = dy * (f32)sin(-1.0f * wpadDpdRoll[chan]);
            object->x = wpadDpdPivotX[chan] + ((dx * (f32)cos(-1.0f * wpadDpdRoll[chan])) - term);
            term = dy * (f32)cos(-1.0f * wpadDpdRoll[chan]);
            (*sample)->dpd[i].y = wpadDpdPivotY[chan] + ((dx * (f32)sin(-1.0f * wpadDpdRoll[chan])) + term);
        }
    }
}

/* Decodes one 9-byte pointer record of an interleaved report into `slot` of the sample, plus its bounding box. */
void wpadDecodeDpdInterleaved(s32 chan, WPADSample** sample, u8 slot, u8* data, s32 reserved)
{
    (*sample)->dpd[slot].x = (s16)(data[0] & 0xFF) | ((data[2] << 4) & 0x300);
    (*sample)->dpd[slot].y = 0x2FF - (s16)((s16)(data[1] & 0xFF) | ((data[2] << 2) & 0x300));
    (*sample)->dpdExt[slot].intensity = (((s16)((data[7] << 8) & 0xFF00) | data[8]) << 6) & 0xFFC0;
    (*sample)->dpdExt[slot].sizeExtra = data[2] & 0xF;
    (*sample)->dpdExt[slot].left = (s8)data[3] == -1 ? 0 : data[3];
    (*sample)->dpdExt[slot].top = (s8)data[4] == -1 ? 0 : data[4];
    (*sample)->dpdExt[slot].right = (s8)data[5] == -1 ? 0 : data[5];
    (*sample)->dpdExt[slot].bottom = (s8)data[6] == -1 ? 0 : data[6];
    (*sample)->dpdExt[slot].left = (*sample)->dpdExt[slot].left * 8;
    (*sample)->dpdExt[slot].top = 0x2FF - (s16)((*sample)->dpdExt[slot].top * 8);
    (*sample)->dpdExt[slot].right = (*sample)->dpdExt[slot].right * 8;
    (*sample)->dpdExt[slot].bottom = 0x2FF - (s16)((*sample)->dpdExt[slot].bottom * 8);
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
void wpadDecodeExtDualStick(s32 chan, WPADSample** sample, u8 variant, u8* data, u32 length)
{
    WPADCB* cb = wpadCBTable[chan];

    switch (variant) {
    case 2:
        (*sample)->classic.leftX = (s16)((s16)(data[0] * 4) & ~3) | (data[4] & 3);
        (*sample)->classic.rightX = (s16)((s16)(data[1] * 4) & ~3) | (s16)((s16)((s32)data[4] >> 2) & 3);
        (*sample)->classic.leftY = (s16)((s16)(data[2] * 4) & ~3) | (s16)((s16)((s32)data[4] >> 4) & 3);
        (*sample)->classic.rightY = (s16)((s16)(data[3] * 4) & ~3) | (s16)((s32)data[4] >> 6);
        (*sample)->classic.leftTrigger = data[5];
        (*sample)->classic.rightTrigger = (length < 9) ? 0 : data[6];
        (*sample)->classic.buttons = (length < 9) ? 0 : (((data[8] & ~0xFF00) | ((data[7] << 8) & 0xFF00)) ^ 0xFFFF);
        break;
    case 3:
        (*sample)->classic.leftX = (s16)(data[0] & 0xFF) * 4;
        (*sample)->classic.rightX = (s16)(data[1] & 0xFF) * 4;
        (*sample)->classic.leftY = (s16)(data[2] & 0xFF) * 4;
        (*sample)->classic.rightY = (s16)(data[3] & 0xFF) * 4;
        (*sample)->classic.leftTrigger = data[4];
        (*sample)->classic.rightTrigger = data[5];
        (*sample)->classic.buttons = (length < 8) ? 0 : (((data[7] & ~0xFF00) | ((data[6] << 8) & 0xFF00)) ^ 0xFFFF);
        break;
    default:
        (*sample)->classic.leftX = (data[0] * 0x10) & 0x3F0;
        (*sample)->classic.leftY = (data[1] * 0x10) & 0x3F0;
        (*sample)->classic.rightX = (s16)((s16)((s32)data[2] >> 7) | ((s16)((s16)((s32)data[0] >> 3) & 0x18) | ((s16)((s16)((s32)data[1] >> 5) & 6) & ~0x18))) << 5;
        (*sample)->classic.rightY = (data[2] << 5) & 0x3E0;
        (*sample)->classic.leftTrigger = (u8)(((((s16)((s32)data[2] >> 2) & 0x18) | (((s32)data[3] >> 5) & ~0x18)) * 8) & 0xF8);
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
        s16 value;
        s16 clamped;

        value = (*sample)->classic.leftX - cb->calB.classic.leftX[0];
        clamped = value;
        if (value < -0x200) {
            clamped = -0x200;
        }
        if (value > 0x1FF) {
            clamped = 0x1FF;
        }
        (*sample)->classic.leftX = clamped;
        value = (*sample)->classic.leftY - cb->calB.classic.leftY[0];
        clamped = value;
        if (value < -0x200) {
            clamped = -0x200;
        }
        if (value > 0x1FF) {
            clamped = 0x1FF;
        }
        (*sample)->classic.leftY = clamped;
    }
    if (cb->deviceType == 2) {
        s16 value;
        s16 clamped;

        value = (*sample)->classic.rightX - cb->calB.classic.rightX[0];
        clamped = value;
        if (value < -0x200) {
            clamped = -0x200;
        }
        if (value > 0x1FF) {
            clamped = 0x1FF;
        }
        (*sample)->classic.rightX = clamped;
        value = (*sample)->classic.rightY - cb->calB.classic.rightY[0];
        clamped = value;
        if (value < -0x200) {
            clamped = -0x200;
        }
        if (value > 0x1FF) {
            clamped = 0x1FF;
        }
        (*sample)->classic.rightY = clamped;
        value = (*sample)->classic.leftTrigger - cb->calB.classic.leftTrigger;
        clamped = value;
        if (value < 0) {
            clamped = 0;
        }
        if (value > 0xFF) {
            clamped = 0xFF;
        }
        (*sample)->classic.leftTrigger = clamped;
        value = (*sample)->classic.rightTrigger - cb->calB.classic.rightTrigger;
        clamped = value;
        if (value < 0) {
            clamped = 0;
        }
        if (value > 0xFF) {
            clamped = 0xFF;
        }
        (*sample)->classic.rightTrigger = clamped;
    }
    if (wpadSticksZeroed[chan] != 0) {
        (*sample)->classic.rightX = 0;
        (*sample)->classic.rightY = 0;
        (*sample)->classic.leftTrigger = 0;
        (*sample)->classic.rightTrigger = 0;
    }
}

/* Decodes the 21-byte payload of the board-style extension into the sample and its calibration. */
void wpadDecodeExtBoard(s32 chan, WPADSample** sample, u8 format, u8* data, u32 length)
{
    (*sample)->board.batteryRef = data[0];
    (*sample)->board.weightRef[0] = (data[1] << 2) | (data[6] >> 6);
    (*sample)->board.weightRef[1] = (data[2] << 2) | ((data[6] >> 4) & 3);
    (*sample)->board.weightRef[2] = (data[3] << 2) | ((data[6] >> 2) & 3);
    (*sample)->board.weightRef[3] = (data[4] << 2) | (data[6] & 3);
    (*sample)->board.weightRef[4] = (data[5] << 2) | (data[7] >> 6);
    (*sample)->board.temperature = data[8];
    (*sample)->board.weight[0] = (data[9] << 2) | (data[14] >> 6);
    (*sample)->board.weight[1] = (data[10] << 2) | ((data[14] >> 4) & 3);
    (*sample)->board.weight[2] = (data[11] << 2) | ((data[14] >> 2) & 3);
    (*sample)->board.weight[3] = (data[12] << 2) | (data[14] & 3);
    (*sample)->board.weight[4] = (data[13] << 2) | (data[15] >> 6);
    (*sample)->board.refA = ((data[7] << 4) & 0xF3F0) | ((data[15] >> 2) & 0xF);
    (*sample)->board.refC = data[15] & 3;
    if (length < 21) {
        (*sample)->board.refB = 0;
        (*sample)->board.refD = 0;
        (*sample)->board.refE = 0;
        (*sample)->board.refF = 0;
        return;
    }
    (*sample)->board.refB = (data[16] << 2) | (data[17] >> 6);
    (*sample)->board.refD = data[18] & 7;
    (*sample)->board.refE = data[19];
    (*sample)->board.refF = data[20];
}

/* Decodes the button word of the 0x32 report, 8 extension bytes follow. */
void wpadReportButtonsExt8(u8 chan, u8* report, WPADSample* sample)
{
    WPADCB* cb = wpadCBTable[chan];
    WPADSample* decoded = sample;

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
        if (cb->deviceType == 1) {
            wpadDecodeNunchukAt(chan, report + 3, &decoded);
        } else if (cb->deviceType == 2) {
            wpadDecodeExtDualStick(chan, &decoded, cb->extensionSubType, report + 3, 8);
        } else if (cb->deviceType == 16) {
            WPADSample* simple = decoded;
            simple->simple.stickX = report[5];
            simple->simple.stickY = report[6];
            simple->simple.buttons = ((report[10] & ~0xFF00) | ((report[9] << 8) & 0xFF00)) ^ 0xFFFF;
        }
        if (memcmp(wpadExtRaw, wpadExtNone, 8) == 0 && sample->extensionError == 0) {
            sample->extensionError = -7;
        }
    }
}

/* Decodes the 0x33 report: buttons, accelerometer and 12 bytes of pointer data. */
void wpadReportButtonsAccelDpd12(u8 chan, u8* report, WPADSample* sample)
{
    WPADCB* cb = wpadCBTable[chan];

    sample->buttons = ((report[1] & ~0xFF00) | ((report[2] << 8) & 0xFF00)) & 0x9F1F;
    sample->deviceType = cb->deviceType;
    if (cb->dataFormat <= 2) {
        sample->extensionError = 0;
    } else {
        sample->extensionError = -4;
    }
    cb->flags.buttonByteTopBit = (report[1] >> 7) & 1;
    wpadDecodeAccelAt(chan, report, &sample);
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
            WPADBLStatus* board = (WPADBLStatus*)sample;
            s32 doubled;
            board->press[0] = (report[4] & ~0xFF00) | ((report[3] << 8) & 0xFF00);
            board->press[1] = (report[6] & ~0xFF00) | ((report[5] << 8) & 0xFF00);
            board->press[2] = (report[8] & ~0xFF00) | ((report[7] << 8) & 0xFF00);
            board->press[3] = (report[10] & ~0xFF00) | ((report[9] << 8) & 0xFF00);
            board->temp = report[11];
            board->battery = report[13];
            doubled = board->battery * 2;
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
        if (memcmp(wpadExtRaw, wpadExtNone, 0x13) == 0 && sample->extensionError == 0) {
            sample->extensionError = -7;
        }
    }
}

/* Decodes the 0x35 report: buttons, accelerometer and 16 extension bytes. */
void wpadReportButtonsAccelExt16(u8 chan, u8* report, WPADSample* sample)
{
    WPADCB* cb = wpadCBTable[chan];
    WPADSample* decoded = sample;

    sample->buttons = ((report[1] & ~0xFF00) | ((report[2] << 8) & 0xFF00)) & 0x9F1F;
    if (cb->dataFormat <= 1 || (u32)(cb->dataFormat - 3) <= 1 || (u32)(cb->dataFormat - 6) <= 1 || cb->dataFormat == 13) {
        sample->extensionError = 0;
    } else {
        sample->extensionError = -4;
    }
    sample->deviceType = cb->deviceType;
    cb->flags.buttonByteTopBit = (report[1] >> 7) & 1;
    wpadDecodeAccel(chan, report, sample);
    memcpy(wpadExtRaw, report + 6, 0x10);
    wpadDecryptExtension(chan, report + 6, 0x10, 0);
    if (cb->flags.extensionAttached != 0) {
        if (cb->deviceType == 1) {
            wpadDecodeNunchukAt(chan, report + 6, &decoded);
        } else if (cb->deviceType == 2) {
            wpadDecodeExtDualStick(chan, &decoded, cb->extensionSubType, report + 6, 0x10);
        } else if (cb->deviceType == 4) {
            wpadDecodeExtBoard(chan, &decoded, cb->dataFormat, report + 6, 0x10);
        }
        if (memcmp(wpadExtRaw, wpadExtNone, 0x10) == 0 && sample->extensionError == 0) {
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
    if (cb->flags.extensionAttached != 0 && memcmp(wpadExtRaw, wpadExtNone, 9) == 0 && sample->extensionError == 0) {
        sample->extensionError = -7;
    }
}

/* Decodes the 0x37 report: buttons, accelerometer, 10 bytes of pointer data and 6 extension bytes. */
void wpadReportButtonsAccelDpd10Ext6(u8 chan, u8* report, WPADSample* sample)
{
    WPADCB* cb = wpadCBTable[chan];
    WPADSample* decoded = sample;

    decoded->buttons = ((report[1] & ~0xFF00) | ((report[2] << 8) & 0xFF00)) & 0x9F1F;
    if (cb->dataFormat <= 8 || cb->dataFormat == 11 || cb->dataFormat == 15) {
        decoded->extensionError = 0;
    } else {
        decoded->extensionError = -4;
    }
    decoded->deviceType = cb->deviceType;
    cb->flags.buttonByteTopBit = (report[1] >> 7) & 1;
    wpadDecodeAccelAt(chan, report, &decoded);
    wpadDecodeDpd(chan, &decoded, cb->dpdFormat, report + 6, 10);
    memcpy(wpadExtRaw, report + 16, 6);
    wpadDecryptExtension(chan, report + 16, 6, 0);
    if (cb->flags.extensionAttached != 0) {
        if (cb->deviceType == 1) {
            wpadDecodeNunchukAt(chan, report + 16, &sample);
        } else if (cb->deviceType == 2 || cb->deviceType == 0x11 || cb->deviceType == 0x12) {
            wpadDecodeExtDualStick(chan, &sample, cb->extensionSubType, report + 16, 6);
        }
        if (memcmp(wpadExtRaw, wpadExtNone, 6) == 0 && decoded->extensionError == 0) {
            decoded->extensionError = -7;
        }
    }
}

/* Decodes the 0x3D report: 21 extension bytes and no buttons. */
void wpadReportExt21(u8 chan, u8* report, WPADSample* sample)
{
    WPADCB* cb = wpadCBTable[chan];
    WPADSample* decoded = sample;

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
            wpadDecodeExtBoard(chan, &decoded, cb->dataFormat, report + 1, 0x15);
        }
        if (memcmp(wpadExtRaw, wpadExtNone, 0x15) == 0 && sample->extensionError == 0) {
            sample->extensionError = -7;
        }
    }
}

/* Decodes the first half (slots 0 and 1) of an interleaved pointer report. */
void wpadReportInterleavedA(u8 chan, u8* report, WPADSample* sample)
{
    WPADCB* cb = wpadCBTable[chan];
    WPADSample* decoded = sample;
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
    sample->accX = wpadInterleavedAxis(chan, report[3], report[1], 0);
    sample->accZ = sample->accZ | (s16)((s16)((s16)(report[2] << 3) & ~0xFF) | ((report[1] << 1) & 0xC0));
    wpadDecodeDpdInterleaved(chan, &decoded, 0, report + 4, 0);
    wpadDecodeDpdInterleaved(chan, &decoded, 1, report + 13, 0);
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
    WPADSample* decoded = sample;
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
    sample->accY = wpadInterleavedAxis(chan, report[3], report[1], 1);
    sample->accZ = sample->accZ | (s16)(((report[1] >> 3) & 0xC) | ((report[2] >> 1) & 0x30));
    wpadDecodeDpdInterleaved(chan, &decoded, 2, report + 4, 0);
    wpadDecodeDpdInterleaved(chan, &decoded, 3, report + 13, 0);
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

    sample->buttons = ((report[1] & ~0xFF00) | ((report[2] << 8) & 0xFF00)) & 0x9F1F;
    sample->deviceType = cb->deviceType;
    if (cb->dataFormat <= 1) {
        sample->extensionError = 0;
    } else {
        sample->extensionError = -4;
    }
    cb->flags.buttonByteTopBit = (report[1] >> 7) & 1;
    wpadDecodeAccel(chan, report, sample);
}
