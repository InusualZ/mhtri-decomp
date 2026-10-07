/*
 * ESP/esp.c - the SDK ES (title/ticket) proxy library over `/dev/es`.
 *
 * RANGE. `.text` 0x804AF420-0x804AFB50; `.sdata` 0x80793E20-0x80793E30 (10 functions / 0x6E4 B).
 *   - every function reads `__esFd` (.sdata 0x80793E20) and the `/dev/es` string (.sdata 0x80793E28); `ESP_InitLib`
 *     opens the device
 * FLAGS. the `OS` lib block of `configure.py` (`Wii/1.3`, `cflags_os`), the block of the unit this range was cut
 *   from; unmeasured until bodies exist.
 * NAMES. the unit name is a GUESS (the library's first source file).
 * RESIDUALS. No body is written (10 functions), the largest `ESP_GetTicketViews` at 0x804AF570 (0x114 B); `python
 *   tools/units/sweepcomments.py --unit ESP/esp.c` lists them.
 */
