/* USB/usb.cpp - the IOS USB client (`IUSB_*` open/close and the bulk, interrupt and control transfer helpers).
 * RANGE. .text 0x804E46F0-0x804E6710 (22 functions); .data 0x8062AB98-0x8062B3C8; .sdata 0x80794150-0x80794160;
 *   .sbss 0x807955C8-0x807955D8.
 *   Edges: the two print helpers 0x804E46F0 ("USB: ") and 0x804E47A0 ("USB ERR: ") open the range and are called by
 *   most of the others; the "USB ERR: " string object (.data 0x8062AB98) is one copy read from both halves
 *   of the range (0x804E4850..0x804E63E0), so the halves are one TU.  The right edge 0x804E6710 is the first
 *   reader of the VI state (.sbss 0x80795608, .bss 0x8075B110).
 * FLAGS. the `OS` lib block of configure.py (`cflags_os`, the compiler the former `WPAD/wpad.cpp` block used); unmeasured until bodies exist.
 * NAMES. `IUSB_OpenLib`, `IUSB_CloseLib`, `IUSB_OpenDeviceIds`, `IUSB_CloseDeviceAsync`, `__LongBlkMsgInt` and
 *   `__IntrBlkMsgInt` are the map's names; the other 16 functions are still generated names.
 * RESIDUALS. every body unwritten (stub).
 */
