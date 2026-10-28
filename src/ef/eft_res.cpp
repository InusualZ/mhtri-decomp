/* ef/eft_res.cpp - the game's effect-resource manager: `eft_control` and its slot pools, the 256-entry proID slot
 *   table, the resource load/create path and the effect-heap push.
 * RANGE. .text 0x800F6520-0x800F95A4 (43 functions); extab 0x8000BB6C-0x8000BC5C, extabindex 0x8002580C-0x80025974,
 *   .data 0x8059B3B0-0x8059B5D0, .sbss 0x80794A40-0x80794A90, .sdata2 0x807965D0-0x807965D8.  The seam is unproven:
 *   the range's last function drives the same `_EFT` state machine as `ef/effect.cpp`'s first.
 * NAMES. The unit name is a GUESS in the siblings' scheme (no `__FILE__` string is reachable; the runtime dump's
 *   `res_eft_*`, `get_eft_res_name` and `push_eft_effect_heap_num` name the `eft` resource layer).  The dump names
 *   seven functions; `eft_res_slot_get`, `eft_res_slot_release`, `eft_res_model_get`, `eft_res_spawn_gate_ck`, `eft_res_models_release` and
 *   `eft_res_models_spawn` are GUESSes from their bodies, `eft_stage_spawn_flag` (0x800F9380: stage size class to spawn
 *   flag) too.
 *   GUESS (from each body and its callers): eft_pro_slot_list_set_area (0x800F6984: the area-tagged file run),
 *   eft_pro_slot_clear (0x800F6A78), eft_pro_slot_find_kind (0x800F6AC8), eft_pro_slot_find_proid (0x800F7AA4),
 *   eft_pro_slot_resource_add (0x800F7BF0), eft_res_load_callback (0x800F7D44), eft_res_files_load (0x800F7F18),
 *   eft_res_archive_load_callback (0x800F8358), eft_res_model_release (0x800F89A4), eft_res_blocks_get /
 *   eft_res_blocks_release / eft_res_block_count (0x800F8B44 / 0x800F8C78 / 0x800F8D80: the 0x40-byte block heap),
 *   eft_res_effect_spawn (0x800F9278), eft_res_handle_copy / eft_res_handle_valid (0x800F92D4 / 0x800F92E0), and
 *   the inline helper eft_res_slot_find.
 *   GUESS (from each body and its callers): eft_control_init
 *   GUESS (from each body and its callers): eft_res_release_all
 *   GUESS (from each body and its callers): eft_pro_slot_assign (0x800F6B6C: files a table entry under a free proID
 *   slot), eft_area_effects_set (0x800F6DB4: `stage_area_enter`'s per-map/area effect starts); the callees named
 *   here are GUESSes filed with their owners: `pack_entry_count`/`pack_entry_data` (system_core), `em_kind_mem_alloc`
 *   (enemy_control), `eft028_set`/`eft028_set_capsule` (eft026_fx), `nw_res_anm_work_num`/`nw_res_anm_create`
 *   (nw_resource), and draw_shape's common file descriptors `q_comn_efm_file`, `lobby_eft_mdl_file`,
 *   `lb_comn_eft_file`, `q_comn_eft_file`.
 *   GUESS (from each body and its callers): `eft_res_pool_reset`.
 * FLAGS. `cflags_main`; `#pragma peephole off` over the file (retail keeps every `clrlwi`+`cmpwi` pair and the
 *   narrowing `clrlwi` of the `u8`/`u16` parameters; with the pass on most rows score lower).
 * RESIDUALS.
 *  - `eft_res_file_load`: retail tests the descriptor's size word in r4, ours in r0 (both branches);
 *  - `eft_res_models_spawn`: the texture loop loads the model's +0x118 before its +0x10C link in retail; ours loads
 *    the link first (the operand order that puts the model pointer in retail's r4);
 *  - `eft_common_load`: retail carries the `eft` run's `j - 1` as its own induction register (three for the loop),
 *    ours derives it with `addi` from `j`;
 *  - `eft_res_files_load`: retail keeps `id * 0x18` and not `id` across the calls (one register fewer, `0` hoisted into
 *    r14); `eft_res_archive_load_callback`: retail hoists the 128-entry `mtctr` count into r18 (one saved register more);
 *  - `eft_pro_slot_find_kind`: retail walks 0xCC records unrolled four-wide (`li r0,0x33`); ours unrolls the same count
 *    six-wide (`li r0,0x22`) for the pointer walk, the `while` form and the counter widths tried (u8/u16/s32/u32);
 *  - `eft_res_slot_get`: the search loop's natural exit runs the caller's NULL test (`li r31,0`; `cmpwi`) where
 *    retail returns at once (a jump past the tail); the inline `eft_res_slot_find` scores best of the goto-free
 *    forms (a `found` flag, a `break` plus an index test and the tail moved into the loop all score lower);
 *  - `eft_res_blocks_get`: retail shares one `return NULL` block between its two bound tests and places the run-found
 *    tail after the scan loop (`beq` out of the loop); ours returns inline after the skip loop, keeps the tail in the
 *    loop (`bne` around it) and colours the registers differently;
 *   The other partial rows have no recorded cause (`symdiff.py -u ef/eft_res --all`).
 * SHAPES. `EftControl* ctrl = &eft_control;` opens every function that touches it, so the address lives in one
 *   callee-saved register (the unhoisted form loses the register on `eft_res_slot_release` and `eft_res_model_release`).
 *   A record reached by ordinal is `&((EftProTable*)lbl_806A2D34)->slots_0x04[id]` (`eft_pro_slot_clear`, `eft_res_release`:
 *   the base and the record land in two registers); a walk starts at `eft_pro_slots()` and steps the counter before
 *   the pointer (`i++, slot++` in the `for` header).  Counters compared against `eft_control`'s pool counts are
 *   signed (`(s32)` on the `u32` fields gives retail's `cmpw`).
 *   `eft_area_effects_set`'s jump-table inner switches end in `default: return;` (it keeps retail's unreachable
 *   `b` of the outer `break`); the compare-tree inner switch of map 10 has none.
 *   `load_file_req`'s arguments are (name, data, size, callback, 4, ctx): the callback is the fourth.
 *   `eft_res_blocks_release` assigns the pointer difference to its `u32` index and shifts it in place (`index >>= 6`): the signed
 *   form emits `srawi` and a `clrrwi`, the one-expression form moves the index to another register.
 */

#include "types.h"

#pragma peephole off
#include "nw4r/math.h"
#include "ef.h"
#include "ef/ef_emitter.h" /* ef_store_word (rule 2) */
#include "pl.h"
#include "gx.h"
#include "unsplit/g3d.h"
#include "Runtime.PPCEABI.H/memset.h"
#include "ef/fn_800AEE48.h"
#include "ef/fn_800CDB2C.h"
#include "nw_resource.h"
#include "sound/fn_800DD1F0.h"
#include "sound/sound_job.h" /* split_screen_size_class (rule 2) */
#include "ef/pRoot.h"
#include "stage/get_now_mapno.h"
#include "draw_shape.h" /* res_file_ctor / res_file_assign (rule 2) */
#include "g3d/g3d_scnmdlsmpl.h" /* nw4r::g3d::ScnMdlSimple::GetResMdl (rule 2) */
#include "ef/system_core.h"   /* pack_entry_count, pack_entry_data */
#include "enemy/enemy_control.h"   /* em_kind_mem_alloc */
#include "ef/eft026_fx.h"   /* eft028_set, eft028_set_capsule */
#include "ef/RetireParticleAll.h"
#include "ef/eft_res_file_load.h"

/* ---------------------------------------------------------------------------------------------------
 * the control block, the slot table and the model records
 * ------------------------------------------------------------------------------------------------- */

/* The 0x48-byte per-effect slot of `eft_control.slots_0x10` (`get_move_work_adrs(4)`): the record the
 * unit's own teardown walks (`+0x34` hook, `+0x38`/`+0x3C` block run, `+0x40` callback). size: 0x48 */
typedef struct EftResSlot {
    /* +0x00 */ u8 active_0x00;             /* 1 while the slot is handed out */
    /* +0x01 */ u8 field_0x01;
    /* +0x02 */ u8 field_0x02;
    /* +0x03 */ u8 field_0x03;
    /* +0x04 */ u8 field_0x04;              /* bit 0 set = the teardown skips the slot */
    /* +0x05 */ u8 field_0x05;
    /* +0x06 */ u8 field_0x06;
    /* +0x07 */ u8 field_0x07;
    /* +0x08 */ u8 field_0x08;
    /* +0x09 */ u8 field_0x09;
    /* +0x0A */ u8 pad_0x0A[0x0A];
    /* +0x14 */ u8 field_0x14;
    /* +0x15 */ u8 field_0x15;
    /* +0x16 */ u8 field_0x16;
    /* +0x17 */ u8 field_0x17;
    /* +0x18 */ u8 pad_0x18[0x1C];
    /* +0x34 */ void (*update_0x34)(void*);   /* the per-frame hook eft_instances_release runs */
    /* +0x38 */ void* blocks_0x38;            /* the 0x40-byte block run this slot owns */
    /* +0x3C */ u32 block_count_0x3C;         /* how many of them */
    /* +0x40 */ void (*release_0x40)(void*);  /* the release hook eft_res_slot_release runs first */
    /* +0x44 */ u32 field_0x44;
} EftResSlot; /* size: 0x48 */

