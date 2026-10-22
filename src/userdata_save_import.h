/* userdata_save_import.h - leaf header (docs/plan.md 6.5 rule 2): `userdata_item.cpp`'s `userdata_save_import`. */
#ifndef MHTRI_USERDATA_SAVE_IMPORT_H
#define MHTRI_USERDATA_SAVE_IMPORT_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x80049F08 (0x74): copies a saved slot (0x6000 bytes) from `src` into the user data block and restores `system_w`'s
 * counter ring and +0x890 word from it; the callers pass the slot size in `size`. */
void userdata_save_import(const u8* src, u32 size);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_USERDATA_SAVE_IMPORT_H */
