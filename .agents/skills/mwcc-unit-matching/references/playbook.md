<!-- GENERATED FILE - do not edit.
     Source: docs/matching.md
     Regenerate: python .agents/skills/mwcc-unit-matching/scripts/sync_reference.py
-->

# Matching compiler flags

A playbook for pinning down the compiler flags of one translation unit, so its compiled object matches the
original object instruction for instruction. Flags are the *second* thing to look at (source shape is the
first), but when the target's code is systematically "less optimized" than ours they are usually the whole
story.

Two rules from `AGENTS.md` apply throughout: a flag change is only acceptable with concrete evidence
(non-negotiable #3), and proven flags belong in `configure.py` as a **per-library** `cflags_*` override -
never by editing `cflags_base`/`cflags_runtime` for everybody.

## The loop

1. **Measure one unit** at instruction level (not the project-wide progress report).
2. **Read the first divergence** and name the *code shape* it implies (a fused instruction, a register save
   idiom, a table base, a prologue).
3. **Classify it**: if the shape is something an optimizer pass or a code-generation switch can produce, it
   is a flag lead; if it is an algorithm or signature difference, it is a source lead.
4. **Isolate**: change exactly one flag, against the *real* command line for that unit.
5. **Verify and land it**: function sizes, per-function match, relocations still resolve, and a metric
   that does not drift. A flag win goes into a per-library `cflags_*` override in `configure.py`, and a
   source win is applied to the unit's own source (`tools/flags/tryvar.py --apply <name>`) and re-measured
   there - the repository must build better, not just the probe. Prove it with the object hash.

The tricks below are the individual moves; the worked example at the end is the whole loop run once.
They are indexed in `AGENTS.md` ("Matching playbook") together with the problem each one solves and a
status column that doubles as the todo list for whatever unit is being worked on - a successful new idea
becomes a section here in the same style.

## Toolbox

All of them are unit-agnostic: pass `-u <unit>` (or omit it when the repo has exactly one unit with
source), where `<unit>` is any of `Lib/file`, `main/Lib/file`, `src/Lib/file.c` or the object path.
`tools/unitutil.py` is the shared layer - unit resolution, the real ninja command line, flag overriding,
the ELF reader - and `tools/unitutil.py` with no arguments lists the units it can work on.

| tool | what it gives you |
| --- | --- |
| `tools/flags/frame.py -u <unit> [--flags-extra "..."] [--versions ...]` | per-function prologue frame size and length next to the target's, no objdiff needed |
| `tools/flags/mwcc_matrix.py -u <unit> [--flags-extra "..."] [versions...]` | compiles with the **exact ninja command line** (plus overrides / other compiler versions), per-function summary in `build/tmp/matrix/summary.txt` |
| `tools/flags/optsweep.py -u <unit> [subs...]` | fast `-opt` keyword filter: frames per candidate, target frames included |
| `tools/flags/tryvar.py -u <unit> [--variants f.py] [names...]` | source-rewrite harness: compile a modified copy of the unit's source, report the per-function diff |
| `tools/objdiff/symdiff.py -u <unit> <symbol> [n] [--all]` | side-by-side target/ours instruction listing with `diff_kind` per row |
| `tools/objdiff/slotmap.py -u <unit> <symbol> [--map] [--slot ...]` | r1-relative stack-slot map and per-slot access timeline (for "one extra local" diffs) |
| `tools/elf/elfsect.py -u <unit>` (or `<obj> ...`) | section table (`.text`, `.rodata`, `extab`, `.comment`, ...) |
| `tools/elf/dwarfmap.py <obj> <func>` | local-variable -> stack-slot map from `-gdwarf-2` debug info |
| `build/tools/objdiff-cli.exe diff -p . -u <unit> <symbol> ...` | the raw instrument; needs the `<symbol>` argument for symbol-level data |
| `build/tmp/ref/mwcc_help.txt` | the compiler's own `-help` output (option semantics) |

## 1. Use a per-unit instrument, not the project-wide check

**Problem.** The project-level pass/fail signal is useless while any object is `NonMatching`:
`ninja build/RMHE08/ok` cannot pass, and the progress report's `complete_code_percent` says 100 % even when
the code is wrong. A unit can be at 1 % fuzzy with half its functions at 0 % and still look "complete".

**Why try it.** Nothing can be measured, so nothing can be improved. A single-unit, instruction-level diff
is the smallest signal that actually moves when a flag changes.

**Result.** `objdiff-cli diff -p . -u <unit> <symbol>` plus a side-by-side printer becomes the instrument:
per-symbol `match_percent`, size, and the aligned instruction stream are enough to attribute every byte of
a mismatch. Note that the pinned objdiff-cli only emits symbol/instruction data when a symbol argument is
given; without it you get a section-level diff and a silently empty symbol list.

**Example**

```sh
python tools/objdiff/symdiff.py -u <unit> <symbol> 40      # runs objdiff itself, then prints
python tools/flags/frame.py -u <unit>                      # frames only, much faster
```

## 2. Read the *first divergence*, never the percentage

**Problem.** `match_percent` is positional: one inserted or deleted instruction shifts every following
instruction, so a function that is a single instruction away from a match reports exactly the same ~0 % as
a function that is entirely wrong.

**Why try it.** A 0 % that really means "one extra instruction in the prologue" sends you hunting in the
wrong place, and it is the single most common way to waste a day on a decomp.

**Result.** Make every decision from "first diff @N `<instruction>`": the instruction names the code shape,
which is what tells you whether to look at flags or at source.

**Example**

```
camellia_encrypt128  target 3228  ours 3228  100.00%  IDENTICAL
camellia_setup256    target 4860  ours 4860   99.83%  first-diff@0 stwu r1, -0x1d0(r1)

# other first divergences from the same unit, each naming a different code shape:
#   @12  extrwi r0, r10, 8, 16      -> index computation is fused (a peephole/level artefact)
#   @67  srwi r0, r0, 16            -> byte extraction is not fused
#   @1   mflr r0                    -> prologue/register-save idiom
```

## 3. Read the target's disassembly, not just the diff

**Problem.** The diff shows *what* differs, not *what the original source looked like*. Several flags are
only discoverable if you already know the target's code shape.

**Why try it.** Every split unit ships a full disassembly of the target, which is the cheapest way to see
prologues, tail calls, unrolling and table addressing - and a prologue is often a flag fingerprint.

**Result.** Whole flag families can be read off the target's prologue and addressing: whether it uses
`stmw`/`lmw` or the EABI save helpers, whether it materialises each table with its own `lis`+`addi` pair or
shares one base, and whether index arithmetic is fused.

**Example**

```
target: stwu r1,-0x140(r1); mflr r0; stw r0,0x144(r1); addi r11,r1,0x140; bl _savegpr_14
ours:   stwu r1,-0x140(r1); stmw r14,0xf8(r1)
```

## 4. Ignore version/`.comment` hints; make codegen the oracle

**Problem.** A unit's `.comment` section and `mw_comment_version` in `config.yml` look like a compiler
fingerprint and invite "we must be using the wrong compiler version".

**Why try it.** In a decomp-toolkit project the target object's `.comment` is *synthesized* from that
config value, not emitted by the original compiler, so it is not evidence about the original toolchain at
all.

**Result.** The cheapest way to close the question is to compile the unit with every available compiler and
diff each result. If the matrix is flat, version is not the lever and all attention can go to flags and
source. (A version matrix is also a good smoke test for the harness - see trick 6.)

**Example**

```sh
python tools/flags/mwcc_matrix.py -u <unit> 0x4201_127 1.0RC1 1.0a 1.0 1.1 1.3 1.5 1.6 1.7
# flat for the Camellia unit: identical sizes, match % and first divergence for all nine
```

## 5. Sweep one flag at a time, against the project's own command line

**Problem.** Several flags are usually in play at once (`-O4,p`, `-inline auto`, `-use_lmw_stmw on`,
`-Cpp_exceptions off`, `-str ...,pool,...`), and it is unclear which single one explains which symptom.

**Why try it.** A hand-written compiler command drifts from what ninja actually runs, so results stop being
reproducible. Overriding one flag in the real command line is the only change that means anything.

**Result.** One flag per run, ordered by how cheap the symptom is to check, and record the effect on
sizes and per-function match. Cumulative overrides build the final set; a flag that moves nothing is
dropped immediately.

**Example**

```sh
python tools/flags/mwcc_matrix.py -u <unit> --flags-extra "-O3 -inline noauto" 1.3
# then add one flag at a time: -opt nopeephole, -pool off, -use_lmw_stmw off, ...
```

## 6. Guard against stale objects when scripting the compiler

**Problem.** A scripted matrix run can report all compilers producing *identical* output - which cannot be
true. Every run was diffing the same stale object, one the compiler never wrote.

**Why try it.** Results that are too clean are a bug report about the harness, not a finding about the
compiler. MWCC's `-o` takes an output **directory** (ninja passes `-o build/RMHE08/src/<Unit>`); given a
file-like path it silently writes an extension-less file and leaves the real object untouched.

**Result.** Delete the object before each compile, wait long enough that the filesystem's whole-second
mtimes differ (objdiff caches on `(mtime, size)`), and fail loudly when the object was not regenerated. Any
number produced before that fix is worthless. The same trap has a second form: a scratch or debug build that
keeps the unit's own `-o` directory **overwrites the unit object** (a `-gdwarf-2` build has the same `.text`
but a different hash), so always redirect `-o` when compiling anything that is not the unit build, and
restore with `rm -f <obj> && ninja <obj>` afterwards.

