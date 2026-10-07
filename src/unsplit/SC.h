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

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_UNSPLIT_SC_H */
