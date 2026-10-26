/* `face_buf_tbl` (.sbss 0x80794870), owned by `fn_80047398.cpp` (leaf header, rule 2): the two face-sprite buffers
 * `userdata_item.cpp`'s sprite queue fills and draws (the record type is that unit's `FaceSprBuf`). */
#ifndef MHTRI_FACE_BUF_TBL_H
#define MHTRI_FACE_BUF_TBL_H

#include "types.h"

struct FaceSprBuf;

#ifdef __cplusplus
extern "C" {
#endif

extern struct FaceSprBuf* face_buf_tbl[2];

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_FACE_BUF_TBL_H */
