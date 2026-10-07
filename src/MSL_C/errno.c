/*
 * MSL_C/errno.c - the C library's `errno` word (data only).
 *
 * RANGE. .sbss 0x80794E08..0x80794E10.
 * FLAGS. the `Runtime.PPCEABI.H` lib's `cflags_ppceabi`.
 * NAMES. file name GUESS (MSL `errno.c`); `errno` is the map's name (renamed from its generated label).
 * EVIDENCE. `.sbss` 0x80794E08 (8 B) is read or written by `_ftell`, `_fseek`, `strtol`, `atoi`, the fdlibm units
 *    and the sqrt wrapper: shared by many units, so it belongs to a data-only TU; its place in the `.sbss` order
 *    is between the allocator's word (0x80794E00) and the console unit's (0x80794E10).
 * RESIDUALS. the map row is 8 B, the source emits the 4 B word (the rest is alignment fill).
 * SHAPES. one zero-initialised word.
 */
#include "MSL_C/errno.h"

int errno;
