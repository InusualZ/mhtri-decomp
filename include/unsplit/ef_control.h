/* unsplit/ef_control.h - the effect manager's global control block, which no registered unit owns yet.
 *
 * `eft_control` (0x806A20F0, `.bss`, 0xC44 B) is the resource half of the `ef` module: `ef/eft_res.cpp` owns its
 * slot/model/heap tables and its load counters (20 sites), `ef/effect.cpp` reads the "initialised" byte and the active
 * effect handle, `ef/fn_800FD864_fx.cpp` retires through its effect system and `ef/eft019.cpp` hands its +0x04 word on.
 * The four lower-bound views those units had are one record here.  The size is the map's so MWCC emits the far
 * `lis`/`addi` address retail has instead of an `@sda21` load; the `+0x04` word is spelled four ways (a union).
 * size: 0xC44 */
#ifndef MHTRI_UNSPLIT_EF_CONTROL_H
#define MHTRI_UNSPLIT_EF_CONTROL_H

#include "types.h"

namespace nw4r {
namespace ef {
struct EffectSystem;
}
}

struct EftResSlot;

struct EftControl {
    union {
        struct {
            /* +0x000 */ u8 initialised_0x00;
            /* +0x001 */ u8 pad_0x01[0x3];
        };
        /* +0x000 */ u32 field_0x00;
    };
    union {
        /* +0x004 */ void* system_0x04;                              /* the nw4r::ef::EffectSystem fn_800D3C0C builds (eft_res.cpp's view) */
        /* +0x004 */ void* effect_0x04;                              /* the active effect handle (effect.cpp's view) */
        /* +0x004 */ nw4r::ef::EffectSystem* effect_system_0x04;     /* the effect system it retires through (fn_800FD864_fx.cpp's view) */
        /* +0x004 */ s32 field_0x04;                                 /* eft019.cpp's view */
    };
    /* +0x008 */ void* resource_0x08;      /* the resource walker fn_800B2878 builds */
    /* +0x00C */ u32 slot_count_0x0C;      /* get_move_work_max(4) */
    /* +0x010 */ EftResSlot* slots_0x10;   /* get_move_work_adrs(4) */
    /* +0x014 */ u32 slot_used_0x14;
    /* +0x018 */ u32 model_count_0x18;     /* get_move_work_max(5) */
    /* +0x01C */ u8* models_0x1C;          /* get_move_work_adrs(5), 0x168 B each */
    /* +0x020 */ u32 model_used_0x20;
    /* +0x024 */ u32 heap_count_0x24;      /* get_move_work_max(6) */
    /* +0x028 */ u8* heap_0x28;            /* get_move_work_adrs(6): count*0x40 B of data */
    /* +0x02C */ u8* heap_flags_0x2C;      /* heap_0x28 + heap_count*0x40: one byte per block */
    /* +0x030 */ u32 loaded_a_0x30;
    /* +0x034 */ u32 loaded_b_0x34;
    /* +0x038 */ u32 loaded_c_0x38;
    /* +0x03C */ s32 id_a_0x3C[0x100];
    /* +0x43C */ s32 id_b_0x43C[0x100];
    /* +0x83C */ s32 id_c_0x83C[0x100];
    /* +0xC3C */ u32 bytes_0xC3C;
    /* +0xC40 */ u8 frame_ready_0xC40;     /* fn_800F8DA4 clears it before it walks the slots */
    /* +0xC41 */ u8 pad_0xC41[0x3];
};

extern "C" EftControl eft_control;

#endif /* MHTRI_UNSPLIT_EF_CONTROL_H */
