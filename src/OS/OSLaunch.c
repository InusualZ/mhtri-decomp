/*
 * OS/OSLaunch.c - the OS title launcher: argument packing, title loading, `__OSGetValidTicketIndex`,
 *    `__OSRelaunchTitle` and the Shop Channel help launch.
 * RANGE. .text 0x804D71F0-0x804D7FF0 (8 functions); .data 0x80629818-0x80629B38; .sdata 0x80793FD8-0x80793FE0; .sbss
 *    0x80795408-0x80795410.  Cut from the OS core band 0x804C1760-0x804D9B4C.  Evidence: the `__FILE__` anchor
 *    "OSLaunch.c" at .data 0x80629874 (read by `__OSGetValidTicketIndex`) and the "/title/%08x/%08x/data" string
 *    (0x80629818) read by both `fn_804D7370` and `fn_804D7A80` (one string pool); the argument-packing helper
 *    `fn_804D71F0` has a twin at 0x804CDD70.
 * FLAGS. `cflags_os` (configure.py), the flags of the unit it was cut from; unchanged by the cut and not measured
 *    with bodies yet.
 * NAMES. `__OSGetValidTicketIndex`, `__OSRelaunchTitle`, `OSLaunchShopChannelHelp` (a GUESS in the scheme of the
 *    `OSLaunchPDChannel` log text) keep their names.
 * RESIDUALS. every body is unwritten.
 * SHAPES. none yet.
 */
