/*
 * Player action module (Pl_act): the largest of the three Pl clusters. .text 0x80276B58-0x8027D684
 * (115 functions, 0x6B2C B) with its own exception tables - extab 0x8001294C-0x80012B54, extabindex
 * 0x8002FC70-0x8002FF7C.
 *
 * Left edge pinned by the `.sdata2` pool run (`lbl_8079A0AC`), right edge by the closure; the reasoning is in
 * configure.py beside the Pl lib entry.
 *
 * It is C++ (the map holds the mangled `Pl_attack_set_sub__FP4_PLWP9_HIT_DATAP6_HIT_WUs`,
 * `Pl_suimen_ck__FP4_PLW`, `Pl_get_gunner_pos__FP4_PLWPQ34nw4r4math4VEC3l`, ...), so the actor type is
 * `_PLW` - that spelling is what the map's mangling encodes - and every unmangled `fn_*` callee is
 * `extern "C"`.
 *
 * Flags - measured against `build/RMHE08/obj/Pl/pl_act.o`, and **the store needs two changes** (both are a
 * this-unit `cflags_pl_act`, never `cflags_base`):
 *   * `-O4,p` -> **`-O3`**. `-O4,p` implies `-func_align 16`; the retail function starts are packed on 4 B
 *     (+0x23c, +0x588, +0xcbc), which alone rules it out. On the codegen axis `-O4,p` also loses badly:
 *     fn_80276B58 85.9 / fn_80276CE8 84.7 / fn_80276D94 86.1 / fn_80276E08 87.9, against `-O3`'s
 *     97.3 / 90.0 / 96.4 / 89.6 (same source).
 *   * peephole **off** (`-opt nopeephole`, i.e. `-O3 -opt nopeephole`): the retail object carries exactly
 *     one record-form instruction in all 115 functions (`andi. r0,r0,20` at 0xcbc, an `x & 0x20`
 *     truth test from instruction selection), so the peephole's sign/zero-extend compare fusion never
 *     fired in the retail build. With it on, fn_80276B58 = 97.3 / fn_80276CE8 = 90.0 /
 *     fn_80276D94 = 96.4 / fn_80276E08 = 89.6; with it off, 100.0 / 92.7 / 100.0 / 94.0.
 *     `-O4,p -opt nopeephole` is worse still (92.5 / 87.4 / 78.7 / 93.9), so the level is `-O3`.
 *   * `-inline auto` -> **`-inline noauto`** (playbook 28): with `auto` the 46-instruction
 *     `fn_802770E8` is inlined into all three arms of `fn_802771A0` (57 -> 229 instructions, 0.00 %);
 *     `noauto` puts it back at 100.00 % and moves nothing else. `-inline off` measures identically here,
 *     but `noauto` is the spelling the sibling `main.cpp` needed, so it is the one to commit.
 *   * `-func_align` stays out: `-O3` already packs on 4 B, which is what the retail starts want.
 *   * the s16-parameter convention: retail sign-extends a `s16` parameter at its first use and keeps the
 *     *raw* register live, which MWCC only does when the parameter is declared `s32` and cast to `s16` at
 *     each use (`fn_80276D94`: `s16 arg1` truncates the `+= 150` and costs 96.4 %, `s32 arg1` + `(s16)`
 *     casts is byte-identical). The unmangled `fn_*` symbols are free to be declared this way, but a
 *     callee's *declaration* has to match its definition's type, so the shared `fn_80276CE8` is declared
 *     `s32` and its call sites cast explicitly.
 *
 * Residual (work in progress - the functions below 100 %, each measured with the flags above, i.e.
 * `-O3 -opt nopeephole -inline noauto`):
 *   * `fn_80276B58` 99.95 % - the only differing row is the pool constant of the
 *     int->float idiom: retail `lfd f1, lbl_8079A0A0@sda21`, ours the synthesised `lfd f1, @37@sda21`.
 *     Unclaimed `.sdata2` (playbook 23): the constants MWCC generates for `(f32)(s16)x` live in our own
 *     object's pool under `@NN` names, so the row stays an ARG until the pool is claimed.
 *   * `fn_80276CE8` 92.67 % - 172 B both, 43 vs 44 rows. Ours emits the `extsh` of the clamped sum one
 *     slot early (`extsh r0,r0; sth r0,0x37a(r30)` where retail stores first); every later row is shifted
 *     by one. Six source shapes tried (an `int` sum with the casts at the compares, a direct
 *     `self->unk37A = self->unk37A + arg1` store, an isolated `s16 v = (s16)t` for the compare) all give
 *     either the same swap or an extra register copy, so it is an operand-order tie-break, not a type.
 *   * `fn_80276E08` 94.00 % - 182 vs 181 rows. Two residuals: the final `v += 1` (the `unk020 & 1`
 *     arm) is the one increment the retail build *does* truncate (`addi r0,r31,1; extsh r31,r0` where the
 *     three earlier ones are plain `addi r31,r31,1`), and the same `extsh`-before-`sth` swap as
 *     fn_80276CE8 in the `unk37C` clamp. `s32 v` + `(s16)` casts at the compares is what removes the three
 *     earlier truncations (184 -> 181 rows); a `s16 v` costs them all.
 *   * `fn_802770E8` 100.00 % - the 48-byte clear at +0x322 is a *flat byte loop*
 *     (`for (s16 i = 0; i < 48; i++) self->unk322[i] = 0;`): MWCC unrolls it by 8 and then by 3, which is
 *     where the `li r0,2; mtctr r0` / `bdnz` and the three 8-store blocks with one running index come from.
 *     Spelling it as the nested loops it looks like (`i < 16; i += 8` outer, `j < 8` inner) keeps the body
 *     but replaces the counted loop with a `blt` (76 %); a nested `j < 24` gets the body back at 76 %.
 *   * `fn_802771A0` 100.00 % - the two range tables must be declared as *arrays* (`extern u32
 *     lbl_805E2248[];`) and passed decayed: taking `&scalar` makes MWCC fold the address into a 16-bit
 *     `li` with a lone `@l` relocation.
 *
 * Written so far (address order, .text 0x80276B58+): B58, CE8, D94, E08, 70E0, 70E8, 71A0 - seven of the
 * 115, all but two at >= 92 %. `Pl_attack_set_sub` (0x80277284, 1776 B) is the next and is not started.
 */

