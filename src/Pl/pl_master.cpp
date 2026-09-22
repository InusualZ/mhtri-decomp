/*
 * Player master module (Pl_master): the actor-master cluster an earlier session carved out of the auto_*
 * scaffolding. .text 0x8026BA1C-0x8026FFBC (24 functions, 0x45A0 B) with its own exception tables - extab
 * 0x8001265C-0x800126CC, extabindex 0x8002F808-0x8002F8B0.
 *
 * Right edge pinned by the .sdata2 run jump `lbl_8079A02C -> lbl_8079A030`; left edge is the closure edge, and
 * `fn_8026FFBC` (0x8026FFBC-0x80270018) sits on the ambiguous side of that seam and is deliberately left
 * unclaimed rather than guessed in - the reasoning is in configure.py beside the Pl lib entry.
 *
 * It is C++ (the map holds the mangled `Pl_master_ck__FP4_PLW` / `Pl_act_ck__FP4_PLWUcUs`), so the actor type
 * is `_PLW` - that spelling is what the map's mangling encodes - and every unmangled `fn_*` callee is
 * `extern "C"`. The retail object also carries a 14-entry `extab`/`extabindex` pair - one entry per framed
 * function in the range - which `cflags_base`'s `-Cpp_exceptions off` does not emit at all; the probe that
 * reproduces it is in the Flags note below.
 *
 * Flags - measured against `build/RMHE08/obj/Pl/pl_master.o`, per-library for `Pl` (never `cflags_base`).
 * The unit's own `.text` is a one-number check on the flag set: 22 of the 24 functions are written, the two
 * unwritten ones are 0xC4C + 0x2B38, and `0x45A0 - 0xC4C - 0x2B38 = 0xE1C = 3612 B` - exactly what the
 * correct flags produce.
 *   * `-O4,p` -> **`-O3`** (playbook 27). Decisive: `.text` 4316 B under `-O4,p` (padding plus extra
 *     instructions) vs 3612 B under `-O3`; and 5 of the 22 functions reach 100 % under `-O4,p`
 *     (`fn_8026BA1C` 83.28, `fn_8026FA6C` 71.44, `fn_8026FB20` 0, `fn_8026FD0C` 75.62, `fn_8026FD94`
 *     86.75, ...) against 18 under `-O3`. `-O4,p` also implies `-func_align 16`, but the retail starts are
 *     only 4/8-byte aligned (.text+0x1254, +0x11c8, +0x3d98).
 *   * `-inline auto` -> **`-inline noauto`** (playbook 28). Decisive: with `-inline auto` the 180-byte
 *     `fn_8026FA6C` is inlined into `fn_8026FB20` (892 B vs the target's 288 B, 0 %); with `-inline noauto`
 *     the retail `bl fn_8026FA6C` is kept (288 B, 100 %). Nothing else in the unit moves.
 *   * `-Cpp_exceptions off` -> **`-Cpp_exceptions on`**. Evidence is the retail object's `extab`/`extabindex`
 *     pair, which `-Cpp_exceptions off` does not emit at all. Turned on, `.text` is unchanged and every
 *     match percent is identical, while the object gains `extab` 96 B / `extabindex` 144 B and each of the
 *     12 entries we can emit carries the same value as the target's entry for the same function
 *     (`fn_8026BA1C` 0x100A, `fn_8026BE94` 0x1008, `fn_8026F7B4` 0x1008, `fn_8026F828`/`fn_8026F888`
 *     0x0808, `fn_8026F9A4`/`fn_8026FA6C`/`fn_8026FC40` 0x2008, `fn_8026FD0C` 0x100A, `fn_8026FD94`/
 *     `Pl_master_ck`/`fn_8026FF20` 0x0808); the target's two extra entries (0x200A, 0x180A) belong to
 *     `fn_8026BF98` and `fn_8026CC7C`, the two functions still unwritten.
 *   * `#pragma peephole off` is load-bearing in six places - it is what turns MWCC's fused record form back
 *     into retail's `clrlwi`/`rlwinm`/`and` + `cmpwi` pair, and what keeps the byte/short parameter
 *     truncations its peephole deletes: `fn_8026CBE4`+`fn_8026CC70`, `fn_8026F888`, `fn_8026F9A4`,
 *     `Pl_act_ck`+`fn_8026FE98`-`fn_8026FEF0`, `fn_8026BE94`. Each is a scoped `off`/`reset` pair; no other
 *     function in the range needs it.
 *
 * Residual (22 of the 24 functions written; 20 are exactly 100 %):
 *   - `fn_8026BA1C` 98.11 %: same length and instruction sequence, only `self`'s callee-saved register
 *     differs (retail r30, ours r31 - the other one holds the `lis r31,1` constant base). Colouring only.
 *   - `fn_8026F908` 98.21 %: same length, whole diff is colouring (retail `level` r6 / `value` r4, ours
 *     r5/r6). Reordering the declarations moves the diff around but never closes it.
 *   - `fn_8026BF98` (3148 B) and `fn_8026CC7C` (11064 B) are not written - 14 KB of the unit's 17.8 KB, the
 *     two remaining functions.
 */

