/*
 * enemy/em_prog_support.cpp - unit, `.text` 0x80383148..0x80385EE0 (75 functions, 11672 bytes).
 *
 * 47 of 75 functions have a body here.
 *
 * FLAGS.  `cflags_main`.  GUESS (rule 7): the stem names the shared support block of the note-pane band; the four
 * helper names `note_pane_mode_set`, `note_pane_motion_set`, `note_pane_motion_end_ck`, `qn_chr_flag_set` are derived
 * from their bodies (their map rows were `note_pane_mode_set`, `note_pane_motion_set`, `note_pane_motion_end_ck`, `qn_chr_flag_set`).
 *
 * Sections: the unit's block in config/RMHE08/splits.txt (.bss, .ctors, .data, .sbss, .sdata, .sdata2, .text, extab,
 * extabindex).
 */
/* ---- header inherited from src/enemy/fn_80382310.cpp (written against its pre-phase-4 range) ---- */
/* enemy/fn_80382310.cpp - the enemy `em009`/`em019` program band's shared support block, `.text`
 * 0x80382310..0x803868DC (the tail 0x803868DC..0x80387844 moved to `enemy/em009_act.cpp` in the
 * 2026-09-30 recut; the range still holds more than one TU, see SEAM).
 *
 * Naming note: the symbol map has only fn_XXXXXXXX for this range (checked with
 * `python tools/symbols/dumpmap.py lookup 0x80382310` -> `zz_0382310_`; the map's own rows for the
 * range in config/RMHE08/symbols.txt carry no real name either), and no `__FILE__` string is
 * referenced by any body - every `lis`/`addi` and every `@sda21` relocation in the range resolves to
 * the `.sdata2` float pool, a switch/jumptable or one of the band's own record tables, never to a
 * source-file-name literal (checked by reading all `R_PPC_*` relocations of the range's 118 split
 * objects and cstring-ing each referenced `data:string` label in orig/RMHE08/sys/main.dol: of the 302
 * bare `<name>.c/.cpp/.h` string labels in the image, none is referenced from 0x80382310..0x80387844).
 *
 * MODULE AND NAME (brief section 2, evidence order).  1. No `__FILE__` string (above).  2. `dumpmap.py
 * lookup` answers only `zz_XXXXXXXX_` placeholders.  3. The code places the unit in `enemy`: the
 * immediately following registered unit is `enemy/em009_act.cpp` (0x80387844, the monster-AI action
 * band), the immediately preceding `.data` is the enemy program-table block (`em019_prog_tbl`
 * at 0x805EE518, its table run 0x805EE584..0x805EE5B0 that `fn_80382310` indexes, `em009_prog_tbl` at
 * 0x805EF990, and the `jumptable_805EF4F4`/`jumptable_805EF52C` switch tables of the same band), and
 * every body drives the shared `_ENEMY_WORK` record through `em_frame_check__FP11_ENEMY_WORKUsff`,
 * `em_act_ck__FP11_ENEMY_WORKUcUc`, `get_joint_wpos_em__FP11_ENEMY_WORKUlPQ34nw4r4math4VEC3`,
 * `get_em_chg_scale__FP11_ENEMY_WORK` and `em_magma_check`.  The file therefore keeps the map's
 * `fn_80382310` stem (brief option 4); no name was invented and no module was guessed.
 *
 * LANGUAGE AND SECTIONS.  C++ (langcheck: the range defines one mangled function,
 * `qn_get_motion_no__FP7_QNPC_W`, and reaches genuinely mangled callees - `MHchar::getTevKColor`/
 * `setTevKColor`, `setVector3__FPQ34nw4r4math4VEC3fff`, `get_joint_wpos_em__FP11_ENEMY_WORK...` -
 * through their real signatures, rule 9).  Every plain `fn_` definition is `extern "C"` so it keeps
 * the map's name (playbook 42).  Sections claimed: `.text` 0x80382310..0x803868DC, extab
 * 0x80017E2C..0x8001803C, extabindex 0x80037BA8..0x80037EC0, `.ctors` 0x8056F3A8..0x8056F3B0, `.data`
 * 0x805EF8C0..0x805EF990, `.bss` 0x806C23E8..0x806C5488 and `.sbss` 0x80794C00..0x80794C08.  The band's
 * `.sdata2` tables live in other splits and are referenced here as the map's `lbl_`/`jumptable_`
 * symbols, never re-emitted (rule 10 / rule 2).
 *
 * SEAM.  The right edge 0x803868DC starts the `em009` TU (`enemy/em009_act.cpp`).  The range is still
 * more than one TU, not split yet: the two `.ctors` words (`fn_8038309C`, `fn_80385E7C`) are two TUs'
 * static initializers (the first TU ends at 0x80383148; `fn_80385E7C` constructs the `lbl_806C4A88`
 * array with `fn_80385E9C`, emitted after it), and the pool repeats 41c00000 at `lbl_8079BFAC` (first
 * read by `fn_803865B4`), so a third TU starts in 0x80385E7C..0x80386028 (taken as
 * 0x80385EE0..0x803868DC: GUESS).  The left edge 0x80382310 is discovery's cap, not a proven boundary -
 * `tudiscover.py at 0x80382310` extends the range left to 0x8037F940 on its strong cuts.
 *
 * RECONSTRUCTION STATUS (measured with `tools/units/recompile.py enemy/fn_80382310 --measure <sym>`,
 * the official report metric, against MAIN's retired split objects `auto_fn_*_text.o`).  72 of the
 * range's 118 functions have a body (5068 of 21812 `.text` bytes, 23.23 %); 34 are byte-identical and
 * 61 measure >= 80 %.  The unit is short of the 80 % bar: the 72 bodies written this session are the
 * band's mechanical half - the colour/slot accessors, the note-pane allocator/constructor/destructor
 * set, the eleven sub-state machines and the tail-call dispatchers - and the band's byte mass sits in
 * the large bodies not attempted yet (fn_80384434 0x700, fn_80386C9C 0x600, fn_80382310 0x46C,
 * fn_803865B4 0x328, fn_803828B8 0x2F8, fn_80386A04 0x298, fn_80387620 0x224, ...) - the 46
 * functions without a body cover 16744 bytes.
 *
 * RESIDUALS (what still differs and why):
 *   - **`fn_80384ECC` (14.5 %).**  The retail body dispatches through the 17-entry jump table
 *     `jumptable_805EF94C`; the conformant `switch (a) { case 0..3 }` spells the same predicate but MWCC
 *     emits a compare chain here (104 B vs 124 B).  The jump-table shape needs an explicit case per
 *     index; recorded, not forced.
 *   - **`fn_803851D4` (48.6 %).**  The five-slot allocator is written from the target's unrolled shape
 *     but indexes `lbl_806C4A88[i]` (156 B vs 152 B): the retail body walks a pointer by +0x1F8 instead.
 *   - **`note_pane_motion_end_ck`/`fn_80385C70`/`fn_80385C80` (60/45/45 %).**  The three MHchar tail-call thunks
 *     differ in the argument-narrowing the compiler inserts before the `b` (12 B vs the target's 16 B
 *     for the two 3-argument forms).
 *   - **`note_pane_set_anim_pair` (60 %).**  The body is right but MWCC knows the u8 parameters are already narrow
 *     and drops the two `clrlwi` the retail object carries (12 B vs 20 B).
 *   - **`fn_80384B34` (74.8 %), `fn_803857BC` (76.8 %), `fn_803852B8` (79.3 %), `fn_80382C00` (73.4 %),
 *     `fn_80382F94` (90.9 %).**  Sign/narrowing and load-order residuals inside otherwise-correct bodies.
 *
 * The per-symbol table is in the outbox .pi/outbox/80382310-fn-80382310-2982.json and the batch note
 * .pi/notes/80382310-fn-80382310-2982.md.
 */

