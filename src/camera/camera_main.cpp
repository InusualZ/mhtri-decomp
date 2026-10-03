/*
 * camera/camera_main.cpp - phase 4 unit, `.text` 0x802B5640..0x802BF278 (157 functions, 39992 bytes).
 *
 * PHASE 4 (docs/splits/phase4, window d).  Fold of 3 registered units: fn_802B2AA0.cpp, fn_802B5C58.cpp, light.cpp.
 * The functions below are the ones those sources define, in address order; every other function of the range keeps its
 * original bytes.  78 of 157 functions have a body here.
 *
 * FLAGS.  `cflags_main`; the pieces come from the stage/camera/light sources, all the same group.
 *
 * RESIDUAL (record views).  The absorbed sources were written as separate units and each carries its own header view
 * of the records they share; where two views disagree on a record layout or a prototype of one extern "C" symbol they
 * are kept apart in a namespace (extern "C" names stay unmangled, so the symbols are unchanged) instead of being
 * unified by guess.  Unifying them (one record header, one prototype per symbol) is the open work and removes the
 * namespace.  Here: `view_fn_802B2AA0` holds the seven camera-range functions of the old stage/fn_802B2AA0.cpp, which
 * view the 0x4F8-byte camera area through `StageAreaObj` (stage/fn_802B2AA0.h) where the camera functions view it as
 * `CamWork`.
 *
 * The light record types the head of the light module needs moved to `include/light/light_work.h` (shared with
 * `light/light.cpp`).
 *
 * Sections: the unit's block in config/RMHE08/splits.txt (.bss, .ctors, .data, .sdata, .sdata2, .text, extab,
 * extabindex).
 */
/* ---- header inherited from src/stage/fn_802B2AA0.cpp (written against its pre-phase-4 range) ---- */
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
 * Data.  `.data` 0x805CF56C-0x805CFBE8 (1660 B) is claimed: the 23-entry switch table MWCC emits for
 * fn_802B3270's `switch (st->mapno)` (0x805CF728, 92 B, arms read out of `main.elf`) sits inside it, and the
 * rest is the band's hand-written tables, which our object does not emit yet (the target object carries
 * 1660 B of `.data`, ours 92 B, so the unit's data row reads 0 % until they are reconstructed).  Every
 * pooled constant is `extern`-declared and never defined (playbook 29).
 *
 * `.sbss` 0x80794B60-0x80794B68 is claimed for `shell_set_func_ptr` (the shell-set job table pointer, typed
 * in `include/stage/shell_set_func_ptr.h`; 111 functions in 38 units read it, nothing in the DOL stores it, an
 * RSO does).  Definer: LOW confidence (about one in three) - `.sbss` follows the text order of the defining
 * TUs, which brackets the word between Pl/fn_8028F66C|fn_80295EF4 (0x80794B58, a `u8`) and light/light.cpp
 * (0x80794B68), i.e. any of menu_item, menu_message, stage/shell, Pl/pl_yure, stage/stg_w, this unit or
 * camera/fn_802B5C58; none of them stores it, and stg_w and this unit are the only ones of those that
 * read it.  This unit is taken as the definer because it is the stage band's shell-effect driver and
 * already owned the declaration.  The claim is the map's 8-byte row: our object emits the pointer only, so
 * `.sbss` is 4 B against the target's 8 B (the word at 0x80794B64 is read by nothing).
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
/* ---- header inherited from src/camera/fn_802B5C58.cpp (written against its pre-phase-4 range) ---- */
/*
 * camera/fn_802B5C58.cpp - the game camera work block and its controller.
 *
 * `.text` 0x802B5C58-0x802BEAAC (132 functions, 36436 B).  Registered from
 * `proposal/802B5C58_fn_802B5C58.cpp`.
 *
 * Module `camera` (brief section 2 class 3: what the code does, plus the neighbours' scheme).  The
 * range owns four real map names, all camera entry points - `get_current_view_mtx` (0x802BDC40),
 * `get_camera_pos` (0x802BDCE0), `get_camera_direction` (0x802BDE14), `set_quake_sub` (0x802BE4B0) -
 * and the rest drives the camera: `nw4r::g3d::Camera::SetPerspective`, `cpSetRotMatrix`, and
 * `Pl_act_ck`/`Pl_frame_check`, because the camera tracks the player's action state.  No `__FILE__`
 * string covers the range (it references no `.rodata` at all) and the shared runtime dump answers only
 * `zz_XXXXXXXX_` for everything but those four names, so the file keeps the map's stem.
 *
 * The camera work.  `fn_802BECD0` (0x802BECD0, `light/light.cpp`'s first function) returns
 * `lbl_806BB7E0` or `lbl_806BB7E0 + 1272`; `lbl_806BB7E0` is a 0x9F0-byte `.bss` object, i.e. two
 * 0x4F8 `CamWork` slots - that is where `CamWork`'s size comes from.  This unit's functions take that
 * pointer as `self`, or call the accessor for it through this unit's own name for the record
 * (`(CamWork*)fn_802BECD0()`: the owner views the same 0x4F8 bytes as `LightWork` and declares only
 * that spelling - `include/light/light.h`; folding the two views into one definition is the rule-1
 * pass `include/unsplit/camera.h` records), and touch fields up to `+0x4F7`.
 *
 * The seam at 0x802B5C58 is pinned by a `.sdata2` pool jump (`lbl_8079A514` -> `lbl_8079A530`);
 * `tudiscover` reports `MATCH SET 0x802B5C58..0x802BEAAC` (132 functions, 5 must-link anchors).  The
 * right edge is the proposal's `--max-bytes` cap, not a strong seam: tudiscover's best right-side
 * candidates are weak (`0x802BF278`/`0x802BF284`, codegen fingerprint + call closure), so the extent
 * may settle a few functions wider once those match.
 *
 * Naming note: the symbol map has only fn_XXXXXXXX for this range (checked with
 * `python tools/symbols/symedit.py range 0x802B5C58 0x802BEAAC --limit 500`, which lists exactly the
 * four camera names above as non-`fn_` entries).
 *
 * Flags.  The file builds with `#pragma peephole off`: retail keeps the unfused form of several folds
 * (`fn_802BC1CC`'s `rlwinm` + branchless `!= 0` where our peephole produces one `extrwi`; the
 * `clrlwi`-before-`add` index masks of `fn_802B81A8`/`fn_802B81C0`/`fn_802BC070`), which is the same
 * finding the sibling unit `stage/fn_802B2978.c` records for the band below this one.  Measured on
 * this unit's symbols, peephole off took the fully matching rows from 19 to 25 of the then-written 37.
 * `fn_802BB0D4` additionally needs `#pragma fp_contract off` (playbook 40): retail keeps `fmuls` +
 * `fadds` where the default contract fuses them into one `fmadds`.
 *
 * Residuals.  The range is reconstructed in address order; the functions not yet in this file are
 * unwritten, and their inventory is `config/RMHE08/symbols.txt` (their target objects are
 * `build/RMHE08/obj/`).  Everything written is byte-identical except `get_camera_pos`, whose residual
 * is on the function (and is shared by the whole accessor family: `get_current_view_mtx`,
 * `get_camera_direction`, `fn_802BDC90`, `fn_802BDDB0`, `fn_802BDE90`, `fn_802BDFC0`, `fn_802BDFFC`,
 * `fn_802BE088`, `camera_position_set`, `fn_802BE1EC`, `fn_802BE0F4` - they all start from `fn_80047398()`,
 * copy the handle through `word_copy_return_dst` and read a field, and all need MWCC to alias their returned
 * local to the `sret` pointer, which the spellings tried so far do not achieve).
 *
 * Shared-file cost.  Three declarations this unit needs are not in their owner's headers yet, so they
 * were added there in this branch (each an addition to an existing `extern "C"` block):
 * `include/mh3_pad.h` (`word_copy_return_dst`, owner `src/mh3_pad.cpp`), `include/fn_80047398.h`
 * (`fn_80047398`, owner `src/fn_80047398.cpp`) and `include/g3d/g3d_camera.h` (`fn_800749C8`, owner
 * `src/g3d/g3d_camera.cpp`).
 */
