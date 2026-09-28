/*
 * stage/fn_802B2AA0.cpp - the stage band's per-area runtime state: the `stage_w` block flags/timers,
 * the two 0x4F8-byte per-area objects and the area colour/effect drivers that read them.
 *
 * `.text` 0x802B2AA0-0x802B5C58 (39 functions, 12728 B), the run right after `stage/fn_802B2978.c`.
 * Registered once, at its final home (docs/plan.md 12), from `proposal/802B2AA0_fn_802B2AA0.cpp`.
 *
 * Name.  Module `stage`: the left neighbour is `stage/fn_802B2978.c`, the lib this file sits in is
 * `stage`, and the range's own entry points are the stage-work block (`stage_w`, .bss 0x806B87C0,
 * materialised as `0x806C0000-0x7840` in the prologues) and the two per-area objects at
 * `lbl_806BB7E0`.  No `__FILE__` string covers the range (the `menu_item.cpp` literal at 0x805CDFC8 is
 * referenced only by the band *below* 0x802B2AA0) and the runtime dump answers only `zz_XXXXXXXX_`, so
 * the file keeps the map's own stem (class 4, brief section 2).
 *
 * Naming note: the symbol map has only fn_XXXXXXXX for this range (checked with
 * `python tools/symbols/symedit.py range 0x802B2AA0 0x802B5C58`: every entry is a bare
 * `fn_XXXXXXXX = .text:0x802BXXXX;` line, and `python tools/symbols/dumpmap.py lookup <addr>` answers
 * the `zz_XXXXXXXX_` placeholder for each).
 *
 * Language C++.  The map's undefined set carries real manglings (`get_camera_pos__Fv`,
 * `setVisibility__6MHcharFUlb`, `calcVecAngXY__FPQ34nw4r4math4VEC3PUlPUl`,
 * `shell_se_req__FP5_se_wPQ34nw4r4math4VEC3UcUl`, `Pl_master_ck__FP4_PLW`, `get_now_mapno__Fv`) and
 * the retail object carries extab/extabindex, which a no-exceptions C unit could not.
 *
 * The band's types, its own entry points and the callee declarations live in
 * `include/stage/fn_802B2AA0.h` (docs/plan.md 6.5 rules 1-5); this file is the bodies.
 *
 * Sections: .text 0x802B2AA0-0x802B5C58; extab 0x80013D34-0x80013E3C (33 8-byte records - the
 * extabindex table has one entry per function that contains a `bl`); extabindex
 * 0x80031A4C-0x80031BD8 (33 x 12 B).  The six functions with no record (fn_802B2F3C, fn_802B3250,
 * fn_802B3260, fn_802B45BC, fn_802B45D4, fn_802B46DC) are exactly the ones with no call at all.
 *
 * Data.  `.data` 0x805CF728-0x805CF784 (92 B) is claimed and 100 %: it is the 23-entry switch table
 * MWCC emits for fn_802B3270's `switch (st->mapno)` (arms read out of `main.elf`; the table is our
 * object's whole `.data`, so the claim is exactly what the object emits, plan 8.4).  The rest of the
 * run the data queue attributes to `auto/802B2AA0_fn_802B2AA0` - the hand-written tables
 * 0x805CF60C-0x805CFBE8 minus that table, 19 labels - stays **unclaimed**: our object does not emit
 * them, so claiming them would only add target bytes nothing reproduces (playbook 23 / 8.4); the
 * `range` config_request in this unit's outbox carries the evidence.  Every pooled constant is
 * therefore still `extern`-declared and never defined (playbook 29).
 *
 * Flags.  The whole band is peephole-OFF, measured rather than inferred: with the lib's `-O3` peephole
 * on, MWCC restores a saved `f32` with `psq_l f31,N(r1)` where retail has the unfused `li r0,N` +
 * `psq_lx f31,r1,r0,0,0`; the probe is the same four-instruction function with `-opt nopeephole`
 * added.  `#pragma peephole off` (playbook 39) reproduces retail's prologue/epilogue pair, and the
 * sibling `stage/fn_802B2978.c` and `stage/stg_w.cpp` already carry the same pragma for the same
 * reason.  A lib-level flag is the durable home; until that lands the pragma pair carries the
 * deviation here.
 *
 * Shapes.  fn_802B3270's list writes are `field[i] = v; i++;` through the *struct field*, not
 * `buf[i++] = v` through the array: retail re-loads `local.show`/`local.hide` off the stack before
 * every store, which only the field spelling produces.  Its dispatches come in both lowerings - the
 * three on `fn_802FB8EC`'s result are `switch`es (one contiguous compare chain, cases merged into a
 * `cmplwi/ble` range test where two share a body) while the `LbCheckKujiraEvent`/`fn_802FB9F8` tests
 * are `if`/`else if` (chain interleaved with the bodies); a rewrite to the other lowering costs
 * ~7 points.  `fn_802FB9F8` returns `u32`, not `u8` (retail compares `r3` raw, with no `clrlwi`).
 *
 * Residuals (measured 2026-09-26, real command line):
 *   - fn_802B414C 36.50 % (320 B): the two effect-strip loops; ours is 332 B, so the loop shape (the
 *     `for(;;)` pair the target's `beq`-to-epilogue implies) still differs.
 *   - fn_802B4E58 43.65 % (1284 B): the colour-cycle blend; ours is 1324 B.  The final
 *     `fn_80057DE0(0, 6, colour, lbl_8079A4C4)` call and the `psq_lx` prologue/epilogue pair are right,
 *     so the residual is the inline per-channel blend shape.
 *   - fn_802B2F60 77.56 % (752 B, ours 812 B): source shape.
 *   - fn_802B4ABC 78.64 % (416 B, ours 412 B): the condition order still differs.  Retail tests
 *     `now - base`'s `& 0x40` first and puts both `field_0x2FDC = 0` tails at the *end* of the function
 *     (0x2188 / 0x2194), while ours emits one of them inline; and retail compares the map number with
 *     `cmpwi` where MWCC emits `cmplwi` for the same `mapno != 5 && mapno != 0x10 && mapno != 0xA`
 *     (both the `s32`/`int`/`s8` spelling and a `switch` give `cmplwi`).  Its *semantics* are now
 *     right: `entry` is `p->offsets` (a sub-table of 8-byte records), which the earlier reconstruction
 *     read as `entry = p` - the outer table and the sub-table share the record layout.
 *   - The claimed jump table's two relocations still pair by *value*, not by name: retail's split names
 *     the table `jumptable_805CF728` where MWCC emits an anonymous `@NNNN` in our object.  The bytes
 *     and the section size are equal (100.0), so this is cosmetic - but it is why the object is not
 *     byte-identical to the target yet.
 *   - Every int->float conversion gets MWCC's own anonymous `.sdata2` slot where the split names
 *     `lbl_8079A470` (2^52) / `lbl_8079A460` (2^52 + 2^31); that constant cannot be named from source
 *     (playbook 29) and it is the same residual `fn_802B2978` carries.
 *   - The unit's `.ctors` word (0x8056F37C, dtk appended the range in the re-split) is not emitted by
 *     our object; no static object with a constructor exists in the reconstruction yet.
 *
 * `fn_802B2F60` and `fn_802B4ABC` keep their pre-existing source shapes.  Three rows that sat below the
 * bar are 100 % on shape alone, and each one's shape is load-bearing:
 *   - fn_802B3270: new here; its call site (`fn_802B4C5C`, which must pass all three arguments) moved
 *     that row 95.69 -> 96.38.
 *   - fn_802B4824: the outer loop must be a `for` with `index` initialised outside it (`for (; index <
 *     4U; index++)`), not a `do`/`while` - only a for-counter gets the range analysis that drops the
 *     `clrlwi` off `index < 4U`; and the per-area body re-reads `st->area_char[index]` (retail loads
 *     the element address once per iteration and reloads the pointer after `frame_init`).  The
 *     declarations are ordered `scale, amount, index, joint, st` because the allocator colours them by
 *     declaration order (98.50 % in the natural order) - a deliberate deviation from the file's style.
 *   - fn_802B45D4: `(x & 1) != 0`, not `!(x & 1)` - the `!` spelling makes MWCC emit the `cntlzw`/`srwi`
 *     pair where retail has `neg`/`or`/`srwi`, and the peephole is off in this band already, so the
 *     asymmetry is source, not flag.
 *
 * Inventory and evidence: `python tools/units/ledger.py unit stage/fn_802B2AA0.cpp`.
 */

