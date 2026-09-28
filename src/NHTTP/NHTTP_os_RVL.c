/*
 * NHTTP_os_RVL.c - the Revolution SDK NHTTP library's RVL platform TU, `.text`
 * 0x80515010..0x80515774 (10 functions, 1892 B).
 *
 * REGISTRATION (recon lane, 2026-09-27).  Left edge 0x80515010 is `NHTTPi_CheckCurrentThread`, the
 * first referrer of the TU's private string, and the `.sdata` run-jump interval
 * (`0x80794394 -> 0x807943A0`, cuts [18653,18686]) admits it.  Right edge 0x80515774 is
 * `0x80515774`, the first referrer of the next TU's `.data` fragment (0x80630B28, `"https://"`,
 * `"CONNECT "`), so the split honours the per-TU data-fragment order; the `.sdata` run jump
 * `0x807943A0 -> 0x807943A8` (cuts [18686,18739]) admits it too.
 *
 * SOURCE FILE.  Real name recovered from the pool: `NHTTPi_CheckCurrentThread` loads
 * `.data:0x80630B18` = `"NHTTP_os_RVL.c"` (group 0x80630AE8, with the `"NHTTPi_CheckCurrentThread"`
 * and `"%s:illegal thread"` asserts).  The range is the RVL-side thread / receive-buffer helpers
 * (`NHTTPi_CheckCurrentThread`, `NHTTPi_isRecvBufFull`, `0x805150CC`/`0x805152C4`/`0x805153BC`/
 * `0x805155AC`); the runtime dump names none of them (`zz_0515xxx_`).
 *
 * LIBRARY.  `NHTTP` - the TU's own `__FILE__` string says NHTTP, and its callees are
 * `NHTTPi_lockReqList`/`0x80514EFC` (same library).
 *
 * SECTIONS.  `.text` only.
 *
 * FLAGS.  `cflags_nhttp`, as `NHTTP_bgnend.c`.
 *
 * BODY (2026-09-27 pass).  Four of the ten functions are reconstructed, all from their own code:
 *
 *   NHTTPi_CheckCurrentThread  (0x80515010, 152 B) - the comm-thread guard; it reports through the
 *       TU's own assert group (`%s:illegal thread` / `NHTTP_os_RVL.c`, `.data` 0x80630AE8) and
 *       panics with .sdata 0x807943A0.  The two arms' shared tail is what the object shows, but the
 *       folded single-condition form measures 61.45 % against the if/else forms' 55.00 %; both are
 *       below the bar and the condition's *polarity* is the reason - retail's `offThread` is true on
 *       the arm that panics when the caller IS the comm thread, which no C spelling of the two
 *       branches reproduces exactly.
 *   NHTTPi_commThreadMain      (0x805150A8, 36 B) - 100 %: runs NHTTPi_commThreadLoop once, answers 0.
 *   NHTTPi_isRecvBufFull       (0x805156F0, 28 B) - 100 %: `info->length <= size`, which MWCC emits
 *       as its branchless unsigned `<=` idiom.
 *   NHTTPi_InitRequestInfo     (0x80515768, 12 B) - 100 %: clears the request-info record.
 *
 * NOT reconstructed, with the reason (each is a stream parser over the same receive ring, and each
 * needs the ring's *full* layout - the linked 512-byte block list behind offset 1024, the ring's
 * index pair and the two out-parameters of the `NHTTPi_SocRecv` wrappers - which the unowned data
 * runs this band reads do not document):
 *   fn_805150CC (504 B)  skips to the next newline in the ring and reports the line's offset
 *   fn_805152C4 (248 B)  the same walk keyed on a space
 *   fn_805153BC (496 B)  the same walk with a 65..90 character test (`li r28,65` seed)
 *   fn_805155AC (324 B)  the ring's buffer-management half
 *   fn_8051570C / fn_8051572C (32 B / 60 B)  two `NHTTPi_SocRecv` wrappers that add an offset to the
 *       ring's base and clamp the length to what is left; they need `NHTTPi_SocRecv`'s signature and
 *       the socket record at `connection+0x2C`, both owned by `d_nhttp.c`, which is still unwritten.
 * Their map rows keep the placeholder names, so this file spells none of them; the unit measures
 * 8.44 % fuzzy over its 1892 B with the four bodies above (272 B ours).
 */
#include "types.h"
#include "NHTTP/NHTTP_os_RVL.h"   /* this unit's own types (rule 1) */
#include "NHTTP/NHTTP_os_RVL.h"   /* this unit's own types (rule 1) */



/* 0x805150A8 (0x24): the comm thread's OS entry point - the request loop runs once and the thread
 * answers 0.  Its argument is the thread's own payload and is not read. */
/* untyped: opaque handle */
void* NHTTPi_commThreadMain(void* arg) {
    (void)arg;
    NHTTPi_commThreadLoop();
    return 0;
}

/* 0x805156F0 (0x1C): true once the receive ring holds at least `size` bytes. */
BOOL NHTTPi_isRecvBufFull(NHTTPRecvBuf* info, u32 size) {
    return info->length <= size;
}

/* 0x80515768 (0xC): clear the request-info record. */
void NHTTPi_InitRequestInfo(NHTTPRequestInfo* info) {
    info->field_0x00 = 0;
}

/* 0x80515010 (0x98): assert the caller's thread.  `offThread` selects which side is legal - with
 * it set the caller must NOT be the comm thread (`NHTTPi_CleanupAsync` passes 1), with it clear the
 * caller must be on it.  Either way the failure path is the TU's `%s:illegal thread` assert. */
void NHTTPi_CheckCurrentThread(NHTTPThreadInfo* info, BOOL offThread) {
    const char* messages = NHTTPi_threadCheckMessages;
    OSThread* current = OSGetCurrentThread();

    if (current != 0) {
        if (offThread ? (current == &info->thread) : (current != &info->thread)) {
            OSReport(messages + 0x1C, messages);
            OSPanic(messages + 0x30, 223, NHTTPi_haltMessage);
        }
    }
}
