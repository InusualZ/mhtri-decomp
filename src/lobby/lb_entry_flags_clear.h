#ifndef MHTRI_LOBBY_LB_ENTRY_FLAGS_CLEAR_H
#define MHTRI_LOBBY_LB_ENTRY_FLAGS_CLEAR_H

/* Leaf header of `lobby/lb_companion_ui.cpp`: the prototype has no types, so it can be included beside the
 * band header `unsplit/lobby.h` (the owner header `lobby/lb_companion_ui.h` spells its own `lobby_w`). */
#ifdef __cplusplus
extern "C" {
#endif

/* 0x80338A84 - clears the lobby entry flags. */
void lb_entry_flags_clear(void);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_LOBBY_LB_ENTRY_FLAGS_CLEAR_H */
