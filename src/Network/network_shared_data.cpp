/*
 * Network/network_shared_data.cpp - the Network band's shared small-data pool, registered as a
 * data-only unit.  It owns:
 *
 *   `.sdata2` 0x8079C690-0x8079C758 (0xC8 = 200 B)  the rate/timer float pool
 *   `.sdata`  0x80793900-0x80793930 (0x30 = 48 B)   the rate/interval words
 *
 * WHAT IT IS FOR.  Both runs are unowned in `splits.txt` and read by more than one party, so rule 12
 * fires on every consumer that spells a word of them: the reader census answers
 * the Network transport units *and* `Network/NetworkSessionManager.cpp` for 0x8079C6EC/0x8079C6F0/0x8079C6F8/
 * 0x8079C708/0x8079C718/0x8079C730..0x8079C754 (`callers.py 0x8079C6EC`), and an unsplit (Network)
 * object reads 0x8079C690 and 0x8079C750 on top of that.  A pool with several readers is the
 * `named-owner-unit` case, not a `claim-into-unit` one: claiming it into either consumer would take
 * rows only the *other* consumer reads, and would make that consumer include a rival's header for its
 * own data.  Registering one owner is strictly better than the anonymous `auto_*_sdata2` unit dtk
 * otherwise creates over the same band, and it is the shape `Pl/pl_frame_data.cpp` already uses for
 * the Pl band's pool (playbook 23/53 route 2, playbook 54).
 *
 * SOURCE DEFINES NOTHING.  The bytes are the original's and a `NonMatching` unit contributes exactly
 * those bytes to the link, so the DOL is untouched; what the registration buys is the ownership, plus
 * `include/Network/network_shared_data.h` as the one place a consumer declares a word (rule 2).  The
 * 14 declarations that header carries were moved there verbatim from `include/Network/NetworkSessionManager.h`,
 * where they were rule 12's finding while the run had no owner.
 *
 * EXTENTS (both 4-aligned, both ends proven by the reader set, not by a byte cap):
 *   - `.sdata2` starts at 0x8079C690, whose readers are this band plus unsplit code, and ends at
 *     0x8079C758, the first row of the next owner (`Network/NetworkSessionManagerPat.cpp` claims
 *     0x8079C758-0x8079C75C).  The `lbl_8079C70C` 4-byte hole between the tool's two reader pools
 *     0x8079C6D8-0x8079C70C and 0x8079C710-0x8079C758 makes them one contiguous run, which is why
 *     the range is claimed whole rather than as two (two ranges of one section in one unit are a
 *     link-order cycle - measured, see the survey note).
 *   - `.sdata` starts at 0x80793900 and ends at 0x80793930, the first row read outside the band
 *     (`networkSessionTimeoutSeconds`, read by `Network/initNetworkSessionStable.cpp` and unsplit
 *     code).
 *
 * RESIDUAL.  The rows this run's neighbours still need but this unit does not own:
 *   - `.sdata2` 0x80795AA0-0x80795AD8 is already claimed by another unit, and the rows before
 *     0x8079C690 (0x8079C660-0x8079C690) are unsplit-only readers.
 *   - the run's interior words are still `lbl_8079C6xx` rows: rule 7 names a row from what it holds
 *     and where it is used, and the bodies that use the unnamed ones are unwritten, so they are named
 *     by the lane that writes those bodies and declared in this unit's header then (the model is
 *     `Pl/pl_frame_data.h`, whose unnamed interior was filled in the same way).
 */

#include "types.h"
#include "Network/network_shared_data.h"
