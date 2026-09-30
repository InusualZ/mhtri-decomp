/* Not-yet-reconstructed `lobby`-band symbols (the bracketing registered units both name `lobby`).
 *
 * The declarations the lobby menu-layer unit (`src/lobby/fn_801E7530.cpp`) needs live here, not in its
 * own source: docs/plan.md 6.5 rule 2 says an extern belongs with the unit that owns the symbol, or in
 * `include/unsplit/<module>.h` when no unit owns it.  Every address below is unclaimed in splits.txt and
 * its bracketing registered bands name `lobby`.
 *
 * The `fn_XXXXXXXX` names and `lbl_*` objects keep the map's own spelling.  The C-linkage functions are
 * inside `extern "C"` so the C++ consumer links them by their map names; the C++-linkage callees
 * (`LbStr`, `draw_sprite_*`, `get_lsp_data`, ...) are declared outside it, with the real signatures that
 * make the front-end emit the map's mangling (rule 9).
 */
#ifndef MHTRI_UNSPLIT_LOBBY_H
#define MHTRI_UNSPLIT_LOBBY_H

#include "types.h"
#include "lobby/lb_npc.h"

/* Owner headers (rule 2).  The menu range's list/cursor entry points and the camera range's
 * `fn_802BBA64` group were declared in this band header while their addresses were unclaimed; both
 * ranges are registered now (`menu/menu_message.cpp`, `camera/fn_802B5C58.cpp`), so their declarations
 * live in these headers and this one re-exports them for the units that already include it.  The
 * `ef/eft052.cpp` entry points this header used to declare (`eft052_page_count_add`,
 * `eft052_hold_row_get`, `eft052_hold_entry_set`) made the same move into `include/ef/eft052.h`. */
#include "camera/camera.h"
#include "ef/eft052.h"
#include "menu/menu_message.h"

/* The 2D integer vector the lobby/HUD helpers exchange (`_mh_ivec2_` in the map's mangling).
 * size: 0x4 */
typedef struct _mh_ivec2_ {
    /* +0x00 */ s16 x;
    /* +0x02 */ s16 y;
} _mh_ivec2_;

/* The sprite-data record the `draw_sprite` family takes by reference; only ever passed through, so the
 * incomplete type is enough here. */
struct _SPR_DATA_;

/* One 6-byte row of the lobby selection tables (`lbl_805BA660`, `lbl_805BA6D4`, `lbl_805BA718`): a
 * u16 id, a u16 payload and a u16 gate flag. */
typedef struct LbTableRow {
    /* +0x00 */ u16 id_0x00;
    /* +0x02 */ u16 value_0x02;
    /* +0x04 */ u16 flag_0x04;
} LbTableRow; /* size: 0x6 */

/* One entry of the menu screen's 6-row selection block at `LbMenuWork::entries_0xD0`. */
typedef struct LbMenuEntry {
    /* +0x00 */ u8 kind_0x00;
    /* +0x01 */ u8 icon_kind_0x01;
    /* +0x02 */ u16 id_0x02;
    /* +0x04 */ s32 value_0x04;
} LbMenuEntry; /* size: 0x8 */

/* The lobby menu screen work object, reached through `lobby_w.menu_0xAC`. */
typedef struct LbMenuWork {
    /* +0x00 */ u8 state_0x00;
    /* +0x01 */ u8 mode_0x01;
    /* +0x02 */ u8 unused_0x02[2];
    /* +0x04 */ u8 active_0x04;
    /* +0x05 */ u8 open_0x05;
    /* +0x06 */ u8 timer_0x06;
    /* +0x07 */ u8 unused_0x07;
    /* +0x08 */ s16 selected_0x08;
    /* +0x0A */ u8 unused_0x0A[2];
    /* +0x0C */ s16 list_mode_0x0C;
    /* +0x0E */ s16 stage_0x0E;
    /* +0x10 */ s16 count_0x10;
    /* +0x12 */ u16 item_id_0x12;
    /* +0x14 */ u8 unused_0x14[8];
    /* +0x1C */ s32 data_0x1C;
    /* +0x20 */ s32 data_0x20;
    /* +0x24 */ u8 str_0x24[0x14];
    /* +0x38 */ s32 value_0x38;
    /* +0x3C */ s32 value_0x3C;
    /* +0x40 */ u8 unused_0x40[4];
    /* +0x44 */ u16 slots_0x44[4];
    /* +0x4C */ s16 bound_0x4C;
    /* +0x4E */ s16 bound_0x4E;
    /* +0x50 */ u16 flags_0x50[0x40];
    /* +0xD0 */ LbMenuEntry entries_0xD0[6];
    /* +0x100 */ u16 value_0x100;
} LbMenuWork; /* size: 0x104 */

