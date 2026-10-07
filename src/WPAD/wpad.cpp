/* WPAD/wpad.cpp - the Wii remote driver: control block, connect/disconnect, speaker, stream data and the report queue.
 * RANGE. .text 0x804EB510-0x804F9B30 (131 functions); .rodata 0x80573C80-0x80573CD8; .data 0x8062BF30-0x8062DBF0;
 *   .bss 0x8075B2A0-0x8075EAA0; .sdata 0x80794198-0x807941C8; .sbss 0x807956A8-0x80795720; .sdata2 0x8079D3C8-0x8079D478.
 *   Edges: the "<< RVL_SDK - WPAD ... Jun 22 2009 ... (0x4302_145) >>" build string (.data 0x8062BF30) and the
 *   `WBCReadDummy`/`WBCSetZEROPointDummy`/`WBCGetTGCWeightDummy` print strings follow it before any other string, so
 *   the nine 8-byte stubs and the three dummies at 0x804EB510..0x804EB630 belong to this TU; the right edge 0x804F9B30
 *   is the first reader of the WUD state (.sbss 0x80795738, .bss 0x8075EAA0).
 * FLAGS. the `OS` lib block of configure.py (`cflags_os`); every function start is 16-aligned.
 * NAMES. the map's names (`WPADInit`, `WPADDisconnect`, `WPADProbe`, `WPADControlSpeaker`, `WPADSendStreamData`,
 *   `WPADiSendWriteData`, `WPADiSendSetReportType`, `WPADiClearQueue`, ...); the other functions are generated.
 * RESIDUALS. every body unwritten (stub).  The TU is probably two: the .sdata2 pool holds the double 4330000080000000
 *   twice (0x8079D418 read by 0x804F3150/0x804F5050/0x804F54F0, 0x8079D460 read by 0x804F84E0/0x804F8C00/0x804F8D30) and
 *   the float 0 twice (0x8079D3E0, 0x8079D46C); the second TU starts after 0x804F5768 and no later than 0x804F8380
 *   (the first reader of 0x8079D430), and the 17 functions 0x804F5770..0x804F8380 read no pool entry, so the seam is
 *   not pinned and the unit is kept whole.
 */