**Example**

```sh
# -o is a directory base, not a file
sjiswrap.exe mwcceppc.exe <flags> -c src/<Unit>/<file>.c -o build/RMHE08/src/<Unit>
```

## 7. Use scratch files to attribute an instruction choice

**Problem.** Two candidate source idioms produce visibly different instructions, and the full-unit diff
cannot tell you whether the difference comes from the source idiom or from an optimizer pass.

**Why try it.** A ten-line file compiled both ways answers that in seconds, and when one form flips the
output you know which pass is responsible - which is exactly the information a flag hunt needs.

**Result.** Scratch experiments attribute a symptom to a pass, and then to the flag that controls it. Two
examples that each pinned a flag: a cast-versus-explicit-mask byte extraction (the peephole pass fuses one
form and not the other), and a table lookup (data pooling decides whether the four tables share one `lis`
base).

**Example**

```sh
# (u8)(x >> 16) -> extrwi ;  (x >> 16) & 0xff -> srwi + clrlwi
mwcceppc.exe -O3 -opt nopeephole -c build/tmp/scratch/mask.c -o build/tmp/scratch
# -pool off -> 4 separate lis pairs, default -> 1 shared base
grep -c lis build/tmp/scratch/tab.s
```

## 8. Ask the compiler which optimizations are actually on

**Problem.** After a long sweep it is still unclear what a flag *set* resolves to: `-O3` and
`-opt level=3,peephole` are the same thing, and one `-O` level can set five switches at once.

**Why try it.** The compiler can print its own effective configuration, which turns a guess about macro
expansion into a fact and prevents "is scheduling on?" debates.

**Result.** `-opt display` prints the resolved option set, which is the check to run before declaring a
flag set "the" answer.

**Example**

```sh
mwcceppc.exe <candidate flags> -opt display -c src/<Unit>/<file>.c -o build/tmp/probe
# - global optimizer level 3
# - peephole optimizations off
# - no instruction scheduling
```

## 9. Enumerate the option space from the compiler, not from memory

**Problem.** Guessed spellings waste runs, and worse, they lie: some invented keywords are **silently
accepted and ignored**, so "no effect" looks like evidence when it is noise. Others do not parse at all
(an `-O` level cannot carry `-opt` keywords).

**Why try it.** The compiler ships an authoritative option list, and anything not on it cannot be the
answer - that closes the search space instead of extending it.

**Result.** Dump `-help` into the repo's scratch area, sweep only keywords that exist, and record the
invalid spellings as no-ops so nobody re-runs them.

**Example**

```sh
build/compilers/Wii/1.3/mwcceppc.exe -help > build/tmp/ref/mwcc_help.txt   # then grep for '-opt'
python tools/flags/optsweep.py -u <unit>                                    # filter the whole axis
```

## 10. Do not use frame size as a success signal

**Problem.** When the last remaining diff is a stack-frame size, it is tempting to accept any flag variant
that produces the target's frame - and several do.

**Why try it.** A frame-size hit looks like a precise binary signal, which is exactly why it needs a second
check before it is believed.

**Result.** Frame size is an alignment artifact: locals are rounded up to the next 16 bytes, so a variant
can hit the target's frame while emitting completely wrong code. Always confirm with per-function match
percentages and function sizes; frame size alone is never the answer.

**Example**

```
-opt size   -> target frame -0x1d0 but 35% of the code matching   (not the answer)
baseline    -> frame -0x1e0, 82.72% (older metric) / 99.83% (v3.6.1)
```

## 11. Do not read a large size gap as "different source"

**Problem.** The target object is often hundreds of bytes bigger than ours, and several functions sit at
0 %, which reads like the original source containing code ours does not have (different unrolling, extra
rounds, extra statements).

**Why try it.** Size gaps are the most misleading signal in a decomp: aggressive optimizer flags *remove*
instructions, so "the target is bigger" can mean nothing more than "our flags are too aggressive".

**Result.** Check the flag hypothesis before rewriting code. In the worked example the entire 2820-byte gap
was flag-caused (peephole fusion, pooled table bases, `stmw`), and with the final flags every function size
matched exactly. A source rewrite at that point would have been wasted work on correct code.

**Example**

```
baseline  -O4,p -inline auto -use_lmw_stmw on    -> .text 21600 B, 6/10 functions at 0%
final     -O3 -inline noauto -opt nopeephole ... -> .text 24420 B, 9/10 byte-identical
```

## 12. Check that the flag's side effects still link

**Problem.** Some flags change the *relocations*, not just the instructions: a save idiom that calls
runtime helpers, or a table base that changes how a symbol is addressed. Matching the code shape while
referencing a symbol the original build never had is a false positive.

**Why try it.** Flags that change relocations have a second acceptance criterion - the referenced symbol
must exist in the target's symbol table - and it is a one-line check.

**Result.** Confirm the symbol exists (`symbols.txt`) or that the relocation shape matches (each table gets
its own `@ha`/`@l` pair). This is also the check that tells you a "flag win" is real rather than cosmetic.

**Example**

```sh
grep -n "_savegpr_14" config/RMHE08/symbols.txt
# 38292:_savegpr_14 = .text:0x80456DD4; // type:label scope:global
```