/* ---- header inherited from src/light/light.cpp (written against its pre-phase-4 range) ---- */
/*
 * light/light.cpp - the map light work: its record, its constructors, its per-frame channels and its
 * accessors.
 *
 * `.text` 0x802BEAAC-0x802C474C (103 functions, 23712 B).  Registered from
 * `proposal/802BEAAC_fn_802BEAAC.cpp`.
 *
 * Module `light` and file name `light.cpp` come from evidence class 2 (brief section 2): the range's
 * own symbols the retail symbol table knows are `light_init__Fv` (0x802BF284), `light_move__Fv`
 * (0x802C1D30), `set_amblight__FUc8_GXColor` (0x802C1DA8) and
 * `make_dir_light2__FlPQ34nw4r4math4VEC38_GXColorl` (0x802C1F74) - four real manglings naming one
 * light subsystem, all confirmed by `tools/symbols/dumpmap.py lookup`; `light_init`/`light_move` are
 * the module's own entry points, which is what this file holds.  No `__FILE__` string covers the
 * range (the `menu_item.cpp` string the discovery note mentions sits at 0x805CDFC8 and the dump
 * attributes its emitter to 0x802A22A4, i.e. the registered `menu_item` proposal, not this band).
 *
 * Language C++: four defined manglings (`light_init__Fv`...), `cflags_main` (`Wii/1.3`, `-O3
 * -inline noauto -Cpp_exceptions on`), the same group as the neighbour `stage/stg_w.cpp`.
 *
 * Naming note: the symbol map has only fn_XXXXXXXX for 99 of this range's 103 symbols (checked
 * with `python tools/symbols/symedit.py range 0x802BEAAC 0x802C474C` and
 * `python tools/symbols/dumpmap.py lookup` over the inventory: every unnamed entry is a bare
 * `fn_XXXXXXXX` in config/RMHE08/symbols.txt, and the runtime dump answers either `zz_XXXXXXXX_` or
 * an unrelated engine symbol for it).
 *
 * Seam - unproven.  The range is one maximal unclaimed run (`attribute.py` cut it at its byte cap),
 * and two observations say a real TU boundary sits at, not inside, its end: the `.ctors` word at
 * 0x8056F380 points at fn_802BEEE0 and the one at 0x8056F384 at fn_802C2530, and the `.sdata2`
 * ordering seam `lbl_8079A698 -> lbl_8079A69C` falls between fn_802C4630 and fn_802C474C (the last
 * function of the range), so the extent stays as proposed until the functions match.
 *
 * The light work record.  `LightWork`'s size is 0x4F8 and it is traced, not guessed: lbl_806BB7E0 is
 * a 0x9F0-byte `.bss` object holding exactly two records (fn_802BECD0 returns one of the two, 0x4F8
 * apart, and fn_802BEEE0 constructs both with `__construct_array(0x806BB7E0, fn_802BEF00, 0, 0x4F8,
 * 2)`), and fn_802BEF00's initialization loop runs its `LightChannel` array from +0x4A8 to +0x4E4 in
 * 0x14 steps.  The sub-records the constructors build carry their own traced extents.
 *
 * Flags: the unit is peephole-off - retail keeps the unfused `extsh` + `cmpwi` pair (playbook 39)
 * where `-O3`'s peephole folds them into one `extsh.`, and the two sibling units of the band
 * (`stage/stg_w.cpp`, `stage/fn_802B2978.c`) carry the same pragma for the same reason.
 *
 * Residuals (31 of the range's 103 functions written, 1960 B of 23712; 28 of the 31 byte-identical):
 *
 *  - fn_802BEAAC (86.81 %, 152 B against the target's 144 B).  Retail materialises the zero once at
 *    the top (`li r0, 0x0`) and keeps the decremented timer in r5; ours rematerialises `li r0, 0x0`
 *    inside each of the three arms and takes r0 for the timer, so the object is two instructions
 *    long.  Every other instruction is identical, and `--timer` (the shape landed) beats both the
 *    `s16 t = timer - 1` local (70.69 %) and the field-assignment form (70.69 %).
 *  - fn_802BF7E8 (85.45 %, 40 B against 44 B).  Retail materialises the counter's address into r4
 *    once at the top and stores the wrap with `sth r3, 0x0(r4)`, where ours keeps the `@sda21`
 *    addressing for both stores, which is one instruction short.
 *  - fn_802BF0AC (99.00 %, 140 B each).  The first run walk has its pointer/bound register pair the
 *    other way round (`addi r31, r31, 0xc` / `cmplw r31, r30` against ours on r30/r31); the second
 *    walk, the four leading vectors and the whole rest of the function are identical.
 *
 * Not yet written: the remaining 72 functions of the range, all still `fn_XXXXXXXX` in the map.
 *
 * Inventory and evidence: `python tools/units/ledger.py unit proposal/802BEAAC_fn_802BEAAC.cpp`.
 */

#include "types.h"
#include "light/light_work.h"
#include "nw4r/math.h"

#include "fn_80047398.h"
#include "g3d/g3d_camera.h"
#include "mh3_pad.h"
#include "Pl/Pl_master_ck.h"
#include "camera/camera.h"
#include "unsplit/camera.h"

/* The low half of `CamWork`'s +0x040 word, which the camera also views as four flag bytes. */
typedef struct CamFlagWord {
    /* +0x00 */ u8 pad_0x00;
    /* +0x01 */ u8 field_0x041;             /* invalid flag (bit 4); fn_802BD5CC tests it */
    /* +0x02 */ u8 field_0x042;             /* armed flag, set by fn_802BAB0C */
    /* +0x03 */ u8 field_0x043;             /* latched by fn_802BAB54 */
} CamFlagWord; /* size: 0x04 */

/* `CamWork`'s +0x040 word: a float `fn_802B7980` compares against a threshold, and the flag bytes the
 * follow and target paths latch.  Anchored at the word-aligned +0x040; both arms are 4 bytes, so the
 * union keeps the field size and the struct's later offsets. */
typedef union CamWord040 {
    /* +0x040 */ f32 value;
    /* +0x040 */ CamFlagWord flags;
} CamWord040; /* size: 0x04 */

