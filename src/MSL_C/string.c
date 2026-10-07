/*
 * MSL_C/string.c - the C string routines: `strcpy`, `strncpy`, `strcat`, `strcmp`, `strncmp`, `strchr`, `strrchr`,
 *    `strtok`, `strstr`.
 *
 * RANGE. .text 0x8045F554..0x8045F9E8 (9 functions in the map, 0x494 B); .sdata 0x80793CE8..0x80793CF0; .sdata2
 *    0x8079CA00..0x8079CA08.
 * FLAGS. the `Runtime.PPCEABI.H` lib's `cflags_ppceabi` (the group of the unit it was cut from; unmeasured until
 *    bodies exist).
 * NAMES. file name GUESS (MSL `string`); `strlen` is registered separately (`MSL/strlen.cpp`).
 * EVIDENCE. `strtok`'s two `.sdata` statics (0x80793CE8 / 0x80793CEC) point at the pooled empty string `.sdata2`
 *    0x8079CA00; no function of the run calls outside it.
 * RESIDUALS. no bodies written.
 * SHAPES. none yet: no body is written, so nothing is measured; the map rows of the range are the work list
 *    (`ledger.py unit MSL_C/string.c`).
 */
