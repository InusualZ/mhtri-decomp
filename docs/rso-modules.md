# RSO modules

The game ships its runtime libraries as **RSO** modules (`orig/RMHE08/files/01/*.rso`, listed by
`orig/RMHE08/files/mh3.sel`). They are REL-like, but the header differs, and decomp-toolkit's module
support is REL-only - so they cannot be split with the stock pipeline yet. This page records what they are,
what is in them, and the options for decompiling them.

Inventory: `python tools/rso/inventory.py --json build/tmp/rso_inventory.json`
(`--rso <file> --sections --symbols` for one module).

## RSO header layout

Verified against the retail files and decomp-toolkit's `src/util/rso.rs`:

| offset | type | field |
| --- | --- | --- |
| 0x00 | u32 | next module link (always 0, filled in at runtime) |
| 0x04 | u32 | previous module link (always 0) |
| 0x08 | u32 | section count |
| 0x0C | u32 | section info offset (always 0x58) |
| 0x10 | u32 | module name offset - the **original build path**, e.g. `D:\MH3_EUR\MH3Wii_e\project\map00.plf` |
| 0x14 | u32 | module name size |
| 0x18 | u32 | version (always 1) |
| 0x1C | u32 | bss size (allocated at runtime, not in the file) |
| 0x20 | u8 x3 | prolog / epilog / unresolved section index (+1 runtime bss index) |
| 0x24 | u32 x3 | prolog / epilog / unresolved section-relative offset |
| 0x30 | u32 x2 | internal relocation table offset / size |
| 0x38 | u32 x2 | external relocation table offset / size |
| 0x40 | u32 x2 | export symbol table offset / size |
| 0x48 | u32 | export name table offset |
| 0x4C | u32 x2 | import symbol table offset / size |
| 0x54 | u32 | import name table offset |

Section info is `section count` entries of `(u32 offset_and_flags, u32 size)`; `offset = value & ~1` and
bit 0 would mark an executable section (**retail RSOs do not set it** - the code section is the one holding
the prolog). Export symbols are 16 bytes (`name_offset, offset, section_index, name_hash`), imports 12 bytes
(`name_offset, offset, reloc_link`).

Note the relationship to a REL header: an RSO header is a REL header with the leading `module_id` u32
removed, so a REL reader sees the section count where it expects `prev` and fails with
`Expected prev == 0`.

## What is in them

`python tools/rso/inventory.py` (21 modules, 3.7 MB total):

| module | size | sections | code | bss | exports / imports |
| --- | --- | --- | --- | --- | --- |
| em_data.rso | 1 121 440 | 18 | 325 200 | 129 304 | 736 / 69 |
| lobby_data.rso | 666 496 | 19 | 100 644 | 77 384 | 988 / 49 |
| quest_data.rso | 659 360 | 19 | 211 560 | 43 264 | 271 / 304 |
| com_data.rso | 380 416 | 19 | 4 420 | 6 748 | 517 / 55 |
| demo_data.rso | 352 800 | 19 | 92 068 | 3 140 | 327 / 338 |
| hbm_data.rso | 310 304 | 18 | 175 144 | 15 144 | 867 / 210 |
| net_data.rso | 100 032 | 17 | 19 380 | 37 932 | 452 / 100 |
| map01..map11, map00, map_town, map_village | 3 712 - 20 768 | 17 | 388 - 1 200 | 2 056 - 3 176 | 15-16 / 3 |

Every module embeds its original build name (`map00.plf`, `em_data.plf`, ...), so the module names are
known. The export tables carry **real symbol names**, C++ mangled where applicable, e.g. for `map00`:

```
__register_global_object, __register_atexit__FPFv_v, __destroy_global_chain,
__global_destructor_chain, _prolog, _epilog, _unresolved, _ctors, _dtors, map00_data,
@LOCAL@GetAnmPlayPolicy__Q24nw4r3g3dFQ34nw4r3g3d9AnmPolicy@policyTable
imports: PlayPolicy_Loop__Q24nw4r3g3dFfff, PlayPolicy_Onetime__Q24nw4r3g3dFfff, get_stg_w__Fv
```

So the modules are C++ (nw4r-based) runtime code plus a lot of data, and each one has a prolog/epilog/
unresolved triple in its code section. The `_prolog`/`_epilog`/`_unresolved` and `__register_*` symbols
mean the module runtime (`Runtime.PPCEABI.H`-style stubs) is part of every module.

