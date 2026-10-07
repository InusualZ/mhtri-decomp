/*
 * EXI/EXIBios.c - the Revolution SDK EXI bus driver (EXIBios.c).
 *
 * `.text` 0x804AFED0..0x804B17D0 (20 functions / 6400 B).  Cut from the BTE band with the strongest
 * boundary evidence on both edges.
 *
 * Evidence for the name (an already-complete SDK roster, as in `OS/PPCArch.c`).
 *   - the 20 symbols in the range already carry their real SDK names and are exactly EXIBios.c's
 *     function list in the SDK's own order: SetExiInterruptMask, EXIImm, EXIImmEx, EXIDma, EXISync,
 *     EXISetExiCallback, __EXIProbe, EXIAttach, EXIDetach, EXISelect, EXIDeselect,
 *     EXIIntrruptHandler, TCIntrruptHandler, EXTIntrruptHandler, EXIInit, EXILock, EXIUnlock,
 *     UnlockedHandler, EXIGetID.  No `fn_XXXXXXXX` needs renaming (rule 7 clean);
 *   - each body's call set is the EXI bus protocol itself: the four interrupt handlers register
 *     through `__OSSetInterruptHandler`/`__OSMaskInterrupts`, `EXIInit` installs them and calls
 *     `OSRegisterVersion(__EXIVersion)`, and `EXIGetID`/`EXISelect`/`EXIDeselect` drive the channel.
 *
 * Extent - the right edge is **proven**, the left is roster-proven.
 *   - right: the next registered unit, `EXI/ProbeBarnacle.c`, starts at exactly 0x804B17D0, so the
 *     claim end is a registered neighbour's start and the 8 B pad at 0x804B17C8 lands inside it.
 *   - left: 0x804AFED0 is 16-byte aligned and is EXIBios.c's first function.  The function before it,
 *     `WriteUARTN` (0x804AFCA0..0x804AFFB8, the UART/debug-print layer), *calls into* this range
 *     (EXIImm / EXISync / EXISelect / EXIDeselect / EXILock / EXIUnlock), so it consumes EXIBios and
 *     cannot be part of it.
 *   - the two SDK libraries below it are the registered units `EUART/EUART.c` (EUARTInit, InitializeUART,
 *     WriteUARTN, 0x804AFB50..0x804AFED0) and `ESP/esp.c` (the ES proxy, 0x804AF420..0x804AFB50).
 *
 * Data this range reads, and why only part of it is claimed.
 *   - `__EXIVersion` (`.sdata` 0x80793E30, 4 B) is this file's own version word - `EXIInit` is its only
 *     reader (`callers.py`: 1 site) - so it is claimed here;
 *   - `Ecb` (`.bss` 0x80746B60, 192 B) is this file's own channel-state array - the range is its only
 *     referrer - so it is claimed here;
 *   - `IDSerialPort1` (`.sbss` 0x80795198, 4 B) is this file's own - `EXIInit` is its only
 *     address-taker - so it is claimed here;
 *   - `lbl_8061A4F0` (`.data` 0x8061A4F0, 0x46 B) is this file's own SDK version banner
 *     (`"<< RVL_SDK - EXI \trelease build: Feb 27 2009 10:02:03 (0x4302_145) >>"`) - the map row is a
 *     `lbl_` stem, but `__EXIVersion`'s `.sdata` word at 0x80793E30 *is* this address (measured), and
 *     `EXIInit` is the only function that registers it through `OSRegisterVersion`, so the string and
 *     the pointer are one claim;
 *   - `__OSInIPL` (`.sbss` 0x807952C8, 4 B) is **deliberately left unclaimed**: it is an *OS* shared
 *     global with 16 reader sites across the DVD and EXI libraries, so it is not this unit's to take,
 *     and a second `.sbss` run here would force a span-claim over the OS/DVD/FS `.sbss` cluster between
 *     the two addresses (`dataclaim.py --unit EXI/EXIBios` proposes exactly that span, 0x80795198..0x807952CC).
 *     Taking it would swallow 300 B of `lbl_807951A0`..`__fsInitialized`/`__devfs`/`__DVD*` globals that
 *     belong to the FS and DVD libraries, and the profile's rule is to leave it rather than inflate the
 *     claim.  The green `ninja build/RMHE08/ok` confirms no link-order cycle is introduced by leaving it
 *     (the tool's cycle warning applies only once the far end is claimed).  The remedy is a named owner
 *     unit for the OS/FS/DVD `.sbss` cluster, recorded in the survey note - not an `extern` in this file.
 *
 * Bodies are **not written here** - the survey registers the range; the decompiler writes the 20
 * functions.  Nothing in this file is a body yet, so it is a registration stub, not a partial
 * reconstruction.  Measured target side (`datagap.py --mode both --unit EXI/EXIBios`): 6400 B `.text`,
 * 70 B `.data`, 192 B `.bss`, 4 B `.sdata`, 4 B `.sbss` target-extra, 0 B ours-extra.
 */

/* -O4,p is this lib section's function alignment (16); cflags_os overrides it to 4 for OSAlarm, but
 * every function start in this range is 16-byte aligned (the 4/8/12 B pads between bodies). */
#pragma function_align 16

#include "types.h"
