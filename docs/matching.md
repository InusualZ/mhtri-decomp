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
flag hunting there and document the residual instead (see the matching policy in `AGENTS.md`).

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

## Ruled out - do not re-run these

* **Compiler version.** Compile the matrix once for the unit; if it is flat, version is not the lever.
* **The whole `-opt` axis.** Sweep the compiler's own keyword list once. Beyond the one or two keywords
  that matter, the rest are no-ops or make the code worse; the frame-size traps are in trick 10.
* **`-Cpp_exceptions`.** It only adds `extab`/`extabindex` sections (needed at the end for a full match) and
  does not change `.text` here.
* **`-O` level and `-schedule`/`-fp_contract`/`-ipa`.** Worth exactly one sweep each; if the level is wrong
  the symptom is unmistakable (sizes off by hundreds of bytes and fused/hoisted code everywhere).

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
instructions identical except the r1 offsets); it is documented on top of the function in
`src/Camellia/camellia.c`. The full blow-by-blow log, including everything that was tried and rejected, is
in `.pi/notes/camellia-match-process.md`.