/* The manager's control block `eft_control` (`EftControl`, 0xC44 B of `.bss`) is the band's one record (`unsplit/ef_control.h`); this unit is its main reader. */
#include "unsplit/ef_control.h"

/* The effect system's allocator `ef_system_memory_manager` returns (`nw4r::ef::MemoryManagerBase` in
 * `nw_resource.cpp`), reached through its +0x0C slot, the pools' garbage collection.  Declared only, so this unit
 * emits no table of its own.  size: 0x04 - lower bound, an approximation (the vtable word) */
class EftResEffectAllocator {
public:
    virtual void destroy();
    virtual void garbage_collection();
};

/* The record eft_res_spawn_gate_ck gates a spawn on: a pointer at +0x30 whose byte 0 must be clear when one is
 * pending.  Only that pointer is touched by this unit. */
typedef struct EftSpawnRecord {
    /* +0x00 */ u8 pad_0x00[0x30];
    /* +0x30 */ u8* pending_0x30;
} EftSpawnRecord; /* size: 0x34 (lower bound: only +0x30 is read by this unit) */

/* The 256-entry slot table that follows `eft_control` in `.bss`: a 4-byte header, then 0x18-byte records
 * whose proID byte `eft_pro_slot_find_kind` looks up (`eft_pro_slot_clear` builds one, `eft_res_release` releases it).
 * Every function reaches it through the map's own absolute label, never as an offset of `eft_control`. */
#include "unsplit/ef_tables.h" /* lbl_806A2D34 (rule 2: the band) */

typedef struct EftProSlot {
    /* +0x00 */ u8 kind_0x00;    /* eft_pro_slot_find_kind's key; 2 = a registered effect (eft_res_release) */
    /* +0x01 */ u8 field_0x01;   /* 0xff; the "handed out" flag eft_pro_slot_find_proid tests for 1 */
    /* +0x02 */ u8 state_0x02;   /* 0xff; 1 while the resource is live */
    /* +0x03 */ u8 id_0x03;      /* the slot ordinal the effect's resources are filed under */
    /* +0x04 */ u8 field_0x04;   /* the proID eft_pro_slot_find_proid matches */
    /* +0x05 */ u8 field_0x05;   /* 0xff, or the 1-based ordinal of the resource in its file */
    /* +0x06 */ s16 field_0x06;  /* -1, or the loaded resource's size */
    /* +0x08 */ s32 field_0x08;  /* -1 from eft_pro_slot_clear, 0x7FFFFFFF from eft_res_archive_load_callback */
    /* +0x0C */ s32 field_0x0C;  /* the loaded resource id */
    /* +0x10 */ s32 field_0x10;
    /* +0x14 */ s32 field_0x14;
} EftProSlot; /* size: 0x18 */

/* The whole slot table as one record: the header's four live counts (all records, then the `efm`, `eft` and `efx`
 * records `eft_res_load_callback`/`eft_res_archive_load_callback` number), then the 256 records. */
typedef struct EftProTable {
    /* +0x0000 */ u8 used_0x00;
    /* +0x0001 */ u8 models_0x01;
    /* +0x0002 */ u8 effects_0x02;
    /* +0x0003 */ u8 textures_0x03;
    /* +0x0004 */ EftProSlot slots_0x04[0x100];
} EftProTable; /* size: 0x1804 */

/* `EftResFile` (the loaders' resource-file descriptor) comes from `draw_shape.h` and `EftResFileHandle` (the one-word
 * file/model handle) from `nw_resource.h`, the owners of the data and the functions that use them (rule 1). */

/* The pool record the 0x168-byte model arrays hold: a used byte, then the EftModel `eft_res_model_get`
 * hands out (the copy `res_eft_model_create_light` fills). */
typedef struct EftResModelSlot {
    /* +0x000 */ u8 used_0x00;
    /* +0x004 */ EftModel model_0x04;
    /* +0x120 */ u8 unused_0x120[0x48];
} EftResModelSlot; /* size: 0x168 */

/* The node `EftModel.field_0x10C` points at; only +0x04 is read here.  `ef.h` leaves the type
 * incomplete, so this unit names the field it needs. */
typedef struct EftResModelLink {
    /* +0x00 */ u8 pad_0x00[0x4];
    /* +0x04 */ u32 field_0x04;
} EftResModelLink; /* size: 0x08 (lower bound: only +0x04 is read) */

/* The stack context the load callbacks receive: the mode byte picks which of the three resource-id
 * arrays the loaded handle is filed in, +0x04 is the record the load belongs to and +0x08 the
 * "not filed under a shared name" flag. */
typedef struct EftLoadCtx {
    /* +0x00 */ u32 mode;
    /* +0x04 */ struct EftProSlot* slot;
    /* +0x08 */ u32 unshared_0x08;
    /* +0x0C */ u32 param_0x0C;
} EftLoadCtx; /* size: 0x10 */

/* The context `eft_res_file_load` hands the archive load and `eft_res_archive_load_callback` reads back: the run's proID bytes, the slot
 * ordinal the run starts at, the run's resource names and the load mode. */
typedef struct EftFileLoadCtx {
    /* +0x00 */ u8* proids_0x00;
    /* +0x04 */ u32 first_0x04;
    /* +0x08 */ char** names_0x08;
    /* +0x0C */ s32 mode_0x0C;
} EftFileLoadCtx; /* size: 0x10 */

/* The records start 4 bytes into the table (its 4-byte header is zeroed by eft_res_pool_reset and counted down
 * by eft_res_release). */
inline EftProSlot* eft_pro_slots(void) {
    return (EftProSlot*)(lbl_806A2D34 + 4);
}

/* ---------------------------------------------------------------------------------------------------
 * this unit's own entry points (`ef/eft_res.h` declares the ones other units call)
 * ------------------------------------------------------------------------------------------------- */

#ifdef __cplusplus
extern "C" {
#endif

void eft_res_pool_reset(void);
void eft_control_init(void);
void eft_res_release_all(void);
void eft_pro_slot_list_set_area(u8, u8, EftResFile*, u8*);
void eft_pro_slot_list_set(u32, EftResFile*, u8*);
void eft_res_stage_release_all(void);
void eft_res_slots_release(void);
void eft_pro_slot_clear(u8);
EftProSlot* eft_pro_slot_find_kind(u8);
u32 eft_res_release(u8);
EftProSlot* eft_pro_slot_find_proid(u8);
void eft_pro_slot_resource_add(EftProSlot*);
void eft_res_bind(u32);
void eft_res_load_callback(u8*, void*, EftLoadCtx*, EftLoadCtx*);
void eft_res_files_load(EftResFile*, s32, u32, u8*);
void eft_res_archive_load_callback(char*, void*, u32, EftFileLoadCtx*);
void eft_common_load(void);
void eft_area_effects_set(void);
u8 eft_pro_slot_assign(u8, u8, u8, u8, u8);
EftResSlot* eft_res_slot_get(u32);
void eft_res_slot_release(void*);
u8* eft_res_model_get(void);
void eft_res_model_release(void*);
void eft_res_models_release(void*, s32);
void* eft_res_blocks_get(u32);
void eft_res_blocks_release(void*, u32);
u32 eft_res_block_count(u32);
void eft_instances_release(void);
void* res_eft_create_ex(u16, u16, u32, u32);
void* eft_res_effect_spawn(void*, void*, u32, u32);
void* eft_res_handle_copy(void*, void*);
u32 eft_res_handle_valid(void*);
u32 eft_res_spawn_gate_ck(void*, u32);
u32 eft_stage_spawn_flag(void);
void eft_res_models_spawn(_EFT*, void**, s32, s32, void*);
void fn_800F95A4(void*);
u32 fn_800F97F0(u16, void*, void*);
void fn_800FA450(void*);
void fn_800FA5D4(_EFT*);
void fn_800FD864(void);
void sound_job_request(s32, u8, s32, s32, s32, void*, s32);
void mhchar_reset(void*);
void mhchar_draw_mode_set(void*, s32);
void mhchar_model_create(MHchar*, u32*, u32, s32);
void res_mdl_copy_ctor(void*, void*);
extern s32 ef_system_memory_manager(void*);
extern void* fn_800A5484(void*);
extern void* fn_800A5A90(void*, void*, void*, u32);

#ifdef __cplusplus
}
#endif

/* The resource tables the scene loaders install (`.sbss` 0x80794A40-0x80794A90): the per-group proID byte tables
 * `eft_pro_slot_assign` resolves an effect through (0xFF ends each run), the model-name table `get_eft_res_name` reads, the
 * area box table `ef/effect.cpp` reads, the four common-file name tables `eft_common_load` hands the loader, and the two per-map common eft/efm
 * name tables `stage_map_obj_load` reads. */
#ifdef __cplusplus
extern "C" {
#endif
u8** eft_em_proID_tbl_ptr;
u8* efx_em_proID_tbl_ptr;
u8** efm_em_proID_tbl_ptr;
u8*** eft_map_proID_tbl_ptr;
u8*** efm_map_proID_tbl_ptr;
u8** efm_map_com_proID_tbl_ptr;
u8** eft_map_com_proID_tbl_ptr;
u8** eft_common_proID_tbl_ptr;
u8** efx_map_com_proID_tbl_ptr;
u8** efx_common_proID_tbl_ptr;
u8** efm_common_proID_tbl_ptr;
char** res_model_name_ptr;
char** eft_name_tbl_ptr;
void** eft_scnbox_data_ptr;
char** q_comn_eft_name_ptr;
char** lb_comn_eft_name_ptr;
char** q_comn_efm_name_ptr;
char** lobby_eft_mdl_name_ptr;
char*** map_com_eft_name_ptr;
char*** eft_map_common_mdl_name_ptr;
#ifdef __cplusplus
}
#endif