## 13. Stop when the remaining diff is no longer flag-shaped

**Problem.** After the flags that clearly apply are found, a near-miss variant often remains: a flag
combination that fixes one visible symptom (a frame size, a single instruction) while leaving the code
different.

**Why try it.** A near-miss deserves one look at *what* it changes, because that distinguishes "a flag I
have not found" from "the wrong flag that happens to fix one symptom".

**Result.** Inspect what the near-miss variant does to the instruction stream. If it reorders instructions
the target does not reorder, the flag is wrong and the residual belongs to source shape or liveness - stop
flag hunting there and document the residual instead (see the matching policy in `AGENTS.md`). Document it
**once, in the unit's file header comment** - not as a comment on the function it concerns: a function
comment is a short description of what the function does, never its symbol name, its match percentage or a
residual (see `AGENTS.md` -> Conventions).

**Example**

```
-opt nopeephole,level=4 -> target frame -0x1d0, but two independent XORs get hoisted
                           (first-diff@958) -> the target's level is 3, not 4
```

## 14. Prove the committed flag list reproduces the proven object

**Problem.** The flags eventually get written into `configure.py` as a per-library override. A
hand-written list can silently differ from the command line that was tested - a leftover `-O4,p`, a
duplicated `-inline auto`, a flag appended after a conflicting default.

**Why try it.** What matters is that the *repository* builds the proven object, not that some script did.

**Result.** Compile the unit once through the scripted override path and once through `ninja`, and compare
the object hashes. The override should *remove* the conflicting defaults rather than append after them, so
the command line contains exactly one `-O` / `-inline` / etc.

**Example**

```sh
python tools/flags/mwcc_matrix.py -u <unit> --flags-extra "<final flags>" 1.3
sha1sum build/RMHE08/src/<Unit>/<file>.o
rm -f build/RMHE08/src/<Unit>/<file>.o && ninja build/RMHE08/src/<Unit>/<file>.o
sha1sum build/RMHE08/src/<Unit>/<file>.o        # must be the same hash
```

## 15. Pin a metric that does not drift

**Problem.** Per-symbol fuzzy percentages change between tool versions: the same two objects can score
82.72 % with one `objdiff-cli` build and 99.83 % with another. A decision made against one number is
meaningless against the other.

**Why try it.** Any progress metric that depends on the tool version will eventually "improve" or "regress"
without a single byte of code changing.

**Result.** Decide on version-independent facts: **`matched_functions X/Y`** from the project report, exact
function sizes, and "first divergence @N". Record the fuzzy percentage as context only, with the tool
version next to it.

**Example**

```sh
ninja build/RMHE08/report.json
# main/Camellia/camellia: matched_functions 9/10, matched_code 80.10%, fuzzy 99.9656%
```

## 16. Scope optimizer settings per function with pragmas

**Problem.** The optimizer levers are command-line flags, i.e. global: `-opt level=4` fixes one function's
frame but reorders instructions elsewhere, so a per-function codegen difference looks unreachable from the
source side.

**Why try it.** MWCC pragmas apply from the point where they appear onward, so a pragma *pair* scopes a
setting to a single function - a restore pragma before the next function puts the defaults back. That makes
every global `-opt` lever testable per function, and it is the only way to ask "would this function's
allocation match under a different optimizer setting, without disturbing the rest of the unit?".

**Result.** `#pragma optimization_level 4` immediately before the function, with
`optimization_level 3` / `peephole off` / `scheduling off` before the next one, moved
`camellia_setup256`'s frame from `-0x1e0` to the target's `-0x1d0` while the other nine functions stayed
byte-identical. It also proved the residual is *not* scheduling (level 4 + `scheduling off` is identical to
level 4 alone) and left exactly one 14-instruction window different: 12 rows, a register choice plus where
`CAMELLIA_RL1` is computed. Pragmas that demonstrably change codegen here: `peephole`, `scheduling`,
`optimization_level`, `opt_common_subs`, `opt_propagation`, `opt_lifetimes`. Pragmas that did nothing:
`opt_dead_code`, `opt_dead_store`, `opt_strength_reduction`, `opt_loop_invariants`, `opt_cse`, `opt_global`,
`opt_space`.

**Example**

```c
#pragma optimization_level 4
void camellia_setup256(const unsigned char *key, u32 *subkey) { ... }

#pragma optimization_level 3
#pragma peephole off
#pragma scheduling off
void camellia_setup192(const unsigned char *key, u32 *subkey) { ... }
```

```sh
python tools/flags/tryvar.py -u <unit> v26_pragma_opt4   # frame -0x1d0, 99.32 % (only the window differs)
```

**A pragma is per *function*, not per region** (measured on the same unit, 2026-09-23). The scope looks
like it starts where the pragma is written, but `optimization_level` and the `opt_*` passes are read once
per function: inserting `#pragma optimization_level 3` (or `4`) mid-function changes nothing at all, and the
same is true of `opt_common_subs off` / `opt_propagation off` / `opt_lifetimes off` - 18 marker positions x
both directions and 36 regional `opt_*` variants all produced results byte-identical to the same pragma at
the function head. So "level 4 for the part that needs the frame, level 3 for the part that needs the
order" is not available: a pragma pair scopes a setting to **one whole function**, and two levers that
pull in opposite directions inside one function cannot be separated this way. The window above is also
worth re-locating before rewriting it - the recorded `v40`-`v44` probes rewrote the *first* `tl`/`tr` group
of the function (`sub256(..., count=1)` matches only the first occurrence), while the reorder was in the
*fifth*; the object's own MWCC `.line` section (`u32 size`, then 10-byte `{u32 addr, u32 line, u16 flags}`
records - note `addr2line` cannot read it) is what maps a diff row to a source line. Rewriting the right
group and reusing the rotated value's own variable (`dw = CAMELLIA_RL1(dw);`, which stops the global
optimizer folding the next XOR operand into the tree) took that window from 14 rows to 9.

## 17. Cross-family version matrix: a unit's compiler is per unit, not per project

**Problem.** `mw_version` is set once per library in `configure.py` and every unit inherits the same one
(the template default here is `Wii/1.3`). Prebuilt SDK libraries in particular were compiled by Nintendo
with whatever compiler the SDK shipped with, so a unit can legitimately come from a *different* toolchain -
and then no flag or source change will ever close its residual.

**Why try it.** The target object's `.comment` cannot answer it: the retail DOL has no `.comment` section at
all (`grep -c -a CodeWarrior orig/RMHE08/sys/main.dol` -> `0`), so the comment in a split object is
synthesized from `config.yml`'s `mw_comment_version` and says nothing about the original build (trick 4).
Codegen is the oracle, so compile the unit with every installed compiler *across families* and diff each
result: a single function whose instruction count differs is enough to separate them.

**Result.** For `RSO/runtime` the matrix is flat *within* each family but separates the families on exactly
one function - every other reconstructed function is byte-identical under all of them:

| compiler | `fn_804DA7E4` | `fn_804DA834` | `fn_804DAA24` |
| --- | --- | --- | --- |
| GC 1.0 - 1.2.5n | 69.5 % | 82.2 % | 84.3 % |
| GC 1.3 - 2.7 | 69.75 % | 90.8 % | 93.3 % |
| **GC 3.0a3 / 3.0a5 / 3.0a5.2** | **100 %** | **100 %** | **99.30 %** (460 B / 115 insns = retail) |
| Wii 0x4201_127 - 1.7 | 100 % | 100 % | 97.30 % (116 insns: one extra `lwz`) |

