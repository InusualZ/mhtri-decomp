/*
 * KBD/kbd.c - the KBD library: the keyboard driver (`KBDResetChannel`, key processing, key maps, LED requests and
 *    channel records).
 *
 * RANGE. .text 0x80528890-0x8052A040 (28 functions, 0x17B0 B); .data 0x806493A0-0x806498C8; .bss
 *    0x8078FF40-0x807901C0; .sdata 0x807944A8-0x807944B0; .sbss 0x80795A08-0x80795A18.  Cut from the old VF block
 *    at 0x80528890; the right edge is `homebutton/fn_8052A040.cpp`.
 * FLAGS. the `OS` lib's group with `cflags_base` (configure.py `cflags=`): every function start of the band is
 *    16-aligned (`-O4,p` default alignment), not the 4-byte packing `cflags_os` gives.
 * NAMES. `kbdProcKey`, `KBDResetChannel`, `kbdInitMap`, `kbdInitMapIntl`, `kbd_free_led`, `kbd_alloc_led`,
 *    `kbd_initialize_led_module` and `kbd_init_keyboard` are the map's names; `KBDSetLedsAsync` and
 *    `KBDSetChannelValue` are GUESSES; the file name `kbd.c` is a GUESS; the other 18 rows are GUESSES from what
 *    they do: `KBDInit`, `KBDEnd`, `KBDSetConnectCallback`, `KBDSetDisconnectCallback`, `KBDSetKeyCallback`,
 *    `KBDSetChannelMap`, `KBDSetChannelLeds`, `kbd_client_attach`, `kbd_process_report`, `kbd_repeat_alarm`,
 *    `kbd_update_lock_state`, `kbd_queue_event`, `kbd_led_report_done`, `kbd_shutdown`, `kbd_lookup_key`,
 *    `kbd_read_done`, `kbd_set_protocol_done`, `kbd_set_idle_done`.
 * EVIDENCE. `.data` 0x806493A0 is the build string `<< RVL_SDK - KBD ... >>` read through `.sdata` 0x807944A8 by
 *    0x80529820, followed by the key map tables (0x806493E8, 0x806497D0) and two jump tables; `.sbss`
 *    0x80795A08..0x80795A18 and `.bss` 0x8078FF40 (0x200 B), 0x80790140 (0x80 B) are read here only;
 *    `KBDSetLedsAsync` and `KBDSetChannelValue` are called by `homebutton/tiHKBManager.cpp`.
 * RESIDUALS. all 28 rows have a body (22 at 100 %); `kbd_initialize_led_module` 69 % (the target walks the pool by
 *    base + offset, every source spelling tried folds to a pointer induction variable), `kbd_client_attach` 97 %
 *    (the two callback stack slots sit 4 bytes apart in the target), `KBDSetLedsAsync` 98 % (return-path layout),
 *    `kbd_lookup_key` 98 % and `kbd_update_lock_state` / `KBDSetChannelLeds` 99+ % (register numbering); flipcheck
 *    blockers: the .text differences above and .sdata/.sbss trailing padding the object does not emit (8 B / 4 B).
 * SHAPES. `kbd_store_lock_state` is a `static inline` expanded into `kbd_update_lock_state` and `KBDSetChannelLeds`
 *    (the value is copied before `|= 0x1000` so the 0x1000 test is not folded); member stores through
 *    `kbd_state->...` are not hoisted; `.sbss` objects are defined in reverse address order.
 */
#include "types.h"
#include "HID/hid.h"
#include "KBD/kbd.h"
#include "OS/OSContext.h"
#include "OS/OS.h"
#include "OS/OSInterrupt.h"
#include "OS/OSReset.h"
#include "OS/OSSetAlarm.h"
#include "OS/OSTime.h"
#include "Runtime.PPCEABI.H/memset.h"
#include "VI/vi.h"

#define OS_TIMER_CLOCK (OS_BUS_CLOCK / 4)

#define KBD_CHANNELS 4
#define KBD_MAPS 18
#define KBD_QUEUE 32

/* Key codes with a function in the lock-state word. */
#define KBD_LOCK_CODE_FIRST 0xF000
#define KBD_CODE_NONE 0xFFFF

/* size: 0x10 - a key event as queued for the application. */
typedef struct KBDEvent {
    /* +0x00 */ u8 channel;
    /* +0x01 */ u8 key;
    /* +0x02 */ u8 pad_0x02[0x2];
    /* +0x04 */ s32 type;       /* bit 0 down, bit 1 auto-repeat */
    /* +0x08 */ u32 lockState;
    /* +0x0C */ u16 code;
    /* +0x0E */ u8 pad_0x0E[0x2];
} KBDEvent; /* size: 0x10 */

/* size: 0x1C - the key map of one layout. */
typedef struct KBDKeyMap {
    /* +0x00 */ u8 stride;     /* entries per key; 0 when the layout is absent */
    /* +0x01 */ u8 lockMask;   /* lock-state bits the layout honours */
    /* +0x02 */ u8 pad_0x02[0x2];
    /* +0x04 */ const u16* keys;     /* usage 4.. */
    /* +0x08 */ const u16* modifiers; /* usage 0xE0..0xE7 */
    /* +0x0C */ u8 shiftColumn;
    /* +0x0D */ u8 altColumn;
    /* +0x0E */ u8 ctrlColumn;
    /* +0x0F */ u8 keyCount;   /* usages 4..4+keyCount */
    /* +0x10 */ u8 remap[4][2]; /* {usage, replacement} pairs */
    /* +0x18 */ u8 remapCount;
    /* +0x19 */ u8 pad_0x19[0x3];
} KBDKeyMap; /* size: 0x1C */

