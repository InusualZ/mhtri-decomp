/* The shell pool unit `stage/shell.cpp` - the shell work record, its pool and the entry points the
 * player, enemy and lobby units call (docs/plan.md 6.5 rules 1-5).
 *
 * `.text` 0x802AA6A8-0x802ABD28.  The unit's notes, extent and residuals live in the source file's
 * header comment.
 */
#ifndef MHTRI_STAGE_SHELL_H
#define MHTRI_STAGE_SHELL_H

#include "types.h"
#include "nw4r/math.h"

#ifdef __cplusplus

class MHchar;
struct _HIT_W;
struct _HIT_DATA;
struct _HIT_SIZE_DATA;
struct _g3d_work;
struct _ENEMY_WORK;
namespace nw4r { namespace ef { struct Effect; } }

/* The hit-registry record a shell embeds at `_SHELL_W` +0x5C (its second, smaller record sits at +0xB8).
 * `Pl/fn_80295EF4.h`'s `_HIT_W` is a lower bound of 0x60, but the second record starts 0x5C later, so the
 * first is exactly 0x5C. */
#define SHELL_HIT_RECORD_SIZE 0x5C
#define SHELL_HIT(shell) ((_HIT_W*)(shell)->hit_0x5C)
#define SHELL_BODY(shell) ((_HIT_W*)(shell)->body_0xB8)

/* One 0x1A-byte row of an attack table (`shell_attack_set`'s `hit_tbl`): the two values it stores into the
 * shell's hit record and the index of its hit-box size record.  size: 0x1A */
struct _HIT_DATA {
    /* +0x00 */ s16 power_0x00;
    /* +0x02 */ s16 knock_0x02;
    /* +0x04 */ u8 pad_0x04[0xB];
    /* +0x0F */ u8 size_index_0x0F;
    /* +0x10 */ u8 pad_0x10[0xA];
};

/* The shell work record (the map's `_SHELL_W`): one projectile or thrown object, live while
 * `active_0x00` is set.  Allocated out of `shell_work_tbl` by `pull_shell_work`.
 * size: 0x10C */
struct _SHELL_W {
    /* +0x000 */ u8 active_0x00;
    /* +0x001 */ u8 visible_0x01;          /* `shell_set_prim` only draws while it is set */
    /* +0x002 */ u8 pad_0x02;
    /* +0x003 */ u8 type_0x03;             /* the shell type, 0..29 (`ef/fn_800FD718.c`'s switch operand) */
    /* +0x004 */ u8 state_0x04;            /* the four state bytes `pull_shell_work` clears */
    /* +0x005 */ u8 state_0x05;
    /* +0x006 */ u8 state_0x06;
    /* +0x007 */ u8 state_0x07;
    /* +0x008 */ u8 area_0x08;             /* the stage area the shell belongs to (`get_now_areano`) */
    /* +0x009 */ u8 master_kind_0x09;      /* 0 player, 1 enemy, 2 and 3 other owners */
    /* +0x00A */ u8 scr_id_0x0A;           /* `set_shell_scr_id`'s or-ed id; bit 1 and bit 2 select the water test */
    /* +0x00B */ u8 serial_slot_0x0B;      /* this shell's slot in the serial table, 0xFF when it has none */
    /* +0x00C */ void* master_0x0C;        /* the owner's work record (its first byte is its active flag) */
    /* +0x010 */ u8 pad_0x10[0x8];
    /* +0x018 */ nw4r::math::VEC3 pos_0x18;
    /* +0x024 */ u32 rot_x_0x24;
    /* +0x028 */ u32 rot_y_0x28;
    /* +0x02C */ u32 rot_z_0x2C;
    /* +0x030 */ nw4r::math::VEC3 vec_0x30;
    /* +0x03C */ nw4r::math::VEC3 vel_0x3C;
    /* +0x048 */ nw4r::math::VEC3 vel_0x48;
    /* +0x054 */ u8 pad_0x54[0x6];
    /* +0x05A */ u16 hit_id_0x5A;          /* 0xFFFF until a hit is registered */
    /* +0x05C */ u8 hit_0x5C[SHELL_HIT_RECORD_SIZE];   /* a `_HIT_W` */
    /* +0x0B8 */ u8 body_0xB8[0x16];       /* the body hit record's head; byte +5 is its active flag */
    /* +0x0CE */ u16 hit_ids_0xCE[20];     /* the ids of the hits this shell has already taken, 0xFFFF for none */
    /* +0x0F6 */ u8 hit_id_count_0xF6;     /* how many ids are in use */
    /* +0x0F7 */ u8 pad_0xF7;
    /* +0x0F8 */ void (*release_area_0xF8)(_SHELL_W*);   /* called by `shell_area_release` */
    /* +0x0FC */ void (*release_0xFC)(_SHELL_W*);        /* called by `push_shell_work` */
    /* +0x100 */ u8* work_0x100;           /* this shell's private run of 64-byte work blocks */
    /* +0x104 */ s32 work_blocks_0x104;    /* how many blocks the run holds */
    /* +0x108 */ u8 group_0x108;           /* the group `shell_area_release` matches against */
    /* +0x109 */ u8 pad_0x109[0x3];
};

