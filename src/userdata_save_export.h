/* userdata_save_export.h - leaf header (docs/plan.md 6.5 rule 2): `userdata_item.cpp`'s `userdata_save_export`. */
#ifndef MHTRI_USERDATA_SAVE_EXPORT_H
#define MHTRI_USERDATA_SAVE_EXPORT_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x80049E9C (0x6C): stores `system_w`'s counter ring and +0x890 word into the user data block and copies the block
 * (0x6000 bytes) to `dst`; the callers pass the slot size in `size`. */
void userdata_save_export(u8* dst, u32 size);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_USERDATA_SAVE_EXPORT_H */
