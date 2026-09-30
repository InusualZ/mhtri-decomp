/* lobby/fn_801F9CD4.cpp - the lobby character-edit (hair/inner colour) screen group.
 *
 * `.text` 0x801F9CD4..0x801FBF78 (14 functions, 8868 B), extab 0x80010A7C..0x80010ADC (12 unwind-only
 * 8-byte records), extabindex 0x8002CEA4..0x8002CF34 (12 x 12 B).  Registered from
 * `proposal/801F9CD4_fn_801F9CD4.cpp`; the extent is a maximal unclaimed run whose seam is unproven.
 *
 * Module `lobby`: the range reads the lobby state blocks (`lobby_w`, `lb_param_w`, `Screen_w`,
 * `system_w`), its callees are the lobby UI API (`get_lsp_data`, `draw_sprite_ary`,
 * `draw_sprite_anim_ary`, `GetMenuFontColor`, `LbStr`) and both bracketing registered units are
 * `lobby/*`.  Language C++: the two named symbols in the range are mangled and most of its callees
 * are (`GetMenuFontColor__Fbbbb`, `create_move_work__Fl`, `file_loading_ck__FPcPl`, ...).
 *
 * Flags: `cflags_lobby`, the group its two neighbours use.  Two file-scope pragmas are load-bearing and
 * measured:
 *   - `#pragma peephole off`: retail keeps the unfused `clrlwi`+`slwi` index scale and the separate
 *     `slwi`/`or` steps of the ARGB pack where the default peephole fuses them into `rlwinm`/`rlwimi`
 *     (`get_change_hair_color` 84.23 -> 100.00, `get_change_inner_color` the same).
 *   - `-Cpp_exceptions on` (`cflags_lobby`, flags-audit 2026-09-28): the target object carries
 *     extab/extabindex and the old `-Cpp_exceptions off` default emitted none.  With the pragma the `.text` is unchanged (all nine written
 *     functions keep their scores) and the unwind sections appear - one 8-byte record and one 12-byte
 *     index entry per written function, 0x38/0x54 against the target's 0x60/0x90 for the twelve the
 *     range will have.
 *
 * Shared headers this unit needed (each filed as a shared-file request in the handoff):
 *   - `unsplit/lobby.h`: the `.data`/`.sdata` tables of the range, `LbChangeColorRec`, the typed
 *     `lb_param_w` block, the `Psw` pad records, and `LbLobbyWork`'s +0x01/+0x02/+0x06/+0x14 fields.
 *   - `unsplit/unknown.h`: `system_w` +0x2D/+0x7CE/+0x8B1 split out of padding, and `unk2149` renamed
 *     to `field_0x865` (main.cpp's one use renamed with it).
 *   - two spellings in those headers disagree with the DOL and are worked around here rather than
 *     edited under another unit: `lobby_world_block` is a `.sbss` **pointer** (`lwz` in every reader, and
 *     `get_userdata` writes it), and `fn_80215C98` takes **five** arguments (all four DOL call sites
 *     pass `r7`).  Both are declared in the `lb_chg` scope below with the real shape.
 *
 * Residuals (measured with `recompile.py ... --measure <symbol>`):
 *   - `fn_801FB364` 99.89: the instructions are equal and only the frame differs - ours 0x30, the
 *     target's 0x20, with the same local offsets (0x8/0xC/0x10), so 16 bytes of the frame are an
 *     allocation difference, not a source one.
 *   - `fn_801FA0DC` 93.13: the range's shape is reproduced (both sentinel arms, the mode tail), but
 *     our allocator keeps the `lbl_805B875C` switch value in a callee-saved register across the two
 *     `get_lsp_data` calls where retail reloads it, which costs a save/restore and one register.
 *   - `fn_801FB524` 50.16: the state machine is reconstructed but 992 B against the target's 744 B -
 *     MWCC did not merge this compile's three identical `system_w.field_0x865 = 0` / `state = 3`
 *     blocks into the one copy retail has, and the dispatch came out a linear compare chain where
 *     retail has a binary search.
 *   - not reconstructed: `fn_801F9CD4` (0x408), `fn_801FA2C8` (0x690), `fn_801FA958` (0x880),
 *     `fn_801FB80C` (0x2D8), `fn_801FBB64` (0x414).  They are left unwritten rather than guessed;
 *     `fn_801FA0DC` is the sibling of `fn_801F9CD4` and already shows the shape the pair shares.
 *   - the unit's `.data` tables are declared, not claimed: the split range is `.text` + extab +
 *     extabindex only, and claiming data moves relocation handling (playbook row 23).
 *
 * Naming note: the symbol map has only fn_XXXXXXXX for this range (checked with dumpmap.py lookup
 * on all 14 names in config/RMHE08/symbols.txt: 12 are bare `fn_` entries and the dump answers only
 * `zz_XXXXXXXX_` placeholders for them)
 */
