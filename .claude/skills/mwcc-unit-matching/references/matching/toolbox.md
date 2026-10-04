# Toolbox

All of them are unit-agnostic: pass `-u <unit>` (or omit it when the repo has exactly one unit with
source), where `<unit>` is any of `Lib/file`, `main/Lib/file`, `src/Lib/file.c` or the object path.
`tools/lib/` is the shared layer - unit resolution and the real ninja command line (`lib.units`), the ELF
reader (`lib.binary`) - and `tools/units/unitinfo.py` with no arguments lists the units it can work on. **The last four rows
are the analysis tier** and take a unit *name* - or, for `callers.py`, an address or a symbol: they answer the
questions the flag and shape tools cannot, and each is introduced where a lane reaches for it (`flipcheck.py`
before any flip, the compiler debugger when a residual is down to one instruction, `mwlink_debugger.py` when a
flip moves the DOL hash, `callers.py` for who-calls / who-reads).

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
| `tools/units/flipcheck.py <unit>` | is this object **flip-ready**: per-section sizes and bytes against the target object, undefined/foreign symbols, the row-36 trim risk, and the `.comment` per-symbol active flags |
| `tools/units/callers.py <address\|name>` | who **calls** this function / who **reads** this data, from a whole-DOL **address-keyed** index (`call`/`branch`/`addr`/`read`/`write` sites, `--pointers` for `.4byte` entries: a function-pointer table, a vtable, an `@eti_`) - the asm dump is stale, so names resolve per run |
| `tools/mwcc-debugger/` (`locate/verify_pcode.py <dump> <object.o>`) | the **compiler's own IR**: the PCode stream after each optimizer pass and the register allocator's decisions, with the dump health-checked against *your* object first (`MATCH` final / `PASS-DELTA` + the attributed pass early) |
| `tools/mwlink_debugger.py trace <unit>` | the **link's own view** of one unit: was it kept, where every section landed, how its symbols resolved, which relocations were applied - `verify` health-checks the map/ELF (`--identity`), `diagnose` prints the failing link's phase stream with catalogue ids, `align --unit` the rows a claimed start cannot honour |
