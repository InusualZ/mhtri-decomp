/* The per-enemy data record `get_enemy_data` returns, shared by `include/enemy.h` and the net sync
 * (`hud/net_char_sync.cpp`) that reads its kind byte (docs/plan.md 6.5 rule 1: one definition, included).
 * Moved here from `include/enemy.h`; the definition is unchanged except that its +0x00F byte is named.
 */
#ifndef MHTRI_ENEMY_ENEMYDATA_H
#define MHTRI_ENEMY_ENEMYDATA_H

#include "types.h"

typedef struct EmDataRecord EmDataRecord;   /* enemy/fn_80138074.c */
typedef struct UserDataItem UserDataItem;   /* enemy/fn_80138074.c */
typedef struct EnemyExtraData EnemyExtraData; /* enemy/fn_8012BDF4.cpp */

/* The per-enemy data record `get_enemy_data` returns.  Union of `enemy/fn_8012BDF4.cpp` and
 * `enemy/fn_80138074.c`; the two views are compatible (a pointer table at +0x04, the value list at
 * +0x64, the user-data items at +0x94 and the extra block at +0xA0). size: 0xA4 */
typedef struct EnemyData {
    /* +0x000 */ u8 field_0x00;
    /* +0x001 */ u8 pad_0x1[0x3];
    /* +0x004 */ EmDataRecord* records;
    /* +0x008 */ u16 field_0x08;
    /* +0x00A */ u16 field_0x0A;
    /* +0x00C */ u8 pad_0xC[0x2];
    /* +0x00E */ u8 field_0x0E;
    /* +0x00F */ u8 kind_0x0F;      /* the enemy kind the net sync packs special cases for */
    /* +0x010 */ u8 pad_0x10[0x4];
    /* +0x014 */ u32 field_0x14;
    /* +0x018 */ u8 pad_0x18[0x4];
    /* +0x01C */ u32 field_0x1C;
    /* +0x020 */ u32 field_0x20;
    /* +0x024 */ u32 field_0x24;
    /* +0x028 */ u8 pad_0x28[0x8];
    /* +0x030 */ u32 bits_0x30;     /* copied into `_ENEMY_WORK::bits_0x824` by fn_8011E5EC */
    /* +0x034 */ u8 pad_0x34[0x30];
    /* +0x064 */ u8* values;
    /* +0x068 */ u8 pad_0x68[0x2C];
    /* +0x094 */ UserDataItem* items;
    /* +0x098 */ s32* table_0x98;
    /* +0x09C */ u8 pad_0x9C[0x4];
    /* +0x0A0 */ EnemyExtraData* extra;
} EnemyData;

#endif /* MHTRI_ENEMY_ENEMYDATA_H */