/* One quake slot: CamWork carries three of them, at +0x4A8 / +0x4BC / +0x4D0 (the `cam + 1192`,
 * `+ 1212`, `+ 1232` of fn_802BE4FC / fn_802BE568 / fn_802BE714 / camera_shake_req). size: 0x14 */
typedef struct CamQuake {
    /* +0x00 */ nw4r::math::VEC3 vec_0x00;   /* origin, or direction */
    /* +0x0C */ u8 active_0x0C;              /* set by every arm */
    /* +0x0D */ u8 kind_0x0D;
    /* +0x0E */ s16 timer_0x0E;              /* from lbl_805D1E7C[kind & 7] */
    /* +0x10 */ u8 flag_0x10;
    /* +0x11 */ u8 pad_0x11[0x3];
} CamQuake; /* size: 0x14 */

/* The caller record fn_802BE568 / fn_802BE714 read a vector at +0x3C from; the callers pass a player
 * work record, and fn_8026FD94 is the only other thing this unit asks of it. size: >= 0x48 */
typedef struct CamWorkSrc {
    /* +0x00 */ u8 pad_0x00[0x3C];
    /* +0x3C */ nw4r::math::VEC3 vec_0x3C;
} CamWorkSrc; /* size: 0x48 (a lower bound, and marked as one: the caller's record is a player
              * work block, and only its +0x3C vector is read here) */

/*
 * The camera work block: 0x4F8 bytes, from `lbl_806BB7E0` (a 0x9F0-byte `.bss` object = two slots) and
 * the accessor's `base + 1272`.  Only the fields this unit touches are named; untouched runs keep
 * their offset as padding.
 *
 * `fn_802BECD0` is declared by its owner (`include/light/light.h`) over the same bytes, under that
 * unit's own view name `LightWork`; this unit's name for the record is `CamWork`, so every read of
 * the accessor goes through a cast to it - a type adaptation, no instruction (rule 1's two-views
 * residual, not a rename of either view).
 */
typedef struct CamWork {
    /* +0x000 */ u8 pad_0x000[0x9];
    /* +0x009 */ u8 field_0x009;             /* mode byte `fn_802B7980` compares against 3 */
    /* +0x00A */ u8 pad_0x00A[0x36];
    /* +0x040 */ CamWord040 word_0x040;
    /* +0x044 */ s16 field_0x044;            /* fn_802BAB0C stores `arg - 1` */
    /* +0x046 */ u8 pad_0x046[0x2];
    /* +0x048 */ f32 field_0x048;            /* fn_802BAB0C: a constant divided by the argument */
    /* +0x04C */ u8 pad_0x04C[0xA];
    /* +0x056 */ u8 field_0x056;             /* gate read by fn_802BAB54 */
    /* +0x057 */ u8 pad_0x057;
    /* +0x058 */ void* field_0x058;          /* record fn_802BAB54 reads a byte from at +0x48 */
    /* +0x05C */ u8 pad_0x05C[0x18];
    /* +0x074 */ u8 field_0x074;             /* fn_802B7980's second gate */
    /* +0x075 */ u8 pad_0x075[0x7];
    /* +0x07C */ void* field_0x07C;          /* 0x24-byte record list fn_802BD894 walks */
    /* +0x080 */ u8 pad_0x080[0xC];
    /* +0x08C */ void* field_0x08C;          /* the camera's target record */
    /* +0x090 */ u8 pad_0x090[0x2];
    /* +0x092 */ u8 field_0x092;             /* raised by fn_802B8B8C */
    /* +0x093 */ u8 pad_0x093[0x9];
    /* +0x09C */ u8 field_0x09C;             /* returned by fn_802B8B68 */
    /* +0x09D */ u8 pad_0x09D[0x19];
    /* +0x0B6 */ u8 field_0x0B6;             /* returned by fn_802B8E48 */
    /* +0x0B7 */ u8 pad_0x0B7[0x49];
    /* +0x100 */ u8 taken_0x100[0x2];        /* fn_802B81A8 tests, fn_802B81C0 sets */
    /* +0x102 */ u8 field_0x102[0xA];        /* fn_802B81C0 stores its value argument here */
    /* +0x10C */ u8 field_0x10C[0x8];        /* fn_802B81C0 clears one byte here */
    /* +0x114 */ u8 pad_0x114[0x4];
    /* +0x118 */ s8 field_0x118;             /* fn_802B9574 sets -1, fn_802B95B4 tests > 0 */
    /* +0x119 */ u8 pad_0x119[0x2C];
    /* +0x145 */ u8 field_0x145;             /* fn_802B954C raises it */
    /* +0x146 */ u8 pad_0x146[0x36];
    /* +0x17C */ f32 field_0x17C;            /* fn_802B9740 subtracts field_0x1D0 from it */
    /* +0x180 */ u8 pad_0x180[0x50];
    /* +0x1D0 */ f32 field_0x1D0;
    /* +0x1D4 */ u8 pad_0x1D4[0x4];
    /* +0x1D8 */ f32 field_0x1D8;            /* fn_802B9740's scale factor */
    /* +0x1DC */ u8 pad_0x1DC[0x10];
    /* +0x1EC */ u8 field_0x1EC;             /* returned by fn_802B9740 */
    /* +0x1ED */ u8 field_0x1ED;             /* fn_802B9740 copies it out through a parameter */
    /* +0x1EE */ u8 field_0x1EE;             /* fn_802B9740 gates on it */
    /* +0x1EF */ u8 pad_0x1EF[0x95];
    /* +0x284 */ u8 field_0x284;             /* camera_work_ck tests it against 1 */
    /* +0x285 */ u8 field_0x285;             /* cleared by fn_802BC468 */
    /* +0x286 */ u8 field_0x286;
    /* +0x287 */ u8 field_0x287;
    /* +0x288 */ s16 field_0x288;            /* fn_802BC82C returns it, or one less */
    /* +0x28A */ u8 pad_0x28A[0x44];
    /* +0x2CE */ u8 field_0x2CE;
    /* +0x2CF */ u8 field_0x2CF;
    /* +0x2D0 */ u32 field_0x2D0;
    /* +0x2D4 */ u8 pad_0x2D4[0x6D];
    /* +0x341 */ u8 field_0x341;
    /* +0x342 */ u8 pad_0x342[0x2];
    /* +0x344 */ s16 field_0x344;            /* returned by fn_802BC878 */
    /* +0x346 */ u8 field_0x346;
    /* +0x347 */ u8 pad_0x347[0x41];
    /* +0x388 */ u8 field_0x388;             /* fn_802BE41C tests it against 1 */
    /* +0x389 */ u8 pad_0x389[0x3F];
    /* +0x3C8 */ u8 field_0x3C8;             /* raised by fn_802B8B8C */
    /* +0x3C9 */ u8 pad_0x3C9[0x97];
    /* +0x460 */ u8 field_0x460;             /* returned by fn_802BBA94 */
    /* +0x461 */ u8 pad_0x461[0x19];
    /* +0x47A */ u8 field_0x47A;             /* fn_802BBA3C clears it */
    /* +0x47B */ u8 pad_0x47B[0x3];
    /* +0x47E */ u8 field_0x47E;             /* fn_802BBA64 stores its argument here */
    /* +0x47F */ u8 field_0x47F;             /* fn_802BBAC4 stores its argument here */
    /* +0x480 */ u8 pad_0x480[0x19];
    /* +0x499 */ u8 field_0x499;             /* the camera slot fn_802BB0EC selects */
    /* +0x49A */ u8 pad_0x49A;
    /* +0x49B */ u8 field_0x49B;             /* set to 0xFF by fn_802B8B8C */
    /* +0x49C */ u8 pad_0x49C[0x2];
    /* +0x49E */ u16 field_0x49E;            /* fn_802B700C clears the five below */
    /* +0x4A0 */ u16 field_0x4A0;
    /* +0x4A2 */ u16 field_0x4A2;
    /* +0x4A4 */ u16 field_0x4A4;
    /* +0x4A6 */ u16 field_0x4A6;
    /* +0x4A8 */ CamQuake quake_0x4A8;
    /* +0x4BC */ CamQuake quake_0x4BC;
    /* +0x4D0 */ CamQuake quake_0x4D0;
    /* +0x4E4 */ u8 pad_0x4E4[0x8];
    /* +0x4EC */ u32 field_0x4EC;            /* fn_802BE060 returns its low half */
    /* +0x4F0 */ u32 field_0x4F0;            /* fn_802BE038 returns its low half */
    /* +0x4F4 */ u8 pad_0x4F4[0x3];
    /* +0x4F7 */ u8 field_0x4F7;             /* camera-slot bit flags (fn_802BC000/70/1CC) */
} CamWork; /* size: 0x4F8 */

