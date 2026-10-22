/* ef/sys_tex_file_table.h - the declaration of `ef/sys_tex_file_table.cpp`'s table (docs/plan.md 6.5 rule 2). */
#ifndef MHTRI_EF_SYS_TEX_FILE_TABLE_H
#define MHTRI_EF_SYS_TEX_FILE_TABLE_H

#include "types.h"
#include "quest/quest_file_table.h" /* ResFileEntry */

/* The system texture files and their byte sizes: `00/system.tpl`, `00/inittex.tpl`, `00/loading.tpl` (the loading
 * display's texture `ef/system_core.cpp` loads), then an empty row. */
extern ResFileEntry sys_tex_file_table[4];

#endif /* MHTRI_EF_SYS_TEX_FILE_TABLE_H */
