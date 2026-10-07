/*
 * SC/SCApi.c - the SC item getters and setters: language, sound mode, screen saver, display offset, Bluetooth and
 *    Wiimote settings, country code and the parental-control checks.
 *
 * RANGE. .text 0x804DC9E0-0x804DD010 (21 functions, 0x630 B); .data 0x80629E88-0x80629EB8; .bss
 *    0x80756600-0x80757608.  Cut from the old SC block between `SC/SCSystemConfig.c` (0x804DC9E0) and
 *    `SC/SCProductInfo.c` (0x804DD010).
 * FLAGS. the `OS` lib's group with `cflags_base` (configure.py `cflags=`): every function start of the band is
 *    16-aligned (`-O4,p` default alignment), not the 4-byte packing `cflags_os` gives.
 * NAMES. map names throughout (`SCGetLanguage`, `SCGetCountryCode`, `SCCheckPC*Restriction`, ...); `SCGetAspectRatio`,
 *    `SCGetProgressiveMode`, `SCGetBtDeviceInfoArray`, `SCGetBtCmpDeviceInfoArray`, `SCSetBtCmpDeviceInfoArray`,
 *    `SimpleAddress` (.bss 0x80756600) and `SCReplaceU8Item` (0x804DC360) are GUESSes from the item names in the
 *    item-name strings (IPL.AR, IPL.PGS, BT.DINF, BT.CDIF, IPL.SADR); the file name `SCApi.c` is a GUESS.
 * EVIDENCE. every function calls `SCFind*Item`/`SCReplace*Item` of the core unit and nothing else; `.bss`
 *    0x80756600 (0x1008 B, the IPL.SADR record) is read by `SCGetCountryCode` only; `.data` 0x80629E88 is the
 *    `<< RVL_SDK - SCCheckPCMessageRestriction >>` string; the core unit's last data reader is the flush at
 *    0x804DC6A0.
 * RESIDUALS. none: all 21 rows are 100 % and the object matches the target in .text, .data and .bss.
 * SHAPES. each getter is `if (!find(&v, item)) v = default; else clamp`, the value kept in a stack slot.
 */

#include "types.h"

#include "OS/OSError.h"
#include "OS/OSInterrupt.h"
#include "Runtime.PPCEABI.H/memset.h"
#include "SC/SCApi.h"
#include "SC/SCGetScreenSaverMode.h"
#include "SC/SCGetWpadMotorMode.h"
#include "SC/SCProductInfo.h"
#include "SC/SCSystemConfig.h"

/* size: 0x1008 - the IPL.SADR simple-address record; only its first word is read here. */
typedef struct SCSimpleAddress {
    /* +0x00 */ u32 head;
    /* +0x04 */ u8 body[0x1004];
} SCSimpleAddress;

SCSimpleAddress SimpleAddress;

u8 SCGetAspectRatio(void)
{
    u8 value;
    if (!SCFindU8Item(&value, 1)) {
        value = 0;
    } else if (value != 1) {
        value = 0;
    }
    return value;
}

s8 SCGetDisplayOffsetH(void)
{
    s8 value;
    if (!SCFindS8Item(&value, 5)) {
        value = 0;
    } else if (value < -32) {
        value = -32;
    } else if (value > 32) {
        value = 32;
    }
    return value & ~1;
}

BOOL SCGetIdleMode(SCIdleModeInfo* info)
{
    return SCFindByteArrayItem(info, 2, 9);
}

u8 SCGetLanguage(void)
{
    u8 value;
    if (!SCFindU8Item(&value, 11)) {
        if (SCGetProductArea() == 0) {
            value = 0;
        } else {
            value = 1;
        }
    } else if (value > 9) {
        value = 1;
    }
    return value;
}

u8 SCGetProgressiveMode(void)
{
    u8 value;
    if (!SCFindU8Item(&value, 14)) {
        value = 0;
    } else if (value != 1) {
        value = 0;
    }
    return value;
}

