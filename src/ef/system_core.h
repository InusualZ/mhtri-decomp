/* ef/system_core.h - the declarations of `ef/system_core.cpp`'s symbols other units call (docs/plan.md 6.5 rule 2). */
#ifndef MHTRI_EF_SYSTEM_CORE_H
#define MHTRI_EF_SYSTEM_CORE_H

#include "types.h"

/* 0x800CF7A8 - a block from the work heap (C++ scope: the map row is `work_mem_alloc__FUl`). */
/* untyped: byte range - the work heap hands out raw blocks the caller types */
void* work_mem_alloc(u32 size);

#ifdef __cplusplus
extern "C" {
#endif

/* 0x800D28FC - stores `mode + 1` as `system_w`'s display-state byte (+0xA58); the network transfer-mode switch
 * calls it with the transfer mode.  GUESS name. */
void setTransferDisplayState(u8 mode);
/* 0x800CEF18 - one step of the random ring's generator (0 is taken as 1; GUESS name). */
u16 rand_lcg_step(u16 value);

/* 0x800D2EA4 - the string table's course-name entry `id` (its +0xFC list; GUESS name). */
u32 str_tbl_course_get(u32 id);

/* 0x800D2F1C - steps the system's two stream counters (GUESS name). */
void system_stream_count_step(void);
/* 0x800CF1B4 / 0x800CF154 - stop the sound, release both stage slots and reset the scene; the second also clears the
 * system work first (GUESS names). */
void system_scene_reset(void);
void system_full_reset(void);

#ifdef __cplusplus
}
#endif

#ifdef __cplusplus
/* 0x800CEAB0 - requests a file load (C++ scope: `load_file_req__FPcUllUllPUl`): name, destination, size,
 * callback, mode and the callback's context. */
void load_file_req(char* name, u32 data, s32 size, u32 callback, s32 mode, u32* ctx);
#endif

#ifdef __cplusplus
/* 0x800CF7A8 - takes `size` bytes of work memory (C++ scope: `work_mem_alloc__FUl`). */
void* work_mem_alloc(u32 size); /* untyped: a byte range the caller types */
/* 0x800D2138 - turns the HOME button menu off (C++ scope: `hbm_disable__Fv`). */
void hbm_disable(void);
#endif

#endif /* MHTRI_EF_SYSTEM_CORE_H */
