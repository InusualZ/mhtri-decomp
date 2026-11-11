/*
 * ef/sys_tex_file_table.cpp - the system texture file table: the three `00/` tpl names and their byte sizes.
 * RANGE. no .text; .data 0x8058AF44-0x8058AF98 (the three names, then `sys_tex_file_table` at 0x8058AF74).  Left edge: `draw_shape.cpp`'s
 *   `.data` ends at 0x8058AF44; right edge: `quest/quest_file_table.cpp`'s starts at 0x8058AF98.  Its only referrer is
 *   `ef/system_core.cpp` (the loading display's reset reads row 2), whose own `.data` is 0x80595118-0x80595378, so the
 *   run is a data-only unit placed where its bytes are, like its right neighbour.
 * FLAGS. `cflags_main`, as `ef/system_core.cpp`.
 * NAMES. GUESS: the file name and `sys_tex_file_table` (the rows' contents and their reader).
 * RESIDUALS. none in this unit.  `ef/system_core.cpp`'s `loading_disp_reset` keeps two load offsets retail does not
 *   (`0x18`/`0x14(r30)` against retail's `0x14`/`0x10(r30)` with `sys_tex_file_table+0x4` as the base): MWCC folds the
 *   `entries` offset into the load where retail materialises the array's address.
 * SHAPES. The table is a 0x24-byte object (a zero word, then the four rows), so it stays 4-aligned and the run starts
 *   at 0x8058AF74 (not an 8-aligned array, playbook 146); the strings precede it because the initialiser emits them.
 */

#include "ef/sys_tex_file_table.h"

SysTexFileTable sys_tex_file_table = {
    0,
    {
        { 0xC5000, "00/system.tpl" },
        { 0xD8DC0, "00/inittex.tpl" },
        { 0x40000, "00/loading.tpl" },
        { 0, NULL },
    },
};
