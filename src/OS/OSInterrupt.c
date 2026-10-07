/*
 * OS/OSInterrupt.c - the OS interrupt layer: enable/disable/restore, the handler table, mask/unmask and the external-
 *    interrupt entry.
 * RANGE. .text 0x804D0C70-0x804D1440 (11 functions); .data 0x8061D3D0-0x8061D400; .sbss 0x80795358-0x80795370.  Cut
 *    from the OS core band 0x804C1760-0x804D9B4C.  Evidence: `InterruptHandlerTable` (.sbss 0x80795368) and the
 *    last-interrupt record (0x80795358..0x80795364) are read by `__OSInterruptInit`, `fn_804D1140` and
 *    `__OSUnhandledException`; the priority table .data 0x8061D3D0 by `fn_804D1140` only; `__OSModuleInit`
 *    (0x804D1440) is not part of it.
 * FLAGS. `cflags_os` (configure.py), the flags of the unit it was cut from; unchanged by the cut and not measured
 *    with bodies yet.
 * NAMES. map names throughout.
 * RESIDUALS. every body is unwritten.
 * SHAPES. none yet.
 */
