/*
 * BTE/hcisu_h2.c - the H2 (USB) HCI services of the Broadcom BTE Bluetooth stack: reassembles incoming HCI messages,
 *   sends outgoing ones through the USB transport, and exports its entry points as the transport interface table.
 *
 * RANGE. .text 0x80479030-0x804796F0 (8 functions); .rodata 0x80573530-0x80573548; .data 0x80612E60-0x80612ED0;
 *   .bss 0x807358C0-0x80735900; .sdata2 0x8079D080-0x8079D090. Strong start: 0x80479030 is a static callback whose
 *   address 0x804795D0 takes. Strong end: 0x804796F0 opens BTE/uusb_ppc.c (it reads the USB .bss 0x80735900).
 *   The .rodata 0x80573530 is the interface table of this file's five entry points (0x804795A0, 0x804795D0,
 *   0x80479640, 0x80479680, 0x804796B0, then a zero word), which BTA_Init installs; the .data are the two "HCIS: ..."
 *   messages, the .bss the reassembly state and the .sdata2 two byte tables, all read by 0x80479050 and the entry
 *   points.
 * FLAGS. the `OS` lib default `cflags_os` (registered flags; BTE units measured so far build with GC/3.0a5.2 +
 *   cflags_nw4r (unmeasured here)). All 8 starts are 16-aligned, as the whole head 0x80478790-0x8047ABC4 and
 *   BTE/gki_buffer.c (`cflags_base`, measured not GC); the GC-built files from 0x8047ABC4 on pack to 4 bytes.
 * NAMES. the map's.
 * RESIDUALS. every body is unwritten: 0x80479030-0x804796F0 (8 functions); the data is unclaimed by source.
 * SHAPES. plain C.
 */
