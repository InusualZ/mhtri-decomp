/*
 * ai/ai_npc.cpp - the AI companion (`_AINPC_W`, `ai/ainpc.h`): the light band's arm dispatcher, the aimed-action
 *   ladder, the per-motion sub-state machines, the work band, the state dispatch over `lbl_805D4150`, the
 *   motion/attack band and the state checks (`ai_skill_ck`, `ai_torch_ck`, `ai_taru_*_ck`, `ai_demo_stop_ck`).  C++;
 *   every `fn_` stem is `extern "C"` so objdiff pairs it by the map's name (playbook 42).
 * RANGE. .text 0x802C2700-0x802D9EA4 (357 functions); extab, extabindex, .ctors 0x8056F388 (`fn_802D9E14`, the
 *   `ainpc_w` constructor), .data 0x805D3A88-0x805D54A0, .bss 0x806BD360-0x806BD808 (`ainpc_w`), .sdata, .sbss,
 *   .sdata2.  `jumptable_805D5428` is the 14-entry table MWCC emits for `fn_802D77A0` (slots read out of `main.elf`).
 * FLAGS. `cflags_main` (configure.py).  `#pragma peephole off` over 0x802C2F08-0x802CC794 and from 0x802D6534 on, on
 *   for 0x802CCD90-0x802D44F4: retail keeps the unfused `clrlwi`+`cmpwi`, `clrlwi`+`slwi` and `extsh`+`cmpwi` pairs
 *   there (playbook 39; peephole on costs `fn_802D773C` 100 -> 73.3, `fn_802C5D10` and `fn_802C6578` 1.8 and 3.9 points).
 * NAMES. Module `ai` from the record every function takes (`ai_skill_ck__FP8_AINPC_WUc`, `ai_area_ck__FP8_AINPC_W`,
 *   `get_joint_wpos_ai__FP8_AINPC_WUlPQ34nw4r4math4VEC3`); no `__FILE__` string covers the range and the runtime dump
 *   answers `zz_` for the `fn_` rows.  `ainpc_entry_tbl`/`ainpc_page_state` (`hud/cockpit.cpp`'s `.bss`, typed here
 *   as `AinpcEntry`/`AinpcPageState`) are GUESSes from their users; `ai_npc_hold_item_arm`, `ai_npc_hold_ck` and
 *   `ai_npc_arrived_ck` are GUESSes from their bodies (the dump has only `zz_` names).
 * RESIDUALS. 153 rows unwritten (objdiff scores them zero): 0x802C2700-0x802C2F08, 0x802C2F2C-0x802C474C,
 *   0x802C6908-0x802C6BE0, 0x802C75F4-0x802CCD90, 0x802CCE18-0x802CD20C, 0x802CD348-0x802CD588,
 *   0x802CD770-0x802CDAB8, 0x802CDB10-0x802CDDC4, 0x802CE698-0x802CF390, 0x802CF588-0x802CF704,
 *   0x802CF808-0x802CF8E4, 0x802CF9E8-0x802CFABC, 0x802CFAF0-0x802CFBC8, 0x802D0C9C-0x802D0DCC,
 *   0x802D44F4-0x802D6534 (`fn_802D44F4`, a 2064-instruction compare tree on `ai_get_motion_no()`),
 *   0x802D6BB0-0x802D7464, 0x802D79A4-0x802D7A50, 0x802D7EB0-0x802D7F10, 0x802D7F58-0x802D84D0,
 *   0x802D84D8-0x802D948C, 0x802D94C4-0x802D9A40, 0x802D9A78-0x802D9EA4.  93 rows partial, including:
 *  - `fn_802D7B5C`, `fn_802D7C04` (zero), `fn_802D7C6C`: the search loop stays a loop in retail, MWCC unrolls the
 *    constant trip count of `for (i = 0; i < N; i++) if (x < table[i+1]) break;`;
 *  - `fn_802D7B24`: the range tests want the raw `subi` result in a 32-bit `cmplwi`; MWCC inserts `subfic`/`orc`;
 *  - `fn_802CDAB8`: retail's mixed-sign 64-bit compare idiom (`srawi`/`srwi`/`subfc`/`adde`) for
 *    `field_0x1F0 <= (s32)((f32)field_0x1F4 * formation->budget)`; no spelling tried reproduces it;
 *  - `fn_802CD588`: the `switch` on +0x420 emits a signed 4-instruction range test, retail a `subi`/`cmplwi`;
 *  - `fn_802D2984`/`fn_802D2990`/`fn_802D29A0`: retail schedules the model address (`addi r3,r3,8`) before the
 *    constant argument; every spelling keeps MWCC's constant-first order;
 *  - `fn_802D3F1C`: retail keeps a provably-true `li r0,0xFF; cmplwi r0,0xFF; beq` our build folds away;
 *  - `fn_802D3984`: retail widens `reverse` with `clrlwi r0,r6,24` and sign-extends both bounds;
 *  - `fn_802D6690`: retail computes the `field_0x33A * 4` index before the table base;
 *  - `fn_802C5428`/`fn_802C5624`: retail hoists the SE word's base+offset ahead of the `arg` compares;
 *    `fn_802C5ACC`/`fn_802C4CD4`: the `shell_set_func_ptr` load schedules three instructions early in ours;
 *  - `fn_802C4EA8`: retail's `arg == 5` is a signed `cmpwi` where the `u8` parameter gives `cmplwi`;
 *    `fn_802C4908`: one `lhz`-vs-`lha` load of `_PLW` +0x650 and the r29/r30 pairing;
 *  - `fn_802C5ECC`/`fn_802C6110`/`fn_802C6318`: retail reuses `setVec3`'s returned pointer (`mr r4,r3`); the
 *    declaration `pl.h` reaches through `ef.h` returns `void` (`enemy/fn_80165FC8.h`'s `VEC3*` spelling clashes);
 *  - `fn_802C5D10`: one extra `clrlwi r0,r31,24` at the third flag test that MWCC CSEs into the second's;
 *  - `fn_802C6BE0`/`fn_802C6E3C`/`fn_802C703C`/`fn_802C722C`: the `clrlwi r5,r5,16` retail masks a constant ternary's
 *    `u16` with, or one `cmpwi`/`cmplwi` case test (`fn_802C6578` too); `fn_802C6E3C` also swaps r30/r31;
 *  - `fn_802D2264`, `fn_802D282C`, `fn_802D30F8`, `fn_802D15DC`, `fn_802D3184`, `fn_802D2F7C`,
 *    `fn_802D6B2C`, `fn_802D77DC`, `fn_802D7A50`, `fn_802D7CE4`, `fn_802D7688`, `fn_802D6A00`: register allocation and
 *    argument width; the instruction streams agree;
 *  - `.data`: the jump tables are emitted byte-identical but anonymous, so objdiff leaves the map's
 *    `jumptable_805D4E80`/`jumptable_805D4EA8`/`jumptable_805D4F3C` unpaired (playbook 23).
 *   flipcheck: `.ctors` (0x4), `.sdata` (0x98) and `.sbss` (0x8) claimed but not emitted; short `.text` 0x944C of
 *   0x177A4, extab 0x360 of 0x770, extabindex 0x510 of 0xB28, `.data` 0x1B8 of 0x1A18, `.sdata2` 0x50 of 0x208; the
 *   bytes of all five differ.
 * SHAPES. Record views: the 0x802D0F34-0x802D44F4 bodies sit in `namespace view_fn_802D0F34` with `ai/ai_npc.h` and
 *   `ai/fn_802D0F34.h`, whose `_AINPC_W` is a second partial view of `ai/ainpc.h`'s (extern "C" keeps their symbols);
 *   its three C++-mangled `ai_*` functions stay global and cast to the view.  Unifying the views removes the namespace.
 *  - Of the eight shared functions the pieces declare differently, each keeps its best-scoring prototype
 *    (`fn_802D30F8` the sub-state view's); the call of `fn_802D66B8` keeps its `(s16)` narrowing as a cast.
 *  - The four motion tables (`lbl_805D4784`..`lbl_805D48D8`) are `u16` word arrays indexed `arg * 5`/`arg * 7`: a
 *    10/14-byte row struct folds the index into one `mulli`.
 *  - State steps are `self->state++`/`self->sub_step--`/`field += 1`, never `x = x + 1` (a `clrlwi` retail lacks,
 *    playbook 38); the flag parameters are `u32` tested `(flag & 0xFF)` (retail's `clrlwi` at every test).
 *  - The `arg == 0 || arg == 2` class tests are a `switch` with shared case labels (playbook 34/37); `fn_802C4EA8`
 *    declares one output vector per case and `fn_802C58DC` writes the ground result with one chained assignment
 *    (retail's single `lfs` for two stores).
 *  - `fn_802D0DCC` writes its `default` arm first: MWCC places it right after the compare chain, as retail does.
 *  - `ai/ai_npc.h` still defines `_HIT_W` beside `Pl/hit_w.h` (rule 1); unifying the views folds it too.
 */

#include "types.h"
#include "camera/camera.h"
#include "nw4r/math.h"

/* Owner headers (rule 2): every symbol a registered unit defines is declared in that unit's header,
 * never here.  `unsplit/unknown.h` carries the module-ambiguous ones. */
#include "ef/fn_800CDB2C.h"
#include "fn_80047398.h"
#include "fn_8004CAD8.h"
#include "g3d/fn_80063888.h"
#include "lobby/lb_npc.h"
#include "mh3_pad.h"
#include "stage/stg_w.h"
#include "unsplit/Runtime.PPCEABI.H.h"
#include "unsplit/unknown.h"

/* The record fn_802C2F08 dispatches on: its state word is at +0x172 and its two arm handlers are
 * fn_802C2DD0 / fn_802C2E6C. */
typedef struct LightArm {
    /* +0x000 */ u8 unused_0x000[0x172];
    /* +0x172 */ u16 state;
} LightArm; /* size: 0x174 traced (the record is larger; approximation) */
extern "C" void fn_802C2DD0(LightArm* self);
extern "C" void fn_802C2E6C(LightArm* self);

/* ------------------------------------------------------------------------------------------------ */
/* functions                                                                                         */
/* ------------------------------------------------------------------------------------------------ */
#include "mh3_pad.h" /* the owner header (rule 2) */
#include "pl.h"                /* `_PLW` - the record's +0x650/+0x652 the motion switch reads */
#include "Pl/fn_8027D684.h"    /* `fn_8027D76C` (rule 2: the owner's header) */
#include "Pl/fn_8028F66C.h"    /* `copyVec3`, `fn_8012A624`, `fn_80291B08` */
#include "fn_8004CAD8.h"       /* `rotVecY`, `subVec3`, `addVec3` */
#include "sound/fn_800D7F54.h" /* `fn_800DCC24` */
#include "ai/ainpc.h"
#include "stage/shell_set_func_ptr.h" /* `shell_set_func_ptr` and its `set_target`/`request` slots (rule 2) */

/* The four motion tables the band's rows live in, `.data` 0x805D4784..0x805D4964: the ladder's
 * 5-word rows (0x805D4810, 0x805D4874) and the wider 7-word rows (0x805D4784, 0x805D48D8).  They sit in
 * this unit's `.data` and are `extern`-declared, never defined (playbook 29).
 *
 * They are declared as `u16` word arrays, not as a row struct: a 10/14-byte struct's index scale folds
 * into one `mulli`, and retail's index math is the strength-reduced `arg * 5` / `arg * 7` followed by
 * the u16 scale (`slwi`+`add`+`slwi`), i.e. the source carried the word stride itself.  Word offsets,
 * ascending - the layout a struct would have stated, kept here because the tables are word arrays:
 *
 *   5-word rows                                7-word rows
 *   word +0 (byte +0x00) anim_id   s16         word +0 (byte +0x00) anim_id   s16
 *   word +1 (byte +0x02) motion    u16         word +1 (byte +0x02) motion    u16
 *   word +2 (byte +0x04) param     s16         word +2 (byte +0x04) param     s16
 *   word +3 (byte +0x06) variant   s16         word +3 (byte +0x06) variant   s16
 *   word +4 (byte +0x08) se_id     u16         word +4 (byte +0x08) motion2   u16
 *                                              word +5 (byte +0x0A) param2    s16
 *                                              word +6 (byte +0x0C) se_id     u16
 */
enum {
    ROW5_ANIM = 0, ROW5_MOTION = 1, ROW5_PARAM = 2, ROW5_VARIANT = 3, ROW5_SE = 4, ROW5_WORDS = 5,
};
enum {
    ROW7_ANIM = 0, ROW7_MOTION = 1, ROW7_PARAM = 2, ROW7_VARIANT = 3, ROW7_MOTION2 = 4,
    ROW7_PARAM2 = 5, ROW7_SE = 6, ROW7_WORDS = 7,
};

extern "C" {

extern u16 lbl_805D4784[];   /* 10 x 7 words: the flight's own ladder */
extern u16 lbl_805D4810[];   /* 10 x 5 words */
extern u16 lbl_805D4874[];   /* 10 x 5 words */
extern u16 lbl_805D48D8[];   /* 10 x 7 words */

/* The `.sdata2` constants the state machines compare against and arm (playbook 29: declared, never
 * defined - this unit's own pool). */
extern f32 lbl_8079A69C;
extern f32 lbl_8079A6A0;
extern f32 lbl_8079A6A4;
extern f32 lbl_8079A6A8;
extern f32 lbl_8079A6AC;
extern f32 lbl_8079A6B0;
extern f32 lbl_8079A6B4;
extern f32 lbl_8079A6B8;

/* The motion band's shared helpers (this unit's own, 0x802D2904-0x802D9D30), declared ahead of their
 * definitions in the call sites' own argument shapes; `fn_802D9D30` is not written yet. */
void fn_802D2904(struct _AINPC_W* self, u32 motion, s32 param, u32 flag);
void fn_802D2910(struct _AINPC_W* self, u32 motion, s32 param, u32 flag);
u32 fn_802D2984(struct _AINPC_W* self);
u32 fn_802D2990(struct _AINPC_W* self, u32 flag, f32 low, f32 high);
void fn_802D29F8(struct _AINPC_W* self, u32 variant);
void fn_802D2ABC(struct _AINPC_W* self, u32 flag, u16 a, u16 se);
void fn_802D2AD4(struct _AINPC_W* self, u32 state);
u32 fn_802D2B78(struct _AINPC_W* self, u32 flag);
void fn_802D2B88(struct _AINPC_W* self, u32 flag);
void fn_802D31FC(struct _AINPC_W* self);
void fn_802D3210(struct _AINPC_W* self, u32* value);
void fn_802D3A2C(struct _AINPC_W* self, u32 flag);
void fn_802D3CCC(struct _AINPC_W* self, u32 flag);
void fn_802D3CD4(struct _AINPC_W* self, u32 flag);
void fn_802D40A4(struct _AINPC_W* self, u16 motion);
void fn_802D40EC(struct _AINPC_W* self, u16 motion, s16 param);
void fn_802D9A44(struct _AINPC_W* self);
void fn_802D9D30(struct _AINPC_W* self, s32 a, u32 b, u32 c, u32 d);
}

#include "pl.h"
#include "Pl/pl_act.h"
#include "Pl/fn_8028F66C.h"
#include "g3d/g3d_calcworld.h"
#include "ef/fn_800CDB2C.h"
#include "unsplit/ai.h"

#ifdef __cplusplus

extern "C" {
#endif

/* More of the motion band's helpers this band dispatches into (this unit's own), declared ahead of
 * their definitions with the callees' own signatures; `fn_802D9400` is not written yet. */
void fn_802D3A2C(struct _AINPC_W* self, u32 a);
void fn_802D3CCC(struct _AINPC_W* self, u32 a);
void fn_802D3CD4(struct _AINPC_W* self, u32 a);
void fn_802D3CFC(struct _AINPC_W* self, u32 a);
void fn_802D29B8(struct _AINPC_W* self, u32 a);
void fn_802D29F8(struct _AINPC_W* self, u32 a);
u32 fn_802D2990(struct _AINPC_W* self, u32 a, f32 b, f32 c);
void fn_802D2B88(struct _AINPC_W* self, u32 a);
void fn_802D2B98(struct _AINPC_W* self, u32 a);
void fn_802D2AD4(struct _AINPC_W* self, u32 a);
void fn_802D35B4(struct _AINPC_W* self, s32 a);
void fn_802D3684(struct _AINPC_W* self);
void fn_802D6690(struct _AINPC_W* self);
void fn_802D9400(struct _AINPC_W* self);
void fn_802D29CC(struct _AINPC_W* self, f32 a);
u32 fn_802D3B34(struct _AINPC_W* self, s32 a, s32 b);
u16 fn_802D30F8(u16 a, u16 b, u16 c);
void fn_802D3210(struct _AINPC_W* self, u32* out);

/* `pl.h` pulls `ef.h` in, which declares `setVec3`/`VEC3_ctor` with their owner's type
 * (`nw4r::math::VEC3*`), the same record this file's locals use. */

/* Callees whose owner is a registered unit but whose header does not declare them yet: each spelling
 * is that owner's own declaration. */
void fn_8010072C(struct _PLW* owner, u8 type, nw4r::math::VEC3* pos, u32 param, f32 scale);
u32 fn_8027DCE0(struct _PLW* self, u8 arg1);
s32 fn_80291B08(struct _PLW* self, nw4r::math::VEC3* pos, LandData* land, f32* out, u32 kind);
void fn_80114CC8(void* actor, u8 key);

#ifdef __cplusplus
}
#endif

/* The target's player work (`_PLW`, `pl.h`), stored at +0x16C; its +0x3C triple is the position. */
struct _PLW;

/* `_AINPC_W` and `AINPCFormation` come from `ai/ainpc.h`. */

/* The C++-spelled entry points this unit defines below: their map names carry argument lists, so
 * declaring them at global C++ scope reproduces the target's mangling (rule 9). */
s32 ai_torch_ck(struct _AINPC_W* self);
u32 ai_skill_ck(struct _AINPC_W* self, u8 skill);

extern "C" {
u32 fn_8027D74C(struct _PLW* plw);
void fn_802CCF00(struct _AINPC_W* self, u32 a);
void fn_802D32B4(struct _AINPC_W* self, void* entry);
u32 fn_8027E06C(struct _PLW* plw, u32 a);
u32 fn_802D86D4(struct _AINPC_W* self, u16* a, s16* b);
void fn_802D65CC(struct _AINPC_W* self);
void fn_802D675C(struct _AINPC_W* self);
void fn_802D67F8(struct _AINPC_W* self);
void fn_802D3A20(struct _AINPC_W* self);
void fn_802D3AD8(struct _AINPC_W* self);
f32 fn_802D7258(struct _AINPC_W* self, u32 a);

/* 0x805D4030 - the 4-pointer table the dispatchers hand to `fn_802D32B4`, in this unit's `.data`:
 * declared, never defined (playbook 29). */
extern void* lbl_805D4030[4];
s32 fn_802D3184(struct _AINPC_W* self, u32 flag);
void fn_802D3B10(struct _AINPC_W* self);
void fn_802D3B24(struct _AINPC_W* self);
void fn_802D4200(struct _AINPC_W* self);
void fn_802D4218(struct _AINPC_W* self);
void fn_802D4230(struct _AINPC_W* self, f32 value);
void fn_802D4238(struct _AINPC_W* self);
void fn_802D2A00(struct _AINPC_W* self, u32 a, u32 b, u32 c);
s16 fn_802D9D14(void);

/* 0x802CD20C is written below; 0x802CCE18 and 0x802CCF00 are this unit's, not written yet. */
}

/* The dispatch state object.  Only the two bytes the function reads are named; everything else is
 * padding the disassembly does not describe. */
typedef struct DispatchState {
    u8 pad_0x000[0x170];    /* +0x000 */
    u8 variant;             /* +0x170: selects the odd/even table entry */
    u8 pad_0x171[0x2AF];    /* +0x171 */
    u8 state;               /* +0x420: the switch value */
} DispatchState;            /* size: 0x421 */

/* 16 entries, each handed to the tail-called handler.  The table is this unit's `.data`, declared and
 * never defined (playbook 29); the handler `fn_802D3398` is defined in the view namespace below. */
extern "C" {
extern void* const lbl_805D4150[16];
extern void fn_802D3398(DispatchState* self, void* entry);
}
#include "ai/ainpc_w.h" /* `ainpc_w`, defined at the foot of this file (rule 2) */
#include "mh3_pad/lb_param_w.h" /* `lb_param_w`, owned by mh3_pad.cpp (rule 2) */
#include "ai/fn_802D44F4.h"    /* this unit's and `hud/cockpit.cpp`'s declarations (rule 2) */
#include "quest/quest_item_slot.h" /* quest_move_state_valid_ck (rule 2: its owner) */
#include "ai/ainpc_w.h" /* `ainpc_w`, defined at the foot of this file (rule 2) */

/* The C++-spelled entry points of this range: their map names carry argument lists, so they are
 * declared at global C++ scope and the front-end reproduces the mangling (rule 9). */
u32 ai_skill_ck(struct _AINPC_W* self, u8 skill);
extern "C" s32 fn_802D9CE0(struct _AINPC_W* self);
s32 ai_area_ck(struct _AINPC_W* self);

extern "C" {

/* The tables these bodies index, in this unit's `.data`: declared, never defined (playbook 29), each as
 * a **complete** array so the base comes out as `lis`/`addi` like the target's. */
extern u8 lbl_805D4190[0x10];
extern u8 lbl_805D41A0[0x10];
extern u16 lbl_805D41B0[0x10];
extern char* lbl_805D4390[4];
extern u32 lbl_805D4358[7];
extern u16 lbl_805D4560[6];

/* The per-monster hold-slot table at 0x805D43A0: 31 records of four u16 (size 0xF8).  Field 0 of a
 * record is the id the scan compares, fields 1..3 the values `fn_802D7BA4`..`fn_802D7BE4` hand out;
 * the records are reached as `lbl_805D43A0[index * 4 + field]` because that is the addressing the
 * object itself uses (`slwi r3,r0,2` then `addi r0,r3,1; slwi r0,r0,1`). */
extern u16 lbl_805D43A0[0x7C];
/* The matching 0x805D44EC table: 21 records of two u16 (size 0x54). */
extern u16 lbl_805D44EC[0x2A];

void fn_800DCCF8(void* state, nw4r::math::VEC3* pos, s32 enable);

/* The AI-NPC work record's own helpers, all inside this range and defined below. */
s32 fn_802D77A0(u8 value);
void fn_802D9544(void);
s32 fn_802D77DC(u8 slot);
s32 fn_802D2B38(struct _AINPC_W* self, s32 a, s32 b);
s32 fn_802D66B8(struct _AINPC_W* self);
void fn_802D84D0(struct _AINPC_W* self, s16 value);
}

