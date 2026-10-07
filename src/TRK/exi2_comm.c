/*
 * TRK/exi2_comm.c - the MetroTRK debugger channel over EXI2: the channel and interrupt handlers, `DBInitComm`,
 *    `DBRead`, `DBWrite`, the reserve stubs and the EXI2 register and RAM transfers.
 *
 * RANGE. .text 0x80522334-0x80522E00 (15 functions, 0xACC B); .sdata 0x80794458-0x80794460; .sbss
 *    0x80795920-0x80795938.  Cut from the old VF block between `VF/vf.cpp` (0x80522334) and `PMIC/pmic.c`
 *    (0x80522E00).
 * FLAGS. the `OS` lib's `cflags_os` (4-byte function alignment): the run's function starts are packed at 4, unlike
 *    the rest of the old VF block.
 * NAMES. the `DB*` and `__DB*` names are the map's; `EXI2_Reserve` and `EXI2_Unreserve` are GUESSES (4-byte stubs
 *    called around a continue/stop); the file name `exi2_comm.c` is a GUESS.
 * EVIDENCE. this is the only 4-byte-packed run of the old block (ten of its 174 function starts are not 16-aligned
 *    and all ten are here), so it is not one of the libraries around it; its only callers are `TRK/gdev_cc.c`
 *    (`DBInitComm`, `DBQueryData`, `DBRead`, `DBWrite`, `DBInitInterrupts`, the reserve stubs); `.sdata`
 *    0x80794458 (8 B) is read by `DBWrite` and `.sbss` 0x80795920..0x80795938 by the two handlers and the DB
 *    calls; the leaf headers of the exported rows sit beside `TRK/gdev_cc.c`.
 * RESIDUALS. no bodies yet: all 15 functions are unwritten (largest __EXI2Imm, 0x2E8 B).
 * SHAPES. none yet: no body is written, so nothing is measured.
 */