#include "types.h"
#include "mh3_pad.h" /* the owner header (rule 2) */
#include "nw4r/math.h"
#include "gx.h"
#include "sound/mhchar.h"
#include "sound/fn_800D7F54.h"
#include "unsplit/ef.h"
#include "unsplit/g3d.h"
#include "unsplit/Pl.h"
#include "unsplit/unknown.h"
#include "Pl/pl_master.h"
#include "fn_8004CAD8.h"
#include "ef/fn_800CDB2C.h"

/* The stage band is peephole-off for the whole range (see the file header). */
#pragma peephole off

#include "stage/fn_802B2AA0.h"

/* ------------------------------------------------------------------------------------------------
 * the range, in address order
 * ------------------------------------------------------------------------------------------------ */

/* Blends the area's three colour groups by the camera/area angle and arms the six results as one
 * sound/effect frame set. */
extern "C" void fn_802B2AA0(StageBlendRec* rec, void* obj, u8 store)
{
    nw4r::math::VEC3 poly;
    nw4r::math::VEC3 world;
    nw4r::math::VEC3 cam;
    u32 ang_x;
    u32 ang_y;
    s32 base;
    u32 colour[6];
    u16 delta;
    f32 t;

    VEC3_ctor(&poly);
    base = fn_802BDF5C();
    fn_80050CA0(&world, &get_camera_pos(), obj);
    copyVec3(&poly, &world);
    calcVecAngXY(&poly, &ang_x, &ang_y);

    delta = (u16)(ang_y - base);
    if (delta < 0x4000U) {
        t = (f32)delta / lbl_8079A49C;
        colour[0] = fn_802B2978(rec->group[0].base, rec->group[0].target_a, t);
        colour[1] = fn_802B2978(rec->group[0].base, rec->group[0].target_b, t);
        colour[2] = fn_802B2978(rec->group[2].base, rec->group[2].target_a, t);
        colour[3] = fn_802B2978(rec->group[2].base, rec->group[2].target_b, t);
        colour[4] = fn_802B2978(rec->group[1].base, rec->group[1].target_a, t);
        colour[5] = fn_802B2978(rec->group[1].base, rec->group[1].target_b, t);
    } else if (delta < 0x8000U) {
        t = (f32)(s32)(delta - 0x4000) / lbl_8079A49C;
        colour[0] = fn_802B2978(rec->group[0].target_a, rec->group[0].axis, t);
        colour[1] = fn_802B2978(rec->group[0].target_b, rec->group[0].axis, t);
        colour[2] = fn_802B2978(rec->group[2].target_a, rec->group[2].axis, t);
        colour[3] = fn_802B2978(rec->group[2].target_b, rec->group[2].axis, t);
        colour[4] = fn_802B2978(rec->group[1].target_a, rec->group[1].axis, t);
        colour[5] = fn_802B2978(rec->group[1].target_b, rec->group[1].axis, t);
    } else if (delta < 0xC000U) {
        t = (f32)(s32)(delta - 0x8000) / lbl_8079A49C;
        colour[0] = fn_802B2978(rec->group[0].axis, rec->group[0].target_b, t);
        colour[1] = fn_802B2978(rec->group[0].axis, rec->group[0].target_a, t);
        colour[2] = fn_802B2978(rec->group[2].axis, rec->group[2].target_b, t);
        colour[3] = fn_802B2978(rec->group[2].axis, rec->group[2].target_a, t);
        colour[4] = fn_802B2978(rec->group[1].axis, rec->group[1].target_b, t);
        colour[5] = fn_802B2978(rec->group[1].axis, rec->group[1].target_a, t);
    } else {
        t = (f32)(s32)(delta - 0xC000) / lbl_8079A49C;
        colour[0] = fn_802B2978(rec->group[0].target_b, rec->group[0].base, t);
        colour[1] = fn_802B2978(rec->group[0].target_a, rec->group[0].base, t);
        colour[2] = fn_802B2978(rec->group[2].target_b, rec->group[2].base, t);
        colour[3] = fn_802B2978(rec->group[2].target_a, rec->group[2].base, t);
        colour[4] = fn_802B2978(rec->group[1].target_b, rec->group[1].base, t);
        colour[5] = fn_802B2978(rec->group[1].target_a, rec->group[1].base, t);
    }

    if (rec->enabled == 0) {
        colour[4] = 0;
        colour[5] = 0;
    }
    if (store == 0) {
        fn_80057DE0(0, 6, colour, rec->se_frame);
        return;
    }
    fn_80057EF4(0, 6, colour, rec->se_frame);
}

/* Copies the current area's staged colour record out of the stage block and drives it. */
extern "C" void fn_802B2E2C(void)
{
    StageRuntime* st = (StageRuntime*)stage_w;
    StageColourRec rec;
    s32 handle;

    handle = fn_80082C80(pRoot, 0);
    fn_802AE250(&rec, &handle);
    fn_8007A1B8(&rec, 0, &rec.flags, &rec.b, 0, 0, &rec.a);

    if (fn_800CF208() == 2) {
        if (fn_8021F238() == 0) {
            fn_802B2F3C(&rec, &st->area[st->areano].colour_a);
        } else {
            fn_802B2F3C(&rec, &st->area[st->areano].colour_b);
        }
        fn_802AE1DC(&rec);
        return;
    }
    if (fn_802AFF38() == 0) {
        return;
    }
    if (fn_802BE088() == 1U) {
        fn_802B2F3C(&rec, &st->area[st->areano].colour_b);
    } else {
        fn_802B2F3C(&rec, &st->area[st->areano].colour_a);
    }
    fn_802AE1DC(&rec);
}

/* Copies the 0x10-byte colour record at `src` into `dst`. */
extern "C" void fn_802B2F3C(StageColourRec* dst, const StageColourRec* src)
{
    dst->colour = src->colour;
    dst->a = src->a;
    dst->b = src->b;
    dst->flags = src->flags;
}

/* Builds the current area's visibility set from the map/area tables and hides every joint that is not
 * in it. */