#pragma peephole off

/* Moves the record to the arm its state selects. */
extern "C" void fn_802C2F08(LightArm* self)
{
    switch (self->state) {
    case 0:
        fn_802C2DD0(self);
        break;
    case 1:
        fn_802C2E6C(self);
        break;
    }
}


/* The aimed action's first stage: arms the swing, then hands the motion's own ladder to the effect
 * pass once `fn_802D2990` reports the arm window open. */
extern "C" void fn_802C474C(struct _AINPC_W* self, u8 arg) {
    u8 state;
    if (fn_802D2B78(self, 0x10) != 0) {
        fn_802D3A2C(self, 2);
    }
    state = self->state;
    switch (state) {
    case 0:
        self->state++;
        self->sub_step = 3;
        if (arg == 0) {
            fn_802D29F8(self, 0);
            fn_802D2904(self, 0x17, 4, 0);
        } else {
            fn_802D29F8(self, 2);
            fn_802D2904(self, 0x3f, 4, 0);
        }
        break;
    case 1:
        if ((arg == 0 && fn_802D2990(self, 1, lbl_8079A69C, lbl_8079A670) != 0) ||
            (arg == 1 && fn_802D2990(self, 1, lbl_8079A6A0, lbl_8079A670) != 0)) {
            if (self->sub_step == 0) {
                self->state++;
            } else {
                self->sub_step--;
                if (arg == 0) {
                    fn_802D2904(self, 0x17, 0, 0x1c);
                } else {
                    fn_802D2904(self, 0x3f, 0, 0x30);
                }
            }
        }
        break;
    case 2:
        if (fn_802D2984(self) == 1) {
            if (arg == 0) {
                fn_802D2AD4(self, 0);
            } else {
                fn_802D2AD4(self, 2);
            }
        }
        break;
    }
}

/* The ranged action's arm: reads the enemy's own motion out of the player work it targets and leaves
 * the record in the matching motion before the target's frame pass runs. */
extern "C" void fn_802C4908(struct _AINPC_W* self, u8 arg) {
    u8 state = self->state;
    switch (state) {
    case 0: {
        struct _PLW* plw;
        self->state++;
        if (arg == 0) {
            fn_802D29F8(self, 0);
            fn_802D2904(self, 6, 4, 0);
        } else {
            fn_802D29F8(self, 2);
            fn_802D2904(self, 0x39, 4, 0);
        }
        plw = self->plw_0x16C;
        switch (plw->field_0x650) {
        case 0x1c:
            self->field_0x3C2 = 0x1c;
            break;
        case 0x1b:
        case 0x2d:
        case 0xef:
            self->field_0x3C2 = 0x1b;
            break;
        case 0x1a:
        case 0xf0:
            self->field_0x3C2 = 0x1a;
            break;
        case 0x1e:
            self->field_0x3C2 = 0x1e;
            break;
        case 0x1f:
            self->field_0x3C2 = 0x1f;
            break;
        case 0x20:
            self->field_0x3C2 = 0x20;
            break;
        default:
            self->field_0x3C2 = 0;
            fn_802D40EC(self, plw->field_0x650, plw->field_0x652);
            if (self->field_0x420 == 3) {
                self->field_0x422 = 0x708;
                self->field_0x42C = plw->field_0x650;
                self->field_0x427 = 0;
                self->field_0x428 = 0;
                self->field_0x429 = 0;
                self->field_0x42A = 0;
                self->field_0x42B = 0;
            }
            break;
        }
        fn_8027D76C(plw);
        break;
    }
    case 1: {
        struct _PLW* plw = self->plw_0x16C;
        if (fn_802D2984(self) != 1) {
            break;
        }
        if (arg == 0) {
            if (self->field_0x3C2 != 0) {
                fn_802D2ABC(self, 1, 0x14, 0);
            } else {
                fn_802D2ABC(self, 1, 0x17, 0);
                fn_802D9D30(self, (s8)self->field_0x482, 0x13, plw->field_0x650, 2);
            }
        } else {
            if (self->field_0x3C2 != 0) {
                fn_802D40A4(self, plw->field_0x650);
            }
            fn_802D2ABC(self, 1, 0x36, 0);
            fn_802D9D30(self, (s8)self->field_0x482, 0x13, plw->field_0x650, 2);
        }
        break;
    }
    }
}

/* The arm-out: cancels the armed motion's tail and returns the record to the variant it started the
 * action with. */
extern "C" void fn_802C4B68(struct _AINPC_W* self, u8 arg) {
    u8 state = self->state;
    switch (state) {
    case 0:
        self->state++;
        if (arg == 0) {
            fn_802D29F8(self, 0);
            fn_802D2904(self, 0x25, 8, 0);
        }
        break;
    case 1:
        if (fn_802D2984(self) == 1) {
            fn_802D2AD4(self, self->variant);
        }
        break;
    }
}

/* The two-stage aimed action's first half. */
extern "C" void fn_802C4BF4(struct _AINPC_W* self, u8 arg) {
    u8 state = self->state;
    switch (state) {
    case 0:
        self->state++;
        if (arg == 0) {
            fn_802D29F8(self, 0);
            fn_802D2904(self, 0x12, 4, 0);
        } else {
            fn_802D29F8(self, 2);
            fn_802D2904(self, 0x42, 4, 0);
        }
        break;
    case 1:
        if (fn_802D2984(self) == 1) {
            self->state++;
            if (arg == 0) {
                fn_802D2AD4(self, 0);
            } else {
                fn_802D2AD4(self, 2);
            }
        }
        break;
    }
}

/* The four-stage aimed action: the swing, its hold, the settle (which hands the target to the shell
 * table's own state change) and the arm-out. */
extern "C" void fn_802C4CD4(struct _AINPC_W* self, u8 arg) {
    u8 state = self->state;
    switch (state) {
    case 0:
        self->state++;
        if (arg == 0) {
            fn_802D29F8(self, 0);
            fn_802D2904(self, 0x2d, 2, 0);
        } else {
            fn_802D29F8(self, 2);
            fn_802D2904(self, 0x51, 2, 0);
        }
        break;
    case 1:
        if (fn_802D2984(self) == 1) {
            self->state++;
            if (arg == 0) {
                fn_802D2904(self, 4, 2, 0);
            } else {
                fn_802D2904(self, 0x37, 2, 0);
            }
        }
        break;
    case 2:
        if (fn_802D2984(self) == 1) {
            self->state++;
            if (arg == 0) {
                fn_802D2904(self, 0x31, 0, 0);
            } else {
                fn_802D2904(self, 0x53, 6, 0);
            }
            shell_set_func_ptr->set_target(self, shell_set_func_ptr);
            fn_802D9A44(self);
            self->field_0x447 = 0;
            self->field_0x446 = 1;
            fn_802D9D30(self, (s8)self->field_0x482, 0xa, 0, 2);
        }
        break;
    case 3:
        if (fn_802D2984(self) == 1) {
            if (arg == 0) {
                fn_802D2AD4(self, 0);
            } else {
                fn_802D2AD4(self, 2);
            }
        }
        break;
    }
}

/* The arrow's flight: the row keys the shot, `rotVecY` builds the shot vector out of the record's own
 * yaw, and the per-frame pass integrates the effect vector and the stage timer. */
extern "C" void fn_802C4EA8(struct _AINPC_W* self, u8 arg) {
    nw4r::math::VEC3 shot;
    VEC3_ctor(&shot);
    shot.x = lbl_8079A670;
    shot.y = lbl_8079A670;
    shot.z = lbl_8079A6A4;
    switch (self->state) {
    case 0: {
        u16* row;
        nw4r::math::VEC3 out;
        self->state++;
        if (arg == 0 || arg == 5) {
            self->sub_step = 1;
        }
        row = &lbl_805D4784[arg * ROW7_WORDS];
        fn_802D29F8(self, (u8)(s16)row[ROW7_ANIM]);
        fn_802D2904(self, row[ROW7_MOTION], (s16)row[ROW7_PARAM], 0);
        if (arg != 4 && arg != 9) {
            self->field_0x3F0 = (u8)(s16)row[ROW7_PARAM2];
        }
        if (self->field_0x3F0 == 0) {
            fn_802D2B88(self, 1);
        }
        rotVecY(&shot, self->field_0x194);
        addVec3(&out, &self->vec_0x178, &shot);
        copyVec3(&self->vec_0x3E0, &out);
        self->field_0x3EC = (u16)self->field_0x194;
        self->field_0x3EE = 0;
        fn_800DCC24(self->sound_0x498, 0, 0);
        break;
    }
    case 1: {
        nw4r::math::VEC3 out;
        self->field_0x3EC += 0x1d1;
        self->field_0x194 = self->field_0x3EC;
        rotVecY(&shot, self->field_0x194);
        subVec3(&out, &self->vec_0x3E0, &shot);
        copyVec3(&self->vec_0x178, &out);
        if (arg >= 5) {
            self->field_0x190 = (u16)fn_802D30F8(0, self->field_0x190 & 0xffff, 0x600);
        }
        if (fn_802D2984(self) == 1) {
            self->state++;
            fn_802D2904(self, lbl_805D4784[arg * ROW7_WORDS + ROW7_MOTION], (s16)lbl_805D4784[arg * ROW7_WORDS + ROW7_PARAM], 0);
        } else if (arg < 5) {
            if (fn_802D2990(self, 3, lbl_8079A670, lbl_8079A6A8) != 0) {
                self->field_0x3EE -= 0x889;
            }
        }
        self->field_0x194 += self->field_0x3EE;
        break;
    }
    case 2: {
        nw4r::math::VEC3 out;
        self->field_0x3EC += 0x1d1;
        self->field_0x194 = self->field_0x3EC;
        rotVecY(&shot, self->field_0x194);
        subVec3(&out, &self->vec_0x3E0, &shot);
        copyVec3(&self->vec_0x178, &out);
        if (fn_802D2984(self) == 1) {
            if (self->sub_step == 0) {
                fn_802D2ABC(self, 1, lbl_805D4784[arg * ROW7_WORDS + ROW7_SE], self->field_0x1EC);
            } else {
                self->state = 1;
                self->sub_step--;
                fn_802D2904(self, lbl_805D4784[arg * ROW7_WORDS + ROW7_MOTION], (s16)lbl_805D4784[arg * ROW7_WORDS + ROW7_PARAM], 0);
                fn_800DCC24(self->sound_0x498, 0, 1);
            }
        } else if (arg < 5) {
            if (fn_802D2990(self, 3, lbl_8079A670, lbl_8079A6A8) != 0) {
                self->field_0x3EE += 0x889;
            }
        }
        self->field_0x194 += self->field_0x3EE;
        break;
    }
    }
}

/* The three-row ladder's first rung (rows 0x805D4810): arms the row's motion, then re-arms either the
 * row's own settle SE or the next rung depending on how far into the ladder the caller is. */
extern "C" void fn_802C522C(struct _AINPC_W* self, u8 arg) {
    u8 state = self->state;
    switch (state) {
    case 0: {
        u16* row;
        self->state++;
        row = &lbl_805D4810[arg * ROW5_WORDS];
        fn_802D29F8(self, (u8)(s16)row[ROW5_ANIM]);
        fn_802D2904(self, row[ROW5_MOTION], (s16)row[ROW5_PARAM], 0);
        if (arg != 4 && arg != 9) {
            self->field_0x3F0 = (u8)(s16)row[ROW5_VARIANT];
            fn_800DCC24(self->sound_0x498, 1, 0);
        } else {
            fn_800DCC24(self->sound_0x498, 1, 1);
        }
        break;
    }
    case 1:
        if (arg >= 5) {
            self->field_0x190 = (u16)fn_802D30F8(0, self->field_0x190 & 0xffff, 0x600);
        }
        if (fn_802D2984(self) == 1) {
            self->state++;
            if (arg == 0) {
                fn_802D2904(self, 0xd3, 0, 0);
                fn_800DCC24(self->sound_0x498, 1, 1);
            } else if (arg == 5) {
                fn_802D2904(self, 0xea, 0, 0);
                fn_800DCC24(self->sound_0x498, 1, 1);
            } else {
                fn_802D2ABC(self, 1, lbl_805D4810[arg * ROW5_WORDS + ROW5_SE], self->field_0x1EC);
            }
        }
        break;
    case 2:
        if (fn_802D2984(self) == 1) {
            fn_802D2ABC(self, 1, lbl_805D4810[arg * ROW5_WORDS + ROW5_SE], self->field_0x1EC);
        }
        break;
    }
}

/* The second rung (rows 0x805D4874), the same ladder one row group on. */
extern "C" void fn_802C5428(struct _AINPC_W* self, u8 arg) {
    u8 state = self->state;
    switch (state) {
    case 0: {
        u16* row;
        self->state++;
        row = &lbl_805D4874[arg * ROW5_WORDS];
        fn_802D29F8(self, (u8)(s16)row[ROW5_ANIM]);
        fn_802D2904(self, row[ROW5_MOTION], (s16)row[ROW5_PARAM], 0);
        if (arg != 4 && arg != 9) {
            self->field_0x3F0 = (u8)(s16)row[ROW5_VARIANT];
            fn_800DCC24(self->sound_0x498, 2, 0);
        } else {
            fn_800DCC24(self->sound_0x498, 2, 1);
        }
        break;
    }
    case 1:
        if (arg >= 5) {
            self->field_0x190 = (u16)fn_802D30F8(0, self->field_0x190 & 0xffff, 0x600);
        }
        if (fn_802D2984(self) == 1) {
            self->state++;
            if (arg == 0) {
                fn_802D2904(self, 0xcb, 0, 0);
                fn_800DCC24(self->sound_0x498, 2, 1);
            } else if (arg == 5) {
                fn_802D2904(self, 0xe2, 0, 0);
                fn_800DCC24(self->sound_0x498, 2, 1);
            } else {
                fn_802D2ABC(self, 1, lbl_805D4874[arg * ROW5_WORDS + ROW5_SE], self->field_0x1EC);
            }
        }
        break;
    case 2:
        if (fn_802D2984(self) == 1) {
            fn_802D2ABC(self, 1, lbl_805D4874[arg * ROW5_WORDS + ROW5_SE], self->field_0x1EC);
        }
        break;
    }
}

/* The four-row ladder (rows 0x805D48D8): the widest of the three, one rung per motion of the volley. */
extern "C" void fn_802C5624(struct _AINPC_W* self, u8 arg) {
    u8 state = self->state;
    switch (state) {
    case 0: {
        u16* row;
        self->state++;
        row = &lbl_805D48D8[arg * ROW7_WORDS];
        fn_802D29F8(self, (u8)(s16)row[ROW7_ANIM]);
        fn_802D2904(self, row[ROW7_MOTION], (s16)row[ROW7_PARAM], 0);
        if (arg != 4 && arg != 9) {
            self->field_0x3F0 = (u8)(s16)row[ROW7_VARIANT];
            fn_800DCC24(self->sound_0x498, 3, 0);
        } else {
            fn_800DCC24(self->sound_0x498, 3, 1);
        }
        break;
    }
    case 1:
        if (arg >= 5) {
            self->field_0x190 = (u16)fn_802D30F8(0, self->field_0x190 & 0xffff, 0x600);
        }
        if (fn_802D2984(self) == 1) {
            self->state++;
            fn_802D2904(self, lbl_805D48D8[arg * ROW7_WORDS + ROW7_MOTION2], (s16)lbl_805D48D8[arg * ROW7_WORDS + ROW7_PARAM2], 0);
        }
        break;
    case 2:
        if (fn_802D2984(self) == 1) {
            self->state++;
            if (arg == 0) {
                fn_802D2904(self, 0xce, 0, 0);
                fn_800DCC24(self->sound_0x498, 3, 1);
            } else if (arg == 5) {
                fn_802D2904(self, 0xe5, 0, 0);
                fn_800DCC24(self->sound_0x498, 3, 1);
            } else {
                fn_802D2ABC(self, 1, lbl_805D48D8[arg * ROW7_WORDS + ROW7_SE], self->field_0x1EC);
            }
        }
        break;
    case 3:
        if (fn_802D2984(self) == 1) {
            self->state++;
            if (arg == 0) {
                fn_802D2904(self, 0xcf, 0, 0);
            } else if (arg == 5) {
                fn_802D2904(self, 0xe6, 0, 0);
            }
        }
        break;
    case 4:
        if (fn_802D2984(self) == 1) {
            fn_802D2ABC(self, 1, lbl_805D48D8[arg * ROW7_WORDS + ROW7_SE], self->field_0x1EC);
        }
        break;
    }
}

/* The aimed shot's release: raises the effect handle, waits for the arm window, then queries the ground
 * under the shot vector and leaves the record in the landing state it reports. */
extern "C" void fn_802C58DC(struct _AINPC_W* self) {
    LandData land;
    f32 ground;
    fn_8012A624(&land);
    switch (self->state) {
    case 0:
        self->state++;
        fn_802D29F8(self, 2);
        fn_802D2910(self, 0xd, 8, 0);
        fn_802D31FC(self);
        self->field_0x1DC = lbl_8079A6AC;
        break;
    case 1:
        fn_802D3210(self, &self->field_0x190);
        self->field_0x190 = (u16)fn_802D30F8(0x4000, self->field_0x190 & 0xffff, 0x800);
        fn_802D3CD4(self, 2);
        if (self->vec_0x178.y > lbl_8079A6B0) {
            self->vec_0x178.y = lbl_8079A6B0;
        }
        if (self->field_0x19C - lbl_8079A6B4 >= self->vec_0x178.y) {
            self->state++;
            fn_802D29F8(self, 2);
            fn_802D2904(self, 0x3a, 8, 0);
        }
        break;
    case 2:
        self->field_0x190 = (u16)fn_802D30F8(0, self->field_0x190 & 0xffff, 0x1000);
        if (fn_802D2984(self) == 1) {
            copyVec3(&self->vec_0x178, &self->vec_0x1B0);
            if (fn_80291B08((struct _PLW*)self, &self->vec_0x178, &land, &ground, -5) == 1) {
                /* One `lfs` and two stores in retail: the traveller's distance and the
                 * height it settles at are the same value. */
                self->vec_0x178.y = self->field_0x19C = ground;
            }
            self->field_0x3C6 = 0;
            fn_802D3CCC(self, 1);
            if ((land.field_0x00 & 3) != 0) {
                fn_802D2ABC(self, 1, 0x37, 0);
            } else {
                fn_802D2ABC(self, 1, 5, 0);
            }
        } else {
            fn_802D3CD4(self, 2);
            fn_802D84D0(self, 2);
        }
        break;
    }
}

/* The guard/parry action: the arm chooses the shell's own state table by the arg class, and the settle
 * asks the shell to play the class's SE. */
extern "C" void fn_802C5ACC(struct _AINPC_W* self, u8 arg) {
    u8 state = self->state;
    switch (state) {
    case 0:
        self->state++;
        switch (arg) {
        case 0:
        case 2:
            fn_802D29F8(self, 0);
            fn_802D2904(self, 0x2e, 0, 0);
            break;
        case 1:
        case 3:
            fn_802D29F8(self, 2);
            fn_802D2904(self, 0x52, 0, 0);
            break;
        }
        break;
    case 1:
        if (fn_802D2984(self) == 1) {
            switch (arg) {
            case 0:
            case 2:
                fn_802D2AD4(self, 0);
                break;
            case 1:
            case 3:
                fn_802D2AD4(self, 2);
                break;
            }
        } else {
            switch (arg) {
            case 0:
            case 2:
                if (fn_802D2990(self, 0, lbl_8079A68C, lbl_8079A670) == 1) {
                    if (arg == 0) {
                        shell_set_func_ptr->request(self, 0xc, shell_set_func_ptr);
                    } else {
                        shell_set_func_ptr->request(self, 0xe, shell_set_func_ptr);
                        fn_802D9D30(self, (s8)self->field_0x482, 0x1a, 0, 2);
                        self->field_0x451 = 0;
                    }
                }
                break;
            case 1:
            case 3:
                if (fn_802D2990(self, 0, lbl_8079A6B8, lbl_8079A670) == 1) {
                    if (arg == 1) {
                        shell_set_func_ptr->request(self, 0xd, shell_set_func_ptr);
                    } else {
                        shell_set_func_ptr->request(self, 0xf, shell_set_func_ptr);
                        fn_802D9D30(self, (s8)self->field_0x482, 0x1a, 0, 2);
                        self->field_0x451 = 0;
                    }
                }
                break;
            }
        }
        break;
    }
}


/* The per-motion step dispatcher: it arms the motion and its scale once, then steps the armed
 * sub-state until the motion ends. */
extern "C" void fn_802C5D10(struct _AINPC_W* self, u32 flag)
{
    nw4r::math::VEC3 vec;

    VEC3_ctor(&vec);
    fn_802D3A2C(self, 2);
    fn_802D29B8(self, 2);
    switch (self->state) {
    case 0:
        self->state += 1;
        fn_802D29F8(self, 0);
        if ((flag & 0xFF) == 0) {
            fn_802D2904(self, 14, 4, 146);
        } else {
            fn_802D2904(self, 14, 4, 186);
        }
        self->field_0x442 = 0;
        self->field_0x444 = 0;
        break;
    case 1:
        fn_802D3CD4(self, 1);
        if (fn_802D2984(self) == 1) {
            if ((flag & 0xFF) == 0) {
                fn_802D2ABC(self, 1, 83, 0);
            } else {
                fn_802D2ABC(self, 1, 85, 0);
            }
            break;
        }
        if ((flag & 0xFF) == 0) {
            if (fn_802D2990(self, 1, lbl_8079A6BC, lbl_8079A670) == 1) {
                fn_802D2B88(self, 2);
                f32 y = self->vec_0x178.y;
                f32 height = lbl_8079A6C0 + self->field_0x19C;

                if (y < height) {
                    self->vec_0x178.y = height;
                    fn_802D2ABC(self, 1, 83, 0);
                }
            }
        } else if ((flag & 0xFF) == 1) {
            if (fn_802D2990(self, 1, lbl_8079A6BC, lbl_8079A670) == 1) {
                fn_802D2B88(self, 2);
            }
        }
        break;
    }
}

/* The four-step variant: it arms a 248-frame motion and a 20-frame ramp, then re-aims the motion
 * through the `+0x005` frame table until the ramp is exhausted. */
