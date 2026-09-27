/* ef/fn_800FD864.cpp - the map/area spawn table of the `eft004` effect family and its two hooks,
 * Naming note: the symbol map has only fn_XXXXXXXX for this range (checked with symedit: every fn_ name this file uses is a bare .text entry in config/RMHE08/symbols.txt)
 * `.text` 0x800FD864..0x800FE978 (3 functions: fn_800FD864, fn_800FE8E8, fn_800FE93C).
 *
 * What it is.  `fn_800FD864` is the effect spawner the map/area code calls once per map area: it takes
 * the 0x48-byte effect slot `fn_800F8788(0x8F4)` hands out (the `0x8F4` is the family work-block size the
 * slot's `+0x38` pointer receives), clears the work flag, reads the current map number and area, and a
 * 23-entry map switch then an area switch select the three spawn parameters (effect id, count, parameter)
 * the family's per-frame handlers read out of the work block.  The common tail writes them, sets the
 * phase to 3, stores the area, seeds the slot with `fn_800F9DF4` and installs `fn_800FE93C` as the
 * `+0x34` state dispatcher and `fn_800FE8E8` as the `+0x40` release hook.
 *
 * `fn_800FE8E8` is the release hook: it hands both of the work block's effect-heap runs back to
 * `push_eft_effect_heap_num` and zeroes their counts.  `fn_800FE93C` is the four-state dispatcher: it
 * tail-calls the state-0 handler `fn_800FE978` and the three `eft004` per-frame handlers
 * (`fn_800FF8D4`, `fn_800FFC98`, `fn_800FFCA8`).
 *
 * Language.  The unit's own symbols (`fn_800FD864`, `fn_800FE8E8`, `fn_800FE93C`) and the plain helpers
 * (`fn_800F8788`, `fn_800F886C`, `fn_800F9DF4`, `fn_803AAF88`) are plain, unmangled names, so those carry
 * C linkage.  The mangled callees (`get_now_mapno__Fv` / `get_now_areano__Fv` / the
 * `push_eft_effect_heap_num(nw4r::ef::Effect**, long)` the map spells with an argument list) are declared
 * with their real C++ signatures and called through them (rule 9); the file is a `.cpp` for that reason,
 * and `fn_800FD864` is still a plain symbol because every definition is `extern "C"`.  `langcheck` reads
 * the mangled callees as *suggested* C++ and suggests keeping `.c` only by spelling the mangled names,
 * which rule 9 forbids; the C++ route reproduces all 61 `.rela.text` relocation names exactly (the 24
 * non-jump-table ones - `get_now_mapno__Fv`, `get_now_areano__Fv`,
 * `push_eft_effect_heap_num__FPPQ34nw4r2ef6Effectl`, the six helpers and `fn_800FE978`/`fn_800FF8D4`/
 * `fn_800FFC98`/`fn_800FFCA8` - are name-identical to the target's), so C++ is the evidence.
 *
 * Result: fn_800FD864 (0x1084), fn_800FE8E8 (0x54) and fn_800FE93C (0x3C) all 100 %; `.text` (0x1114),
 * `extab` (0x10) and `extabindex` (0x18) byte-identical to the target.  The only object-level differences
 * are the eight switch jump tables, which our object emits locally in `.data` (0x224) while the target's
 * code relocates against the shared `.data` run (`jumptable_8059C19C..0x8059C368`), plus the local
 * extab/symbol-table names and the `.comment` version byte (ours 0x0f, retail 0x0e) - none of which
 * reaches the linked DOL or the per-symbol score.
 *
 * Load-bearing source shapes (each measured; the wrong form costs real points):
 *   * `areano` is `u32`, not `u8`.  As a `u8` the small area switches lower to signed compares
 *     (`cmpwi r28,1`); retail uses the unsigned `cmplwi` for the non-zero cases, which a `u32` operand
 *     produces (98.79 -> 99.98 %).  The `(u8)` cast on the assignment keeps retail's `clrlwi r3,24`.
 *   * `#pragma peephole off` is required for the two function-pointer stores at the tail: with the pass
 *     on MWCC folds `addi r0,r3,fn_800FE8E8@l; stw r0,0x40(r30)` into `addi r3,...; stw r3,...`, where
 *     retail keeps the scratch in `r0` (99.98 -> 100 %).
 *   * each case body assigns its three parameters in the retail instruction order (`a`,`b`,`c` for most
 *     maps, `b`,`a`,`c` for the two maps 21/22); the common tail after the outer switch stores them as
 *     `work->+0x00 = b`, `+0x0D0 = a`, `+0x0D4 = c`.
 *
 * Naming.  Evidence class 4: nothing supports a name.  The unit has no `.data` pool and no `__FILE__`
 * string (the target object carries only `.text`/`extab`/`extabindex`, and none of the three functions
 * references a string), the runtime dump gives `zz_00fd864_` only (not evidence), and the two range
 * neighbours (`ef/fn_800FD520.c`, `ef/fn_800FD718.c`) keep the map's `fn_XXXXXXXX` stem.  The file is
 * therefore `ef/fn_800FD864.cpp`, the first symbol's stem, and the rule-7 deferral above is the reason.
 *
 * Data.  The unit owns no pool section: its eight jump tables live in the shared `.data` run
 * (0x8059C19C..0x8059C368) and are referenced by their map names only.  The `extab`/`extabindex`
 * fragments travel with the code and are claimed in `splits.txt`.
 *
 * Types.  `_EFT` and `_EFT_WORK` come from the shared `ef.h` (rule 1); the family work block the slot
 * carries at `+0x38` is this unit's own `_EFT_MAP_WORK` (a 0x8F4-byte block, the size `fn_800F8788` was
 * asked for), defined here because no other unit reads it.
 *
 * Inventory, addresses and sizes: `python tools/units/ledger.py unit ef/fn_800FD864.cpp`.
 */

