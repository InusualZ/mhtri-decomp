/*
 * DWCi/DWCi_NatNeg.h - the DWCi NATNEG / transport-tail unit (`src/DWCi/DWCi_NatNeg.c`,
 * `.text` 0x80512490..0x805145B8).
 *
 * Rule 2: this unit owns these symbols, so their declarations live here and every consumer includes
 * this header - the GameSpy interface unit reaches them through `unsplit/Network.h`, which
 * includes this header, and the band header `unsplit/DWCi.h` includes it too (that is how
 * `src/DWCi/fn_805113B0.c` and this unit itself see the data).  The data ranges
 * (`.sdata` 0x80794368..0x807943A0, `.sbss` 0x80795828..0x80795878, `.bss` 0x807614D8..0x80762A20)
 * were claimed by the data pass on 2026-09-28; nothing here is defined (playbook 29 - the bytes are
 * the target object's).
 *
 * The five entry points the GameSpy interface calls are named from its call sites
 * (`DWCi_NatNegStartSession` -> the negotiation starter that takes the session id and the two
 * callbacks, `DWCi_NatNegEndSession` -> the teardown it is paired with, `DWCi_NatNegCleanup` ->
 * the global cleanup called next to it, `DWCi_NatNegProcess` -> the per-iteration step, and
 * `DWCi_NatNegSendPacket` -> the header+payload send).  Each is a GUESS - the map has only
 * placeholders for the band - and a marker for a later reconstruction to confirm; the unit's own
 * header says so too.  All 17 functions have bodies (see the unit's own header for the measured
 * list).
 */
#ifndef MHTRI_DWCI_DWCI_NATNEG_H
#define MHTRI_DWCI_DWCI_NATNEG_H

#include "types.h"
#include "SO/soi.h"

struct DWCiNatNegSession;

