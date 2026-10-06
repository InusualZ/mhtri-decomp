/*
 * SO/soi.cpp - the SO socket library (`SOInit`, `SOStartup`, `SOSocket`, `SOConnect`, `SORecvFrom`, `SOPoll`,
 *   `SOGetAddrInfo`, ...): the RVL_SDK "SO" and "SOCKET" builds.
 * RANGE. .text 0x8051E864-0x80521340 (46 functions, the last 0xC bytes alignment padding); .data 0x80631230-0x80631468,
 *   .bss 0x80766C40-0x80766C68, .sdata 0x80794448-0x80794458, .sbss 0x807958D8-0x807958F0.  No extab, .rodata or
 *   .sdata2.
 * RANGE. Left edge: `NWC24/nwc24_io.c` ends at 0x8051E864.  Right edge 0x80521340: `VF/vf.cpp` starts there (prfile2's
 *   `VFipf_memset`, the first of its 16-byte-aligned functions).  No `.text` reference crosses the edge; the `.data` run
 *   is the "SO" version string (read through `.sdata` 0x80794448), `/dev/net/ip/top`, three jump tables, the "SOCKET"
 *   version string (through 0x80794450) and `%d.%d.%d.%d`, every label read only from below the edge; the `.bss`,
 *   `.sdata` and `.sbss` pieces split the same way.
 * RANGE. Two TUs by the two version strings (SO built 12:00:00, SOCKET 12:00:01); the `.text` edge between them is not
 *   pinned, so the range stays one unit.
 * FLAGS. the `OS` lib's `cflags_os` (unmeasured until bodies exist); its 4-byte function packing fits `-func_align 4`.
 * NAMES. The map's names; the file name follows the SDK's `soi` prefix (`SOiGetLastError`).
 * RESIDUALS. Unwritten: every function in the range.
 */