/* C++ callees: the target object references their manglings (`getResMemAdrs__Fl`,
 * `get_move_work_adrs__FUc`, `get_move_work_max__FUc`, `get_now_areano__Fv`,
 * `load_file_req__FPcUllUllPUl`, `nwAddResource__FPcPv`, `pull_res_mem__FPcUll`,
 * `push_g3d_wk__FP9_g3d_work`), so they are declared at C++ scope, outside the extern "C" block
 * above (relocaudit). */
struct _g3d_work;
void* get_move_work_adrs(u8 index);
u16 get_move_work_max(u8 index);
u8 get_now_areano(void);
void load_file_req(char*, u32, s32, u32, s32, u32*);
void push_g3d_wk(struct _g3d_work* work);

/* g3d's root node (the map spells it `pRoot`; ef/eft007.cpp declares the same) */

u32 res_eft_model_create_light(MHchar* model, u16 id, u32 arg, long light);
/* untyped: opaque handle - the UV animation record the caller stores */
void* res_eft_UV_model_create_name(MHchar* model, char* name, u32 arg, long light, struct _g3d_work** list, long count,
                                   u8 flag);

/* ---------------------------------------------------------------------------------------------------
 * the functions, in address order
 * ------------------------------------------------------------------------------------------------- */

/* 0x800F6520 - zero the control block, then build all 256 slot-table records. */
extern "C" void eft_res_pool_reset(void) {
    memset(&eft_control, 0, sizeof(eft_control));
    lbl_806A2D34[0] = 0;
    lbl_806A2D34[1] = 0;
    lbl_806A2D34[2] = 0;
    lbl_806A2D34[3] = 0;

    for (s32 i = 0; i < 0x100; i++) {
        eft_control.id_a_0x3C[i] = -1;
        eft_control.id_b_0x43C[i] = -1;
        eft_control.id_c_0x83C[i] = -1;
        eft_pro_slot_clear((u8)i);
    }
}

/* 0x800F65B4 - allocate the three work pools (0x48-byte effect slots, 0x168-byte model records and the
 * 0x40-byte block heap with its trailing flag run) and mark the manager initialised. */
extern "C" void eft_control_init(void) {
    eft_res_bind(0);

    eft_control.slots_0x10 = (EftResSlot*)get_move_work_adrs(4);
    eft_control.slot_count_0x0C = get_move_work_max(4);
    memset(eft_control.slots_0x10, 0, eft_control.slot_count_0x0C * 0x48);

    eft_control.models_0x1C = (u8*)get_move_work_adrs(5);
    eft_control.model_count_0x18 = get_move_work_max(5);
    memset(eft_control.models_0x1C, 0, eft_control.model_count_0x18 * 0x168);

    eft_control.heap_0x28 = (u8*)get_move_work_adrs(6);
    eft_control.heap_count_0x24 = get_move_work_max(6);
    eft_control.heap_flags_0x2C = eft_control.heap_0x28 + eft_control.heap_count_0x24 * 0x40;
    memset(eft_control.heap_0x28, 0, eft_control.heap_count_0x24 + eft_control.heap_count_0x24 * 0x40);

    eft_control.initialised_0x00 = 1;
}

/* 0x800F6688 - run the teardown over every live effect slot. */
extern "C" void eft_res_release_all(void) {
    EftResSlot* slot = eft_control.slots_0x10;

    if (slot == NULL) {
        return;
    }
    for (s32 i = 0; i < (s32)eft_control.slot_count_0x0C; i++, slot++) {
        if (slot->active_0x00 != 0 && (slot->field_0x04 & 1) == 0) {
            eft_res_slot_release(slot);
        }
    }
}

/* 0x800F6710 (0x274): resolves the mode's common effect files (lobby or quest) into proID slots and requests their
 * load: the `efm` run first, then the `eft` run, whose odd entries pair a plain and an alternate id. */
extern "C" void eft_common_load(void) {
    EftResFile* file;
    u8 ids[0x50];
    u32 i;
    u32 j;

    switch (GameMode_ck()) {
    case 2:
        file = lobby_eft_mdl_file;
        for (i = 0; efm_common_proID_tbl_ptr[1][i] != 0xFF; i++) {
            ids[i] = eft_pro_slot_assign(0, 3, 1, 0, i);
        }
        eft_res_file_load(file, 3, 0, ids, efm_common_proID_tbl_ptr[1], lobby_eft_mdl_name_ptr);
        file = lb_comn_eft_file;
        ids[0] = eft_pro_slot_assign(0, 2, 1, 0, 0);
        for (i = 1, j = 2; eft_common_proID_tbl_ptr[1][j - 1] != 0xFF; i += 2, j += 2) {
            ids[j - 1] = eft_pro_slot_assign(0, 1, 1, 0, i);
            ids[j] = eft_pro_slot_assign(0, 1, 1, 1, j);
        }
        eft_res_file_load(file, 0, 0, ids, eft_common_proID_tbl_ptr[1], lb_comn_eft_name_ptr);
        break;
    case 1:
        file = q_comn_efm_file;
        for (i = 0; efm_common_proID_tbl_ptr[0][i] != 0xFF; i++) {
            ids[i] = eft_pro_slot_assign(0, 3, 0, 0, i);
        }
        eft_res_file_load(file, 3, 0, ids, efm_common_proID_tbl_ptr[0], q_comn_efm_name_ptr);
        file = q_comn_eft_file;
        ids[0] = eft_pro_slot_assign(0, 2, 0, 0, 0);
        for (i = 1, j = 2; eft_common_proID_tbl_ptr[0][j - 1] != 0xFF; i += 2, j += 2) {
            ids[j - 1] = eft_pro_slot_assign(0, 1, 0, 0, i);
            ids[j] = eft_pro_slot_assign(0, 1, 0, 1, j);
        }
        eft_res_file_load(file, 0, 0, ids, eft_common_proID_tbl_ptr[0], q_comn_eft_name_ptr);
        break;
    }
}

/* 0x800F6984 - load one resource set with the 0x20000 flag family. */
extern "C" void eft_pro_slot_list_set_area(u8 a, u8 b, EftResFile* desc, u8* table) {
    eft_res_files_load(desc, b, (a << 16) | 2, table);
}

/* 0x800F69A0 - load one resource set with flag word 1. */
extern "C" void eft_pro_slot_list_set(u32 a, EftResFile* desc, u8* table) {
    eft_res_files_load(desc, (u8)a, 1, table);
}

/* 0x800F69B8 - retire every slot whose kind/state pair says its resource list is live. */
extern "C" void eft_res_stage_release_all(void) {
    EftProSlot* slot = &eft_pro_slots()[0x100 - 1];

    for (s32 i = 0xFF; i >= 0; i--, slot--) {
        if (slot->kind_0x00 == 2 && slot->state_0x02 == 1) {
            eft_res_release(slot->id_0x03);
        }
    }
}

/* 0x800F6A24 - retire every slot of the table. */
extern "C" void eft_res_slots_release(void) {
    EftProSlot* slot = &eft_pro_slots()[0x100 - 1];

    for (s32 i = 0xFF; i >= 0; i--, slot--) {
        eft_res_release(slot->id_0x03);
    }
}

/* 0x800F6A78 - build one slot-table record (the id eft_res_pool_reset files it under). */
extern "C" void eft_pro_slot_clear(u8 id) {
    EftProSlot* slot = &((EftProTable*)lbl_806A2D34)->slots_0x04[id];

    slot->kind_0x00 = 0;
    slot->field_0x01 = 0xFF;
    slot->state_0x02 = 0xFF;
    slot->id_0x03 = id;
    slot->field_0x04 = 0xFF;
    slot->field_0x05 = 0xFF;
    slot->field_0x06 = -1;
    slot->field_0x08 = -1;
    slot->field_0x0C = -1;
    slot->field_0x10 = 0;
    slot->field_0x14 = 0;
}

/* 0x800F6AC8 - find the first of the table's 0x33 records whose key byte is `key`. */
extern "C" EftProSlot* eft_pro_slot_find_kind(u8 key) {
    EftProSlot* slot = eft_pro_slots();

    for (s32 i = 0; i < 0xCC; i++, slot++) {
        if (slot->kind_0x00 == key) {
            return slot;
        }
    }
    return NULL;
}

/* 0x800F6B6C (0x248): takes a free proID slot and files the effect `index` of the group/kind's table under it;
 * returns the slot ordinal, or 0xFF when no slot is free or the table has no such effect. */
