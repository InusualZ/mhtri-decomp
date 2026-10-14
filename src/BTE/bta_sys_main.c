/*
 * BTE/bta_sys_main.c - the BTA system manager: init, the event dispatcher, the timer update, the subsystem
 *   registration, message send, timer start and stop, disable and the trace level.
 *
 * RANGE. .text 0x8047AEEC-0x8047B130 (9 functions, 0x244 B); .data 0x80612ED0-0x80612F08; .bss 0x807391F8-0x80739288;
 *   .sbss 0x80795038-0x80795040. Strong start: the .data starts after an 8-byte pad (0x80612ECB -> 0x80612ED0) and its
 *   two strings are read only by bta_sys_event; bta_sys_init is Broadcom's first bta_sys_main.c function. Medium end:
 *   ptim_init follows (the ptim_* names); the next .data object (0x80612F08) is read first by
 *   bta_dm_disable_timer_cback.
 *   The control block 0x807391F8 (bta_sys_cb, 0x90 B) is read by BTE/bta_sys_conn.c and this range; the .sbss byte
 *   0x80795038 is written by bta_sys_set_trace_level and read by every BTA function from bta_sys_init on (the trace
 *   level). Both are Broadcom bta_sys_main.c globals; BTE/bd.c and BTE/bta_sys_conn.c carry no data, so the layout is
 *   the same with them in either file. The cfg pointer 0x80793D60 that bta_sys_init and bta_sys_sendmsg read stays in
 *   BTE/gki_time.c (its header has the order evidence).
 * FLAGS. the `OS` lib default `cflags_os`, as BTE/gki_time.c (registered flags; codegen evidence in BTE/sdp_* points to
 *   GC/3.0a5.2 + cflags_nw4r (unmeasured here)).
 * NAMES. the map's. Unnamed row (GUESS, not applied): 0x8047B05C bta_sys_register (stores a handler into the
 *   control block's per-subsystem table).
 * RESIDUALS. every body is unwritten: 0x8047AEEC-0x8047B130 (9 functions).
 * SHAPES. plain C; the BTE declarations the bodies need live in BTE/gki_time.h until the per-owner headers are split.
 */
