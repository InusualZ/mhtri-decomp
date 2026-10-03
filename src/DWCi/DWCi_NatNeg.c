/*
 * DWCi_NatNeg.c - the DWCi band's tail, `.text` 0x80512490..0x805145B8 (17 functions, 8488 B): the
 * GameSpy NAT-negotiation client (the `natneg.c` of the SDK, in Nintendo's DWC build) and the address
 * helpers it shares with the transport.
 *
 * REGISTRATION (recon lane, 2026-09-27).  Left edge 0x80512490 is the seam with the registered
 * `DWCi_sendControlFrame` unit: `tudiscover.py at 0x80512490` reports `cut 18642 0x80512490 strong x2`
 * (two independent .sdata run jumps, DWCi_addressFormatPort -> DWCi_emptyString and DWCi_emptyString ->
 * 0x80794370), so a TU begins at 0x80512490.  Right edge 0x805145B8 is a HARD instruction-level cut:
 * every function in 0x80512490..0x805145B8 starts 16-byte aligned (17/17, `gap_*` zero runs between
 * neighbours) while 0x805145B8..0x8051B7FC packs on 4 - the DWCi/NHTTP library and flag boundary.
 * Module `DWCi`, not NHTTP: the range calls only the DWCi transport helpers at 0x8050A..-0x8050E.. and
 * carries the negotiator's own pool ("natneg1/2/3.gs.nintendowifi.net", the log lines).
 *
 * NAME.  `DWCi_NatNeg.c` is an evidence call, not a map name (`dumpmap.py lookup` answers only
 * `zz_0512xxx_`); rename if the SDK's real file name is recovered.
 *
 * WHAT THE BODIES ARE.  The packet layout (`DWCiNatNegPacket`: magic FD FC 1E 66 6A B2, version 3, type,
 * cookie, then the INIT / CONNECT / REPORT bodies), the 0x54-byte socket record and the NAT-type verdict
 * are the SDK's, so the names follow it: `DWCi_natNegSendInit` (type 0, one probe per port class),
 * `DWCi_natNegOnReply` (INIT_ACK 1, ERT test 2, REPORT_ACK 14), `DWCi_natNegOnConnect` / `OnConnectPing`
 * (types 5 and 7), `DWCi_natNegDetermineNatType` (the NAT type, promiscuity and mapping scheme from the
 * four probe answers), `DWCi_natNegPollReplies` (the idle-socket poll: ERT replies and the address
 * report) and `DWCi_natNegTickSocket` (the per-record state machine).  Every name is derived from the
 * body and is a GUESS where the image gives no spelling (the dump answers `zz_` for the range).
 * `DWCi_NatNegStartSession` takes SIX arguments - the game socket is `r3` and the Network caller reaches it
 * through `gt2GetSocketSOCKET`'s result - which the old 5-argument declaration hid.  The shared
 * `.bss`/`.sbss`/`.sdata` objects were renamed from what the code stores there
 * (`DWCi_natNegNatType`/`MappingScheme` are the words the report carries, `DWCi_natNegGameName` the
 * `%s.%s` prefix and the report's game name).
 *
 * FLAGS.  `cflags_base` without the lib's `-func_align 4` (see `configure.py`): the 16-byte function
 * alignment reproduces the loop-alignment `nop` of `DWCi_natNegTickIdleSockets` (98.48 -> 100.00).
 *
 * MEASURED (2026-09-29, Wii/1.3, this build): 17 functions written, unit 97.72 %.  Byte-identical (11):
 * `DWCi_GetStringLength`, `DWCi_natNegFormatAddress`, `DWCi_natNegPollRepliesOnce`,
 * `DWCi_natNegDetermineNatType`, `DWCi_NatNegCleanup`, `DWCi_natNegTickIdleSockets`,
 * `DWCi_NatNegEndSession`, `DWCi_NatNegProcess`, `DWCi_NatNegSendPacket`, `DWCi_natNegOnConnectPing`,
 * `DWCi_natNegOnReply`.  The others:
 *   DWCi_natNegOnConnect        99.86   two swapped registers (fd/addr of the inlined send)
 *   DWCi_natNegSetSocket        99.84   the same swap
 *   DWCi_natNegSendInit         98.78   the same swap in the four sends, and the local-address walk's
 *                                       `beq`/`b` pair after `DWCi_hostAddressIsUsable` (ours falls through)
 *   DWCi_NatNegStartSession     96.31   the string-pool base is colour r31 in ours, r25 in the target
 *   DWCi_natNegPollReplies      93.26   see below
 *   DWCi_natNegTickSocket       93.17   see below
 *
 * Load-bearing source shapes (each measured): the token lookup, the close-by-token and the send are
 * `static inline` helpers - MWCC inlines them into SetSocket, TickSocket, OnReply and the senders exactly
 * as the target has them (a helper defined *after* its first use is not inlined; a non-inline `static` is
 * emitted as a symbol); `(gameFd != -1) ? gameFd : fd` written in that polarity and inside the argument list
 * reproduces the target's `beq`/`b` pair and its load order (100 % on OnConnectPing); the local-address walk
 * is a top-tested `for (;;)` with the exit in an `else` (`for (...; ...; i++)` is bottom-tested and costs
 * 10 points); a stack local's address is *reverse declaration order*, so the locals are declared in the
 * target's reverse order (frames match to the byte); `u16 port` on `DWCi_natNegFormatAddress` is what
 * yields the `mr r0,r3` / `clrlwi` argument shape at its callers.
 *
 * Residuals, all measured:
 *   - `DWCi_natNegPollReplies` reaches `natNegMessageMagic` ABSOLUTELY (`lis`/`addi`, the only two ADDR16
 *     sites among twelve) and its two `switch`es are MWCC's COMPARE TREE (`cmpwi 2; beq; bge; ...`) where
 *     Wii/1.0-1.7 lowers the same dense switch to a linear `beq` chain - every GC/2.x and GC/3.0 compiler
 *     produces the tree (measured, scratch compile, cases 1,2,3 and 0..3); at `-O0` the Wii compilers do
 *     too, and no pragma, flag or spelling in this compiler moves it.  `DWCi_natNegOnReply`'s own
 *     switch (cases 1, 2, 14) IS linear in the target.  Both facts point at the SAME conclusion: PollReplies
 *     (and the 4-byte thunk after it) is a DIFFERENT translation unit, compiled by an older compiler,
 *     whose `.sdata` reference to the signature is an `extern` and whose `.sbss` flags (0x8079582C..44)
 *     sit between `DWCi_natNegAddressRingIndex` and the socket list.  Unit-wide `GC/3.0a5.2` measures
 *     PollReplies 85.3 against this build's 82.5 before the loop fix, and lowers every other function
 *     (TickIdle 91.8, GetStringLength 91.6, SetSocket 96.5, OnReply 93.5).  Filed as a seam request:
 *     X = 0x80512490..0x805125F0, P = 0x805125F0..0x80512990, Q = 0x80512990..0x805145B8.  A single
 *     symbol cannot be both `extern[]` (ADDR16) and sized (SDA21) in one MWCC TU - the first use fixes the
 *     addressing, measured with a scratch pair - so inside one unit the sized declaration wins ten sites
 *     to one.
 *   - `DWCi_natNegTickSocket` (1276 B against 1300): the two inlined copies of the report and ping builders
 *     compute `negResult` / `finished` as the un-folded `cmpwi; bne; li 0; b; li 1` branch pair where every
 *     other copy in the unit (SetSocket, OnConnectPing) and every spelling tried (`!=`, `== ? 0 : 1`, an
 *     if/else into a `u8`, a hand-written copy) folds to `subfic`/`cntlzw`; the recv loop's exit test
 *     reloads `node->fd` into `r0` in the target and shares `r3` in ours; the `for` index takes r29 where
 *     the target has r30.
 *   - The fd/address register swap in the inlined send (SetSocket, OnConnect, SendInit): the first-loaded
 *     argument is the lower register in the target and the higher one here; swapping the helper's
 *     parameter order moved it the wrong way (OnConnectPing 100 -> 97.1).
 *   - StartSession: the pool pointer is a local hoisted to the entry in both, but coloured after the
 *     six parameters in the target and before them here; declaration order does not move it (600 of the
 *     5040 orders sampled).
 *
 * DATA.  Claimed by the data pass (2026-09-28) and still only declared: `.sdata` 0x80794368..0x807943A0,
 * `.sbss` 0x80795828..0x80795878, `.bss` 0x807614D8..0x80762A20 (`datagap.py --mode both`: target-extra
 * .bss 5448 B, .sbss 80 B, .sdata 56 B; ours 0 - unchanged by this pass).  Two ranges the bodies now
 * need are filed in the outbox: `.bss` 0x80762A20 (0x200 B, the socket receive buffer, sole referencer
 * `DWCi_natNegTickSocket`) and `.data` 0x806308E8 (0x120 B, the negotiator's string pool, sole referencer
 * `DWCi_NatNegStartSession`, dead-stripped emitters); `DWCi_natProbeStatus` (0x80795818) is written by the
 * unregistered band's `DWCi_natProbePoll`.
 */

