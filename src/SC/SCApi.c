/*
 * SC/SCApi.c - the SC item getters and setters: language, sound mode, screen saver, display offset, Bluetooth and
 *    Wiimote settings, country code and the parental-control checks.
 *
 * RANGE. .text 0x804DC9E0-0x804DD010 (21 functions, 0x630 B); .data 0x80629E88-0x80629EB8; .bss
 *    0x80756600-0x80757608.  Cut from the old SC block between `SC/SCSystemConfig.c` (0x804DC9E0) and
 *    `SC/SCProductInfo.c` (0x804DD010).
 * FLAGS. the `OS` lib's group with `cflags_base` (configure.py `cflags=`): every function start of the band is
 *    16-aligned (`-O4,p` default alignment), not the 4-byte packing `cflags_os` gives.
 * NAMES. map names throughout (`SCGetLanguage`, `SCGetCountryCode`, `SCCheckPC*Restriction`, ...); five rows are
 *    unnamed; the file name `SCApi.c` is a GUESS.
 * EVIDENCE. every function calls `SCFind*Item`/`SCReplaceByteArrayItem` of the core unit and nothing else; `.bss`
 *    0x80756600 (0x1008 B, the IPL.SADR record) is read by `SCGetCountryCode` only; `.data` 0x80629E88 is the
 *    `<< RVL_SDK - SCCheckPCMessageRestriction >>` string; the core unit's last data reader is the flush at
 *    0x804DC6A0.
 * RESIDUALS. no bodies yet: all 21 functions are unwritten (largest SCGetCountryCode, 0xF0 B).
 * SHAPES. none yet: no body is written, so nothing is measured.
 */