extern "C" u8 eft_pro_slot_assign(u8 group, u8 kind, u8 set, u8 sub, u8 index) {
    EftProSlot* slot;
    u8* table;

    slot = eft_pro_slot_find_kind(0);
    if (slot != NULL) {
        switch (group) {
        case 0:
            switch (kind) {
            case 3:
                if (set == 2) {
                    table = efm_map_com_proID_tbl_ptr[sub];
                } else {
                    table = efm_common_proID_tbl_ptr[set];
                }
                break;
            case 1:
                if (set == 2) {
                    table = eft_map_com_proID_tbl_ptr[sub];
                } else if (set == 3) {
                    table = eft_map_com_proID_tbl_ptr[sub];
                    kind = 2;
                } else {
                    table = eft_common_proID_tbl_ptr[set];
                    if (sub == 1) {
                        kind = 2;
                    }
                }
                break;
            case 2:
                if (set == 2) {
                    table = efx_map_com_proID_tbl_ptr[sub];
                } else {
                    table = efx_common_proID_tbl_ptr[set];
                }
                break;
            }
            break;
        case 2:
            switch (kind) {
            case 3:
                table = efm_em_proID_tbl_ptr[set];
                break;
            case 1:
                table = eft_em_proID_tbl_ptr[set];
                break;
            case 2:
                table = efx_em_proID_tbl_ptr;
                index = set;
                break;
            }
            break;
        case 1:
            switch (kind) {
            case 3:
                table = efm_map_proID_tbl_ptr[set][sub];
                break;
            case 1:
            case 2:
                table = eft_map_proID_tbl_ptr[set][sub];
                break;
            }
            break;
        default:
            return 0xFF;
        }
        if (table != NULL) {
            slot->field_0x04 = table[index];
            if (slot->field_0x04 != 0xFF) {
                slot->kind_0x00 = 1;
                slot->state_0x02 = group;
                slot->field_0x01 = kind;
                return slot->id_0x03;
            }
            return 0xFF;
        }
    }
    return 0xFF;
}
/* 0x800F6DB4 (0x9C4): starts the current map area's resident effects on area entry: the eft004 area spawner and the
 * eft028 break/crumble setters each map/area pair places. */
extern "C" void eft_area_effects_set(void) {
    u8 area;
    u8 map;

    area = get_now_areano();
    map = get_now_mapno();
    switch (map) {
    case 1:
        switch (area) {
        case 0:
            fn_800FD864();
            eft028_set(0, 0, area);
            break;
        case 1:
            fn_800FD864();
            break;
        case 2:
            fn_800FD864();
            break;
        case 3:
            fn_800FD864();
            break;
        case 4:
            fn_800FD864();
            break;
        case 5:
            fn_800FD864();
            break;
        case 6:
            fn_800FD864();
            break;
        case 7:
            fn_800FD864();
            eft028_set(0, 0, area);
            break;
        case 8:
            fn_800FD864();
            break;
        case 9:
            fn_800FD864();
            break;
        case 10:
            fn_800FD864();
            break;
        case 11:
            fn_800FD864();
            break;
        case 12:
            fn_800FD864();
            eft028_set(0, 0, area);
            break;
        default:
            return;
        }
        break;
    case 2:
        switch (area) {
        case 0:
            fn_800FD864();
            break;
        case 1:
            fn_800FD864();
            break;
        case 2:
            fn_800FD864();
            break;
        case 3:
            fn_800FD864();
            break;
        case 4:
            fn_800FD864();
            break;
        case 5:
            fn_800FD864();
            break;
        case 6:
            fn_800FD864();
            break;
        case 7:
            fn_800FD864();
            break;
        case 8:
            fn_800FD864();
            break;
        case 9:
            fn_800FD864();
            break;
        case 10:
            fn_800FD864();
            break;
        case 11:
            fn_800FD864();
            break;
        default:
            return;
        }
        break;
    case 3:
        switch (area) {
        case 0:
            fn_800FD864();
            break;
        case 1:
            eft028_set(5, 1, area);
            break;
        case 2:
            fn_800FD864();
            eft028_set(3, 0, area);
            eft028_set(5, 1, area);
            break;
        case 3:
            eft028_set(3, 0, area);
            eft028_set(5, 1, area);
            break;
        case 4:
            fn_800FD864();
            eft028_set(3, 0, area);
            eft028_set(5, 1, area);
            break;
        case 5:
            fn_800FD864();
            eft028_set(5, 1, area);
            break;
        case 6:
            fn_800FD864();
            eft028_set(0, 0, area);
            break;
        case 7:
            eft028_set(5, 1, area);
            break;
        case 8:
            fn_800FD864();
            eft028_set(3, 0, area);
            break;
        case 9:
            fn_800FD864();
            eft028_set(5, 1, area);
            break;
        default:
            return;
        }
        break;
    case 4:
        switch (area) {
        case 0:
            fn_800FD864();
            break;
        case 1:
            fn_800FD864();
            break;
        case 2:
            fn_800FD864();
            break;
        case 3:
            fn_800FD864();
            break;
        case 4:
            fn_800FD864();
            break;
        case 5:
            fn_800FD864();
            break;
        case 6:
            fn_800FD864();
            break;
        case 7:
            fn_800FD864();
            break;
        case 8:
            fn_800FD864();
            break;
        case 9:
            fn_800FD864();
            break;
        default:
            return;
        }
        break;
    case 5:
        switch (area) {
        case 0:
            fn_800FD864();
            break;
        case 1:
            fn_800FD864();
            break;
        case 2:
            fn_800FD864();
            break;
        case 3:
            fn_800FD864();
            break;
        case 4:
            fn_800FD864();
            break;
        case 5:
            fn_800FD864();
            break;
        case 6:
            fn_800FD864();
            break;
        case 7:
            fn_800FD864();
            break;
        case 8:
            fn_800FD864();
            break;
        case 9:
            fn_800FD864();
            break;
        case 10:
            fn_800FD864();
            break;
        default:
            return;
        }
        break;
    case 6:
        switch (area) {
        case 0:
            fn_800FD864();
            break;
        case 1:
            fn_800FD864();
            break;
        case 2:
            fn_800FD864();
            break;
        default:
            return;
        }
        break;
    case 7:
        switch (area) {
        case 0:
            fn_800FD864();
            break;
        case 1:
            fn_800FD864();
            eft028_set(4, 1, area);
            break;
        case 2:
            eft028_set(4, 1, area);
            break;
        case 3:
            fn_800FD864();
            eft028_set(4, 1, area);
            break;
        default:
            return;
        }
        break;
    case 8:
        switch (area) {
        case 1:
            fn_800FD864();
            break;
        case 2:
            eft028_set(0, 0, area);
            break;
        default:
            return;
        }
        break;
    case 9:
        switch (area) {
        case 0:
            fn_800FD864();
            break;
        case 1:
            eft028_set(0, 0, area);
            break;
        default:
            return;
        }
        break;
    case 10:
        switch (area) {
        case 0:
            fn_800FD864();
            break;
        case 1:
            fn_800FD864();
            break;
        }
        break;
    case 11:
        switch (area) {
        case 1:
            eft028_set(0, 0, area);
            break;
        default:
            return;
        }
        break;
    case 12:
        switch (area) {
        case 0:
            fn_800FD864();
            eft028_set(7, 0, area);
            break;
        case 1:
            fn_800FD864();
            break;
        case 2:
            fn_800FD864();
            break;
        case 3:
            fn_800FD864();
            break;
        case 4:
            fn_800FD864();
            break;
        case 5:
            fn_800FD864();
            break;
        case 6:
            fn_800FD864();
            break;
        case 7:
            fn_800FD864();
            eft028_set(7, 0, area);
            break;
        case 8:
            fn_800FD864();
            break;
        case 9:
            fn_800FD864();
            break;
        case 10:
            fn_800FD864();
            break;
        case 11:
            fn_800FD864();
            break;
        case 12:
            fn_800FD864();
            eft028_set(7, 0, area);
            break;
        default:
            return;
        }
        break;
    case 13:
        switch (area) {
        case 0:
            fn_800FD864();
            break;
        case 1:
            fn_800FD864();
            break;
        case 2:
            fn_800FD864();
            break;
        case 3:
            fn_800FD864();
            break;
        case 4:
            fn_800FD864();
            break;
        case 5:
            fn_800FD864();
            break;
        case 6:
            fn_800FD864();
            break;
        case 7:
            fn_800FD864();
            break;
        case 8:
            fn_800FD864();
            break;
        case 9:
            fn_800FD864();
            break;
        case 10:
            fn_800FD864();
            break;
        case 11:
            fn_800FD864();
            break;
        default:
            return;
        }
        break;
    case 14:
        switch (area) {
        case 0:
            fn_800FD864();
            break;
        case 1:
            fn_800FD864();
            break;
        case 2:
            fn_800FD864();
            eft028_set(3, 0, area);
            break;
        case 3:
            fn_800FD864();
            eft028_set(3, 0, area);
            break;
        case 4:
            fn_800FD864();
            eft028_set(3, 0, area);
            break;
        case 5:
            fn_800FD864();
            break;
        case 6:
            fn_800FD864();
            eft028_set(7, 0, area);
            break;
        case 7:
            fn_800FD864();
            break;
        case 8:
            fn_800FD864();
            eft028_set(3, 0, area);
            break;
        case 9:
            fn_800FD864();
            break;
        case 10:
            fn_800FD864();
            break;
        default:
            return;
        }
        break;
    case 15:
        switch (area) {
        case 0:
            fn_800FD864();
            break;
        case 1:
            fn_800FD864();
            break;
        case 2:
            fn_800FD864();
            break;
        case 3:
            fn_800FD864();
            break;
        case 4:
            fn_800FD864();
            break;
        case 5:
            fn_800FD864();
            break;
        case 6:
            fn_800FD864();
            break;
        case 7:
            fn_800FD864();
            break;
        case 8:
            fn_800FD864();
            break;
        case 9:
            fn_800FD864();
            break;
        default:
            return;
        }
        break;
    case 16:
        switch (area) {
        case 0:
            fn_800FD864();
            break;
        case 1:
            fn_800FD864();
            break;
        case 2:
            fn_800FD864();
            break;
        case 3:
            fn_800FD864();
            break;
        case 4:
            fn_800FD864();
            break;
        case 5:
            fn_800FD864();
            break;
        case 6:
            fn_800FD864();
            break;
        case 7:
            fn_800FD864();
            break;
        case 8:
            fn_800FD864();
            break;
        case 9:
            fn_800FD864();
            break;
        case 10:
            fn_800FD864();
            break;
        default:
            return;
        }
        break;
    case 17:
        switch (area) {
        case 0:
            fn_800FD864();
            break;
        case 1:
            fn_800FD864();
            break;
        case 2:
            fn_800FD864();
            break;
        default:
            return;
        }
        break;
    case 18:
        switch (area) {
        case 0:
            fn_800FD864();
            break;
        case 1:
            fn_800FD864();
            eft028_set(4, 1, area);
            break;
        case 2:
            eft028_set(4, 1, area);
            break;
        case 3:
            fn_800FD864();
            eft028_set(4, 1, area);
            break;
        default:
            return;
        }
        break;
    case 19:
        switch (area) {
        case 1:
            fn_800FD864();
            break;
        default:
            return;
        }
        break;
    case 20:
        switch (area) {
        case 1:
            eft028_set(7, 0, area);
            break;
        default:
            return;
        }
        break;
    case 21:
        switch (area) {
        case 0:
            fn_800FD864();
            break;
        case 1:
            fn_800FD864();
            eft028_set_capsule(area);
            break;
        case 2:
            fn_800FD864();
            break;
        case 3:
            fn_800FD864();
            break;
        case 5:
            fn_800FD864();
            break;
        case 6:
            fn_800FD864();
            break;
        case 7:
            fn_800FD864();
            break;
        default:
            return;
        }
        break;
    case 22:
        switch (area) {
        case 0:
            fn_800FD864();
            break;
        case 1:
            fn_800FD864();
            break;
        case 2:
            fn_800FD864();
            break;
        }
        break;
    }
}

