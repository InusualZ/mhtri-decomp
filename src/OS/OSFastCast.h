/* OS/OSFastCast.h - the RVL SDK's paired-single fast casts: float <-> 8/16-bit integer through a quantised
 *   `psq_st`/`psq_l` with the GQR the OS sets up at boot.  Inline only; no translation unit owns them. */
#ifndef MHTRI_OS_OSFASTCAST_H
#define MHTRI_OS_OSFASTCAST_H

#include "types.h"

#define OS_FASTCAST_U8  2
#define OS_FASTCAST_U16 3
#define OS_FASTCAST_S8  4
#define OS_FASTCAST_S16 5

#ifdef __MWERKS__

static inline f32 __OSu16tof32(register const u16* arg) {
    register f32 ret;
    asm { psq_l ret, 0(arg), 1, OS_FASTCAST_U16 }
    return ret;
}

static inline void OSu16tof32(const u16* in, f32* out) {
    *out = __OSu16tof32(in);
}

static inline void OSf32tou16(const f32* in, u16* out) {
    register f32 arg = *in;
    f32 a;
    register f32* ptr = &a;
    register u16 r;
    asm {
        psq_st arg, 0(ptr), 1, OS_FASTCAST_U16
        lhz r, 0(ptr)
    }
    *out = r;
}

static inline f32 __OSs16tof32(register const s16* arg) {
    register f32 ret;
    asm { psq_l ret, 0(arg), 1, OS_FASTCAST_S16 }
    return ret;
}

static inline void OSs16tof32(const s16* in, f32* out) {
    *out = __OSs16tof32(in);
}

static inline void OSf32tos16(const f32* in, s16* out) {
    register f32 arg = *in;
    f32 a;
    register f32* ptr = &a;
    register s16 r;
    asm {
        psq_st arg, 0(ptr), 1, OS_FASTCAST_S16
        lha r, 0(ptr)
    }
    *out = r;
}

#endif /* __MWERKS__ */

#endif /* MHTRI_OS_OSFASTCAST_H */
