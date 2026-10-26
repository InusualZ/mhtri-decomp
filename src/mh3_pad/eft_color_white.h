/* `eft_color_white` and `eft_color_red`, two RGBA words in `src/mh3_pad.cpp`'s `.sdata` (leaf header, docs/plan.md 6.5
 * rule 2).  GUESS names: the words are 0xFFFFFFFF and 0xED1C24FF, and their one reader is the static initialiser of
 * `menu/get_pop_dat_ptr.cpp`'s effect colours. */
#ifndef MHTRI_MH3_PAD_EFT_COLOR_WHITE_H
#define MHTRI_MH3_PAD_EFT_COLOR_WHITE_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* .sdata 0x80790E7C / 0x80790E80 */
extern u32 eft_color_white;
extern u32 eft_color_red;

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_MH3_PAD_EFT_COLOR_WHITE_H */
