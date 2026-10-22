/* `cockpit_hold_block`, defined by `ef/eft052.cpp`'s `.bss` (leaf header, rule 2). */
#ifndef MHTRI_EF_COCKPIT_HOLD_BLOCK_H
#define MHTRI_EF_COCKPIT_HOLD_BLOCK_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* .bss 0x806BF368 - the cockpit hold block (`CockpitHoldBlock` in the owner), seen as its bytes; the first is the
 * strip's mode. */
extern u8 cockpit_hold_block[0x58];

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_EF_COCKPIT_HOLD_BLOCK_H */