So the unit was built with a GC-era compiler (its lib entry now carries `mw_version: "GC/3.0a3"`); every Wii
compiler emits one instruction more than retail in the `R_PPC_REL24` case. The three 3.0a* builds are
codegen-identical on every reconstructed function, so the choice among them is only tied down by
`config.yml`'s `mw_comment_version` (14 = 3.0a3's comment byte; 3.0a5.x and the Wii compilers emit 15) -
that is a config value, not retail evidence. Older compilers also reject options the newer ones accept
(GC 1.x rejects `-gccinc`), so the driver drops unknown options and retries.

**Example**

```sh
python tools/flags/mwcc_matrix.py -u RSO/runtime GC/1.2.5n GC/3.0a3 Wii/1.3 Wii/1.7
# cross-family specs are "<family>/<version>"; a bare version still means the unit's own family
```

## 18. Named temporaries, declaration order and operand order steer the allocator

**Problem.** Once the opcodes, sizes and relocations all match, the residual is often nothing but register
numbers, and it looks unreachable from the source side.

**Why try it.** MWCC colours live ranges from the *source's* temporary structure, not only from the data
flow: an unnamed sub-expression is a short-lived temp with its own range, a named local gets its own
colour, and the order in which two locals are declared (or assigned) decides which one gets the lower
register. Operand order is observable as well - `a != b` and `b != a` emit `cmplw` with swapped operands.

**Result.** The single biggest lever on `RSO/runtime`, closing the last 2-39 % of three functions:
`fn_804DA7E4` 61.45 -> **100 %** (a signed `int count` for the `srwi.`+`ble` entry test plus a named
`RSOImport* pEntry` so the pointer lands in r4 and the byte offset in r5 instead of r0/r6),
`fn_804DA834` 81.56 -> **100 %** (a named `u32 no = <entry>.name_offset` at all three `strcmp` sites, a
named pointer *before* a named offset in the backward scan, and `hash == p->hash` rather than the
reverse), `RSOLink` 98.20 -> **100 %** (function-scope declaration order: the loop pointer before the entry
pointer). `RSOStaticLocateObject` needed the message as a local `char* msg = ...`; using the symbol inline
cost an extra `@ha` register and a whole `_savegpr_14` vs `_savegpr_15` colouring.

**Example**

```c
/* 61.45 % -> 100 %: signed count for the record-form test, named pointer for the colour */
int count = pModule->import_symbol_table_size / 12;   /* srwi. r0,r0,3 + ble */
u32 offset = 0;
RSOImport* pEntry;                                    /* r4; unnamed, the offset is pushed to r6 */
while (count-- > 0) {
    pEntry = (RSOImport*)((u8*)pModule->import_symbol_table_offset + offset);
    if (pEntry->code_offset == pModule->unresolved_function_offset) return FALSE;
    offset += 12;
}
```

## 19. Loop shape decides the loop idiom

**Problem.** A loop can compile to a `mtctr`/`bdnz` countdown, to a compare-and-branch, or to a bottom-tested
loop; the wrong idiom adds or removes instructions and moves every later register.

**Why try it.** MWCC picks the idiom from the source's shape, and it is visible in the first three
instructions after the loop's bound is computed, so it is a cheap thing to check before touching anything
else: `while (count--)` and `for (i = count; i > 0; i--)` become `mtctr` + `ble` + `bdnz` (with a
record-form shift as the entry test), `for (i = 0; i < n; i++)` becomes `cmpw` + `blt`, and an explicit
second induction variable (`for (i = 1, off = sizeof(RSOSection); ...)`) is what produces retail's separate
index and offset registers.

**Result.** `fn_804DA7E4`: the `mtctr`/`bdnz` idiom and the `srwi.` entry test only appeared with
`while (count--)` (a `for (i = 0; i < count; i++)` gave `cmplwi` + `ble` and one extra instruction). The
percentage did not move until idea 18 fixed the colours - the idiom and the colouring are two separate
residuals, so fix the shape first and only then chase registers. `RSOStaticLocateObject` reproduced
retail's `r15..r31` colouring (and `_savegpr_15` instead of `_savegpr_14`) only with the two-variable
`for`.

## 20. Force a loop-invariant address through a `u32` local

**Problem.** Retail materialises a loop-invariant field address into a callee-saved register in the loop
preheader (`addi r30,r26,84` then `lwz r0,0(r30)`); our build folds it into a load displacement
(`lwz r4,0x54(r27)`). That costs one callee-saved register and changes the colouring of the whole
function (`_savegpr_24` where retail has `_savegpr_23`).

**Why try it.** It is a codegen choice, not an algorithmic one, and taking the address (`&p->field`) does
*not* prevent the fold. Routing the address through an integer type does.

**Result.** `fn_804DA6C8` 94.77 -> **100 %** by computing `(u32)pObject + 0x54`, parking it in a `u32 buf[1]`
local, and declaring `int i;` before `count`. Worth trying wherever retail shows a preheader
`addi rN,rM,<disp>`.

**Example**

```c
u32 buf[1];
buf[0] = (u32)pModule + 0x54;   /* retail: addi r30,r26,84 ; lwz r0,0(r30) - not lwz r0,0x54(r26) */
```

## 21. Count record-form instructions to fingerprint peephole/scheduling per unit

**Problem.** The flags of one unit were inferred for the whole binary from "the DOL contains no `extrwi`" -
wrong twice over: it is not a whole-binary property, and the detector could not see what it was looking
for.

**Why try it.** MWCC emits record-form instructions (`srwi.`, `add.`, `extsb.`, `clrrwi.`) only with the
peephole pass on, and their count is a *per-unit* property of the retail bytes. Count them on the target
object before deciding a unit's `-opt` flags; they are the fingerprint the `-opt` axis actually leaves.

**Result.** The `RSO/runtime` target has 9 record forms (`srwi.` x4, `add.` x3, `extsb.`, `clrrwi.`) and
needs `-opt peephole,schedule,level=4`; the `Camellia` target has 0 and needs `-opt nopeephole`. The same
count exposes the alias trap: **GNU objdump never prints `extrwi`** - it prints the underlying
`rlwinm rX,rY,SH,MB,ME` - so "0 `extrwi` in 1 357 339 instructions" said nothing at all (a raw-word scan
finds 5 386 extrwi-form words). Any fingerprint built on a disassembler *alias* is a fingerprint of the
disassembler, not of the build.

**Example**

```sh
powerpc-eabi-objdump -d build/RMHE08/obj/<Unit>.o \
  | grep -oE "(srwi\.|slwi\.|add\.|subf\.|extsb\.|clrrwi\.|and\.|or\.)" | sort | uniq -c
```

## 22. When retail's colouring is your exact mirror, stop

**Problem.** A residual that is nothing but register numbers invites another hundred source variants, each
of which costs a compile-and-diff cycle and none of which can be reasoned about.

**Why try it.** MWCC's allocator assigns the *highest* free callee-saved register first (it minimises the
`_savegpr_*` range), so with N live webs the only free variable is the *priority order* of the webs. If
retail is the exact mirror of your build - same instructions, same sizes, mirrored colours - the source is
not the lever, and the honest move is to record the residual on the function and spend the time elsewhere.

