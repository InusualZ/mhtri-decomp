/* WPAD/WPADEncrypt.cpp - the extension encryption of the Wii remote driver (the SDK's WPADEncrypt.c): the two key
 *   generators and the in-place decryption of extension data.
 * RANGE. .text 0x804F72C0-0x804F8380 (3 functions); .data 0x8062C8C8-0x8062DBF0 (the four key tables, then the generators'
 *   debug strings); .sbss 0x80795718-0x80795720 (the generators' OSGetTick-seeded LCG).  Seam with WPAD/WPADHIDParser.cpp at
 *   0x804F72C0 / 0x8062C8C8: the .data hole 0x8062C8C4-0x8062C8C8, and the tables and strings are read only by
 *   `WPADiCreateKey`/`WPADiCreateKeyFor3rd`, which address them from one base (0x8062C8C8, then +0x30/+0x930/+0x960/+0x1260):
 *   file statics, defined in address order (the map rows are scope:local).  Seam with WPAD/WPADClamp.cpp at 0x804F8380: the
 *   SDK file list, codegen-neutral (no call crosses it and the two own disjoint sections).  `WPADiDebugPrint` is called out of
 *   line (12 calls).
 * FLAGS. `cflags_base` (-O4,p, 16-byte function alignment).
 * NAMES. GUESS: the unit name (SDK spelling), `WPADiCreateKey`, `WPADiCreateKeyFor3rd` (SDK names: the extension key
 *   generators), `wpadDecryptExtension`; data `wpadRandModulus/Increment/Multiplier/Seed` (the LCG), `wpadKeyAnsTable`,
 *   `wpadKeySBox`, `wpadKey3rdAnsTable`, `wpadKey3rdSBox` (the generators' tables).
 * RESIDUALS. Written: 1 of 3 rows; 2 rows are not attempted (0x804F72C0 (0x6F0), 0x804F79B0 (0x6F8)).  `wpadDecryptExtension`
 *   differs in loop unrolling.  Flip blockers: .text (2 unwritten rows), .data (the tables and strings are not defined yet) and
 *   .sbss (the LCG bytes).
 */

#include "types.h"
#include "WPAD/wpad.h"
#include "WPAD/WPADEncrypt.h"

/* Decrypts `length` bytes of extension data in place with the channel's key tables; `offset` is the position within the register block. */
void wpadDecryptExtension(s32 chan, u8* data, u16 length, s32 offset)
{
    WPADCB* cb = wpadCBTable[chan];
    u16 i;

    for (i = 0; i < length; i++) {
        s32 index = (offset + i) % 8;
        data[i] = cb->keyAdd[index] + (data[i] ^ cb->keyXor[index]);
    }
}
