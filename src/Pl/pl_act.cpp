/*
 * Player action module (Pl_act): the largest of the three Pl clusters. .text 0x80276B58-0x8027D684
 * (115 functions, 0x6B2C B) with its own exception tables - extab 0x8001294C-0x80012B54, extabindex
 * 0x8002FC70-0x8002FF7C.
 *
 * Left edge pinned by the `.sdata2` pool run (`lbl_8079A0AC`), right edge by the closure; the reasoning is in
 * configure.py beside the Pl lib entry.
 *
 * It is C++ (the map holds the mangled `Pl_attack_set_sub__FP4_PLWP9_HIT_DATAP6_HIT_Ws`,
 * `Pl_suimen_ck__FP4_PLW`, `Pl_get_gunner_pos__FP4_PLWPQ34nw4r4math4VEC3l`, ...), so the actor type is
 * `_PLW` - that spelling is what the map's mangling encodes - and every unmangled `fn_*` callee is
 * `extern "C"`.
 *
 * Flags - measured against `build/RMHE08/obj/Pl/pl_act.o`, and **the store needs two changes** (both are a
 * this-unit `cflags_pl_act`, never `cflags_base`):
 *   * `-O4,p` -> **`-O3`**. `-O4,p` implies `-func_align 16`; the retail function starts are packed on 4 B
 *     (+0x23c, +0x588, +0xcbc), which alone rules it out. On the codegen axis `-O4,p` also loses badly:
 *     fn_80276B58 85.9 / fn_80276CE8 84.7 / fn_80276D94 86.1 / fn_80276E08 87.9, against `-O3`'s
 *     97.3 / 90.0 / 96.4 / 89.6 (same source).
 *   * peephole **off** (`-opt nopeephole`, i.e. `-O3 -opt nopeephole`): the retail object carries exactly
 *     one record-form instruction in all 115 functions (`andi. r0,r0,20` at 0xcbc, an `x & 0x20`
 *     truth test from instruction selection), so the peephole's sign/zero-extend compare fusion never
 *     fired in the retail build. With it on, fn_80276B58 = 97.3 / fn_80276CE8 = 90.0 /
 *     fn_80276D94 = 96.4 / fn_80276E08 = 89.6; with it off, 100.0 / 92.7 / 100.0 / 94.0.
 *     `-O4,p -opt nopeephole` is worse still (92.5 / 87.4 / 78.7 / 93.9), so the level is `-O3`.
 *   * `-inline auto` -> **`-inline noauto`** (playbook 28): with `auto` the 46-instruction
 *     `fn_802770E8` is inlined into all three arms of `fn_802771A0` (57 -> 229 instructions, 0.00 %);
 *     `noauto` puts it back at 100.00 % and moves nothing else. `-inline off` measures identically here,
 *     but `noauto` is the spelling the sibling `main.cpp` needed, so it is the one to commit.
 *   * `-func_align` stays out: `-O3` already packs on 4 B, which is what the retail starts want.
 *   * the s16-parameter convention: retail sign-extends a `s16` parameter at its first use and keeps the
 *     *raw* register live, which MWCC only does when the parameter is declared `s32` and cast to `s16` at
 *     each use (`fn_80276D94`: `s16 arg1` truncates the `+= 150` and costs 96.4 %, `s32 arg1` + `(s16)`
 *     casts is byte-identical). The unmangled `fn_*` symbols are free to be declared this way, but a
 *     callee's *declaration* has to match its definition's type, so the shared `fn_80276CE8` is declared
 *     `s32` and its call sites cast explicitly.
 *
 * Residual (work in progress - the functions below 100 %, each measured with the flags above, i.e.
 * `-O3 -opt nopeephole -inline noauto`):
 *   * the unclaimed `.sdata2` pool (playbook 23): the unit's own run is `0x8079A080-0x8079A114`
 *     (35 labels - `A080`-`A0B8`, with the two int->float magics as f64s at `A0A0` and `A0B8`, then
 *     `A0C0`-`A110`). All of them are `extern`-declared and used as load operands (`A0C0`, `A0D4`,
 *     `A0D8`, `A0F8` came in with the last bodies), so the `.sdata2` rows pair by name. Three entries
 *     are *shared*: `A080`, `A084` and `A088` are also loaded by `fn_802756F0` / `fn_8027633C` (the
 *     unsplit run before the unit) and `fn_8027D968` (after it), so no contiguous claim can make those
 *     three private - the claim has to leave them global. The only pool-*address* mismatch left is
 *     `.sdata` `lbl_80792150` (`.sdata 0x80792150-0x80792158`), which needs the claim plus the scalar
 *     declaration noted below.
 *   * a byte/short local that accumulates in 32 bits and is sign-extended only where it is *compared*:
 *     retail emits the `extsb`/`extsh` at the compare, not at the assignment. Write `s32 v = *(u8*)...`
 *     and cast at each comparison (`(s8)v <= n`); declaring the local `s8`/`s16` makes MWCC convert at the
 *     assignment instead and shifts the whole function (791FC, 79490, Get_Shell_bure_type, B0BC).
 *   * storing an int into a `u8` field: MWCC inserts a redundant `clrlwi r0,r0,24` where retail's `stb`
 *     truncates. The compound operators (`++`, `--`, `+=`, `-=`) on the lvalue avoid it in some arms only,
 *     so B0BC keeps a residual `clrlwi` on the two `w +- 40` clamp stores.
 *   * the `x > N ? 2 : 1` threshold idiom - **resolved this pass**: the branchless form MWCC picks is
 *     decided by which side of the comparison is written first. `x > N ? 2 : 1` gives the unsigned-bool
 *     form (`xori/srawi/srwi/...`), the negated `x <= N ? 1 : 2` gives retail's
 *     `xoris; subfic; addc; subfe; addi 2`, and the equality form `x == N ? a : b` gives retail's
 *     `addi/subfic/nor/srawi` (Get_Shell_bure_type 90.2 -> 96.7, 791FC 86.8 -> 90.8, 79490 94.2 -> 97.5).
 *   * array base/index order for an offset access - **resolved this pass**: `*(u8*)(lbl + idx * 4 + k)`
 *     makes MWCC compute the index first and fold the base into a scratch register; moving the constant
 *     *before* the index (`*(u8*)(lbl + k + idx * 4)`) makes it compute the base (`lis/addi`) first and
 *     then `add r3,r3,r0`, which is retail (Get_Shell_bure_type 90.2 -> 93.6, 791FC).
 *   * `!= 0` on a `u8` load pairs as `cmpwi` only through a `(s32)` cast (78D1C, B918, B0BC).
 *   * an `if`/`else if` chain whose arm bodies MWCC lays out in reverse has to be written as m2c's
 *     negated/nested form (`>`, `!=`, with the bodies as the `else` arms); the natural `<=` chain flips
 *     the branch polarity and costs a row per arm (78D1C's three-way action-id test).
 *   * D0D4 is size- and instruction-exact with a *different callee-saved numbering only*: retail gives the
 *     switch jump table `r23` and the loop bound `r30`, our allocator inverts that (playbook 22). Its 15-case
 *     `switch` is a jump table (`jumptable_805C61F8`) and one arm assigns the loop counter (`i = 5`) - both
 *     load-bearing.
 *   * 7885C: `(s16)fn_802753E4(...)` - retail's `mr r0,r3; mr r3,self; extsh r4,r0` says the shared helper
 *     returns `s16`, but changing that declaration would disturb 789EC/78D1C, so the cast stays and each of
 *     the five call sites costs one row.
 *   * B358/C89C-family: `fn_803B6150`/`fn_802731B4`/`fn_803B31E0` take/return `s32` (not `s16`/`s8`) - the
 *     missing `extsb` at the call site is the tell; and an `s8` field assigned from an `s32` local needs the
 *     field typed `s8` so `stb` keeps retail's `extsb`.
 *   * `int -> s16`/`s8` store conversion (A044, CA48, 76CE8, 76E08): retail stores the *raw* int sum
 *     (`add r5,r0,r4; sth r5`) and sign-extends only for the compare that follows (`extsh r0,r5`); our
 *     build materialises the s16 value at the store and reuses it. A compound assignment stores raw only
 *     when the RHS is already the field's type (`+= (s8)arg1`) or the operator is `--`/`++`; with a wider
 *     RHS MWCC inserts the conversion. Eight source shapes per function gave the same or an extra row.
 *   * a dead second test whose condition register retail reuses (AF34): keeping the pre-decrement value
 *     in an `s16` local and writing an unreachable `if (v < 0) return;` after the store reproduces
 *     `blelr ... sth ... bltlr` exactly, and is load-bearing - do not "clean it up".
 *   * load-before-pointer-formation (79414, Get_Shell_rate_adj): retail does `lbz r0,0x1e8(r30)` before
 *     `addi r3,r30,0x1e8`, ours forms the pointer first. Two shapes tried (field vs offset cast, a local
 *     for the byte) do not move it.
 *   * boolean-chain layout (7BC48, 784B8, CB1C, D40C): the `||`/`&&` short-circuit blocks, and the two
 *     induction variables of the 32-bit scan, come out in different registers/order; codes are equivalent
 *     but the rows do not pair. 7CFC0 additionally gets two extra `extsh` before its clamp stores.
 *   * `mulli` vs the shift/subf/shift form of `* 14` (78590): MWCC folds `* 7 * 2` before strength
 *     reduction, so the three-instruction form cannot be recovered from a constant product.
 *   * A57C (1668 B): the body (the flag reads, the `fn_803BECA0` swaps, the `lbl_805C6118`
 *     interpolation loop, the per-motion write-back) is instruction-for-instruction, but the two shared
 *     tails were reached through labels until the conformance pass below; see its note for the residual.
 *   * `Pl_zanzo_set` and C89C/A044 read `Get_motion_no` as `s16`: the shared declaration has to stay `s16`
 *     (a `u16` return drops Pl_zanzo_set from 100 to 94.2), so A57C casts at its own call site.
 *
 * Conformance pass (worker `pl-act-09c6`, rule 8: no label as a control-flow device). Four
 * functions carried one - 78144/78310 (their own `ret0`/`ret1` labels), BC48 (`ret1`/`ret0`) and A57C
 * (`block_16`/`block_53`/`block_75`). Every conformant shape is a deliberate approximation of retail's
 * flow, because retail's own source clearly used labels; each was measured against a set of alternatives
 * and the best kept (the numbers are this pass's, `-` = that function's own percent):
 *   * BC48 100 -> 99.24: `do { ... } while (0)` around the whole dispatch. The `(u32)(x - 6) <= 2` test
 *     `break`s out of the loop (a single `ble` into the shared `return 1`, exactly retail's row) and the
 *     case-3/5 failures `return 0` inline. Retail instead branches *to* a shared `return 0`, so those two
 *     arms keep their polarity inverted (2 rows, 152 B either way). Alternatives measured: plain returns
 *     94.47, `switch` case-group 93.68, one result variable 63.6, inline `return 1` per arm 94.47.
 *   * 78144 100 -> 97.74, 78310 100 -> 97.38: the outer `if (m != 9) ... else` becomes a `switch (m)` whose
 *     `default:` (the m != 9 body) is written FIRST and whose `case 9:` is last, which keeps the bodies in
 *     retail's address order and lets the two labelled exits disappear. The residual is the early
 *     `(u32)((u8)m - 6) <= 1` exit: retail reaches it with one `ble` into the block `case 10:` also uses,
 *     and two separate `return 0;` statements cannot share a block without the label, so ours inlines the
 *     `li r3,0; b epilogue` pair (+8 B each). Alternatives measured: compound `||` condition 84.7,
 *     `for (;;)`/`break` dispatch 87.0, shared result variable 67.5.
 *   * A57C 99.96 -> 97.63: the outer dispatch is a `for (;;)` whose `break` carries every "skip the body"
 *     case (retail's `bne 3b40`/`beq 3fc0` edges come out as the same edges), and the body's `block_53` /
 *     `block_75` jumps are gone because the case-0/2 arms now hoist `mag = 5` above their flag-sum test and
 *     `if (mag == 0)` selects the pad scan - `mag` is the discriminator, so no extra live variable is
 *     needed. Same instruction count (417), ~126 rows differ: retail's own dispatch is `switch`-shaped
 *     (`cmpwi 0; beq; cmpwi 2; beq; b body`) where the conformant if/else-if form makes MWCC interleave the
 *     case tests. Alternatives measured: compound `||` dispatch 96.02, extra `skip_scan` variable 96.25,
 *     `mag = 1` marker 97.46, dropping the flag-sum test 94.30.
 *
 * Written so far: all 115 bodies. This pass added the last 13, biggest first: C208 (1684, 93.9),
 * 79C20 (668, 94.3), 7993C (584, 95.4), AC2C (508, 95.4), 77974 (464, 97.2), A340 (428, 94.2),
 * C064 (420, 97.9), 78674 (416, 97.0), 78144 (372, 82.6), 77FF8 (332, 93.5), 79EBC (324, 100.0),
 * 78310 (320, 82.9), 77DAC (276, 97.1); unit fuzzy 70.4 -> 93.8 %, matched bytes 9316 -> 9640,
 * 70 of 115 functions byte-exact, none unwritten. The three source shapes that did most of that, in
 * order of payoff:
 *   * a `switch` whose case bodies end in `return 1` gets folded to a branchless bool; writing `break`
 *     and one `return 1` after the switch restores retail's branchy shared tail (77FF8 72.5 -> 93.5,
 *     78144 66.7 -> 82.6, 78310 66.7 -> 82.9, C208 86.8 -> 93.9).
 *   * an equality on a `u8` lvalue compiles as `cmplwi`; `(s32)(u8)x` compiles as retail's `cmpwi`
 *     (78310 82.2 -> 82.9, and the same rows in 77FF8/78144/C064).
 *   * a small dense `switch` whose *default* body retail emits first means the source's `default:`
 *     (or its highest case) is written first, with the low case bodies after it (C064 71.5 -> 97.9).
 *   * `extern u8 lbl_80792150;` + `&lbl_80792150` gives retail's `li r3, X@sda21` address form where
 *     `extern u8 lbl_80792150[]` gave `lis/addi` (Get_Shell_bure_type 78.6 -> 79.9).
 *
 * This pass (worker `pl-act-09c6`) took the unit 93.73 -> 97.61 % (matched 25714 -> 26780 B; 73 of the 115
 * functions byte-exact) and closed all five symbols below the 80 % bar. The shapes that did it, in order
 * of payoff:
 *   * a sparse `switch` whose first case *falls through into `default:`*: retail's `cmpwi; beq A; cmpwi; beq A;
 *     cmpwi 9; beq B; b default` with A out of line and A's failure falling into `default` is
 *     `switch (m) { case 6: case 17: <body> (falls through) default: ...; case 9: ... }` - `default`
 *     written *between* the cases so the bodies land in address order (784B8 66.6 -> 100.0).
 *   * a big `if` whose failing edge falls into the *next* arm's body: move the shared tail out of the `else`
 *     of the first arm and make it the `switch`'s `default:` after the last case (A57C 66.0 -> 96.0; its
 *     only residual is the two implicit int->float magics, which cannot be named from source - playbook 29).
 *   * a `return` block MWCC refuses to merge: 78144/78310 needed a shared `ret0`/`ret1` label (95.6 -> 100.0,
 *     94.9 -> 100.0) and BC48 a shared `ret1` (94.5 -> 100.0). Rule 8 removed the label again
 *     in the conformance pass below, which records what replaced it and what that cost.
 *   * `(s32)arg0 == N` for a `u8` parameter gives retail's `cmpwi` where `arg0 == N` gives `cmplwi`
 *     (78144, 78310).
 *   * an accumulator retail keeps in 32 bits and sign-extends only at each *compare*: declare it `s32`, write
 *     the widening adds as `b = (s16)(b + N)` on exactly the arms that need the conversion, and let the final
 *     difference reuse the minuend's register by writing `a -= b` on an `s32 a` (79490, 791FC 97.8 -> 100.0,
 *     BCE0 85.3 -> 93.4).
 *   * `n = (f32)(s16)arg1` hoisted into a local is wrong: retail recomputes the int->float magic at every
 *     use, so the conversion has to be written inline at each division (AE28 74.6 -> 99.9, frame 32 -> 48 B).
 *   * the 24-entry `{u16 id; s16 count}` table at 0x278 has to be a *typed array field*, not
 *     `*(u16*)((u8*)self + 0x278 + i * 4)`: the field form gives retail's `lhz r4,632(r3)` displacement loads
 *     and drops the two pointer-materialising `addi`s (BCE0).
 *
 * Round 2 (worker `pl-act-09c6`, same 73 byte-exact functions): 97.42 -> 97.87 %, three functions moved,
 * no flag change. The shapes that did it:
 *   * C208 93.9 -> 99.97 (`.text` now the target's exact 1684 B): retail's switch ends in `return 0` with an
 *     explicit `default: return 1`, and every "not allowed" path is a `break` - so retail shares *one*
 *     `li r3,0` at the function's end and inlines the `li r3,1`s, where the old `return 1`-after-the-switch /
 *     `return 0`-per-case form did the opposite (16 extra instructions). A case body written
 *     `if (c) return 0; break;` has to become its negation, `if (!c) return 1; break;`, because
 *     `if (c) return 0; return 1;` makes MWCC if-convert the two constant arms into a branchless bool.
 *     Its only residual is the frame: 0x30 against retail's 0x40, i.e. 16 B of locals the function never
 *     touches - the target has no stack access we do not, and MWCC drops unused locals (4 `s32`, a `VEC3`
 *     and an `f64` all measured), so the frame cannot be reached from source.
 *   * A340 94.2 -> 98.1: the `switch`'s cases are written in *body address* order (case 0, case 4, case 2 -
 *     not numeric), and the `(u32)(a - 5) <= 6` guard is negated with its arms swapped
 *     (`if ((u32)(a - 5) > 6U) { switch ... } else { ok = 0; }`) so `ok = 0` lands at the end and the
 *     failing edge branches to it. Residual: 2 rows (an extra `li r31,0`/`b` pair where retail shares
 *     case 2's block with the guard's edge).
 *   * Pl_bari_ck 84.4 -> 86.7: `s32 id = self->unk00C;` and `s32 m = (u16)Get_motion_no(self);` - the `u16`
 *     locals made the equality tests `cmplwi` where retail has `cmpwi` (the rule in "Load-bearing source
 *     shapes" below). Residual: retail lays the first arm's body out of line and the second arm's inline
 *     (`ble`/`ble` into the body, then the next arm's tests); the natural `if`/`else if` gives the mirror
 *     image and both rewrites tried (negated `&&` with the arms swapped, two separate `if`s) measured
 *     75.0 and 76.7.
 *
 * Still open in the new bodies: 77FF8 has ~6 rows left in the `t`-vs-`arg1` test polarity; 7885C's five
 * `(s16)fn_802753E4(...)` call sites schedule `extsh r4,r3` before `mr r3,self` where retail copies the
 * result first (`mr r0,r3; mr r3,self; extsh r4,r0`); BCE0 keeps only a `v`/`id` callee-saved swap (retail
 * v=r6/id=r4, ours the mirror); B0BC's six `clrlwi r0,r0,24` in front of the `stb`s into
 * `q + 0x5E1` are a flag-level residual, not source-shaped - the peephole removes them but also turns
 * `extsb`/`clrlwi` + `cmpwi` into record forms retail does not have (whole-function `#pragma peephole on`:
 * 93.95 -> 92.28), and `+= (s8)7`, an `s8` field, a `u8` local and an explicit `(u8)` cast all measured
 * identically.
 *
 * Round 3 (worker `pl-act-09c6`, 80 byte-exact functions): 97.87 -> 98.39 %, seven more functions to 100 %,
 * no flag change. The shapes that did it, in order of payoff:
 *   * an `s16` *parameter* plus a compound assignment is what stores a field raw. With `s32 arg1` and
 *     `field = field + arg1` MWCC materialises the `s16` at the store and reuses it for the following
 *     compare; declaring the parameter `s16` and writing `field += arg1` stores the raw sum and extends only
 *     for the compare (A044 82.2 -> 100, 76CE8 92.7 -> 100, CA48 94.0 -> 100, 76E08 98.0 -> 99.9,
 *     78674 97.0 -> 99.0). The float form is the same: `field += (s16)f` (C8B4 95.6 -> 100, D5A4 96.3 ->
 *     100), and `field -= (s16)v` stores raw where `field -= v` / `field = t` extend at the store (76E08);
 *     `field--` stores raw where `field = field - 1` extends (B0BC 94.0 -> 95.7).
 *   * a byte load followed by pointer formation has to use the *field* form, not the field's address: 79414
 *     93.4 -> 100 and Get_Shell_rate_adj 95.0 -> 99.9 read `self->equipC[0]`/`self->equipC` where
 *     `*((u8*)self + 488)` made MWCC form the pointer before the load.
 *   * `(u32)` on an `s32`-returning helper turns `cmpwi` into retail's `cmplwi` (`(u32)Pl_master_ck`,
 *     `(u32)fn_802753E4`): C8B4, 789EC.
 *   * a negative 16-bit constant written `-0x1006` gives `li r4,-0x1006` where the unsigned `0xEFFA` needs
 *     `lis/subi` (77C94 94.9 -> 97.1).
 *   * 78590's `* 14`: `(x * 7) << 1` keeps MWCC's strength reduction (`slwi/subf/slwi`) where `x * 7 * 2`
 *     folds to `mulli`, and `v <= t[5] ? 5 : 6` gives the negated branchless form (87.0 -> 99.2).
 *   * a `u16` loop index cast to `s16` per iteration reproduces retail's `extsh` (7993C 95.4 -> 96.2);
 *     local declaration order decides the stack slots (swapping the `VEC3`/buffer pair fixed C064 97.9 ->
 *     98.1).
 *   * two real layout bugs: 789EC stored `fn_802753E4(self, 5)` into `unk449` (0x449) where retail writes
 *     0x44C; and `_MOVE_WORK`'s `unk44F[0x464 - 0x44F]` was mis-sized by one byte, so `unk464`/`unk466`
 *     landed at 0x466/0x468 - renaming it `unk450[0x464 - 0x450]` puts them back at 0x464/0x466 (D0D4).
 *
 * Source-order caveat: the bodies were appended in per-batch address order, not as one address-ordered
 * list, so the file is *not* in `.text` order any more (Pl_attack_set_sub in particular sits in the
 * appended run instead of at its seam). The object's function layout is source order, so the file has to
 * be sorted into address order before this unit can link, even at 100 %.
 * 100 %.
 *
 * Load-bearing source shapes (do not "simplify"):
 *   * an `s16`/`s8` *parameter* (not `s32` + a cast) is what keeps the raw register live for a store while
 *     the comparison still sign-extends at its use (79154, 79194, C89C); with a compound assignment it is
 *     also what makes the store raw (A044, 76CE8, CA48, 76E08, 78674, D5A4).
 *   * a `field += value` / `field -= (s16)value` compound assignment stores raw and sign-extends only at the
 *     compare that follows; `field = field + value` and `s16 t = field + value; field = t;` extend at the
 *     store and cost a row (78674, C8B4, D5A4).
 *   * a `u16` id equality needs a signed local (`s32 id = self->unk00C;`) to pair as `cmpwi` rather than
 *     `cmplwi` (A198, C030, 78C7C, 78CD0, A4EC, BCE0); the range tests stay `(u32)(id - lo) <= n`.
 *   * counted loops unroll with `mtctr`: `i < 10` -> by 1, `i < 24` -> by 4 (BCE0), `i < 32` -> by 8 with
 *     `mtctr 4` (D40C).
 *   * one-off field offsets are written as `*(s16*)((u8*)self + N)` (789EC, AF88); the struct only names
 *     fields more than one function touches. The 24-entry `{u16 id; s16 count}` table at 0x278 is named
 *     (`slot_table`, with a `_SLOTENT` entry type) because two functions index it and the field form is what
 *     gives retail's displacement loads; `_SLOTENT` is a copy of `Pl/pl_skill.cpp`'s and the two belong in one
 *     Pl-wide header (see the `_PLW` sharing note in the campaign's `config_requests`).
 *   * the exported helpers are C++ functions so the compiler emits the map's mangled names (Pl_bari_ck,
 *     Pl_condition_ck, Pl_dm_condition_ck, Pl_suimen_ck, Pl_get_gunner_pos/vec, Pl_zanzo_set,
 *     Get_Shell_rate_adj); every `fn_*` callee and the SDK entry points are `extern "C"`.
 *   * MWCC emits `b <callee>` plus a dead `blr` for a void tail call (BE2C).
 */

