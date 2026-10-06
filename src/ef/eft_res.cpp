/* ef/eft_res.cpp - the game's effect-resource manager: `eft_control` and its slot pools, the 256-entry proID slot
 *   table, the resource load/create path and the effect-heap push.
 * RANGE. .text 0x800F6520-0x800F95A4 (43 functions); extab 0x8000BB6C-0x8000BC5C, extabindex 0x8002580C-0x80025974,
 *   .data 0x8059B3B0-0x8059B5D0, .sbss 0x80794A40-0x80794A88, .sdata2 0x807965D0-0x807965D8.  The seam is unproven:
 *   the range's last function drives the same `_EFT` state machine as `ef/effect.cpp`'s first.
 * NAMES. The unit name is a GUESS in the siblings' scheme (no `__FILE__` string is reachable; the runtime dump's
 *   `res_eft_*`, `get_eft_res_name` and `push_eft_effect_heap_num` name the `eft` resource layer).  The dump names
 *   seven functions; `eft_res_slot_get`, `eft_res_slot_release`, `eft_res_model_get`, `eft_res_spawn_gate_ck` and
 *   `eft_res_models_spawn` are GUESSes from their bodies; the map has only `fn_` stems for the rest.
 * RESIDUALS. 9 rows unwritten: 0x800F6710-0x800F6984 (`fn_800F6710`), 0x800F6B6C-0x800F7AA4 (`fn_800F6B6C`,
 *   `fn_800F6DB4`, `fn_800F7778`), 0x800F7F18-0x800F8634 (`fn_800F7F18`, `fn_800F8358`), 0x800F8E68-0x800F91A0
 *   (`res_eft_model_create_light`, `res_eft_UV_model_create`, `res_eft_UV_model_create_name`: they need the
 *   EftModel/g3d mesh chain `fn_8007B878`/`fn_8007BA08`/`fn_8005AB00`/`fn_800D3874`/`fn_800D38F8`).
 *   31 partial rows, including:
 *  - `res_eft_create`, `res_eft_model_create`, `get_eft_res_name`, `fn_800F6AC8`: retail narrows the `u16`/`u8`
 *    arguments (`clrlwi`) at entry, ours does not (or folds it into a `clrlslwi`);
 *  - `fn_800F6AC8`: retail unrolls the 0x18-byte record search three-wide (`li r0,0x33`), ours one-wide with `lbzu`;
 *  - `fn_800F8B44`: retail loads `eft_control`'s pool fields before the loop and walks the flag bytes with a
 *    pointer, ours re-indexes (`lbzx`) and runs a different loop shape.
 *   The other 26 partial rows have no recorded cause (`symdiff.py -u ef/eft_res --all`).
 *   flipcheck: `.data`/`.sbss`/`.sdata2` claimed, not emitted; `.text` (0x13E8 of 0x3084), extab (0xA8 of 0xF0) and
 *   extabindex (0xFC of 0x168) short of the claim and differing.
 * SHAPES. `EftControl* ctrl = &eft_control;` opens every function that touches it, so the address lives in one
 *   callee-saved register (the unhoisted form loses the register on `eft_res_slot_release` and `fn_800F89A4`).
 *   The slot table is addressed two ways, both needed: `lbl_806A2D34 + i*0x18` with the fields at +0x04 (`fn_800F6A78`,
 *   whose `EftProHdrSlot` pads the 4-byte header) and `(lbl_806A2D34 + 4) + i*0x18` with the fields at +0x00 (the
 *   searchers `fn_800F6AC8`/`fn_800F7AA4` and the loaders `fn_800F8358`/`fn_800F7D44`).
 *   `load_file_req`'s arguments are (name, data, size, callback, 4, ctx): the callback is the fourth.
 *   `fn_800F8C78`'s difference is unsigned (`(u32)(ptr - heap) >> 6`): the signed form emits `srawi` and a `clrrwi`.
 */

