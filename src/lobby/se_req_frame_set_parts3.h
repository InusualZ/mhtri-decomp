/* Leaf header (docs/plan.md 6.5 rule 2): a symbol `lobby/lb_equip_page.cpp` defines, for a consumer that cannot include
 * the owner's full header.  C linkage (the map row is a plain name). */
#ifndef MHTRI_LOBBY_SE_REQ_FRAME_SET_PARTS3_H
#define MHTRI_LOBBY_SE_REQ_FRAME_SET_PARTS3_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x80229E10 - requests the armour-part frame SE `code` on each worn part (+0xA3D..+0xA3F). */
void se_req_frame_set_parts3(struct _se_w* work, s32 code, s32 val);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_LOBBY_SE_REQ_FRAME_SET_PARTS3_H */