#include "types.h"
#include "enemy/enemy_control.h"
#include "ef/fn_800CDB2C.h"   /* my_player_no (rule 2: the owner is `ef/fn_800CDB2C.cpp`) */
#include "mh3_pad.h" /* VEC3_ctor / copyVec3 / setVec3 (rule 2) */
#include "fn_8004CAD8/mtx.h" /* the symbols deleted above (rule 2) */

/* The actor the whole Pl_* family takes as its first argument. Only the offsets this unit touches are named;
 * everything in between is padding. */
/* The gunner helpers take the engine's vector types; the names are what the map's mangling encodes. */
struct _CP_VECTOR {
    u32 x;
    u32 y;
    u32 z;
};

/* `nw4r::math::VEC3` / `MTX34` come from `nw4r/math.h`, reached through `enemy_control.h`
 * (rule 1: the layout is defined once).  This file used to carry a second copy. */

extern u8 lbl_806AB848[];

/* One 4-byte entry of the actor's 24-slot item table at 0x278: an item id and a signed value.
 * Pl/pl_skill.cpp carries the same type as `_SLOTENT`; the two belong in one Pl-wide header. */
struct _SLOTENT {
    /* 0x0 */ u16 id;
    /* 0x2 */ s16 value;
};

/* The player actor. size: 0x668 - the extent of the field layout below (the pl_skill.cpp copy of this
 * type stops at 0x644, so that one is short; the two belong in one Pl-wide header, see the header note). */
struct _PLW {
    /* 0x000 */ u8 unk000[2];
    /* 0x002 */ u8 unk002;
    /* 0x003 */ u8 unk003[0x08 - 0x03];
    /* 0x008 */ u8 unk008;
    /* 0x009 */ u8 unk009;
    /* 0x00A */ u8 unk00A;
    /* 0x00B */ u8 unk00B;
    /* 0x00C */ u16 unk00C;
    /* 0x00E */ u8 unk00E[0x15 - 0x0E];
    /* 0x015 */ u8 unk015;
    /* 0x016 */ u8 unk016;
    /* 0x017 */ u8 unk017[0x18 - 0x17];
    /* 0x018 */ u8 unk018;
    /* 0x019 */ u8 unk019[0x20 - 0x19];
    /* 0x020 */ u32 unk020;
    /* 0x024 */ u8 unk024[0x3C - 0x24];
    /* 0x03C */ f32 unk03C;
    /* 0x040 */ f32 unk040;
    /* 0x044 */ f32 unk044;
    /* 0x048 */ u8 unk048[0x54 - 0x48];
    /* 0x054 */ u32 unk054;
    /* 0x058 */ u32 unk058;
    /* 0x05C */ u8 unk05C[0x64 - 0x5C];
    /* 0x064 */ f32 unk064;
    /* 0x068 */ u8 unk068[0x74 - 0x68];
    /* 0x074 */ u8 unk074;
    /* 0x075 */ u8 unk075[0x90 - 0x75];
    /* 0x090 */ f32 unk090[3];
    /* 0x09C */ f32 unk09C;
    /* 0x0A0 */ f32 unk0A0;
    /* 0x0A4 */ f32 unk0A4;
    /* 0x0A8 */ u8 unk0A8[0xB4 - 0xA8];
    /* 0x0B4 */ s16 unk0B4;
    /* 0x0B6 */ u8 unk0B6[0x13C - 0xB6];
    /* 0x13C */ u8* unk13C;
    /* 0x140 */ u8 unused_0x140[0x1D0 - 0x140];
    /* 0x1D0 */ u8 equipB[0xC];
    /* 0x1DC */ u8 unused_0x1DC[0x1E8 - 0x1DC];
    /* 0x1E8 */ u8 equipC[0xC];
    /* 0x1F4 */ u8 equipD[0xC];
    /* 0x200 */ u8 unused_0x200[0x264 - 0x200];
    /* 0x264 */ s16 unk264;
    /* 0x266 */ u8 unk266[0x269 - 0x266];
    /* 0x269 */ u8 unk269;
    /* 0x26A */ u8 unk26A;
    /* 0x26B */ u8 unk26B;
    /* 0x26C */ u8 unk26C;
    /* 0x26D */ u8 unk26D;
    /* 0x26E */ u16 unk26E;
    /* 0x270 */ s16 unk270;
    /* 0x272 */ u8 unk272[0x276 - 0x272];
    /* 0x276 */ u8 unk276;
    /* 0x277 */ u8 unk277[0x278 - 0x277];
    /* 0x278 */ _SLOTENT slot_table[24];
    /* 0x2D8 */ u8 unk2D8[0x306 - 0x2D8];
    /* 0x306 */ u16 unk306;
    /* 0x308 */ u8 unk308[0x30C - 0x308];
    /* 0x30C */ u8 unk30C;
    /* 0x30D */ u8 unk30D;
    /* 0x30E */ u8 unk30E;
    /* 0x30F */ u8 unk30F[0x313 - 0x30F];
    /* 0x313 */ s8 unk313;
    /* 0x314 */ u8 unk314;
    /* 0x315 */ u8 unk315[0x318 - 0x315];
    /* 0x318 */ u32 unk318;
    /* 0x31C */ s16 unk31C;
    /* 0x31E */ s16 unk31E;
    /* 0x320 */ s16 unk320;
    /* 0x322 */ u8 unk322[16];
    /* 0x332 */ u8 unk332[0x354 - 0x332];
    /* 0x354 */ f32 unk354;
    /* 0x358 */ f32 unk358;
    /* 0x35C */ u8 unk35C[0x364 - 0x35C];
    /* 0x364 */ u32 unk364;
    /* 0x368 */ u8 unk368;
    /* 0x369 */ u8 unk369;
    /* 0x36A */ u8 unk36A;
    /* 0x36B */ u8 unk36B;
    /* 0x36C */ s16 unk36C;
    /* 0x36E */ u8 unk36E[0x370 - 0x36E];
    /* 0x370 */ s16 unk370;
    /* 0x372 */ u8 unk372[0x376 - 0x372];
    /* 0x376 */ s16 unk376;
    /* 0x378 */ s16 unk378;
    /* 0x37A */ s16 unk37A;
    /* 0x37C */ s16 unk37C;
    /* 0x37E */ u8 unk37E[0x384 - 0x37E];
    /* 0x384 */ s16 unk384;
    /* 0x386 */ s16 unk386;
    /* 0x388 */ u8 unk388;
    /* 0x389 */ u8 unk389[0x38A - 0x389];
    /* 0x38A */ s16 unk38A;
    /* 0x38C */ s16 unk38C;
    /* 0x38E */ s16 unk38E;
    /* 0x390 */ s16 unk390;
    /* 0x392 */ s16 unk392;
    /* 0x394 */ s16 unk394;
    /* 0x396 */ s16 unk396;
    /* 0x398 */ u8 unk398[0x39E - 0x398];
    /* 0x39E */ u8 unk39E;
    /* 0x39F */ u8 unk39F[0x3A2 - 0x39F];
    /* 0x3A2 */ s8 unk3A2;
    /* 0x3A3 */ s8 unk3A3;
    /* 0x3A4 */ u8 unk3A4[0x3AC - 0x3A4];
    /* 0x3AC */ u32 unk3AC;
    /* 0x3B0 */ u8 unk3B0[0x3B4 - 0x3B0];
    /* 0x3B4 */ u8 unk3B4;
    /* 0x3B5 */ u8 unk3B5;
    /* 0x3B6 */ u8 unk3B6[0x3D8 - 0x3B6];
    /* 0x3D8 */ u32 unk3D8;
    /* 0x3DC */ u32 unk3DC;
    /* 0x3E0 */ u32 unk3E0;
    /* 0x3E4 */ u8 unk3E4[0x3EC - 0x3E4];
    /* 0x3EC */ s16 unk3EC;
    /* 0x3EE */ u8 unk3EE[0x3F2 - 0x3EE];
    /* 0x3F2 */ s16 unk3F2;
    /* 0x3F4 */ u8 unk3F4[0x3F8 - 0x3F4];
    /* 0x3F8 */ s16 unk3F8;
    /* 0x3FA */ u8 unk3FA[0x3FE - 0x3FA];
    /* 0x3FE */ s16 unk3FE;
    /* 0x400 */ u8 unk400[0x404 - 0x400];
    /* 0x404 */ s16 unk404;
    /* 0x406 */ u8 unk406[0x40E - 0x406];
    /* 0x40E */ s16 unk40E;
    /* 0x410 */ u8 unk410[0x414 - 0x410];
    /* 0x414 */ s16 unk414;
    /* 0x416 */ s16 unk416;
    /* 0x418 */ u8 unk418[0x41A - 0x418];
    /* 0x41A */ s16 unk41A;
    /* 0x41C */ s16 unk41C;
    /* 0x41E */ u8 unk41E[0x420 - 0x41E];
    /* 0x420 */ s16 unk420;
    /* 0x422 */ s16 unk422;
    /* 0x424 */ s16 unk424;
    /* 0x426 */ s16 unk426;
    /* 0x428 */ s16 unk428;
    /* 0x42A */ s16 unk42A;
    /* 0x42C */ s16 unk42C;
    /* 0x42E */ u8 unk42E[0x448 - 0x42E];
    /* 0x448 */ s8 unk448;
    /* 0x449 */ s8 unk449;
    /* 0x44A */ u8 unk44A[0x44C - 0x44A];
    /* 0x44C */ s8 unk44C;
    /* 0x44D */ s8 unk44D;
    /* 0x44E */ u8 unk44E[0x45A - 0x44E];
    /* 0x45A */ s16 unk45A;
    /* 0x45C */ u8 unk45C[0x466 - 0x45C];
    /* 0x466 */ s16 unk466;
    /* 0x468 */ s16 unk468;
    /* 0x46A */ s16 unk46A;
    /* 0x46C */ u8 unk46C;
    /* 0x46D */ u8 unk46D;
    /* 0x46E */ u8 unk46E;
    /* 0x46F */ u8 unk46F;
    /* 0x470 */ s16 unk470;
    /* 0x472 */ u8 unk472[0x489 - 0x472];
    /* 0x489 */ u8 unk489;
    /* 0x48A */ u8 unk48A[0x492 - 0x48A];
    /* 0x492 */ u8 unk492;
    /* 0x493 */ u8 unk493[0x4DC - 0x493];
    /* 0x4DC */ u8 unk4DC;
    /* 0x4DD */ u8 unk4DD[0x4E5 - 0x4DD];
    /* 0x4E5 */ u8 unk4E5;
    /* 0x4E6 */ u8 unk4E6[0x4EE - 0x4E6];
    /* 0x4EE */ u8 unk4EE;
    /* 0x4EF */ u8 unk4EF[0x538 - 0x4EF];
    /* 0x538 */ u8 unk538;
    /* 0x539 */ u8 unk539[0x580 - 0x539];
    /* 0x580 */ s16 unk580;
    /* 0x582 */ u8 unk582;
    /* 0x583 */ s8 unk583;
    /* 0x584 */ u8 unk584[0x59C - 0x584];
    /* 0x59C */ u16 unk59C;
    /* 0x59E */ u16 unk59E;
    /* 0x5A0 */ u16 unk5A0;
    /* 0x5A2 */ u8 unk5A2[0x5A4 - 0x5A2];
    /* 0x5A4 */ u16 unk5A4;
    /* 0x5A6 */ u8 unk5A6;
    /* 0x5A7 */ u8 unk5A7[0x5BB - 0x5A7];
    /* 0x5BB */ u8 unk5BB;
    /* 0x5BC */ u8 unk5BC[0x5C4 - 0x5BC];
    /* 0x5C4 */ u8 unk5C4;
    /* 0x5C5 */ u8 unk5C5[0x5E5 - 0x5C5];
    /* 0x5E5 */ u8 unk5E5;
    /* 0x5E6 */ u8 unk5E6;
    /* 0x5E7 */ u8 unk5E7;
    /* 0x5E8 */ s16 unk5E8;
    /* 0x5EA */ u8 unk5EA[0x64F - 0x5EA];
    /* 0x64F */ s8 unk64F;
    /* 0x650 */ u8 unk650[0x65E - 0x650];
    /* 0x65E */ u8 unk65E;
    /* 0x65F */ u8 unk65F[0x662 - 0x65F];
    /* 0x662 */ s16 unk662;
    /* 0x664 */ s16 unk664;
    /* 0x666 */ u16 unk666;
};

/* The actors this unit calls into; the mangling of the source names reproduces the map's spellings. */
s32 Pl_master_ck(_PLW*);
u32 Pl_Skill_ck(_PLW*, u16);
u32 Pl_cat_skill_ck(_PLW*, u16);

extern "C" u32 fn_8027681C(_PLW*);
extern "C" void fn_80276868(_PLW*, s16);
extern "C" void fn_80276CE8(_PLW*, s16);
extern "C" u8 fn_802B0598(u8);
extern "C" s32 fn_80331104(void);
extern "C" void fn_8010D688(_PLW*);
extern "C" u32 fn_8026FE44(_PLW*);
extern "C" void fn_800E1640(u8*, f32);
extern "C" u32 fn_802B0688(void*);
extern "C" void fn_8026FEF0(_PLW*, s32);
extern "C" void fn_80275AC4(_PLW*, s32, u16, u16);
u8* get_move_work_adrs(u8);

/* In-unit callees that the appended bodies reference before their own definition. */
extern "C" void fn_802789EC(_PLW*, s32);
extern "C" void fn_8027A17C(_PLW*);
extern "C" u32 fn_80278144(u8, u8*, u8);
extern "C" u32 fn_80278310(u8, u8*, u8);
extern "C" void fn_80278B58(_PLW*, s32, s32);

/* condition-bit tests over the actor's flag words, one per flag set. */
u32 Pl_condition_ck(_PLW*, u32);
u32 Pl_dm_condition_ck(_PLW*, u32);

s16 Get_motion_no(_PLW*);

u32 Pl_act_ck(_PLW*, u8, u16);
u32 Pl_bari_ck(_PLW*, s32);
void Pl_get_gunner_vec(_PLW*, _CP_VECTOR*);
void Pl_get_gunner_pos(_PLW*, nw4r::math::VEC3*, s32);
void cpSetRotMatrixZXY(_CP_VECTOR*, nw4r::math::MTX34*);
void rotVecXYZ(nw4r::math::VEC3*, _CP_VECTOR*);

extern "C" void fn_800FC0D4(_CP_VECTOR*, void*);
extern "C" f32 fn_80050EF4(void*, void*);
extern "C" u32 fn_80114C20(_PLW*, s32);
extern "C" u32 fn_802B0668(u8);
extern "C" u32 fn_802753E4(_PLW*, s32);
extern "C" void fn_80278D1C(_PLW*);
u8 get_now_mapno(void);
u32 Pl_frame_check(_PLW*, u32, f32, f32);
f32 GetGroundHit2(nw4r::math::VEC3*, u32, u8, u8*);
void rotVecY(nw4r::math::VEC3*, u32);

extern "C" s32 fn_802E5CFC(s32);
extern "C" u32 fn_8042CB9C(s32);
/* 0x80338E04 `lobby/lb_companion_ui.cpp` (the companion/status UI band).  The owner's header cannot be
 * included from this unit - it declares `Pl_cat_skill_ck` returning `void` against this file's `u32`
 * (measured: `(10505) illegal overloading 'Pl_cat_skill_ck(_PLW *, unsigned short)'`), and
 * `include/unsplit/*.h` may not carry a symbol a registered unit owns (rule 2) - so the declaration is
 * this unit's own view in a linkage block, the shape `enemy/fn_80137604.cpp` uses for the same case.
 * The three-argument signature is this unit's call site; the callee reads r3/r4/r5. */