## Symbol map

`python tools/rso/symbols.py --all --out config/RMHE08 --report build/tmp/rso-symbols/summary.md`
writes one `config/RMHE08/<module>/symbols.txt` per module (the template's per-module location) and a
report with the section table and the import resolution.

What the export table can and cannot give:

* **exact**: symbol names, section index, section-relative offset, and the export name hash;
* **not available**: sizes, local (non-exported) symbols, and the section *names* - the RSO section table
  is unnamed, so the names must be inferred and then declared identically in the module's `splits.txt`.

Section naming is therefore recorded per section in the generated file's header comment:

| how | rule |
| --- | --- |
| proven | `.init` = the section holding the prolog entry point; `.ctors` / `.dtors` = the section holding the `_ctors` / `_dtors` label; `.bss` = the section with no file bytes whose size equals the header's bss size |
| inferred | data-looking exports (`*_data`, `table`, `@LOCAL@`) get `.data` / `.rodata`; the remaining sections take names from the conventional REL pool (`extab`, `extabindex`, `.sdata`, ...) in index order |

Two details worth knowing:

* Duplicate export names exist (e.g. `map00` exports `__destroy_global_chain_reference` twice, at
  `.dtors:0x0` and `.dtors:0x4` - two destructor-list slots). The second occurrence gets a `$N` suffix,
  matching the linker's own convention (`_ctors$99`), and the generated file lists them in a comment.
* Labels inside `.ctors` / `.dtors` are written as `type:label`, everything else as `type:function` or
  `type:object` from a name heuristic - the analyzer will correct types when the module can be split.

**Import resolution is complete**: every import of every module resolves against a name in the DOL's
`symbols.txt`, and **no module imports from another module** - the RSOs are all clients of the DOL:

| module | imports | resolved to |
| --- | --- | --- |
| em_data | 69 | DOL 69 |
| lobby_data | 49 | DOL 49 |
| quest_data | 304 | DOL 304 |
| com_data | 55 | DOL 55 |
| demo_data | 338 | DOL 338 |
| hbm_data | 210 | DOL 210 |
| net_data | 100 | DOL 100 |
| map00..map11, map_town, map_village | 3 each | DOL 3 each |

That the mangled nw4r names in the RSO import tables match the DOL's `symbols.txt` exactly is also a good
sanity check on our symbol naming.

## Blocker: dtk cannot split RSOs

Adding a module to `config/RMHE08/config.yml` and running the split fails:

```
$ ninja build/RMHE08/config.json
build\tools\dtk.exe dol split config\RMHE08\config.yml build\RMHE08
 INFO Loading and analyzing 2 modules (using 2 threads)
 INFO module{name=main}: Loading orig\RMHE08\files\mh3.sel
Failed: While loading object 'map00.rso'
    0: Failed to read REL header
    1: Expected prev == 0
```

decomp-toolkit 1.8.3 (and 1.8.4 / `main`) parses RSOs only in `dtk rso info|make`; the splitter never
touches RSOs (`src/util/split.rs` and `src/cmd/split.rs` have no RSO references, while
`src/util/rso.rs::process_rso` already returns an `ObjInfo`). The template's `modules:` config is
documented as REL-only.

Options, cheapest first:

1. **Wire RSOs into dtk's module loader upstream.** `process_rso` already produces `ObjInfo`, so the work
   is dispatching by file type in the module loader plus section/symbol naming. Needs a Rust build of dtk
   (or a release that includes it) and a version bump in `configure.py`.
2. **RSO -> REL converter as a local stopgap** (`tools/rso/to_rel.py`): prepend the 4-byte `module_id` and
   rewrite the header fields into REL layout, shifting absolute offsets by 4 and mapping the RSO
   internal/external relocation tables onto the REL relocation table. Then the stock pipeline (split,
   link, `dtk rso make`, hash) works, but the converter must be exact and its reloc mapping verified.
3. **Hand-rolled splitter** using the parsed header + devkitPPC tools. Most work, and it duplicates what
   dtk already does for RELs.

Until one of these lands, RSO matching is blocked at the *split* step: the inventory above is available
(`tools/rso/inventory.py`), but there are no per-unit target objects to diff against.