**Result.** `fn_804D9B4C`: ~300 source variants (element types, four positions for each induction
variable, `while`/`do..while`/pointer walks/array forms, ten declaration permutations), all 22 GC and all
9 Wii compilers, and six `-opt` keyword permutations all produce the identical 21-row colour residual.
`fn_804DAA24`'s `R_PPC_REL24` case is the same story over 31 shapes. Both are documented as known
residuals on the function (see idea 13).

## 23. Data ranges in `splits.txt`: what objdiff can and cannot fix

**Problem.** A unit's near-miss rows are often just *symbol names* for data the unit owns but whose range
is not claimed in `splits.txt` (`@1841_80629B90` in the target vs our `lbl_80629B90`), so it looks like a
one-line fix.

**Why try it - and why it can backfire.** objdiff matches a *defined* data symbol by (section, offset) but
an *undefined* one by name. Claiming a range therefore fixes rows only when both sides end up defined at
the same offset - and defining a symbol changes what dtk emits. Claiming the RSO unit's string pool made
dtk drop the target's `R_PPC_NONE` pool relocations (they carry the pool-relative addends), so objdiff
could no longer pair the pool-relative instructions at all: 99.36 % -> 98.01 %.

**Result.** Claiming the compiler-generated jump table (`.data 0x80629C08..0x80629C40`) was worth +0.077 %
on `RSOStaticLocateObject`; claiming the string pool at `0x80629B90` cost 1.35 % on `fn_804DABF0` and was
reverted. Measure before *and* after, and check that the linked DOL hash did not change.

Two more jump tables confirmed it (batch 6), and one counter-example shows where the claim stops: claiming
`.data 0x805C5FA0..0x805C5FC4` took `Pl/pl_master`'s `fn_8026CC7C` from 99.9946 to **100 %** and
`.data 0x8060E8A0..0x8060E8E4` did the same for `Gecko_ExceptionPPC.cp`'s `ExPPC_NextAction` - both were
only ever short by the *relocation's* symbol, and the emitted table was byte-equal all along. The same
object's `.bss fragmentinfo` (0x806F4B48, referenced 6 times) must **not** be claimed: the target's `.bss` is
0x180 B and our object emits none, so the claim pairs a section against nothing and the unit's `matched_data`
collapses. There the fix was a plain re-split - dtk names an unclaimed reloc target from the map, and the
target object had been split before the symbol was renamed. Claim data that the object *emits*; re-split when
it only *references* something.

## 24. Merging a probe into the unit is its own step

**Problem.** Probes are measured standalone, in their own translation unit, so their numbers are not the
unit's numbers - and a probe cannot see the unit's types.

**Why try it.** The unit can only have one definition of a struct, so when two functions want different
field types the merge has to choose, and the choice is codegen-relevant: one field as `u8*` instead of
`u32` changed `add r6,r3,r0` into `add r6,r0,r3` and cost 0.3 % on that function.

**Result.** Give the struct the most specific pointer type and **cast at the individual use sites**; then
re-measure *every* function of the unit after the merge, because the unit's per-symbol table - not the
probe's - is what gets recorded. Merging the seven RSO reconstructions moved one function *up*
(`fn_804DABF0` 99.29 in the probe, 99.36 in-unit) and left the five byte-identical ones at 100 %.

## 25. Use the shared memory dump as a name/signature/struct oracle

**Problem.** Before a function can be matched it has to be *understood*, and this repo's `symbols.txt` has
thousands of `fn_XXXX` names and no types at all.

**Why try it.** A second Ghidra project on this machine holds a runtime memory dump of the game with real
SDK symbol names, function signatures, annotated struct layouts and the contents of data blobs this repo
does not own. It answers in one query what otherwise costs a disassembly read - and it answers questions
no static object can.

**Result.** For `RSO/runtime` it named eight of the unit's nine functions (`LocateObject`,
`RSOStaticLocateObject`, `RSOUnLocateObject`, `RSOLink`, `RSOUnLink`, `FindExportIndex`, `RSORelocate`,
`RSORelocateSmallDataSection` - the ninth, `fn_804DA7E4`, is a `zz_` placeholder there too), and it
separately named the four 4-byte `RSONotify*` thunks that sit just *before* the unit's range. It gave
their signatures (`RSOLink(RSOModule*, RSOModule*, ...)` - the second argument is the *exporting* module,
not a private "relocation table"), and its `RSOModule` layout confirmed every offset this project had
derived by hand, with real field names. It is **not** codegen evidence (idea 17), its annotations mix SDK
names with Ghidra placeholders, and a `splits.txt` range taken from it still has to be measured (idea 23).
Recipes and the full worked example: `docs/memory-dump.md`.

The same dump has a **companion symbol map** (`DumpSymbols.zip` -> `Dump_Loading85.raw.map`, 48 367 lines
of `name [args] address flags`), which needs no Ghidra session and answers two questions the project call
cannot answer in bulk: a real name (`fn_800406AC` -> `CntSdRsoTerminate`), and **whether an address is a
function at all**. The second one is worth a playbook row of its own because it is a *map* bug, not a
source bug: `src/auto/80040598_fn_80040598.cpp` carried five 4-byte `fn_80040794`-style symbols that were
not functions but the dead epilogue MWCC emits after a `mtctr`/`bctr` tail-call dispatcher. The dispatchers
compiled exactly 4 bytes longer than the map said (their bodies are byte-identical to *their own symbol
plus the artifact*), so objdiff could never pair them: growing the five owner sizes by 4 and deleting the
five artifacts closed four dispatchers outright (88.9 % -> 100 %) and removed 20 bytes of phantom code
from the map. The tell is in the dump: it carries `zz_<address>_` placeholders for genuinely unnamed
functions (including four inside that very range) and **no** line at those five addresses.

## 26. The target's section is part of the match

Problem: a unit can be instruction-identical and relocation-identical and still measure as *unmatched*, because
the code landed in the wrong section. The compiler emits `.text` by default; the map and the split object may
say something else - `.init` for the MSL runtime and the boot code.

Why try it: objdiff pairs sections, so a `.text`-vs-`.init` mismatch reports `fuzzy_match_percent: None`
while the per-function diff shows every row equal. That is the worst kind of false negative: the diff view says
the code is right and the report says nothing matched, which reads like "not decompiled yet".

Result: `__declspec(section ".init")` on the definition puts the function in the target's section, and the
unit then measures 100 %.

Example: `src/Runtime.PPCEABI.H/memset.c` (`memset`, `.init` 0x80004350-0x80004380, 48 B / 12 instructions).
Plain C produced `.text` + `.rela.text` where the target object has `.init` + `.rela.init`; everything else
agreed (`memset` at offset 0, one `R_PPC_REL24` to `__fill_mem` at 0x14, both 48 B) and the report said
`None`. With the declspec the object carries `.init`/`.rela.init` and the unit reads `fuzzy_match_percent
100.0`, `matched_code 48/48`. Confirm with `tools/elf/elfsect.py`: the forced section plus `.rela.<section>`,
and **no** `.mwcats.<section>` - that one shows up when the flags you test with omit the cats pragma, so test
with the whole flag list from `mt.py info`.

## 27. Same instructions, different order names the `-O` level - probe both, per unit

