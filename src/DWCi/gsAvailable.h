/*
 * DWCi/gsAvailable.h - the declarations `src/DWCi/gsAvailable.c` owns (docs/plan.md 6.5 rule 2): the GameSpy
 *   availability probe, its game name (the NATNEG unit reads it) and its result.
 */
#ifndef MHTRI_DWCI_GSAVAILABLE_H
#define MHTRI_DWCI_GSAVAILABLE_H

#include "types.h"
#include "SO/soi.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x80795818 - the result of the NAT-probe check `DWCi_natProbePoll` runs (1 = failed, 2 / 3 = the two
 * usable answers); the negotiator refuses to start while it reads 1.  Name and meaning are GUESSes
 * from that writer (it sends a probe, waits for its echo and stores 1, 2 or 3). */
extern s32 DWCi_natProbeStatus;

/* 0x807614D8 (.bss) - the game name `DWCi_natProbeStart` stores (the NATNEG announce and host names read it), and
 * 0x80761518 the availability-probe host override behind it (empty: "<game>.available.gs.nintendowifi.net"). */
extern char DWCi_availableGameName[0x40];
extern char DWCi_availableHostName[0x40];

/* 0x80761558 (.bss) - the availability probe `DWCi_natProbeStart` sends and `DWCi_natProbePoll` waits on: its
 * socket, the server address, the query packet and its length, the send time and the resend count. */
typedef struct DWCiAvailableCheck {
    /* +0x00 */ s32 sock;
    /* +0x04 */ SOSockAddrIn address;
    /* +0x0C */ u8 packet[0x40];
    /* +0x4C */ s32 packetLength;
    /* +0x50 */ u32 sendTime;
    /* +0x54 */ s32 resendCount;
    /* +0x58 */ u8 pad_0x58[0x10];
} DWCiAvailableCheck; /* size: 0x68 (the map row's extent; the bodies use 0x58) */
extern DWCiAvailableCheck DWCi_availableCheck;

void DWCi_natProbeStart(const char* gameName);
s32 DWCi_natProbePoll(void);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_DWCI_GSAVAILABLE_H */
