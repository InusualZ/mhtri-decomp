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

Example: `src/g3d/g3d_anmscn.cpp` (`fn_800680A8__FPv`, 0x24 B / 9 instructions; `g3d_resanmamblight.c`
until the wQ-recut re-homed it, and the map name gained its argument list in the wR-mangling pass - playbook 48).
Under `cflags_base`
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

## Ruled out - do not re-run these

* **Compiler version.** Compile the matrix once for the unit; if it is flat, version is not the lever.
* **The whole `-opt` axis.** Sweep the compiler's own keyword list once. Beyond the one or two keywords
  that matter, the rest are no-ops or make the code worse; the frame-size traps are in trick 10.
* **`-Cpp_exceptions`.** It only adds `extab`/`extabindex` sections (needed at the end for a full match) and
  does not change `.text` here.
* **`-O` level and `-schedule`/`-fp_contract`/`-ipa`.** Worth exactly one sweep each; if the level is wrong
  the symptom is unmistakable (sizes off by hundreds of bytes and fused/hoisted code everywhere).
* **Disassembler aliases as fingerprints.** `extrwi`, `clrlslwi` and friends are *aliases* GNU objdump never
  prints (it prints the underlying `rlwinm`), so counting them on either side proves nothing - see idea 21.
  Fingerprint what the compiler emits (record forms, save-helper calls, `lis` sharing), not what the
  disassembler happens to name.
* **`-sdata 0` for a game unit.** Measured on `Pl/pl_skill.cpp` (batch 6): it turns the target's absolute
  `lis`/`addi` to `lobby_w` into small-data accesses and breaks the two functions that *should* be small-data,
  so on the pre-fix source it was +0.085 on two functions and -0.11 on two others (net -0.03), and with the
  real fix landed it is purely harmful (-0.12 pt). The absolute access a target shows is a property of *that
  symbol's section*, not of the unit's addressing mode: declaring `lobby_w` as an unsized array (`lobby_w[0]`)
  gives the absolute form while `lbl_80792140/48` stay `@sda21`, which is what the target does.

## Worked example: the `RSO/runtime` unit (DOL-side RSO loader/linker)

Nine contiguous functions at `0x804D9B4C..0x804DAE40` (4852 B), unsplit when it was picked up, with two
SDK names and seven `fn_XXXX`. Everything below is in the repository now (`src/RSO/runtime.c`,
`cflags_rso` + `mw_version` in `configure.py`, the `.text` and jump-table ranges in `splits.txt`).

**Finding out what the unit is** (idea 25): one `mcpScript` fanning the nine addresses through the shared
dump returned `LocateObject`, `RSOStaticLocateObject`, `RSOUnLocateObject`, `RSOLink`, `RSOUnLink`,
`FindExportIndex`, `RSORelocate` and `RSORelocateSmallDataSection` - eight of the nine (`fn_804DA7E4` is
a `zz_` placeholder there too) - plus the four 4-byte `RSONotify*` thunks that sit immediately *before*
the range, typed signatures whose body lengths match this repo's map sizes exactly, and an `RSOModule`
layout that matched every offset derived from the disassembly.

**Finding out how it was built** (ideas 17, 21): the record-form count (9 in this target, 0 in
`Camellia`'s) said peephole + scheduling were on, so `-opt nopeephole` - the project's `Camellia` setting -
was wrong here; a cross-family version matrix then showed every Wii compiler emitting one instruction more
than retail in `RSORelocate`'s `R_PPC_REL24` case (116 vs 115) while GC 3.0a3/3.0a5/3.0a5.2 reproduced the
retail opcode sequence, so the lib entry carries `mw_version: "GC/3.0a3"`.

**Reconstruction** - the last percent of each function came from one of the levers:

| function | lever that closed it | before -> after |
| --- | --- | --- |
| `fn_804DA7E4` | signed `int count` + named `pEntry` (idea 18), `while (count--)` (idea 19) | 61.45 -> **100 %** |
| `fn_804DA834` | named temps at the `strcmp` sites, named pointer before named offset, `hash == p->hash` (idea 18) | 81.56 -> **100 %** |
| `RSOUnLocateObject` | typed array walk (`((RSORelocation*)p->table)[i]`) + hoisted `pEntry` | 74.92 -> **100 %** |
| `RSOLink` | value merge instead of early return, `offset` as a second induction variable, declaration order (ideas 18, 19) | 59.12 -> **100 %** |
| `fn_804DA6C8` | loop-invariant address through a `u32 buf[1]` (idea 20) | 94.77 -> **100 %** |
| `fn_804DAA24` | comparison operand order (idea 18) - 31 further shapes made no difference | 97.30 -> 99.30 % |
| `fn_804D9B4C` | two named temporaries - then ~300 variants, 31 compilers, 6 `-opt` sets all identical (idea 22) | 0 -> 99.39 % |
| `RSOStaticLocateObject` | two-variable `for` + local `char* msg` (ideas 18, 19); jump table claimed in `splits.txt` (idea 23) | 74.92 -> 99.28 % |
| `fn_804DABF0` | split `u32 e0 = ...; u32 name = e0 + ...;` (idea 18) | 0 -> 99.36 % |

Unit: `fuzzy_match_percent` 5.47 -> 99.71, `matched_code` 2132/4852, **5 of 9 functions byte-identical**,
and every function's instruction count equal to retail. The four residuals are allocator colour (ideas 22
and 13) and are recorded on the functions themselves; the string-pool `splits.txt` claim that looked like
the last fix was measured, found harmful (-1.35 %) and reverted (idea 23).

## Worked example: the `Camellia` unit

Starting point: the project defaults (`-O4,p -inline auto -use_lmw_stmw on -str reuse,pool,readonly`),
`.text` 21600 B against the target's 24420 B, unit at 1.49 % fuzzy, **6 of 10 functions at 0 %**.

Final flag set (now `cflags_camellia` in `configure.py`):

```
-O3  -inline noauto  -opt nopeephole  -pool off  -use_lmw_stmw off
```

which gives **9 of 10 functions byte-identical** and the target's exact `.text` size. Each flag removed one
specific symptom:

| flag | symptom it removes | symptom seen in the target |
| --- | --- | --- |
| `-O3` (not `-O4,p`) | fused index math, hoisted table bases | `extrwi`+`slwi` instead of one fused `clrlslwi` |
| `-inline noauto` | `camellia_setup192` inlined into `Camellia_Ekeygen` | `b camellia_setup192` tail call, 68 B dispatcher |
| `-opt nopeephole` | `srwi`+`clrlwi` fused into `extrwi` | the two-instruction byte-extraction pair |
| `-pool off` | one shared `lis` base for the four S-box tables | one `lis`+`addi` pair per table |
| `-use_lmw_stmw off` | `stmw`/`lmw` register save | EABI `_savegpr_14`/`_restgpr_14` calls |

The sweep, one flag at a time (each row adds one flag to the previous row):

| override | effect |
| --- | --- |
| `-inline noauto` | `Camellia_Ekeygen` **100 %** (68 B) - first confirmed function |
| `+ -O3` | `camellia_setup192` **100 %** |
| `+ -opt nopeephole` | `EncryptBlock`/`DecryptBlock` **100 %**; the four S-box users to ~99 % |
| `+ -pool off` | `encrypt/decrypt128/256` **100 %** (4 more) |
| `+ -use_lmw_stmw off` | `camellia_setup128` **100 %** (9 of 10) |

The one residual, `camellia_setup256`, is a source/liveness difference (one extra 4-byte stack slot, all
instructions identical except the r1 offsets); it is documented in the unit's file header comment in
`src/Camellia/camellia.c`. The full blow-by-blow log, including everything that was tried and rejected, is
in `.pi/notes/camellia-match-process.md`.

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
## 39. A unit whose retail code keeps unfused peephole folds needs the peephole pass off

