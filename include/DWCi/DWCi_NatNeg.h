/*
 * include/DWCi/DWCi_NatNeg.h - the DWCi NATNEG / transport-tail unit (`src/DWCi/DWCi_NatNeg.c`,
 * `.text` 0x80512490..0x805145B8).
 *
 * Rule 2: this unit owns these symbols, so their declarations live here and every consumer includes
 * this header - the GameSpy interface unit reaches them through `include/unsplit/Network.h`, which
 * includes this header, and the band header `include/unsplit/DWCi.h` includes it too (that is how
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
 * header says so too.  Nine of its 17 functions have bodies today (see the unit's own header for
 * the measured list).
 */
#ifndef MHTRI_DWCI_DWCI_NATNEG_H
#define MHTRI_DWCI_DWCI_NATNEG_H

#include "types.h"

struct DWCiNatNegSession;

#ifdef __cplusplus
extern "C" {
#endif

/* the unprototyped callback shape the GameSpy interface installs and casts back */
typedef void (*DWCiCallback)();

/* The address record the DWCi socket wrappers send and receive through: one length byte, one family
 * byte, the network-order port and the network-order address - the same 8 bytes `bind`/`connect`
 * take.  Moved here from `src/DWCi/fn_805113B0.c` when `DWCi_natNegSetSocket` became its second
 * user (brief section 6.5 rule 1). size: 0x08 */
typedef struct DWCiSockAddrIn {
    /* +0x00 */ u8 len;
    /* +0x01 */ u8 family;
    /* +0x02 */ u16 port;
    /* +0x04 */ u32 addr;
} DWCiSockAddrIn;

void DWCi_NatNegCleanup(void);
s32  DWCi_NatNegStartSession(u32 session, s32 flag, DWCiCallback empty, DWCiCallback callback, void* result);
void DWCi_NatNegEndSession(u32 session);
void DWCi_NatNegProcess(void);
void DWCi_NatNegSendPacket(void* data, u32 size, void* header);

/* 0x80512490: lazy string/length accessor (the seam function; body in DWCi_NatNeg.c). */
void DWCi_GetStringLength(void** buf, int* len);

/* ---- the data the unit's claimed runs own (splits.txt, 2026-09-28) ---------------------------- */

/* 0x80794368 (.sdata) - the empty string `DWCi_GetStringLength` defaults a NULL buffer to, and
 * 0x80794370/78/7C the unit's own "%s:%d" / "%s" / ":%d" trio.  A second copy of the trio at
 * 0x80794358 (referenced by `DWCi_formatAddress`, `src/DWCi/fn_805113B0.c`) is a *different* TU's:
 * `-str reuse` merges identical literals inside one TU, never across two. */
extern u8 DWCi_emptyString[8];
extern char DWCi_natNegAddressFormat[6];
extern char DWCi_natNegAddressFormatHost[3];
extern char DWCi_natNegAddressFormatPort[4];

/* 0x80794380 - the NATNEG message signature every announce opens with and every received datagram is
 * compared against.  Deliberately *unsized* here: a sized array reaches the symbol SDA21 while an
 * unsized one reaches it ADDR16_HA/LO (`lis`/`addi`), and the two consumers' target objects need the
 * two forms - `src/Network/fn_8041A87C.cpp` is ADDR16 throughout, and it sees only this header (via
 * `include/unsplit/Network.h`), so this is the form the shared declaration has to carry.  The unit's
 * own source re-declares it sized for its own ten SDA21 sites, which a unit may do for its own object
 * and which the later declaration decides (both measured; playbook row 12 - the reloc kind is a
 * codegen input).  RESIDUAL: `DWCi_natNegPollReplies` needs the absolute form, which the sized
 * declaration cannot produce - test whether it came from a different original TU (a seam near
 * 0x805125F0) before rewriting its source shape. */
extern const u8 natNegMessageMagic[];

/* 0x80794388/0x8079438C - the two idle sockets the negotiator keeps open for its own address
 * discovery; both are -1 when closed.  0x80794390 is the protocol id the announce messages carry. */
extern s32 DWCi_natNegIdleSocketA;
extern s32 DWCi_natNegIdleSocketB;
extern u32 DWCi_natNegServerId;

/* 0x807614D8 (.bss, 0x80 B) - the host-name text the announce copies the local name from (read at 15
 * sites, written nowhere in the link set). */
extern char DWCi_natNegHostText[];

/* 0x807625C0 (.bss) - the two-buffer address-string ring `DWCi_formatAddress` /
 * `DWCi_natNegFormatAddress` write into, `DWCi_addressRingIndex` (declared unowned in
 * `include/unsplit/DWCi.h`) selecting the half.  It is declared here because it falls inside this
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
extern u8 DWCi_natNegSession[];

/* 0x80795828 the ring index, 0x80795848 the negotiator's list of per-server socket records (the
 * object `DWCi_NatNegProcess`/`DWCi_NatNegEndSession` walk and `DWCi_NatNegCleanup` destroys),
 * 0x8079584C/0x8079585C/0x80794390 the three server address words and the id/token pair it puts in
 * every announce, 0x80795860 the tick of its last poll and 0x80795864 the callback it reports a
 * completed negotiation through. */
extern u32 DWCi_natNegAddressRingIndex;
extern void* DWCi_natNegSocketList;
extern u32 DWCi_natNegServerAddr0;
extern u32 DWCi_natNegServerToken;
extern u32 DWCi_natNegLastPollTick;
extern void (*DWCi_natNegPollCallback)(u32, struct DWCiNatNegSession*);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_DWCI_DWCI_NATNEG_H */