/* The `.bss` lobby work block (`lobby_w`, 0x17C bytes); this unit only reads the menu pointer. */
typedef struct LbLobbyWork {
    /* +0x000 */ u8 state_0x000;
    /* +0x001 */ u8 field_0x001;
    /* +0x002 */ union {
        u8 field_0x002;   /* the menu-layer state `src/lobby/fn_801F9CD4.cpp` sets (0/2) and passes to
                           * `fn_801FB2A8`/`fn_802AEEF8` */
        u8 area_0x002;    /* the scene's area id, compared against `_PLW::area_0x16`
                           * (`src/lobby/fn_802076D4.cpp`) - the same byte, two consumers */
    };
    /* +0x003 */ u8 unused_0x003[3];
    /* +0x006 */ u8 field_0x006;
    /* +0x007 */ u8 unused_0x007[0x05];
    /* +0x00C */ u32 slots_0x00C[2];
    /* +0x014 */ u8 field_0x014;
    /* Merge of two views of the same 0x68 bytes: the option/menu layer names nothing in there, the
     * NPC band (`lobby/fn_802029B4.cpp`) names its two state bytes.  Both keep their offsets. */
    union {
        /* +0x015 */ u8 unused_0x015[0x68];
        struct {
            /* +0x015 */ u8 unused_0x015b[0x61];
            /* +0x076 */ u8 field_0x076;
            /* +0x077 */ u8 field_0x077;
            /* +0x078 */ u8 unused_0x078[0x5];
        };
    };
    /* +0x07D */ u8 slots_0x07D[0x2F];
    /* +0x0AC */ LbMenuWork* menu_0xAC;
    /* +0x0B0 */ u8 field_0x0B0;
    /* +0x0B1 */ u8 unused_0x0B1[0x7B];
    /* +0x12C */ u8 field_0x12C;   /* 1 puts the lobby act layer on hold */
    /* +0x12D */ u8 param_0x12D;
    /* +0x12E */ u8 unused_0x12E[0x1];
    /* +0x12F */ u8 param_0x12F;
    /* +0x130 */ u8 unused_0x130[0x2C];
    /* +0x15C */ u8 field_0x15C;
    /* +0x15D */ u8 field_0x15D;
    /* +0x15E */ u8 field_0x15E;
    /* +0x15F */ u8 field_0x15F;
    /* +0x160 */ u8 field_0x160;
    /* +0x161 */ u8 field_0x161;
    /* +0x162 */ u8 unused_0x162[0x14];
    /* +0x176 */ u8 field_0x176;
    /* +0x177 */ u8 unused_0x177[0x5];
} LbLobbyWork; /* size: 0x17C */

/* The lobby's NPC move table block (`lb_npc_move_data`, .bss): +0x0C is the `LbNpcMotionEntry` array
 * `_LB_NPC::field_0x204` is pointed at.  Only the pointer is read here. size: 0x10 */
typedef struct LbNpcMoveData {
    /* +0x00 */ u8 unused_0x00[0x0C];
    /* +0x0C */ LbNpcMotionEntry* table_0x0C;
    /* +0x10 */ LbNpcMotionEntry* table_0x10;
    /* +0x14 */ LbNpcMotionEntry* table_0x14;
    /* +0x18 */ LbNpcMotionEntry** list_0x18;
    /* +0x1C */ LbNpcMotionEntry** list_0x1C;
    /* +0x20 */ LbNpcMotionEntry** list_0x20;
    /* +0x24 */ LbNpcMotionEntry** list_0x24;
    /* +0x28 */ LbNpcMotionEntry** list_0x28;
    /* +0x2C */ LbNpcMotionEntry** list_0x2C;
} LbNpcMoveData; /* size: 0x30+ */

