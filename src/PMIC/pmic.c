/*
 * PMIC/pmic.c - the PMIC library: the USB device bring-up (`PMICInit`) with its firmware image, the transfer state
 *    machine and the sample filter.
 *
 * RANGE. .text 0x80522E00-0x80526C90 (53 functions, 0x3E90 B); .data 0x80631468-0x806492D8; .bss
 *    0x8078FA38-0x8078FF40; .sdata 0x80794460-0x80794498; .sbss 0x80795938-0x807959F0; .sdata2
 *    0x8079D528-0x8079D550.  Cut from the old VF block between `TRK/exi2_comm.c` (0x80522E00) and `KPR/kpr.c`
 *    (0x80526C90).  COARSE: the library's source files are not separated.
 * FLAGS. the `OS` lib's group with `cflags_base` (configure.py `cflags=`): every function start of the band is
 *    16-aligned (`-O4,p` default alignment), not the 4-byte packing `cflags_os` gives.
 * NAMES. only the library name `PMIC` is known (the build string); `PMICInit` is a GUESS for 0x80522E00 (it is the
 *    registrant of the build string) and every row is unnamed; the file name `pmic.c` is a GUESS.
 * EVIDENCE. `.data` 0x80631468..0x806492D8 is one image (0x17CC8 B) whose address the first function passes on,
 *    followed by the build string `<< RVL_SDK - PMIC release build ... (0x4302_145) >>` (read through `.sdata`
 *    0x80794460 by `OSRegisterVersion`), the `PMICInit() : too busy` and `/dev/usb/oh0` strings and a jump
 *    table; `.sbss` 0x80795938..0x807959F0 (the state word 0x80795938 is read by 35 functions) and `.bss`
 *    0x8078FA38..0x8078FF40 are read here only; the entries at 0x80526890 and 0x80526980 are called by
 *    `menu/menu_plsearch.cpp`.
 * RESIDUALS. no bodies yet: all 53 functions are unwritten (largest fn_80525380, 0x4B8 B).
 * SHAPES. none yet: no body is written, so nothing is measured.
 */