#include "types.h"
#include "ef/fn_800FE978.h"
#include "ef/effect.h"
#include "nw4r/math.h"
#include "ef.h"
#include "ef/eft004.h"
#include "unsplit/ef.h"

/* The retail object keeps the function-pointer loads' scratch register in `r0` (an `addi r0,r3,X`
 * feeding the store) where the peephole pass folds it into `r3`; the whole file compiles with the pass
 * off (playbook 39). */
#pragma peephole off

/* ---------------------------------------------------------------------------------------------------
 * the family work block `fn_800F8788(0x8F4)` attaches to the slot at `_EFT::work_0x38`
 * ------------------------------------------------------------------------------------------------- */

/* The `eft004` family's work block.  `fn_800FE8E8` releases the two effect-heap runs the family fills
 * (`+0x00`/`+0x04` and `+0x7C`/`+0x80`); `fn_800FD864` writes the first run's count and the two spawn
 * parameters at `+0x0D0`/`+0x0D4` that the state-1 handler reads.  The remaining offsets are present in
 * the original object but untouched by this unit's functions, so they stay padding. size: 0x8F4 */
struct _EFT_MAP_WORK {
    /* +0x000 */ s32 count_0x000;             /* the first effect-heap run's count */
    /* +0x004 */ nw4r::ef::Effect* effects_0x004; /* its first handle */
    /* +0x008 */ u8 pad_0x008[0x7C - 0x08];
    /* +0x07C */ s32 count_0x07C;             /* the second effect-heap run's count */
    /* +0x080 */ nw4r::ef::Effect* effects_0x080; /* its first handle */
    /* +0x084 */ u8 pad_0x084[0xD0 - 0x84];
    /* +0x0D0 */ s32 effect_id_0x0D0;         /* the effect id the state-1 handler spawns */
    /* +0x0D4 */ s32 param_0x0D4;             /* its spawn parameter */
    /* +0x0D8 */ u8 pad_0x0D8[0x8D0 - 0xD8];
    /* +0x8D0 */ s32 field_0x8D0;             /* cleared with the counts on every spawn */
    /* +0x8D4 */ u8 flag_0x8D4;               /* per-entry flag the map/area switch sets */
    /* +0x8D5 */ u8 pad_0x8D5[0x8F4 - 0x8D5];
}; /* size: 0x8F4 */

