/* `lb_tr_flag_get`, defined by `lobby/fn_8030121C.cpp` (leaf header, rule 2). */
#ifndef MHTRI_LOBBY_LB_TR_FLAG_GET_H
#define MHTRI_LOBBY_LB_TR_FLAG_GET_H

#include "types.h"

struct LbTrFlag;

#ifdef __cplusplus
extern "C" {
#endif

/* 0x803035F8 - the `_tr_flag` record of goods `id` (the table's first record when none matches). */
struct LbTrFlag* lb_tr_flag_get(u16 id);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_LOBBY_LB_TR_FLAG_GET_H */