extern "C" void fn_802B2F60(StageRuntime* st, u8 kind)
{
    nw4r::math::VEC3 poly;
    nw4r::math::VEC3 cam;
    MHchar* chr;
    StageJointLists* list;
    u8* keep;
    u8* drop;
    u8 group;
    u8 changed;
    u8 found;
    u8 i;
    u8 j;

    list = NULL;
    group = 0;
    changed = 0;
    found = 0;
    chr = (MHchar*)st->area_char[kind];
    VEC3_ctor(&poly);

    if (kind == 1) {
        if (lbl_805CF644[st->mapno] != NULL) {
            list = lbl_805CF644[st->mapno] + st->areano;
        }
    } else if (kind == 0) {
        switch (st->mapno) {
        case 1:
        case 12:
            if (st->areano == 7) {
                list = lbl_80792310;
            } else if (st->areano == 0xC) {
                list = lbl_80792330;
            }
            break;
        case 3:
        case 14:
            list = lbl_805CF60C[st->areano];
            break;
        case 9:
            list = lbl_807923C8[st->areano];
            group = 1;
            break;
        }
    }

    if (list == NULL) {
        return;
    }
    if (group == 1U && kind == 0) {
        copyVec3(&poly, &get_camera_pos());
        group = my_player_no();
        if (fn_802B0688(&poly) == 1U) {
            keep = list->show;
            drop = list->hide;
            found = 1;
        } else {
            keep = list->hide;
            drop = list->show;
            found = 2;
        }
        if (st->joint_state[kind] != st->joint_state[kind + 4]) {
            changed = 1;
        }
    } else {
        if (get_camera_pos().z == lbl_8079A448) {
            keep = list->show;
            drop = list->hide;
            found = 1;
        } else {
            keep = list->hide;
            drop = list->show;
            found = 2;
        }
        if (found != st->joint_state[kind]) {
            changed = 1;
        }
    }

    fn_802B0B7C(st, st->areano);

    if (changed != 0) {
        for (;;) {
            if (*keep == 0xFF) {
                break;
            }
            for (j = 0; j < 8; j++) {
                if (st->field_0x2EAD[j] == 0xFF) {
                    chr->setVisibility(1, 1);
                    break;
                }
                if (*keep == st->field_0x2EAD[j]) {
                    break;
                }
            }
            keep++;
        }
        for (;;) {
            if (*drop == 0xFF) {
                break;
            }
            chr->setVisibility(1, 0);
            drop++;
        }
    }
    st->joint_state[group * 4 + kind] = found;
    (void)i;
}

/* Tail-calls the colour/visibility driver for the map's area animation set 0. */
extern "C" void fn_802B3250(void)
{
    fn_802B2F60((StageRuntime*)stage_w, 0);
}

/* Tail-calls the colour/visibility driver for the map's area animation set 1. */
extern "C" void fn_802B3260(void)
{
    fn_802B2F60((StageRuntime*)stage_w, 1);
}

/* Builds the current area's joint visibility set for the map's animation mode and applies it to the
 * area's character: one list of joint ids is shown, the other hidden. */
