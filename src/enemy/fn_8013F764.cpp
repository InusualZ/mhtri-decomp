/* enemy/fn_8013F764.cpp - the enemy program interpreter's second half: the run driver, the stream
 * readers and the command-length/stream-walk helpers, `.text` 0x8013F764..0x801411B8 (45 functions).
 * Naming note: the symbol map has only fn_XXXXXXXX for this range (checked with
 * `python tools/symbols/symedit.py range 0x8013F764 0x801411B8`: every one of the 45 rows is a bare
 * `fn_XXXXXXXX = .text:0x...` entry with no real name).
 *
 * What it is.  The unit directly below it, `enemy/fn_8013BE60.c` (0x8013BE60..0x8013F764), is "the
 * enemy parameter interpreter and its handler table": `fn_8013BE60(self, in, id, sub, value)` applies
 * one parameter record and `fn_8013C244(self, in, id)` is its two-argument tail.  This unit is the
 * rest of that same interpreter:
 *
 *   * the run driver `fn_8013F764` - it walks the enemy's program table (`prog_0xA00`, 3-byte entries
 *     `EmProgTbl`), latches the entry through `fn_801262BC`, measures the enemy against its target
 *     position `vec_0x36C` (`fn_80050F80`) and dispatches on the two entry bytes through
 *     `fn_8013C36C`; `fn_8013F994` is its "next entry" step.
 *   * the stream readers - the long family `fn_8013F8D8`/`fn_8013F9D8`/`fn_8013FB9C`/`fn_8013FC60`/
 *     `fn_8013FD98`/`fn_8013FF5C`/`fn_8014001C`/`fn_801400BC`/`fn_80140178`/`fn_80140298`/
 *     `fn_80140354`/`fn_801404B8`/`fn_80140580`: same shape as the neighbours' readers - `switch
 *     (*in)` over 0/2/255, `in += fn_801406E0(code, *in)` then `fn_8013BDE4(&in, code, &value)` for
 *     command 0, `in += ...; value = fn_80140778(in, code, 1)` for command 2, `fn_80140AF8(self, 0,
 *     0)` for anything else, and an `s16` back through the frame slot.  The command ids run 0x4E
 *     (fn_8013F8D8) to 0x6A (fn_80140648) in the order the target dispatches them.
 *   * the one-line entries - `fn_8013FD1C`/`fn_801400B0` (field setters), `fn_8013FEF4`,
 *     `fn_8013FD28`/`fn_8013FD50`/`fn_8013FD74`/`fn_8013FF08`/`fn_8013FF38`/`fn_80140244`/
 *     `fn_8014026C`/`fn_80140648`/`fn_80140670` (`if (*in == 0) fn_8013BE60(self, in, id, sub, v);
 *     else fn_8013C244(self, in, id);`, the same two-argument family the neighbour carries).
 *   * the stream helpers themselves: `fn_801406E0` (the command-length table `lbl_805A1530`),
 *     `fn_80140768` (its two-byte form), `fn_80140778` (the recursive `{code, sub}` walker that
 *     yields the offset the readers want), `fn_80140AF8`/`fn_80140B0C`/`fn_80140B10` (the trace
 *     helpers retail compiled to nothing), `fn_801408B4` (the interpreter's save/restore stack),
 *     `fn_801409C8` (the action-timeout setup), `fn_80140B20` (the program-table pointer arm),
 *     `fn_80140C00` (the per-enemy extra-table lookup) and the enemy file loader
 *     `fn_80140CA0`/`fn_80140DAC`/`fn_80140E48`/`fn_80140EE8`/`fn_80140FB0`/`fn_80141050` that
 *     `load_file_req` drives.
 *
 * Naming evidence (brief section 2, in order).  1. No `__FILE__` string is reachable from this range:
 * the only absolute addresses its code builds are `lbl_805A1530` (the 0x6D-byte command-length
 * table), `lbl_805A1B34` (the `emNNN_prog_tbl` pointer array), `lbl_805A1B08`, the two file tables
 * `lbl_80581E20`/`lbl_80582348` and the `.sdata2` float pool (`lbl_80796DBC`/`DC0`/`DC4`) - all data,
 * no source-file name.  2. `python tools/symbols/dumpmap.py lookup` answers `zz_013f764_` for every
 * address in the range (a `zz_` placeholder, not evidence).  3. The code is enemy-band: it calls
 * `get_enemy_data(_ENEMY_WORK*)`, `get_move_work_adrs`/`get_move_work_max`, `fn_8013BE60`/
 * `fn_8013C244` (the neighbour unit's interpreter), and the program tables it walks are
 * `em001_prog_tbl`..`em040_prog_tbl`; both bracketing registered units are `enemy`
 * (`enemy/fn_8013BE60.c` below, `enemy/fn_80149D6C.c` above) and the neighbours' scheme is the map's
 * own stem.  The file therefore keeps the map stem (brief option 4); no name was invented, and the
 * module is `enemy`.
 *
 * Language.  The unit calls four mangled callees (`get_enemy_data__FP11_ENEMY_WORK`,
 * `get_move_work_adrs__FUc`, `get_move_work_max__FUc`, `load_file_req__FPcUllUllPUl`) and rule 9
 * forbids spelling a mangling as the callable identifier, so every one of them is declared at C++
 * scope with the signature its mangling encodes (`tools/units/mangle.py` proves each) and the file is
 * C++.  Every definition keeps the map's plain `fn_XXXXXXXX` name, i.e. the `extern "C"` block below.
 *
 * Types.  `_ENEMY_WORK` is the shared record `include/enemy/ENEMY_WORK.h` owns (rule 1); this unit
 * added the fields it measured (+0x010, +0x360, +0x36C, +0x383/+0x384, the 0x961 stack block and the
 * +0x9E8..+0xA00 interpreter bytes) to that header, and the record types that go with them
 * (`EmProgTbl`/`EmCmdRec`/`EmCmdRecWide`/`EmAreaWork`/`EmActRec`/`EmFileEntry`/`EmFileRow`) live
 * there too, because the same interpreter is spread over this unit and its neighbours.
 *
 * Sections.  Besides `.text` the unit owns `extab` 0x8000D5A4..0x8000D664 and `extabindex`
 * 0x80027F60..0x80028080 - the 24 entries of the functions that carry an exception record, taken
 * from the target's own run boundaries (the previous unit's `extabindex` ends exactly at 0x80027F60
 * and the next one starts exactly at 0x80028080).  No `.ctors` word and no data.
 *
 * Residuals.  `python tools/units/recompile.py enemy/fn_8013F764.cpp` (target object from the
 * unit's own split, 6740 B of `.text`): 27 of the 45 bodies are byte-identical, the unit measures
 * `.text` 95.63 % (`matched_code` 2872 of 6740 B, `matched_functions` 27 of 45).  `extab` comes out
 * **100 %** (192 B, byte-identical) and every function is above the 80 % bar except two:
 *
 *   * `fn_801408B4` 67.70: a 7-way dispatch on `field_0x95C`.  The target lowers it as a *range test*
 *     (`subi r0,r4,3` / `cmplwi r0,4` / `ble` into one body shared with case 0xB) followed by the
 *     0/2/0xB/1/0xA compares, and keeps the field in r4.  `switch (self->field_0x95C)` with the
 *     six cases sharing that body produces the target's exact 276 bytes but MWCC lowers it as a
 *     compare chain (`cmplwi r?,3` / `blt`) instead, which costs the first four rows; an explicit
 *     `if (v - 3 <= 4) { ... }` does emit the range test but then duplicates the 0xB body (316 B).
 *     Both spellings were measured; the switch is kept (right size, 67.70; the if-form's extra copy
 *     of the body makes the object 40 B too long, so it is not kept).
 *   * `fn_80140B10` 75.00: the 4-instruction trace wrapper.  The target's last two instructions are
 *     `b .+4` + `blr` - a branch to an empty body the compiler placed 4 bytes after the function -
 *     and its arguments are the caller's r4/r5 (its own r3 is unused), with r4 masked to a byte and
 *     r5 passed through.  `static void em_prog_trace2(u8, u32)` called as
 *     `em_prog_trace2((u8)a, b)` from a 3-parameter wrapper reproduces all three setup instructions
 *     exactly; only the tail branch differs (ours names the helper, the target's is the local label),
 *     which is what the 100 - 25 % costs.  The helper cannot be emitted inline: with
 *     `-inline noauto` MWCC keeps the named call, and with the helper inlined the mask
 *     survives but `mr r4,r5` does not (measured: 58.75 %).
 *
 * The rest, by size: `fn_80140AF8` 88.00 (an extra `mr r0,r4`: MWCC shuffles the second argument into
 * r4 before masking the first, where retail masks r4 into r3 first; measured with the parameters
 * declared u32/u8/u8 and with the cast pulled out into a local), `fn_801409C8` 88.09,
 * `fn_80140778` 88.23, `fn_80141050` 90.28 (register allocation: retail keeps the id in r30 and its
 * counter in r31 and spends r28/r29 on the index and the sub-command), `fn_80140CA0` 92.00,
 * `fn_8014001C` 94.32, `fn_80140E48`/`fn_80140FB0` 94.50, `fn_80140B20` 96.88, `fn_80140C00` 96.58,
 * `fn_8014026C` 99.09 (retail keeps an `extsh` after the `clrlwi` on the value it forwards;
 * `(s16)(u8)arg` folds to the mask alone in this compiler, `(s16)(arg & 0xFF)` and a `(s32)`
 * parameter were both measured), `fn_80140354` 99.47, `fn_8013F764` 99.62, `fn_8013FD98` 99.63
 * (register allocation of the two loop variables), `fn_801406E0` 99.71, `fn_80140EE8` 99.90.
 * `extabindex` (288 B) lands at 98.61 % because the entries carry each function's own size, so the
 * two functions whose bodies are still short shift their `.4byte` size word.
 *
 * Source shape worth keeping: the unit needs `#pragma peephole off`.  Retail keeps the unfused
 * `rlwinm`/`clrlwi` + `cmpwi` pairs the `(v & 0x80) == 0` and `len & 0x30` tests are built from;
 * with the peephole pass on they become `rlwinm.`/`clrlwi.` and the branch moves.  Measured on the
 * whole unit: `fn_80140AF8` 39.00 -> 88.00, `fn_80140B10` 0.00 -> 75.00, `fn_801406E0` 77.38 ->
 * 99.71, `fn_80140C00` 76.20 -> 96.58 (the signed `(s32)(u8)sel` comparison plus the typed
 * `SystemWorkTables` view),
 * `fn_80140778` 85.76 -> 88.23, `fn_801408B4` 59.57 -> 67.70, `fn_80140B20` 93.30 -> 96.88,
 * `fn_80141050` 79.17 -> 90.28; no symbol moved down.
 */