struct KBDState;
typedef void (*KBDChannelCallback)(const u8* channel, struct KBDState* state);
typedef void (*KBDDisconnectCallback)(const u8* channel);
typedef void (*KBDKeyEventCallback)(void);
typedef void (*KBDLedCallback)(const u8* channel, u32 arg);

/* size: 0x4 - the first word of an LED request: the report byte while queued, the free-list link otherwise. */
typedef union KBDLedHead {
    /* +0x00 */ struct KBDLedRequest* next;
    /* +0x00 */ u8 report;
} KBDLedHead; /* size: 0x4 */

/* size: 0x20 - an LED request on its way to the keyboard. */
typedef struct KBDLedRequest {
    /* +0x00 */ KBDLedHead head;
    /* +0x04 */ u8 channel;
    /* +0x05 */ u8 pad_0x05[0x3];
    /* +0x08 */ u32 value;
    /* +0x0C */ u32 pending;
    /* +0x10 */ KBDLedCallback callback;
    /* +0x14 */ u32 arg;
    /* +0x18 */ u32 busy;
    /* +0x1C */ u8 pad_0x1C[0x4];
} KBDLedRequest; /* size: 0x20 */

/* size: 0x8 - the free list of LED requests; only the head is used. */
typedef struct KBDLedList {
    /* +0x00 */ KBDLedRequest* head;
    /* +0x04 */ u8 unused_0x04[0x4];
} KBDLedList; /* size: 0x8 */

/* size: 0x2A8 - one keyboard channel. */
typedef struct KBDChannel {
    /* +0x000 */ u8 index;
    /* +0x001 */ u8 pad_0x001[0x3];
    /* +0x004 */ s32 mapIndex;
    /* +0x008 */ u32 state;        /* bit 0 free, bits 1/2 stalled, 0 else */
    /* +0x00C */ u32 sentLeds;
    /* +0x010 */ u32 pendingLeds;
    /* +0x014 */ u8 pad_0x014[0x4];
    /* +0x018 */ OSAlarm unusedAlarm;
    /* +0x048 */ HIDDeviceInfo* device;
    /* +0x04C */ u8 prevModifiers;
    /* +0x04D */ u8 pad_0x04D;
    /* +0x04E */ u8 prevKeys[6];
    /* +0x054 */ KBDEvent queue[KBD_QUEUE];
    /* +0x254 */ u8 head;        /* next slot to write */
    /* +0x255 */ u8 readIndex;   /* KBD_QUEUE while the ring is empty */
    /* +0x256 */ u8 pad_0x256[0x2];
    /* +0x258 */ u32 enabled;
    /* +0x25C */ u32 lockState;
    /* +0x260 */ u8 lockCount[6];
    /* +0x266 */ u16 repeatDelay;
    /* +0x268 */ u16 repeatInterval;
    /* +0x26A */ u8 repeatKey;
    /* +0x26B */ u8 pad_0x26B[0x5];
    /* +0x270 */ OSAlarm repeatAlarm;
    /* +0x2A0 */ u32 toggleLocks;
    /* +0x2A4 */ u8 pad_0x2A4[0x4];
} KBDChannel; /* size: 0x2A8 */

/* size: 0xCC0 - the driver's working memory, handed in by the application. */
typedef struct KBDState {
    /* +0x000 */ KBDChannel channels[KBD_CHANNELS];
    /* +0xAA0 */ KBDChannelCallback onConnect;
    /* +0xAA4 */ KBDDisconnectCallback onDisconnect;
    /* +0xAA8 */ KBDKeyEventCallback onKey;
    /* +0xAAC */ KBDKeyMap maps[KBD_MAPS];
    /* +0xCA4 */ HIDClient client;
} KBDState; /* size: 0xCC0 */

const char* __KBDVersion = "<< RVL_SDK - KBD 	release build: Feb 27 2009 10:05:57 (0x4302_145) >>";

