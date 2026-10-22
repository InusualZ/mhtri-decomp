/* get_userdata.h - leaf header (docs/plan.md 6.5 rule 2): `fn_8004CAD8.cpp`'s `get_userdata` (0x8004D120), for
 *   `menu/menu_placeinfo.cpp`.  C++ linkage (the map row is the mangling `get_userdata__Fv`); the return type is the
 *   one `quest/quest_entry.h` spells, so the two declarations agree where both are seen. */
#ifndef MHTRI_GET_USERDATA_H
#define MHTRI_GET_USERDATA_H

#include "types.h"

#ifdef __cplusplus
struct Q_UserData;

/* The save/user block (the record `lobby_world_block` points at). */
struct Q_UserData* get_userdata(void);
#endif

#endif /* MHTRI_GET_USERDATA_H */