#include "types.h"
#include "DWCi/DWCi_NatNeg.h"            /* the owner's own header: the entry points and the data */
#include "unsplit/DWCi.h"                /* the band's unowned data and helpers (rule 2) */
#include "SO/soi.h"
#include "unsplit/SO.h"                  /* SOAddressToString / SOAddressToHostPort / SOHtoNs */
#include "unsplit/Runtime.PPCEABI.H.h"   /* sprintf / memcmp / memcpy / strlen */

/* The sized form of this unit's own `natNegMessageMagic`.  The owner's header has to carry the *unsized*
 * spelling, because `src/Network/fn_8041A87C.cpp` (which reaches this symbol through
 * `include/unsplit/Network.h`) is ADDR16_HA/LO throughout, and a sized array yields the SDA form; the
 * first use of the symbol in a translation unit fixes its addressing for every later use, so this line
 * has to precede every body.  Ten of this unit's sites are SDA21 in the target; the eleventh,
 * `DWCi_natNegPollReplies`, is `lis`/`addi` and cannot be reached in the same unit (see the header). */
extern const u8 natNegMessageMagic[8];

/* --------------------------------------------------------------------------------------------- */
/* Types                                                                                          */
/* --------------------------------------------------------------------------------------------- */

/* One negotiator socket record - the element type of `DWCi_natNegSocketList` (created by
 * `DWCi_NatNegStartSession`, ticked by `DWCi_natNegTickSocket`).  `state`: 0 probing the servers, 1 servers
 * answered, 2 pinging the peer, 3 peer answered, 4 closed, 5 report sent, 6 report acknowledged.
 * `result` is the completion callback's first argument: 0 connected, 1 / 2 / 3 / 4 the failures. */
typedef struct DWCiNatNegSocket {
    /* +0x00 */ s32 fd;
    /* +0x04 */ s32 gameFd;         /* the game's own socket the probes may go out through, or -1 */
    /* +0x08 */ s32 token;          /* the cookie shared with the peer; what a reply is routed by */
    /* +0x0C */ s32 clientIndex;
    /* +0x10 */ s32 state;
    /* +0x14 */ u32 initAck[4];     /* set when the server answered the probe of that port class */
    /* +0x24 */ s32 retryCount;
    /* +0x28 */ s32 retryLimit;
    /* +0x2C */ u32 deadline;       /* DWCi_getTick() value the current step times out at */
    /* +0x30 */ u32 peerAddr;
    /* +0x34 */ u16 peerPort;
    /* +0x36 */ u8 gotYourData;     /* the peer's ping reached us */
    /* +0x37 */ u8 finished;
    /* +0x38 */ DWCiCallback progressCb;   /* (state, userData) */
    /* +0x3C */ DWCiCallback completeCb;   /* (result, resultArg, resultAddr, userData) */
    /* +0x40 */ void* userData;
    /* +0x44 */ u32 result;
    /* +0x48 */ u32 resultArg;
    /* +0x4C */ DWCiSockAddrIn resultAddr;
} DWCiNatNegSocket; /* size: 0x54 */

/* One 16-byte entry of the session's address table: this host's own address and port for one probe, and
 * the address and port the server reported seeing.  Read out of `DWCi_natNegPollReplies`'s indexing
 * (0x94 + i*16, fields at +0/+4/+8/+12). */
typedef struct DWCiNatNegAddr {
    /* +0x00 */ u32 ownAddr;
    /* +0x04 */ u16 ownPort;
    /* +0x06 */ u16 pad_0x06;
    /* +0x08 */ u32 seenAddr;
    /* +0x0C */ u16 seenPort;
    /* +0x0E */ u16 pad_0x0E;
} DWCiNatNegAddr; /* size: 0x10 */

/* The negotiator's session record `DWCi_natNegSession`: the probe results `DWCi_natNegPollReplies` collects
 * and the verdict `DWCi_natNegDetermineNatType` draws from them (the NAT type, port-mapping scheme and
 * promiscuity a report carries).  size: 0xD8 */