u16 kbd_key_table_intl[500] = {
    0x00FF, 0x0061, 0x0041, 0x00E1, 0x00C1,
    0x003F, 0x0062, 0x0042, 0x0000, 0x0000,
    0x003F, 0x0063, 0x0043, 0x00A9, 0x00A2,
    0x00FF, 0x0064, 0x0044, 0x00F0, 0x00D0,
    0x00FF, 0x0065, 0x0045, 0x00E9, 0x00C9,
    0x003F, 0x0066, 0x0046, 0x0000, 0x0000,
    0x003F, 0x0067, 0x0047, 0x0000, 0x0000,
    0x003F, 0x0068, 0x0048, 0x0000, 0x0000,
    0x00FF, 0x0069, 0x0049, 0x00ED, 0x00CD,
    0x003F, 0x006A, 0x004A, 0x0000, 0x0000,
    0x003F, 0x006B, 0x004B, 0x0000, 0x0000,
    0x00FF, 0x006C, 0x004C, 0x00F8, 0x00D8,
    0x003F, 0x006D, 0x004D, 0x00B5, 0x0000,
    0x00FF, 0x006E, 0x004E, 0x00F1, 0x00D1,
    0x00FF, 0x006F, 0x004F, 0x00F3, 0x00D3,
    0x00FF, 0x0070, 0x0050, 0x00F6, 0x00D6,
    0x00FF, 0x0071, 0x0051, 0x00E4, 0x00C4,
    0x003F, 0x0072, 0x0052, 0x00AE, 0x0000,
    0x003F, 0x0073, 0x0053, 0x00DF, 0x00A7,
    0x00FF, 0x0074, 0x0054, 0x00FE, 0x00DE,
    0x00FF, 0x0075, 0x0055, 0x00FA, 0x00DA,
    0x003F, 0x0076, 0x0056, 0x0000, 0x0000,
    0x00FF, 0x0077, 0x0057, 0x00E5, 0x00C5,
    0x003F, 0x0078, 0x0058, 0x0000, 0x0000,
    0x00FF, 0x0079, 0x0059, 0x00FC, 0x00DC,
    0x00FF, 0x007A, 0x005A, 0x00E6, 0x00C6,
    0x0000, 0x0031, 0x0021, 0x00A1, 0x00B9,
    0x0000, 0x0032, 0x0040, 0x00B2, 0x0000,
    0x0000, 0x0033, 0x0023, 0x00B3, 0x0000,
    0x0000, 0x0034, 0x0024, 0x00A4, 0x00A3,
    0x0000, 0x0035, 0x0025, 0x20AC, 0x0000,
    0x0000, 0x0036, 0x0302, 0x00BC, 0x0000,
    0x0000, 0x0037, 0x0026, 0x00BD, 0x0000,
    0x0000, 0x0038, 0x002A, 0x00BE, 0x0000,
    0x0000, 0x0039, 0x0028, 0x2018, 0x0000,
    0x0000, 0x0030, 0x0029, 0x2019, 0x0000,
    0x0000, 0xF1CD, 0xF1CD, 0xF1CD, 0xF1CD,
    0x0000, 0xF1DB, 0xF1DB, 0xF1DB, 0xF1DB,
    0x0000, 0xF1C8, 0xF1C8, 0xF1C8, 0xF1C8,
    0x0000, 0xF1C9, 0xF1C9, 0xF1C9, 0xF1C9,
    0x0000, 0x0020, 0x0020, 0x0020, 0x0020,
    0x0000, 0x002D, 0x005F, 0x00A5, 0x0000,
    0x0000, 0x003D, 0x002B, 0x00D7, 0x00F7,
    0x0000, 0x005B, 0x007B, 0x00AB, 0x0000,
    0x0000, 0x005D, 0x007D, 0x00BB, 0x0000,
    0x0000, 0x005C, 0x007C, 0x00AC, 0x00A6,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x003B, 0x003A, 0x00B6, 0x00B0,
    0x0000, 0x030D, 0x030E, 0x00B4, 0x00A8,
    0x0000, 0x0300, 0x0303, 0x0000, 0x0000,
    0x00C0, 0x002C, 0x003C, 0x00E7, 0x00C7,
    0x0000, 0x002E, 0x003E, 0x0000, 0x0000,
    0x0000, 0x002F, 0x003F, 0x00BF, 0x0000,
    0xC000, 0xF008, 0xF008, 0xF008, 0xF008,
    0x0000, 0xF061, 0xF061, 0xF061, 0xF061,
    0x0000, 0xF062, 0xF062, 0xF062, 0xF062,
    0x0000, 0xF063, 0xF063, 0xF063, 0xF063,
    0x0000, 0xF064, 0xF064, 0xF064, 0xF064,
    0x0000, 0xF065, 0xF065, 0xF065, 0xF065,
    0x0000, 0xF066, 0xF066, 0xF066, 0xF066,
    0x0000, 0xF067, 0xF067, 0xF067, 0xF067,
    0x0000, 0xF068, 0xF068, 0xF068, 0xF068,
    0x0000, 0xF069, 0xF069, 0xF069, 0xF069,
    0x0000, 0xF06A, 0xF06A, 0xF06A, 0xF06A,
    0x0000, 0xF06B, 0xF06B, 0xF06B, 0xF06B,
    0x0000, 0xF06C, 0xF06C, 0xF06C, 0xF06C,
    0xC000, 0xF020, 0xF020, 0xF020, 0xF020,
    0xC000, 0xF021, 0xF021, 0xF021, 0xF021,
    0xC000, 0xF022, 0xF022, 0xF022, 0xF022,
    0x0000, 0xF1B0, 0xF1B0, 0xF1B0, 0xF1B0,
    0x0000, 0xF1B7, 0xF1B7, 0xF1B7, 0xF1B7,
    0x0000, 0xF1B9, 0xF1B9, 0xF1B9, 0xF1B9,
    0x0000, 0xF1AE, 0xF1AE, 0xF1AE, 0xF1AE,
    0x0000, 0xF1B1, 0xF1B1, 0xF1B1, 0xF1B1,
    0x0000, 0xF1B3, 0xF1B3, 0xF1B3, 0xF1B3,
    0x0000, 0xF1B6, 0xF1B6, 0xF1B6, 0xF1B6,
    0x0000, 0xF1B4, 0xF1B4, 0xF1B4, 0xF1B4,
    0x0000, 0xF1B2, 0xF1B2, 0xF1B2, 0xF1B2,
    0x0000, 0xF1B8, 0xF1B8, 0xF1B8, 0xF1B8,
    0xC000, 0xF007, 0xF007, 0xF007, 0xF007,
    0x0000, 0xF12F, 0xF12F, 0xF12F, 0xF12F,
    0x0000, 0xF12A, 0xF12A, 0xF12A, 0xF12A,
    0x0000, 0xF12D, 0xF12D, 0xF12D, 0xF12D,
    0x0000, 0xF12B, 0xF12B, 0xF12B, 0xF12B,
    0x0000, 0xF10D, 0xF10D, 0xF10D, 0xF10D,
    0x80FF, 0xF171, 0xF131, 0x0000, 0x0000,
    0x80FF, 0xF172, 0xF132, 0x0000, 0x0000,
    0x80FF, 0xF173, 0xF133, 0x0000, 0x0000,
    0x80FF, 0xF174, 0xF134, 0x0000, 0x0000,
    0x80FF, 0xF175, 0xF135, 0x0000, 0x0000,
    0x80FF, 0xF176, 0xF136, 0x0000, 0x0000,
    0x80FF, 0xF177, 0xF137, 0x0000, 0x0000,
    0x80FF, 0xF178, 0xF138, 0x0000, 0x0000,
    0x80FF, 0xF179, 0xF139, 0x0000, 0x0000,
    0x80FF, 0xF170, 0xF130, 0x0000, 0x0000,
    0x80FF, 0xF16E, 0xF12E, 0x0000, 0x0000,
    0x0000, 0x005C, 0x007C, 0x0000, 0x0000,
    0xC000, 0xF02F, 0xF02F, 0xF02F, 0xF02F,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0xF13D, 0xF13D, 0xF13D, 0xF13D,
};

