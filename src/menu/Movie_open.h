/* Leaf header (docs/plan.md 6.5 rule 2): the `menu/movie.cpp` playback entry points the game-mode flow calls.  C++
 * linkage (the map rows are `Movie_open__F10MovieIndex`, `Movie_draw_sub__Fv`, `Movie_close__Fv`,
 * `Check_movie_finish__Fv`). */
#ifndef MHTRI_MENU_MOVIE_OPEN_H
#define MHTRI_MENU_MOVIE_OPEN_H

#include "types.h"

/* The movie a `Movie_open` call plays; only the opening's index is known (GUESS name). */
enum MovieIndex {
    MOVIE_INDEX_OPENING = 4
};

/* 0x8043D524 - opens movie `index`; 0x8043D7A0 - its draw callback; 0x8043D7A4 - closes it; 0x8043D7E4 - 1 once it
 * has finished. */
void Movie_open(MovieIndex index);
void Movie_draw_sub(void);
void Movie_close(void);
u32 Check_movie_finish(void);

#endif /* MHTRI_MENU_MOVIE_OPEN_H */
