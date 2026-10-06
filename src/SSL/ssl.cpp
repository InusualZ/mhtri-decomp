/*
 * SSL/ssl.cpp - the SSL library (`SSLNew`, `SSLConnect`, `SSLDoHandshake`, `SSLRead`, `SSLWrite`, the certificate
 *   setters): the RVL_SDK "SSL" build, driving `/dev/net/ssl`.
 * RANGE. .text 0x8051B7FC-0x8051C554 (13 functions); .data 0x80630F88-0x80630FE0, .bss 0x80763900-0x80766920, .sdata
 *   0x80794420-0x80794428, .sbss 0x80795888-0x80795890.  No extab, .rodata or .sdata2.
 * RANGE. Left edge: `NHTTP/d_nhttp.c` ends at 0x8051B7FC.  Right edge 0x8051C554: `NCD/ncdsystem.c` starts with
 *   `NCDGetCurrentIfConfig`.  No `.text` reference crosses the edge (the three 4-byte stubs 0x8051C548-0x8051C550 are
 *   called only by `SSLSetClientCert`/`SSLSetRootCA`); the `.data` run is the "SSL" version string and `/dev/net/ssl`,
 *   followed by the "NCD" version string at 0x80630FE0; every `.bss`/`.sdata`/`.sbss` label below its edge is read
 *   only from this range and every one above only from the next.
 * FLAGS. the `OS` lib's `cflags_os` (unmeasured until bodies exist).
 * NAMES. The map's names; the three stubs keep the map's stems.
 * RESIDUALS. Unwritten: every function in the range.
 */
