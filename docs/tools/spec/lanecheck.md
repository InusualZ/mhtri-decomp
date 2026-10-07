# `lanecheck` - The mechanical pre-review of a lane's branch: owners by address, wrong callees, empty stubs, flip blockers, GUESS names

## Purpose

Runs, over the units a branch's diff touches, the checks a code reviewer did by hand each round, and prints one
`file:line | class | what | fix hint` line per finding. It is advisory: the landing gate does not run it; a reviewer or
the lane runs it before the review round so the human review starts from the judgement calls.

## Users

The `codereviewer` and `decompiler` lanes before a review round; the orchestrator triaging a branch.

## CLI

```
python tools/units/lanecheck.py                          # the working tree against merge-base(main, HEAD)
python tools/units/lanecheck.py --branch worker/x        # a branch's committed tree, read from git (nothing checked out)
python tools/units/lanecheck.py --branch C --base C~1    # one commit (the historical sweep below)
python tools/units/lanecheck.py --range BASE..HEAD       # a landed range: the touched files whole, carried-over findings listed
python tools/units/lanecheck.py --no-flipcheck --no-dump # skip the two checks that read outside the diff
python tools/units/lanecheck.py --json                   # {tool, ok, base, head, units, skipped, seconds, rows, summary}
python tools/units/lanecheck.py --strict                 # also fail when a check could not run
```
Flags: `--branch B`, `--base REF` (default `main`; the diff starts at the merge base), `--range BASE..HEAD`, `--no-flipcheck`, `--no-dump`,
`--dump FILE` (default `$MHTRI_DUMP_SYMBOLS` or `lib.dumpsyms.DEFAULT_DUMP`), `--strict`, `--json`, `--root`.
Exit codes: 0 no finding (a check that could not run is listed `not checked:` and does not fail without `--strict`),
1 at least one finding (or, with `--strict`, a skipped check), 2 could not run (git failed).
`--json` rows are `lib.findings.Finding` dicts (`rule` = the class, `file`, `line`, `token`, `detail`) plus `hint` and
`carried`; the payload also carries `carried` (the count).

## Inputs and outputs

Reads `git diff -U0 <merge-base> [<branch>] -- src` (added lines), the head side's files (`lib.facts.Tree`: the working
tree or `git cat-file` at the branch), the head's `symbols.txt`/`splits.txt`/`configure.py`, the tree's
`build/RMHE08/{src,obj}` objects for the touched units, `flipcheck.py --json` (one subprocess for all units) and the
runtime dump (`lib.dumpsyms`). Writes stdout only.

## Invariants and rules

* **Touched units** are the registered units (`configure.py` `Object` rows) whose source or own header the diff changed.
* **(a) owner-by-address / stale-path.** In the added lines' comment text (`lib.comments.comment_only`): every cited unit
  path (`dir/name.c|cpp|h`, `src/` optional) that names no file in the head tree is `stale-path`. An ownership claim is
  a symbol (a generated stem, a backticked map name, or an address) and a path joined by a verb in one clause - passive
  `X ... owned by|owner|defined in|defined by|lives in|belongs to PATH`, the path right after the verb, or active
  `PATH owns|defines X` - and the symbol's address (the map row, or the stem's own address) is resolved against the
  head `splits.txt` by ADDRESS; a cited registered unit that does not own it is `owner-by-address`. `its own view`,
  `declared in` and a bare mention pair nothing.
* **(f) unowned-claim.** A line saying `unowned`, `unclaimed`, `no registered owner|unit`, `nobody owns` or `has no
  owner`, and an address or map symbol in the same clause that a registered `splits.txt` range covers.
* **(b) wrong-callee.** `lib.objcompare.callee_diffs` (what `relocdiff --callees` prints) per touched unit; a
  difference is named when either spelling, its owner stem or its identifier occurs in the header's RESIDUALS (else the
  header minus its RANGE line). One item per function with the unnamed differences. It reads the objects in this tree's
  `build/`: run it from the branch's worktree for the branch's own objects.
* **(c) empty-stub.** A function definition the diff added whose body has no statement but `(void)x;` casts, while
  retail's (the map row's size) is more than 8 bytes: a finding unless the header's residuals say it is unwritten
  (`unwritten`, `stub`, `empty`, `0 %`, ...) - and a different finding when the header lists it as a partial residual.
  A constructor (`Class::Class`) or destructor (`Class::~Class`) with an empty source body is **written** when our
  compiled object emits one of its `__ct__<len>Class...` / `__dt__<len>Class...` rows with more than 8 bytes (the
  compiler writes the vtable store and the member and base calls); with no object to read it stays a finding. An
  overloaded constructor is read leniently: any emitted row clears it.
* **(d) flip-blocker.** flipcheck's problems of four classes - an undefined reference, a row-36 force-active function,
  a section claimed and not emitted, a size gap or an unclaimed emitted section outside `.text`/`extab`/`extabindex` -
  each must be mentioned in the header's residuals by a name or its section. Byte differences of a partial unit are not
  read.