#include "types.h"

typedef struct Vec3 {
    f32 x;
    f32 y;
    f32 z;
} Vec3;

/* The actor the whole Pl_* family takes as its first argument. Only the offsets this unit touches are named;
 * everything in between is padding. The 132-byte block `fn_8026F7B4`/`fn_8026F828` memset (0xB8..0x13B) is
 * the action-state region, and the per-part tables at 0xD0/0xD4 are indexed by the callers' part number. */
struct _PLW {
    u8 pad00[2];          /* 0x00 */
    u8 unk02;             /* 0x02 - weapon/class id: 3 and 6 are switch cases, 4..6 an "is gun" range */
    u8 pad03[5];          /* 0x03 */
    u8 unk08;             /* 0x08 */
    u8 pad09;             /* 0x09 */
    u8 unk0A;             /* 0x0A - action id, compared by Pl_act_ck */
    u8 pad0B;             /* 0x0B */
    u16 unk0C;            /* 0x0C - action step, compared by Pl_act_ck */
    u8 pad0E[8];          /* 0x0E */
    u8 unk16;             /* 0x16 - area number, compared with get_now_areano() */
    u8 pad17;             /* 0x17 */
    u8 unk18;             /* 0x18 */
    u8 pad19[0x23];       /* 0x19 */
    f32 unk3C;            /* 0x3C - vector the sub-object adds into itself */
    f32 unk40;            /* 0x40 */
    f32 unk44;            /* 0x44 */
    u8 pad48[0xC];        /* 0x48 */
    u32 unk54;            /* 0x54 - copied word-for-word into the sub-object */
    u32 unk58;            /* 0x58 */
    u32 unk5C;            /* 0x5C */
    u8 pad60[0x8];        /* 0x60 */
    f32 unk68;            /* 0x68 - facing angle, tested against 0.0f by fn_8026BA1C */
    f32 unk6C;            /* 0x6C */
    f32 unk70;            /* 0x70 */
    u8 pad74[0x34];       /* 0x74 */
    u32 unkA8;            /* 0xA8 - frame counter, subtracted from the current frame */
    u8 padAC[0xC];        /* 0xAC */
    u16 unkB8;            /* 0xB8 } action-state flags, the region fn_8026F7B4 memsets */
    u16 unkBA;            /* 0xBA */
    u16 unkBC;            /* 0xBC */
    u16 unkBE;            /* 0xBE */
    u8 padC0[0xC];        /* 0xC0 */
    u16 unkCC;            /* 0xCC */
    u16 unkCE;            /* 0xCE */
    u16 unkD0[2];         /* 0xD0 - per-part table A (the `lhz 208(this + idx*2)` reads) */
    u16 unkD4[0x1E];      /* 0xD4 - per-part table B (the `lhz/lha 212(this + idx*2)` reads) */
    f32 unk110;           /* 0x110 */
    u8 pad114[0x14];      /* 0x114 */
    u8 unk128;            /* 0x128 */
    u8 unk129;            /* 0x129 - "initialised" latch */
    u8 pad12A[0xA];       /* 0x12A */
    u8 unk134;            /* 0x134 */
    u8 pad135[7];         /* 0x135 */
    void* unk13C;         /* 0x13C - the physics sub-object */
    u8 pad140[0x21C];     /* 0x140 */
    u32 unk35C;           /* 0x35C } two status words; the top bit of the mask picks which one */
    u32 unk360;           /* 0x360 */
    u8 pad364[0x282];     /* 0x364 */
    u8 unk5E6;            /* 0x5E6 */
};

