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