struct DWCiNatNegSession {
    /* +0x00 */ u8 pad_0x000[0x80];
    /* +0x80 */ u32 ert2Missing;    /* cleared when the port-class 2 test reply arrived */
    /* +0x84 */ u32 ert3Missing;    /* cleared when the port-class 3 test reply arrived */
    /* +0x88 */ u32 promiscuity;
    /* +0x8C */ u32 natType;
    /* +0x90 */ u32 mappingScheme;
    /* +0x94 */ DWCiNatNegAddr table[4];
    /* +0xD4 */ u32 portsStable;
}; /* size: 0xD8 */

/* The 8-byte framed header the send layer reads: a reserved halfword, the port and the address, both
 * network order.  Sourced from this unit's own readers - `DWCi_NatNegSendPacket` at 0x80514448
 * `lwz r0,4(r28)` and 0x80514454 `lhz r3,2(r28)`. */
typedef struct DWCiNatNegHeader {
    /* +0x00 */ u16 code;
    /* +0x02 */ u16 port;
    /* +0x04 */ u32 addr;
} DWCiNatNegHeader; /* size: 0x08 */

/* The framed NATNEG packet the negotiator sends and receives: a 12-byte header (signature, version, type,
 * the shared cookie) and one of four bodies.  The packing is the wire's - the local address sits at an odd
 * offset (playbook 65).  size: 0x49 */
#pragma pack(push, 1)
typedef struct DWCiNatNegPacket {
    /* +0x00 */ u8 magic[6];
    /* +0x06 */ u8 version;
    /* +0x07 */ u8 type;
    /* +0x08 */ s32 cookie;
    union {
        struct {
            /* +0x0C */ u8 portType;
            /* +0x0D */ u8 clientIndex;
            /* +0x0E */ u8 useGamePort;
            /* +0x0F */ u32 localAddr;
            /* +0x13 */ u16 localPort;
        } init; /* size: 0x09 */
        struct {
            /* +0x0C */ u8 portType;
            /* +0x0D */ u8 clientIndex;
            /* +0x0E */ u8 useGamePort;
            /* +0x0F */ u8 localAddr[4];
            /* +0x13 */ u8 localPort[2];
            /* +0x15 */ char gameName[52];
        } wireInit; /* size: 0x3D */
        struct {
            /* +0x0C */ u32 remoteAddr;
            /* +0x10 */ u16 remotePort;
            /* +0x12 */ u8 gotYourData;
            /* +0x13 */ u8 finished;
        } connect; /* size: 0x08 */
        struct {
            /* +0x0C */ u8 portType;
            /* +0x0D */ u8 clientIndex;
            /* +0x0E */ u8 negResult;
            /* +0x0F */ u32 natType;
            /* +0x13 */ u32 mappingScheme;
            /* +0x17 */ char gameName[50];
        } report; /* size: 0x3D */
    } body;
} DWCiNatNegPacket; /* size: 0x49 */
#pragma pack(pop)

/* This unit's own bodies that other bodies in this file reach, in address order. */
void DWCi_natNegSetSocket(DWCiNatNegSocket* rec, u32 result, u32 resultArg, DWCiSockAddrIn* addr);
u32 DWCi_natNegDetermineNatType(struct DWCiNatNegSession* session);
void DWCi_natNegSendInit(DWCiNatNegSocket* node);
u32 DWCi_natNegTickIdleSockets(s32 sock);
u32 DWCi_natNegPollReplies(s32 sock, struct DWCiNatNegSession* session);
s32 DWCi_natNegPollRepliesOnce(s32 sock, struct DWCiNatNegSession* session);
void DWCi_natNegTickSocket(DWCiNatNegSocket* node);
void DWCi_natNegOnConnect(DWCiNatNegSocket* node, DWCiNatNegPacket* msg, DWCiNatNegHeader* sender);
void DWCi_natNegOnConnectPing(DWCiNatNegSocket* node, DWCiNatNegPacket* msg, DWCiNatNegHeader* sender);
void DWCi_natNegOnReply(DWCiNatNegSocket* node, DWCiNatNegPacket* msg, DWCiNatNegHeader* sender);

/* --------------------------------------------------------------------------------------------- */
/* Bodies, in address order                                                                        */
/* --------------------------------------------------------------------------------------------- */

/* 0x80512490 (0x64): lazy string/length accessor - a NULL buffer slot becomes "", a -1 length slot
 * becomes strlen(s)+1.  NAME (derived): the callers use it to default/normalise a
 * `{ buffer, length }` pair before the transport reads it (0x8050D860 / 0x8050D9F0 / 0x8050E150 /
 * DWCi_sendTo), which is exactly "get the string, and its length, defaulting both". */
void DWCi_GetStringLength(void** buf, int* len) {
    if (*buf == 0) {
        *buf = DWCi_emptyString;
        *len = 0;
    } else if (*len == -1) {
        *len = strlen((char*)*buf) + 1;
    }
}

/* 0x80512500 (0xE4): format `addr:port`, `addr`, `:port` or the empty string into the caller's
 * buffer, or into this unit's own two-buffer ring when the caller passes none. */
char* DWCi_natNegFormatAddress(u32 addr, u16 port, char* buf) {
    char* out;

    if (buf != 0) {
        out = buf;
    } else {
        DWCi_natNegAddressRingIndex ^= 1;
        out = (char*)(DWCi_natNegAddressRing + DWCi_natNegAddressRingIndex * 0x16);
    }
    if (addr != 0) {
        u32 withPort;
        u32 withoutPort;

        if (port != 0) {
            withPort = addr;
            sprintf(out, DWCi_natNegAddressFormat, SOAddressToString(&withPort), port);
        } else {
            withoutPort = addr;
            sprintf(out, DWCi_natNegAddressFormatHost, SOAddressToString(&withoutPort));
        }
    } else {
        if (port != 0) {
            sprintf(out, DWCi_natNegAddressFormatPort, port);
        } else {
            *out = 0;
        }
    }
    return out;
}

/* 0x805125F0 (0x390): drain the idle socket - every datagram that opens with the NATNEG signature is
 * an ERT reply (type 2, routed by its port type) or a report of the local address (type 11, routed
 * by the cookie, which also records the local address and port).  Returns 0 once every reply has
 * been seen. */
