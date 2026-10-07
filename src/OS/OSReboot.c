/*
 * OS/OSReboot.c - the OS reboot support: `__OSReboot` and the save-region getter.
 * RANGE. .text 0x804D2160-0x804D21F0 (2 functions); .sbss 0x80795378-0x80795380.  Cut from the OS core band
 *    0x804C1760-0x804D9B4C.  Evidence: `OSGetSaveRegion` reads .sbss 0x80795378/0x8079537C (no other reader);
 *    `OSRegisterShutdownFunction` (0x804D21F0) reads the shutdown queue of OSReset.
 * FLAGS. `cflags_os` (configure.py), the flags of the unit it was cut from; unchanged by the cut and not measured
 *    with bodies yet.
 * NAMES. map names throughout.
 * RESIDUALS. every body is unwritten.
 * SHAPES. none yet.
 */
