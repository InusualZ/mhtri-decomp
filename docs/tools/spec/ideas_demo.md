# `ideas_demo` - Compile a playbook demo with the real MWCC and test its EXPECT lines

## Purpose

The library behind `ideas.py demo-check`: it owns the EXPECT grammar, the compile of a demo with the unit's base
cflags plus the demo's FLAGS, and the object model the EXPECT lines are judged against. The demo header parser is
`ideas.demo_header` (one parser).

## Users

`tools/agents/ideas.py` (`demo-check`), `tools/agents/ideas_selftest.py`.

## CLI

None (library).

## Lib dependencies

`lib.units` (`override_flags`), `lib.proc` (the spawn retry before the compile).

## Test contract

Tier: legacy (`tools/agents/ideas_selftest.py`): the EXPECT grammar on a fixed objdump text, an unknown verb as a
failed EXPECT, the excerpt, and a real compile when the tree has the compiler and objdump.

## Moved from the module docstring (WP6)

From `tools/agents/ideas_demo.py`:

EXPECT vocabulary (one assertion per line; `<sym>` matches a symbol exactly or as the C++ mangling prefix
`<sym>__...`, so `f` finds `f__Fi`):

    contains <x>                       <x> is an instruction mnemonic, a symbol, or a relocation target
    absent <x>                         the opposite
    size <sym> <bytes>                 the symbol's st_size
    section <name> <bytes>             the section's size (`.text`, `.data`, `.sdata`, ...)
    order <section> <symA> < <symB>    symA is at a lower address than symB inside the section
    reloc <sym>                        some relocation names <sym>
    seq <m1> <m2> ... [in <func>]      the mnemonics occur in this order (a subsequence), in <func> if given
    count <mnemonic> <n> [in <func>]   the mnemonic occurs exactly n times
    insn <mnemonic> <operands> [in <func>]   an instruction with exactly these operands (`insn rlwinm r3,r0,0,24,24`)
    bytes <section> <hex>              the section's contents contain this byte string (hex digits, spaces ignored)
    nobytes <section> <hex>            ... do not contain it

`section <name> 0` also holds when the object has no such section.
