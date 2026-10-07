/*
 * KPR/kpr.c - the KPR library: the keyboard character queue and its character conversion lookups.
 *
 * RANGE. .text 0x80526C90-0x805272B0 (9 functions, 0x620 B); .rodata 0x80579088-0x80579450; .data
 *    0x806492D8-0x80649348; .sdata 0x80794498-0x807944A0; .sbss 0x807959F0-0x80795A00.  Cut from the old VF block
 *    between `PMIC/pmic.c` (0x80526C90) and `HID/hid.c` (0x805272B0).
 * FLAGS. the `OS` lib's group with `cflags_base` (configure.py `cflags=`): every function start of the band is
 *    16-aligned (`-O4,p` default alignment), not the 4-byte packing `cflags_os` gives.
 * NAMES. `KPRLookAhead` is a GUESS (the old VF unit's); the library name `KPR` is the build string's and the file
 *    name `kpr.c` a GUESS; the other eight rows are GUESSES from what they do: `KPRInitQueue`, `KPRClearQueue`,
 *    `KPRSetQueueMode`, `KPRPutChar` (the overflow message names it), `KPRGetChar`, `kpr_compose_char` (Alt-code
 *    entry), `kpr_convert_pending` (dead-key composition) and `kpr_install_converter`; the tables are
 *    `kpr_compose_table` (dead key, base, result triples), `kpr_cp1252_table` and `kpr_cp437_table` (the Alt-code
 *    pages for 0x80-0x9F and 0x80-0xFF).
 * EVIDENCE. `.data` 0x806492D8 is the build string `<< RVL_SDK - KPR ... >>` read through `.sdata` 0x80794498 by
 *    `KPRInitQueue`, followed by the `kpr_lib.c` / `KPRPutChar: Overflow` panic strings; `.rodata` 0x80579088
 *    (0x288 B) and 0x80579310/0x80579350 are the tables of `kpr_convert_pending` and `kpr_compose_char`; `.sbss`
 *    0x807959F0..0x80795A00 is read here only; `KPRLookAhead` is called by `homebutton/hbm_text_panel.cpp`.
 * RESIDUALS. all 9 rows have a body (4 at 100 %); `KPRGetChar` 95 % and `KPRLookAhead` 96 % (register numbering of the
 *    copy loops), `KPRPutChar` 98.5 %, `kpr_convert_pending` 99.5 % and `kpr_compose_char` 99.8 % (register
 *    numbering); flipcheck blockers: the .text differences above and trailing .data/.sbss/.sdata padding.
 * SHAPES. `kpr_convert_pending` leaves the first dead-key index uninitialised on purpose (the target reads an unset
 *    register on the no-match path); `.sbss` objects are defined in reverse address order.
 */
#include "types.h"
#include "KPR/kpr.h"
#include "OS/OS.h"
#include "OS/OSError.h"
#include "OS/OSInterrupt.h"

const char* __KPRVersion = "<< RVL_SDK - KPR \trelease build: Feb 27 2009 10:06:00 (0x4302_145) >>";

