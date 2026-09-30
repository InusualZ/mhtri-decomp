/*
 * menu/arena_result.h - the published interface of `src/menu/arena_result.cpp` (the quest/arena result
 * record accessors, `.text` band at the head of the menu result screen).
 *
 * Only the accessors other units call are published so far: `quest_record_get` hands back the record the
 * quest work block's `record_0x03C` points at (`quest/arenatask.cpp`'s `dl_acdata_to_ar_eqdata` reads its
 * acdata offset) and `quest_record_find` looks a record up by quest id (`arena_quest_info_build`).  The record type stays in the band header `include/unsplit/menu.h`.
 */
#ifndef MHTRI_MENU_ARENA_RESULT_H
#define MHTRI_MENU_ARENA_RESULT_H

#include "types.h"
#include "unsplit/menu.h"
#include "menu/quest_str_tbl_35_get.h"

#ifdef __cplusplus
extern "C" {
#endif

/* The current result row, or NULL when the screen has none. */
QuestRecord* quest_record_get(void);

/* The result record of quest `quest_id` from the loaded quest list (or the network control's slots), or
 * NULL when none carries that id. */
QuestRecord* quest_record_find(u16 quest_id);

/* 0x803B33B0 / 0x803B33E8 - the quest message string tables 35 and 4: the string at `index`
 * (`quest_str_tbl_35_get` is declared in the leaf header below). */
char* quest_str_tbl_4_get(u32 index);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_MENU_ARENA_RESULT_H */
