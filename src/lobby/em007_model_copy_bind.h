/* lobby/em007_model_copy_bind.h - leaf header: `em007_model_copy_bind` (0x801E2864) and the two data rows
 * `em007_model_eft_create` reads, owned by `lobby/fn_801E0ADC.cpp` (its `.text`, `.data` and `.sdata2` ranges).
 * C linkage: the map rows are plain names. */
#ifndef MHTRI_LOBBY_EM007_MODEL_COPY_BIND_H
#define MHTRI_LOBBY_EM007_MODEL_COPY_BIND_H

#include "types.h"

#ifdef __cplusplus
class Em007ModelCopy;

extern "C" {
/* Points the model copy's world callback at its source model. */
void em007_model_copy_bind(Em007ModelCopy* copy);
#endif

/* 0x805B7608 (0x10): the model resource id of each Cha-Cha mask kind (seven ids and a zero pad). */
extern const u16 em007_model_res_ids[8];
/* 0x80799788 (.sdata2): the uniform scale each pooled mask model starts at (0.75f). */
extern const f32 em007_model_eft_scale;

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_LOBBY_EM007_MODEL_COPY_BIND_H */
