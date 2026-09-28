/*
 * DWCi_NatNeg.c - the DWCi band's tail, `.text` 0x80512490..0x805145B8 (17 functions, 8488 B).
 *
 * REGISTRATION (recon lane, 2026-09-27).  Left edge 0x80512490 is the seam with the registered
 * the sibling `DWCi_sendControlFrame` unit: `tudiscover.py at 0x80512490` reports `cut 18642 0x80512490 strong x2`
 * (two independent .sdata run jumps, DWCi_addressFormatPort -> DWCi_emptyString and DWCi_emptyString -> 0x80794370,
 * intersecting at the single cut 18642), so a TU begins at 0x80512490 and the registered unit's right
 * edge was moved down from 0x805124F4 to 0x80512490 (the 0x64 overlap is exactly the seam function).
 * Right edge 0x805145B8 is a HARD instruction-level cut: every function in 0x80512490..0x805145B8
 * starts 16-byte aligned (17/17, `gap_*` zero runs between neighbours) while 0x805145B8..0x8051B7FC
 * packs on 4 (116 of 145 starts are not 16-aligned) - the DWCi/NHTTP library and flag boundary.
 * Re-confirmed from the data this pass: the two `"%s:%d"` copies at 0x80794358 (referenced by
 * `DWCi_formatAddress`, 0x80512210) and 0x80794370 (referenced by `DWCi_natNegFormatAddress`,
 * 0x80512500) cannot be one TU's - `-str reuse` is in `cflags_dwc` and merges identical literals
 * inside one TU - and this unit's own `.sdata` run (0x80794368 empty string, 0x80794370/78/7C
 * formats, 0x80794380 signature, 0x80794388/8C the two socket fds, 0x80794390 the protocol id,
 * 0x80794394 the "%s.%s" host format) is contiguous and ascending in first-reference order.
 *
 * MODULE.  `DWCi`, not NHTTP.  The task's inventory placed NHTTP's left cut at 0x80512490; that cut
 * is real but it is the DWCi library's own seam.  This range's functions call only the DWCi transport
 * helpers at 0x8050A..-0x8050E.. (`DWCi_listCount`, `DWCi_socketClose`, `DWCi_socketSendTo`,
 * `DWCi_socketTokenFromPeer`, `DWCi_natNegFormatAddress`, `DWCi_getTick`/`SOHtoNs`) and never any
 * NHTTP entry point, while NHTTP (0x805145B8+) calls `NCDGetCurrentIpConfig`/`SOClose`/`SSLShutdown`
 * and no 0x8050A.. helper.  Its `.data` block 0x806308E8 is the GameSpy NAT-negotiator's own pool
 * ("natneg1/2/3.gs.nintendowifi.net", "Sending PING to %s:%d", "REPORT retry FAILED...", "Removing
 * canceled negotiator"), loaded by `DWCi_NatNegStartSession`.
 *
 * NAME.  `DWCi_NatNeg.c` is an evidence call, not a map name: `dumpmap.py lookup` answers only
 * `zz_0512xxx_` for this range's code.  The NATNEG vocabulary above is the strongest content signal;
 * the range also holds the DWCi address-format helpers ("%s:%d", "%s.%s", the "%s" ring buffer) that
 * the negotiator and the transport share.  Rename if the SDK's real file name is recovered.
 *
 * SECTIONS.  `.text` plus the three data runs the next paragraph lists; every other
 * `.bss`/`.sbss`/`.sdata` object it loads stays declared, never defined - the ones the data pass
 * made this unit's own are declared in `include/DWCi/DWCi_NatNeg.h` (rule 2: the owner's header,
 * which the band header and `include/unsplit/Network.h` include), the rest in
 * `include/unsplit/DWCi.h`.
 *
 * DATA CLAIMED (2026-09-28, rule 12).  Three runs of the band's own data.  One per section, and the
 * `.bss` one widened to a single run, because a second run of one section whose gap no registered unit
 * owns makes `dtk dol split` die with `Cyclic dependency encountered while resolving link order`
 * (playbook 53):
 *   .sdata 0x80794368..0x807943A0 - the unit's own pool: the empty string, the "%s:%d"/"%s"/":%d"
 *     trio, the 8-byte NATNEG message signature, the protocol id and the two idle-socket fds.  Three
 *     are stored by this object; the rest are `const` strings it reads, and this object is their
 *     **only** referencer among the current link inputs - which is the definer test for a literal
 *     (the store test does not apply to a read-only pool).
 *   .sbss 0x80795828..0x80795878 - the address-ring index, the four ready flags, the seen-type flags,
 *     the socket list and the server address/token words (all stored here) through the last-poll tick
 *     and the poll callback (read here, referred to by nothing else in the current link set).
 *     The run stops at 0x80795878 because `NHTTP/d_nhttp.c` owns 0x80795878; it starts at
 *     0x80795828 because 0x80795820 `DWCi_addressRingIndex` is `DWCi/fn_805113B0.c`'s (that symbol
 *     appears in that object's relocations and no other's) - a single 0x80795818..0x80795878 claim
 *     would sweep another unit's word.
 *   .bss 0x807614D8..0x80762A20 - ONE run, deliberately wider than this unit's three words: the
 *     0x80-byte host-name text at 0x807614D8 (read at 15 sites, written nowhere in the link set),
 *     the second address ring and the 0xE0-byte session record at 0x80762900/0x80762940.  The three
 *     are 0x1520 bytes apart with no registered owner between them - 0x80761558 and 0x807615C0 (a
 *     0x1000-byte block) belong to the *unregistered* DWCi band 0x80509DB0..0x805113B0, 0x807625C0
 *     `DWCi_addressRing` to `DWCi/fn_805113B0.c` and 0x807625F0 to another unregistered band - so this
 *     is playbook 53's "must own the bytes between them": claiming the three as separate runs leaves
 *     an `auto_08_*` band inside this unit's `.bss` and `dtk dol split` dies with `Cyclic dependency
 *     encountered while resolving link order` (measured, verbatim).  RE-DRAW THIS RANGE when
 *     0x80509DB0..0x805113B0 is registered: 0x80761558, 0x807615C0 and 0x807625F0 are that unit's, and
 *     0x807625C0 is `DWCi/fn_805113B0.c`'s.  The thin claim that would be right today cannot be
 *     expressed (one run per section), which is the residual this claim carries.
 *
 * FLAGS.  Copies `cflags_dwc` (`-func_align 4`); the 16-alignment finding above says the original TU
 * was `-func_align 16`, recorded for the flip pass (same note as `DWCi_Np_CPUCopyFast.c`).
 *
 * NAMING (rule 7, no exemption).  The unit owns 17 functions.  Five are named from the GameSpy
 * interface's call sites (`include/DWCi/DWCi_NatNeg.h`: `DWCi_NatNegStartSession`,
 * `DWCi_NatNegEndSession`, `DWCi_NatNegCleanup`, `DWCi_NatNegProcess`, `DWCi_NatNegSendPacket`);
 * the rest from their own bodies this pass - `DWCi_GetStringLength`, `DWCi_natNegFormatAddress`,
 * `DWCi_natNegTickSocket`, `DWCi_natNegSendType6`/`SendType7`/`HandleReply` (the message type each
 * one answers).  Every `.sdata`/`.bss`/`.sbss` object the bodies touch was renamed in the map from
 * what the code stores there (`DWCi_natNegSocketList`, `DWCi_natNegType1Seen`/`2`/`3`,
 * `DWCi_natNegSlot0Ready`..`Slot3Ready`, ...).  Criterion, stated once for both DWCi units: no
 * auto-generated name is spelled in source, and a row that no written body reaches keeps its stem -
 * this unit has exactly one, `fn_80512E90` (the other three, `DWCi_GetConsoleFriendCode`,
 * `DWCi_authDataTask` and `fn_80509270`, sit in `DWCi_Np_CPUCopyFast.c`'s range, which states the same
 * criterion).  All
 * names are GUESSes where the image gives no real spelling - the runtime dump answers `zz_0512xxx_`
 * for every row - and a later pass may refine them.
 *
 * BODIES (this pass).  Nine of the 17 functions are written, measured with `recompile.py
 * --main . --measure <symbol>` against this worktree's target object; the unit is 19.5839 % over its
 * 8372 B of `.text` (was 1.1945 % before the pass).  Byte-identical: `DWCi_GetStringLength` (100),
 * `DWCi_NatNegCleanup` (100), `DWCi_NatNegProcess` (100), `DWCi_natNegPollRepliesOnce` (100).
 *   DWCi_natNegTickIdleSockets  98.4848  264 B
 *   DWCi_natNegFormatAddress    94.5614  228 B   (the third copy of the "%s:%d" formatter; the
 *                                                 frame is 0x20 on both sides and ours is one word
 *                                                 short: the target keeps `beq`/`mr r31,r5`/`b` to
 *                                                 the join where ours falls through on `bne` - a
 *                                                 source-shape hint, not a frame difference)
 *   DWCi_natNegSetSocket        91.1440  500 B
 *   DWCi_NatNegEndSession       84.6977  172 B
 *   DWCi_NatNegSendPacket       66.0364  440 B
 *
 * Residuals, all measured, none a seam or type error:
 *   - `DWCi_NatNegEndSession` (84.6977): 43 instructions on both sides and the same sequence; the
 *     original colours the parameter into r30 and reuses r31 for both the loop index and the found
 *     record, while our build needs a third callee-saved register (frame 0x20 vs the target's 0x10).
 *     Two declaration orders were tried (`i` before `node` and after it) - both measure 84.6977.
 *   - `DWCi_NatNegSendPacket` (66.0364): 109 instructions against 110.  Two of the differences are
 *     settled and kept - the signature test is written as an assigned boolean (`s32 same =
 *     (memcmp(...) == 0)`), which is the only shape that reproduces the target's
 *     `cntlzw`/`srwi.`/`beq` instead of `cmpwi`/`bne`, and the local message buffer is 0x40 bytes,
 *     which reproduces the target's 0x70-byte frame exactly (`u8 msg[21]` gives 0x40).  What is
 *     left is register colouring: the target keeps data/size/header/type in r31-r28 and lets the
 *     per-branch token/index/record webs reuse size's and data's registers, ours keeps six
 *     callee-saved webs and so saves through `_savegpr_`.  A local declared inline instead of
 *     assigned from the parameter (`(u8*)data` at each use) measured 64.45 and was reverted.
 *   - `DWCi_natNegSetSocket` (91.1440): 122 instructions against 125; the message build and the
 *     send match, the list walk's index/record/token colouring does not (same class as EndSession).
 *   - `DWCi_natNegTickIdleSockets` (98.4848): one word short (ours 0x104, the target's 0x108); the
 *     missing word sits between the session address pair at +0x06a and the callback's SDA21 at
 *     +0x090, and every reloc from there on is 4 bytes late.  The call site was the real defect and
 *     is fixed: MWCC had auto-inlined the 4-byte thunk `DWCi_natNegPollRepliesOnce` (ours emitted
 *     `bl DWCi_natNegPollReplies` where the target has `bl DWCi_natNegPollRepliesOnce`), so the
 *     function is wrapped in `#pragma dont_inline on`/`off` and our `+0x044 bl
 *     DWCi_natNegPollRepliesOnce` now carries the target's relocation - the per-symbol reloc
 *     multiset (20 entries, names and kinds) is equal on both sides.
 *   - `DWCi_natNegPollReplies` (unwritten) reaches the NATNEG signature **absolutely** - `lis`/`addi`
 *     at the target's `.text` +0x1fe/+0x242, the only two `R_PPC_ADDR16_HA`/`_LO` sites on that
 *     symbol in this object, where its other ten sites are `R_PPC_EMB_SDA21`.  A sized declaration
 *     always yields the SDA form and an unsized one always the ADDR16 form in this toolchain and
 *     these flags (probe, measured), so one declaration cannot produce both: before writing that
 *     body, test whether the function came from a different original TU (a seam near 0x805125F0 -
 *     `tudiscover`/`splits.txt`) instead of rewriting its source shape.
 *   - Unwritten (no body), with sizes: `DWCi_natNegPollReplies` (0x390), `DWCi_natNegFillAddressTable`
 *     (0x2C0), `fn_80512E90` (0x320), `DWCi_NatNegStartSession` (0x31C), `DWCi_natNegTickSocket`
 *     (0x514), `DWCi_natNegSendType6` (0x24C), `DWCi_natNegSendType7` (0x378),
 *     `DWCi_natNegHandleReply` (0x1FC).  All eight are dense state machines over the same session
 *     record and the band's socket helpers; the record's and the session's layouts are now modelled
 *     (`DWCiNatNegSocket`, `DWCiNatNegSession`, `DWCiNatNegAddr`), which is the groundwork the next
 *     pass needs, and they are the next pass's work in that address order.
 *   - DATA still unclaimed: `.sdata` 0x80794368..0x8079439A, `.bss` 0x807614D8 / 0x80762700 /
 *     0x80762900..0x80762A20+, and `.sbss` as **two** runs: 0x80795818..0x80795820 (this unit's
 *     `lbl_80795818`, 8 B) and 0x80795828 upward from `DWCi_natNegAddressRingIndex` to
 *     `lbl_80795878`.  The 8 bytes between them, 0x80795820 `DWCi_addressRingIndex`, belong to the
 *     transport unit `DWCi/fn_805113B0` - that symbol appears in `fn_805113B0.o`'s relocations and
 *     in no other object's - so a single 0x80795818..0x80795878 claim would sweep another unit's
 *     object, while a gap left inside one claimed range becomes an `auto_*` unit in the middle of
 *     this one (playbook 53).  Our object does not emit any of it yet (the bodies that do are above),
 *     so claiming now would move every un-emitted object (playbook 23/58) - the data pass claims
 *     them once the unit's bodies are complete.
 */

