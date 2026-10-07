/*
 * MSL_C/printf.c - the formatted output engine: `parse_format`, `long2str`, `longlong2str`, `double2hex`,
 *    `float2str`, `__pformatter` and the `printf` / `vprintf` / `sprintf` / `snprintf` family with the file and
 *    string writers.
 *
 * RANGE. .text 0x8045BADC..0x8045DFA0 (15 functions in the map, 0x24C4 B); .rodata 0x80572B28..0x80572B50; .data
 *    0x8060EDF8..0x8060F028; .sdata 0x80793CD8..0x80793CE0; .sdata2 0x8079C9F8..0x8079CA00.
 * FLAGS. the `Runtime.PPCEABI.H` lib's `cflags_ppceabi` (the group of the unit it was cut from; unmeasured until
 *    bodies exist).
 * NAMES. file name GUESS (MSL `printf`).
 * EVIDENCE. `.sdata` `@wstringBase0` (0x80793CD8), the string base `.rodata` 0x80572B28 (`@stringBase0`, 0x25 B),
 *    the jump tables `.data` 0x8060EDF8 and 0x8060EED8 and the pooled `0.0` at `.sdata2` 0x8079C9F8 are all read
 *    here; the pool value `0.0` repeats at 0x8079C9A8 (floating-point conversion unit) and 0x8079CA10 (string-to-
 *    float unit), which makes this a TU of its own.
 * RESIDUALS. no bodies written.
 * SHAPES. none yet: no body is written, so nothing is measured; the map rows of the range are the work list
 *    (`ledger.py unit MSL_C/printf.c`).
 */
