/* Not-yet-reconstructed `enemy`-band symbols: addresses whose bracketing registered units both name the `enemy` module, so the declaration belongs here until the owning unit is registered.
 *
 * Declarations moved here from the consumer units' `src/` files (docs/plan.md 6.5 rule 2:
 * an extern lives with the TU that owns the symbol).  The signature set is what the
 * consumers used; where only the parameter spelling differed the wider form is kept.
 */
#ifndef MHTRI_UNSPLIT_ENEMY_H
#define MHTRI_UNSPLIT_ENEMY_H

#include "types.h"
#include "nw4r/math.h"
/* The owner header for the 0x8012E968 predicates (`enemy/fn_8012E968.cpp`) owns `fn_8012EC60`'s
 * argument-accepting spelling; the band re-exports it so the existing consumers that include this
 * header (rule 2: a declaration belongs with its owner's TU). */
#include "enemy/fn_8012E968.h"
/* The same for the enemy range `enemy/fn_801502C8.cpp` owns (0x801502C8-0x801545B8):
 * `fn_80154784`/`fn_80154CA4`/`fn_801545B8` were declared in this band header, and this re-exports
 * the owner's header so the consumers that include this one keep them (rule 2). */
#include "enemy/fn_801502C8.h"

struct _ENEMY_WORK;
struct EnemyData;

