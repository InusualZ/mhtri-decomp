/*
 * menu/menu_sysmsg.h - the `.data` symbol other units read from `menu/menu_sysmsg.cpp`'s system-message run
 *   (0x80607E50-0x8060E7FC); the unit has no bodies yet.
 */
#ifndef MHTRI_MENU_MENU_SYSMSG_H
#define MHTRI_MENU_MENU_SYSMSG_H

#include "types.h"

/* .data 0x8060DAD8 - the per-language "no grade" text table, indexed by `system_w`'s map index (read by the arena
 * result text getters of `quest/quest_entry.cpp`). */
extern char* quest_grade_none_text_table[];

/* .data 0x8060DAC0 - the per-language unlock notice tables the quest-result screen's unlock page reads
 * (`[language][notice]`, six languages).  GUESS name. */
extern char** q_result_unlock_text_tbl[];

#endif /* MHTRI_MENU_MENU_SYSMSG_H */
