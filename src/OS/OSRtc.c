/*
 * OS/OSRtc.c - the OS real-time clock and SRAM access: SRAM init/sync, wireless ID and the RTC flags.
 * RANGE. .text 0x804D2B90-0x804D3640 (9 functions); .bss 0x8074D640-0x8074D698.  Cut from the OS core band
 *    0x804C1760-0x804D9B4C.  Evidence: the 0x54-byte control block `Scb` (.bss 0x8074D640) is read by
 *    `fn_804D2B90`, `__OSInitSram`, `UnlockSram`, `__OSSyncSram`, `OSGetWirelessID` and `OSSetWirelessID`, and by
 *    nothing outside the range.
 * FLAGS. `cflags_os` (configure.py), the flags of the unit it was cut from; unchanged by the cut and not measured
 *    with bodies yet.
 * NAMES. `__OSInitSram`, `UnlockSram`, `__OSSyncSram`, `OSGetWirelessID`, ... are the map's names.
 * RESIDUALS. every body is unwritten.
 * SHAPES. none yet.
 */