#include "types.h"
#include "nw4r/math.h"
#include "ef.h"
#include "pl.h"
#include "gx.h"
#include "unsplit/g3d.h"
#include "Runtime.PPCEABI.H/memset.h"
#include "ef/fn_800AEE48.h"
#include "ef/fn_800CDB2C.h"
#include "nw_resource.h"
#include "sound/fn_800DD1F0.h"
#include "ef/pRoot.h"
#include "ef/fn_800D3C0C.h"

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
    /* +0x34 */ void (*update_0x34)(void*);   /* the per-frame hook fn_800F8DA4 runs */
    /* +0x38 */ void* blocks_0x38;            /* the 0x40-byte block run this slot owns */
    /* +0x3C */ u32 block_count_0x3C;         /* how many of them */
    /* +0x40 */ void (*release_0x40)(void*);  /* the release hook eft_res_slot_release runs first */
    /* +0x44 */ u32 field_0x44;
} EftResSlot; /* size: 0x48 */

/* The manager's control block `eft_control` (`EftControl`, 0xC44 B of `.bss`) is the band's one record (`unsplit/ef_control.h`); this unit is its main reader. */
#include "unsplit/ef_control.h"

/* The record eft_res_spawn_gate_ck gates a spawn on: a pointer at +0x30 whose byte 0 must be clear when one is
 * pending.  Only that pointer is touched by this unit. */
typedef struct EftSpawnRecord {
    /* +0x00 */ u8 pad_0x00[0x30];
    /* +0x30 */ u8* pending_0x30;
} EftSpawnRecord; /* size: 0x34 (lower bound: only +0x30 is read by this unit) */

/* The 256-entry slot table that follows `eft_control` in `.bss`: a 4-byte header, then 0x18-byte records
 * whose proID byte `fn_800F6AC8` looks up (`fn_800F6A78` builds one, `fn_800F7778` releases it).
 * Every function reaches it through the map's own absolute label, never as an offset of `eft_control`. */
#include "unsplit/ef_tables.h" /* lbl_806A2D34 (rule 2: the band) */

typedef struct EftProSlot {
    /* +0x00 */ u8 kind_0x00;    /* fn_800F6AC8's key; 2 = a registered effect (fn_800F7778) */
    /* +0x01 */ u8 field_0x01;   /* 0xff; the "handed out" flag fn_800F7AA4 tests for 1 */
    /* +0x02 */ u8 state_0x02;   /* 0xff; 1 while the resource is live */
    /* +0x03 */ u8 id_0x03;      /* the slot ordinal the effect's resources are filed under */
    /* +0x04 */ u8 field_0x04;   /* the proID fn_800F7AA4 matches */
    /* +0x05 */ u8 field_0x05;   /* 0xff, or the 1-based ordinal of the resource in its file */
    /* +0x06 */ s16 field_0x06;  /* -1, or the loaded resource's size */
    /* +0x08 */ s32 field_0x08;  /* -1 from fn_800F6A78, 0x7FFFFFFF from fn_800F8358 */
    /* +0x0C */ s32 field_0x0C;  /* the loaded resource id */
    /* +0x10 */ s32 field_0x10;
    /* +0x14 */ s32 field_0x14;
} EftProSlot; /* size: 0x18 */

/* One entry of the resource-file descriptor array the loaders walk: a byte size and the file name. */
typedef struct EftResFile {
    /* +0x00 */ u32 size_0x00;
    /* +0x04 */ char* name_0x04;
} EftResFile; /* size: 0x08 */

/* The pool record the 0x168-byte model arrays hold: a used byte, then the EftModel `eft_res_model_get`
 * hands out (the copy `res_eft_model_create_light` fills). */
typedef struct EftResModelSlot {
    /* +0x000 */ u8 used_0x00;
    /* +0x004 */ EftModel model_0x04;
} EftResModelSlot; /* size: 0x168 (the remainder past +0x120 is untouched by this unit) */

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

/* The same 0x18-byte record as the two functions that take the table's 4-byte header as part of the
 * record's address (fn_800F6A78's stores land on `+0x4..+0x18` of the header-based record). */