/* ---------------------------------------------------------------------------------------------------
 * externs - the plain helpers, the mangled callees and the sibling's handlers
 * ------------------------------------------------------------------------------------------------- */

/* The map's plain `.text` names carry C linkage.  Their module is undecided (the address band between
 * `sound/fn_800E46E8` and `ef/eft002` names different modules), so they are declared here as the
 * documented rule-2 gap the shared band header leaves. */
extern "C" void* fn_800F8788(u32 work_size);
extern "C" void fn_800F886C(void* self);
extern "C" u8 fn_803AAF88(void);

/* The mangled callees, called through their real signatures (rule 9). */
u8 get_now_mapno();
u8 get_now_areano();
void push_eft_effect_heap_num(nw4r::ef::Effect** effects, long count);

/* The state-0 handler of this family and the three `eft004` handlers states 1-3 tail-call.  The state-0
 * handler is unsplit (it heads the next unclaimed range) and comes from the band header; the three
 * come from eft004's own header (rule 2). */

/* This file's own two hooks, used by the spawner before their definitions. */
extern "C" void fn_800FE8E8(_EFT* self);
extern "C" void fn_800FE93C(_EFT* self);

/* ---------------------------------------------------------------------------------------------------
 * bodies
 * ------------------------------------------------------------------------------------------------- */

