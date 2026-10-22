/*
 * menu/sysmsg_text.h - the system message text tables `menu/sysmsg_text.cpp` defines, as the units that read them
 *   see them: each top table is indexed by `system_w`'s language index (six entries, the first two both English).
 */
#ifndef MHTRI_MENU_SYSMSG_TEXT_H
#define MHTRI_MENU_SYSMSG_TEXT_H

#include "types.h"
#include "menu/npc_talk_name_tbl.h" /* npc_talk_name_tbl (0x8060DDA0) */

/* .data 0x8060A960 - the disc, controller and save error messages, [language][message]. */
extern char** wii_sysmsg_table[];

/* .data 0x8060B1A8 - the save screen's messages, [language][message]. */
extern char** sysmsg_save_text[];

/* .data 0x8060B200 - the save screen's confirm button text, [language][0]. */
extern char** sysmsg_yes_text[];

/* .data 0x8060B2F0 - the data manager button text, [language][0]. */
extern char** sysmsg_manager_text[];

/* .data 0x8060DAC0 - the per-language unlock notice tables the quest-result screen's unlock page reads
 * (`[language][notice]`).  GUESS name. */
extern char** q_result_unlock_text_tbl[];

/* .data 0x8060DAD8 - the per-language "no grade" text, indexed by `system_w`'s language index (read by the arena
 * result text getters of `quest/quest_entry.cpp`). */
extern char* quest_grade_none_text_table[];

/* .data 0x8060DDB8 - the delivery table the lobby's companion screens read (0x1E0 bytes). */
extern u8 lb_deli_data[];

/* .data 0x8060DF98 - the delivery list entries (0x120 bytes). */
extern u8 lb_deli_list_data[];

#endif
