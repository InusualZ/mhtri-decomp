/* `lb_entry_unlock_flags`, defined by `lobby/lb_companion_ui.cpp` (leaf header, rule 2). */
#ifndef MHTRI_LOBBY_LB_ENTRY_UNLOCK_FLAGS_H
#define MHTRI_LOBBY_LB_ENTRY_UNLOCK_FLAGS_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* The progress flag each companion entry (mask kind) is unlocked by; 0xFFFF is always unlocked. */
extern u16 lb_entry_unlock_flags[8];

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_LOBBY_LB_ENTRY_UNLOCK_FLAGS_H */