/* One 0x168-byte slot of `shell_chara_heap_tbl`: a model object a shell borrows.  size: 0x168 */
struct ShellCharaHeap {
    /* +0x000 */ u8 used_0x00;
    /* +0x001 */ u8 pad_0x01[0x3];
    /* +0x004 */ u8 chara_0x04[0x164];     /* the `MHchar` the slot hands out */
};

/* One 0xC-byte row of the serial table: a shell registered for network sync.  size: 0xC */
struct ShellSerialEntry {
    /* +0x00 */ _SHELL_W* shell_0x00;
    /* +0x04 */ u8 mode_0x04;
    /* +0x05 */ u8 player_0x05;            /* the player number `serial_set` was called with */
    /* +0x06 */ u8 seq_0x06;               /* that player's running sequence number */
    /* +0x07 */ u8 state_0x07;             /* only ever raised: 1 set, 4 done */
    /* +0x08 */ u16 word_0x08;             /* the state's payload, 0xFFFF when clear */
    /* +0x0A */ u8 pad_0x0A[0x2];
};

/* One 0xC-byte row of the niku table.  size: 0xC */
struct ShellNikuEntry {
    /* +0x00 */ _SHELL_W* shell_0x00;
    /* +0x04 */ u8 mode_0x04;
    /* +0x05 */ u8 player_0x05;
    /* +0x06 */ u8 seq_0x06;
    /* +0x07 */ u8 state_0x07;
    /* +0x08 */ u8 value_0x08;             /* 0xFF when clear */
    /* +0x09 */ u8 pad_0x09[0x3];
};

#define SHELL_WORK_MAX 64
#define SHELL_SERIAL_MAX 64
#define SHELL_NIKU_MAX 12

/* The shell pool header (`.bss` `shell_pool`, 0x3C0): the three tables' bookkeeping and the two
 * entry tables.  size: 0x3C0 */
struct ShellPool {
    /* +0x000 */ u8 ready_0x00;
    /* +0x001 */ u8 pad_0x01[0x3];
    /* +0x004 */ s32 work_max_0x04;
    /* +0x008 */ _SHELL_W* work_0x08;
    /* +0x00C */ s32 work_used_0x0C;
    /* +0x010 */ s32 heap_max_0x10;
    /* +0x014 */ ShellCharaHeap* heap_0x14;
    /* +0x018 */ s32 heap_used_0x18;
    /* +0x01C */ s32 block_max_0x1C;
    /* +0x020 */ u8* blocks_0x20;          /* 64-byte work blocks */
    /* +0x024 */ u8* block_map_0x24;       /* one used byte per block */
    /* +0x028 */ ShellSerialEntry serial_0x28[SHELL_SERIAL_MAX];
    /* +0x328 */ u8 serial_next_0x328[4];  /* next sequence number, per player */
    /* +0x32C */ ShellNikuEntry niku_0x32C[SHELL_NIKU_MAX];
    /* +0x3BC */ u8 niku_next_0x3BC[4];
};

extern ShellPool shell_pool;
extern _SHELL_W shell_work_tbl[SHELL_WORK_MAX];
extern ShellCharaHeap shell_chara_heap_tbl[SHELL_WORK_MAX];

/* 0x802AA6A8 - clears the pool and its tables and marks it ready; the player and the quest entry call it. */
extern "C" void shell_work_init(void);

