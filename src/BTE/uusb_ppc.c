/*
 * BTE/uusb_ppc.c - the USB transport of the Broadcom BTE Bluetooth stack on the Wii: opens the Bluetooth USB device,
 *   runs its HCI event and ACL receive buffers, writes outgoing messages and closes and unregisters the device.
 *
 * RANGE. .text 0x804796F0-0x8047A370 (11 functions); .bss 0x80735900-0x80737960; .sdata 0x80793D28-0x80793D38;
 *   .sbss 0x80795008-0x80795020. Strong start: 0x804796F0 is the first function to read the transport control block
 *   .bss 0x80735900, which follows BTE/hcisu_h2.c's 0x807358C0 in the .bss. Strong end: UUSB_Unregister
 *   (0x8047A320) is the last reader of 0x80735900; bte_hcisu_send (0x8047A370) reads BTE/bte_hcisu.c's .sbss
 *   0x80795020. The two 0x1000 receive buffers 0x80735960 / 0x80736960 are read by 0x80479790 / 0x80479940; the
 *   .sdata flag 0x80793D28 (written here, also read by BTA_Init) and the device name strings "oh0" / "oh1"
 *   0x80793D30 / 0x80793D34 by 0x80479CB0.
 * FLAGS. the `OS` lib default `cflags_os` (registered flags; BTE units measured so far build with GC/3.0a5.2 +
 *   cflags_nw4r (unmeasured here)). All 11 starts are 16-aligned, as the whole head 0x80478790-0x8047ABC4 and
 *   BTE/gki_buffer.c (`cflags_base`, measured not GC); the GC-built files from 0x8047ABC4 on pack to 4 bytes.
 * NAMES. the map's.
 * RESIDUALS. every body is unwritten: 0x804796F0-0x8047A370 (11 functions); the data is unclaimed by source.
 * SHAPES. plain C.
 */
