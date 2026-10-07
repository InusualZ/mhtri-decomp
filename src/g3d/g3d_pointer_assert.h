/* g3d/g3d_pointer_assert.h - nw4r's resource pointer assert as the g3d units expand it (rule 1: one definition).
 *   `ptr` must fall in one of the seven mapped Wii memory ranges, else the assert panics through `nw4r::db::Panic`. */
#ifndef MHTRI_G3D_G3D_POINTER_ASSERT_H
#define MHTRI_G3D_G3D_POINTER_ASSERT_H

#define G3D_POINTER_ASSERT(file, ptr, line, msg)                                               \
    {                                                                                          \
        BOOL ok1_ = TRUE, ok2_ = TRUE, ok3_ = TRUE, ok4_ = TRUE, ok5_ = TRUE, ok6_ = TRUE;      \
        u32 top_ = (u32)(ptr) & 0xFF000000u;                                                    \
        if (!(top_ == 0x80000000u) && !(((u32)(ptr) & 0xFF800000u) == 0x81000000u))             \
            ok6_ = FALSE;                                                                        \
        if (!ok6_ && !(((u32)(ptr) & 0xF8000000u) == 0x90000000u))                              \
            ok5_ = FALSE;                                                                        \
        if (!ok5_ && !(top_ == 0xC0000000u))                                                    \
            ok4_ = FALSE;                                                                        \
        if (!ok4_ && !(((u32)(ptr) & 0xFF800000u) == 0xC1000000u))                              \
            ok3_ = FALSE;                                                                        \
        if (!ok3_ && !(((u32)(ptr) & 0xF8000000u) == 0xD0000000u))                              \
            ok2_ = FALSE;                                                                        \
        if (!ok2_ && !(((u32)(ptr) & 0xFFFFC000u) == 0xE0000000u))                              \
            ok1_ = FALSE;                                                                        \
        if (!ok1_)                                                                              \
            nw4r::db::Panic(file, line, msg, (ptr));                                             \
    }

#endif /* MHTRI_G3D_G3D_POINTER_ASSERT_H */