/* The four floats fn_802BAAE8 moves: +0x38 of its source record, +0x60 of its destination. */
typedef struct CamVec4 {
    /* +0x00 */ f32 v[4];
} CamVec4; /* size: 0x10 */

/* fn_802BAAE8's source record (0x48 bytes; only the tail is read). */
typedef struct CamMoveSrc {
    /* +0x00 */ u8 pad_0x00[0x38];
    /* +0x38 */ CamVec4 vec_0x38;
} CamMoveSrc; /* size: 0x48 */

/* fn_802BAAE8's destination record (0x70 bytes; only the tail is written). */
typedef struct CamMoveDst {
    /* +0x00 */ u8 pad_0x00[0x60];
    /* +0x60 */ CamVec4 vec_0x60;
} CamMoveDst; /* size: 0x70 */

/* The 0x24-byte record list fn_802BD894 walks; it reads the first word of each. */
typedef struct CamListEntry {
    /* +0x00 */ s32 value;
    /* +0x04 */ u8 pad_0x04[0x20];
} CamListEntry; /* size: 0x24 */

/* The camera's target record: a live flag, the two vectors `fn_802BD54C`/`fn_802BD588` copy out, and
 * the two mode bytes `fn_802BD5CC`/`fn_802BD618` test. */
typedef struct CamTarget {
    /* +0x000 */ u8 alive;
    /* +0x001 */ u8 pad_0x001[0x187];
    /* +0x188 */ nw4r::math::VEC3 vec_0x188;
    /* +0x194 */ u8 pad_0x194[0x28];
    /* +0x1BC */ u32 field_0x1BC;
    /* +0x1C0 */ u32 field_0x1C0;
    /* +0x1C4 */ u8 pad_0x1C4[0x1E];
    /* +0x1E2 */ u8 mode_0x1E2;
    /* +0x1E3 */ u8 mode_0x1E3;
} CamTarget; /* size: 0x1E4 */

/* The previous/current value pairs `fn_802BD260` latches: eight slots, four of them vectors that go
 * through `copyVec3` and the rest scalars.  The size is a lower bound - the record's real extent
 * belongs to whoever allocates it. size: >= 0xF0 */
typedef struct CamTrack {
    /* +0x000 */ u8 pad_0x000[0x4C];
    /* +0x04C */ nw4r::math::VEC3 prev_0x4C;
    /* +0x058 */ nw4r::math::VEC3 cur_0x58;
    /* +0x064 */ nw4r::math::VEC3 prev_0x64;
    /* +0x070 */ nw4r::math::VEC3 cur_0x70;
    /* +0x07C */ u8 pad_0x07C[0x2C];
    /* +0x0A8 */ u32 prev_0xA8;
    /* +0x0AC */ u32 cur_0xAC;
    /* +0x0B0 */ u32 prev_0xB0;
    /* +0x0B4 */ u32 cur_0xB4;
    /* +0x0B8 */ u8 pad_0x0B8[0x8];
    /* +0x0C0 */ s16 prev_0xC0;
    /* +0x0C2 */ s16 cur_0xC2;
    /* +0x0C4 */ u8 pad_0x0C4[0x8];
    /* +0x0CC */ s16 prev_0xCC;
    /* +0x0CE */ s16 cur_0xCE;
    /* +0x0D0 */ u8 pad_0x0D0[0x8];
    /* +0x0D8 */ u32 prev_0xD8;
    /* +0x0DC */ u32 cur_0xDC;
    /* +0x0E0 */ u8 pad_0x0E0[0x8];
    /* +0x0E8 */ u32 prev_0xE8;
    /* +0x0EC */ u32 cur_0xEC;
} CamTrack; /* size: 0xF0 (a lower bound, and marked as one: only the eight latching pairs are
             * named, so the record's real extent is not evidenced here) */

/* The record fn_802BAB54 reads its +0x48 byte from. */
typedef struct CamLead {
    /* +0x00 */ u8 pad_0x00[0x48];
    /* +0x48 */ u8 count_0x48;
} CamLead; /* size: 0x49 */

extern "C" {

/* Defined later in this file. */
void fn_802BB870(u32 mode, u32 a, u32 b, u8 c);
void fn_802B8B8C(void);
void fn_802BC4AC(u32 kind, u32 arg);
void fn_802BB118(CamWork* self);
void fn_802BBEE0(void);
}

/* Owner headers (rule 2): every symbol a registered unit defines is declared in that unit's header,
 * never here.  `include/unsplit/unknown.h` carries the module-ambiguous ones. */
#include "ef/fn_800CDB2C.h"
#include "fn_8004CAD8.h"
#include "g3d/fn_80063888.h"
#include "lobby/lb_npc.h"
#include "stage/stg_w.h"
#include "unsplit/Runtime.PPCEABI.H.h"
#include "unsplit/unknown.h"
#include "mh3_pad.h" /* the owner header (rule 2) */
#include "gx.h"
#include "sound/mhchar.h"
#include "sound/fn_800D7F54.h"
#include "unsplit/ef.h"
#include "unsplit/g3d.h"
#include "unsplit/Pl.h"
#include "ef/fn_800CDB2C.h"

/* ------------------------------------------------------------------------------------------------ */
/* types                                                                                             */
/* ------------------------------------------------------------------------------------------------ */

/* ------------------------------------------------------------------------------------------------ */
/* externs                                                                                           */
/* ------------------------------------------------------------------------------------------------ */

/* The pooled string and the entry points whose address owns no registered unit, so no header exists
 * for them: the map already names the data (playbook 29), and these are the module-ambiguous band
 * (`include/unsplit/unknown.h` carries `get_now_mapno`, the other declarations here are band gaps the
 * lint counts rather than guesses). */
