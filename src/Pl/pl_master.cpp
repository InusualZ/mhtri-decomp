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
 * The unit's own `.text` is a one-number check on the flag set: all 24 functions are written and the object's
 * `.text` is 0x45A0 B, which is what the correct flags produce.
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
 *     `Pl_master_ck`/`fn_8026FF20` 0x0808, `fn_8026BF98` 0x200A, `fn_8026CC7C` 0x180A); with all 24 functions
 *     written our `extab` (0x70) / `extabindex` (0xA8) are byte-identical to the target's.
 *   * `#pragma peephole off` is load-bearing in six places - it is what turns MWCC's fused record form back
 *     into retail's `clrlwi`/`rlwinm`/`and` + `cmpwi` pair, and what keeps the byte/short parameter
 *     truncations its peephole deletes: `fn_8026CBE4`+`fn_8026CC70`, `fn_8026F888`, `fn_8026F9A4`,
 *     `Pl_act_ck`+`fn_8026FE98`-`fn_8026FEF0`, `fn_8026BE94`. Each is a scoped `off`/`reset` pair; no other
 *     function in the range needs it.
 *
 * Residual: none. All 24 functions are 100 % and `.text` is byte-identical to the target (0x45A0 B, 0
 * differing bytes).
 *   - `fn_8026F908` was the last 9 bytes, and they were a colouring tie-break rather than a code shape.
 *     Retail coalesces the load result with the index-address temp (`add r4,r3,r0; lha r4,212(r4)`, so
 *     `value` r4 and the class temp r5); the natural source - `s16 value = (s16)self->unkD4[idx];` before
 *     the early-out, which is the only spelling that puts `level` in r6 - keeps them apart (`lha r5,212(r4)`,
 *     `value` r5 / class temp r4), the exact mirror. ~200 shapes reproduce that mirror: declaration order,
 *     pointer/cast/temp/array spellings, `switch`/`do`/`for`/nested-if early-out forms, bounds and chain
 *     order, named class temporaries, dead statements, all 30 toolchain compilers, and every `-opt` keyword
 *     and `#pragma` (`peephole`, `scheduling`, `optimization_level 0..4`, `opt_lifetimes`, ...) combination.
 *     The lever is the *web list order*: MWCC colours the two webs from the order the IR's copy webs were
 *     born in, so the function carries a three-deep chain of dead copies of `self->weaponClass` (`classCopy0..2`,
 *     all optimised away) and tests a separate `weaponClass` load. That is the only shape tried that lands
 *     `value` in r4 with the same 39 instructions; without the chain the function is retail's mirror.
 *   - The load itself still has to be spelled `(s16)self->unkD4[idx]` (the array+cast form is what puts
 *     `level` in r6; the pointer-arithmetic spelling is 15 bytes off), and moving it below the early-out
 *     return puts the 3-instruction load block after the branch - MWCC does not hoist a load across it.
 *   - `fn_8026CC7C` 100 % since the `.data` claim (`.data start:0x805C5FA0 end:0x805C5FC4`, splits.txt) paired
 *     the switch jump table: the section is byte-identical (0x24 B, nine words). The only trace of the old
 *     mismatch is that our reloc reaches the table through the compiler's local symbol (`.data+0`) where the
 *     target names `jumptable_805C5FA0` - same section and offset, so it resolves to the same address.
 *
 * Load-bearing source shapes in the two big dispatchers (everything else there is plain member access):
 *   - both take the action state as a `st = &self->unkB8` local, which is the register retail keeps the whole
 *     block in;
 *   - `fn_8026BF98` opens with a 16-element `Vec3` loop written `do { ... } while (p < &vec[16])`: a
 *     `for`/`while` in the same place makes MWCC emit an extra loop guard, and the array is only ever
 *     initialised (`VEC3_ctor`), never read;
 *   - `fn_8026BF98` reads `st->unk7D` through `switch (st->unk7D) { default: <hold-start>; case 1: <hold-tick>; }`
 *     - MWCC lowers a switch test to `cmpwi` where an `if` on the same `u8` emits `cmplwi`, and the default
 *     arm has to be written first for its body to be laid out as retail's fall-through;
 *   - `fn_8026BF98` reads the two inner request bits as `self->unkBC` (r30-based) while the enclosing test is
 *     `st->flagsBC` (r31-based) - the base register is observable;
 *   - `fn_8026CC7C` is the m2c reconstruction of the target disassembly; its `1U` / `s32` spellings and the
 *     `st->flagsB8` vs `self->unkCC` base choices are load-bearing, so it is deliberately not re-styled;
 *   - `fn_8026CBE4` takes two unused trailing parameters because its retail callers pass four arguments.
 */

#include "types.h"
#include "mh3_pad.h" /* VEC3_ctor / copyVec3 / setVec3 (rule 2) */

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
    u8 weaponClass;       /* 0x02 - weapon/class id: 3 and 6 are switch cases, 4..6 an "is gun" range */
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
    u8 pad140[0x1C8];     /* 0x140 */
    u8 unk308;            /* 0x308 - the "the action owns the body" flag the dispatchers gate on */
    u8 pad309[0x53];      /* 0x309 */
    u32 unk35C;           /* 0x35C } two status words; the top bit of the mask picks which one */
    u32 unk360;           /* 0x360 */
    u8 pad364[0x261];     /* 0x364 */
    u8 unk5C5;            /* 0x5C5 - the dispatchers' held-attack flag */
    u8 pad5C6[0x20];      /* 0x5C6 */
    u8 unk5E6;            /* 0x5E6 */
};

/* The actor's 132-byte action-state block, `self + 0xB8`: `fn_8026F7B4`/`fn_8026F828` clear it, the two
 * big dispatchers (`fn_8026BF98`, `fn_8026CC7C`) drive it, and `fn_8026BE94` reads the request word and the
 * per-part selector out of it, so one type has to carry both views. Retail materialises the base pointer
 * once and keeps it in a callee-saved register - hence the local `st` in the dispatchers below. */
typedef struct ActState {
    u16 flagsB8;         /* 0x00 - self->unkB8: request bit word A (the "kind" mask fn_8026BE94 tests) */
    u16 unk02;           /* 0x02 */
    u16 flagsBC;         /* 0x04 - self->unkBC: request bit word B */
    u8 pad06[0x0A];      /* 0x06 */
    u16 unk10;           /* 0x10 - the word fn_8026BE94 tests 0x2000/0x1000/0x800/0x400 against */
    u16 unk12;           /* 0x12 */
    u16 flagsCC;         /* 0x14 - self->unkCC: per-part behaviour bits */
    u8 pad16[0x3E];      /* 0x16 */
    f32 unk54;           /* 0x54 - self->unk10C: the charge/axis value fn_8026CC7C bands */
    f32 unk58;           /* 0x58 - self->unk110: the weapon/model axis length */
    u8 pad5C[0xC];       /* 0x5C */
    u8 part[8];          /* 0x68 - per-part selector table (fn_8026BE94 indexes it, the dispatchers read part[1]) */
    u8 mode;             /* 0x70 - self->unk128: the dispatchers' switch value */
    u8 unk71;            /* 0x71 - self->unk129: the "an action is running" latch */
    u8 pad72[6];         /* 0x72 */
    s32 unk78;           /* 0x78 - self->unk130 */
    u8 unk7C;            /* 0x7C - self->unk134: low two bits are a per-part flag */
    u8 unk7D;            /* 0x7D - self->unk135 */
    u8 unk7E;            /* 0x7E - self->unk136: the deferred-command bits */
    u8 unk7F;            /* 0x7F - self->unk137: the hold delay */
    u16 unk80;           /* 0x80 - self->unk138: flags latched for the hold */
    u16 unk82;           /* 0x82 - self->unk13A: flags latched for the hold */
} ActState;

