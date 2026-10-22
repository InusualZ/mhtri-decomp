/* `lb_talk_page_value_set`, defined by `lobby/fn_80219260.cpp` (leaf header, rule 2). */
#ifndef MHTRI_LOBBY_LB_TALK_PAGE_VALUE_SET_H
#define MHTRI_LOBBY_LB_TALK_PAGE_VALUE_SET_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Sets the talk page's value (both the current and the answered word `lb_talk_page_value_get` reads). */
void lb_talk_page_value_set(u8 value);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_LOBBY_LB_TALK_PAGE_VALUE_SET_H */
