#ifndef QUEST_QUEST_FILE_TABLE_H
#define QUEST_QUEST_FILE_TABLE_H

#include "types.h"

/* One row of a file-size table: the byte size a loader allocates and the file's name ("" when the
 * loader builds the name itself, NULL in an unused row). size: 0x8 */
typedef struct ResFileEntry {
    /* +0x00 */ u32 size;
    /* +0x04 */ const char* name;
} ResFileEntry; /* size: 0x8 */

/* The quest list files `quest_list_load_hunt`/`quest_list_load_arena` load, indexed by list kind. */
extern ResFileEntry quest_file_table[4];

/* The per-stage `07/dcm%03d.bin` archive sizes, indexed by stage number (row 0 unused). */
extern ResFileEntry stage_dcm_file_table[50];

#endif