const u16 kpr_compose_table[108][3] = {
    {0x0300, 0x0300, 0x0060},
    {0x0300, 0x0041, 0x00C0},
    {0x0300, 0x0045, 0x00C8},
    {0x0300, 0x0049, 0x00CC},
    {0x0300, 0x004F, 0x00D2},
    {0x0300, 0x0055, 0x00D9},
    {0x0300, 0x0061, 0x00E0},
    {0x0300, 0x0065, 0x00E8},
    {0x0300, 0x0069, 0x00EC},
    {0x0300, 0x006F, 0x00F2},
    {0x0300, 0x0075, 0x00F9},
    {0x0301, 0x0301, 0x00B4},
    {0x0301, 0x0041, 0x00C1},
    {0x0301, 0x0045, 0x00C9},
    {0x0301, 0x0049, 0x00CD},
    {0x0301, 0x004F, 0x00D3},
    {0x0301, 0x0055, 0x00DA},
    {0x0301, 0x0059, 0x00DD},
    {0x0301, 0x0061, 0x00E1},
    {0x0301, 0x0065, 0x00E9},
    {0x0301, 0x0069, 0x00ED},
    {0x0301, 0x006F, 0x00F3},
    {0x0301, 0x0075, 0x00FA},
    {0x0301, 0x0079, 0x00FD},
    {0x0302, 0x0302, 0x005E},
    {0x0302, 0x0041, 0x00C2},
    {0x0302, 0x0045, 0x00CA},
    {0x0302, 0x0049, 0x00CE},
    {0x0302, 0x004F, 0x00D4},
    {0x0302, 0x0055, 0x00DB},
    {0x0302, 0x0061, 0x00E2},
    {0x0302, 0x0065, 0x00EA},
    {0x0302, 0x0069, 0x00EE},
    {0x0302, 0x006F, 0x00F4},
    {0x0302, 0x0075, 0x00FB},
    {0x0303, 0x0303, 0x007E},
    {0x0303, 0x0041, 0x00C3},
    {0x0303, 0x004E, 0x00D1},
    {0x0303, 0x004F, 0x00D5},
    {0x0303, 0x0061, 0x00E3},
    {0x0303, 0x006E, 0x00F1},
    {0x0303, 0x006F, 0x00F5},
    {0x0308, 0x0308, 0x00A8},
    {0x0308, 0x0041, 0x00C4},
    {0x0308, 0x0045, 0x00CB},
    {0x0308, 0x0049, 0x00CF},
    {0x0308, 0x004F, 0x00D6},
    {0x0308, 0x0055, 0x00DC},
    {0x0308, 0x0059, 0x0178},
    {0x0308, 0x0061, 0x00E4},
    {0x0308, 0x0065, 0x00EB},
    {0x0308, 0x0069, 0x00EF},
    {0x0308, 0x006F, 0x00F6},
    {0x0308, 0x0075, 0x00FC},
    {0x0308, 0x0079, 0x00FF},
    {0x0327, 0x0327, 0x00B8},
    {0x0327, 0x0043, 0x00C7},
    {0x0327, 0x0063, 0x00E7},
    {0x030D, 0x030D, 0x0027},
    {0x030D, 0x0041, 0x00C1},
    {0x030D, 0x0045, 0x00C9},
    {0x030D, 0x0049, 0x00CD},
    {0x030D, 0x004F, 0x00D3},
    {0x030D, 0x0055, 0x00DA},
    {0x030D, 0x0059, 0x00DD},
    {0x030D, 0x0043, 0x00C7},
    {0x030D, 0x0061, 0x00E1},
    {0x030D, 0x0065, 0x00E9},
    {0x030D, 0x0069, 0x00ED},
    {0x030D, 0x006F, 0x00F3},
    {0x030D, 0x0075, 0x00FA},
    {0x030D, 0x0079, 0x00FD},
    {0x030D, 0x0063, 0x00E7},
    {0x030E, 0x030E, 0x0022},
    {0x030E, 0x0041, 0x00C4},
    {0x030E, 0x0045, 0x00CB},
    {0x030E, 0x0049, 0x00CF},
    {0x030E, 0x004F, 0x00D6},
    {0x030E, 0x0055, 0x00DC},
    {0x030E, 0x0059, 0x0178},
    {0x030E, 0x0061, 0x00E4},
    {0x030E, 0x0065, 0x00EB},
    {0x030E, 0x0069, 0x00EF},
    {0x030E, 0x006F, 0x00F6},
    {0x030E, 0x0075, 0x00FC},
    {0x030E, 0x0079, 0x00FF},
    {0x0344, 0x0344, 0x0385},
    {0x0344, 0x03B9, 0x0390},
    {0x0344, 0x03C5, 0x03B0},
    {0x0308, 0x0399, 0x03AA},
    {0x0308, 0x03A5, 0x03AB},
    {0x0308, 0x03B9, 0x03CA},
    {0x0308, 0x03C5, 0x03CB},
    {0x0301, 0x0391, 0x0386},
    {0x0301, 0x0395, 0x0388},
    {0x0301, 0x0397, 0x0389},
    {0x0301, 0x0399, 0x038A},
    {0x0301, 0x039F, 0x038C},
    {0x0301, 0x03A5, 0x038E},
    {0x0301, 0x03A9, 0x038F},
    {0x0301, 0x03B1, 0x03AC},
    {0x0301, 0x03B5, 0x03AD},
    {0x0301, 0x03B7, 0x03AE},
    {0x0301, 0x03B9, 0x03AF},
    {0x0301, 0x03BF, 0x03CC},
    {0x0301, 0x03C5, 0x03CD},
    {0x0301, 0x03C9, 0x03CE},
    {0x0000, 0x0000, 0x0000},
};