extern "C" void fn_802B3270(StageRuntime* st, MHchar* chr, u8 mode)
{
    StageJointLists local;
    u8 hide_buf[0x24];
    u8 show_buf[0x24];
    StageJointLists* lists = NULL;
    s32 flag = 0;
    u8 hide_i = 0;
    u8 show_i = 0;

    switch (st->mapno) {
    case 1:
    case 12: {
        u8 area = st->areano;

        lists = lbl_805CF6A0[area];
        switch (area) {
        case 0:
            if (mode == 1) {
                return;
            }
            if (fn_803AAF88() == 1) {
                flag = 1;
            }
            break;
        case 1:
            if (fn_803AAFE0() == 1) {
                flag = 1;
            }
            break;
        case 6:
            if (mode == 0) {
                return;
            }
            if (mode == 1) {
                flag = 1;
            }
            break;
        }
        break;
    }
    case 8:
    case 19:
        if (st->areano == 0) {
            lists = &lbl_80792410;
            flag = 0;
        }
        break;
    case 11:
    case 20:
        if (st->areano == 0) {
            lists = &lbl_80792410;
            flag = 1;
        }
        break;
    case 22: {
        u8 area = st->areano;

        if (area == 0) {
            const u8* src;

            lists = &local;
            local.hide = hide_buf;
            local.show = show_buf;
            if (fn_802FB97C() == 1) {
                src = lbl_80792460;
            } else {
                src = lbl_80792464;
            }
            while (*src != 0xFF) {
                local.show[show_i] = *src;
                src++;
                show_i++;
            }
            if (fn_8021F238() == 0) {
                local.show[show_i] = 1;
                show_i++;
            }
            flag = 1;
            local.hide[hide_i] = 0xFF;
            local.show[show_i] = 0xFF;
        } else if (area == 1) {
            lists = &lbl_80792470;
            if (fn_8021F238() == 1) {
                flag = 1;
            }
        } else if (area == 2) {
            const u8* src;

            lists = &local;
            local.hide = hide_buf;
            local.show = show_buf;
            switch (fn_802FB8EC(0)) {
            case 0:
            case 1:
                local.show[0] = 3;
                local.show[1] = 4;
                show_i = 2;
                break;
            case 2:
                local.show[0] = 1;
                local.show[1] = 4;
                show_i = 2;
                break;
            case 3:
                local.show[0] = 1;
                local.show[1] = 2;
                show_i = 2;
                break;
            }
            switch (fn_802FB8EC(2)) {
            case 0:
                local.show[show_i] = 8;
                show_i++;
                local.show[show_i] = 9;
                show_i++;
                local.show[show_i] = 10;
                show_i++;
                break;
            case 1:
                local.show[show_i] = 9;
                show_i++;
                local.show[show_i] = 10;
                show_i++;
                break;
            case 2:
                local.show[show_i] = 10;
                show_i++;
                break;
            }
            switch (fn_802FB8EC(3)) {
            case 0:
                local.show[show_i] = 11;
                show_i++;
                local.show[show_i] = 12;
                show_i++;
                local.show[show_i] = 13;
                show_i++;
                break;
            case 1:
                local.show[show_i] = 12;
                show_i++;
                local.show[show_i] = 13;
                show_i++;
                break;
            case 2:
                local.show[show_i] = 13;
                show_i++;
                break;
            }
            switch (fn_802FB8EC(1)) {
            case 0:
            case 1:
                local.show[show_i] = 14;
                show_i++;
                local.show[show_i] = 15;
                show_i++;
                break;
            case 2:
                local.show[show_i] = 15;
                show_i++;
                break;
            }
            if (fn_802FB8C4() == 0) {
                local.show[show_i] = 6;
                show_i++;
            } else {
                local.show[show_i] = 5;
                show_i++;
            }
            if (fn_802FB900() == 1) {
                local.show[show_i] = 0x10;
                show_i++;
            }
            if (fn_8021F238() == 0) {
                local.show[show_i] = 7;
                show_i++;
            }
            flag = 1;
            local.hide[hide_i] = 0xFF;
            local.show[show_i] = 0xFF;
        }
        break;
    }
    case 21: {
        u8 area = st->areano;

        switch (area) {
        case 0:
            lists = &local;
            local.hide = hide_buf;
            local.show = show_buf;
            if (fn_8021F238() == 1) {
                const u8* src;

                src = lbl_805CF6D4;
                while (*src != 0xFF) {
                    local.show[show_i] = *src;
                    src++;
                    show_i++;
                }
                src = lbl_80792418;
                while (*src != 0xFF) {
                    local.hide[hide_i] = *src;
                    src++;
                    hide_i++;
                }
                if (LbCheckKujiraEvent() == 0 && fn_802FB9F8() == 0) {
                    local.show[show_i] = 0x12;
                    show_i++;
                    local.show[show_i] = 0x13;
                    show_i++;
                }
            } else {
                const u8* src;

                src = lbl_80792418;
                while (*src != 0xFF) {
                    local.show[show_i] = *src;
                    src++;
                    show_i++;
                }
                src = lbl_805CF6D4;
                while (*src != 0xFF) {
                    local.hide[hide_i] = *src;
                    src++;
                    hide_i++;
                }
                if (LbCheckKujiraEvent() == 0 && fn_802FB9F8() == 0) {
                    local.show[show_i] = 0x10;
                    show_i++;
                    local.show[show_i] = 0x11;
                    show_i++;
                }
            }
            flag = 1;
            local.hide[hide_i] = 0xFF;
            local.show[show_i] = 0xFF;
            break;
        case 1:
            lists = &local;
            local.hide = hide_buf;
            local.show = show_buf;
            if (fn_8021F238() == 1) {
                const u8* src;

                src = lbl_805CF6E8;
                while (*src != 0xFF) {
                    local.show[show_i] = *src;
                    src++;
                    show_i++;
                }
                src = lbl_8079241C;
                while (*src != 0xFF) {
                    local.hide[hide_i] = *src;
                    src++;
                    hide_i++;
                }
                if (LbCheckKujiraEvent() == 0 && fn_802FB9F8() == 0) {
                    local.show[show_i] = 0x11;
                    show_i++;
                    local.show[show_i] = 0x12;
                    show_i++;
                }
            } else {
                const u8* src;

                src = lbl_8079241C;
                while (*src != 0xFF) {
                    local.show[show_i] = *src;
                    src++;
                    show_i++;
                }
                src = lbl_805CF6E8;
                while (*src != 0xFF) {
                    local.hide[hide_i] = *src;
                    src++;
                    hide_i++;
                }
                if (LbCheckKujiraEvent() == 0 && fn_802FB9F8() == 0) {
                    local.show[show_i] = 0x0F;
                    show_i++;
                    local.show[show_i] = 0x10;
                    show_i++;
                }
            }
            flag = 1;
            local.hide[hide_i] = 0xFF;
            local.show[show_i] = 0xFF;
            break;
        case 2:
            lists = &local;
            local.hide = hide_buf;
            local.show = show_buf;
            if (fn_8021F238() == 1) {
                const u8* src;

                src = lbl_805CF6F4;
                while (*src != 0xFF) {
                    local.show[show_i] = *src;
                    src++;
                    show_i++;
                }
                src = lbl_80792420;
                while (*src != 0xFF) {
                    local.hide[hide_i] = *src;
                    src++;
                    hide_i++;
                }
                if (LbCheckKujiraEvent() == 0 && fn_802FB9F8() == 0) {
                    local.show[show_i] = 4;
                    show_i++;
                    local.show[show_i] = 0x0C;
                    show_i++;
                    local.show[show_i] = 0x0D;
                    show_i++;
                } else if (fn_802FB9F8() == 1) {
                    local.show[show_i] = 4;
                    show_i++;
                }
            } else {
                const u8* src;

                src = lbl_80792420;
                while (*src != 0xFF) {
                    local.show[show_i] = *src;
                    src++;
                    show_i++;
                }
                src = lbl_805CF6F4;
                while (*src != 0xFF) {
                    local.hide[hide_i] = *src;
                    src++;
                    hide_i++;
                }
                if (LbCheckKujiraEvent() == 0 && fn_802FB9F8() == 0) {
                    local.show[show_i] = 2;
                    show_i++;
                    local.show[show_i] = 0x0A;
                    show_i++;
                    local.show[show_i] = 0x0B;
                    show_i++;
                } else if (fn_802FB9F8() == 1) {
                    local.show[show_i] = 2;
                    show_i++;
                }
            }
            flag = 1;
            local.hide[hide_i] = 0xFF;
            local.show[show_i] = 0xFF;
            break;
        case 3:
            lists = &local;
            local.hide = hide_buf;
            local.show = show_buf;
            if (fn_8021F238() == 1) {
                const u8* src;

                src = lbl_80792428;
                while (*src != 0xFF) {
                    local.show[show_i] = *src;
                    src++;
                    show_i++;
                }
                src = lbl_80792430;
                while (*src != 0xFF) {
                    local.hide[hide_i] = *src;
                    src++;
                    hide_i++;
                }
                if (LbCheckKujiraEvent() == 0 && fn_802FB9F8() == 0) {
                    local.show[show_i] = 7;
                    show_i++;
                    local.show[show_i] = 8;
                    show_i++;
                } else {
                    local.show[show_i] = 4;
                    show_i++;
                }
            } else {
                const u8* src;

                src = lbl_80792430;
                while (*src != 0xFF) {
                    local.show[show_i] = *src;
                    src++;
                    show_i++;
                }
                src = lbl_80792428;
                while (*src != 0xFF) {
                    local.hide[hide_i] = *src;
                    src++;
                    hide_i++;
                }
                if (LbCheckKujiraEvent() == 0 && fn_802FB9F8() == 0) {
                    local.show[show_i] = 5;
                    show_i++;
                    local.show[show_i] = 6;
                    show_i++;
                } else {
                    local.show[show_i] = 3;
                    show_i++;
                }
            }
            flag = 1;
            local.hide[hide_i] = 0xFF;
            local.show[show_i] = 0xFF;
            break;
        case 5:
            lists = &lbl_80792440;
            if (fn_8021F238() == 1) {
                flag = 1;
            }
            break;
        case 6:
            lists = &lbl_80792450;
            if (fn_8021F238() == 1) {
                flag = 1;
            }
            break;
        case 7:
            lists = &local;
            local.hide = hide_buf;
            local.show = show_buf;
            if (fn_8021F238() == 1) {
                const u8* src;

                src = lbl_805CF700;
                while (*src != 0xFF) {
                    local.show[show_i] = *src;
                    src++;
                    show_i++;
                }
                src = lbl_80792458;
                while (*src != 0xFF) {
                    local.hide[hide_i] = *src;
                    src++;
                    hide_i++;
                }
                if (LbCheckKujiraEvent() == 0 && fn_802FB9F8() == 0) {
                    local.show[show_i] = 0x19;
                    show_i++;
                    local.show[show_i] = 0x1D;
                    show_i++;
                    local.show[show_i] = 0x1E;
                    show_i++;
                    local.show[show_i] = 0x23;
                    show_i++;
                } else {
                    if (fn_802FB9F8() == 1) {
                        local.show[show_i] = 0x19;
                        show_i++;
                    }
                    local.show[show_i] = 0x17;
                    show_i++;
                    local.show[show_i] = 0x18;
                    show_i++;
                    local.show[show_i] = 0x1F;
                    show_i++;
                }
            } else {
                const u8* src;

                src = lbl_80792458;
                while (*src != 0xFF) {
                    local.show[show_i] = *src;
                    src++;
                    show_i++;
                }
                src = lbl_805CF700;
                while (*src != 0xFF) {
                    local.hide[hide_i] = *src;
                    src++;
                    hide_i++;
                }
                if (LbCheckKujiraEvent() == 0 && fn_802FB9F8() == 0) {
                    local.show[show_i] = 5;
                    show_i++;
                    local.show[show_i] = 0x1B;
                    show_i++;
                    local.show[show_i] = 0x1C;
                    show_i++;
                    local.show[show_i] = 0x22;
                    show_i++;
                } else {
                    if (fn_802FB9F8() == 1) {
                        local.show[show_i] = 5;
                        show_i++;
                    }
                    local.show[show_i] = 4;
                    show_i++;
                    local.show[show_i] = 0x16;
                    show_i++;
                    local.show[show_i] = 0x1A;
                    show_i++;
                }
            }
            flag = 1;
            local.hide[hide_i] = 0xFF;
            local.show[show_i] = 0xFF;
            break;
        }
        break;
    }
    }
    if (lists != NULL) {
        const u8* shown;
        const u8* hidden;

        if (flag != 0) {
            shown = lists->hide;
            hidden = lists->show;
        } else {
            shown = lists->show;
            hidden = lists->hide;
        }
        while (*shown != 0xFF) {
            chr->setVisibility(*shown, true);
            shown++;
        }
        while (*hidden != 0xFF) {
            chr->setVisibility(*hidden, false);
            hidden++;
        }
    }
}