/* One entry of the map's NPC spot tables (`lbl_805B8EF0`, `lbl_805B8F28`, ...): an x and a z the
 * `_LB_NPC`'s position is tested against; each table ends with the 10000.0f / 0.0f terminator the
 * scan's limit names. size: 0x8 */
typedef struct LbNpcMoveSpot {
    /* +0x0 */ f32 x;
    /* +0x4 */ f32 z;
} LbNpcMoveSpot;

/* One 0xB20-byte record of the enemy move work `get_move_work_adrs(2)` returns; the NPC band copies
 * its vector at +0x3C into an NPC's target. size: 0xB20 */
typedef struct LbNpcMoveWorkEntry {
    /* +0x00 */ u8 pad_0x00[0x3C];
    /* +0x3C */ VEC3 vec_0x3C;
    /* +0x48 */ u8 pad_0x48[0xAD8];
} LbNpcMoveWorkEntry;

/* The per-item rate record `fn_801E6F10`/`fn_801E6F48` return: a pair of 16-bit rates. */
typedef struct LbItemData {
    /* +0x00 */ u8 unused_0x00[2];
    /* +0x02 */ u16 rate_0x02;
    /* +0x04 */ u16 rate_0x04;
} LbItemData; /* size: 0x6 */

/* The 8-byte string block `fn_801E88C4` copies (four u16 code units). */
typedef struct LbStrBlock {
    /* +0x00 */ u16 code_0x00;
    /* +0x02 */ u16 code_0x02;
    /* +0x04 */ u16 code_0x04;
    /* +0x06 */ u16 code_0x06;
} LbStrBlock; /* size: 0x8 */

/* One 0xC-byte lobby part slot at `lobby_world_block + 0x3F38 + i * 0xC`. */
typedef struct LbPartSlot {
    /* +0x00 */ u8 kind_0x00;
    /* +0x01 */ u8 sub_0x01;
    /* +0x02 */ u8 unused_0x02[0xA];
} LbPartSlot; /* size: 0xC */

/* One 0x10-byte part record copied by `fn_801E9318` (`lbl_805B7F28 + i * 0x10`). */
typedef struct LbPartRec {
    /* +0x00 */ u8 kind_0x00;
    /* +0x01 */ u8 flag_0x01;
    /* +0x02 */ u8 sub_0x02;
    /* +0x03 */ u8 mode_0x03;
    /* +0x04 */ u8 level_0x04;
    /* +0x05 */ u8 count_0x05;
    /* +0x06 */ u16 id_0x06;
    /* +0x08 */ u16 value_0x08;
    /* +0x0A */ u16 rate_0x0A;
    /* +0x0C */ s32 data_0x0C;
} LbPartRec; /* size: 0x10 */

/* A 0xC-byte byte/word copy block (`fn_801EAC30` copies one onto another). */
typedef struct LbParam {
    /* +0x00 */ u8 kind_0x00;
    /* +0x01 */ u8 type_0x01;
    /* +0x02 */ u8 sub_0x02;
    /* +0x03 */ u8 flag_0x03;
    /* +0x04 */ u8 level_0x04;
    /* +0x05 */ u8 count_0x05;
    /* +0x06 */ u16 id_0x06;
    /* +0x08 */ u16 value_0x08;
    /* +0x0A */ u8 rate_0x0A;
    /* +0x0B */ u8 mode_0x0B;
} LbParam; /* size: 0xC */

