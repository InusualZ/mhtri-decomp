# The ef units

The game-side effect units under `src/ef/` (the `eft0NN` families, the effect manager and resource layer, the
system core). The `nw4r::ef` library units (`ef_*.cpp`) keep their notes in their own headers.

## The case-0 nested dispatch

The written `_EFT` state dispatchers that carry this residual - `ef/fn_80119C44.c`'s `fn_80119D9C`,
`ef/eft002.cpp`'s `fn_800FC428`, `ef/fn_8030681C.cpp`'s `fn_80306FF0`, `ef/eft013_fx.cpp`'s `fn_8010AD00`,
`ef/eft022_fx.cpp`'s `fn_80116FDC` and `ef/eft035.cpp`'s `fn_802F2640` - switch on `state_0x05`, and case 0 runs a
nested switch on the type. Retail lays the blocks out as
`[compare chain][default blr][case0: b N][case1][case2][case3][exit blr][N: nested dispatch]`: case 0's block is a
bare jump and the nested switch sits after the exit block, sharing its `break`.

MWCC puts the nested blocks in case 0's slot for every spelling tried in `fn_80119D9C`:
the nested switch as a case-0 body with `break`, `return` or neither; the cases in either source order; an explicit
`default` before or after; braces; an explicit trailing `return`; a table lookup or an `s8` operand;
`#pragma peephole off`. `fn_80119D9C` keeps the `case 0: break;` plus after-switch form, which gives the target's
size and instruction order with a trailing dead `blr`. Writing case 0 last (`fn_80306FF0`, `fn_800FC428`) gives a
compare chain 1, 2, 3, 0 where retail compares 0, 1, 2, 3.

## Handler stores through r0

A spawner installs its hooks with `lis r3, sym@ha; addi r0, r3, sym@l; stw r0, off(rN)` in retail. In
`ef/fn_80119C44.c`'s `fn_80119C44` (C, `cflags_main`) MWCC keeps `r3` for the `addi`; spellings tried: a typed
local, an explicit cast, `void*` fields, a comma expression, the two stores swapped, the stores before the
`eft_state_flags_set` call, a block-scoped declaration, `#pragma peephole off`. The same assignments in
`ef/eft002.cpp`'s `eft002_set` (C++) compile to `r0`.

## Switch-table owners

`jumptable_805E918C` (`_8034239cswitchdataD_805e918c` in the dump) is `ef/eft_slot.cpp`'s `enemy_data_grp`'s, not
the prefix's: playbook 54's 2026-10-06 correction.

## The peephole pass and the u8 argument masks

Playbook 39: the ef rows built with the peephole pass on lose retail's `clrlwi` argument masks and fuse its
`subi`/`rlwinm` + `cmpwi` compares; the partial rows outside a `#pragma peephole off` scope list this as their
residual.