#include "types.h"
#include "nw4r/math.h"
#include "enemy/ENEMY_WORK.h"

#pragma peephole off

/* ------------------------------------------------------------------------------------------------ *
 * pooled data owned by other units: declared, never defined (playbook 29), so the load operands pair
 * with the target's pool relocations
 * ------------------------------------------------------------------------------------------------ */

/* the 0x6D-byte command-length table `fn_801406E0` reads */
extern u8 lbl_805A1530[];
/* the `em001_prog_tbl`..`em040_prog_tbl` pointer array, NULL-terminated (`fn_80140B20` arms it) */
extern u32 lbl_805A1B34[];
/* the per-enemy byte table `fn_80141050` reads (`lbzx` + `extsb`, so it is signed) */
extern s8 lbl_805A1B08[];
/* the two `{key, name}` file tables `fn_80141050` walks */
extern EmFileEntry lbl_80581E20[];
extern EmFileEntry lbl_80582348[];
/* the `.sdata2` pool the range's float work reads */
extern f32 lbl_80796DBC;
extern f32 lbl_80796DC0;
extern f32 lbl_80796DC4;
/* the row base `fn_80140CA0` indexes with `fn_800D4DD8`'s answer */
extern u8* lbl_80794970;
/* the enemy file/sound-proc work blob (`fn_80140B20` clears its second array) */
extern u8 emc_work[];
extern u32 em_snd_proc_tbl[];
/* the system work record `fn_80140C00` looks its extra table up in */
extern u8 system_w[];