#include "types.h"
#include "DWCi/DWCi_NatNeg.h"            /* the owner's own header: the entry points and the data */
#include "unsplit/DWCi.h"                /* the band's unowned data and helpers (rule 2) */
#include "unsplit/SO.h"                  /* SOAddressToString / SOAddressToHostPort / SOHtoNs */
#include "unsplit/Runtime.PPCEABI.H.h"   /* sprintf / memcmp / memcpy / strlen */

/* The sized form of this unit's own `natNegMessageMagic`.  The owner's header has to carry the
 * *unsized* spelling, because `src/Network/fn_8041A87C.cpp` (which reaches this symbol through
 * `include/unsplit/Network.h`) is ADDR16_HA/LO throughout, and a sized array yields the SDA form.
 * Ten of this unit's own sites need the SDA form, and a declaration after the header's one decides
 * it.  Measured both ways on this unit's rows: unsized alone drops it 19.58385 -> 19.419016, and
 * unsized followed by this line reproduces the sized-only baseline exactly (playbook row 12: the
 * reloc kind is a codegen input). */
extern const u8 natNegMessageMagic[8];

/* --------------------------------------------------------------------------------------------- */
/* Types                                                                                          */
/* --------------------------------------------------------------------------------------------- */

/* The callback `DWCi_natNegSetSocket` stores into a socket record's +0x3C and calls when the
 * record's kind is 1 or 2: `(kind word, its companion word, the record's 8-byte buffer at +0x4C,
 * the record's +0x40)`.  Read out of that function's own indirect call - `mtctr r12`/`bctrl` at
 * 0x80512CF8/0x80512CFC - not invented. */
