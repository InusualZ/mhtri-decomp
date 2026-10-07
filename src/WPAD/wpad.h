/*
 * WPAD/wpad.h - the entry points of `WPAD/wpad.cpp` that other units call, and the control block types.
 */
#ifndef WPAD_WPAD_H
#define WPAD_WPAD_H

#include "types.h"
#include "OS/OSThread.h"
#include "OS/OSAlarm.h"

/* The completion callback of a command: the channel and the result (0 = success, negative = error). */
typedef void (*WPADCallback)(s32 chan, s32 result);

/* Runs once per sample with the channel. */
typedef void (*WPADSamplingCallback)(s32 chan);

/* size: 0x30 - one queued output report: the report id, its payload and the callback that reports the result. */
typedef struct WPADCommand {
    /* +0x00 */ u32 reportId;
    /* +0x04 */ u8 data[0x16];
    /* +0x1A */ u16 length;
    /* +0x1C */ u32 argA;
    /* +0x20 */ u16 argB;
    /* +0x22 */ u8 pad_0x22[2];
    /* +0x24 */ u32 argC;
    /* +0x28 */ void* param; /* untyped: the caller-owned payload the report's reply fills in */
    /* +0x2C */ WPADCallback callback;
} WPADCommand; /* size: 0x30 */

/* size: 0x0C - a ring of `WPADCommand` slots: `tail` is the next slot to fill, `head` the next to send. */
typedef struct WPADCmdQueue {
    /* +0x00 */ s8 head;
    /* +0x01 */ s8 tail;
    /* +0x02 */ u8 pad_0x02[2];
    /* +0x04 */ WPADCommand* items;
    /* +0x08 */ s32 capacity;
} WPADCmdQueue; /* size: 0x0C */

/* size: 0x08 - one tracked pointer object: the position, the size and the slot it was reported in. */
typedef struct WPADDpdObject {
    /* +0x00 */ s16 x;
    /* +0x02 */ s16 y;
    /* +0x04 */ s16 size;
    /* +0x06 */ s8 slot;
    /* +0x07 */ u8 pad_0x07;
} WPADDpdObject; /* size: 0x08 */

/* size: 0x0C - the bounding box, intensity and size of one pointer object of an interleaved report. */
typedef struct WPADDpdExt {
    /* +0x00 */ s16 left;
    /* +0x02 */ s16 top;
    /* +0x04 */ s16 right;
    /* +0x06 */ s16 bottom;
    /* +0x08 */ s16 intensity;
    /* +0x0A */ s8 sizeExtra;
    /* +0x0B */ u8 pad_0x0B;
} WPADDpdExt; /* size: 0x0C */

/* size: 0x60 - one decoded input report: the button word at +0x00, the accelerometer, the pointer objects and the extension. */
typedef struct WPADSample {
    /* +0x00 */ u16 buttons;
    /* +0x02 */ s16 accX;
    /* +0x04 */ s16 accY;
    /* +0x06 */ s16 accZ;
    /* +0x08 */ WPADDpdObject dpd[4];
    /* +0x28 */ u8 deviceType;
    /* +0x29 */ s8 extensionError;
    /* +0x2A */ union {
        u16 extensionCode;
        struct { /* the Nunchuk-style extension: the accelerometer relative to its zero and the stick */
            /* +0x2A */ s16 accX;
            /* +0x2C */ s16 accY;
            /* +0x2E */ s16 accZ;
            /* +0x30 */ s8 stickX;
            /* +0x31 */ s8 stickY;
        } nunchuk; /* size: 0x08 */
        struct { /* the Classic-style extension: the button word, two sticks and two triggers */
            /* +0x2A */ u16 buttons;
            /* +0x2C */ s16 leftX;
            /* +0x2E */ s16 leftY;
            /* +0x30 */ s16 rightX;
            /* +0x32 */ s16 rightY;
            /* +0x34 */ u8 leftTrigger;
            /* +0x35 */ u8 rightTrigger;
        } classic; /* size: 0x0C */
        struct { /* the extension that reports two bytes after its button word */
            /* +0x2A */ u16 buttons;
            /* +0x2C */ u8 stickX;
            /* +0x2D */ u8 stickY;
        } simple; /* size: 0x04 */
        WPADDpdExt dpdExt[4]; /* the pointer boxes of an interleaved report, which carries no extension */
        struct { /* the balance-board extension (layout guessed from the decode) */
            /* +0x2A */ s16 weight[5];
            /* +0x34 */ u8 temperature;
            /* +0x35 */ u8 pad_0x35;
            /* +0x36 */ s16 weightRef[5];
            /* +0x40 */ u8 batteryRef;
            /* +0x41 */ u8 pad_0x41;
            /* +0x42 */ s16 refA;
            /* +0x44 */ s16 refB;
            /* +0x46 */ s8 refC;
            /* +0x47 */ s8 refD;
            /* +0x48 */ u8 refE;
            /* +0x49 */ u8 refF;
        } board; /* size: 0x20 */
    };
    /* +0x5A */ u8 pad_0x5A[6];
} WPADSample; /* size: 0x60 */

