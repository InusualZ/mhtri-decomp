/*
 * DWCi/dwc_nasfunc.cpp - STUB (phase 4, docs/splits/phase4; no bodies yet).
 *
 * `.text` 0x80509DB0..0x805113B0.  Sections of the candidate unit: .text 0x80509DB0..0x805113B0; .data 0x806307F0..0x806308A8; .bss 0x807613B8..0x807614D8; .sdata 0x807942FC..0x80794358; .sbss 0x807957F8..0x80795820.
 *
 * WHAT IT IS. the DWC NAS login (`DWC_NASLoginAsync`), SVL, list, table, socket, platform, NAT-probe, buffer and request files merged, up to 0x805113B0 (the registered `DWCi/fn_805113B0.c`).
 *   GUESS: named after the first file in the run (`dwc_nasfunc.c` is the SDK's NAS file).
 *   merged candidate pieces (guess cuts the renderer does not emit): fn_80509DB0, dwc_nas, dwc_svl, dwc_list, dwc_table, dwc_socket, dwc_platform, dwc_natprobe, dwc_buffer, dwc_request, gt2, fn_8050E2B0.
 *
 * WHY IT SITS HERE. phase 1 grade medium, class inherited: 0x80509DB0 follows the registered unit DWCi/DWCi_Np_CPUCopyFast.c with no function in between (<= 16 B pad); baseline order/coverage/text-cut PASS.
 *
 * UNKNOWN. every body and, for a merged block, the file boundaries between its pieces.
 *
 * FLAGS. the `DWCi` lib's `cflags_dwc` (unmeasured until bodies exist).
 *
 * The unit's `.data`/`.sdata`/`.sbss` claims are the candidate's (config/RMHE08/splits.txt); the symbols they hold are
 * in the map (`ledger.py unit DWCi/dwc_nasfunc.cpp`), and the pass that writes the bodies defines them.
 */