u16 kbd_modifier_table_intl[40] = {
    0xC000, 0xF000, 0xF000, 0xF000, 0xF000,
    0xC000, 0xF001, 0xF001, 0xF001, 0xF001,
    0xC000, 0xF002, 0xF002, 0xF002, 0xF002,
    0xC000, 0xF003, 0xF003, 0xF003, 0xF003,
    0xC000, 0xF000, 0xF000, 0xF000, 0xF000,
    0xC000, 0xF001, 0xF001, 0xF001, 0xF001,
    0xC000, 0xF005, 0xF005, 0xF005, 0xF005,
    0xC000, 0xF003, 0xF003, 0xF003, 0xF003,
};

/* Key codes that do not auto-repeat. */
u16 kbd_no_repeat_codes[8] = {0xF006, 0xF008, 0xF007, 0xF021, 0xF020, 0xF022, 0xF02F};

KBDLedRequest kbd_led_request_pool[16];
u8 kbd_report_buffers[KBD_CHANNELS][0x20];

KBDLedList kbd_led_free_list;
u8 kbd_shutdown_guard;
KBDState* kbd_state;

/* The shutdown record (priority 0x7F); the callback is installed by `KBDInit`. */
OSShutdownFunctionInfo kbd_shutdown_info = {NULL, 0x7F};

s32 KBDResetChannel(u8 channel);
void kbdProcKey(u8 key, s32 type, u8 channel);
void kbd_update_lock_state(u16 code, s32 type, u8 channel);
void kbd_queue_event(KBDEvent* event);
u16 kbd_lookup_key(u8 usage, s32 lockState, s32 mapIndex);
/* untyped: caller-owned buffer */
void kbd_led_report_done(HIDDeviceInfo* info, s32 result, void* buffer, s32 length, u32 arg);
void kbd_free_led(KBDLedRequest* request);
KBDLedRequest* kbd_alloc_led(void);
void kbd_initialize_led_module(void);
void kbd_init_keyboard(u32 channel, HIDDeviceInfo* info);

/* Stores a lock-state word; bit 0x1000 asks for the whole word, otherwise only the high bits replace it. */
static inline void kbd_store_lock_state(u8 channel, u32 value)
{
    if (value & 0x1000) {
        kbd_state->channels[channel].lockState = value & ~0x1000;
    } else {
        BOOL enabled = OSDisableInterrupts();
        KBDChannel* ch = &kbd_state->channels[channel];
        ch->lockState = (ch->lockState & 0x3F) | (value & 0xFC0);
        OSRestoreInterrupts(enabled);
    }
}

/* HID attach callback: binds a boot-protocol keyboard to a free channel, or frees the channel of a removed one. */
s32 kbd_client_attach(HIDClient* client, HIDDeviceInfo* info, s32 attached)
{
    int channel;
    KBDChannel* probe;

    if (attached) {
        if (info->vendorId == 0x46A) {
            return 0;
        }
        if (info->interfaceDescriptor[7] == 1) {
            probe = kbd_state->channels;
            for (channel = 0; channel < KBD_CHANNELS; probe++, channel++) {
                if (probe->device == NULL) {
                    kbd_state->channels[channel].device = info;
                    kbd_state->channels[channel].state &= ~1;
                    KBDResetChannel(channel);
                    VIResetDimmingCount();
                    if (kbd_state->onConnect != NULL) {
                        u8 connected = channel;

                        kbd_init_keyboard(channel, info);
                        kbd_state->onConnect(&connected, kbd_state);
                    }
                    return 1;
                }
            }
        }
        return 0;
    }
    probe = kbd_state->channels;
    for (channel = 0; channel < KBD_CHANNELS; probe++, channel++) {
        if (probe->device == info) {
            kbd_state->channels[channel].device = NULL;
            kbd_state->channels[channel].state |= 1;
            if (kbd_state->onDisconnect != NULL) {
                u8 disconnected = channel;

                kbd_state->onDisconnect(&disconnected);
            }
            KBDResetChannel(channel);
            return 1;
        }
    }
    return 0;
}