**Problem.** The unit's retail object keeps instructions our peephole pass folds away: a masked `clrlwi` before a narrowing store, a separate `clrlwi`+`cmpwi`, a record-form `clrlwi.` the target does not have, or an `li r0,<slot>` + `psq_lx`/`psq_stx` epilogue. Our build, with the command line's peephole on, emits the fused form and lands one or two instructions short - a 4-25 % gap that reads as a source problem.

**Why try it.** `#pragma peephole off` is the source spelling of `-opt nopeephole`; it turns the pass off for the file or a scoped region and restores the target's unfused form. Several units in the `auto` bucket were built with it off, so it is the first lever to try when the diff is a *fold*, not a shape. Note the level is not the lever: `#pragma optimization_level 1` leaves the command line's peephole on (see the trap below).

**Result.** 14 unit(s) measured the same lever independently, so the evidence is grouped here rather than written once per outbox:

* `auto/80073398_fn_80073398` - #pragma peephole off (scoped to fn_8007403C) - fn_8007403C: 28.0 -> 100.0
* `auto/800898B0_fn_800898B0` - #pragma peephole off (file scope, cflags_main has peephole on) - fn_80089F94 95.8 -> 100.0, fn_8008A220 95.8 -> 100.0 (record-form `clrlwi.` removed; retail object has zero record forms)
* `auto/8009AA78_fn_8009AA78` - #pragma peephole off (file-scoped) - all 8 symbols -> 100.0 (fn_8009AA78 98.90 -> 100, fn_8009AB28/38 71.25 -> 100, fn_8009AB8C 77.93 -> 100); .text 0x26C / extab 0x28 / extabindex 0x3C byte-identical
* `auto/800C9DD0_fn_800C9DD0` - #pragma peephole off  (== -opt nopeephole) - fn_800C9DD0: 94.24254 -> 99.94403 %; retail's `li r0,136; psq_lx f31,r1,r0,0,0` epilogue and the `clrlwi r4,r30,16` argument narrowing restored; the pragma's object is byte-identical in .text/extab/extabindex to the same source compiled with `-opt nopeephole`
* `auto/800CB948_fn_800CB948` - -opt nopeephole - fn_800CB948: paired-single epilogue restored to retail's `li r0,<slot>; psq_lx` form (0x5a4..0x660 byte-identical); 90.89 -> 97.29 together with -fp_contract off
* `auto/800CC5B0_fn_800CC5B0` - -opt nopeephole - fn_800CC5B0: 84.92 -> 88.64 %
* `auto/800CCCF8_fn_800CCCF8` - #pragma peephole off (per-unit, whole file - the source spelling of -opt nopeephole) - fn_800CCE38: 97.5 -> 100.0; fn_800CCDFC: 99.33 -> 100.0; all 10 symbols 100.0
* `auto/800CCFB0_fn_800CCFB0` - -opt nopeephole (via #pragma peephole off) - fn_800CCFB0: 94.63 -> 97.79, .text 0x5B8 -> 0x5D4 (target 0x5D4). Without it MWCC's peephole folds the epilogue's `li r0,off; psq_lx f,r1,r0` into `psq_l f,off(r1)`, 7 instructions short; the target keeps the indexed form. Same stand-in as the other auto unit (docs/plan.md 6.5)
* `auto/800D7F54_fn_800D7F54` - -opt nopeephole - sysSE_req/fn_800DBC84/fn_800DB4EC/fn_800DC53C 95.0/95.0/92.8/64.4 -> 100/100/100/100; no other function changed
* `auto/800DCFEC_fn_800DCFEC` - -O3 -opt nopeephole + `#pragma peephole off` - fn_800DCFEC 100.0 % (516 B / 516 B) - the committed state
* `auto/800E46E8_fn_800E46E8` - a per-region cflags group with `-opt nopeephole` (everything else as cflags_main), so the whole-file `#pragma peephole off` in this unit's source can go - Same source, real command line from build.ninja, measured with recompile.py: cflags_main as committed (peephole on) = fn_800E46E8 94.81 %, set_stream_main_vol_flag__FUcUc 71.25 %, fn_800E48E4 87.22 %, fn_800E4908 76.67 %; `#pragma peephole off` (the committed state) = 100 % on all four. The signature is the unfused `clrlwi r0,r3,24` (+ `cmp...
* `auto/800FCED4_fn_800FCED4` - #pragma peephole off, scoped to the five functions from fn_800FCED4 on (file scope NOT used) - fn_800FCED4 99.92 -> 100.0, eft002_set 97.73 -> 100.0, eft002_set_shell 99.62 -> 100.0; unit 99.59 -> 100.0; .text 0x64C and extab/extabindex byte-identical; the other five stay 100.0
* `auto/802B2978_fn_802B2978` - -opt nopeephole - fn_802B2978 99.932434 % (296 B, 73/74 rows)
* `auto/80324F7C_fn_80324F7C` - -O3 -opt nopeephole + `#pragma peephole off` - fn_80324F7C 100.0 % (308 B / 308 B) - the committed state

**Example.**

```c
#pragma peephole off   /* the whole unit, or a scoped pair around one function */
```

**Measure it, do not assume it.** On two units this lever is a *regression*: `auto/800FD718` goes 100.00 -> 98.72
(`.text` 0x14C -> 0x150) and `auto/80119C44` 99.61 -> 95.88, where it produces the target's `addi r0` but un-folds two
narrowing stores the peephole was correctly folding. Both targets keep *some* folds, so the pass is not a per-bucket
property - try it, then keep it only if the measurement agrees.

## 40. A unit whose retail code keeps unfused multiply-adds needs `-fp_contract off`

**Problem.** Retail keeps `a*b + c` as two instructions (`fmuls` + `fadds`/`fsubs`) where our default `-fp_contract on` emits one fused `fmadds`/`fmsubs`, so the function is a few instructions short and every later register shifts. It reads as a source-shape problem and sends you rewriting expressions that were already right.

**Why try it.** `#pragma fp_contract off` (or `-fp_contract off`) turns the contraction off, the two instructions come back, and the expression can stay natural. The recurring shapes are `2.0f*x - 1.0f` and `1.0f + rate*t`.

**Result.** 7 unit(s) measured the same lever independently, so the evidence is grouped here rather than written once per outbox:

* `auto/80073398_fn_80073398` - #pragma fp_contract off (file-scoped) - fn_80075940: 87.42 -> 93.47; unit 72.0 -> 73.14
* `auto/800898B0_fn_800898B0` - #pragma fp_contract off (file scope) - fn_80089B88 79.12 -> 100.0 (MWCC fused four fmuls+fadds pairs; retail has none)
* `auto/800C9DD0_fn_800C9DD0` - #pragma fp_contract off - fn_800C9DD0: 99.94403 % (unchanged - temporaries reproduce the same unfused pairs); the natural expressions become usable, and auto/800CB948 measured 4 fused ops removed by the same flag
* `auto/800CB948_fn_800CB948` - -fp_contract off - fn_800CB948: 4 `fmadds`/`fmsubs` -> retail's separate `fmuls`+`fadds`/`fsubs`; no other function moves
* `auto/800CC5B0_fn_800CC5B0` - -fp_contract off - fn_800CC5B0: 84.92 -> 85.92 %
* `auto/800CCFB0_fn_800CCFB0` - -fp_contract on (auto lib default) vs #pragma fp_contract off - fn_800CCFB0: with `on` MWCC contracts two a*b+c chains into fmadds; the target has only fmuls+fadds. `off` restores the target's FP exactly (the whole 0x408-0x45C block becomes instruction-identical)
* `auto/800CD584_fn_800CD584` - #pragma fp_contract off (scoped to this unit's source) - fn_800CD584: 0 % (fused fmsubs/fmadds/fnmsubs) -> 93.69 % (fmuls/fadds/fsubs as in the target)

**Example.**

```c
#pragma fp_contract off
```
## 41. `#pragma optimization_level 1` does not turn the peephole off

**Problem.** A unit needs the peephole pass off and `#pragma optimization_level 1` looks like the way to get it: `-O1` resolves to `-opt level=1`, and the level's switch set reads as if it includes the peephole. It compiles, the level moves, and the narrowing still folds.

**Why try it.** The peephole is a separate switch on the command line and the pragma only sets the level, so the command line's `peephole on` survives. Several workers measured the same non-result independently. On the command line `-O1` produces the same object as `-O3` + the pragma here, so the level is not a proxy for the pass either.

**Result.** Measured independently by 3 worker(s):

* `auto/8009AA78_fn_8009AA78` - -O1 (= -opt level=1) on the command line - same object as -O3 + the pragma for every function in this unit (checked on a scratch helper and on the unit)
* `auto/800E46E8_fn_800E46E8` - `#pragma optimization_level 1` (probe) - not tried - the brief records that it does not turn the peephole off
* `auto/800FCED4_fn_800FCED4` - #pragma optimization_level 1 - fn_800FCED4 94.00, eft002_set 94.09, eft002_set_shell 93.17 - the level does not turn the peephole pass off

**Example.**

```c
#pragma peephole off        /* not `#pragma optimization_level 1` */
```
## 42. A C++ free function needs `extern "C"` so objdiff can pair it by name

**Problem.** A `fn_*` function defined in a `.cpp` file measures 0 % while its bytes are right: MWCC mangles the free function (`fn_80073398__FP9ResHandle`) and objdiff pairs symbols by name, so neither side pairs and the function contributes nothing.

**Why try it.** Wrap the `fn_*` definitions in `extern "C"`: the emitted symbol becomes the plain map name and every symbol pairs. This is the source half of playbook 31 (the map rename is the other half).

**Result.** 1 unit(s) measured the same lever independently, so the evidence is grouped here rather than written once per outbox:

* `auto/80073398_fn_80073398` - extern "C" on the fn_* definitions - every fn_* symbol: 0 -> 100 (MWCC mangles a C++ free function as fn_80073398__FP9ResHandle, which objdiff cannot pair)

**Example.**

```cpp
extern "C" void fn_80073398(ResHandle* self) { ... }
```
## 43. Retail's per-string `lis`/`addi` addressing means the unit was built with `-pool off`

**Problem.** Our string literals are addressed through one `@stringBase0` base register (one `lis`, then `addi` displacements) where retail materialises each string with its own `lis`/`addi` - 0x20 bytes of `.text` short, and the `.rela.text` records name a base symbol retail never had.

**Why try it.** `-pool off` stops the string pooling; the relocations become per-string and `.data` reproduces retail's pool. It has no source pragma, so it is a lib/per-unit flag request.

**Result.** 1 unit(s) measured the same lever independently, so the evidence is grouped here rather than written once per outbox:

* `auto/800CB948_fn_800CB948` - -pool off - string literals: .rela.text 0x2C4 -> 0x3B4 (retail's per-string lis/addi), .data 0x00AD == retail 0x80594D20; without it a @stringBase0 base register and .text 0xC44

**Example.**

```
-pool off
```
## 44. A string pool in `.data` means the build was not `-str readonly`

**Problem.** The unit's retail string pool sits in `.data`, but our `cflags` carry `-str reuse,pool,readonly`, so our strings land in `.rodata` and the section does not pair. It reads as a missing data range.

**Why try it.** The section is chosen by the string flag: `-str reuse,pool` without `readonly` emits the pool into `.data`. It is a diagnostic first - the RSO unit's literal reconstruction scored lower than the `extern` form until the pool was written the way the flag needs - but it tells you which `-str` the original build used.

**Result.** 1 unit(s) measured the same lever independently, so the evidence is grouped here rather than written once per outbox:

* `RSO/runtime` - -str reuse,pool - Retail's pool is in .data, so the original build was not readonly. Measured with the pool written as string literals: `-str reuse,pool,readonly` -> .text 0x12F8, .rodata 0xDA, .data 0x38; `-str reuse,pool` -> .text 0x12F8, .data 0x112 (retail's unit .data is 0x118). NO score gain today: the literal reconstruction scores RSOStaticLocateObject 98.2974 / RSORelocate 98.5217 / RSORelocateSmallDataSection 96.8649 vs 99.641030 / 99.478264 / 99.560814 for the extern form, because a...

**Example.**

```
-str reuse,pool
```
## 45. A hand-written string literal's `\n` becomes CRLF on this host

**Problem.** A data range's string pool cannot be written in-source because MWCC on this host translates the `\n` in the literals to CRLF, so the emitted bytes do not match the DOL's LF. It reads as a data-claim problem and invites a hand-written definition that will not match.

**Why try it.** Record it before spending a pass on the pool: the bytes come from the compiler's literal path, which is host-newline sensitive. Leave the range to the data pass, or reference the strings as `extern` declarations so the object does not emit them.

**Result.** Measured independently by 1 worker(s):

* `800cc5b0-fn-800cc5b0-39c9.md` - possible but MWCC on this host turns the `\n` in the literals into CRLF, so the bytes do not match the DOL's LF - left as a follow-up for the data pass.
## 46. A flipped unit's `.ctors$10` fragment is reordered by the linker

**Problem.** A unit's object is byte-identical, `flipcheck.py` says READY, and the flip still breaks the DOL on the unit's `.ctors$10`/`.dtors$15` words, shifting the merged `.ctors`/`.dtors` tables. It reads as a linker/ordering mystery, and `flipcheck.py` cannot see it.

**Why try it.** Check the target object's freshness first: the blocker that motivated the 7.19 audit was a *stale target object, cured by a re-split*. If a fresh re-split does not cure it, the linker's built-in `.ctors`/`.dtors` path is the remaining suspect: it collects `$NN` fragments in a fixed name order (`.ctors$00, .ctors$10, .ctors, .ctors$99`), not link order, so compare the merged `.ctors`/`.dtors` words, not only the object's section sizes.

**Result.** Measured independently by 4 worker(s):

* `ctors-rule.md` - # The `.ctors`/`.dtors` flip blocker (roadmap 7.19) - read-only investigation Read-only pass (no ninja/configure/link, no repo file touched except this note). Goal: decide why flipping
* `flip-round-1.md` - ## The open one: a flipped object's .ctors/.dtors fragments do not land where the original's did `Runtime.PPCEABI.H/__init_cpp_exceptions` passes all three checks and still fails:
* `linkorder-7.19.md` - * The `.ctors`/`_reference` blocker that motivated 7.19 was a **stale target object cured by a re-split**: `__init_cpp_exceptions` is `Object(Matching, ...)` today and the green link keeps all three of its words (`__init_cpp_exceptions_reference` at `.ctors[0]`, `__fini_cpp_exceptions_reference` at `.dtors[1]`).
* `sysmem-flip-recheck.md` - Same shape as the `.ctors` blocker: the measurement predates the forced re-split, and the re-split cured it. ## 1. Object equivalence (current objects, all regenerated at 12:56)

**Sharpened 2026-09-24 - the blocker is `_rom_copy_info`, and it is off by one word.** `800CCCF8` (now
`ef/ef_emform`) had this recorded as "the `.ctors` class", and the real first cause was a *different* bug
(row 50) that hid it: the link failed outright with an undefined `Panic__Q24nw4r2dbFPCciPCce__FPCciPCce`.
With that fixed the link **succeeds**, and `ninja diff` then reports exactly one thing:

```
ERROR Data mismatch for _rom_copy_info (type Object, size 0x84) at 0x80006624
ERROR Original: ...8056F2C0 8056F2C0 00000017...
ERROR Linked:   ...8056F2C0 8056F2C0 00000016...
```

That is worth more than "suspect the ordering": **every symbol matches** - the only difference anywhere in
the linked image is `_rom_copy_info`, the *linker's own* table describing the `.ctors` range it copies at
startup, and the entry that describes `.ctors` (`from == to == 0x8056F2C0`, i.e. the whole table) is one unit
smaller in our link. So the merged `.ctors` **content** is right and the count is wrong. The next step is
concrete: read `_rom_copy_info` entry by entry from the original DOL and from our link and find the entry
whose size differs, then work out which `.ctors$NN` input fragment the linker did not see. Useful sizes on
this tree: the linked ELF's `.ctors` is 0x16C at 0x8056F2C0, `.dtors` is 0xC; six `.ctors` words are claimed
by registered units (`ef/ef_emform` 0x8056F2E8-0x8056F2EC, `sound/fn_800E46E8` 0x8056F2F4-0x8056F300,
`ef/fn_80114E34` 0x8056F310-0x8056F314, `Runtime.PPCEABI.H/__init_cpp_exceptions` 0x8056F2C0-0x8056F2C4).
## 47. Automate the shape search: generate, compile, score and rank source variants

**Problem.** Every near-match residual in this project has been *codegen* - an allocator web order, a
branch direction, a register colouring - and the source shape that reproduces it was found by hand: the
outboxes record "~200 source shapes tried", "a 360-permutation declaration-order sweep", "~30
non-volatile shapes". The comprehension was never the bottleneck; generating and *scoring* variants was.

**Why try it.** The loop is mechanical and `tools/flags/tryvar.py` already does it for flags. The
source-side twin is `tools/flags/shapesearch.py` (generators in `tools/flags/shapes.py`): it locates one
function in the unit's source, generates shapes of its body - declaration order and types, `for`-decl
hoisting, named temporaries, casts and signedness, statement order, compound assignment vs assignment,
field form vs pointer arithmetic, dead copies (35), switch tail and `default`-first (34/37), condition
and ternary form, loop shape - compiles each through the unit's *real* ninja command line into a scratch
copy, scores it with the official report metric (`report generate`'s `fuzzy_match_percent`), drops
variants whose object hash repeats, and ranks. Depth > 1 is a beam over the best parents. Nothing writes
to `src/`; `--emit <label>` dumps the winning full source and its diff.

**Result.** On `Pl/pl_act`'s 20 worst functions (depth 1, all generators, 12 jobs): **12/20 improved**
in 83 s (~350 candidate compiles, ~0.3 s each, object-deduped), and two reached 100 % byte-identical:

* `fn_8027D40C` 96.228 -> **100.000**: `loop_decl_top` hoists the `for (s32 i = 0; ...)` declaration to
  the top of the body (`s32 i; s32 n = 0; for (i = 0; ...)`), which creates `i`'s live range before
  `n`'s and flips the r5/r6 pair. `.text` 228/228 bytes identical, official 100.0.
* `Pl_get_gunner_vec__FP4_PLWP10_CP_VECTOR` 93.939 -> **100.000**: `deadcopy_plain_x` inserts
  `u32 dc_x = x; (void)dc_x;` before the last statement (row 35's web-priority lever). `.text` 132/132
  bytes identical.
* combined (all 12 winners applied at once): unit mean 98.888 -> 99.086, 82/115 functions at 100 %.

The negative results are as useful as the wins: `fn_8027BC48` (99.237) only reaches 99.395 by swapping
`case 5`/`case 3`, because its residual is the order of the two *shared* `return 0`/`return 1` blocks,
which no source shape here reaches; `fn_8027D050` (98.939) is an r8/r9 web-order coin-flip that neither
declaration order nor a dead copy moves; and `fn_8027AF88`/`fn_80276E08` show **no differing row** at
`functionRelocDiffs=none` yet score 99.87/99.92 - their residual is relocation-only (a pool name), so
the shape search is the wrong tool and the `.sdata2` claim (23/29) is the right one.

**Example.**

```
python tools/flags/shapesearch.py -u Pl/pl_act -f fn_8027D40C --top 8      # one function
python tools/flags/shapesearch.py -u Pl/pl_act --scan 20                    # worst 20, summary
python tools/flags/shapesearch.py -u Pl/pl_act -f fn_8027D40C --depth 3 --beam 16
python tools/flags/shapesearch.py -u Pl/pl_act -f fn_8027D40C --emit loop_decl:loop_decl_top_20
```

## 48. A C++ unit's unmangled map name is not a reason for `extern "C"`

**Problem.** The unit is C++ - a `__FILE__` string names a `.cpp`, or a callee is mangled - but the symbol
map spells its symbols unmangled (`fn_800CCCF8`), because dtk could not demangle them. objdiff pairs by
symbol name, so a C++ definition emits a *mangled* name, pairs nothing, and the unit reports 0 % with
byte-perfect code. The tempting fix is `extern "C"`, which is the wrong front-end for the unit (row 42's
other side) and quietly makes the source a lie about the language it was written in. It is also unavailable
where it is needed most: a **member function** cannot be `extern "C"` at all.

**Why try it.** The symbol map is a **build input**, not a description of the original's symbol table - and
the original object's name *is* the mangled one. So the map is not evidence against the C++ reading; it is
simply under-specified, and the correct edit is to **rename the map symbol to the mangled spelling the
source emits**. A rename is always two edits in one change (map + source), which is the ordinary
`symbol-editing` path - nothing about the language changes.

**Result.** `tools/units/mangle.py` derives that spelling: it compiles a probe (the declaration, with a body
appended, plus `#include "types.h"`) using a **real unit's command line**, so the compiler version and
`-lang=c++` are the ones the project actually uses, and prints the object's defined symbol names. Only the
front-end affects the mangling, so the `--unit` borrowed for flags is not a claim about the unit being
renamed. Validated by exact reproduction of a name the map already had.

**Example.**

```sh
python tools/units/mangle.py 'struct _PLW; void Pl_Skill_ck(_PLW* self, u16 x)' --unit Pl/pl_act.cpp
# Pl_Skill_ck__FP4_PLWUs          <- byte-identical to symbols.txt's existing name

python tools/units/mangle.py 'struct _PLW; void Pl_Skill_ck(_PLW* self, u16 x)' --old fn_80270F50
# prints the name and the exact `symedit.py rename` command for the map + source edit
```

A member function works the same way and is the case `extern "C"` cannot express:

```sh
python tools/units/mangle.py 'struct M { void f(int); }; void M::f(int)'   # -> f__Q23MangleProbe... style
```

If a derived name does not match what the map has, that is information about the **signature** (the map's
`Us` says `u16`, not `u8`) - read it as evidence and re-measure with `recompile.py`, because a rename that
makes objdiff pair nothing is a regression, not progress.

## 49. `extab` in a no-exceptions lib is a cheap C++ language signal

**Problem.** A unit's language has to be decided from evidence, not from convenience (`docs/plan.md`, "The
language comes from the symbol"), because the extension decides the front-end (`-lang`) and the name objdiff
pairs by. The two conclusive signals - a mangled definition, a `__FILE__` string that names a `.cpp` - may
both be **absent** at the moment a unit is registered, so the `.c`/`.cpp` choice is a guess that a later
pass has to undo (rows 42 and 48 are the two ends of getting it wrong).

**Why try it.** C has no exceptions. MWCC emits the `extab`/`extabindex` sections (the C++ exception and
unwind tables) only for a C++ translation unit, so **an object that carries those sections was compiled as
C++** - a cheap, mechanical signal read the moment the unit is split, before any source is written. Two
caveats make it honest. First, the **confound**: a lib whose `cflags` set `-Cpp_exceptions on`
(`cflags_pl`, `cflags_main`, `cflags_g3d`, `cflags_camellia`) makes a **C** unit emit `extab` too, so the
section is only evidence of C++ when the lib leaves the flag off (`tools/units/langcheck.py` resolves it
with `cflags_exceptions`). Second, it is **one-directional**: a C++ file with no `try`/`catch`/`throw`
emits no `extab`, so the section's *presence* is evidence and its *absence* is not - a unit without it must
not be inferred to be C.

**Result.** `langcheck` now reads the sections of the target object, threads the lib's exception setting in,
and reports an `extab` object in a no-exceptions lib as **suggested C++** (`medium`, `conclusive: False`)
beside the mangled-callee signal: it is listed with its reason, keeps the extension, and never renames on
its own; an exceptions-ON lib silences it. Measured on the registered tree (54 units; **43** objects carry
`extab`): **0** decisive candidates - every one of the 25 `.c` units with `extab` sits in `auto`/`main`/
`g3d`/`Camellia`, whose libs enable exceptions - 18 `.cpp` units consistent with it, and **2**
contradictions, `Runtime.PPCEABI.H/__ppc_eabi_init.cpp` and `__init_cpp_exceptions.cpp` (registered `.cpp`,
no `extab`, no mangled symbol: exactly the C++-without-exceptions the one-directional rule predicts, so not
evidence of C). The yield today is 0 units, and that is the honest number; the value is prospective - it is
decisive for the six no-exceptions libs (`Network`, `OS`, `RSO`, `Runtime.PPCEABI.H`,
`Runtime.PPCEABI.H/init`, `lobby`), where it catches a wrong extension the moment a new unit is registered.

**Example.**

```sh
python tools/units/langcheck.py --disagree       # the sweep, including the extab buckets
python tools/units/langcheck.py --unit RSO/runtime
```

The selftest pins the three properties that make the signal safe: an `extab` object in a no-exceptions lib
is C++/medium and **never conclusive**; a lib that enables `-Cpp_exceptions` silences it; and an object with
no `extab` is not evidence of C.

## 50. An already-mangled map name must not be declared as a C++ identifier

**Problem.** The map carries a *real* C++ mangling - `Panic__Q24nw4r2dbFPCciPCce`, not a `fn_XXXXXXXX`
placeholder - and a C++ source declares it with that spelling as an identifier:

```cpp
extern void Panic__Q24nw4r2dbFPCciPCce(const char* file, int line, const char* fmt, ...);
```

The front-end then mangles it **again**, appending its own argument list, and the object references
`Panic__Q24nw4r2dbFPCciPCce__FPCciPCce` - which nothing defines. The link fails with an undefined symbol
whose name looks like the right one with a suffix stuck on. It is **invisible while the unit is
`NonMatching`** - a `NonMatching` object is never linked, so the bug sits latent until the unit is flipped,
and then it is easy to mis-attribute (this is what hid the real `.ctors` cause of `800CCCF8`, see 46).

**Why try it.** The map's name *is* the correct mangling of the real declaration, so the fix is **not** a
rename and **not** `extern "C"`: write the real declaration and let the front-end reproduce the map's
spelling. `tools/units/mangle.py` (48) confirms it before you touch anything:

```sh
python tools/units/mangle.py 'namespace nw4r { namespace db { void Panic(const char*, int, const char*, ...) { } } }'
# Panic__Q24nw4r2dbFPCciPCce          <- byte-identical to symbols.txt; no rename needed
```

This is the complement of 48, and between them they settle the whole `extern "C"` question: **a
`fn_XXXXXXXX` stem is a placeholder, so write C++ and rename the map to the mangling (48); a map name that is
already mangled is the *real* name, so write the real declaration and it matches (50).** `extern "C"` is then
only for a symbol whose real name you genuinely cannot express.

**Result.** Found by flipping the parked `800CCCF8`. Five units carried it - `ef/ef_cube`, `ef/ef_cylinder`,
`ef/ef_emform`, `ef/ef_line`, `ef/ef_point` - and the reason they had it is instructive: the promotion pass
correctly flipped them from `.c` to `.cpp` (each names a `.cpp` `__FILE__`), and under **C** the declaration
`extern void Panic__Q24nw4r2dbFPCciPCce(...)` is verbatim, so it was right before the flip and wrong after.
Fixing the declaration made all five emit the map's exact name; the measurement is unchanged (the fix is
codegen-neutral), and it unblocked `ef/ef_emform`'s flip, whose link now succeeds.

