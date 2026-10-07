/*
 * MSL_C/locale.c - the C locale tables (data only): the ctype class/map tables, the `_current_locale` records, the
 *    time-format strings and the pooled `.`/`AM|PM` strings.
 *
 * RANGE. .rodata 0x80572620..0x80572B28; .data 0x8060EC00..0x8060EDF8; .sdata2 0x8079C9E0..0x8079C9F8.
 * FLAGS. the `Runtime.PPCEABI.H` lib's `cflags_ppceabi` (the group of the unit it was cut from; unmeasured until
 *    bodies exist).
 * NAMES. file name GUESS (MSL `locale.c`); the map name `DWCi_digitClassTable` (0x8060EDB0) is another unit's name
 *    for one of its records.
 * EVIDENCE. no code in the band reads these objects by name: `.data` 0x8060EC00..0x8060EDF8 holds records whose
 *    words point at `.rodata` 0x80572620 (the 0x200 B class table), 0x80572820 / 0x80572920 (lower/upper maps)
 *    and at `.sdata2` 0x8079C9E0..0x8079C9F8 (`.`, ``, `AM|PM`); the `.rodata` run 0x80572A20..0x80572B28 is `%a
 *    %b %e %T %Y`, day and month names. The run sits between the floating-point conversion unit's data and the
 *    printf unit's string base in every section.
 * RESIDUALS. no bodies; no `.text`.
 * SHAPES. none yet: no body is written, so nothing is measured; the map rows of the range are the work list
 *    (`ledger.py unit MSL_C/locale.c`).
 */