extern "C" {
void lb_entry_handover_send(s32, u8, u8);
}
extern "C" void fn_80272E30(_PLW*, u16, s16);
extern "C" void fn_802E5D68(u16);

extern "C" s32 fn_8026A644(_PLW*, s32);
extern "C" u8 fn_802748C8(_PLW*);
u32 GetItemData(u16);

extern "C" u32 fn_8027BCE0(_PLW*);
extern "C" s32 fn_80272C80(_PLW*, u8);
extern "C" u8 fn_80274B20(u16);
extern "C" s16 fn_80272CC8(_PLW*, u8);
extern "C" u8 fn_80274D98(_PLW*, u8);
extern "C" u32 fn_8028732C(_PLW*);
extern "C" u8* fn_8027ED18(void*);
extern "C" u8* fn_80279360(u8*, u8);

extern const f32 lbl_8079A088;
extern const f32 lbl_8079A08C;
extern const f32 lbl_8079A090;
extern const f32 lbl_8079A094;
extern const f32 lbl_8079A098;
extern const f32 lbl_8079A09C;
extern const f64 lbl_8079A0A0;
extern const f32 lbl_8079A0A8;
extern const f32 lbl_8079A0AC;
extern const f32 lbl_8079A0B0;
extern const f32 lbl_8079A0B4;
extern const f32 lbl_8079A084;
extern const f32 lbl_8079A0C8;
extern const f32 lbl_8079A0CC;
extern const f32 lbl_8079A0D0;
extern const f32 lbl_8079A0DC;
extern const f32 lbl_8079A0E0;
extern const f32 lbl_8079A0E4;
extern const f32 lbl_8079A0E8;
extern const f32 lbl_8079A0FC;
extern const f32 lbl_8079A0F4;
extern const f32 lbl_8079A100;
extern const f32 lbl_8079A104;
extern const f32 lbl_8079A108;
extern const f32 lbl_8079A10C;
extern const f32 lbl_8079A0C4;
extern const f32 lbl_8079A0EC;
extern const f32 lbl_8079A110;
extern u8* lbl_805BFFA8[];

extern u32 lbl_805BF448[];
extern u32 lbl_805BF46C[];
extern u32 lbl_805E2248[];
extern u32 lbl_805E25D0[];
extern u8 lbl_805BF5E0[];
extern u8 lbl_805BF538[];

/* 0x80276B58: the attack-scale multiplier the actor's active skills grant for a signed modifier. */
extern "C" void fn_80276B58(_PLW* self, s32 arg1)
{
    if (Pl_master_ck(self) != 0) {
        f32 f = (f32)(s16)arg1;
        if ((s16)arg1 < 0) {
            if (Pl_Skill_ck(self, 0xB9) == 1) {
                if (Pl_cat_skill_ck(self, 4) == 1) {
                    f *= lbl_8079A088;
                } else {
                    f *= lbl_8079A08C;
                }
            } else if (Pl_Skill_ck(self, 0xBA) == 1) {
                f *= lbl_8079A088;
            } else if (Pl_Skill_ck(self, 0xBB) == 1) {
                if (Pl_cat_skill_ck(self, 4) == 1) {
                    f *= lbl_8079A090;
                } else {
                    f *= lbl_8079A094;
                }
            } else if (Pl_Skill_ck(self, 0xBC) == 1) {
                if (Pl_cat_skill_ck(self, 4) == 1) {
                    f *= lbl_8079A098;
                } else {
                    f *= lbl_8079A09C;
                }
            } else if (Pl_cat_skill_ck(self, 4) == 1) {
                f *= lbl_8079A08C;
            }
        }
        fn_80276868(self, (s16)f);
    }
}

/* 0x80276CE8: adds a signed amount to the actor's stamina pool and clamps it, with a lower bound reset. */
extern "C" void fn_80276CE8(_PLW* self, s16 arg1)
{
    if (Pl_master_ck(self) == 0) {
        return;
    }
    if (arg1 < 0 && fn_8027681C(self) == 1) {
        return;
    }
    {
        self->unk37A += arg1;
        if (self->unk37A <= 0x96) {
            self->unk37A = 0x96;
        } else if (self->unk37A > 0x384) {
            self->unk37A = 0x384;
            self->unk37C = 0x2A30;
        }
        s16 v = self->unk37A;
        if (self->unk378 > v) {
            self->unk378 = v;
        }
    }
}

/* 0x80276D94: applies the stamina bonus the two armour skills grant, then the signed amount. */
extern "C" void fn_80276D94(_PLW* self, s32 arg1)
{
    if ((s16)arg1 > 0 && (Pl_Skill_ck(self, 0x48) == 1 || Pl_Skill_ck(self, 0x49) == 1)) {
        arg1 += 150;
    }
    fn_80276CE8(self, (s16)arg1);
}

/* 0x80276E08: recomputes the actor's attack-range/level modifier from its weapon class and skills. */
extern "C" void fn_80276E08(_PLW* self)
{
    if (Pl_master_ck(self) != 0) {
        s16 cls = fn_802B0598(self->unk016);
        s32 v = 0;
        if (cls == 2 || cls == 4) {
            if (self->unk466 == 0 && (cls != 2 || (Pl_Skill_ck(self, 0x80) != 1 && Pl_Skill_ck(self, 0x81) != 1))
                && (cls != 4 || Pl_Skill_ck(self, 0x81) != 1)) {
                if (cls == 2) {
                    if (Pl_Skill_ck(self, 0x82) == 1) {
                        v = 3;
                    } else if (Pl_Skill_ck(self, 0x83) == 1) {
                        v = 4;
                    } else {
                        v = 2;
                    }
                } else if (Pl_Skill_ck(self, 0x80) == 1) {
                    v = 1;
                } else if (Pl_Skill_ck(self, 0x82) == 1) {
                    v = 4;
                } else if (Pl_Skill_ck(self, 0x83) == 1) {
                    v = 6;
                } else {
                    v = 3;
                }
            }
        } else {
            v = 0;
        }
        if (Pl_Skill_ck(self, 0x45) == 0) {
            if (self->unk009 == 3) {
                if ((self->unk020 & 1) == 0 && (Pl_Skill_ck(self, 0x44) != 1 || (self->unk020 & 3) != 0)) {
                    v += 1;
                    if (Pl_Skill_ck(self, 0x47) == 1) {
                        v += 1;
                    } else if (Pl_Skill_ck(self, 0x46) == 1 && (self->unk020 & 3) == 0) {
                        v += 1;
                    }
                }
            } else if (Pl_Skill_ck(self, 0x44) != 1 || (self->unk020 & 1) != 0) {
                v += 1;
                if (Pl_Skill_ck(self, 0x47) == 1) {
                    v += 1;
                } else if (Pl_Skill_ck(self, 0x46) == 1 && (self->unk020 & 1) == 0) {
                    v = (s16)(v + 1);
                }
            }
        }
        if ((s16)v > 0) {
            self->unk37C -= (s16)v;
            if ((s16)self->unk37C <= 0) {
                if (fn_8027681C(self) == 1) {
                    self->unk37C = 1;
                    return;
                }
                self->unk37C = 0x2A30;
                fn_80276CE8(self, -0x96);
            }
        }
    }
}

/* 0x802770E0 */
extern "C" s32 fn_802770E0(void)
{
    return 0;
}

/* 0x802770E8: initialises the actor's attack-range state - the id/count words, the zeroed tail fields and
 * the 16-byte per-range table. */
extern "C" void fn_802770E8(_PLW* self, u32 table, s32 arg2)
{
    self->unk318 = table;
    self->unk313 = (s8)arg2;
    self->unk314 = 0;
    self->unk31C = 0;
    self->unk320 = 0;
    self->unk31E = 0;
    for (s16 i = 0; i < 48; i++) {
        self->unk322[i] = 0;
    }
}

/* 0x802771A0: picks the attack-range table for the actor's weapon class and initialises it. */
extern "C" void fn_802771A0(_PLW* self, s32 arg1)
{
    if (self->unk009 != 3) {
        if (self->unk002 == 8 && fn_80331104() == 0) {
            fn_802770E8(self, (u32)lbl_805E2248, (s16)arg1);
        } else {
            fn_802770E8(self, lbl_805BF448[self->unk002], (s16)arg1);
        }
    } else {
        if (self->unk002 == 8 && fn_80331104() == 0) {
            fn_802770E8(self, (u32)lbl_805E25D0, (s16)arg1);
        } else {
            fn_802770E8(self, lbl_805BF46C[self->unk002], (s16)arg1);
        }
    }
}

/* 0x80277B44 */
extern "C" void fn_80277B44(_PLW* self, s16 arg1)
{
    self->unk396 = arg1;
}

/* 0x80277B4C: scales the actor's attack-range modifier by the two armour skills. */
extern "C" void fn_80277B4C(_PLW* self, s16 arg1)
{
    self->unk396 = arg1;
    if (Pl_cat_skill_ck(self, 28) == 1) {
        self->unk396 = (s16)(self->unk396 * 3);
    } else if (Pl_cat_skill_ck(self, 29) == 1) {
        self->unk396 = (s16)(self->unk396 * 2);
    }
}

/* 0x80277C48 */
extern "C" void fn_80277C48(_PLW* self, s16 arg1)
{
    self->unk580 = arg1;
}

/* 0x80277C50 */
extern "C" void fn_80277C50(_PLW* self, s16 arg1)
{
    self->unk45A = arg1;
}

/* 0x80277FE0 */
extern "C" s32 fn_80277FE0(_PLW* self)
{
    return 1;
}

/* 0x80277FE8 */
extern "C" u32 fn_80277FE8(_PLW* self)
{
    return (u32)(self->unk30C - 1) >> 31;
}

/* 0x802784A8 */
extern "C" s32 fn_802784A8(_PLW* self)
{
    return (u32)(self->unk30D - 3) >> 31;
}

/* 0x80278564 */
extern "C" void fn_80278564(_PLW* self, u32 arg1)
{
    self->unk388 |= (u8)arg1;
}

/* 0x80278578 */
extern "C" s32 fn_80278578(_PLW* self, u32 arg1)
{
    return (self->unk388 & (u16)arg1) == 0;
}

/* 0x80276E08 sibling: whether the actor is inside the invulnerability window the suimen skill
 * grants (a +/- one-unit band around its current height). */
s32 Pl_suimen_ck(_PLW* self)
{
    if (self->unk074 != 0) {
        if (self->unk040 >= self->unk064 - lbl_8079A0CC && self->unk040 <= lbl_8079A0CC + self->unk064) {
            return 1;
        }
    }
    return 0;
}

/* 0x8027594C: whether any of the given condition bits is set in the actor's condition word. */
u32 Pl_condition_ck(_PLW* self, u32 mask)
{
    return (self->unk3D8 & mask) != 0;
}

/* 0x8027594C: whether any of the given bits is set in the actor's damage-condition word. */
u32 Pl_dm_condition_ck(_PLW* self, u32 mask)
{
    return (self->unk3DC & mask) != 0;
}

/* 0x802790E4 */
extern "C" u32 fn_802790E4(_PLW* self, u32 mask)
{
    return (self->unk3E0 & mask) != 0;
}

/* 0x80279154 */
extern "C" void fn_80279154(_PLW* self, s32 arg1, s8 arg2)
{
    if (arg1 == 0) {
        if (self->unk448 < arg2) {
            self->unk448 = arg2;
        }
    } else {
        if (self->unk449 < arg2) {
            self->unk449 = arg2;
        }
    }
}

/* 0x802798FC */
extern "C" s32 fn_802798FC(_PLW* self)
{
    if ((self->unk269 == 0 || (self->unk269 < self->unk26A && self->unk269 < self->unk270))
        && self->unk270 > 0) {
        return 1;
    }
    return 0;
}

/* 0x8027A000: adds a signed amount to the actor's charge counter and clamps it to +/-100. */
extern "C" void fn_8027A000(_PLW* self, s32 arg1)
{
    self->unk64F += (s8)arg1;
    if (self->unk64F >= 100) {
        self->unk64F = 100;
    }
    if (self->unk64F <= -100) {
        self->unk64F = -100;
    }
}

/* 0x8027A044: adds a signed amount to the actor's aim angle and clamps it to +/-90 degrees. */
extern "C" void fn_8027A044(_PLW* self, s16 arg1)
{
    self->unk5E8 += arg1;
    if (arg1 >= 0) {
        if (self->unk5E8 >= 8192) {
            self->unk5E8 = 8192;
        }
    } else {
        if (self->unk5E8 <= -8192) {
            self->unk5E8 = -8192;
        }
    }
}

/* 0x8027A17C */
extern "C" void fn_8027A17C(_PLW* self)
{
    self->unk5E6 = 0;
    self->unk5E5 = 0;
    self->unk5E8 = 0;
}

/* 0x8027A190 */
extern "C" s32 fn_8027A190(_PLW* self)
{
    return 1;
}

/* 0x8027A198 */
extern "C" s32 fn_8027A198(_PLW* self)
{
    if (self->unk00A == 0) {
        s32 id = self->unk00C;
        if ((u32)(id - 68) <= 1 || id == 71) {
            return 1;
        }
    }
    return 0;
}

/* 0x8027A554 */
extern "C" s32 fn_8027A554(_PLW* self)
{
    if (self->unk018 == 1 && self->unk5E6 != 0) {
        return 0;
    }
    return 1;
}

/* 0x8027AC00 */
extern "C" void fn_8027AC00(_PLW* self)
{
    self->unk5BB = 1;
}

/* 0x8027AC0C */
extern "C" void fn_8027AC0C(_PLW* self)
{
    self->unk5BB = 0;
}

/* 0x8027AF34: ticks the actor's residual-velocity timer and integrates its position deltas. */
extern "C" void fn_8027AF34(_PLW* self)
{
    s16 v = self->unk0B4;
    if (v > 0) {
        self->unk0B4--;
        /* unreachable - it makes the compiler reuse the first test's condition register */
        if (v < 0) {
            return;
        }
        self->unk03C += self->unk09C;
        self->unk040 += self->unk0A0;
        self->unk044 += self->unk0A4;
    }
}

/* 0x8027AF80 */
extern "C" s32 fn_8027AF80(u8 dir)
{
    return 1;
}

/* 0x8027C030 */
extern "C" s32 fn_8027C030(_PLW* self)
{
    if (self->unk00A == 7) {
        s32 id = self->unk00C;
        if ((u32)(id - 5) <= 1 || id == 2) {
            return 1;
        }
    }
    return 0;
}

/* 0x8027C89C: raises the actor's stored action id to the given one. */
extern "C" void fn_8027C89C(_PLW* self, s16 arg1)
{
    if (self->unk468 < arg1) {
        self->unk468 = arg1;
    }
}

/* 0x8027CA04 */
extern "C" s32 fn_8027CA04(_PLW* self, s32 arg1)
{
    if (self->unk002 != 7) {
        return 0;
    }
    if (self->unk468 > 0) {
        return 1;
    }
    return self->unk384 >= (s16)arg1;
}

/* 0x8027CC2C */
extern "C" u32 fn_8027CC2C(_PLW* self)
{
    return (self->unk5A4 & 0x40) != 0;
}

/* 0x8027CF24 */
extern "C" s32 fn_8027CF24(_PLW* self)
{
    if (fn_8026FE44(self) == 1 && self->unk276 != 0) {
        return 1;
    }
    return 0;
}

/* 0x8027CF70 */
extern "C" s32 fn_8027CF70(_PLW* self)
{
    return Pl_condition_ck(self, 0x20000);
}

/* 0x8027CFB4: starts a zanzo (afterimage) trail on the actor's current motion. */
void Pl_zanzo_set(_PLW* self, s32 arg1, u8 arg2)
{
    self->unk662 = (s16)arg1;
    self->unk65E = arg2;
    self->unk664 = Get_motion_no(self);
    self->unk666 = 0xFFFF;
}

/* 0x8027D3F0 */
extern "C" void fn_8027D3F0(_PLW* self, u8 arg1)
{
    self->unk3AC |= 1 << arg1;
}

/* 0x8027D4F0 */
extern "C" void fn_8027D4F0(_PLW* self)
{
    self->unk3B4 = 1;
}

/* 0x8027D4FC */
extern "C" s32 fn_8027D4FC(_PLW* self)
{
    return self->unk3B4 != 0;
}

/* 0x8027D510 */
extern "C" void fn_8027D510(_PLW* self)
{
    self->unk3B5 = 1;
}

/* 0x8027D51C */
extern "C" s32 fn_8027D51C(_PLW* self)
{
    return self->unk3B5 != 0;
}

/* 0x8027D584 */
extern "C" void fn_8027D584(_PLW* self, u8* arg1)
{
    self->unk59C = arg1[3] | 0x8000;
    self->unk59E = 0;
    self->unk5A0 = 0;
}

/* 0x80277C58: writes the actor's remaining vertical range from its motion frame data. */
extern "C" void fn_80277C58(_PLW* self)
{
    u8* p = self->unk13C;
    f32 v = *(f32*)(p + 72);
    if (v < lbl_8079A084) {
        v = lbl_8079A084;
    }
    self->unk264 = (s16)(*(f32*)(p + 120) - v);
}

/* 0x80277BC4: the attack-range modifier the actor's two armour skills set. */
extern "C" void fn_80277BC4(_PLW* self, u8 arg1)
{
    s32 v;
    if (arg1 == 0) {
        v = 6;
        if (Pl_Skill_ck(self, 0xA1) == 1) {
            v = 10;
        } else if (Pl_Skill_ck(self, 0xA2) == 1) {
            v = 12;
        }
    } else {
        v = 12;
    }
    fn_80277B44(self, v);
}

/* 0x80277EC0: hands the actor's vertical speed to its motion frame and caches the result. */
extern "C" void fn_80277EC0(_PLW* self)
{
    if (self->unk3A2 > 0) {
        fn_800E1640(self->unk13C + 4, lbl_8079A084);
    } else if (self->unk3A3 > 0) {
        fn_800E1640(self->unk13C + 4, lbl_8079A0C8 * self->unk354);
    } else {
        fn_800E1640(self->unk13C + 4, self->unk354);
    }
    self->unk358 = *(f32*)(self->unk13C + 0x60);
}

/* 0x80277F54: whether the actor's current motion is still free to be interrupted. */
extern "C" s32 fn_80277F54(_PLW* self)
{
    if (self->unk30E >= 2) {
        return 0;
    }
    s32 kind = self->unk015;
    if (kind != 9) {
        u8* p = get_move_work_adrs(0);
        if (p != 0 && self->unk016 == p[0xF6]) {
            return 0;
        }
    } else {
        if (fn_802B0688((u8*)self + 60) == 1) {
            return 0;
        }
    }
    return 1;
}

/* 0x802782B8 */
extern "C" s32 fn_802782B8(_PLW* self)
{
    if (fn_80277FE8(self) == 1 && fn_80278144(self->unk016, (u8*)self + 60, self->unk5A6) == 1) {
        return 1;
    }
    return 0;
}

/* 0x80278450 */
extern "C" s32 fn_80278450(_PLW* self)
{
    if (fn_80277FE8(self) == 1 && fn_80278310(self->unk016, (u8*)self + 60, self->unk5A6) == 1) {
        return 1;
    }
    return 0;
}

/* 0x80278994: clears the actor's stored per-motion scratch values. */
extern "C" void fn_80278994(_PLW* self)
{
    self->unk39E = 0;
    self->unk38A = 0;
    self->unk3EC = 0;
    self->unk3F8 = 0;
    self->unk3FE = 0;
    self->unk3F2 = 0;
    self->unk40E = 0;
    self->unk414 = 0;
    self->unk41A = 0;
    self->unk420 = 0;
    self->unk38C = 0;
    self->unk424 = 0;
    self->unk38E = 0;
    self->unk426 = 0;
    self->unk390 = 0;
    self->unk428 = 0;
    self->unk392 = 0;
    self->unk42A = 0;
    self->unk394 = 0;
    self->unk42C = 0;
}