Problem: the instruction multiset and the relocations agree, but independent instructions are swapped or
split across registers - typically the epilogue's `lwz r0,0x14(r1)` (LR reload) and the function's real last
load. It reads like a scheduling residual with no lever, and source rewrites do nothing.

Why try it: the schedule is a property of the `-O` level, and the project default (`-O4,p` in `cflags_base`)
is not what every unit wants. It is also **not one setting for the build**: of the six units reconstructed so
far, `-O3` is right for Camellia, RSO, `g3d` and `lobby`, while `-O4,p` is right for `OS/OSAlarm.c` and
`NetworkWiiMediator.c` - under the wrong one the instructions come out reordered (g3d, lobby) or with the
`li r0,-1` no longer hoisted above the stores (OSAlarm).

Why the probe comes first: the target function can be diffed **before** the unit is registered, which saves a
re-split (minutes) per guess. It sits inside the `auto_*` object covering its region at `offset = function
address - blob base`, and the base is in the object's own name - region blobs are `auto_<n>_<BASE>_text.o`,
per-function objects are `auto_fn_<ADDR>_text.o`:

```sh
# auto_03_80458A60_text.o covers 0x80458A60..., so 0x804CBC50 is at offset 0x731F0
build/binutils/powerpc-eabi-objdump.exe -d --section=.text build/RMHE08/obj/auto_03_80458A60_text.o
#     its window for 0x804CBC50-0x804CBC60 is offset 0x731F0-0x73200
```

Compile the candidate source under both levels into a scratch directory and compare that window: the level
that reproduces it byte-for-byte (mnemonic + operands, reloc names normalised) is the one for the per-library
`cflags_*` override.

Example: `src/g3d/g3d_resanmamblight.c` (`fn_800680A8`, 0x24 B / 9 instructions). Under `cflags_base`
(`-O4,p`) the unit measured 97.56 % with `lwz r0,0x14(r1)` before `lwz r3,0xc(r3)`; the same source under
`-O3` is byte-identical (100 %), now `cflags_g3d`. The same two-variant probe then landed
`src/OS/OSAlarm.c` (`-O4,p` identical, `-O3` differs) and `src/lobby/lobby_scene.c` (`-O3` identical) at
100 % on their first registration. `tools/flags/optsweep.py` cannot see any of this: it reports frame sizes,
so a frame-equal sweep row is not evidence about instruction order.

The level also decides **function packing**, which is a second and independent reason to probe it: `-O4,p`
implies `-func_align 16`, so under it every function after the first moves to the next 16-byte boundary. Which
one a unit wants is visible in its target object - `Runtime.PPCEABI.H`'s five objects are all `.init
align 2**2` and `memcpy.o` packs two functions contiguously, so `-func_align 4` was required there (it is a
per-lib flag now, `cflags_ppceabi`, with `#pragma function_align 4` in `src/Runtime.PPCEABI.H/memcpy.c` as its
first witness; `__start.o` in the same lib does want 16-byte function starts, which come from each function's
own `#pragma section code_type ".init"` section, not from the function alignment). A `-O` change that fixes an
instruction order but moves every symbol after the first is not a fix: check the section alignment and the
function offsets too.

The same flag bites *inside* a function: MWCC aligns loop heads to 8 bytes relative to the object's `.text`
start, so 16-byte packing made two `Gecko_ExceptionPPC.cp` functions carry a `nop` before their loop
(`ExPPC_FindExceptionRecord` 99.07 -> 100.00 with `-func_align 4`), and left 4-8 bytes of padding between the
functions of two other objects in that lib (`global_destructor_chain` `.text` 0x20 vs 0x18,
`__init_cpp_exceptions` 0x74 vs 0x70). Three independent witnesses to one flag is what makes the re-split worth
it.

## 28. A kept `bl` to a tiny static names the unit's inlining setting

Problem: the target calls a small file-local function the source could just as well inline
(`bl fn_8003F554`), while our build inlines it away - the callee has no counterpart in our object and the
caller comes out an instruction short, shifting every register after it. It reads as a missing helper, so the
first instinct is to hunt for a source shape that suppresses the inline.

Why try it: `-inline auto` in `cflags_base` inlines eagerly, and not every retail unit was built that way
(`-inline off`, or a threshold we cannot see). The inline setting is a per-library flag like the `-O` level,
and a `#pragma` can scope it to the one function that needs it.

Result: for the `main` lib (batch 2), `-O3` **plus `-inline noauto`** took `fn_8003F52C` and `fn_8003F564`
from 74 % to 100 % (retail keeps their `bl fn_8003F554`), `change_widemode_req_default__Fv` from 21.18 % to
100 %, and `main` itself from 71.12 % to 96.73 %. Pick the *middle* setting, not `-inline off`: `fn_8003F940`
is retail's inlined aggregate `GXRenderModeObj` copy, and `off` turns it into a call to the implicit
copy-assignment operator (99.02 %, and 4 bytes short) where `noauto` still inlines it (100 %). Two spelling
traps came out of the same batch: `#pragma peephole on/off` is honoured (it is what keeps retail's unfused
`srwi`+`clrlwi` in `change_widemode_req_default__Fv`), while `opt_peephole`/`peep` parse and do nothing, and
whole-unit `-opt nopeephole` cost `main` 2.2 points - scope the pragma to the function that needs it, and
never assume a pragma name works because it parses.

Example: the inline half belongs in the library's flags

```python
cflags_main = [
    *[f for f in cflags_lobby if f != "-inline auto"],
    "-inline noauto",
]
```

and the peephole half from the same batch is a pragma, scoped to the function that needs it because
`-opt nopeephole` for the whole unit cost `main` 2.2 points:

```c
#pragma peephole off
void change_widemode_req(unsigned char mode);
```

## 29. A claimed literal pool: declare the constants, never define them

Problem: a unit whose `.sdata2`/`.sdata` fragment is claimed in `splits.txt` still shows its pooled constants
as `ARG` rows - the target loads `lbl_80795AC0@sda21`, our source loads a literal the compiler put in a pool of
its own, and the names cannot pair. The obvious fix (define the constants in the source) is the wrong one: it
rebuilds the pool, so the section and every load that references it changes.

Why try it: the claim already put the target's pool *inside* the unit, so the map's names (`lbl_80795AC0`,
`lbl_80790E24`, ...) can be declared `extern` in the source and used as the operands of the loads. The compiler
then references the claimed address instead of pooling a new copy, and the rows pair by name.

Result: `main.cpp` (batch 3) declared its `.sdata2` constants (0x80795AA0-0x80795AD8) and its `.sdata` byte
instead of redefining them, which is what let the remaining float rows in `main`, `fn_8003FEBC`,
`fn_8003FF98` and `fn_8003FC64` be judged on their instructions rather than on a pool name.

Example: the `splits.txt` claim plus

```c
extern f32 lbl_80795AC0;   /* not `f32 lbl_80795AC0 = ...;` - that would rebuild the pool */
```

The claim is only half of it, and the pool's *extent* has to be read before claiming it (batch 6,
`Pl/pl_skill.cpp`): the unit's own run is bounded by its neighbours' (0x8079A030-0x8079A080, left by
`pl_master`'s last entry 0x8079A028, right by `pl_act`'s first 0x8079A080), and the entries it *shares* with
them (0.0f, 1.0f, 10.0f and the int->float magic) sit in the **preceding** unit's run, so no contiguous claim
can cover them. Claiming a pool the object does not emit is worse than not claiming it - the target's 80 B
section then pairs against nothing - so declare the unit's own constants `extern` and use them as load
operands (that alone took five `pl_skill` functions to 100 %, +0.072 pt for the unit, nothing worse). The two
implicit int->float magics the compiler emits on its own cannot be named from source at all: they are the
residual, and the header should say so rather than claim the range.

