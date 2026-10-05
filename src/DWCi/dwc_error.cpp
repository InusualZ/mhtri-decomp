/*
 * DWCi/dwc_error.cpp - STUB (no bodies yet).
 *
 * `.text` 0x805073C0..0x80507C40.  Sections of the candidate unit: .text 0x805073C0..0x80507C40; .data 0x8062FD00..0x8062FF90; .sdata 0x807941F8..0x80794200; .sbss 0x807957B0..0x807957D0.
 *
 * WHAT IT IS. the DWC error / memory-function / report files merged (`DWC_GetLastErrorEx`, `DWC_ClearError`, `DWCi_allocNode`, `DWCi_report`).
 *   merged candidate pieces (guess cuts the renderer does not emit): dwc_error, dwc_memfunc, dwc_report.
 *
 * WHY IT SITS HERE. phase 1 grade medium, class data-seam: DWC_GetLastErrorEx/DWC_ClearError: DWC roster begins.
 *
 * UNKNOWN. every body and, for a merged block, the file boundaries between its pieces.
 *
 * FLAGS. the `DWCi` lib's `cflags_dwc` (unmeasured until bodies exist).
 *
 * The unit's `.data`/`.sdata`/`.sbss` claims are the candidate's (config/RMHE08/splits.txt); the symbols they hold are
 * in the map (`ledger.py unit DWCi/dwc_error.cpp`), and the pass that writes the bodies defines them.
 */