#include "types.h"

#include "lobby/lb_npc.h" /* LbResId / LbResource / LbResRec - the lobby resource types (rule 2) */
#include "unsplit/lobby.h"
#include "unsplit/unknown.h" /* SystemWork / system_w */
#include "Network/network_pat_control.h" /* the owner's header (rule 2) */
#include "fn_8004CAD8/get_qResult_work.h" /* get_qResult_work (rule 2) */
#include "quest/quest_result_work.h"        /* Q_ResultWork (rule 1) */

#pragma peephole off

/* ---------------------------------------------------------------------------------------------------
 * The two shared spellings that disagree with the DOL (see the header): `lobby_world_block` is a pointer,
 * and `fn_80215C98` takes five arguments.  A scoped `extern "C"` declaration cannot be beaten by a
 * global re-declaration - MWCC reports `illegal function overloading` (10197) - so the correct shape
 * lives in this scope, which names the same symbols.
 * ------------------------------------------------------------------------------------------------- */
namespace lb_chg {
extern "C" {
u8* lobby_world_block;
s32 fn_80215C98(s32 text, u8 flag, s16* pos, s32 color, u8 mode);
}
}  // namespace lb_chg

/* ---------------------------------------------------------------------------------------------------
 * Types this unit reconstructs.
 * ------------------------------------------------------------------------------------------------- */
/* The character-edit screen work object the two `self`-taking functions read. */
typedef struct LbChgColorWork {
    /* +0x00 */ u8 mode_0x00; /* 0 = body, 1 = hair - the tail's switch value */
    /* +0x01 */ u8 unused_0x01;
    /* +0x02 */ u8 table_0x02; /* 0 selects the body colour table, non-zero the hair one */
    /* +0x03 */ u8 unused_0x03;
    /* +0x04 */ s16 value_0x04;
    /* +0x06 */ s16 value_0x06;
    /* +0x08 */ s16 selected_0x08;
    /* +0x0A */ u8 unused_0x0A[2];
    /* +0x0C */ s16 value_0x0C;
    /* +0x0E */ s16 offset_0x0E;
    /* +0x10 */ s16 offset_0x10;
    /* +0x12 */ u8 unused_0x12[2];
    /* +0x14 */ u32 color_0x14;
    /* +0x18 */ u32 value_0x18;
    /* +0x1C */ u8 unused_0x1C[4];
    /* +0x20 */ void* cursor_0x20;
    /* +0x24 */ void* cursor_0x24;
    /* +0x28 */ void* table_0x28;
} LbChgColorWork; /* size: 0x2C */

/* The sequence record `fn_801FB524`'s caller owns: a step counter and the state the driver switches
 * on. */
typedef struct LbChgSeqWork {
    /* +0x00 */ u8 state_0x00;
    /* +0x01 */ u8 pad_0x01[9];
    /* +0x0A */ u8 step_0x0A;
    /* +0x0B */ u8 state_0x0B;
} LbChgSeqWork; /* size: 0xC */

/* One 8-byte row of the archive table `fn_801FB478` streams: the byte count and the source name. */
typedef struct LbChgFileReq {
    /* +0x0 */ u32 size_0x00;
    /* +0x4 */ const char* name_0x04;
} LbChgFileReq; /* size: 0x8 */

/* ---------------------------------------------------------------------------------------------------
 * Declarations.  `fn_*`/`ckResourceName` are plain C symbols; the lobby UI helpers come from
 * `unsplit/lobby.h`; the mangled C++ callees are declared with the signatures that reproduce their map
 * names (rule 9).  Declarations whose owner is another registered unit (`fn_801FCA80`, `fn_801FCADC`,
 * `fn_803C3F60`, ... - the `lobby/lb_npc.cpp` range) are filed as shared-file requests.
 * ------------------------------------------------------------------------------------------------- */