typedef void (*DWCiNatNegCallback)(u32, u32, u8*, u32);

/* One negotiator socket record - the element type of `DWCi_natNegSocketList`.  Only the fields the
 * written bodies read are named; the gaps are the original's, and the size is the last offset a
 * body reaches, so it is a lower bound (`>= 0x54`) rather than a proven extent. */
typedef struct DWCiNatNegSocket {
    /* +0x00 */ s32 fd;
    /* +0x04 */ u8 pad_0x04[0x04];
    /* +0x08 */ s32 token;      /* DWCi_peerTokenFromSocket(fd); what a reply is routed by */
    /* +0x0C */ s32 sequence;   /* byte copied into the outgoing message's [13] */
    /* +0x10 */ s32 state;      /* 3 = awaiting a reply, 4 = closed, 5 = announce sent */
    /* +0x14 */ u8 pad_0x14[0x10];
    /* +0x24 */ u32 retryCount;  /* cleared when the announce goes out */
    /* +0x28 */ u32 retryLimit;
    /* +0x2C */ u32 deadline;    /* DWCi_getTick() + 1000 at the announce */
    /* +0x30 */ u8 pad_0x30[0x0C];
    /* +0x3C */ DWCiNatNegCallback callback;
    /* +0x40 */ u32 callbackArg;  /* the callback's fourth argument */
    /* +0x44 */ u32 kind;         /* 1/2 tear the record down, anything else announces */
    /* +0x48 */ u32 kindArg;      /* the callback's second argument */
    /* +0x4C */ u8 tag_0x4C[0x08];
} DWCiNatNegSocket; /* size: 0x54 - the last offset a body reaches; a lower bound */