/* ------------------------------------------------------------------------------------------------ *
 * the callees
 *
 * The four the map carries mangled are declared at C++ scope with the signature the mangling encodes
 * (`tools/units/mangle.py` proves each one), so no call site spells a mangling (rule 9).
 * ------------------------------------------------------------------------------------------------ */

/* `get_enemy_data__FP11_ENEMY_WORK` (owner `enemy/fn_801251D0.cpp`).  `EnemyData`'s full definition
 * lives in `include/enemy.h`, which cannot be included beside `enemy/ENEMY_WORK.h` (both define
 * `_ENEMY_WORK`), so the record is read through the two words the target loads. */
struct EnemyData;
struct EnemyData* get_enemy_data(_ENEMY_WORK* work);

/* `get_move_work_adrs__FUc` / `get_move_work_max__FUc` (owner `ef/fn_800CDB2C.cpp`). */
void* get_move_work_adrs(u8 area);
u16 get_move_work_max(u8 area);

/* `load_file_req__FPcUllUllPUl` (owner `sound/`'s file layer; the same spelling `ef/eft_res.cpp`
 * uses): (name, destination, size, callback, flag, context). */
void load_file_req(char* name, u32 data, s32 size, u32 callback, s32 flag, u32* ctx);

/* the two records this unit's own call sites name */
/* the 0x18-byte per-enemy file slot of `emc_work` (`fn_80140E48`/`fn_80140EE8`/the load callback) */
struct EmcWork {
    /* +0x00 */ u8 field_0x00;   /* the id `fn_800E3358` gets back */
    /* +0x01 */ u8 unused_0x01;
    /* +0x02 */ u8 index_0x02;   /* `fn_80146F10`'s file-table index */
    /* +0x03 */ u8 flags_0x03;   /* bit 0 = the sound proc was requested, bit 1 = the other file */
    /* +0x04 */ u8 unused_0x04[4];
    /* +0x08 */ s32 handle_0x08; /* the first file's handle */
    /* +0x0C */ u8 unused_0x0C[4];
    /* +0x10 */ s32 handle_0x10; /* the second file's handle */
    /* +0x14 */ u8 unused_0x14[4];
}; /* size: 0x18 */

/* the record `fn_801262BC` hands `fn_8013F764` */
struct EmActionInfo {
    /* +0x00 */ u8 field_0x00;
    /* +0x01 */ u8 unused_0x01[3];
    /* +0x04 */ f32 value_0x04;
}; /* size: 0x08 */

/* the head `fn_80126494` returns: its +0x08 word is the first `EmActRec` of the list `fn_801409C8`
 * scans.
 * size: 0x0C */
struct EmActList {
    /* +0x00 */ u32 field_0x00;
    /* +0x04 */ u32 field_0x04;
    /* +0x08 */ EmActRec* entries_0x08;
};

/* `get_enemy_data`'s record, as far as this unit reads it: the two words `fn_801409C8` loads for its
 * timer comparison are the enemy data's own limit (`include/enemy.h`'s `EnemyData`, which cannot be
 * included beside `enemy/ENEMY_WORK.h`).
 * size: 0x0C */
struct EmEnemyDataSub {
    /* +0x00 */ u8 unused_0x00[0x28];
    /* +0x28 */ s16* value_0x28;
};

/* size: 0xA4 */
struct EmEnemyData {
    /* +0x00 */ u8 unused_0x00[0xA0];
    /* +0xA0 */ EmEnemyDataSub* sub_0xA0;
};

/* the system work record `system_w`, as far as this unit reads it: its +0x898 word is the extra table
 * array `fn_80140C00` indexes by enemy id and then by kind.  Deliberately not called `SystemWork`:
 * `enemy/fn_8014A1BC.c` already owns that name, and rule 1 forbids a second definition of it.
 * size: 0x89C */
struct SystemWorkTables {
    /* +0x000 */ u8 unused_0x000[0x898];
    /* +0x898 */ u32** tables_0x898;
};

/* the handle record `fn_800D5418` returns, as `fn_80140CA0`/`fn_80140EE8` read it (its +0x44 word
 * is handed to `fn_80054FAC`) */
struct EmHandleRec {
    /* +0x00 */ u8 unused_0x00[0x44];
    /* +0x44 */ u32 field_0x44;
}; /* size: 0x48 */