const u16 kpr_cp1252_table[32] = {
    0x20AC, 0x0000, 0x201A, 0x0192, 0x201E, 0x2026, 0x2020, 0x2021,
    0x02C6, 0x2030, 0x0160, 0x2039, 0x0152, 0x0000, 0x017D, 0x0000,
    0x0000, 0x2018, 0x2019, 0x201C, 0x201D, 0x2022, 0x2013, 0x2014,
    0x02DC, 0x2122, 0x0161, 0x203A, 0x0153, 0x0000, 0x017E, 0x0178,
};

const u16 kpr_cp437_table[128] = {
    0x00C7, 0x00FC, 0x00E9, 0x00E2, 0x00E4, 0x00E0, 0x00E5, 0x00E7,
    0x00EA, 0x00EB, 0x00E8, 0x00EF, 0x00EE, 0x00EC, 0x00C4, 0x00C5,
    0x00C9, 0x00E6, 0x00C6, 0x00F4, 0x00F6, 0x00F2, 0x00FB, 0x00F9,
    0x00FF, 0x00D6, 0x00DC, 0x00A2, 0x00A3, 0x00A5, 0x20A7, 0x0192,
    0x00E1, 0x00ED, 0x00F3, 0x00FA, 0x00F1, 0x00D1, 0x00AA, 0x00BA,
    0x00BF, 0x2310, 0x00AC, 0x00BD, 0x00BC, 0x00A1, 0x00AB, 0x00BB,
    0x2591, 0x2592, 0x2593, 0x2502, 0x2524, 0x2561, 0x2562, 0x2556,
    0x2555, 0x2563, 0x2551, 0x2557, 0x255D, 0x255C, 0x255B, 0x2510,
    0x2514, 0x2534, 0x252C, 0x251C, 0x2500, 0x253C, 0x255E, 0x255F,
    0x255A, 0x2554, 0x2569, 0x2566, 0x2560, 0x2550, 0x256C, 0x2567,
    0x2568, 0x2564, 0x2565, 0x2559, 0x2558, 0x2552, 0x2553, 0x256B,
    0x256A, 0x2518, 0x250C, 0x2588, 0x2584, 0x258C, 0x2590, 0x2580,
    0x03B1, 0x00DF, 0x0393, 0x03C0, 0x03A3, 0x03C3, 0x00B5, 0x03C4,
    0x03A6, 0x0398, 0x03A9, 0x03B4, 0x221E, 0x03C6, 0x03B5, 0x2229,
    0x2261, 0x00B1, 0x2265, 0x2264, 0x2320, 0x2321, 0x00F7, 0x2248,
    0x00B0, 0x2219, 0x00B7, 0x221A, 0x207F, 0x00B2, 0x25A0, 0x00A0,
};


typedef void (*KPRHook)(KPRQueue* queue);

u8 kpr_version_registered;
KPRHook kpr_extra_hook;
KPRHook kpr_convert_hook;

BOOL kpr_compose_char(KPRQueue* queue, u16 ch);
void kpr_convert_pending(KPRQueue* queue);

/* Installs the dead-key converter as the compose hook. */
void kpr_install_converter(void)
{
    kpr_convert_hook = kpr_convert_pending;
}

/* Registers the library version once and resets the queue to Alt-code mode. */
void KPRInitQueue(KPRQueue* queue)
{
    if (kpr_version_registered == 0) {
        OSRegisterVersion(__KPRVersion);
        kpr_version_registered = 1;
    }
    queue->mode = 1;
    queue->ready = 0;
    queue->pending = 0;
    queue->altCode = 0;
}

/* Empties the queue. */
void KPRClearQueue(KPRQueue* queue)
{
    queue->ready = 0;
    queue->pending = 0;
    queue->altCode = 0;
}

/* Selects the queue mode and empties it. */
void KPRSetQueueMode(KPRQueue* queue, u32 mode)
{
    queue->mode = mode;
    queue->ready = 0;
    queue->pending = 0;
    queue->altCode = 0;
}

/* Appends a character, composing it first when the mode asks for it; returns the number of complete characters. */
u8 KPRPutChar(KPRQueue* queue, u16 ch)
{
    u32 mode;
    BOOL enabled;
    u8 count;

    if (queue->ready + queue->pending + 1 >= 5) {
        OSPanic("kpr_lib.c", 0xD7, "KPRPutChar: Overflow");
    }
    enabled = OSDisableInterrupts();
    if (!(queue->mode & 1) || kpr_compose_char(queue, ch) == 0) {
        queue->chars[queue->ready + queue->pending] = ch;
        mode = queue->mode;
        count = queue->pending + 1;
        queue->pending = count;
        if (mode & 2) {
            kpr_convert_hook(queue);
        } else if ((mode & 8) || (mode & 4)) {
            kpr_extra_hook(queue);
        } else {
            queue->ready = count;
            queue->pending = 0;
        }
        if (ch == 0xFFFF) {
            queue->ready--;
        }
    }
    OSRestoreInterrupts(enabled);
    return queue->ready;
}

