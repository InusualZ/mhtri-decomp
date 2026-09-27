# mwcc-debugger (Windows-native port)

Dump the internals of the Metrowerks PowerPC compiler while it compiles one of
our source files: the frontend AST, the backend PCode between optimisation
passes, and the register allocator's interference graph and priority order.

This is a port of [cadmic/mwcc-debugger](https://github.com/cadmic/mwcc-debugger)
(see `PROVENANCE.md` for the commit, the licence status and every change we
made).  Upstream runs the Windows compiler under
[retrowin32](https://github.com/evmar/retrowin32) - an emulator whose entire
purpose is to run a Windows x86 binary on a POSIX host - and attaches `gdb` to
the emulator's gdb stub.  On Windows the compiler already runs natively, so the
emulator is the part that goes away: this port runs `gdb` **directly on
`mwcceppc.exe`**.  (You can still use the emulator path on a POSIX host with
`--emulator path/to/retrowin32`.)

Supported compiler builds:

| version  | status                                                            |
|----------|-------------------------------------------------------------------|
| `Wii/1.3`| **ported and verified here** - PCode dumps + register allocation   |
| `GC/1.1` | address tables carried over from upstream; **not verified here**   |
| `GC/2.6` | address tables carried over from upstream; **not verified here**   |

## Install: a gdb that can debug a 32-bit Windows process

Windows needs a **native mingw-w64 gdb**.  A Cygwin/MSYS gdb cannot debug a
native `mwcceppc.exe`, and neither can a gdb built for a cross target.

With MSYS2:

```
pacman -S mingw-w64-x86_64-gdb
# then, e.g. --gdb C:/msys64/mingw64/bin/gdb.exe
```

Without MSYS2 (downloads the package and its runtime DLLs into a prefix):

```
python tools/mwcc-debugger/fetch_gdb.py --dest C:/tools/mwcc-dbg
# then, e.g. --gdb C:/tools/mwcc-dbg/mingw64/bin/gdb.exe
```

Either way put it on `PATH` and `--gdb` can be omitted.  If no gdb is found the
tool prints exactly this advice instead of a traceback.

## Usage

```
python tools/mwcc-debugger/mwcc_debugger.py \
    [-g PATH_TO_GDB] \
    -a '<mwcceppc.exe command line>' \
    FUNCTION_NAME [OUTPUT_DIR]
```

The command line is the **bare** `mwcceppc.exe` invocation - no `sjiswrap.exe`,
no `python tools/elf/objalign.py`, no `transform_dep.py`.  Get it with
`ninja -t commands | grep <source file>` and cut everything before
`mwcceppc.exe` and everything after the last compile flag:

```bash
ninja -t commands | grep 'fn_8004C9A0.cpp'
# cmd /c build\tools\sjiswrap.exe build\compilers\Wii\1.3\mwcceppc.exe -nodefaults ... -O3 ... -c src\fn_8004C9A0.cpp -o build\RMHE08\src && <post-processing>
#                              ^--------------------------------------------- from here ---------------------------------------------^
```

### The exact invocation that works in this repository

From the repository root, in git-bash (one line; `\` continues it):

```bash
python tools/mwcc-debugger/mwcc_debugger.py \
  --gdb "$(command -v gdb.exe)" \
  -a 'build/compilers/Wii/1.3/mwcceppc.exe -nodefaults -proc gekko -align powerpc -enum int -fp hardware -Cpp_exceptions off -inline auto -pragma "cats off" -pragma "warn_notinlined off" -maxerrors 1 -nosyspath -RTTI off -fp_contract on -str reuse -multibyte -i include -i build/RMHE08/include -DBUILD_VERSION=0 -DVERSION_RMHE08 -DNDEBUG=1 -O3 -inline noauto -Cpp_exceptions on -lang=c++ -sym on -c src\fn_8004C9A0.cpp -o build/mwcc-debug' \
  fn_8004C9A0 build/mwcc-debug/dumps
```

Notes on that command line:

* `-sym on` is what upstream recommends for line numbers.  Wii/1.3 accepts it
  and it does not change code generation, but **this port does not yet print
  per-instruction line numbers** (the PCode record's line field is still
  undetermined), so today it only documents intent.  Blocks do print the
  function's line.
* `-o build/mwcc-debug` replaces the build's `-o build\RMHE08\src` so the debug
  run can never overwrite a real object.  Keep the output path under `build/`.
* The tool stops the compiler at the end of the function's codegen, so the
  object file is **not** written by a debug run.
* From `cmd.exe`/PowerShell the same command works - pass the whole compiler
  command line as one quoted argument.  (The launcher splits it itself,
  Windows-style: `shlex.split` would eat the backslashes of `src\foo.cpp`.)

### Output

For every backend pass and register-allocation pass the tool writes a text file
into `OUTPUT_DIR` (default `debug-<FUNCTION_NAME>`, sanitised because mangled
C++ names contain `?@<>:*`):

```
backend-00-<pass>.txt            PCode after <pass>, block by block
regalloc-gpr-pass-1-all.txt      every interference-graph node, in vreg order
regalloc-gpr-pass-1-assigned.txt the priority-ordered assignment list
frontend-NN-ast-<pass>.txt       AST dumps (Wii/1.3: not yet, see below)
variables.txt                    stack frame layout (GC/1.1 only)
```

## What works, and what does not

**Works on Wii/1.3 (verified against the emitted object - see below):**

* `backend-*.txt` - the complete PCode instruction stream per basic block,
  after each of the 34 passes a one-function `-O3` unit runs, with mnemonics,
  register operands, immediates and block lines.
* `regalloc-*.txt` - the interference graph (virtual register, interfering
  registers, cost, degree) in priority order, and the physical register each
  virtual register was assigned.

**Does not work yet on Wii/1.3 (deliberately reported, not faked):**

* `frontend-*.txt` - the AST dumps.  Every breakpoint fires and the tool says
  in its log that it skipped them; the frontend record layout (`ENode`,
  `Statement`) for this build is not derived.  Upstream's GC layouts do not
  apply: Wii/1.3 is a later Metrowerks core with different records throughout.
* `variables.txt` - the stack-frame dump (upstream only ever had it for
  GC/1.1).
* Successor/predecessor/label lists and per-instruction line numbers inside
  the PCode dumps.  The instruction stream and block index/line/count are
  there; the block's edge fields are not derived, and each dump says so.
* Operand *rendering* for two cases: kinds the compiler calls "fixups"
  (printed `fixup[...]`, they are the relocations the assembler patches later,
  e.g. the TOC address of a `lis`/`addi` pair) and branch targets (printed
  `@L<id>`, they need the block label list).

The GC/1.1 and GC/2.6 rows are carried over from upstream unchanged except for
the address-base conversion (see `versions.py`).  They cannot be exercised on
this machine - that path needs retrowin32, which needs a POSIX host - so treat
them as *ported but untested here*.  Their record decoders are upstream's code,
unedited, and their address tables are lifted mechanically from upstream by
`locate/extract_upstream_tables.py`.

## How it works

`mwcc_debugger.py` has two roles.  Started as `python mwcc_debugger.py ...` it
identifies the compiler build from the PE file, checks the tool chain, and
re-executes itself under gdb.  Sourced by gdb (`-x mwcc_debugger.py`) it sets a
breakpoint at the start of the per-function driver, waits for the requested
function, then sets one breakpoint per dump point and writes the state out.

Everything compiler-build-specific lives in `versions.py`:

* addresses (image-relative RVAs, converted to VAs at load time, with a check
  that the image really is loaded at its PE `ImageBase`),
* the `opcodeinfo` table's entry size (0x10 / 0x12 / 0x16 - it changes per
  build),
* the `{address: pass name}` breakpoint tables,
* which colouring class number means GPR and which FPR,
* which record layout family the build uses (`gc11`, `gc26`, `wii13`).

Adding a build is therefore a data edit, which is the point of the exercise:
`locate/README.md` records how the Wii/1.3 row was derived, and
`locate/dissect.py` can do most of the work again for another Wii build because
several of them ship their own symbol table.

## Verification

The port was checked end-to-end on `src/fn_8004C9A0.cpp`
(`fn_8004C9A0`), whose header documents a known residual.

* `backend-34-after-code-labels.txt` (the last PCode dump) reproduces the
  object `powerpc-eabi-objdump -d` emits, instruction for instruction -
  including the `add`/`addi`/`lbz` -> `lbzu r0,0xe00(r7)` peephole fusion the
  source file's residual comment describes, and the `stwu/mflr/stw r31,0xc(r1)`
  prologue.
* `regalloc-gpr-pass-1-assigned.txt` assigns `r32 (self) -> r31` and
  `r35 (equip) -> r7`, which are the registers that function's prologue and
  address computation actually use.

`locate/verify_pcode.py` runs that comparison (it needs
`build/binutils/powerpc-eabi-objdump.exe`, which the repository already has).

## Layout

```
mwcc_debugger.py    the tool (upstream + the port - see PROVENANCE.md)
versions.py         every compiler-build-specific fact (data)
upstream/           the upstream files, byte-for-byte as cloned
make_port.py        regenerates mwcc_debugger.py from upstream/ (reviewable diff)
locate/             how the Wii/1.3 facts were derived + verification tooling
fetch_gdb.py        installs a native gdb without MSYS2
```