/* Diffs a boot-protocol input report against the previous one and turns the changes into key events. */
void kbd_process_report(HIDDeviceInfo* info, const u8* report)
{
    u8 channel;
    KBDChannel* ch;
    s8 j;
    s8 i;
    int k;
    int bit;
    u32 mask;
    u8 key;

    for (channel = 0; channel < KBD_CHANNELS; channel++) {
        if (kbd_state->channels[channel].device == info) {
            break;
        }
    }
    if (channel == KBD_CHANNELS) {
        return;
    }
    ch = &kbd_state->channels[channel];
    if ((u8)(report[2] + 0xFF) <= 2) {
        OSCancelAlarm(&ch->repeatAlarm);
        if (report[2] == 1) {
            ch->state |= 2;
        } else {
            ch->state |= 4;
        }
        return;
    }
    ch->state = 0;
    VIResetDimmingCount();
    mask = 1;
    for (bit = 0; bit < 8; bit++) {
        if (ch->prevModifiers & mask) {
            if (!(report[0] & mask)) {
                kbdProcKey(0xE0 + bit, 0, channel);
            }
        } else if (report[0] & mask) {
            kbdProcKey(0xE0 + bit, 1, channel);
        }
        mask <<= 1;
    }
    for (i = 0; i < 6; i++) {
        key = ch->prevKeys[i];
        if (key != 0) {
            for (k = 0; k < 6; k++) {
                if (key == report[2 + k]) {
                    break;
                }
            }
            if (k == 6) {
                kbdProcKey(key, 0, channel);
                ch->prevKeys[i] = 0;
            }
        }
    }
    for (j = 0; j < 6; j++) {
        key = report[2 + j];
        if (key != 0) {
            for (k = 0; k < 6; k++) {
                if (key == ch->prevKeys[k]) {
                    break;
                }
            }
            if (k == 6) {
                kbdProcKey(key, 1, channel);
            }
        }
    }
    ch->prevModifiers = report[0];
    ch->prevKeys[0] = report[2];
    ch->prevKeys[1] = report[3];
    ch->prevKeys[2] = report[4];
    ch->prevKeys[3] = report[5];
    ch->prevKeys[4] = report[6];
    ch->prevKeys[5] = report[7];
}

/* Alarm handler: repeats the held key. */
void kbd_repeat_alarm(OSAlarm* alarm, OSContext* context)
{
    KBDChannel* ch = OSGetAlarmUserData(alarm);

    kbdProcKey(ch->repeatKey, 3, ch->index);
}

/* Maps a usage through the channel's layout and queues the resulting key event, arming the repeat alarm. */
void kbdProcKey(u8 key, s32 type, u8 channel)
{
    KBDEvent event;
    KBDChannel* ch = &kbd_state->channels[channel];
    KBDKeyMap* map;
    int i;
    u32 n;
    u16 code;
    s64 ticks;

    event.channel = channel;
    event.type = type;
    event.lockState = ch->lockState;
    map = &kbd_state->maps[ch->mapIndex];
    for (i = 0; i < map->remapCount; i++) {
        if (key == map->remap[i][0]) {
            key = map->remap[i][1];
            break;
        }
    }
    event.key = key;
    code = kbd_lookup_key(key, event.lockState, ch->mapIndex);
    event.code = code;
    if ((u16)(code + 0x1000) <= 8 || code == 0xF021) {
        kbd_update_lock_state(code, type, channel);
        if (!(code >= 0xF006 && code <= 0xF008) && code != 0xF021) {
            kbd_queue_event(&event);
            return;
        }
    }
    if (!(type & 1)) {
        if (key == ch->repeatKey) {
            OSCancelAlarm(&ch->repeatAlarm);
        }
        kbd_queue_event(&event);
        return;
    }
    OSCancelAlarm(&ch->repeatAlarm);
    for (n = 0; n < 7; n++) {
        if (code == kbd_no_repeat_codes[n]) {
            break;
        }
    }
    if (n == 7) {
        ch->repeatKey = key;
        if (ch->repeatDelay != 0) {
            if (type & 2) {
                ticks = ch->repeatInterval * (OS_TIMER_CLOCK / 1000);
            } else {
                ticks = ch->repeatDelay * (OS_TIMER_CLOCK / 1000);
            }
            OSSetAlarm(&ch->repeatAlarm, ticks, kbd_repeat_alarm);
        }
    }
    event.code = code;
    kbd_queue_event(&event);
}

/* Applies a lock-key code (num/caps/scroll-style toggles and layer switches) to the channel's lock state. */
void kbd_update_lock_state(u16 code, s32 type, u8 channel)
{
    KBDChannel* ch = &kbd_state->channels[channel];
    s32 step = -1;
    s32 down = type & 1;
    u32 locks;
    u32 value;
    u8 mapFlags;

    if (down) {
        step = 1;
    }
    mapFlags = kbd_state->maps[ch->mapIndex].lockMask;
    locks = kbd_state->channels[channel].lockState;
    switch (code) {
    case 0xF001:
        if (ch->toggleLocks) {
            if (down) {
                locks ^= 2;
            }
        } else {
            ch->lockCount[1] = ch->lockCount[1] + step;
            locks = (locks & ~2) | ((ch->lockCount[1] != 0) << 1);
        }
        break;
    case 0xF005:
        if (ch->toggleLocks) {
            if (down) {
                locks ^= 0x20;
            }
        } else {
            ch->lockCount[4] = ch->lockCount[4] + step;
            locks = (locks & ~0x20) | ((ch->lockCount[4] != 0) << 5);
        }
        break;
    case 0xF000:
        if (ch->toggleLocks) {
            if (down) {
                locks ^= 1;
            }
        } else {
            ch->lockCount[0] = ch->lockCount[0] + step;
            locks = (locks & ~1) | (ch->lockCount[0] != 0);
        }
        break;
    case 0xF008:
        if (down && ch->enabled) {
            locks ^= 0x200;
        }
        break;
    case 0xF007:
        if (down && ch->enabled) {
            locks ^= 0x100;
        }
        break;
    case 0xF006:
        if (down && ch->enabled) {
            switch ((s32)(locks & 0xC0)) {
            case 0x00:
                if ((mapFlags & 1) == 1) {
                    locks |= 0x40;
                }
                break;
            case 0x40:
                locks &= ~0x40;
                if ((mapFlags & 4) == 4) {
                    locks |= 0x80;
                }
                break;
            case 0x80:
                locks &= ~0x80;
                break;
            case 0xC0:
                locks &= ~0xC0;
                break;
            }
        }
        break;
    case 0xF002:
        if (ch->toggleLocks) {
            if (down) {
                locks ^= 4;
            }
        } else {
            ch->lockCount[2] = ch->lockCount[2] + step;
            locks = (locks & ~4) | ((ch->lockCount[2] != 0) << 2);
        }
        break;
    case 0xF003:
        if (ch->toggleLocks) {
            if (down) {
                locks ^= 8;
            }
        } else {
            ch->lockCount[3] = ch->lockCount[3] + step;
            locks = (locks & ~8) | ((ch->lockCount[3] != 0) << 3);
        }
        break;
    case 0xF004:
        if (ch->toggleLocks) {
            if (down) {
                locks ^= 0x10;
            }
        } else {
            ch->lockCount[5] = ch->lockCount[5] + step;
            locks = (locks & ~0x10) | ((ch->lockCount[5] != 0) << 4);
        }
        break;
    case 0xF021:
        if (down && ch->enabled) {
            locks ^= 0x400;
        }
        break;
    }
    value = locks;
    value |= 0x1000;
    kbd_store_lock_state(channel, value);
}

