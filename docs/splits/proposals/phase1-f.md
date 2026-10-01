# Phase 1, band `f`: `.text` 0x80460000 .. 0x80540000

Proposal file: `docs/splits/proposals/phase1-f.json` (format and extensions: `docs/splits-program.md`). Tree `15dc2cf7d`.
The band holds 3,574 functions: 15 registered units own 480 of them, **3,094 are unowned** (10 runs). Nearly all of it is
Nintendo/third-party SDK code (MSL, TRK, AI/ARC/AX, Broadcom BTE stack, DB/DSP/DVD/ENC, EXI, GX, IPC, NAND, OS, PAD, SC, SI,
THP/TPL, USB, VI, WPAD/WUD, nw4r, DWC/SSL/NCD/SO, VF, HID/KBD) and **no function of the band has an `extab` record**, so the
C++ extab signal does not exist here.

## Counts

| | |
| --- | --- |
| cuts emitted (one per proposal unit) | 227: **strong 6**, **medium 42**, **guess 179** |
| cuts by class | guess: file 131, roster 43, anchor 6; medium: pool 12, ctors 8, inherited 8, file 11, recut 3, anchor 1; strong: file 4, pool 1, anchor 1 |
| proposal units in the file / rendered (guess cuts merged) | 227 / **48** (candidate 354 units = baseline 306 + 48) |
| unowned functions covered by a proposal unit | 3,094 of 3,094 (plus 19 functions of two registered units' tails); 226 of the 3,113 sit in rendered units that hold no guess candidate and no unresolved interval |
| registered units recut / merged | **2 tails cut off / 0 merged**: `AX/AXFXReverbHi.c` -> `AXFXReverbHiExp` 0x80474F30 (medium), `OS/FindContainHeap_.c` -> `mtxvec`, `mtx44`, `vec` (medium, NonMatching units only); 14 `guess` candidates inside registered units stay top-level `open_questions` (`EXI/ProbeBarnacle.c` 10, `OS/FindContainHeap_.c` 4) |
| registered-unit splitcheck FAILs in the band | 1 (`RSO/runtime.c` pool, a false decode, below); no recut proposed for it |
| unresolved intervals (a cut is required, nothing emitted supplies it) | 58 (35 of <= 40 positions, written into the units' `open_questions`) |

If `guess` cuts of class `roster` (43: a library's roster changes, e.g. BTE -> DB -> DSP -> DVD) were rated `medium`, the same
checker renders 399 units; with every `guess` cut accepted, 535. Both variants have 0 new failures outside the proposal units
and all proposal units PASS, i.e. no guess cut contradicts a shared pool literal, a `.ctors` word or a must-link.

## Grade rubric used (band `a`'s, plus the anchors)

* **strong**: an exact invariant (pooled value held twice with one admissible boundary, a `__sinit` end, an anchor change
  between adjacent functions) **plus** an independent observation for the same cut (a registered edge, the named roster).
* **medium**: one exact invariant, an inherited registered edge, or a roster/file cut inside one narrow (<= 4 position) interval.
* **guess**: a roster/file cut named from symbols and SDK layout only. Never emitted, merged by the renderer.
* Not counted as evidence: tudiscover's adjacent-label `pool` run jumps (precision 0.104), codegen/gap votes (28 % base rate).

## What is actually proven here

* **17 exact `pooldup` cuts** (a pooled `.sdata2` value sits at two addresses and only one boundary can separate the copies'
  readers): 13 in the MSL libm fdlibm run (`0x804642C8` with 15 values, `0x80464560`, `0x80464DEC`, `0x80464F00`, `0x80465714`,
  `0x80465A98`, `0x80465BA8`, `0x80467260`, `0x80467570`, `0x804677A0`, `0x804679E0`, `0x80467BB8`, `0x80467EEC`), `0x80475E30`
  (9 values, right after the registered AXFXReverbHi), `0x805012C4`, `0x80504A3C` (2 values), `0x80533474`.
* **8 `.ctors` cuts** (`0x80502828`, `0x8050661C`, `0x8052B004`, `0x8052C880`, `0x8052E0CC`, `0x80530680`, `0x8053072C`,
  `0x8053E808`): the sinit is the TU's last function because its closure (callees and address-taken functions after it) is empty
  for all eight, and the function at L has no referrer inside the unit (band `a`'s closure rule; `callers.py` in each `reproduce`). The last one moved from
  0x8053E7D0 to 0x8053E808: the seven 8-byte functions between are slots of `lbl_8064DC30`, whose store (`fn_805385D0`) is in the unit
  (`splitcheck.py --proposal phase1-f.json --unit fn_80533474` prints the `detail` line).
* **1 exact anchor change**: `0x804E45B0` (THP banner in `fn_804E4510`, then `TPL.c` in `TPLBind`, adjacent functions).
* **Library identity from `RVL_SDK` banners** (new anchor class, `docs/splits-program.md`): 28 banners anchor AI, AX, DSP, DVD,
  ENC, EXI, GX, KPAD, NAND, OS, PAD, SC, SI, THP, VI, WPAD, DWC, NHTTP, SSL, NCD, NWC24, SO, SOCKET, PMIC, KPR, HID, KBD to the
  function that reads them. Multi-reader banners are must-links (AX `0x8046E3D0..0x8046E430`, GX `0x804B3690..0x804B3BA0` ending
  exactly where my GXFifo candidate starts, NAND, OS `0x804CA710..0x804CAF20`, VI `0x804E7480..0x804E7F60`, WPAD `0x804EC650..0x804EE2A0`).
  They corrected three of my first guesses: ENC is a library of its own inside the "DVDLow" run, `fn_804E4510` is THP not TPL,
  and the SO library starts at `fn_8051E864` (not NWC24).

## Units

Each unowned run is one chain of proposal units; `guess` links (file/roster candidates) make the renderer merge the chain.
Runs: `0x80460D0C..0x80474CB0` (MSL, TRK, AI, ARC, AX, AXFX), `0x80475E30..0x804770E0`, `0x804772F0..0x804AFED0` (BTE stack 183 KB,
DB, DSP, DVD, ENC, ESP, EUART), `0x804B8020..0x804C1760` (GX tail, IPC, KPAD), `0x804C6D70..0x804CBC50` (NAND), `0x804CBC60..0x804D9B4C`
(OS files, ppc_eabi_init, PAD), `0x804DAE40..0x80507C40` (SC, SI, THP/TPL, USB, VI, WPAD, WUD, nw4r, DWC base), `0x80509DB0..0x805113B0`
(DWC middle), `0x8051B7FC..0x8051D710` (SSL, NCD), `0x8051E864..0x8054032C` (SO, VF, DB, PMIC, KPR, HID, KBD, the HOME-button keyboard UI).
Module names follow the SDK library (`RVLGX` instead of `GX` because `src/gx/` exists); `nw4r` from the `Q24nw4r` class prefix;
`homebutton` for the unnamed C++ run because the strings `P_abc`, `P_hiragana`, `P_HENKAN_JP` are HBM keyboard panes.
The last unit ends at `0x8054032C` (the end of the last function that starts inside the band); band `g` owns what follows.

## Top open questions

1. **BTE `0x804772F0..0x804AFED0` is one rendered unit of 895 functions**: 72 file/roster candidates (gki_*, bta_*, btm_*, btu_*,
   l2c_*, rfc_*, sdp_*, then DB/DSP/DVD/ENC/ESP), none provable: Broadcom code has no `__FILE__`, few floats, no `.ctors`.
2. `OS/OSAlarm.c` (registered, **Matching**, 16 B at `0x804CBC50`, symbol `cPhs_Set`) is folded into ONE unit `OS/OSAlarm` `0x804CB440..0x804CBD10`:
   `AlarmQueue` (.sbss 0x80795310, scope:local) is accessed 14 times in 6 functions from `__OSInitAlarm` (0x804CB464) to `__OSCancelInternalAlarms` (0x804CBC80),
   so the earlier head | registered | tail split (a medium cut at 0x804CBC60) was wrong. `cPhs_Set` is re-measured in the merged object (reconcile.md section 4).
3. `EXI/ProbeBarnacle.c` holds EXICommon, fs (ISFS), GXInit, GXFifo, GXAttr, GXMisc, GXGeometry, GXFrameBuf, GXLight and the head of
   GXTexture (`0x804B7F60`): its own header says so; every candidate is `guess` (the narrowest pooldup interval has 9 positions).
4. `OS/FindContainHeap_.c`: mem_expHeap, mem_allocator, mix, mtx stay `guess` (39 positions); the three tail cuts are medium but each
   pair shares an interval (`0x804C68A0`/`0x804C6900`: a pooled `0.0f` at `0x8079D27C` and `0x8079D2A0`, one of the two is a cut).
5. `AX/AXFXReverbHi.c`: the cut is in {`0x80474F20`, `0x80474F30`} (a pooled value is held twice, 2 positions); `0x80474F30` is
   named from `AXFXReverbHiExpInit` and the registered unit's own note that a sister build splits there.
6. `nw4r::db` TU start: one of 7 positions `0x80500710..0x80500900` (`fn_80500900`/`fn_805009AC` share a literal with `Warning`).
7. Unresolved pooled-value intervals with no candidate: THP decoder `0x804DFBE0..0x804DFC50` (2), `0x804E01E0..0x804E0470` (4), WPAD
   `0x804F5770..0x804F84E0` (19), HBM `0x80533814..0x80533B04` (4: guess candidates 0x80533814 or 0x80533854 now, one cut settles three rows).
8. `fn_8051E864` (SO) .. `fn_8051F9D0` (SOCKET banner): the SO/SOCKET file split has 17 positions.

## Moved and removed cuts (a `scope:local` data object read on both sides)

`splitcheck.py --proposal` now reports every proposed cut, a `guess` included, that a local static is read across (guess cuts are merged away, so the candidate's own
`local-static` check never sees them). Eight crossings in five cuts of this band, all fixed: `0x804BB750` (IPC `ipcclt`: `hid`, `__mailboxAck`, `__responses`) -> `0x804BB340`
(`0x804BB310`, `strnlen`, is the other admissible start); `0x804ABC70` (`dvderrorcode`: `Callback`) removed, `dvderror` runs to `0x804ABCE0`; `0x804ABF00` (`FatalFunc`) ->
`0x804ABF40` (`lowCallback`); `0x804D2CD0` (`OSRtc`: `Scb`) -> `0x804D2B90`; `0x804E70C0` (`vi`: `CurrTvMode`) -> `0x804E68B0` (`USB/fn_804E5600` holds a vi function); and the medium `0x804CBC60`
(`AlarmQueue`, above). `AXFXReverbHiExpShutdown` (0x804760C0) lies in the later TU although its name says AXFXReverbHiExp: an open question on both units (rule 7 follow-up).

## Rendered candidate (`splitcheck.py --proposal ... --emit-splits`)

Candidate 354 units (baseline 306), band alone. `order`, `coverage`, `text-cut`, `extab`, `dtors`, `vtable`, `bss`, `local-static`: 0 FAIL, unchanged;
`ctors` 39 -> 39, `pool` 129 -> 136, `data-order` 3, `jumptable` 1. **New failures outside the proposal units: 0**; lint: one line (`strtoul` has no adjacent unit when f is rendered
alone; with band e and the reconcile file it lints clean). Proposal units that FAIL `pool` (8): `MSL/strtoul`, `RVLGX/GXTexture_tail`, `NAND/nand` (the merged guess block that now holds OSAlarm),
`THP/fn_804DF200`, `WPAD/wpad`, `nw4r/fn_80501CE8`, `SO/soi`, `homebutton/fn_80533474`: merged guess blocks that hold one value at two pool addresses (each carries the pooldup rows in its
`open_questions`; `splitcheck.py --baseline --only pool --intervals --unit X` prints them). `RSO/runtime.c` pool is a false decode: `lbl_80795AA0` is read only by `main`/`fn_8003F9E4`
(`callers.py 0x80795AA0`); the decoder counts the `lis/addi` that only forms `.sdata2`'s start address in `RSOStaticLocateObject`.

## Tool gaps hit

* **`tudiscover` must-links from mistyped `.sdata` rows**: 42 spans in the band come from `.sdata` objects the map types `string`
  but the code loads/stores by value (`lbl_80793D28`, `lbl_80793E08` (21 DVDLow readers), `lbl_80794180`...). They forbid real cuts
  (the BTE `uusb`/`bte_main` and DVD `dvdlow` boundaries were refused). Filter used: keep a `.sdata` pool must-link only if some
  reader takes the address (`callers.py` kind `addr`). The map rows need retyping. (A true shared `.sdata2` literal did correct
  one cut: GXTransform starts at `0x804BA230`, not `0x804BA3C0`.)
* **`tudiscover` ignores the 28 `RVL_SDK` banners** as anchors; it reads only `__FILE__` names (12 in this band).
* **`pooldup` intervals can be tightened by address order** (last reader of any private literal at or below copy 1, first reader
  of any at or above copy 2): 1,967 intervals tree-wide tighten; in this band the exact pins stay 17. Scratch only, not a tool.
* **String duplicates are not a TU witness in NW4R-built units**: 125 of 659 consecutive copy pairs lie inside one `__FILE__`
  span (`"ref"`, assert messages), so the `-str reuse` premise holds only for some libraries (DWCi). Not used.
* **The symbol map carries wrong names here** (a name is not layout evidence): `DVDCancel` `0x804751A0` (AX reverb helper),
  `PatInterface_VTable` `0x8046E5C0` (AX), `cPhs_Set` `0x804CBC50` (OSAlarm), `glx_SetLoadCallback` `0x804CBF40`,
  `EmissionControllerFinished` `0x804C9010`, `KPADRead` `0x804C0160` (12 B inside the KPAD run), `Panic/Warning__Q24nw4r2db`
  are fine. Every file-level candidate that rests on a name is therefore `guess`.
* **Renderer**: a merged unit keeps the name of its largest piece (0x804E45B0..0x80500E10, a merge of TPL, USB, VI, WPAD and WUD,
  renders as `WPAD/wpad.cpp`); a guess chain would read better named after its first piece or a library span. A `guess` run start would fold into a
  non-adjacent earlier unit, so every run start here carries an inherited or exact cut (checked by the builder, not by the tool).
