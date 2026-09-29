/*
 * OS/PPCArch.c - the Revolution SDK processor-architecture accessors (PPCArch.c).
 *
 * `.text` 0x804770E0..0x804772F0 (22 functions / 528 B), `.data` 0x80612CE0..0x80612D18 (56 B).
 * Registered by the BTE-region survey (branch `worker/bte-survey-846f`); the 238 KB unclaimed run
 * 0x80475E24..0x804B17D0 has this file as one of its provable interior TUs.
 *
 * Evidence for the name (this is the strongest class the survey found: an already-complete roster).
 *   - every symbol in the range already carries its real SDK name in `symbols.txt` and the list is
 *     exactly the SDK's PPCArch.c, in the SDK's own order: PPCMfmsr, PPCMtmsr, PPCMfhid0, __setHID0,
 *     PPCMfl2cr, PPCMtl2cr, PPCMtdec, PPCSync, PPCHalt, PPCMtmmcr0/1, PPCMtpmc1..4, PPCMffpscr,
 *     PPCMtfpscr, PPCMfhid2, PPCMthid2, PPCMtwpar, PPCDisableSpeculation, PPCSetFpNonIEEEMode,
 *     PPCMthid4.  Not one `fn_XXXXXXXX` needs renaming, so rule 7 is satisfied with no invention;
 *   - the bodies are the SPR accessors themselves (`mfmsr r3; blr`, `mtspr HID0, r3; blr`,
 *     `mtspr L2CR, r3; blr`, `mtdec r3; blr`, `sc; blr`), i.e. the file's own content proves it;
 *   - the range is bounded by non-PPCArch code on both sides: `AXFXGetHooks` (AX) ends at
 *     0x804770D4 (12 B pad) below it, and `fn_804772F0` (544 B, unnamed, not an SPR accessor) starts
 *     above it;
 *   - `PPCMthid4` is the only function in the whole image that reads `lbl_80612CE0` (`callers.py`
 *     reports exactly one address-taken site), and those 56 bytes are the "H4A should not be cleared
 *     because of Broadway errata." string PPCMthid4 hands to `OSReport`.  That is the file's own
 *     `.data` claim, not a shared pool.
 *
 * The `.text` edges are 4-byte aligned claims (`0x804770E0` / `0x804772F0`) and every function start
 * inside is 16-byte aligned, so the source carries `#pragma function_align 16` exactly as the sibling
 * SDK band `AX/AXFXReverbHi.c` and `EXI/ProbeBarnacle.c` do under the same lib block (cflags_os).
 *
 * Bodies are **not written here** - the survey registers the range; the decompiler writes the 22
 * two-to-five-instruction accessors.  Nothing in this file is a body yet, so it is a registration
 * stub, not a partial reconstruction.
 */

/* -O4,p is this lib section's function alignment (16); cflags_os overrides it to 4 for OSAlarm, but
 * every function start in this range is 16-byte aligned (the 8 B pads between bodies). */
#pragma function_align 16

#include "types.h"
