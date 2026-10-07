/*
 * OS/OSNet.c - the OS network shutdown hook, `__OSInitNet`.
 * RANGE. .text 0x804D6740-0x804D6800 (1 function); .data 0x80629650-0x806297B8.  Cut from the OS core band
 *    0x804C1760-0x804D9B4C.  Evidence: the NWC24/kd device strings (.data 0x80629650..0x806297B8, one 0x168-byte
 *    label) are read only by `__OSInitNet`.
 * FLAGS. `cflags_base` per object (configure.py).
 * NAMES. map name; `OSIOSRev` (OS/OS.h) is the record `__OSGetIOSRev` fills.
 * RESIDUALS. `__OSInitNet` (97 %): the target tests the revision with `bne` plus an unconditional `b` to the epilogue where the
 *    compiler folds both into one `beq`; no source spelling kept the pair.  .data: only the three `OSReport` messages (0x80629650, 0x80629684, 0x806296B8) are emitted; the 0xC4 bytes after
 *    them (the NWC24 function-name and /dev/net/kd path strings) are referenced by no function of the image.
 * SHAPES. plain C.
 */

#include "types.h"

#include "NWC24/nwc24_io.h"
#include "NWC24/nwc24_msg.h"
#include "OS/OS.h"
#include "OS/OSError.h"

/* Shuts the NWC24 scheduler and the network time sync down on IOS builds newer than 4 (except 9). */
void __OSInitNet(void)
{
    OSIOSRev revision;
    int result;

    __OSGetIOSRev(&revision);
    if (revision.major > 4 && revision.major != 9) {
        result = NWC24iPrepareShutdown();
        if (result != 0) {
            if (result < 0) {
                OSReport("Failed to register network shutdown function. %d\n", result);
            }
            result = NWC24SuspendScheduler();
            if (result < 0) {
                OSReport("Failed to suspend the WiiConnect24 scheduler. %d\n", result);
            }
        }
        if (!__OSInIPL) {
            result = NWC24iSynchronizeRtcCounter(0);
            if (result != 0) {
                OSReport("Failed to synchronize time with network resource managers. %d\n", result);
            }
        }
    }
}