extern "C" const char lbl_805D1EB8[];
extern "C" int sprintf(char* buffer, const char* format, ...);

/* The rest of this unit's own functions, declared before their definitions. */
extern "C" LightWork* fn_802BECD0(void);
extern "C" LightWork* fn_802BEF00(LightWork* self);
extern "C" LightTriple* fn_802BF004(LightTriple* self);
extern "C" LightOctal* fn_802BF044(LightOctal* self);
extern "C" LightParams* fn_802BF0AC(LightParams* self);
extern "C" LightQuad* fn_802BF138(LightQuad* self);
extern "C" LightQuadMid* fn_802BF180(LightQuadMid* self);
extern "C" LightRoot* fn_802BF1D8(LightRoot* self);
extern "C" LightChannel* fn_802BEF84(LightChannel* self);
extern "C" LightQuadTriple* fn_802BEFB4(LightQuadTriple* self);

/* The two light-work records the module keeps (`.bss` 0x806BB7E0, 0x9F0 B = 2 x 0x4F8).  Declared,
 * never defined here: the object emits only the references (playbook 29). */
extern LightWork lbl_806BB7E0[2];

#pragma peephole off

/* The C++-linkage callees the view below calls: declared at global scope so the calls resolve to the map's
 * manglings (a declaration inside the view namespace would mangle the namespace in). */
nw4r::math::VEC3 get_camera_pos(void);