/* What the action dispatcher hands `fn_8026BE94`: a kind mask, a state word and a per-part selector
 * byte table, all belonging to the requesting action rather than to the actor. */
typedef struct ActReq {
    u16 kind;            /* 0x00 - bitmask tested against the caller's mask */
    u8 pad02[0xE];       /* 0x02 */
    u16 state;           /* 0x10 */
    u8 pad12[0x56];      /* 0x12 */
    u8 part[4];          /* 0x68 - per-part selector, indexed by the caller */
} ActReq;

/* Unmangled map names: `extern "C"` so the compiler emits the map's spelling. */
extern "C" {
void fn_8026A618(_PLW* self, u32 id);
void fn_8026AF08(_PLW* self, u32 value);
void fn_80043EA8(void* vec);
void* fn_80041E40(void* dst, void* src);
void fn_800E09D0(void* dst, void* src);
u8 fn_800CF208(void);
s8 fn_800CF384(void);
u32 fn_80212060(void);
u16 fn_802BE038(void);
void* memset(void* dst, int value, u32 size);
}

/* A pool literal owned by a neighbouring Pl unit: 60.0f, the half-cone the "in front" test uses.
 * Declared, not defined - defining it would rebuild the section (docs/matching.md 29). */
extern f32 lbl_8079A020;

/* Mangled map names: written by their source name so the compiler emits the map's spelling. */
u8 PlayMode_ck(void);
u8 get_now_areano(void);
u32 Pl_Skill_ck(_PLW* self, u16 skill);

/* This unit's own order: the file has to emit the functions in the map's address order. */
extern "C" void fn_8026BE94(_PLW* self, ActReq* req, u32 mask, u32 idx);
extern "C" void fn_8026BA1C(_PLW* self);
u32 Pl_master_ck(_PLW* self);
extern "C" u8 fn_8026F908(_PLW* self, u32 idx);
extern "C" u32 fn_8026F9A4(_PLW* self, u32 idx, u16 low, u16 high);
extern "C" u32 fn_8026FA6C(_PLW* self, u32 idx, u16 low, u16 high);

/* 0x8026BA1C: the master action dispatcher - picks the actor's action id from the angle window the
 * target lies in, per weapon class. */
