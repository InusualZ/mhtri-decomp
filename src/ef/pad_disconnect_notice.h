/* Leaf header (docs/plan.md 6.5 rule 2): `ef/system_core.cpp`'s `pad_disconnect_notice`, which `Pl/pl_motion.cpp` calls. */
#ifndef MHTRI_EF_PAD_DISCONNECT_NOTICE_H
#define MHTRI_EF_PAD_DISCONNECT_NOTICE_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x800D2D50 (0xD8): pushes channel `channel`'s pad-disconnect message once and clears it on reconnect. GUESS name. */
void pad_disconnect_notice(u8 channel);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_EF_PAD_DISCONNECT_NOTICE_H */