/* Spawns the family's effect for the current map and area. */
extern "C" void fn_800FD864(void)
{
    _EFT* self;
    _EFT_MAP_WORK* work;
    u8 mapno;
    u32 areano;
    s32 a;
    s32 b;
    s32 c;

    self = (_EFT*)fn_800F8788(0x8F4);
    if (self == 0) {
        return;
    }
    work = (_EFT_MAP_WORK*)self->work_0x38;
    work->flag_0x8D4 = 0;
    mapno = (u8)get_now_mapno();
    areano = (u8)get_now_areano();
switch (mapno) {
    case 1:
        switch (areano) {
            case 0:
                self->type_0x02 = 0;
                if (fn_803AAF88() == 1) {
                    a = 0x3E;
                    b = 7;
                } else {
                    a = 0x3C;
                    b = 9;
                }
                c = 0xA;
                work->flag_0x8D4 = 1;
                break;
            case 1:
                self->type_0x02 = 1;
                a = 112;
                b = 18;
                c = 15;
                break;
            case 2:
                self->type_0x02 = 0;
                a = 69;
                b = 6;
                c = 11;
                break;
            case 3:
                self->type_0x02 = 0;
                a = 92;
                b = 5;
                c = 13;
                break;
            case 4:
                self->type_0x02 = 0;
                a = 359;
                b = 7;
                c = 42;
                break;
            case 5:
                self->type_0x02 = 0;
                a = 130;
                b = 2;
                c = 16;
                break;
            case 6:
                self->type_0x02 = 0;
                a = 370;
                b = 6;
                c = 44;
                break;
            case 7:
                self->type_0x02 = 1;
                a = 520;
                b = 5;
                c = 53;
                work->flag_0x8D4 = 1;
                break;
            case 8:
                self->type_0x02 = 0;
                a = 348;
                b = 11;
                c = 41;
                break;
            case 9:
                self->type_0x02 = 0;
                a = 75;
                b = 16;
                c = 12;
                break;
            case 10:
                self->type_0x02 = 0;
                a = 100;
                b = 3;
                c = 14;
                break;
            case 11:
                self->type_0x02 = 0;
                a = 389;
                b = 22;
                c = 45;
                break;
            case 12:
                self->type_0x02 = 1;
                a = 135;
                b = 17;
                c = 17;
                work->flag_0x8D4 = 1;
                break;
            default:
                fn_800F886C(self);
                return;
        }
        break;
    case 2:
        switch (areano) {
            case 0:
                self->type_0x02 = 0;
                a = 559;
                b = 7;
                c = 57;
                break;
            case 1:
                self->type_0x02 = 1;
                a = 428;
                b = 3;
                c = 46;
                break;
            case 2:
                self->type_0x02 = 1;
                a = 438;
                b = 2;
                c = 47;
                break;
            case 3:
                self->type_0x02 = 0;
                a = 578;
                b = 1;
                c = 59;
                break;
            case 4:
                self->type_0x02 = 0;
                a = 450;
                b = 7;
                c = 48;
                break;
            case 5:
                self->type_0x02 = 0;
                a = 463;
                b = 9;
                c = 49;
                break;
            case 6:
                self->type_0x02 = 0;
                a = 527;
                b = 4;
                c = 54;
                break;
            case 7:
                self->type_0x02 = 1;
                a = 162;
                b = 2;
                c = 18;
                break;
            case 8:
                self->type_0x02 = 1;
                a = 549;
                b = 2;
                c = 55;
                break;
            case 9:
                self->type_0x02 = 0;
                a = 572;
                b = 3;
                c = 58;
                break;
            case 10:
                self->type_0x02 = 0;
                a = 552;
                b = 7;
                c = 56;
                break;
            case 11:
                self->type_0x02 = 0;
                a = 510;
                b = 9;
                c = 52;
                break;
            default:
                fn_800F886C(self);
                return;
        }
        break;
    case 3:
        switch (areano) {
            case 0:
                self->type_0x02 = 0;
                a = 707;
                b = 25;
                c = 68;
                work->flag_0x8D4 = 0;
                break;
            case 1:
                self->type_0x02 = 0;
                a = 757;
                b = 17;
                c = 72;
                work->flag_0x8D4 = 1;
                break;
            case 2:
                self->type_0x02 = 0;
                a = 877;
                b = 8;
                c = 77;
                work->flag_0x8D4 = 1;
                break;
            case 3:
                self->type_0x02 = 0;
                a = 895;
                b = 4;
                c = 78;
                work->flag_0x8D4 = 1;
                break;
            case 4:
                self->type_0x02 = 0;
                a = 735;
                b = 11;
                c = 70;
                work->flag_0x8D4 = 1;
                break;
            case 5:
                self->type_0x02 = 0;
                a = 797;
                b = 1;
                c = 74;
                work->flag_0x8D4 = 0;
                break;
            case 6:
                self->type_0x02 = 0;
                a = 983;
                b = 9;
                c = 83;
                work->flag_0x8D4 = 1;
                break;
            case 7:
                self->type_0x02 = 0;
                a = 923;
                b = 8;
                c = 80;
                work->flag_0x8D4 = 0;
                break;
            case 8:
                self->type_0x02 = 0;
                a = 969;
                b = 11;
                c = 82;
                work->flag_0x8D4 = 1;
                break;
            case 9:
                self->type_0x02 = 0;
                a = 953;
                b = 10;
                c = 81;
                work->flag_0x8D4 = 0;
                break;
            case 10:
                self->type_0x02 = 0;
                a = 915;
                b = 3;
                c = 79;
                work->flag_0x8D4 = 0;
                break;
            default:
                fn_800F886C(self);
                return;
        }
        break;
    case 4:
        switch (areano) {
            case 0:
                self->type_0x02 = 0;
                a = 1379;
                b = 18;
                c = 102;
                break;
            case 1:
                self->type_0x02 = 1;
                a = 1397;
                b = 2;
                c = 103;
                break;
            case 2:
                self->type_0x02 = 1;
                a = 1409;
                b = 2;
                c = 104;
                break;
            case 3:
                self->type_0x02 = 1;
                a = 665;
                b = 6;
                c = 64;
                break;
            case 4:
                self->type_0x02 = 1;
                a = 1415;
                b = 12;
                c = 105;
                break;
            case 5:
                self->type_0x02 = 0;
                a = 1427;
                b = 9;
                c = 106;
                break;
            case 6:
                self->type_0x02 = 1;
                a = 1445;
                b = 15;
                c = 107;
                break;
            case 7:
                self->type_0x02 = 1;
                a = 1460;
                b = 1;
                c = 108;
                break;
            case 8:
                self->type_0x02 = 0;
                a = 1467;
                b = 5;
                c = 109;
                break;
            case 9:
                self->type_0x02 = 0;
                a = 1483;
                b = 1;
                c = 110;
                break;
            default:
                fn_800F886C(self);
                return;
        }
        break;
    case 5:
        switch (areano) {
            case 0:
                self->type_0x02 = 0;
                a = 812;
                b = 11;
                c = 75;
                break;
            case 1:
                self->type_0x02 = 0;
                a = 1068;
                b = 5;
                c = 86;
                break;
            case 2:
                self->type_0x02 = 0;
                a = 1080;
                b = 2;
                c = 87;
                break;
            case 3:
                self->type_0x02 = 0;
                a = 1132;
                b = 10;
                c = 89;
                break;
            case 4:
                self->type_0x02 = 0;
                a = 1170;
                b = 4;
                c = 92;
                break;
            case 5:
                self->type_0x02 = 0;
                a = 1190;
                b = 8;
                c = 93;
                break;
            case 6:
                self->type_0x02 = 0;
                a = 1247;
                b = 3;
                c = 96;
                break;
            case 7:
                self->type_0x02 = 0;
                a = 1333;
                b = 8;
                c = 99;
                break;
            case 8:
                self->type_0x02 = 0;
                a = 1345;
                b = 16;
                c = 100;
                break;
            case 9:
                self->type_0x02 = 0;
                a = 1362;
                b = 2;
                c = 101;
                break;
            case 10:
                self->type_0x02 = 0;
                a = 782;
                b = 6;
                c = 73;
                break;
            default:
                fn_800F886C(self);
                return;
        }
        break;
    case 6:
        switch (areano) {
            case 0:
                self->type_0x02 = 0;
                a = 1600;
                b = 6;
                c = 115;
                break;
            case 1:
                self->type_0x02 = 0;
                a = 651;
                b = 3;
                c = 61;
                break;
            case 2:
                self->type_0x02 = 1;
                a = 1606;
                b = 4;
                c = 116;
                break;
            default:
                fn_800F886C(self);
                return;
        }
        break;
    case 7:
        switch (areano) {
            case 0:
                self->type_0x02 = 0;
                a = 1096;
                b = 22;
                c = 88;
                break;
            case 1:
                self->type_0x02 = 0;
                a = 2470;
                b = 2;
                c = 181;
                break;
            case 2:
                self->type_0x02 = 0;
                a = 2473;
                b = 1;
                c = 182;
                break;
            case 3:
                self->type_0x02 = 0;
                a = 1302;
                b = 7;
                c = 97;
                break;
            default:
                fn_800F886C(self);
                return;
        }
        break;
    case 8:
        switch (areano) {
            case 1:
                self->type_0x02 = 1;
                a = 1008;
                b = 4;
                c = 84;
                break;
            default:
                fn_800F886C(self);
                return;
        }
        break;
    case 9:
        switch (areano) {
            case 0:
                self->type_0x02 = 1;
                a = 1014;
                b = 4;
                c = 85;
                break;
            default:
                fn_800F886C(self);
                return;
        }
        break;
    case 10:
        switch (areano) {
            case 0:
                self->type_0x02 = 0;
                a = 1720;
                b = 13;
                c = 118;
                break;
            case 1:
                self->type_0x02 = 0;
                a = 1745;
                b = 12;
                c = 119;
                break;
            default:
                fn_800F886C(self);
                return;
        }
        break;
    case 12:
        switch (areano) {
            case 0:
                self->type_0x02 = 0;
                if (fn_803AAF88() == 1) {
                    a = 1768;
                    b = 7;
                } else {
                    a = 1766;
                    b = 9;
                }
                c = 120;
                work->flag_0x8D4 = 1;
                break;
            case 1:
                self->type_0x02 = 1;
                a = 1856;
                b = 5;
                c = 129;
                break;
            case 2:
                self->type_0x02 = 0;
                a = 1775;
                b = 7;
                c = 121;
                break;
            case 3:
                self->type_0x02 = 0;
                a = 1782;
                b = 5;
                c = 122;
                break;
            case 4:
                self->type_0x02 = 0;
                a = 1787;
                b = 7;
                c = 123;
                break;
            case 5:
                self->type_0x02 = 0;
                a = 1794;
                b = 2;
                c = 124;
                break;
            case 6:
                self->type_0x02 = 0;
                a = 1799;
                b = 6;
                c = 125;
                break;
            case 7:
                self->type_0x02 = 1;
                a = 1838;
                b = 4;
                c = 128;
                break;
            case 8:
                self->type_0x02 = 0;
                a = 1813;
                b = 11;
                c = 126;
                break;
            case 9:
                self->type_0x02 = 0;
                a = 1824;
                b = 11;
                c = 127;
                break;
            case 10:
                self->type_0x02 = 0;
                a = 1861;
                b = 2;
                c = 130;
                break;
            case 11:
                self->type_0x02 = 0;
                a = 1870;
                b = 22;
                c = 131;
                break;
            case 12:
                self->type_0x02 = 1;
                a = 1892;
                b = 4;
                c = 132;
                break;
            default:
                fn_800F886C(self);
                return;
        }
        break;
    case 13:
        switch (areano) {
            case 0:
                self->type_0x02 = 0;
                a = 1896;
                b = 6;
                c = 133;
                break;
            case 1:
                self->type_0x02 = 1;
                a = 1902;
                b = 4;
                c = 134;
                break;
            case 2:
                self->type_0x02 = 1;
                a = 1907;
                b = 3;
                c = 135;
                break;
            case 3:
                self->type_0x02 = 0;
                a = 1913;
                b = 5;
                c = 136;
                break;
            case 4:
                self->type_0x02 = 0;
                a = 1924;
                b = 8;
                c = 137;
                break;
            case 5:
                self->type_0x02 = 0;
                a = 1939;
                b = 9;
                c = 138;
                break;
            case 6:
                self->type_0x02 = 0;
                a = 1953;
                b = 5;
                c = 139;
                break;
            case 7:
                self->type_0x02 = 1;
                a = 1975;
                b = 1;
                c = 140;
                break;
            case 8:
                self->type_0x02 = 1;
                a = 1977;
                b = 3;
                c = 141;
                break;
            case 9:
                self->type_0x02 = 0;
                a = 1980;
                b = 3;
                c = 142;
                break;
            case 10:
                self->type_0x02 = 0;
                a = 1986;
                b = 7;
                c = 143;
                break;
            case 11:
                self->type_0x02 = 0;
                a = 1993;
                b = 9;
                c = 144;
                break;
            default:
                fn_800F886C(self);
                return;
        }
        break;
    case 14:
        switch (areano) {
            case 0:
                self->type_0x02 = 0;
                a = 2002;
                b = 21;
                c = 145;
                break;
            case 1:
                self->type_0x02 = 0;
                a = 2023;
                b = 9;
                c = 146;
                break;
            case 2:
                self->type_0x02 = 0;
                a = 2040;
                b = 9;
                c = 147;
                break;
            case 3:
                self->type_0x02 = 0;
                a = 2062;
                b = 3;
                c = 148;
                work->flag_0x8D4 = 1;
                break;
            case 4:
                self->type_0x02 = 0;
                a = 2069;
                b = 13;
                c = 149;
                work->flag_0x8D4 = 1;
                break;
            case 5:
                self->type_0x02 = 0;
                a = 2082;
                b = 4;
                c = 150;
                work->flag_0x8D4 = 0;
                break;
            case 6:
                self->type_0x02 = 0;
                a = 2092;
                b = 9;
                c = 151;
                work->flag_0x8D4 = 1;
                break;
            case 7:
                self->type_0x02 = 0;
                a = 2106;
                b = 2;
                c = 152;
                work->flag_0x8D4 = 0;
                break;
            case 8:
                self->type_0x02 = 0;
                a = 2114;
                b = 9;
                c = 153;
                work->flag_0x8D4 = 1;
                break;
            case 9:
                self->type_0x02 = 0;
                a = 2123;
                b = 7;
                c = 154;
                work->flag_0x8D4 = 0;
                break;
            case 10:
                self->type_0x02 = 0;
                a = 2132;
                b = 3;
                c = 155;
                work->flag_0x8D4 = 0;
                break;
            default:
                fn_800F886C(self);
                return;
        }
        break;
    case 15:
        switch (areano) {
            case 0:
                self->type_0x02 = 0;
                a = 2141;
                b = 18;
                c = 157;
                break;
            case 1:
                self->type_0x02 = 1;
                a = 2159;
                b = 2;
                c = 158;
                break;
            case 2:
                self->type_0x02 = 1;
                a = 2171;
                b = 2;
                c = 159;
                break;
            case 3:
                self->type_0x02 = 1;
                a = 2174;
                b = 5;
                c = 160;
                break;
            case 4:
                self->type_0x02 = 1;
                a = 2212;
                b = 12;
                c = 162;
                break;
            case 5:
                self->type_0x02 = 0;
                a = 2224;
                b = 9;
                c = 163;
                break;
            case 6:
                self->type_0x02 = 1;
                a = 2242;
                b = 15;
                c = 164;
                break;
            case 7:
                self->type_0x02 = 1;
                a = 2257;
                b = 1;
                c = 165;
                break;
            case 8:
                self->type_0x02 = 0;
                a = 2261;
                b = 5;
                c = 166;
                break;
            case 9:
                self->type_0x02 = 0;
                a = 2277;
                b = 1;
                c = 167;
                break;
            default:
                fn_800F886C(self);
                return;
        }
        break;
    case 16:
        switch (areano) {
            case 0:
                self->type_0x02 = 0;
                a = 2298;
                b = 9;
                c = 168;
                break;
            case 1:
                self->type_0x02 = 0;
                a = 2307;
                b = 5;
                c = 169;
                break;
            case 2:
                self->type_0x02 = 0;
                a = 2318;
                b = 2;
                c = 170;
                break;
            case 3:
                self->type_0x02 = 0;
                a = 2326;
                b = 10;
                c = 171;
                break;
            case 4:
                self->type_0x02 = 0;
                a = 2349;
                b = 4;
                c = 172;
                break;
            case 5:
                self->type_0x02 = 0;
                a = 2368;
                b = 8;
                c = 173;
                break;
            case 6:
                self->type_0x02 = 0;
                a = 2384;
                b = 3;
                c = 174;
                break;
            case 7:
                self->type_0x02 = 0;
                a = 2398;
                b = 8;
                c = 175;
                break;
            case 8:
                self->type_0x02 = 0;
                a = 2411;
                b = 16;
                c = 176;
                break;
            case 9:
                self->type_0x02 = 0;
                a = 2429;
                b = 2;
                c = 177;
                break;
            case 10:
                self->type_0x02 = 0;
                a = 2434;
                b = 6;
                c = 178;
                break;
            default:
                fn_800F886C(self);
                return;
        }
        break;
    case 17:
        switch (areano) {
            case 0:
                self->type_0x02 = 0;
                a = 2498;
                b = 6;
                c = 186;
                break;
            case 1:
                self->type_0x02 = 0;
                a = 2504;
                b = 3;
                c = 187;
                break;
            case 2:
                self->type_0x02 = 1;
                a = 2516;
                b = 4;
                c = 188;
                break;
            default:
                fn_800F886C(self);
                return;
        }
        break;
    case 18:
        switch (areano) {
            case 0:
                self->type_0x02 = 0;
                a = 2455;
                b = 15;
                c = 180;
                break;
            case 1:
                self->type_0x02 = 0;
                a = 2477;
                b = 2;
                c = 183;
                break;
            case 2:
                self->type_0x02 = 0;
                a = 2480;
                b = 1;
                c = 184;
                break;
            case 3:
                self->type_0x02 = 0;
                a = 2484;
                b = 7;
                c = 185;
                break;
            default:
                fn_800F886C(self);
                return;
        }
        break;
    case 19:
        switch (areano) {
            case 1:
                self->type_0x02 = 1;
                a = 2441;
                b = 4;
                c = 179;
                break;
            default:
                fn_800F886C(self);
                return;
        }
        break;
    case 21:
        self->field_0x07 = 1;
        switch (areano) {
            case 0:
                self->type_0x02 = 0;
                b = 4;
                a = 603;
                c = 8;
                break;
            case 1:
                self->type_0x02 = 0;
                b = 10;
                a = 3;
                c = 4;
                break;
            case 2:
                self->type_0x02 = 0;
                b = 4;
                a = 607;
                c = 9;
                break;
            case 3:
                self->type_0x02 = 0;
                b = 2;
                a = 611;
                c = 10;
                break;
            case 5:
                self->type_0x02 = 0;
                b = 1;
                a = 613;
                c = 11;
                break;
            case 6:
                self->type_0x02 = 0;
                b = 2;
                a = 614;
                c = 12;
                break;
            case 7:
                self->type_0x02 = 0;
                b = 1;
                a = 616;
                c = 13;
                break;
            default:
                fn_800F886C(self);
                return;
        }
        break;
    case 22:
        self->field_0x07 = 1;
        switch (areano) {
            case 0:
                self->type_0x02 = 0;
                b = 18;
                a = 175;
                c = 7;
                break;
            case 1:
                self->type_0x02 = 0;
                b = 10;
                a = 1048;
                c = 23;
                break;
            case 2:
                self->type_0x02 = 0;
                b = 6;
                a = 1019;
                c = 21;
                break;
            default:
                fn_800F886C(self);
                return;
        }
        break;
default:
    fn_800F886C(self);
    return;
}
work->count_0x000 = b;
work->effect_id_0x0D0 = a;
work->param_0x0D4 = c;
work->count_0x07C = 0;
work->field_0x8D0 = 0;
self->field_0x03 = 3;
self->area_0x44 = areano;
fn_800F9DF4(self, 0, 0);
self->release_0x40 = fn_800FE8E8;
self->dispatch_0x34 = fn_800FE93C;
}

/* The `+0x40` release hook: hands the two effect-heap runs back to the pool and zeroes their counts. */
void fn_800FE8E8(_EFT* self)
{
    _EFT_MAP_WORK* work;

    work = (_EFT_MAP_WORK*)self->work_0x38;
    push_eft_effect_heap_num(&work->effects_0x004, work->count_0x000);
    work->count_0x000 = 0;
    push_eft_effect_heap_num(&work->effects_0x080, work->count_0x07C);
    work->count_0x07C = 0;
}

/* The `+0x34` state dispatcher: tail-calls the handler for the current state. */
void fn_800FE93C(_EFT* self)
{
    switch (self->state_0x05) {
    case 0:
        fn_800FE978(self);
        return;
    case 1:
        fn_800FF8D4(self);
        return;
    case 2:
        fn_800FFC98(self);
        return;
    case 3:
        fn_800FFCA8(self);
        return;
    }
}
