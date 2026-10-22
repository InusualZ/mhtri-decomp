/* pmic_stop.h - leaf header (docs/plan.md 6.5 rule 2): `mh3_pad.cpp`'s microphone stop, for `menu/movie.cpp`.  GUESS name (the registry carries it). */
#ifndef MHTRI_PMIC_STOP_H
#define MHTRI_PMIC_STOP_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x80041408 - stops the P-Mic (a tail call to PMICStop). */
void pmic_stop(void);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_PMIC_STOP_H */
