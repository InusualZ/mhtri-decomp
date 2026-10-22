/* `mailbox_unread_ck`, defined by `menu/menu_plsearch.cpp` (leaf header, rule 2). */
#ifndef MHTRI_MENU_MAILBOX_UNREAD_CK_H
#define MHTRI_MENU_MAILBOX_UNREAD_CK_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x8044D9E4 (0x80): whether the lobby mailbox holds an unread mail. */
s32 mailbox_unread_ck(void);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_MENU_MAILBOX_UNREAD_CK_H */