/* size: 0x18 - the remote's status block: the capability flags the status report sets (handed to the caller of `WPADGetInfoAsync`). */
typedef struct WPADFlags {
    /* +0x00 */ s32 dpdEnabled;
    /* +0x04 */ s32 speakerEnabled;
    /* +0x08 */ s32 extensionAttached;
    /* +0x0C */ s32 batteryLow;
    /* +0x10 */ u32 buttonByteTopBit;
    /* +0x14 */ u8 batteryLevel;
    /* +0x15 */ s8 ledMask;
    /* +0x16 */ s8 protect;
    /* +0x17 */ s8 statusFlags;
} WPADFlags; /* size: 0x18 */

/* size: 0x2E - the remote's own calibration block: the accelerometer zero and one-G readings. */
typedef struct WPADCalA {
    /* +0x00 */ u8 raw[0x20];
    /* +0x20 */ s16 accZero[3];
    /* +0x26 */ s16 accOne[3];
    /* +0x2C */ u8 pad_0x2C[2];
} WPADCalA; /* size: 0x2E */

/* size: 0x1A - the attached extension's calibration block; the layout depends on the extension type. */
typedef union WPADCalB {
    struct { /* the Nunchuk-style extension: stick centre triples, then the accelerometer zero and one-G readings */
        /* +0x00 */ s16 stickX[3];
        /* +0x06 */ s16 stickY[3];
        /* +0x0C */ s16 accZero[3];
        /* +0x12 */ s16 accOne[3];
        /* +0x18 */ u8 pad_0x18[2];
    } nunchuk;
    struct { /* the Classic-style extension: four stick centre triples and the trigger rests */
        /* +0x00 */ s16 leftX[3];
        /* +0x06 */ s16 leftY[3];
        /* +0x0C */ s16 rightX[3];
        /* +0x12 */ s16 rightY[3];
        /* +0x18 */ u8 leftTrigger;
        /* +0x19 */ u8 rightTrigger;
    } classic;
} WPADCalB; /* size: 0x1A */

/* size: 0x06 - one accelerometer reading per axis. */
typedef struct WPADAccGravityUnit {
    /* +0x00 */ s16 x;
    /* +0x02 */ s16 y;
    /* +0x04 */ s16 z;
} WPADAccGravityUnit; /* size: 0x06 */

/* size: 0x57 (approximate: only the name and handle are read) - the HID host's record of a connected device. */
typedef struct WPADDeviceInfo {
    /* +0x00 */ char name[0x56];
    /* +0x56 */ u8 handle;
} WPADDeviceInfo;

/* size: 0x38 - the pointer calibration block as it is stored in the remote's memory. */
typedef struct WPADCalibration {
    /* +0x00 */ s64 timestamp;
    /* +0x08 */ u8 dpdCalibration[0x22];
    /* +0x2A */ u8 titleCode[4];
    /* +0x2E */ u8 appType;
    /* +0x2F */ u8 checksum;
    /* +0x30 */ u8 pad_0x30[8];
} WPADCalibration; /* size: 0x38 */