extern "C" void fn_8026BA1C(_PLW* self)
{
    fn_8026A618(self, 4);
    fn_8026A618(self, 25);
    if (self->unk129 == 0) {
        if (fn_8026FA6C(self, 0, -9102, 9102) == 1) {
            fn_8026A618(self, 62);
            fn_8026A618(self, 68);
        } else if (fn_8026FA6C(self, 0, 23666, -23666) == 1) {
            fn_8026A618(self, 63);
            fn_8026A618(self, 69);
        } else if (fn_8026FA6C(self, 0, -23666, -9102) == 1) {
            fn_8026A618(self, 61);
            fn_8026A618(self, 67);
            fn_8026A618(self, 65);
        } else if (fn_8026FA6C(self, 0, 9102, 23666) == 1) {
            fn_8026A618(self, 60);
            fn_8026A618(self, 66);
            fn_8026A618(self, 64);
        }
    } else {
        if (self->unkD4[0] < 50 && self->unk128 == 1) {
            if (self->unk110 <= lbl_8079A020) {
                fn_8026A618(self, 64);
            } else {
                fn_8026A618(self, 65);
            }
        }
        switch (self->unk02) {
        case 6:
            if (fn_8026FA6C(self, 0, -8192, 8192) == 1) {
                fn_8026A618(self, 62);
            } else if (fn_8026FA6C(self, 0, 24576, -24576) == 1) {
                fn_8026A618(self, 63);
            } else if (fn_8026FA6C(self, 0, -24576, -8192) == 1) {
                fn_8026A618(self, 61);
            } else if (fn_8026FA6C(self, 0, 8192, 24576) == 1) {
                fn_8026A618(self, 60);
            }
            if (fn_8026FA6C(self, 0, -16384, 5461) == 1) {
                fn_8026A618(self, 68);
            } else if (fn_8026FA6C(self, 0, 27307, -16384) == 1) {
                fn_8026A618(self, 69);
            } else if (fn_8026FA6C(self, 0, 5461, 27307) == 1) {
                fn_8026A618(self, 66);
            }
            break;
        case 3:
            if (fn_8026FA6C(self, 0, -5461, 16384) == 1) {
                fn_8026A618(self, 62);
                fn_8026A618(self, 68);
            } else if (fn_8026FA6C(self, 0, 16384, -27307) == 1) {
                fn_8026A618(self, 63);
                fn_8026A618(self, 69);
            } else if (fn_8026FA6C(self, 0, -27307, -5461) == 1) {
                fn_8026A618(self, 61);
                fn_8026A618(self, 67);
            }
            break;
        default:
            if (fn_8026FA6C(self, 0, -16384, 5461) == 1) {
                fn_8026A618(self, 62);
                fn_8026A618(self, 68);
            } else if (fn_8026FA6C(self, 0, 27307, -16384) == 1) {
                fn_8026A618(self, 63);
                fn_8026A618(self, 69);
            } else if (fn_8026FA6C(self, 0, 5461, 27307) == 1) {
                fn_8026A618(self, 60);
                fn_8026A618(self, 66);
            }
            break;
        }
    }
}

#pragma peephole off

/* 0x8026BE94: raises the requests an action's own kind mask and state word imply. */
extern "C" void fn_8026BE94(_PLW* self, ActReq* req, u32 mask, u32 idx)
{
    if ((req->kind & mask) == 0) {
        u8 part = req->part[idx];
        if ((u8)(part - 1) <= 7) {
            fn_8026A618(self, 54);
        }
    }
    if (req->state & 0x2000) {
        fn_8026A618(self, 55);
        fn_8026A618(self, 56);
    } else if (req->state & 0x1000) {
        fn_8026A618(self, 55);
        fn_8026A618(self, 57);
    }
    if (req->state & 0x800) {
        fn_8026A618(self, 55);
        fn_8026A618(self, 58);
    } else if (req->state & 0x400) {
        fn_8026A618(self, 55);
        fn_8026A618(self, 59);
    }
}

#pragma peephole off

/* 0x8026CBE4 */
extern "C" u32 fn_8026CBE4(_PLW* self, u8 which)
{
    if (self->unk18 == 1) {
        u16 flags = which == 0 ? self->unkB8 : self->unkBC;
        if ((flags & 4) != 0) {
            return 1;
        }
    } else if (which == 0) {
        if ((self->unkB8 & 0x44) == 0x44) {
            return 1;
        }
    } else if ((self->unkBC & 0x44) != 0 && (self->unkB8 & 0x44) == 0x44) {
        return 1;
    }
    return 0;
}

#pragma peephole reset

/* 0x8026CC70: the retail body compares a byte of its second argument's structure and returns the first. */
extern "C" u32 fn_8026CC70(u32 value, u8* data)
{
    switch ((u32)data[0x70]) {
    case 1:
        return value;
    }
    return value;
}