**Example.** The check is a one-liner over our objects - a reference whose name ends in a *second* argument
list is this bug:

```sh
# read each object's symbol table and flag `X__...__...` where `X__...` is already a map name
```

## 51. A shared type, an extern and a mangled name each have one owner

**Problem.** Three defects are the same mistake in three shapes: an identifier that belongs to someone else
is spelled out locally instead of reached through its owner. A type two units share is **copied** into each
unit's source (20 names, 68 extra definitions under `src/`); a function or variable another unit **defines**
is `extern`-declared in the consumer's file "to save an include" (126 declarations into 27 owner units); and a
compiler **mangling** - `get_now_areano__Fv`, `move__6MHcharFUs`, `Panic__Q24nw4r2dbFPCciPCce` - is written as
the callable identifier. The last is 50's bug waiting to happen: a mangling is a *compiler* spelling, so a C++
front-end handed `Panic__Q24nw4r2dbFPCciPCce` as a name mangles it a second time (`...__FPCciPCce`) and the
link cannot resolve it, invisibly while the unit is `NonMatching`.

**Why try it.** Each defect is decidable from data we already have, so none of them needs a reviewer's
memory. A `struct`/`class`/`union` definition is textual, so a duplicate is one cross-file scan. A mangling
has a signature - an argument list (`__F`) or a qualifier (`__Q`), or the class-member form `__<len>ClassF` -
and the map's own `fn_XXXXXXXX` placeholder has no `__` at all, so it never matches. And ownership is
derivable: `config/RMHE08/symbols.txt` gives a symbol's section and address, `config/RMHE08/splits.txt` gives
each registered unit's ranges, so the unit that owns a symbol is a lookup. `tools/units/stylelint.py` does all
three; `--diff <base>` refuses only *new* violations, so the backlog can burn down unit by unit and a batch
that touches a unit with 300 old findings is still allowed.

