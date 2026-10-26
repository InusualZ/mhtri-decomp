/* mh3_pad/rc_record.h - the entry points of `mh3_pad/rc_record.c` (the GameCube pad poller that fills the RcRecord
 * table), for `mh3_pad.cpp`'s pad setup and frame read. */
#ifndef MHTRI_MH3_PAD_RC_RECORD_H
#define MHTRI_MH3_PAD_RC_RECORD_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x804A4D70: reads and clamps the four GameCube pads into the RcRecord table. */
void rc_record_read(void);
/* 0x804A4E40: initialises the GameCube pads and the RcRecord table. */
void rc_record_init(void);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_MH3_PAD_RC_RECORD_H */
