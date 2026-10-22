/* enemy/em035_init.h - the declarations of em035's program head, which `enemy/em033_prog.cpp` defines (docs/plan.md
 * 6.5 rule 2): the three `em035_prog_tbl` entries at 0x8035F060-0x8035F2B4. */
#ifndef MHTRI_ENEMY_EM035_INIT_H
#define MHTRI_ENEMY_EM035_INIT_H

#include "types.h"

struct _ENEMY_WORK;

#ifdef __cplusplus
extern "C" {
#endif
/* 0x8035F060 - arms the action from mode 2 or 0 (the table's init slot, GUESS name). */
void em035_init(struct _ENEMY_WORK* work, u8 mode);
/* 0x8035F174 - an empty body (the table's +0x10 slot). */
void em035_noop(void);
/* 0x8035F178 - the table's (kind, sub) event slot (GUESS name). */
void em035_event_notify(struct _ENEMY_WORK* work, u8 kind, u8 sub);
#ifdef __cplusplus
}
#endif

#endif /* MHTRI_ENEMY_EM035_INIT_H */
