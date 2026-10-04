# Provenance

## Upstream

* URL: <https://github.com/cadmic/mwcc-debugger>
* Commit cloned: `bad9cea2423bed957188c930086f9dabe669d30c`
  ("Fix printing of negative constants", 2025-06-13)
* Contents at that commit: `README.md` and `mwcc_debugger.py` and nothing else.
* Vendored files, byte-for-byte identical to the upstream blobs (verified with
  `curl https://raw.githubusercontent.com/cadmic/mwcc-debugger/<sha>/<file>`):

  | file | sha256 |
  |---|---|
  | `upstream/README.md` | `3f42503a27ab3b80a7364b4a137bd4001ba52d848dc129884c70a4bb9990609d` |
  | `upstream/mwcc_debugger.py` | `c8da21ad79ff5b342c45037a4fa5bea9fb993bba13e12817a0aa1fc2f72f6fe3` |

  Both hashes were re-checked against the blobs *as committed* (the checkout
  has `core.autocrlf=true`, so the working copies were normalised to LF before
  being added; `git show HEAD:...` reproduces the upstream bytes exactly).
  The ported `mwcc_debugger.py` is
  `7f6a1ec9e8fd95874574db2fd34d137d8b831659a1191ccf2015d5314f7ef7ef`, and
  `make_port.py` regenerates it byte-for-byte from `upstream/mwcc_debugger.py`.

## Licence status: **upstream has no licence**

The repository at that commit has **no `LICENSE`, `COPYING`, `NOTICE` or any
other licence or copyright statement** - only the two files above, neither of
which contains a licence header.  GitHub's API reports `"license": null` for
the repository, and it is not published on PyPI.  There is therefore **no grant
of rights from the author** for this code; by default that means "all rights
reserved" and this vendoring is a decision the owner has to make knowingly.  It
is recorded here, and in the branch report, rather than buried in a commit.

What has been done about it:

* the upstream files are kept verbatim in `upstream/` so the original text, its
  authorship and this commit's relationship to it are unambiguous;
* the porting changes are *separable*: `make_port.py` regenerates the ported
  `mwcc_debugger.py` from `upstream/mwcc_debugger.py` by an ordered list of
  textual replacements, and this note records exactly what they are.  If the
  owner decides the upstream terms are unacceptable, deleting the derived
  `mwcc_debugger.py` (and keeping `make_port.py`, which is our own work) is a
  one-file change;
* nothing about the tool is load-bearing for the build - it is a research tool.

Two other ports of the same script were consulted for method (not copied):
`monde-lointain/mwcc-debugger` (a heavily documented GC/1.3.2 port) and
encounter's `retrowin32` `gdb-stub` branch.  No code was taken from either.

**Action for the owner:** if the tool is to stay long-term, ask upstream for a
licence (the author is active in the decomp community and a MIT/CC0 grant is a
one-line change), or reimplement the driver from this specification.  Until
then, treat `upstream/` and `mwcc_debugger.py` as "all rights reserved" and
keep them inside this repository.

## Vendor, not submodule

Vendored rather than added as a git submodule on purpose: this is a **port**.
Our changes (native-Windows execution, the version-table split, the Wii/1.3
support and the record decoders that go with it) have to be committed in *this*
repository, and a submodule cannot carry local modifications - it would pin a
foreign tree and force every user to maintain an out-of-tree patch.  Keeping
the untouched upstream files in `upstream/` gets the provenance benefit a
submodule would have given (exact commit, verifiable hashes) without that cost.

## Our changes

Everything below applies to `mwcc_debugger.py`; the upstream copies are
untouched.  `make_port.py` is the authoritative, reviewable list (each
replacement is anchored on text that must exist, so it fails loudly if the
vendored copy changes).

**Emulator removal (the point of the port)**

* The compiler runs **natively**: `file <mwcceppc.exe>` + `set args ...` +
  `run`, instead of `target remote localhost:9001` against retrowin32's stub.
  `set architecture i386` / `set osabi none` are only issued on the emulator
  path now; `--emulator PATH` keeps upstream's behaviour available on POSIX.
* The node-name and opcode-info tables are read from the inferior's memory, so
  they are loaded *after* the first stop rather than before `run`.
