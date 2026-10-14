/*
 * BTE/btm_pm.c - the BTM power management: client registration, set/read power mode, the mode state machine, the HCI
 *   mode request and the command status and mode change handlers.
 *
 * RANGE. .text 0x80486450-0x804870E4 (10 functions, 0xC94 B); .rodata 0x80573840-0x80573850; .data
 *   0x806143F0-0x80614430; .sdata2 0x8079D0C0-0x8079D0C8. The start is medium: the .data starts after a 4-byte pad
 *   beyond 4-byte alignment (0x806143EC -> 0x806143F0), allowing a cut in [0x8048610C, 0x80486EBC]; the BTM_Pm*
 *   family and the BTE source order put it at BTM_PmRegister, after btm_init. The end is pinned by the readers:
 *   btm_pm_proc_mode_change (the last function) reads 0x806143F0 and btm_sco_init reads the next .rodata object
 *   0x80573850, so 0x804870E4 is the only cut that keeps both sections in order. The .rodata table is read only by
 *   btm_pm_compare_modes and the .sdata2 words only by BTM_SetPowerMode.
 * FLAGS. the `OS` lib default `cflags_os`, as BTE/gki_time.c (registered flags; codegen evidence in BTE/sdp_* points to
 *   GC/3.0a5.2 + cflags_nw4r (unmeasured here)).
 * NAMES. the map's.
 * RESIDUALS. every body is unwritten: 0x80486450-0x804870E4 (10 functions).
 * SHAPES. plain C; the BTE declarations the bodies need live in BTE/gki_time.h until the per-owner headers are split.
 */
