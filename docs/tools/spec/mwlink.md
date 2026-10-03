# `mwlink` - Interrogate `mwldeppc.exe` about a real link: trace a unit, diagnose a failing link, verify the map, derive phases/anchors/records/alignment from the PE

<!-- generated from the module docstring of `tools/mwlink_debugger.py` at ec2609b46 by the tools-design lane; tightened by hand where marked -->

## Purpose

Interrogate the Metrowerks linker (``mwldeppc.exe``) about a real link.

## Users

profiles (`.claude/agents`) (5); skills (12); CLAUDE.md (1); docs (23)

## CLI

Subcommands: `info`, `messages`, `order`, `anchors`, `records`, `align`, `timeline`, `verify`, `trace`, `diagnose`, `phases`.
Flags: `--all-anchors`, `--args`, `--elf`, `--full-ctor`, `--gdb`, `--grep`, `--identity`, `--json`, `--kind`, `--ldscript`, `--limit`, `--link`, `--link-out`, `--map`, `--messages`, `--object`, `--out`, `--prove`, `--require-all`, `--rsp`, `--selftest`, `--splits`, `--unit`.
Exit codes: 0 ok, 1 findings or refusal, 2 could not run (the `lib.findings` convention; today's tool documents none, so `migration.md` records the current behaviour before changing it).
`--json`: the `lib.findings` schema `{tool, rows, ok, summary}` where the tool has `--json`; otherwise none.

## Inputs and outputs

Inputs -> outputs: main.elf, link rsp, mwld PE -> report.

## Invariants and rules

* This is the linker-side sibling of ``tools/mwcc-debugger/``. That tool answers "which optimizer pass did that?" for the *compiler* by driving ``mwcceppc.exe`` under gdb and classifying the PCode dump against the object. The compiler is tractable because every Wii ``mwcceppc.exe`` ships a CodeView ``NB11`` symbol blob naming its own functions; this tool starts by checking whether the linker has the same lever, and reports what it has instead.
* **The lever is not a symbol blob.** Every one of the 31 ``mwldeppc.exe`` files in ``build/compilers/{Wii,GC}/*`` has an *empty* PE debug directory - no CodeView blob, no symbols (``info`` prints this). What the linker does have, and what this tool uses, is two things the compiler-side technique cannot use:
* its own **diagnostics** - ``-v`` / ``-progress`` / ``-map`` emit a real phase timeline (``Linking:`` -> ``Optimizing:`` -> ``Writing:`` -> ``Layout: <section>`` xN -> ``Writing: <section>`` xN), so the "dump" is the link map and the "timeline" is the verbose stream (``timeline``);
* a **message catalog** in the PE resources - the linker's engine messages (including every phase name) live in the RT_STRING resource as length-prefixed UTF-16LE, which is why an ASCII ``strings`` pass misses them; ``messages`` decodes the catalog and ``anchors`` finds the code that references the address-referenced strings.
* **Anchors are derived, not transcribed.** In the same spirit as ``locate/pass_points.py`` (which takes the return address of every ``call <pass>``), ``anchors`` scans ``.text`` for instructions whose immediate operand is the VA of a known string and reports ``{anchor RVA: what the string is}``. The header calls the ctor/dtor name-dispatch sites out specifically because they are the documented Row 46 mystery, and ``anchors --prove`` (gdb) counts the hits on a real link - a breakpoint that never fires is reported as *unproven*, not as an anchor.
* **The phase table.** ``phases`` derives the linker's *phase* code the way ``locate/pass_points.py`` derives the compiler's pass table, but it has to go one level further because the phase messages are referenced by resource id: the message loader is the `call [LoadStringA]` site whose ``uID`` argument is not a constant, its callers are the one-per-message formatters, and the return address of ``call <formatter>`` is a phase anchor. ``phases --prove`` breaks on them - and on the loader itself - during a real link, prints the ``(id, text)`` stream the linker really emits, and reports every anchor that never fired as unproven. That run is also what corrected this tool's catalogue numbering: the id is ``(block_name - 1) * 16 + slot`` (50 of 50 observed messages agree).
* **Tracing one unit.** ``trace <unit|object>`` answers "how did this unit get linked?": whether the link kept it, where each of its sections landed (checked by reading the object's bytes back out of the output ELF), how its symbols resolved (the map *and* the output ELF's own symbol table), which relocations touched it (each one's field decoded out of the artifact and compared with the ABI), and where its ctor/dtor fragment went in the linker's fixed class order. The argument is a **unit name** (``Network/NetworkWiiMediator``) as well as a path: the object is resolved from the build's own link statement, and since the build writes no map, the tool links one into ``build/scratch/`` when it needs to. ``trace --link`` never writes ``build/RMHE08/main.elf``.
* **An erroring link is the production case.** ``diagnose`` runs the build's own link with ``-v`` and reports the phase stream, then every diagnostic classified against the message catalogue and attributed to the phase that printed it - a diagnostic the catalogue does not hold is reported *without* an id rather than given one. ``trace --link`` uses the same attribution when its link fails and then still traces the object against whatever map exists.
* **The internal records.** ``records`` documents the linker's own input-file record (stride, array, and each field) derived from the ``.comment`` parser's instructions, and ``records --prove`` reads that array out of a running link and cross-checks every field against the object it names. ``align`` derives where the linker aligns a fragment's address and what it compares - the answer to the ``tools/elf/objalign.py`` question. Fields that could not be derived are rows that say so; nothing is invented.
* **Health check.** ``verify`` clasps the link map against the ELF ``elf2dol`` will be run on: section addresses and sizes in ``main.MAP`` must *be* the section headers of ``main.elf``. It classifies first and stays loud: a ``FAIL`` names the first section that disagrees, and it is never a formality.
* **Ground truth check for a run**: ``anchors --prove`` and ``phases --prove`` stop the linker at the derived anchors and report what it was doing there; ``trace --link`` reports whether the artifact it traced is byte-identical to the one ``ninja`` built.
* Provenance: no code is copied from ``tools/mwcc-debugger/``. The PE header / section / data-directory / resource parsing here is our own, stdlib-only (that tree's ``locate/dissect.py`` is a different, symbol-blob-shaped helper). The *method* - dump the tool's own state, then prove the dump describes the artifact before believing it - is borrowed from ``locate/verify_pcode.py``; the anchor derivation borrows its shape from ``locate/pass_points.py``. The cc0 / fork provenance of ``tools/mwcc-debugger/`` does not reach this file: this is original work in this repository.

## Lib dependencies

binary, proc, project.

## Test contract

Tier: fixture (ELF/object fixtures); smoke: a real link.
Today's selftest: in-file `selftest()` (`--selftest`).
Target: `tools/tests/top/test_mwlink.py` on `lib.testing` (`FixtureTree`/`GitFixture`/`ElfBuilder`); live-tree checks, if any, under `TIER='smoke'` and tolerant.

## Known gaps

becomes the package `tools/mwlink/` (design 5)
