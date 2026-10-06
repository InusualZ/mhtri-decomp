/*
 * Declarations for the symbols `src/fn_80040598.cpp` owns (docs/plan.md 6.5 rule 2).  Both are the
 * game-root RSO loaders that unit defines: `fn_80040598` loads a module and publishes the pool top the
 * `mode` selects, `CntSdRsoTerminate` is the same load without the pool bookkeeping.  Consumers
 * (`src/mh3_pad.cpp`) include this header instead of re-declaring them.
 */
#ifndef MHTRI_FN_80040598_H
#define MHTRI_FN_80040598_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

void* fn_80040598(const char* path, void* buffer, u32 mode);
void* CntSdRsoTerminate(const char* path, void* buffer);

/* 0x8004080C - runs the system's keyboard reset hook when one is set (GUESS name). */
void kbd_reset_call(void);

#ifdef __cplusplus
}

/* 0x80040798 / 0x80040770 / 0x800407C4 - the keyboard dispatchers (C++ free functions: `kbd_open__FUc`, `kbd_move__Fv`,
 * `set_kbd_param__FPcUl` - rule 9): open keyboard `mode` (1 once it opened), step it (1 done, -1 cancelled) and hand
 * it the buffer it edits. */
u32 kbd_open(u8 mode);
s32 kbd_move(void);
void set_kbd_param(char* buffer, u32 length);
#endif

#ifdef __cplusplus
/* 0x8004074C / 0x80040854 - start and stop the USB keyboard layer (C++ scope: `kbd_init__FUc`, `kbd_exit__Fv`). */
void kbd_init(u8 mode);
void kbd_exit(void);
#endif

#endif /* MHTRI_FN_80040598_H */
