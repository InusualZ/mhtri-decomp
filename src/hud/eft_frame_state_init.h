/* hud/eft_frame_state_init.h - the declarations of the frame effect's four state bodies, which `hud/pl_frame_sync.cpp`
 * owns (docs/plan.md 6.5 rule 2). */
#ifndef MHTRI_HUD_EFT_FRAME_STATE_INIT_H
#define MHTRI_HUD_EFT_FRAME_STATE_INIT_H

#include "types.h"

struct _EFT;

#ifdef __cplusplus
extern "C" {
#endif
/* 0x8033041C / 0x803305F8 / 0x80330E00 / 0x80330E10 - the frame effect's states 0-3: set up, step, end and the
 * empty state. */
void eft_frame_state_init(struct _EFT* self);
void eft_frame_state_step(struct _EFT* self);
void eft_frame_state_end(struct _EFT* self);
void eft_frame_state_nop(struct _EFT* self);
#ifdef __cplusplus
}
#endif

#endif /* MHTRI_HUD_EFT_FRAME_STATE_INIT_H */
