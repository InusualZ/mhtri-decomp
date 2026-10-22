/* menu/plsearch_work_init.h - leaf header (docs/plan.md 6.5 rule 2): `menu/menu_plsearch.cpp`'s lobby message menu, for `menu/movie.cpp`.  C linkage (the map rows are plain names). */
#ifndef MHTRI_MENU_PLSEARCH_WORK_INIT_H
#define MHTRI_MENU_PLSEARCH_WORK_INIT_H

#include "types.h"

struct PlMessageWork;
struct LobbyMailRow;

#ifdef __cplusplus
extern "C" {
#endif

/* 0x806F3488 (.bss) - the lobby message menu's work. */
extern struct PlMessageWork plsearch_message_work;
/* 0x8044E2CC - opens the message menu on `mail` (`active` 1: a new message). */
void plsearch_work_init(struct PlMessageWork* work, u8 active, const struct LobbyMailRow* mail);
/* 0x8044E340 / 0x8044EC08 - steps (2 closed, 4 failed) and draws the message menu. */
s32 plsearch_step(struct PlMessageWork* work);
void plsearch_draw(struct PlMessageWork* work);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_MENU_PLSEARCH_WORK_INIT_H */