/* 0x800F7778 (0x32C): releases one slot-table record - its registered project, its filed id and its resource
 * memory - and rebuilds it empty; returns whether the resource itself was deleted. */
extern "C" u32 eft_res_release(u8 id) {
    EftProSlot* slot;
    EftControl* ctrl;
    u32 deleted;
    EfResourceManager* resource;
    u32 size;
    u8 shared;

    slot = &((EftProTable*)lbl_806A2D34)->slots_0x04[id];
    ctrl = &eft_control;
    deleted = 0;
    resource = ef_resource_instance();
    if (slot->kind_0x00 == 0) {
        return 0;
    }
    if (slot->kind_0x00 == 2) {
        shared = slot->field_0x05 == 0xEE;
        switch (slot->field_0x01) {
        case 1:
            if (slot->field_0x0C != -1 && nwDelResource(slot->field_0x0C) == 0) {
                if (slot->field_0x10 != 0) {
                    ef_resource_remove_effect_project(resource, (void*)slot->field_0x10);
                }
                if (slot->field_0x05 != 0xFF && slot->field_0x0C == ctrl->id_a_0x3C[slot->field_0x05]) {
                    ctrl->id_a_0x3C[slot->field_0x05] = -1;
                    ctrl->loaded_a_0x30--;
                }
                deleted = 1;
            }
            if (shared == 0) {
                if (slot->field_0x08 != -1) {
                    size = nw_res_mem_size(slot->field_0x08);
                    if (push_res_mem(slot->field_0x08) == 0) {
                        ctrl->bytes_0xC3C -= size;
                    }
                }
            } else {
                ctrl->loaded_a_0x30--;
            }
            lbl_806A2D34[2]--;
            break;
        case 2:
            if (slot->field_0x0C != -1 && nwDelResource(slot->field_0x0C) == 0) {
                if (slot->field_0x14 != 0) {
                    ef_resource_remove_texture_project(resource, (void*)slot->field_0x14);
                }
                if (slot->field_0x05 != 0xFF && slot->field_0x0C == ctrl->id_b_0x43C[slot->field_0x05]) {
                    ctrl->id_b_0x43C[slot->field_0x05] = -1;
                    ctrl->loaded_b_0x34--;
                }
                deleted = 1;
            }
            if (shared == 0) {
                if (slot->field_0x08 != -1) {
                    size = nw_res_mem_size(slot->field_0x08);
                    if (push_res_mem(slot->field_0x08) == 0) {
                        ctrl->bytes_0xC3C -= size;
                    }
                }
            } else {
                ctrl->loaded_b_0x34--;
            }
            lbl_806A2D34[3]--;
            break;
        case 3:
            if (slot->field_0x0C != -1) {
                nw_res_entry_clear(slot->field_0x0C);
                if (slot->field_0x05 != 0xFF && slot->field_0x0C == ctrl->id_c_0x83C[slot->field_0x05]) {
                    ctrl->id_c_0x83C[slot->field_0x05] = -1;
                    ctrl->loaded_c_0x38--;
                }
                deleted = 1;
            }
            if (shared == 0) {
                if (slot->field_0x08 != -1) {
                    size = nw_res_mem_size(slot->field_0x08);
                    if (push_res_mem(slot->field_0x08) == 0) {
                        ctrl->bytes_0xC3C -= size;
                    }
                }
            } else {
                ctrl->loaded_c_0x38--;
            }
            lbl_806A2D34[1]--;
            break;
        }
    }
    slot->kind_0x00 = 0;
    slot->field_0x01 = 0xFF;
    slot->state_0x02 = 0xFF;
    slot->id_0x03 = id;
    slot->field_0x04 = 0xFF;
    slot->field_0x05 = 0xFF;
    slot->field_0x06 = -1;
    slot->field_0x08 = -1;
    slot->field_0x0C = -1;
    slot->field_0x10 = 0;
    slot->field_0x14 = 0;
    lbl_806A2D34[0]--;
    return deleted;
}

/* 0x800F7AA4 - find the handed-out record whose proID is `key`. */
extern "C" EftProSlot* eft_pro_slot_find_proid(u8 key) {
    EftProSlot* slot = eft_pro_slots();

    for (u32 i = 0; i < 0x100; i++, slot++) {
        if (slot->field_0x04 == key && slot->field_0x01 == 1) {
            return slot;
        }
    }
    return NULL;
}

/* 0x800F7BF0 - add the record's resource file to the resource system. */
extern "C" void eft_pro_slot_resource_add(EftProSlot* slot) {
    s32 res = slot->field_0x08;
    char* name;

    if (res != 0x7FFFFFFF && res >= 0 && slot->field_0x05 != 0xFF) {
        name = res_mem_name_get(res);
        if (name != NULL) {
            switch (slot->field_0x01) {
            case 3:
                slot->field_0x0C = nw_res_entry_add(name, NULL);
                break;
            default:
                slot->field_0x0C = nwAddResource(name, NULL);
                break;
            }
        }
    }
}

/* 0x800F7C74 - rebuild the resource walker and (in mode 0) hand every armed slot to eft_pro_slot_resource_add. */
extern "C" void eft_res_bind(u32 mode) {
    s32 i;
    EftControl* ctrl = &eft_control;
    EftProSlot* slot = eft_pro_slots();
    u32 count;

    if ((u8)mode == 0) {
        for (i = 0; i < 0x100; i++, slot++) {
            if (slot->kind_0x00 == 2 && slot->field_0x06 == -1) {
                eft_pro_slot_resource_add(slot);
            }
        }
    }
    ctrl->system_0x04 = (void*)nw_res_ef_system_get();
    ctrl->resource_0x08 = ef_resource_instance();
    count = (u16)ef_resource_num_texture_projects((EfResourceManager*)ctrl->resource_0x08);
    for (i = 0; i < count; i++) {
        ef_resource_texture_project_at((EfResourceManager*)ctrl->resource_0x08, (u16)i);
    }
    if (ctrl->resource_0x08 != NULL) {
        ef_resource_bind_references((EfResourceManager*)ctrl->resource_0x08);
    }
}

/* 0x800F7D44 - the resource-loader callback: file the loaded handle into the mode's id array and update
 * the record's ordinal/flag bytes; a texture project also re-arms the resource walker. */
extern "C" void eft_res_load_callback(u8* name, void* data, EftLoadCtx* unused, EftLoadCtx* ctx) {
    EftControl* ctrl = &eft_control;
    void* work;
    u32 unshared;
    EftProSlot* slot;
    u8* table;

    table = lbl_806A2D34;
    work = ef_resource_instance();
    slot = ctx->slot;
    unshared = ctx->unshared_0x08;

    slot->field_0x10 = 0;
    slot->field_0x14 = 0;
    switch (ctx->mode) {
    case 3:
        slot->field_0x06 = (s16)ctrl->loaded_c_0x38;
        slot->field_0x0C = nw_res_entry_add((char*)name, data);
        ctrl->id_c_0x83C[ctrl->loaded_c_0x38++] = slot->field_0x0C;
        if (unshared == 1) {
            table[1]++;
        } else {
            slot->field_0x05 = table[1]++;
        }
        break;
    case 1:
        slot->field_0x06 = (s16)ctrl->loaded_a_0x30;
        slot->field_0x0C = nwAddResource((char*)name, data);
        ctrl->id_a_0x3C[ctrl->loaded_a_0x30++] = slot->field_0x0C;
        slot->field_0x10 = ef_resource_add_effect_project(work, data);
        if (unshared == 1) {
            table[2]++;
        } else {
            slot->field_0x05 = table[2]++;
        }
        break;
    case 2:
        slot->field_0x06 = (s16)ctrl->loaded_b_0x34;
        slot->field_0x0C = nwAddResource((char*)name, data);
        ctrl->id_b_0x43C[ctrl->loaded_b_0x34++] = slot->field_0x0C;
        if (unshared == 1) {
            table[3]++;
        } else {
            slot->field_0x05 = table[3]++;
        }
        slot->field_0x14 = ef_resource_add_texture_project(work, data);
        eft_res_bind(1);
        break;
    }
}

