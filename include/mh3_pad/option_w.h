/*
 * `option_w` - the game option table (.bss 0x80659090, 0x24 B), defined by `src/mh3_pad.cpp` (its `.bss`
 * 0x806585B8-0x806694E8 is that unit's own).  One byte per option id: `get_option_cfg`/`ck_option_cfg`
 * (`menu/get_pop_dat_ptr.cpp`) read it unsigned (`lbzx`), index 20 inverted.  The one declaration of the
 * object (rule 2); the element type stays a byte array because no reader names a field of it.
 */
#ifndef MHTRI_MH3_PAD_OPTION_W_H
#define MHTRI_MH3_PAD_OPTION_W_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

extern u8 option_w[0x24];

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_MH3_PAD_OPTION_W_H */
