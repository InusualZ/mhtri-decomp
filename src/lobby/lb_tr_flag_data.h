/* `lb_tr_flag_data`, defined by `lobby/lb_npc.cpp` (leaf header, rule 2). */
#ifndef MHTRI_LOBBY_LB_TR_FLAG_DATA_H
#define MHTRI_LOBBY_LB_TR_FLAG_DATA_H

#include "types.h"

struct LbTrFlag;

#ifdef __cplusplus
extern "C" {
#endif

/* .sbss 0x80794AF4 - the `_tr_flag` goods table (records of `struct LbTrFlag`, ended by id 0xFFFF). */
extern struct LbTrFlag* lb_tr_flag_data;

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_LOBBY_LB_TR_FLAG_DATA_H */
