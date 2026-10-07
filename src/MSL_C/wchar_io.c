/*
 * MSL_C/wchar_io.c - wide-character stream orientation (`fwide`).
 *
 * RANGE. .text 0x80463C48..0x80463CC0 (1 functions in the map, 0x78 B).
 * FLAGS. the `Runtime.PPCEABI.H` lib's `cflags_ppceabi`.
 * NAMES. file name GUESS (MSL `wchar_io`).
 * EVIDENCE. the dump names the single function `fwide`; it is called by `printf`, `vprintf` and `__fwrite`.
 * RESIDUALS. none known.
 * SHAPES. the orientation is a two-bit field of the stream mode word: 1 byte, 2 wide.
 */
#include "MSL_C/wchar_io.h"

s32 fwide(FILE* file, s32 mode)
{
    s32 orientation;

    if (file == NULL || file->mode.file_kind == 0) {
        return 0;
    }
    orientation = file->mode.file_orientation;
    switch (orientation) {
    case 0:
        if (mode > 0) {
            file->mode.file_orientation = 2;
        } else if (mode < 0) {
            file->mode.file_orientation = 1;
        }
        break;
    case 2:
        mode = 1;
        break;
    case 1:
        mode = -1;
        break;
    }
    return mode;
}
