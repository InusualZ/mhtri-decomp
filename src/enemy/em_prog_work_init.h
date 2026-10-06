/* Leaf header (docs/plan.md 6.5 rule 2): the `enemy/em_prog_support.cpp` symbols `enemy/em_pop.cpp` calls, for a consumer that cannot
 * include the owner's full header.  C linkage (the map rows are plain names). */
#ifndef MHTRI_ENEMY_EM_PROG_WORK_INIT_H
#define MHTRI_ENEMY_EM_PROG_WORK_INIT_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x80385068 / 0x803852B8 - set the program slots and the program work up (GUESS names). */
void em_prog_slots_init(void);
void em_prog_work_init(void);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_ENEMY_EM_PROG_WORK_INIT_H */