extern "C" void fn_802C5ECC(struct _AINPC_W* self)
{
    nw4r::math::VEC3 vec;
    nw4r::math::VEC3 offset;

    VEC3_ctor(&vec);
    fn_802D3A2C(self, 2);
    fn_802D29B8(self, 2);
    fn_802D3CD4(self, 1);
    switch (self->state) {
    case 0:
        self->state += 1;
        fn_802D2B88(self, 2);
        fn_802D29F8(self, 0);
        fn_802D2904(self, 14, 0, 248);
        fn_802D3CD4(self, 1);
        self->vec_0x024.x = lbl_8079A678;
        self->vec_0x024.y = lbl_8079A678;
        self->vec_0x024.z = lbl_8079A678;
        self->field_0x1AC = 20;
        break;
    case 1: {
        s32 frames = self->field_0x1AC - 1;

        self->field_0x1AC = frames;
        if (frames > 0) {
            break;
        }
        self->state += 1;
        fn_802D2B98(self, 2);
        self->field_0x002 = 0;
        setVec3(&offset, lbl_8079A670, lbl_8079A670, lbl_8079A6C4);
        copyVec3(&vec, &offset);
        rotVecY(&vec, self->field_0x194);
        addVec3To(&vec, &self->vec_0x178);
        vec.y = self->field_0x19C;
        fn_8010072C((struct _PLW*)self, 42, &vec, 0, lbl_8079A678);
        self->field_0x1AC = lbl_80792598[(s16)fn_802D66B8(self)];
        self->sub_step = 0;
        break;
    }
    case 2: {
        s16 step = self->field_0x442 + 1;

        self->field_0x442 = step;
        if (step > lbl_80792598[self->sub_step]) {
            u32 slot = self->field_0x444 + 1;

            self->field_0x444 = slot;
            if (slot == 3) {
                self->field_0x444 = 4;
            } else if (slot > 4) {
                self->field_0x444 = 4;
            }
            self->sub_step += 1;
        }
        if (self->field_0x442 > (s16)self->field_0x1AC) {
            self->field_0x1AC = 20;
            self->state += 1;
        }
        break;
    }
    case 3: {
        s32 frames = self->field_0x1AC - 1;

        self->field_0x1AC = frames;
        if (frames > 0) {
            break;
        }
        fn_802D2ABC(self, 1, 87, 0);
        break;
    }
    }
}

/* The three-step variant: it copies the player's position into `vec_0x178` and arms a 14-frame
 * ramp, then re-aims the motion once the ramp hits its second-to-last frame. */
extern "C" void fn_802C6110(struct _AINPC_W* self)
{
    nw4r::math::VEC3 vec;
    LandData land;
    nw4r::math::VEC3 offset;
    f32 height;

    VEC3_ctor(&vec);
    fn_8012A624(&land);
    fn_802D3A2C(self, 2);
    fn_802D84D0(self, 2);
    switch (self->state) {
    case 0:
        self->state += 1;
        fn_802D29F8(self, 0);
        fn_802D2904(self, 54, 0, 0);
        copyVec3(&self->vec_0x178, &self->plw_0x16C->vec_0x03C);
        self->field_0x194 = self->plw_0x16C->field_0x058;
        self->field_0x1AC = 14;
        fn_802D2B88(self, 2);
        self->vec_0x024.x = lbl_8079A678;
        self->vec_0x024.y = lbl_8079A678;
        self->vec_0x024.z = lbl_8079A678;
        fn_802D3CCC(self, 1);
        break;
    case 1: {
        s32 frames = self->field_0x1AC - 1;

        self->field_0x1AC = frames;
        if (frames <= 0) {
            self->state += 1;
            if (fn_8027DCE0(self->plw_0x16C, 0) == 1) {
                fn_802D2B98(self, 2);
            }
            self->field_0x002 = 0;
        } else if (frames == 2) {
            setVec3(&offset, lbl_8079A670, lbl_8079A670, lbl_8079A6CC);
            copyVec3(&vec, &offset);
            rotVecY(&vec, self->field_0x194);
            addVec3To(&vec, &self->vec_0x178);
            if (fn_8027DCE0(self->plw_0x16C, 0) == 1) {
                fn_8010072C((struct _PLW*)self, 42, &vec, 0, lbl_8079A678);
            }
        }
        break;
    }
    case 2:
        copyVec3(&self->vec_0x178, &self->plw_0x16C->vec_0x03C);
        self->field_0x194 = self->plw_0x16C->field_0x058;
        if (fn_8027DCE0(self->plw_0x16C, 0) == 0) {
            if (fn_80291B08((struct _PLW*)self, &self->vec_0x178, &land, &height, -5) == 1) {
                self->vec_0x178.y = height;
            }
            fn_802D2ABC(self, 1, 86, self->field_0x1EC);
        }
        break;
    }
}

/* The two-step variant: it arms a 30-frame wait and a 0.75 scale, then fires the motion in the
 * direction the player's position implies. */
extern "C" void fn_802C6318(struct _AINPC_W* self, u8 flag)
{
    nw4r::math::VEC3 vec;
    nw4r::math::VEC3 front;
    nw4r::math::VEC3 back;

    VEC3_ctor(&vec);
    fn_802D3A2C(self, 2);
    switch (self->state) {
    case 0:
        self->state += 1;
        self->field_0x1AC = 30;
        self->field_0x002 = 1;
        self->vec_0x024.x = lbl_8079A674;
        self->vec_0x024.y = lbl_8079A674;
        self->vec_0x024.z = lbl_8079A674;
        if ((flag & 0xFF) == 0) {
            setVec3(&front, lbl_8079A670, lbl_8079A670, lbl_8079A6CC);
            copyVec3(&vec, &front);
        } else {
            self->vec_0x178.y = self->field_0x19C;
            setVec3(&back, lbl_8079A670, lbl_8079A670, lbl_8079A6C4);
            copyVec3(&vec, &back);
        }
        if (fn_802D2B78(self, 2) != 0) {
            break;
        }
        fn_802D2B88(self, 2);
        rotVecY(&vec, self->field_0x194);
        addVec3To(&vec, &self->vec_0x178);
        fn_8010072C((struct _PLW*)self, 42, &vec, 0, lbl_8079A678);
        break;
    case 1: {
        s32 frames = self->field_0x1AC - 1;

        self->field_0x1AC = frames;
        if (frames > 0) {
            break;
        }
        self->state += 1;
        fn_802D2904(self, 16, 0, 0);
        fn_802D2B98(self, 2);
        break;
    }
    case 2:
        if (fn_802D2984(self) != 1) {
            break;
        }
        if ((flag & 0xFF) == 0) {
            fn_802D2AD4(self, 0);
        } else {
            fn_802D6690(self);
            fn_802D40EC(self, 29, -1);
            switch ((u32)self->field_0x444) {
            case 0:
            case 1:
                fn_802D2ABC(self, 1, 23, 0);
                fn_802D40EC(self, 27, 1);
                break;
            case 2:
            case 3:
                fn_802D40EC(self, 26, 1);
                fn_802D2ABC(self, 1, 23, 0);
                break;
            case 4:
                fn_802D40EC(self, 28, 1);
                fn_802D2ABC(self, 1, 24, 0);
                break;
            }
        }
        self->field_0x440 = 0;
        break;
    }
}

/* The two-state projectile-launch dispatcher: it arms a 6-frame shot or a 57-frame hold depending on
 * the tier the record carries. */
extern "C" void fn_802C6578(struct _AINPC_W* self, u8 flag)
{
    switch (self->state) {
    case 0:
        self->state += 1;
        if ((flag & 0xFF) == 0) {
            fn_802D29F8(self, 0);
            fn_802D2904(self, 6, 4, 0);
        } else {
            fn_802D29F8(self, 2);
            fn_802D2904(self, 57, 4, 0);
        }
        self->field_0x1AC = 600;
        if (self->field_0x420 - 3 <= 1) {
            break;
        }
        fn_802D9D30(self, (s8)self->field_0x482, 20, self->field_0x462, 2);
        break;
    case 1: {
        s32 frames = self->field_0x1AC - 1;

        self->field_0x1AC = frames;
        if (frames > 0 && self->field_0x461 != 0) {
            break;
        }
        if ((flag & 0xFF) == 0) {
            fn_802D2AD4(self, 0);
        } else {
            fn_802D2AD4(self, 2);
        }
        self->field_0x461 = 0;
        break;
    }
    }
}

/* The four-state charge dispatcher: it arms a 300-frame charge, releases it into the bite/stomp
 * motion the `+0x484` tier selects, then waits for the motion to end. */
extern "C" void fn_802C6690(struct _AINPC_W* self)
{
    fn_802D3A2C(self, 2);
    fn_802D84D0(self, 2);
    switch (self->state) {
    case 0:
        self->state += 1;
        fn_802D29F8(self, 0);
        fn_802D2904(self, 14, 4, 0);
        break;
    case 1:
        if (fn_802D2984(self) == 1) {
            self->field_0x1AC = 300;
            fn_802D2B88(self, 2);
            self->state += 1;
        }
        break;
    case 2: {
        s32 frames = self->field_0x1AC - 1;

        self->field_0x1AC = frames;
        if (frames >= 0) {
            break;
        }
        fn_802D2B98(self, 2);
        fn_802D2904(self, 16, 0, 0);
        fn_802D9400(self);
        self->field_0x485 = 0;
        fn_802D3684(self);
        if (self->field_0x484 < 3) {
            fn_802D9D30(self, 9, 1, 0, 2);
            self->field_0x486 = 900;
        } else {
            fn_802D9D30(self, 9, 2, 0, 2);
        }
        fn_802D9A44(self);
        self->state += 1;
        break;
    }
    case 3:
        if (fn_802D2984(self) == 1) {
            fn_802D2AD4(self, 0);
        }
        break;
    }
}

/* The two-state tail-swipe dispatcher: it arms a 6-frame wind-up, then fires the swipe through the
 * `+0x1F4` motion as the countdown crosses frame 4. */
extern "C" void fn_802C681C(struct _AINPC_W* self)
{
    fn_802D3A2C(self, 2);
    fn_802D84D0(self, 2);
    switch (self->state) {
    case 0:
        self->state += 1;
        self->field_0x1AC = 6;
        fn_802D29F8(self, 0);
        fn_802D2904(self, 29, 6, 0);
        break;
    case 1: {
        s32 frames;

        if (fn_802D2984(self) != 1) {
            break;
        }
        frames = self->field_0x1AC - 1;
        self->field_0x1AC = frames;
        if (frames == 4) {
            fn_802D35B4(self, (s16)self->field_0x1F4);
            fn_802D3CFC(self, 1);
            fn_80114CC8(self, 0);
            break;
        }
        if (frames <= 0) {
            fn_802D2AD4(self, 0);
        }
        break;
    }
    }
}

/* The two-state approach dispatcher: it picks the near/far ring the flag names, arms a 450-frame
 * approach, and gives up when the target leaves the ring or the timer runs out. */
extern "C" void fn_802C6BE0(struct _AINPC_W* self, u32 flag)
{
    nw4r::math::VEC3 start;
    nw4r::math::VEC3 diff;
    u32 ang_a;
    u32 ang_b;
    f32 near_limit;
    f32 far_limit;
    s32 armed;

    VEC3_ctor(&start);
    near_limit = lbl_8079A670;
    far_limit = near_limit;
    armed = 0;
    switch (self->state) {
    case 0:
        self->state += 1;
        self->field_0x1AC = 450;
        switch (flag & 0xFF) {
        case 0:
        case 1:
            fn_802D2910(self, 8, 4, 0);
            break;
        case 2:
            fn_802D2910(self, 10, 2, 0);
            fn_802D29CC(self, lbl_8079A680);
            break;
        }
        fn_802D29F8(self, 0);
        break;
    case 1: {
        s32 limit;
        f32 distance;

        switch (flag & 0xFF) {
        case 0:
            near_limit = lbl_8079A6CC;
            far_limit = lbl_8079A6D0;
            break;
        case 1:
            near_limit = lbl_8079A6D4;
            far_limit = lbl_8079A6D8;
            break;
        case 2:
            near_limit = lbl_8079A68C;
            far_limit = lbl_8079A694;
            armed = 1;
            break;
        }
        limit = (flag & 0xFF) == 2 ? 10 : 90;
        distance = calcDistanceSqXZ(&self->vec_0x178, &self->vec_0x1B0);
        if (fn_802D3B34(self, (s32)limit, armed) == 1) {
            fn_802D2ABC(self, 1, 9, 0);
        } else if (distance <= near_limit * near_limit) {
            fn_802D2AD4(self, 0);
        } else if (distance >= far_limit * far_limit) {
            fn_802D2AD4(self, 0);
        } else if (--self->field_0x1AC < 0) {
            fn_802D2AD4(self, 0);
            break;
        }
        subVec3(&diff, &self->vec_0x1B0, &self->vec_0x178);
        copyVec3(&start, &diff);
        calcVecAngXY(&start, &ang_a, &ang_b);
        self->field_0x194 = fn_802D30F8((u16)ang_b, (u16)self->field_0x194, 2048);
        break;
    }
    }
}

/* The two-state guard dispatcher: it waits for the target to close inside the `+0x1BC` ring, then
 * aims the motion at it and reports the aim window through the `+0x194` angle selector. */
extern "C" void fn_802C6E3C(struct _AINPC_W* self, u32 flag)
{
    nw4r::math::VEC3 start;
    nw4r::math::VEC3 diff;
    u32 ang_a;
    u32 ang_b;
    f32 limit;
    s32 armed;
    s32 limit2;

    VEC3_ctor(&start);
    limit = lbl_8079A670;
    armed = 0;
    switch (self->state) {
    case 0:
        self->state += 1;
        self->field_0x1AC = 450;
        fn_802D29F8(self, 0);
        fn_802D2910(self, 11, 2, 0);
        break;
    case 1: {
        f32 distance;

        if ((flag & 0xFF) == 0) {
            limit = lbl_8079A6DC;
        }
        distance = calcDistanceSqXZ(&self->vec_0x178, &self->vec_0x1B0);
        if ((flag & 0xFF) == 2) {
            limit2 = 20;
            armed = 1;
        } else {
            limit2 = 90;
        }
        if (fn_802D3B34(self, limit2, armed) == 1) {
            fn_802D2ABC(self, 1, 9, 0);
        } else if (distance <= self->field_0x1BC * self->field_0x1BC) {
            fn_802D2AD4(self, 0);
            break;
        } else if (--self->field_0x1AC < 0) {
            fn_802D2AD4(self, 0);
            break;
        } else if ((flag & 0xFF) == 0) {
            if (distance >= limit * limit) {
                fn_802D2AD4(self, 0);
                break;
            }
        }
        subVec3(&diff, &self->vec_0x1B0, &self->vec_0x178);
        copyVec3(&start, &diff);
        calcVecAngXY(&start, &ang_a, &ang_b);
        self->field_0x194 = fn_802D30F8((u16)ang_b, (u16)self->field_0x194,
                                        distance > lbl_8079A6E0 ? 1536 : 2560);
        break;
    }
    }
}

/* The two-state guard dispatcher: it arms one of two motions on the flag, then steers the motion
 * while the target stays inside the 2048-unit aim window and the 90-frame guard lasts. */
extern "C" void fn_802C703C(struct _AINPC_W* self, u32 flag)
{
    nw4r::math::VEC3 start;
    nw4r::math::VEC3 diff;
    u32 ang_a;
    u32 ang_b;

    VEC3_ctor(&start);
    fn_802D29B8(self, 2);
    switch (self->state) {
    case 0:
        self->state += 1;
        switch (flag & 0xFF) {
        case 0:
            fn_802D29F8(self, 0);
            fn_802D2910(self, 8, 4, 0);
            break;
        case 1:
            fn_802D29F8(self, 2);
            fn_802D2910(self, 12, 4, 0);
            fn_802D31FC(self);
            self->field_0x1DC = lbl_8079A6E4;
            break;
        }
        self->field_0x1AC = 90;
        break;
    case 1:
        subVec3(&diff, &self->vec_0x1B0, &self->vec_0x178);
        copyVec3(&start, &diff);
        calcVecAngXY(&start, &ang_a, &ang_b);
        if ((flag & 0xFF) == 1) {
            f32 distance = calcDistanceSqXZ(&self->vec_0x178, &self->vec_0x1B0);

            self->field_0x194 =
                fn_802D30F8((u16)ang_b, (u16)self->field_0x194, distance > lbl_8079A6E0 ? 2048 : 6656);
            fn_802D3210(self, &self->field_0x190);
            self->field_0x190 = fn_802D30F8((u16)ang_a, (u16)self->field_0x190, 1536);
        } else {
            self->field_0x194 = fn_802D30F8((u16)ang_b, (u16)self->field_0x194, 2048);
        }
        if (self->field_0x194 - ang_b < 2048) {
            fn_802D2AD4(self, self->variant);
            break;
        }
        if (--self->field_0x1AC <= 0) {
            fn_802D2AD4(self, self->variant);
        }
        break;
    }
}

/* The two-state guard dispatcher: it arms a 1.3-scaled motion and a 450-frame guard, then gives up
 * as soon as the target closes inside the `+0x1BC` ring. */
extern "C" void fn_802C722C(struct _AINPC_W* self, u32 flag)
{
    nw4r::math::VEC3 start;
    nw4r::math::VEC3 diff;
    u32 ang_a;
    u32 ang_b;
    s32 armed;
    s32 limit;

    VEC3_ctor(&start);
    armed = 0;
    switch (self->state) {
    case 0:
        self->state += 1;
        self->field_0x1AC = 450;
        fn_802D29F8(self, 0);
        fn_802D2910(self, 11, 2, 0);
        fn_802D29CC(self, lbl_8079A6E8);
        break;
    case 1: {
        f32 distance = calcDistanceSqXZ(&self->vec_0x178, &self->vec_0x1B0);

        if ((flag & 0xFF) == 1) {
            limit = 20;
            armed = 1;
        } else {
            limit = 90;
        }
        if (fn_802D3B34(self, limit, armed) == 1) {
            fn_802D2ABC(self, 1, 9, 0);
        } else if (distance <= self->field_0x1BC * self->field_0x1BC) {
            fn_802D2AD4(self, 0);
            break;
        } else if (--self->field_0x1AC < 0) {
            fn_802D2AD4(self, 0);
            break;
        }
        subVec3(&diff, &self->vec_0x1B0, &self->vec_0x178);
        copyVec3(&start, &diff);
        calcVecAngXY(&start, &ang_a, &ang_b);
        self->field_0x194 = fn_802D30F8((u16)ang_b, (u16)self->field_0x194,
                                        distance > lbl_8079A6E0 ? 2048 : 4096);
        break;
    }
    }
}

/* The two-state dying/stagger dispatcher: it rolls one of two motions, then folds the aim every
 * frame until the target closes in or the 300-frame guard runs out. */
extern "C" void fn_802C73E4(struct _AINPC_W* self)
{
    nw4r::math::VEC3 start;
    nw4r::math::VEC3 diff;
    u32 ang_a;
    u32 ang_b;

    VEC3_ctor(&start);
    switch (self->state) {
    case 0:
        self->state += 1;
        self->field_0x1AC = 300;
        fn_802D29F8(self, 0);
        self->field_0x194 -= 16384;
        fn_802D2910(self, 200, 6, 0);
        fn_802D31FC(self);
        self->field_0x1D4 = lbl_8079A6EC;
        if (ran_suu(1) & 1) {
            fn_802D9D30(self, (s8)self->field_0x482, 6, 0, 2);
        } else {
            fn_802D9D30(self, (s8)self->field_0x482, 7, 0, 2);
        }
        break;
    case 1: {
        f32 distance;
        u16 selector;

        fn_802D3210(self, &self->field_0x190);
        distance = calcDistanceSqXZ(&self->vec_0x178, &self->vec_0x1B0);
        if (fn_802D3B34(self, 30, 1) == 1) {
            fn_802D2ABC(self, 1, 9, 0);
        } else if (distance <= self->field_0x1BC * self->field_0x1BC) {
            fn_802D2AD4(self, 0);
            break;
        } else if (--self->field_0x1AC < 0) {
            fn_802D2AD4(self, 0);
            break;
        }
        subVec3(&diff, &self->vec_0x1B0, &self->vec_0x178);
        copyVec3(&start, &diff);
        calcVecAngXY(&start, &ang_a, &ang_b);
        ang_b -= 16384;
        if (distance > lbl_8079A6E0) {
            selector = 1024;
        } else if (distance > lbl_8079A6F0) {
            selector = 2048;
        } else {
            selector = 4096;
        }
        self->field_0x194 = fn_802D30F8((u16)ang_b, (u16)self->field_0x194, selector);
        break;
    }
    }
}


#pragma peephole on