u32 DWCi_natNegPollReplies(s32 sock, struct DWCiNatNegSession* session) {
    DWCiSockAddrIn from;
    DWCiSockAddrIn local;
    int fromLen;
    int localLen;
    DWCiNatNegPacket msg;
    DWCiNatNegPacket* in;
    s32 n;

    fromLen = 8;
    if (DWCi_natNegType1Seen != 0 && DWCi_natNegType2Seen != 0 && DWCi_natNegType3Seen != 0
        && DWCi_natNegSlot0Ready != 0 && DWCi_natNegSlot3Ready != 0 && DWCi_natNegSlot1Ready != 0
        && DWCi_natNegSlot2Ready != 0) {
        return 0;
    }
    in = (DWCiNatNegPacket*)DWCi_natNegRecvBuffer;
    if (sock != -1) {
        while (sock != -1 && DWCi_socketHasData(sock) != 0) {
            u32 type;

            n = DWCi_socketRecvFrom(sock, DWCi_natNegRecvBuffer, 512, 0, &from, &fromLen);
            if (n == -1) {
                DWCi_socketGetLastError(sock);
                break;
            }
            if (memcmp(DWCi_natNegRecvBuffer, natNegMessageMagic, 6) != 0) {
                return 1;
            }
            type = in->type;
            if (n < 21) {
                return 1;
            }
            if (type == 2) {
                memcpy(&msg, in, 21);
                switch (msg.body.init.portType) {
                case 1:
                    DWCi_natNegType1Seen = 1;
                    DWCi_natNegFormatAddress(from.addr, SOAddressToHostPort(from.port), 0);
                    break;
                case 2:
                    session->ert2Missing = 0;
                    DWCi_natNegType2Seen = 1;
                    DWCi_natNegFormatAddress(from.addr, SOAddressToHostPort(from.port), 0);
                    break;
                case 3:
                    session->ert3Missing = 0;
                    DWCi_natNegType3Seen = 1;
                    DWCi_natNegFormatAddress(from.addr, SOAddressToHostPort(from.port), 0);
                    break;
                default:
                    break;
                }
            } else if (type == 11) {
                DWCiHostEntry* host;
                DWCiHostAddr* entry;
                DWCiNatNegAddr* slot;
                u32 addr;
                u32 ip;
                s32 loopback;
                u16 port;
                s32 i;

                memcpy(&msg, in, 21);
                msg.cookie = DWCi_socketTokenFromPeer(msg.cookie);
                switch (msg.cookie) {
                case 0:
                    DWCi_natNegSlot0Ready = 1;
                    break;
                case 3:
                    DWCi_natNegSlot3Ready = 1;
                    break;
                case 1:
                    DWCi_natNegSlot1Ready = 1;
                    break;
                case 2:
                    DWCi_natNegSlot2Ready = 1;
                    break;
                default:
                    break;
                }
                addr = 0;
                host = DWCi_socketGetLocalHostEntry();
                if (host == 0) {
                    addr = 0;
                } else {
                    i = 0;
                    for (;;) {
                        entry = host->hosts[i];
                        if (entry == 0) {
                            break;
                        }
                        loopback = DWCi_peerTokenFromSocket(0x7F000001);
                        ip = entry->addr;
                        if (ip != (u32)loopback) {
                            addr = ip;
                            if (DWCi_hostAddressIsUsable(entry) != 0) {
                                break;
                            }
                        }
                        i++;
                    }
                }
                session->table[msg.cookie].ownAddr = addr;
                localLen = 8;
                if (DWCi_socketGetLocalName(sock, &local, &localLen) == -1) {
                    port = 0;
                } else {
                    port = local.port;
                }
                session->table[msg.cookie].ownPort = SOAddressToHostPort(port);
                session->table[msg.cookie].seenAddr = msg.body.init.localAddr;
                session->table[msg.cookie].seenPort = SOAddressToHostPort(msg.body.init.localPort);
                DWCi_natNegFormatAddress(from.addr, SOAddressToHostPort(from.port), 0);
                slot = &session->table[msg.cookie];
                if (slot != 0) {
                    DWCi_natNegFormatAddress(slot->ownAddr, slot->ownPort, 0);
                    DWCi_natNegFormatAddress(slot->seenAddr, slot->seenPort, 0);
                }
            }
        }
    }
    return 1;
}


/* 0x80512980 (0x4): a tail thunk onto `DWCi_natNegPollReplies` - the image keeps the same routine
 * under two addresses, with the arguments passed straight through (GUESS: only the call shape is
 * evidence, and the name says which of the two the caller reached). */
s32 DWCi_natNegPollRepliesOnce(s32 sock, struct DWCiNatNegSession* session) {
    return DWCi_natNegPollReplies(sock, session);
}

/* 0x80512990 (0x2C0): derive the NAT type, the promiscuity, the port-mapping scheme and the
 * ports-stable flag from the four probe results in the session record; 0 while a probe is missing. */
u32 DWCi_natNegDetermineNatType(struct DWCiNatNegSession* session) {
    struct DWCiNatNegSession* s = session;
    DWCiNatNegAddr* t = s->table;

    s->natType = 6;
    s->promiscuity = 4;
    s->portsStable = 1;
    if (t[0].seenAddr == 0 || t[1].seenAddr == 0 || t[2].seenAddr == 0) {
        return 0;
    }
    if (s->ert3Missing == 0 && s->ert2Missing == 0 && t[0].seenAddr == t[0].ownAddr) {
        s->natType = 0;
    } else if (t[0].seenAddr == t[0].ownAddr) {
        s->natType = 1;
    } else if (s->ert2Missing == 0 && s->ert3Missing == 0 && __abs(t[2].seenPort - t[1].seenPort) >= 1) {
        s->natType = 5;
        s->promiscuity = 0;
    } else if (s->ert2Missing != 0 && s->ert3Missing == 0 && __abs(t[2].seenPort - t[1].seenPort) >= 1) {
        s->natType = 5;
        s->promiscuity = 2;
    } else if (s->ert2Missing == 0 && s->ert3Missing != 0 && __abs(t[2].seenPort - t[1].seenPort) >= 1) {
        s->natType = 5;
        s->promiscuity = 3;
    } else if (s->ert2Missing != 0 && s->ert3Missing != 0 && __abs(t[2].seenPort - t[1].seenPort) >= 1) {
        s->natType = 5;
        s->promiscuity = 1;
    } else if (s->ert3Missing != 0) {
        s->natType = 4;
    } else if (s->ert2Missing != 0 && s->ert3Missing == 0) {
        s->natType = 3;
    } else if (s->ert2Missing == 0 && s->ert3Missing == 0) {
        s->natType = 2;
    } else {
        s->natType = 6;
    }
    if (t[0].seenPort == t[0].ownPort && t[1].seenPort == t[1].ownPort && t[2].seenPort == t[2].ownPort) {
        s->mappingScheme = 1;
    } else if (t[0].seenPort == t[1].seenPort && t[1].seenPort == t[2].seenPort) {
        s->mappingScheme = 2;
    } else if (t[0].seenPort == t[0].ownPort && t[2].seenPort - t[1].seenPort == 1) {
        s->mappingScheme = 4;
    } else if (t[2].seenPort - t[1].seenPort == 1) {
        s->mappingScheme = 3;
    } else {
        s->mappingScheme = 0;
    }
    if (t[3].seenPort != 0 && t[0].seenPort != t[3].seenPort) {
        s->portsStable = 0;
    }
    return 1;
}

