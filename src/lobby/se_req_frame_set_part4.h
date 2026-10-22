/* Leaf header (docs/plan.md 6.5 rule 2): a symbol `lobby/lb_equip_page.cpp` defines, for a consumer that cannot include
 * the owner's full header.  C linkage (the map row is a plain name). */
#ifndef MHTRI_LOBBY_SE_REQ_FRAME_SET_PART4_H
#define MHTRI_LOBBY_SE_REQ_FRAME_SET_PART4_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x80229EA8 - requests the frame SE of the fourth armour part (+0xA40). */
void se_req_frame_set_part4(struct _se_w* work, s32 a, s32 b, s32 c);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_LOBBY_SE_REQ_FRAME_SET_PART4_H */