/* One 16-byte entry of the session's address table: the address the server reported and the
 * address this host saw for it, each with its port.  Read out of `DWCi_natNegPollReplies`'s
 * indexing (0x94 + i*16, fields at +0/+4/+8/+12). */
typedef struct DWCiNatNegAddr {
    /* +0x00 */ u32 addr;
    /* +0x04 */ u16 port;
    /* +0x06 */ u16 pad_0x06;
    /* +0x08 */ u32 localAddr;
    /* +0x0C */ u16 localPort;
    /* +0x0E */ u16 pad_0x0E;
} DWCiNatNegAddr; /* size: 0x10 */

/* The negotiator's session record `DWCi_natNegSession`, as the copy in
 * `DWCi_natNegTickIdleSockets` sizes it (216 bytes) and its own fields index it. size: 0xD8 */
struct DWCiNatNegSession {
    /* +0x00 */ u8 pad_0x000[0x80];
    /* +0x80 */ u32 connectFlag;    /* set when the server's type-2 reply arrived */
    /* +0x84 */ u32 connectAckFlag; /* set when its type-3 reply arrived */
    /* +0x88 */ u8 pad_0x088[0x04];
    /* +0x8C */ u32 serverId;
    /* +0x90 */ u32 serverToken;
    /* +0x94 */ DWCiNatNegAddr table[4];
    /* +0xD4 */ u8 pad_0xD4[0x04];    /* the copy in DWCi_natNegTickIdleSockets moves 0xD8 bytes */
}; /* size: 0xD8 */

