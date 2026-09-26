# AGENTS.md

Guidance for AI agents (and humans) working in this repository.

## What this repository is

A **matching decompilation of Monster Hunter Tri** (Nintendo Wii, USA, disc ID **`RMHE08`**), built on the
[decomp-toolkit](https://github.com/encounter/decomp-toolkit) project template ([`encounter/dtk-template`](https://github.com/encounter/dtk-template)).

"Success" here means one specific thing: **the C/C++ source in `src/` recompiles to code that links into a
`main.dol` byte-identical to the original game**. It is not enough for code to compile, look correct, or
produce the same output — it must match the original instructions, relocations and section layout.

The original binary is split into relocatable objects by decomp-toolkit (no hand-written assembly, no game
assets in the repo), and the final `main.dol` is verified against `config/RMHE08/build.sha1`.

* Original binary: `orig/RMHE08/sys/main.dol` — SHA-1 `BF4850739478CAAEDFE675949EB7C28595A7FDE9`
* Current state: `src/` holds many registered units (see `docs/plan.md` §2 for the measured totals); the
  `src/auto/` units are moved to their final homes under the register-once rule (`docs/plan.md` §12), and a
  region still unclaimed in `symbols.txt` is a proposal backlog, not a defect.


## Non-negotiables

1. **Never modify `orig/RMHE08/**`.** It is the original game data and the ground truth for every diff.
   Read-only, always.
2. **Never commit build output or original files.** `build/`, `orig/RMHE08/**` (except `.gitkeep`),
   `*.dol`, `*.rel`, `*.elf`, `*.o`, `*.map`, `objdiff.json` and `compile_commands.json` are gitignored —
   keep it that way.
3. **Do not edit `configure.py` compiler flags, `mw_version` values or tool version tags to make something
   build.** Those settings change codegen for every translation unit. Changing them is only acceptable with
   concrete evidence (an instruction/size diff that points at the flag), and must be called out explicitly.
   See "Gotchas" for the incident that motivates this rule.
4. **Only mark an object `Object(Matching, ...)` when it actually matches.** Otherwise use `NonMatching`.
   A wrong `Matching` flag breaks the final DOL hash for everyone.
5. **Don't rename or delete symbols that already exist in `config/RMHE08/symbols.txt`** unless you have
   verified nothing else depends on them. Symbol names are referenced by `splits.txt`, the linker script
   and the analysis output.
6. **Commit only what the task covers, and never push.** History is never rewritten (`rebase`,
   `commit --amend`, `reset --hard`, force-push, deleting/moving tags) and nothing is ever pushed - `origin`
   is the upstream template, not a fork of this project. On **commit approval**: the project owner has granted
   the campaign's orchestrator **standing approval to commit its own work without asking per commit**
   (2026-09-21, docs/plan.md, "Commits"), so a `docs/plan.md` batch ends by committing - that is not a licence
   to commit *anything*: an experiment, a probe, a half-registered unit, another stream's file or a scratch
   artifact still stays uncommitted, and anything outside the campaign keeps the old rule (leave it in the
   working tree and report what you would commit).
7. **Never paste `config/RMHE08/symbols.txt` into a prompt/tool output.** It is ~65,700 lines / 4.5 MB.
   Grep it, slice it, or use `dtk`/objdiff; do not print it.
8. **Never commit the local-only block in this file.** Everything between `<!-- LOCAL-ONLY-BEGIN` and
   `<!-- LOCAL-ONLY-END -->` (the `## Current task / plan` section) is live agent working state, not repo
   content: pull it out before `git add AGENTS.md`, restore it afterwards, and commit every *other* AGENTS.md
   edit normally. Use the tool, not `sed`: `python tools/agents/localonly.py pull` before staging and
   `python tools/agents/localonly.py push` after the commit (skill: `agents-md-local-only`). Verify with
   `python tools/agents/localonly.py verify` - i.e. `git show HEAD:AGENTS.md | grep -c '^<!-- LOCAL-ONLY'`
   must print `0`. (This rule's own prose mentions the markers, so anchor the match at line start; the tool
   matches whole marker lines for the same reason.)

## Matching policy: flags and source variants

Two working rules that apply to **every** unit, agreed with the project owner:

1. **Apply the best-scoring variant even if it is not a full match.** A source rewrite or flag change is
   worth landing as soon as it *measurably improves* the objdiff score (`ninja build/RMHE08/report.json`,
   per-symbol `match_percent`) and regresses nothing else. Do not hold a real improvement back waiting for
   100 %: record the residual diff (what still differs and why) **in the unit's file header comment**, so
   the next reader finds it where the code lives. The residual belongs in that one place, never as a
   per-function comment - see "Commenting and naming" under Conventions for what a function comment is for.
   Never land a change that makes any function worse. Landing means the unit's own source or
   `configure.py` carries the change and the repository rebuilds better - the skill does it with
   `python .agents/skills/mwcc-unit-matching/scripts/mt.py variants --apply <name>` followed by a forced
   rebuild; a probe-only winner is not progress.
2. **Evidence-backed flags live in `configure.py` as soon as they are proven**, even while the unit is
   still short of 100 %, so the repo always reflects the best known state. They must be **per library**
   (never edit `cflags_base`/`cflags_runtime` for everyone) and must be called out explicitly, as
   non-negotiable #3 requires, with the instruction/size evidence in a comment next to them.

A unit's flag evidence belongs next to its definition in `configure.py` (a per-library `cflags_*`
override below `cflags_runtime`), not in this file; this file only carries the policy.

The *how* - the ideas, the problem each one solves and whether it has been tried - is the playbook index
below, which doubles as the todo list for whatever unit is being worked on.

## Operational mode: production runs