#include "types.h"
#include "hud/cockpit.h"
#include "enemy/em_prog_tail.h"
#include "stage/stg_w.h"
#include "nw4r/math.h"
#include "sound/mhchar.h"
#include "enemy/ENEMY_WORK.h"
#include "enemy/fn_8011D448.h"
#include "enemy/fn_8012E968.h"
#include "enemy/fn_80138074.h"
#include "enemy/fn_80147CE0.h"
#include "sound/fn_800DD1F0.h"
#include "mh3_pad.h" /* VEC3_ctor / copyVec3 / setVec3 (rule 2) */
#include "enemy/note_work.h" /* NoteWork and the pane/slot/layout types (rule 1) */
#include "lobby/lb_quest_screen.h" /* note_pane_get_motion (rule 2: the owner's header) */
#include "ef/pRoot.h"
#include "lobby/lb_server_sel_trans.h" /* fn_803C7EAC / fn_803C7F88 (owner's header, rule 2) */
#include "Runtime.PPCEABI.H/CPlusLibPPC.h" /* __construct_array (owner's header, rule 2) */

/* --- the declarations this band's bodies need (the owners are not registered yet; the address of
 * each sits inside 0x80380000.., the band this unit opens) ------------------------------- */

extern "C" {
/* The note band's own still-unwritten members (declared here until their bodies land below). */
void fn_80217934(void);
void fn_802125C8(void);
void fn_8021F248(u8 a);
void fn_801FC2F0(void);

struct _QNPC_W;
struct _g3d_work;

/* The enemy record's note pane (the fields `fn_803843D8` seeds): `+0x14` is `_ENEMY_WORK::field_0x14`
 * and the two bytes it clears/arms are the pane's own.
 * size: 0x1A0 */
struct NotePane {
    /* +0x000 */ u8 pad_0x000[0x014];
    /* +0x014 */ u8 field_0x014;
    /* +0x015 */ u8 pad_0x015[0x01C - 0x015];
    /* +0x01C */ u8 field_0x01C;
    /* +0x01D */ u8 pad_0x01D[0x19E - 0x01D];
    /* +0x19E */ u16 field_0x19E;
};

/* The note record types this band shares with the band above it (0x803A3A50..) live in
 * `enemy/note_work.h` (rule 1: one definition, included).
 */

/* The band's own data / unsplit globals (referenced, never defined - rule 2/10). */
extern u8* lobby_world_block;         /* .sbss 0x80794880 - the 4-byte block pointer `fn_803836EC` reads */
extern NoteWork lbl_806C4A88[5]; /* .bss  0x806C4A88 - the retail 5-record note array (0x9D8) */
extern NoteSlot lbl_806C5460[3]; /* .bss  0x806C5460 - the 3-slot seat set (0x28) */
extern NoteLayout lbl_806C2418;  /* .bss  0x806C2418 - the note layout state (0x2670) */
extern NoteScreenScale Screen_w; /* .bss  0x8065903C - the screen frame scale */
extern void* q_npc_snd_func;     /* .sbss 0x80794C00 - the NPC sound callback pointer */
extern u32* lbl_805EF940[3];     /* .data 0x805EF940 - the per-kind model-table run `fn_80385B28` indexes */
extern f32 lbl_8079BF60;         /* .sdata2 - the note pen's width factor */
}