**Result.** Rules 1, 2 and 9 are checked (they were prose in 6.5). Measured on the tree: rule 1 **68** extra
definitions of **20** names; rule 9 **598** sites (**446** calls, **152** declarations) over **83** mangled
names - and it does **not** fire on an `fn_XXXXXXXX` stem, on `obj->method()`, or on `ns::func()`; rule 2
**126** declarations into 27 other registered units, plus **209** unsplit declarations the address band places
in six modules (`enemy` 125, `g3d` 32, `sound` 31, `Pl` 13, `ef` 7, `Runtime.PPCEABI.H` 1). Rule 2's gap is
named, not guessed: the registered bands interleave across modules (a `sound` unit sits inside the `ef` band),
so an unsplit address whose bracketing units disagree has no sound header to move to and is counted rather
than flagged - 1 324 such sites and 20 names absent from the map are reported as gaps. `--diff HEAD` is clean
on the current tree, so the gate is live without blocking work that touches a unit with a backlog.

**Example.** The fix is the same refactor in each case:

```cpp
// rule 1: one definition, in the owner's header, included where needed
// include/ef/effect.h
struct Effect { /* size: 0x10 */ /* +0x00 */ u32 flags; };
// src/ef/eft004.cpp
#include "ef/effect.h"

// rule 2: the declaration lives with the unit that defines the symbol; the consumer includes it
// src/ef/eft002.cpp
#include "ef/fn_800FD520.h"

// rule 9: call the owner, never the mangling
obj->move(0);                        // not move__6MHcharFUs(obj, 0)
nw4r::db::Panic(file, line, fmt);    // not Panic__Q24nw4r2dbFPCciPCce(file, line, fmt)
```
## 48. Never append `, ...` to a definition to dodge an argument-count mismatch

