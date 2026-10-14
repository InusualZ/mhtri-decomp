/*
 * BTE/bta_dm_api.c - the BTA device manager API: BTA_EnableBluetooth and BTA_DisableBluetooth, device state, name
 *   and visibility, search and cancel, PIN reply and adding and removing devices.
 *
 * RANGE. .text 0x8047D690-0x8047DAFC (11 functions, 0x46C B); .sdata2 0x8079D0A0-0x8079D0B0. Strong: the BTA_Dm*
 *   run; every function builds a GKI message and posts it through bta_sys_sendmsg. 0x8047DAC4 (event 0x10C) is the
 *   last poster before bta_dm_sm_execute. The two .sdata2 doubles are read only by BTA_EnableBluetooth.
 * FLAGS. the `OS` lib default `cflags_os`, as BTE/gki_time.c (registered flags; codegen evidence in BTE/sdp_* points to
 *   GC/3.0a5.2 + cflags_nw4r (unmeasured here)).
 * NAMES. the map's. GUESS BTA_DisableBluetooth (0x8047D718) is marked in BTE/gki_time.c's header, where it was
 *   named. Unnamed row: 0x8047DAC4 posts BTA_DM event 0x10C with no payload (name not pinned).
 * RESIDUALS. every body is unwritten: 0x8047D690-0x8047DAFC (11 functions).
 * SHAPES. plain C; the BTE declarations the bodies need live in BTE/gki_time.h until the per-owner headers are split.
 */