/* 0x80512C50 (0x34): destroy the negotiator's socket list and clear its head. */
void DWCi_NatNegCleanup(void) {
    if (DWCi_natNegSocketList != 0) {
        DWCi_listDestroy(DWCi_natNegSocketList);
        DWCi_natNegSocketList = 0;
    }
}

/* Finds the negotiator socket record whose token is `token`; 0 when there is none. */
static inline DWCiNatNegSocket* DWCi_natNegFindByToken(s32 token) {
    s32 i;

    if (DWCi_natNegSocketList == 0) {
        return 0;
    }
    for (i = 0; i < (s32)DWCi_listCount(DWCi_natNegSocketList); i++) {
        DWCiNatNegSocket* it = (DWCiNatNegSocket*)DWCi_listItem(DWCi_natNegSocketList, i);
        if (it->token == token) {
            return it;
        }
    }
    return 0;
}

/* Closes the negotiator socket whose record carries `token` and marks the record finished. */
static inline void DWCi_natNegCloseByToken(s32 token) {
    DWCiNatNegSocket* node = DWCi_natNegFindByToken(token);

    if (node != 0) {
        if (node->fd != -1) {
            DWCi_socketClose(node->fd);
        }
        node->fd = -1;
        node->state = 4;
    }
}

/* Sends one packet to `addr:port` through `sock`. */
static inline void DWCi_natNegSendTo(s32 sock, DWCiNatNegPacket* pkt, s32 len, u32 addr, u16 port) {
    DWCiSockAddrIn sa;

    sa.family = 2;
    sa.port = SOHtoNs(port);
    sa.addr = addr;
    DWCi_socketSendTo(sock, pkt, len, 0, &sa, 8);
}

/* Sends the completion report (packet type 13) to the first server: the result, this side's client index,
 * the NAT type and mapping scheme the idle sockets worked out, and the game name. */
static inline void DWCi_natNegSendReport(DWCiNatNegSocket* node) {
    DWCiNatNegPacket pkt;
    u32 addr;

    memcpy(pkt.magic, natNegMessageMagic, 6);
    pkt.version = 3;
    pkt.type = 13;
    pkt.cookie = DWCi_peerTokenFromSocket(node->token);
    pkt.body.report.clientIndex = node->clientIndex;
    pkt.body.report.negResult = (node->result == 0);
    pkt.body.report.natType = DWCi_natNegNatType;
    pkt.body.report.mappingScheme = DWCi_natNegMappingScheme;
    if (strlen(DWCi_natNegGameName) != 0) {
        memcpy(pkt.body.report.gameName, DWCi_natNegGameName, 50);
    }
    addr = DWCi_natNegServerAddr0;
    SOAddressToString(&addr);
    DWCi_natNegSendTo(node->fd, &pkt, 73, DWCi_natNegServerAddr0, 27901);
}

/* 0x80512C90 (0x1F4): record the result and its address, then act on it - results 1 and 2 finish
 * the negotiation on the spot (fire the completion callback and drop the socket out of the list),
 * anything else sends the completion report and waits for its acknowledgement. */
void DWCi_natNegSetSocket(DWCiNatNegSocket* rec, u32 result, u32 resultArg, DWCiSockAddrIn* addr) {
    rec->result = result;
    rec->resultArg = resultArg;
    if (addr != 0) {
        memcpy(&rec->resultAddr, addr, 8);
    }
    if (result == 1 || result == 2) {
        rec->state = 3;
        rec->completeCb(rec->result, rec->resultArg, &rec->resultAddr, rec->userData);
        DWCi_natNegCloseByToken(rec->token);
    } else {
        DWCi_natNegSendReport(rec);
        rec->state = 5;
        rec->deadline = DWCi_getTick() + 1000;
        rec->retryCount = 0;
        rec->retryLimit = 5;
    }
}

/* 0x80512E90 (0x320): send the INIT probes of a negotiation - one per port type, each to the server that
 * type is answered by, until the server acknowledges it - and arm the retry timer. */
