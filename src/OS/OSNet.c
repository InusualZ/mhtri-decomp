/*
 * OS/OSNet.c - the OS network shutdown hook, `__OSInitNet`.
 * RANGE. .text 0x804D6740-0x804D6800 (1 linked function, 6 stripped); .data 0x80629650-0x806297B8.  Cut from the OS core band
 *    0x804C1760-0x804D9B4C.  Evidence: the NWC24/kd device strings (.data 0x80629650..0x806297B8, one 0x168-byte
 *    label) are read only by `__OSInitNet`.
 * FLAGS. `cflags_base` per object (configure.py).
 * NAMES. map name; `OSIOSRev` (OS/OS.h) is the record `__OSGetIOSRev` fills.  GUESS: `NetPrepareShutdown`, `NetSuspendScheduler`,
 *    `NetResumeScheduler`, `NetRequestShutdown`, `NetShutdownGiveUp` and `NetSetRtcCounter` (named for the tag string each pools).
 * RESIDUALS. .text: the six helpers after `__OSInitNet` have no caller and the linker strips them, so the object's .text is 0xCC longer
 *    than the claim.  Their bodies are GUESSES beyond the string order: only their pooled name tags and /dev/net/kd paths
 *    (.data 0x806296F8..0x80629761) survive in the DOL, referenced by no function of the image.
 * SHAPES. plain C; `__OSInitNet` returns early on an old or excluded revision (the `bne`/`b` pair; a guarding `if` around the body folds it to `beq`).
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
    if (revision.major <= 4 || revision.major == 9) {
        return;
    }

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

/* The network device helpers below have no caller in the image (the linker strips them); only their pooled
   name tags and device paths survive, after the `__OSInitNet` messages. */

/* Opens the request device for the shutdown handler. */
static int NetPrepareShutdown(s32* fd)
{
    return NWC24iOpenFd((u32)"NWC24iPrepareShutdown", "/dev/net/kd/request", fd, 1);
}

/* Asks the request device to suspend the WiiConnect24 scheduler. */
static int NetSuspendScheduler(s32 fd, u32* out)
{
    return NWC24iIoctl((u32)"NWC24SuspendScheduler", fd, 1, NULL, 0, out, 32);
}

/* Asks the request device to resume the WiiConnect24 scheduler. */
static int NetResumeScheduler(s32 fd, u32* out)
{
    return NWC24iIoctl((u32)"NWC24ResumeScheduler", fd, 2, NULL, 0, out, 32);
}

/* Hands the shutdown event to the request device. */
static int NetRequestShutdown(s32 fd, u32* in, u32* out, u32* result)
{
    return NWC24iIoctlAsync((u32)"NWC24iRequestShutdown", fd, 40, in, 32, out, 32, result);
}

/* Reports a shutdown request that ran out of retries. */
static void NetShutdownGiveUp(void)
{
    OSReport("NWC24Shutdown_: Give up!\n");
}

/* Opens the time device for an RTC counter write. */
static int NetSetRtcCounter(s32* fd)
{
    return NWC24iOpenFd((u32)"NWC24iSetRtcCounter_", "/dev/net/kd/time", fd, 0);
}