/* 0x80278B58: starts the actor's dodge/step motion. */
extern "C" void fn_80278B58(_PLW* self, s32 arg1, s32 arg2)
{
    fn_8026FEF0(self, 4);
    fn_8026FEF0(self, 16);
    self->unk370 = 0;
    self->unk376 = 0;
    fn_802789EC(self, 1);
    fn_8027A17C(self);
    fn_80275AC4(self, 8, (u16)arg1, (u16)(arg2 | 32));
}

/* 0x80278BE4 */
extern "C" void fn_80278BE4(_PLW* self)
{
    if (self->unk00A == 8) {
        return;
    }
    fn_8026FEF0(self, 4);
    fn_8026FEF0(self, 16);
    self->unk370 = 0;
    self->unk376 = 0;
    fn_802789EC(self, 1);
    fn_8027A17C(self);
    if (self->unk009 == 3) {
        fn_80278B58(self, 1, 0);
    } else {
        fn_80278B58(self, 0, 0);
    }
}

/* 0x80278C7C */
extern "C" u32 fn_80278C7C(_PLW* self)
{
    if (self->unk416 > 0) {
        return 1;
    }
    if (self->unk00A == 6) {
        s32 id = self->unk00C;
        if ((u32)(id - 46) <= 5 || (u32)(id - 43) <= 1 || id == 73) {
            return 1;
        }
    }
    return 0;
}

/* 0x80278CD0 */
extern "C" u32 fn_80278CD0(_PLW* self)
{
    if (self->unk41C > 0) {
        return 1;
    }
    if (self->unk00A == 6) {
        s32 id = self->unk00C;
        if ((u32)(id - 66) <= 6 || (u32)(id - 63) <= 1) {
            return 1;
        }
    }
    return 0;
}

/* 0x802790FC */
extern "C" s32 fn_802790FC(_PLW* self)
{
    if (self->unk46C == 1 && self->unk46C != self->unk46D && self->unk46E == 0) {
        self->unk46E = 1;
        if (self->unk470 == 0) {
            self->unk470 = 1800;
            return 1;
        }
        return 0;
    }
    return 0;
}

/* 0x80279194 */
extern "C" s32 fn_80279194(_PLW* self, s32 arg1, s8 arg2)
{
    if (arg1 == 0) {
        self->unk422 = 0;
        if (self->unk44C < arg2) {
            self->unk44C = arg2;
        }
    } else {
        if (self->unk422 > 0) {
            self->unk422 = 0;
            return 0;
        }
        if (self->unk44D < arg2) {
            self->unk44D = arg2;
        }
    }
    return 1;
}

/* 0x8027A08C: applies a charge delta to the actor's aim and converts the counter to an angle. */
extern "C" void fn_8027A08C(_PLW* self, s8 arg1)
{
    f32 v;
    self->unk583 += arg1;
    if (self->unk583 > 100) {
        self->unk583 = 100;
    }
    if (self->unk583 < -100) {
        self->unk583 = -100;
    }
    v = (f32)self->unk583 / lbl_8079A0D0;
    if (self->unk583 >= 0) {
        v = lbl_8079A0E0 * v * lbl_8079A0DC / lbl_8079A0E4 + lbl_8079A088;
        self->unk5E8 = (s16)(u16)v;
    } else {
        v = lbl_8079A0E8 * v * lbl_8079A0DC / lbl_8079A0E4 + lbl_8079A088;
        self->unk5E8 = (s16)(u16)v;
    }
}

/* 0x8027BE2C */
extern "C" void fn_8027BE2C(_PLW* self)
{
    if ((self->unk5C4 & 0xF) != 0) {
        self->unk5C4 &= 0xF0;
        fn_8010D688(self);
    }
}

/* 0x8027AC18 */
extern "C" s32 fn_8027AC18(_PLW* self)
{
    return self->unk5BB != 0;
}

/* 0x8027BDD4 */
extern "C" s32 fn_8027BDD4(_PLW* self)
{
    if (Pl_Skill_ck(self, 93) == 1) {
        return 1;
    }
    return (u16)fn_8027BCE0(self) == 395;
}

/* 0x8027CBC8: the highest carve-slot value the actor has available. */
extern "C" u8 fn_8027CBC8(_PLW* self)
{
    u8 v = 0;
    if (self->unk489 != 0 && self->unk492 != 255) {
        if (v < self->unk4DC) {
            v = self->unk4DC;
        }
    }
    if (self->unk4E5 != 0 && self->unk4EE != 255) {
        if (v < self->unk538) {
            v = self->unk538;
        }
    }
    return v;
}

/* 0x8027D530 */
extern "C" s32 fn_8027D530(_PLW* self)
{
    if (Pl_act_ck(self, 0, 64) == 1 && self->unk306 == 206) {
        return 1;
    }
    return 0;
}

/* 0x8027A4EC */
extern "C" s32 fn_8027A4EC(_PLW* self)
{
    s32 v = 0;
    if (self->unk00A == 4) {
        s32 id = self->unk00C;
        if ((u32)(id - 19) <= 30 || (u32)(id - 5) <= 8) {
            v = 1;
        }
    }
    if (fn_8028732C(self) == 1) {
        v = 1;
    }
    return v;
}

/* 0x80279414: looks the given id up in the actor's two reaction tables. */
extern "C" u8* fn_80279414(_PLW* self, u8 arg1)
{
    u8* p = fn_80279360(fn_8027ED18((u8*)self + 464), arg1);
    if (p != 0) {
        return p;
    }
    u8 n = self->equipC[0];
    if (n == 12) {
        u8* q = fn_80279360(fn_8027ED18(self->equipC), arg1);
        if (q != 0) {
            return q;
        }
    }
    return 0;
}

/* 0x80279360: finds the reaction-table entry the given motion id maps to. */
extern "C" u8* fn_80279360(u8* self, u8 arg1)
{
    for (s32 i = 0; i < 4; i++) {
        u8 idx = self[0x18 + i];
        if (idx == 0) {
            break;
        }
        u8* p = lbl_805BF5E0 + idx * 4;
        if (p[0] == arg1) {
            return p;
        }
    }
    return 0;
}

/* 0x80279B84: refreshes the actor's held-item state from its item id. */
extern "C" void fn_80279B84(_PLW* self)
{
    if (fn_8026FE44(self) != 0 && self->unk26E != 255) {
        (void)GetItemData((u16)fn_80272C80(self, (u8)self->unk26E));
        self->unk26C = fn_80274B20((u16)fn_80272C80(self, (u8)self->unk26E));
        self->unk270 = fn_80272CC8(self, (u8)self->unk26E);
        self->unk26A = fn_80274D98(self, self->unk26C);
        self->unk269 = 0;
    }
}

/* 0x8027A2A0: whether the actor may still act - master/rage state, the bari timer, the stun flag
 * and the held-item lock all have to agree. */
extern "C" s32 fn_8027A2A0(_PLW* self, s32 arg1)
{
    s32 v = 1;
    if (Pl_master_ck(self) == 0) {
        v = 0;
    }
    if (Pl_bari_ck(self, 1) == 0) {
        v = 0;
    }
    if (self->unk5E6 != 0) {
        v = 0;
    }
    if (fn_8026A644(self, 55) == 0 && (u8)arg1 == 0) {
        v = 0;
    }
    return v;
}

/* 0x8027BC48: whether the current motion still takes directional input. */
extern "C" s32 fn_8027BC48(s32 arg1)
{
    u8* p = get_move_work_adrs(0);
    if (p == 0) {
        return 1;
    }
    s32 x = p[0xFA];
    /* `break` = "this motion still takes directional input"; the loop is the shared `return 1` the
     * retail source reached through a label (the other half, the shared `return 0`, is inlined). */
    do {
        if ((u32)(x - 6) <= 2) {
            break;
        }
        switch (x) {
        case 3:
            if (arg1 != 0) {
                return 0;
            }
            break;
        case 5:
            if (arg1 == 2) {
                return 0;
            }
            break;
        case 4:
            break;
        default:
            return 0;
        }
    } while (0);
    return 1;
}

/* 0x8027CFC0: ticks down the actor's stun timer and clears the stun condition at zero. */
extern "C" void fn_8027CFC0(_PLW* self)
{
    if (Pl_master_ck(self) != 0 && self->unk404 > 0) {
        if (Pl_dm_condition_ck(self, 2) == 1) {
            self->unk404 = (s16)(self->unk404 - 90);
        } else {
            self->unk404 = (s16)(self->unk404 - 120);
        }
        if (self->unk404 <= 0) {
            self->unk404 = 0;
            self->unk3DC &= 0xFFFFFFFC;
        }
    }
}

/* 0x8027D050: the actor's stored carve value, scanned out of the item table. */
extern "C" u8 fn_8027D050(_PLW* self)
{
    u8 v = 0;
    if (self->unk369 == 0) {
        return 0;
    }
    if ((self->unk364 & 0xE0000007) != 0) {
        for (s32 i = 0; i < 10; i++) {
            if (lbl_806AB848[i * 24 + self->unk008 * 264 + 20] != 0) {
                v = lbl_806AB848[self->unk008 * 264 + i * 24 + 20];
                break;
            }
        }
    }
    return v;
}

/* 0x8027D5A4: applies a signed charge delta to the actor's stored charge, scaled by the two
 * charge skills, and clamps it to 0..100. */
extern "C" void fn_8027D5A4(_PLW* self, s32 arg1)
{
    f32 f = (f32)(s16)arg1;
    if (f > lbl_8079A084) {
        if (Pl_Skill_ck(self, 191) == 1) {
            f *= lbl_8079A094;
        } else if (Pl_Skill_ck(self, 192) == 1) {
            f *= lbl_8079A0FC;
        }
    }
    self->unk36C += (s16)f;
    if (self->unk36C < 0) {
        self->unk36C = 0;
    } else if (self->unk36C > 100) {
        self->unk36C = 100;
    }
}

/* 0x8027CE94: the gunner's world-space origin, biased by the charge counter. */
void Pl_get_gunner_vec(_PLW* self, _CP_VECTOR* out)
{
    u32 x = self->unk054;
    f32 t = (f32)self->unk64F / lbl_8079A0D0;
    t = lbl_8079A110 * t * lbl_8079A0DC / lbl_8079A0E4 + lbl_8079A088;
    out->x = x + (u16)t;
    out->y = self->unk058;
    out->z = 0;
}

/* 0x8027CE74: whether the actor's current motion is one of the gunner charge motions. */
extern "C" s32 fn_8027CE74(_PLW* self)
{
    if (fn_8026FE44(self) == 1) {
    switch ((u16)Get_motion_no(self)) {
    case 1104:
    case 1114:
    case 1115:
    case 1130:
    case 1131:
    case 1132:
    case 1153:
    case 1163:
    case 1164:
    case 1180:
    case 1181:
    case 1182:
        return 1;
    }
    }
    return 0;
}

/* 0x8027CA48: applies a charge delta to the weapon's charge timer. */
extern "C" void fn_8027CA48(_PLW* self, s16 arg1)
{
    if (self->unk002 != 7) {
        return;
    }
    self->unk386 += arg1;
    if (arg1 >= 0) {
        self->unk46A = 1800;
        if (self->unk386 >= 100) {
            self->unk386 = 100;
        }
        switch ((u8)fn_802748C8(self)) {
        case 1:
            fn_8027C89C(self, 900);
            break;
        case 2:
            fn_8027C89C(self, 900);
            break;
        case 3:
            fn_8027C89C(self, 900);
            break;
        }
    } else {
        if (self->unk386 < 0) {
            self->unk386 = 0;
        }
    }
}

/* 0x8027CB1C: the carve slot to select, sentinel 255 meaning "none". */
extern "C" u8 fn_8027CB1C(_PLW* self)
{
    u8 v = 255;
    if (self->unk489 != 0 && self->unk492 != 255 && (v == 255 || v < self->unk492)) {
        v = self->unk492;
    }
    if (self->unk4E5 != 0 && self->unk4EE != 255 && (v == 255 || v < self->unk4EE)) {
        v = self->unk4EE;
    }
    if (Pl_Skill_ck(self, 24) == 1 && v == 0) {
        v = 2;
    }
    return v;
}

/* 0x8027D40C: the number of set bits in the actor's action-lock word. */
extern "C" s32 fn_8027D40C(_PLW* self)
{
    s32 n = 0;
    for (s32 i = 0; i < 32; i++) {
        if (self->unk3AC & (1 << i)) {
            n++;
        }
    }
    return n;
}

/* 0x8027CD10: builds the gunner's aim matrix from its rotation and gun position. */
extern "C" void fn_8027CD0C(_PLW* self, nw4r::math::MTX34* mtx)
{
    _CP_VECTOR pos;
    nw4r::math::VEC3 v;
    VEC3_ctor(&v);
    fn_800FC0D4(&pos, (u8*)self + 84);
    f32 t = (f32)self->unk64F / lbl_8079A0D0;
    t = lbl_8079A110 * t * lbl_8079A0DC / lbl_8079A0E4 + lbl_8079A088;
    pos.x = pos.x + (u16)t;
    cpSetRotMatrixZXY(&pos, mtx);
    Pl_get_gunner_pos(self, &v, 0);
    mtx->m[0][3] = v.x;
    mtx->m[1][3] = v.y;
    mtx->m[2][3] = v.z;
}

/* 0x8027CD18: the gunner's gun position in actor-local space, biased by the charge counter. */
void Pl_get_gunner_pos(_PLW* self, nw4r::math::VEC3* out, s32 arg2)
{
    if (self->unk009 == 3) {
        out->x = lbl_8079A100;
        out->y = lbl_8079A104;
        out->z = lbl_8079A108;
    } else {
        out->x = lbl_8079A100;
        out->y = lbl_8079A10C;
        out->z = lbl_8079A108;
    }
    if (arg2 != 0) {
        out->z = out->z + lbl_8079A0CC;
    }
    rotVecXYZ(out, (_CP_VECTOR*)((u8*)self + 84));
    out->x = out->x + self->unk03C;
    out->y = out->y + self->unk040;
    out->z = out->z + self->unk044;
}

/* 0x8027AE28: sets the actor's residual-velocity timer from a target position over the given
 * number of frames. */
extern "C" void fn_8027AE28(_PLW* self, s32 arg1)
{
    if (fn_80050EF4((u8*)self + 60, (u8*)self + 144) >= lbl_8079A0F4 || (s16)arg1 == 0) {
        self->unk0B4 = 0;
        self->unk03C = self->unk090[0];
        self->unk040 = self->unk090[1];
        self->unk044 = self->unk090[2];
        self->unk09C = lbl_8079A084;
        self->unk0A0 = lbl_8079A084;
        self->unk0A4 = lbl_8079A084;
    } else {
        self->unk0B4 = arg1;
        self->unk09C = (self->unk090[0] - self->unk03C) / (f32)(s16)arg1;
        self->unk0A0 = (self->unk090[1] - self->unk040) / (f32)(s16)arg1;
        self->unk0A4 = (self->unk090[2] - self->unk044) / (f32)(s16)arg1;
    }
}

/* 0x8027C8B4: applies a signed charge delta to the actor's attack-range counter, with the two
 * charge skills scaling a positive delta. */
extern "C" void fn_8027C8B4(_PLW* self, s32 arg1)
{
    if (self->unk002 != 7) {
        return;
    }
    if ((s16)arg1 < 0 && self->unk468 > 0) {
        return;
    }
    f32 f = (f32)(s16)arg1;
    if (f > lbl_8079A084) {
        if (Pl_Skill_ck(self, 191) == 1) {
            f *= lbl_8079A094;
        } else if (Pl_Skill_ck(self, 192) == 1) {
            f *= lbl_8079A0FC;
        }
    }
    self->unk384 += (s16)f;
    if (f >= lbl_8079A084) {
        if (self->unk384 >= 100) {
            self->unk384 = 100;
            if ((u32)Pl_master_ck(self) == 1 && self->unk468 == 0) {
                fn_80114C20(self, 1);
            }
            fn_8027C89C(self, 900);
        }
    } else {
        if (self->unk384 < 0) {
            self->unk384 = 0;
        }
    }
}

/* 0x8027A2A0 sibling: whether the actor's bari (rage) state covers the given action class. */
u32 Pl_bari_ck(_PLW* self, s32 arg1)
{
    s32 v = 0;
    if (self->unk00A == 0) {
        s32 id = self->unk00C;
        if ((u32)(id - 169) <= 1 || (u32)(id - 172) <= 1) {
            if (arg1 != 2) {
                v = 1;
            } else {
                s32 m = (u16)Get_motion_no(self);
                if ((m == 314 || m == 365)
                    && Pl_frame_check(self, 1, lbl_8079A0EC, lbl_8079A084) == 1) {
                    v = 1;
                }
            }
        } else if ((id == 171 || id == 174) && (u32)(arg1 - 1) <= 1) {
            v = 1;
        }
    }
    return v;
}

/* 0x802784B8 */
extern "C" s32 fn_802784B8(_PLW* self)
{
    s32 m = (u8)fn_802B0668((u8)get_now_mapno());
    switch (m) {
    case 6:
    case 17:
        if (self->unk016 == 2) {
            return 0;
        }
        /* falls through */
    default: {
        u8* p = get_move_work_adrs(0);
        if (p != 0 && self->unk016 == p[0xF6]) {
            return 0;
        }
        break;
    }
    case 9:
        if (fn_802B0688((u8*)self + 60) == 1) {
            return 0;
        }
        break;
    }
    return 1;
}

/* 0x80278590: the actor's attack-range tier for the current weapon class. */
extern "C" s32 fn_80278590(_PLW* self)
{
    if (fn_8026FE44(self) == 1) {
        return 0;
    }
    s16* t = (s16*)(lbl_805BFFA8[self->unk002]
                    + ((*((u8*)self + 0x56A) * 7) << 1));
    s16 v = *((s16*)self + 0x2B6);
    if (v <= t[0]) {
        return 0;
    }
    if (v <= t[1]) {
        return 1;
    }
    if (v <= t[2]) {
        return 2;
    }
    if (v <= t[3]) {
        return 3;
    }
    if (v <= t[4]) {
        return 4;
    }
    return v <= t[5] ? 5 : 6;
}

/* 0x802789EC: resets the actor's whole action state and re-applies the armour skill values. */
extern "C" void fn_802789EC(_PLW* self, s32 arg1)
{
    fn_80278994(self);
    *(u32*)((u8*)self + 988) = 0;
    *(s16*)((u8*)self + 1042) = 0;
    *(s16*)((u8*)self + 1002) = 0;
    *(s16*)((u8*)self + 1006) = 0;
    *(s16*)((u8*)self + 1024) = 0;
    *(s16*)((u8*)self + 1020) = 0;
    *(s16*)((u8*)self + 1008) = 0;
    *(s16*)((u8*)self + 1012) = 0;
    *(s16*)((u8*)self + 1014) = 0;
    *(s16*)((u8*)self + 1018) = 0;
    *(s16*)((u8*)self + 1026) = 0;
    *(s16*)((u8*)self + 900) = 0;
    *(s16*)((u8*)self + 902) = 0;
    *(s16*)((u8*)self + 1040) = 0;
    *(s16*)((u8*)self + 1048) = 0;
    *(s16*)((u8*)self + 1054) = 0;
    *(s16*)((u8*)self + 1046) = 0;
    *(s16*)((u8*)self + 1052) = 0;
    *(s16*)((u8*)self + 1118) = 0;
    *(s16*)((u8*)self + 1120) = 0;
    *(s16*)((u8*)self + 1122) = 0;
    *(s16*)((u8*)self + 1124) = 0;
    *(s16*)((u8*)self + 1126) = 0;
    *(u8*)((u8*)self + 1096) = 0;
    *(u8*)((u8*)self + 1100) = 0;
    *(u8*)((u8*)self + 1097) = 0;
    *(u8*)((u8*)self + 1101) = 0;
    *(s16*)((u8*)self + 1098) = 0;
    *(s16*)((u8*)self + 1102) = 0;
    *(s16*)((u8*)self + 1028) = 0;
    *(s16*)((u8*)self + 1030) = 0;
    *(s16*)((u8*)self + 1032) = 0;
    *(s16*)((u8*)self + 1034) = 0;
    *(s16*)((u8*)self + 1036) = 0;
    *(s16*)((u8*)self + 1108) = 0;
    *(u8*)((u8*)self + 1104) = 0;
    *(s16*)((u8*)self + 1112) = 0;
    *(u8*)((u8*)self + 1106) = 0;
    *(s16*)((u8*)self + 1128) = 0;
    *(s16*)((u8*)self + 1130) = 0;
    *(s16*)((u8*)self + 1070) = 0;
    *(s16*)((u8*)self + 1080) = 0;
    *(s16*)((u8*)self + 1072) = 0;
    *(s16*)((u8*)self + 1082) = 0;
    *(s16*)((u8*)self + 1074) = 0;
    *(s16*)((u8*)self + 1084) = 0;
    *(s16*)((u8*)self + 1076) = 0;
    *(s16*)((u8*)self + 1086) = 0;
    *(s16*)((u8*)self + 1078) = 0;
    *(s16*)((u8*)self + 1088) = 0;
    *(s16*)((u8*)self + 1058) = 0;
    *(s16*)((u8*)self + 1114) = 0;
    *(s16*)((u8*)self + 1116) = 0;
    if ((u32)Pl_master_ck(self) == 1 && (arg1 == 0 || Pl_cat_skill_ck(self, 39) == 1)) {
        self->unk448 = (s8)fn_802753E4(self, 4);
        self->unk44C = (s8)fn_802753E4(self, 5);
    }
    fn_80278D1C(self);
}