/* `qn_get_motion_no__FP7_QNPC_W` is a C++ free function (the map name carries the mangling), so its
 * declaration must sit at C++ scope with the `_QNPC_W` parameter spelling to reproduce it (rule 9). */
u16 qn_get_motion_no(_QNPC_W* self);

extern "C" {
void fn_80384FF8(u8 idx);
NoteWork* fn_803851D4(void);
NoteWork* fn_803853C8(u8 idx);
void fn_8038530C(u8 idx);
void fn_80385478(NoteWork* self);
void fn_803857BC(NoteWork* self);
void fn_80385AA8(NoteWork* self, u8 a, u8 b);
void note_pane_set_anim_pair(NoteWork* self, u32 a, u32 b);
void fn_803854E4(void);
void fn_80385598(void);
void fn_803852B8(void);
void fn_800D58B0(s32 handle);
s32 fn_800D9804(u32 a, void* b, void* c);
void mhchar_reset(MHchar* self);
void mhchar_construct(void* self);
s32 fn_80383F0C(_ENEMY_WORK* self, s16 a);
}

extern "C" void fn_80385CA0(_QNPC_W* self);
extern "C" NoteWork* fn_80385E9C(NoteWork* self);
struct _g3d_work;
void push_g3d_wk(_g3d_work* wk);

/* The band's plain-name callees. */

extern "C" {
void res_file_ctor(void* obj, s32 flag);
void fn_800D8E44(s32 handle);
void fn_800E26C4(void* chr);
void g3d_root_model_bind(s32 root, u32 id);
void fn_80385828(NoteWork* self);
void fn_803858F8(NoteWork* self);
void fn_800E1280(MHchar* self, u32 a, u16 b, u32 c, u32 d, u32 e, f32 f, f32 g);
extern f32 lbl_8079BF6C;
}

/* 0x803831B0 - a tail-call thunk. */
extern "C" void fn_803831B0(void) {
    fn_80217934();
}

/* 0x803836EC - the note band's per-frame step: refresh, then rebuild the note pane from the block
 * `lobby_world_block` points at. */
