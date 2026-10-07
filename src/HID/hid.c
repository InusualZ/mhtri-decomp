/*
 * HID/hid.c - the HID library: client registration, device attach, detach and adoption, the interface table and the
 *    request transfers (with its exported thunks at the start).
 *
 * RANGE. .text 0x805272B0-0x80528890 (44 functions, 0x15E0 B); .data 0x80649348-0x806493A0; .sdata
 *    0x807944A0-0x807944A8; .sbss 0x80795A00-0x80795A08.  Cut from the old VF block between `KPR/kpr.c`
 *    (0x805272B0) and `KBD/kbd.c` (0x80528890).  COARSE: the library's source files are not separated.
 * FLAGS. the `OS` lib's group with `cflags_base` (configure.py `cflags=`): every function start of the band is
 *    16-aligned (`-O4,p` default alignment), not the 4-byte packing `cflags_os` gives.
 * NAMES. `HIDRegisterClient`, `hid_set_suspend`, `hid_client_attach`, `hid_client_detach`,
 *    `hid_device_manage_change`, `hid_device_adopt_orphans`, `hid_interface_free` and `hid_interface_alloc` are
 *    the map's names; the file name `hid.c` is a GUESS; 36 rows are unnamed.
 * EVIDENCE. the seven 4-byte thunks at 0x805272B0..0x80527320 tail-call HID functions only (the keyboard unit calls
 *    them); `.data` 0x80649348 is the build string `<< RVL_SDK - HID ... >>` read through `.sdata` 0x807944A0
 *    by 0x80528510 (reached from the first thunk); `.sbss` 0x80795A00 (8 B) is read by 29 of the range's
 *    functions; the keyboard unit's state starts at 0x80795A08, first read by 0x80528890.
 * RESIDUALS. no bodies yet: all 44 functions are unwritten (largest fn_80527EC0, 0x1B8 B).
 * SHAPES. none yet: no body is written, so nothing is measured.
 */