extern "C" {
s32 score_add_clamped(s32 delta, s32* value);
void fn_80040DE8(u8 mode);
s32 res_file_assign(void* dst, void* src);
void* res_file_ctor(void* out, u32 value);
void fn_800D5CAC(void* rec);
void fn_800E3358(u32 kind, u8 arg, void* str);
void fn_800F6520(void);
void fn_800F65B4(void);
void fn_800F6710(void);
void fn_801E92B0(void);
void fn_801F3588(const void* table);
s32 fn_801F36CC(const void* table, s16 a, s16 b);
s32 fn_801F3EDC(LbChgColorWork* self);
s32 fn_801F3F14(void* table, u8 index);
void fn_801F54C4(void);
void fn_801F9AF8(s16* pos, s32 color, s32 selected, s16 value, s32 mode);
void fn_801FB2A8(u8 a, u8 b, u8 c);
void fn_801FB364(u8 index);
void fn_801FB438(void);
void fn_801FB478(void);
void fn_801FBAE4(void);
void fn_801FC318(void);
void fn_801FC6A0(u32 unused, s32* value);
void fn_801FC810(void);
void fn_801FC874(void);
void fn_801FC8C8(void);
void fn_801FC8F0(void);
void fn_801FCA80(u32 index);
void fn_801FCADC(u8* self, u32 index);
void fn_8021D9F8(void);
void fn_802A0568(void);
void fn_802AD9CC(void);
s32 fn_802AE5C0(u32 a, u8 b, u8 c);
s32 fn_802AEEF8(u8 a, u8 b);
void fn_802AFA40(void);
s32 fn_802AFB48(u8 a, u8 b);
void fn_802B2318(u8 mode);
void fn_802B56E0(void);
void fn_802BB0EC(void);
void fn_802BECB8(char* buf, u8 index);
void sprite_frame_apply(void* dst, u16 id, u8 flag, const _mh_ivec2_* src);
void fn_8032422C(void);
void fn_8035A9E4(void);
void fn_803A75D8(void);
void fn_803C3A70(void);
void fn_803C3F60(void);
s32 fn_80449860(void);
s32 fn_804498C8(void);
s32 fn_804498CC(void);
s32 game_save_wait(void);
s32 chg_nand_err2msgcode(void);
}

/* The C++-linkage callees: their map names are manglings, and these signatures reproduce them. */
void* ckResourceName(char* name);
s32 create_move_work(long kind);
s32 file_loading_ck(char* name, s32* out);
void init_player_work(void);
void light_init(void);
void load_file(char* name, u32 dst, long size);
void player_control_move(void);
void player_init_data_load(void);
void setSoftresetFlag(bool flag);
void subTransSet(u32 a, long b, u32* value);
void village_tex_load(void);
void* work_mem_alloc(u32 size);

/**
 * Fills the caller's colour word and shape outputs from the hair-colour table entry the selection
 * index names.
 */
void get_change_hair_color(u8 index, u32* color, u16* out_id, s16* out_value_0x06, s16* out_value_0x08)
{
    const LbChangeColorRec* rec = &lbl_805B8674[lbl_805B87D8[index]];

    *out_id = rec->id_0x00;
    *color = ((u32)rec->r_0x03 << 24) | ((u32)rec->g_0x04 << 16) | ((u32)rec->b_0x05 << 8) | 0xFF;
    *out_value_0x06 = rec->value_0x06;
    *out_value_0x08 = rec->value_0x08;
}

/**
 * Fills the caller's colour word and shape outputs from the inner-colour table entry the selection
 * index names.
 */
void get_change_inner_color(u8 index, u32* color, u16* out_id, s16* out_value_0x06, s16* out_value_0x08)
{
    const LbChangeColorRec* rec = &lbl_805B8674[lbl_805B87F4[index]];

    *out_id = rec->id_0x00;
    *color = ((u32)rec->r_0x03 << 24) | ((u32)rec->g_0x04 << 16) | ((u32)rec->b_0x05 << 8) | 0xFF;
    *out_value_0x06 = rec->value_0x06;
    *out_value_0x08 = rec->value_0x08;
}

/* The range's own `fn_XXXXXXXX` definitions keep C linkage so objdiff pairs them by the map's
 * spelling (`extern "C"` - the mangled stem would pair nothing; playbook row 42). */
