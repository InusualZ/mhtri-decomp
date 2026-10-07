/*
 * DVD/dvdidutils.c - the SDK disk-ID comparison (`DVDCompareDiskID`).
 *
 * RANGE. `.text` 0x804ABCE0-0x804ABDD0 (1 function / 0xF0 B).
 *   - one function with no data of its own; both neighbours read disjoint data, so the seam is clean on each side.
 *     The file attribution has no data evidence (roster only)
 * FLAGS. the `OS` lib block of `configure.py` (`Wii/1.3`) with `cflags_base` (-O4,p, default alignment 16).
 * NAMES. the unit name is a GUESS (single function, no string); the `DVDDiskID` field names are named from their use.
 * RESIDUALS. no data evidence ties this function to a library file; it could belong to `dvderror.c` or `dvdFatal.c`.
 */

#include "types.h"
#include "DVD/dvdidutils.h"
#include "MSL_C/strstr.h"

BOOL DVDCompareDiskID(const DVDDiskID* id1, const DVDDiskID* id2)
{
    if (id1->gameName[0] && id2->gameName[0]) {
        if (strncmp(id1->gameName, id2->gameName, 4)) {
            return FALSE;
        }
    }

    if (!id1->company[0] || !id2->company[0] || strncmp(id1->company, id2->company, 2)) {
        return FALSE;
    }

    if (id1->diskNumber != 0xFF && id2->diskNumber != 0xFF) {
        if (id1->diskNumber != id2->diskNumber) {
            return FALSE;
        }
    }

    if (id1->gameVersion != 0xFF && id2->gameVersion != 0xFF) {
        if (id1->gameVersion != id2->gameVersion) {
            return FALSE;
        }
    }

    return TRUE;
}