/* The 6-byte framed header the send layer reads: a reserved halfword, the port and the address, both
 * network order.  Sourced from this unit's own readers - `DWCi_NatNegSendPacket` at 0x80514448
 * `lwz r0,4(r28)` and 0x80514454 `lhz r3,2(r28)`. */
typedef struct DWCiNatNegHeader {
    /* +0x00 */ u16 code;
    /* +0x02 */ u16 port;
    /* +0x04 */ u32 addr;
} DWCiNatNegHeader; /* size: 0x08 */

/* This unit's own bodies that other bodies in this file reach, in address order. */
void DWCi_natNegSetSocket(DWCiNatNegSocket* rec, u32 kind, u32 kindArg, u8* tag);
u32 DWCi_natNegFillAddressTable(struct DWCiNatNegSession* session);
u32 DWCi_natNegTickIdleSockets(s32 sock);
u32 DWCi_natNegPollReplies(s32 sock, struct DWCiNatNegSession* session);
s32 DWCi_natNegPollRepliesOnce(s32 sock, struct DWCiNatNegSession* session);

/* This unit's own entry points that the written bodies reach (rule 2: the owner's own header is the
 * .c itself, because nothing outside this file names them). */
s32 DWCi_natNegTickSocket(DWCiNatNegSocket* node);
void DWCi_natNegSendType6(DWCiNatNegSocket* node, u8* msg, DWCiNatNegHeader* sender);
void DWCi_natNegSendType7(DWCiNatNegSocket* node, u8* msg, DWCiNatNegHeader* sender);
void DWCi_natNegHandleReply(DWCiNatNegSocket* node, u8* msg, DWCiNatNegHeader* sender);


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
char* DWCi_natNegFormatAddress(u32 addr, u32 port, char* buf) {
    if (buf == 0) {
        DWCi_natNegAddressRingIndex ^= 1;
        buf = (char*)(DWCi_natNegAddressRing + DWCi_natNegAddressRingIndex * 0x16);
    }
    if (addr != 0) {
        u32 withPort;
        u32 withoutPort;

        if (port != 0) {
            withPort = addr;
            sprintf(buf, DWCi_natNegAddressFormat, SOAddressToString(&withPort), port);
        } else {
            withoutPort = addr;
            sprintf(buf, DWCi_natNegAddressFormatHost, SOAddressToString(&withoutPort));
        }
    } else {
        if (port != 0) {
            sprintf(buf, DWCi_natNegAddressFormatPort, port);
        } else {
            *buf = 0;
        }
    }
    return buf;
}