/* Appends an event to the channel's ring buffer and notifies the application; a full ring records an overflow marker. */
void kbd_queue_event(KBDEvent* event)
{
    KBDChannel* ch;
    u8 next;
    u8 channel;

    if (kbd_state->onKey != NULL) {
        kbd_state->onKey();
    }
    channel = event->channel;
    ch = &kbd_state->channels[channel];
    if (ch->head != ch->readIndex) {
        next = (ch->head + 1) % KBD_QUEUE;
        if (next == ch->readIndex) {
            ch->queue[ch->head].channel = channel;
            ch->queue[ch->head].key = 0xFF;
            ch->queue[ch->head].type = 0;
            ch->queue[ch->head].lockState = 0;
            ch->queue[ch->head].code = 0;
        } else {
            ch->queue[ch->head] = *event;
            if (ch->readIndex == KBD_QUEUE) {
                ch->readIndex = ch->head;
            }
        }
        ch->head = next;
    }
}

/* Completion of an LED report: records what the keyboard now shows, runs the caller's callback and recycles the request. */
/* untyped: caller-owned buffer */
void kbd_led_report_done(HIDDeviceInfo* info, s32 result, void* buffer, s32 length, u32 arg)
{
    KBDLedRequest* request = (KBDLedRequest*)arg;

    if (result == 0) {
        kbd_state->channels[request->channel].sentLeds = request->value;
    }
    if (request->callback != NULL) {
        request->callback(&request->channel, request->arg);
    }
    request->busy = 0;
    kbd_free_led(request);
}

/* 0x80529430 (0x11C): queues an LED report for the channel. */
/* untyped: opaque band object, typed by the callers' views */
u32 KBDSetLedsAsync(u32 channel, u32 value, void* callback, u32 arg)
{
    KBDLedRequest* request;
    s32 status;
    s32 result;

    if (kbd_state == NULL) {
        result = 2;
    } else if (channel >= KBD_CHANNELS) {
        result = 4;
    } else {
        s32 state = kbd_state->channels[channel].state;
        if (state == 1 || state == 4) {
            result = 5;
        } else {
            result = 0;
        }
    }
    if (result != 0) {
        return result;
    }
    request = kbd_alloc_led();
    if (request != NULL) {
        request->channel = channel;
        request->value = value;
        request->pending = 1;
        request->callback = (KBDLedCallback)callback;
        request->arg = arg;
        request->head.report = value;
        status = HIDSetReport(kbd_state->channels[channel].device, 2, 0, request, 1, kbd_led_report_done,
                              (u32)request);
        if (status != 0) {
            kbd_free_led(request);
        }
        if (status == 0) {
            return 0;
        }
        return 7;
    }
    return 7;
}

/* Fills in the key map of layout `index`: its tables, the shift/alt/ctrl columns, the key count and the remap pairs. */
void kbdInitMap(s32 index, u8 stride, u8 lockMask, const u16* keys, const u16* modifiers, u8 shiftColumn,
                u8 altColumn, u8 ctrlColumn, u8 keyCount, s32 layout)
{
    kbd_state->maps[index].stride = stride;
    kbd_state->maps[index].lockMask = lockMask;
    kbd_state->maps[index].keys = keys;
    kbd_state->maps[index].modifiers = modifiers;
    kbd_state->maps[index].shiftColumn = shiftColumn;
    kbd_state->maps[index].altColumn = altColumn;
    kbd_state->maps[index].ctrlColumn = ctrlColumn;
    kbd_state->maps[index].keyCount = keyCount;
    switch (layout) {
    case 0:
        kbd_state->maps[index].remap[0][0] = 0x32;
        kbd_state->maps[index].remap[0][1] = 0x31;
        kbd_state->maps[index].remapCount = 1;
        break;
    case 1:
        kbd_state->maps[index].remap[0][0] = 0x31;
        kbd_state->maps[index].remap[0][1] = 0x32;
        kbd_state->maps[index].remap[1][0] = 0x94;
        kbd_state->maps[index].remap[1][1] = 0x35;
        kbd_state->maps[index].remapCount = 2;
        break;
    case 2:
        kbd_state->maps[index].remap[0][0] = 0x31;
        kbd_state->maps[index].remap[0][1] = 0x32;
        kbd_state->maps[index].remapCount = 1;
        break;
    }
}

