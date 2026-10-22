/* Leaf header (docs/plan.md 6.5 rule 2): a symbol `sound/fn_800D7F54.cpp` defines, for a consumer that cannot include
 * the owner's full header.  C linkage (the map row is a plain name). */
#ifndef MHTRI_SOUND_SE_REQ_FRAME_SET_BANK_H
#define MHTRI_SOUND_SE_REQ_FRAME_SET_BANK_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x800DA428 - requests SE (param & 0xFFFFFF) of the bank in param's top byte when the work's motion reaches `frame`.  GUESS name. */
void se_req_frame_set_bank(struct _se_w* work, s32 frame, u32 param, s32 d, s32 e);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_SOUND_SE_REQ_FRAME_SET_BANK_H */