## 30. A C++ unit's exception settings live in its object, not in the source

Problem: every function of a unit matches instruction for instruction, and the *unit* still measures short
because its target object carries `extab`/`extabindex` while ours carries none, or carries different records.
It reads as a codegen problem, so the search goes into the source and stays there.

Why try it: `-Cpp_exceptions off` in `cflags_base` is a **build** setting, and not every retail object was built
with it. The object says which one it was: `extab`/`extabindex` present means exceptions were on, and their
*entries* say which constructs were used - a `throw()` specification emits one handler reference and an empty
action list, where a real `try`/`catch` emits far more. The setting is per unit, not per project: in this repo
the `Pl` lib and `Gecko_ExceptionPPC.cp` need it on, while `__init_cpp_exceptions.cpp` would *gain* extab its
target does not have.

Result (batch 5): `-Cpp_exceptions on` for the `Pl` lib left every `.text` byte unchanged and made all 12 extab
entries we could emit equal the target's; `#pragma exceptions on` in `sys_mem.cpp` (a unit whose lib keeps the
flag off) turned its `operator new`/`operator delete` `throw()` specs into the target's extab byte for byte; the
same pragma on `Gecko_ExceptionPPC.cp` reproduced the target's `extab 0x10` + `extabindex 0x18` exactly. So make
it part of a unit's measurement: compare the two objects' `extab`/`extabindex` **sizes and bytes**, not only the
score.

Example:

```c
#pragma exceptions on   /* the source spelling of the one flag, for a unit in a lib that has it off */
```

and for a `$`-suffixed section the pragma **pair** is required: `#pragma section const_type ".ctors$10"` to name
it plus `__declspec(section ".ctors$10")` to place it - `__declspec` alone is rejected (error 33048), and
`code_type` would add a `.mwcats` section the target does not have.

## 31. A function unpaired by name measures 0 %, not 60 %

Problem: a unit's functions are written and their bytes are right, and the score stays near zero. The
instinct is to re-read the code - the wrong place, because objdiff pairs functions **by symbol name**.

Why try it: a map name that does not match the name the object emits (a `fn_XXXXXXXX` placeholder, a renamed
function whose definition was not renamed with it, a C++ function whose mangled name the map spells
differently) leaves both sides unpaired, and an unpaired function contributes nothing at all. The unit's own
symbol table has the exact answer: the object says what it calls the function.

Result (batch 6): three of `Gecko_ExceptionPPC.cp`'s five functions measured 0 % while being byte-perfect -
the map called them `fn_80457504`/`fn_8045759C`/`fn_8045774C` and the object emitted
`ExPPC_FindExceptionFragment__FPcP12FragmentInfo`, `ExPPC_FindExceptionRecord__FPcP15MWExceptionInfo` and
`ExPPC_NextAction__FP14ActionIterator`. Three `symedit.py rename`s and **no source edit** took the unit from
9.82 % / 1 of 5 functions to ~99.5 % / 4 of 5 (the fifth is a preheader scheduling tie-break).

Example: read the name the object emits, then rename the map to it:

```sh
build/binutils/powerpc-eabi-nm.exe --defined-only build/RMHE08/src/main/<Dir>/<unit>.o | grep <addr>
python tools/symbols/symedit.py rename fn_80457504 ExPPC_FindExceptionFragment__FPcP12FragmentInfo --dry-run
```

The same mechanism runs the other way: dtk names a *target* object's relocation targets from the map, so a
target object split before a rename keeps the old name and looks like a mismatch - a plain re-split (or
`ninja build/RMHE08/obj/...`) refreshes it. Check the object's mtime against `symbols.txt`'s before believing
a reloc row, and never "fix" it by claiming the data range it points at.

## 32. A pragma region is not local to the functions it covers

**Problem.** A function whose residual is a missing instruction - a `lis` the build CSE-ed away, a base
re-materialised at a merge - needs a scoped `#pragma peephole off` pair (row 28's spelling). The pair fixes it,
but the *reset* also decides where the region ends.

**Why try it.** The pragma is per-region, not per-function: everything between the two pragmas is affected, so the
reset's position is a free variable and moving it changes codegen in functions the author never named.

**Result.** In `src/main.cpp`, scoping `#pragma peephole off` around `fn_8003FEBC`/`fn_8003FF98` closed them
(98.00 -> 99.82 and 99.02 -> 99.95), and moving the reset past two further functions flipped `fn_8004029C`
99.52 -> 100 and `fn_80040360` 99.47 -> 100. Unit 97.77 -> 99.25 %; matched 228 -> 234 across the campaign.

**Example.** The pragma pair in `src/main.cpp` around those two functions: widening the region by two functions
fixed two functions it was never aimed at. When a pragma pair is in play, re-measure the whole unit - where the
region ends is part of the change.

## 38. An `s16` parameter with a compound assignment is what makes a field store raw

**Problem.** A store into a narrow field (`u8`/`u16`) comes out masked - a `clrlwi` before the `stb` - where retail
stores the value as it stands. The function sits at 82-94 % with one extra instruction and every later register
shifted, and no cast, temporary or operand order in the store itself moves it.

**Why try it.** That mask is MWCC narrowing the value to the field's width. Taking the value through a *signed*
16-bit parameter and writing the field with a **compound assignment** leaves the compiler holding a value that is
already the right width, so it stores it raw. It closed four functions outright in one round and improved four more.

**Result.** `Pl/pl_act` 97.86864 -> **98.43198 %**: matched bytes 10648 -> 12152, byte-exact functions 73 -> **80 of
115**, nineteen functions improved and **zero regressions**, `.text` still the target's exact 27436 B, no flag change.
A044 82.2 -> 100, 76CE8 92.7 -> 100, CA48 94.0 -> 100, C8B4/D5A4 -> 100, 76E08 -> 99.9, 78674 -> 99.0, B0BC
94.0 -> 95.7.

**Sibling shapes from the same round** (each worth trying before assuming a flag): the **field form**
(`self->equipC[0]`) for a load immediately followed by pointer formation (79414 -> 100, `Get_Shell_rate_adj` ->
99.9); `(u32)` on an `s32` helper to get `cmplwi`; `-0x1006` to get `li`; `(x * 7) << 1` for `* 14`. Two *real* layout
bugs fell out of the same sweep: one function stored into 0x449 where retail writes 0x44C, and `_MOVE_WORK.unk44F`
was declared one byte too long.

**Example.**

```c
void fn_8027A044(s16 amount) {   /* s16, not s32 and not u32 */
    self->field += amount;       /* compound assignment, not self->field = self->field + amount */
}
```

## 37. A switch's `default` arm goes first in the source

**Problem.** A `switch` whose default dispatches into a helper comes out a few bytes too big - 364 against the
target's 360 - with the default body sitting in the middle of the compare chain and the function stuck around 94 %,
even though every case body is right. It reads as a missing case or a wrong table.

**Why try it.** MWCC emits the default body *after* the compare chain wherever it is written, so writing `default:`
**first** in the source lands it where retail has it. Written last, the compiler orders the chain the other way and
the function grows by a word.

**Result.** `auto/802D0DCC_fn_802D0DCC`'s only function went 0 -> **100.00 %** (360 B both sides, 90/90 rows, `.text`
byte-identical, all 42 `.rela.text` records matching) under the stock flags - no flag, no pragma. `default` written
last gave 364 B / 93.73 %. The sibling shape in the same function: the callee is called as `(self, entry)` so `r3`
stays `self` across the call.