/* Installs the international layout as map 0. */
void kbdInitMapIntl(void)
{
    kbdInitMap(0, 5, 0x30, kbd_key_table_intl, kbd_modifier_table_intl, 0, 0, 3, 0x64, 0);
}

/* Shutdown hook: blanks the LEDs of every connected keyboard before the console powers down. */
BOOL kbd_shutdown(BOOL final, u32 event)
{
    u8 channel;
    s64 start;

    if (kbd_state == NULL) {
        return TRUE;
    }
    if (kbd_shutdown_guard++ <= 0x10 && !final) {
        for (channel = 0; channel < KBD_CHANNELS; channel++) {
            KBDChannel* ch = &kbd_state->channels[channel];

            if (!(ch->state & 1) && ch->sentLeds != 0) {
                KBDSetLedsAsync(channel, 0, NULL, 0);
                start = OSGetTime();
                do {
                } while ((OSGetTime() - start) * 8 / (OS_TIMER_CLOCK / 125000) < 1);
            }
        }
    }
    kbd_shutdown_guard = 0;
    return TRUE;
}

/* Brings the driver up in `memory`: registers the version, the callbacks, the channels and the HID client. */
s32 KBDInit(KBDState* memory, KBDChannelCallback onConnect, KBDDisconnectCallback onDisconnect,
            KBDKeyEventCallback onKey)
{
    u8 channel;

    if (kbd_state != NULL) {
        return 3;
    }
    OSRegisterVersion(__KBDVersion);
    memset(memory, 0, sizeof(KBDState));
    kbd_state = memory;
    memory->onConnect = onConnect;
    kbd_state->onDisconnect = onDisconnect;
    kbd_state->onKey = onKey;
    kbd_initialize_led_module();
    if (kbd_shutdown_info.func == NULL) {
        kbd_shutdown_info.func = kbd_shutdown;
        OSRegisterShutdownFunction(&kbd_shutdown_info);
    }
    for (channel = 0; channel < KBD_CHANNELS; channel++) {
        KBDChannel* ch = &kbd_state->channels[channel];

        ch->index = channel;
        ch->mapIndex = 17;
        ch->state = 1;
        OSCreateAlarm(&ch->unusedAlarm);
        OSSetAlarmUserData(&ch->unusedAlarm, ch);
        ch->device = NULL;
        ch->enabled = 1;
        ch->repeatDelay = 500;
        ch->repeatInterval = 33;
        ch->toggleLocks = 0;
        OSCreateAlarm(&ch->repeatAlarm);
        OSSetAlarmUserData(&ch->repeatAlarm, ch);
        KBDResetChannel(channel);
    }
    HIDRegisterClient(&kbd_state->client, kbd_client_attach);
    return 0;
}

/* Shuts the driver down: resets every channel, clears the callbacks and unregisters from HID. */
s32 KBDEnd(void)
{
    u8 channel;

    if (kbd_state == NULL) {
        return 2;
    }
    for (channel = 0; channel < KBD_CHANNELS; channel++) {
        KBDResetChannel(channel);
    }
    kbd_state->onConnect = NULL;
    kbd_state->onDisconnect = NULL;
    kbd_state->onKey = NULL;
    HIDUnregisterClient(&kbd_state->client, NULL, 0);
    kbd_state = NULL;
    return 0;
}

/* Replaces the connect callback and returns the previous one. */
KBDChannelCallback KBDSetConnectCallback(KBDChannelCallback callback)
{
    KBDChannelCallback previous = kbd_state->onConnect;

    kbd_state->onConnect = callback;
    return previous;
}

/* Replaces the disconnect callback and returns the previous one. */
KBDDisconnectCallback KBDSetDisconnectCallback(KBDDisconnectCallback callback)
{
    KBDDisconnectCallback previous = kbd_state->onDisconnect;

    kbd_state->onDisconnect = callback;
    return previous;
}

/* Replaces the key-event callback and returns the previous one. */
KBDKeyEventCallback KBDSetKeyCallback(KBDKeyEventCallback callback)
{
    KBDKeyEventCallback previous = kbd_state->onKey;

    kbd_state->onKey = callback;
    return previous;
}

/* Clears the channel's key state, queue indices, lock counters and repeat alarm. */
s32 KBDResetChannel(u8 channel)
{
    KBDChannel* ch = &kbd_state->channels[channel];

    ch->prevModifiers = 0;
    ch->prevKeys[0] = 0;
    ch->prevKeys[1] = 0;
    ch->prevKeys[2] = 0;
    ch->prevKeys[3] = 0;
    ch->prevKeys[4] = 0;
    ch->prevKeys[5] = 0;
    ch->readIndex = KBD_QUEUE;
    kbd_state->channels[channel].lockState = 0;
    ch->sentLeds = 0x10;
    ch->pendingLeds = 0x10;
    ch->lockCount[0] = 0;
    ch->lockCount[1] = 0;
    ch->lockCount[2] = 0;
    ch->lockCount[3] = 0;
    ch->lockCount[4] = 0;
    ch->lockCount[5] = 0;
    ch->repeatKey = 0;
    OSCancelAlarm(&ch->repeatAlarm);
    return 0;
}

/* Selects the key map of a channel and resets it; 4 when the layout does not exist. */
s32 KBDSetChannelMap(u8 channel, s32 mapIndex)
{
    KBDChannel* ch;

    if (mapIndex >= KBD_MAPS) {
        return 4;
    }
    if (kbd_state->maps[mapIndex].stride == 0) {
        return 4;
    }
    ch = &kbd_state->channels[channel];
    ch->mapIndex = mapIndex;
    KBDResetChannel(channel);
    return 0;
}