/* 0x800F7F18 (0x440): requests the load of every file of a descriptor run into the proID slots `ids` names: mode
 * 3 the `efm` models, mode 1 the effect projects, mode 2 the texture projects (mode 0 both of the last two, one
 * descriptor each); an area-tagged request (`flags` low half 2) loads into the area's enemy memory. */
extern "C" void eft_res_files_load(EftResFile* file, s32 mode, u32 flags, u8* ids) {
    EftControl* ctrl = &eft_control;
    u32 count = 0;
    EftProTable* table = (EftProTable*)lbl_806A2D34;
    EftLoadCtx ctx;
    s32 handle;
    void* data;
    u8 area;
    u32 from_area;
    u8 id;

    if ((u16)flags == 2) {
        area = (flags >> 16) & 0xFF;
        from_area = 1;
    } else {
        from_area = 0;
    }
    for (;;) {
        if (mode == 3) {
            if (file->size_0x00 == 0) {
                return;
            }
            if (file->name_0x04 != NULL && ids != NULL) {
                id = ids[(u8)count];
                if (id == 0xFF) {
                    return;
                }
                table->slots_0x04[id].field_0x06 = -1;
                do {
                    if (from_area == 1) {
                        data = em_kind_mem_alloc((u8)area, file->size_0x00);
                        if (data == NULL) {
                            break;
                        }
                        ctx.mode = 3;
                        ctx.slot = &table->slots_0x04[id];
                        ctx.unshared_0x08 = 1;
                        load_file_req(file->name_0x04, (u32)data, file->size_0x00, (u32)eft_res_load_callback, 3, (u32*)&ctx);
                        table->slots_0x04[id].field_0x05 = 0xEE;
                        table->slots_0x04[id].kind_0x00 = 2;
                        table->slots_0x04[id].field_0x01 = 3;
                        table->slots_0x04[id].field_0x08 = flags;
                    } else {
                        handle = res_mem_index_ref(file->name_0x04);
                        if (handle < 0) {
                            handle = pull_res_mem(file->name_0x04, file->size_0x00, 0);
                            if (handle < 0) {
                                break;
                            }
                            ctx.mode = 3;
                            ctx.slot = &table->slots_0x04[id];
                            ctx.unshared_0x08 = 0;
                            load_file_req(file->name_0x04, (u32)getResMemAdrs(handle), file->size_0x00, (u32)eft_res_load_callback,
                                          3, (u32*)&ctx);
                            ctrl->bytes_0xC3C += file->size_0x00;
                        }
                        table->slots_0x04[id].kind_0x00 = 2;
                        table->slots_0x04[id].field_0x01 = 3;
                        table->slots_0x04[id].field_0x08 = handle;
                    }
                    count++;
                    table->used_0x00++;
                } while (0);
            }
            file++;
        } else {
            if ((u32)mode <= 1) {
                if (file->size_0x00 == 0) {
                    return;
                }
                if (file->name_0x04 != NULL && ids != NULL) {
                    id = ids[(u8)count];
                    if (id == 0xFF) {
                        return;
                    }
                    table->slots_0x04[id].field_0x06 = -1;
                    do {
                        if (from_area == 1) {
                            data = em_kind_mem_alloc((u8)area, file->size_0x00);
                            if (data == NULL) {
                                break;
                            }
                            ctx.mode = 1;
                            ctx.slot = &table->slots_0x04[id];
                            ctx.unshared_0x08 = 1;
                            load_file_req(file->name_0x04, (u32)data, file->size_0x00, (u32)eft_res_load_callback, 3, (u32*)&ctx);
                            table->slots_0x04[id].field_0x05 = 0xEE;
                            table->slots_0x04[id].kind_0x00 = 2;
                            table->slots_0x04[id].field_0x01 = 1;
                            table->slots_0x04[id].field_0x08 = flags;
                        } else {
                            handle = res_mem_index_ref(file->name_0x04);
                            if (handle < 0) {
                                handle = pull_res_mem(file->name_0x04, file->size_0x00, 0);
                                if (handle < 0) {
                                    break;
                                }
                                ctx.mode = 1;
                                ctx.slot = &table->slots_0x04[id];
                                ctx.unshared_0x08 = 0;
                                load_file_req(file->name_0x04, (u32)getResMemAdrs(handle), file->size_0x00, (u32)eft_res_load_callback,
                                              3, (u32*)&ctx);
                                ctrl->bytes_0xC3C += file->size_0x00;
                            }
                            table->slots_0x04[id].kind_0x00 = 2;
                            table->slots_0x04[id].field_0x01 = 1;
                            table->slots_0x04[id].field_0x08 = handle;
                        }
                        count++;
                        table->used_0x00++;
                    } while (0);
                }
                file++;
            }
            if (mode == 0 || mode == 2) {
                if (file->size_0x00 == 0) {
                    return;
                }
                if (file->name_0x04 != NULL && ids != NULL) {
                    id = ids[(u8)count];
                    if (id == 0xFF) {
                        return;
                    }
                    table->slots_0x04[id].field_0x06 = -1;
                    do {
                        if (from_area == 1) {
                            data = em_kind_mem_alloc((u8)area, file->size_0x00);
                            if (data == NULL) {
                                break;
                            }
                            ctx.mode = 2;
                            ctx.slot = &table->slots_0x04[id];
                            ctx.unshared_0x08 = 1;
                            load_file_req(file->name_0x04, (u32)data, file->size_0x00, (u32)eft_res_load_callback, 3, (u32*)&ctx);
                            table->slots_0x04[id].field_0x05 = 0xEE;
                            table->slots_0x04[id].kind_0x00 = 2;
                            table->slots_0x04[id].field_0x01 = 2;
                            table->slots_0x04[id].field_0x08 = flags;
                        } else {
                            handle = res_mem_index_ref(file->name_0x04);
                            if (handle < 0) {
                                handle = pull_res_mem(file->name_0x04, file->size_0x00, 0);
                                if (handle < 0) {
                                    break;
                                }
                                ctx.mode = 2;
                                ctx.slot = &table->slots_0x04[id];
                                ctx.unshared_0x08 = 0;
                                load_file_req(file->name_0x04, (u32)getResMemAdrs(handle), file->size_0x00, (u32)eft_res_load_callback,
                                              3, (u32*)&ctx);
                                ctrl->bytes_0xC3C += file->size_0x00;
                            }
                            table->slots_0x04[id].kind_0x00 = 2;
                            table->slots_0x04[id].field_0x01 = 2;
                            table->slots_0x04[id].field_0x08 = handle;
                        }
                        count++;
                        table->used_0x00++;
                    } while (0);
                }
                file++;
            }
        }
    }
}

/* 0x800F8358 (0x2DC): the archive-load callback: files every member of the loaded archive under the proID
 * slot `eft_common_load`'s run reserved for it (searched from the run's first ordinal) and registers it. */
extern "C" void eft_res_archive_load_callback(char* path, void* data, u32 size, EftFileLoadCtx* ctx) {
    EftControl* ctrl = &eft_control;
    u8* table = lbl_806A2D34;
    u8* proids = ctx->proids_0x00;
    EfResourceManager* resource = ef_resource_instance();
    char** names = ctx->names_0x08;
    s32 count = pack_entry_count(data);
    s32 mode = ctx->mode_0x0C;
    EftProSlot* slot;
    void* file;
    s32 i;
    s32 j;
    u16 pos;
    u32 index;

    for (i = 0; i < count; i++) {
        pos = (u16)ctx->first_0x04 + i;
        if (pos > 0xFF) {
            pos = 0;
        }
        for (j = 0; j < 0x100; pos++, j++) {
            index = pos;
            if (index > 0xFF) {
                index = (u16)(index - 0x100);
            }
            slot = (EftProSlot*)(table + index * 0x18 + 4);
            if (slot->field_0x04 == *proids && slot->kind_0x00 == 1) {
                if (mode == 3) {
                    if (mode == slot->field_0x01) {
                        break;
                    }
                } else if (slot->field_0x01 != 3) {
                    break;
                }
            }
        }
        if (j > 0xFF) {
            return;
        }
        file = pack_entry_data(data, i);
        slot->field_0x10 = 0;
        slot->field_0x14 = 0;
        switch (slot->field_0x01) {
        case 3:
            slot->field_0x06 = (s16)ctrl->loaded_c_0x38;
            slot->field_0x0C = nw_res_entry_add(*names, file);
            ctrl->id_c_0x83C[ctrl->loaded_c_0x38++] = slot->field_0x0C;
            slot->field_0x05 = table[1]++;
            break;
        case 1:
            slot->field_0x06 = (s16)ctrl->loaded_a_0x30;
            slot->field_0x0C = nwAddResource(*names, file);
            ctrl->id_a_0x3C[ctrl->loaded_a_0x30++] = slot->field_0x0C;
            slot->field_0x10 = ef_resource_add_effect_project(resource, file);
            slot->field_0x05 = table[2]++;
            break;
        case 2:
            slot->field_0x06 = (s16)ctrl->loaded_b_0x34;
            slot->field_0x0C = nwAddResource(*names, file);
            ctrl->id_b_0x43C[ctrl->loaded_b_0x34++] = slot->field_0x0C;
            slot->field_0x14 = ef_resource_add_texture_project(resource, file);
            slot->field_0x05 = table[3]++;
            eft_res_bind(1);
            break;
        }
        slot->kind_0x00 = 2;
        slot->field_0x08 = 0x7FFFFFFF;
        names++;
        proids++;
    }
}