**Example.**

```c
switch (entry->kind) {
default:                          /* first in the source; MWCC still emits it after the chain */
    fn_802D3398(self, entry);
    break;
case 1:
    ...
}
```

## 36. A flipped unit's unreferenced trailing function is trimmed by the linker

**Problem.** The unit's object is byte-identical, `flipcheck.py` is happy and `linkorder.py` says `LINK OK` - and the
flip still breaks the DOL, by exactly the size of the object's *last* function, with every later section shifted. It
reads as an unreachable linker mystery, because nothing about the code differs.

**Why it happens.** `dol split` writes the target objects with `export_all: true`, which stamps `active_flags=0x08`
on every entry of the `.comment` symbol table; MWCC-compiled objects write `0x00`. The linker honours that flag, so
our object's *trailing* function - the one nothing inside the object references - is dropped, along with its `extab`
record and `extabindex` entry. Measured on `sys_mem.cpp`: `ninja diff` reported `_eti_init_info` at 0x8003F17C where
the target has it at 0x8003F1C8, and the 0x4C difference is exactly `fn_8004054C`'s size.

**Result.** Marking the function `__declspec(export)` sets the flag. `.text` is unchanged and the link reproduces the
original DOL byte for byte - `sys_mem.cpp`, flip 12. Proven three ways: adding the symbol to the script's
`FORCEACTIVE` restores the target layout, and swapping the two objects' `.comment` sections in each direction swaps
the behaviour. Ruled out by measurement: the `.comment` version byte alone, `SHF_INFO_LINK` on `.rela.*`, section
order, `.note.split`, local symbol names, and all 21 Wii/GC compilers.

**Example.** Only a *trailing* unreferenced function needs it, and only in a unit that flips. The generalised check
belongs in `flipcheck.py`: compare the `.comment` per-symbol `active_flags` (offset 0x2C + 8*index, byte 5) between
`obj/<unit>.o` and `src/<unit>.o` - a mismatch means the link will trim.

## 35. A dead copy chain steers the allocator's web priority

**Problem.** The residual is two live ranges sharing one register pair - retail colours them one way, we colour them
the mirror - and no source shape, type, cast, statement order or flag moves it. `Pl/pl_master`'s `fn_8026F908` sat
at 98.85 % (9 bytes) with the same 39 instructions; only the register pair differed.

**Why try it.** The allocator colours webs in web-list order, and the IR's *copy* webs participate in that order even
when the copies are dead. A chain of dead copies of the competing value, plus one separate live load of it, flips
which web is coloured first without changing a single emitted instruction. It is the **copy count** that matters:
chains of one or two copies, and equally many fresh loads, do nothing.

**Result.** `fn_8026F908` 98.85 -> **100 %**, the unit's `.text` byte-identical (0x45A0, 0 differing bytes, 24/24
functions 100 %, and the object links - flip 11). ~200 source shapes, all 30 toolchain compilers and every `-opt`
keyword were measured and ruled out first; `scheduling on` / `-O4` do flip the register but reorder the block.

**Example.** The dead copies exist only to steer the allocator and are optimised away.

```c
u32 classCopy0 = self->weaponClass;   /* dead */
u32 classCopy1 = self->weaponClass;   /* dead */
u32 classCopy2 = self->weaponClass;   /* dead */
u32 weaponClass = self->weaponClass;  /* the one live load */
if ((u32)(weaponClass - 4) <= 2 && self->unk18 == 1) { ... }
```

**Honest note.** This is a matching *trick*, not the original source - retail had no dead copies. Record it as such
in the unit's header comment (done there), and reach for a flag or a real source shape when one exists (row 33).

## 34. A switch tail's constant returns are if-converted, so write the arms negated

**Problem.** A `switch` whose default is `return 1` and whose allowed cases `break` leaves the target as
`return 0` looks like a missing arm: the diff shows the target loading a constant and branching while ours
returns early per case, and the function sits at ~94 % with the same opcodes in a different order.

**Why try it.** MWCC if-converts two constant return arms (`return 0` / `return 1`) into a branchless boolean,
so a body written `if (c) return 0; break;` compiles to an early `return 0` the target never has. Negating the
test (`if (!c) return 1; break;`) gives the compiler the *same* two arms but in the order it folds into the
branchless form, and the tail matches. The same class of shape - a case body that falls through to a shared
constant - has to be written the way the *tail* reads, not the way the condition reads.

**Result.** `Pl/pl_act`'s `fn_8027C208` 93.926 -> **99.967 %**, `.text` exact. The sibling shapes in the same
round: cases written in **body-address order** (not condition order) took `fn_8027A340` 94.234 -> 98.084 %, and
`s32` locals for equality tests (so the compiler emits `cmpwi`, not `cmplwi`) took `Pl_bari_ck`
84.415 -> 86.679 %. Unit 97.42033 -> **97.86864 %**, matched bytes unchanged, no flag change.

**Example.**

```c
/* target: the tail is `return 0`, the default `return 1`, every case falls through */
switch (id) {
case 0: case 4: case 2:            /* body-address order, not condition order */
    if (!allowed) return 1;        /* negated: gives MWCC its two constant arms */
    break;
default:
    return 1;
}
return 0;
```

## 33. Prefer the unit's flags over a per-function flag - a TU was compiled once

**Problem.** A function that will not match invites a scoped pragma (`optimization_level`, `peephole`,
`scheduling`) aimed at that function alone. Three wins this session did exactly that.

**Why it is usually wrong.** A translation unit is compiled **once**, with **one** flag set. If the other
functions in the same unit already match, they are evidence that the unit's flags are right - so a function that
needs *different* flags is far more likely to be a **source** difference (shape, liveness, declaration order), a
**boundary/attribution** error, or a **stale target object**, than evidence of a per-function flag. A pragma that
fixes one function by changing codegen the original compiler never had is matching the bytes for the wrong reason,
and it will fight the rest of the unit (RSO's level-3 pragma costs `RSOUnLink` and `FindExportIndex` their 100 %;
`optimization_level`/`opt_*` pragmas are whole-function, so a mid-function switch is silently ignored anyway).

**Result / how to apply it.** When a set of symbols is *known* to belong to one TU and one function lags:

1. re-derive the **unit-level** flag from the functions that already match, and check the lagging function against
   it - if it needs something else, look at the *source* first;
2. check the three usual non-flag causes: a stale **target object** (re-split), a **boundary** error (the symbol
   belongs to another unit), and a **naming** mismatch (objdiff pairs by name);
3. only if the unit's flags are genuinely ambiguous - different functions demanding different levels, with
   byte-level evidence both ways - is a scoped pragma defensible, and it must then be recorded as a *suspected
   flag hack* in the unit's header, not as a solution.

**Example.** The RSO and `pl_skill` pragmas each closed a function while costing a sibling its 100 %, which is the
signature: one flag set cannot be right for a unit and wrong for one function inside it.
