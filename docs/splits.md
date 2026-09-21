# `splits.txt`

This file contains file splits for a module.

Example:

```yaml
Sections:
	.text       type:code align:32
	.ctors      type:rodata align:32
	.data       type:data align:32
	.bss        type:bss align:32

path/to/file.cpp:
	.text       start:0x80047E5C end:0x8004875C
	.ctors      start:0x803A54C4 end:0x803A54C8
	.data       start:0x803B1B40 end:0x803B1B60
	.bss        start:0x803DF828 end:0x803DFA8C
	.bss        start:0x8040D4AC end:0x8040D4D8 common
```

## Header

```yaml
Sections:
    section     [section attributes]
```

### Attributes

- `type:` The section type. `code`, `data`, `rodata` or `bss`.
- `align:` The section alignment in bytes.
- `vaddr:` (REL only) The fixed virtual address of the section. When set, split and symbol addresses are written as absolute addresses.

## Files

```yaml
path/to/file.cpp: [file attributes]
    section     [section attributes]
    ...
```

- `path/to/file.cpp` The name of the source file, usually relative to `src`. The file does **not** need to exist to start.  
  This corresponds to an entry in `configure.py` for specifying compiler flags and other options.

### File attributes

- `comment:` Overrides the `mw_comment_version` setting in [`config.yml`](/config/GAMEID/config.example.yml) for this file. See [Comment section](comment_section.md).
  - `comment:0` is used to disable `.comment` section generation for a file that wasn't compiled with `mwcc`.  
  Example: `TRK_MINNOW_DOLPHIN/ppc/Export/targsupp.s: comment:0`  
  This file was assembled and only contains label symbols. Generating a `.comment` section for it will crash `mwld`.

- `order:` Allows influencing the resolved link order of objects. This is **not required**, as decomp-toolkit will generate the link order automatically. This can be used to fine-tune the link order for ambiguous cases.  
  Example:
  ```
  file1.cpp: order:0
    ...

  file2.cpp: order:1
    ...

  file3.cpp: order:2
    ...
  ```
  This ensures that `file2.cpp` is always anchored in between 1 and 3 when resolving the final link order.

### Section attributes

- `start:` The start address of the section within the file. For DOLs, this is the absolute address (e.g. `0x80001234`). For RELs, this is the section-relative address (e.g. `0x1234`).
- `end:` The end address of the section within the file.
- `align:` Specifies the alignment of the section. If not specified, the default alignment for the section is used.
- `rename:` Writes this section under a different name when generating the split object. Used for `.ctors$10`, etc.
- `common` Only valid for `.bss`. See [Common BSS](common_bss.md).
- `skip` Skips this data when writing the object file. Used for ignoring data that's linker-generated.

## Editing `splits.txt` re-splits the whole DOL

Any change to a split range makes the next `ninja` re-run `dtk`'s split over the whole DOL: 6-12 minutes on
this machine and ~14 GB of split objects rewritten, because the split is one step over the file. So **batch**
registrations - every new unit's source and ranges first, then one split, one link and one report. The same
cost is why a range is never adjusted "just to see": propose it, then measure the unit before and after it
(`ninja changes`), because a claimed range has already been measured to make a match *worse* (see
`docs/matching.md` 23).

Registering a range also re-attributes bytes: that region moves out of the per-function `auto_*` scaffolding
objects into the new unit, so the DOL-wide `total_code` and `fuzzy_match` totals move for bookkeeping reasons,
not because more code matched. Judge an attempt by the unit's own per-symbol numbers and by `ninja changes`
reporting no per-unit regression - never by the totals.

One false alarm to expect: when a range takes the last of an `auto_*` blob's bytes, that blob disappears from
the report and `ninja changes` prints it as `<auto_* unit> | matched_data | 100.00% -> 0.00%`. That is a
*vanished* unit, not a regression - its region became a real unit. Check the totals (`total_data` unchanged,
`matched_code` up by exactly the new units' bytes) and the new unit's `fuzzy_match_percent` before calling it
anything.