/* 0x8027BCE0: the highest of the actor's 24 stored item ids that is in the carve set. */
extern "C" u32 fn_8027BCE0(_PLW* self)
{
    s32 id;
    u16 v = 0xFFFF;
    for (s32 i = 0; i < 24; i++) {
        if (self->slot_table[i].value > 0) {
            id = self->slot_table[i].id;
            if ((u32)(id - 381) <= 1 || id == 139 || id == 395) {
                v = (u16)id;
            }
        }
    }
    return v;
}

/* 0x80275A80: the shell multiplier adjustment the two shell tables give. */
f32 Get_Shell_rate_adj(_PLW* self, u8 arg1)
{
    f32 v = (f32)*(s16*)(fn_8027ED18((u8*)self + 464) + 10);
    if (self->equipC[0] == 12) {
        v = v * (f32)*(s16*)(fn_8027ED18(self->equipC) + 10) / lbl_8079A0D0;
    }
    return v / lbl_8079A0D0;
}

/* 0x80277C94: ground-height probe along the actor's facing, returning whether the probe hit
 * inside the requested band. */
extern "C" s32 fn_80277C94(_PLW* self, nw4r::math::VEC3* out, f32 arg2, f32 arg3)
{
    nw4r::math::VEC3 v;
    u8 hit;
    VEC3_ctor(&v);
    out->x = lbl_8079A084;
    v.x = lbl_8079A084;
    v.y = arg2 + arg3;
    v.z = lbl_8079A0C4;
    rotVecY(&v, self->unk058);
    v.x = v.x + self->unk03C;
    v.y = v.y + self->unk040;
    v.z = v.z + self->unk044;
    f32 h = GetGroundHit2(&v, -0x1006, self->unk016, &hit);
    f32 c = self->unk040 + arg2;
    if (h >= c && h <= arg3 + c && hit != 0) {
        out->x = h;
        return 1;
    }
    return 0;
}

/* 0x8027AF88: refreshes the actor's shell/clutch state from its move work. */
extern "C" void fn_8027AF88(_PLW* self)
{
    *(s16*)((u8*)self + 1474) = 30;
    u8* p = get_move_work_adrs(0);
    if (p == 0) {
        return;
    }
    u8* q = *(u8**)(p + 220);
    if (q == 0) {
        return;
    }
    if (fn_8042CB9C(fn_802E5CFC(*(s8*)(q + 1505))) == 1) {
        *(s16*)((u8*)self + 1626) = 900;
        *((u8*)self + 1625) = 1;
        lb_entry_handover_send(1, (u8)my_player_no(), *(u8*)(q + 1505));
        return;
    }
    *(s16*)((u8*)self + 1626) = 0;
    *((u8*)self + 1625) = 0;
    fn_80272E30(self, *(u16*)(q + 1506 + *(s8*)(q + 1505) * 4),
                *(s16*)(q + 1508 + *(s8*)(q + 1505) * 4));
    *(u32*)(q + 1668 + (*(s8*)(q + 1505) >> 5) * 4) |= 1 << (*(s8*)(q + 1505) & 31);
    fn_802E5D68(*(u16*)(q + 1506 + *(s8*)(q + 1505) * 4));
    *(s16*)(q + 1506 + *(s8*)(q + 1505) * 4) = 0;
    *(s16*)(q + 1508 + *(s8*)(q + 1505) * 4) = 0;
}

/* 0x80278D1C: rebuilds the actor's two condition word sets from its stored state, its current action id
 * and the two related-player flags the small helpers report. */
extern "C" void fn_80278D1C(_PLW* self)
{
    s32 bits = 0;
    if (Pl_master_ck(self) != 0) {
        if (*(s16*)((u8*)self + 1002) != 0) {
            bits |= 1;
        }
        if (self->unk448 + self->unk449 > 0) {
            bits |= 0x10;
        }
        if (*(s16*)((u8*)self + 900) >= 100) {
            bits |= 0x80000000;
        }
        if (self->unk44C + self->unk44D > 0) {
            bits |= 0x40;
        }
        if (self->unk00A == 6) {
            s32 id = self->unk00C;
            if ((u32)(id - 0x1F) > 3) {
                if ((u32)(id - 0x1C) > 2) {
                    if (id != 0x1B && id != 0x23) {
                    } else {
                        bits |= 8;
                    }
                } else {
                    bits |= 2;
                }
            } else {
                bits |= 4;
            }
        }
        if (*(s16*)((u8*)self + 1124) > 0) {
            bits |= 0x400;
        }
        if (*(s16*)((u8*)self + 1126) > 0) {
            bits |= 0x800;
        }
        if (*(s16*)((u8*)self + 1118) > 0) {
            bits |= 0x4000;
        }
        if (fn_80278C7C(self) == 1) {
            bits |= 0x100;
        }
        if (fn_80278CD0(self) == 1) {
            bits |= 0x200;
        }
        if (*(s16*)((u8*)self + 1058) > 0) {
            bits |= 0x80;
        }
        if (*(s16*)((u8*)self + 1040) > 0) {
            bits |= 0x20000;
        }
        if (*(s16*)((u8*)self + 1112) > 0) {
            bits |= 0x10000;
        }
        if (*(u32*)((u8*)self + 944) != 0) {
            bits |= 0x40000;
        }
        if (*(s16*)((u8*)self + 1108) > 0) {
            bits |= 0x2000;
        }
        self->unk3D8 = bits;
        self->unk3DC &= 0xFFF003FF;
        if (*(s16*)((u8*)self + 1070) > 0 || (s16)fn_802753E4(self, 6) < 0) {
            self->unk3DC |= 0x400;
        }
        if (*(s16*)((u8*)self + 1072) > 0 || (s16)fn_802753E4(self, 7) < 0) {
            self->unk3DC |= 0x800;
        }
        if (*(s16*)((u8*)self + 1074) > 0 || (s16)fn_802753E4(self, 8) < 0) {
            self->unk3DC |= 0x1000;
        }
        if (*(s16*)((u8*)self + 1076) > 0 || (s16)fn_802753E4(self, 9) < 0 || fn_80278C7C(self) == 1) {
            self->unk3DC |= 0x2000;
        }
        if (*(s16*)((u8*)self + 1078) > 0 || (s16)fn_802753E4(self, 10) < 0) {
            self->unk3DC |= 0x4000;
        }
        if (*(s16*)((u8*)self + 1080) > 0 || (s16)fn_802753E4(self, 6) > 0) {
            self->unk3DC |= 0x8000;
        }
        if (*(s16*)((u8*)self + 1082) > 0 || (s16)fn_802753E4(self, 7) > 0) {
            self->unk3DC |= 0x10000;
        }
        if (*(s16*)((u8*)self + 1084) > 0 || (s16)fn_802753E4(self, 8) > 0) {
            self->unk3DC |= 0x20000;
        }
        if (*(s16*)((u8*)self + 1086) > 0 || (s16)fn_802753E4(self, 9) > 0) {
            self->unk3DC |= 0x40000;
        }
        if (*(s16*)((u8*)self + 1088) > 0 || (s16)fn_802753E4(self, 10) > 0) {
            self->unk3DC |= 0x80000;
        }
    }
}

extern "C" s32 fn_8026A6F4(_PLW*, s32);
void sysSE_req(s32);
extern "C" u32 fn_803B521C(s32);
extern "C" u32 fn_803AB190(s32);

/* 0x8027B918: steps the actor's clutch state machine from the pad edge events the move work reports. */
extern "C" void fn_8027B918(_PLW* self)
{
    u8* p;
    u8* q;
    if (Pl_master_ck(self) == 0) {
        return;
    }
    if (*(u8*)((u8*)self + 0x5BD) == 0) {
        return;
    }
    p = get_move_work_adrs(0);
    if (p == 0) {
        return;
    }
    q = *(u8**)(p + 220);
    if (q == 0) {
        return;
    }
    switch (*(u8*)((u8*)self + 0x5C0)) {
    case 0:
        sysSE_req(5);
        *(s16*)((u8*)self + 0x5C2) = 30;
        (*(u8*)((u8*)self + 0x5C0))++;
        return;
    case 1: {
        if (*(s16*)((u8*)self + 0x5C2) != 0) {
            (*(s16*)((u8*)self + 0x5C2))--;
        }
        if (*(s16*)((u8*)self + 0x5C2) != 0) {
            break;
        }
        if ((u32)fn_8026A6F4(self, 0x17) == 1) {
            sysSE_req(1);
            *(u8*)((u8*)self + 0x5BD) = 0;
            return;
        }
        if ((u32)fn_8026A6F4(self, 0x16) == 1) {
            switch (*(u8*)((u8*)self + 0x5BF)) {
            case 0:
                sysSE_req(0);
                *(u8*)((u8*)self + 0x5BD) = 1;
                return;
            case 1:
                if ((u32)fn_8027BC48(0) != 1) {
                    if ((u32)fn_803B521C(0) == 1) {
                        if (fn_803AB190(1) == 1 || fn_803AB190(2) == 1) {
                            (*(u8*)((u8*)self + 0x5C0))++;
                            *(u8*)(q + 0x68D) = 1;
                            sysSE_req(0);
                            return;
                        }
                        sysSE_req(2);
                        return;
                    }
                    sysSE_req(2);
                    return;
                }
                sysSE_req(2);
                return;
            }
        } else {
            if ((u32)fn_8026A6F4(self, 0x12) == 1) {
                sysSE_req(6);
                if (*(u8*)((u8*)self + 0x5BF) == 0) {
                    *(u8*)((u8*)self + 0x5BF) = 1;
                } else {
                    (*(u8*)((u8*)self + 0x5BF))--;
                }
            }
            if ((u32)fn_8026A6F4(self, 0x13) == 1) {
                sysSE_req(6);
                if (*(u8*)((u8*)self + 0x5BF) >= 1) {
                    *(u8*)((u8*)self + 0x5BF) = 0;
                    return;
                }
                (*(u8*)((u8*)self + 0x5BF))++;
                return;
            }
        }
        break;
    }
    case 2:
        if (*(s8*)(q + 0x68D) == 0) {
            if ((u32)fn_8026A6F4(self, 0x17) == 1) {
                *(u8*)(q + 0x68D) = 1;
                sysSE_req(1);
                return;
            }
            if ((u32)fn_8026A6F4(self, 0x16) == 1) {
                if ((s32)*(u8*)((u8*)self + 0x5BF) == 1 && (u32)fn_8027BC48(0) != 1) {
                    sysSE_req(0);
                    *(u8*)((u8*)self + 0x5BD) = 1;
                    return;
                }
            } else if ((u32)fn_8026A6F4(self, 0x15) == 1) {
                *(u8*)(q + 0x68D) = 1;
                sysSE_req(3);
                return;
            }
        } else {
            if ((u32)fn_8026A6F4(self, 0x16) == 1 || (u32)fn_8026A6F4(self, 0x17) == 1) {
                *(u8*)((u8*)self + 0x5C0) = 1;
                sysSE_req(1);
                return;
            }
            if ((u32)fn_8026A6F4(self, 0x14) == 1) {
                *(u8*)(q + 0x68D) = 0;
                sysSE_req(3);
            }
        }
        break;
    }
}

extern "C" s32 fn_802731B4(_PLW*, u16);
extern "C" s32 fn_803B31E0(s8);
extern "C" void fn_803B6078(u16, u16*);
extern "C" s32 fn_803B6150(u16);
extern "C" void fn_803B2C60(_PLW*, u16, s8);

/* 0x8027B358: steps the shell-selection state machine - the clutch index the move work stores plus the
 * three charge levels - from the pad edge events. */
extern "C" void fn_8027B358(_PLW* self)
{
    u16 sp8;
    if (Pl_master_ck(self) == 0) {
        return;
    }
    if (*(u8*)((u8*)self + 0x5BE) == 0) {
        return;
    }
    u8* p = get_move_work_adrs(0);
    if (p == 0) {
        return;
    }
    u8* q = *(u8**)(p + 220);
    if (q == 0) {
        return;
    }
    if ((u32)fn_8027BC48(0) == 1) {
        sysSE_req(1);
        *(u8*)((u8*)self + 0x5BE) = 0;
        return;
    }
    fn_803B6078((u16)*(s8*)(q + 0x68C), &sp8);
    s32 r28 = fn_803B6150((u16)*(s8*)(q + 0x68C));
    *(u16*)(q + 0x696) = 0;
    switch (*(u8*)((u8*)self + 0x5C0)) {
    case 0:
        *(u8*)(q + 0x68E) = 1;
        *(u8*)((u8*)self + 0x5C0) = 1;
        return;
    case 1:
        if ((u32)fn_8026A6F4(self, 0x13) == 1) {
            (*(u8*)(q + 0x68C))++;
            if (*(s8*)(q + 0x68C) >= 3) {
                *(u8*)(q + 0x68C) = 2;
                return;
            }
            sysSE_req(6);
            return;
        }
        if ((u32)fn_8026A6F4(self, 0x12) == 1) {
            (*(u8*)(q + 0x68C))--;
            if (*(s8*)(q + 0x68C) < 0) {
                *(u8*)(q + 0x68C) = 0;
                return;
            }
            sysSE_req(6);
            return;
        }
        if ((u32)fn_8026A6F4(self, 0x16) == 1) {
            switch (fn_803B31E0(*(s8*)(q + 0x68C))) {
            case 0:
                sysSE_req(2);
                return;
            case 1:
                sysSE_req(2);
                return;
            case 2:
                if (fn_802731B4(self, sp8) < 1) {
                    sysSE_req(2);
                    return;
                }
                *(u8*)((u8*)self + 0x5C0) = 2;
                *(u8*)(q + 0x68E) = 1;
                sysSE_req(0);
                return;
            }
        } else if ((u32)fn_8026A6F4(self, 0x17) == 1) {
            sysSE_req(1);
            *(u8*)((u8*)self + 0x5C0) = 1;
            *(u8*)((u8*)self + 0x5BE) = 0;
            return;
        }
        break;
    case 2:
        if (fn_803B31E0(*(s8*)(q + 0x68C)) != 2) {
            sysSE_req(1);
            *(u8*)((u8*)self + 0x5C0) = 1;
            return;
        }
        if (*(s8*)(q + 0x68E) > r28) {
            sysSE_req(6);
            *(s8*)(q + 0x68E) = r28;
            return;
        }
        if ((u32)fn_8026A6F4(self, 0x13) == 1) {
            if (*(s8*)(q + 0x68E) > 1) {
                sysSE_req(6);
                *(u16*)(q + 0x696) |= 2;
                (*(u8*)(q + 0x68E))--;
                return;
            }
            *(u8*)(q + 0x68E) = 1;
            return;
        }
        if ((u32)fn_8026A6F4(self, 0x12) == 1) {
            u8 t31 = *(u8*)(q + 0x68E);
            if ((s8)t31 < fn_802731B4(self, sp8)) {
                if ((s8)t31 < r28) {
                    sysSE_req(6);
                    *(u16*)(q + 0x696) |= 1;
                    (*(u8*)(q + 0x68E))++;
                    return;
                }
                *(s8*)(q + 0x68E) = r28;
                return;
            }
        } else if ((u32)fn_8026A6F4(self, 0x15) == 1) {
            u8 t31b = *(u8*)(q + 0x68E);
            if (r28 < fn_802731B4(self, sp8)) {
                *(s8*)(q + 0x68E) = r28;
            } else {
                *(s8*)(q + 0x68E) = fn_802731B4(self, sp8);
            }
            if ((s8)t31b != *(s8*)(q + 0x68E)) {
                sysSE_req(6);
                *(u16*)(q + 0x696) |= 1;
                return;
            }
        } else {
            if ((u32)fn_8026A6F4(self, 0x14) == 1) {
                if (*(s8*)(q + 0x68E) != 1) {
                    sysSE_req(6);
                    *(u16*)(q + 0x696) |= 2;
                }
                *(u8*)(q + 0x68E) = 1;
                return;
            }
            if ((u32)fn_8026A6F4(self, 0x16) == 1) {
                sysSE_req(0);
                *(u8*)((u8*)self + 0x5C0) = 3;
                *(u8*)(q + 0x68D) = 0;
                return;
            }
            if ((u32)fn_8026A6F4(self, 0x17) == 1) {
                sysSE_req(1);
                *(u8*)((u8*)self + 0x5C0) = 1;
                return;
            }
        }
        break;
    case 3:
        if (fn_803B31E0(*(s8*)(q + 0x68C)) != 2) {
            sysSE_req(1);
            *(u8*)((u8*)self + 0x5C0) = 1;
            return;
        }
        if (*(s8*)(q + 0x68E) > r28) {
            *(s8*)(q + 0x68E) = r28;
            return;
        }
        if ((u32)fn_8026A6F4(self, 0x13) == 1) {
            if (*(s8*)(q + 0x68D) < 1) {
                sysSE_req(6);
                (*(u8*)(q + 0x68D))++;
                return;
            }
            *(u8*)(q + 0x68D) = 1;
            return;
        }
        if ((u32)fn_8026A6F4(self, 0x12) == 1) {
            if (*(s8*)(q + 0x68D) > 0) {
                sysSE_req(6);
                (*(u8*)(q + 0x68D))--;
                return;
            }
            *(u8*)(q + 0x68D) = 0;
            return;
        }
        if ((u32)fn_8026A6F4(self, 0x16) == 1) {
            if (*(s8*)(q + 0x68D) == 1) {
                sysSE_req(1);
                *(u8*)((u8*)self + 0x5C0) = 2;
                *(u8*)(q + 0x68D) = 0;
                return;
            }
            sysSE_req(8);
            *(u8*)((u8*)self + 0x5C0) = 1;
            fn_803B2C60(self, sp8, *(s8*)(q + 0x68E));
            return;
        }
        if ((u32)fn_8026A6F4(self, 0x17) == 1) {
            sysSE_req(1);
            *(u8*)((u8*)self + 0x5C0) = 2;
            return;
        }
        break;
    default:
        *(u8*)((u8*)self + 0x5BE) = 0;
        break;
    }
}

extern "C" s32 fn_80273228(_PLW*, u16, s32);

/* 0x8027B0BC: steps the handling-direction state machine - an 8-way direction on a five-row grid of
 * stored action entries - from the pad edge events. */
