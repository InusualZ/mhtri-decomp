/*
 * enemy/em020_ai.h - declarations `enemy/em020_prog.cpp` owns that other units call: `getInstance_`,
 * `fn_803768F8`, `fn_80377664`, `em020_aim_target_ck` and `em020_hit_info_get`.
 */
#ifndef MHTRI_ENEMY_EM020_AI_H
#define MHTRI_ENEMY_EM020_AI_H

#include "types.h"

/* The network singleton `getInstance_` returns (`mpInstance__12PatInterface`): the class is
 * `Network/PatInterface.h`'s, and only the tag is needed here. */
class PatInterface;
/* The shared enemy work record `em020_aim_target_ck` takes a pointer to. */
struct _ENEMY_WORK;

#ifdef __cplusplus
extern "C" {
#endif

/* 0x803768F0 - the network singleton (an 8-byte accessor). */
PatInterface* getInstance_(void);
/* 0x803768F8 - the lobby's "can leave the map" predicate (`lobby_w` +0x163/+0x16F/+0x0B0). */
u32 fn_803768F8(void);
/* 0x80377664 - the network big-data request entry (the `fn_804273EC(1, 0, 0)` wrapper). */
s32 fn_80377664(void* unused);
/* 0x803754F4 - the "aim target found" predicate (`+0x836` bit 15). */
u32 em020_aim_target_ck(struct _ENEMY_WORK* self);
/* The out record `em020_hit_info_get` fills (the first 8 bytes of a `Q_QuestStat`).  size: 0x08 */
struct Em020HitInfo {
    /* +0x0 */ u8 hit_0x00;
    /* +0x1 */ u8 levels_0x01;
    /* +0x2 */ s16 angle_0x02;
    /* +0x4 */ u32 damage_0x04;
};
/* 0x80375540 - the em020 area hit's damage-level gate. */
void em020_hit_info_get(struct _ENEMY_WORK* self, struct Em020HitInfo* out);

#ifdef __cplusplus
}
#endif

#endif