* `startup-with-shell off` is set (best-effort) so the compiler's `-pragma
  "cats off"` arguments are not re-split by `cmd.exe`.
* A check that the image really is mapped at its PE `ImageBase` (the version
  tables are image-relative) - it fails with one sentence instead of dumping
  addresses from the wrong place.

**Version data moved out of the code**

* `init_mwcc_version()`'s inline chain of `if read_memory(MAGIC_ADDR, 10) ==
  b"Metrowerks"` tests is gone; identification happens in the launcher, from
  the PE file, against a table of `(rva, expected bytes)` probes in
  `versions.py`, and the address data lives there too.
* Addresses are stored as image-relative RVAs and converted at load time
  (upstream stored absolute VAs that silently assumed a 0x400000 load).
* The opcode-table entry size is per-version data (`0x10` / `0x12` / `0x16`),
  not a hard-coded `if` chain.
* Which colouring class number means GPR/FPR, which stack slot holds the
  assigned-node list, and whether the tool may call `CMangler_GetLinkName` are
  per-version data.

**Windows correctness**

* `shlex.split` on the compiler command line is replaced by a Windows-aware
  splitter - `shlex` eats the backslashes of `src\foo.cpp`, silently compiling
  the wrong file.
* `--args` is passed to gdb through `set args` with gdb's escaping rules, and
  the launcher/`gdb` state crosses in an environment variable instead of an
  `-ex 'py ...'` string, so paths with spaces or quotes cannot break it.
* Default output directory name is sanitised (mangled C++ names contain
  `?@<>:*`, which Windows rejects in a path).
* Output files are written with `\n` and explicit UTF-8, so dumps are
  byte-comparable across platforms.
* `gdb` is resolved with a search, and a missing gdb produces install
  instructions (MSYS2 package, or `fetch_gdb.py`) instead of a traceback.
* `fetch_gdb.py` installs a native mingw-w64 gdb and its runtime DLLs without
  MSYS2/pacman.

**Repository integration**

* The launcher carries the repository's one-line `sys.path` prologue, so
  `versions.py` can use `tools/lib/binary/pe.py` (the repository's one PE
  reader, shared with the linker debugger) in the launcher and inside gdb.
* The gdb session is started through `tools/lib/proc.py` (`lib_proc.run`),
  with gdb's output left on the terminal.

**Wii/1.3 support (new)**

* A version row for `build/compilers/Wii/1.3/mwcceppc.exe`, verified against
  the CodeView symbol table the binary carries (and re-verified on every run).
* `pcode_breakpoints` are *derived* from the pass call sites by
  `locate/pass_points.py` rather than transcribed.
* Three new record layouts (`wii13`): object, PCode instruction + operand, and
  interference-graph node.  Each is documented against the disassembly it came
  from in `locate/README.md`.  The neighbouring GC paths are upstream's code,
  unedited.
* Frontend/successor/line-number fields that are *not* derived are skipped with
  an explicit message in the tool's log and in the dump file, never fabricated.

**Small upstream-adjacent fixes**

* `isinstance(arg.kind, str)` operands (the raw markers for underived Wii
  operand kinds) are printed instead of raising.
* `if arg.obj_addr:` instead of `!= 0`, so an operand that carries no object
  does not try to dereference `None`.

## Regenerating the port

```
python tools/mwcc-debugger/make_port.py \
    tools/mwcc-debugger/upstream/mwcc_debugger.py \
    tools/mwcc-debugger/mwcc_debugger.py
```

The output should be byte-identical to the committed `mwcc_debugger.py`; any
difference is either an intentional edit that should also be added to
`make_port.py`, or a sign that the vendored upstream copy changed.

## Licence

The vendored upstream carried **no licence** when it was taken (2026-09-27): no `LICENSE`/`COPYING`/`NOTICE`
file, no header in either file, and GitHub reported `license: null`. That is all-rights-reserved by default,
which is why this tree sat unlanded for a decision instead of being committed.

**The repository owner's decision (2026-09-27) is to include it as a fork and to license the fork - like the
rest of this repository - under CC0 1.0 Universal** (`LICENSE` in this directory, a copy of the repository's
own), **and to reconcile with upstream afterwards.** Nothing here is load-bearing for the build: delete the
directory and `ninja` is unaffected.

What that grant can and cannot cover, stated plainly: the owner can license their own work - the Windows port
in `mwcc_debugger.py`, `make_port.py`, `fetch_gdb.py`, `locate/`, and `versions.py`'s Wii/1.3 row with its
self-check - and does not own the two files in `upstream/` nor the upstream portions of the driver. Those are
reproduced verbatim (hashes above) with attribution to `cadmic/mwcc-debugger` @ `bad9cea2423bed957188c930086f9dabe669d30c`.

**Follow-up: ask the author for a licence (MIT/CC0) and relicense the tree accordingly.** Until that happens
this is a source-available fork of an unlicensed upstream, not a copy upstream has relicensed.