namespace view_fn_802B2AA0 {

namespace nw4r = ::nw4r;  /* the view headers below reopen it */

/* data keeps its map name (a view namespace would mangle it): the header is read with C linkage */
extern "C" {
#include "stage/fn_802B2AA0.h"
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
    move = (s32)::get_move_work_adrs(2);
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
    subVec3(&world, &::get_camera_pos(), &pos);
    copyVec3(&poly, &world);
    if (screen_split_mode_ck() != 0) {
        setVector3(&poly, lbl_8079A4D8, lbl_8079A4DC, lbl_8079A4E0);
    }
    fn_802C20A4(&poly);
    fn_802B5980(area);
}

/* Runs one per-area object's update and, when the extra gate is open, the second object's too. */
extern "C" void fn_802B58D4(void)
{
    fn_802B5738((StageAreaObj*)lbl_806BB7E0);
    if (screen_split_mode_ck() != 0) {
        my_player_no_set(1);
        fn_802B5738((StageAreaObj*)(lbl_806BB7E0 + 0x4F8));
        my_player_no_set(0);
    }
}

/* Points the g3d scene root's camera at the current view. */
extern "C" void fn_802B592C(u8 index, s32 camera)
{
    nw4r::math::VEC3 pos;
    s32 handle;
    s32 root;

    handle = ((nw4r::g3d::ScnRoot*)pRoot)->GetCamera(index);
    word_copy_return_dst(&pos, &handle);
    camera_posture_info_ctor(&root);
    root = camera;
}

/* Places one per-area object's seat from the camera and the area's effect table. */
extern "C" void fn_802B5980(StageAreaObj* area)
{
    nw4r::math::VEC3 poly;
    nw4r::math::VEC3 cam;
    nw4r::math::VEC3 out;

    VEC3_ctor(&poly);
    copyVec3(&poly, &::get_camera_pos());
    if (fn_80291BBC(&poly, ::get_now_areano(), (s32)&area->field_0x4F4, &out, 0xFFFF) == 0) {
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
    subVec3(&world, &area->seat_pos, &area->seat_pos2);
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
}  /* namespace view_fn_802B2AA0 */

extern "C" {

/*
 * Clears the five 16-bit records the camera carries over from the previous frame.
 */
void fn_802B700C(CamWork* self)
{
    self->field_0x49E = 0;
    self->field_0x4A0 = 0;
    self->field_0x4A2 = 0;
    self->field_0x4A4 = 0;
    self->field_0x4A6 = 0;
}

/*
 * Copies one 16-bit record.
 */
void fn_802B7028(u16* dst, const u16* src)
{
    *dst = *src;
}

/*
 * Whether the camera is in its third mode, its timer is above the threshold and its target is live.
 */
u32 fn_802B7980(CamWork* self)
{
    if (self->field_0x009 == 3 && self->word_0x040.value > lbl_8079A56C && self->field_0x074 != 0)
        return 1;
    return 0;
}

/*
 * Whether the indexed slot is still free.
 */
bool fn_802B81A8(CamWork* self, u8 index)
{
    return self->taken_0x100[index] == 0;
}

/*
 * Marks the indexed slot taken and stores its value.
 */
void fn_802B81C0(CamWork* self, u8 index, u8 value)
{
    self->taken_0x100[index] = 1;
    self->field_0x102[index] = value;
    self->field_0x10C[index] = 0;
}

/*
 * Reads the +0x9C byte of the current camera work.
 */
u8 fn_802B8B68(void)
{
    return ((CamWork*)fn_802BECD0())->field_0x09C;
}

/*
 * Reads the +0xB6 byte of the current camera work.
 */
u8 fn_802B8E48(void)
{
    return ((CamWork*)fn_802BECD0())->field_0x0B6;
}

/*
 * Raises the +0x145 flag of the current camera work.
 */
void fn_802B954C(void)
{
    CamWork* self = (CamWork*)fn_802BECD0();

    self->field_0x145 = 1;
}

/*
 * Invalidates the +0x118 timer of the current camera work when `reset` is zero.
 */
void fn_802B9574(u8 reset)
{
    CamWork* self = (CamWork*)fn_802BECD0();

    if (reset == 0)
        self->field_0x118 = -1;
}

/*
 * Whether the +0x118 timer of the current camera work is positive.
 */
bool fn_802B95B4(void)
{
    return ((CamWork*)fn_802BECD0())->field_0x118 > 0;
}

/*
 * Copies two state bytes out of the camera work, writes the timer scaled by the work's own factor, and
 * returns the mode byte.
 */
u8 fn_802B9740(void* unused, u8* out_a, u8* out_b, f32* out_timer)
{
    CamWork* self = (CamWork*)fn_802BECD0();

    *out_a = self->field_0x1ED;
    *out_b = self->field_0x1EE;
    if (self->field_0x1EE != 0) {
        f32 t = self->field_0x17C - self->field_0x1D0;
        *out_timer = t;
        *out_timer = t * self->field_0x1D8;
    }
    return self->field_0x1EC;
}

/*
 * Copies the four floats at +0x38 of the source record into +0x60 of the destination.
 */
void fn_802BAAE8(CamMoveDst* dst, const CamMoveSrc* src)
{
    dst->vec_0x60.v[0] = src->vec_0x38.v[0];
    dst->vec_0x60.v[1] = src->vec_0x38.v[1];
    dst->vec_0x60.v[2] = src->vec_0x38.v[2];
    dst->vec_0x60.v[3] = src->vec_0x38.v[3];
}

/*
 * The interval from the argument, as the constant at lbl_8079A4EC divided by it.
 */
void fn_802BAB0C(CamWork* self, s32 value)
{
    self->word_0x040.flags.field_0x042 = 1;
    self->field_0x044 = (s16)(value - 1);
    self->field_0x048 = lbl_8079A4EC / (f32)value;
}

/*
 * Starts the camera follow when the lead record has a positive count, and latches +0x43 when the
 * camera is already fading.
 */
void fn_802BAB54(CamWork* self)
{
    s32 count;

    if (self->field_0x056 == 0)
        return;
    count = 15;
    if (self->field_0x058 != NULL)
        count = ((CamLead*)self->field_0x058)->count_0x48;
    if (count < 1) {
        self->word_0x040.flags.field_0x042 = 0;
        return;
    }
    if (self->word_0x040.flags.field_0x042 == 2) {
        self->word_0x040.flags.field_0x043 = 1;
        return;
    }
    fn_802BAB0C(self, count);
}

/*
 * Blends two values by `t`.  Retail keeps `a * t` and `b * (c - t)` as separate multiplies and
 * adds, so this function builds with the contract pass off (playbook 40).
 */
#pragma fp_contract off

f32 fn_802BB0D4(f32 a, f32 b, f32 t)
{
    f32 other = lbl_8079A4EC - t;

    return a * t + b * other;
}
#pragma fp_contract on

/*
 * Arms the current camera work through the `fn_802BB870` entry point with no arguments.
 */
void fn_802BBA2C(u32 mode)
{
    fn_802BB870(mode, 0, 0, 0);
}

/*
 * Clears the mode flag of the current camera work.
 */
void fn_802BBA3C(void)
{
    CamWork* self = (CamWork*)fn_802BECD0();

    self->field_0x47A = 0;
}

/*
 * Stores the +0x47E byte of the current camera work.
 */
void fn_802BBA64(u8 value)
{
    ((CamWork*)fn_802BECD0())->field_0x47E = value;
}

/*
 * Reads the +0x460 byte of the current camera work.
 */
u8 fn_802BBA94(void)
{
    return ((CamWork*)fn_802BECD0())->field_0x460;
}

/*
 * Clears the mode flag with no argument.
 */
void fn_802BBAC0(void)
{
    fn_802BBA3C();
}

/*
 * Stores the +0x47F byte of the current camera work.
 */
void fn_802BBAC4(u8 value)
{
    ((CamWork*)fn_802BECD0())->field_0x47F = value;
}

/*
 * Arms the camera work with mode 0.
 */
void fn_802BBAB8(void)
{
    fn_802BBA2C(0);
}

/*
 * Clears the mode flag with no argument.
 */
void fn_802BBB78(void)
{
    fn_802BBA3C();
}

/*
 * Sets or clears one camera-slot bit: with `set` == 1 the bit for `slot` is raised, otherwise bit 7 is.
 */
void fn_802BC000(u8 slot, u8 set)
{
    CamWork* self = (CamWork*)fn_802BECD0();

    if (set == 1)
        self->field_0x4F7 |= 1 << slot;
    else
        self->field_0x4F7 |= 0x80;
}

/*
 * Whether the indexed camera-slot bit is set.
 */
bool fn_802BC070(CamWork* self, u8 slot)
{
    return (self->field_0x4F7 & (1 << slot)) != 0;
}

/*
 * Whether the high camera-slot bit is set.
 */
bool fn_802BC1CC(CamWork* self)
{
    return (self->field_0x4F7 & 0x80) != 0;
}

/*
 * Clears the camera mode bytes the quake and fade paths latch.
 */
void fn_802BC468(void)
{
    CamWork* self = (CamWork*)fn_802BECD0();

    self->field_0x286 = 0;
    self->field_0x287 = 0;
    self->field_0x2CE = 0;
    self->field_0x2CF = 0;
    self->field_0x2D0 = 0;
    self->field_0x285 = 0;
    self->field_0x341 = 0;
    self->field_0x346 = 0;
}

/*
 * Returns the +0x288 counter, one lower when `full` is set.
 */
s16 fn_802BC82C(u8 full)
{
    CamWork* self = (CamWork*)fn_802BECD0();

    if (full == 0)
        return self->field_0x288;
    return (s16)(self->field_0x288 - 1);
}

/*
 * Returns the +0x344 counter.
 */
s16 fn_802BC878(void)
{
    return ((CamWork*)fn_802BECD0())->field_0x344;
}

/*
 * Whether the camera work is usable and its target record is alive in the first mode.
 */
bool fn_802BD5CC(CamWork* self)
{
    CamTarget* target;

    if ((self->word_0x040.flags.field_0x041 & 0x10) != 0)
        return false;
    target = (CamTarget*)self->field_0x08C;
    if (target != NULL && target->alive != 0 && target->mode_0x1E2 == 1)
        return true;
    return false;
}

/*
 * Whether the target record is alive and in the second mode.
 */
bool fn_802BD618(CamWork* self)
{
    CamTarget* target = (CamTarget*)self->field_0x08C;

    if (target != NULL && target->alive != 0 && target->mode_0x1E2 == 1 && target->mode_0x1E3 == 2)
        return true;
    return false;
}

/*
 * Returns half of the first value of the record at +0x7C, or zero when the list is empty.
 */
s16 fn_802BD894(CamWork* self)
{
    CamListEntry* entry = (CamListEntry*)self->field_0x07C;
    s32 value = 0;

    if (entry->value >= 0) {
        while (entry->value >= 0)
            entry++;
        value = entry[-1].value;
    }
    return (s16)(value / 2);
}

/*
 * The 16-bit difference of two values, shifted right.
 */
s32 fn_802BDC2C(s16 a, s16 b, u8 shift)
{
    return (s16)(a - b) >> shift;
}

/*
 * Returns the low half of the +0x4EC word.
 */
u32 fn_802BE060(void)
{
    CamWork* self = (CamWork*)fn_802BECD0();

    return (u16)self->field_0x4EC;
}

/*
 * Returns the low half of the +0x4F0 word.
 */
u32 fn_802BE038(void)
{
    CamWork* self = (CamWork*)fn_802BECD0();

    return (u16)self->field_0x4F0;
}

/*
 * Whether the +0x284 byte holds 1.
 */
bool camera_work_ck(void)
{
    CamWork* self = (CamWork*)fn_802BECD0();

    return self->field_0x284 == 1;
}

/*
 * Whether the +0x388 byte holds 1.
 */
bool fn_802BE41C(void)
{
    CamWork* self = (CamWork*)fn_802BECD0();

    return self->field_0x388 == 1;
}

/*
 * Copies the target record's +0x1BC word and +0x188 vector out, or raises the camera's invalid flag
 * when there is no live target.
 */
void fn_802BD54C(CamWork* self, u32* out, nw4r::math::VEC3* vec)
{
    CamTarget* target = (CamTarget*)self->field_0x08C;

    if (target != NULL && target->alive != 0) {
        *out = target->field_0x1C0;
        copyVec3(vec, &target->vec_0x188);
        return;
    }
    self->word_0x040.flags.field_0x041 |= 1;
}

/*
 * Copies the target record's +0x1BC and +0x1C0 words and its +0x188 vector out, or raises the camera's
 * invalid flag when there is no live target.
 */
void fn_802BD588(CamWork* self, u32* out_a, u32* out_b, nw4r::math::VEC3* vec)
{
    CamTarget* target = (CamTarget*)self->field_0x08C;

    if (target != NULL && target->alive != 0) {
        *out_a = target->field_0x1BC;
        *out_b = target->field_0x1C0;
        copyVec3(vec, &target->vec_0x188);
        return;
    }
    self->word_0x040.flags.field_0x041 |= 1;
}

/*
 * Latches the current value of every slot selected by `mask` into its previous value.
 */
void fn_802BD260(CamTrack* self, u32 mask)
{
    if ((mask & 1) != 0)
        copyVec3(&self->cur_0x58, &self->prev_0x4C);
    if ((mask & 2) != 0)
        copyVec3(&self->cur_0x70, &self->prev_0x64);
    if ((mask & 4) != 0)
        self->cur_0xAC = self->prev_0xA8;
    if ((mask & 8) != 0)
        self->cur_0xB4 = self->prev_0xB0;
    if ((mask & 0x10) != 0)
        self->cur_0xC2 = self->prev_0xC0;
    if ((mask & 0x20) != 0)
        self->cur_0xCE = self->prev_0xCC;
    if ((mask & 0x40) != 0)
        self->cur_0xDC = self->prev_0xD8;
    if ((mask & 0x80) != 0)
        self->cur_0xEC = self->prev_0xE8;
}

/*
 * Arms one quake slot: the origin vector, the kind and the duration `lbl_805D1E7C` gives it.
 */
void fn_802BE44C(CamQuake* slot, const nw4r::math::VEC3* origin, u8 kind, u8 flag)
{
    slot->active_0x0C = 1;
    slot->kind_0x0D = kind;
    slot->timer_0x0E = lbl_805D1E7C[kind & 0x1F];
    copyVec3(&slot->vec_0x00, origin);
    slot->flag_0x10 = flag;
}

/*
 * Starts the first quake slot from the zero vector, with the kind's high bit set.
 */
void fn_802BE4FC(u8 kind)
{
    CamWork* self = (CamWork*)fn_802BECD0();
    nw4r::math::VEC3 origin;

    VEC3_ctor(&origin);
    setVector3(&origin, lbl_8079A4D8, lbl_8079A4D8, lbl_8079A4D8);
    fn_802BE44C(&self->quake_0x4A8, &origin, (u8)(kind | 0x80), 0);
}

/*
 * Starts the first quake slot from the work record's +0x3C vector when the player has a live action.
 */
void fn_802BE568(CamWorkSrc* work, u8 kind)
{
    CamQuake* slot = &((CamWork*)fn_802BECD0())->quake_0x4A8;

    if (fn_8026FD94((_PLW*)work) != 0)
        fn_802BE44C(slot, &work->vec_0x3C, kind, 0);
}

/*
 * Starts the second quake slot from the work record's +0x3C vector when the player has a live action.
 */
void fn_802BE714(CamWorkSrc* work, u8 kind)
{
    CamQuake* slot = &((CamWork*)fn_802BECD0())->quake_0x4BC;

    if (fn_8026FD94((_PLW*)work) != 0)
        fn_802BE44C(slot, &work->vec_0x3C, kind, 0);
}

/*
 * Starts the third quake slot from the zero vector, with the kind's high bits set.
 */
void camera_shake_req(u8 kind)
{
    CamWork* self = (CamWork*)fn_802BECD0();
    nw4r::math::VEC3 origin;

    VEC3_ctor(&origin);
    setVector3(&origin, lbl_8079A4D8, lbl_8079A4D8, lbl_8079A4D8);
    fn_802BE44C(&self->quake_0x4D0, &origin, (u8)(kind | 0xA0), 0);
}

/*
 * Resets the three camera slot flags and the three state bytes the follow path uses.
 */
void fn_802B8B8C(void)
{
    CamWork* self = (CamWork*)fn_802BECD0();

    self->quake_0x4D0.active_0x0C = 0;
    self->quake_0x4BC.active_0x0C = 0;
    self->quake_0x4A8.active_0x0C = 0;
    fn_802BBEE0();
    fn_802BB118(self);
    self->field_0x49B = 0xFF;
    self->field_0x092 = 1;
    self->field_0x3C8 = 1;
}

/*
 * Selects the fourth camera slot and resets the follow state.
 */
void fn_802BB0EC(void)
{
    CamWork* self = (CamWork*)fn_802BECD0();

    self->field_0x499 = 4;
    fn_802B8B8C();
}

/*
 * Puts the camera into its second mode with a 40-frame hold.
 */
void fn_802BC65C(void)
{
    CamWork* self = (CamWork*)fn_802BECD0();

    fn_802BC4AC(2, 0);
    self->field_0x285 = 40;
}

/*
 * Puts the camera into its third mode with an 8-frame hold.
 */
void fn_802BC69C(void)
{
    CamWork* self = (CamWork*)fn_802BECD0();

    fn_802BC4AC(3, 0);
    self->field_0x285 = 8;
}

/*
 * Puts the camera into mode 12 with the caller's argument.
 */
void fn_802BC7B0(u32 arg)
{
    fn_802BC4AC(12, arg);
}

/*
 * Puts the camera into mode 17 with a zero argument.
 */
void fn_802BC7BC(void)
{
    fn_802BC4AC(17, 0);
}

/*
 * Puts the camera into mode `base + 17`.
 */
void fn_802BC820(u32 base, u32 arg)
{
    fn_802BC4AC((u8)(base + 17), arg);
}

/*
 * Puts the camera into `mode` and holds it for 8 frames.
 */
void fn_802BC7C8(u8 mode, u32 arg)
{
    CamWork* self = (CamWork*)fn_802BECD0();

    fn_802BC4AC(mode, arg);
    self->field_0x285 = 8;
}

/*
 * Clears the quake/fade state and switches mode, but only from the +0x284 == 1 state.
 */
void camera_event_set(u8 mode, u32 arg)
{
    if (camera_work_ck()) {
        fn_802BC468();
        fn_802BC4AC(mode, arg);
    }
}
}

/*
 * The current camera's world position.  The four camera entry points share one shape: build a camera
 * handle from `fn_80047398`, copy it through `word_copy_return_dst`, then read the wanted field out of it.
 *
 * Residual: 59.4 %.  The three calls are right and in retail's order, but retail's second object is
 * the return slot itself (MWCC aliased the returned local to the `sret` pointer), while ours keeps
 * both objects on the stack and copies the result out at the end: frame 0x30 vs 0x20, two extra
 * `lwz/stw` pairs and `addi r3, r1, 0xC` where retail passes `r31`.  Declaration order of the two
 * locals, and a `&&`-style single-return spelling, were tried - neither makes MWCC alias the local to
 * the return slot.  The sibling accessors (`get_current_view_mtx`, `get_camera_direction`,
 * `fn_802BDC90`/`fn_802BDE90`/`fn_802BDFC0`) have the same shape and the same blocker, so they are left
 * unwritten rather than landed at 0 %.
 */
nw4r::math::VEC3 get_camera_pos(void)
{
    nw4r::math::VEC3 pos;
    nw4r::math::VEC3 tmp;
    void* cam;

    VEC3_ctor(&pos);
    cam = fn_80047398();
    word_copy_return_dst(&tmp, &cam);
    fn_800749C8(&tmp, &pos);
    return pos;
}

/*
 * Starts a camera quake at `origin` in the first quake slot.
 */
void set_quake_sub(u8 kind, nw4r::math::VEC3* origin)
{
    fn_802BE44C(&((CamWork*)fn_802BECD0())->quake_0x4A8, origin, kind, 0);
}


/* Ages the three channel timers by one and hands the channel the given index selects to the
 * per-channel update. */
extern "C" void fn_802BEAAC(LightWork* self, u8 index)
{
    if (--self->channel[0].timer <= 0) {
        self->channel[0].enable = 0;
        self->channel[0].timer = 0;
    }

    if (--self->channel[1].timer <= 0) {
        self->channel[1].enable = 0;
        self->channel[1].timer = 0;
    }

    if (--self->channel[2].timer <= 0) {
        self->channel[2].enable = 0;
        self->channel[2].timer = 0;
    }

    if (index == 3) {
        fn_802BE7E8(self, &self->channel[2], index);
    } else if (index == 1) {
        fn_802BE7E8(self, &self->channel[1], index);
    } else {
        fn_802BE7E8(self, &self->channel[0], index);
    }
}

/* Formats the per-map light file name into the caller's buffer. */
extern "C" int fn_802BECB8(char* buffer, u8 mapno)
{
    return sprintf(buffer, lbl_805D1EB8, mapno);
}

/* Returns the light work of the loaded map: the second record while the special-map flag is up, the
 * first one otherwise. */
extern "C" LightWork* fn_802BECD0(void)
{
    if (screen_split_mode_ck() != 0 && (s8)my_player_no() != 0) {
        return &lbl_806BB7E0[1];
    }
    return &lbl_806BB7E0[0];
}

/* Runs the map's light-record builder with the level counter's bank switched to the given one and
 * puts the previous bank back afterwards. */
extern "C" void* fn_802BEDE8(s8 bank)
{
    s32 previous;
    void* result;

    previous = my_player_no();
    my_player_no_set(bank);
    result = (void*)(u32)camera_work_ck();
    my_player_no_set(previous);
    return result;
}

/* Queries the light resource for the given id with the bank reset and then with it raised, letting
 * each failed query fall through to the id's own record handler. */
extern "C" void fn_802BEE3C(u8 id, void* arg)
{
    s32 previous;

    previous = my_player_no();

    my_player_no_set(0);
    if (!fn_802B0688(&fn_802BECD0()->resource->entry)) {
        fn_802BC564(id, arg);
    }

    my_player_no_set(1);
    if (!fn_802B0688(&fn_802BECD0()->resource->entry)) {
        fn_802BC564(id, arg);
    }

    my_player_no_set(previous);
}

/* Constructs the two light-work records the module keeps. */
extern "C" void fn_802BEEE0(void)
{
    __construct_array(&lbl_806BB7E0[0], (void*)fn_802BEF00, NULL, 0x4F8, 2);
}

/* Constructs a light work record: every sub-record in turn, then the three channels. */
extern "C" LightWork* fn_802BEF00(LightWork* self)
{
    LightChannel* end;
    LightChannel* channel;

    fn_802BF1D8(&self->root);
    fn_802BF180(&self->anim);
    fn_802BF138(&self->lights);
    fn_802BF0AC(&self->params);
    fn_802BF044(&self->colors);
    fn_802BEFB4(&self->entry);

    channel = &self->channel[0];
    end = &self->channel[3];
    do {
        fn_802BEF84(channel);
        channel++;
    } while (channel < end);
    return self;
}

/* Constructs a channel's position vector. */
extern "C" LightChannel* fn_802BEF84(LightChannel* self)
{
    VEC3_ctor(&self->pos);
    return self;
}

/* Constructs a record of four vectors and a LightTriple. */
extern "C" LightQuadTriple* fn_802BEFB4(LightQuadTriple* self)
{
    VEC3_ctor(&self->v[0]);
    VEC3_ctor(&self->v[1]);
    VEC3_ctor(&self->v[2]);
    VEC3_ctor(&self->v[3]);
    fn_802BF004(&self->inner);
    return self;
}

/* Constructs a record of three vectors. */
extern "C" LightTriple* fn_802BF004(LightTriple* self)
{
    VEC3_ctor(&self->a);
    VEC3_ctor(&self->b);
    VEC3_ctor(&self->c);
    return self;
}

/* Constructs a record of eight vectors. */
extern "C" LightOctal* fn_802BF044(LightOctal* self)
{
    VEC3_ctor(&self->v[0]);
    VEC3_ctor(&self->v[1]);
    VEC3_ctor(&self->v[2]);
    VEC3_ctor(&self->v[3]);
    VEC3_ctor(&self->w[0]);
    VEC3_ctor(&self->w[1]);
    VEC3_ctor(&self->w[2]);
    VEC3_ctor(&self->w[3]);
    return self;
}

/* Constructs a record of four vectors. */
extern "C" LightQuad* fn_802BF138(LightQuad* self)
{
    VEC3_ctor(&self->v[0]);
    VEC3_ctor(&self->v[1]);
    VEC3_ctor(&self->v[2]);
    VEC3_ctor(&self->v[3]);
    return self;
}

/* Constructs a record of four vectors and two two-element vector runs. */
extern "C" LightParams* fn_802BF0AC(LightParams* self)
{
    nw4r::math::VEC3* vec;
    nw4r::math::VEC3* end;

    VEC3_ctor(&self->v0);
    VEC3_ctor(&self->v1);
    VEC3_ctor(&self->v2);
    VEC3_ctor(&self->v3);

    vec = &self->runs[0];
    end = &self->runs[2];
    do {
        VEC3_ctor(vec);
        vec++;
    } while (vec < end);

    end = &self->runs[4];
    do {
        VEC3_ctor(vec);
        vec++;
    } while (vec < end);
    return self;
}

/* Constructs a record of four vectors, a mid block and a trailing vector. */
extern "C" LightQuadMid* fn_802BF180(LightQuadMid* self)
{
    VEC3_ctor(&self->v[0]);
    VEC3_ctor(&self->v[1]);
    VEC3_ctor(&self->v[2]);
    VEC3_ctor(&self->v[3]);
    MTX34_ctor(&self->mid);
    VEC3_ctor(&self->tail);
    return self;
}

/* Constructs the scene root record's fifteen vectors. */
extern "C" LightRoot* fn_802BF1D8(LightRoot* self)
{
    VEC3_ctor(&self->v0);
    VEC3_ctor(&self->v1);
    VEC3_ctor(&self->v2);
    VEC3_ctor(&self->v3);
    VEC3_ctor(&self->v4);
    VEC3_ctor(&self->v5);
    VEC3_ctor(&self->v6);
    VEC3_ctor(&self->v7);
    VEC3_ctor(&self->v8);
    VEC3_ctor(&self->v9);
    VEC3_ctor(&self->v10);
    VEC3_ctor(&self->v11);
    VEC3_ctor(&self->v12);
    VEC3_ctor(&self->v13);
    VEC3_ctor(&self->v14);
    return self;
}