void DWCi_natNegSendInit(DWCiNatNegSocket* node) {
    DWCiNatNegPacket pkt;
    DWCiNatNegPacket* p = &pkt;
    DWCiHostEntry* host;
    DWCiHostAddr* entry;
    u32 ip;
    u32 a;
    s32 i;
    u32 wire;
    s32 len;
    u16 port;
    s32 sock;
    u32 addr1;
    u32 addr2;
    u32 addr3;
    u32 addr4;
    int localLen;

    memcpy(p->magic, natNegMessageMagic, 6);
    pkt.version = 3;
    pkt.type = 0;
    pkt.cookie = DWCi_peerTokenFromSocket(node->token);
    ip = 0;
    pkt.body.wireInit.clientIndex = node->clientIndex;
    pkt.body.wireInit.useGamePort = (node->gameFd != -1);
    host = DWCi_socketGetLocalHostEntry();
    if (host == 0) {
        a = 0;
    } else {
        i = 0;
        for (;;) {
            entry = host->hosts[i];
            if (entry != 0) {
                a = entry->addr;
                if (a != (u32)DWCi_peerTokenFromSocket(0x7F000001)) {
                    ip = a;
                    if (DWCi_hostAddressIsUsable(entry) != 0) {
                        break;
                    }
                }
                i++;
            } else {
                a = ip;
                break;
            }
        }
    }
    wire = DWCi_socketTokenFromPeer(a);
    pkt.body.wireInit.localAddr[0] = wire >> 24;
    pkt.body.wireInit.localAddr[1] = wire >> 16;
    pkt.body.wireInit.localAddr[2] = wire >> 8;
    pkt.body.wireInit.localAddr[3] = wire;
    pkt.body.wireInit.localPort[0] = 0;
    pkt.body.wireInit.localPort[1] = 0;
    strcpy(pkt.body.wireInit.gameName, DWCi_natNegGameName);
    len = strlen(DWCi_natNegGameName) + 22;
    if (pkt.body.wireInit.useGamePort != 0 && node->initAck[0] == 0) {
        DWCiSockAddrIn sa;
        s32 dstSock;
        u32 dst;

        p->body.wireInit.portType = 0;
        addr1 = DWCi_natNegServerAddr0;
        SOAddressToString(&addr1);
        dstSock = node->gameFd;
        dst = DWCi_natNegServerAddr0;
        sa.family = 2;
        sa.port = SOHtoNs(27901);
        sa.addr = dst;
        DWCi_socketSendTo(dstSock, p, len, 0, &sa, 8);
    }
    if (node->initAck[1] == 0) {
        DWCiSockAddrIn sa;
        s32 dstSock;
        u32 dst;

        p->body.wireInit.portType = 1;
        addr2 = DWCi_natNegServerAddr0;
        SOAddressToString(&addr2);
        dstSock = node->fd;
        dst = DWCi_natNegServerAddr0;
        sa.family = 2;
        sa.port = SOHtoNs(27901);
        sa.addr = dst;
        DWCi_socketSendTo(dstSock, p, len, 0, &sa, 8);
    }
    {
        DWCiSockAddrIn local;

        sock = (pkt.body.wireInit.useGamePort != 0) ? node->gameFd : node->fd;
        localLen = 8;
        if (DWCi_socketGetLocalName(sock, &local, &localLen) == -1) {
            port = 0;
        } else {
            port = local.port;
        }
    }
    port = SOAddressToHostPort(port);
    pkt.body.wireInit.localPort[0] = port >> 8;
    pkt.body.wireInit.localPort[1] = port;
    if (node->initAck[2] == 0) {
        DWCiSockAddrIn sa;
        s32 dstSock;
        u32 dst;

        p->body.wireInit.portType = 2;
        addr3 = DWCi_natNegServerAddr1;
        SOAddressToString(&addr3);
        dstSock = node->fd;
        dst = DWCi_natNegServerAddr1;
        sa.family = 2;
        sa.port = SOHtoNs(27901);
        sa.addr = dst;
        DWCi_socketSendTo(dstSock, p, len, 0, &sa, 8);
    }
    if (node->initAck[3] == 0) {
        DWCiSockAddrIn sa;
        s32 dstSock;
        u32 dst;

        p->body.wireInit.portType = 3;
        addr4 = DWCi_natNegServerAddr2;
        SOAddressToString(&addr4);
        dstSock = node->fd;
        dst = DWCi_natNegServerAddr2;
        sa.family = 2;
        sa.port = SOHtoNs(27901);
        sa.addr = dst;
        DWCi_socketSendTo(dstSock, p, len, 0, &sa, 8);
    }
    node->deadline = DWCi_getTick() + 500;
    node->retryLimit = 10;
}

/* 0x805131B0 (0x108): keep the two idle sockets alive - poll them at most every 10 s, and when a
 * negotiation completes publish the NAT type and mapping scheme it worked out and close both. */
/* `DWCi_natNegPollRepliesOnce` is a 4-byte tail thunk onto `DWCi_natNegPollReplies`; the target
 * keeps the `bl` to the thunk, so the per-function inliner is turned off here instead of changing
 * the unit's flags (playbook 61). */
#pragma dont_inline on
u32 DWCi_natNegTickIdleSockets(s32 sock) {
    u32 ok;
    u32 report;

    ok = 1;
    if (sock != -1) {
        ok = ((u32)(DWCi_getTick() - DWCi_natNegLastPollTick) < 10000)
                 ? DWCi_natNegPollRepliesOnce(sock, (struct DWCiNatNegSession*)DWCi_natNegSession)
                 : 0;
        if (ok == 0) {
            report = DWCi_natNegDetermineNatType((struct DWCiNatNegSession*)DWCi_natNegSession);
            {
                struct DWCiNatNegSession copy;
                copy = *(struct DWCiNatNegSession*)DWCi_natNegSession;
                DWCi_natNegPollCallback(report, &copy);
            }
            DWCi_natNegNatType = ((struct DWCiNatNegSession*)DWCi_natNegSession)->natType;
            DWCi_natNegMappingScheme = ((struct DWCiNatNegSession*)DWCi_natNegSession)->mappingScheme;
            if (DWCi_natNegIdleSocketA != -1) {
                DWCi_socketClose(DWCi_natNegIdleSocketA);
            }
            DWCi_natNegIdleSocketA = -1;
            if (DWCi_natNegIdleSocketB != -1) {
                DWCi_socketClose(DWCi_natNegIdleSocketB);
            }
            DWCi_natNegIdleSocketB = -1;
        }
    }
    return ok;
}
#pragma dont_inline off

/* 0x805132C0 (0x31C): begin a negotiation - resolve the three servers (a configured name, or the
 * game name joined to the built-in host, and a resolver fallback), then append and open a socket
 * record and send its INIT probes.  See the header for the return codes. */