extern "C" {

/* 0x802CCD90 - the 0x350-frame gate: once the countdown has run out it latches +0x354 and arms the
 * 0x708-frame re-check. */
s32 fn_802CCD90(struct _AINPC_W* self)
{
    if (self->field_0x354 != 0) {
        return 1;
    }
    if (self->field_0x350 >= fn_802D9D14()) {
        self->field_0x354 = 1;
        self->field_0x352 = 0x708;
        fn_802D4218(self);
        return 1;
    }
    return 0;
}

/* 0x802CCE04 - true while the gate byte is exactly 2. */
s32 fn_802CCE04(struct _AINPC_W* self)
{
    return self->field_0x354 == 2;
}

/* 0x802CD75C - true while the +0x468 flag is set. */
u32 fn_802CD75C(struct _AINPC_W* self)
{
    return self->field_0x468 != 0;
}

/* 0x802CDF4C - queue the variant's intro command. */
void fn_802CDF4C(struct _AINPC_W* self)
{
    if (self->variant == 2) {
        fn_802D2A00(self, 1, 0xF, 0);
    } else {
        fn_802D2A00(self, 1, 8, 0);
    }
}

/* 0x802CDF78 - the 4-step "recover from stagger" handler. */
void fn_802CDF78(struct _AINPC_W* self)
{
    switch (self->step) {
    case 0:
        fn_802D4230(self, 150.0f);
        fn_802D2A00(self, 2, 0xF, 0);
        self->step += 1;
        break;
    case 1:
        if (fn_802D3184(self, 0x2000) == 0) {
            fn_802D2A00(self, 2, 0x10, 0);
            self->step += 1;
        } else {
            if ((u16)ran_suu(1) & 1) {
                fn_802D2A00(self, 3, 9, 0);
            } else {
                fn_802D2A00(self, 3, 0xD, 0);
            }
            self->step = 3;
        }
        break;
    case 2:
        if ((u16)ran_suu(1) & 1) {
            fn_802D2A00(self, 3, 9, 0);
        } else {
            fn_802D2A00(self, 3, 0xD, 0);
        }
        self->step += 1;
        break;
    case 3:
        fn_802D3B24(self);
        fn_802D4200(self);
        break;
    }
}

/* 0x802CE0D8 - the 3-step "approach" handler. */
void fn_802CE0D8(struct _AINPC_W* self)
{
    switch (self->step) {
    case 0:
        fn_802D4230(self, 400.0f);
        fn_802D2A00(self, 2, 3, 0);
        self->step += 1;
        break;
    case 1:
        fn_802D2A00(self, 1, 0xA, 0);
        self->step += 1;
        break;
    case 2:
        fn_802D4200(self);
        break;
    }
}

/* 0x802CE170 - rebuilds `vec_0x1B0` from `vec_0x178` rotated by the angle the two make (the rotated stack offset
 * `addVec3` turns into the new aim point), then queues the follow-up command. */
void fn_802CE170(struct _AINPC_W* self)
{
    nw4r::math::VEC3 tmp;

    VEC3_ctor(&tmp);
    switch (self->step) {
    case 0:
    {
        nw4r::math::VEC3 offset;
        s32 angle = calcVecAng2(&self->vec_0x1B0, &self->vec_0x178);

        setVector3(&tmp, 0.0f, 0.0f, 800.0f);
        rotVecY(&tmp, angle);
        addVec3(&offset, &self->vec_0x178, &tmp);
        copyVec3(&self->vec_0x1B0, &offset);
        self->field_0x389 = 0;
        fn_802D4230(self, 30.0f);
        fn_802D2A00(self, 3, 0x12, 0);
        self->step += 1;
        break;
    }
    case 1:
        fn_802CCF00(self, 0);
        fn_802D2A00(self, 2, 5, 0);
        self->field_0x389 = 1;
        self->step += 1;
        break;
    case 2:
        fn_802D2A00(self, 1, 0xA, 0);
        fn_802D4200(self);
        break;
    }
}

/* 0x802CF390 - the 3-step "hold/flinch" handler. */
void fn_802CF390(struct _AINPC_W* self)
{
    switch (self->step) {
    case 0:
        if (self->field_0x1C0 > 800.0f) {
            fn_802D4230(self, 30.0f);
            fn_802D2A00(self, 2, 7, 0);
        } else if (self->field_0x1C0 > 30.0f) {
            fn_802D4230(self, 30.0f);
            fn_802D2A00(self, 2, 3, 0);
        }
        self->step += 1;
        break;
    case 1:
        if (self->field_0x358 != 0) {
            fn_802D2A00(self, 1, 4, 0);
        } else if (self->field_0x35C != 0) {
            fn_802D2A00(self, 1, 0x1A, 0);
        }
        self->step += 1;
        break;
    case 2:
        fn_802D4200(self);
        break;
    }
}

/* 0x802CFABC - latch +0x3D2 and queue the variant's command. */
void fn_802CFABC(struct _AINPC_W* self)
{
    self->field_0x3D2 = 1;
    if (self->variant == 2) {
        fn_802D2A00(self, 5, 6, 0);
    } else {
        fn_802D2A00(self, 5, 5, 0);
    }
}

/* 0x802CFBC8 - queue the +0x440-dependent command. */
void fn_802CFBC8(struct _AINPC_W* self)
{
    if (self->field_0x440 == 1) {
        fn_802D2A00(self, 1, 0x54, 0);
    } else {
        fn_802D2A00(self, 1, 0x52, 0);
    }
}

/* 0x802CF704 - the 4-step "use item" handler. */
void fn_802CF704(struct _AINPC_W* self)
{
    switch (self->step) {
    case 0:
        if (self->field_0x1C0 > 800.0f) {
            fn_802D4230(self, 30.0f);
            fn_802D2A00(self, 2, 8, 0);
        } else {
            fn_802D4230(self, 30.0f);
            fn_802D2A00(self, 2, 4, 0);
        }
        self->step += 1;
        break;
    case 1:
        fn_802CCF00(self, 0);
        fn_802D2A00(self, 2, 5, 0);
        self->step += 1;
        break;
    case 2:
        fn_802D2A00(self, 1, 0xA, 0);
        self->step += 1;
        break;
    case 3:
        fn_802D4200(self);
        fn_802D4238(self);
        break;
    }
}

/* 0x802CFFD0 - the 3-step "guard" handler. */
void fn_802CFFD0(struct _AINPC_W* self)
{
    switch (self->step) {
    case 0:
        fn_802D4230(self, 300.0f);
        fn_802D2A00(self, 2, 4, 0);
        self->step += 1;
        break;
    case 1:
        fn_802D2A00(self, 3, 5, 0);
        self->step += 1;
        break;
    case 2:
        fn_802D4200(self);
        break;
    }
}

/* 0x802D07AC - the 3-step "roar" handler. */
void fn_802D07AC(struct _AINPC_W* self)
{
    switch (self->step) {
    case 0:
        fn_802D2A00(self, 2, 5, 0);
        self->step += 1;
        break;
    case 1:
        fn_802D2A00(self, 1, 0xA, 0);
        self->step += 1;
        break;
    case 2:
        fn_802D3B24(self);
        fn_802D4200(self);
        break;
    }
}

/* 0x802D08D4 - the 4-step "pick target" handler. */
void fn_802D08D4(struct _AINPC_W* self)
{
    switch (self->step) {
    case 0:
        fn_802D4230(self, 400.0f);
        fn_802D2A00(self, 2, 0xF, 0);
        self->step += 1;
        break;
    case 1:
        fn_802D2A00(self, 2, 0x10, 0);
        self->step += 1;
        break;
    case 2:
        fn_802D2A00(self, 1, 0x16, 0);
        self->step += 1;
        break;
    case 3:
        fn_802D4200(self);
        break;
    }
}

/* 0x802CD6FC - true once the +0x460 latch is set or the player work reports state 1. */
u32 fn_802CD6FC(struct _AINPC_W* self)
{
    if (self->field_0x460 == 1) {
        return 1;
    }
    if (fn_8027D74C(self->plw_0x16C) == 1) {
        self->field_0x460 = 1;
        return 1;
    }
    return 0;
}

/* 0x802CDAB8 - true while the elapsed frame count has not passed the formation's budget-scaled
 * factor. */
u32 fn_802CDAB8(struct _AINPC_W* self)
{
    return self->field_0x1F0 <= (s32)((f32)self->field_0x1F4 * self->formation_0x41C->budget_0x0C);
}

/* 0x802CDDC4 - the 4-step "switch to another action" handler. */
void fn_802CDDC4(struct _AINPC_W* self, u8 arg)
{
    switch (self->step) {
    case 0:
        fn_802D4230(self, 130.0f);
        if (arg == 0) {
            fn_802D2A00(self, 2, 4, 0);
        } else {
            fn_802D2A00(self, 3, 0x12, 0);
        }
        self->step += 1;
        break;
    case 1:
        if (fn_802D3184(self, 0x2000) == 0) {
            fn_802D2A00(self, 2, 5, 0);
            self->step += 1;
        } else {
            if (self->vec_0x1B0.y - self->vec_0x178.y > 180.0f) {
                fn_802D2A00(self, 3, 8, 0);
            } else {
                fn_802D32B4(self, lbl_805D4030[2]);
            }
            self->step = 3;
        }
        break;
    case 2:
        if (self->vec_0x1B0.y - self->vec_0x178.y > 180.0f) {
            fn_802D2A00(self, 3, 8, 0);
        } else {
            fn_802D32B4(self, lbl_805D4030[2]);
        }
        self->step += 1;
        break;
    case 3:
        fn_802D3B24(self);
        fn_802D4200(self);
        break;
    }
}

/* 0x802CE2A0 - the distance-banded dispatcher behind the "watch" action. */
void fn_802CE2A0(struct _AINPC_W* self)
{
    if (self->field_0x1C0 > 700.0f) {
        fn_802D4230(self, 300.0f);
        fn_802D2A00(self, 2, 7, 0);
        return;
    }
    if (self->field_0x1C0 > 400.0f) {
        fn_802D4230(self, 250.0f);
        fn_802D2A00(self, 2, 2, 0);
        return;
    }
    if (self->field_0x1C0 > 200.0f) {
        fn_802D2A00(self, 2, 0, 0);
        return;
    }
    if (fn_802D3184(self, 0x5000) == 0 && self->field_0x1C0 > 50.0f) {
        fn_802D2A00(self, 2, 5, 0);
        return;
    }
    if (fn_802CCE04(self) == 1) {
        fn_802D32B4(self, lbl_805D4030[3]);
        return;
    }
    fn_802D32B4(self, lbl_805D4030[0]);
}

/* 0x802CE618 - the distance-banded dispatcher behind the "hold ground" action. */
void fn_802CE618(struct _AINPC_W* self)
{
    if (self->field_0x1C0 > 700.0f) {
        fn_802D4230(self, 400.0f);
        fn_802D2A00(self, 2, 0xB, 0);
        return;
    }
    if (self->field_0x1C0 > 300.0f) {
        fn_802D4230(self, 150.0f);
        fn_802D2A00(self, 2, 0xA, 0);
    }
}

/* 0x802D0068 - the 4-step "turn" handler. */
void fn_802D0068(struct _AINPC_W* self, u8 arg)
{
    switch (self->step) {
    case 0:
        fn_802D4230(self, 800.0f);
        fn_802D2A00(self, 2, 4, 0);
        self->step += 1;
        break;
    case 1:
        fn_802D2A00(self, 2, 5, 0);
        self->step += 1;
        break;
    case 2:
        if (arg == 0) {
            fn_802D2A00(self, 3, 6, 0);
        } else {
            fn_802D2A00(self, 3, 7, 0);
        }
        self->step += 1;
        break;
    case 3:
        fn_802D4200(self);
        break;
    }
}

/* 0x802D0148 - the 4-step "turn" handler, the long variant. */
void fn_802D0148(struct _AINPC_W* self, u8 arg)
{
    switch (self->step) {
    case 0:
        fn_802D4230(self, 1000.0f);
        fn_802D2A00(self, 2, 0xF, 0);
        self->step += 1;
        break;
    case 1:
        fn_802D2A00(self, 2, 0x10, 0);
        self->step += 1;
        break;
    case 2:
        if (arg == 0) {
            fn_802D2A00(self, 3, 0xF, 0);
        } else {
            fn_802D2A00(self, 3, 0x10, 0);
        }
        self->step += 1;
        break;
    case 3:
        fn_802D4200(self);
        break;
    }
}

/* 0x802D0228 - the 3-step "ready stance" handler. */
void fn_802D0228(struct _AINPC_W* self)
{
    switch (self->step) {
    case 0:
        fn_802D4230(self, 800.0f);
        fn_802D2A00(self, 2, 3, 0);
        self->step += 1;
        break;
    case 1:
        fn_802D2A00(self, 3, 0x13, 0);
        self->step += 1;
        break;
    case 3:
        fn_802D4200(self);
        break;
    }
}

/* 0x802D02C0 - the 3-step "ready stance" handler, the long variant. */
void fn_802D02C0(struct _AINPC_W* self)
{
    switch (self->step) {
    case 0:
        fn_802D4230(self, 800.0f);
        fn_802D2A00(self, 2, 0xF, 0);
        self->step += 1;
        break;
    case 1:
        fn_802D2A00(self, 3, 0x14, 0);
        self->step += 1;
        break;
    case 3:
        fn_802D4200(self);
        break;
    }
}

/* 0x802D0358 - the 2-step "sidestep" handler: it rotates the +0x178 aim-offset into +0x1B0. */
void fn_802D0358(struct _AINPC_W* self, u8 arg)
{
    nw4r::math::VEC3 v;

    VEC3_ctor(&v);
    switch (self->step) {
    case 0:
    {
        s32 angle;

        fn_802CCF00(self, 0);
        angle = calcVecAng2(&self->vec_0x1B0, &self->vec_0x178);
        setVector3(&v, 0.0f, 0.0f, 800.0f);
        if (arg == 0) {
            setVector3(&v, 0.0f, 0.0f, fn_802D7258(self, 0));
        } else {
            setVector3(&v, 0.0f, 0.0f, fn_802D7258(self, 1));
        }
        rotVecY(&v, angle);
        self->vec_0x1B0.x = self->vec_0x178.x + v.x;
        self->vec_0x1B0.y = self->vec_0x178.y;
        self->vec_0x1B0.z = self->vec_0x178.z + v.z;
        fn_802D4230(self, 30.0f);
        if (arg == 0) {
            fn_802D2A00(self, 1, 6, 0);
        } else {
            fn_802D2A00(self, 1, 0x10, 0);
        }
        self->step += 1;
        break;
    }
    case 1:
        fn_802D4200(self);
        fn_802D3A20(self);
        break;
    }
}

/* 0x802D04CC - the 4-step "step in" handler. */
void fn_802D04CC(struct _AINPC_W* self)
{
    nw4r::math::VEC3 v;

    VEC3_ctor(&v);
    switch (self->step) {
    case 0:
        if (self->field_0x1C0 == 400.0f) {
            s32 angle = calcVecAng2(&self->vec_0x1B0, &self->vec_0x178);

            setVector3(&v, 0.0f, 0.0f, 800.0f);
            rotVecY(&v, angle);
            self->vec_0x1B0.x = self->vec_0x178.x + v.x;
            self->vec_0x1B0.y = self->vec_0x178.y;
            self->vec_0x1B0.z = self->vec_0x178.z + v.z;
            fn_802D4230(self, 20.0f);
            fn_802D2A00(self, 2, 4, 0);
        }
        self->field_0x389 = 0;
        self->step += 1;
        break;
    case 1:
        fn_802CCF00(self, 1);
        self->field_0x389 = 1;
        fn_802D2A00(self, 2, 5, 0);
        self->step += 1;
        break;
    case 2:
        fn_802D2A00(self, 3, 0x15, 0);
        self->step += 1;
        break;
    case 3:
        fn_802D3B24(self);
        fn_802D4200(self);
        break;
    }
}

/* 0x802D063C - the 4-step "step in" handler, the far variant. */
void fn_802D063C(struct _AINPC_W* self)
{
    nw4r::math::VEC3 v;

    VEC3_ctor(&v);
    switch (self->step) {
    case 0:
        if (self->field_0x1C0 == 400.0f) {
            s32 angle = calcVecAng2(&self->vec_0x1B0, &self->vec_0x178);

            setVector3(&v, 0.0f, 0.0f, 800.0f);
            rotVecY(&v, angle);
            self->vec_0x1B0.x = self->vec_0x178.x + v.x;
            self->vec_0x1B0.y = self->vec_0x178.y;
            self->vec_0x1B0.z = self->vec_0x178.z + v.z;
            fn_802D4230(self, 20.0f);
            fn_802D2A00(self, 2, 0xF, 0);
        }
        self->field_0x389 = 0;
        self->step += 1;
        break;
    case 1:
        fn_802CCF00(self, 1);
        self->field_0x389 = 1;
        fn_802D2A00(self, 2, 0x10, 0);
        self->step += 1;
        break;
    case 2:
        fn_802D2A00(self, 3, 0x16, 0);
        self->step += 1;
        break;
    case 3:
        fn_802D3B24(self);
        fn_802D4200(self);
        break;
    }
}

/* 0x802D0840 - the 3-step "roar" handler. */
void fn_802D0840(struct _AINPC_W* self)
{
    switch (self->step) {
    case 0:
        fn_802D2A00(self, 2, 0x10, 0);
        self->step += 1;
        break;
    case 1:
        fn_802D2A00(self, 1, 0x16, 0);
        self->step += 1;
        break;
    case 2:
        fn_802D3B24(self);
        fn_802D4200(self);
        break;
    }
}

/* 0x802D0994 - the 2-step "post-roar" handler. */
void fn_802D0994(struct _AINPC_W* self)
{
    switch (self->step) {
    case 0:
        if (self->variant == 2) {
            fn_802D2A00(self, 1, 0x16, 0);
        } else {
            fn_802D2A00(self, 1, 0xA, 0);
        }
        self->step += 1;
        break;
    case 1:
        fn_802D4200(self);
        fn_802D3AD8(self);
        break;
    }
}

/* 0x802D0A20 - the 4-step "taunt" handler. */
void fn_802D0A20(struct _AINPC_W* self)
{
    switch (self->step) {
    case 0:
        fn_802D4230(self, 30.0f);
        fn_802D2A00(self, 2, 4, 0);
        self->step += 1;
        break;
    case 1:
        fn_802CCF00(self, 0);
        fn_802D4230(self, 500.0f);
        fn_802D2A00(self, 2, 4, 0);
        self->step += 1;
        break;
    case 2:
        if (fn_80278310(self->field_0x1A4, &self->vec_0x178, self->field_0x326) == 1) {
            fn_802D2A00(self, 1, 0x1F, 0);
        }
        self->field_0x448 += 1;
        self->step += 1;
        break;
    case 3:
        fn_802D4200(self);
        fn_802D4238(self);
        break;
    }
}

/* 0x802D0B28 - the 4-step "taunt" handler, the long variant. */
void fn_802D0B28(struct _AINPC_W* self)
{
    switch (self->step) {
    case 0:
        fn_802D4230(self, 30.0f);
        fn_802D2A00(self, 2, 0xF, 0);
        self->step += 1;
        break;
    case 1:
        fn_802CCF00(self, 0);
        fn_802D4230(self, 500.0f);
        fn_802D2A00(self, 2, 0xF, 0);
        self->step += 1;
        break;
    case 2:
        if (fn_80278310(self->field_0x1A4, &self->vec_0x178, self->field_0x326) == 1) {
            fn_802D2A00(self, 1, 0x3B, 0);
        }
        self->field_0x448 += 1;
        self->step += 1;
        break;
    case 3:
        fn_802D4200(self);
        fn_802D4238(self);
        break;
    }
}

/* 0x802CD20C - the player-action predicate chain the state machines test. */
s16 fn_802CD20C(struct _AINPC_W* self)
{
    struct _PLW* plw = self->plw_0x16C;

    if (Pl_condition_ck(plw, 0x30A) == 1) {
        return 0;
    }
    if (Pl_condition_ck(plw, 4) == 1) {
        return 1;
    }
    if (Pl_condition_ck(plw, 0x40000) == 1) {
        return ai_torch_ck(self) != 1;
    }
    if (fn_8027E06C(plw, 0) == 1) {
        return 0;
    }
    return fn_8027E06C(plw, 1) != 1;
}

/* 0x802CD2E0 - latch the +0x34D gate once the predicate chain answers non-negative. */
u32 fn_802CD2E0(struct _AINPC_W* self)
{
    s16 r;

    if (self->field_0x34D != 0) {
        return 1;
    }
    r = fn_802CD20C(self);
    if (r >= 0) {
        self->field_0x34D = 1;
        self->field_0x34E = (u8)r;
        return 1;
    }
    return 0;
}

/* 0x802CD588 - the stance/guard dispatcher. */
u32 fn_802CD588(struct _AINPC_W* self)
{
    u16 a;
    s16 b;

    if (self->field_0x461 == 1) {
        return 1;
    }
    if (self->field_0x43E <= 0) {
        self->field_0x43D = 0;
    }
    if (self->field_0x171 == 5) {
        return 0;
    }
    if (self->field_0x43D == 1) {
        self->field_0x43D = 2;
        fn_802D4218(self);
        switch (self->field_0x420) {
        case 3:
        case 4:
            if (fn_802D86D4(self, &a, &b) == 1) {
                if (a != 0) {
                    self->field_0x462 = a;
                    self->field_0x464 = b;
                    self->field_0x461 = 1;
                    fn_802D9D30(self, (s8)self->field_0x482, 0x12, 0, 2);
                }
            } else {
                fn_802D9D30(self, (s8)self->field_0x482, 0x11, 0, 2);
            }
            break;
        case 1:
            fn_802D65CC(self);
            break;
        case 2:
            fn_802D675C(self);
            break;
        case 5:
            fn_802D67F8(self);
            break;
        default:
            fn_802D9D30(self, (s8)self->field_0x482, 0x11, 0, 2);
            break;
        }
    }
    return self->field_0x43D != 0;
}

/* 0x802CE3B4 - the 4-step "flinch" handler. */
void fn_802CE3B4(struct _AINPC_W* self)
{
    switch (self->step) {
    case 0:
        if (self->field_0x1C0 > 180.0f) {
            fn_802D4230(self, 180.0f);
            fn_802D2A00(self, 2, 8, 0);
        }
        self->step += 1;
        break;
    case 1:
        if (fn_802CD20C(self) < 0) {
            fn_802D3B10(self);
            fn_802D4200(self);
        } else {
            fn_802D2A00(self, 2, 5, 0);
            self->step += 1;
        }
        break;
    case 2:
        if (self->field_0x34E == 0) {
            fn_802D2A00(self, 3, 0, 4);
        } else {
            fn_802D2A00(self, 3, 1, 4);
        }
        fn_802D9D30(self, (s8)self->field_0x482, 0x1B, 0, 2);
        self->step += 1;
        break;
    case 3:
        fn_802D3B10(self);
        fn_802D4200(self);
        break;
    }
}

/* 0x802CE4EC - the 4-step "flinch" handler, the long variant. */
void fn_802CE4EC(struct _AINPC_W* self)
{
    switch (self->step) {
    case 0:
        fn_802D4230(self, 180.0f);
        fn_802D2A00(self, 2, 0xB, 0);
        self->step += 1;
        break;
    case 1:
        if (fn_802CD20C(self) < 0) {
            fn_802D3B10(self);
            fn_802D4200(self);
        } else {
            fn_802D2A00(self, 2, 0x10, 0);
            self->step += 1;
        }
        break;
    case 2:
        if (self->field_0x34E == 0) {
            fn_802D2A00(self, 3, 9, 4);
        } else {
            fn_802D2A00(self, 3, 0xD, 4);
        }
        fn_802D9D30(self, (s8)self->field_0x482, 0x1B, 0, 2);
        self->step += 1;
        break;
    case 3:
        fn_802D3B10(self);
        fn_802D4200(self);
        break;
    }
}

/* 0x802CF48C - the 3-step "call" handler. */
void fn_802CF48C(struct _AINPC_W* self)
{
    switch (self->step) {
    case 0:
        if (self->field_0x1C0 > 800.0f) {
            fn_802D4230(self, 30.0f);
            fn_802D2A00(self, 2, 0xB, 0);
        } else if (self->field_0x1C0 > 30.0f) {
            fn_802D4230(self, 30.0f);
            fn_802D2A00(self, 2, 0xE, 0);
        }
        self->step += 1;
        break;
    case 1:
        if (self->field_0x358 != 0) {
            fn_802D2A00(self, 1, 0xE, 0);
        } else if (self->field_0x35C != 0) {
            fn_802D2A00(self, 1, 0x34, 0);
        }
        self->step += 1;
        break;
    case 2:
        fn_802D4200(self);
        break;
    }
}

/* 0x802CF8E4 - the 4-step "call" handler, the long variant. */
void fn_802CF8E4(struct _AINPC_W* self)
{
    switch (self->step) {
    case 0:
        if (self->field_0x1C0 > 800.0f) {
            fn_802D4230(self, 30.0f);
            fn_802D2A00(self, 2, 0xF, 0);
        } else {
            fn_802D4230(self, 50.0f);
            fn_802D2A00(self, 2, 0xE, 0);
        }
        self->step += 1;
        break;
    case 1:
        fn_802CCF00(self, 0);
        fn_802D2A00(self, 2, 0x10, 0);
        self->step += 1;
        break;
    case 2:
        fn_802D2A00(self, 1, 0x16, 0);
        self->step += 1;
        break;
    case 3:
        fn_802D4200(self);
        fn_802D4238(self);
        break;
    }
}

/* 0x802CFBF4 - the 3-step "call" handler, the short variant. */
void fn_802CFBF4(struct _AINPC_W* self)
{
    switch (self->step) {
    case 0:
        fn_802D4230(self, 300.0f);
        fn_802D2A00(self, 2, 0xF, 0);
        self->step += 1;
        break;
    case 1:
        fn_802D2A00(self, 3, 0xE, 0);
        self->step += 1;
        break;
    case 2:
        fn_802D3B24(self);
        fn_802D4200(self);
        break;
    }
}

/* 0x802CFC94 - the 4-step "sidestep" handler. */
void fn_802CFC94(struct _AINPC_W* self)
{
    nw4r::math::VEC3 tmp;
    nw4r::math::VEC3 offset;

    VEC3_ctor(&tmp);
    switch (self->step) {
    case 0:
        if (self->field_0x1C0 > 800.0f) {
            fn_802D4230(self, 800.0f);
            fn_802D2A00(self, 2, 4, 0);
        } else {
            s32 angle = calcVecAng2(&self->vec_0x1B0, &self->vec_0x178);

            setVector3(&tmp, 0.0f, 0.0f, 800.0f);
            rotVecY(&tmp, angle);
            addVec3(&offset, &self->vec_0x178, &tmp);
            copyVec3(&self->vec_0x1B0, &offset);
            self->field_0x389 = 0;
            fn_802D4230(self, 20.0f);
            fn_802D2A00(self, 2, 4, 0);
        }
        self->step += 1;
        break;
    case 1:
        fn_802CCF00(self, 0);
        fn_802D2A00(self, 2, 5, 0);
        self->field_0x389 = 1;
        self->step += 1;
        break;
    case 2:
        fn_802D2A00(self, 1, 0x39, 0);
        self->step += 1;
        break;
    case 3:
        fn_802D2A00(self, 1, 0xA, 0);
        fn_802D4200(self);
        break;
    }
}

/* 0x802CFE20 - the 4-step "sidestep" handler, the far variant. */
void fn_802CFE20(struct _AINPC_W* self)
{
    nw4r::math::VEC3 tmp;
    nw4r::math::VEC3 delta;
    nw4r::math::VEC3 offset;
    u32 x, z;

    VEC3_ctor(&tmp);
    switch (self->step) {
    case 0:
        if (self->field_0x1C0 > 800.0f) {
            fn_802D4230(self, 1100.0f);
            fn_802D2A00(self, 2, 0xF, 0);
        } else {
            subVec3(&delta, &self->vec_0x178, &self->vec_0x1B0);
            copyVec3(&tmp, &delta);
            calcVecAngXY(&tmp, &x, &z);
            setVector3(&tmp, 0.0f, 0.0f, 800.0f);
            rotVecX(&tmp, x);
            rotVecY(&tmp, (u16)z);
            addVec3(&offset, &self->vec_0x178, &tmp);
            copyVec3(&self->vec_0x1B0, &offset);
            self->field_0x389 = 0;
            fn_802D4230(self, 20.0f);
            fn_802D2A00(self, 2, 0xF, 0);
        }
        self->step += 1;
        break;
    case 1:
        fn_802CCF00(self, 0);
        fn_802D2A00(self, 2, 0x10, 0);
        self->field_0x389 = 1;
        self->step += 1;
        break;
    case 2:
        fn_802D2A00(self, 1, 0x3A, 0);
        self->step += 1;
        break;
    case 3:
        fn_802D2A00(self, 1, 0x16, 0);
        fn_802D4200(self);
        break;
    }
}

/* 0x802D0C30 - the 2-step "sleep" handler. */
void fn_802D0C30(struct _AINPC_W* self)
{
    switch (self->step) {
    case 0:
        fn_802D2A00(self, 1, 0x5A, 0);
        self->step += 1;
        break;
    case 1:
        self->field_0x485 = 0;
        fn_802D4200(self);
        break;
    }
}

#ifdef __cplusplus
}
#endif


