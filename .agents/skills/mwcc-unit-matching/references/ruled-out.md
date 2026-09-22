<!-- GENERATED FILE - do not edit.
     Source: docs/matching.md
     Regenerate: python .agents/skills/mwcc-unit-matching/scripts/sync_reference.py
-->

## Ruled out - do not re-run these

* **Compiler version.** Compile the matrix once for the unit; if it is flat, version is not the lever.
* **The whole `-opt` axis.** Sweep the compiler's own keyword list once. Beyond the one or two keywords
  that matter, the rest are no-ops or make the code worse; the frame-size traps are in trick 10.
* **`-Cpp_exceptions`.** It only adds `extab`/`extabindex` sections (needed at the end for a full match) and
  does not change `.text` here.
* **`-O` level and `-schedule`/`-fp_contract`/`-ipa`.** Worth exactly one sweep each; if the level is wrong
  the symptom is unmistakable (sizes off by hundreds of bytes and fused/hoisted code everywhere).
* **Disassembler aliases as fingerprints.** `extrwi`, `clrlslwi` and friends are *aliases* GNU objdump never
  prints (it prints the underlying `rlwinm`), so counting them on either side proves nothing - see idea 21.
  Fingerprint what the compiler emits (record forms, save-helper calls, `lis` sharing), not what the
  disassembler happens to name.
* **`-sdata 0` for a game unit.** Measured on `Pl/pl_skill.cpp` (batch 6): it turns the target's absolute
  `lis`/`addi` to `lobby_w` into small-data accesses and breaks the two functions that *should* be small-data,
  so on the pre-fix source it was +0.085 on two functions and -0.11 on two others (net -0.03), and with the
  real fix landed it is purely harmful (-0.12 pt). The absolute access a target shows is a property of *that
  symbol's section*, not of the unit's addressing mode: declaring `lobby_w` as an unsized array (`lobby_w[0]`)
  gives the absolute form while `lbl_80792140/48` stay `@sda21`, which is what the target does.