extern "C" void fn_8027B0BC(_PLW* self)
{
    if (Pl_master_ck(self) == 0) {
        return;
    }
    if ((s32)*(u8*)((u8*)self + 0x5BC) == 0) {
        return;
    }
    u8* p = get_move_work_adrs(0);
    if (p == 0) {
        return;
    }
    u8* q = *(u8**)(p + 220);
    if (q == 0) {
        return;
    }
    if (*(s16*)((u8*)self + 0x5C2) != 0) {
        (*(s16*)((u8*)self + 0x5C2))--;
    }
    if (*(s16*)((u8*)self + 0x65A) != 0) {
        (*(s16*)((u8*)self + 0x65A))--;
        if (*(s16*)((u8*)self + 0x65A) == 0) {
            *(u8*)((u8*)self + 0x659) = 0;
        }
    }
    if ((s32)*(u8*)((u8*)self + 0x659) != 0) {
        return;
    }
    if (*(s16*)((u8*)self + 0x5C2) != 0) {
        return;
    }
    if ((u32)fn_8026A6F4(self, 0x17) == 1) {
        *(u8*)((u8*)self + 0x5BC) = 0;
        sysSE_req(1);
        return;
    }
    if ((u32)fn_8026A6F4(self, 0x14) == 1) {
        sysSE_req(3);
        s32 v = *(u8*)(q + 0x5E1);
        if ((s32)((s8)v & 7) != 0) {
            (*(u8*)(q + 0x5E1))--;
        } else {
            (*(u8*)(q + 0x5E1)) += 7;
        }
    } else if ((u32)fn_8026A6F4(self, 0x15) == 1) {
        sysSE_req(3);
        s32 v = *(u8*)(q + 0x5E1);
        if ((s32)((s8)v & 7) != 7) {
            (*(u8*)(q + 0x5E1))++;
        } else {
            (*(u8*)(q + 0x5E1)) -= 7;
        }
    }
    if ((u32)fn_8026A6F4(self, 0x13) == 1) {
        sysSE_req(3);
        (*(u8*)(q + 0x5E1)) += 8;
        s32 w = *(u8*)(q + 0x5E1);
        if ((s8)w >= 40) {
            (*(u8*)(q + 0x5E1)) -= 40;
        }
    } else if ((u32)fn_8026A6F4(self, 0x12) == 1) {
        sysSE_req(3);
        (*(u8*)(q + 0x5E1)) -= 8;
        s32 w = *(u8*)(q + 0x5E1);
        if ((s8)w < 0) {
            (*(u8*)(q + 0x5E1)) += 40;
        }
    }
    u8 dir = *(u8*)(q + 0x5E1);
    if (((1 << ((s8)dir & 0x1F)) & *(u32*)(q + 0x684 + ((s8)dir >> 5) * 4)) == 0) {
        if ((u32)fn_8027AF80(*(u8*)(q + 0x5E1)) == 1) {
            if ((u32)fn_8026A6F4(self, 0x16) == 1) {
                u16 item = *(u16*)(q + 0x5E2 + (s8)*(u8*)(q + 0x5E1) * 4);
                if ((s32)item != 0) {
                    s16 val = *(s16*)(q + 0x5E4 + (s8)*(u8*)(q + 0x5E1) * 4);
                    if (val > 0) {
                        if (fn_80273228(self, item, val) >= val) {
                            sysSE_req(8);
                            fn_8027AF88(self);
                            return;
                        }
                        sysSE_req(2);
                    }
                }
            }
        }
    }
}

/* The two side structures the attack-subsystem setters fill in: `_HIT_W` is the attack entry the
 * caller uses, `_HIT_DATA` the hit record it came from. Only the offsets this unit touches are named. */
struct _HIT_DATA {
    u8 unk00[0x12];
    s8 unk12;
};

struct _HIT_W {
    u8 unk00[0x31];
    u8 unk31;
    u8 unk32;
    u8 unk33[0x40 - 0x33];
    s16 unk40;
    u8 unk42[0x48 - 0x42];
    u16 unk48;
    u8 unk4A;
    u8 unk4B;
    u8 unk4C;
    u8 unk4D;
    s8 unk4E;
    u8 unk4F[0x5A - 0x4F];
    u8 unk5A;
};

extern "C" u8 fn_80331210(_PLW*);
extern "C" s16 fn_80273ED8(_PLW*, s32, s32);
s32 Pl_critical_get(_PLW*);

/* 0x80277284: fills in the attack entry an incoming hit produces - the skill-granted hit flags, the
 * hit rate multipliers and the critical roll - from the attack flags and the actor's skills. */
void Pl_attack_set_sub(_PLW* self, _HIT_DATA* hit, _HIT_W* data, u16 flags)
{
    s32 f8;
    s32 f10;
    data->unk31 = 0xF;
    data->unk32 = 1;
    f10 = flags & 0x10;
    if (f10 != 0 && Pl_Skill_ck(self, 0xC7) == 1) {
        s8 v = hit->unk12;
        if ((s32)v < 0) {
            data->unk4E = (s8)-v;
        }
    }
    if ((s8)data->unk4E > 0 && Pl_cat_skill_ck(self, 0x28) == 1) {
        data->unk4E = (s8)(lbl_8079A0A8 * (f32)(s8)data->unk4E);
    }
    if ((s32)(flags & 2) != 0) {
        s32 sel;
        s16 t;
        f8 = flags & 8;
        if (f8 != 0 && fn_80331210(self) == 1) {
            sel = 1;
        } else {
            sel = 0;
            if (Pl_Skill_ck(self, 0xC9) == 1) {
                sel = 1;
            }
        }
        t = fn_80273ED8(self, 7, sel);
        if (t > 0) {
            data->unk48 |= 0x10;
            data->unk4A = (u8)t;
        }
        t = fn_80273ED8(self, 8, sel);
        if (t > 0) {
            data->unk48 |= 0x20;
            data->unk4A = (u8)t;
        }
        t = fn_80273ED8(self, 9, sel);
        if (t > 0) {
            data->unk48 |= 0x40;
            data->unk4A = (u8)t;
        }
        t = fn_80273ED8(self, 0xA, sel);
        if (t > 0) {
            data->unk48 |= 0x80;
            data->unk4A = (u8)t;
        }
        t = fn_80273ED8(self, 0xB, sel);
        if (t > 0) {
            data->unk48 |= 0x200;
            data->unk4A = (u8)t;
        }
        if (f8 != 0 && fn_80331210(self) == 3) {
            data->unk48 = 0x80;
            t = fn_80273ED8(self, 0xF, 1);
            if (t <= 0) {
                data->unk4A = 1;
            } else {
                data->unk4A = (u8)t;
            }
        }
        if ((s32)(*(u16*)((u8*)self + 0xB6) % 3) == 0) {
            t = fn_80273ED8(self, 0xC, sel);
            if (t > 0) {
                data->unk48 |= 2;
                data->unk4A = (u8)t;
            }
            t = fn_80273ED8(self, 0xD, sel);
            if (t > 0) {
                data->unk48 |= 4;
                data->unk4A = (u8)t;
            }
            t = fn_80273ED8(self, 0xE, sel);
            if (t > 0) {
                data->unk48 |= 1;
                data->unk4A = (u8)t;
            }
            if (f8 != 0 && fn_80331210(self) == 2) {
                data->unk48 = (u16)((data->unk48 & 0xFFFC) | 4);
                t = fn_80273ED8(self, 0x10, 1);
                if (t <= 0) {
                    data->unk4A = 1;
                } else {
                    data->unk4A = (u8)t;
                }
            }
        }
        if ((s32)data->unk48 != 0) {
            data->unk5A = 0xA;
        }
    }
    if (self->unk002 == 7 && (flags & 1) != 0) {
        switch (fn_802748C8(self)) {
        case 1:
            data->unk40 = (s16)(lbl_8079A0AC * (f32)data->unk40);
            break;
        case 2:
            data->unk40 = (s16)(lbl_8079A0A8 * (f32)data->unk40);
            break;
        case 3:
            data->unk40 = (s16)(lbl_8079A0B0 * (f32)data->unk40);
            break;
        }
    }
    if ((flags & 8) != 0) {
        switch (fn_80331210(self)) {
        case 0:
            data->unk40 = (s16)(lbl_8079A0B4 * (f32)data->unk40);
            break;
        case 1:
            if ((s32)data->unk48 != 0) {
                u8 c = data->unk4A;
                if ((s32)c != 0) {
                    data->unk4A = (u8)(s32)(lbl_8079A0B4 * (f32)c);
                }
            }
            break;
        }
    }
    if ((flags & 0x20) != 0 && Pl_cat_skill_ck(self, 0x1F) == 1) {
        data->unk40 = (s16)(data->unk40 * 5);
    }
    if ((flags & 0x40) != 0 && Pl_cat_skill_ck(self, 0x20) == 1) {
        data->unk40 = (s16)(data->unk40 * 5);
    }
    if ((flags & 0x14) != 0) {
        s32 crit = Pl_critical_get(self);
        s32 sc;
        if (f10 != 0 && Pl_Skill_ck(self, 0xB7) == 1) {
            crit = 100;
        }
        sc = (s16)crit;
        if (sc != 0 && (u32)*(u8*)((u8*)self + 0x56B) >= 1) {
            if (sc > 0) {
                if ((s32)(*(u16*)((u8*)self + 0xB6) % 100) < sc) {
                    data->unk40 = (s16)(lbl_8079A0B4 * (f32)data->unk40);
                    data->unk4C |= 0x80;
                }
            } else {
                if ((s32)(*(u16*)((u8*)self + 0xB6) % 100) < -sc) {
                    if (data->unk40 > 0) {
                        data->unk40 = (s16)(lbl_8079A08C * (f32)data->unk40);
                        if (data->unk40 == 0) {
                            data->unk40 = 1;
                        }
                        data->unk4C |= 0x40;
                    }
                }
            }
        }
    }
}

/* The move-work records `get_move_work_adrs(2)` hands back: an array of actor-shaped entries on a
 * 0xB20 stride. Only the offsets this unit touches are named. */
struct _MOVE_WORK {
    u8 unk000;
    u8 unk001[0x16 - 0x01];
    u8 unk016;
    u8 unk017[0x3DC - 0x17];
    u32 unk3DC;
    u8 unk3E0[0x3EA - 0x3E0];
    s16 unk3EA;
    u8 unk3EC[0x404 - 0x3EC];
    s16 unk404;
    s16 unk406;
    s16 unk408;
    s16 unk40A;
    s16 unk40C;
    u8 unk40E[0x412 - 0x40E];
    s16 unk412;
    u8 unk414[0x42E - 0x414];
    s16 unk42E;
    s16 unk430;
    s16 unk432;
    s16 unk434;
    s16 unk436;
    s16 unk438;
    s16 unk43A;
    s16 unk43C;
    s16 unk43E;
    s16 unk440;
    u8 unk442[0x44A - 0x442];
    s16 unk44A;
    u8 unk44C[0x44E - 0x44C];
    s16 unk44E;
    u8 unk450[0x464 - 0x450];
    s16 unk464;
    s16 unk466;
};

u16 get_move_work_max(u8);
extern "C" void fn_8027628C(_MOVE_WORK*, s32);
extern "C" void fn_8027D6A4(_MOVE_WORK*, s32, s32);
extern "C" void fn_8027D6C0(_MOVE_WORK*, s32, s32);

/* 0x8027D0D4: applies one scripted action to every move-work record of type 2 whose kind byte matches
 * `arg0` - the per-action clamps and the state-helper calls of the attack/critical chain. */
extern "C" void fn_8027D0D4(u8 arg0, u8 arg1)
{
    s32 i;
    u8* r29;
    s32 max;
    r29 = get_move_work_adrs(2);
    max = get_move_work_max(2);
    for (i = 0; i < max; i++) {
        _MOVE_WORK* p = (_MOVE_WORK*)r29;
        if (p->unk000 != 0 && p->unk016 == arg0) {
            switch (arg1) {
            case 0:
                fn_8027628C(p, 0x32);
                fn_80114C20((_PLW*)p, 0);
                break;
            case 1:
                fn_80279154((_PLW*)p, 1, 0xA);
                p->unk44A = 0x1518;
                fn_80114C20((_PLW*)p, 1);
                break;
            case 2:
                if ((u32)fn_80279194((_PLW*)p, 1, 0x14) == 1) {
                    p->unk44E = 0x1518;
                }
                fn_80114C20((_PLW*)p, 2);
                break;
            case 3:
            case 6:
                if (arg1 == 3) {
                    fn_8027628C(p, 0x14);
                } else {
                    fn_8027628C(p, 0x32);
                }
                fn_80114C20((_PLW*)p, 0);
                break;
            case 4:
            case 7:
                if (arg1 == 4) {
                    fn_80279154((_PLW*)p, 1, 3);
                } else {
                    fn_80279154((_PLW*)p, 1, 5);
                }
                p->unk44A = 0x1518;
                fn_80114C20((_PLW*)p, 1);
                break;
            case 5:
            case 8:
                if (arg1 == 5) {
                    if ((u32)fn_80279194((_PLW*)p, 1, 0xA) == 1) {
                        p->unk44E = 0x1518;
                    }
                } else if ((u32)fn_80279194((_PLW*)p, 1, 0x14) == 1) {
                    p->unk44E = 0x1518;
                }
                fn_80114C20((_PLW*)p, 2);
                break;
            case 9:
                p->unk3EA = 0;
                fn_80114C20((_PLW*)p, 4);
                break;
            case 10:
                if (p->unk466 < 0x2328) {
                    p->unk466 = 0x2328;
                }
                fn_80114C20((_PLW*)p, 7);
                break;
            case 11:
                if (p->unk464 < 0x2328) {
                    p->unk464 = 0x2328;
                }
                fn_80114C20((_PLW*)p, 6);
                break;
            case 12:
                if (p->unk412 < 0x1518) {
                    p->unk412 = 0x1518;
                }
                fn_80114C20((_PLW*)p, 5);
                break;
            case 13:
                fn_8027D6A4(p, 1, 0x1518);
                fn_8027D6C0(p, 1, 0x1518);
                fn_80114C20((_PLW*)p, 9);
                break;
            case 14:
                p->unk42E = 0;
                if (p->unk438 < 0x1518) {
                    p->unk438 = 0x1518;
                }
                p->unk430 = 0;
                if (p->unk43A < 0x1518) {
                    p->unk43A = 0x1518;
                }
                p->unk432 = 0;
                if (p->unk43C < 0x1518) {
                    p->unk43C = 0x1518;
                }
                p->unk434 = 0;
                if (p->unk43E < 0x1518) {
                    p->unk43E = 0x1518;
                }
                p->unk436 = 0;
                if (p->unk440 < 0x1518) {
                    p->unk440 = 0x1518;
                }
                i = 5;
                p->unk404 = 0;
                p->unk408 = 0;
                p->unk406 = 0;
                p->unk40A = 0;
                p->unk40C = 0;
                p->unk3DC &= 0xFFFFFC00;
                fn_80114C20((_PLW*)p, 9);
                break;
            }
        }
        r29 += 0xB20;
    }
}

/* 0x8027BE4C: maps the actor's five stored status durations onto the condition word and the shell
 * timers it exposes. */
extern "C" void fn_8027BE4C(_PLW* self)
{
    s16 t;
    u32 bits;
    if (Pl_master_ck(self) == 0) {
        return;
    }
    if (Pl_Skill_ck(self, 0xC8) == 1) {
        return;
    }
    t = self->unk38C;
    if (t > 0) {
        bits = self->unk3DC & 0xFFFFFFFC;
        self->unk3DC = bits;
        if (t >= 0x33) {
            self->unk3DC = bits | 2;
            self->unk404 = 0x1C2;
        } else if (t >= 0x1F) {
            self->unk3DC = bits | 1;
            self->unk404 = 0x1C2;
        }
    }
    t = self->unk38E;
    if (t > 0) {
        bits = self->unk3DC & 0xFFFFFFF3;
        self->unk3DC = bits;
        if (t >= 0x33) {
            self->unk3DC = bits | 8;
            *(s16*)((u8*)self + 0x406) = 0x708;
        } else if (t >= 0x1F) {
            self->unk3DC = bits | 4;
            *(s16*)((u8*)self + 0x406) = 0x384;
        }
    }
    t = self->unk390;
    if (t > 0) {
        bits = self->unk3DC & 0xFFFFFFCF;
        self->unk3DC = bits;
        if (t >= 0x33) {
            self->unk3DC = bits | 0x20;
            *(s16*)((u8*)self + 0x408) = 0xE10;
            *(s16*)((u8*)self + 0x442) = 0xB4;
        } else if (t >= 0x1F) {
            self->unk3DC = bits | 0x10;
            *(s16*)((u8*)self + 0x408) = 0x708;
            *(s16*)((u8*)self + 0x442) = 0xB4;
        }
    }
    t = self->unk392;
    if (t > 0) {
        bits = self->unk3DC & 0xFFFFFF3F;
        self->unk3DC = bits;
        if (t >= 0x33) {
            self->unk3DC = bits | 0x80;
            *(s16*)((u8*)self + 0x40A) = 0x708;
        } else if (t >= 0x1F) {
            self->unk3DC = bits | 0x40;
            *(s16*)((u8*)self + 0x40A) = 0x384;
        }
    }
    t = self->unk394;
    if (t > 0) {
        bits = self->unk3DC & 0xFFFFFCFF;
        self->unk3DC = bits;
        if (t >= 0x33) {
            self->unk3DC = bits | 0x200;
            *(s16*)((u8*)self + 0x40C) = 0xE10;
            *(s16*)((u8*)self + 0x442) = 0xB4;
            return;
        }
        if (t >= 0x1F) {
            self->unk3DC = bits | 0x100;
            *(s16*)((u8*)self + 0x40C) = 0x708;
            *(s16*)((u8*)self + 0x442) = 0xB4;
        }
    }
}

extern "C" void fn_80276690(_PLW*, s16, s32);
extern "C" void fn_80276778(_PLW*, s16);
extern "C" s16 fn_8026FF20(_PLW*);

/* 0x8027885C: resets the actor's shell/ammo timers and, when the relevant skill is on, reseeds the
 * three shell tables from the actor's current state. */
extern "C" void fn_8027885C(_PLW* self, s32 arg1, s32 arg2)
{
    *(s16*)((u8*)self + 0x37A) = 0x258;
    *(s16*)((u8*)self + 0x372) = 0x64;
    *(s16*)((u8*)self + 0x380) = 0x64;
    if ((u32)Pl_master_ck(self) == 1 && (arg1 == 0 || Pl_cat_skill_ck(self, 0x27) == 1)) {
        fn_80276690(self, (s16)fn_802753E4(self, 1), 0);
        fn_80276CE8(self, (s16)fn_802753E4(self, 2));
        fn_80276778(self, (s16)fn_802753E4(self, 3));
    }
    fn_80276690(self, fn_8026FF20(self), 0);
    *(s16*)((u8*)self + 0x36C) = 0x32;
    *(s16*)((u8*)self + 0x36E) = 0x96;
    if (arg2 == 0) {
        s16 v;
        *(s16*)((u8*)self + 0x378) = *(s16*)((u8*)self + 0x37A);
        *(s16*)((u8*)self + 0x37C) = 0x2A30;
        *(u8*)((u8*)self + 0x39E) = 0;
        *(s16*)((u8*)self + 0x38A) = 0;
        v = *(s16*)((u8*)self + 0x372);
        *(s16*)((u8*)self + 0x370) = v;
        *(s16*)((u8*)self + 0x376) = v;
        *(s16*)((u8*)self + 0x37E) = *(s16*)((u8*)self + 0x380);
    }
}

/* 0x802791FC: the shell-level cap one equipment slot contributes, before the actor's skill modifiers
 * are applied. */
