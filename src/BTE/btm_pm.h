/* BTE/btm_pm.h - the power mode entry points `BTE/btm_pm.c` owns that other units call, and the power mode records of
 *   the control block (docs/plan.md 6.5 rule 2). */
#ifndef MHTRI_BTE_BTM_PM_H
#define MHTRI_BTE_BTM_PM_H

#include "types.h"
#include "BTE/gki_time.h"

/* The power mode clients the registration table holds. */
#define BTM_MAX_PM_RECORDS 2

/* size: 0x22 - the power mode state of one ACL link: the mode each client asked for, the interval of the last mode
 * change, the current state and whether a change is pending. */
typedef struct tBTM_PM_MCB {
    /* +0x00 */ BtmPmPwrMode req_mode[BTM_MAX_PM_RECORDS + 1];
    /* +0x1E */ u16 interval;
    /* +0x20 */ u8 state;
    /* +0x21 */ u8 chg_ind;
} tBTM_PM_MCB; /* size: 0x22 */

/* size: 0x08 - a power mode client registration. */
typedef struct tBTM_PM_RCB {
    /* +0x00 */ BtmPmCallback cback;
    /* +0x04 */ u8 mask;
    /* +0x05 */ u8 pad_0x05[3];
} tBTM_PM_RCB; /* size: 0x08 */


#ifdef __cplusplus
extern "C" {
#endif

/* 0x804866DC - writes the power mode of the link to `remote_bda` to `p_mode`; returns the BTM status. */
u8 BTM_ReadPowerMode(u8* remote_bda, u8* p_mode);

/* 0x80486794 / 0x8048680C - clear the power mode state after a controller reset, or for a new link at `ind`. */
void btm_pm_reset(void);
void btm_pm_sm_alloc(u8 ind);

/* 0x8048685C / 0x80486A78 - merge two requested modes, or every client's request for one link, into `p_res`. */
BtmPmPwrMode* btm_pm_compare_modes(BtmPmPwrMode* p_md1, BtmPmPwrMode* p_md2, BtmPmPwrMode* p_res);
u8 btm_pm_get_set_mode(u8 pm_id, tBTM_PM_MCB* p_cb, BtmPmPwrMode* p_mode, BtmPmPwrMode* p_res);

/* 0x80486BD8 - sends the HCI command that moves the link at `link_ind` to the merged mode. */
u8 btm_pm_snd_md_req(u8 pm_id, int link_ind, BtmPmPwrMode* p_mode);

/* 0x80486E0C / 0x80486EBC - handle the command status of a mode request and the mode change event. */
void btm_pm_proc_cmd_status(u8 status);
void btm_pm_proc_mode_change(u8 hci_status, u16 hci_handle, u8 mode, u16 interval);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_BTE_BTM_PM_H */
