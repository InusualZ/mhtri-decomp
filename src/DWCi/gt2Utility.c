/*
 * DWCi/gt2Utility.c - the GameSpy GT2 utility helpers: the byte-order wrapper and the address text helpers
 *   (gt2AddressToString / gt2StringToAddress), with the stats server host name.
 *
 * RANGE. `.text` 0x80512200..0x80512490 (3 functions); `.data` 0x806308A8..0x806308E8 (the stats host buffer);
 *   `.bss` 0x807625C0..0x807625F0 (the two-string address ring); `.sdata` 0x80794358..0x80794368 (the three address
 *   formats); `.sbss` 0x80795820..0x80795828 (the ring index).
 *   `DWCi_htons` opens this file: GameSpy declares the byte-order helpers ahead of the address helpers, the GT2
 *   files link in name order, and nothing in the bytes places it in gt2Socket instead (both cuts link the same).
 * FLAGS. `cflags_base` (-func_align 16): the retail functions start 16-byte aligned with zero words between them,
 *   which the lib's `cflags_dwc` (-func_align 4) packs away - no row moves, the object layout does.
 * NAMES. GUESSes from the bodies (`DWCi_htons` = gt2HostToNetworkShort, `DWCi_formatAddress` = gt2AddressToString,
 *   `DWCi_parseAddress` = gt2StringToAddress); GUESS on `DWCi_addressRingIndex` and `DWCi_statsServerHostname`. The
 *   file name is the GameSpy SDK's `gt2Utility.c` (no `__FILE__` string).
 * RESIDUALS. `DWCi_parseAddress` keeps the port in r26 where retail shares r27 with the colon pointer (no
 *   declaration order or `int` spelling of the atoi result moved it). `DWCi_statsServerHostname` is the GameSpy stats buffer (gstats); its
 *   `.data` word sits between gt2's and natneg's, and its `.bss` partner (`DWCi_gameName`, 0x807625F0) follows the
 *   address ring, so it may be a third, text-less TU - it is placed here until that is decided.
 *   Flip blocker: the target names the three `.sdata` formats (`DWCi_addressFormat*`) where ours emits anonymous
 *   literals (same bytes).
 * SHAPES. `DWCi_formatAddress` writes through a separate output pointer picked by an if/else (retail lays the
 *   caller's-buffer arm first); `DWCi_parseAddress` initialises the address at its declaration and tests each port
 *   digit with an MSL-style `isdigit` (an out-of-range helper whose result is materialised, then the class table).
 */

#include "types.h"
#include "DWCi/gt2Utility.h"             /* this unit's own exported entry points */
#include "Runtime.PPCEABI.H/memcpy.h"
#include "Runtime.PPCEABI.H/memset.h"
#include "unsplit/DWCi.h"                /* the band's unowned data and helpers (rule 2) */
#include "MSL_C/alloc.h"
#include "unsplit/SO.h"                  /* SOAddressToString / SOAddressToHostPort / SOHtoNs */
#include "SO/soi.h"
#include "unsplit/Runtime.PPCEABI.H.h"   /* sprintf / strchr / strlen / atoi */

/* The GameSpy stats server host name; `DWC_Init` rewrites it for the server type. */
char DWCi_statsServerHostname[64] = "gamestats.gs.nintendowifi.net";

/* The two address strings `DWCi_formatAddress` alternates between when the caller passes no buffer. */
static char DWCi_addressRing[2][0x16];

/* Which of the two ring buffers `DWCi_formatAddress` writes next. */
static u32 DWCi_addressRingIndex;

/* The active locale's character tables (MSL's ctype tables): +0x08 points at the per-character u16 class flags, bit 3
 * marking a decimal digit. */
typedef struct DWCiCharClass {
    /* +0x00 */ u8 pad_0x00[0x8];
    /* +0x08 */ const u16* bits;
} DWCiCharClass; /* size: 0xC */

/* The locale record `_current_locale` is: +0x38 points at its character tables. */
typedef struct DWCiCType {
    /* +0x00 */ u8 pad_0x00[0x38];
    /* +0x38 */ DWCiCharClass* cls;
    /* +0x3C */ u8 pad_0x3c[0xC];
} DWCiCType; /* size: 0x48 */

/* The unit's own entry points, in address order. */
u16 DWCi_htons(u16 port);
char* DWCi_formatAddress(u32 addr, u16 port, char* buf);
u32 DWCi_parseAddress(char* str, u32* outAddr, u16* outPort);

/* 0x80512200 - the host-to-network short wrapper the transport clients call. */
u16 DWCi_htons(u16 port)
{
    return SOAddressToHostPort(port);
}

/* 0x80512210 - format an address for the log/text layer: "addr:port", "addr", ":port" or the empty
 * string, into the caller's buffer or into the two-buffer ring when the caller passes none. */
char* DWCi_formatAddress(u32 addr, u16 port, char* buf)
{
    char* out;

    if (buf != 0) {
        out = buf;
    } else {
        DWCi_addressRingIndex ^= 1;
        out = DWCi_addressRing[DWCi_addressRingIndex];
    }
    if (addr != 0) {
        u32 withPort;
        u32 withoutPort;

        if (port != 0) {
            withPort = addr;
            sprintf(out, "%s:%d", SOAddressToString(&withPort), port);
        } else {
            withoutPort = addr;
            sprintf(out, "%s", SOAddressToString(&withoutPort));
        }
    } else {
        if (port != 0) {
            sprintf(out, ":%d", port);
        } else {
            *out = 0;
        }
    }
    return out;
}

/* Whether a character lies outside the locale's 256-entry class table. */
static inline int DWCi_outOfRange(int c)
{
    return c < 0 || c > 255;
}

/* Whether a character is a decimal digit in the active locale's class table. */
static inline int DWCi_isDigit(int c)
{
    return DWCi_outOfRange(c) ? 0 : (_current_locale.cls->bits[c] & 8);
}

/* 0x80512300 - parse "host:port": split at the colon, validate the port digits against the band's
 * character-class table, convert with atoi, resolve the host name and hand both halves back. */
u32 DWCi_parseAddress(char* str, u32* outAddr, u16* outPort)
{
    char buf[0x108];
    char* colon;
    char* p;
    u32 addr = 0;
    u16 port;
    u32 value;

    if (str == 0 || *str == 0) {
        addr = 0;
        port = 0;
    } else {
        colon = strchr(str, ':');
        if (colon == 0) {
            port = 0;
        } else {
            if (colon == str) {
                str = 0;
                addr = 0;
            } else {
                memcpy(buf, str, colon - str);
                buf[colon - str] = 0;
                str = buf;
            }
            p = colon + 1;
            while (*p != 0) {
                if (!DWCi_isDigit(*p)) {
                    return 0;
                }
                p++;
            }
            value = atoi(colon + 1);
            if (value > 0xFFFF) {
                return 0;
            }
            port = (u16)value;
        }
        if (str != 0) {
            addr = DWCi_socketResolveAddress(str);
            if (addr == (u32)-1) {
                DWCiHostEntry* entry = DWCi_socketLookupHost(str);

                if (entry == 0) {
                    return 0;
                }
                addr = (*entry->hosts)->addr;
            }
        }
    }
    if (outAddr != 0) {
        *outAddr = addr;
    }
    if (outPort != 0) {
        *outPort = port;
    }
    return 1;
}