/* The 0x1C-byte parameter block `eft052_hold_entry_set` reads (the `fn_801E79E4` menu action). */
typedef struct LbSeParam {
    /* +0x00 */ s32 value_0x00;
    /* +0x04 */ s16 mode_0x04;
    /* +0x06 */ s16 id_0x06;
    /* +0x08 */ s16 limit_0x08;
    /* +0x0A */ s16 max_0x0A;
    /* +0x0C */ s16 cur_0x0C;
    /* +0x0E */ s16 unused_0x0E;
    /* +0x10 */ s16 time_0x10;
    /* +0x12 */ s16 count_0x12;
    /* +0x14 */ void* data_0x14;
    /* +0x18 */ s8 flag_0x18;
} LbSeParam; /* size: 0x1C */

/* The big lobby data block at `lobby_world_block`; this unit reads two byte fields and one string run. */
typedef struct LbBigBlock {
    /* +0x0000 */ u8 unused_0x0000[0x4068];
    /* +0x4068 */ u8 name_0x4068[4];
    /* +0x406C */ u8 index_0x406C;
    /* +0x406D */ u8 unused_0x406D[0x1FA3];
    /* +0x6010 */ u8 tail_0x6010[];
} LbBigBlock; /* size: 0x6010+ */

#ifdef __cplusplus
extern "C" {
#endif

/* The option parameter block (`lb_param_w`, 0x806590B4, 0x9C bytes): three per-kind flags with the
 * matching value words, then four trailing shape words.  Only the fields this unit clears are named. */
typedef struct LbParamWork {
    /* +0x00 */ u16 field_0x00;
    /* +0x02 */ u8 pad_0x02[2];
    /* +0x04 */ u32 field_0x04;
    /* +0x08 */ u8 pad_0x08[3];
    /* +0x0B */ u8 flag_0x0b;      /* == 1 with +0x00 set is `quest_select_ready_ck` */
    /* +0x0C */ u8 flag_0x0C[3];
    /* +0x0F */ u8 pad_0x0F;
    /* +0x10 */ u16 value_0x10[3];
    /* +0x16 */ u16 value_0x16;
    /* +0x18 */ u16 value_0x18;
    /* +0x1A */ u16 value_0x1A;
    /* +0x1C */ u16 value_0x1C;
    /* +0x1E */ u8 pad_0x1E[0x7E];
} LbParamWork; /* size: 0x9C */

/* The per-player pad record block `Psw` (0x80659350, 0x350-byte stride - four players); only the
 * status word this unit tests is named. */
typedef struct LbPswBlock {
    /* +0x000 */ u8 pad_0x000[0x2C4];
    /* +0x2C4 */ u16 status_0x2C4;
    /* +0x2C6 */ u8 pad_0x2C6[0xE];
    /* +0x2D4 */ u16 status2_0x2D4;
    /* +0x2D6 */ u8 pad_0x2D6[0x7A];
} LbPswBlock; /* size: 0x350 */

extern LbPswBlock Psw[4];
extern u8 jumptable_805B7CD8[36];
extern u8 lb_item_get_data[];
extern LbParamWork lb_param_w;
extern u8 lbex_main_str[];
extern u8 lbex_zaco_str[];
extern u8 lbl_805B7A88[64];
extern u8 lbl_805B7CB0[40];
extern const u16 lbl_805B7CFC[8];
extern u16 lbl_805B7D0C[6];
extern const u16 lbl_805B7D18[16];
extern const u16 lbl_805B7D38[10];
extern const u16 lbl_805B7D4C[8];
extern const u16 lbl_805B7D5C[10];
extern const u16 lbl_805B7D70[24];
extern const u16 lbl_805B7DA0[18];
extern const u16 lbl_805B7DC4[30];
extern u8 lbl_805B7E00[12];
extern const u16 lbl_805B7E0C[6];
extern u8 lbl_805B7E18[24];
extern const u16 lbl_805B7E30[16];
extern const u16 lbl_805B7E50[6];
extern const u16 lbl_805B7E5C[6];
extern const u16 lbl_805B7E68[12];
extern const u16 lbl_805B7E80[6];
extern const u16 lbl_805B7E8C[30];
extern u8 lbl_805B7EC8[24];
extern const u16 lbl_805B7EE0[10];
extern u16 lbl_805B7EF4[6];
extern const u16 lbl_805B7F00[20];
extern u8 lbl_805B7F28[208];
extern u8 lbl_805B82C8[16];
extern u8 lbl_805B82D8[48];
extern u8 lbl_805B8308[48];
extern u8 lbl_805B8338[16];
extern u8 lbl_805B8348[12];
extern u8 lbl_805B8354[12];
extern u8 lbl_805B8360[36];
extern u16 lbl_805B8384[10];
extern u8 lbl_805B8398[20];
extern u8 lbl_805B83AC[72];
extern u8 lbl_805B83F4[28];
extern u8 lbl_805B8470[48];
extern u8 lbl_805B84A0[16];
extern u8 lbl_805B84B0[24];
extern u8 lbl_805B84C8[32];
extern u8 lbl_805B84E8[12];
extern u8 lbl_805B84F4[12];
extern u8 lbl_805B8500[12];
extern u8 lbl_805B850C[16];
extern u8 lbl_805B851C[44];
extern u8 lbl_805B8548[12];
extern u8 lbl_805B8554[12];
extern u8 lbl_805B8560[20];
extern u8 lbl_805B8574[12];
extern u8 lbl_805B8580[56];
extern u8 lbl_805B85B8[32];

/* The `.data`/`.sdata` tables the character-edit (hair/inner colour) screen unit
 * (`src/lobby/fn_801F9CD4.cpp`) addresses; no registered unit owns them yet, so they are declared
 * here rather than in that unit's source (rule 2). */
extern u32 lbl_80582988[44];
extern u8 lbl_80582B30[40];
extern u32 lbl_8058AA98[4];
extern u32 lbl_8058AFE8[28];
/* The 0xA-byte colour record the character-edit screen's `lbl_805B8674` table holds, and the two
 * 14-entry selection index tables beside it (`-2`/`-3` are the "no colour" sentinels the callers test).
 * Declared here because no registered unit owns the `.data` range (rule 2). */
typedef struct LbChangeColorRec {
    /* +0x0 */ u16 id_0x00;
    /* +0x2 */ u8 unused_0x02;
    /* +0x3 */ u8 r_0x03;
    /* +0x4 */ u8 g_0x04;
    /* +0x5 */ u8 b_0x05;
    /* +0x6 */ s16 value_0x06;
    /* +0x8 */ s16 value_0x08;
} LbChangeColorRec; /* size: 0xA */

extern LbChangeColorRec lbl_805B8674[23];
extern const s16 lbl_805B875C[6];
extern const u16 lbl_805B8768[8];
extern const u16 lbl_805B8778[16];
extern const u16 lbl_805B8798[8];
extern const u16 lbl_805B87A8[16];
extern const u16 lbl_805B87C8[8];
extern const s16 lbl_805B87D8[14];
extern const s16 lbl_805B87F4[14];
extern const u16 lbl_805B89D0[20];
extern const u16 lbl_805B8A04[6];
extern const u16 lbl_805B8A10[18];
extern const u16 lbl_805B8A34[10];
extern const u16 lbl_805B8A48[20];
extern const u16 lbl_805B8A70[8];
extern const u16 lbl_805B8A80[14];
extern const s16 lbl_805B8A9C[8];
extern const u16 lbl_805B8AAC[12];
extern const u16 lbl_805B8AC4[6];
extern const u16 lbl_805B8AD0[20];
extern const char lbl_805B8CE0[16];
extern u8 lbl_806BC1D0[200];
extern const u16 lbl_80791CA4[4];
extern const u16 lbl_80791D18[4];
extern const u16 lbl_80791D20[4];
extern const f64 lbl_80799878;
extern u8 jumptable_805B8C74[104];
extern u8 jumptable_805B8CF0[84];

extern u16 lbl_805BA660[58];
extern u16 lbl_805BA6D4[34];
extern u16 lbl_805BA718[28];
extern const u16 lbl_805BAA20[10];
extern u8 lbl_806BF310[88];
extern u16 lbl_80791B70[4];
extern u16 lbl_80791B78[4];
extern u16 lbl_80791B80[4];
extern u8 lbl_80791B88[3];
extern u8 lbl_80791B90[8];
extern u16 lbl_80791B98[4];
extern u16 lbl_80791BA0[4];
extern u16 lbl_80791BA8[1];
extern u8 lbl_80791BAC[12];
extern u16 lbl_80791BB8[4];
extern u8 lbl_80791BC0[5];
extern u8 lbl_80791BD0[8];
extern u8 lbl_80791BD8[8];
extern u8 lbl_80791BE0[8];
extern u8 lbl_80791BE8[8];
extern u8 lbl_80791BF0[8];
extern u8 lbl_80791BF8[8];
extern u8 lbl_80791C00[8];
extern u8 lbl_80791C08[8];
extern u8 lbl_80791C10[8];
extern u8 lbl_80791C18[8];
extern u8 lbl_80791C20[8];
extern u8 lbl_80791C28[8];
extern u8 lbl_80791C30[3];
extern u8 lbl_80791C38[8];
extern u8 lbl_80791C40[2];
extern u8 lbl_80791C44[5];
extern u8 lbl_80791C50[8];
extern u8 lobby_world_block[];
/* The four 6-entry sprite/index tables `fn_803A4F7C`/`fn_803A5070` search, and the flat u16 run
 * their search continues into (both terminated by a 0 entry).  The addresses are in the unclaimed
 * `.data` run 0x805F2038..0x805F2A38 / `.sdata` run 0x80793530.., which no registered unit owns
 * (rule 2: the band header carries them). */
extern const u16* note_slot_table[4];
extern const u16 note_slot_flat_table[];
/* The `.data` pair/lookup tables the lobby list band reads: `lbl_805F0EC8[kind]` is a pointer to a
 * table of 6-byte-stride records whose first u16 is the key, terminated by an entry whose u16 is
 * 0xFFFF (`lb_ui_pair_lookup`, `lb_ui_pair_offset`).  The `.data` range 0x805F0EC8 is unclaimed, so
 * the band header is its home (docs/plan.md 6.5 rule 2). */
extern u16* lbl_805F0EC8[];
/* The 12-entry signed table the option-parameter copy indexes by a byte flag (`lb_ui_param_apply`).
 * `.data` 0x805F0CDC, unclaimed, so the band header is its home. */
extern const s16 lbl_805F0CDC[12];

/* The two handlers of the band above this unit (`0x80394144`, `0x80394154`) that its per-detail step
 * tails into.  Both addresses are still unclaimed and their band's bracketing registered units name
 * `lobby` on the left, so the band header declares them (rule 2's unsplit case). */
void fn_80394144(void* self);
void fn_80394154(void* self);
extern LbLobbyWork lobby_w;

/* The lobby NPC band's own `.sdata2` pool (`src/lobby/fn_802029B4.cpp`'s state machines hand these to
 * `fn_801FE1CC` as its motion speed/base pair).  The run is unclaimed, so the band header is their
 * home; the names are the map's own. */
extern f32 lbl_8079999C;
extern f32 lbl_807999A0;
extern f32 lbl_807999B0;
extern f32 lbl_807999C4;
extern f32 lbl_807999C8;
extern f32 lbl_807999CC;
extern f32 lbl_807999D0;
extern f32 lbl_807999D4;
extern f32 lbl_807999D8;
extern f32 lbl_807999DC;
extern f32 lbl_807999E4;
extern f32 lbl_807999E8;
extern f32 lbl_807999EC;
extern f32 lbl_807999F0;
extern f32 lbl_807999F4;
extern f32 lbl_807999E0;
extern f32 lbl_807999F8;
extern f32 lbl_807999FC;
extern f32 lbl_80799A00;
extern f32 lbl_80799A14;
extern f32 lbl_80799A18;
extern f32 lbl_80799A10;
extern f32 lbl_80799A1C;
extern f32 lbl_80799A20;
extern f32 lbl_80799A24;
extern f32 lbl_80799A28;
extern f32 lbl_80799A2C;
extern f32 lbl_80799A30;
extern f32 lbl_80799A34;
extern f32 lbl_80799A38;
extern f32 lbl_80799A3C;
extern f32 lbl_80799A40;
extern f32 lbl_80799A74;
extern f32 lbl_80799A78;
extern f32 lbl_80799A7C;
extern f32 lbl_80799A80;
extern f32 lbl_80799A84;
extern f32 lbl_80799A5C;
extern f32 lbl_80799A88;
extern f32 lbl_80799A8C;
extern f32 lbl_80799AA4;
extern f32 lbl_80799AA8;
extern f32 lbl_80799AAC;
extern f32 lbl_80799AB0;
extern f32 lbl_80799AB4;
extern f32 lbl_80799AB8;
extern f32 lbl_80799ABC;
extern f32 lbl_80799AC0;
extern f32 lbl_80799AC4;
extern f32 lbl_80799AC8;
extern f32 lbl_80799ACC;
extern f32 lbl_80799AD0;
extern f32 lbl_80799AD4;
extern f32 lbl_80799AD8;
extern f32 lbl_80799ADC;
extern LbNpcMoveData lb_npc_move_data;
extern LbNpcMotionEntry lbl_805B8EC8;
extern LbNpcMoveSpot lbl_805B8EF0[7];
extern LbNpcMoveSpot lbl_805B8F28[2];
extern LbNpcMoveSpot lbl_805B8F38[3];
extern LbNpcMoveSpot lbl_805B8F50[2];
extern LbNpcMoveSpot lbl_805B8F60[2];
extern LbNpcMoveSpot lbl_805B8F70[3];

/* The unsplit plain-C callees. */
void* fn_801E6F10(u16 id);
void* fn_801E6F48(u16 id);
s8* str_tbl_33_get(u8 id);
s32 fn_801E6850(s16 *, void *);
s32 fn_801E6DCC(void);
u16 fn_801E6EA8(u16, s32);
s32 fn_801E6F80(void *);
s32 fn_801E7178(void *);
u16 fn_801E72F4(u16);
s32 fn_801E73A4(void *);
s16 fn_801E74B0(void *, s16);
u16 fn_802089F4(s32);
s32 fn_8021213C(s32, s16);
s32 fn_802121F4(s32);
u16 fn_802122AC(s32);
u16 fn_802122E8(void);
s32 fn_80212540(void *);
u8 fn_802126E8(s32, s16, s32);
s32 fn_80214654(void *, s32, s32, s32, s32, s32, s32, s32);
s32 fn_802146F0(void *, void*, u16, s32);
s32 fn_80214798(void *);
s32 fn_80214948(u8 *, const void*, const void*, void*, s32);
s32 fn_80214EF0(s32, s16);
s32 fn_80214FB8(s32, s32, u16);
s32 fn_8021505C(s32, s16, s32);
s32 fn_802150DC(s32, s16, s32, s32);
s32 fn_80215170(s32, s32);
s32 fn_8021565C(u8, s16 *);
s32 fn_8021591C(s32, s16 *);
s32 fn_802159F0(s32, s16, s16, u16, u32);
s32 fn_80215C04(s32);
s32 fn_80215C98(s32, s16 *, s32, s32);
s32 fn_80216560(u16, u16, s16 *, s32, s32, s32, s32);
s32 fn_80216678(u16, s32, s16 *, s32, s32, s32, s32);
s32 fn_80217934(void);
s32 fn_8021CBB0(s32);
s32 fn_8021D5BC(void);
s32 fn_8021F238(void);
s32 fn_80222848(s32, s16 *);
s32 fn_80222BC4(void*, s32, u8);
/* `menu_hold_row_draw_by_lsp`/`menu_cursor_step_fixed_tail`/`menu_cursor_step_open_last`/`menu_cursor_step`/`toggle_word_step` (0x802A7C04-0x802A8F50) were
 * declared here while the menu band had no registered unit.  `menu/menu_message.cpp` owns that range
 * now, so its header `include/menu/menu_message.h` declares them and this header includes it (rule 2).
 * They stood here with `s16` returns and `void*`/`s32` tails while the owner defines `s32` - that
 * mismatch is the `(10505) illegal overloading` this move clears. */
s32 fn_802DE224(void);
/* `fn_802BBA64`/`fn_802BBAC0`/`fn_802BBAC4` (0x802BBA64-0x802BBAF4) were declared here as `s32 (s32)`;
 * `camera/fn_802B5C58.cpp` owns that range and `include/camera/camera.h` - included by this header -
 * declares them `void (u8)` (rule 2). */
s32 fn_802DF6E4(s32);
s32 fn_802E0DA8(s16 *, u16, s16 *);
s32 fn_802FB4BC(u16);
s32 fn_802FB4F4(s32);
u16 fn_802FB54C(s32);
s16 fn_8033B6C0(s32, s32);
s32 fn_8033B990(void);
s32 fn_8033C1AC(void);
s32 fn_80359628(void);
s32 fn_80359B00(s32);
s32 fn_80359D98(void *, void *);
s32 fn_8035A034(void);
s32 fn_8035A7D8(s32, void*, void*, s32, s32);
/* `fn_803768F8` (0x803768F8) and `fn_80377664` (0x80377664) are owned by
 * `enemy/em020_ai.cpp` now that its range is registered - rule 2: their declarations moved to the
owner's header and are included here. */
#include "enemy/em020_ai.h"

/* The lobby band's unclaimed `.bss` / `.sdata2` objects this unit and its neighbours read.  An
 * `extern` for an unsplit symbol belongs in this band header (docs/plan.md 6.5 rule 2). */
extern u8 lbl_806AA6F0[0x20];   /* .bss 0x806AA6F0 - the lobby sub-scene latch block */
extern f32 lbl_80799B18;
extern f32 lbl_80799B1C;
extern f32 lbl_80799B20;

#ifdef __cplusplus
}
#endif

