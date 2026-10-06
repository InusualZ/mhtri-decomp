# The g3d units

The `src/g3d/` units (the nw4r g3d library band). Each unit's evidence is its file header; this page holds the
measurements behind the lib's flags, which `configure.py` points at.

## Flags

`cflags_g3d` is `cflags_base` without `-O4,p` and `-inline auto`, plus `-O3`, `-inline noauto` and
`-Cpp_exceptions on`.

* **Compiler: Wii/1.3.** A `mt.py matrix` over 7 units (g3d_calcvtx, g3d_resanmchr, g3d_resanm, g3d_resanmamblight,
  g3d_resanmcamera, g3d_resmat, g3d_cpu) against GC/3.0a3, 3.0a5, 3.0a5.2, Wii/1.0 and Wii/1.3, with and without
  `-fp_contract off`: Wii/1.3 is best or tied on every unit (g3d_resmat 92.91 vs 91.16 on GC/3.0a5.2), unlike the
  nw4r math/ut/db stubs, which match under GC/3.0a5.2.

* **`-O3`** (playbook 27). `g3d/g3d_anmscn.cpp`'s `fn_800680A8__FPv` (0x24 B, 9 instructions) loads the field at
  +0xC before the epilogue's LR reload. Under `-O4,p` the same nine instructions come in the other order
  (`lwz r0, 0x14(r1)` before `lwz r3, 0xc(r3)`); under `-O3` the object is byte-identical. Every `-O3` variant
  tried (`-inline auto`, `-inline noauto`, `-opt nopeephole`) keeps the retail order, so the level is the lever.
* **`-inline noauto`**. Measured with a scratch compile of the whole lib (report metric plus a raw per-section byte
  compare): two units reconstructed under `-inline noauto` reproduce their scores under it (83.20733 / 94.86212)
  and drop under `-inline auto` (70.17290 / 93.59429), while `g3d/g3d_anmscn.cpp`'s object is identical under
  both. Under `-inline noauto` the `*_ac.h` inline constructors are called out of line (`fn_800900A4`,
  `fn_80062D58`, ...), so the units declare and call those copies by their map stems.
* **`-Cpp_exceptions on`**. The retail objects carry `extab`/`extabindex`; `g3d/g3d_anmscn.cpp`'s are extab 0x8 +
  extabindex 0xC, unwind-only records (a 4-byte flag word and a zero terminator, no PC ranges, no actions), and
  the flag alone reproduces them - the same finding as `cflags_pl` and `Gecko_ExceptionPPC.cp`.
