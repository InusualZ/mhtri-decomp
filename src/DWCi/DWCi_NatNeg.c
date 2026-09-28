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
 *
 * MODULE.  `DWCi`, not NHTTP.  The task's inventory placed NHTTP's left cut at 0x80512490; that cut
 * is real but it is the DWCi library's own seam.  This range's functions call only the DWCi transport
 * helpers at 0x8050A..-0x8050E.. (`DWCi_listCount`, `DWCi_socketClose`, `DWCi_socketSendTo`, `0x8050C270`,
 * `0x8050C450`, `DWCi_getTick`, `SOHtoNs`) and never any NHTTP entry point, while NHTTP
 * (0x805145B8+) calls `NCDGetCurrentIpConfig`/`SOClose`/`SSLShutdown` and no 0x8050A.. helper; and
 * the seam function is itself called from the 0x8050D.. band (`0x8050D860`/`D9F0`/`E150`) as well as
 * from the registered unit.  Its `.data` block 0x806308E8 is the GameSpy NAT-negotiator's own pool
 * ("natneg1/2/3.gs.nintendowifi.net", "Sending PING to %s:%d", "REPORT retry FAILED...", "Removing
 * canceled negotiator"), loaded by `DWCi_NatNegStartSession`.
 *
 * NAME.  `DWCi_NatNeg.c` is an evidence call, not a map name: `dumpmap.py lookup` answers only
 * `zz_0512xxx_` for this range's code.  The NATNEG vocabulary above is the strongest content signal;
 * the range also holds the DWCi address-format helpers ("%s:%d", "%s.%s", the "%s" ring buffer) that
 * the negotiator and the transport share.  Rename if the SDK's real file name is recovered.
 *
 * SECTIONS.  `.text` plus the three data runs the next paragraph lists; every other
 * `.bss`/`.sbss`/`.sdata` object it loads stays declared in `include/unsplit/DWCi.h`, never defined.
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
 * NAMING (rule 7, no exemption).  The unit owns 17 functions; one has a body here and is named from
 * its code.  The five the GameSpy interface calls are named from those call sites and declared in
 * `include/DWCi/DWCi_NatNeg.h`: `DWCi_NatNegStartSession` (0x805132C0), `DWCi_NatNegEndSession`
 * (0x805135E0), `DWCi_NatNegCleanup` (0x80512C50), `DWCi_NatNegProcess` (0x80513BB0) and
 * `DWCi_NatNegSendPacket` (0x80514400) - all GUESSes, markers for the body pass to confirm.  The
 * other eleven map rows (`0x80512500`, `0x805125F0`, `0x80512980`, `0x80512990`, `0x80512C90`,
 * `0x80512E90`, `0x805131B0`, `0x80513690`, `0x80513C30`, `0x80513E80`, `0x80514200`) have no
 * body and no caller anywhere in `src/`, so there is no reference site to derive a name from; they
 * are unfilled map rows, not identifiers this tree spells.
 *
 * BODY (probe).  `DWCi_GetStringLength` (0x80512490, the seam function) is byte-identical at 100 %.
 */

#include "DWCi/DWCi_NatNeg.h"
#include "unsplit/DWCi.h"                /* the band's `.sdata` empty string (rule 2) */
#include "unsplit/Runtime.PPCEABI.H.h"   /* strlen */

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