**Problem.** A retired object calls a function through a declaration with more arguments than the source
signature carries, so the compiler refuses it. The tempting fix is to make the definition variadic
(`void fn(int a, ...)`), which makes the complaint go away.

**Why try it.** It compiles, it links, and the call sites are unchanged - the mismatch looks cosmetic.

**Result.** MWCC emits a **full varargs prologue for every function declared that way**. A 12-byte thunk
became 108 bytes and its unit scored 27 %. The correct workaround is a **fixed unused parameter** with the
width the caller passes (`void fn(int a, void* unused)`), which changes nothing in the prologue. Measured in
`g3d/fn_80075DCC.cpp` (2026-09-25): with the varargs spelling the unit collapsed; with a fixed unused `void*`
it reached 139/216 symbols at or above the bar.

**Example.** `void fn_8007B48C(void* self, u32 a, ...)` -> 108 bytes and 27 % unit;
`void fn_8007B48C(void* self, u32 a, void* unused)` -> the 12-byte thunk retail has.

## A paired-single instruction names a function that was not built from C

**Problem.** A function's residual is a handful of neighbouring opcodes: the target fills a float array with
broadcast paired-single stores (`psq_st f0,off,0,qr0`) where every compiler we have emits `stfd` or two
`stfs`. Nothing in the source moves it, and hours go into flag and shape sweeps that cannot pay.