#ifdef __cplusplus
extern "C" {
#endif

struct EnemyData* fn_80140C00(u8 group, u8 index);
u8* fn_8014260C(u8 id);

void CancelFade(struct _ENEMY_WORK *self);
u32 em_frame_check__FP11_ENEMY_WORKUsff(struct _ENEMY_WORK *self, u16 a, f32 b, f32 c);
u32 em_sleep_ck__FP11_ENEMY_WORKUc(struct _ENEMY_WORK* enemy, u8 kind);
u32 fn_8012ECF0(void);
void fn_8012F5B8(struct _ENEMY_WORK* self, s32 a, s32 b, s32 c);
/* 0x8012F504 - the five-argument motion setter `fn_8012F5B8` tail-calls; moved here from
 * `enemy/fn_801550FC.cpp` on landing (rule 2).  `fn_8012F5B8` narrows its second argument to u16
 * (`clrlwi r4,r4,16`) before the tail call, so the owner's first argument is u16. */
void fn_8012F504(struct _ENEMY_WORK* self, u32 a, u32 b, u32 c, u32 d);
void fn_8012F62C(struct _ENEMY_WORK *self, u32 a, u32 b, u32 c);
/* 0x8012F7D4 - reads r3, r4, r5, r6 and f1 (its body does `mr r4,r5` / `mr r5,r6` before the tail
 * call to 0x8012F758), so the four scalar arguments and the float are in the call sites' order. */
void fn_8012F7D4(struct _ENEMY_WORK* self, u32 a, u32 b, u32 c, f32 d);
/* 0x8012F810 - one `self` argument, no return (restores the motion state). */
void fn_8012F810(struct _ENEMY_WORK* self);
/* 0x8012F860 - reads f1 and f2 (`fcmpo cr0,f1,f0` then `fadds f0,f0,f2`), so it takes two floats;
 * `enemy/fn_80156EA4` sets f1 (the computed ratio) and f2 (`lbl_80797150`) before the call. */
void fn_8012F860(struct _ENEMY_WORK* self, f32 a, f32 b);
/* 0x8012F8C8 - two arguments: r3 (`self`) and f1, which it stores at +0x7c4 and multiplies by the
 * float at +0x7bc before tail-calling 0x800E1640.  The 0-argument form this band used to carry was
 * wrong: `enemy/fn_8014A1BC.c` calls it with `self` only and `enemy/fn_801550FC.cpp` with a float,
 * so C keeps the old-style declaration and C++ gets the real one. */
#ifdef __cplusplus
void fn_8012F8C8(struct _ENEMY_WORK* self, f32 a);
#else
void fn_8012F8C8();
#endif
f32 fn_8012F8E4(struct _ENEMY_WORK *self);
f32 fn_8012F8EC(struct _ENEMY_WORK *self);
f32 fn_8012F8F4(struct _ENEMY_WORK *self);
u32 fn_8012F93C(struct _ENEMY_WORK *self);
u32 fn_8012F948(struct _ENEMY_WORK *self);
void fn_8012FC60(struct _ENEMY_WORK* work);
void fn_8012FCC4(struct _ENEMY_WORK* work, s32 arg1, f32 arg2);
void fn_8012FCE4(struct _ENEMY_WORK* work);
void fn_8012FF38(struct _ENEMY_WORK* work);
/* 0x80130008 - r3 (`self`) and f1 (it does `fmr f31,f1` and uses it against `get_em_scale`), return
 * in r3 (1/0).  C keeps the old-style declaration because `enemy/fn_8014A1BC.c` calls it both with
 * one and with two arguments; C++ gets the one-argument form `enemy/fn_801550FC.cpp` uses (it leaves
 * f1 as the tail of the preceding `fn_80130248` call, exactly as the target does). */
#ifdef __cplusplus
u32 fn_80130008(struct _ENEMY_WORK* self);
#else
u32 fn_80130008();
#endif
u8 fn_8013023C(struct _ENEMY_WORK* work);
f32 fn_80130248(struct _ENEMY_WORK* self);
f32 fn_801302E4(struct _ENEMY_WORK* work);
/* 0x801303EC - r3 (`self`) and f1 (stored at +0x1ac); 0x801303FC tail-calls it with `f0 + f1`.
 * `enemy/fn_8014A1BC.c` calls both with two arguments, so a real two-argument prototype is safe for
 * C too. */
void fn_801303EC(struct _ENEMY_WORK* self, f32 a);
/* 0x801303FC - r3 (`self`) plus the f1 `0x801303EC` consumes (its body does `lfs f0,0x1ac(r3);
 * fadds f1,f0,f1; b 0x801303EC`).  The callee READS f1, so the real signature is two-argument
 * (docs/plan.md 6.5 rule 6: settled from the callee's body).  `enemy/fn_8014A1BC.c` also calls it
 * with one argument (leaving f1 as the tail of its preceding call), so C keeps the old-style
 * declaration. */
#ifdef __cplusplus
void fn_801303FC(struct _ENEMY_WORK* self, f32 a);
#else
void fn_801303FC();
#endif
void fn_80130438(struct _ENEMY_WORK* work);
void fn_80130478(struct _ENEMY_WORK *self, u32 a);
void fn_801305C4(struct _ENEMY_WORK *self);
u32 fn_80130778(s32 kind);
void fn_80130858(struct _ENEMY_WORK* enemy, s16 value);
void fn_80130A10(struct _ENEMY_WORK* enemy, s32 value);
/* 0x80130CDC - r3 (`self`) and r4, which it sign-extends (`extsh r4,r4`) before tail-calling
 * 0x80130B6C; `enemy/fn_8014A1BC.c` calls it with two arguments, so the prototype is safe for C. */
void fn_80130CDC(struct _ENEMY_WORK* self, u32 a);
u32 fn_80130DF8(void);
struct _ENEMY_WORK* fn_80131034(struct _ENEMY_WORK* self, u8 kind, u8 distance_check);
void fn_80131150(struct _ENEMY_WORK* work);
void fn_80131D9C(struct _ENEMY_WORK* work);
void fn_80131DB4(struct _ENEMY_WORK* work);
void fn_80131DF4(struct _ENEMY_WORK* work);
void fn_80131E0C(struct _ENEMY_WORK* work);
void fn_80131E74(struct _ENEMY_WORK* work);
void fn_80131E00(struct _ENEMY_WORK* work);
f32 fn_8013032C(struct _ENEMY_WORK* work);
u32 fn_80132198(struct _ENEMY_WORK* work);
void fn_801321B0(struct _ENEMY_WORK* work);
void fn_80131EC0();
void fn_801320A4(struct _ENEMY_WORK* work);
u32 fn_80132184(void);
u32 fn_801322CC(struct _ENEMY_WORK* enemy, s32 value);
void fn_801324E0(struct _ENEMY_WORK* work);
void fn_801333E0(struct _ENEMY_WORK* work);
void fn_80133B5C(struct _ENEMY_WORK* work);
void fn_80133BB4(struct _ENEMY_WORK* enemy);
void fn_80133BC0(struct _ENEMY_WORK* work);
void fn_80133C30(struct _ENEMY_WORK* work);
void fn_80133C3C(struct _ENEMY_WORK *self);
u32 fn_80133C50(struct _ENEMY_WORK *self, u32 a);
void fn_80133CC8(struct _ENEMY_WORK *self, u32 a, u32 b);
void fn_80133E3C(struct _ENEMY_WORK *self, s32 a, f32 b, f32 c);
/* 0x80133F4C - r3 (`self`), f1, f2 and r4 (the callee's body does `fmr f30,f1` / `fmr f31,f2` /
 * `mr r31,r4`), so the real signature is four-argument; `enemy/fn_8014BDF8` (C) leaves r4 as the
 * tail of its preceding call, so C keeps the old-style declaration (docs/plan.md 6.5 rule 6). */
#ifdef __cplusplus
void fn_80133F4C(struct _ENEMY_WORK* self, f32 a, f32 b, s32 c);
#else
void fn_80133F4C();
#endif
u32 fn_80134114(struct _ENEMY_WORK* self, s32 a, s32 b);
void fn_80134964(struct _ENEMY_WORK* self, void* tbl, s32 a, s32 b, s32 c);
u32 fn_80134B0C(struct _ENEMY_WORK *self, void *tbl);
void fn_80134DF4(struct _ENEMY_WORK *self);
void fn_80134E28(struct _ENEMY_WORK *self);
void fn_80134E8C(struct _ENEMY_WORK *self);
void fn_80134F18(struct _ENEMY_WORK *self);
void fn_80134F70(struct _ENEMY_WORK* self, void* tbl);
void fn_80135000(struct _ENEMY_WORK* self, u32 a, void* tbl);
void fn_801353E4(struct _ENEMY_WORK *self);
void fn_801353F8(struct _ENEMY_WORK *self);
void fn_80135418(struct _ENEMY_WORK *self);
void fn_801354F4(struct _ENEMY_WORK *self, void *p);
void fn_80135584(struct _ENEMY_WORK* self, void* p);
/* 0x801355C8 - r3 (`self`) and r4, the pointer it forwards to `fn_801354F4` unchanged; the C
 * callers pass `&self->field_0x1bc`, so the owner's argument is a pointer, not the integer the
 * consumer used to spell. */
void fn_801355C8(struct _ENEMY_WORK* self, void* p);
/* 0x80135600 - r3 (`self`), the r4 pointer it forwards to `fn_801355C8`, and the f1 it holds for
 * `UpdateValue`; `enemy/fn_8014A1BC.c` calls it with two and with three arguments, so C keeps the
 * old-style declaration and C++ gets the two-argument form the call site uses. */
#ifdef __cplusplus
void fn_80135600(struct _ENEMY_WORK* self, void* p);
#else
u32 fn_80135600();
#endif
f32 fn_80135644(struct _ENEMY_WORK *self, void *tbl);
f32 fn_801356A8(struct _ENEMY_WORK *self, f32 a, f32 b, f32 c);
/* 0x80135748 - the part-mask probe: r3 (`self`) and r4, which it narrows to u16 (`clrlwi r4,r4,16`)
 * before ANDing it against the record's `flags_0x836`; the body's `neg`/`or`/`srwi 31` returns 1
 * when any masked bit is set, so the result is a u32 0/1 and every call site compares it with
 * `cmplwi`.  The old-style `s32 fn_80135748()` declaration could not carry the two arguments the
 * landed callers pass (`enemy/fn_8014A1BC.c` and `enemy/fn_801DB8E0.cpp` both call it
 * `(self, mask)`). */
u32 fn_80135748(struct _ENEMY_WORK* self, u32 a);
u32 fn_80135BC4(struct _ENEMY_WORK* work, s32 arg1);
void fn_801363F8(struct _ENEMY_WORK* work);
/* Declarations for the action band 0x80178378.. (`enemy/fn_80178378.cpp`): the arming helpers its 64
 * action functions call and no registered unit owns.  Signatures are the call sites' - the argument
 * counts are what the callers set in r4..r6/f1..f3 and the functions are in this band. */
void fn_801823A0(struct _ENEMY_WORK* self, u32 a);
/* fn_8012F948(self) is declared above with the rest of the 0x8012F band - the sibling landing
 * `enemy/fn_8015E854.cpp` had added it there first, so this branch's copy was dropped by hand
 * instead of leaving two identical prototypes. */
u32 fn_80182430(struct _ENEMY_WORK* self, u32 a);
void fn_802B1FEC(void);
void fn_80136D14(struct _ENEMY_WORK* work);
void fn_80136E38(struct _ENEMY_WORK* work, s32 arg1);
void fn_801373D0(struct _ENEMY_WORK* work);
s32 fn_80137C9C(struct _ENEMY_WORK* work, void* arg1);
void fn_80137DD0(struct _ENEMY_WORK* work);
s32 fn_80137EE0(struct _ENEMY_WORK* work, s32 arg1);
void fn_80138024(struct _ENEMY_WORK* work, s32 a, s32 b);
void fn_8013BDE4(u8 **in, u32 id, s16 *out);
u32 fn_801406E0(u32 id, u32 cmd);
s16 fn_80140778(u8 *stream, u32 id, u32 mode);
s32 fn_801408B4(struct _ENEMY_WORK* work);
void fn_801409C8(struct _ENEMY_WORK *self, u8 *in, u32 id, u32 sub, s32 value);
void fn_80140AF8(struct _ENEMY_WORK *self, u32 a, u32 b);
void fn_80140B10(struct _ENEMY_WORK *self, u32 a, u32 b);
void fn_80143190(void);
void fn_80144584(s32 kind);
f32 get_em_chg_scale__FP11_ENEMY_WORK(struct _ENEMY_WORK* work);
f32 get_em_scale__FP11_ENEMY_WORK(struct _ENEMY_WORK* work);
void get_joint_wpos_em__FP11_ENEMY_WORKUlPQ34nw4r4math4VEC3(struct _ENEMY_WORK* work, u32 joint, Vec3* out);

/* The enemy action band 0x80127F48.. and the handler band 0x80170A54..0x80170EF4, owned by the
 * not-yet-registered proposals `proposal/8016xxxx`/`proposal/8017xxxx`.  Added by the
 * `enemy/fn_80170FA8.cpp` registration: its dispatcher tail-calls the 0x80170xxx handlers and the
 * state machines call `fn_80127F48`/`fn_80128A14`.  The band brackets as `enemy` on both sides
 * (fn_8014A1BC .. fn_80170FA8), so rule 2 sends the declarations here.  Signatures: `self` only for
 * the handlers that take one argument, and `fn_80170EF4` takes the action's extra selector in r4.
 *
 * `fn_80128A14`'s last two are u8 in the consumer's view, but the owner's body narrows them itself
 * (`clrlwi r4,r4,24`/`clrlwi r5,r5,24`), so the definition takes u32 and the declaration here is
 * widened to match the owner (enemy/fn_801251D0.cpp).  Three landed units used to declare them three
 * ways in their own files (`u8` in
 * fn_80149D6C, `u32` in fn_8014A1BC, `s32` in fn_80177890) - the same rule-2 debt.  Every
 * call site passes a constant, so the spelling is codegen-neutral. */
void fn_80127F48(struct _ENEMY_WORK* self);
void fn_80128A14(struct _ENEMY_WORK* self, u32 a, u32 b);
void fn_80170A54(struct _ENEMY_WORK* self);
void fn_80170AD0(struct _ENEMY_WORK* self);
void fn_80170B4C(struct _ENEMY_WORK* self);
void fn_80170C68(struct _ENEMY_WORK* self);
void fn_80170D04(struct _ENEMY_WORK* self);
void fn_80170D74(struct _ENEMY_WORK* self);
void fn_80170DF0(struct _ENEMY_WORK* self);
void fn_80170E78(struct _ENEMY_WORK* self);
void fn_80170EF4(struct _ENEMY_WORK* self, u32 a);
/* Declarations moved here from `enemy/fn_80176C58.cpp` (docs/plan.md 6.5 rule 2). */
void fn_8012F5C4(struct _ENEMY_WORK* self, u32 a, u32 b, u32 c, u32 d);
/* `fn_80154784` (0x80154784) was declared here (from `enemy/fn_801B0010.cpp`'s call site) as
 * `u32 (struct _ENEMY_WORK*)` while the band had no registered unit.  `enemy/fn_801502C8.cpp` owns
 * 0x801502C8-0x801545B8 now, and its header `include/enemy/fn_801502C8.h` declares the symbol with
 * that same unsigned spelling (the call site reads the answer unsigned) - this band header includes
 * it (rule 2). */
u32 fn_8012EC3C(struct _ENEMY_WORK* self);
void fn_80130F74(struct _ENEMY_WORK* self);
void fn_801376B4(struct _ENEMY_WORK* self);
void fn_80135C5C(struct _ENEMY_WORK* self, u32 a, u32 b);
/* Declarations moved here from `enemy/fn_801679B0.cpp` (docs/plan.md 6.5 rule 2): enemy-band
 * symbols no registered unit owns.  Guarded for C++ because the C consumers carry their own
 * ABI-equivalent spellings of `fn_80134004` (`f32,u16` in `enemy/fn_80149D6C.c`, `u32,f32` in
 * `enemy/fn_8014A1BC.c`), which MWCC's C front-end treats as a conflicting redeclaration. */
#ifdef __cplusplus
void fn_80131D84(struct _ENEMY_WORK* self);
void fn_80134004(struct _ENEMY_WORK* self, u32 a, f32 b);
void fn_80167404(struct _ENEMY_WORK* self);
void fn_80167968(struct _ENEMY_WORK* self);
void fn_801321C4(struct _ENEMY_WORK* self);
void fn_801321D0(struct _ENEMY_WORK* self);
#endif
void fn_8013221C(struct _ENEMY_WORK* self, f32 a, u32 b, u32 c);
void fn_80132224(struct _ENEMY_WORK* self);
void fn_80132264(struct _ENEMY_WORK* self);
/* `fn_80154CA4`/`fn_801545B8` (0x80154CA4/0x801545B8) were declared here (moved from
 * `enemy/fn_80147CE0.cpp`'s call sites) while the band 0x801545B8..0x80154CA4 had no registered
 * unit.  `enemy/fn_801502C8.cpp` owns 0x801502C8-0x801545B8 now, so `include/enemy/fn_801502C8.h`
 * (included above) declares both: `fn_80154CA4` takes the record and `fn_801545B8` the 0x18-byte
 * spawn record; the spellings here were the call sites' `void*`/`u32` view (rule 2). */
/* The tenth argument is a POINTER, settled from the callee's own body (`auto_fn_80141B88_text.s`):
 * it loads the outgoing stack word into r21 and hands it to `fn_80041E40` as the second argument
 * when it is non-null, and the two callers pass a `VEC3*` (`enemy/fn_8015D860.cpp`'s `fn_8015DB68`)
 * or null (`enemy/fn_80170600.cpp`'s `fn_801706B8`). */
void fn_80141B88(u16 a, s32 b, s32 c, u8 d, u8 e, s32 f, s32 g, s32 h, s32 i, void* j, s32 k);
/* The unclaimed `.text` run 0x801926EC..0x801993E0 (a proposal of its own, registered by nobody
 * yet): its per-action entry points are what the dispatchers in `enemy/fn_801993E0.cpp` switch
 * over.  Added with that unit's registration (docs/plan.md 6.5 rule 2): the address band brackets as
 * `enemy` on both sides (`enemy/fn_80191598.cpp` below, `enemy/fn_801993E0.cpp` above), so the
 * declarations belong in this band header until the run's own unit claims them.  Every one of them
 * takes the `_ENEMY_WORK` record and returns nothing - they are called as `fn(self); break;` from a
 * `void` dispatcher and the target tail-calls the last ones. */
void fn_80198F28(struct _ENEMY_WORK* self);
void fn_80198FE8(struct _ENEMY_WORK* self);
void fn_801990E0(struct _ENEMY_WORK* self);
void fn_801991E4(struct _ENEMY_WORK* self);
void fn_80192F24(struct _ENEMY_WORK* self);
void fn_80193394(struct _ENEMY_WORK* self);
void fn_801938D8(struct _ENEMY_WORK* self);
void fn_801953BC(struct _ENEMY_WORK* self);
void fn_80196618(struct _ENEMY_WORK* self);
void fn_801987E4(struct _ENEMY_WORK* self);
void fn_80198910(struct _ENEMY_WORK* self);
void fn_80198E00(struct _ENEMY_WORK* self);
void fn_80198F14(struct _ENEMY_WORK* self);
/* 0x801B701C - unowned (it is the last 4-byte word of the still-unregistered 0x801926EC..0x801B7020
 * hole; both bracketing registered units are `enemy`).  Its body is a bare `blr`, so it returns its
 * own r3 argument, and every caller compares that return with 0 - `src/ef/eft001.cpp` declares it
 * `void`, which is wrong for those call sites (recorded in this unit's outbox).  The `u32` return
 * below is the form `enemy/fn_801B7020.cpp`'s `fn_801B73F0` needs. */
u32 fn_801B701C(struct _ENEMY_WORK* self);
/* The enemy effect-slot cluster below `fn_801A4504`'s range (0x801A3xxx-0x801A9xxx): unowned (the
 * bracketing registered units are `lobby` above), so the declarations live in this band header.
 * `enemy/fn_801A4504.cpp` calls all of them; the signatures are its call sites' registers.  The
 * 0x8019E9xx members are NOT here - they are inside `enemy/fn_801993E0.cpp`'s range, so its owner
 * header carries them. */
void fn_801A3E90(struct _ENEMY_WORK* self);
void fn_801A3FD8(struct _ENEMY_WORK* self);
void fn_801A4218(struct _ENEMY_WORK* self);
/* r3 the work record, r4 a joint id, r5/r6 two `VEC3*` (the target's call sites set all three). */
void fn_801A42F4(struct _ENEMY_WORK* self, u32 joint, Vec3* a, Vec3* b);
/* r3 the work record, r4/r5/r6/r7 four scalars and f1 (the target's call sites set all five). */
void fn_801A437C(struct _ENEMY_WORK* self, u32 a, u32 b, u32 c, u32 d, f32 e);
/* Added with `enemy/fn_801A9540.cpp`'s registration (rule 2): the enemy-band callees that
 * range's state machines call and no registered unit owns.  Their bracketing registered units
 * both name `enemy`, so this band header is their home. */
void fn_801A9748(struct _ENEMY_WORK* self);
void fn_801A98F8(struct _ENEMY_WORK* self);
/* The enemy action-table head above `enemy/fn_80387844.cpp`'s range (0x803874C8..0x80387844): the
 * first four `state_sub` handlers `fn_80388144` dispatches to.  Unowned (the bracketing registered
 * units are `enemy` on both sides), so the band header is their rule-2 home.  Each drives the shared
 * `_ENEMY_WORK`; `fn_803874C8` takes the work record, the other three take none (they forward r3). */
/* 0x80346268 - the two-argument request `enemy/em_action.cpp`'s entry action's case 11 makes when
 * the enemy data entry is latched.  `ef/eft_slot.cpp` registered the band that owns the address, so
 * the declaration moved to that unit's header, `include/ef/eft_slot.h` (rule 2). */
void fn_803874C8(struct _ENEMY_WORK* self);
void fn_80387528(void);
void fn_803875A4(void);
void fn_80387620(void);

/* The `0x803253BC` band's pooled constants - the `.sdata2` run 0x8079AF4C..0x8079B100 and the
 * `.data` word its action record's first field is set to.  Declared, never defined (playbook 29):
 * the target object owns no data section at all, so the original TU referenced pool entries that
 * live in the unclaimed data bands, and a definition here would emit a second copy.  Their values
 * are the DOL's own (0.0f at 0x8079AF54, 1300.0f at 0x8079AF4C, ...). */
extern const f32 lbl_8079AF4C;   /* 1300f */
extern const f32 lbl_8079AF50;   /* 700f */
extern const f32 lbl_8079AF54;   /* 0f */
extern const f32 lbl_8079AF58;   /* 5f */
extern const f32 lbl_8079AF5C;   /* 50f */
extern const f32 lbl_8079AF64;
extern const f32 lbl_8079AF68;   /* 40f */
extern const f32 lbl_8079AF6C;   /* -3f */
extern const f32 lbl_8079AF70;   /* 2f */
extern const f32 lbl_8079AF74;   /* 45f */
extern const f32 lbl_8079AF78;   /* 0.7f */
extern const f32 lbl_8079AF7C;   /* 1.5f */
extern const f32 lbl_8079AF80;   /* 150f */
extern const f32 lbl_8079AF84;   /* 0.9f */
extern const f32 lbl_8079AF88;   /* 0.5f */
extern const f32 lbl_8079AF8C;   /* 65536f */
extern const f32 lbl_8079AF90;   /* 360f */
extern const f32 lbl_8079AF94;   /* 90f */
extern const f32 lbl_8079AF98;   /* -6f */
extern const f32 lbl_8079AF9C;
extern const f32 lbl_8079AFA0;
extern const f32 lbl_8079AFA4;
extern const f32 lbl_8079AFA8;
extern const f32 lbl_8079AFAC;
extern const f32 lbl_8079AFB0;
extern const f32 lbl_8079AFB4;
extern const f32 lbl_8079AFB8;
extern const f32 lbl_8079AFBC;
extern const f32 lbl_8079AFC0;
extern const f32 lbl_8079AFC4;
extern const f32 lbl_8079AFC8;
extern const f32 lbl_8079AFCC;
extern const f32 lbl_8079AFD0;
extern const f32 lbl_8079AFD4;
extern const f32 lbl_8079AFD8;
extern const f32 lbl_8079AFDC;
extern const f32 lbl_8079AFE0;
extern const f32 lbl_8079AFE4;
extern const f32 lbl_8079AFE8;
extern const f32 lbl_8079AFEC;
extern const f32 lbl_8079AFF0;
extern const f32 lbl_8079AFF4;
extern const f32 lbl_8079AFF8;
extern const f32 lbl_8079AFFC;
extern const f32 lbl_8079B000;
extern const f32 lbl_8079B004;
extern const f32 lbl_8079B008;
extern const f32 lbl_8079B00C;
extern const f32 lbl_8079B010;
extern const f32 lbl_8079B014;
extern const f32 lbl_8079B018;
extern const f32 lbl_8079B01C;
extern const f32 lbl_8079B020;
extern const f32 lbl_8079B024;
extern const f32 lbl_8079B028;
extern const f32 lbl_8079B02C;
extern const f32 lbl_8079B030;
extern const f32 lbl_8079B034;
extern const f32 lbl_8079B038;
extern const f32 lbl_8079B03C;
extern const f32 lbl_8079B040;
extern const f32 lbl_8079B044;
extern const f32 lbl_8079B048;
extern const f32 lbl_8079B04C;
extern const f32 lbl_8079B050;
extern const f32 lbl_8079B054;
extern const f32 lbl_8079B058;
extern const f32 lbl_8079B05C;
extern const f32 lbl_8079B060;
extern const f32 lbl_8079B064;
extern const f32 lbl_8079B068;
extern const f32 lbl_8079B06C;
extern const f32 lbl_8079B070;
extern const f32 lbl_8079B074;
extern const f32 lbl_8079B078;
extern const f32 lbl_8079B07C;
extern const f32 lbl_8079B080;
extern const f32 lbl_8079B084;
extern const f32 lbl_8079B088;
extern const f32 lbl_8079B08C;
extern const f32 lbl_8079B090;
extern const f32 lbl_8079B094;
extern const f32 lbl_8079B098;
extern const f32 lbl_8079B09C;
extern const f32 lbl_8079B0A0;
extern const f32 lbl_8079B0A4;
extern const f32 lbl_8079B0A8;
extern const f32 lbl_8079B0AC;
extern const f32 lbl_8079B0B0;
extern const f32 lbl_8079B0B4;
extern const f32 lbl_8079B0B8;
extern const f32 lbl_8079B0BC;
extern const f32 lbl_8079B0C0;
extern const f32 lbl_8079B0C4;
extern const f32 lbl_8079B0C8;
extern const f32 lbl_8079B0CC;
extern const f32 lbl_8079B0D0;
extern const f32 lbl_8079B0D4;
extern const f32 lbl_8079B0D8;
extern const f32 lbl_8079B0DC;
extern const f32 lbl_8079B0E0;
extern const f32 lbl_8079B0E4;
extern const f32 lbl_8079B0E8;
extern const f32 lbl_8079B0EC;
extern const f32 lbl_8079B0F0;
extern const f32 lbl_8079B0F4;
extern const f32 lbl_8079B0F8;
extern const f32 lbl_8079B0FC;
extern const f32 lbl_8079B100;
extern const f32 lbl_8079AF60;   /* the low word of the 0.0 double at 0x8079AF60 */

/* The word the 0xC-byte action record's first field is set to (the shared vtable of that
 * record's class; the table itself is outside this unit's ranges - rule 10). */
extern u32 lbl_805DFC00[];

/* Added with `enemy/em_act_step.cpp`'s registration (rule 2): the `.sdata2` pool half
 * 0x8079B108..0x8079B210 that unit's 74 functions load their constants from, and the class vtable
 * `lbl_805E04E0` its constructor installs.  No registered unit owns either (the pool's bracketing
 * units are the same unclaimed band), so this header is their home.  The pool labels are used as load
 * operands and never defined (playbook 29: defining one would rebuild the section and move every later
 * constant); the comment after each is the value the DOL holds at that address.
 *
 * This is the FIRST of the two halves of the proposal's range, and the pool run splits with it: the
 * next word 0x8079B210 begins `enemy/em_pl_frame.cpp`'s pool half 0x8079B210..0x8079B2AC, so that
 * half's labels (0x8079B210, 0x8079B214, 0x8079B218, 0x8079B21C, 0x8079B220, 0x8079B228's magic,
 * 0x8079B230, 0x8079B238, 0x8079B23C, 0x8079B240, 0x8079B244, 0x8079B258, 0x8079B294, 0x8079B2A0)
 * are NOT declared here - they belong to that unit and are listed in its file header.  The two halves
 * are two objects, proved by the compiler's own pool: `0x4330000080000000` (the u32->f32 magic) is
 * emitted at 0x8079B140 for this unit and at 0x8079B228 for the other, and 0.0f / 0.5f / 1.0f /
 * 10.0f / 20.0f / 30.0f / 60.0f / 0.8f / -30.0f likewise occur once per half (two emitters - see
 * `enemy/em_pl_frame.cpp`).  MWCC's pool emits one copy per object, and our own object emits one
 * 8-byte magic for two functions that use it (the linker merges nothing: that magic occurs >= 40x in
 * the DOL).  0x8079B13C is not a symbol: it is the 4-byte alignment hole before the 8-aligned double
 * at 0x8079B140, and a declaration of it was deleted here (finding, seam redraw 2026-09-26). */
extern const f32 lbl_8079B108; /* 0.0 */
extern const f32 lbl_8079B10C; /* -35.0 */
extern const f32 lbl_8079B110; /* 10.0 */
extern const f32 lbl_8079B114; /* 500.0 */
extern const f32 lbl_8079B118; /* 0.25 */
extern const f32 lbl_8079B11C; /* 1.4 */
extern const f32 lbl_8079B120; /* 0.01 */
extern const f32 lbl_8079B124; /* -100.0 */
extern const f32 lbl_8079B128; /* 1000.0 */
extern const f32 lbl_8079B12C; /* 30.0 */
extern const f32 lbl_8079B130; /* 1.0 */
extern const f32 lbl_8079B134; /* 0.1 */
extern const f32 lbl_8079B138; /* -200.0 */
extern const f32 lbl_8079B148; /* 4.0 */
extern const f32 lbl_8079B14C; /* 0.2 */
extern const f32 lbl_8079B150; /* 800.0 */
extern const f32 lbl_8079B158; /* 700.0 */
extern const f32 lbl_8079B15C; /* 25.0 */
extern const f32 lbl_8079B160; /* 20.0 */
extern const f32 lbl_8079B164; /* -3.0 */
extern const f32 lbl_8079B168; /* 60.0 */
extern const f32 lbl_8079B16C; /* -15.0 */
extern const f32 lbl_8079B170; /* -25.0 */
extern const f32 lbl_8079B174; /* -10.0 */
extern const f32 lbl_8079B178; /* 0.8 */
extern const f32 lbl_8079B17C; /* -30.0 */
extern const f32 lbl_8079B180; /* 40.0 */
extern const f32 lbl_8079B184; /* 10000.0 */
extern const f32 lbl_8079B188; /* -20.0 */
extern const f32 lbl_8079B18C; /* -5.0 */
extern const f32 lbl_8079B1E4; /* 0.5 */
#ifdef __cplusplus
}
#endif

#ifdef __cplusplus
/* The C++ spellings of the mangled callees this band calls, so a call site never spells the
 * mangling (docs/plan.md 6.5 rule 9); each mangles back to its map name.  `get_joint_wpos_em`
 * itself is C++ in the target (`get_joint_wpos_em__FP11_ENEMY_WORKUlPQ34nw4r4math4VEC3`), so the
 * declaration moves here from the extern "C" block (relocaudit). */
u32 em_frame_check(struct _ENEMY_WORK* self, u16 a, f32 b, f32 c);
void get_joint_wpos_em(struct _ENEMY_WORK* enemy, u32 joint, Vec3* out);
#endif

#endif /* MHTRI_UNSPLIT_ENEMY_H */
