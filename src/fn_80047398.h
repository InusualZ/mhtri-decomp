/*
 * Declarations for the symbols `src/fn_80047398.cpp` owns (docs/plan.md 6.5, rule 2).  `drawSpr2TF`
 * and `subTransSet` are C++ manglings (`drawSpr2TF__FUcP9fltSpr2TFUc`, `subTransSet__FUllPUl`), so
 * they are declared at global scope with the owner's parameter types - never inside `extern "C"`,
 * which would ask the linker for an unmangled name.
 */
#ifndef MHTRI_FN_80047398_H
#define MHTRI_FN_80047398_H

#include "types.h"
#include "id_value.h"
#include "fn_80047398/userdata_gunner_ck.h"   /* userdata_gunner_ck (leaf header) */

/* The 2D sprite record `drawSpr2TF` consumes. size: 0x18 */
typedef struct fltSpr2TF {
    /* +0x00 */ s16 x;
    /* +0x02 */ s16 y;
    /* +0x04 */ s16 w;
    /* +0x06 */ s16 h;
    /* +0x08 */ s16 angle; /* degrees; drawSpr2TF scales it to radians */
    /* +0x0A */ s16 pad_0x0A;
    /* +0x0C */ u32 color;
    /* +0x10 */ s16 r;
    /* +0x12 */ s16 g;
    /* +0x14 */ s16 b;
    /* +0x16 */ s16 a;
} fltSpr2TF; /* size: 0x18 */

#ifdef __cplusplus
void drawSpr2TF(u8 id, fltSpr2TF* spr, u8 flag);
void subTransSet(u32 a, s32 b, u32* c);
#endif

#ifdef __cplusplus
/* 0x800497AC - the node-buffer allocator the ScnMdl replacement passes call (rule 2).  The target
 * object references the plain name, so C linkage - it belongs inside the `extern "C"` block. */
extern "C" {
#endif

/* Added when `camera/fn_802B5C58.cpp` registered (rule 2).  0x80047398 returns the current
 * `nw4r::g3d::Camera` (through `ScnRoot`); every camera accessor starts from it. */
void* fn_80047398(void);
/* 0x8004BA00 - the index of `id` in a run of `count` `{id, value}` pairs, or -1. */
s32 item_pair_index_find(u16 id, const IdValue* table, s32 count);
/* 0x8004C4F0 - the record copy the light unit's copy constructor calls; added with the
 * `light/light.cpp` registration (rule 2: this range owns the address). */
void color_rgba_copy(u8* dst, const u8* src);
/* 0x8004A240 - applies one 0x100-byte arena user-data record to a player's move-work record: it
 * copies the record's header bytes, unpacks its two packed big-endian words into the work's own
 * `+0x258`/`+0x25C` byte pairs, copies six `_EQUIP` records and two `{u16 id, s16 level}` arrays
 * into the work, and stores the equip type it derives.  Its two consumers are the arena task band's
 * `aqua_eqdata_from_userdata`/`from_vsuser` (both publish the record they built) and
 * `Pl/fn_80288CEC.cpp`'s `fn_8028F1E8`.  Both call sites hand it byte views, so the parameters are
 * the byte pointers they pass.  Renamed from `fn_8004A240` with `quest/arenatask.cpp` (rule 7). */
void arena_userdata_apply(u8* user_data, u8* work);

/* Added with the `ef/eft052.cpp` registration (rule 2: this range owns every one of these
 * addresses - the cabinet/item-page helpers the cockpit hold band calls). */
/* 0x8004A1F8 - the 6-byte (u16, s16) record copy: `dst[0] = src[0]; dst[1] = src[1];` on two
 * `A0 04 00 00`/`A8 04 00 02` pairs.  Added with `menu/menu_result.cpp`, whose result-row
 * initialiser hands it the row state it was given (rule 2: this TU owns the address).  It returns `dst`. */
IdValue* item_pair_copy(void* dst, const void* src);
u16 userdata_box_capacity(void* userdata);
/* 0x8004AEC0 - `userdata_gunner_ck`: declared in the leaf header `fn_80047398/userdata_gunner_ck.h`. */
s32 userdata_pouch_size(u8 idx);
s16 fn_8004AF20(void* userdata);
void* userdata_pouch_get(void* userdata, u8 idx);
/* 0x8004AF78 - the item-slot run `userdata_gunner_ck` selects (`userdata_pouch_get`'s set 1 for a gunner set, else set 0).  GUESS name. */
void* userdata_equip_item_slots_get(void* userdata);
u32 item_count_find(u16 id, void* a, s32 b);
u32 item_slots_count_sum(u16 id, void* a, u16 b);
void fn_8004B200(void* userdata, u16 id, s16 delta);
s16 fn_8004B624(void* userdata, u16 id);
s16 item_slots_room_get(u16 id, void* a, u16 b);
void userdata_item_give(void* userdata, u16 id, s16 count, s32 flag);
void item_box_store(u16 id, s16 count, void* out);

/* 0x8004A430 - decodes the packed 0x100-byte character record `blob` into the player card `card`
 * (`NetPlayerCard`, Network/network_pat_control.h). */
struct NetPlayerCard;
void decodePlayerCard(const u8* blob, struct NetPlayerCard* card);

/* 0x8004AA0C - whether the user record's online byte (+0x3F0D) is clear (GUESS: the network band
 * skips its friend sync while it is). */
s32 isOnlineFlagClear(void);

#ifdef __cplusplus
}
#endif

#ifdef __cplusplus
/* 0x80049070 - registers the sub-transition draw callback `func` (a code address; 0 clears it) for
 * priority slot `slot`.  A C++ free function, the map's `subTransSetPrio__FUcUllPUl`.  Added with
 * `quest/arenatask.cpp` (rule 2: this range owns the address). */
void subTransSetPrio(u8 slot, u32 func, s32 arg, u32* out);
#endif

#endif /* MHTRI_FN_80047398_H */

