/*
 * BTE/bd.c - the Bluetooth device address helpers: a 6-byte address copy and a 6-byte address compare.
 *
 * RANGE. .text 0x8047ABC4-0x8047AC98 (2 functions, 0xD4 B); no data. Strong start: 0x8047ABC4 is the first function
 *   start of the BTE band that is not 16-aligned (all 61 starts from 0x80478790 are), the alignment seam that ends the
 *   BTE/gki_time.c head. Medium end: BTE/bta_sys_conn.c's callback registrations follow, the first functions that write
 *   the bta_sys control block. The all-zero address 0x8079D7E8 (.sbss2), passed by address to 0x8047ABF8 by
 *   bta_hh_find_cb, sits with its only reader in BTE/bta_hh_utils.c; Broadcom's bd.c defines a bd_addr_null of that
 *   shape, and placing it here instead gives the same layout.
 * FLAGS. GC/3.0a5.2 with `cflags_nw4r`, the compiler of every measured BTE unit (BTE/l2c_utils.c FLAGS line).
 * NAMES. bdcpy and bdcmp, the Broadcom bd.c names (the runtime dump has both at these addresses; bdcpy copies 6 bytes
 *   from r4 to r3, bdcmp compares 6 bytes and returns 0 or -1).
 * RESIDUALS. none known.
 * SHAPES. plain C.
 */

#include "BTE/bd.h"

/* 0x8047ABC4 (0x34): Copies the device address `b` to `a`. */
void bdcpy(BD_ADDR a, const BD_ADDR b) {
    int i;

    for (i = BD_ADDR_LEN; i != 0; i--) {
        *a++ = *b++;
    }
}

/* 0x8047ABF8 (0xA0): Compares the device addresses `a` and `b`; returns 0 when they are equal, else -1. */
int bdcmp(const BD_ADDR a, const BD_ADDR b) {
    int i;

    for (i = BD_ADDR_LEN; i != 0; i--) {
        if (*a++ != *b++) {
            return -1;
        }
    }

    return 0;
}
