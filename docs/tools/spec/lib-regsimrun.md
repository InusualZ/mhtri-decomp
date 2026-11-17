# `lib/regsimrun` - run MWCC under the debugger on one unit's compile command

## Purpose

Builds the debugger's compiler command line from the unit's real one (`recompile.py --dry-run`'s: MAIN's graph re-read
from this tree's `configure.py`), finds or fetches the gdb into the slot, compiles the unit with its own compiler and
with the Wii/1.3 stand-in, dumps one function, and says whether the stand-in's function is the unit's own.

## Users

`tools/mwcc-debugger/regsim.py run` (`mt.py regsim run`).

## Public API

* `quote_arg(tok)`, `command_line(tokens)`: the `-a` string the debugger splits back to the same tokens (`cats off` is
  `"cats off"`); `ValueError` for a token holding both quote kinds.
* `split_compile(cmd) -> Compile(compiler, flags, source, version)`; `Compile.tokens(out_dir, compiler=None,
  flags=None)` is the bare invocation (no wrapper, no `-MMD`, no post-processing); `stand_in_compiler(compiler)`;
  `unit_compile(spelling, root, main) -> (Compile, full command, notes)`.
* `locate_gdb(root, explicit=None, which=, fetch=) -> (path, how)`: `--gdb`, `<root>/.pi/tmp/mwcc-dbg/mingw64/bin/gdb.exe`,
  `PATH`, else `fetch_gdb.py --dest <root>/.pi/tmp/mwcc-dbg` (network once). `SystemExit` when none can be had.
* `compile_into(cmd, root, scratch, compiler=None, retry_unknown=False) -> (rc, log, object, dropped flags)`: with
  `retry_unknown` an option the stand-in does not know is dropped and the compile repeated.
* `run_dump(gdb, tokens, function, out_dir, root, size, timeout)`, `dumped_symbol(dump_dir)`.
* `compare_function(real_obj, other_obj, symbol) -> Comparison(same_bytes, same_relocs, first_diff, sizes, notes)`:
  the function's `.text` bytes and its relocation targets (local `@N` names folded).

## Invariants and rules

* Writes only under `<root>/.pi/tmp/` (the gdb, `regsim-run/<unit>/<function>/`); nothing outside the checkout.
* The stand-in is `Wii/1.3`, the one compiler the debugger's address tables are verified for.

## Test contract

Tier: fixture - `tools/tests/lib/test_regsimrun.py`: the quoting round-trips through the debugger's splitter, the split
command and the stand-in path, gdb discovery (fetch once, slot copy beats PATH, a missing `--gdb` refuses), the
comparison (identical, a changed word, a changed callee, a missing symbol). Measured by hand on the real rows (see
`spec/regsim.md`).

## Known gaps

A unit whose flags the stand-in rejects beyond the unknown-option retry is refused. The dump is one function: a
later pass's temps are not nodes of pass 1's graph.