/* 0x80512C50 (0x34): destroy the negotiator's socket list and clear its head. */
void DWCi_NatNegCleanup(void) {
    if (DWCi_natNegSocketList != 0) {
        DWCi_listDestroy(DWCi_natNegSocketList);
        DWCi_natNegSocketList = 0;
    }
}

/* 0x805135E0 (0xAC): close the negotiator socket registered for `session` and mark it finished. */
void DWCi_NatNegEndSession(u32 session) {
    DWCiNatNegSocket* node;
    s32 i;

    node = 0;
    if (DWCi_natNegSocketList != 0) {
        for (i = 0; i < (s32)DWCi_listCount(DWCi_natNegSocketList); i++) {
            DWCiNatNegSocket* it = (DWCiNatNegSocket*)DWCi_listItem(DWCi_natNegSocketList, i);
            if (it->token == (s32)session) {
                node = it;
                break;
            }
        }
    }
    if (node != 0) {
        if (node->fd != -1) {
            DWCi_socketClose(node->fd);
        }
        node->fd = -1;
        node->state = 4;
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

/* 0x80514400 (0x1B8): route a framed NATNEG message to the negotiator socket its peer token maps
 * to: message type 5 goes to the type-6 reply path, 7 to the type-7 one, everything else to the
 * generic reply handler.  The header argument is the sender's address, read only. */
/* untyped: caller-owned payload - the framed header the GameSpy caller builds as a C++ class */
void DWCi_NatNegSendPacket(void* data, u32 size, void* header) {
    u8 msg[64];
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
            u32 i;
            u32 token;
            DWCiNatNegSocket* node;

            if ((s32)size >= 20) {
                memcpy(msg, (u8*)data, 20);
                token = *(u32*)&msg[8];
                node = 0;
                if (DWCi_natNegSocketList != 0) {
                    for (i = 0; i < (s32)DWCi_listCount(DWCi_natNegSocketList); i++) {
                        DWCiNatNegSocket* it =
                            (DWCiNatNegSocket*)DWCi_listItem(DWCi_natNegSocketList, i);
                        if (it->token == DWCi_socketTokenFromPeer(token)) {
                            node = it;
                            break;
                        }
                    }
                }
                if (node != 0) {
                    if (type == 5) {
                        DWCi_natNegSendType6(node, msg, (DWCiNatNegHeader*)header);
                    } else {
                        DWCi_natNegSendType7(node, msg, (DWCiNatNegHeader*)header);
                    }
                }
            }
        } else {
            u32 i;
            u32 token;
            DWCiNatNegSocket* node;

            if ((s32)size >= 21) {
                memcpy(msg, (u8*)data, 21);
                token = *(u32*)&msg[8];
                node = 0;
                if (DWCi_natNegSocketList != 0) {
                    for (i = 0; i < (s32)DWCi_listCount(DWCi_natNegSocketList); i++) {
                        DWCiNatNegSocket* it =
                            (DWCiNatNegSocket*)DWCi_listItem(DWCi_natNegSocketList, i);
                        if (it->token == DWCi_socketTokenFromPeer(token)) {
                            node = it;
                            break;
                        }
                    }
                }
                if (node != 0) {
                    DWCi_natNegHandleReply(node, msg, (DWCiNatNegHeader*)header);
                }
            }
        }
    }
}