extern "C" {

/* 0x801FA0DC - draw the colour-slot list: one row per selectable colour with the cursor on the
 * current one, then the mode-dependent tail. */
void fn_801FA0DC(LbChgColorWork* self)
{
    _mh_ivec2_ origin;
    _mh_ivec2_ pos;
    u32 i;
    u16 count;
    s16 value;
    s32 color;
    s32 selected;
    s32 active;

    get_lsp_data(0x1C61U, &origin);
    draw_sprite_ary((const u16*)lbl_805B89D0, &origin);
    get_lsp_data(0x1C5BU, &origin);
    count = (u16)fn_801F3EDC(self);
    i = 0;
    for (; (u8)i < count; i++) {
        if (self->selected_0x08 == (s32)(u8)i) {
            selected = 1;
            active = 1;
        } else {
            selected = 0;
            active = 0;
        }
        color = GetMenuFontColor(1, active, 1, fn_801F3F14(self->table_0x28, (u8)i) == 1);
        get_lsp_data(lbl_805B8A04[(u8)i], &pos);
        pos.x += origin.x;
        pos.y += origin.y;
        value = lbl_805B875C[(u8)i];
        switch (value) {
        case -1:
            lb_chg::fn_80215C98((s32)LbStr(0, 0x231), (u8)active, (s16*)&pos, color, 8);
            break;
        case -2:
            fn_8021565C((u8)(selected | 0x80), (s16*)&pos);
            break;
        default:
            get_lsp_data(lbl_805B8A04[(u8)i], &pos);
            pos.x += origin.x;
            pos.y += origin.y;
            fn_801F9AF8((s16*)&pos, color, selected, lbl_805B875C[(u8)i], 0);
            break;
        }
    }
    {
        s32 mode = -1;

        switch (self->mode_0x00) {
        case 0:
            fn_801F54C4();
            mode = 0x120;
            break;
        case 1:
            mode = 0x124;
            fn_80215170(0x1879, (s32)self->cursor_0x24);
            break;
        }
        if (mode != -1) {
            fn_80214EF0(0x1877, (s16)mode);
        }
    }
}

/* 0x801FB2A8 - bring the editor screen up: the player/word init chain, the two lobby resource loads,
 * the colour-table setup and the mode handover. */
void fn_801FB2A8(u8 a, u8 b, u8 c)
{
    init_player_work();
    fn_803A75D8();
    fn_802AFA40();
    player_init_data_load();
    fn_800D5CAC((void*)lbl_80582988);
    fn_800D5CAC((void*)lbl_8058AA98);
    fn_801FCA80(a);
    fn_800F6520();
    fn_800F6710();
    if (isCityMode() == 0 || system_w.field_0x8b1 == 1) {
        village_tex_load();
    }
    fn_80040DE8(b);
    fn_802B2318(b);
    fn_802AE5C0(0, b, c);
}

/* 0x801FB364 - queue the model resources the editor needs: the `index + 1`-th row of the lobby
 * resource id table, the shared `mot_aicom.brres`, then the caller's own table entry. */
void fn_801FB364(u8 index)
{
    LbResRec dst;

    res_file_ctor(&dst, 0);
    {
        LbResId* ids = (LbResId*)lbl_80582B30;
        u32 name = ids[index + 1].b_0x04; /* the row's name pointer, a `u32` in the shared header */
        LbResource* res = (LbResource*)ckResourceName((char*)name);

        if (res != 0) {
            u32 tmp;

            res_file_assign(&dst, res_file_ctor(&tmp, (u32)res->payload_0x44));
            fn_800E3358(0, 0, &dst);
        }
    }
    {
        LbResource* res = (LbResource*)ckResourceName((char*)lbl_805B8CE0);

        if (res != 0) {
            u32 tmp;

            res_file_assign(&dst, res_file_ctor(&tmp, (u32)res->payload_0x44));
            fn_800E3358(3, 0, &dst);
        }
    }
    fn_801FCADC(NULL, index);
}

/* 0x801FB438 - run the nine subsystem initialisers the editor's first frame needs. */
void fn_801FB438(void)
{
    light_init();
    fn_800F65B4();
    fn_802AD9CC();
    fn_802B56E0();
    fn_8021D9F8();
    fn_802A0568();
    fn_8032422C();
    fn_8035A9E4();
    fn_803C3A70();
}

/* 0x801FB478 - load the ten `07/dcm%03d.bin` archives the editor streams, one work allocation each. */
void fn_801FB478(void)
{
    char name[0x20];
    LbChgFileReq* req;
    void** dst;
    s32 i;

    i = 3;
    req = (LbChgFileReq*)lbl_8058AFE8 + i;
    dst = (void**)lbl_806BC1D0 + i;
    for (; i <= 0xD; i++) {
        if (req != NULL && req->size_0x00 != 0 && *dst == NULL) {
            fn_802BECB8(name, (u8)i);
            *dst = work_mem_alloc(req->size_0x00);
            load_file(name, (u32)*dst, (s32)req->size_0x00);
        }
        req++;
        dst++;
    }
}

/* 0x801FBAE4 - hand the pending colour choice to the network side, then clear the option parameter
 * block back to its empty state. */
void fn_801FBAE4(void)
{
    Q_ResultWork* q = get_qResult_work();

    if (q->phase_0x1E2 == 3 && q->credit_0x3F0 > 0) {
        score_add_clamped(q->credit_0x3F0, (s32*)(lb_chg::lobby_world_block + 0x18));
    }
    lb_param_w.field_0x00 = 0;
    lb_param_w.field_0x04 = 0;
    lb_param_w.flag_0x0C[0] = 0;
    lb_param_w.value_0x10[0] = 0;
    lb_param_w.flag_0x0C[1] = 0;
    lb_param_w.value_0x10[1] = 0;
    lb_param_w.flag_0x0C[2] = 0;
    lb_param_w.value_0x10[2] = 0;
    lb_param_w.value_0x16 = 0;
    lb_param_w.value_0x18 = 0;
    lb_param_w.value_0x1A = 0;
    lb_param_w.value_0x1C = 0;
}

/* 0x801FB524 - the editor's sequence driver: one state step per frame, returning whether the sequence
 * has finished. */
s32 fn_801FB524(LbChgSeqWork* self)
{
    u16 status = Psw[0].button_0x2C0.pressed_0x04;
    s32 done = 0;
    Q_ResultWork* q;
    s32 v;

    switch (self->state_0x0B) {
    case 0:
        fn_801FBAE4();
        fn_801FC810();
        lobby_w.field_0x006 = 1;
        lobby_w.field_0x001 = 0x16;
        lobby_w.field_0x002 = 0;
        create_move_work(2);
        if (system_w.field_0x7ce == 2) {
            lobby_w.field_0x002 = 2;
        }
        fn_801FC318();
        fn_801FC874();
        fn_801FC8C8();
        fn_801E92B0();
        q = get_qResult_work();
        if (q != NULL && system_w.field_0x2d == 0 &&
            (q->kind_0x1E3 == 1 || q->kind_0x1E3 == 3)) {
            if (fn_80449860() != 0) {
                self->state_0x0B = 1;
                system_w.field_0x865 = 1;
                break;
            }
            if (fn_804498C8() != 0) {
                self->state_0x0B = 2;
                break;
            }
        }
        system_w.field_0x865 = 0;
        self->state_0x0B = 3;
        break;
    case 1:
        v = game_save_wait();
        if ((u8)(v + 2) <= 1U) {
            self->state_0x0B = 0x64;
            break;
        }
        if (v != 1) {
            break;
        }
        if (fn_804498C8() != 0) {
            self->state_0x0B = 2;
            break;
        }
        system_w.field_0x865 = 0;
        self->state_0x0B = 3;
        break;
    case 2:
        v = fn_804498CC();
        if ((u8)(v + 2) <= 1U) {
            self->state_0x0B = 0x64;
            break;
        }
        if (v != 1) {
            break;
        }
        if (fn_804498C8() != 0) {
            self->state_0x0B = 2;
            break;
        }
        system_w.field_0x865 = 0;
        self->state_0x0B = 3;
        break;
    case 3:
        fn_801FB2A8(0, lobby_w.field_0x001, lobby_w.field_0x002);
        self->state_0x0B++;
        setSoftresetFlag(0);
        break;
    case 4:
        if (file_loading_ck(NULL, NULL) != 1) {
            fn_801FB364(0);
            lobby_w.slots_0x00C[lobby_w.field_0x014] = (u32)(s8)fn_802AFB48(lobby_w.field_0x001, lobby_w.field_0x002);
            self->state_0x0B++;
        }
        break;
    case 6:
        fn_802AEEF8(lobby_w.field_0x001, lobby_w.field_0x002);
        fn_803C3F60();
        fn_801FC8F0();
        fn_802BB0EC();
        self->step_0x0A++;
        self->state_0x0B = 0;
        setSoftresetFlag(1);
        return 0;
    case 0x64:
        if ((status & 0x10) != 0) {
            system_w.field_0x865 = 0;
            self->state_0x0B = 3;
        } else {
            s32 code = chg_nand_err2msgcode();

            done = 1;
            subTransSet((u32)fn_801FC6A0, 1, (u32*)&code);
        }
        break;
    }
    if (self->state_0x0B >= 3) {
        player_control_move();
    }
    return done;
}

} /* extern "C" */
