/*
 * BTE/ptim.c - the BTA protocol timer service: init, the timer update and timer start and stop.
 *
 * RANGE. .text 0x8047B130-0x8047B310 (4 functions, 0x1E0 B); no data. Medium: the ptim_* run between
 *   bta_sys_set_trace_level (the last BTE/bta_sys_main.c function) and utl_freebuf; no data on either side pins it.
 * FLAGS. the `OS` lib default `cflags_os`, as BTE/gki_time.c (registered flags; codegen evidence in BTE/sdp_* points to
 *   GC/3.0a5.2 + cflags_nw4r (unmeasured here)).
 * NAMES. the map's.
 * RESIDUALS. every body is unwritten: 0x8047B130-0x8047B310 (4 functions).
 * SHAPES. plain C; the BTE declarations the bodies need live in BTE/gki_time.h until the per-owner headers are split.
 */