typedef struct EftProHdrSlot {
    /* +0x00 */ u8 pad_0x00[0x4];
    /* +0x04 */ u8 kind_0x04;
    /* +0x05 */ u8 field_0x05;
    /* +0x06 */ u8 state_0x06;
    /* +0x07 */ u8 id_0x07;
    /* +0x08 */ u8 field_0x08;
    /* +0x09 */ u8 field_0x09;
    /* +0x0A */ s16 field_0x0A;
    /* +0x0C */ s32 field_0x0C;
    /* +0x10 */ s32 field_0x10;
    /* +0x14 */ s32 field_0x14;
    /* +0x18 */ s32 field_0x18;
} EftProHdrSlot; /* size: 0x1C (the last field is the next record's +0x00) */

/* The records start 4 bytes into the table (its 4-byte header is zeroed by fn_800F6520 and counted down
 * by fn_800F7778). */
inline EftProSlot* eft_pro_slots(void) {
    return (EftProSlot*)(lbl_806A2D34 + 4);
}

/* ---------------------------------------------------------------------------------------------------
 * this unit's own entry points (`ef/eft_res.h` declares the ones other units call)
 * ------------------------------------------------------------------------------------------------- */

#ifdef __cplusplus
extern "C" {
#endif

void fn_800F6520(void);
void fn_800F65B4(void);
void fn_800F6688(void);
void fn_800F6984(u32, u32, void*, void*);
void fn_800F69A0(u32, void*, void*);
void fn_800F69B8(void);
void fn_800F6A24(void);
void fn_800F6A78(u32);
EftProSlot* fn_800F6AC8(u32);
void fn_800F7778(u8);
EftProSlot* fn_800F7AA4(u32);
void fn_800F7BF0(EftProSlot*);
void fn_800F7C74(u32);
void fn_800F7D44(u8*, void*, EftLoadCtx*, EftLoadCtx*);
void fn_800F7F18(void*, u32, u32, void*);
void fn_800F8358(void*, void*, void*, void*);
void fn_800F8634(EftResFile*, u32, void*, u8*, u32, u32);
EftResSlot* eft_res_slot_get(u32);
void eft_res_slot_release(void*);
u8* eft_res_model_get(void);
void fn_800F89A4(void*);
void fn_800F8A44(void*, s32);
void* fn_800F8B44(u32);
void fn_800F8C78(void*, u32);
u32 fn_800F8D80(u32);
void fn_800F8DA4(void);
void* fn_800F91C4(u16, u16, u32, u32);
void* fn_800F9278(void*, void*, u32, u32);
void* fn_800F92D4(void*, void*);
u32 fn_800F92E0(void*);
u32 eft_res_spawn_gate_ck(void*, u32);
u32 fn_800F9380(void);
void eft_res_models_spawn(_EFT*, void**, s32, s32, void*);
void fn_800F95A4(void*);
u32 fn_800F97F0(u16, void*, void*);
void fn_800FA450(void*);
void fn_800FA5D4(_EFT*);
void fn_800FD864(void);
void sound_job_request(s32, u8, s32, s32, s32, void*, s32);
void mhchar_reset(void*);
void fn_800E0BE8(void*, s32);
void fn_800E2228(void*, void*, void*, s32);
void res_file_ctor(void*, s32);
void res_file_assign(void*, void*);
void fn_8007B878(void*, s32);
void fn_8007BA08(void*, void*);
extern s32 nwDelResource(s32);
extern s32 fn_800A4420(void*);
extern void fn_800A8998(void*, u32);
extern void* fn_800A5484(void*);
extern void* fn_800A5A90(void*, void*, void*, u32);
extern void* res_model_name_ptr;

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

void* res_eft_model_create_light(MHchar* model, u16 id, u32 arg, long light);

/* ---------------------------------------------------------------------------------------------------
 * the functions, in address order
 * ------------------------------------------------------------------------------------------------- */

/* 0x800F6520 - zero the control block, then build all 256 slot-table records. */
extern "C" void fn_800F6520(void) {
    memset(&eft_control, 0, sizeof(eft_control));
    lbl_806A2D34[0] = 0;
    lbl_806A2D34[1] = 0;
    lbl_806A2D34[2] = 0;
    lbl_806A2D34[3] = 0;

    for (u32 i = 0; i < 0x100; i++) {
        eft_control.id_a_0x3C[i] = -1;
        eft_control.id_b_0x43C[i] = -1;
        eft_control.id_c_0x83C[i] = -1;
        fn_800F6A78((u8)i);
    }
}

/* 0x800F65B4 - allocate the three work pools (0x48-byte effect slots, 0x168-byte model records and the
 * 0x40-byte block heap with its trailing flag run) and mark the manager initialised. */
extern "C" void fn_800F65B4(void) {
    u8* heap;
    u32 count;

    fn_800F7C74(0);

    eft_control.slots_0x10 = (EftResSlot*)get_move_work_adrs(4);
    eft_control.slot_count_0x0C = get_move_work_max(4);
    memset(eft_control.slots_0x10, 0, eft_control.slot_count_0x0C * 0x48);

    eft_control.models_0x1C = (u8*)get_move_work_adrs(5);
    eft_control.model_count_0x18 = get_move_work_max(5);
    memset(eft_control.models_0x1C, 0, eft_control.model_count_0x18 * 0x168);

    heap = (u8*)get_move_work_adrs(6);
    eft_control.heap_0x28 = heap;
    count = get_move_work_max(6);
    eft_control.heap_count_0x24 = count;
    eft_control.heap_flags_0x2C = heap + count * 0x40;
    memset(heap, 0, count + count * 0x40);

    eft_control.initialised_0x00 = 1;
}

/* 0x800F6688 - run the teardown over every live effect slot. */
extern "C" void fn_800F6688(void) {
    EftResSlot* slot = eft_control.slots_0x10;

    if (slot == NULL) {
        return;
    }
    for (u32 i = 0; i < eft_control.slot_count_0x0C; i++) {
        if (slot[i].active_0x00 != 0 && (slot[i].field_0x04 & 1) == 0) {
            eft_res_slot_release(&slot[i]);
        }
    }
}

/* 0x800F6984 - load one resource set with the 0x20000 flag family. */
extern "C" void fn_800F6984(u32 a, u32 b, void* desc, void* table) {
    u32 tag = a & 0xFF;
    tag = tag << 0x10;
    fn_800F7F18(desc, (u8)b, tag | 2, table);
}

/* 0x800F69A0 - load one resource set with flag word 1. */
extern "C" void fn_800F69A0(u32 a, void* desc, void* table) {
    fn_800F7F18(desc, (u8)a, 1, table);
}

/* 0x800F69B8 - retire every slot whose kind/state pair says its resource list is live. */
extern "C" void fn_800F69B8(void) {
    EftProSlot* slot = &eft_pro_slots()[0x100 - 1];

    for (s32 i = 0xFF; i >= 0; i--) {
        if (slot->kind_0x00 == 2 && slot->state_0x02 == 1) {
            fn_800F7778(slot->id_0x03);
        }
        slot--;
    }
}

/* 0x800F6A24 - retire every slot of the table. */
extern "C" void fn_800F6A24(void) {
    EftProSlot* slot = &eft_pro_slots()[0x100 - 1];

    for (s32 i = 0xFF; i >= 0; i--) {
        fn_800F7778(slot->id_0x03);
        slot--;
    }
}

/* 0x800F6A78 - build one slot-table record (the id fn_800F6520 files it under). */
extern "C" void fn_800F6A78(u32 id) {
    EftProHdrSlot* slot = (EftProHdrSlot*)(lbl_806A2D34 + (id & 0xFF) * 0x18);

    slot->kind_0x04 = 0;
    slot->field_0x05 = 0xFF;
    slot->state_0x06 = 0xFF;
    slot->id_0x07 = id;
    slot->field_0x08 = 0xFF;
    slot->field_0x09 = 0xFF;
    slot->field_0x0A = -1;
    slot->field_0x0C = -1;
    slot->field_0x10 = -1;
    slot->field_0x14 = 0;
    slot->field_0x18 = 0;
}

/* 0x800F6AC8 - find the first of the table's 0x33 records whose key byte is `key`. */
extern "C" EftProSlot* fn_800F6AC8(u32 key) {
    EftProSlot* slot = eft_pro_slots();

    for (u32 i = 0; i < 0x33; i++) {
        if (slot->kind_0x00 == key) {
            return slot;
        }
        slot++;
    }
    return NULL;
}

/* 0x800F7AA4 - find the handed-out record whose proID is `key`. */
extern "C" EftProSlot* fn_800F7AA4(u32 key) {
    EftProSlot* slot = eft_pro_slots();

    for (u32 i = 0; i < 0x100; i++) {
        if (slot->field_0x04 == key && slot->field_0x01 == 1) {
            return slot;
        }
        slot++;
    }
    return NULL;
}

/* 0x800F7BF0 - add the record's resource file to the resource system. */
extern "C" void fn_800F7BF0(EftProSlot* slot) {
    s32 res = slot->field_0x08;

    if (res == 0x7FFFFFFF || res < 0 || slot->field_0x05 == 0xFF) {
        return;
    }
    if (fn_800D4D9C(res) == 0) {
        return;
    }
    if (slot->field_0x01 == 3) {
        slot->field_0x0C = fn_800D56F4(NULL, NULL);
    } else {
        slot->field_0x0C = nwAddResource(NULL, NULL);
    }
}

/* 0x800F7C74 - rebuild the resource walker and (in mode 0) hand every armed slot to fn_800F7BF0. */
extern "C" void fn_800F7C74(u32 mode) {
    EftControl* ctrl = &eft_control;

    if ((u8)mode == 0) {
        EftProSlot* slot = eft_pro_slots();
        for (int i = 0; i < 0x100; i++) {
            if (slot->kind_0x00 == 2 && slot->field_0x06 == -1) {
                fn_800F7BF0(slot);
            }
            slot++;
        }
    }
    ctrl->system_0x04 = (void*)fn_800D3C0C();
    ctrl->resource_0x08 = fn_800B2878();
    u16 count = (u16)fn_800B4A90((EfPostField*)ctrl->resource_0x08);
    for (int i = 0; i < count; i++) {
        fn_800B4A98((EfPostField*)ctrl->resource_0x08, (u16)i);
    }
    if (ctrl->resource_0x08 != NULL) {
        fn_800B44F4(ctrl->resource_0x08);
    }
}

/* 0x800F7D44 - the resource-loader callback: file the loaded handle into the mode's id array and update
 * the record's ordinal/flag bytes, then re-arm the resource walker. */
extern "C" void fn_800F7D44(u8* name, void* data, EftLoadCtx* unused, EftLoadCtx* ctx) {
    EftControl* ctrl = &eft_control;
    u8* table = lbl_806A2D34;
    void* work = fn_800B2878();
    EftProSlot* slot = ctx->slot;
    u32 unshared = ctx->unshared_0x08;

    slot->field_0x10 = 0;
    slot->field_0x14 = 0;
    switch (ctx->mode) {
    case 3:
        slot->field_0x06 = (s16)ctrl->loaded_c_0x38;
        slot->field_0x0C = fn_800D56F4(name, data);
        ctrl->id_c_0x83C[ctrl->loaded_c_0x38] = slot->field_0x0C;
        ctrl->loaded_c_0x38++;
        if (unshared == 1) {
            table[1]++;
        } else {
            slot->field_0x05 = table[1];
            table[1]++;
        }
        break;
    case 1:
        slot->field_0x06 = (s16)ctrl->loaded_a_0x30;
        slot->field_0x0C = nwAddResource((char*)name, data);
        ctrl->id_a_0x3C[ctrl->loaded_a_0x30] = slot->field_0x0C;
        ctrl->loaded_a_0x30++;
        slot->field_0x10 = fn_800B3670(work, data);
        if (unshared == 1) {
            table[2]++;
        } else {
            slot->field_0x05 = table[2];
            table[2]++;
        }
        break;
    case 2:
        slot->field_0x06 = (s16)ctrl->loaded_b_0x34;
        slot->field_0x0C = nwAddResource((char*)name, data);
        ctrl->id_b_0x43C[ctrl->loaded_b_0x34] = slot->field_0x0C;
        ctrl->loaded_b_0x34++;
        if (unshared == 1) {
            table[3]++;
        } else {
            slot->field_0x05 = table[3];
            table[3]++;
        }
        slot->field_0x14 = fn_800B3E80(work, data);
        break;
    }
    fn_800F7C74(1);
}

/* 0x800F8634 - the resource-file request: pull the file into memory and start the async load whose
 * callback is fn_800F8358. */
extern "C" void fn_800F8634(EftResFile* desc, u32 mode, void* unused, u8* name, u32 a7, u32 a8) {
    EftControl* ctrl = &eft_control;
    u32 ctx[6];

    if (mode == 3) {
        if (desc->size_0x00 == 0) {
            return;
        }
        if (fn_800D4CE4(desc->name_0x04) >= 0) {
            return;
        }
        s32 handle = pull_res_mem(desc->name_0x04, desc->size_0x00, 0);
        if (handle < 0) {
            return;
        }
        ctx[0] = a7;
        ctx[1] = *name;
        ctx[2] = a8;
        ctx[3] = mode;
        load_file_req(desc->name_0x04, (u32)getResMemAdrs(handle), (s32)desc->size_0x00, (u32)fn_800F8358, 4, ctx);
        eft_control.bytes_0xC3C += *(u32*)desc;
        return;
    }
    if (mode > 1) {
        return;
    }
    if (desc->size_0x00 == 0) {
        return;
    }
    if (fn_800D4CE4(desc->name_0x04) >= 0) {
        return;
    }
    s32 handle = pull_res_mem(desc->name_0x04, desc->size_0x00, 0);
    if (handle < 0) {
        return;
    }
    ctx[0] = a7;
    ctx[1] = *name;
    ctx[2] = a8;
    ctx[3] = mode;
    load_file_req(desc->name_0x04, (u32)getResMemAdrs(handle), (s32)desc->size_0x00, (u32)fn_800F8358, 4, ctx);
    eft_control.bytes_0xC3C += *(u32*)desc;
}

/* 0x800F8788 - hand out one of the 0x48-byte effect slots plus its block run. */
extern "C" EftResSlot* eft_res_slot_get(u32 size) {
    EftResSlot* found;

    if (eft_control.slot_used_0x14 >= eft_control.slot_count_0x0C) {
        return NULL;
    }
    EftResSlot* slot = eft_control.slots_0x10;
    for (u32 i = 0; i < eft_control.slot_count_0x0C; i++) {
        if (slot->active_0x00 == 0) {
            eft_control.slot_used_0x14++;
            found = slot;
            break;
        }
        slot++;
    }
    if (slot->active_0x00 != 0) {
        return NULL;
    }
    found->active_0x00 = 1;
    found->field_0x04 = 0;
    found->field_0x05 = 0;
    found->field_0x06 = 0;
    found->field_0x07 = 0;
    found->field_0x08 = 0;
    found->field_0x14 = 0;
    found->field_0x15 = 0;
    found->field_0x16 = 0;
    found->field_0x17 = 0;
    if (size != 0) {
        found->block_count_0x3C = fn_800F8D80(size);
        found->blocks_0x38 = (void*)fn_800F8B44(found->block_count_0x3C);
        if (found->blocks_0x38 == NULL) {
            return NULL;
        }
    }
    return found;
}

/* 0x800F886C - release one effect slot: run its hooks, retire the effect system's emitter and clear it. */
extern "C" void eft_res_slot_release(void* self_) {
    EftResSlot* self = (EftResSlot*)self_;
    EftControl* ctrl = &eft_control;

    if (self->blocks_0x38 != NULL) {
        if (self->release_0x40 != NULL) {
            self->release_0x40(self);
        }
        fn_800F8C78(self->blocks_0x38, self->block_count_0x3C);
    }
    if (ctrl->system_0x04 != NULL) {
        void* obj = (void*)fn_800A4420(ctrl->system_0x04);
        void** vt = (void**)obj;
        ((void (*)(void*))vt[3])(obj);
    }
    memset(self, 0, 0x48);
    if ((s32)ctrl->slot_used_0x14 > 0) {
        ctrl->slot_used_0x14--;
    }
}

/* 0x800F8914 - hand out one of the 0x168-byte model records. */
extern "C" u8* eft_res_model_get(void) {
    EftControl* ctrl = &eft_control;

    if (ctrl->model_used_0x20 >= ctrl->model_count_0x18) {
        return NULL;
    }
    EftResModelSlot* slot = (EftResModelSlot*)ctrl->models_0x1C;
    for (int i = 0; i < ctrl->model_count_0x18; i++) {
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
extern "C" void fn_800F89A4(void* self_) {
    EftModel* self = (EftModel*)self_;
    EftControl* ctrl = &eft_control;

    if (self->field_0x10C != NULL) {
        push_g3d_wk((struct _g3d_work*)self->field_0x10C);
        mhchar_reset(self);
        self->field_0x10C = NULL;
    }
    EftResModelSlot* slot = (EftResModelSlot*)ctrl->models_0x1C;
    for (int i = 0; i < ctrl->model_count_0x18; i++) {
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
extern "C" void fn_800F8A44(void* list, s32 count) {
    void** items = (void**)list;

    for (s32 i = 0; i < count; i++) {
        if (items[0] != NULL) {
            fn_800F89A4(items[0]);
        }
        items[0] = NULL;
        items++;
    }
}

/* 0x800F8AB8 - retire a run of pooled effects and hand their storage back. */
void push_eft_effect_heap_num(nw4r::ef::Effect** effects, long count) {
    nw4r::ef::EffectSystem* system = (nw4r::ef::EffectSystem*)eft_control.system_0x04;

    for (long i = 0; i < count; i++) {
        if (effects[i] != NULL) {
            effects[i]->RetireEmitterAll();
            if (system != NULL) {
                system->RetireEffect(effects[i]);
            }
            effects[i] = NULL;
        }
    }
}

/* 0x800F8B44 - hand out a run of `count` 0x40-byte blocks from the block heap. */
extern "C" void* fn_800F8B44(u32 count) {
    u8* flags;
    u8* start;
    u32 pos;
    u32 run;

    if ((s32)count <= 0) {
        return NULL;
    }
    flags = eft_control.heap_flags_0x2C;
    pos = 0;
    while (pos < eft_control.heap_count_0x24) {
        if (flags[pos] != 0) {
            pos++;
            continue;
        }
        start = flags + pos;
        run = 0;
        while (pos < eft_control.heap_count_0x24 && flags[pos] == 0) {
            run++;
            if (run == count) {
                u8* mark = start;
                for (u32 k = 0; k < run; k++) {
                    *mark = 1;
                    mark++;
                }
                return eft_control.heap_0x28 + ((u32)(start - flags) << 6);
            }
            pos++;
        }
    }
    return NULL;
}

/* 0x800F8C78 - release `count` blocks of the run that starts at `ptr`. */
extern "C" void fn_800F8C78(void* ptr, u32 count) {
    u32 index = (u32)((u8*)ptr - eft_control.heap_0x28) >> 6;

    memset(eft_control.heap_0x28 + index * 0x40, 0, count * 0x40);
    u8* flags = eft_control.heap_flags_0x2C + index;
    for (u32 i = 0; i < count; i++) {
        *flags = 0;
        flags++;
    }
}

/* 0x800F8D80 - the block count a byte size needs (0x40 bytes per block). */
extern "C" u32 fn_800F8D80(u32 size) {
    u32 blocks = size >> 6;

    if ((size - (blocks << 6)) & 0x3F) {
        blocks++;
    }
    return blocks;
}

/* 0x800F8DA4 - the per-frame walk: retire the ready effect system, then run every slot's hook. */
extern "C" void fn_800F8DA4(void) {
    EftResSlot* slot;

    fn_800B2878();
    void* system = eft_control.system_0x04;
    if (system == NULL) {
        return;
    }
    fn_800F95A4(system);
    slot = eft_control.slots_0x10;
    eft_control.frame_ready_0xC40 = 0;
    for (u32 i = 0; i < eft_control.slot_count_0x0C; i++) {
        if (slot[i].active_0x00 != 0) {
            slot[i].update_0x34(&slot[i]);
        }
    }
    void* obj = (void*)fn_800A4420(system);
    void** vt = (void**)obj;
    ((void (*)(void*))vt[3])(obj);
}

/* 0x800F8E5C - create a model on an actor (no light argument). */
void* res_eft_model_create(MHchar* model, u16 id, u32 arg) {
    return res_eft_model_create_light(model, id, arg, 0);
}

/* 0x800F91A0 - the model-name table lookup. */
char* get_eft_res_name(u16 id) {
    u32 index = id & 0xFFFF;

    return ((char**)res_model_name_ptr)[index];
}

/* 0x800F91B4 - create a pooled effect from a proID pair and a resource ordinal. */
nw4r::ef::Effect* res_eft_create(u16 id, u16 kind, unsigned long arg) {
    return (nw4r::ef::Effect*)fn_800F91C4((u16)id, (u16)kind, arg, 0);
}

/* 0x800F91C4 - resolve one resource and hand it to the effect spawner. */
extern "C" void* fn_800F91C4(u16 id, u16 kind, u32 arg, u32 count) {
    u8 rec[8];
    u8 copy[4];
    void* system;

    fn_800A8998(rec, 0);
    system = eft_control.system_0x04;
    if (fn_800F97F0(id, fn_800F7AA4((u8)kind), rec) == 0) {
        return NULL;
    }
    if (fn_800F92E0(rec) == 0) {
        return NULL;
    }
    return fn_800F9278(system, fn_800F92D4(copy, rec), arg, count & 0xFFFF);
}

/* 0x800F9278 - hand the resolved resource to the effect spawner. */
extern "C" void* fn_800F9278(void* system, void* res, u32 a, u32 b) {
    return fn_800A5A90(system, fn_800A5484(res), (void*)a, (u16)b);
}

/* 0x800F92D4 - the one-word copy of the resolved handle. */
extern "C" void* fn_800F92D4(void* dst, void* src) {
    *(u32*)dst = *(u32*)src;
    return dst;
}

/* 0x800F92E0 - is the resolved handle non-NULL. */
extern "C" u32 fn_800F92E0(void* self) {
    u32 v = *(u32*)self;

    return (u32)(-v | v) >> 31;
}

/* 0x800F92F4 - the spawn gate: the effect's pending record must be clear before a spawn. */
extern "C" u32 eft_res_spawn_gate_ck(void* self_, u32 mode) {
    EftSpawnRecord* self = (EftSpawnRecord*)self_;

    switch (mode) {
    case 0:
        if (self->pending_0x30[0] != 0) {
            return 1;
        }
        return 0;
    case 1:
        if (self->pending_0x30[0] != 0) {
            return 1;
        }
        return 0;
    case 2:
        if (self->pending_0x30[0] != 0) {
            return 1;
        }
        return 0;
    case 3:
        if (self->pending_0x30[0] != 0) {
            return 1;
        }
        return 0;
    }
    return 1;
}

/* 0x800F9380 - map the stage's effect size class to the spawn flag. */
extern "C" u32 fn_800F9380(void) {
    u32 size = fn_800E3B3C();

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
    u16 flags = 0;

    VEC3_ctor(&pos);
    if (self->area_0x44 != get_now_areano() || self->flag_0x01 != 0) {
        if ((mode & 2) == 0) {
            return;
        }
        for (s32 i = 0; i < count; i++) {
            EftModel* model = (EftModel*)models[i];
            if (model != NULL
                && model->field_0x118 == ((EftResModelLink*)model->field_0x10C)->field_0x04) {
                g3d_root_model_bind(pRoot, model->field_0x118);
            }
        }
        return;    }
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
        for (s32 i = 0; i < count; i++) {
            EftModel* model = (EftModel*)models[i];
            if (model != NULL) {
                fn_800FA450(model);
                sound_job_request(1, 1, (s32)model, 2, flags, arg, 0);
            }
        }
    }
    if (mode & 2) {
        for (s32 i = 0; i < count; i++) {
            EftModel* model = (EftModel*)models[i];
            if (model != NULL
                && model->field_0x118 == ((EftResModelLink*)model->field_0x10C)->field_0x04) {
                fn_800FA5D4(self);
                sound_job_request(1, model->field_0x35, (s32)model->field_0x118, 0, flags, arg, 0);
                g3d_root_model_bind(pRoot, model->field_0x118);
            }
        }
    }
}