/* Hides every joint the current area's kind list names. */
extern "C" void fn_802B40D8(StageRuntime* st, MHchar* chr)
{
    u8* p;

    chr->get_joint_num();
    p = fn_802B04A0(st->areano);
    if (p == NULL) {
        return;
    }
    while (*p != 0xFF) {
        chr->setVisibility(0, 0);
        p++;
    }
}

/* Spawns the `eft028` break effects along the map's effect strips. */
extern "C" void fn_802B414C(StageRuntime* st)
{
    nw4r::math::VEC3 pos;
    f32* strip;
    u8* slot;
    f32 limit;
    s32 i;

    VEC3_ctor(&pos);
    if (st->mapno != 0xA || st->areano != 1) {
        return;
    }
    strip = lbl_805CF784;
    slot = lbl_805CF7C0;
    limit = lbl_8079A448;
    for (;;) {
        if (*strip <= limit) {
            return;
        }
        i = 0;
        for (;;) {
            if ((f32)i >= strip[2] / strip[1]) {
                break;
            }
            if (fn_800E16DC((void*)st->area_char[0], 0, 0, *strip + (f32)i * strip[1], lbl_8079A448) != 0) {
                vec_to_mh_vec3(&pos, (struct Vec*)slot);
                eft028_set_koware(0xB, &pos, st->areano, 0);
            }
            i++;
        }
        strip += 3;
        slot += 0x0C;
    }
}

/* Spawns the map/area's fixed break effects and sound. */
extern "C" void fn_802B428C(StageRuntime* st)
{
    nw4r::math::VEC3 pos;
    u8 mapno;
    u8 areano;

    VEC3_ctor(&pos);
    fn_802B414C(st);
    mapno = st->mapno;
    if (mapno != 5 && mapno != 0x10) {
        return;
    }
    areano = st->areano;
    switch (areano) {
    case 9:
        if ((s32)(system_w.field_0x0c % 170) == 0) {
            setVector3(&pos, lbl_8079A4A0, lbl_8079A4A4, lbl_8079A4A8);
            eft028_set_koware(6, &pos, st->areano, 0);
            return;
        }
        break;
    case 10:
        if ((s32)(system_w.field_0x0c % 160) == 0) {
            setVector3(&pos, lbl_8079A4AC, lbl_8079A4B0, lbl_8079A4B4);
            eft028_set_koware(6, &pos, st->areano, 0);
            fn_802BE4FC(0x43);
            fn_800DD38C();
        }
        break;
    }
}

/* Seats the map/area's joint effects on the `shell_set_func_ptr` job table. */
extern "C" void fn_802B43A8(MHchar* chr, u8 kind, u16 flags)
{
    nw4r::math::VEC3 pos;
    s8 order[3];
    f32* thresholds;
    u8* strip;
    u8** joint_tables;
    u8* joints;
    f32 height;
    s32 band;
    u8 index;
    u8 mapno;

    VEC3_ctor(&pos);
    strip = NULL;
    mapno = get_now_mapno();
    if ((mapno == 4 || mapno == 0xF) && kind == 5) {
        strip = lbl_805CF7FC;
        thresholds = lbl_805CF7F0;
        joint_tables = (flags & 1) ? lbl_805CF8E0 : lbl_805CF8F0;
    }
    if (strip == NULL) {
        return;
    }
    band = 0;
    height = chr->pos_0x04.y;
    if (!(height < thresholds[0])) {
        band = 1;
        if (!(height < thresholds[1])) {
            band = 2;
            if (!(height < thresholds[2])) {
                band = 3;
            }
        }
    }
    joints = joint_tables[band];
    index = (s8)(u8)(flags % 3);
    order[0] = index;
    if ((flags & 0x10) != 0) {
        order[1] = index + 1;
        if (order[1] >= 3) {
            order[1] = 0;
        }
        order[2] = order[1] + 1;
        if (order[2] >= 3) {
            order[2] = 0;
        }
    } else {
        order[1] = index - 1;
        if (order[1] < 0) {
            order[1] = 2;
        }
        order[2] = order[1] - 1;
        if (order[2] < 0) {
            order[2] = 2;
        }
    }
    index = 0;
    while ((s8)*joints > 0) {
        vec_to_mh_vec3(&pos, (struct Vec*)(strip + (s8)*joints * 0xC));
        shell_set_func_ptr->method_0x88(&pos, kind, (s16)((s8)order[index] * 0xA), shell_set_func_ptr);
        joints++;
        index++;
    }
}

/* Raises the stage block's one-shot flag byte. */
extern "C" void fn_802B45BC(void)
{
    ((StageRuntime*)stage_w)->field_0x2F91 |= 0x81;
}

/* Returns whether the stage block's one-shot flag byte has bit 0 clear. */
extern "C" s32 fn_802B45D4(void)
{
    return (((StageRuntime*)stage_w)->field_0x2F91 & 1) != 0;
}

/* Arms area `index`'s timer on first use and keeps its 4-second window. */
extern "C" void fn_802B45F4(u8 index)
{
    StageRuntime* st = (StageRuntime*)stage_w;
    u16 bit = 1 << index;
    s32 now;

    if ((st->field_0x2F92 & bit) == 0) {
        st->field_0x2F92 |= bit;
        fn_802FBA94();
    }
    now = fn_803A8858();
    st->field_0x2F94[index] = now - fn_803A87E0();
}

/* Tests area bit `index` of the stage block's 16-bit mask and reports whether it is clear. */
extern "C" s32 fn_802B46DC(u8 index)
{
    return (((StageRuntime*)stage_w)->field_0x2F92 & (1 << index)) == 0;
}

/* Dispatches an area-index event to either the local stage or the caller's own handler. */
extern "C" void fn_802B4680(void* plw, u8 index)
{
    if (index >= 0x10U) {
        return;
    }
    if (plw == NULL) {
        fn_802B45F4(index);
        return;
    }
    if (Pl_master_ck((struct _PLW*)plw) == 1U) {
        lb_sub12_send(index);
    }
}

