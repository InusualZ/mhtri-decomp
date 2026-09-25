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
    /* +0x001 */ u8 unused_0x001[0x7C];
    /* +0x07D */ u8 slots_0x07D[0x2F];
    /* +0x0AC */ LbMenuWork* menu_0xAC;
    /* +0x0B0 */ u8 unused_0x0B0[0x7D];
    /* +0x12D */ u8 param_0x12D;
    /* +0x12E */ u8 unused_0x12E[0x4E];
} LbLobbyWork; /* size: 0x17C */

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

/* One 0xC-byte lobby part slot at `lbl_80794880 + 0x3F38 + i * 0xC`. */
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

/* The 0x1C-byte parameter block `fn_80359568` reads (the `fn_801E79E4` menu action). */
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

/* The big lobby data block at `lbl_80794880`; this unit reads two byte fields and one string run. */
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

extern u8 Psw[3392];
extern u8 jumptable_805B7CD8[36];
extern u8 lb_item_get_data[];
extern u8 lb_param_w[156];
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
extern u8 lbl_80794880[];
extern LbLobbyWork lobby_w;

/* The unsplit plain-C callees. */
void* fn_801E6F10(u16 id);
void* fn_801E6F48(u16 id);
s8* fn_802DFACC(u8 id);
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
s32 fn_802A7C04(s32, u16 *);
s16 fn_802A8EC0(s16, s16, u16, s32, s32, void *);
s16 fn_802A8ED8(s16, s16, u16, s32, s32, s32);
s16 fn_802A8EFC(s16, s16, u16, s32, s32);
s32 fn_802A8F50(void *, u16, s32, s32, s32);
s32 fn_802BBA64(s32);
s32 fn_802BBAC0(void);
s32 fn_802BBAC4(s32);
s32 fn_802DE224(void);
s32 fn_802DF6E4(s32);
s32 fn_802E0DA8(s16 *, u16, s16 *);
s32 fn_802FB4BC(u16);
s32 fn_802FB4F4(s32);
u16 fn_802FB54C(s32);
s16 fn_8033B6C0(s32, s32);
s32 fn_8033B990(void);
s32 fn_8033C1AC(void);
s32 fn_80359140(u16, s32);
s32 fn_80359530(u16 *, s32 *);
s32 fn_80359568(s32 *, s32);
s32 fn_80359628(void);
s32 fn_80359B00(s32);
s32 fn_80359D98(void *, void *);
s32 fn_8035A034(void);
s32 fn_8035A7D8(s32, void*, void*, s32, s32);
u32 fn_803768F8(void);
s32 fn_80377664(void *);

#ifdef __cplusplus
}
#endif

/* C++-linkage callees whose map names are manglings; the front-end reproduces those names from these
 * declarations, and rule 9 forbids spelling the manglings at the call site. */
#ifdef __cplusplus

s32 GetMenuFontColor(bool, bool, bool, bool);
void* ItemName(u16);
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
u8 get_option_cfg(u8);
void put_menu_cursor(u16*, u16, const _mh_ivec2_*);

#endif /* __cplusplus */

#endif /* MHTRI_UNSPLIT_LOBBY_H */

