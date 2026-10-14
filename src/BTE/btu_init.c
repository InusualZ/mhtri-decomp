/*
 * BTE/btu_init.c - the BTU stack initialisation: the core init that starts btm, l2c, sdp and the discovery database,
 *   and BTE_Init, which clears the BTU control block.
 *
 * RANGE. .text 0x8048C13C-0x8048C1B4 (2 functions, 0x78 B); no data. 0x8048C13C calls btm_init, l2c_init, sdp_init and
 *   btm_discovery_db_init and is called from btu_task_init: the BTE btu_init.c btu_init_core, which precedes BTE_Init
 *   in that file. Medium confidence: no data on either side of 0x8048C13C, so the start rests on the two bodies;
 *   wbt_ext.c's WBT_ExtCreateRecord follows. The BTU control block BTE_Init clears (.bss 0x80739170) sits in the .bss
 *   order of BTE/gki_time.c's head files and stays there.
 * FLAGS. GC/3.0a5.2 with `cflags_nw4r`, the compiler of every measured BTE unit (BTE/l2c_utils.c FLAGS line).
 * NAMES. the map's and the BTE source's: btu_init_core (0x8048C13C; the runtime dump has the same name).
 * RESIDUALS. none known.
 * SHAPES. plain C.
 */

#include "BTE/btu_init.h"
#include "BTE/gki_time.h"
#include "BTE/l2c_main.h"
#include "BTE/sdp_main.h"
#include "Runtime.PPCEABI.H/memset.h"
#include "BTE/btm_discovery.h"   /* btm_discovery_db_init */

/* The default ACL data size and the HCI ACL header in front of it. */
#define BTU_DEFAULT_DATA_SIZE 672
#define HCI_DATA_PREAMBLE_SIZE 4

/* 0x8048C13C (0x2C): Initialises the device manager, L2CAP, SDP and the discovery database. */
void btu_init_core(void) {
    btm_init();
    l2c_init();
    sdp_init();
    btm_discovery_db_init();
}

/* 0x8048C168 (0x4C): Clears the BTU control block and sets the ACL packet size and the command window. */
void BTE_Init(void) {
    memset(&btu_cb, 0, sizeof(tBTU_CB));
    btu_cb.hcit_acl_pkt_size = BTU_DEFAULT_DATA_SIZE + HCI_DATA_PREAMBLE_SIZE;
    btu_cb.hci_cmd_window = 1;
}
