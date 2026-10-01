/* The player act-step data unit `Pl/pl_act_step_data.cpp`: the `.data` tables the act state machine
 * (`Pl/pl_act.cpp`) indexes by act number - skill-id and step tables, per-act timing words and effect-pair tables.
 * Declared, never defined: the bytes are the data unit's own.
 */
#ifndef MHTRI_PL_PL_ACT_STEP_DATA_H
#define MHTRI_PL_PL_ACT_STEP_DATA_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

extern u16 lbl_805C0198[];
extern u16 lbl_805C01B8[];
extern u8 lbl_805C01C8[];
extern u8* lbl_805BFFA8[];
extern u32 lbl_805BF448[];
extern u32 lbl_805BF46C[];
extern u8 lbl_805BF5E0[];
extern u8 lbl_805BF538[];
extern u8 lbl_805BFCD8[];
extern u8 lbl_805BFFCC[];

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_PL_PL_ACT_STEP_DATA_H */