#include "types.h"

/* The actor the whole Pl_* family takes as its first argument. Only the offsets this unit touches are named;
 * everything in between is padding. */
struct _PLW {
    /* 0x000 */ u8 unk000[2];
    /* 0x002 */ u8 unk002;
    /* 0x003 */ u8 unk003[0x09 - 0x03];
    /* 0x009 */ u8 unk009;
    /* 0x00A */ u8 unk00A[0x16 - 0x0A];
    /* 0x016 */ u8 unk016;
    /* 0x017 */ u8 unk017[0x20 - 0x17];
    /* 0x020 */ u32 unk020;
    /* 0x024 */ u8 unk024[0x313 - 0x24];
    /* 0x313 */ s8 unk313;
    /* 0x314 */ u8 unk314;
    /* 0x315 */ u8 unk315[0x318 - 0x315];
    /* 0x318 */ u32 unk318;
    /* 0x31C */ s16 unk31C;
    /* 0x31E */ s16 unk31E;
    /* 0x320 */ s16 unk320;
    /* 0x322 */ u8 unk322[16];
    /* 0x332 */ u8 unk332[0x378 - 0x332];
    /* 0x378 */ s16 unk378;
    /* 0x37A */ s16 unk37A;
    /* 0x37C */ s16 unk37C;
    /* 0x37E */ u8 unk37E[0x466 - 0x37E];
    /* 0x466 */ s16 unk466;
};

/* The actors this unit calls into; the mangling of the source names reproduces the map's spellings. */
s32 Pl_master_ck(_PLW*);
u32 Pl_Skill_ck(_PLW*, u16);
u32 Pl_cat_skill_ck(_PLW*, u16);

extern "C" u32 fn_8027681C(_PLW*);
extern "C" void fn_80276868(_PLW*, s16);
extern "C" void fn_80276CE8(_PLW*, s32);
extern "C" u8 fn_802B0598(u8);
extern "C" s32 fn_80331104(void);

extern const f32 lbl_8079A088;
extern const f32 lbl_8079A08C;
extern const f32 lbl_8079A090;
extern const f32 lbl_8079A094;
extern const f32 lbl_8079A098;
extern const f32 lbl_8079A09C;
extern const f64 lbl_8079A0A0;

extern u32 lbl_805BF448[];
extern u32 lbl_805BF46C[];
extern u32 lbl_805E2248[];
extern u32 lbl_805E25D0[];

/* 0x80276B58: the attack-scale multiplier the actor's active skills grant for a signed modifier. */
extern "C" void fn_80276B58(_PLW* self, s32 arg1)
{
    if (Pl_master_ck(self) != 0) {
        f32 f = (f32)(s16)arg1;
        if ((s16)arg1 < 0) {
            if (Pl_Skill_ck(self, 0xB9) == 1) {
                if (Pl_cat_skill_ck(self, 4) == 1) {
                    f *= lbl_8079A088;
                } else {
                    f *= lbl_8079A08C;
                }
            } else if (Pl_Skill_ck(self, 0xBA) == 1) {
                f *= lbl_8079A088;
            } else if (Pl_Skill_ck(self, 0xBB) == 1) {
                if (Pl_cat_skill_ck(self, 4) == 1) {
                    f *= lbl_8079A090;
                } else {
                    f *= lbl_8079A094;
                }
            } else if (Pl_Skill_ck(self, 0xBC) == 1) {
                if (Pl_cat_skill_ck(self, 4) == 1) {
                    f *= lbl_8079A098;
                } else {
                    f *= lbl_8079A09C;
                }
            } else if (Pl_cat_skill_ck(self, 4) == 1) {
                f *= lbl_8079A08C;
            }
        }
        fn_80276868(self, (s16)f);
    }
}

