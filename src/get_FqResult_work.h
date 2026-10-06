/* Leaf header (docs/plan.md 6.5 rule 2): the save-band unit's (0x8004CAD8..) symbols `enemy/em_pop.cpp` calls, for a consumer that cannot
 * include the owner's full header.  C linkage (the map rows are plain names). */
#ifndef MHTRI_GET_FQRESULT_WORK_H
#define MHTRI_GET_FQRESULT_WORK_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x8004D134 - the 0x27C-byte free-hunt result buffer (`clear_FqResult_work` clears it; the record is
 * `enemy/em_set_work.h`'s `FqResultWork`). */
struct FqResultWork;
struct FqResultWork* get_FqResult_work(void);
/* 0x8004D334 / 0x8004E568 / 0x8004E5A4 / 0x8004E5E0 - the save block's money (clamped to 9999999), a monster's
 * hunt and capture counts (capped at 9999), and its smallest/largest size record.  GUESS names. */
void userdata_zenny_add(s32 amount);
void userdata_hunt_count_add(u8 monster, s32 count);
void userdata_capture_count_add(u8 monster, s32 count);
void userdata_size_record_set(u8 monster, u16 size);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_GET_FQRESULT_WORK_H */