extern "C" void fn_802D0DCC(DispatchState* self)
{
    switch (self->state) {
    default:
        if (self->variant == 2) {
            fn_802D3398(self, lbl_805D4150[15]);
        } else {
            fn_802D3398(self, lbl_805D4150[14]);
        }
        break;
    case 1:
        if (self->variant == 2) {
            fn_802D3398(self, lbl_805D4150[3]);
        } else {
            fn_802D3398(self, lbl_805D4150[2]);
        }
        break;
    case 2:
        if (self->variant == 2) {
            fn_802D3398(self, lbl_805D4150[5]);
        } else {
            fn_802D3398(self, lbl_805D4150[4]);
        }
        break;
    case 3:
        if (self->variant == 2) {
            fn_802D3398(self, lbl_805D4150[7]);
        } else {
            fn_802D3398(self, lbl_805D4150[6]);
        }
        break;
    case 4:
        if (self->variant == 2) {
            fn_802D3398(self, lbl_805D4150[9]);
        } else {
            fn_802D3398(self, lbl_805D4150[8]);
        }
        break;
    case 5:
        if (self->variant == 2) {
            fn_802D3398(self, lbl_805D4150[11]);
        } else {
            fn_802D3398(self, lbl_805D4150[10]);
        }
        break;
    case 6:
        if (self->variant == 2) {
            fn_802D3398(self, lbl_805D4150[13]);
        } else {
            fn_802D3398(self, lbl_805D4150[12]);
        }
        break;
    }
}

/* The C++-linkage callees the view below calls: declared at global scope so the calls resolve to the map's
 * manglings (a declaration inside the view namespace would mangle the namespace in). */
#include "ef/get_move_work_adrs.h"
#include "ef/pRoot.h"
struct _HIT_W;
void hit_flag_set(struct _HIT_W* hit, u32 flags);
u16 ai_get_motion_no(struct _AINPC_W* self);

