/* hud/hud_mail_notice_clear.h - leaf header (docs/plan.md 6.5 rule 2): `hud/cockpit.cpp`'s `hud_mail_notice_clear`.
 *   C linkage (the map row is a plain name).  GUESS name: it clears HUD notice 1, which the mailbox clears once no
 *   mail is unread. */
#ifndef MHTRI_HUD_HUD_MAIL_NOTICE_CLEAR_H
#define MHTRI_HUD_HUD_MAIL_NOTICE_CLEAR_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x802DFC74 (0x8): clears the HUD's mail notice. */
void hud_mail_notice_clear(void);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_HUD_HUD_MAIL_NOTICE_CLEAR_H */
