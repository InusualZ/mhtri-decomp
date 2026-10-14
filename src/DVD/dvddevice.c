/*
 * DVD/dvddevice.c - the SDK DVD device check (`__DVDCheckDevice` and its low-level callback).
 *
 * RANGE. `.text` 0x804ABF40-0x804AC1D0; `.rodata` 0x805739E0-0x80573A00; `.data` 0x80619408-0x806195A0; `.bss`
 *   0x80746980-0x807469A0; `.sdata` 0x80793E00-0x80793E08; `.sbss` 0x80795150-0x80795158; `.sdata2`
 *   0x8079D0E0-0x8079D0E8 (2 functions / 0x28C B).
 *   - `lowDone` (.sdata), `lowIntType` (.sbss), `CheckBuffer` (.bss), `__DVDDeviceErrorMessage` (.rodata, whose seven
 *     pointers are the `.data` literals) and the white text colour (.sdata2): data no other unit reads
 * FLAGS. `cflags_base` (`Wii/1.3`, -O4,p) in the BTE-tail SDK block of `configure.py`, as the sibling `DVD/dvdFatal.c`.
 * NAMES. the unit name is a GUESS (named for `__DVDCheckDevice`); `DVD_DISC_LAYER_FLAG` (low memory 0x8000319C, 0x81
 *   selects the second read offset) is a GUESS. The map rows of the seven `.data` literals keep their generated `lbl_`
 *   names (the source spells them as literals).
 * RESIDUALS. `__DVDCheckDevice`: the two "error code has a high byte" exits return FALSE inline (`beq`+`li`+`b`)
 *   where the SDK jumps to one shared `return FALSE` after the fatal block (a `goto`; that shape matches byte for byte,
 *   rule 8 has no exemption for it, and no loop/switch nesting reaches a block between the fatal block and
 *   `__DVDShowFatalMessage`).
 * SHAPES. the low-level result is read through `switch`es (case 2 = error, case 1 = completed, default = a failed
 *   request; the single-case `switch (lowIntType)` and `switch (code)` keep the target's `beq`+`b`); a
 *   `do { } while (0)` gives the failed-request paths (`continue`) the shared `__DVDShowFatalMessage` tail; the
 *   unauthorized-device screen is the `static inline` `ShowDeviceErrorMessage` (as a block in place its colours swap
 *   stack slots and registers); the first `DVDLowGetImmBufferReg` after `DVDLowRequestError` is a discarded call.
 */
#include "DVD/dvddevice.h"
#include "DVD/dvd_broadway.h"
#include "DVD/dvdFatal.h"
#include "OS/OS.h"
#include "OS/OSFatal.h"
#include "OS/OSFont.h"
#include "OS/OSMemory.h"
#include "SC/SCApi.h"
#include "gx.h"

#define DVD_DISC_LAYER_FLAG (*(u8*)0x8000319C)

static u8 CheckBuffer[32] __attribute__((aligned(32)));
static volatile BOOL lowDone = TRUE;
static volatile s32 lowIntType;

/* The console languages (SCGetLanguage), in table order: Japanese, English, German, French, Spanish, Italian, Dutch. */
const char* const __DVDDeviceErrorMessage[] = {
    "\n\n\nエラーコード００１。\n"
    "不明なデバイスが見つかりました。",
    "\n\n\nError #001,\n"
    "unauthorized device has been detected.",
    "\n\n\nFehler #001:\n"
    "Es wurde eine unzul\344ssige Komponente\n"
    "entdeckt.",
    "\n\n\nErreur 001:\n"
    "un dispositif non autoris\351 a \351t\351 d\351tect\351.",
    "\n\n\nError 001:\n"
    "Se ha detectado un dispositivo no\n"
    "autorizado.",
    "\n\n\nErrore #001:\n"
    "rilevato un dispositivo non autorizzato.",
    "\n\n\nFout #001:\n"
    "ongeoorloofd onderdeel gevonden.",
};

/* 0x804ABF40 (0x10): records the low-level result and marks the request done. */
static void lowCallback(u32 intType) {
    lowIntType = intType;
    lowDone = TRUE;
}

/* Halts with the unauthorized-device message in the console language. */
static inline void ShowDeviceErrorMessage(void) {
    GXColor bg = {0, 0, 0, 0};
    GXColor fg = {255, 255, 255, 0};
    const char* const* messages;
    const char* message;

    if (SCGetLanguage() == 0) {
        OSSetFontEncode(1);
    } else {
        OSSetFontEncode(0);
    }

    messages = __DVDDeviceErrorMessage;
    if (SCGetLanguage() > 6) {
        message = messages[1];
    } else {
        message = messages[SCGetLanguage()];
    }

    OSFatal(fg, bg, message);
}

/* 0x804ABF50 (0x27C): probes the drive with an unencrypted read and a key report; halts on an unauthorized device. */
BOOL __DVDCheckDevice(void) {
    OSIOSRev rev;
    u32 offset;

    offset = 0x460A0000;
    if (OSGetPhysicalMem2Size() == 0x08000000) {
        return TRUE;
    }

    __OSGetIOSRev(&rev);
    if (rev.major < 30 || rev.major >= 254) {
        return TRUE;
    }

    if (DVD_DISC_LAYER_FLAG == 0x81) {
        offset = 0x7ED40000;
    }

    do {
        lowDone = FALSE;
        DVDLowUnencryptedRead(CheckBuffer, sizeof(CheckBuffer), offset, lowCallback);
        while (!lowDone) {
        }

        switch (lowIntType) {
        case 2:
            lowDone = FALSE;
            DVDLowRequestError(lowCallback);
            while (!lowDone) {
            }
            DVDLowGetImmBufferReg();
            switch (lowIntType) {
            case 1:
                break;
            default:
                continue;
            }
            if (DVDLowGetImmBufferReg() & 0xFF000000) {
                return FALSE;
            }
            switch ((s32)(DVDLowGetImmBufferReg() & 0xFFFFFF)) {
            case 0x52100:
                lowDone = FALSE;
                DVDLowReportKey(CheckBuffer, 0x40000, 0, lowCallback);
                while (!lowDone) {
                }

                switch (lowIntType) {
                case 2:
                    lowDone = FALSE;
                    DVDLowRequestError(lowCallback);
                    while (!lowDone) {
                    }
                    DVDLowGetImmBufferReg();
                    switch (lowIntType) {
                    case 1:
                        break;
                    default:
                        continue;
                    }
                    if (DVDLowGetImmBufferReg() & 0xFF000000) {
                        return FALSE;
                    }
                    switch ((s32)(DVDLowGetImmBufferReg() & 0xFFFFFF)) {
                    case 0x53100:
                    case 0x52000:
                        return TRUE;
                    }
                    break;
                case 1:
                    break;
                default:
                    continue;
                }
                break;
            }
            break;
        case 1:
            break;
        default:
            continue;
        }

        ShowDeviceErrorMessage();
        return FALSE;
    } while (0);

    __DVDShowFatalMessage();
    return FALSE;
}
