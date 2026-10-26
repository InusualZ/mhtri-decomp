/* Leaf header (docs/plan.md 6.5 rule 2): `em_res_file_row_copy`, which `enemy/em_kind` owns; declared for `stage/stg_w.cpp`.  C linkage. */
#ifndef MHTRI_ENEMY_EM_RES_FILE_ROW_COPY_H
#define MHTRI_ENEMY_EM_RES_FILE_ROW_COPY_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x80140DAC - copies one 0x4C-byte resource file row from `src` into `dst`. */
void em_res_file_row_copy(u8* dst, u8* src);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_ENEMY_EM_RES_FILE_ROW_COPY_H */
