/*
 * NHTTP_os_RVL.c - the Revolution SDK NHTTP library's RVL platform TU, `.text`
 * 0x80515010..0x80515774 (10 functions, 1892 B).
 *
 * REGISTRATION (recon lane, 2026-09-27).  Left edge 0x80515010 is `NHTTPi_CheckCurrentThread`, the
 * first referrer of the TU's private string, and the `.sdata` run-jump interval
 * (`lbl_80794394 -> lbl_807943A0`, cuts [18653,18686]) admits it.  Right edge 0x80515774 is
 * `fn_80515774`, the first referrer of the next TU's `.data` fragment (0x80630B28, `"https://"`,
 * `"CONNECT "`), so the split honours the per-TU data-fragment order; the `.sdata` run jump
 * `lbl_807943A0 -> lbl_807943A8` (cuts [18686,18739]) admits it too.
 *
 * SOURCE FILE.  Real name recovered from the pool: `NHTTPi_CheckCurrentThread` loads
 * `.data:0x80630B18` = `"NHTTP_os_RVL.c"` (group lbl_80630AE8, with the `"NHTTPi_CheckCurrentThread"`
 * and `"%s:illegal thread"` asserts).  The range is the RVL-side thread / receive-buffer helpers
 * (`NHTTPi_CheckCurrentThread`, `NHTTPi_isRecvBufFull`, `fn_805150CC`/`fn_805152C4`/`fn_805153BC`/
 * `fn_805155AC`); the runtime dump names none of them (`zz_0515xxx_`).
 *
 * LIBRARY.  `NHTTP` - the TU's own `__FILE__` string says NHTTP, and its callees are
 * `NHTTPi_lockReqList`/`fn_80514EFC` (same library).
 *
 * SECTIONS.  `.text` only.
 *
 * FLAGS.  `cflags_nhttp`, as `NHTTP_bgnend.c`.
 *
 * BODY.  Not reconstructed (recon deliverable).
 */
typedef unsigned char  u8;
typedef unsigned short u16;
typedef unsigned int   u32;
typedef signed char    s8;
typedef signed short   s16;
typedef signed int     s32;