_SHELL_W* pull_shell_work(u32 work_size);
void push_shell_work(_SHELL_W* shell);
extern "C" MHchar* pull_shell_chara_heap(void);
s32 pull_shell_chara_heap_num(MHchar** out, u32 count);
extern "C" void push_shell_chara_heap(MHchar* chara);
void push_shell_chara_heap_num(MHchar** charas, u32 count);
extern "C" u8* shell_block_alloc(s32 count);
extern "C" void shell_block_free(u8* blocks, s32 count);
extern "C" void shell_block_clear_all(void);
extern "C" s32 shell_block_count(u32 size);
extern "C" void shell_pool_static_init(void);
extern "C" ShellCharaHeap* shell_chara_heap_construct(ShellCharaHeap* self);
extern "C" _SHELL_W* shell_work_construct(_SHELL_W* self);
nw4r::ef::Effect* res_eft_create_shell(u32 group, u32 id);
extern "C" void shell_area_release(u8 group);
void res_shell_model_create(MHchar* model, u32 id, u32 arg);
s32 shell_check_master_beflag(_SHELL_W* shell);
/* untyped: opaque handle - an `MHchar` (draw_set, draw_set2) or an `nw4r::ef::Effect` (draw_set_eff): the three in-unit callers pass two types; the map name fixes `Pv` */
void shell_set_prim(_SHELL_W* shell, void* prim, u8 kind);
void shell_attack_set(_SHELL_W* shell, _HIT_DATA* hit_tbl, _HIT_SIZE_DATA** size_tbl, s32 index,
                      u8 attack_flags, u8 attack_kind);
void shell_hit_cont(_SHELL_W* shell);
void shell_erase_hit(_SHELL_W* shell);
void shell_erase_body(_SHELL_W* shell);
/* untyped: caller-owned payload - the owner's work record (player, enemy or NPC by `kind`); no direct `bl` caller in the DOL (one pointer-table entry), the map name fixes `Pv` */
void shell_master_set(_SHELL_W* shell, void* master, u8 kind);
extern "C" void shell_place_model(_SHELL_W* shell, MHchar* model);
void shell_draw_set(_SHELL_W* shell, MHchar* model);
void shell_draw_set2(_SHELL_W* shell, MHchar* model, nw4r::math::MTX34* mtx);
void shell_draw_set_eff(_SHELL_W* shell, nw4r::ef::Effect* effect);
void shell_rate_add(_SHELL_W* shell);
void shell_rate_add_g(_SHELL_W* shell);
void set_shell_scr_id(_SHELL_W* shell, u8 area_bits, u8 water_bits);
s32 get_shell_in_water(_SHELL_W* shell);

extern "C" void serial_entry_clear(ShellSerialEntry* entry);
extern "C" void serial_table_init(void);
extern "C" ShellSerialEntry* serial_entry_at(u8 index);
ShellSerialEntry* get_shell_serial_sw(_SHELL_W* shell);
extern "C" ShellSerialEntry* serial_find(u8 player, u8 seq);
ShellSerialEntry* serial_set(_SHELL_W* shell, u8 mode, u8 player);
extern "C" void serial_state_raise(ShellSerialEntry* entry, u8 state);
s32 serial_st_change_sw(_SHELL_W* shell, u8 state);
extern "C" s32 serial_st_change_by_key(u8 player, u8 seq, u8 state);
extern "C" void serial_state_set_word(ShellSerialEntry* entry, u8 state, u16 word);
extern "C" void serial_state_set_word_net(ShellSerialEntry* entry, u16 word);
extern "C" s32 serial_state_reached(ShellSerialEntry* entry, u8 state);
s32 serial_st_check_sw(_SHELL_W* shell, u8 state);
void serial_erase(_SHELL_W* shell);

extern "C" void niku_entry_clear(ShellNikuEntry* entry);
extern "C" void niku_table_init(void);
extern "C" ShellNikuEntry* niku_entry_at(u8 index);
ShellNikuEntry* get_niku_info_sw(_SHELL_W* shell);
extern "C" ShellNikuEntry* niku_find(u8 player, u8 seq);
ShellNikuEntry* niku_set(_SHELL_W* shell, u8 mode, u8 player);
extern "C" void niku_state_set(ShellNikuEntry* entry, u8 state);
s32 niku_st_change_sw(_SHELL_W* shell, u8 state);
extern "C" void niku_enemy_attach(ShellNikuEntry* entry, _ENEMY_WORK* enemy);
extern "C" u32 niku_enemy_serial_matches(ShellNikuEntry* entry, _ENEMY_WORK* enemy);
extern "C" s32 niku_st_change_by_key(u8 player, u8 seq, u8 state);
void niku_erase(_SHELL_W* shell);

#endif /* __cplusplus */

#endif /* MHTRI_STAGE_SHELL_H */