extern "C" void fn_803836EC(void) {
    fn_802125C8();
    fn_8021F248(lobby_world_block[15872]);
    fn_802B4C5C();
    fn_801FC2F0();
}

/* 0x8038392C */
extern "C" u32 fn_8038392C(void) {
    return lbl_806C2418.cursor_x_0x00 != 0;
}

/* 0x803839C0 */
extern "C" void fn_803839C0(void) {
    lbl_806C2418.field_0x004 = 0;
    lbl_806C2418.field_0x005 = 0;
    lbl_806C2418.field_0x006 = 0;
    lbl_806C2418.field_0x007 = 0;
    lbl_806C2418.field_0x008 = 4;
    lbl_806C2418.field_0x009 = 0;
}

/* 0x80384004 */
extern "C" void fn_80384004(_ENEMY_WORK* self, s16 a) {
    s32 r = fn_80383F0C(self, a);
    if ((s16)r >= 0) {
        self->state_0x006 = (u8)r;
    }
}

/* 0x80384304 */
extern "C" void fn_80384304(void) {
    lbl_806C2418.field_0x009 = 1;
    lbl_806C2418.field_0x007 = 1;
    lbl_806C2418.field_0x008 = 12;
}

/* 0x80384324 - the note pen: seed the layout state from the screen frame scale, then rebuild. */
extern "C" void fn_80384324(void) {
    lbl_806C2418.field_0x008 = 4;
    lbl_806C2418.cursor_x_0x00 = (u16)(lbl_8079BF60 * Screen_w.frame_scale);
    lbl_806C2418.field_0x003 = 0;
    fn_802DFC6C();
}

/* 0x80384380 */
extern "C" void fn_80384380(void) {
    lbl_806C2418.field_0x009 = 0;
    lbl_806C2418.field_0x007 = 0;
    lbl_806C2418.field_0x008 = 4;
    lbl_806C2418.field_0x006 = lbl_806C2418.field_0x004;
}

/* 0x803843A8 */
extern "C" u32 fn_803843A8(void) {
    if (lbl_806C2418.field_0x009 != 0) {
        if (lbl_806C2418.field_0x007 == 1) {
            return 1;
        }
    }
    return 0;
}

/* 0x803843D8 - the pane reset `fn_80384304` seeds. */
extern "C" void fn_803843D8(NotePane* self, u8 a) {
    self->field_0x014 = a;
    self->field_0x19E = 0;
    self->field_0x01C = 0;
    fn_80384304();
    self->field_0x01C = (s8)(s16)(lbl_806C2418.field_0x008 - 1);
    fn_802DFC6C();
}

/* 0x80384F48 */
extern "C" u32 fn_80384F48(u8 a, u8 b) {
    if (b == 0) {
        return 1;
    }
    if (a == 9 && b == 1) {
        return 1;
    }
    return 0;
}

/* 0x80385068 */
extern "C" void fn_80385068(void) {
    for (u32 i = 0; i < 3; i++) {
        fn_80384FF8((u8)i);
    }
}

/* 0x8038526C */
extern "C" void fn_8038526C(NoteWork* self) {
    mhchar_reset(&self->model);
    self->field_0x000 = 0;
    self->field_0x001 = 0;
    self->field_0x168 = 0;
    self->field_0x169 = 0;
    self->field_0x16A = 0;
    self->field_0x16B = 0;
}

/* 0x803852B8 */
extern "C" void fn_803852B8(void) {
    for (u32 i = 0; i < 5; i++) {
        fn_8038526C(&lbl_806C4A88[i]);
    }
    fn_803C7EAC();
}

/* 0x803853C8 */
extern "C" NoteWork* fn_803853C8(u8 idx) {
    NoteWork* p = fn_803851D4();
    if (p != 0) {
        fn_8038530C(idx);
    }
    return p;
}

/* 0x80385414 */
extern "C" void* fn_80385414(void) {
    return q_npc_snd_func;
}

/* 0x8038541C */
extern "C" void fn_8038541C(u8 idx) {
    NoteWork* p = fn_803853C8(2);
    p->field_0x198 = idx;
    p->field_0x1F4 = fn_800D9804(10, p, fn_80385414());
}

/* 0x803854E4 */
extern "C" void fn_803854E4(void) {
    for (u32 i = 0; i < 5; i++) {
        fn_80385478(&lbl_806C4A88[i]);
    }
    fn_80385598();
}

