/*
 * DWCi_NatNeg.c - the DWCi band's tail, `.text` 0x80512490..0x805145B8 (17 functions, 8488 B).
 *
 * REGISTRATION (recon lane, 2026-09-27).  Left edge 0x80512490 is the seam with the registered
 * `DWCi/fn_805113B0.c`: `tudiscover.py at 0x80512490` reports `cut 18642 0x80512490 strong x2`
 * (two independent .sdata run jumps, lbl_80794364 -> lbl_80794368 and lbl_80794368 -> lbl_80794370,
 * intersecting at the single cut 18642), so a TU begins at 0x80512490 and the registered unit's right
 * edge was moved down from 0x805124F4 to 0x80512490 (the 0x64 overlap is exactly the seam function).
 * Right edge 0x805145B8 is a HARD instruction-level cut: every function in 0x80512490..0x805145B8
 * starts 16-byte aligned (17/17, `gap_*` zero runs between neighbours) while 0x805145B8..0x8051B7FC
 * packs on 4 (116 of 145 starts are not 16-aligned) - the DWCi/NHTTP library and flag boundary.
 *
 * MODULE.  `DWCi`, not NHTTP.  The task's inventory placed NHTTP's left cut at 0x80512490; that cut
 * is real but it is the DWCi library's own seam.  This range's functions call only the DWCi transport
 * helpers at 0x8050A..-0x8050E.. (`fn_8050AC30`, `fn_8050B8C0`, `fn_8050B9E0`, `fn_8050C270`,
 * `fn_8050C450`, `fn_8050C4F0`, `SOHtoNs`) and never any NHTTP entry point, while NHTTP
 * (0x805145B8+) calls `NCDGetCurrentIpConfig`/`SOClose`/`SSLShutdown` and no 0x8050A.. helper; and
 * the seam function is itself called from the 0x8050D.. band (`fn_8050D860`/`D9F0`/`E150`) as well as
 * from the registered unit.  Its `.data` block 0x806308E8 is the GameSpy NAT-negotiator's own pool
 * ("natneg1/2/3.gs.nintendowifi.net", "Sending PING to %s:%d", "REPORT retry FAILED...", "Removing
 * canceled negotiator"), loaded by `fn_805132C0`.
 *
 * NAME.  `DWCi_NatNeg.c` is an evidence call, not a map name: `dumpmap.py lookup` answers only
 * `zz_0512xxx_` for this range's code.  The NATNEG vocabulary above is the strongest content signal;
 * the range also holds the DWCi address-format helpers ("%s:%d", "%s.%s", the "%s" ring buffer) that
 * the negotiator and the transport share.  Rename if the SDK's real file name is recovered.
 *
 * SECTIONS.  `.text` only - the `.bss`/`.sbss`/`.sdata` objects it loads are declared, never defined.
 *
 * FLAGS.  Copies `cflags_dwc` (`-func_align 4`); the 16-alignment finding above says the original TU
 * was `-func_align 16`, recorded for the flip pass (same note as `DWCi_Np_CPUCopyFast.c`).
 *
 * NAMING (rule 7).  The unit owns 17 functions; one has a body here and is named from its code.  The
 * other 16 are still the map's `fn_` placeholders (they are *references* this unit has not
 * reconstructed): `fn_80512C50`, `fn_80512500`, `fn_805125F0`, `fn_80512980`, `fn_80512990`,
 * `fn_80512C90`, `fn_80512E90`, `fn_805131B0`, `fn_805132C0`, `fn_805135E0`, `fn_80513690`,
 * `fn_80513BB0`, `fn_80513C30`, `fn_80513E80`, `fn_80514200`, `fn_80514400`.  Five of them are
 * declared in `include/DWCi/DWCi_NatNeg.h` (this unit's header) for the GameSpy callers.
 *
 * BODY (probe).  `DWCi_GetStringLength` (0x80512490, the seam function) is byte-identical at 100 %.
 */

#include "DWCi/DWCi_NatNeg.h"

extern u8 lbl_80794368[8]; /* .sdata - the empty string */
extern u32 strlen(const char* s);

/* 0x80512490 (0x64): lazy string/length accessor - a NULL buffer slot becomes "", a -1 length slot
 * becomes strlen(s)+1.  NAME (derived): the callers use it to default/normalise a
 * `{ buffer, length }` pair before the transport reads it (fn_8050D860 / fn_8050D9F0 / fn_8050E150 /
 * fn_80511CF0), which is exactly "get the string, and its length, defaulting both". */
void DWCi_GetStringLength(void** buf, int* len) {
    if (*buf == 0) {
        *buf = lbl_80794368;
        *len = 0;
    } else if (*len == -1) {
        *len = strlen((char*)*buf) + 1;
    }
}
