/* enemy/em_prog_support.cpp - the note-pane band's support block between the em019 and em009 programs: the colour/slot
 *   accessors, the note-pane allocator/constructor/destructor set, the sub-state machines and the MHchar tail-call
 *   thunks.
 * RANGE. .text 0x80383148-0x80385EE0 (75 functions); .ctors 0x8056F3AC-0x8056F3B0 (`fn_80385E7C`),
 *   .data 0x805EF8C0-0x805EF990, .bss 0x806C2418-0x806C5528, .sdata 0x807933C0-0x807933D8, .sbss 0x80794C00-0x80794C08,
 *   .sdata2 0x8079BF60-0x8079BF90, extab, extabindex.
 * SEAM. The left edge closes `enemy/em019_ai.cpp`, whose static initializer `fn_8038309C` ends there.  This unit's
 *   own initializer `fn_80385E7C` constructs the `lbl_806C4A88` array with `fn_80385E9C`, emitted after it; the pool
 *   repeats 41c00000 at `lbl_8079BFAC` (first read by `fn_803865B4`), so the next TU starts in
 *   0x80385E7C-0x80386028 and the right edge 0x80385EE0 is a GUESS.
 * NAMES. `em_prog_support` and the `note_pane_*`/`qn_chr_flag_set` names are GUESSES from the band's role and the
 *   bodies; `qn_get_motion_no` is the map's mangled name.
 *   GUESS (from each body and its callers): em_prog_slots_init, em_prog_work_init, qnpc_load_ck, qnpc_res_load_done
 *   GUESS: `note_pane_motion_start`
 * RESIDUALS. 28 rows unwritten: 0x80383148-0x803831B0, 0x803831B4-0x803836EC, 0x80383720-0x8038392C,
 *   0x80383944-0x803839C0, 0x803839EC-0x80384004, 0x80384048-0x80384304, 0x80384434-0x80384B34, 0x80384BA0-0x80384ECC,
 *   0x803850A4-0x803851D4, 0x8038530C-0x803853C8, 0x803855D4-0x8038575C, 0x80385828-0x80385A54, 0x80385CAC-0x80385E7C.
 *  - `fn_80384ECC`: retail dispatches through the 17-entry `jumptable_805EF94C`, ours emits a compare chain
 *    (104 B against 124 B);
 *  - `fn_803851D4`: ours indexes `lbl_806C4A88[i]`, retail walks a pointer by +0x1F8;
 *  - `fn_80385A78`: ours copies the position to +0x30/+0x34 and a byte at +0x38, retail stores the three words to
 *    +0x2C/+0x30/+0x34;
 *  - `note_pane_motion_end_ck`, `fn_80385C70`, `fn_80385C80`: retail adds 4 to r3 before the argument setup of the
 *    tail call, ours after;
 *  - `fn_80384F48`, `qnpc_load_ck`, `fn_80384FF8`, `fn_803853C8`, `fn_8038541C`, `fn_80385538`, `fn_80385B28`,
 *    `note_pane_motion_start`, `note_pane_anim_pair_ck`, `note_pane_motion_set`: retail narrows the argument with `clrlwi`, ours
 *    drops it;
 *  - `fn_80384004`, `fn_80384B34`: retail keeps `extsh`/`extsb` + `cmpwi`, ours the record form; `fn_803843D8`:
 *    retail sign-extends twice more;
 *  - `fn_803852B8`, `fn_803854E4`, `fn_8038575C`: the loop's index setup is ordered differently; `fn_803857BC`: ours
 *    compares the narrowed value unsigned where retail compares signed.
 *   flipcheck: `.bss`/`.ctors`/`.data`/`.sbss`/`.sdata` claimed, not emitted; `.sdata2`/`.text`/extab/extabindex
 *   short of the claim.
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

/* The declarations this band's bodies need. */

extern "C" {
/* The lobby helpers the band calls (`lobby/fn_80212810.cpp`, `lobby/lb_npc.cpp`, `lobby/lb_menu_pos_tbl.cpp`). */
void lb_panel_close(void);
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

/* The note record types this band shares with the band at 0x803A3A50 are `enemy/note_work.h`'s. */

/* The data the band reads, declared and never defined. */
extern u8* lobby_world_block;         /* .sbss 0x80794880 (`userdata_item.cpp`), read by `fn_803836EC` */
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
void em_prog_work_init(void);
void nw_res_entry_clear(s32 handle);
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
    lb_panel_close();
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
extern "C" void em_prog_slots_init(void) {
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
extern "C" void em_prog_work_init(void) {
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
        nw_res_entry_clear(slot->handle_0x04);
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
    mhchar_joint_mtx_get(&self->model, joint, out);
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
extern "C" u32 qnpc_load_ck(u8 a, u8 b) {
    if (a >= 21) {
        return 0;
    }
    if (fn_80384ECC(a, b) == 1) {
        return 1;
    }
    return fn_80384F48(a, b) == 1;
}

/* 0x80384FF8 - seeds one seat record. */
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

/* 0x80385478 - tears one seat record down. */
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
extern "C" void note_pane_motion_start(NoteWork* self, u16 a, u32 b, s32 c) {
    if (a <= 16) {
        fn_800E1280(&self->model, 0, a, b, fn_80385B28(self, a), 0, (f32)c, lbl_8079BF6C);
    }
}

/* 0x80385BF4 */
extern "C" void note_pane_motion_set(NoteWork* self, u16 a, u32 b, u32 c) {
    if ((u16)qn_get_motion_no((_QNPC_W*)self) != a) {
        note_pane_motion_start(self, a, b, c);
    }
}

