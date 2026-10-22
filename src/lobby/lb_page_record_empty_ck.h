/* `lb_page_record_empty_ck`, defined by `lobby/lobby_scene.c` (leaf header, rule 2). */
#ifndef MHTRI_LOBBY_LB_PAGE_RECORD_EMPTY_CK_H
#define MHTRI_LOBBY_LB_PAGE_RECORD_EMPTY_CK_H

#include "types.h"

struct _PLW;

#ifdef __cplusplus
extern "C" {
#endif

/* Nonzero when the player's equipment records are settled (the shop waits for it before taking input). */
u32 lb_page_record_empty_ck(struct _PLW* plw);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_LOBBY_LB_PAGE_RECORD_EMPTY_CK_H */
