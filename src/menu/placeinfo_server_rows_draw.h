/* menu/placeinfo_server_rows_draw.h - leaf header (docs/plan.md 6.5 rule 2): `menu/menu_placeinfo.cpp`'s connection rows, for `menu/movie.cpp`.  GUESS names (the registry carries them). */
#ifndef MHTRI_MENU_PLACEINFO_SERVER_ROWS_DRAW_H
#define MHTRI_MENU_PLACEINFO_SERVER_ROWS_DRAW_H

#include "types.h"

struct NetPeerCard;

#ifdef __cplusplus
extern "C" {
#endif

/* 0x8043C0E4 / 0x8043C3B4 - draw the server, account and nickname rows / the settings rows at layout `id`. */
void placeinfo_server_rows_draw(u16 id, u8 flag, struct NetPeerCard* card);
void placeinfo_settings_rows_draw(u16 id, u8 flag, struct NetPeerCard* card);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_MENU_PLACEINFO_SERVER_ROWS_DRAW_H */