/* size: 0x9C0 - the per-channel state of one Wii remote. */
typedef struct WPADCB {
    /* +0x000 */ union {
        WPADCalibration cal;
        u8 calBytes[0x38];
    };
    /* +0x038 */ s32 dpdStatusA;
    /* +0x03C */ s32 dpdStatusB;
    /* +0x040 */ WPADSample lastSample;
    /* +0x0A0 */ WPADSample samples[2];
    /* +0x160 */ WPADCmdQueue cmdQueue;
    /* +0x16C */ WPADCommand cmdStorage[0x18];
    /* +0x5EC */ WPADCmdQueue auxQueue;
    /* +0x5F8 */ WPADCommand auxStorage[0xC];
    /* +0x838 */ WPADFlags flags;
    /* +0x850 */ void* infoOut;
    /* +0x854 */ WPADCalA calA;
    /* +0x882 */ WPADCalB calB;
    /* +0x89C */ WPADCallback cmdCallback;
    /* +0x8A0 */ WPADCallback extensionCallback;
    /* +0x8A4 */ WPADCallback connectCallback;
    /* +0x8A8 */ WPADSamplingCallback samplingCallback;
    /* +0x8AC */ void* autoSampleBuf; /* untyped: the caller-owned ring of samples */
    /* +0x8B0 */ s32 latestIndex;
    /* +0x8B4 */ u32 autoSampleCount;
    /* +0x8B8 */ u32 dataFormat;
    /* +0x8BC */ s32 status;
    /* +0x8C0 */ u8 statusRequested;
    /* +0x8C1 */ u8 deviceType;
    /* +0x8C2 */ u8 extensionSubType;
    /* +0x8C3 */ s8 devHandle;
    /* +0x8C4 */ u32 unused_0x8C4;
    /* +0x8C8 */ u8 sampleIndex;
    /* +0x8C9 */ u8 unused_0x8C9;
    /* +0x8CA */ u8 reportRate;
    /* +0x8CB */ u8 dpdFormat;
    /* +0x8CC */ u8 dpdFormatWanted;
    /* +0x8CD */ u8 weakRadio;
    /* +0x8CE */ u8 weakRadioHysteresis;
    /* +0x8CF */ u8 pendingStreamPackets;
    /* +0x8D0 */ u32 motorDirty;
    /* +0x8D4 */ u32 motorOn;
    /* +0x8D8 */ u32 unused_0x8D8;
    /* +0x8DC */ s32 ready;
    /* +0x8E0 */ u32 calibrationPhase;
    /* +0x8E4 */ OSThreadQueue threadQueue;
    /* +0x8EC */ u8 pad_0x8EC[4];
    /* +0x8F0 */ s64 lastReportTime;
    /* +0x8F8 */ u16 unused_0x8F8[6];
    /* +0x904 */ u8 pad_0x904[4];
    /* +0x908 */ s64 reconnectTime;
    /* +0x910 */ u8 unused_0x910;
    /* +0x911 */ u8 centerValid;
    /* +0x912 */ u16 idleCount;
    /* +0x914 */ u8 pad_0x914[0x10];
    /* +0x924 */ u8 keyAdd[8];
    /* +0x92C */ u8 keyXor[8];
    /* +0x934 */ u8 extId[6];
    /* +0x93A */ u8 pad_0x93A[0x3A];
    /* +0x974 */ u32 replyA;
    /* +0x978 */ u32 replyB;
    /* +0x97C */ u32 replyC;
    /* +0x980 */ u16 replyD;
    /* +0x982 */ u8 unused_0x982;
    /* +0x983 */ u8 radioSensitivity;
    /* +0x984 */ u16 sampleCount;
    /* +0x986 */ u8 disconnecting;
    /* +0x987 */ u8 lastReportId;
    /* +0x988 */ WPADCallback infoCallback;
    /* +0x98C */ u8 infoPending;
    /* +0x98D */ u8 extInitState;
    /* +0x98E */ u8 reportOnChange;
    /* +0x98F */ u8 extBatteryLevel;
    /* +0x990 */ u8 extensionResult;
    /* +0x991 */ u8 unused_0x991;
    /* +0x992 */ s16 unused_0x992;
    /* +0x994 */ u8 dpdBlocked;
    /* +0x995 */ u8 pad_0x995[3];
    /* +0x998 */ u32 speakerStateA;
    /* +0x99C */ u32 speakerStateB;
    /* +0x9A0 */ u16 speakerStateC;
    /* +0x9A2 */ u8 pad_0x9A2[2];
    /* +0x9A4 */ u32 speakerStateD;
    /* +0x9A8 */ WPADCallback bulkDoneCallback;
    /* +0x9AC */ u8 pad_0x9AC[0x14];
} WPADCB; /* size: 0x9C0 */