/* Ages the stage block's per-area timers and clears the bits whose 4-second window has lapsed. */
extern "C" void fn_802B4704(StageRuntime* st)
{
    s32 now;
    s32 span;
    u8 index;

    now = fn_803A8858();
    span = now - fn_803A87E0();
    if (fn_8027BC48(NULL) == 1U) {
        return;
    }
    for (index = 0; index < 0x10U; index++) {
        if (fn_802B46DC(index) == 0) {
            if (span - (s32)st->field_0x2F94[index] >= 0x2A30) {
                st->field_0x2F92 ^= 1 << index;
                st->field_0x2F94[index] = 0;
            }
        }
    }
}

/* Reports whether every area from 1 to 4 is armed for the current map's stage kind 4. */
extern "C" s32 fn_802B47B0(void)
{
    StageRuntime* st = (StageRuntime*)stage_w;
    u8 index;

    if (fn_802B0668(st->mapno) != 4) {
        return 0;
    }
    index = 1;
    while (index < 5U) {
        if (fn_802B46DC(index) == 1U) {
            return 0;
        }
        index++;
    }
    return 1;
}

/* Re-drives every live area actor's per-joint frame init for the palette the kind selects. */
extern "C" void fn_802B4824(u8 kind)
{
    f32 scale;
    f32 amount;
    u8 index;
    u8 joint;
    StageRuntime* st = (StageRuntime*)stage_w;

    switch (kind) {
    case 18:
        amount = lbl_8079A4B8;
        break;
    case 33:
        fn_802B4680(NULL, 0);
        return;
    case 47:
        amount = lbl_8079A4BC;
        break;
    default:
        return;
    }
    index = 0;
    scale = lbl_8079A484;
    for (; index < 4U; index++) {
        if (st->area_char[index] != NULL) {
            for (joint = 0; joint < ((StageActorJoints*)st->area_char[index])->joint_count; joint++) {
                st->area_char[index]->frame_init(joint, (u16)joint, amount * scale, 0, lbl_8079A468);
            }
        }
    }
}

/* Arms the map's joint sound effects for the current area. */
extern "C" void fn_802B493C(StageRuntime* st)
{
    nw4r::math::VEC3 pos;
    MHchar* chr;
    u8* list;
    u8* p;
    s32 sound;
    s32 target;

    VEC3_ctor(&pos);
    list = NULL;
    sound = 0;
    target = 0;
    switch (st->mapno) {
    case 21:
        if (fn_8021F238() == 0) {
            switch (st->areano) {
            case 0:
                list = lbl_80792498;
                sound = 1;
                break;
            case 7:
                list = lbl_8079249C;
                break;
            }
        }
        break;
    case 8:
    case 11:
        if (st->areano == 0) {
            list = lbl_807924A0;
        }
        break;
    case 9:
        if (st->areano == 0) {
            list = lbl_807924A4;
        }
        break;
    case 10:
        if (st->areano == 1) {
            list = lbl_807924A8;
            sound = 0;
            target = 1;
        }
        break;
    }
    if (list == NULL) {
        return;
    }
    chr = st->area_char[0];
    for (p = list; (s8)*p > 0; p++) {
        chr->get_joint_wpos((s8)*p, &pos);
        switch (target) {
        case 0:
            shell_se_req(NULL, &pos, 0x10, sound);
            break;
        case 1:
            shell_se_req(NULL, &pos, 0x11, sound);
            break;
        }
    }
}

/* Places the map's demo marker objects on their scripted offsets. */
extern "C" void fn_802B4ABC(StageRuntime* st)
{
    nw4r::math::VEC3 pos;
    StageDemoEntry* entry;
    StageDemoEntry* p;
    int mapno;
    u16 secs;
    s32 now;
    s32 base;

    VEC3_ctor(&pos);
    entry = NULL;
    mapno = get_now_mapno();
    if (mapno != 5 && mapno != 0x10 && mapno != 0xA) {
        return;
    }
    now = fn_803A8858();
    base = fn_803A87E0();
    secs = (u16)((now - base) / 300);
    secs += fn_803A8F60(0);
    if (--st->field_0x2FDC > 0) {
        return;
    }
    if (((now - base) & 0x40) == 0x40) {
        st->field_0x2FDC = 0;
        return;
    }
    if (((now - base) & 0x30) != 0) {
        st->field_0x2FDC = 0;
        return;
    }
    if (event_demo_ck() == 1U) {
        return;
    }
    for (p = lbl_805CFA38; p->mapno != 0xFF; p++) {
        if (p->mapno == get_now_mapno()) {
            entry = (StageDemoEntry*)p->offsets;
            break;
        }
    }
    if (entry == NULL) {
        return;
    }
    for (;;) {
        if (entry->mapno == 0xFF) {
            break;
        }
        vec_to_mh_vec3(&pos, (struct Vec*)(entry->offsets + (secs % entry->count) * 0xC));
        shell_set_func_ptr->method_0x80(&pos, entry->mapno, 0x8C, shell_set_func_ptr);
        entry++;
    }
    st->field_0x2FDC = 0x12C;
}

/* Re-drives every live area object's own per-frame update. */
extern "C" void fn_802B4C5C(void)
{
    StageRuntime* st = (StageRuntime*)stage_w;
    u8 index;

    if (fn_800CF208() == 1) {
        return;
    }
    index = 0;
    do {
        if (st->area_char[index] != NULL) {
            fn_802B3270(st, st->area_char[index], index);
        }
        index++;
    } while (index < 4U);
}

/* Returns the blend table for the map's current area group. */
extern "C" StageBlendEntry* fn_802B4CD0(void)
{
    StageBlendEntry* table;
    u8 areano;

    areano = get_now_areano();
    if ((u32)(areano - 5) <= 2U) {
        return lbl_805CFAE8;
    }
    table = lbl_805CFA68;
    if (areano == 4) {
        table = lbl_805CFB68;
    }
    return table;
}

/* Blends the two areas the 30-second colour cycle straddles into a wrapped 0..1 value. */
extern "C" f32 fn_802B4D24(s16 index, f32 elapsed)
{
    StageBlendEntry* table;
    u32 kind;
    f32 value;

    table = fn_802B4CD0();
    kind = *(u32*)((u8*)table + index * 0x10);
    if (kind < 4U) {
        value = lbl_805CFA58[kind];
    } else if (kind == 5) {
        if (index >= 7) {
            value = lbl_805CFA58[table->kind] - elapsed;
        } else {
            value = lbl_805CFA58[*(u32*)((u8*)table + (index + 1) * 0x10)] - elapsed;
        }
    } else if (index == 0) {
        value = elapsed + lbl_805CFA58[table[1].colour_c];
    } else {
        value = elapsed + lbl_805CFA58[*(u32*)((u8*)table + (index - 1) * 0x10)];
    }
    if (value < lbl_8079A448) {
        return value + lbl_8079A468;
    }
    if (value > lbl_8079A468) {
        value -= lbl_8079A468;
    }
    return value;
}

