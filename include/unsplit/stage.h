/*
 * The stage-work block (`stage_w`, `.bss` 0x806B87C0, 0x2FE0 B) as a light header (`include/unsplit/<band>.h`,
 * the fallback home of a symbol no registered unit owns).
 *
 * `include/stage/fn_802B2AA0.h` carries the full `StageRuntime` view but pulls `sound/mhchar.h`, whose
 * `_GXChannelID` enum and `MHchar` record clash with the ones `pl.h` defines, so a translation unit that
 * includes `pl.h` (the arena task) cannot include it.  The symbol's declaration lives here and that header
 * includes this one; the view below names only the two bytes `StageRuntime` calls `mapno` / `areano` (+0xBC5 /
 * +0xBC6), and the one full layout stays in `stage/fn_802B2AA0.h`.
 */
#ifndef MHTRI_UNSPLIT_STAGE_H
#define MHTRI_UNSPLIT_STAGE_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* size: 0x2FE0 */
typedef struct StageMapView {
    /* +0x0000 */ u8 pad_0x0000[0xBC5];
    /* +0x0BC5 */ u8 mapno;          /* the current map number (0xFF = none) */
    /* +0x0BC6 */ u8 areano;         /* the current area number */
    /* +0x0BC7 */ u8 pad_0x0BC7[0x2419];
} StageMapView; /* size: 0x2FE0 */

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_UNSPLIT_STAGE_H */
