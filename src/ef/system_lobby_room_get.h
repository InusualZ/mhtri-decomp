/* Leaf header (docs/plan.md 6.5 rule 2): `ef/system_core`'s `system_lobby_room_get`.  GUESS name (camera/camera_main.cpp, the unit header). */
#ifndef MHTRI_EF_SYSTEM_LOBBY_ROOM_GET_H
#define MHTRI_EF_SYSTEM_LOBBY_ROOM_GET_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x800D0744 (ef/system_core): the system work's +0x886 byte, the lobby room the camera picks its distance by. */
u8 system_lobby_room_get(void);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_EF_SYSTEM_LOBBY_ROOM_GET_H */
