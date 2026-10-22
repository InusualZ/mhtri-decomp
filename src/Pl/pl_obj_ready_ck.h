/* Pl/pl_obj_ready_ck.h - leaf header (docs/plan.md 6.5 rule 2): `Pl/player_control.cpp`'s player object state, for `menu/movie.cpp`.  GUESS names (the registry carries them). */
#ifndef MHTRI_PL_PL_OBJ_READY_CK_H
#define MHTRI_PL_PL_OBJ_READY_CK_H

#include "types.h"

struct _PLOBJ;
struct _PLGLOBAL;

#ifdef __cplusplus
extern "C" {
#endif

/* 0x80269394 - non-zero once player object `obj` is ready. */
s32 pl_obj_ready_ck(struct _PLOBJ* obj);
/* 0x80794B28 (.sbss) - the player-global record. */
extern struct _PLGLOBAL* pl_global;

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_PL_PL_OBJ_READY_CK_H */