/* Interpolates the two areas' colour records by the cycle weight and arms the result. */
extern "C" void fn_802B4E58(void)
{
    StageBlendEntry* table;
    u32 colour[6];
    u32 from;
    u32 to;
    f32 peak;
    f32 step;
    f32 best;
    f32 prev;
    f32 frac;
    f32 span;
    f32 weight;
    f32 fa;
    f32 fb;
    f32 fc;
    f32 fd;
    f32 fe;
    f32 ff;
    f32 fg;
    f32 fh;
    u32 count;
    s16 low;
    s16 high;
    s16 i;

    count = fn_8021F218();
    peak = fn_8021F228(count);
    table = fn_802B4CD0();
    step = lbl_8079A4C0 / (f32)count;
    low = 7;
    high = 8;
    prev = 0.0f;
    best = 0.0f;
    for (i = 0; i < 8; i++) {
        best = fn_802B4D24(i, step);
        if (best > peak) {
            high = i;
            break;
        }
        low = i;
        prev = best;
    }
    if (high == 0) {
        prev = fn_802B4D24(low, step);
    }
    if (high >= 8) {
        high = 0;
        best = fn_802B4D24(0, step);
    }
    if (high <= 0) {
        span = (lbl_8079A468 - prev) + best;
        frac = peak - prev;
        if (frac < lbl_8079A448) {
            frac += lbl_8079A468;
        }
    } else {
        span = best - prev;
        frac = peak - prev;
    }
    weight = frac / span;

    from = table[low].colour_a;
    to = table[high].colour_a;
    fa = (f32)(from >> 24);
    fb = (f32)(u8)(from >> 16);
    fc = (f32)(u8)(from >> 8);
    fd = (f32)(u8)from;
    colour[0] = (u8)(s32)(fd + weight * ((f32)(u8)to - fd))
              | ((u8)(s32)(fc + weight * ((f32)(u8)(to >> 8) - fc)) << 8)
              | ((u8)(s32)(fa + weight * ((f32)(to >> 24) - fa)) << 24)
              | ((u8)(s32)(fb + weight * ((f32)(u8)(to >> 16) - fb)) << 16);
    colour[1] = colour[0];

    from = table[low].colour_b;
    to = table[high].colour_b;
    fe = (f32)from;
    ff = (f32)(u8)(from >> 16);
    fg = (f32)(u8)(from >> 8);
    fh = (f32)(u8)from;
    colour[2] = (u8)(s32)(fh + weight * ((f32)(u8)to - fh))
              | ((u8)(s32)(fg + weight * ((f32)(u8)(to >> 8) - fg)) << 8)
              | ((u8)(s32)(fe + weight * ((f32)(to >> 24) - fe)) << 24)
              | ((u8)(s32)(ff + weight * ((f32)(u8)(to >> 16) - ff)) << 16);
    colour[3] = colour[2];

    from = table[low].colour_c;
    to = table[high].colour_c;
    colour[4] = (u8)(s32)((f32)(u8)from + weight * ((f32)(u8)to - (f32)(u8)from))
              | ((u8)(s32)((f32)(u8)(from >> 8) + weight * ((f32)(u8)(to >> 8) - (f32)(u8)(from >> 8))) << 8)
              | ((u8)(s32)((f32)(from >> 24) + weight * ((f32)(to >> 24) - (f32)(from >> 24))) << 24)
              | ((u8)(s32)((f32)(u8)(from >> 16) + weight * ((f32)(u8)(to >> 16) - (f32)(u8)(from >> 16))) << 16);
    colour[5] = colour[4];

    fn_80057DE0(0, 6, colour, lbl_8079A4C4);
}

/* Seeds the three per-area effect seats and the stage block's first area record. */
extern "C" void fn_802B535C(void)
{
    fn_802B53CC((u32)stage_w);
    /* Each seat is its own `lbl_` symbol (0xC apart); a `VEC3[]` view would fold them into one
     * relocation (measured 100 -> 80.86 on fn_802B535C). */
    setVec3((nw4r::math::VEC3*)lbl_806BB7B8, lbl_8079A448, lbl_8079A448, lbl_8079A448);
    setVec3((nw4r::math::VEC3*)lbl_806BB7C4, lbl_8079A4C8, lbl_8079A4CC, lbl_8079A4D0);
    setVec3((nw4r::math::VEC3*)lbl_806BB7D0, lbl_8079A448, lbl_8079A448, lbl_8079A448);
}

/* Runs the stage block's cleanup over every sub-block array it owns. */
extern "C" u32 fn_802B53CC(u32 base)
{
    u32 p;

    p = base + 0x24;
    do {
        fn_802B55E8(p);
        p += 0x5D0;
    } while (p < base + 0xBC4);
    p = base + 0xC08;
    VEC3_ctor((Vec3*)p);
    p = base + 0xC20;
    do {
        fn_802B5538(p);
        p += 0x1DC;
    } while (p < base + 0x2804);
    p = base + 0x2808;
    VEC3_ctor((Vec3*)p);
    p = base + 0x2C2C;
    do {
        fn_802B54C4(p);
        p += 0xA0;
    } while (p < base + 0x2EAC);
    p = base + 0x2EC0;
    do {
        fn_802B5488(p);
        p += 0x34;
    } while (p < base + 0x2F90);
    return base;
}

/* Clears the two 12-byte vector pairs inside one 0x34-byte sub-block. */
extern "C" u32 fn_802B5488(u32 block)
{
    u32 p;

    p = block + 4;
    fn_800FA420((Vec3*)p);
    p = block + 0x14;
    fn_800FA3B8((Vec3*)p);
    return block;
}

/* Clears the 12-byte records of one 0xA0-byte sub-block. */
extern "C" u32 fn_802B54C4(u32 block)
{
    u32 p;

    p = block + 4;
    do {
        VEC3_ctor((Vec3*)p);
        p += 0xC;
    } while (p < block + 0x28);
    p = block + 0x4C;
    do {
        VEC3_ctor((Vec3*)p);
        p += 0xC;
    } while (p < block + 0x70);
    return block;
}

/* Clears the 0x30-byte slots of one 0x1DC-byte area record. */
extern "C" u32 fn_802B5538(u32 rec)
{
    u32 p;

    p = rec + 4;
    do {
        fn_802B5590(p);
        p += 0x30;
    } while (p < rec + 0x124);
    return rec;
}

/* Clears the 12-byte records inside one 0x30-byte slot. */
extern "C" u32 fn_802B5590(u32 slot)
{
    u32 p;

    p = slot + 0xC;
    do {
        VEC3_ctor((Vec3*)p);
        p += 0xC;
    } while (p < slot + 0x30);
    return slot;
}

/* Clears the 0x164-byte records of one 0x5D0-byte sub-block. */
extern "C" u32 fn_802B55E8(u32 block)
{
    u32 p;

    p = block + 4;
    do {
        fn_801FF984((void*)p);
        p += 0x164;
    } while (p < block + 0x594);
    return block;
}

/* Resets one per-area object and runs its six sub-block initialisers. */
extern "C" void fn_802B5640(StageAreaObj* area, u8 index)
{
    area->field_0x498 = (u8)index;
    area->field_0x49A = 0;
    area->field_0x499 = 0;
    ((nw4r::g3d::ScnRoot*)pRoot)->SetCurrentCamera(0);
    fn_802B700C(area);
    fn_802B59F8(area);
    fn_802B95E8(area);
    fn_802BBEE0(area);
    fn_802BA25C(area);
    fn_802BB118(area);
    area->field_0x494 = NULL;
    area->field_0x4DC = 0;
    area->field_0x4C8 = 0;
    area->field_0x4B4 = 0;
    area->field_0x4F0 = 0;
    area->field_0x4E4 = 0;
    area->field_0x4E8 = 0;
    area->field_0x4F4 = 0;
    area->field_0x4F7 = 0;
}

