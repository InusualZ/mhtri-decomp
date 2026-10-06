/* Leaf header (docs/plan.md 6.5 rule 2): the `enemy/em_common.cpp` symbols the quest spawns call, for a consumer
 * that cannot include the owner's full header.  C linkage (the map rows are plain names). */
#ifndef MHTRI_ENEMY_EM_QUEST_ELEMENT_SET_H
#define MHTRI_ENEMY_EM_QUEST_ELEMENT_SET_H

#include "types.h"

struct _ENEMY_WORK;

/* 0x80137604 / 0x8013760C - latch the quest element index into the record's +0x0D byte (GUESS names; the quest
 * spawns pass the record and the element index).  `s8` is the callers' view (they pass the index unmasked); the
 * owner's definition spells `u8` because an `s8` parameter there adds an `extsb` retail does not have. */
#ifdef __cplusplus
extern "C" {
#endif
void em_quest_element_set(struct _ENEMY_WORK* self, s8 value);
void em_quest_element_set_large(struct _ENEMY_WORK* self, s8 value);
#ifdef __cplusplus
}
#endif

#endif /* MHTRI_ENEMY_EM_QUEST_ELEMENT_SET_H */
