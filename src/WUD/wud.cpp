/* WUD/wud.cpp - the Wii remote device manager over the Bluetooth stack: device table, pairing/sync and the HID host callbacks.
 * RANGE. .text 0x804F9B30-0x80500770 (96 functions); .data 0x8062DBF0-0x8062F030; .bss 0x8075EAA0-0x80760C48;
 *   .sdata 0x807941C8-0x807941E0; .sbss 0x80795720-0x80795758; .sdata2 0x8079D478-0x8079D480.
 *   Edges: the left edge 0x804F9B30 (`WUDIsLinkedWBC`) is the first reader of .sbss 0x80795738 and the first .data
 *   object (0x8062DBF0) is read from 0x804F9DC0; the `WUD.c` __FILE__ string (.sdata 0x807941D8) and the " %s\n" string
 *   (0x807941C8) are one copy each read from 0x804FA750 to 0x804FF980; the right edge 0x80500770 is the first of
 *   three nw4r functions (`nw4r/db_console.cpp`).  The last function 0x80500720 is the print helper called from
 *   0x804F9B60 onward.
 * FLAGS. the `OS` lib block of configure.py (`cflags_os`, the compiler the former `WPAD/wpad.cpp` block used); unmeasured until bodies exist.
 * NAMES. the map's names (`WUDGetBufferStatus`, `WUDSetSniffMode`, `WUDSetVisibility`, `WUDiGetDevInfo`,
 *   `WUDDeviceStatusCallback`, `WUDSetDeviceHistory`, `_WUDGetDevAddr`, `_WUDGetQueuedSize`, `_WUDGetNotAckedSize`,
 *   `_WUDGetLinkNumber`, `App_MEMalloc`, `App_MEMfree`, `bta_hh_co_data`, `bta_hh_co_open`, `bta_hh_co_close`, ...);
 *   the other functions are generated.
 * RESIDUALS. every body unwritten (stub).  The HID host callbacks (0x80500130..0x80500720: "BTA_HH_ENABLE_EVT",
 *   `bta_hh_co_*`, jump table 0x8062EFB0) are probably a second file; no data seam separates them from the rest
 *   (one .bss object 0x8075EAA0 is read on both sides), so the unit is kept whole.
 */