/* Unmangled map names: `extern "C"` so the compiler emits the map's spelling. */
extern "C" {
void fn_8026A618(_PLW* self, u32 id);
u32 fn_8026A644(_PLW* self, u32 id);
void fn_8026A678(_PLW* self, u32 id);
u32 fn_8026A6F4(_PLW* self, u32 id);
u32 fn_8026B99C(_PLW* self);
u32 fn_8026B934(_PLW* self);
u32 fn_8026BA04(_PLW* self);
u32 fn_803BECC8(u8 value, u32 low, u32 high);
void fn_8026AF08(_PLW* self, u32 value);
void fn_800E09D0(void* dst, void* src);
u8 fn_800CF208(void);
s8 my_player_no(void);
u32 fn_80212060(void);
u16 fn_802BE038(void);
void* memset(void* dst, int value, u32 size);
}

/* Pool literals owned by a neighbouring Pl unit: 60.0f and the two charge bands `fn_8026CC7C` compares
 * `st->unk54` against. Declared, not defined - defining them would rebuild the section (docs/matching.md 29). */
extern f32 lbl_8079A020;
extern f32 lbl_8079A024;
extern f32 lbl_8079A028;

/* Mangled map names: written by their source name so the compiler emits the map's spelling. */
u8 PlayMode_ck(void);
u8 get_now_areano(void);
u32 Pl_Skill_ck(_PLW* self, u16 skill);