/* 0x80529B50 (0x18): stores the channel's enable word. */
s32 KBDSetChannelValue(u8 channel, u32 value)
{
    kbd_state->channels[channel].enabled = value;
    return 0;
}

/* Sets the lock-state word of a channel from the caller's side; returns 0. */
s32 KBDSetChannelLeds(u8 channel, u32 value)
{
    kbd_store_lock_state(channel, value);
    return 0;
}

/* Returns the key code a usage produces under the given lock state, 0 for none and 0xFFFF for an invalid request. */
u16 kbd_lookup_key(u8 usage, s32 lockState, s32 mapIndex)
{
    KBDKeyMap* map;
    const u16* table;
    u8 row;
    int base;
    u16 entry;
    int kind;
    int column;
    int shift;
    int variant;
    u16 mask;
    u8 stride;

    if (mapIndex >= KBD_MAPS) {
        return 0xFFFF;
    }
    map = &kbd_state->maps[mapIndex];
    stride = map->stride;
    if (stride == 0) {
        return 0;
    }
    if (usage < 4) {
        return 0xFFFF;
    }
    if (usage < map->keyCount + 4) {
        table = map->keys;
        row = usage - 4;
    } else if (usage >= 0xE0) {
        if (usage > 0xE7) {
            return 0;
        }
        table = map->modifiers;
        row = usage - 0xE0;
    } else {
        return 0;
    }
    base = row * stride;
    entry = table[base];
    kind = entry & 0xC000;
    if (kind == 0xC000) {
        variant = 1;
        column = 0;
    } else {
        if ((lockState & 0x20) || (lockState & 5) == 5) {
            variant = map->ctrlColumn;
            shift = 0x40;
        } else if (lockState & 0x40) {
            variant = map->shiftColumn;
            shift = 4;
        } else if (lockState & 0x80) {
            variant = map->altColumn;
            shift = 0x10;
        } else {
            variant = 1;
            shift = 1;
        }
        if (!(lockState & 0x800) && (lockState & 0x1D)) {
            column = 0;
        } else {
            column = (lockState >> 1) & 1;
            mask = shift << column;
            if (kind == 0 && (entry & mask)) {
                column ^= (lockState >> 9) & 1;
            } else if (kind == 0x8000 && (entry & mask)) {
                int toggled = 0;

                if ((lockState & 0x100) == 0x100 && column == 0) {
                    toggled = 1;
                }
                column = toggled;
            } else if (kind == 0x4000 && variant == 1 && (lockState & 0x200)) {
                variant = map->shiftColumn;
            }
        }
        if (column == 0) {
            if (variant == map->ctrlColumn && !(map->lockMask & 0x10)) {
                return 0;
            }
            if (variant == map->shiftColumn && !(map->lockMask & 1)) {
                return 0;
            }
            if (variant == map->altColumn && !(map->lockMask & 4)) {
                return 0;
            }
        } else {
            if (variant == map->ctrlColumn && !(map->lockMask & 0x20)) {
                return 0;
            }
            if (variant == map->shiftColumn && !(map->lockMask & 2)) {
                return 0;
            }
            if (variant == map->altColumn && !(map->lockMask & 8)) {
                return 0;
            }
        }
    }
    return table[base + column + variant];
}

/* Pushes an LED request record back onto the free list. */
void kbd_free_led(KBDLedRequest* request)
{
    BOOL enabled = OSDisableInterrupts();

    request->head.next = kbd_led_free_list.head;
    kbd_led_free_list.head = request;
    OSRestoreInterrupts(enabled);
}

/* Pops an LED request record off the free list, or returns null. */
KBDLedRequest* kbd_alloc_led(void)
{
    BOOL enabled = OSDisableInterrupts();
    KBDLedRequest* request = kbd_led_free_list.head;

    if (request != NULL) {
        kbd_led_free_list.head = request->head.next;
    }
    OSRestoreInterrupts(enabled);
    return request;
}

/* Builds the free list of LED request records. */
void kbd_initialize_led_module(void)
{
    int i;

    kbd_led_free_list.head = NULL;
    for (i = 0; i < 16; i++) {
        kbd_free_led(&kbd_led_request_pool[i]);
    }
}

/* Interrupt-read completion: hands a good report to the parser and issues the next read. */
/* untyped: caller-owned buffer */
void kbd_read_done(HIDDeviceInfo* info, s32 result, void* buffer, s32 length, u32 arg)
{
    if (result == 0) {
        kbd_process_report(info, buffer);
        HIDRead(info, kbd_report_buffers[arg], 8, kbd_read_done, arg);
    }
}

/* SET_PROTOCOL completion: starts reading reports. */
/* untyped: caller-owned buffer */
void kbd_set_protocol_done(HIDDeviceInfo* info, s32 result, void* buffer, s32 length, u32 arg)
{
    HIDRead(info, kbd_report_buffers[arg], 8, kbd_read_done, arg);
}

/* SET_IDLE completion: selects the boot protocol. */
/* untyped: caller-owned buffer */
void kbd_set_idle_done(HIDDeviceInfo* info, s32 result, void* buffer, s32 length, u32 arg)
{
    HIDSetProtocol(info, 0, kbd_set_protocol_done, arg);
}

/* Starts the setup sequence of a newly attached keyboard: idle rate, then protocol, then reports. */
void kbd_init_keyboard(u32 channel, HIDDeviceInfo* info)
{
    HIDSetIdle(info, 0, 0, kbd_set_idle_done, channel);
}