extern "C" s32 fn_802791FC(_PLW* self, u8 arg1)
{
    if ((s32)fn_8026FE44(self) == 0) {
        return 1;
    }
    s32 a = *(u8*)(lbl_805BF538 + 1 + arg1 * 4);
    s32 b = *(u8*)(fn_8027ED18(&self->equipB) + 9);
    if (self->equipD[0] == 0xD) {
        b += *(u8*)(fn_8027ED18(&self->equipD) + 9);
    }
    if (Pl_Skill_ck(self, 0x27) == 1) {
        b = (s16)(b + 2);
    }
    if (Pl_Skill_ck(self, 0x28) == 1) {
        b = (s16)(b + 3);
    }
    if (Pl_Skill_ck(self, 0x29) == 1) {
        b = (s16)(b + 4);
    }
    if (Pl_Skill_ck(self, 0x2A) == 1) {
        b = (s16)(b - 1);
    }
    if (Pl_Skill_ck(self, 0x2B) == 1) {
        b = (s16)(b - 2);
    }
    if (Pl_Skill_ck(self, 0x2C) == 1) {
        b = (s16)(b - 3);
    }
    a -= b;
    if ((s16)a <= 4) {
        return 0;
    }
    return ((s16)a <= 7) ? 1 : 2;
}

extern u8 lbl_805BFCD8[];
extern u8 lbl_805C6100[];
extern u8 lbl_80792150;

/* 0x80275A80: the shell "bure" type (0/1/2) the actor's equipped shell and its skills resolve to. */
u32 Get_Shell_bure_type(_PLW* self, u8 arg1)
{
    u8* t31 = lbl_805BFCD8 + *(u8*)(fn_8027ED18(&self->equipB) + 0x10) * 2;
    s32 v29 = t31[1];
    v29 += *(s8*)(lbl_805BF538 + 3 + arg1 * 4);
    s32 r29;
    if (self->equipC[0] == 0xC) {
        v29 -= (&lbl_80792150)[*(u8*)(fn_8027ED18(&self->equipC) + 0x10)];
    }
    if ((s8)v29 <= 5) {
        r29 = 0;
    } else {
        r29 = ((s8)v29 <= 15) ? 1 : 2;
    }
    if (Pl_Skill_ck(self, 0xAC) == 1) {
        if (Pl_cat_skill_ck(self, 0x1E) == 1) {
            if ((u8)r29 != 0) {
                r29 -= 1;
            }
        } else {
            r29 = 0;
        }
    } else if (Pl_Skill_ck(self, 0xAB) == 1) {
        if (Pl_cat_skill_ck(self, 0x1E) == 0 && (u8)r29 != 0) {
            r29 -= 1;
        }
    } else if (Pl_Skill_ck(self, 0xAD) == 1) {
        if (Pl_cat_skill_ck(self, 0x1E) == 1) {
            r29 = 2;
        } else if ((u8)r29 < 2) {
            r29 += 1;
        }
    } else if (Pl_cat_skill_ck(self, 0x1E) == 1 && (u8)r29 < 2) {
        r29 = (u8)((u8)r29 + 1);
    }
    u8 t3 = t31[0];
    return lbl_805C6100[(u8)r29 + (t3 * 4 - t3)];
}

/* 0x80279490: the shell capacity one equipment slot contributes after the actor's skill modifiers,
 * mapped onto the 0-3 capacity class the caller uses. */
extern "C" s32 fn_80279490(_PLW* self, u8 arg1)
{
    if ((s32)fn_8026FE44(self) == 0) {
        return 2;
    }
    s32 a = *(u8*)(lbl_805BF538 + arg1 * 4);
    u8* p = fn_80279414(self, arg1);
    if (p != 0) {
        return (s16)(*(u8*)(p + 3) + 3);
    }
    s32 b = 0;
    if (fn_80279414(self, arg1) == 0) {
        b = *(u8*)(fn_8027ED18(&self->equipB) + 7);
        if (self->equipD[0] == 0xD) {
            b += *(u8*)(fn_8027ED18(&self->equipD) + 7);
        }
        if (Pl_Skill_ck(self, 0x2D) == 1) {
            b += 2;
        } else if (Pl_Skill_ck(self, 0x2E) == 1) {
            b += 3;
        } else if (Pl_Skill_ck(self, 0x2F) == 1) {
            b = (s16)(b + 4);
        }
        if (Pl_Skill_ck(self, 0x30) == 1) {
            b -= 1;
        } else if (Pl_Skill_ck(self, 0x31) == 1) {
            b -= 2;
        } else if (Pl_Skill_ck(self, 0x32) == 1) {
            b = (s16)(b - 3);
        }
    }
    a -= b;
    s32 r3 = 2;
    if ((s16)a <= 0) {
        a = 0;
    }
    if ((s16)a <= 8) {
        r3 = 0;
    } else if ((s16)a <= 0xA) {
        r3 = 1;
    }
    if ((u32)(arg1 - 0x27) <= 2) {
        if (r3 == 0) {
            return 3;
        }
        return (r3 == 1) ? 8 : 9;
    }
    return r3;
}

extern "C" s32 fn_8026A6A4(_PLW*, s32);
extern "C" void fn_80279EBC(_PLW*, u16, u16);
extern "C" s32 fn_8027A340(_PLW*);
extern "C" u32 fn_80287244(_PLW*, s32);
extern "C" void fn_802B9740(_PLW*, u8*, u8*, f32*);
extern "C" u8 fn_803BECA0(u8, s32);
extern u8 lbl_805C6118[];
extern const f32 lbl_8079A080;
extern const f32 lbl_8079A0F0;

/* 0x8027A57C: resolves one motion's stick input into the actor's movement - the four pad-direction
 * flags, the direction and magnitude the move table interpolates for the current speed, and the
 * per-motion write-back. */
extern "C" void fn_8027A57C(_PLW* self, u16 arg1, u8 arg2)
{
    u8 sp9;
    u8 sp8;
    f32 spC;
    s32* t;
    s32 mag, dir, speed, step;
    u8 f27, f26, f25, f24;
    f27 = 0;
    f26 = 0;
    f25 = 0;
    f24 = 0;
    mag = 0;
    dir = 0;
    step = 0;
    speed = *(s32*)((u8*)self + 0xA8);

    if (Pl_master_ck(self) == 0) {
        return;
    }
    if (arg1 != (u16)Get_motion_no(self)) {
        return;
    }
    if ((s8)self->unk36A != 0) {
        return;
    }
    if ((s32)self->unk5E5 != 0 || (s32)self->unk5E6 != 0) {
        self->unk5E5 = 0xA;
    }
    for (;;) {
        if ((s32)self->unk5E5 == 0 && (s32)self->unk5E6 == 0) {
            if ((u32)fn_8026A6F4(self, 0xB) == 1) {
                break;
            }
            if (arg2 == 0) {
                if (!(self->unk00A == 4 || fn_8027A340(self) != 0)) {
                    break;
                }
            } else if (arg2 == 2) {
                if (fn_8026A644(self, 0x37) == 0) {
                    break;
                }
            }
        }
        switch (arg2) {
        case 0:
            if (fn_8027A340(self) != 0 ||
                ((s32)self->unk5E6 == 0 && (u32)fn_80287244(self, 0) == 1)) {
                u8 c;
                u8 r;
                if (fn_8026A6A4(self, 0) != 0) {
                    f25 = 1;
                } else if (fn_8026A6A4(self, 1) != 0) {
                    f24 = 1;
                }
                if (fn_8026A6A4(self, 2) != 0) {
                    f27 = 1;
                } else if (fn_8026A6A4(self, 3) != 0) {
                    f26 = 1;
                }
                r = fn_803BECA0(self->unk008, 2);
                if (r == 1 || r == 3) {
                    c = f25;
                    f25 = f24;
                    f24 = c;
                }
                if ((u8)(r + 0xFE) <= 1) {
                    c = f27;
                    f27 = f26;
                    f26 = c;
                }
                mag = 5;
                if ((s32)(f26 + f24 + (f27 + f25)) != 0) {
                    step = 0x180;
                }
            }
            break;
        case 2:
            if ((s32)self->unk5E6 == 0) {
                u8 r;
                u8 c;
                if (fn_8026A644(self, 0x38) != 0) {
                    f25 = 1;
                } else if (fn_8026A644(self, 0x39) != 0) {
                    f24 = 1;
                }
                if (fn_8026A644(self, 0x3A) != 0) {
                    f27 = 1;
                } else if (fn_8026A644(self, 0x3B) != 0) {
                    f26 = 1;
                }
                r = fn_803BECA0(self->unk008, 2);
                if (r == 1 || r == 3) {
                    c = f25;
                    f25 = f24;
                    f24 = c;
                }
                if ((u8)(r + 0xFE) <= 1) {
                    c = f27;
                    f27 = f26;
                    f26 = c;
                }
                mag = 5;
                if ((s32)(f26 + f24 + (f27 + f25)) != 0) {
                    step = 0x200;
                }
            }
            break;
        default:
            break;
        }
        if (mag == 0) {
            /* `mag` is still 0 only when no case arm fired: that is this unit's rule-8 replacement for the
             * `block_53` edge, and the pad scan below is what retail's label reached. */
            if ((s32)self->unk5E5 != 0 || (s32)self->unk5E6 != 0) {
                u16 w = *(u16*)((u8*)self + 0xC8);
                u8 r;
                u8 c;
                if ((s32)(w & 0x3C00) != 0 && (u16)*(u16*)((u8*)self + 0xD4) >= 0x32) {
                    if ((s32)(w & 0x2000) != 0) {
                        f25 = 1;
                    } else if ((s32)(w & 0x1000) != 0) {
                        f24 = 1;
                    }
                    if ((s32)(w & 0x800) != 0) {
                        f27 = 1;
                    } else if ((s32)(w & 0x400) != 0) {
                        f26 = 1;
                    }
                    r = fn_803BECA0(self->unk008, 9);
                    if (r == 1 || r == 3) {
                        c = f25;
                        f25 = f24;
                        f24 = c;
                    }
                    if ((u8)(r + 0xFE) <= 1) {
                        c = f27;
                        f27 = f26;
                        f26 = c;
                    }
                    spC = lbl_8079A080;
                    fn_802B9740(self, &sp9, &sp8, &spC);
                    mag = 1;
                    step = 0x4C;
                    t = (s32*)lbl_805C6118;
                    while (t[0] != -1) {
                        if ((s32)*(u16*)((u8*)self + 0xD4) >= t[0]) {
                            f32 a = (f32)t[1];
                            f32 m = spC * ((f32)t[2] - a);
                            mag = (s32)(a + m);
                            f32 b = (f32)t[3];
                            f32 s = spC * ((f32)t[4] - b);
                            step = (s32)(b + s);
                            break;
                        }
                        t += 5;
                    }
                }
            }
        {
            s32 c = f25 + f24;
                if (c != 0 && (s32)(f27 + f26) != 0) {
                    step = (s32)(lbl_8079A0F0 * (f32)step);
                    mag = (s32)(lbl_8079A0F0 * (f32)mag);
                    if (mag < 1) {
                        mag = 1;
                    }
                }
                if (c != 0) {
                    dir = mag;
                    if ((s32)f25 == 0) {
                        dir = -mag;
                    }
                    self->unk5E5 = 5;
                }
                if ((s32)f27 != 0) {
                    self->unk5E5 = 0xA;
                    speed += step;
                }
                if ((s32)f26 != 0) {
                    self->unk5E5 = 0xA;
                    speed -= step;
                }
            }
        }
        break;
    }
    switch (arg2) {
    case 0:
        *(s32*)((u8*)self + 0xA8) = speed;
        fn_8027A000(self, dir);
        switch (arg1) {
        case 0x3E9:
            fn_80279EBC(self, 0x406, 0x407);
            return;
        case 0x41A:
            fn_80279EBC(self, 0x438, 0x439);
            return;
        }
        break;
    case 1:
        *(s32*)((u8*)self + 0xA8) = speed;
        fn_8027A044(self, (s16)(dir << 6));
        return;
    case 2: {
        *(s32*)((u8*)self + 0xA8) = speed;
        u16 m = *(u16*)((u8*)self + 0x38);
        u16 d = speed - m;
        if ((u32)(d - 0x4001) <= 0x3FFF) {
            *(s32*)((u8*)self + 0xA8) = m + 0x4000;
        } else if ((u32)(d - 0x8000) <= 0x3FFF) {
            *(s32*)((u8*)self + 0xA8) = m - 0x4000;
        }
        fn_8027A08C(self, (s8)dir);
        break;
    }
    }
}

/* ===== appended pass: the 13 bodies the previous pass left unwritten ==============================
 * Order below is address order of the bodies written here, not the file's; the whole file still has to
 * be sorted into .text order before the unit can link (see the header).
 * New callees these bodies reference, under the map's spellings. */
extern "C" u16 fn_803BA9B0(u8, u8, u8*, s16*, void*, void*, void*);
extern "C" void hit_flag_set__FP6_HIT_WUl(void*, u32);
extern "C" void fn_8029F204(void*, void*);
extern "C" void fn_8029F538(void*);
extern "C" void fn_8035B700(s32, s32, s32);
extern "C" void fn_8033A920(s32);
extern "C" u16 ran_suu__Fl(s32);
extern "C" void fn_8004A1F8(void*, void*);
extern "C" u32 fn_80274DCC(_PLW*, u8);
extern "C" s32 fn_8027D968(_PLW*, void*, void*, void*);
extern "C" s32 fn_8027DC90(void);
extern "C" u32 fn_802D7804(s32, f32);
extern "C" void fn_800504D4(void*);
extern "C" void fn_8008C484(void*, f32, f32, f32);
extern "C" void fn_80051574(void*, void*);
extern "C" u8 fn_80224E28(_PLW*, u8);
extern "C" void fn_8026A394(_PLW*, s32, void*);
extern "C" void fn_8026A230(_PLW*, s32, u16, s32, s32);
extern "C" void fn_8026A23C(_PLW*, s32, f32);
extern "C" f32 fn_8026A34C(_PLW*);

void mulVecMat(nw4r::math::VEC3*, nw4r::math::MTX34*);
void setVector3(nw4r::math::VEC3*, f32, f32, f32);

extern const f32 lbl_8079A0C0;
extern const f32 lbl_8079A0D4;
extern const f32 lbl_8079A0D8;
extern const f32 lbl_8079A0F8;
extern u8 lbl_805BFFCC[];
extern u8 lbl_805BAA90[];
extern u8 lbl_805BAC98[];
extern u8 lbl_805C6168[];
extern u8 lbl_805C61D4[];

/* 0x80277DAC: whether a ground-height probe along the actor's fractional facing clears the band. */
extern "C" s32 fn_80277DAC(_PLW* self, s32 arg1, f32 arg2, f32 arg3)
{
    nw4r::math::VEC3 v;
    u8 hit;
    VEC3_ctor(&v);
    v.x = lbl_8079A084;
    v.y = lbl_8079A084;
    v.z = arg2;
    rotVecY(&v, self->unk058);
    v.x = v.x + self->unk03C;
    v.y = v.y + self->unk040;
    v.z = v.z + self->unk044;
    f32 h = GetGroundHit2(&v, (u32)-2, self->unk016, &hit);
    if ((s32)hit == 0) {
        return 0;
    }
    f32 c = self->unk040 + arg3;
    if (arg1 == 0) {
        if (h < c) {
            return 1;
        }
    } else {
        if (h > c) {
            return 1;
        }
    }
    return 0;
}

/* 0x80277FF8: whether the given action may start on the current map/weapon combination. */
extern "C" s32 fn_80277FF8(u8 arg0, u8 arg1, s32 arg2)
{
    s32 t = arg2 & 0x7F;
    switch (arg0) {
    case 1:
        if (((u32)(arg1 - 2) <= 4U || (s32)(u8)arg1 == 0xB) && t != 0) {
            return 0;
        }
        break;
    case 2:
        if (((s32)(u8)arg1 == 6 || (s32)(u8)arg1 == 9 || (s32)(u8)arg1 == 0xB) && t != 0) {
            return 0;
        }
        break;
    case 3:
        if ((u32)(arg1 - 5) <= 1U) {
            if (t != 0) {
                return 0;
            }
        } else if ((s32)(u8)arg1 == 1 || (s32)(u8)arg1 == 8) {
            if ((u32)t > 1U) {
                return 0;
            }
        } else if ((s32)(u8)arg1 == 9) {
            if (t != 0) {
                return 0;
            }
        }
        break;
    case 4:
        if ((u32)(arg1 - 1) <= 5U && t != 0) {
            return 0;
        }
        break;
    case 5:
        if (((s32)(u8)arg1 == 3 || (s32)(u8)arg1 == 5 || (s32)(u8)arg1 == 8 || (s32)(u8)arg1 == 10) &&
            t != 0) {
            return 0;
        }
        break;
    case 8:
    case 9:
    case 10:
        if (t != 0) {
            return 0;
        }
        break;
    }
    return 1;
}

/* 0x80278144: whether the given action may start in the player's current map/move-work state. */
extern "C" u32 fn_80278144(u8 arg0, u8* arg1, u8 arg2)
{
    u32 m = fn_802B0668(get_now_mapno());
    if (fn_80277FF8((u8)m, arg0, arg2) == 0) {
        return 0;
    }
    /* `default:` first and `case 9:` last keeps the two bodies in retail's address order - the else arm
     * is out of line, and this is the rule-8 shape for the two labelled exits retail's source had. */
    switch ((s32)(u8)m) {
    default: {
        u8* w = get_move_work_adrs(0);
        if (w != 0 && arg0 == *(u8*)(w + 0xF6)) {
            return 0;
        }
        if ((u32)((u8)m - 6) <= 1) {
            return 0;
        }
        switch ((s32)(u8)m) {
        case 1:
            if ((s32)arg0 == 7 || (s32)arg0 == 0xB) {
                return 0;
            }
            break;
        case 2:
            if ((s32)arg0 == 0xB) {
                return 0;
            }
            break;
        case 3:
            if ((s32)arg0 == 2 || (s32)arg0 == 4 || (s32)arg0 == 8) {
                return 0;
            }
            break;
        case 5:
            if ((s32)arg0 == 0xA) {
                return 0;
            }
            break;
        case 10:
            return 0;
        }
        break;
    }
    case 9:
        if (fn_802B0688(arg1) == 1U) {
            return 0;
        }
        break;
    }
    return 1;
}

/* 0x80278310: the same map/move-work gate as 80278144, for the smaller action set. */
extern "C" u32 fn_80278310(u8 arg0, u8* arg1, u8 arg2)
{
    u32 m = fn_802B0668(get_now_mapno());
    if (fn_80277FF8((u8)m, arg0, arg2) == 0) {
        return 0;
    }
    /* `default:` first and `case 9:` last keeps the two bodies in retail's address order - the else arm
     * is out of line, and this is the rule-8 shape for the two labelled exits retail's source had. */
    switch ((s32)(u8)m) {
    default: {
        u8* w = get_move_work_adrs(0);
        if (w != 0 && arg0 == *(u8*)(w + 0xF6)) {
            return 0;
        }
        if ((u32)((u8)m - 6) <= 1) {
            return 0;
        }
        switch ((s32)(u8)m) {
        case 1:
            if ((s32)arg0 == 0xB) {
                return 0;
            }
            break;
        case 2:
            if ((s32)arg0 == 0xB) {
                return 0;
            }
            break;
        case 5:
            if ((s32)arg0 == 0xA) {
                return 0;
            }
            break;
        case 10:
            return 0;
        }
        break;
    }
    case 9:
        if (fn_802B0688(arg1) == 1U) {
            return 0;
        }
        break;
    }
    return 1;
}