/* untyped: caller-owned payload - handed back to both callbacks */
s32 DWCi_NatNegStartSession(s32 gameFd, s32 cookie, s32 clientIndex, DWCiCallback progress,
                            DWCiCallback complete, void* userData) {
    char hostBuf2[64];
    char hostBuf1[64];
    char hostBuf0[64];
    DWCiNatNegSocket rec;
    DWCiNatNegSocket* node;
    char* name;
    char* fallback;
    DWCiHostEntry* host;
    u32 addr;
    s32 ok;
    s32 i;
    char (*hostNames)[28] = DWCi_natNegStringPool;

    if (DWCi_natProbeStatus != 1) {
        return 2;
    }
    if (DWCi_natNegServerAddr0 == 0) {
        name = DWCi_natNegServerName0;
        fallback = hostNames[0];
        if (name == 0) {
            snprintf(hostBuf0, 64, DWCi_natNegHostFormat, DWCi_natNegGameName, fallback);
            name = hostBuf0;
        }
        addr = DWCi_socketResolveAddress(name);
        if (addr == (u32)-1) {
            host = DWCi_socketLookupHost(name);
            if (host == 0) {
                addr = 0;
            } else {
                addr = (*host->hosts)->addr;
            }
        }
        DWCi_natNegServerAddr0 = addr;
    }
    if (DWCi_natNegServerAddr1 == 0) {
        name = DWCi_natNegServerName1;
        fallback = hostNames[1];
        if (name == 0) {
            snprintf(hostBuf1, 64, DWCi_natNegHostFormat, DWCi_natNegGameName, fallback);
            name = hostBuf1;
        }
        addr = DWCi_socketResolveAddress(name);
        if (addr == (u32)-1) {
            host = DWCi_socketLookupHost(name);
            if (host == 0) {
                addr = 0;
            } else {
                addr = (*host->hosts)->addr;
            }
        }
        DWCi_natNegServerAddr1 = addr;
    }
    if (DWCi_natNegServerAddr2 == 0) {
        name = DWCi_natNegServerName2;
        fallback = hostNames[2];
        if (name == 0) {
            snprintf(hostBuf2, 64, DWCi_natNegHostFormat, DWCi_natNegGameName, fallback);
            name = hostBuf2;
        }
        addr = DWCi_socketResolveAddress(name);
        if (addr == (u32)-1) {
            host = DWCi_socketLookupHost(name);
            if (host == 0) {
                addr = 0;
            } else {
                addr = (*host->hosts)->addr;
            }
        }
        DWCi_natNegServerAddr2 = addr;
    }
    ok = (DWCi_natNegServerAddr0 != 0 && DWCi_natNegServerAddr1 != 0 && DWCi_natNegServerAddr2 != 0);
    if (ok == 0) {
        return 3;
    }
    memset(&rec, 0, 84);
    if (DWCi_natNegSocketList == 0) {
        DWCi_natNegSocketList = DWCi_listCreate(84, 4, 0);
    }
    DWCi_listAppend(DWCi_natNegSocketList, &rec);
    node = (DWCiNatNegSocket*)DWCi_listItem(DWCi_natNegSocketList, DWCi_listCount(DWCi_natNegSocketList) - 1);
    if (node == 0) {
        return 1;
    }
    node->gameFd = gameFd;
    node->clientIndex = clientIndex;
    node->token = cookie;
    node->progressCb = progress;
    node->completeCb = complete;
    node->userData = userData;
    node->fd = DWCi_socketCreate(2, 2, 17);
    node->retryCount = 0;
    node->gotYourData = 0;
    node->finished = 0;
    node->peerAddr = 0;
    node->peerPort = 0;
    node->retryLimit = 0;
    node->result = 5;
    if (node->fd == -1) {
        for (i = 0; i < (s32)DWCi_listCount(DWCi_natNegSocketList); i++) {
            if (node == (DWCiNatNegSocket*)DWCi_listItem(DWCi_natNegSocketList, i)) {
                DWCi_listDeleteAt(DWCi_natNegSocketList, i);
                break;
            }
        }
        return 2;
    }
    DWCi_natNegSendInit(node);
    return 0;
}

/* 0x805135E0 (0xAC): close the negotiator socket registered for `session` and mark it finished. */
void DWCi_NatNegEndSession(u32 session) {
    DWCi_natNegCloseByToken(session);
}

/* Pings the peer (packet type 7) with what this side knows - whether the peer's own ping reached us
 * and whether the negotiation is past its probing - and re-arms the retry timer. */
static inline void DWCi_natNegSendPing(DWCiNatNegSocket* node) {
    DWCiNatNegPacket pkt;
    u32 addr;

    memcpy(pkt.magic, natNegMessageMagic, 6);
    pkt.version = 3;
    pkt.type = 7;
    pkt.cookie = DWCi_peerTokenFromSocket(node->token);
    pkt.body.connect.remoteAddr = node->peerAddr;
    pkt.body.connect.remotePort = SOHtoNs(node->peerPort);
    pkt.body.connect.gotYourData = node->gotYourData;
    pkt.body.connect.finished = (node->state != 2);
    addr = node->peerAddr;
    SOAddressToString(&addr);
    DWCi_natNegSendTo((node->gameFd != -1) ? node->gameFd : node->fd, &pkt, 20, node->peerAddr, node->peerPort);
    node->deadline = DWCi_getTick() + 700;
    node->retryLimit = 7;
    if (node->gotYourData != 0) {
        node->finished = 1;
    }
}

/* 0x80513690 (0x514): one step of a negotiator socket - keep the idle sockets polled, drop a closed
 * record from the list, feed every waiting datagram to the packet router, then act on the timers of
 * the current state: probe again or give up (0, 2), announce the peer (3), report the server
 * answers (1) and repeat or abandon the report (5). */
void DWCi_natNegTickSocket(DWCiNatNegSocket* node) {
    int len;
    DWCiSockAddrIn from;
    s32 n;
    s32 i;

    len = 8;
    if (DWCi_natNegIdlePolling != 0) {
        DWCi_natNegIdlePolling = DWCi_natNegTickIdleSockets(DWCi_natNegIdleSocketA);
        DWCi_natNegIdlePolling = DWCi_natNegTickIdleSockets(DWCi_natNegIdleSocketB);
    }
    if (node == 0) {
        return;
    }
    if (node->state == 4) {
        for (i = 0; i < (s32)DWCi_listCount(DWCi_natNegSocketList); i++) {
            if (node == (DWCiNatNegSocket*)DWCi_listItem(DWCi_natNegSocketList, i)) {
                DWCi_listDeleteAt(DWCi_natNegSocketList, i);
                return;
            }
        }
        return;
    }
    if (node->fd != -1) {
        while (node->state != 4 && node->fd != -1 && DWCi_socketHasData(node->fd) != 0) {
            n = DWCi_socketRecvFrom(node->fd, DWCi_natNegSocketRecvBuffer, 512, 0, &from, &len);
            if (n == -1) {
                DWCi_socketGetLastError(node->fd);
                break;
            }
            DWCi_NatNegSendPacket(DWCi_natNegSocketRecvBuffer, n, &from);
        }
    }
    if (node->state == 0 || node->state == 2) {
        if (DWCi_getTick() > node->deadline) {
            if (node->retryCount > node->retryLimit) {
                if (node->state == 0) {
                    DWCi_natNegSetSocket(node, 2, -1, 0);
                } else {
                    DWCi_natNegSetSocket(node, 3, -1, 0);
                }
            } else {
                node->retryCount++;
                if (node->state == 0) {
                    DWCi_natNegSendInit(node);
                } else {
                    DWCi_natNegSendPing(node);
                }
            }
        }
    }
    if (node->state == 3) {
        if (DWCi_getTick() > node->deadline) {
            DWCiSockAddrIn sa;

            sa.family = 2;
            sa.port = SOHtoNs(node->peerPort);
            sa.addr = node->peerAddr;
            if (node->gameFd == -1) {
                DWCi_natNegSetSocket(node, 0, node->fd, &sa);
            } else {
                DWCi_natNegSetSocket(node, 0, node->gameFd, &sa);
            }
        }
    }
    if (node->state == 1) {
        if (DWCi_getTick() > node->deadline) {
            DWCi_natNegSetSocket(node, 1, -1, 0);
        }
    }
    if (node->state == 5) {
        if (DWCi_getTick() > node->deadline) {
            if (node->retryCount > node->retryLimit) {
                node->completeCb(node->result, node->resultArg, &node->resultAddr, node->userData);
                if (node->gameFd == -1) {
                    node->fd = -1;
                }
                DWCi_natNegCloseByToken(node->token);
            } else {
                DWCi_natNegSendReport(node);
                node->retryCount++;
                node->deadline = DWCi_getTick() + 1000;
            }
        }
    }
}