/* C++-linkage callees whose map names are manglings; the front-end reproduces those names from these
 * declarations, and rule 9 forbids spelling the manglings at the call site. */
#ifdef __cplusplus

/* `GetMenuFontColor` (0x802AA3EC, the map's `GetMenuFontColor__Fbbbb`) was declared here while the
 * menu band had no registered unit; `menu/menu_message.cpp` owns the address now and the owner's header
 * `include/menu/menu_message.h`, included above, declares it with this same spelling (rule 2). */
/* `ItemName` (0x8029F628) and `put_menu_cursor` (0x802A2564) were declared here while the band between
 * `Pl/fn_80295EF4.cpp` and `stage/stg_w.cpp` had no registered unit.  `menu/menu_item.cpp` now owns
 * both addresses, so the declarations live in its header `include/menu/menu_item.h` (rule 2) - the
 * `ItemName` one had a different return type here, which is the `(10505) illegal overloading` class. */
void* LbStr(u8, u16);
s32 chk_pointer(void);
void draw_font(const _SPR_DATA_&, s8*, u32, const _mh_ivec2_*);
void draw_font_idx(u16, s8*, u32, const _mh_ivec2_*);
void draw_itemicon_item_id(const _SPR_DATA_&, u16, const _mh_ivec2_*);
void draw_monstericon_idx(u16, u8, const _mh_ivec2_*);
void draw_sprite(const _SPR_DATA_&, const _mh_ivec2_*);
void draw_sprite_anim_ary(const u16*, u16, const _mh_ivec2_*);
void draw_sprite_anim_idx(u16, u16, const _mh_ivec2_*);
void draw_sprite_ary(const u16*, const _mh_ivec2_*);
void draw_sprite_idx(u16, const _mh_ivec2_*);
void font_set_size(s16, s16);
void* get_lsp_data(u16, _mh_ivec2_*);
/* `get_option_cfg` (0x803BEC70) is no longer declared here: the range that defines it,
 * `src/menu/get_pop_dat_ptr.cpp`, owns it and publishes it in `include/menu/get_pop_dat_ptr.h`,
 * which this header includes below (rule 2). */
#include "menu/get_pop_dat_ptr.h"

#endif /* __cplusplus */

#endif /* MHTRI_UNSPLIT_LOBBY_H */