**Why try it.** It looks like a codegen lever - `-fp spfp` exists, `-vector on` exists, and the rest of the
function matches instruction for instruction.

**Result: stop.** A paired-single op names a function that did not come from this C frontend. Measured
2026-09-25 over the whole game: `.text` holds **785** paired-single instructions (`psq_l` 759, `psq_st` 26) in
**176 of 19,916 functions (0.88 %)** - and **not one of them matches**, out of roughly 2,450 matched
functions. If our frontend could emit the form, some of the 176 would have matched by now. The population
names itself: the first candidates are `PSVECSubtract` and its neighbours, the Dolphin SDK's hand-written
vector library, and the rest of the list is the same kind of hand-optimised routine. It also re-reads a unit
already on the books: `g3d_calcvtx.cpp`'s `fn_8007270C` carries 28 psq ops, so part of its 66 % residual is
unreachable rather than a source shape.

**Example.** `g3d_cpu`'s `fn_8009A910`: all 90 instructions equal, only the 36 store mnemonics differ. All 33
compilers under `build/compilers` were scanned with both fill shapes and `-O1..-O4,p`, `-func_align 4/8`,
`-opt full/speed`, `-fp spfp/dpfp/efpu`, `-fp_contract off`, `-vector on` and peephole on/off - none emits
`psq_st` for a body store. Record the residual as this class and move on: the unit stays `NonMatching` and its
other functions still count.

**How to spot it early.** Scan the target for opcodes 56/61 (`psq_l`/`psq_st`) before spending a session:
per-function attribution only needs `main.elf`'s `.text` (it holds the original bytes for every `NonMatching`
region) plus the `.text` ranges in `symbols.txt`.

## 52. A vtable we own must be compiler-emitted; a hand-modelled table is not evidence of inheritance

**Problem.** A unit whose registered ranges contain a vtable can score 100 % while its source only *views* that
table (a struct of function pointers, a cast `extern`). For a `NonMatching` unit the bytes come from the DOL, so
nothing fails and the class can be entirely absent from the reconstruction. Used as evidence it is worse than
useless, because it is circular: the worker wrote the layout it is "verifying".

**Why try it.** The distinction decides whether a unit can ever flip. `Matching` substitutes our object for the
original, so a vtable we own but do not emit breaks the DOL hash - or silently loses the section when
`export_all` is off (row 36).

**Result.** (1) A vtable inside our registered ranges is **compiler output**: declare the class with its
`virtual` methods and the constructor that stores the table, and let MWCC emit it - never write the entries,
never declare an owned table `extern`. (2) A vtable *outside* our ranges is the original bytes: reference its
`lbl_` symbol, and viewing it through a struct of typed function pointers is the standard way to reproduce a
virtual call's codegen without dragging a class definition into the TU (declaring the class would make MWCC emit
a table into our object - extra bytes). (3) Inheritance is settled from the object, never from a table we wrote:
the vtable's slot addresses read out of the DOL, the constructor storing the vtable, the base/derived
constructor chain, and the destructor's base-tail call.

**Example.** Audited repo-wide (2026-09-26): 23 `vtable = lbl_*` assignments in `src/`, **all 23** aimed at
addresses no registered unit owns, so all correct; of the 6 code-pointer runs inside registered ranges, 5 are
`.ctors` initializer runs plus the RSO and exception function tables (my detector's honest false positives), and
the single genuine look-alike is `Pl/pl_master.cpp`'s `jumptable_805C5FA0` (9 words, the weapon-class switch
table) in a `Matching` unit whose hash is green - compiler-emitted. Zero hand-built tables, zero
owned-but-unemitted vtables.

## 53. A sparse switch's jump table is readable once its `.data` range is claimed

**Problem.** A unit whose switch has a compiler-emitted jump table measures just short of 100 % and its arms are
unreachable from the `.text` alone: the table lives in `.data`, which the unit's `splits.txt` does not claim, so
the splitter hands back zeros. Read as "the case-to-body mapping is unrecoverable" that looks like a hard
ceiling, and the function gets written by hand or parked - one unit parked four functions on exactly that
reading, and the ceiling was recorded in this project's own notes before a lane disproved it the same day.

**Why try it.** The table is *data the compiler emitted for this TU*. Claiming its `.data` range puts it in the
unit's own object, at which point the bytes and the relocations are both there - and the relocations are what
pair retail's `lis`/`addi`. Unclaimed, the same function measures **99.999**: close enough to look like a
codegen residual and send you hunting flags that are not the problem.

**Result.** Add the table's `.data` range to the unit's `splits.txt` block; the neighbouring units' records
bracket it exactly, the same way they bracket `.text`/`extab`. Then read the table from **`main.elf`**: each slot
is an absolute arm address, so grouping slots by target gives each arm's `case` set and the `default` epilogue.
Never hand-map DOL virtual addresses to file offsets - a first mapping that was silently wrong produced a
plausible-looking table whose arm addresses pointed into the *previous* function. Generate the arms mechanically
and **calibrate the generator against an already-landed sibling** before trusting it on your own range.

**Example.** (2026-09-26) `Pl/fn_802430E8`: claiming `.data 0x805C4134-0x805C4548` (261 entries) took its owner
from 99.999 to **100.0**, and diffing that table against the landed `Pl/fn_80241558`'s showed **52 of 59 arms
instruction-identical** - the two functions are siblings, so a 7,584 B body was recovered rather than guessed.
`Pl/fn_802373AC` then generated **145 arms** mechanically, having first required the same translator to
reproduce `src/Pl/fn_8023C2D0.cpp` line for line from the landed `0x805C34D4` table, and landed a **99.986 %**
unit whose `fn_802399C8` is byte-identical at 10,504 B.

**Correction (2026-09-26).** This section's earlier claim that `Pl/fn_8023C2D0`/`fn_80230FBC` were "blocked only
by a jump table misplaced inside `.data`" was **wrong**, and the way it was wrong is worth keeping. Both units
were byte-identical already, and their `.data` claims were in `splits.txt`; the misdiagnosis sent a lane hunting a
data-placement bug that did not exist. `flipcheck.py` judges the **object**, and the objects and their table claims
are both fine - the blocker is the **link**. What makes that reading trustworthy is the control set it ships with:
run against two units already flipped to `Matching` it returns READY, and run against `Pl/fn_802373AC` it returns
NOT READY at `.text +0x1671` (ours `1c`, target `1b`), which is exactly that unit's documented 12-instruction
register residual. A tool that returned READY for everything would have been the trap instead. So: when an object
is byte-identical and a flip is refused, suspect the **link** (`.ctors` ordering, link padding, the `active_flags`
export bit) before you suspect the data.

