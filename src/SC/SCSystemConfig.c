/*
 * SC/SCSystemConfig.c - the SC (system configuration) library core: `SCInit`, the SYSCONF file reload and flush, the
 *    item table and the typed item readers.
 *
 * RANGE. .text 0x804DB050-0x804DC9E0 (21 functions, 0x1990 B); .rodata 0x80573B58-0x80573BB0; .data
 *    0x80629CA8-0x80629E88; .bss 0x8074E460-0x80756600; .sdata 0x80794000-0x80794110; .sbss
 *    0x80795448-0x80795460.  Cut from the old SC block; the left edge is `SCInit` (0x804DB050, after the RSO list
 *    functions), the right edge 0x804DC9E0 is the first item getter of `SC/SCApi.c`.
 * FLAGS. the `OS` lib's group with `cflags_base` (configure.py `cflags=`): every function start of the band is
 *    16-aligned (`-O4,p` default alignment), not the 4-byte packing `cflags_os` gives.
 * NAMES. `SCInit`, `SCCheckStatus`, `SCReloadConfFileAsync`, `SCFlushAsync`, `SCFind*Item`, `SCReplaceByteArrayItem`
 *    and the *CallbackFromReload names are the map's; the file name `SCSystemConfig.c` is a GUESS.
 * EVIDENCE. `.rodata` 0x80573B58..0x80573BB0 holds `/shared2/sys`, `/shared2/sys/SYSCONF` and
 *    `/title/00000001/00000002/data/setting.txt`, read by `SCReloadConfFileAsync`, `SCFlushAsync` and the
 *    0x804DC6A0 flush; `.data` 0x80629CA8 is the build string `<< RVL_SDK - SC release build ... (0x4302_145)
 *    >>` (`__SCVersion`), followed by the `IPL.*` item-name strings and the item table 0x80629D38; `.bss`
 *    0x8074E460 (`Control`) and 0x8074E600 (`ConfBuf`, 0x4000 B) and 0x80752600 (0x4000 B) are read here only;
 *    `.sbss` 0x80795448..0x80795460 is the job state.
 * RESIDUALS. no bodies yet: all 21 functions are unwritten (largest fn_804DC6A0, 0x338 B).
 * SHAPES. none yet: no body is written, so nothing is measured.
 */
