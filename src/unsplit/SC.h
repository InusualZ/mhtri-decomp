/*
 * SC declarations with no registered owner (docs/plan.md 6.5 rule 2).
 *
 * The Revolution SDK's disc-interface (SC) library sits in the 0x804D5xxx-0x804DExxx band, which no
 * registered `splits.txt` range covers: the bracketing registered units are `SDK/fn_804C1760.c`'s
 * neighbours (`0x804C1760-0x804C6D68`, `0x804CBC50-0x804CBC60`) below and `0x804D9B4C-0x804DAE40`
 * above, so `stylelint`'s rule 2 has no owner's header to resolve these names to.  The module the
 * names themselves carry is `SC` (`SCCheckStatus`, `SCGetIdleMode`, `SCGetLanguage`,
 * `SCFindByteArrayItem`, ... are all real map names in that band), so this file is their home.
 *
 * Added with the `NWC24/nwc24_io.c` body pass, which drives the SC through the shutdown pair.
 */
#ifndef MHTRI_UNSPLIT_SC_H
#define MHTRI_UNSPLIT_SC_H

#include "types.h"
#include "SC/sc.h"

#ifdef __cplusplus
extern "C" {
#endif

/* size: 0x09 - the idle-mode record `SCGetIdleMode` fills (`SCFindByteArrayItem(info, 2, 9)`); only
 * `subIdle` is read by the NWC24 band.  The field names past `idle` are derived from the record's own
 * order (a GUESS - the image carries no spelling for them). */
typedef struct SCIdleModeInfo {
    /* +0x00 */ u8 idle;
    /* +0x01 */ u8 subIdle;
    /* +0x02 */ u8 subIdle2;
    /* +0x03 */ u8 disc;
    /* +0x04 */ u8 pad_0x04[0x05];
} SCIdleModeInfo; /* size: 0x09 */

/* 0x804DCAC0 - copy the idle-mode record out of the SC configuration. */
BOOL SCGetIdleMode(SCIdleModeInfo* info);


#ifdef __cplusplus
}
#endif

#endif /* MHTRI_UNSPLIT_SC_H */