/* 0x800F8634 - the resource-file request: pull the file into memory and start the async load whose
 * callback is eft_res_archive_load_callback. */
extern "C" void eft_res_file_load(EftResFile* file, u32 mode, u32 reserved, u8* ids, u8* proids, char** names) {
    EftControl* ctrl = &eft_control;
    EftFileLoadCtx ctx;
    s32 handle;

    if ((s32)mode == 3) {
        if (file->size_0x00 == 0) {
            return;
        }
        if (res_mem_index_ref(file->name_0x04) >= 0) {
            return;
        }
        handle = pull_res_mem(file->name_0x04, file->size_0x00, 0);
        if (handle < 0) {
            return;
        }
        ctx.first_0x04 = *ids;
        ctx.proids_0x00 = proids;
        ctx.names_0x08 = names;
        ctx.mode_0x0C = mode;
        load_file_req(file->name_0x04, (u32)getResMemAdrs(handle), (s32)file->size_0x00, (u32)eft_res_archive_load_callback, 4,
                      (u32*)&ctx);
        ctrl->bytes_0xC3C += file->size_0x00;
        return;
    }
    if (mode == 0 || mode == 1) {
        if (file->size_0x00 == 0) {
            return;
        }
        if (res_mem_index_ref(file->name_0x04) >= 0) {
            return;
        }
        handle = pull_res_mem(file->name_0x04, file->size_0x00, 0);
        if (handle < 0) {
            return;
        }
        ctx.first_0x04 = *ids;
        ctx.proids_0x00 = proids;
        ctx.names_0x08 = names;
        ctx.mode_0x0C = mode;
        load_file_req(file->name_0x04, (u32)getResMemAdrs(handle), (s32)file->size_0x00, (u32)eft_res_archive_load_callback, 4,
                      (u32*)&ctx);
        ctrl->bytes_0xC3C += file->size_0x00;
    }
}

/* Takes the first free effect slot and counts it used; NULL when every slot is taken. */
inline EftResSlot* eft_res_slot_find(EftControl* ctrl) {
    EftResSlot* slot = ctrl->slots_0x10;

    for (s32 i = 0; i < (s32)ctrl->slot_count_0x0C; i++, slot++) {
        if (slot->active_0x00 == 0) {
            ctrl->slot_used_0x14++;
            return slot;
        }
    }
    return NULL;
}

/* 0x800F8788 - hand out one of the 0x48-byte effect slots plus its block run. */
extern "C" EftResSlot* eft_res_slot_get(u32 size) {
    EftControl* ctrl = &eft_control;
    EftResSlot* slot;

    if ((s32)ctrl->slot_used_0x14 >= (s32)ctrl->slot_count_0x0C) {
        return NULL;
    }
    slot = eft_res_slot_find(ctrl);
    if (slot == NULL) {
        return NULL;
    }
    slot->active_0x00 = 1;
    slot->field_0x04 = 0;
    slot->field_0x05 = 0;
    slot->field_0x06 = 0;
    slot->field_0x07 = 0;
    slot->field_0x08 = 0;
    slot->field_0x14 = 0;
    slot->field_0x15 = 0;
    slot->field_0x16 = 0;
    slot->field_0x17 = 0;
    if (size != 0) {
        slot->block_count_0x3C = eft_res_block_count(size);
        slot->blocks_0x38 = (void*)eft_res_blocks_get(slot->block_count_0x3C);
        if (slot->blocks_0x38 == NULL) {
            return NULL;
        }
    }
    return slot;
}

/* 0x800F886C - release one effect slot: run its hooks, retire the effect system's emitter and clear it. */
extern "C" void eft_res_slot_release(void* self_) {
    EftResSlot* self = (EftResSlot*)self_;
    EftControl* ctrl = &eft_control;

    if (self->blocks_0x38 != NULL) {
        if (self->release_0x40 != NULL) {
            self->release_0x40(self);
        }
        eft_res_blocks_release(self->blocks_0x38, self->block_count_0x3C);
    }
    if (ctrl->system_0x04 != NULL) {
        ((EftResEffectAllocator*)ef_system_memory_manager(ctrl->system_0x04))->garbage_collection();
    }
    memset(self, 0, 0x48);
    if ((s32)ctrl->slot_used_0x14 > 0) {
        ctrl->slot_used_0x14--;
    }
}

/* 0x800F8914 - hand out one of the 0x168-byte model records. */
extern "C" u8* eft_res_model_get(void) {
    EftControl* ctrl = &eft_control;

    if ((s32)ctrl->model_used_0x20 >= (s32)ctrl->model_count_0x18) {
        return NULL;
    }
    EftResModelSlot* slot = (EftResModelSlot*)ctrl->models_0x1C;
    for (int i = 0; i < (s32)ctrl->model_count_0x18; i++) {
        if (slot->used_0x00 == 0) {
            slot->used_0x00 = 1;
            ctrl->model_used_0x20++;
            mhchar_reset(&slot->model_0x04);
            return (u8*)&slot->model_0x04;
        }
        slot++;
    }
    return NULL;
}

/* 0x800F89A4 - release one model record and the g3d work slot it carries. */
extern "C" void eft_res_model_release(void* self_) {
    EftModel* self = (EftModel*)self_;
    EftControl* ctrl = &eft_control;

    if (self->field_0x10C != NULL) {
        push_g3d_wk((struct _g3d_work*)self->field_0x10C);
        mhchar_reset(self);
        self->field_0x10C = NULL;
    }
    EftResModelSlot* slot = (EftResModelSlot*)ctrl->models_0x1C;
    for (int i = 0; i < (s32)ctrl->model_count_0x18; i++) {
        if ((void*)&slot->model_0x04 == self_) {
            slot->used_0x00 = 0;
            if ((s32)ctrl->model_used_0x20 > 0) {
                ctrl->model_used_0x20--;
            }
            return;
        }
        slot++;
    }
}

/* 0x800F8A44 - release `count` model records off the front of a list. */
extern "C" void eft_res_models_release(void* list, s32 count) {
    void** items = (void**)list;

    for (s32 i = 0; i < count; i++) {
        if (items[i] != NULL) {
            eft_res_model_release(items[i]);
        }
        items[i] = NULL;
    }
}

/* 0x800F8AB8 - retire a run of pooled effects and hand their storage back. */
void push_eft_effect_heap_num(nw4r::ef::Effect** effects, long count) {
    long i;
    nw4r::ef::EffectSystem* system = (nw4r::ef::EffectSystem*)eft_control.system_0x04;

    for (i = 0; i < count; i++) {
        if (effects[i] != NULL) {
            RetireParticleAll(effects[i]);
            effects[i]->RetireEmitterAll();
            if (system != NULL) {
                system->RetireEffect(effects[i]);
            }
        }
        effects[i] = NULL;
    }
}

/* 0x800F8B44 - hand out a run of `count` 0x40-byte blocks from the block heap. */
extern "C" void* eft_res_blocks_get(u32 count) {
    EftControl* ctrl = &eft_control;
    u8* flags = ctrl->heap_flags_0x2C;
    u8* start;
    s32 first;
    s32 run;
    s32 i;
    s32 k;
    s32 max;

    if ((s32)count <= 0) {
        return NULL;
    }
    i = 0;
    max = ctrl->heap_count_0x24;
    for (;;) {
        run = 0;
        while (*flags != 0) {
            flags++;
            i++;
            if (i >= max) {
                return NULL;
            }
        }
        start = flags;
        first = i;
        for (;;) {
            run++;
            if (*flags != 0) {
                break;
            }
            if (run == (s32)count) {
                for (k = 0; k < run; k++) {
                    *start = 1;
                    start++;
                }
                return ctrl->heap_0x28 + (first << 6);
            }
            flags++;
            i++;
            if (i >= max) {
                return NULL;
            }
        }
    }
}

/* 0x800F8C78 - release `count` blocks of the run that starts at `ptr`. */
extern "C" void eft_res_blocks_release(void* ptr, u32 count) {
    EftControl* ctrl = &eft_control;
    u32 index;
    s32 i;
    u8* flags;

    index = (u8*)ptr - ctrl->heap_0x28;
    index >>= 6;
    memset(ctrl->heap_0x28 + index * 0x40, 0, count * 0x40);
    flags = ctrl->heap_flags_0x2C + index;
    for (i = 0; i < (s32)count; i++) {
        *flags = 0;
        flags++;
    }
}

/* 0x800F8D80 - the block count a byte size needs (0x40 bytes per block). */
extern "C" u32 eft_res_block_count(u32 size) {
    u32 blocks = size >> 6;

    if ((size - (blocks << 6)) & 0x3F) {
        blocks++;
    }
    return blocks;
}