/* This unit's own order: the file has to emit the functions in the map's address order. */
extern "C" void fn_8026BE94(_PLW* self, ActState* req, u32 mask, u32 idx);
extern "C" void fn_8026CC7C(_PLW* self);
extern "C" void fn_8026BA1C(_PLW* self);
u32 Pl_master_ck(_PLW* self);
extern "C" u8 fn_8026F908(_PLW* self, u32 idx);
extern "C" u32 fn_8026F9A4(_PLW* self, u32 idx, u16 low, u16 high);
extern "C" u32 fn_8026FA6C(_PLW* self, u32 idx, u16 low, u16 high);
extern "C" u32 fn_8026FB20(_PLW* self, s32 kind);
extern "C" u32 fn_8026FC40(_PLW* self, u32 idx, u16 low, u16 high);
extern "C" u32 fn_8026CBE4(_PLW* self, u8 which, u32 arg2, u32 arg3);

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
        switch (self->weaponClass) {
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
extern "C" void fn_8026BE94(_PLW* self, ActState* req, u32 mask, u32 idx)
{
    if ((req->flagsB8 & mask) == 0) {
        u8 part = req->part[idx];
        if ((u8)(part - 1) <= 7) {
            fn_8026A618(self, 54);
        }
    }
    if (req->unk10 & 0x2000) {
        fn_8026A618(self, 55);
        fn_8026A618(self, 56);
    } else if (req->unk10 & 0x1000) {
        fn_8026A618(self, 55);
        fn_8026A618(self, 57);
    }
    if (req->unk10 & 0x800) {
        fn_8026A618(self, 55);
        fn_8026A618(self, 58);
    } else if (req->unk10 & 0x400) {
        fn_8026A618(self, 55);
        fn_8026A618(self, 59);
    }
}

/* 0x8026BF98: the action dispatcher. Switches on the action state's mode byte and raises the command set
 * the actor's request words and hold state imply. */
extern "C" void fn_8026BF98(_PLW* self)
{
    ActState* st = (ActState*)&self->unkB8;
    Vec3 vec[16];
    Vec3* p = vec;

    do {
        VEC3_ctor(p);
        p++;
    } while (p < &vec[16]);

    switch (st->mode) {
    case 2:
        switch (st->unk7D) {
        default:
                if ((st->flagsBC & 0x120) == 0x120) {
                    st->unk7E |= 8;
                } else {
                    st->unk7E = 0;
                    if (st->flagsBC & 0x320) {
                        st->unk7D++;
                        st->unk7F = 2;
                        st->unk80 = st->flagsBC & 0x320;
                        st->unk82 = st->flagsBC & 0x100;
                    } else {
                        st->unk80 = 0;
                        st->unk82 = 0;
                    }
                }
            break;
        case 1:
                if (--st->unk7F == 0 || (st->flagsBC & 0x320) != 0) {
                    if (st->flagsBC & 0x320) {
                        if (st->flagsBC & 0x200) {
                            st->unk7E |= 4;
                        }
                        if (st->flagsBC & 0x100) {
                            st->unk7E |= 1;
                        }
                        if (st->flagsBC & 0x20) {
                            st->unk7E |= 2;
                        }
                        if (((u16)(st->flagsBC | st->unk80) & 0x120) == 0x120) {
                            st->unk7E |= 8;
                        }
                    } else {
                        if (st->unk80 & 0x200) {
                            st->unk7E |= 4;
                        }
                        if (st->unk80 & 0x100) {
                            st->unk7E |= 1;
                        }
                        if (st->unk80 & 0x20) {
                            st->unk7E |= 2;
                        }
                    }
                    st->unk7D = 0;
                } else if ((st->flagsB8 & 0x320) == 0) {
                    if (st->unk80 & 0x200) {
                        st->unk7E |= 4;
                    }
                    if (st->unk80 & 0x100) {
                        st->unk7E |= 1;
                    }
                    if (st->unk80 & 0x20) {
                        st->unk7E |= 2;
                    }
                    st->unk7D = 0;
                }
            break;
        }
        fn_8026BE94(self, st, 0x80, 2);
        if (st->flagsB8 & 0x80) {
            fn_8026A618(self, 0);
            fn_8026A618(self, 34);
            fn_8026A618(self, 37);
            fn_8026A618(self, 50);
        }
        if (st->flagsBC & 0x200) {
            fn_8026A618(self, 1);
            fn_8026A618(self, 20);
            fn_8026A618(self, 13);
            fn_8026A618(self, 14);
            fn_8026A618(self, 22);
        }
        if (self->unk5C5 != 0) {
            if (st->flagsBC & 0x8000) {
                fn_8026A618(self, 8);
            }
            if (st->unk7E & 1) {
                fn_8026A618(self, 10);
                fn_8026A618(self, 11);
            }
            if (st->unk7E & 2) {
                if (self->unk308 == 0) {
                    if (fn_8026F908(self, 0) >= 1) {
                        fn_8026A618(self, 27);
                    } else {
                        fn_8026A618(self, 26);
                    }
                }
                fn_8026A618(self, 76);
            }
        } else {
            if (st->unk7E & 1) {
                fn_8026A618(self, 8);
                fn_8026A618(self, 76);
            }
            if ((st->flagsCC & 0x3C) != 0 && self->unk308 == 0) {
                if (st->flagsCC & 0x10) {
                    fn_8026A618(self, 12);
                    fn_8026A618(self, 24);
                }
                fn_8026A618(self, 11);
                fn_8026A618(self, 10);
            }
            if (st->flagsB8 & 0x100) {
                if (fn_8026F908(self, 0) >= 1) {
                    fn_8026A618(self, 27);
                } else {
                    fn_8026A618(self, 26);
                }
            }
            if (st->flagsBC & 0x20) {
                fn_8026A618(self, 13);
                fn_8026A618(self, 14);
            }
        }
        if (st->flagsB8 & 0x80) {
            if (st->unk7E & 8) {
                fn_8026A618(self, 18);
                fn_8026A618(self, 19);
                fn_8026A618(self, 15);
                fn_8026A618(self, 16);
            } else if (st->unk7E & 1) {
                fn_8026A618(self, 15);
                fn_8026A618(self, 16);
            }
            fn_8026A618(self, 28);
        }
        if (st->flagsB8 & 0x80) {
            fn_8026A618(self, 17);
        }
        if (fn_803BECC8(self->unk08, 10, 1) == 1) {
            if (st->flagsB8 & 4) {
                fn_8026A618(self, 43);
            } else if (st->flagsB8 & 0x10) {
                fn_8026A618(self, 44);
            }
        }
        if (st->flagsB8 & 0x10) {
            fn_8026A618(self, 39);
        }
        if (st->flagsB8 & 4) {
            fn_8026A618(self, 38);
        }
        if (st->flagsBC & 0x10) {
            fn_8026A618(self, 33);
            fn_8026A618(self, 49);
        }
        if (st->flagsBC & 4) {
            fn_8026A618(self, 32);
            fn_8026A618(self, 48);
        }
        if (st->flagsBC & 0x40) {
            fn_8026A618(self, 3);
            fn_8026A618(self, 51);
            fn_8026A618(self, 7);
            fn_8026A618(self, 47);
            fn_8026BA1C(self);
        } else if (st->unk71 != 0) {
            if (self->unkBC & 4) {
                fn_8026A618(self, 64);
            } else if (self->unkBC & 0x10) {
                fn_8026A618(self, 65);
            }
        }
        if (st->flagsBC & 0x20) {
            fn_8026A618(self, 5);
            fn_8026A618(self, 6);
            fn_8026A618(self, 9);
            fn_8026A618(self, 53);
            if (self->unk308 == 0) {
                fn_8026A618(self, 2);
                fn_8026A618(self, 52);
                fn_8026A618(self, 77);
            }
        }
        if ((st->flagsB8 & 0x80) != 0 && (st->flagsBC & 0x20) != 0) {
            fn_8026A618(self, 71);
            fn_8026A618(self, 73);
            if (fn_8026A644(self, 1) == 0) {
                fn_8026A618(self, 70);
                fn_8026A618(self, 72);
            }
        }
        if (st->flagsB8 & 0x80) {
            fn_8026A618(self, 21);
        }
        break;
    case 1:
        if (fn_803BECC8(self->unk08, 10, 1) == 1) {
            if (st->flagsB8 & 0x2000) {
                fn_8026A618(self, 43);
            } else if (st->flagsB8 & 0x1000) {
                fn_8026A618(self, 44);
            }
        }
        if (st->flagsB8 & 4) {
            fn_8026A618(self, 0);
            fn_8026A618(self, 50);
            fn_8026A618(self, 37);
        }
        if (st->flagsBC & 0x80) {
            fn_8026A618(self, 13);
            fn_8026A618(self, 14);
            fn_8026A618(self, 10);
            fn_8026A618(self, 11);
        }
        if ((st->flagsBC & 0x20) != 0 || ((st->flagsBC & 0x200) != 0 && self->unk308 == 1)) {
            fn_8026A618(self, 1);
            fn_8026A618(self, 20);
            fn_8026A618(self, 13);
            fn_8026A618(self, 14);
        }
        if ((st->flagsB8 & 0x100) == 0 && (u8)(st->part[1] - 1) <= 7) {
            fn_8026A618(self, 47);
        }
        if (st->flagsBC & 0x100) {
            fn_8026A618(self, 3);
            fn_8026A618(self, 51);
            fn_8026A618(self, 7);
            if (st->unk78 <= 24) {
                if (st->unk58 <= lbl_8079A020) {
                    fn_8026A618(self, 32);
                } else {
                    fn_8026A618(self, 33);
                }
            }
            fn_8026BA1C(self);
        }
        if (st->flagsB8 & 0x100) {
            if (st->unk58 <= lbl_8079A020) {
                fn_8026A618(self, 38);
                if ((s8)st->part[1] > 8) {
                    fn_8026A618(self, 48);
                }
            } else {
                fn_8026A618(self, 39);
                if ((s8)st->part[1] > 8) {
                    fn_8026A618(self, 49);
                }
            }
        }
        fn_8026BE94(self, st, 4, 7);
        if (st->flagsBC & 0x40) {
            fn_8026A618(self, 8);
        }
        if ((st->flagsBC & 0x200) != 0 || fn_8026BA04(self) == 1) {
            fn_8026A618(self, 6);
            if (fn_8026A644(self, 1) == 0) {
                fn_8026A618(self, 52);
                fn_8026A618(self, 2);
                fn_8026A618(self, 5);
            }
        }
        if (((st->flagsB8 & 4) != 0 && (st->flagsBC & 0x200) != 0) || fn_8026BA04(self) == 1) {
            fn_8026A618(self, 71);
            if (st->unk58 <= lbl_8079A020) {
                fn_8026A618(self, 73);
            }
            if (fn_8026A644(self, 1) == 0) {
                fn_8026A618(self, 70);
                if (st->unk58 <= lbl_8079A020) {
                    fn_8026A618(self, 72);
                }
            }
        }
        if ((st->flagsBC & 0x220) != 0 || (st->unk7C & 4) != 0) {
            fn_8026A618(self, 9);
        }
        if (st->flagsB8 & 4) {
            fn_8026A618(self, 21);
        }
        if (st->flagsBC & 0x20) {
            fn_8026A618(self, 20);
        }
        if (st->flagsBC & 0x200) {
            fn_8026A618(self, 76);
            if (fn_8026A644(self, 1) == 0) {
                fn_8026A618(self, 53);
                fn_8026A618(self, 77);
            }
        }
        if (st->flagsBC & 0x20) {
            fn_8026A618(self, 22);
        }
        if (fn_8026B934(self) == 1) {
            fn_8026A618(self, 10);
            fn_8026A618(self, 11);
        }
        if ((st->flagsBC & 0x200) != 0 && self->unk308 == 0) {
            if (fn_8026F908(self, 0) >= 1) {
                fn_8026A618(self, 27);
            } else {
                fn_8026A618(self, 26);
            }
        }
        break;
    }
}

#pragma peephole off

/* 0x8026CBE4 */
extern "C" u32 fn_8026CBE4(_PLW* self, u8 which, u32 arg2, u32 arg3)
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


/* 0x8026CC7C: the per-weapon-class action dispatcher - a nine-entry jump table on the actor's weapon
 * class, each arm a further switch on the action state's mode byte. */
extern "C" void fn_8026CC7C(_PLW* self)
{
    ActState* st = (ActState*)&self->unkB8;
    fn_8026CC70((u32)self, (u8*)st);
    switch (self->weaponClass) {
    case 0:
        switch (st->mode) {
        case 2:
            if ((s32) self->unk5C5 != 0) {
                if ((s32) (st->unk7E & 0xB) != 0) {
                    fn_8026A678(self, 4);
                }
                if ((s32) (st->unk7E & 8) != 0) {
                    if ((s32) (st->flagsB8 & 0x80) != 0) {
                        fn_8026A618(self, 0xC);
                    } else {
                        fn_8026A618(self, 0x18);
                    }
                    fn_8026A678(self, 1);
                    fn_8026A678(self, 8);
                }
                if ((s32) (st->unk7E & 1) != 0) {
                    if ((s32) (st->flagsB8 & 0x80) != 0) {
                        fn_8026A618(self, 0xC);
                    } else {
                        fn_8026A618(self, 0x18);
                    }
                    fn_8026A678(self, 3);
                    fn_8026A678(self, 6);
                    fn_8026A678(self, 0xC);
                    if (fn_8026F908(self, 0) >= 1U) {
                        fn_8026A678(self, 0x10);
                    } else {
                        fn_8026A678(self, 0x12);
                        fn_8026A678(self, 0x13);
                    }
                }
                if ((s32) (st->unk7E & 2) != 0) {
                    fn_8026A678(self, 2);
                    fn_8026A678(self, 9);
                }
                if ((s32) (st->flagsB8 & 0x100) != 0) {
                    fn_8026A678(self, 5);
                    fn_8026A678(self, 0x11);
                    return;
                }
            } else {
                if (fn_8026F9A4(self, 1, 0x2000, 0x6000) == 1U) {
                    fn_8026A678(self, 0);
                    fn_8026A678(self, 7);
                    fn_8026A678(self, 0x10);
                    fn_8026A678(self, 0xA);
                    fn_8026A678(self, 0xB);
                    fn_8026A678(self, 0xC);
                } else if (fn_8026F9A4(self, 1, 0xE000, 0x2000) == 1U) {
                    fn_8026A678(self, 1);
                    fn_8026A678(self, 8);
                } else if (fn_8026F9A4(self, 1, 0x6000, 0xA000) == 1U) {
                    fn_8026A678(self, 2);
                    fn_8026A678(self, 9);
                } else if (fn_8026F9A4(self, 1, 0xA000, 0x2000) == 1U) {
                    fn_8026A678(self, 3);
                    fn_8026A678(self, 0x12);
                    fn_8026A678(self, 6);
                    fn_8026A678(self, 0x13);
                }
                if ((s32) (self->unkCC & 0x3C) != 0) {
                    fn_8026A678(self, 4);
                }
                if (fn_8026FC40(self, 1, 0xA000, 0x2000) == 1U) {
                    fn_8026A678(self, 5);
                }
                if (fn_8026FC40(self, 1, 0x2000, 0x6000) == 1U) {
                    fn_8026A678(self, 0x11);
                    return;
                }
            }
            break;
        case 1:
            if ((s32) (st->flagsBC & 0x200) != 0) {
                if (st->unk58 <= lbl_8079A020) {
                    fn_8026A678(self, 3);
                    fn_8026A678(self, 0x12);
                    fn_8026A678(self, 6);
                    fn_8026A678(self, 0x13);
                    if (fn_8026A644(self, 1) == 0) {
                        fn_8026A618(self, 0xC);
                        fn_8026A618(self, 0x18);
                    }
                }
            }
            if (fn_8026CBE4(self, 0, 0, 4) == 1U) {
                fn_8026A618(self, 0xF);
                fn_8026A618(self, 0x12);
                fn_8026A618(self, 0x1C);
            }
            if (fn_8026CBE4(self, 0, 0, 4) == 1U) {
                fn_8026A618(self, 0x11);
            }
            if ((s32) (st->flagsB8 & 0x200) != 0) {
                fn_8026A678(self, 5);
                fn_8026A678(self, 0x11);
            }
            if ((s32) (st->flagsBC & 0x200) != 0) {
                fn_8026A678(self, 4);
                fn_8026A678(self, 0xC);
                fn_8026A678(self, 0xE);
            }
            if ((s32) (st->flagsBC & 0x200) != 0) {
                if (st->unk54 <= lbl_8079A024) {
                    fn_8026A678(self, 9);
                } else {
                    if (st->unk54 >= lbl_8079A028) {
                        fn_8026A678(self, 8);
                    } else {
                        fn_8026A678(self, 7);
                        fn_8026A678(self, 0xB);
                        fn_8026A678(self, 0x10);
                    }
                }
            }
            if ((s32) (st->flagsBC & 0x200) != 0) {
                if (st->unk54 <= lbl_8079A024) {
                    fn_8026A678(self, 2);
                } else {
                    if (st->unk54 >= lbl_8079A028) {
                        fn_8026A678(self, 1);
                    } else {
                        fn_8026A678(self, 0);
                        fn_8026A678(self, 0xA);
                    }
                }
            } else if (fn_8026B99C(self) == 1U) {
                fn_8026A678(self, 4);
                fn_8026A678(self, 0xC);
                fn_8026A678(self, 7);
                fn_8026A678(self, 0x10);
                fn_8026A678(self, 0);
                fn_8026A678(self, 0xA);
                fn_8026A678(self, 0xF);
            }
            if ((s32) (st->unk7C & 1) != 0) {
                fn_8026A678(self, 0xD);
                return;
            }
            break;
        }
        break;
    case 4:
    case 5:
    case 6:
        switch (st->mode) {
        case 2:
            if ((s32) (st->flagsB8 & 0x80) != 0) {
                if (((s8) st->part[2] > 8) && ((u8) self->unk18 == 1)) {
                    fn_8026A618(self, 0x1D);
                }
            } else if ((u8) (st->part[2] - 1) <= 7U) {
                fn_8026A678(self, 7);
            }
            if ((s32) (st->flagsB8 & 0x2000) != 0) {
                fn_8026A678(self, 0);
            } else if ((s32) (st->flagsB8 & 0x1000) != 0) {
                fn_8026A678(self, 1);
            }
            if ((s32) (st->flagsB8 & 0x800) != 0) {
                fn_8026A678(self, 2);
            } else if ((s32) (st->flagsB8 & 0x400) != 0) {
                fn_8026A678(self, 3);
            }
            if ((s32) self->unk5C5 != 0) {
                if (((s32) (st->unk7E & 8) != 0) && ((s32) self->unk5E6 == 0)) {
                    fn_8026A678(self, 6);
                }
                if ((s32) (st->unk7E & 1) != 0) {
                    fn_8026A678(self, 5);
                }
                if ((s32) (st->unk7E & 2) != 0) {
                    fn_8026A678(self, 4);
                }
                if (((s32) (st->flagsB8 & 0x80) != 0) && ((s32) (st->unk7E & 8) != 0)) {
                    fn_8026A618(self, 0xC);
                    fn_8026A618(self, 0xA);
                    fn_8026A618(self, 0x18);
                    return;
                }
            } else {
                if ((s32) (st->flagsBC & 0x100) != 0) {
                    fn_8026A678(self, 4);
                }
                if (fn_8026F9A4(self, 1, 0x2000, 0x6000) == 1U) {
                    fn_8026A678(self, 6);
                    return;
                }
                if (((s32) self->unk308 == 0) && ((s32) (self->unkCC & 0x10) != 0)) {
                    fn_8026A678(self, 5);
                    fn_8026A618(self, 0xC);
                    fn_8026A618(self, 0x18);
                    return;
                }
            }
            break;
        case 1:
            if (fn_8026CBE4(self, 1, 1, 4) == 1U) {
                fn_8026A618(self, 0xC);
                fn_8026A618(self, 0x18);
            }
            if ((s32) (st->flagsBC & 0x200) != 0) {
                fn_8026A678(self, 4);
            }
            if (((s32) (st->flagsBC & 0x200) != 0) && ((s32) (st->flagsB8 & 0x100) != 0)) {
                fn_8026A678(self, 8);
            }
            if ((s32) (st->flagsB8 & 4) != 0) {
                if (((s8) st->part[7] > 8) && ((u8) self->unk18 == 1)) {
                    fn_8026A618(self, 0x1D);
                }
                if ((s32) (st->flagsBC & 0x40) != 0) {
                    fn_8026A618(self, 0x18);
                }
            } else if ((u8) (st->part[7] - 1) <= 7U) {
                fn_8026A678(self, 7);
            }
            if ((s32) (st->flagsBC & 0x40) != 0) {
                fn_8026A678(self, 5);
                if ((s32) (st->flagsB8 & 4) != 0) {
                    fn_8026A618(self, 0xC);
                    fn_8026A618(self, 0xA);
                    fn_8026A618(self, 0xB);
                }
            }
            if ((s32) (st->flagsB8 & 0x2000) != 0) {
                fn_8026A678(self, 0);
            } else if ((s32) (st->flagsB8 & 0x1000) != 0) {
                fn_8026A678(self, 1);
            }
            if ((s32) (st->flagsB8 & 0x800) != 0) {
                fn_8026A678(self, 2);
            } else if ((s32) (st->flagsB8 & 0x400) != 0) {
                fn_8026A678(self, 3);
            }
            if ((fn_8026B99C(self) == 1U) && ((s32) self->unk5E6 == 0)) {
                fn_8026A678(self, 6);
                return;
            }
            break;
        }
        break;
    case 1:
        switch (st->mode) {
        case 2:
            if ((s32) self->unk5C5 != 0) {
                if (((s32) (st->flagsB8 & 0x80) != 0) && ((s32) (st->flagsBC & 0x200) != 0)) {
                    fn_8026A678(self, 0x15);
                }
                if ((s32) (st->flagsBC & 0x8000) != 0) {
                    fn_8026A678(self, 3);
                    fn_8026A678(self, 0xF);
                    fn_8026A678(self, 0x13);
                    fn_8026A678(self, 8);
                    fn_8026A678(self, 0x14);
                }
                if ((s32) (st->unk7E & 0xB) != 0) {
                    fn_8026A678(self, 9);
                }
                if ((s32) (st->unk7E & 8) != 0) {
                    fn_8026A678(self, 7);
                }
                if ((s32) (st->unk7E & 1) != 0) {
                    if (fn_8026F908(self, 0) >= 1U) {
                        fn_8026A678(self, 4);
                        fn_8026A678(self, 5);
                    }
                    if (fn_8026FA6C(self, 0, 0xAAAB, 0xD555) == 1U) {
                        fn_8026A678(self, 0xF);
                    }
                    fn_8026A678(self, 0);
                    fn_8026A678(self, 6);
                    fn_8026A678(self, 0xD);
                    fn_8026A678(self, 0xE);
                    fn_8026A678(self, 0x11);
                    fn_8026A678(self, 0x12);
                    fn_8026A678(self, 5);
                    fn_8026A678(self, 0xA);
                }
                if ((s32) (st->unk7E & 2) != 0) {
                    fn_8026A678(self, 0xC);
                    fn_8026A678(self, 0xB);
                    if (fn_8026F908(self, 0) >= 1U) {
                        fn_8026A678(self, 1);
                        fn_8026A618(self, 0x4B);
                        return;
                    }
                    fn_8026A678(self, 2);
                    fn_8026A618(self, 0x4A);
                    return;
                }
            } else {
                if ((s32) (st->flagsBC & 0x200) != 0) {
                    fn_8026A678(self, 0x15);
                }
                if (fn_8026F9A4(self, 1, 0x1555, 0x6AAB) == 1U) {
                    if (fn_8026F908(self, 0) >= 1U) {
                        fn_8026A678(self, 4);
                        fn_8026A678(self, 5);
                    }
                    fn_8026A678(self, 0);
                    fn_8026A678(self, 6);
                    fn_8026A678(self, 0xD);
                    fn_8026A678(self, 0xE);
                    fn_8026A678(self, 0x11);
                    fn_8026A678(self, 0x12);
                    fn_8026A678(self, 5);
                    fn_8026A678(self, 0xA);
                } else if (fn_8026F9A4(self, 1, 0xD555, 0x1555) == 1U) {
                    fn_8026A678(self, 1);
                    fn_8026A678(self, 0xC);
                    if (fn_8026F908(self, 0) >= 1U) {
                        fn_8026A618(self, 0x4B);
                    } else {
                        fn_8026A618(self, 0x4A);
                    }
                } else if (fn_8026F9A4(self, 1, 0x6AAB, 0xAAAB) == 1U) {
                    fn_8026A678(self, 2);
                    fn_8026A678(self, 0xB);
                } else if (fn_8026F9A4(self, 1, 0xAAAB, 0xD555) == 1U) {
                    if (fn_8026FA6C(self, 0, 0xAAAB, 0xD555) == 1U) {
                        fn_8026A678(self, 0xF);
                    }
                    fn_8026A678(self, 7);
                }
                if ((s32) (st->flagsBC & 0x100) != 0) {
                    fn_8026A678(self, 3);
                    fn_8026A678(self, 8);
                    fn_8026A678(self, 0xF);
                    fn_8026A678(self, 0x13);
                    fn_8026A678(self, 0x14);
                }
                if ((s32) (self->unkCC & 0x3C) != 0) {
                    fn_8026A678(self, 9);
                    return;
                }
            }
            break;
        case 1:
            if ((st->unk58 <= lbl_8079A020) && ((s32) (st->flagsBC & 0x200) != 0) && (fn_8026A644(self, 1) == 0)) {
                fn_8026A618(self, 0xC);
                fn_8026A618(self, 0x18);
            }
            if (fn_8026CBE4(self, 0, 0, 4) == 1U) {
                fn_8026A618(self, 0xF);
                fn_8026A618(self, 0x12);
                fn_8026A618(self, 0x1C);
            }
            if (fn_8026CBE4(self, 0, 0, 4) == 1U) {
                fn_8026A618(self, 0x11);
            }
            if ((fn_8026A6F4(self, 0xB) == 0) && ((s32) (st->flagsBC & 0x200) != 0)) {
                if (st->unk58 <= lbl_8079A020) {
                    fn_8026A678(self, 7);
                }
            }
            if (((s32) (st->flagsBC & 0x200) != 0) && (fn_8026A6F4(self, 0xB) == 0)) {
                if (st->unk54 <= lbl_8079A024) {
                    fn_8026A678(self, 2);
                } else {
                    if (st->unk54 >= lbl_8079A028) {
                        fn_8026A678(self, 1);
                    } else {
                        fn_8026A678(self, 0);
                        fn_8026A678(self, 6);
                    }
                }
                if (fn_8026F908(self, 0) >= 1U) {
                    fn_8026A678(self, 3);
                }
                fn_8026A678(self, 9);
                fn_8026A678(self, 0xA);
            } else if (fn_8026B99C(self) == 1U) {
                fn_8026A678(self, 0xA);
                fn_8026A678(self, 6);
                fn_8026A678(self, 0xE);
                fn_8026A678(self, 7);
                fn_8026A678(self, 0x11);
                fn_8026A678(self, 0x12);
                fn_8026A678(self, 9);
            }
            if (((s32) (st->flagsBC & 0x200) != 0) && (fn_8026A6F4(self, 0xB) == 0)) {
                if (st->unk54 <= lbl_8079A024) {
                    fn_8026A678(self, 0xB);
                } else {
                    if (st->unk54 >= lbl_8079A028) {
                        fn_8026A678(self, 0xC);
                        if (fn_8026F908(self, 0) >= 1U) {
                            fn_8026A618(self, 0x4B);
                        } else {
                            fn_8026A618(self, 0x4A);
                        }
                    } else {
                        fn_8026A678(self, 0xD);
                        fn_8026A678(self, 0xE);
                    }
                }
                if (fn_8026FA6C(self, 0, 0xA000, 0xE000) == 1U) {
                    fn_8026A678(self, 0xF);
                }
                if (fn_8026F908(self, 0) >= 1U) {
                    fn_8026A678(self, 8);
                }
                fn_8026A678(self, 5);
                fn_8026A678(self, 0x11);
                fn_8026A678(self, 0x12);
            }
            if ((s32) (st->flagsBC & 0x40) != 0) {
                if (fn_8026F908(self, 0) >= 1U) {
                    fn_8026A678(self, 4);
                    fn_8026A678(self, 5);
                }
                fn_8026A678(self, 0x14);
                fn_8026A678(self, 0xF);
                fn_8026A678(self, 8);
                fn_8026A678(self, 0x13);
                fn_8026A678(self, 3);
            }
            if (((s32) (st->flagsBC & 0x20) != 0) || (((s32) (st->flagsBC & 0x200) != 0) && ((u8) self->unk308 == 1))) {
                fn_8026A678(self, 0x15);
                return;
            }
            break;
        }
        break;
    case 3:
        switch (st->mode) {
        case 2:
            if ((s32) (st->flagsBC & 0x40) != 0) {
                fn_8026A678(self, 3);
            }
            if ((s32) self->unk5C5 != 0) {
                if ((s32) (st->flagsBC & 0x8000) != 0) {
                    fn_8026A678(self, 2);
                }
                if ((s32) (st->unk7E & 0xB) != 0) {
                    fn_8026A678(self, 8);
                }
                if ((s32) (st->unk7E & 8) != 0) {
                    fn_8026A678(self, 0xC);
                    fn_8026A678(self, 0xD);
                }
                if ((s32) (st->unk7E & 1) != 0) {
                    fn_8026A678(self, 0);
                    fn_8026A678(self, 1);
                    fn_8026A678(self, 4);
                    fn_8026A678(self, 0xF);
                    fn_8026A678(self, 0x12);
                    if (fn_8026F908(self, 0) >= 1U) {
                        fn_8026A678(self, 0xE);
                    } else {
                        fn_8026A678(self, 7);
                    }
                }
                if ((s32) (st->unk7E & 2) != 0) {
                    fn_8026A678(self, 5);
                    fn_8026A678(self, 6);
                }
                if ((s32) (st->flagsB8 & 0x80) != 0) {
                    if ((s32) (st->unk7E & 2) != 0) {
                        fn_8026A678(self, 0xA);
                        fn_8026A678(self, 0x10);
                        return;
                    }
                } else {
                    fn_8026A678(self, 0xB);
                    return;
                }
            } else {
                if (fn_8026F9A4(self, 1, 0x1555, 0x6AAB) == 1U) {
                    fn_8026A678(self, 0);
                    fn_8026A678(self, 1);
                    fn_8026A678(self, 4);
                    if (fn_8026F908(self, 0) >= 1U) {
                        fn_8026A678(self, 0xE);
                    } else {
                        fn_8026A678(self, 7);
                    }
                    fn_8026A678(self, 0x12);
                    fn_8026A678(self, 0xF);
                } else if (fn_8026F9A4(self, 1, 0xD555, 0x1555) == 1U) {
                    fn_8026A678(self, 0xC);
                    fn_8026A678(self, 0xD);
                } else if (fn_8026F9A4(self, 1, 0x6AAB, 0xAAAB) == 1U) {
                    fn_8026A678(self, 9);
                    fn_8026A678(self, 0xA);
                } else if (fn_8026F9A4(self, 1, 0xAAAB, 0xD555) == 1U) {
                    fn_8026A678(self, 5);
                    fn_8026A678(self, 6);
                }
                if (fn_8026FC40(self, 1, 0x6AAB, 0xAAAB) == 0) {
                    fn_8026A678(self, 0xB);
                }
                if ((s32) (st->flagsBC & 0x100) != 0) {
                    fn_8026A678(self, 2);
                }
                if ((s32) (self->unkCC & 0x3C) != 0) {
                    fn_8026A678(self, 8);
                    return;
                }
            }
            break;
        case 1:
            if ((st->unk58 <= lbl_8079A020) && ((s32) (st->flagsBC & 0x200) != 0) && (fn_8026A644(self, 1) == 0)) {
                fn_8026A678(self, 9);
                fn_8026A678(self, 0xA);
            }
            if (((s32) (st->flagsB8 & 0x200) == 0) || ((s32) (st->unk7C & 1) != 0)) {
                fn_8026A678(self, 0xB);
            }
            if ((s32) (st->flagsBC & 0x100) != 0) {
                fn_8026A678(self, 3);
            }
            if ((s32) (st->flagsBC & 0x40) != 0) {
                fn_8026A678(self, 2);
            }
            if (fn_8026CBE4(self, 0, 0, 4) == 1U) {
                fn_8026A618(self, 0xF);
                fn_8026A618(self, 0x12);
                fn_8026A618(self, 0x1C);
            }
            if (fn_8026CBE4(self, 0, 0, 4) == 1U) {
                fn_8026A618(self, 0x11);
            }
            if (((s32) (st->flagsBC & 0x200) != 0) && (fn_8026A6F4(self, 0xB) == 0)) {
                fn_8026A678(self, 0x11);
                if (st->unk54 <= lbl_8079A024) {
                    fn_8026A678(self, 5);
                    fn_8026A678(self, 6);
                } else {
                    if (st->unk54 >= lbl_8079A028) {
                        fn_8026A678(self, 0xC);
                        fn_8026A678(self, 0xD);
                    } else {
                        fn_8026A678(self, 0);
                        fn_8026A678(self, 1);
                    }
                }
                fn_8026A678(self, 4);
                fn_8026A678(self, 7);
                fn_8026A678(self, 0xF);
                fn_8026A678(self, 0x12);
            } else if (fn_8026B99C(self) == 1U) {
                fn_8026A678(self, 0);
                fn_8026A678(self, 1);
                fn_8026A678(self, 0x11);
                fn_8026A678(self, 7);
                fn_8026A678(self, 0xE);
                fn_8026A678(self, 0xF);
                fn_8026A678(self, 0x12);
                fn_8026A678(self, 4);
            }
            if (((s32) (st->flagsBC & 0x200) != 0) && (fn_8026A6F4(self, 0xB) == 0)) {
                if (st->unk54 <= lbl_8079A024) {
                    fn_8026A678(self, 5);
                    fn_8026A678(self, 6);
                } else {
                    if (st->unk54 >= lbl_8079A028) {
                        fn_8026A678(self, 0xC);
                        fn_8026A678(self, 0xD);
                    } else {
                        fn_8026A678(self, 0);
                        fn_8026A678(self, 1);
                    }
                }
                fn_8026A678(self, 4);
                fn_8026A678(self, 8);
                return;
            }
            break;
        }
        break;
    case 2:
        switch (st->mode) {
        case 2:
            if ((s32) (st->flagsB8 & 0x80) != 0) {
                fn_8026A618(self, 0x11);
            } else {
                fn_8026A678(self, 5);
            }
            if ((s32) self->unk5C5 != 0) {
                if ((s32) (st->unk7E & 0xB) != 0) {
                    fn_8026A678(self, 6);
                    fn_8026A678(self, 5);
                }
                if ((s32) (st->unk7E & 9) != 0) {
                    fn_8026A678(self, 0);
                    fn_8026A678(self, 2);
                    fn_8026A618(self, 0xB);
                    fn_8026A618(self, 0xA);
                }
                if ((s32) (st->unk7E & 2) != 0) {
                    fn_8026A678(self, 1);
                    fn_8026A678(self, 3);
                    return;
                }
            } else {
                if (fn_8026F9A4(self, 1, 0xEAAB, 0x9555) == 1U) {
                    fn_8026A678(self, 0);
                    fn_8026A678(self, 2);
                    if ((s32) self->unk308 == 0) {
                        fn_8026A618(self, 0xB);
                        fn_8026A618(self, 0xA);
                    }
                } else if (fn_8026F9A4(self, 1, 0x9555, 0xEAAB) == 1U) {
                    fn_8026A678(self, 1);
                    fn_8026A678(self, 3);
                }
                if ((s32) (self->unkCC & 0x3C) != 0) {
                    fn_8026A678(self, 6);
                    fn_8026A678(self, 5);
                    return;
                }
            }
            break;
        case 1:
            if (fn_8026CBE4(self, 0, 1, 4) == 1U) {
                fn_8026A618(self, 0x11);
                fn_8026A618(self, 0xF);
                fn_8026A618(self, 0x12);
                fn_8026A618(self, 0x1C);
            } else {
                fn_8026A678(self, 5);
            }
            if ((s32) (st->flagsBC & 0x200) != 0) {
                fn_8026A678(self, 6);
            }
            if ((s32) (st->flagsBC & 0x200) != 0) {
                fn_8026A678(self, 2);
                if (st->unk54 <= lbl_8079A024) {
                    fn_8026A678(self, 3);
                } else {
                    if (st->unk54 >= lbl_8079A028) {
                        fn_8026A678(self, 3);
                    }
                }
            }
            if ((s32) (st->flagsBC & 0x200) != 0) {
                fn_8026A678(self, 5);
                if (st->unk54 <= lbl_8079A024) {
                    fn_8026A678(self, 1);
                } else {
                    if (st->unk54 >= lbl_8079A028) {
                        fn_8026A678(self, 1);
                    } else {
                        fn_8026A678(self, 0);
                    }
                }
            } else if (fn_8026B99C(self) == 1U) {
                fn_8026A678(self, 6);
                fn_8026A678(self, 0);
                fn_8026A678(self, 2);
                fn_8026A678(self, 5);
            }
            if ((s32) (st->flagsBC & 0x40) != 0) {
                fn_8026A678(self, 1);
                fn_8026A678(self, 3);
                return;
            }
            break;
        }
        break;
    case 7:
        switch (st->mode) {
        case 2:
            if ((s32) (st->flagsBC & 0x80) != 0) {
                fn_8026A678(self, 6);
                fn_8026A678(self, 9);
                fn_8026A678(self, 0xD);
            }
            if ((s32) self->unk5C5 != 0) {
                if ((s32) (st->unk7E & 0xB) != 0) {
                    fn_8026A678(self, 7);
                }
                if ((s32) (st->unk7E & 8) != 0) {
                    fn_8026A678(self, 1);
                    fn_8026A678(self, 4);
                    if (fn_8026FB20(self, 0x800) == 1U) {
                        fn_8026A678(self, 0x12);
                    } else if (fn_8026FB20(self, 0x400) == 1U) {
                        fn_8026A678(self, 0x11);
                    }
                }
                if ((s32) (st->unk7E & 1) != 0) {
                    fn_8026A678(self, 0);
                    fn_8026A678(self, 3);
                    fn_8026A678(self, 8);
                    fn_8026A618(self, 0xB);
                    fn_8026A618(self, 0xA);
                    fn_8026A678(self, 0xC);
                }
                if ((s32) (st->unk7E & 2) != 0) {
                    fn_8026A678(self, 2);
                    fn_8026A678(self, 0xE);
                    fn_8026A678(self, 5);
                    fn_8026A678(self, 0xC);
                    return;
                }
            } else {
                if (fn_8026F9A4(self, 1, 0x1555, 0x6AAB) == 1U) {
                    fn_8026A678(self, 0);
                    fn_8026A678(self, 3);
                    fn_8026A678(self, 8);
                    fn_8026A678(self, 0xC);
                    if ((s32) self->unk308 == 0) {
                        fn_8026A618(self, 0xB);
                        fn_8026A618(self, 0xA);
                    }
                } else if (fn_8026F9A4(self, 1, 0xD555, 0x1555) == 1U) {
                    fn_8026A678(self, 0xF);
                    fn_8026A678(self, 0x11);
                } else if (fn_8026F9A4(self, 1, 0x6AAB, 0xAAAB) == 1U) {
                    fn_8026A678(self, 0x10);
                    fn_8026A678(self, 0x12);
                } else if (fn_8026F9A4(self, 1, 0xAAAB, 0xD555) == 1U) {
                    fn_8026A678(self, 1);
                    fn_8026A678(self, 4);
                }
                if ((s32) (st->flagsBC & 0x100) != 0) {
                    fn_8026A678(self, 2);
                    fn_8026A678(self, 0xE);
                    fn_8026A678(self, 5);
                }
                if ((s32) (self->unkCC & 0x3C) != 0) {
                    fn_8026A678(self, 7);
                    return;
                }
            }
            break;
        case 1:
            if (fn_8026CBE4(self, 1, 1, 4) == 1U) {
                fn_8026A618(self, 0xF);
                fn_8026A618(self, 0x12);
                fn_8026A618(self, 0x1C);
                fn_8026A678(self, 6);
                fn_8026A678(self, 9);
            }
            if ((s32) (st->flagsBC & 0x200) != 0) {
                fn_8026A678(self, 0xC);
                fn_8026A678(self, 0xD);
                fn_8026A678(self, 8);
                fn_8026A678(self, 0xE);
                if (st->unk54 <= lbl_8079A024) {
                    fn_8026A678(self, 5);
                    fn_8026A678(self, 0xA);
                } else {
                    if (st->unk54 >= lbl_8079A028) {
                        fn_8026A678(self, 5);
                        fn_8026A678(self, 0xA);
                    } else {
                        fn_8026A678(self, 3);
                        fn_8026A678(self, 0xB);
                    }
                }
            }
            if ((s32) (st->flagsBC & 0x200) != 0) {
                fn_8026A678(self, 7);
                if (st->unk54 <= lbl_8079A024) {
                    fn_8026A678(self, 2);
                } else {
                    if (st->unk54 >= lbl_8079A028) {
                        fn_8026A678(self, 2);
                    } else {
                        fn_8026A678(self, 0);
                    }
                }
            } else if (fn_8026B99C(self) == 1U) {
                fn_8026A678(self, 0);
                fn_8026A678(self, 3);
                fn_8026A678(self, 8);
                fn_8026A678(self, 7);
                fn_8026A678(self, 0xB);
                fn_8026A678(self, 0xE);
            }
            if ((s32) (st->flagsBC & 0x40) != 0) {
                fn_8026A678(self, 1);
                fn_8026A678(self, 4);
                if (st->unk54 >= lbl_8079A028) {
                    fn_8026A678(self, 0xF);
                    fn_8026A678(self, 0x11);
                    return;
                }
                if (st->unk54 <= lbl_8079A024) {
                    fn_8026A678(self, 0x10);
                    fn_8026A678(self, 0x12);
                    return;
                }
            }
            break;
        }
        break;
    case 8:
        switch (st->mode) {
        case 2:
            if ((s32) (st->flagsBC & 0x80) != 0) {
                fn_8026A678(self, 0);
                fn_8026A678(self, 5);
            }
            if ((s32) (st->flagsBC & 0x40) != 0) {
                fn_8026A678(self, 9);
            }
            if ((s32) self->unk5C5 != 0) {
                if ((s32) (st->unk7E & 0xB) != 0) {
                    fn_8026A678(self, 0xD);
                    fn_8026A678(self, 0x1F);
                }
                if ((s32) (st->unk7E & 8) != 0) {
                    fn_8026A678(self, 3);
                    fn_8026A678(self, 0x10);
                    fn_8026A678(self, 0x19);
                    fn_8026A678(self, 0x1C);
                    fn_8026A678(self, 1);
                    if ((s32) fn_8026F908(self, 0) == 0) {
                        fn_8026A678(self, 0xC);
                    }
                    fn_8026A678(self, 0xA);
                    fn_8026A678(self, 0x13);
                }
                if ((s32) (st->unk7E & 1) != 0) {
                    fn_8026A678(self, 0xE);
                    fn_8026A678(self, 0xF);
                    fn_8026A678(self, 0x1B);
                    fn_8026A678(self, 0x1C);
                    fn_8026A678(self, 0x1D);
                    fn_8026A678(self, 1);
                    if ((s32) fn_8026F908(self, 0) == 0) {
                        fn_8026A678(self, 0xC);
                    }
                    fn_8026A678(self, 0xA);
                    fn_8026A678(self, 0x13);
                    fn_8026A678(self, 0x11);
                    fn_8026A678(self, 0x12);
                    fn_8026A678(self, 7);
                    fn_8026A678(self, 0x16);
                    if (fn_8026FB20(self, 0x1000) == 1U) {
                        fn_8026A678(self, 8);
                    }
                }
                if ((s32) (st->unk7E & 2) != 0) {
                    fn_8026A678(self, 2);
                    fn_8026A678(self, 0xB);
                    fn_8026A678(self, 0x17);
                    fn_8026A678(self, 0x18);
                    fn_8026A678(self, 0x14);
                    fn_8026A678(self, 0x15);
                }
                if ((s32) (st->flagsBC & 0x8000) != 0) {
                    fn_8026A678(self, 6);
                    fn_8026A678(self, 7);
                    fn_8026A678(self, 4);
                    fn_8026A678(self, 0x13);
                    if (fn_8026FB20(self, 0x1000) == 1U) {
                        fn_8026A678(self, 8);
                        return;
                    }
                }
            } else {
                if (fn_8026F9A4(self, 1, 0x2000, 0x6000) == 1U) {
                    fn_8026A678(self, 0xE);
                    fn_8026A678(self, 0xF);
                    fn_8026A678(self, 0x1C);
                    fn_8026A678(self, 0x1D);
                    fn_8026A678(self, 1);
                    fn_8026A678(self, 0xA);
                    fn_8026A678(self, 0x11);
                    fn_8026A678(self, 0x12);
                    fn_8026A678(self, 0x13);
                    if ((s32) fn_8026F908(self, 0) == 0) {
                        fn_8026A678(self, 0xC);
                    }
                    fn_8026A678(self, 7);
                    fn_8026A678(self, 0x16);
                    if (fn_8026FB20(self, 0x1000) == 1U) {
                        fn_8026A678(self, 8);
                    }
                } else {
                    if ((fn_8026F9A4(self, 1, 0xE000, 0x2000) == 1U) || (fn_8026F9A4(self, 1, 0x6000, 0xA000) == 1U)) {
                        fn_8026A678(self, 2);
                        fn_8026A678(self, 0xB);
                        fn_8026A678(self, 3);
                        fn_8026A678(self, 0x10);
                        fn_8026A678(self, 0x19);
                        fn_8026A678(self, 0x1C);
                    } else if (fn_8026F9A4(self, 1, 0xA000, 0x2000) == 1U) {
                        fn_8026A678(self, 0x1B);
                        fn_8026A678(self, 0x1C);
                        fn_8026A678(self, 4);
                        fn_8026A678(self, 0x13);
                        fn_8026A678(self, 0x17);
                        fn_8026A678(self, 0x14);
                        fn_8026A678(self, 0x15);
                        fn_8026A678(self, 0x18);
                        if ((s32) fn_8026F908(self, 0) == 0) {
                            fn_8026A678(self, 0xC);
                        }
                    }
                    if ((s32) (st->flagsBC & 0x100) != 0) {
                        fn_8026A678(self, 6);
                        fn_8026A678(self, 7);
                        if (fn_8026FB20(self, 0x1000) == 1U) {
                            fn_8026A678(self, 8);
                        }
                    }
                    if ((s32) (self->unkCC & 0x3C) != 0) {
                        fn_8026A678(self, 7);
                        if (fn_8026FB20(self, 0x1000) == 1U) {
                            fn_8026A678(self, 8);
                        }
                    }
                }
                if ((s32) (self->unkCC & 0x3C) != 0) {
                    fn_8026A678(self, 0xD);
                    fn_8026A678(self, 0x1F);
                    return;
                }
            }
            break;
        case 1:
            if (fn_8026CBE4(self, 1, 1, 4) == 1U) {
                fn_8026A618(self, 0xF);
                fn_8026A618(self, 0x12);
                fn_8026A618(self, 0x1C);
                fn_8026A678(self, 0);
                fn_8026A678(self, 5);
            }
            if ((st->unk58 <= lbl_8079A020) && ((s32) (st->flagsBC & 0x200) != 0)) {
                if ((s32) fn_8026F908(self, 0) == 0) {
                    fn_8026A678(self, 0xC);
                }
                fn_8026A678(self, 0x14);
                fn_8026A678(self, 0x15);
            }
            if ((s32) (st->flagsBC & 0x100) != 0) {
                fn_8026A678(self, 9);
            }
            if ((s32) (st->flagsBC & 0x40) != 0) {
                fn_8026A678(self, 6);
                fn_8026A678(self, 7);
                if (fn_8026FB20(self, 0x1000) == 1U) {
                    fn_8026A678(self, 8);
                }
                fn_8026A678(self, 4);
                fn_8026A678(self, 0x13);
            }
            if ((s32) (st->flagsBC & 0x200) != 0) {
                fn_8026A678(self, 0xD);
                fn_8026A678(self, 0x1F);
                fn_8026A678(self, 0xA);
                fn_8026A678(self, 0x12);
                fn_8026A678(self, 0x13);
                if (st->unk58 > lbl_8079A020) {
                    fn_8026A678(self, 0xF);
                }
                fn_8026A678(self, 0x1C);
                fn_8026A678(self, 7);
                if (fn_8026FB20(self, 0x1000) == 1U) {
                    fn_8026A678(self, 8);
                }
                fn_8026A678(self, 0x16);
                fn_8026A678(self, 0x1D);
                if (st->unk54 <= lbl_8079A024) {
                    fn_8026A678(self, 0xB);
                    fn_8026A678(self, 0x10);
                    fn_8026A678(self, 0x19);
                } else {
                    if (st->unk54 >= lbl_8079A028) {
                        fn_8026A678(self, 0xB);
                        fn_8026A678(self, 0x18);
                        fn_8026A678(self, 0x1A);
                    }
                }
            }
            if ((s32) (st->flagsBC & 0x200) != 0) {
                if (st->unk58 > lbl_8079A020) {
                    fn_8026A678(self, 0xE);
                }
                fn_8026A678(self, 0x1B);
                fn_8026A678(self, 1);
                fn_8026A678(self, 0x11);
                if (st->unk54 <= lbl_8079A024) {
                    fn_8026A678(self, 2);
                    fn_8026A678(self, 3);
                    return;
                }
                if (st->unk54 >= lbl_8079A028) {
                    fn_8026A678(self, 2);
                    fn_8026A678(self, 0x17);
                    return;
                }
            } else if (fn_8026B99C(self) == 1U) {
                fn_8026A678(self, 1);
                fn_8026A678(self, 0xA);
                fn_8026A678(self, 0x13);
                fn_8026A678(self, 0x17);
                fn_8026A678(self, 0x18);
                fn_8026A678(self, 0x10);
                fn_8026A678(self, 0xD);
                fn_8026A678(self, 0x1F);
                fn_8026A678(self, 7);
                fn_8026A678(self, 0x1B);
                fn_8026A678(self, 0x1C);
                if (fn_8026FB20(self, 0x1000) == 1U) {
                    fn_8026A678(self, 8);
                }
                if ((s32) fn_8026F908(self, 0) == 0) {
                    fn_8026A678(self, 0xC);
                }
            }
            break;
        }
        break;
    }
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
    s16 value = (s16)self->unkD4[idx];
    /* Dead copies, load-bearing for the allocator: retail's colouring needs the class web to be born
     * after a chain of copies of it (see the unit header). */
    u32 classCopy0 = self->weaponClass;
    u32 classCopy1 = classCopy0;
    u32 classCopy2 = classCopy1;
    u32 weaponClass = self->weaponClass;
    s32 high;
    s32 mid;
    s32 low;

    if ((u32)(weaponClass - 4) <= 2 && self->unk18 == 1 && self->unk5E6 == 1) {
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

    VEC3_ctor(&vec);
    p = (u32*)((u8*)self->unk13C + 4);
    p[10] = self->unk54;
    p[11] = self->unk58;
    p[12] = self->unk5C;
    copyVec3(&p[1], (u8*)self + 60);
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
    return my_player_no() == self->unk08;
}

/* 0x8026FE44: true for the "gun" weapon classes. */
extern "C" u32 fn_8026FE44(_PLW* self)
{
    return (u32)(self->weaponClass - 4) <= 2;
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