/* 0x8026F7B4: resets the action-state region, or starts a new action from the current area/state. */
extern "C" void fn_8026F7B4(_PLW* self)
{
    u32 value = 0;

    if (Pl_master_ck(self) == 0) {
        memset((u8*)self + 184, 0, 132);
    } else {
        if (PlayMode_ck() == 2) {
            value = self->unk08;
        }
        fn_8026AF08(self, value);
    }
}

/* 0x8026F828 */
extern "C" void fn_8026F828(_PLW* self)
{
    if (Pl_master_ck(self) == 0 || fn_80212060() == 1) {
        memset((u8*)self + 184, 0, 132);
    } else {
        fn_8026AF08(self, 0);
    }
}

#pragma peephole off

/* 0x8026F888: true while the actor has an action-state flag set. */
extern "C" u32 fn_8026F888(_PLW* self)
{
    if (Pl_master_ck(self) == 0) {
        return 0;
    }
    if ((self->unkBC & 0x3FF) != 0) {
        return 1;
    }
    if ((self->unkCC & 0x3C3C) != 0) {
        return 1;
    }
    return (self->unk134 & 3) != 0;
}

#pragma peephole reset

/* 0x8026F908: maps a part's motion value to an attack level (3/1/0), 0 for the "gun" weapon classes. */
extern "C" u8 fn_8026F908(_PLW* self, u32 idx)
{
    u32 level = 0;
    s16 value = *(s16*)((u8*)self + 0xD4 + idx * 2);
    s32 high;
    s32 mid;
    s32 low;

    if ((u32)(self->unk02 - 4) <= 2 && self->unk18 == 1 && self->unk5E6 == 1) {
        return 0;
    }
    if (self->unk128 == 2) {
        high = 110;
        mid = 90;
        low = 50;
    } else {
        high = 100;
        mid = 80;
        low = 40;
    }
    if (value >= high) {
        level = 3;
    } else if (value >= mid) {
        level = 1;
    } else if (value >= low) {
        level = 1;
    }
    return level;
}

#pragma peephole off

/* 0x8026F9A4: an attack-level gate plus the part's angle window. */
extern "C" u32 fn_8026F9A4(_PLW* self, u32 idx, u16 low, u16 high)
{
    u16 flags = self->unkCC;

    if (idx == 0) {
        if ((flags & 0x3C00) == 0) {
            return 0;
        }
    } else {
        if ((flags & 0x3C) == 0) {
            return 0;
        }
    }
    if (fn_8026F908(self, idx) >= 1 && (u16)(self->unkD0[idx] - low) <= (u16)(high - low)) {
        return 1;
    }
    return 0;
}

#pragma peephole reset

/* 0x8026FA6C: the same window test, relative to the action's start frame. */
extern "C" u32 fn_8026FA6C(_PLW* self, u32 idx, u16 low, u16 high)
{
    u16 offset = 0;

    if (self->unk129 != 0) {
        offset = (u16)(self->unkA8 - fn_802BE038());
    }
    if (self->unkD4[idx] >= 50) {
        u16 value = (u16)(self->unkD0[idx] - offset);
        if ((u16)(value - low) <= (u16)(high - low)) {
            return 1;
        }
    }
    return 0;
}

/* 0x8026FB20: picks the angle window for a direction code. */
extern "C" u32 fn_8026FB20(_PLW* self, s32 kind)
{
    switch (kind) {
    case 8192:
        return fn_8026FA6C(self, 0, 8192, 24576);
    case 1024:
        return fn_8026FA6C(self, 0, -8192, 8192);
    case 2048:
        return fn_8026FA6C(self, 0, 24576, -24576);
    case 4096:
        return fn_8026FA6C(self, 0, -24576, -8192);
    case 32:
        return fn_8026FA6C(self, 1, 8192, 24576);
    case 4:
        return fn_8026FA6C(self, 1, -8192, 8192);
    case 8:
        return fn_8026FA6C(self, 1, 24576, -24576);
    case 16:
        return fn_8026FA6C(self, 1, -24576, -8192);
    }
    return 0;
}

