---
id: 59
title: A flipped unit's `extab`/`extabindex` entries must carry the map's names, and a global binding
status: works
problem: A C++ unit whose object is byte-identical to its target - every section compared, `flipcheck.py` saying READY, `extab` and `extabindex` included - cannot link once it is `Matching`:
tags: [linker, symbols]
applies: []
demo:
---

# 59. A flipped unit's `extab`/`extabindex` entries must carry the map's names, and a global binding

**Problem.** A C++ unit whose object is byte-identical to its target - every section compared, `flipcheck.py`
saying READY, `extab` and `extabindex` included - cannot link once it is `Matching`:

```
mwldeppc.exe Linker Error:
  undefined: '@eti_800222FC'
  Referenced from 'lbl_8057CED0' in auto_07_8057C820_data.o
```

Nothing in the source moves it. `mwcceppc.exe -help` has no option that names or exports an extab symbol, the
identifier is not expressible, and the unit's own exception settings are already right (our object and the
target agree byte-for-byte: `extab` `0x138`, `extabindex` `0x1D4`, `.relaextabindex` 936 B).

**Why try it.** `dtk dol split` names the exception tables it synthesises after the map address each entry
occupies - `@etb_<VA>` for an `extab` entry, `@eti_<VA>` for an `extabindex` entry, uppercase hex - and
defines those names in the **target** object, which is exactly the definition a `NonMatching` unit provides to
the link. MWCC emits the same bytes under its own anonymous ordinals (`@905`) with a **local** `st_info`, so on
the flip the name's only definition disappears. The reference itself can be an artefact of dtk's DOL
relocation guess: the word at `0x8057EA88` is `0x80022300`, which dtk accepted as `@eti_800222FC+4` because it
*lands inside* the 12-byte extabindex entry - the neighbouring words are packed bit fields, so the entry is not
read by anything. It does not matter: the linker wants the name.

**Result.** Rename the entry symbols to the map's spelling **and** make them global - in a post-compile step
next to `objalign`, not in source. `tools/elf/objextab.py` computes each entry's address as *the section's
claimed start in `splits.txt` + `st_value`* and rewrites `st_name` and `st_info` (`0x10 | binding`) in
`.symtab`, appending the new names to `.strtab` - no section's contents change, so the step cannot move the
DOL. (The symbol's index is not moved, so it stays inside `.symtab`'s local range: repairing `sh_info` would
mean renumbering the whole symbol table and every relocation's symbol index, i.e. rewriting section contents.
`mwld` resolves from `st_info` - 20 of the already-`Matching` link inputs carry 94 renamed symbols and the DOL
is unchanged.) Measured: over the whole build it touches **215 of 254** objects, renaming **7284** symbols, and across
**2423** section rows exactly **430** differ, all of them `.symtab`/`.strtab` and **none** outside them. Both halves
are needed: a rename alone still fails, and so does a rename plus the target's `.comment` `active_flags = 0x08`.
**18 of the 254 registered units** own a symbol another *linked* object references, so each of them would have
been unlinkable on flip (scanning `build/RMHE08/obj/` instead of the link inputs overstates the class by 44 %:
26 vs 18).

**Example.** `g3d/g3d_resfile`: our object has the entry as `@905` at `extabindex` +0x1A4, `st_info` `0x01`;
the claim starts at `0x80022158`, so `0x80022158 + 0x1A4 = 0x800222FC` and the name is `@eti_800222FC` -
exactly the target's spelling and binding (`0x11`, global). The flip then links green and `main.dol` hashes
`BF4850739478CAAEDFE675949EB7C28595A7FDE9` = `config/RMHE08/build.sha1`. The rule is not a guess: it
reproduces **17490** map-name symbols over the 510 target/our objects of the registered units, **0
mismatches**, case included. Like row 58, this is a compiler-synthesised name the map owns; unlike a pool
entry, a post-compile step can fix it because the entry always sits in a section the unit claims.
