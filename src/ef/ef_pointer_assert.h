/*
 * ef/ef_pointer_assert.h - nw4r's `NW4R_ASSERT_VALID_PTR` as its six-flag chain: each window test clears one
 *   flag, and the panic fires when the last is clear.  The flags are set before the pointer is read, so a pointer
 *   that is a field load (`pm->resource`, `self->data`) is loaded after the six `li`s, as retail's; `ef.h`'s
 *   `IsValidPointer` inline reads the pointer first.
 */
#ifndef MHTRI_EF_EF_POINTER_ASSERT_H
#define MHTRI_EF_EF_POINTER_ASSERT_H

#include "types.h"

#define EF_VALID_PTR_ASSERT(file, line, msg, ptr)                                                  \
    {                                                                                              \
        BOOL ok1_ = TRUE, ok2_ = TRUE, ok3_ = TRUE, ok4_ = TRUE, ok5_ = TRUE, ok6_ = TRUE;         \
        u32 top_ = (u32)(ptr) & 0xFF000000u;                                                       \
        if (!(top_ == 0x80000000u) && !(((u32)(ptr) & 0xFF800000u) == 0x81000000u))                \
            ok6_ = FALSE;                                                                          \
        if (!ok6_ && !(((u32)(ptr) & 0xF8000000u) == 0x90000000u))                                 \
            ok5_ = FALSE;                                                                          \
        if (!ok5_ && !(top_ == 0xC0000000u))                                                       \
            ok4_ = FALSE;                                                                          \
        if (!ok4_ && !(((u32)(ptr) & 0xFF800000u) == 0xC1000000u))                                 \
            ok3_ = FALSE;                                                                          \
        if (!ok3_ && !(((u32)(ptr) & 0xF8000000u) == 0xD0000000u))                                 \
            ok2_ = FALSE;                                                                          \
        if (!ok2_ && !(((u32)(ptr) & 0xFFFFC000u) == 0xE0000000u))                                 \
            ok1_ = FALSE;                                                                          \
        if (!ok1_)                                                                                 \
            nw4r::db::Panic(file, line, msg, (ptr));                                               \
    }

#endif
