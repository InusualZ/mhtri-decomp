# `m2cinput` - Turn a target object's objdump into the GNU-as shape m2c accepts (relocation spelling, `loc_` labels, tail calls, `@` names, jump tables)

<!-- generated from the module docstring of `tools/units/m2cinput.py` at ec2609b46 by the tools-design lane; tightened by hand where marked -->

## Purpose

Turn a target object's disassembly into the assembly `m2c` reads.

## Users

skills (4); CLAUDE.md (3); docs (5); imported by `phantom`

## CLI

```
python tools/units/m2cinput.py build/RMHE08/obj/<unit>.o [-f name]... [-o out.s] [--list]
python tools/units/m2cinput.py build/RMHE08/obj/RSO/runtime.o -f fn_804DA7E4 -o build/tmp/x.s
python tools/m2c/m2c.py -t ppc-mwcc-c --no-cache -f fn_804DA7E4 build/tmp/x.s
```
Flags: `--addresses`, `--base`, `--dol`, `--list`, `--no-tables`, `--objdump`, `--section`, `--symbols`.
Exit codes: 0 ok, 1 findings or refusal, 2 could not run (the `lib.findings` convention; today's tool documents none, so `migration.md` records the current behaviour before changing it).
`--json`: the `lib.findings` schema `{tool, rows, ok, summary}` where the tool has `--json`; otherwise none.

## Inputs and outputs

Inputs -> outputs: target .o -> .s.

## Invariants and rules

* `m2c` (the `tools/m2c` submodule) recovers C from GNU-as style assembly, which makes it a second shape oracle next to the Ghidra decompiler - and the only one that works offline. Our target objects are `build/RMHE08/obj/<unit>.o`, and `powerpc-eabi-objdump -dr` prints them in a shape m2c rejects (a `00000000 <LocateObject>:` header per symbol, and an address in front of every instruction), so this translates one into the other:
```
glabel fn_804DA7E4
lwz r6, 12(r3)
bl RSORelocate
beq loc_2c8
...
```
* What it rewrites, and why each one has to happen:
* Relocations become the spelling m2c expects: `R_PPC_REL24` the target symbol, `R_PPC_ADDR16_HA`/`_HI`/ `_LO` `sym@ha`/`sym@h`/`sym@l`, `R_PPC_EMB_SDA21` `sym@sda21(r13)`. m2c asserts on any other spelling (`@hi` crashes it), and an unapplied relocation left as a raw immediate would decompile silently wrong. An sda21 access is encoded with RA=0 and that *is* the sda base register, so it is printed as `(r13) - the same object form mwcc itself emits (checked against `mwcceppc` on a small-data access).
* A branch to an address that is a symbol in this object keeps that name; any other target inside the function becomes a `loc_<address>` label - the `loc_` prefix is what stops m2c treating it as the start of a new function.
* A tail call in the middle of a function (`b <function>`) becomes `bl <function>` + `blr`. m2c only accepts that shape when the branch is the function's *last* instruction (`TailCallPattern` in `m2c/arch_ppc.py`) and otherwise fails the function with "Cannot find branch target". It is m2c's own rewrite, applied early, so the C is the shape it would have produced for a trailing one.
* A symbol m2c cannot spell gets a readable alias: dtk names pooled constants `@1841_80629B90`, and `@` is m2c's relocation separator, so it becomes `_1841_80629B90` - definitions and references alike.
* A symbol whose body is only data (`.long`, `...`, the `gap_*` blobs) is dropped: m2c aborts the whole run with "Function ... contains no instructions" if one of those reaches it.
* The section is always emitted as `.text`. m2c only accepts a label as a function inside `.text`, so a `.init` function (memset, boot code) has to be handed over under that name; `--section` picks what to read out of the object.
* A `bctr` switch is handed its jump table: the table is the nearest symbol loaded before the `bctr`, its bytes are read out of the original DOL (`orig/RMHE08/sys/main.dol`, read-only - the table lives in a different section than the code, often a different split object) and emitted as `.data` with `.long loc_<address>` entries. A table m2c could not recognize by name (an anonymous local like `@1845`, or an SDK name) is written as `jumptable_<address>` so m2c looks at it at all. A table whose entries are not this function's case labels - a relative table, a wrong size, a function pointer read - is refused with a warning instead of guessed at, and then m2c reports it itself. The table's address comes from `config/RMHE08/symbols.txt` (through its own parser), which is also what tells this script where a unit object is linked (`--base` overrides both).
* The Gekko/Broadway paired-single save/restore is put into the shape m2c has a table entry for. The pinned binutils defaults to a later PowerPC core and decodes those opcodes as the VSX/VMX instructions that reused them - `psq_lx`/`psq_stx` as `vmrghb`/`vpku*`, `psq_l`/`psq_st` as `xscmpgedp`/`xxsel`/ `xsmsubasp` - so objdump is asked for the `gekko` core (`DISASM_CPU`) and the indexed frame save/restore MWCC emits with its peephole off is folded to the displacement form m2c handles: `li rX,N` + `psq_lx fD,base,rX,W,I` (or `addi rX,base,N` + `psq_stx fD,r0,rX,W,I`) becomes `psq_l/psq_st fD,N(base),W,I`. Without both, the frame and float saves come out as `M2C_ERROR(unknown instruction: ...)` woven into every return path - the part that decides a match.
* Not every instruction survives the trip: m2c has no `mfcr` or `cmpwi cr1, ...` and prints `M2C_ERROR(...)` inline where it meets one, which is visible in its output. The file written is throwaway - `build/tmp/` is gitignored. Nothing here is codegen evidence: the disassembly is the arbiter (playbook 4).
```
--list       the object's functions with size and instruction count, data-only ones marked `data` and
             the ones that switch through a jump table marked `switch`
--no-tables  do not pull jump tables in (no DOL, no symbol map)
--dol PATH   original DOL to read them out of (default `orig/RMHE08/sys/main.dol`)
--base       address used for `loc_` labels and `--addresses` comments (default: the address in an
             `auto_<nn>_<address>_<section>.o` name, else 0, i.e. object-relative)
--section    section to read (default `.text`; repeatable, or `all`)
--addresses  comment each instruction with its address
-f           keep only these functions (repeatable)
```

## Lib dependencies

binary.objdump, binary.dol, project.symbols.

## Test contract

Tier: fixture (real objdump text).
Today's selftest (`tools/units/m2cinput_selftest.py`): No object, no objdump and no build: it feeds the converter real objdump text (copied from `build/RMHE08/obj/**` output, tabs included) through the pure functions and checks the m2c input it produces. That covers the rewrites that are easy to get subtly wrong - an `@`-named pooled constant, an sda21 access whose base register objdump prints as `0`, a `bdnz-` hint, a mid-function tail call, a data-only `gap_*` blob - without depending on a split object being present. The rows are the contract: when a rewrite changes on purpose, the row changes with it.
Target: `tools/tests/units/test_m2cinput.py` on `lib.testing` (`FixtureTree`/`GitFixture`/`ElfBuilder`); live-tree checks, if any, under `TIER='smoke'` and tolerant.

## Known gaps

None recorded.