/* Resets both per-area objects. */
extern "C" void fn_802B56E0(void)
{
    u8 index;

    index = 0;
    do {
        fn_802B5640((StageAreaObj*)(lbl_806BB7E0 + index * 0x4F8), index);
        index++;
    } while (index < 2U);
    fn_802BD658();
}

/* Runs one per-area object's per-frame update and resolves the area's kind. */
extern "C" void fn_802B5738(StageAreaObj* area)
{
    nw4r::math::VEC3 poly;
    nw4r::math::VEC3 world;
    nw4r::math::VEC3 pos;
    u32 move;
    u8 kind;

    VEC3_ctor(&poly);
    move = (s32)get_move_work_adrs(2);
    area->field_0x494 = (u32)move + my_player_no() * 0xB20;
    fn_802B7034(area);
    fn_802B5C58(area);
    fn_802B9828(area);
    fn_802B9C04(area);
    fn_802BC1E4(area);
    fn_802BA39C(area);
    fn_802BB7FC(area);
    area->field_0x49B = 0xFFU;
    if (area->field_0x284 == 1) {
        area->field_0x49B = 3U;
    } else if (area->field_0x188 == 1) {
        area->field_0x49B = 1U;
    } else if (area->field_0x230 == 1) {
        area->field_0x49B = 2U;
    } else if (area->field_0x43C == 1) {
        area->field_0x49B = 5U;
    } else if (area->field_0x388 == 1) {
        area->field_0x49B = 4U;
    }
    kind = area->field_0x49B;
    if (kind == 0xFF) {
        kind = area->field_0x499;
    }
    fn_800473F4(kind);
    fn_802B2E2C();
    fn_802BEAAC(area, kind);
    fn_802BE1EC(area);
    fn_802BDDB0(&pos);
    fn_80050CA0(&world, &get_camera_pos(), &pos);
    copyVec3(&poly, &world);
    if (fn_80047058() != 0) {
        setVector3(&poly, lbl_8079A4D8, lbl_8079A4DC, lbl_8079A4E0);
    }
    fn_802C20A4(&poly);
    fn_802B5980(area);
}

/* Runs one per-area object's update and, when the extra gate is open, the second object's too. */
extern "C" void fn_802B58D4(void)
{
    fn_802B5738((StageAreaObj*)lbl_806BB7E0);
    if (fn_80047058() != 0) {
        fn_800CF394(1);
        fn_802B5738((StageAreaObj*)(lbl_806BB7E0 + 0x4F8));
        fn_800CF394(0);
    }
}

/* Points the g3d scene root's camera at the current view. */
extern "C" void fn_802B592C(u8 index, s32 camera)
{
    nw4r::math::VEC3 pos;
    s32 handle;
    s32 root;

    handle = ((nw4r::g3d::ScnRoot*)pRoot)->GetCamera(index);
    fn_8004723C(&pos, &handle);
    fn_80067E70(&root);
    root = camera;
}

/* Places one per-area object's seat from the camera and the area's effect table. */
extern "C" void fn_802B5980(StageAreaObj* area)
{
    nw4r::math::VEC3 poly;
    nw4r::math::VEC3 cam;
    nw4r::math::VEC3 out;

    VEC3_ctor(&poly);
    copyVec3(&poly, &get_camera_pos());
    if (fn_80291BBC(&poly, get_now_areano(), (s32)&area->field_0x4F4, &out, 0xFFFF) == 0) {
        area->field_0x4F4 = 0;
    }
}

/* Seeds one per-area object's colour and joint state. */
extern "C" void fn_802B59F8(StageAreaObj* area)
{
    nw4r::math::VEC3 poly;
    nw4r::math::VEC3 world;
    u8 mode;

    VEC3_ctor(&poly);
    setVector3(&area->seat_pos, lbl_8079A4D8, lbl_8079A4D8, lbl_8079A4E4);
    setVector3(&area->seat_pos2, lbl_8079A4D8, lbl_8079A4D8, lbl_8079A4D8);
    area->span = lbl_8079A4E8;
    area->pitch = lbl_8079A4D8;
    fn_802B592C(0, 2);
    fn_802BE0F4(0, area, &area->seat_pos2, area->span, area->pitch);
    fn_80050CA0(&world, &area->seat_pos, &area->seat_pos2);
    copyVec3(&poly, &world);
    calcVecAngXY(&poly, &area->ang_x, &area->ang_y);
    area->field_0x06C = area->ang_x;
    area->field_0x070 = area->ang_y;
    mode = get_cfg(area->field_0x498, 3);
    switch (mode) {
    case 0:
        area->field_0x07C = 0;
        break;
    case 1:
        area->field_0x07C = 1;
        break;
    case 3:
        area->field_0x07C = 3;
        break;
    case 4:
        area->field_0x07C = 4;
        break;
    default:
        area->field_0x07C = 2;
        break;
    }
    area->field_0x092 = 1;
    area->field_0x080 = 0;
    area->field_0x074 = 0;
    area->field_0x078 = 0;
    setVector3(&area->field_0x0B8, lbl_8079A4D8, lbl_8079A4D8, lbl_8079A4D8);
    setVector3(&area->field_0x0C4, lbl_8079A4D8, lbl_8079A4D8, lbl_8079A4D8);
    setVector3(&area->field_0x0D0, lbl_8079A4D8, lbl_8079A4D8, lbl_8079A4D8);
    setVector3(&area->field_0x0DC, lbl_8079A4D8, lbl_8079A4D8, lbl_8079A4D8);
    setVector3(&area->field_0x0E8, lbl_8079A4D8, lbl_8079A4D8, lbl_8079A4D8);
    setVector3(&area->field_0x0F4, lbl_8079A4D8, lbl_8079A4D8, lbl_8079A4D8);
    area->field_0x0A0 = 0;
    area->field_0x0A1 = 0;
    area->field_0x0A2 = 0;
    area->field_0x0A3 = 0;
    area->field_0x0A5 = 0;
    area->field_0x0A6 = 0;
    area->field_0x0A4 = 0;
    area->field_0x0A8 = 0;
    area->field_0x0AA = 0;
    area->field_0x0AB = 0;
    area->field_0x0AD = 0;
    area->field_0x0AE = 0;
    area->field_0x0B0 = 0;
    area->field_0x0B2 = 0;
    area->field_0x0AC = 0;
    area->field_0x0B5 = 0;
    area->field_0x0B4 = 0;
    area->field_0x100[1] = 0;
    area->field_0x100[0] = 0;
    area->field_0x100[3] = 0;
    area->field_0x100[2] = 0;
    area->field_0x10D = 0;
    area->field_0x10C = 0;
    area->field_0x10F = 0;
    area->field_0x10E = 0;
    area->field_0x110 = 0;
    area->field_0x09E = 0;
    area->field_0x09F = 0;
    area->field_0x0B6 = 0;
    area->field_0x0B7 = 0;
    area->field_0x084 = 0;
    area->field_0x114 = 0;
    area->field_0x118 = 0;
    area->field_0x11A = 0;
    area->field_0x11C = 0;
    area->field_0x144 = 0;
    area->field_0x145 = 0;
}
