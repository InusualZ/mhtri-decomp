/* Leaf header (docs/plan.md 6.5 rule 2): `ef/eft052.cpp`'s item hand-over window and item box, as the lobby's note trade
 * calls them.  C linkage (the map rows are plain names). */
#ifndef MHTRI_EF_EFT052_ITEM_GET_OPEN_H
#define MHTRI_EF_EFT052_ITEM_GET_OPEN_H

#include "types.h"
#include "ef/eft052_box.h"   /* the request and item box records */


#ifdef __cplusplus
extern "C" {
#endif

/* 0x80359D98 - opens the item hand-over window on `req` with the window data `data` (GUESS name). */
void eft052_item_get_open(Eft052ItemGetReq* req, u8* data);
/* 0x8035A034 - one step of the window; 1 or 2 once it is closed (GUESS name). */
s32 eft052_item_get_step(void);
/* 0x8035A7D8 - draws the item box's list at panel `panel` with its sprite rows (GUESS name). */
void eft052_box_list_draw(u16 panel, const u16* sprites, const u16* rows, u16 help, u8 flag);
/* The item box (GUESS name). */
extern Eft052ItemBox eft052_item_box;

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_EF_EFT052_ITEM_GET_OPEN_H */
