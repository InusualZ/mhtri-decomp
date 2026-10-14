/*
 * BTE/utl.c - the BTA utility function utl_freebuf.
 *
 * RANGE. .text 0x8047B310-0x8047B354 (1 function, 0x44 B); no data. Medium: utl_freebuf is the one utl.c function this
 *   link keeps; it sits between ptim_stop_timer and bta_dm_enable, the first function that reads the bta_dm data.
 * FLAGS. GC/3.0a5.2 with `cflags_nw4r`, the compiler of every measured BTE unit (BTE/l2c_utils.c FLAGS line).
 * NAMES. the map's.
 * RESIDUALS. none known.
 * SHAPES. plain C.
 */

#include "BTE/utl.h"
#include "BTE/gki_buffer.h"

/* 0x8047B310 (0x44): Frees the GKI buffer `*p` holds, if any, and clears the pointer. */
/* untyped: a caller-owned payload - the slot holds any GKI buffer */
void utl_freebuf(void** p) {
    if (*p != NULL) {
        GKI_freebuf(*p);
        *p = NULL;
    }
}