* **(e) guess.** A name defined on an added line, not generated (rule 7's), whose map row's ADDRESS carries no real
  (non-placeholder) dump name equal to it, and that no NAMES **sentence** containing `GUESS`/`GUESSES`/`GUESSED` mentions
  (the section's wrapping and ` * ` margins are undone and it is cut after `.` and `;`, so `A, B and C are GUESSES
  (why)` over several lines marks all three). By address, never
  by name: a class-qualified dump name and its mangled map spelling never compare equal as text.
* **`--range BASE..HEAD`** judges a landed range: the diff is exactly `BASE..HEAD` (no merge base), every file it
  touched is judged **whole** at HEAD, and a finding of a line class (owner-by-address, stale-path, unowned-claim,
  empty-stub, guess) on a line the range did not add is printed `[carried over]`, counted apart, and does not fail the
  run; an added finding does. Unit-level findings (wrong-callee, flip-blocker) are never marked carried over.
* **The hint says what it read.** With `--branch` or `--range` every hint ends `(reads committed state only: commit the
  fix, then re-run)`: a fix left in the working tree is not seen.
* **`name-only` is not a wrong callee.** `callee_diffs` pairs one of our compiler-local labels (`@N`) with one target
  symbol of the same relocation type that resolves to the same address (section, symbol value + addend) as `name-only`:
  a naming difference, never reported by (b).
* A check that could not run (no object, no dump, flipcheck off or failing) is listed `not checked:` - never silently
  clean.
* **Whose build it judges.** It builds nothing. The first output line names the tree whose `build/RMHE08` objects (b)
  and (d) read, how many touched units have an object there and the oldest/newest object time (`build` in `--json`),
  and with `--branch` says it is this tree's build, not the branch's. Per unit those two checks are refused
  (`not checked: wrong-callee/flip-blocker <unit>: refused - <reason>`) when the object cannot stand for the judged
  source: older than the unit's include closure on disk (`lib.report.unit_reasons`), or, with `--branch`, the
  branch's copy of the source is not the file on disk. Measured: `--branch faa04ea9c` from this worktree refuses all 35
  units (sources moved since, objects older than `src/nw4r/math.h`), where the first version judged them silently.

## Measured

* 2026-10-06, the historical sweep (each commit against its parent, from MAIN's build at 65b341058, so (b) and (d) read
  today's objects against that commit's headers):

  | commit | files | units | findings | time |
  | --- | ---: | ---: | --- | ---: |
  | `3f04cda47` (lobby sweep) | 45 | 19 | wrong-callee 19 | 8.5 s |
  | `faa04ea9c` (g3d sweep) | 62 | 35 | stale-path 2, wrong-callee 42, flip-blocker 4 | 8.4 s |
  | `d29784bd7` (Pl sweep) | 44 | 15 | stale-path 5, wrong-callee 6 | 7.3 s |

  The stale paths are real (`enemy/fn_801A9540.cpp`, `camera/fn_802B5C58.cpp`, `ai/fn_802D44F4.cpp`,
  `hud/net_char_sync.cpp`, ... added in comments that the tree did not have); the four flip blockers are
  `g3d/g3d_state.cpp`'s claimed-not-emitted `.data`/`.rodata`/`.sdata`/`.sdata2`, which its header does not record.
  Before the clause rule, a first cut paired a symbol with the nearest path and raised 8 false `owner-by-address` items
  on `3f04cda47` and 4 false `unowned-claim` items on `faa04ea9c` (`... 0x807911A8.  Both edges are the unclaimed run's`).
* Sweeps add no definitions, so (c) and (e) are silent there; on a body branch they judge only added definitions.

## Lib dependencies

`lib.facts` (`Tree`, `parse_added`), `lib.comments` (`comment_only`), `lib.cscan` (definitions and bodies),
`lib.objcompare` (`callee_diffs`, `symbol_locations`), `lib.dumpsyms` (the dump by address), `lib.project` (`Ownership`, `configure`),
`lib.names`, `lib.findings`, `lib.cli`, `lib.git`, `lib.proc`, `lib.repo`; `flipcheck.py` as a subprocess (no import).

## Test contract

Tier: fixture (`tools/tests/units/test_lanecheck.py`, 34 checks, 15 of them for constructors/destructors, the GUESS
sentence forms, `--range` and the committed-state hint; the first 19: a `GitFixture` repo with two units; a branch whose
added comments mis-cite an owner, name a gone path, call owned data unowned (and truly unowned data and a phrase in
another clause, which are not findings), add an empty 0x20-byte stub and a 4-byte one, and add names the injected dump
does not carry; the working tree mode and a GUESS mark; `check_callees` and `check_blockers` on fixed inputs; the CLI's
lines, JSON, exit codes and `--strict`; the object gate: refused for a branch source that is not on disk and for an
object older than its source, judged when current - `object_reasons` returning nothing fails 2). Mutations: no clause break fails 1, no tiny-function exemption 1, an
`owner_at` that finds nothing 5, the whole header as the residual text 1.

## Known gaps

* (b) and (d) only read the invocation tree's `build/`: a branch is judged there only when its sources are the ones on
  disk and the objects are current; otherwise they are refused, not rebuilt.
* The ownership-claim grammar is English and narrow on purpose: a claim phrased otherwise is not judged.
* A header without a RESIDUALS label is read whole (minus RANGE), so a section named anywhere else counts as named.
