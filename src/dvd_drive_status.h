/* dvd_drive_status.h - leaf header (docs/plan.md 6.5 rule 2): `main.cpp`'s drive status, power requests and frame end, for `menu/movie.cpp`.  GUESS names (the registry carries them). */
#ifndef MHTRI_DVD_DRIVE_STATUS_H
#define MHTRI_DVD_DRIVE_STATUS_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x80794798 (.sbss) - the drive status the disc-error code switches on (the DVD_STATE_* values). */
extern s32 dvd_drive_status;
/* 0x807947A5 / 0x807947A6 (.sbss) - set when the console asks for a restart / a shutdown. */
extern u8 reset_request;
extern u8 shutdown_request;
/* 0x8003FF98 - copies the finished frame to the display buffer. */
void frame_copy_display(void);
/* 0x80040144 - swaps to the next frame buffer and waits for the retrace. */
void frame_swap_wait(void);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_DVD_DRIVE_STATUS_H */