How the campaign is run while a batch of units is being produced (owner's instruction, 2026-09-24). It
supersedes "keep the queue full": the aim is **steady** throughput, not maximum throughput.

* **Six worker subagents at most, and that budget covers everything** - unit workers and tool fixes share it.
  Five units plus one fix is fine; six units plus a fix is not.
* **A slot goes to a problem before it goes to a new unit.** When something breaks - a gate refusal, a blocked
  or waiting worker, a tool that cannot express what the work needs, MAIN's HEAD on the wrong branch, a hole
  in the queue - the next free slot fixes that. Only when nothing is outstanding does a freed slot take a new
  proposal.
* **Refill one slot per completion, not in waves.** Claim with `queue.py next`, launch that one worker, and let
  its completion free the slot. The 12-13 worker waves are what produced the repair work this policy exists to
  avoid: unions mangling struct bodies, declarations whose owner registered mid-wave, an empty `--message`.
* **When a wave is claimed, claim it with `queue.py next --count N` - never N adjacent proposals.** The
  stride takes `i, i+N, i+2N, ...` in the queue's address order, so no two workers hold adjacent proposals.
  Adjacency is the vector for almost every clash this campaign has had: it is what puts two workers on one
  translation unit (`proposal/8007270C` + `proposal/80073180`, both `g3d_calcvtx.cpp`), it is what produces
  the rule-2 boundary artefacts (a neighbour registers a symbol you declare), and neighbouring units share
  owner headers and types by construction. The stride makes that **provable** - two adjacent proposals can
  share a wave only if both indices are congruent mod N, impossible for N > 1 - where random selection would
  only make it unlikely (both halves of one TU in a wave about once in N tries). The caveat is locality:
  spreading costs cross-unit knowledge reuse, so prefer the spread *within* a band that the address order
  already gives, and do not defeat it by re-sorting the ready set (by score or otherwise).
* **Land one unit per commit, one unit at a time, from `main`.** Check `git rev-parse --abbrev-ref HEAD` prints
  `main` before landing - `land.py` now refuses otherwise, because a batch landed off `main` puts its commits
  on the wrong ref and slides the merge-base that `applybranch.sh` and the gate both resolve against.
* Keep `ninja build/RMHE08/ok` green and `orig/RMHE08/**` untouched as the invariant of every step (see
  Non-negotiables).

**KNOWN BUG, measured 2026-09-24 (booked a slot):** `queue.py next` offered `proposal/80063888_fn_80063888` a second
and third time while that proposal was already claimed and its worker was running, refusing each time with
"branch worker/80063888-fn-80063888-b806 already exists - the unit is claimed (or was never released)". So a
claimed proposal is still `ready` as far as selection is concerned. This is the pool re-hand bug on the *claim*
axis rather than the *covered* axis. Until it is fixed, `queue.py next` can waste turns refusing at random;
claiming with the claim registry checked first (`claims.py list`) is the workaround.

The steady loop, per unit:

1. `queue.py next` claims one proposal - one worktree, one branch, one brief - and prints the paste-ready spawn.
2. The worker registers its range at its final home and commits the bodies on its branch, measured.
3. Apply that branch with `.pi/bin/applybranch.sh` (the merge-base diff; the local-only block records why the
   two obvious alternatives lose work), resolve the shared-file conflicts, then
   `land.py record-base` -> `land.py land --units <claim>` -> `claims.py release`.
4. `ninja build/RMHE08/ok` green, then refill exactly that one slot.

Two tool behaviours the loop leans on, both fixed this session: `queue.py` never offers a proposal whose range
a registered unit already covers, so re-attributing a region cannot re-hand landed work; and
`attribute.py queue <start> <end>` **rewrites** the queue file with only that region's proposals rather than
appending - run it over the whole unclaimed region (the file records this as `cap: 0`), or the rest of the
backlog disappears.

## Matching playbook (index of `docs/matching.md`)

`docs/matching.md` is the playbook for making a unit match its original object. Every idea in it is
indexed below together with the problem it solves. **Treat this table as the todo list**: for a unit that
does not match yet, work down the rows and keep the status current.

The same method is packaged as a project skill, `.agents/skills/mwcc-unit-matching/` (tracked - the
`.gitignore` excepts it), so an agent can load it on demand instead of reading the playbook every session: `SKILL.md` holds the loop and the idea list,
`references/` is *generated* from `docs/matching.md` (never edit it - run
`python .agents/skills/mwcc-unit-matching/scripts/sync_reference.py`, or `--check` to detect staleness),
and `scripts/mt.py` forwards to the `tools/` helpers (`units`, `info`, `frames`, `matrix`, `sweep`,
`variants`, `diff`, `slots`, `sections`, `dwarf`). The table below stays the source of truth for state.

All project **subagent profiles** live in one tracked folder too, `.agents/agents/` (`/.gitignore` excepts it
next to `.agents/skills/`). The harness discovers them as *project* agents - no install step: write the file,
`subagent({ action: "list" })` shows it, and `{ agent: "decompiler" }` runs it. `.agents/agents/decompiler.md`
is the role for unit work: it inherits the project context (`inheritProjectContext: true`, so this file is in
the child's prompt), loads the matching/verify/registration skills (`skills:`), and carries the operational
rules that used to be hand-typed into every launch - isolation (write only in your worktree, read the brief in
MAIN), convergence (best-scoring variant, never a regression, residual in the unit header), the per-symbol
measurement traps, the section 6.5 style rules and the standard report. The generic `worker` role stays for
non-unit tasks. When a launch prompt and the profile disagree, the profile wins - so **keep the rules in the
profile, not in the prompt** (QA: `subagent({ action: "list" })` must show `decompiler (project)`).

All project skills live in one tracked folder, `.agents/skills/`, so every harness sees the same set:
`mwcc-unit-matching/` (this playbook), `symbol-map-editing/` (`tools/symbols/symedit.py` - look up, list by
range and rename symbols without ever loading `symbols.txt` into context), `agents-md-local-only/`
(`tools/agents/localonly.py` - pull the local-only section out of this file before a commit and push it
back after), `objdiff-verify/` (proving a unit really matches), `tu-boundary-discovery/`
(`tools/splits/tudiscover.py` - from one symbol address, work out which functions and data ranges form one
translation unit, before any source is written) and `decompile-symbol/` (`tools/units/symbolpreflight.py`,
plus `tools/units/m2cinput.py` for the `tools/m2c` decompiler - one symbol from an address to a registered,
measured unit).

| # | idea | problem it solves | status |
| --- | --- | --- | --- |
| 1 | Per-unit instrument | The project-wide pass/fail cannot measure one unit (`ninja build/RMHE08/ok` cannot pass while any object is `NonMatching`, and `complete_code_percent` says 100 % for wrong code), so no change can be judged. | no |
| 2 | First divergence, not the percentage | `match_percent` is positional, so one instruction too many in the prologue reports the same ~0 % as completely wrong code and sends you hunting in the wrong place. | no |
| 3 | Read the target's disassembly | The diff says *what* differs, not what code shape the original source had - and a prologue or an addressing idiom is a flag fingerprint. | no |
| 4 | Codegen is the oracle, not `.comment` | A synthesized `.comment`/`mw_comment_version` looks like a compiler fingerprint and invites a version hunt that cannot pay off. | no |
| 5 | One flag at a time, real command line | A hand-written command drifts from what ninja runs, and with several flags in play it is unclear which one explains which symptom. | no |
| 6 | Guard against stale objects | Scripted compiles silently measure an object the compiler never wrote (MWCC's `-o` is a *directory*), producing impossible "all versions identical" results. | no |
| 7 | Scratch files to attribute a symptom | The full-unit diff cannot tell whether an instruction choice comes from a source idiom or from an optimizer pass, so the wrong thing gets blamed. | no |
| 8 | Ask the compiler what is on (`-opt display`) | One `-O` level sets several switches, so what a flag set actually resolves to (peephole? scheduling?) is guesswork. | no |
| 9 | Enumerate options from `-help` | Invented spellings are silently accepted and ignored, so "no effect" looks like evidence; other spellings do not parse at all. | no |
| 10 | Frame size is not a success signal | Locals are rounded to 16 bytes, so several unrelated variants hit the target's frame while emitting wrong code. | no |
| 11 | Size gap is not "different source" | Aggressive flags *remove* instructions, so a target that is hundreds of bytes bigger can be purely a flag problem - and rewriting correct code wastes days. | no |
| 12 | Check the flag's relocations | A flag can match the code shape while referencing symbols (save helpers, table bases) the original build never had. | no |
| 13 | Stop when the diff is not flag-shaped | A near-miss variant that fixes one symptom (a frame, a single instruction) keeps you hunting flags when the residual is really source or liveness. | no |
| 14 | Prove the committed flags reproduce the object | A hand-written `cflags_*` list can silently differ from the command line that was tested (leftover `-O4,p`, duplicated `-inline`). | no |
| 15 | Pin a metric that does not drift | Fuzzy percentages change between objdiff versions, so a decision made against one number is meaningless against the other. | no |
| 16 | Scope optimizer settings per function with pragmas | The `-opt` levers are global, so a per-function codegen difference looks unreachable from the source side - a pragma pair scopes them to one function. | no |
| 17 | Cross-family version matrix | `mw_version` is inherited project-wide and the target's `.comment` is synthesized, so a unit built by a different toolchain (a prebuilt SDK library, say) looks like an unexplainable residual. | done |
| 18 | Named temporaries, declaration order, operand order | With the opcodes already equal the residual is register numbers only and looks unreachable - but the allocator colours live ranges from the source's temporary structure. | done |
| 19 | Loop shape decides the loop idiom | A countdown loop only becomes `mtctr`/`bdnz` from the right source shape; the wrong shape adds an instruction and shifts every later register. | done |
| 20 | Loop-invariant address through a `u32` local | Retail keeps a field address in a callee-saved register where we fold it into a load displacement, which costs a register and the whole function's colouring. | done |
| 21 | Record-form count as the peephole/scheduling fingerprint | `-opt` is per unit, and the old whole-DOL `extrwi` scan could not see the fused form at all (it is an objdump alias). | done |
| 22 | Stop when retail's colouring is your mirror image | Once only the allocator's web priority differs, more source shapes cannot help - record the residual and move on. | done |
| 23 | `splits.txt` data ranges: what objdiff can and cannot fix | Defining a data symbol fixes name rows only by (section, offset), and can make dtk drop the target's `R_PPC_NONE` pool relocs - a regression. | done |
| 24 | Merging a probe into the unit is its own step | Probe numbers are not unit numbers, and one struct definition has to serve every function, so types need use-site casts and everything must be re-measured. | done |
| 25 | Shared memory dump as a name/signature/struct oracle | Unnamed `fn_*` functions and untyped structs can be resolved in one query from the game's runtime dump (`docs/memory-dump.md`). | done |
| 26 | The target's section is part of the match | objdiff pairs sections, so a unit whose code landed in `.text` while the target object says `.init` diffs perfectly and still reports `None` (`__declspec(section "...")`). | done |
| 27 | Same instructions, different order names the `-O` level - probe it per unit | An epilogue swap looks like an unreachable scheduling residual and is not source-shaped. `-O4,p` vs `-O3` is per unit (g3d/lobby want `-O3`, OSAlarm/NetworkWiiMediator want `-O4,p`), and the two-variant probe can be run against the split target object that covers the region before the unit is registered. The level also decides function packing (`-O4,p` implies `-func_align 16`), which is the second reason to probe it. | done |
| 28 | A kept `bl` to a tiny static names the unit's inlining setting | The callee is inlined away, so the caller is an instruction short and it reads as a missing helper. `-O3` + `-inline noauto` closed four `main.cpp` functions (74 -> 100, 21.18 -> 100, 71.12 -> 96.73) - `noauto`, not `off`, because `off` also de-inlines retail's aggregate copy (fn_8003F940 99.02). `#pragma peephole off` is the only spelling the compiler honours. | done |
| 29 | A claimed literal pool: declare the constants, never define them | With the unit's `.sdata2`/`.sdata` claimed, the map's pool names can be `extern`-declared and used as load operands so they pair; *defining* them rebuilds the pool and moves the whole section. | done |
| 30 | A C++ unit's exception settings live in its object, not in the source | Every function matches and the unit still falls short because the target has `extab`/`extabindex` and ours has none: `-Cpp_exceptions on` per lib (Pl, unchanged `.text`, all 12 entries equal) or `#pragma exceptions on` per file (`sys_mem.cpp`'s `throw()` specs, `Gecko_ExceptionPPC.cp`'s `0x10`+`0x18`). Compare the two objects' extab sizes and bytes, and use the pragma *pair* for a `$`-section. | done |
| 31 | A function unpaired by name measures 0 %, not 60 % | objdiff pairs by symbol name, so a `fn_XXXXXXXX` the map never renamed (or a mangled name it spells differently) contributes nothing even when the bytes are perfect - read the name the object emits and rename the map to it. Three `Gecko_ExceptionPPC.cp` functions went 0 % -> 100 % with no source edit; the reverse also holds, a stale *target* object keeps the old reloc name until the next re-split. | done |
| 32 | A pragma region is not local to the functions it covers | A scoped `#pragma peephole off` pair fixes one function's residual, but the *reset* decides where the region ends - moving it past two more functions in `main.cpp` flipped them 99.52 -> 100 and 99.47 -> 100, functions the pragma was never aimed at. | done |
| 33 | Prefer the unit's flags over a per-function flag | A TU is compiled once, with one flag set: if the unit's other functions match, a function that needs *different* flags is a source, boundary or stale-target problem, not a flag one. A scoped pragma that fixes one function fights the rest (RSO's level-3 pragma cost `RSOUnLink` and `FindExportIndex` their 100 %), and `optimization_level`/`opt_*` are whole-function anyway. | done |
| 34 | A switch tail's constant returns are if-converted | The target's `return 0` tail and a `default: return 1` look like a missing arm, but MWCC folds two constant return arms into a branchless bool - so the case bodies must be written negated (`if (!c) return 1; break;`) and in body-address order. | done |
| 35 | A dead copy chain steers the allocator's web priority | Two webs sharing one register pair look unreachable from the source - but the allocator colours in web-list order and the IR's *dead* copy webs count, so a chain of dead copies of the competing value plus one live load flips the pair without changing an instruction. | done |
| 36 | A flipped unit's unreferenced trailing function is trimmed | The object is byte-identical and the flip still breaks the DOL by exactly the last function's size: `dol split` stamps `active_flags=0x08` in `.comment` (export_all) while MWCC writes `0x00`, so the linker drops the unreferenced tail. `__declspec(export)` fixes it. | done |
| 37 | A switch's `default` arm goes first in the source | MWCC emits the default body after the compare chain wherever it is written, so `default:` written first lands where retail has it - written last the chain is ordered the other way and the function grows by a word. | done |
| 38 | An `s16` parameter with a compound assignment makes a narrow field store raw | A masked `stb`/`sth` (a `clrlwi` retail does not have) means MWCC is narrowing the value to the field - taking it through an `s16` parameter and writing with `+=` leaves it the right width, so it stores raw. | done |
| 39 | A unit whose retail code keeps unfused peephole folds needs the peephole pass off | Retail keeps the unfused form of a fold our `-O3` peephole makes - a masked `clrlwi` before a narrowing store, a separate `clrlwi`+`cmpwi`, an `li r0` + `psq_lx` epilogue; `#pragma peephole off` (or `-opt nopeephole`) restores it. | done |
| 40 | A unit whose retail code keeps unfused multiply-adds needs `-fp_contract off` | Our default `-fp_contract on` fuses `a*b + c` into one `fmadds`/`fmsubs` where retail keeps `fmuls` + `fadds`; `#pragma fp_contract off` restores the two instructions. | done |
| 41 | `#pragma optimization_level 1` does not turn the peephole off | A unit needs the peephole pass off and `#pragma optimization_level 1` looks like the way to get it; the pragma only sets the level, so the command line's peephole still folds. The lever is `#pragma peephole off`. | done |
| 42 | A C++ free function needs `extern "C"` so objdiff can pair it by name | A `fn_*` defined in a `.cpp` measures 0 % with byte-perfect code: MWCC mangles the name and objdiff pairs by symbol name; `extern "C"` makes the emitted name the map's name. | done |
| 43 | Retail's per-string `lis`/`addi` addressing means the unit was built with `-pool off` | Our string literals are addressed through one `@stringBase0` base register where retail materialises each string with its own `lis`/`addi`; `-pool off` stops the pooling. | done |
| 44 | A string pool in `.data` means the build was not `-str readonly` | Retail's string pool in `.data` (not `.rodata`) says the build did not use `-str readonly`; it is a placement diagnostic even when the score does not move on its own. | done |
| 45 | A hand-written string literal's `\n` becomes CRLF on this host | MWCC on this host translates the `\n` in a literal to CRLF, so a string pool written in-source does not match the DOL's LF bytes; leave the range to the data pass. | done |
| 46 | A flipped unit's `.ctors$10` fragment is reordered by the linker | The object is byte-identical and `flipcheck.py` says READY, and the flip still loses the unit's `.ctors$10`/`.dtors$15` words; re-split first, then suspect the linker's fixed ctor/dtor name order. | done |
| 50 | An already-mangled map name must not be declared as a C++ identifier | The map carries a *real* mangling (`Panic__Q24nw4r2dbFPCciPCce`) and the C++ source declares that spelling as an identifier, so the front-end mangles it AGAIN (`...__FPCciPCce`) and the link cannot resolve it - invisible while the unit is `NonMatching`, because its object is never linked. The map's name IS the real declaration's mangling: write `namespace nw4r { namespace db { void Panic(const char*, int, const char*, ...); } }` and the front-end reproduces it exactly (confirm with `tools/units/mangle.py`). This is row 48's complement: a `fn_XXXXXXXX` stem is a placeholder (write C++ and rename the map), an already-mangled name is real (write the real declaration). Five `ef` units had it, all flipped `.c`->`.cpp` by the promotion. | done |
| 47 | Automate the shape search: generate, compile, score and rank source variants | The residual is codegen, so finding the source shape was a hand-run search of hundreds of variants per function; `tools/flags/shapesearch.py` does it mechanically (declaration order/types, `for`-decl hoisting, temps, casts, statement order, compound assignment, field form, dead copies, switch/cond/ternary/loop shape) and ranks by the official report metric. On `Pl/pl_act`'s worst 20: 12/20 improved, two byte-identical to 100 % (`fn_8027D40C` via `loop_decl_top`, `Pl_get_gunner_vec` via `deadcopy_plain_x`), combined unit mean 98.888 -> 99.086. | done |
| 53 | A sparse switch's compiler-emitted jump table is readable once its `.data` range is claimed | The table reads as zeros while the range is unclaimed, so the arms look unreachable and the function gets parked as a ceiling; claiming the range puts the bytes and the `lis`/`addi` relocations into the unit. `Pl/fn_802430E8` went 99.999 -> 100.0 on the claim and then found 52 of 59 arms instruction-identical to its landed sibling; `Pl/fn_802373AC` generated 145 arms mechanically (calibrated against a landed sibling first) for a 99.986 % unit. Read tables from `main.elf`, never by hand-mapping DOL VAs. **Refined 2026-09-26 (evening):** claim the *table's own* range (4 x cases, a size the compare chain proves), not the band around it - those labels are target bytes nothing reproduces and claiming them lowers the score. The row can be **0 %**, not just 99.999: with the table claimed our object emits it (`.data` pairs at 100 %) and the arms come out of `main.elf` (`stage/fn_802B3270`: 0 -> 100.00 % byte-identical at 3688 B, unit 59.05 -> 88.75 %). | done |
| 54 | Dolphin's `.map` names a jump table's OWNER and a `__FILE__` emitter | Section 53 says to claim the table's `.data` range but not *whose* it is, and naming-evidence class 1 (the `__FILE__` static) does not say which function emitted the string; the dump's own local symbols encode both (`_<fnaddr>switchdataD_<addr>` = the table's owner, `_<fnaddr>s_<file>_<addr>` = the function emitting that file's `__FILE__`). One query named `menu_item.cpp` and proved its seam. | done |
| 55 | An odd-start section claim cannot be linked with MWCC's alignment | A byte-identical object refuses to flip and every later table is 4 bytes late: mwld cannot honour a 4-mod-8 address for a section MWCC marked align 8. `tools/elf/objalign.py` lowers the emitted alignment to `lowbit(claimed start)`, dtk's own rule, chained into every MWCC rule. | done |
| 56 | Two lanes' views of one work record are merged by tiling, not by choosing a side | Two neighbouring lanes write the same `include/<area>/<rec>.h` and the second landing hits an add/add conflict on a file `main` already has - neither side is a superset and one offset has two names. Taking the live header as the base and splicing the other's only-fields into the covering filler (size from *its* layout, every filler recomputed so the tiling is exact) keeps one definition and both consumers - but splicing without shrinking the filler silently grows the struct and drops **every** row of **every** consumer a fraction while build, stylelint and the gate all pass. Only re-measuring the consumers' rows proves it. | done |
| 57 | A call-site mask means the callee's parameter is declared wider than the value | A caller is one instruction off at a `bl` - retail masks the argument, ours passes it through - and the residual reads as scheduling or source order in the caller, where a mask cannot come from. MWCC converts an argument to the *callee's* declared parameter width, so widening that parameter to `s32`/`u32` (narrowing at the use) restores retail's mask; a mask that is *too* wide is the same lever the other way, and a field's width comes from its access (`lbz` = 1 B), not from the type you guessed - a wrong one shifted every later field and cost ~20 functions ~20 points each. | done |
| 52 | A vtable we own must be compiler-emitted; a hand-modelled table is not evidence of inheritance | A `NonMatching` unit whose source only *views* its own vtable still scores 100 % (the DOL keeps the bytes), so the class can be missing from the reconstruction and nothing fails until the flip - and a table the worker wrote cannot prove the layout it was written from. Owner's concern, audited repo-wide 2026-09-26: 23 `vtable = lbl_*` assignments, **all 23** aimed outside our ranges (correct), and of 6 code-pointer runs inside registered ranges only one is vtable-like (`Pl/pl_master.cpp`'s `jumptable_805C5FA0`, a compiler-emitted switch table in a `Matching` unit) - zero hand-built tables, zero owned-but-unemitted vtables. | done |
| 48 | Never append `, ...` to a definition to dodge an argument-count mismatch | The variadic spelling compiles and links and looks cosmetic, but MWCC emits a full varargs prologue for EVERY function declared that way: a 12-byte thunk became 108 bytes and the unit scored 27 %. A fixed unused parameter of the caller width (`void* unused`) changes nothing in the prologue. | done |
| 48 | A C++ unit's unmangled map name is not a reason for `extern "C"` | The unit is C++ but the map spells its symbols `fn_XXXXXXXX`, so a C++ definition mangles, pairs nothing and reports 0 % - and a member function cannot be `extern "C"` at all. The map is a build input, not the original's symbol table: `tools/units/mangle.py` compiles a probe with a real unit's command line and prints the mangled spelling, so the fix is a map+source rename (validated by exact reproduction: `void Pl_Skill_ck(_PLW*, u16)` -> `Pl_Skill_ck__FP4_PLWUs`). | done |
| 49 | `extab` in a no-exceptions lib is a cheap C++ language signal | A unit's language has to come from evidence, but the conclusive signals (mangled definition, `.cpp` `__FILE__` string) can be absent at registration, so the extension is a guess that a later pass must undo. C has no exceptions, so `extab`/`extabindex` in the object proves C++ - **unless the lib sets `-Cpp_exceptions on`** (`cflags_pl`/`cflags_main`/`cflags_g3d`/`cflags_camellia`), which lets a C unit emit it too. `tools/units/langcheck.py` resolves the lib flag (`cflags_exceptions`) and reports the signal as *suggested* C++ (never conclusive, one-directional: no extab is not evidence of C), so a wrong `.c`/`.cpp` in a no-exceptions lib is caught at registration. Measured: 43 objects carry extab but **0** decisive candidates (all 25 `.c`-with-extab sit in exceptions-ON libs), 2 `.cpp` with no extab. | done |
| 51 | A shared type, an extern and a mangled name each have one owner | Three defects that used to survive by review are the same mistake: an identifier that belongs to someone else is spelled out locally. A type more than one unit uses is **copied** (20 names, 68 extra definitions under `src/`), an `extern` for a symbol another unit **defines** is declared in the consumer's file (126 declarations into 27 owner units), and a compiler **mangling** (`Name__FP...`/`Name__Q...`, a class member `name__<len>ClassF...`) is written as the callable identifier - the last is how row 50's double-mangle is born. `tools/units/stylelint.py` checks all three (`--diff` refuses only new ones); ownership is derived from `symbols.txt` + `splits.txt`, and the rule-2 unsplit case is a **named** gap where the registered address bands interleave, not a guessed header. | done |

Ruled out for this project - recorded so nobody re-runs them (details in `docs/matching.md`):

| idea | problem it solves | status |
| --- | --- | --- |
| Compiler-version matrix | "The original used a different compiler release" is the first suspicion and has to be closed once, per unit. | no |
| The rest of the `-opt` axis | An unknown sub-option might be what controls fusion or stack allocation. | no |
| `-Cpp_exceptions` | `extab`/`extabindex` presence suggests exceptions were on; it adds those sections but no `.text` bytes here. | no |
| `-O4`/`-O4,p`/`-O2`, `-schedule off`, `-fp_contract off`, `-ipa off` | Another optimizer level or codegen switch might be the retail setting. | no |
| A paired-single op in a function (the SDK's vector library, `fn_8007270C`'s fill loop) | It reads as a codegen lever and eats flag and shape sweeps. Measured 2026-09-25: **176 of 19,916** functions contain one and **0** of ~2,450 matched functions do, so the frontend cannot emit the body-store form. Record it and move on. | no |

The status column is about the **current target** - the one unit/diff being worked on - not about whether
an idea is any good. The target and the step being worked are kept in the local-only `Current task / plan`
section at the top of this file.

| status | meaning |
| --- | --- |
| `todo` | queued for the current target, not tried yet |
| `no` | tried for the current target and it did not resolve it, or it does not apply |
| `done` | this is what resolved the current target (and it is the `docs/matching.md` section with this row's number) |

Rows 1-16 read `no` because the table was derived from the `Camellia` flag hunt: the ideas all worked there
and their outcome is already in `configure.py`, but none of them is what the open `camellia_setup256`
residual needs. Rows 17-25 are `done` for the target in flight, the `RSO/runtime` unit - each one is the
idea that closed part of it, and the full walkthrough is the "Worked example: the `RSO/runtime` unit"
section of `docs/matching.md`. Every idea that produced a win gets recorded here (and as a section) **in the
same session it worked** - see "How to work the list" below.

New ideas for the current target (no `docs/matching.md` section yet - they earn one only if they work):

| idea | problem it solves | status |
| --- | --- | --- |
| Third historical source variant | Our `camellia_setup256` matches NSS and NetBSD textually, but the retail file may be a third variant (original NTT 1.2.0 / SDK copy). Diffing another copy's absorb region is the cheapest way to find the source shape the allocator liked. | no - NSS 3.19.1 / 3.24 / 3.28.4, the older NSS fetch and NetBSD NTT-derived are all statement-identical to ours (only `PRUint32`/`SUBL` naming and whitespace); no third variant exists in that lineage. |
| Perturbation probe | The slot outcome is sensitive to IR shape (level 4 flips the frame). Deliberately perturb one independent statement, watch whether `subL[29]` coalesces, then look for the natural source form that produces the same perturbation. | no - 22 statement/pragma perturbations tried; none flips the frame at level 3. The productive version of this idea turned out to be per-function pragmas (row 16). |
| Absorb `dw` / `CAMELLIA_RL1` IR shape | The level-4 near-miss changes exactly this chain, and it is the only region whose IR shape demonstrably moves the frame. Rewrites here (temps, ordering, expression form) are the highest-probability remaining lever. | no - five source forms tried *with* the level-4 pragma (split comma, RL1 temp, operand swap, `tl` rewrite, `tl` temp): the 12-row window is unchanged, so it is level-4 optimizer behaviour, not source shape. |
| Pragma combination search | With the level-4 pragma the frame is correct and only a 12-row window differs; a pragma that suppresses the level-4 reorder would finish the job. | todo - try `opt_lifetimes on`, `opt_common_subs on` explicitly, and level 4 combined with each honoured pragma. |
| Level-4 pragma + window source forms | The window is 12 rows of register choice plus `CAMELLIA_RL1` placement; a source form that makes level 4 emit the target order there would be a 100 % match. | todo - 5 forms tried, more structural rewrites of the whole kw4 chain remain. |

How to work the list:

1. Set the **target** in the local-only section, queue the rows as `todo`, and start at row 1. Do **one
   idea at a time** and record *evidence* - numbers, sizes, first-divergence indices - not impressions.
2. Update the row's status and put the step you are on in the local-only section, so a fresh session knows
   where to resume.
3. A `done` idea gets a section in `docs/matching.md` in the house style - **Problem / Why try it /
   Result / Example**, short and to the point - and the row's number is that section's number. **Record it in
   the same session, as soon as it works**: a win that only exists in a chat message or a scratch report is
   lost at the next compaction and the next unit re-derives it (this has already happened once here), so
   treat "the idea is written into the table and the playbook" as part of the win, not as follow-up work.
   The same commit has to bring the skill's copy with it: `python
   .agents/skills/mwcc-unit-matching/scripts/sync_reference.py --check` must come back clean, because
   `references/` is what a fresh session and every subagent actually load - it sat 55 lines behind
   `docs/matching.md` the day this was written, i.e. current knowledge that no agent could see.
4. When every row is `no` again, the target needs **new** ideas: add them here as `todo` rows first
   (idea + problem it solves), try them, and promote the ones that work into `docs/matching.md` (same
   style, next free number). Ideas that fail stay in the table as `no`, so they are not re-run.
5. Unit-specific findings that are not playbook material (a residual diff, a known-bad flag) belong in
   the unit's own header comment, per the matching policy above.

## Repository layout

```
configure.py              Project config + build generator (compiler flags, libs, tool versions)
config/RMHE08/config.yml  Analyzer/build settings, DOL path + hash, selfile (RSO list)
config/RMHE08/symbols.txt Symbol map: name = section:address; // type/size/scope  (generated, hand-editable)
config/RMHE08/splits.txt  Which address ranges belong to which translation unit / section
config/RMHE08/build.sha1  SHA-1 of each built artifact — the pass/fail check for the whole project
src/                      Our C/C++ source; a unit is registered once, at its final home
                          src/<module>/<name>.<ext> (no src/auto/ bucket - docs/plan.md §12)
include/types.h            The project's common scalar types (u8..s64, f32/f64, BOOL/TRUE/FALSE/NULL) - one
                          definition, included by every unit that needs them; `cflags` already get `-i include`.
                          A declaration moves here the *second* time a unit needs it, never the first (a type
                          one unit uses belongs to that unit, or beside it like `src/Camellia/camellia.h`);
                          `src/Camellia/camellia.c` is the vendor exception and keeps its own typedefs.
                          An SDK type (`GXRenderModeObj`, `Vec`, `Mtx`, ...) gets a `dolphin/` mirror here
                          rather than a fresh declaration per unit.
orig/RMHE08/              Original game files (read-only, gitignored). main.dol, files/mh3.sel, ...
build/                    Everything generated: build.ninja, compilers/, tools/, RMHE08/ (gitignored)
tools/                    Tooling. dtk-template's scripts at the top level (project.py, download_tool.py, ...),
                          plus ours, grouped by what they do:
                            unitutil.py  shared unit/flag/ELF layer used by the tools below
                            flags/    compiler-flag and source-shape experiments (frame.py,
                                      mwcc_matrix.py, optsweep.py, tryvar.py,
                                      shapesearch.py + shapes.py,
                                      infer.py) + variants/<lib>.py data
                                      - see docs/matching.md
                            objdiff/  objdiff consumers (symdiff.py, slotmap.py)
                            elf/      object/DWARF readers (elfsect.py, dwarfmap.py)
                            splits/   TU boundary discovery (tudiscover.py): from one symbol address,
                                      work out which functions and data ranges form one translation
                                      unit - see the `tu-boundary-discovery` skill
                            symbols/  symbol-map proxy (symedit.py): look up, list by range and rename
                                      symbols in config/RMHE08/symbols.txt without loading it into
                                      context - see the `symbol-map-editing` skill
                            agents/   AGENTS.md housekeeping (localonly.py): pull the local-only
                                      working-state section out before a commit and push it back after
                                      - see the `agents-md-local-only` skill
                            units/    the `decompile-symbol` helpers (symbolpreflight.py: one symbol's
                                      owner and collision pre-flight; m2cinput.py: a target object's
                                      disassembly as `tools/m2c` input) plus their self-tests, and
                                      ledger.py: the campaign's progress and the next symbol to claim
                                      (docs/plan.md), vtableaudit.py: the rule 10 audit (an owned
                                      vtable our object does not emit, plus non-`.text` section-size
                                      completeness), and land.py's band-ownership warning (a batch
                                      that registers a range whose symbols a band header still
                                      declares - the `illegal overloading` class)
                            m2c/      matt-kempster/m2c as a git submodule - the offline decompiler
                                      units/m2cinput.py feeds - see the `decompile-symbol` skill
docs/                     Where all documentation lives — ours and dtk-template's. Anything worth
                          writing down goes here. Keep docs short and to the point, not dense.
                          matching.md is the matching playbook; its ideas are indexed in the
                          "Matching playbook" section above, which doubles as the todo list.
                          plan.md is the campaign plan: every symbol in symbols.txt, the four steps per
                          symbol, the 80 % bar for closing one, and the order to work in.
                          memory-dump.md documents the shared Ghidra runtime memory dump: real SDK
                          symbol names, annotated struct layouts and data contents, used as an
                          oracle for names/signatures/data (never for codegen) - see below.
                          rso-modules.md documents the RSO module format, the inventory and the
                          splitter blocker.
```

### External oracles

* **Shared Ghidra project `MH3Shared` / runtime memory dump `/DolphinDump85.raw.keep`** (a dump of the
game running, reachable through the `ghidra` MCP server). It is the quickest way to turn a region full
of `fn_XXXX` into named SDK functions, to get a function's real signature, to confirm a struct's field
offsets/names, or to read a data blob this repo does not own yet. Example: it named the `RSO/runtime`
unit's thunks `RSONotifyPreRSOLink`/`RSONotifyPostRSOLink`, its `fn_804DA834` `FindExportIndex` and its
`fn_804DAA24` `RSORelocate`, and its `RSOModule` layout matched every offset we had derived by hand.
Three rules: it is **not** codegen evidence (flags and the compiler family still come from diffing the
retail bytes, see playbook 17), a `splits.txt` range derived from it must be **measured before and
after** - claiming the RSO unit's string pool *lowered* a function by 1.35 points, claiming its jump
table raised another by 0.08 - and it is **read-only**: the project is shared with another effort, so we
consult it and never modify it (no renames, no types, no comments, no imports, no saves), and our names
go in `config/RMHE08/symbols.txt` through `symedit.py`, never into Ghidra. Details and the recipes:
`docs/memory-dump.md`.

Inside `build/RMHE08/`:

* `src/<Unit>.o` — **our** compiled object (the candidate; ninja target `build/RMHE08/src/...`)
* `obj/<Unit>.o` — the **original** object split out of the DOL (the target)
* `main.elf`, `main.dol`, `main.MAP`, `ldscript.lcf`, `report.json`, `progress.json`, `ok`

objdiff compares the `src/` candidate against the `obj/` target. Keeping those two straight is essential.

## Build & verify

Windows-friendly (this is the supported setup here): Python 3.12 + `ninja` on `PATH`. The toolchain
(binutils, Metrowerks compilers, dtk, objdiff-cli, sjiswrap) is downloaded automatically into `build/`.

```sh
python configure.py            # regenerate build.ninja + objdiff.json (needed after editing configure.py)
ninja                          # default target: build/RMHE08/progress.json (builds + verifies)
ninja build/RMHE08/ok          # build main.dol and check it against config/RMHE08/build.sha1  <-- the real test
ninja build/RMHE08/src/Camellia/camellia.o   # compile a single translation unit
```

Other useful targets:

| Target | What it does |
| --- | --- |
| `ninja all_source` | Compile all configured source files (no link) |
| `ninja build/RMHE08/report.json` | objdiff report for all units (progress + per-function diffs) |
| `ninja diff` | `dtk dol diff` — report symbols in the linked ELF that don't match |
| `ninja apply` | `dtk dol apply` — apply symbol/label info from the linked ELF back into `symbols.txt` |
| `ninja baseline` | Save the current report as a baseline for regression checks |
| `ninja changes` / `ninja changes_all` | Compare against the baseline (markdown: `regressions.md`) |
| `ninja tools` | (Re-)download the pinned toolchain |

Notes:

* `python configure.py --warn all|error` adds compiler warnings; the committed default has no `-W` flag.
  `--non-matching` builds "equivalent" code for extra units without linking them.
* On non-Windows hosts a `--wrapper` (wibo/wine) is required to run the Metrowerks compilers; on Windows
  they run natively.
* `mwcc_sjis` (sjiswrap) wraps the compiler, so keep source files UTF-8 with no BOM.
* If results look impossible, the build tree is probably stale: `rm -rf build/RMHE08` then
  `python configure.py && ninja`.
* The **`dol split` is the slow step** and it re-runs whenever `symbols.txt`, `splits.txt` or the DOL
  changes - so a rename costs a re-split, which is why renames ride one batch (see the batch rule in
  `docs/plan.md`). It is **~18 s as configured** and **200-400 s with the asm dump on**: the dump is on
  demand (`write_asm: false` + `python tools/splits/dump_asm.py`) because only `tudiscover` reads
  `build/RMHE08/asm/`. Measurements, the link-ordering pitfall and the recipes for reading `.ninja_log`:
  `docs/build-performance.md`.

Verifying whether a unit, function or symbol matches is its own procedure — per-symbol objdiff plus raw ELF
evidence, and a specific set of traps (`complete_code_percent` lies, `ninja build/RMHE08/ok` cannot isolate
one unit, a function missing from the report is 0 %). Follow skill **`.agents/skills/objdiff-verify/SKILL.md`**
(the only tracked path under `.pi/`; everything else there is gitignored).

## The core loop: adding / matching a translation unit

1. **Find the unit.** Locate the function in `config/RMHE08/symbols.txt` (`grep`), get its address and size,
   and find the surrounding section ranges in `config/RMHE08/splits.txt`. If it is an unnamed `fn_XXXX`,
   look it up in the shared memory dump (`docs/memory-dump.md`) first - the real SDK name and signature
   usually come back in one query, and they tell you what the function is before you read a byte of code.
2. **Register it** in `config.libs` in `configure.py`: pick the right `mw_version` (this is a Wii title:
   `Wii/1.0` for REL-type code, `Wii/1.3` for runtime-style code — **not** the GC compilers) and an
   appropriate `cflags` group (`cflags_runtime` for runtime units, `cflags_base` otherwise).
   Start with `Object(NonMatching, "Dir/file.c")`.
3. **Create `src/Dir/file.c`** (and headers under `include/` if needed) - **in the same change as the
   registration, because a unit's source file is mandatory, and it is created with its functions, not just a
   header: the functions the unit is known to hold, with their decompiled bodies wherever they are recoverable
   and measured against the target.** A unit whose bodies cannot be written yet still gets its file, whose
   header says what it is, its range, why it sits there, what is unknown, and where its inventory and evidence
   live (`ledger.py unit <path>`, the map, `splits.txt`) - never a copied-out function list, which goes stale
   on the next range edit. Registering a unit without a source leaves the build warning about it and its object
   unbuildable - a bug, not a state.
4. **Add the splits** to `config/RMHE08/splits.txt`: one line per section with exact `start:`/`end:`
   addresses, including the small `.ctors`/`.dtors`/`.sdata` fragments that runtime units own (Wii linkers
   use `.ctors$10`, `.dtors$10`, `.dtors$15` — see `docs/getting_started.md`, "GC 2.7+ and Wii linkers").
5. **Compile and diff:**
   `python configure.py && ninja build/RMHE08/src/Dir/file.o`, then produce/refresh the report and inspect
   the unit's per-function diff (objdiff GUI reads the generated `objdiff.json`). When it does not match,
   work the **Matching playbook** index above - one idea at a time, keeping its status column current -
   instead of guessing at flags.
6. **Flip to `Object(Matching, ...)`** only once the unit matches (bytes/instructions + relocations).
7. **Prove it end-to-end:** `ninja build/RMHE08/ok` must finish green, i.e. `main.dol` matches
   `config/RMHE08/build.sha1`.

Keep changes small and verified. A micro-optimization for a function that was already matching is a
regression if the hash goes red.

## Gotchas learned in this repo

* **Compiler-flag drift is silent and fatal.** An earlier revision of `cflags_base` had `-O4,p`,
  `-inline auto`, `-Cpp_exceptions off` and `-RTTI off` removed (and `-use_lmw_stmw on` commented out).
  Nothing failed to compile; the resulting `main.dol` was just wrong. `Camellia` compiled 0x3244 bytes too
  large, and per-function size deltas summed to **exactly the total DOL growth (+12,868 bytes)** — that
  sum-vs-total check is a great way to diagnose "everything is slightly bigger" symptoms.
* **Don't trust a single objdiff number.** `complete_code_percent: 100.0` has been observed alongside
  `fuzzy_match_percent: 1.77` for the same unit, and again with 1.49 for Camellia. Cross-check
  `fuzzy_match_percent`, the per-function diff, and ultimately `build.sha1` / `ninja build/RMHE08/ok`.
  In the report JSON a *function* entry with **no** `fuzzy_match_percent` key is **0 %**, not 100 % — the
  unit's percent is exactly the sum of the listed partial matches over `total_code`, so arithmetic-check it.
* **`Object(Matching, …)` is a claim, not evidence, and it changes the link.** `NonMatching` is literally
  `False` in `configure.py` ("should not be linked"): those regions keep their original bytes, while a
  `Matching` unit's object is substituted in. A `Matching` flag on a wrong object is worse than no flag —
  and a failing `ninja build/RMHE08/ok` cannot tell you *which* unit is wrong.
* **A `.comment` version-byte difference means a different compiler build.** Dump it with
  `python tools/elf/elfsect.py <obj>` (also at `.agents/skills/objdiff-verify/scripts/elfsect.py`): the
  original Camellia object is `"CodeWarrior" 0e …`, our `Wii/1.3` build is `"CodeWarrior" 0f …`, and
  `config.yml`'s `mw_comment_version: 14` describes the original. Different version byte + `0 %`/size-very-
  different functions = suspect the compiler release, not the source.
* **`extab` / `extabindex` (and `.relaextabindex`) presence is flag evidence.** The original Camellia object
  has them; ours has none, i.e. the original TU was built with C++ exceptions enabled (or as C++) while our
  `cflags` group passes `-Cpp_exceptions off`. Treat it as a hypothesis to test on one unit, per rule 3.
* **`mw_comment_version: 14`** in `config.yml` must match the `.comment` section of the original objects.
  A mismatch makes the analyzer mis-identify the toolchain.
* **`quick_analysis: false`** is required while function boundaries are still being discovered; setting it
  to `true` skips boundary analysis and is only valid after analysis is complete and `symbols.txt` /
  `splits.txt` are generated.
* Sizes/addresses in `symbols.txt` and `splits.txt` are absolute addresses from the **unlinked** original
  DOL; they are not offsets, and section order matters (`.init`, `extab`, `extabindex`, `.text`, ...).
* Stale `build/` output causes false conclusions (an old object from different flags can look "matching").
  Prefer a clean rebuild of the specific unit, and `rm -rf build/RMHE08` when in doubt.
* Local agent scratch directories (`.lavish/` and everything under `.pi/` - notes, prompts, scratch) are
  gitignored; keep them that way and never add their contents to commits. `.agents/` is ignored **except**
  its skills folder, which is tracked in full (`.agents/skills/`) so every harness shares one set of skills.

## Conventions

* **Commit messages** (only once a commit has been approved — see Non-negotiables rule 6): short imperative
  subject, area-prefixed, e.g. `Camellia: match Camellia_Ekeygen`, `RMHE08: refresh symbols.txt`,
  `configure.py: add REL flags`. Describe *why* when fixing a mismatch.
* **Keep generated/large churn separate.** A `symbols.txt` regeneration or an analyzer settings change gets
  its own commit; never mix it with source changes or unrelated formatting.
* **Naming and commenting** (see "Commenting and naming" below): use the real name when it's known, leave
  dtk's generated `FUN_xxxxxxxx`/`fn_xxxxxxxx` names in place until they're understood, and keep function
  comments descriptive. Vendor files keep vendor naming (e.g. `Camellia/` uses `CAMELLIA_*` constants and its
  original MPL-1.1 header — keep those intact).
* **Commenting and naming** (applies to every unit we write):
  * **A comment on top of a function is a short description of what the function does** - one or two lines,
    in the present tense ("Rebases the module's section pointers, then patches every import's relocation
    chain."). It must **not** carry the symbol's name and **not** a matching percentage: the name is already
    the identifier below it, and the percentage lives in the objdiff report (and changes on every build).
    Residuals, flag evidence, name provenance and anything else that is about the *unit* go in the unit's
    file header comment.
  * **The unit's file header comment is the one place for the unit's own notes** - what it is, the `.text`
    range and function order, where its flags/evidence live, the residuals, and any load-bearing source
    shapes. Keep it to the essentials: one line per fact, no repetition of what `configure.py`, the playbook
    or a report already says, and no per-function inventory (addresses, sizes and instruction counts are in
    `symbols.txt` and the objdiff report). A header that re-argues the flag hunt or tabulates every function
    is noise the next reader has to skip - the two existing units' headers are the length to aim for.
  * **Use the real name when it is known** (the retail symbol map, the shared memory dump in
    `docs/memory-dump.md`, the SDK), or a descriptive name when you clearly have a better one - but it has
    to fit the **naming scheme of the surrounding symbols**, especially where siblings are already named:
    in the `RSO/runtime` unit `RSOStaticLocateObject`/`RSOUnLocateObject` make `LocateObject` an obvious fit,
    and `RSORelocate`/`RSORelocateSmallDataSection`/`RSOUnLink`/`RSONotifyPreRSOLink` match the `RSO*` public
    API around them. A name that reads like it belongs to another module is worse than `fn_xxxxxxxx`. Leave
    a generated name in place when there is no known or clearly better one - a speculative name is a bug.
  * **A rename is always two edits**: `config/RMHE08/symbols.txt` (which names the *target* object) and the
    source that defines/references it, in the same change - otherwise objdiff stops matching the symbol by
    name and reports it as 0 %. Do it through the proxy, never by hand:
    `python tools/symbols/symedit.py rename <old> <new> --dry-run` first, then for real; it refuses
    ambiguous renames, keeps the file's line endings, prints the one-line diff and lists the in-repo
    references that are the other half of the edit. Verify with `mt.py diff -u <unit> <symbol>` and keep the
    linked DOL hash unchanged. Never open or regenerate the map for this - it is 4.5 MB and must not enter
    an agent's context (skill: `symbol-map-editing`).
  * **Name `unk` variables, fields and parameters from the context they are used in** - what is stored,
    what it is compared against, which SDK type the offset belongs to, what the value is later passed to
    (e.g. `unk50` in a struct became `import_symbol_table_size` once the dump confirmed the layout, and an
    argument only ever used as a string pointer became `symbol`). Leaving `unk`/`unkNN` in place is fine and
    expected when the context does not support a name - do not invent one to fill the gap. **Exception: work done
    under the campaign plan (`docs/plan.md` §6.5) is held to a stricter standard** - there, no `fn_XXXXXXXX` or
    `unkNN` may survive in `src/`, every reconstructed type states its size, every field carries its offset and a
    context name (padding excepted), shared types live in one header, an `extern` lives with the unit that owns
    the symbol, and pointer arithmetic to reach a field is forbidden. `tools/units/stylelint.py` (roadmap 7.21)
    enforces those seven rules at the campaign's land gate.
* **Style:** match the file you're editing (vendor sources mirror upstream formatting; new project code
  follows the surrounding 4-space-indent C style). Files are UTF-8, LF endings (`.gitattributes`
  enforces the checkout).
* **Documentation:** `docs/` is the home for all documentation — put new knowledge there instead of
  leaving it in chat, commit messages or code comments. Write straight to the point: setup steps, recipes
  and findings as short bullets, not dense prose or oversized files. Split into one file per topic rather
  than growing a single wall of text. dtk-template docs already in `docs/` stay authoritative for template
  behaviour; add project-specific notes alongside them instead of rewriting them.

## Before claiming success

* [ ] `ninja build/RMHE08/ok` passes (for anything affecting the linked DOL), or the change is explicitly
      described as unverified.
* [ ] For a single unit/symbol: the object compiled **and** its objdiff diff shows the claimed match level
      (per-symbol `match_percent`, equal section sizes) — see the `objdiff-verify` skill.
* [ ] `git status --short` shows only intended files (no `build/`, no `orig/`, no scratch dirs). A lone
      `M AGENTS.md` just means the local-only block differs, which is expected.
* [ ] The committed `AGENTS.md` has no local-only block: `git show HEAD:AGENTS.md | grep -c '^<!-- LOCAL-ONLY'`
      → `0`.
* [ ] `symbols.txt` / `splits.txt` edits are byte-clean for the lines you didn't mean to touch
      (`git diff --stat` sanity check — these files are huge; a symbol rename goes through
      `python tools/symbols/symedit.py rename`, so its diff is exactly one line per symbol).
* [ ] No new compiler flags / tool version changes smuggled in.
* [ ] Nothing was committed or pushed unless the user asked for it (see Non-negotiables rule 6); staged vs.
      unstaged state reported clearly.
