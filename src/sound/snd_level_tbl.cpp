/* sound/snd_level_tbl.cpp - the level-table lookup
 *
 * RANGE. `.text` 0x800E8E48..0x800E8E60 (1 function); `.data` 0x80597E20..0x80598020 (the level table).
 * NAMES. GUESS: `snd_level_to_db` and `snd_level_db_table` - one function indexing a 128-entry table of
 *   rising negative values ending at 0 (-904 .. 0, read as tenths of a decibel per volume level).
 * RESIDUALS. none.
 * SHAPES. The lookup is compiled under `#pragma peephole off`.
 */

#include "types.h"

/* `.data` 0x80597E20 (0x200): the attenuation of each of the 128 volume levels, in tenths of a decibel. */
s32 snd_level_db_table[128] = {
    -904, -540, -434, -365, -314, -283, -262, -251,
    -240, -230, -226, -229, -213, -207, -205, -200,
    -196, -191, -185, -180, -171, -163, -149, -145,
    -141, -138, -135, -132, -128, -126, -123, -120,
    -117, -115, -112, -110, -107, -105, -103, -101,
    -98, -96, -94, -92, -90, -88, -87, -85,
    -83, -81, -79, -78, -76, -74, -73, -71,
    -70, -68, -67, -65, -64, -62, -61, -60,
    -58, -57, -56, -54, -53, -52, -51, -49,
    -48, -47, -46, -45, -44, -43, -41, -40,
    -39, -38, -37, -36, -35, -34, -33, -32,
    -31, -30, -29, -28, -27, -26, -25, -24,
    -24, -23, -22, -21, -20, -19, -18, -18,
    -17, -16, -15, -14, -13, -13, -12, -11,
    -10, -10, -9, -8, -7, -7, -6, -5,
    -4, -4, -3, -2, -2, -1, 0, 0,
};

extern "C" {
u32 snd_level_to_db(s16 idx);
}

#pragma peephole off

/* One entry of the level table. */
extern "C" u32 snd_level_to_db(s16 idx)
{
    return snd_level_db_table[idx];
}