/* 0x800F8DA4 - the per-frame walk: retire the ready effect system, then run every slot's hook. */
extern "C" void eft_instances_release(void) {
    EftControl* ctrl = &eft_control;
    void* system;
    s32 i;
    EftResSlot* slot;

    ef_resource_instance();
    system = ctrl->system_0x04;
    if (system == NULL) {
        return;
    }
    fn_800F95A4(system);
    slot = ctrl->slots_0x10;
    ctrl->frame_ready_0xC40 = 0;
    for (i = 0; i < (s32)ctrl->slot_count_0x0C; i++, slot++) {
        if (slot->active_0x00 != 0) {
            slot->update_0x34(slot);
        }
    }
    ((EftResEffectAllocator*)ef_system_memory_manager(system))->garbage_collection();
}

/* 0x800F8E5C - create a model on an actor (no light argument). */
void* res_eft_model_create(MHchar* model, u16 id, u32 arg) {
    return (void*)res_eft_model_create_light(model, id, arg, 0);
}

/* 0x800F8E68 (0xF4): binds the effect model resource `id` (the model-name table entry) to `model` with the light
 * set `light`; returns 1, or 0 without a model, past the table's 0xAD names or when the file is not loaded. */
u32 res_eft_model_create_light(MHchar* model, u16 id, u32 arg, long light) {
    u32 res;
    u32 file;
    ResEntry* entry;

    res_file_ctor(&res, 0);
    if (model == NULL) {
        return 0;
    }
    if (id >= 0xAD) {
        return 0;
    }
    entry = ckResourceName(res_model_name_ptr[id]);
    if (entry == NULL) {
        return 0;
    }
    res_file_assign(&res, res_file_ctor(&file, (u32)entry->data));
    mhchar_reset(model);
    model->g3d_0x10C = NULL;
    mhchar_draw_mode_set(model, 1);
    mhchar_model_create(model, &res, arg, light);
    model->ready = 1;
    return 1;
}

/* 0x800F8F5C (0xAC): binds the UV-animated effect model resource `id` to `model` through
 * `res_eft_UV_model_create_name`; NULL without a model or past the table's 0xAD names. */
/* untyped: opaque handle - the UV animation record the caller stores */
void* res_eft_UV_model_create(MHchar* model, u16 id, u32 arg, long light, struct _g3d_work** list, long count, u8 flag) {
    u32 res;

    res_file_ctor(&res, 0);
    nw4r::g3d::ResMdl mdl((void*)0);
    if (model == NULL) {
        return NULL;
    }
    if (id >= 0xAD) {
        return NULL;
    }
    return res_eft_UV_model_create_name(model, res_model_name_ptr[id], arg, light, list, count, flag);
}

/* 0x800F9008 (0x198): binds the effect model file `name` to `model`, then builds its UV animation over the `count`
 * g3d works of `list` (full strength for flag 1/2, half otherwise); returns the animation, or NULL after releasing
 * the works when the file is not loaded or the works do not fit. */
/* untyped: opaque handle - the UV animation record the caller stores */
void* res_eft_UV_model_create_name(MHchar* model, char* name, u32 arg, long light, struct _g3d_work** list, long count,
                                   u8 flag) {
    EftResFileHandle res;
    ResEntry* entry;
    void* uv;
    f32 strength;
    s32 i;

    res_file_ctor(&res.data_0x00, 0);
    nw4r::g3d::ResMdl mdl((void*)0);
    u32 file;

    entry = ckResourceName(name);
    if (entry == NULL) {
        return NULL;
    }
    res_file_assign(&res.data_0x00, res_file_ctor(&file, (u32)entry->data));
    mhchar_reset(model);
    model->g3d_0x10C = NULL;
    mhchar_draw_mode_set(model, 1);
    mhchar_model_create(model, &res.data_0x00, arg, light);
    model->ready = 1;
    nw4r::g3d::ResMdl scn_mdl = ((nw4r::g3d::ScnMdlSimple*)model->field_0x118)->GetResMdl();
    res_mdl_copy_ctor(&mdl, &scn_mdl);
    if (nw_res_anm_work_num(res, *(EftResFileHandle*)&mdl, 5, list) <= count) {
        if ((u32)(flag - 1) <= 1) {
            strength = 1.0f;
        } else {
            strength = 0.5f;
        }
        uv = nw_res_anm_create(res, strength, *(EftResFileHandle*)&mdl, (nw4r::g3d::ScnMdlSimple*)model->field_0x118, 5, list[0], &list[1]);
        if (uv != NULL) {
            return uv;
        }
    }
    for (i = 0; i < count; i++) {
        if (*list != NULL) {
            push_g3d_wk(*list);
        }
        *list = NULL;
        list++;
    }
    return NULL;
}

/* 0x800F91A0 - the model-name table lookup. */
char* get_eft_res_name(u16 id) {
    u32 index = id & 0xFFFF;

    return res_model_name_ptr[index];
}

/* 0x800F91B4 - create a pooled effect from a proID pair and a resource ordinal. */
nw4r::ef::Effect* res_eft_create(u16 id, u16 kind, unsigned long arg) {
    return (nw4r::ef::Effect*)res_eft_create_ex((u16)id, (u16)kind, arg, 0);
}

/* 0x800F91C4 - resolve one resource and hand it to the effect spawner. */
extern "C" void* res_eft_create_ex(u16 id, u16 kind, u32 arg, u32 count) {
    u8 rec[4];
    u8 copy[4];
    void* system;

    ef_store_word(rec, 0);
    system = eft_control.system_0x04;
    if (fn_800F97F0(id, eft_pro_slot_find_proid((u8)kind), rec) == 0) {
        return NULL;
    }
    if (eft_res_handle_valid(rec) == 0) {
        return NULL;
    }
    return eft_res_effect_spawn(system, eft_res_handle_copy(copy, rec), arg, count & 0xFFFF);
}

/* 0x800F9278 - hand the resolved resource to the effect spawner. */
extern "C" void* eft_res_effect_spawn(void* system, void* res, u32 a, u32 b) {
    return fn_800A5A90(system, fn_800A5484(res), (void*)a, (u16)b);
}

/* 0x800F92D4 - the one-word copy of the resolved handle. */
extern "C" void* eft_res_handle_copy(void* dst, void* src) {
    *(u32*)dst = *(u32*)src;
    return dst;
}

/* 0x800F92E0 - is the resolved handle non-NULL. */
extern "C" u32 eft_res_handle_valid(void* self) {
    u32 v = *(u32*)self;

    return (u32)(-v | v) >> 31;
}

/* 0x800F92F4 - the spawn gate: the effect's pending record must be clear before a spawn. */
extern "C" u32 eft_res_spawn_gate_ck(void* self_, u32 mode) {
    EftSpawnRecord* self = (EftSpawnRecord*)self_;

    switch (mode) {
    case 0:
        if (self->pending_0x30[0] == 0) {
            return 0;
        }
        break;
    case 1:
        if (self->pending_0x30[0] == 0) {
            return 0;
        }
        break;
    case 2:
        if (self->pending_0x30[0] == 0) {
            return 0;
        }
        break;
    case 3:
        if (self->pending_0x30[0] == 0) {
            return 0;
        }
        break;
    }
    return 1;
}

/* 0x800F9380 - map the stage's effect size class to the spawn flag. */
extern "C" u32 eft_stage_spawn_flag(void) {
    u32 size = split_screen_size_class();

    switch (size) {
    case 0x400:
        return 8;
    case 0x800:
        return 0x10;
    case 0x8:
        return 4;
    default:
        return 0;
    }
}

/* 0x800F93D8 - spawn or release the effect's models for the current area and mode bits. */
extern "C" void eft_res_models_spawn(_EFT* self, void** models, s32 mode, s32 count, void* arg) {
    VEC3 pos;
    u16 flags;
    s32 i;

    VEC3_ctor(&pos);
    flags = 0;
    if (self->area_0x44 != get_now_areano() || self->flag_0x01 == 0) {
        if ((mode & 2) == 0) {
            return;
        }
        for (i = 0; i < count; i++) {
            if (((EftModel**)models)[i] != NULL
                && ((EftModel**)models)[i]->field_0x118
                       == ((EftResModelLink*)((EftModel**)models)[i]->field_0x10C)->field_0x04) {
                g3d_root_model_bind(pRoot, ((EftModel**)models)[i]->field_0x118);
            }
        }
        return;
    }
    if (mode & 4) {
        flags |= 8;
    }
    if (mode & 8) {
        flags |= 0x400;
    }
    if (mode & 0x10) {
        flags |= 0x800;
    }
    if (mode & 1) {
        for (i = 0; i < count; i++) {
            if (((EftModel**)models)[i] != NULL) {
                fn_800FA450(((EftModel**)models)[i]);
                sound_job_request(1, 1, (s32)((EftModel**)models)[i], 2, flags, arg, 0);
            }
        }
    }
    if (mode & 2) {
        for (i = 0; i < count; i++) {
            if (((EftModel**)models)[i] != NULL
                && ((EftResModelLink*)((EftModel**)models)[i]->field_0x10C)->field_0x04
                       == ((EftModel**)models)[i]->field_0x118) {
                fn_800FA5D4(self);
                sound_job_request(1, ((EftModel**)models)[i]->field_0x35, (s32)((EftModel**)models)[i]->field_0x118, 0,
                                  flags, arg, 0);
                g3d_root_model_bind(pRoot, ((EftModel**)models)[i]->field_0x118);
            }
        }
    }
}