/* Removes and returns the oldest complete character, or 0 when none is ready. */
u16 KPRGetChar(KPRQueue* queue)
{
    u16 ch;
    u32 i;

    if (queue->ready == 0) {
        return 0;
    }
    OSDisableInterrupts();
    ch = queue->chars[0];
    for (i = 1; i < queue->pending + queue->ready; i++) {
        queue->chars[i - 1] = queue->chars[i];
    }
    queue->ready--;
    OSRestoreInterrupts(TRUE);
    return ch;
}

/* 0x80526F00 (0xD0): copies the queued characters to `outAddress` and returns the queue's length. */
/* untyped: opaque band object, typed by the callers' views */
u32 KPRLookAhead(void* queueAddress, u32 outAddress, u32 max)
{
    KPRQueue* queue = queueAddress;
    u16* out = (u16*)outAddress;
    u8 i;

    if (out == NULL || max == 0) {
        return (u8)(queue->pending + queue->ready);
    }
    OSDisableInterrupts();
    for (i = 0; i < queue->pending + queue->ready && i < max; i++) {
        out[i] = queue->chars[i];
    }
    if (i < max) {
        out[i] = 0;
    }
    OSRestoreInterrupts(TRUE);
    return (u8)(queue->pending + queue->ready);
}

/* Alt-code entry: collects the digits typed with the Alt key and, on release, queues the character they name. */
BOOL kpr_compose_char(KPRQueue* queue, u16 ch)
{
    u32 state = queue->altCode;
    u32 page;
    u32 value;
    u32 c;
    s32 i;

    if (state != 0) {
        value = state & 0x7FFFFFFF;
        queue->altCode = value;
        page = state & 0x80000000;
        if ((u16)(ch + 0xED0) <= 9) {
            if (value > 0x6666666) {
                return TRUE;
            }
            queue->altCode = page | (ch - 0xF130 + value * 10);
            return TRUE;
        }
        if (value - 0x80 <= 0x7F) {
            if (page != 0) {
                if (value < 0xA0) {
                    value = kpr_cp1252_table[value - 0x80];
                }
            } else {
                value = kpr_cp437_table[value - 0x80];
            }
        } else if (value > 0xFF) {
            c = 0x20;
            if (value <= 0x6666666) {
                c = (u8)value;
            }
            value = c;
        }
        for (i = queue->ready + queue->pending; i > queue->ready; i--) {
            queue->chars[i] = queue->chars[i - 1];
        }
        queue->chars[queue->ready] = value;
        queue->altCode = 0;
        queue->ready++;
        return ch == 0;
    }
    if ((u16)(ch + 0xED0) <= 9) {
        if (ch == 0xF130) {
            queue->altCode = 0x80000000;
        } else {
            queue->altCode = ch - 0xF130;
        }
        return TRUE;
    }
    return ch == 0;
}

/* Composes the pending dead key and base character through the table, or completes a lone pending character. */
void kpr_convert_pending(KPRQueue* queue)
{
    u32 i;
    u16 a;
    u32 firstDead;
    u32 secondDead;
    u16 b;

    if (queue->pending == 1) {
        for (i = 0; i < 107; i++) {
            if (queue->chars[queue->ready] == kpr_compose_table[i][0]) {
                return;
            }
        }
        queue->ready++;
        queue->pending = 0;
        return;
    }
    secondDead = 107;
    for (i = 0; i < 107; i++) {
        a = queue->chars[queue->ready];
        if (a == kpr_compose_table[i][0] && queue->chars[queue->ready + 1] == kpr_compose_table[i][1]) {
            queue->chars[queue->ready] = kpr_compose_table[i][2];
            queue->pending = 0;
            queue->ready++;
            return;
        }
        if (a == kpr_compose_table[i][0] && a == kpr_compose_table[i][1]) {
            firstDead = i;
        }
        b = queue->chars[queue->ready + 1];
        if (b == kpr_compose_table[i][0] && b == kpr_compose_table[i][1]) {
            secondDead = i;
        }
    }
    queue->chars[queue->ready] = kpr_compose_table[firstDead][2];
    if (secondDead < 107) {
        queue->chars[queue->ready + 1] = kpr_compose_table[secondDead][2];
    }
    queue->pending = 0;
    queue->ready += 2;
}