/* 0x80513BB0 (0x74): step every negotiator socket, newest first; with no list, step the idle one. */
void DWCi_NatNegProcess(void) {
    s32 i;

    if (DWCi_natNegSocketList == 0 || DWCi_listCount(DWCi_natNegSocketList) == 0) {
        DWCi_natNegTickSocket(0);
    } else {
        for (i = (s32)DWCi_listCount(DWCi_natNegSocketList) - 1; i >= 0; i--) {
            DWCi_natNegTickSocket((DWCiNatNegSocket*)DWCi_listItem(DWCi_natNegSocketList, i));
        }
    }
}

/* 0x80513C30 (0x24C): a CONNECT (type 5) reached a socket record - acknowledge it (type 6) unless it is
 * the peer's final one, then either finish the negotiation with the result the packet carries or note
 * the peer's address, tell the progress callback and start pinging. */
void DWCi_natNegOnConnect(DWCiNatNegSocket* node, DWCiNatNegPacket* msg, DWCiNatNegHeader* sender) {
    u32 addr;
    u32 result;

    addr = msg->body.connect.remoteAddr;
    SOAddressToString(&addr);
    SOAddressToHostPort(msg->body.connect.remotePort);
    if (msg->body.connect.finished == 0) {
        DWCiNatNegPacket ack;

        memcpy(ack.magic, natNegMessageMagic, 6);
        ack.version = 3;
        ack.type = 6;
        ack.cookie = DWCi_peerTokenFromSocket(node->token);
        ack.body.init.clientIndex = node->clientIndex;
        DWCi_natNegSendTo(node->fd, &ack, 21, sender->addr, SOAddressToHostPort(sender->port));
    }
    if (node->state < 2) {
        if (msg->body.connect.finished != 0) {
            result = 4;
            if (msg->body.connect.finished == 1) {
                result = 1;
            } else if (msg->body.connect.finished == 2) {
                result = 2;
            }
            DWCi_natNegSetSocket(node, result, -1, 0);
        } else {
            node->peerAddr = msg->body.connect.remoteAddr;
            node->peerPort = SOAddressToHostPort(msg->body.connect.remotePort);
            node->retryCount = 0;
            node->state = 2;
            node->progressCb(2, node->userData);
            DWCi_natNegSendPing(node);
        }
    }
}

/* 0x80513E80 (0x378): a CONNECT_PING (type 7) reached a socket record - remember the peer's address, and
 * ping back until the peer has our data and the negotiation can move on to state 3. */
void DWCi_natNegOnConnectPing(DWCiNatNegSocket* node, DWCiNatNegPacket* msg, DWCiNatNegHeader* sender) {
    u32 addr;

    if (node->state >= 2) {
        addr = sender->addr;
        SOAddressToString(&addr);
        SOAddressToHostPort(sender->port);
        node->peerAddr = sender->addr;
        node->peerPort = SOAddressToHostPort(sender->port);
        node->gotYourData = 1;
        if (msg->body.connect.gotYourData == 0) {
            DWCi_natNegSendPing(node);
        } else if (node->state == 2) {
            if (node->finished == 0) {
                DWCi_natNegSendPing(node);
            }
            node->state = 3;
            node->deadline = DWCi_getTick() + 5000;
        } else if (msg->body.connect.finished == 0) {
            DWCi_natNegSendPing(node);
        }
    }
}

/* 0x80514200 (0x1FC): the replies that address a socket record by cookie and are not pings - the
 * server's acknowledgement of a probe (1), a peer's test packet to bounce back (2) and the server's
 * acknowledgement of the report (14). */
void DWCi_natNegOnReply(DWCiNatNegSocket* node, DWCiNatNegPacket* msg, DWCiNatNegHeader* sender) {
    switch (msg->type) {
    case 1:
        if (msg->body.init.portType <= 3) {
            node->initAck[msg->body.init.portType] = 1;
            if (node->state == 0 && node->initAck[1] != 0 && node->initAck[2] != 0 && node->initAck[3] != 0
                && (node->gameFd == -1 || node->initAck[0] != 0)) {
                node->state = 1;
                node->deadline = DWCi_getTick() + 60000;
                node->progressCb(node->state, node->userData);
            }
        }
        break;
    case 2:
        msg->type = 3;
        DWCi_natNegSendTo(node->fd, msg, 21, sender->addr, SOAddressToHostPort(sender->port));
        break;
    case 14:
        node->state = 6;
        node->completeCb(node->result, node->resultArg, &node->resultAddr, node->userData);
        if (node->gameFd == -1) {
            node->fd = -1;
        }
        DWCi_natNegCloseByToken(node->token);
        break;
    }
}

/* 0x80514400 (0x1B8): route a framed NATNEG message to the negotiator socket its peer token maps
 * to: message type 5 goes to the CONNECT path, 7 to the CONNECT_PING one, everything else to the
 * generic reply handler.  The header argument is the sender's address, read only. */
/* untyped: caller-owned payload - the framed header the GameSpy caller builds as a C++ class */
void DWCi_NatNegSendPacket(void* data, u32 size, void* header) {
    DWCiNatNegPacket msg;
    s32 same;

    same = (memcmp((u8*)data, natNegMessageMagic, 6) == 0);
    if (same) {
        u32 addr;
        u32 type;

        type = ((u8*)data)[7];
        addr = ((DWCiNatNegHeader*)header)->addr;
        SOAddressToString(&addr);
        SOAddressToHostPort(((DWCiNatNegHeader*)header)->port);
        if (type == 5 || type == 7) {
            DWCiNatNegSocket* node;

            if ((s32)size >= 20) {
                memcpy(&msg, (u8*)data, 20);
                node = DWCi_natNegFindByToken(DWCi_socketTokenFromPeer(msg.cookie));
                if (node != 0) {
                    if (type == 5) {
                        DWCi_natNegOnConnect(node, &msg, (DWCiNatNegHeader*)header);
                    } else {
                        DWCi_natNegOnConnectPing(node, &msg, (DWCiNatNegHeader*)header);
                    }
                }
            }
        } else {
            DWCiNatNegSocket* node;

            if ((s32)size >= 21) {
                memcpy(&msg, (u8*)data, 21);
                node = DWCi_natNegFindByToken(DWCi_socketTokenFromPeer(msg.cookie));
                if (node != 0) {
                    DWCi_natNegOnReply(node, &msg, (DWCiNatNegHeader*)header);
                }
            }
        }
    }
}
