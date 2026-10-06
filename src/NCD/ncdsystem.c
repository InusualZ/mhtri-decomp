/*
 * NCD/ncdsystem.c - the NCD library (`NCDGetCurrentIfConfig`, `NCDGetCurrentIpConfig` and their `/dev/net/ncd/manage`
 *   ioctl helpers) and the NET helpers behind it (`NETGetStartupErrorCode`, `NETMemCpy`, `NETMemSet`).
 * RANGE. .text 0x8051C554-0x8051D710 (14 functions); .data 0x80630FE0-0x80631128, .bss 0x80766920-0x80766980, .sdata
 *   0x80794428-0x80794438, .sbss 0x80795890-0x80795898.  No extab, .rodata or .sdata2.
 * RANGE. Left edge 0x8051C554: cut from `SSL/ssl.cpp` (its header holds the evidence).  Right edge:
 *   `NWC24/nwc24_msg.c` at 0x8051D710.
 * RANGE. Two libraries, not split: `.data` is the "NCD" version string, the NCD names and `ncdsystem.c` (read by
 *   0x8051C554-0x8051CCE0), then "Unknown SOStartup Error: %d\n" (read by 0x8051D0C8) and the "REX-PPC" version
 *   string (through `.sdata` 0x80794430, read by 0x8051D240).  The `.text` edge lies in 0x8051CDD0-0x8051D048
 *   (0x8051CDD0 reads no data and is called only by `NETGetStartupErrorCode`), so the range stays one unit.
 * FLAGS. the `OS` lib's `cflags_os` (unmeasured until bodies exist).
 * NAMES. `ncdsystem.c` is the `__FILE__` string the range's own panic passes (0x8051CD40, "Could not reserve heap
 *   for NCD library from IPC arena"); the NET tail shares the file only because its edge is unproven.
 * RESIDUALS. Unwritten: every function in the range.
 */

#include "NCD/ncdsystem.h"
