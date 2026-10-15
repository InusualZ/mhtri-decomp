/*
 * DWCi/gsAvailable.c - the GameSpy availability probe: sends the "is the game's backend up" query for a game name to
 *   "<game>.available.gs.nintendowifi.net" and polls for the answer.
 *
 * RANGE. `.text` 0x8050C5F0..0x8050C950 (2 functions); `.data` 0x80630858..0x80630880 (the host format); `.bss`
 *   0x807614D8..0x807615C0 (game name, host override, probe state); `.sdata` 0x80794340..0x80794348 (the answer
 *   tag); `.sbss` 0x80795818..0x80795820 (the result). The left edge follows the socket layer's "localhost" string
 *   (ends 0x80630852, next object on the 8-byte boundary 0x80630858) and `gsiSocketError`'s padded `.sbss` word; the
 *   right edge is gt2Auth's secret key (`DWCi/gt2Auth.c`).
 * FLAGS. `cflags_base` (-func_align 16): the retail functions start 16-byte aligned.
 * NAMES. GUESS `DWCi_natProbeStart` (GSIStartAvailableCheck), GUESS `DWCi_natProbePoll` (GSIAvailableCheckThink),
 *   GUESS `DWCi_availableGameName`, `DWCi_availableHostName`, `DWCi_availableCheck`, `DWCi_natProbeStatus`. The file
 *   name is the GameSpy SDK's `gsAvailable.c` (no `__FILE__` string).
 * RESIDUALS. `DWCi_natProbePoll` stores each result from r0 and loads it into r3 again where retail stores r3 (the
 *   assign-and-return, macro and typed spellings of the result helper all do), and assembles the answer flags
 *   with one `rlwimi` fewer. `DWCi_natProbeStart` addresses the three `.bss` objects from the section symbol where
 *   the target's relocation names `DWCi_availableGameName` (same address).
 * SHAPES. The address resolution is a static inline (retail expands it in `DWCi_natProbeStart`); the host override
 *   test is read once into an `int` (retail sign-extends it once and re-tests the register). The two name
 *   buffers are zero-initialised definitions: uninitialised, MWCC orders the `.bss` by first use (game name, probe
 *   state, host override) where retail has game name, host override, probe state.
 */
#include "DWCi/gsAvailable.h"
#include "DWCi/dwc_nasfunc.h"
#include "Runtime.PPCEABI.H/memcpy.h"
#include "unsplit/Runtime.PPCEABI.H.h"
#include "MSL_C/alloc.h"

/* The probe's game name and host override (zero-initialised: MWCC lays them out ahead of the probe state, as
 * retail does), and the probe state. */
char DWCi_availableGameName[0x40] = {0};
char DWCi_availableHostName[0x40] = {0};
DWCiAvailableCheck DWCi_availableCheck;

/* The probe result. */
s32 DWCi_natProbeStatus;

/* Resolves `host` (a dotted address or a name) into `addr` with `port`; 0 when the name cannot be resolved. */
static inline int DWCi_getSockAddrIn(char* host, u16 port, SOSockAddrIn* addr) {
    DWCiHostEntry* entry;
    addr->family = 2;
    addr->port = SOHtoNs(port);
    addr->addr = DWCi_socketResolveAddress(host);
    if (addr->addr == (u32)-1) {
        entry = DWCi_socketLookupHost(host);
        if (entry == NULL) {
            return 0;
        }
        addr->addr = entry->hosts[0]->addr;
    }
    return 1;
}

/* 0x8050C5F0 (0x17C): opens the probe socket and sends the availability query for `gameName` to
 * "<gameName>.available.gs.nintendowifi.net" (or the override host) port 27900. */
void DWCi_natProbeStart(const char* gameName) {
    char hostName[0x40];
    int overrideHost;
    int length;

    strcpy(DWCi_availableGameName, gameName);
    DWCi_availableCheck.sock = -1;
    SocketStartUp();
    overrideHost = DWCi_availableHostName[0];
    if (overrideHost == 0) {
        sprintf(hostName, "%s.available.gs.nintendowifi.net", gameName);
    }
    if (DWCi_getSockAddrIn(overrideHost != 0 ? DWCi_availableHostName : hostName, 27900,
                           &DWCi_availableCheck.address)) {
        DWCi_availableCheck.sock = DWCi_socketCreate(2, 2, 17);
        if (DWCi_availableCheck.sock != -1) {
            DWCi_availableCheck.packet[0] = 9;
            length = strlen(gameName);
            memcpy(&DWCi_availableCheck.packet[5], gameName, length + 1);
            DWCi_availableCheck.packetLength = length + 6;
            DWCi_socketSendTo(DWCi_availableCheck.sock, DWCi_availableCheck.packet,
                              DWCi_availableCheck.packetLength, 0, &DWCi_availableCheck.address, 8);
            DWCi_availableCheck.sendTime = current_time();
            DWCi_availableCheck.resendCount = 0;
        }
    }
}

/* Records and answers the probe result. */
static inline s32 DWCi_natProbeResult(s32 result) {
    DWCi_natProbeStatus = result;
    return result;
}

/* 0x8050C770 (0x1D4): polls the probe: 0 while waiting, 1 available, 2 unavailable, 3 temporarily unavailable
 * (also answered 1 when the socket could not be opened or nothing came back after one resend). */
s32 DWCi_natProbePoll(void) {
    __attribute__((aligned(32))) u8 packet[0x40];
    SOSockAddrIn from;
    int fromLength;
    int error;
    u32 flags;

    fromLength = 8;
    if (DWCi_availableCheck.sock == -1) {
        return DWCi_natProbeResult(1);
    }
    if (DWCi_socketHasData(DWCi_availableCheck.sock)) {
        if (DWCi_socketRecvFrom(DWCi_availableCheck.sock, packet, 0x40, 0, &from, &fromLength) < 7) {
            error = 1;
        } else if (memcmp(&from.addr, &DWCi_availableCheck.address.addr, 4) != 0) {
            error = 1;
        } else if (from.port != DWCi_availableCheck.address.port) {
            error = 1;
        } else if (memcmp(packet, "\xFE\xFD\x09", 3) != 0) {
            error = 1;
        } else {
            error = 0;
            flags = ((packet[3] << 24) | (packet[4] << 16)) | ((packet[5] << 8) | packet[6]);
        }
        if (!error) {
            DWCi_socketClose(DWCi_availableCheck.sock);
            if (flags & 1) {
                return DWCi_natProbeResult(2);
            }
            if (flags & 2) {
                return DWCi_natProbeResult(3);
            }
            return DWCi_natProbeResult(1);
        }
    }
    if (current_time() > DWCi_availableCheck.sendTime + 2000) {
        if (DWCi_availableCheck.resendCount == 1) {
            DWCi_socketClose(DWCi_availableCheck.sock);
            return DWCi_natProbeResult(1);
        }
        DWCi_socketSendTo(DWCi_availableCheck.sock, DWCi_availableCheck.packet, DWCi_availableCheck.packetLength, 0,
                          &DWCi_availableCheck.address, 8);
        DWCi_availableCheck.sendTime = current_time();
        DWCi_availableCheck.resendCount++;
    }
    return 0;
}
