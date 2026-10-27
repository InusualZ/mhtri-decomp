/* Leaf header (docs/plan.md 6.5 rule 2): `ef/system_core.cpp`'s `display_state_hook_call` (registry name, map row `display_state_hook_call`). */
#ifndef MHTRI_EF_DISPLAY_STATE_HOOK_CALL_H
#define MHTRI_EF_DISPLAY_STATE_HOOK_CALL_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x800D2928 (0x2C): calls `system_w`'s display-state hook (+0xA54) with the transfer display state while one is set. */
void display_state_hook_call(void);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_EF_DISPLAY_STATE_HOOK_CALL_H */