namespace view_fn_802D0F34 {

namespace nw4r = ::nw4r;  /* the view headers below reopen it */

#include "ai/ai_npc.h"
/* data keeps its map name (a view namespace would mangle it): the header is read with C linkage */
extern "C" {
#include "ai/fn_802D0F34.h"
}

/* One entry of the motion table `lbl_805D3AD8` (`fn_802D2F7C` walks it with a 0x1A stride): the two
 * s16 distances and the u8 motion row `lbl_80792508` is indexed with. size: 0x1A */
struct MotionEntry {
    /* +0x00 */ s16 approach_0x00;
    /* +0x02 */ s16 retreat_0x02;
    /* +0x04 */ u8 unused_0x04[0x0F - 0x04];
    /* +0x0F */ u8 motion_id_0x0F;
    /* +0x10 */ u8 unused_0x10[0x1A - 0x10];
};

extern "C" {

/* 0x802D0F34 - the attack driver: the counter-attack countdown, the "blocked" arm and the whole
 * attack-plan dispatch on +0x1CC. */
void fn_802D0F34(struct _AINPC_W* self)
{
    u8* enemy_work = (u8*)::get_move_work_adrs(3);

    if (self->motion != 7 &&
        (fn_802D948C(self) != 1 || self->area == self->plw_0x16C->area_0x16 ||
         self->field_0x485 == 1)) {
        if (self->field_0x1D1 != 0) {
            self->field_0x35C = 0;
            self->field_0x358 = 0;
            fn_802D3A20(self);
            if (self->field_0x1D1 == 1) {
                fn_802D4200(self);
                self->field_0x389 = 1;
                fn_802D41C8(self);
            }
            self->field_0x1D1 = 0;
        }
        if (fn_802D3DE8(self) != 1) {
            if (self->variant == 2) {
                self->field_0x1C0 = fn_80050EF4(&self->pos_0x1B0, &self->pos_0x178);
            } else {
                self->field_0x1C0 = calcVecDistXZ(&self->pos_0x1B0, &self->pos_0x178);
            }
            if (self->field_0x1CD != 0) {
                if (self->field_0x1CF != 0) {
                    fn_802D4238(self);
                    switch (self->field_0x1CC) {
                    case 0:
                        if (self->field_0x354 == 1) {
                            fn_802D41E0(self, 2, 0);
                            self->field_0x354 = 2;
                        } else if (self->field_0x354 == 3) {
                            fn_802D41E0(self, 0x33, 1);
                        } else if (self->variant == 2) {
                            if (self->field_0x460 == 1) {
                                fn_802D41E0(self, 0x29, 1);
                            } else if (self->field_0x461 == 1) {
                                fn_802D41E0(self, 0x2A, 1);
                            } else if (self->field_0x468 != 0) {
                                fn_802D41E0(self, 0x2C, 1);
                            } else if (self->field_0x34D == 1) {
                                fn_802D41E0(self, 5, 1);
                            } else {
                                fn_802D41E0(self, 0x0A, 0);
                            }
                        } else if (self->field_0x460 == 1) {
                            fn_802D41E0(self, 0x27, 1);
                        } else if (self->field_0x461 == 1) {
                            fn_802D41E0(self, 0x28, 1);
                        } else if (self->field_0x468 != 0) {
                            fn_802D41E0(self, 0x2B, 1);
                        } else if (self->field_0x440 != 0) {
                            fn_802D41E0(self, 0x13, 0);
                        } else if (self->field_0x34D == 1) {
                            fn_802D41E0(self, 4, 1);
                        } else {
                            fn_802D41E0(self, 3, 0);
                        }
                        break;
                    case 1:
                        switch (self->field_0x354) {
                        case 1:
                            self->field_0x1CE = 2;
                            self->field_0x354 = 2;
                            break;
                        case 2:
                            fn_802D0DCC(self);
                            break;
                        case 3:
                            fn_802D41E0(self, 0x33, 1);
                            break;
                        default:
                            if (self->variant == 2) {
                                if (self->field_0x420 == 6) {
                                    fn_802D3398(self, (u16*)lbl_805D4150[1]);
                                } else if (self->field_0x420 == 5) {
                                    fn_802D41E0(self, 0x2E, 1);
                                } else if (self->field_0x384 == 1) {
                                    fn_802D41E0(self, 0x24, 1);
                                } else {
                                    fn_802D41E0(self, 0x0D, 1);
                                }
                            } else {
                                if (self->field_0x420 == 6) {
                                    fn_802D3398(self, (u16*)lbl_805D4150[0]);
                                } else if (self->field_0x420 == 5) {
                                    u8* enemy = enemy_work;

                                    if (self->enemy_index != 0xFF) {
                                        enemy += self->enemy_index * 0xB18;
                                    }
                                    if ((u8)(enemy[0x1E5] + 0xF8) <= 1) {
                                        fn_802D41E0(self, 0x19, 1);
                                    } else {
                                        fn_802D41E0(self, 0x2D, 1);
                                    }
                                } else if (self->field_0x384 == 1) {
                                    fn_802D41E0(self, 0x23, 1);
                                } else {
                                    fn_802D41E0(self, 1, 1);
                                }
                            }
                            break;
                        }
                        break;
                    case 2:
                    case 3:
                        if (self->variant == 2) {
                            fn_802D41E0(self, 0x11, 1);
                        } else {
                            fn_802D41E0(self, 6, 1);
                        }
                        break;
                    case 4:
                        if (self->variant == 2) {
                            if (self->field_0x447 == 2) {
                                self->field_0x447 = 1;
                                fn_802D41E0(self, 0x26, 1);
                            } else if (self->field_0x3DE == 1) {
                                fn_802D41E0(self, 0x12, 1);
                            } else if (self->field_0x34F == 1) {
                                fn_802D41E0(self, 0x20, 1);
                            } else {
                                fn_802D41E0(self, 0x0C, 1);
                            }
                        } else if (self->field_0x447 == 2) {
                            self->field_0x447 = 1;
                            fn_802D41E0(self, 0x25, 1);
                        } else if (self->field_0x3DE == 1) {
                            fn_802D41E0(self, 9, 1);
                        } else if (self->field_0x34F == 1) {
                            fn_802D41E0(self, 0x1F, 1);
                        } else {
                            fn_802D41E0(self, 8, 1);
                        }
                        self->field_0x376 = 1;
                        break;
                    case 5:
                        if (self->variant == 2) {
                            if (self->field_0x3DE == 1) {
                                fn_802D41E0(self, 0x12, 1);
                            } else {
                                fn_802D41E0(self, 0x0B, 1);
                            }
                        } else if (self->field_0x3DE == 1) {
                            fn_802D41E0(self, 9, 1);
                        } else {
                            fn_802D41E0(self, 7, 1);
                        }
                        self->field_0x376 = 1;
                        break;
                    case 6:
                        self->field_0x1CE = 0x10;
                        break;
                    case 7:
                        if (self->variant == 2) {
                            fn_802D41E0(self, 0x22, 1);
                        } else {
                            fn_802D41E0(self, 0x21, 1);
                        }
                        break;
                    case 8:
                        fn_802D41E0(self, 0x2F, 1);
                        break;
                    case 9:
                        if (self->variant == 2) {
                            fn_802D41E0(self, 0x32, 1);
                        } else {
                            fn_802D41E0(self, 0x31, 1);
                        }
  
                        break;
                    }
                }
                fn_802D0C9C(self);
                fn_802D41D4(self);
            }
        }
    }
}

/* 0x802D15DC - the move solver: fills the two probe vectors, walks the ~0x20-radius band and hands
 * the result to the collision query. */
void fn_802D15DC(struct _AINPC_W* self)
{
    nw4r::math::VEC3 probe;
    nw4r::math::VEC3 current;
    u16 flags = 0x20;
    s32 blocked = 0;

    VEC3_ctor(&probe);
    VEC3_ctor(&current);
    if (self->field_0x3CA > 0) {
        self->field_0x3C8 = 0;
        return;
    }
    if (fn_802D2B38(self, 2, 9) == 1 || fn_802D2B38(self, 2, 0x13) == 1) {
        blocked = 1;
    }
    if (fn_802D7B24(self) == 1) {
        blocked = 1;
    }
    if ((u32)(self->motion - 4) > 1 && blocked == 0) {
        flags = 0x20 | 0xC0;
    }
    copyVec3(&probe, &self->pos_0x184);
    probe.y = probe.y + lbl_8079A7A8;
    copyVec3(&current, &self->pos_0x178);
    current.y = current.y + lbl_8079A7A8;
    self->field_0x3C8 = (s8)fn_8029208C(&probe, &current, &self->pos_0x178, 0xFFFF, flags, 1,
                                        self->area, lbl_8079A7A8);
}

/* 0x802D1710 - the step-height/ground probe: walks the two forward probes and re-seeds the motion
 * when the NPC has to step up or down. */
void fn_802D1710(struct _AINPC_W* self)
{
    nw4r::math::VEC3 base;
    f32 value;

    fn_8012A624(&base);
    self->field_0x3CE = 0;
    if (self->field_0x3D0 <= 0 && fn_802D2B78(self, 2) == 0) {
        if (fn_80291B08(self, &self->pos_0x178, &self->field_0x324, &value, -6) == 1) {
            self->field_0x19C = value;
            if (self->variant == 0) {
                if (self->pos_0x178.y - value <= lbl_8079A6A8) {
                    self->pos_0x178.y = value;
                } else {
                    fn_802D2A00(self, 6, 0, 0);
                }
            } else if (self->variant == 2) {
                if (fn_802D2B38(self, 2, 0x0D) != 1) {
                    fn_802D2A00(self, 2, 0x0D, 0);
                    fn_802D29F8(self, 0);
                    fn_802D4200(self);
                }
            }
        } else if (fn_80291B08(self, &self->pos_0x178, &self->field_0x324, &value, 1) == 1) {
            self->field_0x19C = value;
            if (self->variant == 2) {
                if (self->pos_0x178.y > lbl_8079A6B0) {
                    self->pos_0x178.y = lbl_8079A6B0;
                    self->field_0x3CE = 1;
                } else if (self->pos_0x178.y < lbl_8079A754 + value) {
                    self->pos_0x178.y = lbl_8079A754 + value;
                    self->field_0x3CE = 1;
                }
            } else {
                fn_802D2A00(self, 2, 0x0C, 0);
                fn_802D29F8(self, 2);
                fn_802D4200(self);
            }
        }
        if (fn_80291B08(self, &self->pos_0x178, &base, &value, 4) == 1) {
            self->field_0x1A0 = value;
        }
    }
}

/* 0x802D18E8 - mirrors the model's visibility flag onto the engine model when it moved. */
void fn_802D18E8(struct _AINPC_W* self)
{
    u8 visible = self->field_0x417;

    if (self->field_0x418 != visible) {
        if (visible == 1) {
            self->model.setVisibility(0x15, false);
        } else {
            self->model.setVisibility(0x15, true);
        }
    }
    self->field_0x418 = self->field_0x417;
}

/* 0x802D1954 - re-seeds the NPC from the player work it targets: area, position, motion and the
 * whole attack bookkeeping. */
void fn_802D1954(struct _AINPC_W* self)
{
    _PLW* plw = self->plw_0x16C;
    nw4r::math::VEC3 offset;
    nw4r::math::VEC3 position;

    VEC3_ctor(&offset);
    if (fn_802D948C(self) != 1) {
        if (fn_802D94A0(self) == 1 && plw->area_0x16 == 0) {
            self->field_0x486 = 0x5A;
        }
        self->area = plw->area_0x16;
        eft_rot_vec_copy(&self->vec_0x190, (_CP_VECTOR*)&plw->param_0x54);
        setVector3(&offset, lbl_8079A7AC, lbl_8079A670, lbl_8079A7B0);
        rotVecY(&offset, self->vec_0x190.y);
        addVec3(&position, &plw->vec_0x03C, &offset);
        copyVec3(&self->pos_0x184, &position);
        copyVec3(&self->pos_0x178, &self->pos_0x184);
        if (plw->kind_0x09 != 3) {
            fn_802D29F8(self, 0);
        } else {
            fn_802D29F8(self, 2);
        }
        if (self->motion == 5) {
            if ((u32)(self->motion_step - 5) <= 1) {
                fn_802D2A00(self, 0, 0, 0);
            }
        } else {
            fn_802D2AD4(self, self->variant);
        }
        fn_802D1710(self);
        self->enemy_index = 0xFF;
        self->field_0x1C8 = 0;
        fn_802D4200(self);
        self->field_0x389 = 1;
        fn_802D41C8(self);
        fn_802D3D84(self);
        self->field_0x383 = 1;
        self->field_0x447 = fn_802D8550(self);
        self->field_0x448 = 0;
        self->field_0x38A = 0;
        self->field_0x440 = 0;
        self->model.scale_0x1C.x = lbl_8079A674;
        self->model.scale_0x1C.y = lbl_8079A674;
        self->model.scale_0x1C.z = lbl_8079A674;
        fn_802D1FFC(self);
        fn_800E0914(&self->model);
    }
}

/* 0x802D1AFC - the weak-point roll: after the counter-attack window it picks one of the five
 * weak-point rates, bumps it and books the two hit effects. */
void fn_802D1AFC(struct _AINPC_W* self)
{
    u16 id;
    s16 count;

    if (self->field_0x420 == 3) {
        if (fn_802D86D4(&id, &count) == 1) {
            if (self->field_0x422 <= 0 && self->field_0x461 == 0) {
                u8 slot;

                self->field_0x422 = 0x384;
                slot = ::ran_suu(1) % 5;
                self->field_0x426 = slot;
                if (::ran_suu(1) % 100 < self->weak_point_0x427[slot]) {
                    slot = ::ran_suu(1) % 5;
                    self->field_0x426 = slot;
                }
                self->weak_point_0x427[slot] = self->weak_point_0x427[slot] + 0x0A;
                if (self->weak_point_0x427[slot] >= 0x64) {
                    self->weak_point_0x427[slot] = 0x64;
                }
                fn_802D40EC(self, id, -count);
                if (self->field_0x42C != 0) {
                    fn_802D40EC(self, fn_802D89D4(self), count);
                }
            }
        } else {
            self->field_0x422 = 0;
            self->field_0x426 = 0;
        }
    }
}

/* 0x802D1C90 - the aim timer: bands the target's distance and runs the two aim-build limits. */
void fn_802D1C90(struct _AINPC_W* self)
{
    if (fn_802CB900(&self->pos_0x178, &self->plw_0x16C->vec_0x03C, lbl_8079A6F0) != 0) {
        if (fn_802CB900(&self->pos_0x178, &self->plw_0x16C->vec_0x03C, lbl_8079A774) != 0) {
            self->field_0x416 = 2;
        } else {
            self->field_0x416 = 1;
        }
    } else {
        self->field_0x416 = 0;
    }
    if (self->field_0x416 == 0) {
        self->field_0x38C = self->field_0x38C + 1;
        if (self->field_0x38C >= 0x12C) {
            self->field_0x38C = 0x12C;
            self->field_0x38E = 1;
        }
    } else if (self->field_0x416 == 1) {
        self->field_0x38C = self->field_0x38C + 1;
        if (self->field_0x38C >= 0x96) {
            self->field_0x38C = 0x96;
            self->field_0x38E = 1;
        }
    } else {
        self->field_0x38C = 0;
        self->field_0x38E = 0;
    }
}

/* 0x802D1D90 - the band's per-frame driver: runs the whole chain of sub-steps and then the model
 * transform. */
void fn_802D1D90(struct _AINPC_W* self)
{
    _PLW* plw = self->plw_0x16C;
    u8 i;

    copyVec3(&self->pos_0x184, &self->pos_0x178);
    self->field_0x3C4 = 0;
    self->field_0x417 = 0;
    self->field_0x450 = 0;
    self->field_0x46C = 0;
    if (self->area != plw->area_0x16) {
        fn_802D1954(self);
    }
    fn_802D36B8(self);
    fn_802D1AFC(self);
    fn_802D1C90(self);
    fn_802D8478(self);
    fn_802CD770(self);
    if (fn_802D94A0(self) == 1) {
        fn_802D9A40(self);
    }
    if (fn_802D948C(self) == 1) {
        fn_802D93B8(self);
    } else {
        fn_802CDB10(self);
    }
    fn_802CDA1C(self);
    if (fn_802D2264(self) == 0 && self->motion != 4 && self->motion != 5) {
        fn_802D0F34(self);
    }
    fn_802D6D4C(self);
    fn_802D6DF4(self);
    fn_802D214C(self);
    fn_802CB858(self);
    if (self->field_0x174 != 0) {
        fn_802CB858(self);
    }
    if (fn_802D2B78(self, 2) != 0 || self->area != ::get_now_areano()) {
        self->in_area = 0;
    } else {
        self->in_area = 1;
    }
    self->model.field_0x34 = self->in_area;
    if (self->field_0x002 == 0) {
        self->model.field_0x34 = self->field_0x002;
    }
    fn_802D18E8(self);
    fn_802D6888(self);
    fn_802D1FFC(self);
    self->model.move(0);
    copyVec3(&self->pos_0x178, &self->model.pos_0x04);
    if (self->variant != 1) {
        if (self->variant != 2) {
            fn_800524C0(&self->pos_0x184, &self->pos_0x178, &self->pos_0x32C, &self->pos_0x178,
                         lbl_8079A7B4);
        } else if (self->pos_0x184.y < lbl_8079A688 + self->field_0x19C) {
            fn_800524C0(&self->pos_0x184, &self->pos_0x178, &self->pos_0x32C, &self->pos_0x178,
                         lbl_8079A73C);
        }
    }
    fn_802D15DC(self);
    fn_802D1710(self);
    fn_802D1FFC(self);
    fn_800E0914(&self->model);
    for (i = 0; i < 2; i++) {
        hit_attack_list_push(&self->hit[i]);
    }
    g3d_root_model_bind(pRoot, self->model.field_0x118);
}

/* 0x802D1FFC - copies the model-transform words into the engine model and the position into its
 * own +0x04. */
void fn_802D1FFC(struct _AINPC_W* self)
{
    self->model.field_0x28 = self->vec_0x190.x;
    self->model.field_0x2C = self->vec_0x190.y;
    self->model.field_0x30 = self->vec_0x190.z;
    copyVec3(&self->model.pos_0x04, &self->pos_0x178);
}

/* 0x802D2024 - advances the four counter-attack counter blocks. */
u32 fn_802D2024(struct _AINPC_W* self)
{
    if (self->field_0x21E > 0 && ::ai_skill_ck((::_AINPC_W*)self, 0x1A) == 0) {
        self->field_0x21C = self->field_0x21C + self->field_0x21E;
        self->field_0x220 = 0x1E;
    }
    if (self->field_0x230 > 0) {
        self->field_0x22E = self->field_0x22E + self->field_0x230;
    }
    if (self->field_0x22A > 0) {
        self->field_0x228 = self->field_0x228 + self->field_0x22A;
        if (self->variant == 2) {
            fn_802D2ABC(self, 4, 0x13, 0);
        } else {
            fn_802D2ABC(self, 4, 8, 0);
        }
        return 1;
    }
    if (self->field_0x224 > 0) {
        self->field_0x222 = self->field_0x222 + self->field_0x224;
        if (self->variant == 2) {
            fn_802D2ABC(self, 4, 0x15, 0);
        } else {
            fn_802D2ABC(self, 4, 0x0A, 0);
        }
        return 1;
    }
    return 0;
}

/* 0x802D214C - runs down the aim-build timer and books the effect when it expires. */
void fn_802D214C(struct _AINPC_W* self)
{
    nw4r::math::VEC3 offset;

    VEC3_ctor(&offset);
    setVector3(&offset, lbl_8079A670, lbl_8079A68C, lbl_8079A670);
    if (self->motion != 5) {
        s16 count = self->field_0x21C;

        if (count > 0) {
            self->field_0x220 = self->field_0x220 - 1;
            if (self->field_0x220 <= 0) {
                self->field_0x21C = count - 1;
                self->field_0x220 = 0x1E;
                fn_801075AC(self, &offset, 0x0E, 2, lbl_8079A674);
                fn_802D35EC(self, -1);
            }
        }
    }
}

/* 0x802D21F8 - the attack selectors that need no counter-attack support. */
s32 fn_802D21F8(u8 selector)
{
    if ((u32)(selector - 4) <= 5 || (u32)(selector - 0x1B) <= 4 || (u32)(selector - 0x12) <= 1 ||
        (s32)selector == 0x24) {
        return 1;
    }
    return 0;
}

/* 0x802D2238 - starts the "raise guard" motion. */
void fn_802D2238(struct _AINPC_W* self)
{
    if (self->variant == 2) {
        fn_802D2ABC(self, 4, 0x16, 0);
    } else {
        fn_802D2ABC(self, 4, 0x0B, 0);
    }
}

/* 0x802D2264 - the counter-attack gate: rolls the guard, drains the window and dispatches the
 * support attack on +0x21A. */
s32 fn_802D2264(struct _AINPC_W* self)
{
    s16 window;
    u8 selector;
    s32 step = 0;
    s32 move;
    s32 commit;
    s32 near;
    s32 on_target;

    if (self->field_0x216 == 0) {
        return 0;
    }
    selector = self->field_0x21A;
    window = self->field_0x212;
    move = 0;
    commit = 1;
    near = 0;
    on_target = 0;
    if (fn_802D7B24(self) == 1) {
        near = 1;
    }
    if (fn_802D3F08(self) == 1) {
        on_target = 1;
        move = -1;
        if (fn_802D21F8(selector) == 0) {
            fn_802D2238(self);
        }
    } else {
        for (;;) {
        if (fn_802D2024(self) == 1) {
            step = 1;
        }
        if (window != 0) {
            fn_802D3A34(self, window);
            if (fn_802D35EC(self, (s16)-window) == 1) {
                move = 1;
                break;
            }
        }
        if (self->variant == 1 && fn_802D21F8(selector) == 0) {
            fn_802D2A00(self, 4, 2, 0);
        } else if (step == 0) {
            switch (selector) {
            case 1:
                if (self->variant == 2) {
                    fn_802D2A00(self, 4, 0x0C, 0);
                } else {
                    fn_802D2A00(self, 4, 0, 0);
                }
                break;
            case 2:
            case 3:
                if (self->variant == 2) {
                    fn_802D2A00(self, 4, 0x0D, 0);
                } else {
                    fn_802D2A00(self, 4, 1, 0);
                }
                move = 1;
                break;
            case 4:
                if (::ai_skill_ck((::_AINPC_W*)self, 0x18) == 1) {
                    move = -1;
                    commit = -1;
                } else if (on_target != 0) {
                    fn_802D2238(self);
                } else if (self->variant == 2) {
                    fn_802D2A00(self, 4, 0x0E, 0);
                } else if (self->variant == 1) {
                    fn_802D2A00(self, 4, 2, 0);
                } else {
                    fn_802D2A00(self, 4, 3, 0);
                }
                break;
            case 36:
                if (::ai_skill_ck((::_AINPC_W*)self, 0x18) == 1) {
                    move = -1;
                    commit = -1;
                } else if (on_target != 0) {
                    fn_802D2238(self);
                } else if (self->variant == 2) {
                    fn_802D2A00(self, 4, 0x18, 0);
                } else if (self->variant == 1) {
                    fn_802D2A00(self, 4, 2, 0);
                } else {
                    fn_802D2A00(self, 4, 0x17, 0);
                }
                break;
            case 5:
                if (::ai_skill_ck((::_AINPC_W*)self, 0x18) == 1) {
                    move = -1;
                    commit = -1;
                } else if (on_target != 0) {
                    fn_802D2238(self);
                } else if (self->variant == 2) {
                    fn_802D2A00(self, 4, 0x0F, 0);
                } else if (self->variant == 1) {
                    fn_802D2A00(self, 4, 2, 0);
                } else {
                    fn_802D2A00(self, 4, 4, 0);
                }
                break;
            case 6:
            case 7:
            case 18:
                if (::ai_skill_ck((::_AINPC_W*)self, 0x19) == 1 || self->field_0x23C >= 1) {
                    move = -1;
                    commit = -1;
                } else if (on_target != 0) {
                    fn_802D2238(self);
                } else if (self->variant == 2) {
                    fn_802D2A00(self, 4, 0x11, 0);
                } else if (self->variant == 1) {
                    fn_802D2A00(self, 4, 2, 0);
                } else {
                    fn_802D2A00(self, 4, 6, 0);
                }
                break;
            default:
                if (self->variant == 2) {
                    fn_802D2A00(self, 4, 0x0C, 0);
                } else {
                    fn_802D2A00(self, 4, 0, 0);
                }
                break;
            case 27:
            case 28:
            case 29:
            case 30:
            case 31:
                move = -1;
                commit = -1;
                break;
            }
            }
            break;
        }
    }
    if (move >= 0) {
        fn_800DCB74(self->sound_handle_0x498, move, &self->pos_0x178);
    }
    if (commit >= 0) {
        fn_802D4200(self);
        fn_802D41C8(self);
        fn_802D3D84(self);
        self->field_0x389 = 1;
        if (near != 0) {
            fn_800DCC24(self->sound_handle_0x498, 7, 0);
            fn_802D9D30(self, (s8)self->field_0x482, 9, 0, 2);
        }
        return 1;
    }
    return 0;
}
}

/* -------------------------------------------------------------------------------------------------
 * 0x802D27E0 .. 0x802D44F4
 * ---------------------------------------------------------------------------------------------- */

extern "C" {

/* 0x802D27E0 - the address of the band's shared `.bss` work block. */
u8* fn_802D27E0(void)
{
    return (u8*)&ainpc_w;
}

/* 0x802D282C - the two-level motion-table walk: row `index / 100` of `lbl_805D3FB8` holds that
 * row's 100-entry column and the entry is `column[index % 100]`. */
s32 fn_802D282C(s32 index)
{
    u16 value = index;
    s32* row = (s32*)lbl_805D3FB8[value / 100];

    if (row == NULL) {
        return 0;
    }
    return row[value % 100];
}

/* 0x802D287C - plays `motion` on the model with the row `fn_802D282C` resolves. */
void fn_802D287C(struct _AINPC_W* self, s32 motion, s32 a, s32 b, s32 c)
{
    fn_800E11C0(&self->model, 3, 0, (u16)motion, a, fn_802D282C((u16)motion), b, (f32)c,
                lbl_8079A678);
}

/* 0x802D2904 - the same with the default row. */
void fn_802D2904(struct _AINPC_W* self, s32 motion, s32 a, s32 b)
{
    fn_802D287C(self, (u16)motion, a, b, 0);
}

/* 0x802D2910 - plays the motion only when the model is not already on it. */
void fn_802D2910(struct _AINPC_W* self, s32 motion, s32 a, s32 b)
{
    if ((u16)motion != ::ai_get_motion_no((::_AINPC_W*)self)) {
        fn_802D287C(self, (u16)motion, a, b, 0);
    }
}

/* 0x802D2984 - resets the model's motion layer. */
void fn_802D2984(struct _AINPC_W* self)
{
    MHchar* model = &self->model;

    fn_800E2198(model, 0);
}

/* 0x802D2990 - plays `motion` as a loop. */
void fn_802D2990(struct _AINPC_W* self, s32 motion)
{
    MHchar* model = &self->model;

    fn_800E16DC(model, (u16)motion, 0);
}

/* 0x802D29A0 - plays `motion` as a one-shot. */
void fn_802D29A0(struct _AINPC_W* self, s32 motion)
{
    MHchar* model = &self->model;

    fn_800E16DC(model, (u16)motion, 1);
}

/* 0x802D29B8 - arms the model's second state flag. */
void fn_802D29B8(struct _AINPC_W* self, s8 value)
{
    self->model.field_0xF2 = value;
}

/* 0x802D29C0 - clears it. */
void fn_802D29C0(struct _AINPC_W* self)
{
    self->model.field_0xF2 = 0;
}

/* 0x802D29CC - sets the model's speed to the record's +0x44C times `scale`. */
void fn_802D29CC(struct _AINPC_W* self, f32 scale)
{
    fn_800E1640(&self->model, self->field_0x44C * scale);
}

/* 0x802D29DC - the model rotation component the scale setters read back. */
f32 fn_802D29DC(struct _AINPC_W* self)
{
    return self->model.field_0x5C;
}

/* 0x802D29E4 - arms the model's first state flag. */
void fn_802D29E4(struct _AINPC_W* self, s8 value)
{
    self->model.field_0xF1 = value;
}

/* 0x802D29EC - clears it. */
void fn_802D29EC(struct _AINPC_W* self)
{
    self->model.field_0xF1 = 0;
}

/* 0x802D29F8 - selects the locomotion arm. */
void fn_802D29F8(struct _AINPC_W* self, s8 variant)
{
    self->variant = variant;
}

/* 0x802D2A00 - the motion state setter: latches the gauge word, saves the outgoing motion and
 * sub-step, clears the three follow-up bytes, re-arms the model flags and sets the speed. */
void fn_802D2A00(struct _AINPC_W* self, u8 motion, u16 step, s16 gauge)
{
    self->field_0x1EC = gauge;
    self->field_0x004 = 0;
    self->field_0x005 = 0;
    self->field_0x006 = 0;
    self->prev_motion = self->motion;
    self->prev_motion_step = self->motion_step;
    self->motion = motion;
    self->motion_step = step;
    fn_802D29C0(self);
    fn_802D29EC(self);
    self->field_0x002 = 1;
    self->field_0x44C = lbl_8079A678;
    if (::ai_skill_ck((::_AINPC_W*)self, 0x11) == 1 && motion == 2 && self->variant != 2) {
        self->field_0x44C = lbl_8079A6F8;
    }
    fn_802D29CC(self, lbl_8079A678);
}

/* 0x802D2ABC - the same setter through the "pending" byte the drivers test. */
void fn_802D2ABC(struct _AINPC_W* self, s32 motion, s32 step, s32 gauge)
{
    self->field_0x174 = 1;
    fn_802D2A00(self, motion, step, gauge);
}

/* 0x802D2AD4 - switches the locomotion arm and re-seeds the motion. */
void fn_802D2AD4(struct _AINPC_W* self, u8 variant)
{
    self->variant = variant;
    if (variant != 2) {
        fn_802D2ABC(self, 0, 0, 0);
    } else {
        fn_802D2ABC(self, 0, 1, 0);
    }
    fn_802D41C8(self);
}

/* 0x802D2B38 - whether the record is already on the given motion and sub-step. */
s32 fn_802D2B38(struct _AINPC_W* self, s32 motion, s32 step)
{
    if (self->motion == (u8)motion && self->motion_step == (u16)step) {
        return 1;
    }
    return 0;
}

/* 0x802D2B68 - resets the model's joint layer. */
void fn_802D2B68(struct _AINPC_W* self)
{
    mhchar_joint_mtx_get(&self->model);
}

/* 0x802D2B78 - tests the record's flag word. */
s32 fn_802D2B78(struct _AINPC_W* self, s32 mask)
{
    return self->field_0x1EC & (u16)mask;
}

/* 0x802D2B88 - sets flags in it. */
void fn_802D2B88(struct _AINPC_W* self, u16 flags)
{
    self->field_0x1EC |= flags;
}

/* 0x802D2B98 - clears flags in it. */
void fn_802D2B98(struct _AINPC_W* self, u16 flags)
{
    self->field_0x1EC &= ~flags;
}

/* 0x802D2BB0 - fills hit entry `hit` for the running attack: the selector bytes, one OR per owned skill, the random
 * weak-point roll, and the "blocked" case that zeroes the entry again. */
void fn_802D2BB0(struct _AINPC_W* self, struct _HIT_W* hit)
{
    u8 applied = 0;

    hit->field_0x31 = 0x0A;
    hit->field_0x32 = 4;
    if (::ai_skill_ck((::_AINPC_W*)self, 2) == 1) {
        hit->field_0x48 |= 0x10;
        hit->field_0x4A = self->field_0x210;
        applied = 1;
    }
    if (::ai_skill_ck((::_AINPC_W*)self, 3) == 1) {
        hit->field_0x48 |= 0x20;
        hit->field_0x4A = self->field_0x210;
        applied = 1;
    }
    if (::ai_skill_ck((::_AINPC_W*)self, 4) == 1) {
        hit->field_0x48 |= 0x200;
        hit->field_0x4A = self->field_0x210;
        applied = 1;
    }
    if (::ai_skill_ck((::_AINPC_W*)self, 5) == 1) {
        hit->field_0x48 |= 0x40;
        hit->field_0x4A = self->field_0x210;
        applied = 1;
    }
    if (::ai_skill_ck((::_AINPC_W*)self, 6) == 1) {
        hit->field_0x48 |= 0x80;
        hit->field_0x4A = self->field_0x210;
        applied = 1;
    }
    if (::ran_suu(1) % 3 == 0) {
        if (::ai_skill_ck((::_AINPC_W*)self, 8) == 1) {
            hit->field_0x48 |= 2;
            hit->field_0x4A = self->field_0x210;
            applied |= 2;
        }
        if (::ai_skill_ck((::_AINPC_W*)self, 9) == 1) {
            hit->field_0x48 |= 4;
            hit->field_0x4A = self->field_0x210;
            applied |= 2;
        }
        if (::ai_skill_ck((::_AINPC_W*)self, 0x0A) == 1) {
            hit->field_0x48 |= 1;
            hit->field_0x4A = self->field_0x210;
            applied |= 2;
        }
    }
    if (::ai_skill_ck((::_AINPC_W*)self, 7) == 1 && (applied & 1) != 0) {
        hit->field_0x4A = (hit->field_0x4A * 0x82) / 100;
    }
    if (::ai_skill_ck((::_AINPC_W*)self, 0x0B) == 1 && (applied & 2) != 0) {
        hit->field_0x4A = (hit->field_0x4A * 0x82) / 100;
    }
    if (self->field_0x420 == 1 && self->field_0x422 > 0) {
        hit->field_0x42 = 0x11;
    }
    if (::ai_skill_ck((::_AINPC_W*)self, 0x0D) == 1 && ::ran_suu(1) % 100 < 0x14) {
        hit->field_0x40 = (hit->field_0x40 * 0x7D) / 100;
        hit->field_0x4C |= 0x80;
    }
    if (::ai_skill_ck((::_AINPC_W*)self, 0x0C) == 1) {
        hit->field_0x4E = (s8)(hit->field_0x4E * 0x78) / 100;
    }
    if (fn_802D2B78(self, 4) != 0) {
        hit->field_0x31 = 1;
        hit->field_0x40 = 0;
        if (self->field_0x34E == 1) {
            hit->field_0x42 = 2;
        }
        hit->field_0x4E = 0;
        hit->field_0x48 = 0;
        hit->field_0x4A = 0;
    }
    if ((applied & 3) != 0) {
        ::hit_flag_set((::_HIT_W*)hit, 0x800);
    }
}

/* 0x802D2F7C - fills hit entry `index` from motion-table entry `row` and hands it to the attack
 * filler. */
void fn_802D2F7C(struct _AINPC_W* self, u8 index, s32 row)
{
    struct _HIT_W* hit = &self->hit[index];
    struct MotionEntry* entry = (struct MotionEntry*)(lbl_805D3AD8 + row * 0x1A);
    f32 limit;

    hit->field_0x08 = lbl_80792508[entry->motion_id_0x0F];
    hit_flags_clear(hit);
    ::hit_flag_set((::_HIT_W*)hit, 0x320);
    hit->motion_no_0x18 = ::ai_get_motion_no((::_AINPC_W*)self);
    hit->field_0x1A = 0;
    hit->field_0x1C = 0;
    hit->field_0x1E = 0;
    limit = self->model.field_0x44 - lbl_8079A730;
    if (limit < lbl_8079A670 || (self->model.field_0x4C & 1) != 0) {
        limit = lbl_8079A670;
    }
    hit->offset_0x38 = entry->approach_0x00;
    hit->offset_0x3C = entry->retreat_0x02;
    if (entry->approach_0x00 != 0) {
        hit->offset_0x38 = hit->offset_0x38 + (limit - self->model.field_0x74);
        if (hit->offset_0x38 < lbl_8079A670) {
            hit->offset_0x3C = hit->offset_0x3C + hit->offset_0x38;
            hit->offset_0x38 = lbl_8079A670;
            if (hit->offset_0x3C == lbl_8079A670) {
                hit->field_0x05 = 0;
                return;
            }
        }
    }
    hit_data_apply(hit, entry, lbl_8079A670);
    fn_802D2BB0(self, hit);
}

/* 0x802D30F8 - clamps the angle difference to `limit` degrees, taking the short way round. */
u16 fn_802D30F8(u16 angle, s32 target, u16 limit)
{
    u16 diff = angle - target;
    u16 result;

    if (abs((s16)diff) < limit) {
        result = angle;
    } else {
        result = target - limit;
        if (diff < -0x8000) {
            result = target + limit;
        }
    }
    return result;
}

/* 0x802D3184 - whether the model's facing is outside `limit` degrees of the record's +0x194. */
s32 fn_802D3184(struct _AINPC_W* self, u32 limit)
{
    u16 diff = self->vec_0x190.y - ::calcVecAng2(&self->pos_0x178, &self->pos_0x1B0);

    if (diff <= (u16)(limit * -1) && diff >= (u16)limit) {
        return 0;
    }
    return 1;
}

/* 0x802D31FC - zeroes the model offset vector at +0x1D4. */
void fn_802D31FC(struct _AINPC_W* self)
{
    self->pos_0x1D4.x = lbl_8079A670;
    self->pos_0x1D4.y = lbl_8079A670;
    self->pos_0x1D4.z = lbl_8079A670;
}

/* 0x802D3210 - builds the offset vector at +0x1D4 from the two angles at `angles[0]`/`angles[1]`
 * and adds it to the position. */
void fn_802D3210(struct _AINPC_W* self, u32* angles)
{
    nw4r::math::VEC3 offset;

    VEC3_ctor(&offset);
    copyVec3(&offset, &self->pos_0x1D4);
    rotVecX(&offset, angles[0]);
    rotVecY(&offset, angles[1]);
    addVec3To(&self->pos_0x178, &offset);
}

/* 0x802D327C - the stored variant, then the same offset walk from +0x1D4. */
void fn_802D327C(struct _AINPC_W* self, u32* angles)
{
    fn_802D3210(self, angles);
    addVec3To(&self->pos_0x1D4, &self->pos_0x1E0);
}

/* 0x802D32B4 - rolls one of the six-byte attack rows the skill table points at, weighted by the
 * rows' first word, and starts the motion the row names. */
s32 fn_802D32B4(struct _AINPC_W* self, u16* row)
{
    u16 total = 0;
    u16 roll;
    u16 walked = 0;
    u16* walk = row;

    while (*walk != 0xFFFF) {
        total += *walk;
        walk += 3;
    }
    if (total == 0) {
        return 0;
    }
    roll = ::ran_suu(1) % total;
    for (; *row != 0xFFFF; row += 3) {
        walked += *row;
        if (roll < walked) {
            fn_802D2A00(self, (u8)row[1], row[2], 0);
            return 1;
        }
    }
    return 0;
}

/* 0x802D3398 - the same for the four-byte rows the dispatchers use. */
s32 fn_802D3398(struct _AINPC_W* self, u16* row)
{
    u16 total = 0;
    u16 roll;
    u16 walked = 0;
    u16* walk = row;

    while (*walk != 0xFFFF) {
        total += *walk;
        walk += 2;
    }
    if (total == 0) {
        return 0;
    }
    roll = ::ran_suu(1) % total;
    for (; *row != 0xFFFF; row += 2) {
        walked += *row;
        if (roll < walked) {
            fn_802D41E0(self, (u8)row[1], 1);
            return 1;
        }
    }
    return 0;
}

/* 0x802D3474 - the same motion setter for the "blocked" arm. */
void fn_802D3474(struct _AINPC_W* self, s32 motion, s32 step)
{
    fn_802D2ABC(self, 5, (u16)motion, (u16)step);
}

/* 0x802D348C - drops the aim/attack state: clears the gauge, resets the attack bookkeeping and
 * re-seeds the motion (the recorder's counter bumps below 7 while the skill is held). */
void fn_802D348C(struct _AINPC_W* self)
{
    if (self->motion != 5) {
        self->gauge_0x1F0 = 0;
        fn_802D3AD8(self);
        fn_802D3CFC(self, 0);
        fn_802D3D84(self);
        fn_802D4200(self);
        self->field_0x389 = 1;
        if (fn_802D948C(self) == 1) {
            fn_802D3474(self, 9, 0);
            return;
        }
        if (self->variant != 2) {
            fn_802D3474(self, 0, 0);
        } else {
            fn_802D3474(self, 4, 0);
        }
        if (::ai_skill_ck((::_AINPC_W*)self, 0x15) == 1) {
            if (self->field_0x43C < 7) {
                self->field_0x43C = self->field_0x43C + 1;
                self->field_0x1FE = (self->field_0x1FE * 0x6A) / 100;
                self->field_0x200 = (self->field_0x200 * 0x6A) / 100;
            }
        }
    }
}

/* 0x802D35B4 - adjusts the motion gauge and clamps it to [0, +0x1F4]. */
void fn_802D35B4(struct _AINPC_W* self, s32 delta)
{
    s32 value = self->gauge_0x1F0 + (s16)delta;

    self->gauge_0x1F0 = value;
    if (value <= 0) {
        self->gauge_0x1F0 = 0;
    }
    if (self->gauge_0x1F0 >= self->gauge_max_0x1F4) {
        self->gauge_0x1F0 = self->gauge_max_0x1F4;
    }
}

/* 0x802D35EC - drains the gauge (zeroed while the player is down) and reports whether it ran out. */
u32 fn_802D35EC(struct _AINPC_W* self, s32 delta)
{
    if (Pl_motion_input_ck(0) == 1) {
        delta = 0;
    }
    fn_802D35B4(self, delta);
    if (fn_802D948C(self) == 1) {
        fn_802D92F8(self);
    }
    if (self->gauge_0x1F0 <= 0 && self->variant != 1) {
        fn_802D348C(self);
        return 1;
    }
    return 0;
}

/* 0x802D3684 - re-seeds the gauge from the +0x1F8 value. */
void fn_802D3684(struct _AINPC_W* self)
{
    self->gauge_max_0x1F4 = self->gauge_refill_0x1F8;
    self->gauge_0x1F0 = self->gauge_refill_0x1F8;
}

/* 0x802D3694 - raises the gauge cap by `delta`, clamped at 150. */
void fn_802D3694(struct _AINPC_W* self, s32 delta)
{
    s32 value = self->gauge_max_0x1F4 + (s16)delta;

    self->gauge_max_0x1F4 = value;
    if (value >= 0x96) {
        self->gauge_max_0x1F4 = 0x96;
    }
}

/* 0x802D36B8 - the frame step: runs down every one of the band's countdowns. */
void fn_802D36B8(struct _AINPC_W* self)
{
    if (self->field_0x214 > 0) {
        self->field_0x214 = self->field_0x214 - 1;
    }
    if (self->field_0x35A > 0) {
        self->field_0x35A = self->field_0x35A - 1;
    }
    if (self->field_0x35E > 0) {
        self->field_0x35E = self->field_0x35E - 1;
    }
    if (self->field_0x352 > 0) {
        self->field_0x352 = self->field_0x352 - 1;
        if (self->field_0x352 == 1) {
            self->field_0x354 = 3;
        }
    }
    if (self->field_0x34A != 0) {
        self->field_0x34A = self->field_0x34A - 1;
    }
    if (self->field_0x3CA > 0) {
        self->field_0x3CA = self->field_0x3CA - 1;
    }
    if (self->field_0x3D0 > 0) {
        self->field_0x3D0 = self->field_0x3D0 - 1;
    }
    if (self->field_0x43E > 0) {
        self->field_0x43E = self->field_0x43E - 1;
    }
    if (self->field_0x422 > 0) {
        self->field_0x422 = self->field_0x422 - 1;
        if (self->field_0x420 == 1 && (self->field_0x422 == 0 || self->variant == 2)) {
            self->field_0x422 = 0;
            fn_800DCCF8(self->sound_handle_0x498, &self->pos_0x178, 1);
        }
    }
    if (self->field_0x424 > 0) {
        self->field_0x424 = self->field_0x424 - 1;
    }
    if (self->field_0x374 > 0) {
        self->field_0x374 = self->field_0x374 - 1;
    }
    if (self->field_0x236 > 0) {
        self->field_0x236 = self->field_0x236 - 1;
        if (self->field_0x236 == 0) {
            self->field_0x235 = 0;
        }
    }
    if (self->field_0x23A > 0) {
        self->field_0x23A = self->field_0x23A - 1;
        if (self->field_0x23A == 0) {
            self->field_0x239 = 0;
        }
    }
    if (self->field_0x3D4 < self->field_0x3D6) {
        self->field_0x3DC = self->field_0x3DC - 1;
        if (self->field_0x3DC <= 0) {
            fn_802D6B2C(self, 1);
        }
    }
    if (self->field_0x23E > 0) {
        self->field_0x23E = self->field_0x23E - 1;
        if (self->field_0x23E <= 0) {
            self->field_0x23E = 0;
            self->field_0x23C = 0;
        }
    }
    if (self->field_0x408 > 0) {
        self->field_0x408 = self->field_0x408 - 1;
    }
    if (self->field_0x40A > 0) {
        self->field_0x40A = self->field_0x40A - 1;
    }
    if (self->field_0x40C > 0) {
        self->field_0x40C = self->field_0x40C - 1;
    }
    if (self->field_0x40E > 0) {
        self->field_0x40E = self->field_0x40E - 1;
    }
    if (self->field_0x410 > 0) {
        self->field_0x410 = self->field_0x410 - 1;
    }
    if (self->field_0x412 > 0) {
        self->field_0x412 = self->field_0x412 - 1;
    }
    if (self->field_0x47C > 0) {
        self->field_0x47C = self->field_0x47C - 1;
    }
    if (self->field_0x46A > 0) {
        self->field_0x46A = self->field_0x46A - 1;
    }
    if (self->field_0x38A > 0) {
        self->field_0x38A = self->field_0x38A - 1;
    }
    if (self->field_0x486 > 0) {
        self->field_0x486 = self->field_0x486 - 1;
    }
}

/* 0x802D3984 - the 0..0x14 progress interpolation `high` -> `low`. */
s16 fn_802D3984(struct _AINPC_W* self, s32 high, s32 low, u32 reverse)
{
    s16 hi = high;
    s16 lo = low;
    s16 progress;

    if ((u8)reverse == 0) {
        progress = self->field_0x339;
    } else {
        progress = 0x14 - self->field_0x339;
    }
    return lo + (progress * (hi - lo)) / 20;
}

/* 0x802D39DC - seeds the entry timer from that interpolation. */
void fn_802D39DC(struct _AINPC_W* self)
{
    self->field_0x35A = fn_802D3984(self, 0x1E, 0x0A, 1) * 0x1E;
}

/* 0x802D3A20 - clears the +0x34F counter. */
void fn_802D3A20(struct _AINPC_W* self)
{
    self->field_0x34F = 0;
}

/* 0x802D3A2C - arms the +0x214 countdown. */
void fn_802D3A2C(struct _AINPC_W* self, s16 value)
{
    self->field_0x214 = value;
}

/* 0x802D3A34 - adds to the +0x350 counter (halved without the skill) and clamps it to the
 * model's limit. */
void fn_802D3A34(struct _AINPC_W* self, s16 delta)
{
    s16 limit;

    if (::ai_skill_ck((::_AINPC_W*)self, 0x17) == 1) {
        self->field_0x350 = self->field_0x350 + delta;
    } else {
        self->field_0x350 = self->field_0x350 + (s16)(delta / 2);
    }
    limit = fn_802D9D14(self);
    if (self->field_0x350 > limit) {
        self->field_0x350 = limit;
    }
    if (self->field_0x350 < 0) {
        self->field_0x350 = 0;
    }
}

/* 0x802D3AD8 - clears the three +0x350..+0x354 words. */
void fn_802D3AD8(struct _AINPC_W* self)
{
    if (self->field_0x354 != 0) {
        self->field_0x350 = 0;
        self->field_0x354 = 0;
        self->field_0x352 = 0;
    }
}

/* 0x802D3AF8 - clears the four +0x348..+0x34B bytes. */
void fn_802D3AF8(struct _AINPC_W* self)
{
    self->field_0x348 = 0;
    self->field_0x349 = 0;
    self->field_0x34A = 0;
    self->field_0x34B = 0;
}

/* 0x802D3B10 - clears +0x34D and arms +0x34E. */
void fn_802D3B10(struct _AINPC_W* self)
{
    self->field_0x34D = 0;
    self->field_0x34E = 0xFF;
}

/* 0x802D3B24 - clears +0x382 and +0x34B. */
void fn_802D3B24(struct _AINPC_W* self)
{
    self->field_0x382 = 0;
    self->field_0x34B = 0;
}

/* 0x802D3B34 - advances the three +0x3C6/+0x3CC/+0x480 timers and reports whether the move the
 * caller handed in is a repeat of the last accepted one. */
s32 fn_802D3B34(struct _AINPC_W* self, u16 limit, u8 copy)
{
    u32 moved = 0;

    if (self->field_0x3C8 > 0) {
        self->field_0x3C6 = self->field_0x3C6 + 1;
        if (self->field_0x3C6 >= (s32)limit) {
            self->field_0x3C6 = limit;
            moved = 1;
        }
    } else {
        self->field_0x3C6 = 0;
    }
    if (self->field_0x3CE > 0) {
        self->field_0x3CC = self->field_0x3CC + 1;
        if (self->field_0x3CC >= (s32)limit) {
            self->field_0x3CC = limit;
            moved = 1;
        }
    } else {
        self->field_0x3CC = 0;
    }
    if (self->field_0x47E != 0) {
        self->field_0x480 = self->field_0x480 + 1;
        if (self->field_0x480 >= (s32)limit) {
            self->field_0x480 = limit;
            moved = 1;
            self->field_0x389 = 0;
        }
    } else {
        self->field_0x480 = 0;
    }
    if (moved == 1) {
        if (copy == 1) {
            copyVec3(&self->pos_0x1B0, &self->pos_0x178);
            fn_802D3CB8(self);
            return 0;
        }
        if ((self->field_0x358 != 0 || self->field_0x35C != 0) &&
            ::calcDistanceSqXZ(&self->pos_0x178, &self->pos_0x1B0) < lbl_8079A6E0) {
            copyVec3(&self->pos_0x1B0, &self->pos_0x178);
            fn_802D3CB8(self);
            return 0;
        }
        return 1;
    }
    return 0;
}

/* 0x802D3CB8 - clears the three move timers. */
void fn_802D3CB8(struct _AINPC_W* self)
{
    self->field_0x3C6 = 0;
    self->field_0x3CC = 0;
    self->field_0x480 = 0;
}

/* 0x802D3CCC - arms the +0x3CA countdown. */
void fn_802D3CCC(struct _AINPC_W* self, s16 value)
{
    self->field_0x3CA = value;
}

/* 0x802D3CD4 - arms the +0x3D0 countdown. */
void fn_802D3CD4(struct _AINPC_W* self, s16 value)
{
    self->field_0x3D0 = value;
}

/* 0x802D3CDC - clears the two +0x22E block words. */
void fn_802D3CDC(struct _AINPC_W* self)
{
    self->field_0x22E = 0;
    self->field_0x232 = 0;
}

/* 0x802D3CEC - clears the two +0x222 block words. */
void fn_802D3CEC(struct _AINPC_W* self)
{
    self->field_0x222 = 0;
    self->field_0x226 = 0;
}

/* 0x802D3CFC - clears the whole motion-gauge block; `full` also drops the +0x234..+0x23E run. */
void fn_802D3CFC(struct _AINPC_W* self, u8 full)
{
    fn_802D3CDC(self);
    fn_802D3CEC(self);
    self->field_0x21C = 0;
    self->field_0x220 = 0;
    self->field_0x228 = 0;
    self->field_0x22C = 0;
    if (full == 0) {
        self->field_0x234 = 0;
        self->field_0x235 = 0;
        self->field_0x238 = 0;
        self->field_0x239 = 0;
        self->field_0x236 = 0;
        self->field_0x23A = 0;
        self->field_0x23C = 0;
        self->field_0x23E = 0;
    }
    fn_802D6888(self);
}

/* 0x802D3D84 - resets the band's whole attack/aim bookkeeping. */
void fn_802D3D84(struct _AINPC_W* self)
{
    self->field_0x35C = 0;
    self->field_0x358 = 0;
    self->field_0x3DE = 0;
    self->field_0x461 = 0;
    self->field_0x451 = 0;
    fn_802D3AF8(self);
    fn_802D3B10(self);
    fn_802D3B24(self);
    fn_802D3A20(self);
    fn_802D4238(self);
}

/* 0x802D3DE8 - reports (and starts the follow-up motion) when the +0x22E counter has run out. */
s32 fn_802D3DE8(struct _AINPC_W* self)
{
    if (self->field_0x22E >= 0x96) {
        if (self->variant == 2) {
            fn_802D2ABC(self, 4, 0x14, 0);
        } else {
            fn_802D2ABC(self, 4, 9, 0);
        }
        return 1;
    }
    return 0;
}

/* 0x802D3E4C - the counter-attack roll. */
s32 fn_802D3E4C(struct _AINPC_W* self)
{
    s32 chance = 0x0A;

    self->field_0x321 = 0;
    if ((u32)(self->field_0x21A - 0x1B) <= 4) {
        return 0;
    }
    if (::ai_skill_ck((::_AINPC_W*)self, 0x10) == 1) {
        chance = 0x28;
    }
    if (self->variant != 1 && ::ran_suu(1) % 100 < chance) {
        self->field_0x321 = 1;
        return 1;
    }
    return 0;
}

/* 0x802D3F08 - whether the counter-attack roll latched. */
u32 fn_802D3F08(struct _AINPC_W* self)
{
    return self->field_0x321 != 0;
}

/* 0x802D3F1C - the highest weak-point rate of the two hit entries. */
u8 fn_802D3F1C(struct _AINPC_W* self)
{
    u8 highest = 0xFF;
    u8 value = self->hit[0].field_0x0E;

    if (value != 0xFF && (highest == 0xFF || highest < value)) {
        highest = value;
    }
    value = self->hit[1].field_0x0E;
    if (value != 0xFF && (highest == 0xFF || highest < value)) {
        highest = value;
    }
    return highest;
}

/* 0x802D3F70 - the percentage roll the +0x339 progress interpolates between `high` and `low`. */
s32 fn_802D3F70(struct _AINPC_W* self, u8 high, u8 low)
{
    return ::ran_suu(1) % 100 < low + ((self->field_0x339 * (high - low)) / 20);
}

/* 0x802D4020 - the formation's attack roll, raised by the skill. */
s32 fn_802D4020(struct _AINPC_W* self)
{
    u8 chance = self->formation_0x41C->attack_rate;

    if (::ai_skill_ck((::_AINPC_W*)self, 0x14) == 1) {
        chance += 0x14;
    }
    if (::ran_suu(1) % 100 < chance) {
        return 1;
    }
    return 0;
}

/* 0x802D40A4 - appends an entry to the 8-slot ring at +0x390. */
void fn_802D40A4(struct _AINPC_W* self, s16 id)
{
    self->slots[self->entry_index].id = id;
    self->slots[self->entry_index].count = 1;
    self->entry_index = self->entry_index + 1;
    if (self->entry_index >= 8) {
        self->entry_index = 0;
    }
}

/* 0x802D40EC - adds `delta` to the ring/pending entry `id` carries; a new id goes to the first free
 * pending slot. */
void fn_802D40EC(struct _AINPC_W* self, s32 id, s32 delta)
{
    u8 i;

    for (i = 8; i < 12; i++) {
        if ((u16)id == self->slots[i].id) {
            self->slots[i].count = self->slots[i].count + (s16)delta;
            if (self->slots[i].count <= 0) {
                self->slots[i].count = 0;
                self->slots[i].id = 0;
            }
            return;
        }
    }
    if (self->slots[8].id == 0 && delta > 0) {
        self->slots[8].id = id;
        self->slots[8].count = delta;
        return;
    }
    if (self->slots[9].id == 0 && delta > 0) {
        self->slots[9].id = id;
        self->slots[9].count = delta;
        return;
    }
    if (self->slots[10].id == 0 && delta > 0) {
        self->slots[10].id = id;
        self->slots[10].count = delta;
        return;
    }
    if (self->slots[11].id == 0 && delta > 0) {
        self->slots[11].id = id;
        self->slots[11].count = delta;
        return;
    }
}

/* 0x802D41C8 - latches +0x1CD. */
void fn_802D41C8(struct _AINPC_W* self)
{
    self->field_0x1CD = 1;
}

/* 0x802D41D4 - clears it. */
void fn_802D41D4(struct _AINPC_W* self)
{
    self->field_0x1CD = 0;
}

/* 0x802D41E0 - latches the attack selector; `reset` clears the two follow-up bytes. */
void fn_802D41E0(struct _AINPC_W* self, s8 selector, s32 reset)
{
    self->field_0x1CE = selector;
    if ((u8)reset == 1) {
        self->field_0x1CF = 0;
        self->field_0x1D0 = 0;
    }
}

/* 0x802D4200 - re-arms the attack step. */
void fn_802D4200(struct _AINPC_W* self)
{
    self->field_0x1D0 = 0;
    self->field_0x1CF = 1;
    self->field_0x389 = 1;
}

/* 0x802D4218 - selects the first follow-up. */
void fn_802D4218(struct _AINPC_W* self)
{
    self->field_0x1D1 = 1;
}

/* 0x802D4224 - selects the second. */
void fn_802D4224(struct _AINPC_W* self)
{
    self->field_0x1D1 = 2;
}

/* 0x802D4230 - stores the model offset factor. */
void fn_802D4230(struct _AINPC_W* self, f32 value)
{
    self->field_0x1BC = value;
}

/* 0x802D4238 - clears the two +0x376 bytes. */
void fn_802D4238(struct _AINPC_W* self)
{
    self->field_0x376 = 0;
    self->field_0x378 = 0;
}

/* 0x802D4248 - spawns the attack's effect: takes the joint's world position (or the record's own),
 * drops it by the entry's offset and maps the attack kind onto the effect id the emitter takes. */
void fn_802D4248(struct _AINPC_W* self, u8 a1, u8 kind, u32 joint, s32 effect, f32 scale)
{
    nw4r::math::VEC3 pos;
    u8 code = kind;

    VEC3_ctor(&pos);
    if ((self->field_0x324 & 0x4000) != 0 || a1 != 0) {
        return;
    }
    if (joint != 0xFF) {
        self->model.get_joint_wpos(joint, &pos);
    } else {
        copyVec3(&pos, &self->pos_0x178);
    }
    if ((self->field_0x324 & 6) != 0) {
        pos.y = lbl_8079A7B8 + self->field_0x1A0;
        switch (code) {
        case 0:
        case 17:
            code = 1;
            fn_801006A0(2, &pos, effect, self->area, 2, scale);
            break;
        case 6:
        case 7:
        case 41:
            code = 0x23;
            break;
        case 18:
            code = 1;
            fn_801006A0(2, &pos, effect, self->area, 2, scale);
            break;
        case 40:
            code = 0x0E;
            break;
        case 42:
        case 43:
        case 44:
            fn_801006A0(0x23, &pos, effect, self->area, 3, scale);
            return;
        case 45:
            fn_801006A0(0x23, &pos, effect, self->area, 1, scale);
            return;
        case 46:
            fn_801006A0(2, &pos, effect, self->area, 1, scale);
            return;
        case 47:
            fn_801006A0(1, &pos, effect, self->area, 1, scale);
            fn_801006A0(2, &pos, effect, self->area, 1, scale);
            return;
        case 48:
            fn_801006A0(1, &pos, effect, self->area, 1, scale);
            return;
        }
        fn_801006A0(code, &pos, effect, self->area, 2, scale);
    } else {
        pos.y = lbl_8079A7B8 + self->field_0x19C;
        switch (code) {
        case 2:
        case 42:
        case 46:
            return;
        case 40:
        case 43:
        case 45:
            code = 6;
            break;
        case 41:
            code = 0;
            break;
        case 44:
            code = 7;
            break;
        case 47:
            code = 0x12;
            break;
        case 48:
            code = 1;
            break;
        }
        fn_801006A0(code, &pos, effect, self->area, 2, scale);
    }
}
}
}  /* namespace view_fn_802D0F34 */