#ifdef __cplusplus
extern "C" {
#endif

void WPADiDebugPrint(const char* format, ...);

/* The driver's own functions, in address order. */
BOOL wpadShutdownCallback(BOOL final, u32 event);
void wpadSendCommand(s32 chan, WPADCommand* cmd);
void wpadUpdateRadioSensitivity(s32 chan);
BOOL wpadSampleChanged(WPADCB* cb, WPADSample* sample, WPADSample* last);
void wpadCountIdleReports(s32 chan, WPADSample* sample);
void wpadPollSample(s32 chan);
void wpadSamplingFiber(void);
void wpadAlarmHandler(OSAlarm* alarm, OSContext* context);
void wpadResetChannel(s32 chan);
void wpadStart(void);
void WPADInit(void);
void WPADStartSyncDevice(void);
void WPADStartFastSimpleSync(void);
void WPADStopSimpleSync(void);
void WPADStartClearDevice(void);
void WPADSetSyncDeviceCallback(void (*callback)(s32 result));
void WPADSetSimpleSyncCallback(void (*callback)(s32 result));
void WPADSetClearDeviceCallback(void (*callback)(s32 result));
void WPADRegisterAllocator(void* (*alloc)(u32 size), s32 (*dealloc)(void* block)); /* untyped: the caller-owned block */
s32 WPADGetStatus(void);
u8 WPADGetRadioSensitivity(s32 chan);
u8 WPADGetSensorBarPosition(void);
void wpadInitDoneCallback(s32 chan, s32 result);
void wpadAbortCallback(s32 chan, s32 result);
void wpadHidDataCallback(u8 handle, u8* report, u16 length);
s32 wpadAssignChannel(WPADDeviceInfo* info);
void wpadHidOpenCallback(WPADDeviceInfo* info, s32 connected);
void wpadInitSequence(s32 chan, s32 result);
void WPADGetAccGravityUnit(s32 chan, s32 type, WPADAccGravityUnit* unit);
void wpadCloseLinkCallback(s32 chan, s32 result);
void WPADDisconnect(s32 chan);
void WPADSetAutoSleepTime(s8 minutes);
s32 WPADProbe(s32 chan, u32* type);
WPADSamplingCallback WPADSetSamplingCallback(s32 chan, WPADSamplingCallback callback);
WPADCallback WPADSetConnectCallback(s32 chan, WPADCallback callback);
WPADCallback WPADSetExtensionCallback(s32 chan, WPADCallback callback);
u32 WPADGetDataFormat(s32 chan);
s32 WPADSetDataFormat(s32 chan, u32 format);
void wpadInfoCallback(s32 chan, s32 result);
s32 WPADGetInfoAsync(s32 chan, void* info, WPADCallback callback); /* untyped: the caller-owned status record */
void WPADControlMotor(s32 chan, u32 command);
void WPADEnableMotor(s32 enable);
s32 WPADIsMotorEnabled(void);
s32 WPADControlLed(s32 chan, u8 leds, WPADCallback callback);
BOOL WPADSaveConfig(void (*callback)(s32 result));
s32 WPADGetLatestIndexInBuf(s32 chan);
s32 WPADIsSpeakerEnabled(s32 chan);
u8 WPADGetSpeakerVolume(void);
void WPADSetSpeakerVolume(u8 volume);
u8 WPADGetDpdSensitivity(void);
s32 WPADIsDpdEnabled(s32 chan);
u8 WPADGetDpdFormat(s32 chan);
void wpadApplyDpdFormat(s32 chan);
s32 WPADiSendSetReportType(WPADCmdQueue* queue, u32 type, s32 onChangeOnly, WPADCallback callback);
void WPADiClearQueue(WPADCmdQueue* queue);
void WPADSetCallbackByKPAD(s32 owned);
u8 wpadGetAppType(void);
char* wpadGetGameName(void);
s32 wpadDispatchReport(s32 chan, u8* report);
void wpadResetSpeakerState(s32 chan);
void wpadSetDpdStatusA(s32 chan, s32 error);
void wpadSetDpdStatusB(s32 chan, s32 error);
void wpadReportButtons(u8 chan, u8* report, WPADSample* sample);
void wpadReportStatus(u8 chan, u8* report, WPADSample* sample);
void wpadReportReadData(u8 chan, u8* report, WPADSample* sample);
void wpadReportAck(u8 chan, u8* report, WPADSample* sample);
void wpadReportIgnored(u8 chan, u8* report, WPADSample* sample);
void wpadReportButtonsExt8(u8 chan, u8* report, WPADSample* sample);
void wpadReportButtonsAccelDpd12(u8 chan, u8* report, WPADSample* sample);
void wpadReportExt19(u8 chan, u8* report, WPADSample* sample);
void wpadReportButtonsAccelExt16(u8 chan, u8* report, WPADSample* sample);
void wpadReportButtonsDpd10Ext9(u8 chan, u8* report, WPADSample* sample);
void wpadReportButtonsAccelDpd10Ext6(u8 chan, u8* report, WPADSample* sample);
void wpadReportExt21(u8 chan, u8* report, WPADSample* sample);
void wpadReportInterleavedA(u8 chan, u8* report, WPADSample* sample);
void wpadReportInterleavedB(u8 chan, u8* report, WPADSample* sample);
void wpadDecodeDpd(s32 chan, WPADSample** sample, u32 format, const u8* data, s32 length);
void wpadDecodeDpdInterleaved(s32 chan, WPADSample** sample, u8 slot, const u8* data, s32 reserved);
void wpadDecodeExtDualStick(s32 chan, WPADSample** sample, u8 variant, const u8* data, u32 length);
void wpadDecodeExtBoard(s32 chan, WPADSample** sample, u8 format, const u8* data, u32 length);
void wpadExtInitCallback(s32 chan, s32 result);
void wpadAccCalibrationReadDone(s32 chan, s32 result);
void wpadExtCalibrationReadDone(s32 chan, s32 result);
void wpadExtIdReadDone(s32 chan, s32 result);
void wpadDpdCalibrationReadDone(s32 chan, s32 error, s32 which);
void wpadCopyTitleString(const u16* title);
s32 wpadSendWbcCommand(s32 chan, s8 command, WPADCallback callback);
void wpadBulkWriteNext(s32 chan, s32 result);
void wpadBulkWriteFirst(s32 chan, s32 result);
s32 wpadBulkWrite(s32 chan, const void* src, u16 size, u32 addr, WPADCallback callback); /* untyped: byte range */
void wpadReportButtonsAccel(u8 chan, u8* report, WPADSample* sample);
void wpadDecryptExtension(s32 chan, u8* data, u16 length, s32 offset);
s32 wpadGetDpdCalibration(s32 chan, u8** out);
void wpadCopyCurrentSample(s32 chan, void* dest); /* untyped: caller-owned sample record */
void WPADSetAutoSamplingBuf(s32 chan, void* buf, u32 count); /* untyped: caller-owned ring of samples */
void wpadFilterButtons(s32 chan);
void wpadStoreSample(s32 chan);
BOOL __wpadIsBusyStream(s32 chan);
BOOL WPADCanSendStreamData(s32 chan);
s32 WPADSendStreamData(s32 chan, const void* data, u32 length); /* untyped: byte range */
s32 wpadSendWriteByte(WPADCmdQueue* queue, s8 value, u32 addr, WPADCallback callback);
s32 WPADiSendWriteData(WPADCmdQueue* queue, const void* src, s32 size, u32 addr, WPADCallback callback); /* untyped: byte range */
s32 wpadSendReadMemory(WPADCmdQueue* queue, void* dest, u16 size, u32 addr, WPADCallback callback); /* untyped: caller-owned byte buffer */
s32 wpadQueueHasRoom(WPADCmdQueue* queue, s8 count);
void wpadSpeakerDoneCallback(s32 chan, s32 result);
s32 wpadReadMemoryGuarded(s32 chan, void* dest, u16 size, u32 addr, WPADCallback callback); /* untyped: caller-owned byte buffer */
s32 wpadReadMemory(s32 chan, void* dest, u16 size, u32 addr, WPADCallback callback); /* untyped: caller-owned byte buffer */
s32 wpadWriteMemory(s32 chan, const void* src, s32 size, u32 addr, WPADCallback callback); /* untyped: byte range */
s32 WPADWriteExtReg(s32 chan, const void* src, s32 size, u32 addr, WPADCallback callback); /* untyped: byte range */

void* WPADiNullCallbackA(u32 size); /* untyped: caller-owned block */
s32 WPADiNullCallbackB(void* block); /* untyped: the caller-owned block */
u32 WPADiGetReserved0(void);
u32 WPADiGetReserved1(void);
u32 WPADiGetReserved2(void);
u32 WPADiGetReserved3(void);
s32 WPADiReturnZeroA(void);
s32 WPADiReturnZeroB(void);
s32 WPADiReturnZeroC(void);
s32 WBCReadDummy(void);
s32 WBCSetZEROPointDummy(void);
s32 WBCGetTGCWeightDummy(void);

#ifdef __cplusplus
}
#endif

#endif