/* 0x8026FC40 */
extern "C" u32 fn_8026FC40(_PLW* self, u32 idx, u16 low, u16 high)
{
    if (fn_8026F908(self, idx) >= 1 && (u16)(self->unkD0[idx] - low) <= (u16)(high - low)) {
        return 1;
    }
    return 0;
}

/* 0x8026FCCC: the angle window test without the attack-level gate. */
extern "C" u32 fn_8026FCCC(_PLW* self, u32 idx, u16 low, u16 high)
{
    if (self->unkD4[idx] >= 50) {
        if ((u16)(self->unkD0[idx] - low) <= (u16)(high - low)) {
            return 1;
        }
    }
    return 0;
}

/* 0x8026FD0C: hands the actor's transform to its physics sub-object. */
extern "C" void fn_8026FD0C(_PLW* self)
{
    Vec3 vec;
    u32* p;

    fn_80043EA8(&vec);
    p = (u32*)((u8*)self->unk13C + 4);
    p[10] = self->unk54;
    p[11] = self->unk58;
    p[12] = self->unk5C;
    fn_80041E40(&p[1], (u8*)self + 60);
    vec.x = self->unk68;
    vec.y = self->unk6C;
    vec.z = self->unk70;
    fn_800E09D0(p, &vec);
}

/* 0x8026FD94: true when the actor is in the area the player is currently in. */
extern "C" u32 fn_8026FD94(_PLW* self)
{
    return get_now_areano() == self->unk16;
}

/* 0x8026FDD4: master gate - true while the actor's Pl_master is the one in charge. */
u32 Pl_master_ck(_PLW* self)
{
    if (PlayMode_ck() == 2) {
        return 1;
    }
    if (fn_800CF208() == 3) {
        return 1;
    }
    return fn_800CF384() == self->unk08;
}

/* 0x8026FE44: true for the "gun" weapon classes. */
extern "C" u32 fn_8026FE44(_PLW* self)
{
    return (u32)(self->unk02 - 4) <= 2;
}

#pragma peephole off

/* 0x8026FE68 */
u32 Pl_act_ck(_PLW* self, u8 action, u16 step)
{
    if (self->unk0A == action && self->unk0C == step) {
        return 1;
    }
    return 0;
}

/* 0x8026FE98: tests one bit of the two status words; the mask's top bit picks the word. */
extern "C" u32 fn_8026FE98(_PLW* self, u32 mask)
{
    if ((mask & 0x80000000) == 0) {
        return self->unk35C & mask;
    }
    return self->unk360 & (mask & 0x7FFFFFFF);
}

/* 0x8026FEC0 */
extern "C" void fn_8026FEC0(_PLW* self, u32 mask)
{
    if ((mask & 0x80000000) == 0) {
        self->unk35C |= mask;
    } else {
        self->unk360 |= mask & 0x7FFFFFFF;
    }
}

/* 0x8026FEF0 */
extern "C" void fn_8026FEF0(_PLW* self, u32 mask)
{
    if ((mask & 0x80000000) == 0) {
        self->unk35C &= ~mask;
    } else {
        self->unk360 &= ~(mask & 0x7FFFFFFF);
    }
}

#pragma peephole reset

/* 0x8026FF20: the attack bonus the actor's active skills grant. */
extern "C" s32 fn_8026FF20(_PLW* self)
{
    if (Pl_Skill_ck(self, 13) == 1) {
        return 20;
    }
    if (Pl_Skill_ck(self, 14) == 1) {
        return 50;
    }
    if (Pl_Skill_ck(self, 15) == 1) {
        return -10;
    }
    return Pl_Skill_ck(self, 16) == 1 ? -30 : 0;
}