#ifdef __cplusplus
extern "C" {
#endif

/* ------------------------------------------------------------------------------------------------
 * The plain `fn_XXXXXXXX` callees.  The map spells them as C symbols, so the whole set - the map's
 * own definitions included - is `extern "C"`: without it this C++ front-end would mangle them and
 * objdiff would pair nothing (docs/matching.md row 42).  Their owning units' headers do not carry
 * them yet, so they are declared here (rule 2's interim home for the enemy band, the same one
 * `enemy/fn_80137604.cpp` uses).
 * ------------------------------------------------------------------------------------------------ */

/* this unit's own entry points, used before their definitions below */
u8 fn_801406E0(u32 code, u32 sub);
u8 fn_80140768(u8* in);
s16 fn_80140778(u8* in, u8 code, u8 mode);
void fn_80140AF8(_ENEMY_WORK* self, u32 a, u8 b);
void fn_80140B0C(u8 a, u8 b, u8 c);
void fn_80140DAC(EmFileRow* dst, EmFileRow* src);
s32 fn_80140E48(char* name, u32 size, u8 flag);
void fn_80140EE8(char* name, u32 size, u32 a, u32 b, u32 c, u32* ctx);
s32 fn_80140FB0(char* name, u32 size, u8 flag);
void fn_8013F994(_ENEMY_WORK* self);

/* `enemy/fn_8013BE60.c` - the interpreter this unit drives */
s16 fn_8013BE60(_ENEMY_WORK* self, u8* in, u32 id, u32 sub, s16 value);
void fn_8013C244(_ENEMY_WORK* self, u8* in, u32 id);

/* the enemy band's other units */
void* fn_801262BC(_ENEMY_WORK* self, u8 code);        /* the EmActionInfo record */
void fn_8012B380(_ENEMY_WORK* self, u32 a, u32 b, u8 c);
void fn_80128BF8(_ENEMY_WORK* self, u32 mode);
void fn_8013C36C(_ENEMY_WORK* self, u8 a, u8* in);
void fn_8013BDE4(u8** in, u8 code, s16* out);
void fn_8013AA00(_ENEMY_WORK* self);
u32 fn_801339AC(_ENEMY_WORK* self);
u32 fn_8013023C(_ENEMY_WORK* self);
u32 fn_801321B0(_ENEMY_WORK* self);
u32 fn_80137704(_ENEMY_WORK* self, u32 kind);
u32 fn_801358D0(_ENEMY_WORK* self, u8 idx);
void fn_80130CDC(_ENEMY_WORK* self, s32 value);
void fn_80137720(_ENEMY_WORK* self, u32 mode);
void fn_80130858(_ENEMY_WORK* self, s32 value);
EmActList* fn_80126494(_ENEMY_WORK* self);
u32 fn_8027D530(EmAreaWork* rec);
u32 fn_802B0998(u8 id);
char* fn_80146F10(u8 id);
void fn_80146B98(s32 id);
u32 fn_80146DF4(u8 id);
u8 fn_80141470(u8 id);
u8 fn_801414D4(u8 id);
void fn_801414C8(void* work);
void fn_80147160(u8 id);
u8 fn_80147550(u8 id);
s32 fn_80147684(u8 id, s32 a);
void fn_801411B8(void* str);   /* owner: `enemy/fn_801411B8.cpp`'s range, the next proposal */
void fn_800FA9B8(u8* rec, u8 id);
void fn_800F0F9C(u8 id);

/* the shared layers the range's helpers call */
f32 fn_80050F80(nw4r::math::VEC3* a, nw4r::math::VEC3* b);
u32 fn_803B4EC8(u32 a);
u32 fn_803B5030(u32 a);
void* fn_80054FE8(void* out, u32 a);
void fn_80054FAC(void* dst, void* src);
void fn_8007B878(void* out, u32 a);
void fn_8007BA08(void* a, void* b);
s32 fn_800D56F4(char* name, u32 data);
void* fn_800D5418(s32 handle);
s32 fn_800D4DD8(char* name);
void fn_800D5D60(void* work);
void fn_800D5E30(void* work);
void fn_800E38BC(void* a);
void fn_800E3358(u32 a, u8 b, void* c);
s32 fn_800924CC(void* a, u32 b);

/* ------------------------------------------------------------------------------------------------
 * the run driver
 * ------------------------------------------------------------------------------------------------ */

u8 fn_8013F764(_ENEMY_WORK* self, u8* in) {
    EmProgEntry* entry;
    u8 first = in[0];
    u8* p = in + 1;
    u8 ret = 0;
    EmProgTbl* prog = self->prog_0xA00;

    if (prog != NULL) {
        if (self->field_0x9FD & 1) {
            entry = &prog->entries_0x04[prog->count_0x00 - 1 - self->field_0x9FC];
        } else {
            entry = &prog->entries_0x04[self->field_0x9FC];
        }
        if (self->field_0x9FE == 0) {
            EmActionInfo* info = (EmActionInfo*)(void*)fn_801262BC(self, entry->code);
            self->field_0x383 = 1;
            self->field_0x384 = info->value_0x04;
            fn_8012B380(self, 5, 8, info->field_0x00);
            fn_80128BF8(self, 0);
            self->field_0x9FE++;
        }
        if (fn_80050F80(&self->pos, &self->vec_0x36C) < lbl_80796DBC) {
            self->field_0x9FE = 2;
        } else {
            self->field_0x9FE = 1;
        }
        switch (self->field_0x9FE) {
        case 1:
            fn_8013C36C(self, entry->field_0x01, p);
            ret = 1;
            break;
        case 2:
            fn_8013C36C(self, entry->field_0x02, p);
            ret = 1;
            if (first == 0) {
                fn_8013F994(self);
            }
            break;
        }
    }
    self->field_0x9FF = 1;
    return ret;
}

void fn_8013F994(_ENEMY_WORK* self) {
    EmProgTbl* prog = self->prog_0xA00;

    if (prog == NULL) {
        return;
    }
    if (self->field_0x9FE != 2) {
        return;
    }
    self->field_0x9FC++;
    if ((u8)self->field_0x9FC >= prog->count_0x00) {
        fn_8013AA00(self);
        return;
    }
    self->field_0x9FE = 0;
}

/* ------------------------------------------------------------------------------------------------
 * the stream readers (command id in the trailing comment)
 * ------------------------------------------------------------------------------------------------ */

s16 fn_8013F8D8(_ENEMY_WORK* self, u8* in) {                    /* 0x4E */
    s16 value = 0;

    switch (in[0]) {
    case 0:
        if (self->prog_0xA00 == NULL) {
            in += fn_801406E0(0x4E, in[0]);
            fn_8013BDE4(&in, 0x4E, &value);
        }
        break;
    case 2:
        in += fn_801406E0(0x4E, in[0]);
        value = fn_80140778(in, 0x4E, 1);
        break;
    case 0xFF:
        break;
    default:
        fn_80140AF8(self, 0, 0);
        break;
    }
    return value;
}

s16 fn_8013F9D8(_ENEMY_WORK* self, u8* in) {                    /* 0x50 */
    s16 value = 0;

    switch (in[0]) {
    case 0:
        if (fn_801339AC(self) != 0) {
            break;
        }
        in += fn_801406E0(0x50, in[0]);
        fn_8013BDE4(&in, 0x50, &value);
        break;
    case 2:
        in += fn_801406E0(0x50, in[0]);
        value = fn_80140778(in, 0x50, 1);
        break;
    case 0xFF:
        break;
    default:
        fn_80140AF8(self, 0, 0);
        break;
    }
    return value;
}

void fn_8013FA9C(_ENEMY_WORK* self, u8* in) {                   /* 0x51, two-argument */
    if (in[0] == 0) {
        fn_8013BE60(self, in, 0x51, 0, (s16)(u8)fn_8013023C(self));
    } else {
        fn_8013C244(self, in, 0x51);
    }
}

void fn_8013FB08(_ENEMY_WORK* self, u8* in) {                   /* 0x52, two-argument */
    if (in[0] == 0) {
        EmCmdRec* rec = self->recs_0x9A4;
        if (rec != NULL) {
            fn_8013BE60(self, in, 0x52, 0, (s16)rec->code);
        } else {
            fn_80140AF8(self, 0x11, self->field_0x95C);
            in += fn_801406E0(0x52, in[0]);
            fn_80140778(in, 0x52, 1);
        }
    } else {
        fn_8013C244(self, in, 0x52);
    }
}

s16 fn_8013FB9C(_ENEMY_WORK* self, u8* in) {                    /* 0x53 */
    s16 value = 0;

    switch (in[0]) {
    case 0:
        if (fn_801321B0(self) != 0) {
            break;
        }
        in += fn_801406E0(0x53, in[0]);
        fn_8013BDE4(&in, 0x53, &value);
        break;
    case 2:
        in += fn_801406E0(0x53, in[0]);
        value = fn_80140778(in, 0x53, 1);
        break;
    case 0xFF:
        break;
    default:
        fn_80140AF8(self, 0, 0);
        break;
    }
    return value;
}

s16 fn_8013FC60(_ENEMY_WORK* self, u8* in) {                    /* 0x54 */
    s16 value = 0;

    switch (in[0]) {
    case 0:
        if (self->field_0x1FD != 0) {
            break;
        }
        in += fn_801406E0(0x54, in[0]);
        fn_8013BDE4(&in, 0x54, &value);
        break;
    case 2:
        in += fn_801406E0(0x54, in[0]);
        value = fn_80140778(in, 0x54, 1);
        break;
    case 0xFF:
        break;
    default:
        fn_80140AF8(self, 0, 0);
        break;
    }
    return value;
}

void fn_8013FD1C(_ENEMY_WORK* self, u8* in) {                   /* 0x55, field setter */
    self->field_0x382 = in[0];
}

void fn_8013FD28(_ENEMY_WORK* self, u8* in) {                   /* 0x56, two-argument */
    if (in[0] == 0) {
        fn_8013BE60(self, in, 0x56, 0, (s16)self->field_0x9F8);
    } else {
        fn_8013C244(self, in, 0x56);
    }
}

void fn_8013FD50(_ENEMY_WORK* self, u8* in) {                   /* 0x57, two-argument */
    if (in[0] == 0) {
        fn_8013BE60(self, in, 0x57, 0x0C, 0);
    } else {
        fn_8013C244(self, in, 0x57);
    }
}

void fn_8013FD74(_ENEMY_WORK* self, u8* in) {                   /* 0x58, two-argument */
    if (in[0] == 0) {
        fn_8013BE60(self, in, 0x58, 0x0D, 0);
    } else {
        fn_8013C244(self, in, 0x58);
    }
}

s16 fn_8013FD98(_ENEMY_WORK* self, u8* in) {                    /* 0x59 */
    s16 value = 0;

    switch (in[0]) {
    case 0: {
        u8 found = 0;
        if (self->team != 0x12 || self->field_0x360 <= 0) {
            u16 count = get_move_work_max(2);
            EmAreaWork* rec = (EmAreaWork*)get_move_work_adrs(2);
            for (u16 i = 0; i < count; i++) {
                if (rec->active != 0 && rec->area_no == self->area_no && fn_8027D530(rec) != 0) {
                    found = 1;
                    break;
                }
                rec++;
            }
        }
        if (found == 0) {
            in += fn_801406E0(0x59, in[0]);
            fn_8013BDE4(&in, 0x59, &value);
        }
        break;
    }
    case 2:
        in += fn_801406E0(0x59, in[0]);
        value = fn_80140778(in, 0x59, 1);
        break;
    case 0xFF:
        break;
    default:
        fn_80140AF8(self, 0, 0);
        break;
    }
    return value;
}

void fn_8013FEF4(_ENEMY_WORK* self, u8* in) {                   /* 0x5A, stack push */
    self->stack_0x961[in[0]] = in[1];
}

void fn_8013FF08(_ENEMY_WORK* self, u8* in) {                   /* 0x5B, two-argument */
    if (in[0] == 0) {
        fn_8013BE60(self, in, 0x5B, 0, (s16)self->stack_0x961[in[1]]);
    } else {
        fn_8013C244(self, in, 0x5B);
    }
}

void fn_8013FF38(_ENEMY_WORK* self, u8* in) {                   /* 0x5C, two-argument */
    if (in[0] == 0) {
        fn_8013BE60(self, in, 0x5C, 0x0E, 0);
    } else {
        fn_8013C244(self, in, 0x5C);
    }
}

s16 fn_8013FF5C(_ENEMY_WORK* self, u8* in) {                    /* 0x5E */
    s16 value = 0;

    switch (in[0]) {
    case 0:
        if (self->field_0x9F7 == self->area_no) {
            break;
        }
        in += fn_801406E0(0x5E, in[0]);
        fn_8013BDE4(&in, 0x5E, &value);
        break;
    case 2:
        in += fn_801406E0(0x5E, in[0]);
        value = fn_80140778(in, 0x5E, 1);
        break;
    case 0xFF:
        break;
    default:
        fn_80140AF8(self, 0, 0);
        break;
    }
    return value;
}

void fn_8014001C(_ENEMY_WORK* self, u8* in) {                    /* 0x5F, two-argument with a ratio */
    if (in[0] == 0) {
        fn_8013BE60(self, in, 0x5F, 2,
                    (s16)(s32)(lbl_80796DC0 * ((f32)self->field_0x7A0 / (f32)self->field_0x7A4)));
    } else {
        fn_8013C244(self, in, 0x5F);
    }
}

void fn_801400B0(_ENEMY_WORK* self, u8* in) {                   /* 0x60, field setter */
    self->field_0x1E7 = in[0];
}

s16 fn_801400BC(_ENEMY_WORK* self, u8* in) {                    /* 0x61 */
    s16 value = 0;

    switch (in[0]) {
    case 0:
        if (self->field_0x1F9 != 0) {
            break;
        }
        in += fn_801406E0(0x61, in[0]);
        fn_8013BDE4(&in, 0x61, &value);
        break;
    case 2:
        in += fn_801406E0(0x61, in[0]);
        value = fn_80140778(in, 0x61, 1);
        break;
    case 0xFF:
        break;
    default:
        fn_80140AF8(self, 0, 0);
        break;
    }
    return value;
}

s16 fn_80140178(_ENEMY_WORK* self, u8* in) {                    /* 0x62 */
    s16 value = 0;

    switch (in[0]) {
    case 0:
        if (fn_802B0998(in[1]) != 0) {
            break;
        }
        in += fn_801406E0(0x62, in[0]);
        fn_8013BDE4(&in, 0x62, &value);
        break;
    case 2:
        in += fn_801406E0(0x62, in[0]);
        value = fn_80140778(in, 0x62, 1);
        break;
    case 0xFF:
        break;
    default:
        fn_80140AF8(self, 0, 0);
        break;
    }
    return value;
}

void fn_80140244(_ENEMY_WORK* self, u8* in) {                   /* 0x63, two-argument */
    if (in[0] == 0) {
        fn_8013BE60(self, in, 0x63, 0, (s16)self->field_0x010);
    } else {
        fn_8013C244(self, in, 0x63);
    }
}

void fn_8014026C(_ENEMY_WORK* self, u8* in, u32 arg) {           /* 0x65, two-argument */
    if (in[0] == 0) {
        fn_8013BE60(self, in, 0x65, 0, (s16)(arg & 0xFF));
    } else {
        fn_8013C244(self, in, 0x65);
    }
}

s16 fn_80140298(_ENEMY_WORK* self, u8* in) {                    /* 0x66 */
    s16 value = 0;

    switch (in[0]) {
    case 0:
        if (self->field_0x1F8 != 0) {
            break;
        }
        in += fn_801406E0(0x66, in[0]);
        fn_8013BDE4(&in, 0x66, &value);
        break;
    case 2:
        in += fn_801406E0(0x66, in[0]);
        value = fn_80140778(in, 0x66, 1);
        break;
    case 0xFF:
        break;
    default:
        fn_80140AF8(self, 0, 0);
        break;
    }
    return value;
}

s16 fn_80140354(_ENEMY_WORK* self, u8* in) {                    /* 0x67 */
    s16 value = 0;

    switch (in[0]) {
    case 0: {
        u8 hit_team = in[1];
        u8 found = 0;
        u16 count = get_move_work_max(3);
        _ENEMY_WORK* rec = (_ENEMY_WORK*)get_move_work_adrs(3);
        for (u16 i = 0; i < count; i++) {
            if (self->group != i && rec->active != 0 && rec->team == hit_team &&
                (rec->field_0x1C8 & 8) != 0) {
                found = 1;
                break;
            }
            rec++;
        }
        if (found == 0) {
            in += fn_801406E0(0x67, in[0]);
            fn_8013BDE4(&in, 0x67, &value);
        }
        break;
    }
    case 2:
        in += fn_801406E0(0x67, in[0]);
        value = fn_80140778(in, 0x67, 1);
        break;
    case 0xFF:
        break;
    default:
        fn_80140AF8(self, 0, 0);
        break;
    }
    return value;
}

s16 fn_801404B8(_ENEMY_WORK* self, u8* in) {                    /* 0x68 */
    s16 value = 0;

    switch (in[0]) {
    case 0:
        if (fn_80137704(self, 1) != 0) {
            break;
        }
        in += fn_801406E0(0x68, in[0]);
        fn_8013BDE4(&in, 0x68, &value);
        break;
    case 2:
        in += fn_801406E0(0x68, in[0]);
        value = fn_80140778(in, 0x68, 1);
        break;
    case 0xFF:
        break;
    default:
        fn_80140AF8(self, 0, 0);
        break;
    }
    return value;
}

s16 fn_80140580(_ENEMY_WORK* self, u8* in) {                    /* 0x69 */
    s16 value = 0;

    switch (in[0]) {
    case 0:
        if (fn_80137704(self, 2) != 0) {
            break;
        }
        in += fn_801406E0(0x69, in[0]);
        fn_8013BDE4(&in, 0x69, &value);
        break;
    case 2:
        in += fn_801406E0(0x69, in[0]);
        value = fn_80140778(in, 0x69, 1);
        break;
    case 0xFF:
        break;
    default:
        fn_80140AF8(self, 0, 0);
        break;
    }
    return value;
}

void fn_80140648(_ENEMY_WORK* self, u8* in) {                   /* 0x6A, two-argument */
    if (in[0] == 0) {
        fn_8013BE60(self, in, 0x6A, 2, (s16)self->field_0x7C8);
    } else {
        fn_8013C244(self, in, 0x6A);
    }
}

void fn_80140670(_ENEMY_WORK* self, u8* in) {                   /* 0x6C, two-argument */
    if (in[0] == 0) {
        fn_8013BE60(self, in, 0x6C, 0, (s16)(u8)fn_801358D0(self, in[1]));
    } else {
        fn_8013C244(self, in, 0x6C);
    }
}

/* ------------------------------------------------------------------------------------------------
 * the stream helpers
 * ------------------------------------------------------------------------------------------------ */

u8 fn_801406E0(u32 code, u32 sub) {
    u8 len;

    if ((u8)code < 0x6D) {
        len = lbl_805A1530[(u8)code];
        if ((len & 0x80) == 0) {
            return len;
        }
        switch ((u8)sub) {
        case 0:
            return (len + 1) & 0xF;
        case 1:
            switch (len & 0x30) {
            case 0xA0:
                return 4;
            case 0xB0:
                return 5;
            default:
                return 2;
            }
        default:
            return 1;
        }
    }
    return 0xFF;
}

u8 fn_80140768(u8* in) {
    return fn_801406E0(in[0], in[1]);
}

s16 fn_80140778(u8* in, u8 code, u8 mode) {
    s16 result = 0;
    s16 prev = 0;
    u8 stop = 0;
    u16 pos = 0;
    u8 want = code;

    for (;;) {
        if (in[0] == 0xFF) {
            fn_80140B0C(2, code, 0);
            break;
        }
        if (in[0] == want) {
            if (in[1] == 0) {
                u8 skip = fn_80140768(in) + 1;
                s16 off = fn_80140778(in + skip, code, 1) + skip;
                pos += off;
                in += off;
                continue;
            }
            switch (mode) {
            case 0:
                stop = 1;
                result = pos;
                break;
            case 1:
                if (in[1] == 0xFF) {
                    stop = 1;
                    result = pos;
                }
                break;
            case 2:
                if (in[1] == 0xFF) {
                    stop = 1;
                    result = prev;
                } else {
                    prev++;
                }
                break;
            }
        }
        if (stop == 0) {
            u8 skip = fn_80140768(in) + 1;
            pos += skip;
            in += skip;
        }
        if (stop != 0) {
            break;
        }
    }
    return result;
}

void fn_80140AF8(_ENEMY_WORK* self, u32 a, u8 b) {
    u8 a8 = a;
    fn_80140B0C(a8, b, self->team);
}

void fn_80140B0C(u8 a, u8 b, u8 c) {
}

/* retail's second two-argument trace entry: the callee is a static helper whose body the
 * release build compiled to nothing (the branch lands on its `blr`, 4 bytes after this
 * function's) - the same shape `fn_80140AF8` reaches through the global `fn_80140B0C`. */
static void em_prog_trace2(u8 a, u32 b) {
}

void fn_80140B10(u32 unused, u32 a, u32 b) {
    em_prog_trace2((u8)a, b);
}

u8 fn_801408B4(_ENEMY_WORK* self) {
    switch (self->field_0x95C) {
    case 3:
    case 4:
    case 5:
    case 6:
    case 7:
    case 0xB:
        self->field_0x95C = self->field_0x9DC;
        self->field_0x95D = self->field_0x9DD;
        self->field_0x958 = self->field_0x9E0;
        return 1;
    case 0: {
        u8 depth;
        if (self->stack_0x961[1] == 0) {
            return 0;
        }
        self->stack_0x961[1]--;
        depth = self->stack_0x961[1];
        self->field_0x95D = self->stack_0x961[2 + depth];
        self->field_0x958 = self->values_0x970[depth];
        return 1;
    }
    case 2:
        self->field_0x95C = self->rec_0x99C.code;
        self->field_0x95D = self->rec_0x99C.sub;
        self->field_0x958 = self->rec_0x99C.value;
        return 1;
    case 1: {
        u8 idx;
        self->count_0x9A8--;
        idx = self->count_0x9A8;
        self->field_0x95C = self->recs_0x9AC[idx].code;
        self->field_0x95D = self->recs_0x9AC[idx].sub;
        self->field_0x958 = self->recs_0x9AC[idx].value;
        return 1;
    }
    case 0xA:
        self->field_0x95C = self->field_0x9E8;
        self->field_0x95D = self->field_0x9E9;
        self->field_0x958 = self->field_0x9EC;
        return 1;
    default:
        return 0;
    }
}

void fn_801409C8(_ENEMY_WORK* self) {
    struct EnemyData* data = get_enemy_data(self);
    s32 value;

    if (fn_803B4EC8(0) == 1 || fn_803B5030(0) == 1) {
        value = 0x3E8;
    } else {
        EmActList* list = fn_80126494(self);
        if (list == NULL) {
            value = 0x1F4;
        } else {
            u8 act = self->field_0x7C8;
            EmActRec* rec = list->entries_0x08;
            value = 0;
            for (; rec->code != 0xFF; rec++) {
                if (act > rec->code) {
                    value = rec->value;
                }
            }
        }
    }
    fn_80130CDC(self, value);
    fn_80137720(self, 0);
    {
        s16* limit = ((EmEnemyData*)(void*)data)->sub_0xA0->value_0x28;
        if ((f32)self->field_0x8A6 == lbl_80796DC4 * (f32)(limit != NULL ? *limit : 0x258)) {
            fn_80130858(self, 0x258);
        }
    }
}

void fn_80140B20(void) {
    u32* src = lbl_805A1B34;
    u32* dst = (u32*)0x90348000;

    while (*src != 0) {
        *dst++ = *src++;
    }
    {
        u32 i;
        for (i = 0; i < 41; i++) {
            em_snd_proc_tbl[i] = 0;
        }
    }
}

u32 fn_80140C00(u8 id, u8 kind) {
    u8 sel = id;
    u32 sub;

    if ((s32)(u8)sel == 0x20) {
        switch ((u8)kind) {
        case 1:
            sub = 1;
            break;
        case 2:
            sub = 2;
            break;
        case 3:
            sub = 3;
            break;
        case 4:
            sub = 4;
            break;
        case 5:
            sub = 5;
            break;
        case 6:
            sub = 6;
            break;
        default:
            sub = 0;
            break;
        }
    } else {
        sub = 0;
    }
    return ((SystemWorkTables*)system_w)->tables_0x898[sel][sub];
}

/* ------------------------------------------------------------------------------------------------
 * the enemy file loader
 * ------------------------------------------------------------------------------------------------ */

void fn_80140CA0(char* name, u32 data, u32 a, u32 b, u32 c, u32* ctx) {
    EmcWork* work = (EmcWork*)&emc_work[*ctx * 0x18];
    u32 h1;
    u32 h2;
    u32 tmp;
    u32 res;
    EmFileRow row;
    u8* rec;

    fn_80054FE8(&h1, 0);
    fn_8007B878(&h2, 0);
    get_move_work_adrs(0);
    work->handle_0x08 = fn_800D56F4(name, data);
    if (work->handle_0x08 == -1) {
        return;
    }
    rec = (u8*)fn_800D5418(work->handle_0x08);
    fn_80140DAC(&row, (EmFileRow*)(lbl_80794970 + fn_800D4DD8(name) * 0x4C + 0x3070));
    if (rec == NULL) {
        return;
    }
    fn_80054FAC(&h1, fn_80054FE8(&tmp, ((struct EmHandleRec*)(rec))->field_0x44));
    fn_801411B8(&h1);
    res = fn_800924CC(&h1, 0);
    fn_8007BA08(&h2, &res);
    fn_800E38BC(&h2);
    fn_800D5D60(work);
}

void fn_80140DAC(EmFileRow* dst, EmFileRow* src) {
    *dst = *src;
}

s32 fn_80140E48(char* name, u32 size, u8 flag) {
    EmcWork* work = (EmcWork*)&emc_work[flag * 0x18];
    char* file;
    u32 ctx;

    work->flags_0x03 |= 1;
    ctx = flag;
    file = fn_80146F10(work->index_0x02);
    if (file == NULL) {
        return 0;
    }
    load_file_req(name, (u32)file, (s32)size, (u32)fn_80140CA0, 1, &ctx);
    return (s32)file;
}

void fn_80140EE8(char* name, u32 size, u32 a, u32 b, u32 c, u32* ctx) {
    EmcWork* work = (EmcWork*)&emc_work[*ctx * 0x18];
    u32 h1;
    u32 tmp;

    fn_80054FE8(&h1, 0);
    work->handle_0x10 = fn_800D56F4(name, size);
    if (work->handle_0x10 != -1) {
        u8* rec = (u8*)fn_800D5418(work->handle_0x10);
        if (rec != NULL) {
            fn_80054FAC(&h1, fn_80054FE8(&tmp, ((struct EmHandleRec*)(rec))->field_0x44));
            fn_801411B8(&h1);
            fn_800E3358(2, work->field_0x00, &h1);
            fn_800D5E30(work);
        }
    }
    fn_801414C8(work);
}

s32 fn_80140FB0(char* name, u32 size, u8 flag) {
    EmcWork* work = (EmcWork*)&emc_work[flag * 0x18];
    char* file;
    u32 ctx;

    work->flags_0x03 |= 2;
    ctx = flag;
    file = fn_80146F10(work->index_0x02);
    if (file == NULL) {
        return 0;
    }
    load_file_req(name, (u32)file, (s32)size, (u32)fn_80140EE8, 1, &ctx);
    return (s32)file;
}

s32 fn_80141050(u8 id) {
    s32 loaded = 0;

    if (fn_801414D4(id) != 0xFF) {
        s8 v = lbl_805A1B08[id];
        if (v > 0) {
            fn_80146B98(v);
        }
        return 0;
    }
    {
        u8 sub = fn_80141470(id);
        if (sub == 0xFF) {
            return -1;
        }
        if (fn_80146DF4(id) == 0) {
            return -1;
        }
        if (lbl_80581E20[id].key_0x00 != 0) {
            fn_80140E48(lbl_80581E20[id].name_0x04, lbl_80581E20[id].key_0x00, sub);
            loaded = 1;
        }
        if (lbl_80582348[id].key_0x00 != 0) {
            fn_80140FB0(lbl_80582348[id].name_0x04, lbl_80582348[id].key_0x00, sub);
            loaded++;
        }
    }
    fn_80147160(id);
    if (fn_80147550(id) == 1) {
        s32 idx = fn_80147684(id, 0);
        if (idx >= 0) {
            fn_800FA9B8(&emc_work[0xEA0 + idx * 0x17], id);
        }
    }
    fn_800F0F9C(id);
    return loaded;
}

#ifdef __cplusplus
}
#endif