/* 0x80277974: builds the attack entry an incoming hit produces and runs the shared attack set-up. */
extern "C" void fn_80277974(_PLW* self, _HIT_W* hit, u8* base, u16 idx, s32* ids, u16 flags)
{
    u8* p = (u8*)self->unk13C;
    u8* d = base + idx * 0x1A;
    *(s32*)((u8*)hit + 0x08) = ids[*(u8*)(d + 0xF)];
    fn_8029F538(hit);
    if ((flags & 1) != 0) {
        hit_flag_set__FP6_HIT_WUl(hit, 0x728);
    } else {
        hit_flag_set__FP6_HIT_WUl(hit, 0x708);
    }
    if (self->unk002 == 1) {
        hit_flag_set__FP6_HIT_WUl(hit, 0x800);
    }
    if ((flags & 0x80) != 0) {
        hit_flag_set__FP6_HIT_WUl(hit, 0x1000);
    }
    if ((flags & 0x100) != 0) {
        hit_flag_set__FP6_HIT_WUl(hit, 0x2000);
    }
    *(s16*)((u8*)hit + 0x18) = Get_motion_no(self);
    *(s16*)((u8*)hit + 0x1A) = 0;
    *(s16*)((u8*)hit + 0x1C) = 0;
    *(s16*)((u8*)hit + 0x1E) = 0;
    f32 f = *(f32*)(p + 0x48) - lbl_8079A0C0;
    if (f < lbl_8079A084 || (*(u32*)(p + 0x50) & 1) != 0) {
        f = lbl_8079A084;
    }
    f32 g = *(f32*)(p + 0x78);
    f32 t = (f32)*(s16*)(d + 0);
    f32 u = (f32)*(s16*)(d + 2);
    *(f32*)((u8*)hit + 0x38) = t;
    *(f32*)((u8*)hit + 0x3C) = u;
    if (*(s16*)(d + 0) != 0) {
        f32 x = t + (f - g);
        *(f32*)((u8*)hit + 0x38) = x;
        if (x < lbl_8079A084) {
            f32 y = u + x;
            *(f32*)((u8*)hit + 0x3C) = y;
            *(f32*)((u8*)hit + 0x38) = lbl_8079A084;
            if (y <= lbl_8079A084) {
                *(u8*)((u8*)hit + 0x05) = 0;
                return;
            }
        }
    }
    fn_8029F204(hit, d);
    Pl_attack_set_sub(self, (_HIT_DATA*)d, hit, flags);
}

/* 0x80278674: adds a signed amount to the actor's stamina pool, with the two armour-skill modifiers. */
extern "C" void fn_80278674(_PLW* self, s16 arg1, u8 arg2)
{
    s16 v = arg1;
    s32 changed = 0;
    if (Pl_master_ck(self) != 0 && fn_8026FE44(self) != 1U) {
        if (v < 0 && arg2 == 0) {
            if (Pl_Skill_ck(self, 0x15) == 1U) {
                if (v == -1 && (ran_suu__Fl(1) & 1) != 0) {
                    return;
                }
                v = (s16)((((s32)((u32)v >> 31)) + v) >> 1);
                if (v == 0) {
                    v = -1;
                }
            } else if (Pl_Skill_ck(self, 0x16) == 1U) {
                v = (s16)(v * 2);
            }
        }
        *(s16*)((u8*)self + 0x56C) += v;
        if (*(s16*)((u8*)self + 0x56C) <= 0) {
            *(s16*)((u8*)self + 0x56C) = 0;
        }
        s16 lim = *(s16*)((u8*)self + 0x56E);
        if (*(s16*)((u8*)self + 0x56C) > lim) {
            *(s16*)((u8*)self + 0x56C) = lim;
            changed = 1;
        }
        s32 m = fn_80278590(self);
        u8 old = *(u8*)((u8*)self + 0x56B);
        if ((u8)m != old) {
            if (old > (u8)m) {
                fn_8035B700(1, 4, 0);
                fn_8033A920(0x15);
            } else {
                fn_8035B700(1, 5, 0);
            }
            *(u8*)((u8*)self + 0x56B) = m;
        }
        if (changed != 0) {
            fn_8035B700(1, 6, 0);
        }
    }
}

/* 0x8027993C: the first item slot the actor may use, scanned round the 33-entry shell ring. */
extern "C" u16 fn_8027993C(_PLW* self, u16 arg1, u8 arg2)
{
    u16 slot;
    s32 i;
    if (fn_8026FE44(self) == 0) {
        return 0xFF;
    }
    u8 ring[33 * 4];
    for (i = 0; i < 0x18; i++) {
        fn_8004A1F8(ring + i * 4, (u8*)self + 0x278 + i * 4);
    }
    for (i = 0; i < 9; i++) {
        s16 j = (s16)i;
        fn_8004A1F8(ring + (j + 0x18) * 4, (u8*)self + 0x278 + (j + 0x1A) * 4);
    }
    slot = arg1;
    if ((slot & 0x80) != 0) {
        slot = (u16)((slot & 0x7F) + 0x18);
    }
    if ((s32)slot >= 0x21) {
        slot = 0;
    }
    switch (arg2) {
    case 0:
        slot = (u16)((slot + 1) % 33);
        break;
    case 1:
        slot = (u16)((slot == 0) ? 32 : slot - 1);
        break;
    }
    for (i = 0; i < 0x21; i++) {
        u8* e = ring + slot * 4;
        if (*(u16*)(e + 0) != 0 && *(s16*)(e + 2) > 0) {
            u8* it = (u8*)GetItemData(*(u16*)(e + 0));
            if ((it[2] & 8) != 0 && it[0] == 1 &&
                fn_80274DCC(self, (u8)fn_80274B20(*(u16*)(e + 0))) == 1U) {
                if ((u32)slot >= 0x18U) {
                    slot = (u16)((slot - 0x18) | 0x80);
                }
                return slot;
            }
        }
        switch (arg2) {
        case 0:
        case 2:
            slot = (u16)((slot + 1) % 33);
            break;
        case 1:
        case 3:
            slot = (u16)((slot == 0) ? 32 : slot - 1);
            break;
        }
    }
    return 0xFF;
}

/* 0x80279C20: refreshes the actor's held-shell state each frame - the ring position, the shell search,
 * and the save/restore of the fields it rewrites. */
extern "C" void fn_80279C20(_PLW* self)
{
    if (Pl_master_ck(self) == 0) {
        return;
    }
    if (fn_8026FE44(self) == 0) {
        return;
    }
    if (fn_8027BC48(1) == 1U) {
        return;
    }
    if (Pl_bari_ck(self, 1) == 1U) {
        return;
    }
    u8 a = self->unk00A;
    if ((u32)(a - 8) <= 1U) {
        return;
    }
    switch (a) {
    case 6: {
        s32 id = self->unk00C;
        if (id == 0x1B) {
            return;
        }
        if (id == 0x23) {
            return;
        }
        break;
    }
    case 4: {
        s32 id = self->unk00C;
        if (id >= 0x17) {
            if (id >= 0x32) {
                break;
            }
            if (id >= 0x1A) {
                return;
            }
        } else {
            if (id >= 0x0E) {
                if (id >= 0x13) {
                    return;
                }
                break;
            }
            if (id < 5) {
                break;
            }
        }
        if (*(u8*)((u8*)self + 5) <= 2) {
            return;
        }
        break;
    }
    }
    if (*(u8*)((u8*)self + 0x26B) == 0) {
        *(u8*)((u8*)self + 0x26B) = 1;
        *(u8*)((u8*)self + 0x26D) = self->unk26C;
        *(s16*)((u8*)self + 0x272) = *(s16*)((u8*)self + 0x270);
        *(u8*)((u8*)self + 0x275) = *(u8*)((u8*)self + 0x26A);
        *(u8*)((u8*)self + 0x274) = self->unk269;
    }
    if (fn_8026A6F4(self, 0xB) == 1U) {
        if (fn_8026A6F4(self, 0xD) == 1U) {
            if (self->unk26E == 0xFF) {
                self->unk26E = fn_8027993C(self, 0, 3);
            } else {
                self->unk26E = fn_8027993C(self, 1, 0);
            }
            fn_80279B84(self);
            if (self->unk26E != 0xFF) {
                *(u8*)((u8*)self + 0x309) = (u8)(*(u8*)((u8*)self + 0x309) | 8);
                sysSE_req(6);
            }
        } else if (fn_8026A6F4(self, 0xC) == 1U) {
            if (self->unk26E == 0xFF) {
                self->unk26E = fn_8027993C(self, 0, 2);
            } else {
                self->unk26E = fn_8027993C(self, 0, 0);
            }
            fn_80279B84(self);
            if (self->unk26E != 0xFF) {
                *(u8*)((u8*)self + 0x309) = (u8)(*(u8*)((u8*)self + 0x309) | 0x10);
                sysSE_req(6);
            }
        }
    } else if (self->unk26B != 0) {
        self->unk26B = 0;
        if (self->unk26D != self->unk26C) {
            fn_80279B84(self);
        } else {
            *(s16*)((u8*)self + 0x270) = *(s16*)((u8*)self + 0x272);
            *(u8*)((u8*)self + 0x26A) = *(u8*)((u8*)self + 0x275);
            self->unk269 = *(u8*)((u8*)self + 0x274);
        }
    }
    if (self->unk26D == self->unk26C) {
        *(s16*)((u8*)self + 0x270) = *(s16*)((u8*)self + 0x272);
        *(u8*)((u8*)self + 0x26A) = *(u8*)((u8*)self + 0x275);
        self->unk269 = *(u8*)((u8*)self + 0x274);
    }
}

/* 0x80279EBC: starts the shell-swap motion, scaled by the actor's stored shell count. */
extern "C" void fn_80279EBC(_PLW* self, u16 arg1, u16 arg2)
{
    s32 v = *(s8*)((u8*)self + 0x64F);
    if (v == 0) {
        fn_8026A23C(self, 0, lbl_8079A080);
        fn_8026A23C(self, 1, lbl_8079A084);
        return;
    }
    f32 f = (f32)v;
    *(u8*)((u8*)self + 0x64E) = 1;
    u16 m;
    if (v > 0) {
        if (arg1 == 0x581) {
            m = 0x580;
            *(u8*)((u8*)self + 0x64E) = 0;
        } else {
            m = arg1;
        }
    } else {
        if (arg2 == 0x582) {
            m = 0x580;
            *(u8*)((u8*)self + 0x64E) = 0;
        } else {
            m = arg2;
        }
        f = f * lbl_8079A0D4;
    }
    fn_8026A230(self, 1, m, 0, (s32)(lbl_8079A0C0 + fn_8026A34C(self)));
    f32 t = f * lbl_8079A0D8;
    fn_8026A23C(self, 0, lbl_8079A080 - t);
    fn_8026A23C(self, 1, t);
}

/* 0x8027A340: whether the actor may act at all this frame - master/bari state, the stun flag and the
 * per-action-class exceptions. */
extern "C" s32 fn_8027A340(_PLW* self)
{
    s32 ok = 1;
    if (Pl_master_ck(self) == 0) {
        ok = 0;
    }
    if (fn_8026FE44(self) == 0) {
        ok = 0;
    }
    if (self->unk5E6 != 0) {
        ok = 0;
    }
    if (fn_8026A644(self, 0x1D) == 0) {
        if (self->unk00A == 4) {
            s32 id = self->unk00C;
            if (id != 0x13 && id != 0x2A && id != 0x2E && id != 0x15 && id != 0x2C && id != 0x30) {
                ok = 0;
            }
        } else {
            ok = 0;
        }
    }
    u8 a = *(u8*)((u8*)self + 0x00A);
    if ((u32)(a - 5) > 6U) {
        switch (a) {
        case 0: {
            s32 id = self->unk00C;
            switch (id) {
            case 1:
            case 2:
            case 5:
            case 6:
            case 7:
            case 20:
            case 21:
            case 28:
            case 73:
            case 91:
            case 122:
            case 158:
            case 159:
            case 160:
            case 161:
                ok = 0;
                break;
            }
            break;
        }
        case 4: {
            s32 id = self->unk00C;
            if ((u32)(id - 2) <= 1U || (u32)(id - 0x10) <= 1U) {
                ok = 0;
            }
            break;
        }
        case 2:
            ok = 0;
            break;
        }
    } else {
        ok = 0;
    }
    return ok;
}

/* 0x8027AC2C: sets the two stored shell ids (or their per-weapon-class defaults) and scales them by
 * the actor's ammo skill. */
extern "C" void fn_8027AC2C(_PLW* self, u8 arg1, u8 arg2)
{
    if (arg1 == 0xFF) {
        u8 t = self->unk002;
        *(u8*)((u8*)self + 0x568) = lbl_805BFFCC[t * 2];
        if (t == 7 && Pl_condition_ck(self, 0x80000000) == 1U) {
            *(u8*)((u8*)self + 0x568) = 0x71;
        }
    } else {
        *(u8*)((u8*)self + 0x568) = arg1;
    }
    if (arg2 == 0xFF) {
        u8 t = self->unk002;
        *(u8*)((u8*)self + 0x569) = lbl_805BFFCC[t * 2 + 1];
    } else {
        *(u8*)((u8*)self + 0x569) = arg2;
    }
    f32 f = (f32)((s8)*(u8*)((u8*)self + 0x452) + 100) / lbl_8079A0D0;
    if (Pl_cat_skill_ck(self, 0x26) == 1U) {
        s32 n = *(u8*)((u8*)self + 0x445);
        if (n != 0) {
            s16 i = 0;
            if (n > 0) {
                for (; i < n; i++) {
                    f = f * lbl_8079A0AC;
                }
            }
        }
    }
    *(u8*)((u8*)self + 0x568) = (u8)(s32)((f32)(*(u8*)((u8*)self + 0x568)) * f);
    *(u8*)((u8*)self + 0x569) = (u8)(s32)((f32)(*(u8*)((u8*)self + 0x569)) * f);
}

/* 0x8027C064: builds the impact vector an attacking part starts from - the per-weapon-class motion, the
 * hit matrix and the actor's world offset. */
extern "C" void fn_8027C064(_PLW* self, nw4r::math::VEC3* out)
{
    nw4r::math::VEC3 v;
    u8 buf[12];
    nw4r::math::MTX34 m;
    nw4r::math::MTX34 m2;
    s32 part;
    MTX34_ctor(&m);
    MTX34_ctor(&m2);
    VEC3_ctor(&v);
    u8 t = fn_80224E28(self, lbl_805BAA90[self->unk002]);
    out->x = lbl_8079A084;
    out->y = lbl_8079A084;
    setVector3(&v, lbl_8079A084, lbl_8079A084, lbl_8079A084);
    switch ((s32)t) {
    default:
        part = *(s32*)(lbl_805C6168 + (self->unk002 * 3 + 2) * 4);
        out->z = lbl_8079A084;
        break;
    case 1:
        if (self->unk002 == 3) {
            copyVec3(&v, (const nw4r::math::VEC3*)fn_80143174(buf, lbl_805BAC98 + self->unk002 * 0x18 + 0xC,
                                                              self->unk002 * 0x18));
        }
        /* fall through */
    case 0:
        part = *(s32*)(lbl_805C6168 + (t + self->unk002 * 3) * 4);
        out->z = *(f32*)(lbl_805C61D4 + self->unk002 * 4);
        break;
    }
    fn_8026A394(self, part, &m);
    fn_800504D4(&m2);
    fn_8008C484(&m2, v.x, v.y, v.z);
    fn_80051574(&m, &m2);
    mulVecMat(out, &m);
    out->x = out->x + m.m[0][3];
    out->y = out->y + m.m[1][3];
    out->z = out->z + m.m[2][3];
}

/* 0x8027C208: the action-class gate - whether the actor may start the given action id. */
extern "C" s32 fn_8027C208(_PLW* self, u16 arg1)
{
    nw4r::math::VEC3 sp24;
    s32 sp18;
    s32 sp14;
    s32 sp10;
    s32 spC;
    s16 sp8;
    VEC3_ctor(&sp24);
    if (self->unk00A == 7) {
        if (arg1 == 0x15B) {
            if ((u32)self->unk00C <= 1U) {
                return 0;
            }
        } else {
            return 0;
        }
    }
    if ((Pl_act_ck(self, 6, 0x1C) == 1U || Pl_act_ck(self, 6, 0x1D) == 1U) && arg1 != 0x176) {
        return 0;
    }
    if ((fn_80278C7C(self) == 1U || fn_80278CD0(self) == 1U) && arg1 != 0x177) {
        return 0;
    }
    switch ((s32)arg1) {
    case 0x177:
        if (fn_80278C7C(self) == 1U || fn_80278CD0(self) == 1U) {
            return 1;
        }
        break;
    case 3:
        if (self->unk009 == 3) {
            return 0;
        }
        if (*(u8*)((u8*)self + 0x585) != 0) {
            return 0;
        }
        return fn_802782B8(self);
    case 4:
    case 0x31:
        if (*(u8*)((u8*)self + 0x585) != 0) {
            return 0;
        }
        return fn_80278450(self);
    case 0x22:
    case 0x23:
    case 0x24:
        if (fn_803BA9B0(self->unk008, self->unk016, (u8*)self + 0x3C, &sp8, &sp24, &sp18, &sp14) != 0xFFFFU &&
            sp8 == 3) {
            return 1;
        }
        break;
    case 0x25:
    case 0x26:
    case 0x27:
        if (self->unk009 == 3) {
            return 0;
        }
        if (fn_803BA9B0(self->unk008, self->unk016, (u8*)self + 0x3C, &sp8, &sp24, &sp18, &sp14) != 0xFFFFU &&
            sp8 == 4) {
            return 1;
        }
        break;
    case 0xCA:
    case 0xCB:
    case 0xCC:
    case 0xCD:
    case 0xCE:
    case 0x15F:
    case 0x161:
    case 0x163:
    case 0x185:
    case 0x186:
        if (self->unk009 == 3) {
            return 0;
        }
        if (fn_803BA9B0(self->unk008, self->unk016, (u8*)self + 0x3C, &sp8, &sp24, &sp18, &sp14) != 0xFFFFU &&
            sp8 == 5) {
            return 1;
        }
        break;
    case 0x159:
    case 0x15A:
    case 0x15B:
        if (*(u8*)((u8*)self + 0x585) != 0) {
            return 0;
        }
        if (self->unk009 != 3) {
            return 1;
        }
        break;
    case 0x1A:
    case 0x1B:
    case 0x1C:
    case 0xA6:
    case 0xA7:
    case 0xD0:
    case 0xD6:
        if (self->unk009 != 3) {
            return 1;
        }
        break;
    case 0x1D:
    case 0x1E:
    case 0x1F:
    case 0x20:
        if (*(u8*)((u8*)self + 0x585) != 0) {
            return 0;
        }
        if (self->unk009 == 3) {
            return 0;
        }
        return fn_802784A8(self);
    case 0x16E:
        return fn_802784B8(self);
    case 6:
    case 0x2E:
    case 0x8A:
    case 0xC2:
    case 0x184:
        if (self->unk009 == 3) {
            return 1;
        }
        break;
    case 0x28:
    case 0x2F:
    case 0x109:
        if (*(u8*)((u8*)self + 0x585) != 0) {
            return 0;
        }
        if (fn_8027CC2C(self) == 1U) {
            return 0;
        }
        if (fn_802731B4(self, 0x1D) > 0 && self->unk009 != 3) {
            return 1;
        }
        break;
    case 0x1B6:
        if (self->unk009 == 3) {
            return 0;
        }
        if (fn_8027CC2C(self) == 1U) {
            return 0;
        }
        if (fn_802731B4(self, 0x1D) > 0 && fn_802D7804(4, lbl_8079A0F8) == 1U) {
            return 1;
        }
        break;
    case 0x169:
    case 0x16A:
    case 0x16B:
    case 0x16C:
    case 0x16D:
        if (self->unk009 != 3) {
            return 1;
        }
        break;
    case 0x2B:
        if (self->unk009 != 3) {
            return 1;
        }
        break;
    case 0x17F:
        if (*(u8*)((u8*)self + 0x585) != 0) {
            return 1;
        }
        break;
    case 2:
    case 0x34:
    case 0x246:
        if (*(u8*)((u8*)self + 0x585) != 0) {
            return 0;
        }
        return fn_80277F54(self);
    case 0x108:
        if (self->unk009 == 3) {
            return 0;
        }
    case 1:
    case 0x237:
    case 0x258:
    case 0x259:
        if (*(u8*)((u8*)self + 0x585) == 0) {
            return 1;
        }
        break;
    case 0x30:
    case 0x62:
    case 0xCF:
        if (fn_8026FE44(self) == 0) {
            return 1;
        }
        break;
    case 0x180:
        if (fn_8027DC90() == 0) {
            return 0;
        }
    case 0x17C:
        if (fn_8027D968(self, &sp24, &sp10, &spC) == 1) {
            return 1;
        }
        break;
    case 0x247:
        if (fn_8027D968(self, &sp24, &sp10, &spC) == 5) {
            return 1;
        }
        break;
    default:
        return 1;
    }
    return 0;
}
