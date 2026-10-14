/*
 * BTE/btm_sec.c - the BTM security manager: registration, PIN type and security levels, PIN reply, authorization,
 *   bonding, the L2CAP/multiplexer access checks and the authentication, encryption, link key and PIN code events.
 *
 * RANGE. .text 0x80487F1C-0x8048AEE0 (31 functions, 0x2FC4 B); .data 0x80614730-0x80615140; .sdata2
 *   0x8079D0C8-0x8079D0D8. Strong start: the .data pad seam 0x80614730 (BTE/btm_sco.c's header) and BTM_SecRegister,
 *   its first reader. It ends with btm_read_trusted_mask; BTE/btu_hcif.c follows (its header pins that cut). Every
 *   .data object is read only by this range's functions, and the .sdata2 bytes 0x8079D0C8-0x8079D0CD only by
 *   btm_sec_mkey_comp_event. The 8-byte all-ones address at 0x8079D0D0 is read by bta_dm_disable, bta_dm_keep_acl and
 *   L2CA_SetIdleTimeoutByBdAddr, never by a btm function; its owner is a GUESS from the data order - it follows
 *   btm_sec's .sdata2 and precedes DVD/dvdFatal.c's, so any unit from here to BTE/sdp_utils.c gives the same order,
 *   and BTE/gki_time.c cannot keep it without a second .sdata2 run.
 * FLAGS. the `OS` lib default `cflags_os`, as BTE/gki_time.c (registered flags; codegen evidence in BTE/sdp_* points to
 *   GC/3.0a5.2 + cflags_nw4r (unmeasured here)).
 * NAMES. the map's.
 * RESIDUALS. every body is unwritten: 0x80487F1C-0x8048AEE0 (31 functions).
 * SHAPES. plain C; the BTE declarations the bodies need live in BTE/gki_time.h until the per-owner headers are split.
 */
