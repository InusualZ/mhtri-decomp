/*
 * VF/vf.cpp - the SDK block between SO and the HOME-button code: VF/prfile2 (`VFipf2Init`, `VFSysInit`,
 *   `dHash_InitHashTable`, ...), the DB/EXI debugger interface (`DBInitComm`, `DBRead`, `__DBEXIReadReg`, ...),
 *   PMIC, KPR, HID (`HIDRegisterClient`, `hid_client_attach`, ...) and KBD (`kbdProcKey`, `KBDResetChannel`, ...).
 * RANGE. .text 0x80521340-0x8052A040 (174 functions); .rodata 0x80574E10-0x80579450, .data 0x80631468-0x806498C8,
 *   .bss 0x80766C68-0x807901C0, .sdata 0x80794458-0x807944B0, .sbss 0x807958F0-0x80795A18, .sdata2
 *   0x8079D528-0x8079D550.  No extab.
 * RANGE. Left edge 0x80521340: cut from `SO/soi.cpp` (its header holds the evidence).  Right edge:
 *   `homebutton/fn_8052A040.cpp` at 0x8052A040.  The consumers are `homebutton/` and `DWCi/DWCi_Np_CPUCopyFast.c`,
 *   not the network stack.
 * RANGE. Several libraries, not split: the RVL_SDK version strings for PMIC (.data 0x80649130), KPR (0x806492D8), HID
 *   (0x80649348) and KBD (0x806493A0), and the DB/EXI run 0x80522334-0x80522DF4 is packed at 4 bytes where the rest is
 *   16-byte aligned (`gap_` padding), so this unit holds at least six TUs; no `.text` seam was measured.
 * FLAGS. the `OS` lib's `cflags_os` (unmeasured until bodies exist; the 16-byte-aligned libraries do not fit its
 *   `-func_align 4`, the DB run does).
 * NAMES. The file name is a GUESS from the first library of the block, VF.  GUESSES read off the bodies:
 *   `KPRLookAhead` (0x80526F00: counts the queued u16 characters, copying up to a maximum), `KBDSetLedsAsync`
 *   (0x80529430: queues an LED request through `kbd_alloc_led` with a callback) and `KBDSetChannelValue`
 *   (0x80529B50: stores a word in the channel's 0x2A8-byte record); `VFipf2*` are GUESSES in the prfile2 scheme.
 * RESIDUALS. Unwritten: every function in the range.
 */

#include "VF/vf.h"