u8 SCGetScreenSaverMode(void)
{
    u8 value;
    if (!SCFindU8Item(&value, 15)) {
        value = 1;
    } else if (value != 1) {
        value = 0;
    }
    return value;
}

u8 SCGetSoundMode(void)
{
    u8 value;
    if (!SCFindU8Item(&value, 17)) {
        value = 1;
    } else if (value > 2) {
        value = 1;
    }
    return value;
}

u32 SCGetCounterBias(void)
{
    u32 value;
    if (!SCFindU32Item(&value, 0)) {
        value = 0x0B49D800;
    }
    return value;
}

/* untyped: byte range */
BOOL SCGetBtDeviceInfoArray(void* array)
{
    return SCFindByteArrayItem(array, 1121, 28);
}

/* untyped: byte range */
s32 SCSetBtDeviceInfoArray(const void* array)
{
    return SCReplaceByteArrayItem(array, 1121, 28);
}

/* untyped: byte range */
BOOL SCGetBtCmpDeviceInfoArray(void* array)
{
    return SCFindByteArrayItem(array, 517, 29);
}

/* untyped: byte range */
BOOL SCSetBtCmpDeviceInfoArray(const void* array)
{
    return SCReplaceByteArrayItem(array, 517, 29);
}

u32 SCGetBtDpdSensibility(void)
{
    u32 value;
    if (!SCFindU32Item(&value, 30)) {
        value = 2;
    } else if (value < 1) {
        value = 1;
    } else if (value > 5) {
        value = 5;
    }
    return value;
}

u8 SCGetWpadMotorMode(void)
{
    u8 value;
    if (!SCFindU8Item(&value, 32)) {
        value = 1;
    } else if (value != 1) {
        value = 0;
    }
    return value;
}

s32 SCSetWpadMotorMode(s32 mode)
{
    return SCReplaceU8Item(mode, 32);
}

u8 SCGetWpadSensorBarPosition(void)
{
    u8 value;
    if (!SCFindU8Item(&value, 33)) {
        value = 0;
    } else if (value != 1) {
        value = 0;
    }
    return value;
}

u8 SCGetWpadSpeakerVolume(void)
{
    u8 value;
    if (!SCFindU8Item(&value, 31)) {
        value = 89;
    } else if (value > 127) {
        value = 127;
    }
    return value;
}

s32 SCSetWpadSpeakerVolume(u8 volume)
{
    return SCReplaceU8Item(volume, 31);
}

static inline BOOL LoadSimpleAddress(void)
{
    u32 head;

    if (SCFindByteArrayItem(&SimpleAddress, 4104, 16) && SimpleAddress.head != 0xFFFFFFFF
        && (SimpleAddress.head & 0xFF000000) != 0 && (SimpleAddress.head & 0xFF000000) != 0xFF000000
        && (SimpleAddress.head & 0x00FF0000) != 0x00FF0000) {
        BOOL level = OSDisableInterrupts();
        head = SimpleAddress.head;
        if ((head & 0x00FF0000) == 0) {
            memset(&SimpleAddress, 0, 4104);
            SimpleAddress.head = head;
        }
        OSRestoreInterrupts(level);
        return TRUE;
    }
    return FALSE;
}

BOOL SCGetCountryCode(u8* country)
{
    if (LoadSimpleAddress()) {
        *country = SimpleAddress.head >> 24;
        return TRUE;
    }
    return FALSE;
}

BOOL SCCheckPCMessageRestriction(void)
{
    u32 value;
    OSReport("<< RVL_SDK - SCCheckPCMessageRestriction >>");
    if (!SCFindU32Item(&value, 20)) {
        value = 0;
    }
    return (value >> 1) & 1;
}

BOOL SCCheckPCShopRestriction(void)
{
    u32 value;
    if (!SCFindU32Item(&value, 20)) {
        value = 0;
    }
    return (value >> 2) & 1;
}
