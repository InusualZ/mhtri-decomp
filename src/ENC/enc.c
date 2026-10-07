/*
 * ENC/enc.c - the SDK encoding library (UTF-8 / UTF-16 conversion).
 *
 * RANGE. `.text` 0x804AE880-0x804AF420; `.data` 0x8061A4A8-0x8061A4F0; `.sdata` 0x80793E18-0x80793E20; `.sbss`
 *   0x80795180-0x80795188 (9 functions / 0xB50 B).
 *   - version string `<< RVL_SDK - ENC ... >>` at `.data` 0x8061A4A8 is registered by 0x804AE880 through `.sdata`
 *     0x80793E18; the run ends where the ESP library's `/dev/es` string starts
 *   - `ENCConvertStringUtf16ToUtf8` / `Utf8ToUtf16` are two-instruction wrappers over the converters; `.sbss`
 *     0x80795180 is the version-registered flag
 * FLAGS. the `OS` lib block of `configure.py` (`Wii/1.3`, `cflags_os`), the block of the unit this range was cut
 *   from; unmeasured until bodies exist.
 * NAMES. the unit name is a GUESS (the library's first source file); the two converters and the validator keep `fn_`
 *   names in the map.
 * RESIDUALS. No body is written (9 functions), the largest `fn_804AEDF0` at 0x804AEDF0 (0x314 B); `python
 *   tools/units/sweepcomments.py --unit ENC/enc.c` lists them.
 */