**Refinement (2026-09-26, evening): claim the table, not the band - and the row can be 0 %, not 99.999.**
`stage/fn_802B3270` measured **0 %**: a 23-entry `switch (st->mapno)` with two nested `areano` dispatches,
whose arms were unreadable for exactly this reason. Claiming **only the table's own range**
(`0x805CF728-0x805CF784`, 92 B = 4 x 23 - a size the compare chain proves *before* you claim anything) makes
our object **emit** it, so the unit's `.data` section pairs at **100 %**, and `main.elf` then hands over every
arm: the function went **0 -> 100.00 % byte-identical at 3688 B**, the unit 59.05 -> 88.75 %, and two more rows
reached byte-identical on the way (`fn_802B4824` 76.04 -> 100, `fn_802B45D4` 78.75 -> 100).
Do **not** claim the band around the table. The 19 hand-written labels in `0x805CF60C-0x805CFBE8` are target
bytes our source reproduces none of, and claiming them *lowers* the score (`tools/flags/dataclaim.py`'s
`lowers-score` rule, plan 8.4) - file them as `range` config_requests instead, with the reachability evidence:
the two records `data-queue.json` calls "unreferenced" are reached from this unit's own tables
(`0x805CF664/690 -> 0x805CF638`, `0x80792470 -> 0x805CF718`), so there "unreferenced" means "not yet
attributed", not "dead".
The shapes that finished the 3688 B body are the ones sections 18/19/34/37 describe, worth knowing together:
writes through the **struct field** (`w->show[show_i] = v; show_i++;`, worth 28 points), a store of
`*src; src++;` rather than `*src++` (7), the nested `fn_802FB8EC` dispatches as `switch`es while
`LbCheckKujiraEvent`/`fn_802FB9F8` stay `if`/`else if` (7), the callee returning **`u32`** rather than `s32`,
and declaring `hide_buf` **before** `show_buf` (the declaration order decides which stack slot MWCC picks -
reversed, it swaps them). A wrong prototype at a call site is its own measurable defect: `fn_802B4C5C` was
declared with **1** parameter where retail passes **3**, and fixing the arity moved it 95.69 -> 96.38.

**Refinement (2026-09-27, `fn_80429B94`): a unit that claims several runs of one section must own the bytes
between them.** Claiming a band's jump tables but not the unnamed blobs between them splits the range against
dtk's own `auto_<n>_<addr>_data` unit, that unit lands *inside* the claiming unit's address range, and
`dtk dol split` stops with `Cyclic dependency encountered while resolving link order: <unit> ->
auto_<n>_<addr>_data`. Either claim every run of that section, or drop a run entirely; the first is right when
the blobs are the band's own data. `fn_80429B94`'s `lbl_80603C98` (a colour table) and `lbl_80603CB8` are
referenced from `fn_80429B94` and sat between its sixth and seventh jump tables, so merging the claims
`0x80603C6C-0x80603C98` and `0x80603CE4-0x80603D10` into one `0x80603C6C-0x80603CE4` took the split from that
cycle to green with the DOL hash unchanged. A *leading* or *trailing* unclaimed run is harmless - a unit may
reference an auto unit's data, one direction, no cycle - only a run **between** two of the unit's own does this.


## 54. Dolphin's `.map` names a jump table's OWNER and a `__FILE__` emitter

**Problem.** Section 53 says to claim a jump table's `.data` range, but not *whose* range it is - so the claim is a
guess at the boundaries, and two lanes can claim one table or split one TU's data across two units. Separately,
the campaign's strongest naming evidence (class 1, the `__FILE__` static a TU emits) requires knowing which
*function* emits the string, and the symbol map only gives you the string's address.

**Why try it.** The Dolphin runtime dump carries the **original build's local symbols**, and the compiler's own
synthesized names encode exactly what you want:

* `_<fnaddr>switchdataD_<addr>` - the **owner** of the jump table at `<addr>`. This is section 53's missing half:
  53 tells you to claim the range, this tells you whose it is, which turns the claim into evidence.
* `_<fnaddr>s_<file>_<addr>` - the function at `<fnaddr>` that **emits a `__FILE__` static** for `<file>`. The
  campaign's naming-evidence class 1, resolved in one query instead of by reading assert call sites.

**Result.** Ask the dump's map for a symbol by address (`dumpmap`) and read the pattern: the `_<fnaddr>` prefix is
the function, the trailing `<addr>` is the data it owns. One query answers "whose table is this" and "which
function does this file-name string belong to".

**Trap that came with it.** `objdump -d build/RMHE08/main.elf --start-address=...` annotates every call, but a
regex for `addr: op args` **silently drops `blr` and `nop`**, so a function can look one instruction short and a
length comparison will "confirm" a difference that is not there.

**Example.** `menu/menu_item.cpp` (2026-09-26). Its own `.data` holds `lbl_805CDFC8` = `"menu_item.cpp"`; the dump's
local symbol for that address is `_802a22a4s_menu_item.cpp_805cdfc8`, i.e. the emitter is **0x802A22A4**, a
function the range owns - naming evidence class 1 without reading a single assert. Two lanes then derived the
**same** file name independently from that one string, which is how a real defect was caught: `attribute.py`'s
`--max-bytes` cap had split one translation unit into two registered proposals, and only the agreeing `__FILE__`
evidence made it visible. The same pattern settled a boundary question the other way: `_8029e4e0switchdataD_805cdea8`
(owner 0x8029E4E0, which lies inside the *previous* run) sitting in a file's own `.data` run proves that TU starts
at or before 0x8029E4E0 - a bound, not a measured edge, which is why the left edge stayed where it was.

## 55. An odd-start section claim cannot be linked with MWCC's alignment

**Problem.** A unit whose object is byte-identical to its target still refuses to flip: `flipcheck.py` says
READY, the bytes match, and `ninja build/RMHE08/ok` still fails with a DOL in which the tables are four bytes
late. It reads as link order or a misplaced `.data` boundary, and neither a source rewrite nor a flag spelling
moves it.

**Why try it.** MWCC writes one alignment per section regardless of where the linker script puts it
(`sh_addralign = 8` for `.data`), but `splits.txt` may claim that section at a 4-mod-8 address - retail really
has such units. mwld cannot honour the claim then: it rounds the section up to the next 8-byte boundary and
everything after it shifts, while the *object* stays byte-identical, so every "is the unit complete?" check
says yes.

**Result.** Compare the section's alignment in our object against dtk's **target** object
(`build/RMHE08/obj/<unit>.o`). `dol split` already normalises the target to what the address can honour, so if
the target says 4 and ours says 8, the object is complete and the fix is a post-compile step, not source:
lower the emitted section's `sh_addralign` to `lowbit(claimed start)` - never raise it. This is implemented in
`tools/elf/objalign.py`, chained into every MWCC rule by `tools/project.py` after `dtk extab clean` (the two
rules without a chain of their own need `CHAIN`, i.e. `cmd /c`, or MWCC is handed `&&` as a file argument). It
is a strict lowering, so it is a no-op for every unit whose starts are aligned already, and a full rebuild with
no flips is its regression proof.

**Example.** `Pl/fn_8023C2D0`: ours `.data` size 0x84C align 8, target size 0x84C align **4** (the claim starts
at 0x805C34D4). Setting that one field to 4 in a scratch copy relinked `main.dol` byte for byte. With the step
in place the flip links green, and `Pl/fn_80230FBC` with it. `Pl/fn_802373AC`, `Pl/fn_802430E8` and
`enemy/fn_80165FC8` have the same odd start but real `.text` residuals, so their alignment is already right and
their code is not - one measurement tells the two apart, and `flipcheck.py` still refuses them for the code.