/* 0x80385538 */
extern "C" void fn_80385538(u8 idx) {
    NoteSlot* slot = &lbl_806C5460[idx];
    if (slot->field_0x01 != 0) {
        slot->field_0x00 = idx;
        slot->field_0x01 = 0;
        fn_800D58B0(slot->handle_0x04);
        slot->handle_0x04 = -1;
    }
}

/* 0x80385598 */
extern "C" void fn_80385598(void) {
    for (u32 i = 0; i < 3; i++) {
        fn_80385538((u8)i);
    }
}

/* 0x8038575C */
extern "C" void fn_8038575C(void) {
    for (u32 i = 0; i < 5; i++) {
        if (lbl_806C4A88[i].field_0x000 != 0) {
            fn_803857BC(&lbl_806C4A88[i]);
        }
    }
    fn_803C7F88();
}

/* 0x80385A54 */
extern "C" void fn_80385A54(NoteWork* self) {
    switch (self->field_0x003) {
    case 1:
        note_pane_get_motion(self);
        return;
    case 2:
        fn_803865B4(self);
        return;
    }
}

/* 0x80385A78 */
extern "C" void fn_80385A78(NoteWork* self) {
    self->model.field_0x2C = self->field_0x188;
    self->model.field_0x30 = self->field_0x18C;
    self->model.field_0x34 = self->field_0x190;
    copyVec3(&self->model.pos_0x04, &self->vec_0x170);
}

/* 0x80385AA0 */
extern "C" void note_pane_mode_set(NoteWork* self, u8 a) {
    self->field_0x19C = a;
}

/* 0x80385AA8 */
extern "C" void fn_80385AA8(NoteWork* self, u8 a, u8 b) {
    self->field_0x169 = 0;
    self->field_0x16A = 0;
    self->field_0x16B = 0;
    self->field_0x19E = self->field_0x19D;
    self->field_0x1A0 = self->field_0x19F;
    self->field_0x19D = a;
    self->field_0x19F = b;
    fn_80385CA0((_QNPC_W*)self);
}

/* 0x80385AD4 */
extern "C" void note_pane_set_anim_pair(NoteWork* self, u32 a, u32 b) {
    self->field_0x1A1 = 1;
    fn_80385AA8(self, (u8)a, (u8)b);
}

/* 0x80385AE8 */
extern "C" u32 note_pane_anim_pair_ck(NoteWork* self, u8 a, u8 b) {
    if (self->field_0x19D == a && self->field_0x19F == b) {
        return 1;
    }
    return 0;
}

/* 0x80385B18 */
extern "C" void fn_80385B18(NoteWork* self, u32 joint, nw4r::math::MTX34* out) {
    fn_800E0A14(&self->model, joint, out);
}

/* 0x80385B20 */
extern "C" void fn_80385B20(NoteWork* self, u32 joint, nw4r::math::VEC3* out) {
    self->model.get_joint_wpos(joint, out);
}

/* 0x80385B28 */
extern "C" u32 fn_80385B28(NoteWork* self, u16 index) {
    if (index > 20) {
        return 0;
    }
    return lbl_805EF940[self->field_0x003][index];
}

/* 0x80385C64 */
extern "C" u32 note_pane_motion_end_ck(NoteWork* self) {
    return fn_800E2198(&self->model, 0);
}

/* 0x80385C70 */
extern "C" u32 fn_80385C70(NoteWork* self, u32 a) {
    return fn_800E16DC(&self->model, (u16)a, 0);
}

/* 0x80385C80 */
extern "C" u32 fn_80385C80(NoteWork* self, u32 a) {
    return fn_800E16DC(&self->model, (u16)a, 1);
}

/* 0x80385C90 - the quest NPC's motion number. */
u16 qn_get_motion_no(_QNPC_W* self) {
    return (*self).motion_no_0x54;
}

/* 0x80385C98 */
extern "C" void qn_chr_flag_set(_QNPC_W* self, u8 a) {
    (*self).field_0xF6 = a;
}

/* 0x80385CA0 */
extern "C" void fn_80385CA0(_QNPC_W* self) {
    (*self).field_0xF6 = 0;
}

/* 0x80385E7C */
extern "C" void fn_80385E7C(void) {
    __construct_array(lbl_806C4A88, (void*)fn_80385E9C, 0, 504, 5);
}