#ifdef __cplusplus
extern "C" {
#endif

/* the unprototyped callback shape the GameSpy interface installs and casts back */
typedef void (*DWCiCallback)();

/* The address record the DWCi socket wrappers send and receive through: SO's own `SOSockAddrIn` (length,
 * family, network-order port and address). */
typedef SOSockAddrIn DWCiSockAddrIn;

/* 0x807625F0 - the game name `DWC_Init` copies in (the word sits in this unit's `.bss` run). GUESS on the name. */
extern char DWCi_gameName[];

void DWCi_NatNegCleanup(void);
/* Begins one negotiation: the game socket to bind the probes to (-1 for none), the cookie the peers share,
 * this side's client index, the progress and completion callbacks and the caller's payload.  Returns
 * 0 once the probes are out, 1 when the record cannot be allocated, 2 when the probe check failed or
 * the socket cannot be created, 3 when a server address cannot be resolved. */
/* untyped: caller-owned payload - handed back to both callbacks */
s32 DWCi_NatNegStartSession(s32 gameFd, s32 cookie, s32 clientIndex, DWCiCallback progress,
                            DWCiCallback complete, void* userData);
void DWCi_NatNegEndSession(u32 session);
void DWCi_NatNegProcess(void);
void DWCi_NatNegSendPacket(void* data, u32 size, void* header);

/* 0x80512490: lazy string/length accessor (the seam function; body in DWCi_NatNeg.c). */
void DWCi_GetStringLength(void** buf, int* len);

/* ---- the data the unit's claimed runs own (splits.txt) ---------------------------- */

/* 0x80794368 (.sdata) - the empty string `DWCi_GetStringLength` defaults a NULL buffer to, and
 * 0x80794370/78/7C the unit's own "%s:%d" / "%s" / ":%d" trio.  A second copy of the trio at
 * 0x80794358 (referenced by `DWCi_formatAddress`, `src/DWCi/fn_805113B0.c`) is a *different* TU's:
 * `-str reuse` merges identical literals inside one TU, never across two. */
extern u8 DWCi_emptyString[8];
extern char DWCi_natNegAddressFormat[6];
extern char DWCi_natNegAddressFormatHost[3];
extern char DWCi_natNegAddressFormatPort[4];

/* 0x80794394 - the "%s.%s" the negotiator joins the game name and a server host with. */
extern char DWCi_natNegHostFormat[6];

/* 0x80794380 - the NATNEG message signature every announce opens with and every received datagram is
 * compared against.  Deliberately *unsized* here: a sized array reaches the symbol SDA21 while an
 * unsized one reaches it ADDR16_HA/LO (`lis`/`addi`), and the two consumers' target objects need the
 * two forms - `src/Network/GameSpyInterfaceThread.cpp` is ADDR16 throughout, and it sees only this header (via
 * `unsplit/Network.h`), so this is the form the shared declaration has to carry.  The unit's
 * own source re-declares it sized before its first use, for the target's ten SDA21 sites (the first
 * use of a symbol fixes its addressing for the whole translation unit - playbook row 12, the reloc
 * kind is a codegen input).  RESIDUAL: `DWCi_natNegPollReplies` reaches it ABSOLUTELY in the target, which
 * one TU cannot do together with the SDA sites - see the unit's header (seam request). */
extern const u8 natNegMessageMagic[];

/* 0x80794388/0x8079438C - the two idle sockets the negotiator keeps open for its own address
 * discovery; both are -1 when closed.  0x80794390 is the NAT type the report messages carry. */
extern s32 DWCi_natNegIdleSocketA;
extern s32 DWCi_natNegIdleSocketB;
extern u32 DWCi_natNegNatType;

/* 0x807614D8 (.bss, 0x80 B) - the game name `DWCi_natProbeStart` stores (the NATNEG announce and host names read
 * it) and the availability-probe host override behind it (empty: "<game>.available.gs.nintendowifi.net"). */
typedef struct DWCiAvailableNames {
    /* +0x00 */ char gameName[0x40];
    /* +0x40 */ char hostName[0x40];
} DWCiAvailableNames; /* size: 0x80 */
extern DWCiAvailableNames DWCi_availableNames;

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

/* 0x807615C0 (.bss, 0x1000 B) - the datagram buffer `gti2ReceiveMessages` reads into. */
extern u8 DWCi_gt2ReceiveBuffer[0x1000];

/* 0x807625C0 (.bss) - the two-buffer address-string ring `DWCi_formatAddress` /
 * `DWCi_natNegFormatAddress` write into, `DWCi_addressRingIndex` (declared unowned in
 * `unsplit/DWCi.h`) selecting the half.  It is declared here because it falls inside this
 * unit's `.bss` run 0x807614D8..0x80762A20 - which is the claim's own documented residual: the word
 * belongs to `src/DWCi/fn_805113B0.c` on the reloc evidence, but the run cannot be drawn thin enough
 * to leave it out without a link-order cycle (playbook 53).  When 0x80509DB0..0x805113B0 is
 * registered, re-draw the run and move this declaration with it. */
extern u8 DWCi_addressRing[];

/* 0x80762900 (two 0x16-byte strings) - the second address-string ring, the one the NATNEG unit
 * formats its peer addresses into; `DWCi_natNegFormatAddress` swaps the index - and 0x80762940
 * (0xE0) the negotiator's session record, whose own layout is completed in `src/DWCi/DWCi_NatNeg.c`
 * (struct DWCiNatNegSession). */
extern u8 DWCi_natNegAddressRing[];

/* 0x80762700 (.bss, 0x200 B) - the receive buffer every negotiator poll reads a datagram into. */
extern u8 DWCi_natNegRecvBuffer[0x200];

/* 0x80762A20 (.bss, 0x200 B) - the receive buffer every negotiator socket's tick reads a datagram into.
 * Inside the unit's claimed `.bss` run (0x807614D8..0x80762C20); this unit is its only referencer
 * (`callers.py`), claimed in `splits.txt`. */
extern u8 DWCi_natNegSocketRecvBuffer[0x200];

/* 0x806308E8 (.data, 0x120 B) - the negotiator's string pool: the three server hosts
 * ("natneg1/2/3.gs.nintendowifi.net", 0x1C bytes apart) then its log lines; claimed in `splits.txt`. */
extern char DWCi_natNegStringPool[][28];
extern u8 DWCi_natNegSession[];

/* 0x80795828 the ring index, 0x80795848 the negotiator's list of per-server socket records (the
 * object `DWCi_NatNegProcess`/`DWCi_NatNegEndSession` walk and `DWCi_NatNegCleanup` destroys),
 * 0x8079584C/0x8079585C/0x80794390 the three server address words and the id/token pair it puts in
 * every announce, 0x80795860 the tick of its last poll and 0x80795864 the callback it reports a
 * completed negotiation through. */
extern u32 DWCi_natNegAddressRingIndex;
/* 0x8079583C/40/44 - set as the server's three reply types arrive; 0x80795838/30/2C/34 - set as the four
 * slot reports (cookie 0..3) arrive.  The poll is finished when every one of them is set. */
extern u32 DWCi_natNegType3Seen;
extern u32 DWCi_natNegType2Seen;
extern u32 DWCi_natNegType1Seen;
extern u32 DWCi_natNegSlot0Ready;
extern u32 DWCi_natNegSlot1Ready;
extern u32 DWCi_natNegSlot2Ready;
extern u32 DWCi_natNegSlot3Ready;
extern void* DWCi_natNegSocketList;
extern u32 DWCi_natNegServerAddr0;
extern u32 DWCi_natNegServerAddr1;
extern u32 DWCi_natNegServerAddr2;
extern char* DWCi_natNegServerName0;
extern char* DWCi_natNegServerName1;
extern char* DWCi_natNegServerName2;
extern u32 DWCi_natNegIdlePolling;
extern u32 DWCi_natNegMappingScheme;
extern u32 DWCi_natNegLastPollTick;
extern void (*DWCi_natNegPollCallback)(u32, struct DWCiNatNegSession*);

/* 0x80794380 is declared **once**, above, as `natNegMessageMagic`, and unsized on purpose: an unknown-
 * size array is the shape that gets the `lis`/`addi` (ADDR16_HA/LO) pair the target's relocation asks for,
 * so sizing it would move `Network/GameSpyInterfaceThread.cpp`'s codegen.  It moved here from
 * `unsplit/Network.h`, which had declared it while the range was unowned; the band reaches it by
 * including this header (rule 2). */
#ifdef __cplusplus
}
#endif

#endif /* MHTRI_DWCI_DWCI_NATNEG_H */
