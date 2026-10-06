/*
 * quest/quest_file_table.cpp - the quest list file table and the per-stage dcm archive size table.
 * RANGE. no .text; .data 0x8058AF98-0x8058B178 (the three quest list names, `quest_file_table`,
 *   `stage_dcm_file_table`), .sdata 0x807910F8-0x80791100 (the "" name both tables point at).
 * RANGE. Left edge 0x8058AF98: the 0x8058AF44-0x8058AF98 run (the `00/` tpl names and their table) is
 *   read only by `ef/system_core.cpp` and stays outside this unit.  Right edge: `fn_80056F24.cpp`'s
 *   `.data` starts at 0x8058B178.  The `.sdata` word sits between `draw_shape.cpp` and
 *   `draw_shape_arm.cpp`, and its only references are the 50 pointer words of these two tables.  A claim
 *   of this range inside `quest/quest_entry.cpp` makes `dtk dol split` refuse a cyclic link order, so it
 *   is a data-only unit placed where its bytes are.
 * RANGE. Referrers: `quest_file_table` from `quest_list_load_hunt`/`quest_list_load_arena`
 *   (quest/quest_entry.cpp); `stage_dcm_file_table` from `quest/quest_entry.cpp`, `enemy/em_pop.cpp`,
 *   `lobby/lb_pane_ui.cpp` and `menu/menu_plsearch.cpp`.
 * FLAGS. the `menu` lib's `cflags_menu`, as the quest units it serves.
 * NAMES. The file name and `quest_file_table`, `stage_dcm_file_table`, `ResFileEntry` are GUESSes (the
 *   rows' contents and their loaders).
 * RESIDUALS. flipcheck: .sdata is 0x1 against the claimed 0x8 (the "" byte; the other seven are the
 *   alignment fill before `draw_shape_arm.cpp`'s 8-aligned .sdata).  .data is byte-identical.
 * SHAPES. Each table is initialised with its literals, so MWCC emits the strings before the table.
 */

#include "quest/quest_file_table.h"

/* The hunt lists (low and high tier), an empty variant and the arena list. */
ResFileEntry quest_file_table[4] = {
    { 0x3A540, "05/quest00.bin" },
    { 0x64180, "05/quest01.bin" },
    { 0, "" },
    { 0x3BC0, "05/quest03.bin" },
};

ResFileEntry stage_dcm_file_table[50] = {
    { 0, NULL },
    { 0x2800, "" }, { 0x2800, "" }, { 0x2800, "" }, { 0x4000, "" }, { 0x4000, "" },
    { 0x4000, "" }, { 0x2800, "" }, { 0x2800, "" }, { 0x4000, "" }, { 0x4000, "" },
    { 0x2800, "" }, { 0x2800, "" }, { 0x4000, "" }, { 0x2800, "" }, { 0x4400, "" },
    { 0x2800, "" }, { 0x2800, "" }, { 0x4000, "" }, { 0x4000, "" }, { 0x4000, "" },
    { 0xB400, "" }, { 0xA000, "" }, { 0xA000, "" }, { 0xC800, "" }, { 0xD000, "" },
    { 0x2800, "" }, { 0xA000, "" }, { 0xB400, "" }, { 0x6400, "" }, { 0xA000, "" },
    { 0xB400, "" }, { 0xC800, "" }, { 0xC800, "" }, { 0xD000, "" }, { 0xC800, "" },
    { 0xA000, "" }, { 0xD000, "" }, { 0xD000, "" }, { 0xD400, "" }, { 0x5000, "" },
    { 0xA000, "" }, { 0x4000, "" }, { 0xA000, "" }, { 0xA000, "" }, { 0x5000, "" },
    { 0x5000, "" }, { 0xD000, "" }, { 0x2800, "" }, { 0xA000, "" },
};