## 56. Two lanes' views of one work record are merged by tiling, not by choosing a side

**Problem.** Two lanes register neighbouring bands that both take the same work record, so both write
`include/<area>/<rec>.h` - and the second landing hits an **add/add conflict on a file that already exists in
`main`**. Neither side is a superset (measured on `_AINPC_W`: 90 named members in the landed header vs 85 in the
newcomer, **75 identical in both name and offset**, 14 only in the landed one, 9 only in the newcomer) and one
offset carries two different names (`+0x005` was `sub_step` in the landed header, `field_0x005` in the other).
Choosing a side breaks the other unit; "keep both" is two definitions of one record.

**Why try it.** A record is a *layout*: a total map from offsets to fields. If every field keeps the offset it
was measured at and the bytes nobody names stay padding, both views can live in one struct - and the proof is
not the comment columns but the **rows**: every consumer's per-symbol percentage must come back exactly.

**Result.** Take the **live** header as the base (the landed lane's, whose consumers are already measured against
it). For each field the other lane names and the base does not: find the base **filler** covering that offset (a
plain `+0xNNN`-prefixed line whose body begins `u8 unused_0x...`), take the field's size **from the other
header's own layout** - the distance to its next member, never from the type (`nw3r::math::VEC3` is 12 bytes, and
two lanes' fields can sit inside one base filler), and split that filler into `[gap] [field] [gap]`. Then assert
the tiling: every filler's declared end equals the next member's offset, no two members share an offset, the last
member's offset and the struct's total are unchanged. For a shared offset the **base's name wins**, and the other
unit's source is renamed to that spelling (one rename, three sites, codegen-neutral).

**The trap that costs you every row.** Splicing a field **without shrinking its filler** silently enlarges the
struct while the hand-written `/* +0xNNN */` comments still describe the old layout - the file reads correctly,
the build is clean, and *every* row of *every* consumer drifts a fraction (measured: `ai/fn_802C474C` 100.00 ->
99.94, `fn_802C4B68` 100.00 -> 99.91, its unit's `matched_code` 32.52 % -> 0.00 %). Nothing else catches it:
build, stylelint and the gate's structural checks all pass. So verify a record merge by **re-measuring the
consumers' rows** - `ninja changes` must print *no* line for a unit that already owned the record. Offsets in
comments are not evidence; percentages are.

**Example.** (2026-09-26) `include/ai/ainpc.h`, the `_AINPC_W` record (0x49C bytes): after the merge 99 named
members (90 base + 9 spliced), 48 fillers, last field `+0x498`, total unchanged. The newcomer's 12 rows then
reproduced its own report exactly (`fn_802C5D10` 99.0541, `fn_802C6690` 100.0000, unit 20.159) and the landed
band's rows did not move at all - which is what made the landing acceptable with one `_AINPC_W` definition
instead of two.

## 57. A call-site mask means the callee's parameter is declared wider than the value

**Problem.** A wrapper (or any caller) sits at 50-90 % and the first divergence is one instruction at the `bl`:
retail masks or sign-extends the argument (`clrlwi r4,r4,16`, `extsh r5,r5`, a `slwi`/`srawi` pair) and ours
passes it straight through. It reads as a scheduling or inlining residual, nothing in the caller's own source
hints at a mask, and the search goes to flags and to the caller's statement order - where the mask cannot exist.

**Why try it.** The mask is the *caller's* cost of the *callee's* prototype: MWCC converts an argument to the
callee's declared parameter width, so a parameter declared `s32`/`u32` forces a mask on a value whose own type
is narrower (`s16`, `u8`, a bitfield), while a parameter declared with the narrow type does not. The mask is
therefore evidence about a declaration that is not in the function you are looking at.

**Result.** When retail's call site carries a mask ours does not, widen the **callee's parameter** to
`s32`/`u32` and narrow explicitly at the use inside the callee (which keeps the callee's own codegen) - or, for
a local, widen the local's declared type and mask at the use. Measured across one `ai` band without any flag
change: `fn_802D2ABC` 56.7 -> 90, `fn_802D287C` 85.9 -> 93.8, `fn_802D2264` 77.9 -> 81.7, and ten thin wrappers
went to **100 %**. The mirror case is the same lever: `fn_802D3984` (76.14 %) has our mask *too* wide, i.e. a
parameter declared wider than retail's, so the presence *or absence* of the mask is a statement about the
callee's declared width rather than about the caller.

**Example (the layout calculator that found the band's other class).** These records are written as explicit pad
arrays (`u8 pad_0xNNN[0xMMM - 0xNNN];`), which makes the layout mechanically checkable: a 30-line checker
asserts that the offsets tile (every filler's declared end equals the next member's offset, no duplicate
offsets). It caught `field_0x216` declared `s32` where the code does a `lbz` - the field is **1** byte, so every
later field sat 2 or 8 bytes out and ~20 functions each lost ~20 points. Take a field's size from the **access
width in the code** (`lbz` = 1, `lhz` = 2, `lwz` = 4, `stb`/`sth` likewise), never from the type you guessed,
and assert the tiling before you measure.

**Refinement, same session (a second and third view of the same record).** Two traps the first merge walked into,
both of them invisible to the tiling assert:

* **A header file can define more than one struct.** Keying members by offset alone matched `_AINPC_W`'s offsets
  against a smaller struct that sits above it in the same file (`0x0`, `0xC`), so the "next member offset" that
  sizes a field came from the wrong struct - `active` was sized 12 B instead of 1 B, and every splice was then
  refused for lack of room. Parse **per struct**: name each group, keep its own member list, and compute every
  size and every covering filler *within* the group. (A byte-wide filler is also written `u8 unused_0xNNN;` with
  no `[0xE - 0xN]` bracket, so a tiling check that reads only the bracket form should not flag it - the field
  ends at the next member either way.)
* **Compare the declaration, not just `(offset, name)`.** Fourteen differences: six were fillers the splices
  themselves superseded (a filler shrinks as its sub-fields arrive - the expected shape), and one was a genuine
  conflict the `(offset, name)` rule skipped silently - main had `u8 field_0x3F8;`, the newcomer
  `u8 field_0x3F8[4];`, and the newcomer's code indexes it. The merged header therefore compiled every *landed*
  consumer and not the newcomer's own source (`illegal operands 'unsigned char' [ 'unsigned char'`). **The test
  of a record merge is that both sides' sources compile** - whole-tree `ninja -k 0` at 0 FAILED, not just the
  consumer's object - **plus the rows** (`ninja changes` must print no line for a unit that already owned the
  record). Only the same-name-different-size class needs a decision, and the side whose *code* depends on the
  declaration has the better claim: here the array won, the scalar's neighbouring `unused_0x3F9` filler (which
  the array now covers) was dropped, and the whole-tree build plus an empty `ninja changes` proved both
  consumers and the newcomer.

**The tool.** `tools/units/recordmerge.py` implements the three rules above:

    python tools/units/recordmerge.py --base include/ai/ainpc.h         --other worker/<slug>:include/ai/ainpc.h --out include/ai/ainpc.h

It refuses to write while anything is unresolved - a named member in the way, no room in the covering
filler, a same-offset rename, a member whose size is the struct total - and prints the per-group delta so
the decision is visible. Top-level lines only the other view has are carried verbatim after the last group
when they are declarations (reported), and reported but not carried when they are not. Run against the
merge that produced this section (`089491a7b`'s header and the `802d44f4` view) it reproduces the 36
splices, the 33 filler splits and the `0x3F8` decision exactly, and keeps one trailing comment the hand
pass typed away. It is still only the edit: **the proof remains the whole-tree build plus `ninja changes`.**