/* 0x802D27EC - whether the NPC is in the area the game is currently in. */
s32 ai_area_ck(struct _AINPC_W* self)
{
    return self->field_0x1A4 == get_now_areano();
}

/* 0x802D29B0 - the motion number the model is playing. */
u16 ai_get_motion_no(struct _AINPC_W* self)
{
    return ((view_fn_802D0F34::_AINPC_W*)self)->model.motion_no_0x50;
}

/* 0x802D2B70 - the model's joint world position. */
void get_joint_wpos_ai(struct _AINPC_W* self, u32 joint, nw4r::math::VEC3* out)
{
    ((view_fn_802D0F34::_AINPC_W*)self)->model.get_joint_wpos(joint, out);
}


#pragma peephole off

extern "C" {

/* 0x802D6534 - arms the "hold item" timer when the player is within reach: latches the +0x43D gate once and gives the
 * +0x43E timer the long or short value by the distance test. */
void ai_npc_hold_item_arm(void)
{
    if (quest_move_state_valid_ck() == 0) {
        return;
    }
    if (ainpc_w.active == 0) {
        return;
    }
    if (ainpc_w.field_0x171 == 5) {
        return;
    }
    if (ainpc_w.field_0x43D != 0) {
        return;
    }
    ainpc_w.field_0x43D = 1;
    if (fn_80050EF4(&ainpc_w.vec_0x178,
                    (nw4r::math::VEC3*)&ainpc_w.plw_0x16C->motion_pos_0x3C) >= lbl_8079A770) {
        ainpc_w.field_0x43E = 0x258;
    } else {
        ainpc_w.field_0x43E = 0x12C;
    }
}

/* 0x802D65CC - the hold-item countdown: past 0x384 frames it fires the release animation and the
 * effect, otherwise it re-arms from the per-record tuning table. */
void fn_802D65CC(struct _AINPC_W* self)
{
    if (self->field_0x422 > 0x384) {
        self->field_0x422 = 0;
        fn_800DCCF8(self->sound_0x498, &self->vec_0x178, 1);
        fn_802D9D30(self, self->field_0x482, 0x11, 0, 2);
    } else if (self->variant != 2) {
        self->field_0x422 = (s16)(lbl_805D5354[self->field_0x33A] * 0x1E);
        fn_800DCCF8(self->sound_0x498, &self->vec_0x178, 0);
        fn_802D9D30(self, self->field_0x482, 0x12, 0, 2);
    }
}

/* 0x802D6690 - the hold countdown's frame budget, from the per-record tuning table's +0x3 byte. */
void fn_802D6690(struct _AINPC_W* self)
{
    u8 index = self->field_0x33A;
    s16 value = (s16)(lbl_805D5360[index].field_0x3 * 0x708);

    self->field_0x424 = value;
}

/* 0x802D66B8 - the weighted three-way choice: 1 while the roll is under the first threshold, 2 at
 * or past the sum of both, 0 in between. */
s32 fn_802D66B8(struct _AINPC_W* self)
{
    s16 roll = (s16)((u16)ran_suu(1) % 100);
    s16 first = (s16)lbl_805D5360[self->field_0x33A].field_0x0;

    if (roll < first) {
        return 1;
    }
    return (roll >= first + lbl_805D5360[self->field_0x33A].field_0x1) ? 2 : 0;
}

/* 0x802D675C - holds the sub-state while the frame budget is running, then re-arms both timers from
 * the second tuning table and starts the release animation. */
void fn_802D675C(struct _AINPC_W* self)
{
    if (self->variant != 2) {
        return;
    }
    if (self->field_0x424 > 0) {
        if (self->field_0x422 != 0) {
            return;
        }
        fn_802D9D30(self, self->field_0x482, 0x11, 0, 2);
        return;
    }
    s16 first = (s16)(lbl_805D5378[self->field_0x33A * 2] * 0x1E);
    self->field_0x422 = first;
    self->field_0x424 = (s16)(first + lbl_805D5378[self->field_0x33A * 2 + 1] * 0x1E);
    fn_802D9D30(self, self->field_0x482, 0x12, 0, 2);
}

/* 0x802D67F8 - the same hold/ re-arm pair as `fn_802D675C` but from the third tuning table. */
void fn_802D67F8(struct _AINPC_W* self)
{
    if (self->field_0x424 > 0) {
        if (self->field_0x422 != 0) {
            return;
        }
        fn_802D9D30(self, self->field_0x482, 0x11, 0, 2);
        return;
    }
    s16 first = (s16)(lbl_805D5390[self->field_0x33A * 2] * 0x1E);
    self->field_0x422 = first;
    self->field_0x424 = (s16)(first + lbl_805D5390[self->field_0x33A * 2 + 1] * 0x1E);
    fn_802D9D30(self, self->field_0x482, 0x12, 0, 2);
}

/* 0x802D6888 - rebuilds the behaviour flag word at +0x20C from the hold counters and the sub-state
 * of the +0x171 == 4 reaction state. */
void fn_802D6888(struct _AINPC_W* self)
{
    u16 flags = 0;

    if (self->field_0x21C != 0) {
        flags |= 1;
    }
    if (self->field_0x23C != 0) {
        flags |= 0x2000;
    }
    if ((s16)(self->field_0x234 + self->field_0x235) > 0) {
        flags |= 0x10;
    }
    if ((s16)(self->field_0x238 + self->field_0x239) > 0) {
        flags |= 0x40;
    }
    if (self->field_0x171 == 4) {
        switch (self->field_0x172) {
        case 8:
        case 0x13:
            flags |= 4;
            break;
        case 9:
        case 0x14:
            flags |= 8;
            break;
        case 0xA:
        case 0x15:
            flags |= 2;
            break;
        }
    }
    self->field_0x20C = flags;
}

/* 0x802D6964 - raises one of the two halves of the first held-item counter. */
void fn_802D6964(struct _AINPC_W* self, s32 slot, s8 value)
{
    if (slot == 0) {
        if (self->field_0x234 < value) {
            self->field_0x234 = value;
        }
    } else if (self->field_0x235 < value) {
        self->field_0x235 = value;
    }
}

/* 0x802D69A4 - raises one of the two halves of the second held-item counter. */
void fn_802D69A4(struct _AINPC_W* self, s32 slot, s8 value)
{
    if (slot == 0) {
        if (self->field_0x238 < value) {
            self->field_0x238 = value;
        }
    } else if (self->field_0x239 < value) {
        self->field_0x239 = value;
    }
}

/* 0x802D69E4 - latches the +0x23C flag and raises the +0x23E timer. */
void fn_802D69E4(struct _AINPC_W* self, u8 flag, s16 value)
{
    self->field_0x23C = flag;
    if (self->field_0x23E < value) {
        self->field_0x23E = value;
    }
}

/* 0x802D6A00 - re-rolls the four per-direction slot levels: the two 0/1 rolls at +0x3F6/+0x3F7
 * pick four entries of the shared level tables, and the best of them becomes the +0x414 timer. */
void fn_802D6A00(struct _AINPC_W* self)
{
    s16 best = 0;
    u16 w[4];
    u8 i;

    self->field_0x414 = 0;
    w[0] = (u16)(self->field_0x3F6 * 5);
    w[1] = (u16)(self->field_0x3F6 * 4 + self->field_0x3F7);
    w[2] = (u16)(self->field_0x3F7 * 5);
    w[3] = (u16)(self->field_0x3F7 * 4 + self->field_0x3F6);
    for (i = 0; i < 4; i++) {
        u16 v = w[i];
        s16 value;

        self->field_0x3F8[i] = lbl_805D4190[v];
        self->field_0x3FC[i] = lbl_805D41A0[v];
        self->field_0x400[i] = lbl_805D41B0[v];
        value = (u8)fn_802D77A0(self->field_0x3FC[i]);
        if (best <= value) {
            best = value;
        }
    }
    self->field_0x414 = fn_802D77DC((u8)best);
    self->field_0x3F4 = (u16)(self->field_0x400[0] | self->field_0x400[1] | self->field_0x400[2] |
                              self->field_0x400[3]);
}

/* 0x802D6B2C - moves the +0x3D4 gauge and clamps it into [0, +0x3D6], then mirrors +0x3DA. */
void fn_802D6B2C(struct _AINPC_W* self, s16 delta)
{
    s16 value = self->field_0x3D4 + delta;

    self->field_0x3D4 = value;
    if (self->field_0x3D6 < value) {
        self->field_0x3D4 = self->field_0x3D6;
    }
    if (self->field_0x3D4 < 0) {
        self->field_0x3D4 = 0;
    }
    self->field_0x3DC = self->field_0x3DA;
}

/* 0x802D6B6C - raises the +0x3D6 cap by `delta`, clamped at 0x96. */
void fn_802D6B6C(struct _AINPC_W* self, s16 delta)
{
    if (self->field_0x3D6 < 0x96) {
        s16 value = self->field_0x3D6 + delta;

        self->field_0x3D6 = value;
        if (value >= 0x96) {
            self->field_0x3D6 = 0x96;
        }
    }
}

/* 0x802D6B98 - latches the +0x3D8 gauge into both the value and its cap, and mirrors +0x3DA. */
void fn_802D6B98(struct _AINPC_W* self)
{
    self->field_0x3D6 = self->field_0x3D8;
    self->field_0x3D4 = self->field_0x3D8;
    self->field_0x3DC = self->field_0x3DA;
}

/* 0x802D7464 - the skill load-out: copies the three skill slots out of `lb_param_w`, bumps the two skill counters per
 * unlocked skill and scales the +0x3D4 gauge by the resulting percentage. */
void fn_802D7464(struct _AINPC_W* self)
{
    s32 scale = 100;

    self->field_0x431 = 0;
    self->field_0x434 = 100;
    self->field_0x438 = 100;
    self->skill_0x42E[0] = lb_param_w.sub_0x26;
    self->skill_0x42E[1] = lb_param_w.sub_0x27;
    self->skill_0x42E[2] = lb_param_w.sub_0x28;
    if (ai_skill_ck(self, 2) == 1 || ai_skill_ck(self, 3) == 1 || ai_skill_ck(self, 4) == 1 ||
        ai_skill_ck(self, 5) == 1 || ai_skill_ck(self, 6) == 1 || ai_skill_ck(self, 8) == 1 ||
        ai_skill_ck(self, 9) == 1 || ai_skill_ck(self, 0xA) == 1) {
        self->field_0x431 = 1;
    }
    if (ai_skill_ck(self, 0xE) == 1) {
        self->field_0x434 += 0x14;
    }
    if (ai_skill_ck(self, 0xF) == 1) {
        self->field_0x438 += 0x14;
    }
    if (ai_skill_ck(self, 0x13) == 1) {
        self->field_0x434 += 0x14;
        self->field_0x438 += 0x14;
        scale = 0x78;
    }
    if (ai_skill_ck(self, 0x16) == 1) {
        self->field_0x434 += 0x32;
        self->field_0x438 += 0x32;
        scale += 0x32;
    }
    self->field_0x3D4 = (s16)(self->field_0x3D4 * scale / 100);
    self->field_0x3D6 = self->field_0x3D8 = self->field_0x3D4;
}

/* 0x802D7688 - which of the four behaviour lists the given id belongs to (3 when in none). */
s32 fn_802D7688(u8 value)
{
    s32 i;

    for (i = 0; i < 4; i++) {
        u8* p = (u8*)lbl_805D4390[i];

        while (*p != 0) {
            if (*p == value) {
                return i;
            }
            p++;
        }
    }
    return 3;
}

/* 0x802D773C - the behaviour-list record for the given index. */
u32 fn_802D773C(u8 index)
{
    return ((u32*)lbl_805D4358)[index];
}

/* 0x802D7754 - the four hold-countdown tuning values for the two 0/1 rolls at +0x3F6/+0x3F7. */
void fn_802D7754(u8 first, u8 second, u16* out)
{
    out[0] = lbl_805D41A0[first * 5];
    out[1] = lbl_805D41A0[first * 4 + second];
    out[2] = lbl_805D41A0[second * 5];
    out[3] = lbl_805D41A0[second * 4 + first];
}

/* 0x802D77A0 - the 14-entry level lookup: which of the three level values a slot level index maps
 * to (the table at 0x805D5428 is MWCC's own output for this switch). */
s32 fn_802D77A0(u8 value)
{
    switch (value) {
    default:
        return 0;
    case 1:
    case 3:
    case 6:
    case 9:
    case 12:
    case 13:
        return 1;
    case 2:
    case 4:
    case 7:
    case 10:
    case 11:
        return 2;
    }
}

/* 0x802D77DC - the +0x414 timer for the best slot level, in frames. */
s32 fn_802D77DC(u8 slot)
{
    if (slot == 1) {
        return 0x32;
    }
    s32 value = 0x1E;

    if (slot == 2) {
        value = 0x46;
    }
    return value;
}

/* 0x802D7A50 - raises the player-side AI NPC's "release" flag when the record is live, then
 * forwards to the shared dispatcher. */
void fn_802D7A50(void)
{
    if (ainpc_w.active == 0) {
        return;
    }
    ainpc_w.field_0x440 = 1;
    fn_802D4218(&ainpc_w);
}

/* 0x802D7A74 - whether one of the four hold slots is holding the live-chain item (0x1D) while the
 * AI NPC is in the +0x420 == 4 state. */
s32 fn_802D7A74(struct _AINPC_W* self)
{
    if (self->field_0x420 != 4) {
        return 0;
    }
    if (self->field_0x1CC != 0 && self->field_0x1CC != 5) {
        return 0;
    }
    if (self->hold_id_0x3B0 == 0x1D && self->field_0x424 == 0) {
        return 1;
    }
    if (self->hold_id_0x3B4 == 0x1D && self->field_0x424 == 0) {
        return 1;
    }
    if (self->hold_id_0x3B8 == 0x1D && self->field_0x424 == 0) {
        return 1;
    }
    if (self->hold_id_0x3BC == 0x1D && self->field_0x424 == 0) {
        return 1;
    }
    return 0;
}

/* 0x802D7B24 - whether the +0x171 == 1 state's sub-state is one of the two 0x20/0x3D.. ranges. */
s32 fn_802D7B24(struct _AINPC_W* self)
{
    u16 sub;

    if (self->field_0x171 != 1) {
        return 0;
    }
    sub = self->field_0x172;
    if ((u32)(sub - 0x20) <= 0x13) {
        return 1;
    }
    if ((u32)(sub - 0x3D) <= 0x13) {
        return 1;
    }
    return 0;
}

/* 0x802D7B5C - the hold-slot record index the item id falls in. */
s32 fn_802D7B5C(u16 value)
{
    u8 i;

    for (i = 1; i < 0x1E; i++) {
        if (value < lbl_805D43A0[(i + 1) * 4]) {
            break;
        }
    }
    return i;
}

/* 0x802D7BA4 - the second field of the hold-slot record. */
u16 fn_802D7BA4(u8 index)
{
    return lbl_805D43A0[index * 4 + 1];
}

/* 0x802D7BC4 - the third field of the hold-slot record. */
u16 fn_802D7BC4(u8 index)
{
    return lbl_805D43A0[index * 4 + 2];
}

/* 0x802D7BE4 - the fourth field of the hold-slot record. */
u16 fn_802D7BE4(u8 index)
{
    return lbl_805D43A0[index * 4 + 3];
}

/* 0x802D7C04 - the hold-range record index the item id falls in. */
s32 fn_802D7C04(u16 value)
{
    u8 i;

    for (i = 0; i < 0x14; i++) {
        if (value < lbl_805D44EC[(i + 1) * 2]) {
            break;
        }
    }
    return i;
}

/* 0x802D7C4C - the upper bound of the hold-range record. */
u16 fn_802D7C4C(u8 index)
{
    return lbl_805D44EC[index * 2 + 1];
}

/* 0x802D7C6C - the hold-range record index the item id falls in, over the 0x805D4560 table. */
s32 fn_802D7C6C(u16 value)
{
    u8 i;

    for (i = 0; i < 5; i++) {
        if (value < lbl_805D4560[i + 1]) {
            break;
        }
    }
    return i;
}

/* 0x802D7CB0 - the hang time the AI NPC's wake-up skill grants. */
s32 fn_802D7CB0(struct _AINPC_W* self)
{
    if (ai_skill_ck(self, 0x12) == 1) {
        return 5;
    }
    return 0xA;
}

/* 0x802D7CE4 - the "found the player" reaction: state 0x1A in a live area drops the AI NPC into
 * step 7. */
void fn_802D7CE4(u8 state)
{
    if (state != 0x1A) {
        return;
    }
    if (ainpc_w.active == 0) {
        return;
    }
    if (ai_area_ck(&ainpc_w) == 0) {
        return;
    }
    fn_802D2A00(&ainpc_w, 7, 0, 0);
}

/* 0x802D7D4C - the matching "lost the player" reaction for the +0x171 == 7 state. */
void fn_802D7D4C(void)
{
    if (ainpc_w.active == 0) {
        return;
    }
    if (ai_area_ck(&ainpc_w) == 0) {
        return;
    }
    if (ainpc_w.field_0x171 != 7) {
        return;
    }
    fn_802D2A00(&ainpc_w, 7, 1, 0);
}

/* 0x802D7DB4 - whether the +0x420 == 5 state's timer is running. */
s32 ai_npc_hold_ck(struct _AINPC_W* self)
{
    if (self->active == 0) {
        return 0;
    }
    if (self->field_0x420 != 5) {
        return 0;
    }
    return self->field_0x422 != 0;
}

/* 0x802D7DF0 - sets the +0x374 gauge. */
void fn_802D7DF0(struct _AINPC_W* self, s16 value)
{
    self->field_0x374 = value;
}

/* 0x802D7DF8 - whether the +0x374 gauge is running. */
s32 fn_802D7DF8(struct _AINPC_W* self)
{
    if (self->active == 0) {
        return 0;
    }
    return self->field_0x374 != 0;
}

/* 0x802D7E20 - whether the AI NPC has arrived (its step counter is at 1). */
s32 ai_npc_arrived_ck(struct _AINPC_W* self)
{
    if (self->active == 0) {
        return 0;
    }
    return fn_802D2B38(self, 5, 3) == 1;
}

/* 0x802D7E68 - the same arrival test for the other motion slot. */
s32 fn_802D7E68(struct _AINPC_W* self)
{
    if (self->active == 0) {
        return 0;
    }
    return fn_802D2B38(self, 5, 2) == 1;
}

/* 0x802D7F10 - whether the id is one of the three slots of the list. */
s32 fn_802D7F10(u8* slots, u8 value)
{
    s32 i;

    for (i = 0; i < 3; i++) {
        if (slots[i] == value) {
            return 1;
        }
    }
    return 0;
}

/* 0x802D84D0 - sets the +0x47C gauge. */
void fn_802D84D0(struct _AINPC_W* self, s16 value)
{
    self->field_0x47C = value;
}

/* 0x802D948C - whether the item page at +0x483 is the first one. */
s32 fn_802D948C(struct _AINPC_W* self)
{
    return self->field_0x483 == 1;
}

/* 0x802D94A0 - whether the item page at +0x483 is the first or the third one. */
s32 fn_802D94A0(struct _AINPC_W* self)
{
    if (self->field_0x483 == 2) {
        return 1;
    }
    return self->field_0x483 == 3;
}

/* 0x802D9A40 - forwards to the page-arrow routine. */
void fn_802D9A40(void)
{
    fn_802D9544();
}

/* 0x802D9A44 - marks the AI NPC as having acknowledged the current item page. */
void fn_802D9A44(struct _AINPC_W* self)
{
    self->field_0x46C = 1;
}

/* 0x802D9A50 - whether the acknowledgement has already happened. */
s32 fn_802D9A50(struct _AINPC_W* self)
{
    if (self->active == 0) {
        return 0;
    }
    return self->field_0x46C == 1;
}

/* 0x802D7804 - whether the AI NPC is close enough to the player to act on it. */
s32 fn_802D7804(u8 index, f32 distance)
{
    _PLW* plw;

    if (ainpc_w.active == 0) {
        return 0;
    }
    plw = ainpc_w.plw_0x16C;
    if (plw == 0) {
        return 0;
    }
    if (distance < lbl_8079A670) {
        return 1;
    }
    if (ainpc_w.field_0x1A4 != plw->area_0x16) {
        return 0;
    }
    if (ainpc_w.field_0x171 == 5) {
        return 0;
    }
    if (index == 4 && fn_802D9CE0(&ainpc_w) == 1) {
        return 0;
    }
    return fn_80050EAC(&ainpc_w.vec_0x178, (nw4r::math::VEC3*)&plw->motion_pos_0x3C) <
           distance * distance;
}

/* 0x802D78FC - whether the player is inside the AI NPC's current attack reach. */
s32 fn_802D78FC(void)
{
    _PLW* plw;

    if (ainpc_w.active == 0) {
        return 0;
    }
    if (ainpc_w.field_0x420 != 2) {
        return 0;
    }
    plw = ainpc_w.plw_0x16C;
    if (plw == 0) {
        return 0;
    }
    if (ainpc_w.field_0x1A4 != plw->area_0x16) {
        return 0;
    }
    if (ainpc_w.field_0x422 == 0) {
        return 0;
    }
    return fn_80050EAC(&ainpc_w.vec_0x178, (nw4r::math::VEC3*)&plw->motion_pos_0x3C) <
           lbl_8079A870;
}

#ifdef __cplusplus
}
#endif

