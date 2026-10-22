/*
 * ef/sys_tex_file_table.cpp - the system texture file table: the three `00/` tpl names and their byte sizes.
 * RANGE. no .text; .data 0x8058AF44-0x8058AF98 (the three names, then `sys_tex_file_table`).  Left edge: `draw_shape.cpp`'s
 *   `.data` ends at 0x8058AF44; right edge: `quest/quest_file_table.cpp`'s starts at 0x8058AF98.  Its only referrer is
 *   `ef/system_core.cpp` (the loading display's reset reads row 2), whose own `.data` is 0x80595118-0x80595378, so the
 *   run is a data-only unit placed where its bytes are, like its right neighbour.
 * FLAGS. `cflags_main`, as `ef/system_core.cpp`.
 * NAMES. GUESS: the file name and `sys_tex_file_table` (the rows' contents and their reader).
 * RESIDUALS. flipcheck: the target places the table at +0x34 (four zero bytes after the third name, so it sits
 *   8-aligned at 0x8058AF78) where ours emits it at +0x30; the names and rows are otherwise identical (12 of 0x50
 *   bytes differ).  No source shape that keeps the run one object reproduces the gap (playbook 146).
 * SHAPES. The table is initialised with its literals, so MWCC emits the strings before the table.
 */

#include "ef/sys_tex_file_table.h"

ResFileEntry sys_tex_file_table[4] = {
    { 0xC5000, "00/system.tpl" },
    { 0xD8DC0, "00/inittex.tpl" },
    { 0x40000, "00/loading.tpl" },
    { 0, NULL },
};
