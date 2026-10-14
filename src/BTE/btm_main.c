/*
 * BTE/btm_main.c - the BTM main module: btm_init and the BTM control block.
 *
 * RANGE. .text 0x804863F4-0x80486450 (1 function, 0x5C B); .bss 0x80739698-0x8073BE60. Medium: btm_init alone,
 *   between btm_inq.c and btm_pm.c in the BTE source order. The .bss is the BTM control block btm_cb (0x27C8 B, read
 *   by every btm unit and by bta/l2c); its owner is a GUESS from the BTE source (btm_main.c defines it) - no other btm
 *   unit has .bss, so any of them gives the same .bss order.
 * FLAGS. the `OS` lib default `cflags_os`, as BTE/gki_time.c (registered flags; codegen evidence in BTE/sdp_* points to
 *   GC/3.0a5.2 + cflags_nw4r (unmeasured here)).
 * NAMES. the map's.
 * RESIDUALS. every body is unwritten: 0x804863F4-0x80486450 (1 function).
 * SHAPES. plain C; the BTE declarations the bodies need live in BTE/gki_time.h until the per-owner headers are split.
 */
