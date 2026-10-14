/*
 * BTE/hidd_pm.c - the HID device power management: hidd_pm_set_now, the mode-change handler and the inactivity timeout.
 *
 * RANGE. .text 0x8048FD58-0x804900D4 (3 functions, 0x37C B); no data. Medium confidence: the hidd_pm_* family between
 *   hidd_proc_repage_timeout and HID_HostGetSDPRecord; no data on either side.
 * FLAGS. the `OS` lib default `cflags_os`, as BTE/gki_time.c: the function starts of this band are packed to 4 bytes
 *   (the alignment scan of the map's function rows).
 * NAMES. the map's.
 * RESIDUALS. every body is unwritten: 0x8048FD58-0x804900D4 (3 functions).
 * SHAPES. plain C; the BTE declarations the bodies need live in BTE/gki_time.h until the per-owner headers are split.
 */
