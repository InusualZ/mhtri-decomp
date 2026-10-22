/* Leaf header (docs/plan.md 6.5 rule 2): a symbol `sound/fn_800D7F54.cpp` defines, for a consumer that cannot include
 * the owner's full header.  C linkage (the map row is a plain name). */
#ifndef MHTRI_SOUND_SE_TALK_BANK_REQ_H
#define MHTRI_SOUND_SE_TALK_BANK_REQ_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x800DBC0C - requests SE bank 0x25 (the talk window's).  GUESS name. */
void se_talk_bank_req(void);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_SOUND_SE_TALK_BANK_REQ_H */