/* 0x80512980 (0x4): a tail thunk onto `DWCi_natNegPollReplies` - the image keeps the same routine
 * under two addresses, with the arguments passed straight through (GUESS: only the call shape is
 * evidence, and the name says which of the two the caller reached). */
s32 DWCi_natNegPollRepliesOnce(s32 sock, struct DWCiNatNegSession* session) {
    return DWCi_natNegPollReplies(sock, session);
}

/* 0x80512C90 (0x1F4): record the socket's kind and tag, then act on it - kinds 1 and 2 fire the
 * record's own callback and drop the socket out of the negotiator list, anything else announces the
 * socket's address to the server with a 73-byte type-13 message. */
void DWCi_natNegSetSocket(DWCiNatNegSocket* rec, u32 kind, u32 kindArg, u8* tag) {
    rec->kind = kind;
    rec->kindArg = kindArg;
    if (tag != 0) {
        memcpy(rec->tag_0x4C, tag, 8);
    }
    if (kind == 1 || kind == 2) {
        u32 i;
        s32 token;
        DWCiNatNegSocket* node;

        rec->state = 3;
        rec->callback(rec->kind, rec->kindArg, rec->tag_0x4C, rec->callbackArg);
        token = rec->token;
        node = 0;
        if (DWCi_natNegSocketList != 0) {
            for (i = 0; i < (s32)DWCi_listCount(DWCi_natNegSocketList); i++) {
                DWCiNatNegSocket* it =
                    (DWCiNatNegSocket*)DWCi_listItem(DWCi_natNegSocketList, i);
                if (it->token == token) {
                    node = it;
                    break;
                }
            }
        }
        if (node != 0) {
            if (node->fd != -1) {
                DWCi_socketClose(node->fd);
            }
            node->fd = -1;
            node->state = 4;
        }
    } else {
        DWCiSockAddrIn sa;
        u8 msg[73];
        u32 announceAddr;
        u32 port;

        memcpy(msg, natNegMessageMagic, 6);
        msg[6] = 3;
        msg[7] = 13;
        *(u32*)&msg[8] = DWCi_peerTokenFromSocket(rec->token);
        msg[13] = rec->sequence;
        msg[14] = (rec->kind == 0);
        *(u32*)&msg[15] = DWCi_natNegServerId;
        *(u32*)&msg[19] = DWCi_natNegServerToken;
        if (strlen(DWCi_natNegHostText) != 0) {
            memcpy(&msg[23], DWCi_natNegHostText, 50);
        }
        announceAddr = DWCi_natNegServerAddr0;
        SOAddressToString(&announceAddr);
        port = DWCi_natNegServerAddr0;
        sa.family = 2;
        sa.port = SOHtoNs(27901);
        sa.addr = port;
        DWCi_socketSendTo(rec->fd, msg, 73, 0, &sa, 8);
        rec->state = 5;
        rec->deadline = DWCi_getTick() + 1000;
        rec->retryCount = 0;
        rec->retryLimit = 5;
    }
}

/* 0x805131B0 (0x108): keep the two idle sockets alive - poll them at most every 10 s, and when a
 * negotiation completes publish the server's id/token pair and close both. */
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
            report = DWCi_natNegFillAddressTable((struct DWCiNatNegSession*)DWCi_natNegSession);
            {
                struct DWCiNatNegSession copy;
                copy = *(struct DWCiNatNegSession*)DWCi_natNegSession;
                DWCi_natNegPollCallback(report, &copy);
            }
            DWCi_natNegServerId = ((struct DWCiNatNegSession*)DWCi_natNegSession)->serverId;
            DWCi_natNegServerToken = ((struct DWCiNatNegSession*)DWCi_natNegSession)->serverToken;
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