/* 0x80276CE8: adds a signed amount to the actor's stamina pool and clamps it, with a lower bound reset. */
extern "C" void fn_80276CE8(_PLW* self, s32 arg1)
{
    if (Pl_master_ck(self) == 0) {
        return;
    }
    if ((s16)arg1 < 0 && fn_8027681C(self) == 1) {
        return;
    }
    {
        s16 v = self->unk37A + (s16)arg1;
        self->unk37A = v;
        if (v <= 0x96) {
            self->unk37A = 0x96;
        } else if (v > 0x384) {
            self->unk37A = 0x384;
            self->unk37C = 0x2A30;
        }
        v = self->unk37A;
        if (self->unk378 > v) {
            self->unk378 = v;
        }
    }
}

/* 0x80276D94: applies the stamina bonus the two armour skills grant, then the signed amount. */
extern "C" void fn_80276D94(_PLW* self, s32 arg1)
{
    if ((s16)arg1 > 0 && (Pl_Skill_ck(self, 0x48) == 1 || Pl_Skill_ck(self, 0x49) == 1)) {
        arg1 += 150;
    }
    fn_80276CE8(self, (s16)arg1);
}

/* 0x80276E08: recomputes the actor's attack-range/level modifier from its weapon class and skills. */
extern "C" void fn_80276E08(_PLW* self)
{
    if (Pl_master_ck(self) != 0) {
        s16 cls = fn_802B0598(self->unk016);
        s32 v = 0;
        if (cls == 2 || cls == 4) {
            if (self->unk466 == 0 && (cls != 2 || (Pl_Skill_ck(self, 0x80) != 1 && Pl_Skill_ck(self, 0x81) != 1))
                && (cls != 4 || Pl_Skill_ck(self, 0x81) != 1)) {
                if (cls == 2) {
                    if (Pl_Skill_ck(self, 0x82) == 1) {
                        v = 3;
                    } else if (Pl_Skill_ck(self, 0x83) == 1) {
                        v = 4;
                    } else {
                        v = 2;
                    }
                } else if (Pl_Skill_ck(self, 0x80) == 1) {
                    v = 1;
                } else if (Pl_Skill_ck(self, 0x82) == 1) {
                    v = 4;
                } else if (Pl_Skill_ck(self, 0x83) == 1) {
                    v = 6;
                } else {
                    v = 3;
                }
            }
        } else {
            v = 0;
        }
        if (Pl_Skill_ck(self, 0x45) == 0) {
            if (self->unk009 == 3) {
                if ((self->unk020 & 1) == 0 && (Pl_Skill_ck(self, 0x44) != 1 || (self->unk020 & 3) != 0)) {
                    v += 1;
                    if (Pl_Skill_ck(self, 0x47) == 1) {
                        v += 1;
                    } else if (Pl_Skill_ck(self, 0x46) == 1 && (self->unk020 & 3) == 0) {
                        v += 1;
                    }
                }
            } else if (Pl_Skill_ck(self, 0x44) != 1 || (self->unk020 & 1) != 0) {
                v += 1;
                if (Pl_Skill_ck(self, 0x47) == 1) {
                    v += 1;
                } else if (Pl_Skill_ck(self, 0x46) == 1 && (self->unk020 & 1) == 0) {
                    v += 1;
                }
            }
        }
        if ((s16)v > 0) {
            s32 t = self->unk37C - v;
            self->unk37C = t;
            if ((s16)t <= 0) {
                if (fn_8027681C(self) == 1) {
                    self->unk37C = 1;
                    return;
                }
                self->unk37C = 0x2A30;
                fn_80276CE8(self, -0x96);
            }
        }
    }
}

/* 0x802770E0 */
extern "C" s32 fn_802770E0(void)
{
    return 0;
}

/* 0x802770E8: initialises the actor's attack-range state - the id/count words, the zeroed tail fields and
 * the 16-byte per-range table. */
extern "C" void fn_802770E8(_PLW* self, u32 table, s32 arg2)
{
    self->unk318 = table;
    self->unk313 = (s8)arg2;
    self->unk314 = 0;
    self->unk31C = 0;
    self->unk320 = 0;
    self->unk31E = 0;
    for (s16 i = 0; i < 48; i++) {
        self->unk322[i] = 0;
    }
}

/* 0x802771A0: picks the attack-range table for the actor's weapon class and initialises it. */
extern "C" void fn_802771A0(_PLW* self, s32 arg1)
{
    if (self->unk009 != 3) {
        if (self->unk002 == 8 && fn_80331104() == 0) {
            fn_802770E8(self, (u32)lbl_805E2248, (s16)arg1);
        } else {
            fn_802770E8(self, lbl_805BF448[self->unk002], (s16)arg1);
        }
    } else {
        if (self->unk002 == 8 && fn_80331104() == 0) {
            fn_802770E8(self, (u32)lbl_805E25D0, (s16)arg1);
        } else {
            fn_802770E8(self, lbl_805BF46C[self->unk002], (s16)arg1);
        }
    }
}
