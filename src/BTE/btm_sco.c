/*
 * BTE/btm_sco.c - the BTM SCO/eSCO links: init, connection request/response, connected/removed handlers, link
 *   removal, eSCO parameter changes and the active-link queries.
 *
 * RANGE. .text 0x804870E4-0x80487F1C (15 functions, 0xE38 B); .rodata 0x80573850-0x80573860; .data
 *   0x80614430-0x80614730. The start is pinned by the readers (BTE/btm_pm.c's header). The end is strong: the next
 *   .data starts after a 4-byte pad beyond 4-byte alignment (0x8061472C -> 0x80614730), allowing a cut in
 *   [0x80487DC4, 0x80487F1C]; the BTE source's btm_sco.c ends with btm_is_sco_active_by_bdaddr and btm_sec.c opens
 *   with BTM_SecRegister. Every .data/.rodata object is read only by this range's functions.
 * FLAGS. the `OS` lib default `cflags_os`, as BTE/gki_time.c (registered flags; codegen evidence in BTE/sdp_* points to
 *   GC/3.0a5.2 + cflags_nw4r (unmeasured here)).
 * NAMES. the map's.
 * RESIDUALS. every body is unwritten: 0x804870E4-0x80487F1C (15 functions).
 * SHAPES. plain C; the BTE declarations the bodies need live in BTE/gki_time.h until the per-owner headers are split.
 */