/* 0x802D7640 - whether `skill` occupies any of the three skill slots copied out of `lb_param_w`. */
u32 ai_skill_ck(struct _AINPC_W* self, u8 skill)
{
    s32 i;

    for (i = 0; i < 3; i++) {
        if (skill == self->skill_0x42E[i]) {
            return 1;
        }
    }
    return 0;
}

/* One 0x20-byte entry of `ainpc_entry_tbl`: `ai_slots_clear` clears the sixteen of them with `fn_802DA200`. */
struct AinpcEntry {
    /* +0x00 */ u8 pad_0x00[0x20];
}; /* size: 0x20 */

/* The page state `fn_802DB2F0` and its helpers keep at `ainpc_page_state`: +0x00 and +0x26 are cleared per
 * page, +0x01 is saved across the clear. */
struct AinpcPageState {
    /* +0x00 */ u8 active_0x00;
    /* +0x01 */ u8 saved_0x01;
    /* +0x02 */ u8 pad_0x02[0x24];
    /* +0x26 */ u8 flag_0x26;
    /* +0x27 */ u8 pad_0x27[0x29];
}; /* size: 0x50 */

/* This unit's own `.bss` (0x806BD360-0x806BD808), defined at the foot of the file, after every use.
 * `ainpc_w` is constructed by the `.ctors` word `fn_802D9E14` (a tail call into the record's constructor
 * `fn_802D9E20`, not reconstructed here). */
_AINPC_W ainpc_w;                      /* +0x806BD360 */