/* 0x80385E9C */
extern "C" NoteWork* fn_80385E9C(NoteWork* self) {
    mhchar_construct(&self->model);
    VEC3_ctor(&self->vec_0x170);
    VEC3_ctor(&self->vec_0x17C);
    return self;
}

/* ---------- batch 2: the state machines, the slot set and the pane steps ---------- */

/* 0x80384B34 - the seat record lookup (the 100-byte-stride table at `self + 12`). */
extern "C" u8* fn_80384B34(u8* self, s8 a) {
    s8 v;
    if (self[8] < self[5]) {
        v = (s8)(a - self[8] - 1);
        v = (s8)(v - self[6] - 1);
        if (v < 0) {
            v = (s8)(v + 96);
        }
    } else {
        v = (s8)((s8)(self[5] - self[8]) - a);
    }
    return self + v * 100 + 12;
}

/* 0x80384ECC - the per-(kind, sub) admission table. */
extern "C" u32 fn_80384ECC(u8 a, u8 b) {
    switch (a) {
    case 0:
        return b == 11;
    case 1:
        return b == 8;
    case 2:
        return b == 0;
    case 3:
        return b == 10;
    }
    return 0;
}

/* 0x80384F80 */
extern "C" u32 fn_80384F80(u8 a, u8 b) {
    if (a >= 21) {
        return 0;
    }
    if (fn_80384ECC(a, b) == 1) {
        return 1;
    }
    return fn_80384F48(a, b) == 1;
}

/* 0x80384FF8 - seed one seat record. */
extern "C" void fn_80384FF8(u8 idx) {
    u8 local[16];
    NoteSlot* slot = &lbl_806C5460[idx];
    res_file_ctor(local, 0);
    slot->field_0x00 = idx;
    slot->field_0x01 = 0;
    slot->field_0x02 = 0xFF;
    slot->handle_0x04 = -1;
    slot->handle_0x08 = -1;
}

/* 0x803851D4 - the five-slot allocator (unrolled: the retail body tests each record in turn). */
extern "C" NoteWork* fn_803851D4(void) {
    if (lbl_806C4A88[0].field_0x000 == 0) {
        lbl_806C4A88[0].field_0x002 = 0;
        return &lbl_806C4A88[0];
    }
    if (lbl_806C4A88[1].field_0x000 == 0) {
        lbl_806C4A88[1].field_0x002 = 1;
        return &lbl_806C4A88[1];
    }
    if (lbl_806C4A88[2].field_0x000 == 0) {
        lbl_806C4A88[2].field_0x002 = 2;
        return &lbl_806C4A88[2];
    }
    if (lbl_806C4A88[3].field_0x000 == 0) {
        lbl_806C4A88[3].field_0x002 = 3;
        return &lbl_806C4A88[3];
    }
    if (lbl_806C4A88[4].field_0x000 == 0) {
        lbl_806C4A88[4].field_0x002 = 4;
        return &lbl_806C4A88[4];
    }
    return 0;
}

/* 0x80385478 - tear one seat record down. */
extern "C" void fn_80385478(NoteWork* self) {
    if (self->field_0x000 != 0) {
        fn_800E26C4(&self->model);
        if (self->view.g3d_0x110 != 0) {
            push_g3d_wk(self->view.g3d_0x110);
            self->view.g3d_0x110 = 0;
        }
        if (self->field_0x1F4 != 0) {
            fn_800D8E44(self->field_0x1F4);
        }
        fn_8038526C(self);
    }
}

/* 0x803857BC - the note pane's per-record step. */
extern "C" void fn_803857BC(NoteWork* self) {
    u8 v = self->field_0x168;
    if ((u8)(v - 2) > 1) {
        if (v == 0) {
            self->field_0x168 = v + 1;
            fn_80385828(self);
        } else if (v == 1) {
            fn_803858F8(self);
        }
    }
    g3d_root_model_bind(pRoot, self->model.field_0x118);
}

/* 0x80385B5C */
extern "C" void fn_80385B5C(NoteWork* self, u16 a, u32 b, s32 c) {
    if (a <= 16) {
        fn_800E1280(&self->model, 0, a, b, fn_80385B28(self, a), 0, (f32)c, lbl_8079BF6C);
    }
}

/* 0x80385BF4 */
extern "C" void note_pane_motion_set(NoteWork* self, u16 a, u32 b, u32 c) {
    if ((u16)qn_get_motion_no((_QNPC_W*)self) != a) {
        fn_80385B5C(self, a, b, c);
    }
}

