/* kbd_close_call.h - leaf header (docs/plan.md 6.5 rule 2): `fn_80040598.cpp`'s keyboard close, for `menu/movie.cpp`.  C linkage (the map row is a plain name). */
#ifndef MHTRI_KBD_CLOSE_CALL_H
#define MHTRI_KBD_CLOSE_CALL_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x8004082C - closes the software keyboard. */
void kbd_close_call(void);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_KBD_CLOSE_CALL_H */
