#!/usr/bin/env python3

###
# Generates build files for the project.
# This file also includes the project configuration,
# such as compiler flags and the object matching status.
#
# Usage:
#   python3 configure.py
#   ninja
#
# Append --help to see available options.
###

import argparse
import os
import re
import sys
from pathlib import Path
from typing import Any, Dict, List

from tools.project import (
    Object,
    ProgressCategory,
    ProjectConfig,
    calculate_progress,
    generate_build,
    is_windows,
)

# Game versions
DEFAULT_VERSION = 0
VERSIONS = [
    "RMHE08",  # 0
]

parser = argparse.ArgumentParser()
parser.add_argument(
    "mode",
    choices=["configure", "progress"],
    default="configure",
    help="script mode (default: configure)",
    nargs="?",
)
parser.add_argument(
    "-v",
    "--version",
    choices=VERSIONS,
    type=str.upper,
    default=VERSIONS[DEFAULT_VERSION],
    help="version to build",
)
parser.add_argument(
    "--build-dir",
    metavar="DIR",
    type=Path,
    default=Path("build"),
    help="base build directory (default: build)",
)
parser.add_argument(
    "--binutils",
    metavar="BINARY",
    type=Path,
    help="path to binutils (optional)",
)
parser.add_argument(
    "--compilers",
    metavar="DIR",
    type=Path,
    help="path to compilers (optional)",
)
parser.add_argument(
    "--map",
    action="store_true",
    help="generate map file(s)",
)
parser.add_argument(
    "--debug",
    action="store_true",
    help="build with debug info (non-matching)",
)
if not is_windows():
    parser.add_argument(
        "--wrapper",
        metavar="BINARY",
        type=Path,
        help="path to wibo or wine (optional)",
    )
parser.add_argument(
    "--dtk",
    metavar="BINARY | DIR",
    type=Path,
    help="path to decomp-toolkit binary or source (optional)",
)
parser.add_argument(
    "--objdiff",
    metavar="BINARY | DIR",
    type=Path,
    help="path to objdiff-cli binary or source (optional)",
)
parser.add_argument(
    "--sjiswrap",
    metavar="EXE",
    type=Path,
    help="path to sjiswrap.exe (optional)",
)
parser.add_argument(
    "--ninja",
    metavar="BINARY",
    type=Path,
    help="path to ninja binary (optional)",
)
parser.add_argument(
    "--verbose",
    action="store_true",
    help="print verbose output",
)
parser.add_argument(
    "--non-matching",
    dest="non_matching",
    action="store_true",
    help="builds equivalent (but non-matching) or modded objects",
)
parser.add_argument(
    "--warn",
    dest="warn",
    type=str,
    choices=["all", "off", "error"],
    help="how to handle warnings",
)
parser.add_argument(
    "--no-progress",
    dest="progress",
    action="store_false",
    help="disable progress calculation",
)
args = parser.parse_args()

config = ProjectConfig()
config.version = str(args.version)
version_num = VERSIONS.index(config.version)

# Apply arguments
config.build_dir = args.build_dir
config.dtk_path = args.dtk
config.objdiff_path = args.objdiff
config.binutils_path = args.binutils
config.compilers_path = args.compilers
config.generate_map = args.map
config.non_matching = args.non_matching
config.sjiswrap_path = args.sjiswrap
config.ninja_path = args.ninja
config.progress = args.progress
if not is_windows():
    config.wrapper = args.wrapper
# Don't build asm unless we're --non-matching
if not config.non_matching:
    config.asm_dir = None

# Tool versions
config.binutils_tag = "2.42-2"
config.compilers_tag = "20251118"
config.dtk_tag = "v1.8.3"
config.objdiff_tag = "v3.6.1"
config.sjiswrap_tag = "v1.2.2"
config.wibo_tag = "1.0.3"

# Project
config.config_path = Path("config") / config.version / "config.yml"
config.check_sha_path = Path("config") / config.version / "build.sha1"
config.asflags = [
    "-mgekko",
    "--strip-local-absolute",
    "-I src",
    f"-I build/{config.version}/include",
    f"--defsym BUILD_VERSION={version_num}",
]
config.ldflags = [
    "-fp hardware",
    "-nodefaults",
]
if args.debug:
    config.ldflags.append("-g")  # Or -gdwarf-2 for Wii linkers
if args.map:
    config.ldflags.append("-mapunused")
    # config.ldflags.append("-listclosure") # For Wii linkers

# Use for any additional files that should cause a re-configure when modified
config.reconfig_deps = []

# Optional numeric ID for decomp.me preset
# Can be overridden in libraries or objects
config.scratch_preset_id = None

# Base flags, common to most GC/Wii games.
# Generally leave untouched, with overrides added below.
cflags_base = [
    "-nodefaults",
    "-proc gekko",
    "-align powerpc",
    "-enum int",
    "-fp hardware",
    "-Cpp_exceptions off",
    "-O4,p",
    "-inline auto",
    '-pragma "cats off"',
    '-pragma "warn_notinlined off"',
    "-maxerrors 1",
    "-nosyspath",
    "-RTTI off",
    "-fp_contract on",
    "-str reuse",
    "-multibyte",  # For Wii compilers, replace with `-enc SJIS`
    "-i src",
    f"-i build/{config.version}/include",
    f"-DBUILD_VERSION={version_num}",
    f"-DVERSION_{config.version}",
]

# Debug flags
if args.debug:
    # Or -sym dwarf-2 for Wii compilers
    cflags_base.extend(["-sym on", "-DDEBUG=1"])
else:
    cflags_base.append("-DNDEBUG=1")

# Warning flags
if args.warn == "all":
    cflags_base.append("-W all")
elif args.warn == "off":
    cflags_base.append("-W off")
elif args.warn == "error":
    cflags_base.append("-W error")

# Metrowerks library flags
cflags_runtime = [
    *cflags_base,
    "-use_lmw_stmw on",
    "-str reuse,pool,readonly",
    "-gccinc",
    "-common off",
    "-inline auto",
]
# Runtime.PPCEABI.H flags: cflags_runtime + -func_align 4. Evidence: -O4,p implies -func_align 16 and all
# five target objects of this lib are `.init align 2**2`; with 16 the compiler pads between functions
# (global_destructor_chain .text 0x20 vs 0x18, __init_cpp_exceptions 0x74 vs 0x70, Gecko's two loop
# preheaders gain a nop - ExPPC_FindExceptionRecord 99.07 -> 100.00 with the flag). __start.c's 16-byte
# function starts are unaffected: they come from each function's own `#pragma section code_type ".init"`
# section, not from the function alignment.
cflags_ppceabi = [*cflags_runtime, "-func_align", "4"]

# The two bootstrap files want 16-byte function alignment (their .init regions carry the retail zero padding:
# __start 0x300 vs our 0x2E0, __ppc_eabi_init 0x64 vs our 0x58), while the rest of this lib packs on 4.
cflags_ppceabi16 = [*cflags_runtime, "-func_align", "16"]


# REL flags
cflags_rel = [
    *cflags_base,
    "-sdata 0",
    "-sdata2 0",
]

# cflags_g3d: -O3, -inline noauto and -Cpp_exceptions on over cflags_base; measurements in docs/g3d.md.
cflags_g3d = [
    *[f for f in cflags_base if f not in ("-O4,p", "-inline auto")],
    "-O3",
    "-inline noauto",
    "-Cpp_exceptions on",
]

# cflags_network and cflags_os: -func_align 4 (+ -Cpp_exceptions on for Network); measurements in docs/network.md.
cflags_network = [*cflags_base, "-func_align", "4", "-Cpp_exceptions", "on"]
cflags_os = [*cflags_base, "-func_align", "4"]
# DWCi (the Wii Wi-Fi Connection SDK library the retail link places between the game's own SDK uses
# and NHTTP).  Same shape as the sibling SDK groups above: the retail .text packs the band's
# functions back to back with 4-byte gaps, so `-func_align 4`.  Evidence: DWCi/fn_805113B0.c.
cflags_dwc = [*cflags_base, "-func_align", "4"]
# NWC24 (the WiiConnect24 library: `NWC24IsMsgLibOpened` .. `NWC24iPrepareShutdown`,
# src/NWC24/fn_8051D710.c + src/NWC24/fn_8051E068.c).  Same shape as the sibling SDK groups above -
# cflags_base + `-func_align 4`, copied, not changed: the band's retail .text packs its functions on
# 4-byte boundaries (e.g. NWC24IsMsgLibOpened 0x8051D8B0 -> ...ByTool 0x8051D8C4 -> fn_8051D8D8
# 0x8051D8D8 -> NWC24BlockOpenMsgLib 0x8051D8EC, and the 0x10-byte command thunks at 0x8051DEA8,
# 0x8051DEB8, 0x8051DEC8 are not 16-byte aligned), while cflags_base's `-O4,p` implies -func_align 16.
# The level itself is unprobed for this band - no unit here is flipped - so a body pass should sweep
# -O3/-O4,p per unit (playbook 33) before the flags are called settled.
cflags_nwc24 = [*cflags_base, "-func_align", "4"]

# NHTTP (the Revolution SDK HTTP library the retail link places immediately after the DWCi band,
# 0x805145B8..0x8051B7FC).  Same shape as the sibling SDK groups above: `Wii/1.3` and
# `-func_align 4`, copied from `cflags_dwc`/`cflags_os`.  The change from 16-byte to 4-byte
# function packing at 0x805145B8 is the DWCi/NHTTP boundary (see NHTTP/NHTTP_bgnend.c).
cflags_nhttp = [*cflags_base, "-func_align", "4"]

# lobby flags (src/lobby/lobby_scene.c). Evidence: the retail fn_801EC9E0 (0x18 B / 6 instructions) reads the
# small-data scene pointer, then the +0x10 table base, before the argument's byte, and keeps the table base
# in r4; -O4,p instead hoists the byte load, splits the base across r3 and reorders the two loads. -O3 is
# byte-identical. Note the OS unit next to this list wants -O4,p: the level is per unit, so probe both.
cflags_lobby = [
    *[f for f in cflags_base if f != "-O4,p"],
    "-O3",
    # Evidence (lib-wide before/after, 2026-09-25): `-inline noauto` over the whole lobby lib. Of the 13
    # registered units, 5 improve (fn_801E7530 27.64778 -> 31.76088, fn_801F9CD4 20.33288 -> 22.07398,
    # fn_8021E1EC 9.21654 -> 9.66140, lb_npc 13.36667 -> 13.74540, fn_80219260 11.00314 -> 11.08171), 8 are
    # unchanged (including every unit already at its ceiling), and NONE regresses - verified at per-function
    # granularity, not just per unit, and the already-Matching lobby_scene.c stays byte-identical at 100.0.
    # The same knob is what closed four main.cpp functions (cflags_main derives from this list); cflags_lobby
    # keeps `-O3` for the same reason main does.
    "-inline noauto",
    # `-Cpp_exceptions on` (flags-audit 2026-09-28): 19 of the lib's 20 registered targets carry
    # extab/extabindex (lobby_scene.c, a C file, is the one that does not - and the flag leaves its
    # object byte-identical, measured), and 8 of them spelled it out as a file-wide
    # `#pragma exceptions on`. With the lib flag on and all 8 pragmas deleted: all 20 objects'
    # allocatable sections byte-identical (lobby_scene.c included), every unit score identical,
    # main.dol still BF4850739478CAAEDFE675949EB7C28595A7FDE9. The 11 lobby targets that carry extab
    # but had no pragma now emit it (extab 0 -> 56..216 B of their 480..752 B targets), which is the
    # first time those units can score their extab at all.
    "-Cpp_exceptions",
    "on",
]

# main flags (src/main.cpp). Evidence in the lib entry below: -O3, and the inline knob has to move off `auto`,
# because the retail main.cpp keeps its tiny file-local calls (fn_8003F554 out of fn_8003F52C/fn_8003F564) and
# does not inline them. `-inline noauto` rather than `-inline off`: fn_8003F940 is the retail aggregate
# GXRenderModeObj copy, and `off` makes MWCC emit a call to the implicit copy-assignment operator (99.02 %)
# where `noauto` inlines it (100.00 %) - nothing else in the unit moves between the two.

# cflags_pl (src/Pl/*.cpp): -O3 -inline noauto -opt nopeephole -Cpp_exceptions on; measurements in docs/pl.md.
cflags_pl = [
    *[f for f in cflags_base if f not in ("-O4,p", "-inline auto", "-Cpp_exceptions off")],
    "-O3",
    "-inline noauto",
    "-opt nopeephole",
    "-Cpp_exceptions on",
]

# cflags_pl_skill: cflags_pl at `-opt nopeephole,level=4`; no object uses it (src/Pl/pl_act.cpp states the level
# as a pragma); measurements in docs/pl.md.
cflags_pl_skill = [
    *[f for f in cflags_pl if f != "-opt nopeephole"],
    "-opt nopeephole,level=4",
]
cflags_main = [
    *[f for f in cflags_lobby if f != "-inline noauto"],
    "-inline noauto",
    # Evidence: the retail main.o carries extab 0x90 + extabindex 0xD8 (18 unwind-only records, one per
    # function with a frame) and our object emitted none, while every function's .text is unaffected by the
    # flag - the same finding as Pl, g3d and camellia. sys_mem.cpp in this lib spelled the same thing
    # out with a per-file `#pragma exceptions on` before this group existed; flags-audit 2026-09-28
    # measured that pragma (and fn_80040598.cpp's) byte-identical without it once the lib carries the
    # flag, and removed both (analysis: .pi/notes/extab-gap.md).
    "-Cpp_exceptions on",
]

# hud flags (src/hud/*). cflags_main plus `-opt nopeephole`: the whole lib needs the unfused forms, each
# file saying so with a file-wide `#pragma peephole off` before this group existed (2026-09-27):
#   * hud/fn_80324F7C.c - `draw_shape` keeps `clrlwi`+`cmpwi` separate where the fold fuses them (the
#     object is `Matching`, so this group must reproduce that pragma byte for byte: it does, see the
#     identical object hash in the commit that added this);
#   * hud/layout.cpp - the band's `draw_*` wrappers keep unfused `slwi`+`or`/`clrlslwi` pairs;
#   * hud/cockpit_quest.cpp - fn_802E7408 93.63 -> 100.0 and four siblings with it (measured per file).
# Three of three registered hud objects agree, so it is the lib's flag, not a per-unit deviation; the
# per-file pragmas were removed with this group and every object stayed byte-identical.
cflags_hud = [
    *cflags_main,
    "-opt nopeephole",
]

# menu flags (src/menu/menu_item.cpp). cflags_main plus `-opt nopeephole`: the retail `ItemName`
# (0x8029F628, 44 B / 13 ins) keeps `clrlwi r0,r3,16` + `slwi r0,r0,2` separate where the peephole pass
# folds them into one `clrlslwi r0,r3,16,2` (measured: 34.090908 -> 100.0 with the flag off, 36 B ->
# 44 B). The fold is a peephole emission, not a source shape - the index is a `u16` in both - and the
# unit carries 0 record-form instructions, like the Pl and stage bands beside it.
cflags_menu = [
    *cflags_main,
    "-opt nopeephole",
]

# Camellia flags. Evidence-backed per-object overrides for Camellia/camellia.c, which does not match with
# the cflags_runtime defaults:
#   * the retail object has no stmw/lmw -- it calls the EABI _savegpr_14/_restgpr_14 helpers
#     => -use_lmw_stmw off (24 B of the size gap)
#   * the retail object never fuses srwi+clrlwi into extrwi
#     => -opt nopeephole
#   * the retail object emits one lis+addi pair per S-box table (no shared base register)
#     => -pool off  (`-pool` controls whether a function shares one section-relative base register
#        across the distinct data objects it references; measured 2026-09-29: without the flag `.text`
#        is 0x5F34 against 0x5F64, the tables are addressed off a shared base, and the unit falls
#        99.96 -> 99.24 with camellia_encrypt/decrypt/setup 128/256 each dropping from 100 %)
#   * -O4,p / -inline auto do not reproduce the retail code shape at all
#     => -O3 -inline noauto
# With these, 9 of the 10 functions in the unit are byte-identical to the retail object and only
# camellia_setup256 differs (one 4-byte stack slot). See .pi/notes/camellia-match-process.md.
# The conflicting cflags_runtime defaults are removed rather than appended, so the command line has
# exactly one -O / -inline / -use_lmw_stmw.
cflags_camellia = [
    *[f for f in cflags_runtime if f not in ("-O4,p", "-inline auto", "-use_lmw_stmw on")],
    "-O3",
    "-inline noauto",
    "-use_lmw_stmw off",
    "-opt nopeephole",
    "-pool off",
    # Evidence: the retail object carries extab 0x48 that ours did not emit, and the same unwind-only
    # records the flag reproduces for g3d (e9522b4) and Pl. Analysis: .pi/notes/extab-gap.md.
    "-Cpp_exceptions on",
]

# RSO runtime flags (DOL-side RSO loader/linker, src/RSO/runtime.c, retail .text 0x804D9B4C..0x804DAE40).
# This unit is NOT built like Camellia: its retail object contains 9 record-form instructions
# (srwi. x4, add. x3, extsb., clrrwi.) while the Camellia object contains 0, and MWCC only emits the
# record forms with the peephole pass ON.  Fingerprinted from the retail object:
#   * the record forms above, the 5 `rlwimi` field-inserts in fn_804DAA24, and the retail prologue
#     instruction ORDER (lis / lwz / addi / li / mulhwu / srwi.) all need peephole + the instruction
#     scheduler, and fn_804DA834 needs optimizer level 4 (99.80 % at level 3)
#     => -opt peephole,schedule,level=4
#   * -O4,p implies -func_align 16, which pads every function to 16 B; the retail .text packs these
#     functions back to back
#     => -func_align 4
#   * RSOLink's prologue is `addi r11,r1,48; bl _savegpr_23` and its epilogue `bl _restgpr_23`, and
#     stmw/lmw do not occur anywhere in the range
#     => -use_lmw_stmw off
#   * RSOStaticLocateObject emits 12 `lis` for 11 distinct ADDR16_HA symbols, one dedicated base
#     register each (r19, r21-r30) and never a shared base.  `-pool off` was carried here for that
#     shape; measured redundant 2026-09-29: the object is byte-identical (every section, symbol table
#     and relocation; only `.comment`'s flag byte differs) with and without it, so the flag is gone.
# -inline noauto is not observable in the binary yet; it is carried over from the Camellia evidence
# and still needs a source-level experiment here.
# NOTE (corrects an earlier revision of this comment): the whole-DOL "extrwi == 0 => nopeephole"
# argument was wrong -- GNU objdump never prints the `extrwi` alias (it prints `rlwinm rX,rY,SH,MB,ME`),
# so that scan could not see the fused form at all.  The per-unit discriminator is the record-form
# count above, which is a real property of the retail bytes.
# The conflicting cflags_runtime defaults are removed rather than appended, so the command line has
# exactly one -O / -inline / -use_lmw_stmw.
cflags_rso = [
    *[f for f in cflags_runtime if f not in ("-O4,p", "-inline auto", "-use_lmw_stmw on")],
    "-O3",
    "-opt peephole,schedule,level=4",
    "-func_align 4",
    "-inline noauto",
    "-use_lmw_stmw off",
]

config.linker_version = "Wii/1.0"


# Helper function for Dolphin libraries
def DolphinLib(lib_name: str, objects: List[Object]) -> Dict[str, Any]:
    return {
        "lib": lib_name,
        "mw_version": "GC/1.2.5n",
        "cflags": cflags_base,
        "progress_category": "sdk",
        "objects": objects,
    }


# Helper function for REL script objects
def Rel(lib_name: str, objects: List[Object]) -> Dict[str, Any]:
    return {
        "lib": lib_name,
        "mw_version": "Wii/1.0",
        "cflags": cflags_rel,
        "progress_category": "game",
        "objects": objects,
    }


Matching = True                   # Object matches and should be linked
NonMatching = False               # Object does not match and should not be linked
Equivalent = config.non_matching  # Object should be linked when configured with --non-matching


# Object is only matching for specific versions
def MatchingFor(*versions):
    return config.version in versions


config.warn_missing_config = True
config.warn_missing_source = True
config.libs = [

    {
        "lib": "ai",
        "mw_version": "Wii/1.3",
        "cflags": cflags_main,
        "progress_category": "game",
        "objects": [
                        Object(NonMatching, "ai/ai_npc.cpp"),
                        # Registered (`.text`
                        # 0x802D44F4..0x802DDC04, 165 functions / 38672 B).  See the unit header for
                        # the seam, module and language evidence.
                        # Phase 4: the band's tail (0x802D9EA4..0x802E0740), recut from ai/fn_802D44F4.cpp; same cflags_main.
                        Object(NonMatching, "hud/cockpit.cpp"),
        ],
    },

    {
        "lib": "stage",
        "mw_version": "Wii/1.3",
        "cflags": cflags_main,
        "progress_category": "game",
        "objects": [
            # Registered once, at its final home (docs/plan.md 12): proposal
            # `802AD9C0_get_stg_w__Fv.cpp` (`.text` 0x802AD9C0..0x802B2978, 98 functions / 20408 B) -
            # the stage-work accessors and the block they read (`stage_w`, .bss 0x806B87C0).  Module
            # `stage` from the code (`get_stg_w`, `get_stg_weapon_work`, `get_stg_eft_col`, and the
            # `stage_w` name buffer `fn_802B050C` returns) and from the sibling `stage/fn_802B2978.c`;
            # no `__FILE__` string covers the range and the dump has only `zz_` placeholders, so the
            # file name is class 3 (brief section 2).  `.text` only: the range references `.bss`
            # (`stage_w`, `lbl_806BB7A0`) and shares the pool of the earlier units, so no data range
            # is claimed.
            # The gunner-shell pool `_SHELL_W` below the menu band (`.text` 0x802AA6A8..0x802ABD28,
            # 58 functions; cut out of `menu/menu_message.cpp`'s tail; extab 0x80013984..0x80013AC4,
            # extabindex 0x800314C4..0x800316A4, `.ctors` 0x8056F374, `.bss` 0x806AD698..0x806B8798,
            # `.sdata2` 0x8079A400..0x8079A410).  The seam to `Pl/pl_yure.cpp` is the static initialiser
            # `.ctors` word (one `__sinit` per TU) and the disjoint `.data`/`.bss`/`.sdata2` referrers -
            # see the unit headers.  Flags: `cflags_main` plus the source's `#pragma peephole off`, the
            # stage band's same deviation (see `stage/stg_w.cpp`).
            Object(NonMatching, "stage/shell.cpp"),
            Object(NonMatching, "stage/stg_w.cpp"),
            # Camera band 0x802B5C58-0x802BEAAC (132 functions, 36436 B), registered.  The lib and cflags are the neighbours' ("stage"
            # and "ai" both build with cflags_main / Wii/1.3) and the module is `camera`: the range's
            # own named exports are get_camera_pos / get_camera_direction / get_current_view_mtx /
            # set_quake_sub, and the seam at 0x802B5C58 is a .sdata2 pool jump.
            # Phase 4: fold of the head of stage/fn_802B2AA0.cpp, camera/fn_802B5C58.cpp and the head of light/light.cpp; the lib's cflags_main.
            Object(NonMatching, "camera/camera_main.cpp"),
            # Registered once, at its final home (docs/plan.md 12): proposal
            # `802BEAAC_fn_802BEAAC.cpp` (`.text` 0x802BEAAC..0x802C474C, 103 functions / 23712 B) -
            # the map's light work block.  Module `light` and file `light.cpp` from the range's own
            # real symbol names (`light_init__Fv`, `light_move__Fv`, `set_amblight__FUc8_GXColor`,
            # `make_dir_light2__FlPQ34nw4r4math4VEC38_GXColorl`), all confirmed by dumpmap; no
            # `__FILE__` string covers the range (brief section 2, class 2).  `.text` only: the range
            # references `lbl_806BB7E0`/`lbl_806BC300`/`lbl_806BD360` (.bss) and shares pooled
            # constants with its unclaimed neighbours, and the two `.ctors` words that point into the
            # range (0x8056F380 -> fn_802BEEE0, 0x8056F384 -> fn_802C2530) stay with their auto units
            # until those static objects are reconstructed.
            Object(NonMatching, "light/light.cpp"),
        ],
    },

    {
        "lib": "hud",
        "mw_version": "Wii/1.3",
        # cflags_hud = cflags_main + `-opt nopeephole`: every object in this lib needs the unfused forms
        # (see the group's evidence above); the three per-file `#pragma peephole off` are gone with it.
        "cflags": cflags_hud,
        "progress_category": "game",
        "objects": [
                        Object(Matching, "hud/fn_80324F7C.c"),
            # Registered once, at its final home (docs/plan.md 12).  The `proposal/802DDC04_fn_802DDC04`
            # range (0x802DDC04..0x802E4978, 149 functions) is a union of translation units, and this is
            # the one that lies wholly inside it: `.text` 0x802E0740..0x802E4978 (88 functions, 0x4238 B),
            # extab 0x80014DAC..0x80014FFC (74 8-byte records), extabindex 0x800332E8..0x80033660
            # (74 12-byte records), .data 0x805D5798..0x805D5B48, .sdata 0x807927B0..0x807927BA and
            # .sdata2 0x8079A8C4..0x8079A8E0.  The name `layout.cpp` is class-1 evidence: `.data`
            # 0x805D5800, 0xB = "layout.cpp", is referenced by `fn_802E2440`/`fn_802E2524` of this range
            # and by nothing else (the dump's local symbol for it is `s_layout.cpp_805d5800`).  Module
            # `hud`: the registered `hud/fn_80324F7C.c` calls this range's `get_lsp_data`/`draw_sprite_ary`
            # and carries the same `_mh_ivec2_`, and the lobby screens call the `draw_*` family directly -
            # the `ai` module below is a link-order neighbour, not this file's system.  cflags_main: the
            # range keeps `bl`s to its own tiny helpers (`fn_802E0DA8` -> `fn_802E0CE4`) and carries the
            # 74 extab records `-Cpp_exceptions on` emits.
            Object(NonMatching, "hud/layout.cpp"),
            # Registered (`.text` 0x802E7408..0x802EBED8, 64
            # functions / 19152 B; extab 0x800150C4..0x8001529C and extabindex 0x8003378C..0x80033A50,
            # 59 records each - both runs start exactly where `hud/layout.cpp`'s band's runs end).
            # The name is class-1 evidence: `.data` 0x805D5D08 is the bare source name
            # "cockpit_quest.cpp" (0x12 B, the dump's `_802e4e00s_cockpit_quest.cpp_805d5d08`) and it
            # is referenced from inside this range (`fn_802E7408`/`fn_802E7548`'s `nw4r::db::Panic`
            # asserts) and by nothing else.  Module `hud` from the naming scheme of the band's
            # neighbours: `cockpit.cpp` (0x802D9EB4..0x802E0740) and `layout.cpp`
            # (0x802E0740..0x802E4978) are both registered in this lib, and this range drives the
            # same two `lbl_806BDCC8` work records and calls the same `hud` 2D element library.
            # Same `cflags_main` as `hud/layout.cpp`.
            # `-pool off` evidence (playbook 43; compiled with and without the flag, scored against the target):
            # each row below reads 3+ `.data` tables of one section that this unit defines, and without the flag MWCC
            # shares one base register across them where retail loads each with its own `lis`/`addi`.
            #   quest_bar_a_next_id       93.188 -> 100.000
            #   quest_bar_b_next_id       83.841 ->  99.207
            #   quest_gauge_blend_update  82.810 -> 100.000
            #   quest_gauge_draw          93.728 ->  96.270
            # weighted code 16553.20 B -> 16765.04 B matched, no row lower with the flag.
            # Phase 4: fold of menu/fn_802E4978.cpp, this unit, hud/fn_802EBED8.cpp and the head of ef/eft035.cpp; `-pool off` is this unit's own and now covers the absorbed bodies (see the unit header).
            Object(NonMatching, "hud/cockpit_quest.cpp", cflags=[*cflags_hud, "-pool off"]),
            # Registered, at its final home (docs/plan.md 12):
            # `.text` 0x803250B0..0x803253BC (1 function, 0x30C B) plus the range's own extab
            # 0x80016224..0x8001622C and extabindex 0x8003519C..0x800351A8 (one framed function; both
            # runs start exactly where `hud/fn_80324F7C.c`'s end and the next registered band begins, and
            # the target object owns no `.data`/`.sdata2` at all).  Module `hud`: the range below is the
            # landed `hud/fn_80324F7C.c`, which this unit calls with such a move-work record, and the
            # range reads the same `lobby_w`/`lb_npc`/`lbl_806BE340` `.bss` keys; no `__FILE__` string
            # covers it and the dump answers only `zz_03250b0_`, so the unit is named for what it does -
            # the per-frame move-work update (its naming pass replaced the map's stem; see the header).
            # `.cpp`, not `.c`: the target relocates against `get_move_work_adrs__FUc`
            # / `set_zmode__FbUcb` / `get_option_cfg__FUc`, so the callees are C++ and rule 9 forbids
            # spelling those manglings (see the file header).  Same `cflags_hud` as the three siblings.
            # `Matching`: the object's `.text`/`extab`/`extabindex` are byte-identical to the target's
            # and all 50 relocations agree on offset, type, addend and target section, so this
            # registration substitutes the object rather than leaving the range's original bytes.
            Object(Matching, "hud/move_work_update.cpp"),
            # Registered once, at its final home (docs/plan.md 12): the character-state network sync
            # (`.text` 0x80334568..0x80338808, 77 functions / 17056 B; extab 0x8001677C..0x80016994 and
            # extabindex 0x800359A0..0x80035CC4, one 8- and one 12-byte record per framed function;
            # `.data` 0x805E2788..0x805E27D4, the two switch tables its bodies emit).  Its builders pack
            # `_PLW`/`_ENEMY_WORK`/`EmcWork`/`EftSlot` state into small local messages and send them with
            # `broadcastSessionCommand` (the `NetworkSessionManagerPat` slot 0x128 send, guarded by
            # `isServerSelectState()`); its receivers unpack them back.  No `__FILE__` string covers the
            # range and the runtime dump answers `zz_` for all 77 addresses; the file stem names the
            # role (`net_char_sync`, a GUESS).  Module `hud` is the flag-evidence choice: `cflags_hud`
            # is the group whose `-opt nopeephole` (keeps a redundant `clrlwi` before a narrowing
            # store) and `-Cpp_exceptions on` (67 framed functions, 67 extab records) reproduce the
            # target, and `hud` is also the nearest preceding registered unit (`hud/fn_80324F7C.c`).
            # The *content* reads as network rather than HUD, which the unit header records as this
            # unit's first promotion candidate.  Not `Matching`: our object also emits the 0x164-byte
            # dispatcher switch table (`jumptable_805E0EA0`), which stays unclaimed (span-blocked by
            # `enemy/em_pl_frame`), and an 8-byte `.sdata2` pool entry.  The seam is unproven.
            # Phase 4: fold of enemy/em_pl_frame.cpp (no bodies) and hud/net_char_sync.cpp, cflags_hud of the lib.
            Object(NonMatching, "hud/pl_frame_sync.cpp"),
        ],
    },

    {
        # The menu lib: `cflags_menu` (the group's evidence is its comment above); each unit's notes are its header.
        "lib": "menu",
        "mw_version": "Wii/1.3",
        "cflags": cflags_menu,
        "progress_category": "game",
        "objects": [
            Object(NonMatching, "menu/menu_item.cpp"),
            Object(NonMatching, "menu/menu_item_sub.cpp"),
            Object(NonMatching, "menu/menu_message.cpp"),
            Object(NonMatching, "menu/menu_infomation.cpp"),
            Object(NonMatching, "menu/fn_8031A6C0.cpp"),
            Object(NonMatching, "menu/menu_item_effect.cpp"),
            Object(NonMatching, "menu/menu_effect_slot.cpp"),
            Object(NonMatching, "menu/fn_8031EA8C.cpp"),
            Object(NonMatching, "menu/menu_item_page.cpp"),
            Object(Matching, "menu/menu_note.cpp"),
            Object(NonMatching, "menu/menu_row.cpp"),
            Object(NonMatching, "menu/menu_result.cpp"),
            Object(NonMatching, "menu/multi_result.cpp"),
            Object(NonMatching, "menu/get_pop_dat_ptr.cpp"),
            Object(NonMatching, "lobby/lb_server_sel_trans.cpp"),  # phase 4 recut of the line above (its tail), same cflags_menu
            # Registered once, at its final home: the quest
            # entry/init band (`.text` 0x803AA4A4..0x803B0F98, 70 functions / 27380 B) with extab
            # 0x80018A6C..0x80018C54 (60 records) and extabindex 0x80038E08..0x800390E4 (60 x 12 B) -
            # both runs are exactly the gap between the bracketing registrations.  Module `quest` from
            # the runtime dump's own names for the band's globals (`q_result_msg_adrs`,
            # `quest_ex_condition_tbl`, `em_bui_tbl`/`em_hokaku_rem_l,h`) plus the range's one real
            # function name, `quest_init(unsigned char)` 0x803AD47C; the file name is a marked GUESS
            # (no `__FILE__` string reaches the range - every `.data` reference of all 70 auto objects
            # was relocated to check).  The seam is UNPROVEN (the discovery `--max-bytes` cap) and the
            # run is plainly a sequence of objects; see the unit header.  Same lib and flags as its
            # link neighbour `menu/multi_result.cpp` (cflags_menu: the band carries 0 record-form
            # instructions and needs `-Cpp_exceptions on`, which every one of its 60 framed functions
            # shows in its own extab record).
            Object(NonMatching, "quest/quest_item_slot.cpp"),
            Object(NonMatching, "quest/quest_entry.cpp"),
            # Registered once, at its final home: the field-side
            # enemy population/roster manager (`.text` 0x803B465C..0x803BE30C, 139 functions / 40112 B;
            # extab 0x80018DC4..0x80019164 and extabindex 0x8003930C..0x8003987C, both the exact gap
            # between the bracketing auto objects).  Module `enemy` (evidence class 3+4, GUESS recorded
            # in the unit header): no `__FILE__` string covers the range, the dump answers `zz_` for
            # every symbol in it, and the bracketing registered units name different modules - but nine
            # registered `src/enemy/*` units call into the range and it owns the 0x224-byte monster
            # roster record whose string pool names the `em_set`/`_pop.dat` data it consumes.  Same
            # `cflags_menu` as the address neighbour below it (`menu/multi_result.cpp`); the flag probe
            # is recorded in the unit header.  This pass writes 12 of the 139 bodies; the rest keep
            # their original bytes and measure 0 %.
            Object(NonMatching, "enemy/em_pop.cpp"),
            Object(NonMatching, "enemy/em_model.cpp"),  # phase 4 recut of the line above (its tail), same cflags_menu
            # Registered once, at its final home: the arena task band (`.text`
            # 0x804459E4..0x80448404, 19 functions / 10784 B; extab 0x8001DE9C..0x8001DF24 and
            # extabindex 0x8003EB20..0x8003EBEC, both exactly this run's records - every entry is an
            # 8-byte extab chunk and the sequence is monotone, so they tile the band with no cut).
            # Module `quest` and file name `arenatask.cpp` are class-1 evidence: the range's own
            # `.data` 0x80607390 is the bare `__FILE__` string "arenatask.cpp", one copy in the DOL,
            # and its only referrer (0x80445B3C) is inside the range's head, `arena_resource_load`
            # (0x804459E4) - which `ArenaSelExec` (0x80041A34) reaches through its task `arena_task`
            # (0x804463C4).  The left edge is the band's own `.data`/`.sdata` run start (the previous
            # object's `.data` ends exactly at 0x80607210); the right edge is the save-file module's
            # first function, whose nine-function block shares the private `.bss` path buffer
            # `lbl_806E40C0` and calls `strcpy`/`OSReport`/`NAND*`.  One internal cut (0x80446990) is
            # a candidate the data cannot settle - it is written up in the unit header, and
            # `tudiscover.py`'s "strong" cuts in this band are unusable: it has no strong cut at
            # either registered edge (share 0.005 left, 0.003 right), its strong left candidate is 23
            # functions earlier in another band, and the one in-band `.sdata2` pin is vetoed by this
            # unit's own shared pool constants (`arena_zero_f`/`arena_50f`, must-link 15578..15585) -
            # measured with `at 0x804459E4 --window 48`, and the phenomenon behind the caveat is real
            # (640 of the 7245 `.sdata2` labels are cited by more than one registered unit).  Same
            # `cflags_menu` as its link neighbour `quest/quest_entry.cpp`.
            # This pass writes 6 of the 19 bodies (740 B of 10784); the other 13 keep their original
            # bytes and are listed with their blockers in the unit header and the outbox.
            Object(NonMatching, "quest/arenatask.cpp"),
            Object(NonMatching, "menu/menu_placeinfo.cpp"),
            Object(NonMatching, "menu/movie.cpp"),
            Object(NonMatching, "menu/menu_plsearch.cpp"),
            Object(NonMatching, "menu/menu_sysmsg.cpp"),
        ],
    },

    {
        "lib": "enemy",
        "mw_version": "Wii/1.3",
        "cflags": cflags_main,
        "progress_category": "game",
        "objects": [
            # Registered once, at its final home (docs/plan.md 12).  The `enemy/fn_8011D448.cpp`
            # range (`.text` 0x8011D448..0x801251D0, 101 functions / 32136 B): the enemy module's
            # effect-spawner band - the `_EFT` creators whose owner is an `_ENEMY_WORK` (they install
            # `fn_8011D8C0`/`fn_8011D9B8` release/dispatch callbacks exactly like `ef/eft001.cpp`) and
            # the action/state machine that follows them.  Module `enemy` from the code (`_ENEMY_WORK`,
            # `em_parts_damage_level_get`) and the naming scheme of the neighbour above; no `__FILE__`
            # string survives and the dump's only in-range real name is a function name, so the map stem
            # is kept (brief section 2, class 3+4).  Sections: extab 0x8000C63C..0x8000C8AC (78 records),
            # extabindex 0x80026844..0x80026BEC (78 x 12 B), .text 0x8011D448..0x801251D0.  C++ from the
            # range's mangled callees; every plain `fn_XXXXXXXX` definition is `extern "C"`.
            Object(NonMatching, "enemy/fn_8011D448.cpp"),
                        # enemy/em_common.cpp: the enemy control unit
            # (0x801251D0..0x8012BA00, 145 symbols). Registered once, at its final home
            # (docs/plan.md 12); cflags are this lib's.
            Object(NonMatching, "enemy/em_common.cpp"),
            # Registered (a 0x80137604 run discovery
            # proposed): an enemy's per-motion action/rotation update set, 20 functions / 0x2670
            # bytes.  Module `enemy` from the link band (both bracketing units are `enemy`) and
            # from the code (it calls `em_act_ck(_ENEMY_WORK*)`, `get_enemy_data`, ...); C++
            # because every mangled callee must be declared at its real signature (rule 9).
            # No `__FILE__` string survives in the range and the dump answers only `zz_`
            # placeholders, so the file keeps the map's own stem (see the unit's header).
            Object(NonMatching, "enemy/em_motion_update.cpp"),
            Object(NonMatching, "enemy/fn_80138074.c"),
            # Registered (a 0x8013ACC4 run discovery
            # proposed): the enemy user-data command interpreter and its 0x100-entry dispatch
            # table, 3 functions / 0x119C bytes.  Module `enemy` from the link band (both
            # bracketing units are `enemy`) and from the code (`_ENEMY_WORK`, `fn_8013A900`);
            # C++ because the range calls the mangled `ran_suu__Fl`.  No `__FILE__` string
            # survives and the dump answers only `zz_` placeholders, so the file keeps the map's
            # own stem (see the unit's header).
            Object(NonMatching, "enemy/fn_8013ACC4.cpp"),
            Object(NonMatching, "enemy/em_kind.cpp"),
            # Registered: the enemy control TU's lower half,
            # 155 functions / 0x6B28 bytes (0x801411B8..0x80147CE0), plus the extab/extabindex runs
            # its 107 framed functions carry.  Module `enemy` and file `enemy_control.cpp` from the
            # `__FILE__` string at 0x805A1BB8 ("enemy_control.cpp"), referenced only by this range's
            # `fn_801411B8` (0x801411DC/0x80141258) - brief section 2 option 1.  C++ because four
            # callees are mangled and rule 9 forbids spelling a mangling at the call site.  The
            # upper edge 0x80147CE0 is `attribute.py`'s `--max-bytes` cut, not evidence: this unit
            # and the registered `enemy/fn_80147CE0.cpp` are ONE TU (the `__FILE__` string lives
            # here, `em001_prog_tbl` straddles the cut, and this unit's extab/extabindex runs end
            # exactly where that unit's begin).  The merge is requested in the outbox; see the unit
            # header.
            Object(NonMatching, "enemy/enemy_control.cpp"),
            Object(NonMatching, "enemy/em001_prog.cpp"),
            # proposal/801550FC_fn_801550FC.cpp: the em003 action unit (0x801550FC..0x8015D860,
            # 104 functions).  C++ (the range defines three em003_* manglings).  The boundary is
            # provisional - see the unit header.
            Object(NonMatching, "enemy/em003_prog.cpp"),
            # enemy/em008_prog.cpp: the em008 per-action state-step band
            # (0x8015D860..0x8015E854, 28 functions), the run between the two units above and
            # below.  C++ (the range's callees are manglings and it allocates with `operator
            # new`); registered once, at its final home - the file keeps the map's `fn_XXXXXXXX`
            # stem because no `__FILE__` string names it (rule 7 deferred, see the unit header).
            Object(NonMatching, "enemy/em008_prog.cpp"),
            # Registered: the enemy per-area seat/action unit
            # (0x80165FC8..0x801679B0, 21 functions / 0x19E8 bytes), the exact unclaimed gap between
            # the two registered units above and below (each ends where this range begins/ends), plus
            # the extab/extabindex runs its 18 framed functions carry and the one .ctors word for its
            # static initializer `fn_80166330`.  Module `enemy` from the link band (both bracketing
            # units are `enemy`) and from the code (`_ENEMY_WORK`, `fn_802B0668`, `em_frame_check`);
            # C++ because the range reaches mangled callees through their real signatures (rule 9).
            # No `__FILE__` string survives in the range and the dump answers only `zz_` placeholders,
            # so the file keeps the map's own stem (see the unit's header).
            Object(NonMatching, "enemy/em010_prog.cpp"),
            Object(NonMatching, "enemy/em011_prog.cpp"),
            Object(NonMatching, "enemy/em015_prog.cpp"),
            # Registered (the 0x80181C88 run discovery
            # proposed): the enemy MHchar material/step band 0x80181C88..0x80182D5C, 21 functions /
            # 0x10D4 bytes, plus the 16 extab/extabindex entries its functions carry.  Module
            # `enemy` from the link band (fn_80178378.cpp ends at this range's start, fn_80182D5C.cpp
            # starts at its end) and from the code (`_ENEMY_WORK`, `em_parts_damage_level_get`,
            # `get_em_chg_scale`, the MHchar TEV setters).  C++ because the range reaches MHchar
            # members through their real signatures.  No `__FILE__` string survives in the range
            # (the only `enemy_control.cpp` literal is referenced from the already-registered
            # enemy_control band, not here) and the dump answers only `zz_` placeholders, so the
            # file keeps the map's own stem (see the unit's header).
            Object(NonMatching, "enemy/em016_prog.cpp"),
            Object(NonMatching, "enemy/em012_prog.cpp"),
            # Registered (a 0x80191598 run discovery
            # proposed): the enemy aim/action group the per-enemy class tables at 0x805AA960..
            # 0x805AAA60 hold, 26 functions / 0x1154 bytes plus its extab/extabindex run.
            # Module `enemy` from the link band and the code (`em_act_ck(_ENEMY_WORK*)`,
            # `get_move_work_adrs(3)`, the enemy work's aim record); C++ because every callee out
            # of the range is a mangled symbol.  No `__FILE__` string survives in the range and
            # the dump answers only `zz_` placeholders, so the file keeps the map's own stem (see
            # the unit's header).
            Object(NonMatching, "enemy/em018_prog.cpp"),
            # Registered once, at its final home (docs/plan.md 12) (the 0x801CCBC4 run discovery proposed; 58
            # functions / 0x76C8 bytes): the enemy action/step band 0x801CCBC4..0x801D428C, plus the
            # extab run 0x8000FF4C..0x800100D4 and the extabindex run 0x8002BDDC..0x8002C028 its 49
            # framed functions carry.  Module `enemy` from the link band (the unit below is
            # `enemy/fn_801B7020.cpp`, the unit above `enemy/fn_801D428C.cpp`, and every callee out
            # of the range is an `_ENEMY_WORK`-based enemy-band body); C++ because the range reaches
            # mangled callees (`em_frame_check__FP11_ENEMY_WORKUsff`,
            # `calcVecAng2__FPQ34nw4r4math4VEC3PQ34nw4r4math4VEC3`, `get_em_scale__FP11_ENEMY_WORK`).
            # No `__FILE__` string is reachable from the range and the runtime dump answers only
            # `zz_` placeholders, so the file keeps the map's own stem (see the unit's header).
            Object(NonMatching, "enemy/em005_act.cpp"),
            # Registered (a 0x801B0010 run discovery
            # proposed): the em030 (enemy #30) program unit, 60 functions / 0x4448 bytes plus the
            # extab/extabindex entries its 45 framed functions carry.  Module `enemy` from the link
            # band (both bracketing units are `enemy`) and from the range's own data: the `.data`
            # table `em030_prog_tbl` (0x805B0FD0) lists this range's entry points, and two of the
            # range's symbols carry real runtime-dump names (`em030_condition_ck`,
            # `em030_homing_range_ck`).  C++ because the range reaches mangled callees through
            # their real signatures.  No `__FILE__` string survives in the range and the dump
            # answers only `zz_` placeholders for the other 58 rows, so the file keeps the map's
            # own stem (see the unit's header).
            Object(NonMatching, "enemy/em030_prog.cpp"),
            Object(NonMatching, "enemy/em034_prog.cpp"),
            # Registered once, at its final home (docs/plan.md 12): the enemy action band 0x801D80EC..0x801DB8E0, 49
            # functions / 0x37F4 bytes, plus the extab run 0x800101E4..0x8001031C (39 records) and
            # the extabindex run 0x8002C1C0..0x8002C394 (39 records) its framed functions carry.
            # Module `enemy` from the link band (the unit below is `enemy/fn_801D428C.cpp` and the
            # unit above `enemy/fn_801DB8E0.cpp`) and from the code (every function takes the shared
            # `_ENEMY_WORK`).  C++ because the range reaches mangled callees
            # (`em_frame_check__FP...`, `calcVecAng2__FP...`, `rotVecY__FP...`).  No `__FILE__`
            # string is reachable and the runtime dump answers only `zz_` placeholders, so the file
            # keeps the map's own stem (see the unit header).
            Object(NonMatching, "enemy/em007_act.cpp"),
            # Registered once, at its final home (docs/plan.md 12) (the 0x8019ED34 gap between this unit and
            # `enemy/fn_801A4504.cpp`; 2026-09-30 recut: left edge 0x8019E670 (the end of the preceding TU's
            # static initializer), the former enemy/fn_801A4504.cpp folded in and the head of
            # enemy/fn_801A9540.cpp taken up to 0x801AA154, one TU): the enemy motion/action band 0x8019ED34..0x801A4504, 65
            # functions / 0x57D0 bytes, plus the extab run 0x8000F14C..0x8000F304 (55 records) and
            # the extabindex run 0x8002A8DC..0x8002AB70 (55 records).  Module `enemy` from the link
            # band (both bracketing units are `enemy`) and the code (every callee out of the range
            # is enemy-band, every state machine switches on `_ENEMY_WORK::state`); the name keeps
            # the map's `fn_` stem (no `__FILE__` string, the dump answers only `zz_`/`FUN_`).  C++,
            # every plain `fn_XXXXXXXX` definition `extern "C"` (see the unit's header).
            Object(NonMatching, "enemy/em025_prog.cpp"),
            # 2026-09-30 recut of the 0x801B7020..0x801E0ADC band into its real translation units (the
            # `.ctors` words, the `em0NN_prog_tbl` data order and the `.sdata2` pool dedupe agree): em036
            # 0x801B7020..0x801B98C8 (this file), em040 0x801B98C8..0x801BB758, em006 0x801BB758..0x801C29F8
            # (`fn_801BD6C0.cpp`), em004 0x801C29F8..0x801CA8DC (`fn_801CA004.cpp`), em005 0x801CA8DC..0x801D71C4
            # (`fn_801CCBC4.cpp`), em007 0x801D71C4..0x801E0ADC (`fn_801D80EC.cpp`); the files keep their old stems.
            Object(NonMatching, "enemy/fn_801B7020.cpp"),
            # Registered in the 2026-09-30 recut: the em040 translation unit (no bodies written yet); the name
            # follows `em040_prog_tbl` (0x805B2808), the first data of its `.data` chunk (see the unit's header).
            Object(NonMatching, "enemy/em040_ai.cpp"),
            # Registered: the enemy motion/act-instruction
            # band's continuation, 128 functions / 0xC944 bytes (0x801BD6C0..0x801CA004) plus the
            # extab run 0x8000FACC..0x8000FDFC (102 records) and the extabindex run
            # 0x8002B71C..0x8002BBE4 (102 x 12 B) its framed functions carry.  Module `enemy` from
            # the link band (the unit below ends exactly at this range's start; the unit above starts
            # later at 0x801D428C after the still-unclaimed 0x801CA004..0x801D428C run) and from the
            # code (every callee is the enemy work API).  C++ because the range reaches mangled
            # callees.  No `__FILE__` string survives and the dump answers only `zz_` placeholders,
            # so the file keeps the map's own stem (see the unit header).
            Object(NonMatching, "enemy/em006_prog.cpp"),
            # Registered once, at its final home (docs/plan.md 12): the enemy action/state band, 47 functions /
            # 0x2BC0 bytes (0x801CA004..0x801CCBC4), plus the extab run 0x8000FDFC..0x8000FF4C and
            # the extabindex run 0x8002BBE4..0x8002BDDC its 42 framed functions carry.  Module
            # `enemy` from the link band (both bracketing registered units are `enemy/*`) and from
            # the code (every function takes the shared `_ENEMY_WORK`); C++ because the range's
            # callees are mangled (`getTevKColor__6MHchar...`, `__nw__FUl`).  No `__FILE__` string
            # is reachable and the dump answers only `zz_` placeholders, so the file keeps the
            # map's own stem (see the unit header).
            Object(NonMatching, "enemy/em004_act.cpp"),
            # Registered once, at its final home (docs/plan.md 12): the enemy per-action state-machine band
            # (`.text` 0x801A9540..0x801B0010, 82 functions / 0x6AD0 bytes) plus its
            # extab run 0x8000F324..0x8000F54C (69 records) and extabindex run
            # 0x8002ABA0..0x8002AEDC (69 x 12 B).  Module `enemy` from the link band (both
            # bracketing registered units are `enemy/*`) and from the code (every callee is the
            # `_ENEMY_WORK` API and every body drives that record); C++ because the range
            # reaches genuinely mangled callees (`em_frame_check__FP11_ENEMY_WORKUsff`,
            # `setVector3__FPQ34nw4r4math4VEC3fff`, `ran_suu__Fl`) through their real
            # signatures (rule 9).  No `__FILE__` string is referenced by the range (checked
            # with `nm -u` over its 82 split objects) and the runtime dump answers only `zz_`
            # placeholders, so the file keeps the map's own stem (see the unit's header).
            Object(NonMatching, "enemy/em027_prog.cpp"),
            # Registered once, at its final home (docs/plan.md 12):
            # the em033/em035 enemy-program handlers (`.text` 0x8035E034..0x8035F2B4, 16 functions /
            # 0x1280 bytes) plus their extab/extabindex group.  Module `enemy` from the `.data` program
            # tables `em033_prog_tbl`/`em035_prog_tbl` that list the range's entry points and from the
            # shared `_ENEMY_WORK` record every body drives; the name keeps the map's `fn_` stem (no
            # `__FILE__` string is reachable and the dump answers only `zz_` placeholders).  Sections:
            # extab 0x80017574..0x800175DC (13 records), extabindex 0x80036E94..0x80036F30 (13 x 12 B),
            # `.text` 0x8035E034..0x8035F2B4.  C++; every plain `fn_` definition is `extern "C"`.
            # Phase 4: fold of the head of ef/eft052.cpp (no bodies) and enemy/fn_8035E034.cpp; cflags_main of the lib.
            Object(NonMatching, "enemy/em033_prog.cpp"),
            # Registered once, at its final home (docs/plan.md 12) and re-cut
            # 2026-09-29 to the em024 monster's own AI: `.text` 0x8034F138..0x80358624 (69 functions /
            # 38124 B), extab 0x800171A4..0x8001736C (57 records), extabindex 0x800368DC..0x80036B88
            # (57 x 12 B), `.data` 0x805EBBE0..0x805ED0C0 (`em024_prog_tbl`, the switch jump tables and
            # per-motion tables), `.sdata` 0x80793330..0x80793338 and `.sdata2` 0x8079B3C8..0x8079B640.
            # The original range 0x8034C1D0..0x80358624 was three TUs: `menu/menu_row.cpp` and
            # `Pl/pl_act_class3.cpp` took the head and the player band.  One TU from here: the pool
            # float `lbl_8079B3CC` is loaded by 28 functions across the range and by nothing outside.
            # File name a GUESS from `em024_prog_tbl` and the enemy id 0x18 tested by the caller of
            # `em024_action11_state5_ck`.  C++; every plain `fn_` definition is `extern "C"`.
            Object(NonMatching, "enemy/em024_ai.cpp"),
            # Registered, re-cut to the em035 program's own
            # half: `.text` 0x8035F2B4..0x8035FC18 (20 functions / 2404 B) plus its extab run
            # 0x800175DC..0x80017634 (11 records) and extabindex run 0x80036F30..0x80036FB4
            # (11 x 12 B), both contiguous with `enemy/fn_8035E034.cpp`'s runs above.  The brief's
            # `--max-bytes` range was 0x8035F2B4..0x80365C84, which is TWO TUs: the `em035_prog_tbl`
            # (0x805ED838) entry-point list ends at `fn_8035FB60`, whose body ends exactly at
            # 0x8035FC18, and from there the run drives the lobby work block `lobby_w` and the
            # crafting-screen path (`seisan_data`, `fn_8021AA78`, `Get_pl_type`), not `_ENEMY_WORK` -
            # the 61-record extab run splits 11 + 50 at the same address and our object's own extab
            # equals the first 11 records byte for byte.  The lobby half
            # (0x8035FC18..0x80365C84, 59 functions, extab 0x80017634..0x800177C4) is left for its own
            # `lobby` unit (see the unit header).  Module `enemy` (the entry points come from
            # `em035_prog_tbl`, every body drives `_ENEMY_WORK`, no `__FILE__` string is reachable and
            # the dump answers only `zz_`), and the file and all 20 symbols are GUESSES from their own
            # bodies on the module's `em*` scheme (`em_action.cpp`'s `em_act_*` sibling precedent) -
            # the map had only `fn_XXXXXXXX`, so the batch also renames its own 20 rows (see the unit
            # header's NAMING section for each derivation).  C++;
            # every plain `fn_` definition is `extern "C"`.  Same `cflags_main` as its enemy
            # neighbours plus a file-wide `#pragma peephole off` (the target keeps the unfused
            # `addi`+`cmpwi` timer compares).
            Object(NonMatching, "enemy/em035_prog.cpp"),
            # Registered once, at its final home (docs/plan.md 12): the em020 enemy program
            # (`.text` 0x8036CF64..0x80375084, 57 functions / 0x8120 B, plus its extab run
            # 0x800178E4..0x80017A5C - 47 records - and extabindex run 0x800373BC..0x800375F0 -
            # 47 x 12 B; each is exactly the gap the bracketing split objects leave).  Module
            # `enemy` and the file name `em020_prog` (brief class 3): the map's own global
            # `em020_prog_tbl` (`.data` 0x805EE098, 0x70 B) lists seven of this range's handlers by
            # address (0x8036E2BC/E320/E6B8/72D58/E570/E574/733BC) exactly as `em035_prog_tbl`
            # lists `enemy/em035_prog.cpp`'s, and the range's callee profile is the enemy band's
            # (`em_frame_check`, `em_get_mot_no`, `em_act_ck`, `get_joint_wmat_em`, `fn_8013*`).
            # No `__FILE__` string covers the range and the dump answers only `zz_` placeholders.
            # C++; every plain `fn_` definition is `extern "C"`.  Same `cflags_main` as its enemy
            # neighbours.  SEAM UNPROVEN (the `.sdata2` run 0x8079B820..0x8079BC64 is an ordered
            # disjoint partition between the two neighbours' pools, which is the reliable class -
            # but an ordered partition cannot separate one object from two adjacent ones); the range
            # also holds a head group (0x8036CF64..0x8036E26C) that owns no pool and no `.data`, and
            # `em020_prog_tbl`'s first entry (0x8036E2BC) is the apparent cut - a re-cut is requested
            # in this batch's `config_requests`.  See the unit header for the full evidence.
            Object(NonMatching, "enemy/em020_prog.cpp"),
            # Registered once, at its final home (docs/plan.md 12): the em019 monster-AI file's body
            # (`.text` 0x80378F9C..0x8037EA64, 61 functions / 0x5AC8 bytes) with its extab
            # 0x80017C44..0x80017DBC (47 records) and extabindex 0x800378CC..0x80037B00
            # (47 x 12 B).  Module `enemy` and the name `em019` from `em019_prog_tbl`
            # (0x805EE518, `scope:global`), which starts the `.data` block right after the em020
            # one and lists this band's entry points (fn_80379090, fn_80379124, fn_8037946C,
            # fn_8037924C, fn_8037939C, fn_8037F524); the file's real extent is
            # 0x80378F9C..0x8037F940 (the strong right seam `tudiscover.py at 0x8037E0E8`
            # reports), and the brief's 0x8037EA64 cut is a `range` config_request.  C++;
            # every plain `fn_` definition is `extern "C"`.  See the unit's file header.
            Object(NonMatching, "enemy/em019_ai.cpp"),
            # Registered once, at its final home (docs/plan.md 12): proposal
            # `803253BC_fn_803253BC` (the map stem it was claimed under) - the 0x803253BC-0x8032C920
            # band (85 functions / 30052 B) between the registered `hud/fn_80324F7C.c` above and the
            # unclaimed 0x8032C920 run below.  Module `enemy` (brief class 3) from the code: the range
            # drives the `_ENEMY_WORK` record through `em_frame_check`/`em_get_mot_no`/`em_die_ck`/
            # `em_after_frame_check` and calls into the registered enemy units
            # `enemy/enemy_control.cpp`, `enemy/fn_801251D0.cpp` and `enemy/fn_8012EC74.cpp`
            # (fn_8012F5B8 x75, fn_80130478 x62, fn_801251D0 x60, fn_801303EC x51, fn_8012F93C x44,
            # fn_801280F4 x42).  No `__FILE__` string covers the range and the runtime dump answers
            # only `zz_<addr>_` for every address probed (`dumpmap.py join` reports no rename candidate
            # anywhere in it), so the file name and the 14 symbols the file defines are **guesses from
            # their bodies** on the module's `em_*` scheme - `em_action.cpp` / `em_act_*`; the file
            # header lists each one with its reason.  The `rule 7 deferred` escape the file still
            # carries covers only the 25 callee names it references in other units (42 occurrences),
            # which a cross-unit rename batch owns, not this unit.  cflags_main, the lib flag group
            # its enemy neighbours use.  Claims .text 0x803253BC-0x8032C920, extab
            # 0x8001622C-0x80016464, extabindex 0x800351A8-0x800354FC and the one `.ctors` word
            # 0x8056F39C -> fn_8032C65C.
            Object(NonMatching, "enemy/em_action.cpp"),
            # Phase 4: folded into enemy/em020_prog.cpp (was Matching, demoted).
            # Registered once, at its final home (docs/plan.md 12), renamed in phase 4 from `enemy/fn_80387844`:
            # the em009 enemy's monster-AI action band, `.text` 0x803868DC..0x8038EC44 (2026-09-30 recut of the
            # range first registered as 0x80387844..0x8038E8E8; see the unit's header).  Module `enemy` from the code (every body drives the shared `_ENEMY_WORK` record
            # through `em_frame_check__FP11_ENEMY_WORKUsff`, `em_after_frame_check`, `get_joint_wpos_em`,
            # `em_magma_check`, `get_em_chg_scale`) and from the `.data` `em0XX_prog_tbl` program tables
            # of the bracketing enemy bands; C++ because the range reaches genuinely mangled callees
            # (`setVector3__FPQ34nw4r4math4VEC3fff`, `mulVecMatAddTrans`, `rotVecY`) through their real
            # signatures (rule 9).  No `__FILE__` string is reachable from the range and the runtime dump
            # answers only `zz_` placeholders, so the file stem is derived from the map's `em009_act_*` names.
            Object(NonMatching, "enemy/em009_act.cpp"),
            # Registered once, at its final home (docs/plan.md 12):
            # the enemy note-pane/program band's shared support block, `.text` 0x80382310..0x803868DC, extab to
            # 0x8001803C, extabindex to 0x80037EC0 (2026-09-30 recut of the range first registered as
            # 0x80382310..0x80387844).  Module `enemy` from the code (every body drives the shared
            # `_ENEMY_WORK` record through `em_frame_check__FP11_ENEMY_WORKUsff`, `em_act_ck`, `em_magma_check`,
            # `get_joint_wpos_em`, `get_em_chg_scale`) and from the `.data` `em019_prog_tbl`/`em009_prog_tbl`
            # program tables that bracket the range; the next registered unit is `enemy/em009_act.cpp`.
            # C++ because the range defines `qn_get_motion_no__FP7_QNPC_W` and reaches genuinely mangled
            # callees through their real signatures (rule 9).  No `__FILE__` string is referenced by the
            # range and the runtime dump answers only `zz_` placeholders, so the file keeps the map's own
            # `fn_80382310` stem.
            # Phase 4: the old range is recut into enemy/em019_ai.cpp (head), enemy/em_prog_support.cpp and enemy/em_prog_tail.cpp; same cflags_main.
            Object(NonMatching, "enemy/em_prog_support.cpp"),
            Object(NonMatching, "enemy/em_prog_tail.cpp"),
            # Registered (`.text` 0x802F5138..0x802FA9A0, 72
            # functions / 22632 B).  Module `enemy` from the code (of the range's 194 distinct
            # callees the largest block is the enemy module - `em_die_ck`, `em_work_die_ck`,
            # `em_frame_check`, `fn_8012EC74.cpp`'s motion band - and every body drives the shared
            # `_ENEMY_WORK` record) and from the `.data` `em0XX_prog_tbl` program tables that
            # bracket the band; no `__FILE__` string reaches the range and the runtime dump answers
            # only `zz_` placeholders, so the file keeps the map's own `fn_802F5138` stem (brief
            # section 2, class 3+4; see the unit header).  Language C++ (the range reaches genuinely
            # mangled callees through their real signatures, rule 9); every plain `fn_` definition is
            # `extern "C"`.  Sections: `.text` 0x802F5138..0x802FA9A0, `extab`
            # 0x80015514..0x800156B4 (52 records), `extabindex` 0x80033E04..0x80034074 (52 x 12 B) and
            # the `.ctors` word 0x8056F390..0x8056F394 (`fn_802F9898`) - each run is exactly the gap
            # the bracketing objects leave (`ef/eft035.cpp` ends extab at 0x80015514 / extabindex
            # 0x80033E04 / .ctors 0x8056F38C, and `fn_802FA9A0`'s object starts extab at 0x800156B4 /
            # extabindex 0x80034074).
            Object(NonMatching, "enemy/fn_802F5138.cpp"),
            Object(NonMatching, "enemy/em_sub_state_prog.cpp"),  # phase 4 recut of the line above (its tail), same cflags_main
            # Registered once, at its final home (docs/plan.md 12), whose range 0x8032C920..0x80334568 turned out to be
            # two TUs (`--max-bytes` had cut them in one).  FIRST TU 0x8032C920..0x8033041C, 74
            # functions / 15100 B: the enemy work record's action band - it hands its r3 record to
            # `em_frame_check`/`em_act_ck`/`em_die_ck`/`em_after_frame_check`/`em_get_mot_no`
            # (67 sites) and every field it reads on that pointer is one `src/enemy/ENEMY_WORK.h`
            # names.  It owns the class vtable `lbl_805E04E0`, the table run `.data`
            # 0x805DFC9C..0x805E0510, extab 0x80016464..0x8001662C, extabindex
            # 0x800354FC..0x800357A8, the `.ctors` word 0x8056F3A0 -> `fn_80330128` and its
            # `.sdata2` pool half 0x8079B108..0x8079B210 (declared, not claimed - playbook 23).
            # Module `enemy` from the code and the link band; C++ because every out-of-range callee
            # is a mangled symbol.  No `__FILE__` string is reachable and the runtime dump answers
            # only `zz_` placeholders, so the file name and its own symbols come from the bodies and
            # the module's `em_<noun>_<verb>` scheme (the merger lane's naming pass, 2026-09-26).
            Object(NonMatching, "enemy/em_act_step.cpp"),
            Object(NonMatching, "enemy/em_act_step_tail.cpp"),  # phase 4 stub: tail recut of the line above, same cflags_main
        ],
    },

    {
        "lib": "sound",
        "mw_version": "Wii/1.3",
        "cflags": cflags_main,
        "progress_category": "game",
        "objects": [
                        Object(NonMatching, "sound/fn_800D7F54.cpp"),
            # Registered (125 symbols / 0x6ACC bytes) and recut in phase 4: the head is folded into
            # sound/fn_800D7F54.cpp, this row is the SE request cluster's middle, then the `MHchar` model class and the job request.
            # `sound` module (src/unsplit/sound.h is the band) and C++ (the `* __FP...` / `move__6MHcharFUs` manglings).
            Object(NonMatching, "sound/se_req.cpp"),
            Object(NonMatching, "sound/mhchar.cpp"),
            Object(NonMatching, "sound/sound_job.cpp"),
            # Registered (the 0x800E3CBC run discovery
            # proposed).  Module from the placed link-neighbour sound/fn_800E46E8.cpp; the dump's
            # prim_init_all/set_blendmode/set_zmode name the functions, not the TU (see the file header).
            Object(NonMatching, "sound/fn_800E3CBC.cpp"),
            Object(NonMatching, "sound/fn_800E46E8.cpp"),
            Object(NonMatching, "sound/sound_obj.cpp"),
            Object(NonMatching, "sound/snd_stream_reloc.cpp"),
            Object(NonMatching, "sound/snd_level_tbl.cpp"),
            # Registered (a 0x800E8E60 run discovery proposed).
            # The quest/challenge sound work system: 149 functions / 0x6978 bytes.  C++ from the range's
            # own mangled symbols; no `__FILE__` string survives, so the map's stem is the file name.
            Object(NonMatching, "sound/fn_800E8E60.cpp"),
            Object(NonMatching, "sound/snd_stream_mgr.cpp"),
            Object(NonMatching, "sound/snd_voice_pool.cpp"),
            Object(NonMatching, "sound/quest_snd.cpp"),
            Object(NonMatching, "sound/snd_bank_loader.cpp"),
            # Registered (a 0x800F2A94 run discovery proposed,
            # 88 functions / 27408 B).  Same lib and same cflags as its neighbours: the range continues the
            # sound band up to the effect (eft_control) block at 0x800F6520.  Flags are this lib's
            # cflags_main; the per-symbol measurements are in the worker's outbox.
            Object(NonMatching, "sound/fn_800F2A94.cpp"),
        ],
    },

    {
        "lib": "ef",
        "mw_version": "Wii/1.3",
        "cflags": cflags_main,
        "progress_category": "game",
        "objects": [
            # Registered once, at its final home (docs/plan.md 12): the `eft052` effect family (its `_EFT` tag is 52)
            # and the cockpit item-page band it draws from (`.text` 0x80358624..0x8035E034, 92
            # functions / 23056 B).  Module `ef` (brief class 3): the range is an `eft` family plus
            # the cockpit hold/item layer in exactly the shape the registered `ef/eft050.cpp`
            # documents, and its callers are the `ef` and `lobby` bands (`ef/ef_emitter.cpp`,
            # `ef/eft050.cpp`, `lobby/fn_801E7530.cpp`, `lobby/fn_801EC9F8.cpp`).  No `__FILE__`
            # string is reachable (every `lbl_` reference resolves to the float pool, the `.data`
            # run or a call) and the runtime dump answers only `zz_` placeholders, so the name is
            # DERIVED from the tag `eft052_set` seeds `_EFT::field_0x03` with - the file name of
            # every registered sibling the dump knows (`eft001`, `eft002`, `eft009`, `eft019`,
            # `eft035`, `eft050`) - and stays a guess the unit header records.  Every symbol this
            # file defines is named from its own body; the `rule 7 deferred` line in its header
            # covers only references to OTHER units' unrenamed symbols.  C++ (`GetItemData__FUs`,
            # `LbStr__FUcUs`, `calcDistanceSqXZ__FP...`, plus the class whose constructor
            # `fn_8035BBE8` installs the vtable `lbl_805ED808`); every plain `fn_` definition is
            # `extern "C"`.  Only `.text` is claimed - the `.data` run 0x805ED0C0..0x805ED938, the
            # `.sdata2` pool 0x8079B640..0x8079B704 and the extab/extabindex records stay
            # unclaimed (the pooled constants and tables are declared, never defined).
            Object(NonMatching, "ef/eft052.cpp"),
            # Registered once, at its final home (docs/plan.md 12).  The `ef/ef_util.cpp`
            # range (`.text` 0x8009B374..0x8009CD64, 16 functions / 6640 B): the NW4R effect library's
            # shared math/utility file.  The range's own `__FILE__` string (`ef_util.cpp` at 0x80591948,
            # read out of orig/RMHE08/sys/main.dol) names the TU - see the unit's file header.  Sections:
            # extab 0x80009A38..0x80009A98, extabindex 0x80022B30..0x80022BC0, .text 0x8009B374..0x8009CD64.
            # The lib is `ef` (the file's own module); its cflags_main is token-identical to the
            # neighbouring g3d/g3d_gpu.cpp's cflags_g3d, and dtk links by address, so the lib choice
            # cannot move the object or change its codegen.
            Object(NonMatching, "ef/ef_util.cpp"),
            # Registered once, at its final home (docs/plan.md 12).  The
            # `ef/ef_animcurve.cpp` range (0x8009CDBC..0x800A3044, 26 functions): the
            # effect library's key-frame animation curve.  The range's own `__FILE__` string
            # (`ef_animcurve.cpp` at 0x80591E68) names the TU - see the unit's file header.  Sections:
            # extab 0x80009A98..0x80009B40, extabindex 0x80022BC0..0x80022CBC,
            # .text 0x8009CDBC..0x800A3044.
            Object(NonMatching, "ef/ef_animcurve.cpp"),
            # Registered once, at its final home (docs/plan.md 12).  The
            # `ef/ef_creationqueue.cpp` range (0x800A3044..0x800A388C, 7 functions): the NW4R
            # effect library's creation queue.  The range's own `__FILE__` string (`ef_creationqueue.cpp`
            # at 0x805922C0) names the TU - see the unit's file header.
            Object(NonMatching, "ef/ef_creationqueue.cpp"),
            # Registered once, at its final home (docs/plan.md 12).  The
            # `ef/ef_draworder.cpp` range (0x800A388C..0x800A40F4, 9 functions): the NW4R
            # effect library's draw-order helpers.  The range's own `__FILE__` string
            # (`ef_draworder.cpp` at 0x805923A0) names the TU - see the unit's file header.  Sections:
            # extab 0x80009B60..0x80009BA0, extabindex 0x80022CEC..0x80022D4C,
            # .text 0x800A388C..0x800A40F4.
            Object(NonMatching, "ef/ef_draworder.cpp"),
            # Registered once, at its final home (docs/plan.md 12).  The
            # `ef/ef_effect.cpp` range (0x800A40F4..0x800A56B0, 39 functions): the NW4R
            # effect library's `nw4r::ef::Effect` object (its table lbl_80592588, the create/retire
            # paths, the emitter sweeps and the EffectSystem constructor/destructor).  The range's own
            # `__FILE__` string (`ef_effect.cpp` at 0x80592430) names the TU - see the unit's file
            # header.  Sections: extab 0x80009BA0..0x80009C38, extabindex 0x80022D4C..0x80022E30,
            # .text 0x800A40F4..0x800A56B0.
            Object(NonMatching, "ef/ef_effect.cpp"),
            # Registered once, at its final home (docs/plan.md 12).  The
            # `ef/ef_effectsystem.cpp` range (0x800A56B0..0x800A6350, 23 functions): the game side
            # of the NW4R effect library's system object.  The range's own `__FILE__` string
            # (`ef_effectsystem.cpp` at 0x80592698) names the TU - see the unit's file header.  Sections:
            # extab 0x80009C38..0x80009CD4, extabindex 0x80022E30..0x80022EE4,
            # .text 0x800A56B0..0x800A6350, .ctors 0x8056F2D8..0x8056F2DC.
            Object(NonMatching, "ef/ef_effectsystem.cpp"),
                        Object(NonMatching, "ef/ef_drawstripestrategy.cpp"),
            Object(NonMatching, "ef/ef_particlemanager.cpp"),
            # Registered once, at its final home (docs/plan.md 12).  The `ef/fn_800AEE48.cpp`
            # range, recut in phase 4: this row keeps the first two TUs (ef_postfield.cpp, ef_resource.cpp) of `nw4r::ef`; the
            # ef_drawstripestrategy.cpp head is `ef/ef_drawstripestrategy.cpp` (its row is above, by file order of the old registration).
            # C++ from the `.cpp` __FILE__ strings and Panic__Q24nw4r2dbFPCciPCce.
            Object(NonMatching, "ef/fn_800AEE48.cpp"),
            Object(NonMatching, "ef/ef_drawbillboardstrategy.cpp"),
                        Object(NonMatching, "ef/ef_drawpointstrategy.cpp"),
                        Object(NonMatching, "ef/ef_drawlinestrategy.cpp"),
                        Object(NonMatching, "ef/ef_drawsmoothstripestrategy.cpp"),
            Object(NonMatching, "ef/ef_drawstrategyimpl.cpp"),
            Object(NonMatching, "ef/ef_drawfreestrategy.cpp"),
            # Registered (a 0x800C9540 run discovery proposed).
            Object(NonMatching, "ef/ef_torus.cpp"),
            Object(NonMatching, "ef/ef_cube.cpp"),
            Object(NonMatching, "ef/ef_cylinder.cpp"),
            Object(NonMatching, "ef/ef_disc.cpp"),
            Object(NonMatching, "ef/ef_emitterform.cpp"),
            # Registered once, at its final home (docs/plan.md 12).  `proposal/800A6350_fn_800A6350`:
            # the range's own `__FILE__` string is "ef_emitter.cpp" (0x80592850, read from the DOL),
            # it is C++ (the .cpp suffix, the Panic__Q24nw4r2dbFPCciPCce callees and the range's own
            # vtable at 0x80592BB0), and both seams are proven - the run starts where
            # ef/ef_effectsystem.cpp ends and stops at ef/ef_emitterform.cpp's first instruction
            # (0x800A99B4).  Sections: extab 0x80009CD4..0x80009DF4, extabindex
            # 0x80022EE4..0x80023094, .text 0x800A6350..0x800A99B4.
            Object(NonMatching, "ef/ef_emitter.cpp"),
            Object(NonMatching, "ef/ef_particle.cpp"),
            Object(NonMatching, "ef/ef_emform.cpp"),
            Object(NonMatching, "ef/ef_line.cpp"),
            Object(NonMatching, "ef/ef_point.cpp"),
            # Carved out of the 0x800CDB2C proposal range on 2026-09-30: the `ef_sphere.cpp` TU (one function; see the unit header).
            Object(NonMatching, "ef/ef_sphere.cpp"),
            # Registered (a 0x800CDB2C run discovery proposed at a --max-bytes cap, recut
            # 2026-09-30 and renamed in phase 4) - the game-system core; see the unit's file header.
            Object(NonMatching, "ef/system_core.cpp"),
            Object(NonMatching, "ef/eft001.cpp"),
            # Registered once, at its final home (docs/plan.md 12).  The
            # `ef/eft_res.cpp` range (0x800F6520..0x800F95A4, 43 functions): the game's
            # eft resource manager (eft_control, the 256-slot proID table, the load/create path and the
            # effect-heap push).  Module `ef` from both bracketing units; the name follows the siblings'
            # scheme - see the unit's file header for the evidence.
            Object(NonMatching, "ef/eft_res.cpp"),
            # Registered once, at its final home (docs/plan.md 12).  The
            # `ef/effect.cpp` range: the game's `effect.cpp` manager, named from the
            # range's own `__FILE__` string (0x8059B5D0, cited by fn_800F9884's Panic).
            Object(NonMatching, "ef/effect.cpp"),
            Object(NonMatching, "ef/eft_model_slot.cpp"),
            Object(NonMatching, "ef/eft002.cpp"),
            Object(NonMatching, "ef/fn_800FD520.c"),
            Object(NonMatching, "ef/fn_800FD718.c"),
            Object(NonMatching, "ef/fn_800FD864_fx.cpp"),
            Object(NonMatching, "ef/eft004_fx.cpp"),
            Object(NonMatching, "ef/fn_80101DF4.cpp"),
            Object(NonMatching, "ef/em_effect_ctrl.cpp"),
            Object(NonMatching, "ef/eft007.cpp"),
            Object(NonMatching, "ef/eft009.cpp"),
            Object(NonMatching, "ef/fn_80104BD0.c"),
            # Registered once, at its final home (docs/plan.md 12).  ef/fn_80105314.cpp:
            # a maximal unclaimed run, seam unproven; class 4 decided the name (the map's
            # fn_80105314 stem, the scheme the bracketing fn_80104BD0/fn_8010D1A8 units use) and
            # the range holds several original effect families - see the unit's file header.
            Object(NonMatching, "ef/fn_80105314.cpp"),
            Object(NonMatching, "ef/eft013_fx.cpp"),
            # Registered (a 0x8010BDE4 run discovery proposed).
            # The range's own symbols are plain fn_XXXXXXXX (rule 7 deferred); it is built as C++
            # because every callee it reaches is a C++ mangling (rule 9) - see the unit's header.
            Object(NonMatching, "ef/fn_8010BDE4.cpp"),
            Object(NonMatching, "ef/fn_8010D1A8.c"),
            # Registered once, at its final home (docs/plan.md 12).  The
            # `ef/eft019.cpp` range: named from the runtime
            # dump's own `eft019_set` (dumpmap.py), C++ from its mangled definition.
            Object(NonMatching, "ef/eft019.cpp"),
            Object(NonMatching, "ef/fn_80114E34.cpp"),
            Object(NonMatching, "ef/eft022_fx.cpp"),
            Object(NonMatching, "ef/fn_8011722C.c"),
            # Registered once, at its final home (docs/plan.md 12).  The
            # `ef/fn_801173AC.cpp` range (`.text` 0x801173AC..0x80119C44, 30 functions
            # / 10392 B): the tail of the eft024 job machine, the whole eft025 player family, the whole
            # eft026 enemy family and the head of eft028.  Two of the range's own definitions are
            # manglings (`eft026_set__FP4_PLWUcUlUl`, `eft028_set_koware__FUcPQ34nw4r4math4VEC3Ucl`), so
            # it is built as C++ and every plain `fn_XXXXXXXX` definition is `extern "C"`.  Sections:
            # extab 0x8000C49C..0x8000C554, extabindex 0x800265D4..0x800266E8,
            # .text 0x801173AC..0x80119C44.
            Object(NonMatching, "ef/fn_801173AC.cpp"),
            Object(NonMatching, "ef/eft026_fx.cpp"),
            Object(NonMatching, "ef/fn_80119C44.c"),
            # Registered once, at its final home (docs/plan.md 12).  The `ef/eft029.cpp`
            # range, at the TU-bounded 0x80119DEC..0x8011D448 the attribution queue carries: the runtime
            # dump's own `eft029_set_scale` / `eft029_set_kaihou` name the TU (dumpmap.py); C++ from
            # their mangled definitions.  See the unit's file header for the stale-brief record.
            Object(NonMatching, "ef/eft029.cpp"),
            Object(NonMatching, "ef/eft029_fx.cpp"),
            # Registered once, at its final home (docs/plan.md 12).  The
            # `proposal/802F140C_fn_802F140C` range (`.text` 0x802F140C..0x802F5138, 39 functions /
            # 15660 B): the runtime dump's own `eft035_set`/`eft035_set2` name the TU (dumpmap.py;
            # every other address is the dump's `zz_XXXXXXXX_` placeholder), so the module is `ef`
            # and the file follows the `eft00X.cpp` scheme of the neighbours.  Sections: extab
            # 0x80015424..0x80015514, extabindex 0x80033C9C..0x80033E04, .text
            # 0x802F140C..0x802F5138 - exactly the bytes the bracketing units leave unclaimed.  The
            # seam is unproven (one maximal unclaimed run); see the unit's file header for the
            # two-cluster evidence.
            Object(NonMatching, "ef/eft035.cpp"),
            # Registered once, at its final home (docs/plan.md 12).  The
            # `ef/eft050.cpp` range (`.text` 0x8033F270..0x803432B4, 46 functions /
            # 16452 B): the runtime dump's own `eft050_set` at 0x80342F34 names the TU (dumpmap.py;
            # every other address is the dump's `zz_XXXXXXXX_` placeholder), so the module is `ef`
            # and the file follows the `eft00X.cpp` scheme of the neighbours.  Sections: extab
            # 0x80016CA4..0x80016DB4, extabindex 0x8003615C..0x800362F4, .text
            # 0x8033F270..0x803432B4 - exactly the bytes the bracketing units leave unclaimed.  The
            # seam is unproven (one maximal unclaimed run); see the unit's file header.
            Object(NonMatching, "ef/eft050.cpp"),
            Object(NonMatching, "ef/fn_803066F0.c"),
            # Registered once, at its final home (docs/plan.md 12), and re-drawn by the seam round (`.text`
            # 0x8030681C..0x80308FB4, 32 functions / 10136 B): the eft041/042 effect machine, whose
            # first body drives fn_803066F0's `_EFT` record and which defines `eft042_set2`.  The
            # `menu_infomation.cpp` `__FILE__` string's referrer set puts the seam at 0x80308FB4 (the
            # screen bodies above it belong to `menu/menu_infomation.cpp`), so this range keeps only
            # the two screen entry points below it (`fn_80308EC0`/`fn_80308F1C`).  Class 4 decided
            # the name (the map's own fn_8030681C stem) and class 2 the module (`ef`, the left
            # bracket).  C++; every plain `fn_` definition is `extern "C"`.  Sections: extab
            # 0x80015AB4..0x80015B54 (20 records), extabindex 0x80034674..0x80034764 (20 x 12 B).
            Object(NonMatching, "ef/fn_8030681C.cpp"),
            # ef/eft_slot.cpp: the `_EFT` family at `.text` 0x803432B4..0x80349DD8
            # (92 functions / 0x6B24 bytes).  Module `ef` from the code: the range's `self` is the
            # 0x48-byte `_EFT` field for field (`flag_0x01`, `state_0x05`, `field_0x06`,
            # `timer_0x0C`, `pos_0x18`, `work_0x38`, `area_0x44` - the `src/ef.h` layout), it
            # spawns models through `ef/eft_res.cpp`'s `res_eft_model_create` and gates on
            # `eft_control`; the sibling units are `ef/eft035.cpp`/`ef/fn_803066F0.c`.  The file is
            # `ef/eft_slot.cpp`: the range is the family's 10-entry slot pool and the enemy-record
            # scan that drives it (`eft_slot_spawn`, `eft_slot_work_update`), and every one of the 34
            # symbols it defines is named from its own body, the unit header's NAMES section carrying
            # the evidence - no `__FILE__` string is reachable from the range and `dumpmap.py lookup`
            # answers only `zz_` placeholders, so the names are guesses a later pass may refine.
            # Sections: `.text` 0x803432B4..0x80349DD8, extab 0x80016DB4..0x80016FE4, extabindex
            # 0x800362F4..0x8003663C, `.ctors` 0x8056F3A4, `.data` 0x805E9168..0x805E91E8.  C++;
            # every plain `fn_` definition is `extern "C"`.
            Object(NonMatching, "ef/eft_slot.cpp"),
            # Registered, whose 0x80366618..0x8036CF64 range is a
            # discovery `--max-bytes` cut.  `tudiscover.py at 0x80366618` returns a 15-function MATCH SET,
            # 0x80366618..0x8036A690, from two must-link `lbl_8079B744` anchors; the range's private
            # `.sdata2` run (0x8079B740..0x8079B820, no leak) ends at its last referrer `fn_80369D50`,
            # and the next run's first referrer is `fn_8036E320`, so the TU stops at 0x8036A690 and the
            # 0x8036A690..0x8036CF64 tail stays unclaimed (seam re-draw, see the unit's file header).
            # Sections: extab 0x800177D4..0x80017844, extabindex 0x80037224..0x800372CC,
            # .text 0x80366618..0x8036A690.  The runtime dump's own `eft053_get_shell_data` /
            # `eft053_get_model_ang` name the TU (dumpmap.py), so the module is `ef`; same
            # `cflags_main` as the two sibling units in this block.
            Object(NonMatching, "ef/eft053.cpp"),
        ],
    },

    {
        "lib": "gx",
        "mw_version": "Wii/1.3",
        "cflags": cflags_main,
        "progress_category": "game",
        "objects": [
                        Object(Matching, "gx/fn_8009AA78.c"),
                        Object(NonMatching, "gx/fn_8009ACE4.c"),
        ],
    },

    {
        "lib": "auto",
        "mw_version": "Wii/1.3",
        "cflags": cflags_main,
        "progress_category": "auto",
        "objects": [
        ],
    },
    {
        # The bootstrap pair, whose retail .init regions carry 16-byte zero padding between functions
        # (__start 0x300 vs 0x2E0, __ppc_eabi_init 0x64 vs 0x58 with 4-byte alignment). Same compiler and
        # version as the lib below; only the function alignment differs, so it needs its own cflags group.
        "lib": "Runtime.PPCEABI.H/init",
        "mw_version": "Wii/1.3",
        "cflags": cflags_ppceabi16,
        "progress_category": "sdk",
        "host": False,
        "objects": [
            Object(Matching, "Runtime.PPCEABI.H/TRK_interrupt_vectors.c"),
            Object(Matching, "Runtime.PPCEABI.H/TRK_interrupt_vector_stubs.c"),
            Object(Matching, "Runtime.PPCEABI.H/__start.c"),
            Object(Matching, "Runtime.PPCEABI.H/__ppc_eabi_init.cpp"),
        ],
    },
    {
        "lib": "Runtime.PPCEABI.H",
        "mw_version": "Wii/1.3",
        "cflags": cflags_ppceabi,
        "progress_category": "sdk",  # str | List[str]
        "objects": [
            # Phase 4 stubs: MSL/runtime candidate units with no bodies yet.
            Object(NonMatching, "MSL/strlen.cpp"),
            Object(NonMatching, "Runtime.PPCEABI.H/__va_arg.cpp"),
            Object(NonMatching, "Runtime.PPCEABI.H/global_destructor_chain.c"),
            Object(NonMatching, "Runtime.PPCEABI.H/CPlusLibPPC.cpp"),
            Object(NonMatching, "Runtime.PPCEABI.H/runtime.cpp"),
            Object(NonMatching, "MSL_C/alloc.cpp"),
            # Phase 4 stubs (window fg): the MSL libm (fdlibm) units after MSL_C/alloc.
            Object(NonMatching, "MSL/e_asin.cpp"),
            Object(NonMatching, "MSL/e_atan2.cpp"),
            Object(NonMatching, "MSL/e_fmod.cpp"),
            Object(NonMatching, "MSL/e_log.cpp"),
            Object(NonMatching, "MSL/e_log10.cpp"),
            Object(NonMatching, "MSL/e_pow.cpp"),
            Object(NonMatching, "MSL/e_rem_pio2.cpp"),
            Object(NonMatching, "MSL/k_cos.cpp"),
            Object(NonMatching, "MSL/k_rem_pio2.cpp"),
            Object(NonMatching, "MSL/k_sin.cpp"),
            Object(NonMatching, "MSL/k_tan.cpp"),
            Object(NonMatching, "MSL/s_atan.cpp"),
            Object(NonMatching, "MSL/s_ceil.cpp"),
            Object(NonMatching, "MSL/s_copysign.cpp"),
            Object(NonMatching, "MSL/s_cos.cpp"),
            Object(NonMatching, "MSL/s_floor.cpp"),
            Object(NonMatching, "MSL/s_frexp.cpp"),
            Object(NonMatching, "MSL/s_ldexp.cpp"),
            Object(NonMatching, "MSL/s_modf.cpp"),
            Object(NonMatching, "MSL/s_sin.cpp"),
            Object(NonMatching, "Runtime.PPCEABI.H/ptmf.c"),
            # Metrowerks' Gecko exception runtime, with the SDK's own extension (.cp, resolved as C++).
            Object(NonMatching, "Runtime.PPCEABI.H/Gecko_ExceptionPPC.cp"),
            Object(Matching, "Runtime.PPCEABI.H/__init_cpp_exceptions.cpp"),
            # The .init runtime, in the order the retail section lays it out. memcpy.c is the provisional
            # half of MSL's __mem.o (the other half is memset.c, already at 100 %); making them one file
            # is proposed in the escalation queue - see the unit's file header comment.
            Object(Matching, "Runtime.PPCEABI.H/memcpy.c"),
            # First flip (docs/plan.md 7.6): the object is byte-identical, so its bytes now come from src/.
            Object(Matching, "Runtime.PPCEABI.H/memset.c"),
        ],
    },
    {
        "lib": "Camellia",
        "mw_version": "Wii/1.3",
        "cflags": cflags_camellia,
        "host": False,
        "objects": [
            # Not a match yet: 9 of the 10 functions are byte-identical, camellia_setup256 still has
            # one extra 4-byte stack slot (see the file header comment in src/Camellia/camellia.c).
            # NonMatching keeps the original bytes in the link, so the DOL hash is unaffected by it.
            Object(NonMatching, "Camellia/camellia.c"),
        ],
    },
    {
        "lib": "RSO",
        # GC/3.0a3, not a Wii compiler: the retail object's fn_804DAA24 is 460 B / 115 instructions
        # and the GC 3.0a3-3.0a5.2 family reproduces that opcode sequence exactly (only register
        # colours left, 99.30 %), while every installed Wii compiler (0x4201_127, 1.0RC1, 1.0a, 1.0,
        # 1.1, 1.3, 1.5, 1.6, 1.7) emits one extra `lwz` in the R_PPC_REL24 case (116 insns, 97.30 %).
        # The older GC compilers (1.0-2.7) are far worse (53-93 %), so it is specifically GC 3.0a3+.
        # 3.0a3 / 3.0a5 / 3.0a5.2 are codegen-identical on every function reconstructed so far; the
        # tiebreak is the comment version (config.yml mw_comment_version: 14 == 3.0a3's 0e byte,
        # 3.0a5.2 and the Wii compilers emit 0f) - that is a config value, not retail evidence, since
        # the DOL carries no .comment section at all.
        # Method: playbook 17 (cross-family version matrix).
        "mw_version": "GC/3.0a3",
        "cflags": cflags_rso,
        "host": False,
        "objects": [
            Object(NonMatching, "RSO/runtime.c"),
        ],
    },
{
        "lib": "Pl",
        # Wii/1.0 (mwcc 4.3 build 145): game code. Each unit's evidence is its file header under src/Pl/.
        "mw_version": "Wii/1.0",
        "cflags": cflags_pl,
        "progress_category": "game",
        "host": False,
        "objects": [
            Object(Matching, "Pl/fn_80230FBC.cpp"),
            Object(NonMatching, "Pl/fn_802373AC.cpp"),
            Object(Matching, "Pl/fn_80229ECC.cpp"),
            Object(Matching, "Pl/fn_8023C2D0.cpp"),
            Object(Matching, "Pl/fn_80241558.cpp"),
            Object(NonMatching, "Pl/player_control.cpp"),
            Object(NonMatching, "Pl/pl_act_step.cpp"),
            Object(NonMatching, "Pl/pl_coll.cpp"),
            Object(NonMatching, "Pl/pl_act.cpp"),
            Object(NonMatching, "Pl/pl_act_step_data.cpp"),
            Object(NonMatching, "Pl/fn_802840DC.cpp"),
            Object(NonMatching, "Pl/pl_motion.cpp"),
            Object(NonMatching, "Pl/pl_hit_sphere.cpp"),
            Object(NonMatching, "Pl/pl_act_class3.cpp"),
            Object(NonMatching, "Pl/pl_yure.cpp"),
        ],
    },
    {
        "lib": "g3d",
        "mw_version": "Wii/1.3",
        "cflags": cflags_g3d,
        "host": False,
        "objects": [
            Object(Matching, "g3d/g3d_anmscn.cpp"),
            Object(NonMatching, "g3d/g3d_anmchr.cpp"),
            Object(NonMatching, "g3d/fn_800680CC.cpp"),
            Object(NonMatching, "g3d/g3d_calcmaterial.cpp"),
            Object(NonMatching, "g3d/g3d_calcview.cpp"),
            Object(NonMatching, "g3d/g3d_anmvis.cpp"),
            Object(NonMatching, "g3d/g3d_calcvtx.cpp"),
            Object(NonMatching, "g3d/g3d_calcworld.cpp"),
            Object(NonMatching, "g3d/g3d_camera.cpp"),
            Object(NonMatching, "g3d/g3d_state.cpp"),
            Object(NonMatching, "g3d/g3d_resvtx.cpp"),
            Object(NonMatching, "g3d/g3d_resanm.c"),
            Object(NonMatching, "g3d/g3d_resanmamblight.c"),
            Object(NonMatching, "g3d/g3d_resanmcamera.cpp"),
            Object(NonMatching, "g3d/g3d_resanmfog.cpp"),
            Object(NonMatching, "g3d/g3d_resanmlight.cpp"),
            Object(NonMatching, "g3d/g3d_resanmscn.cpp"),
            Object(Matching, "g3d/g3d_resfile.cpp"),
            Object(NonMatching, "g3d/g3d_resmat.cpp"),
            Object(NonMatching, "g3d/g3d_resnode.cpp"),
            Object(NonMatching, "g3d/g3d_resshp.cpp"),
            Object(NonMatching, "g3d/g3d_resanmchr.cpp"),
            Object(NonMatching, "g3d/g3d_resanmtexsrt.cpp"),
            Object(NonMatching, "g3d/g3d_xsi.cpp"),
            Object(NonMatching, "g3d/fn_800D77B0.cpp"),
            Object(NonMatching, "g3d/g3d_basic.cpp"),
            Object(NonMatching, "g3d/fn_80063888.cpp"),
            Object(NonMatching, "g3d/fn_8005AA28.cpp"),
            Object(NonMatching, "g3d/fn_80075DCC.cpp"),
            Object(NonMatching, "g3d/g3d_scnmdl.cpp"),
            Object(NonMatching, "g3d/g3d_scnmdlsmpl.cpp"),
            Object(NonMatching, "g3d/g3d_scnobj.cpp"),
            Object(NonMatching, "g3d/g3d_scnroot.cpp"),
            Object(NonMatching, "g3d/g3d_cpu.cpp"),
            Object(NonMatching, "g3d/g3d_gpu.cpp"),
        ],
    },
    {
        "lib": "Network",
        "mw_version": "Wii/1.3",
        "cflags": cflags_network,
        "host": False,
        "objects": [
            # Flags: unit header of src/Network/NetworkLayerPat.cpp; measurements in docs/network.md.
            Object(NonMatching, "Network/NetworkLayerPat.cpp",
                   cflags=[f for f in cflags_network if f not in ("-O4,p", "-inline auto")] + ["-O3", "-inline noauto"]),
            # Flags: unit header of src/Network/NetworkCommunity.cpp; measurements in docs/network.md.
            Object(NonMatching, "Network/NetworkCommunity.cpp",
                   cflags=[f for f in cflags_network if f not in ("-O4,p", "-inline auto")] + ["-O3", "-inline noauto"]),
            # Flags: unit header of src/Network/NetworkCommunityPat.cpp; measurements in docs/network.md.
            Object(NonMatching, "Network/NetworkCommunityPat.cpp",
                   cflags=[f for f in cflags_network if f not in ("-O4,p", "-inline auto")] + ["-O3", "-inline noauto"]),
            # Flags: unit header of src/Network/NetworkFetcherBase.cpp; measurements in docs/network.md.
            Object(NonMatching, "Network/NetworkFetcherBase.cpp",
                   cflags=[f for f in cflags_network if f != "-O4,p"] + ["-O3"]),
            # Flags: unit header of src/Network/NetworkFileFetcher.cpp; measurements in docs/network.md.
            Object(NonMatching, "Network/NetworkFileFetcher.cpp",
                   cflags=[f for f in cflags_network if f != "-O4,p"] + ["-O3"]),
            # Flags: unit header of src/Network/NetworkNullFetcher.cpp; measurements in docs/network.md.
            Object(NonMatching, "Network/NetworkNullFetcher.cpp",
                   cflags=[f for f in cflags_network if f != "-O4,p"] + ["-O3"]),
            # Flags: unit header of src/Network/NetworkSocketBase.cpp; measurements in docs/network.md.
            Object(NonMatching, "Network/NetworkSocketBase.cpp",
                   cflags=[f for f in cflags_network if f != "-O4,p"] + ["-O3"]),
            # Flags: unit header of src/Network/NetworkSocketWii.cpp; measurements in docs/network.md.
            Object(NonMatching, "Network/NetworkSocketWii.cpp",
                   cflags=[f for f in cflags_network if f != "-O4,p"] + ["-O3"]),
            # Flags: unit header of src/Network/NetworkUniqueId.cpp; measurements in docs/network.md.
            Object(NonMatching, "Network/NetworkUniqueId.cpp",
                   cflags=[f for f in cflags_network if f != "-O4,p"] + ["-O3", "-pool off"]),
            # Flags: unit header of src/Network/NetworkUnitPacket.cpp; measurements in docs/network.md.
            Object(NonMatching, "Network/NetworkUnitPacket.cpp",
                   cflags=[f for f in cflags_network if f != "-O4,p"] + ["-O3"]),
            # Flags: unit header of src/Network/PatConnection.cpp; measurements in docs/network.md.
            Object(NonMatching, "Network/PatConnection.cpp"),
            # Flags: unit header of src/Network/PatInterface.cpp; measurements in docs/network.md.
            Object(NonMatching, "Network/PatInterface.cpp",
                   cflags=[f for f in cflags_network if f not in ("-O4,p", "-inline auto")] + ["-O3", "-inline noauto"]),
            # Flags: unit header of src/Network/NetworkPool.cpp; measurements in docs/network.md.
            Object(NonMatching, "Network/NetworkPool.cpp",
                   cflags=[f for f in cflags_network if f not in ("-O4,p", "-inline auto")] + ["-O3", "-inline noauto"]),
            # Flags: unit header of src/Network/NetworkWiiMediator.cpp; measurements in docs/network.md.
            Object(NonMatching, "Network/NetworkWiiMediator.cpp",
                   cflags=[f for f in cflags_network if f not in ("-O4,p", "-inline auto")] + ["-O3", "-inline noauto"]),
            # Flags: unit header of src/Network/sNetworkLibrary.cpp; measurements in docs/network.md.
            Object(NonMatching, "Network/sNetworkLibrary.cpp",
                   cflags=[f for f in cflags_network if f != "-O4,p"] + ["-O3"]),
            # Flags: unit header of src/Network/sNetworkLibraryWii.cpp; measurements in docs/network.md.
            Object(NonMatching, "Network/sNetworkLibraryWii.cpp",
                   cflags=[f for f in cflags_network if f not in ("-O4,p", "-inline auto")] + ["-O3", "-inline noauto"]),
            # Flags: unit header of src/Network/NetworkPat.cpp; measurements in docs/network.md.
            Object(Matching, "Network/NetworkPat.cpp",
                   cflags=[f for f in cflags_network if f not in ("-O4,p", "-inline auto")] + ["-O3", "-inline noauto"]),
            # Flags: unit header of src/Network/NetworkReflectService.cpp; measurements in docs/network.md.
            Object(NonMatching, "Network/NetworkReflectService.cpp",
                   cflags=[f for f in cflags_network if f not in ("-O4,p", "-inline auto")] + ["-O3", "-inline noauto"]),
            # Flags: unit header of src/Network/NetworkSessionManager.cpp; measurements in docs/network.md.
            Object(NonMatching, "Network/NetworkSessionManager.cpp",
                   cflags=[f for f in cflags_network if f != "-O4,p"] + ["-O3"]),
            # Flags: unit header of src/Network/NetworkPeerBase.cpp; measurements in docs/network.md.
            Object(Matching, "Network/NetworkPeerBase.cpp",
                   cflags=[f for f in cflags_network if f != "-O4,p"] + ["-O3", "-pool off"]),
            # Flags: unit header of src/Network/NetworkPeerBuffer.cpp; measurements in docs/network.md.
            Object(Matching, "Network/NetworkPeerBuffer.cpp",
                   cflags=[f for f in cflags_network if f != "-O4,p"] + ["-O3", "-pool off"]),
            # Flags: unit header of src/Network/NetworkPeerUdp.cpp; measurements in docs/network.md.
            Object(Matching, "Network/NetworkPeerUdp.cpp",
                   cflags=[f for f in cflags_network if f != "-O4,p"] + ["-O3", "-pool off"]),
            # Flags: unit header of src/Network/NetworkPeerMcs.cpp; measurements in docs/network.md.
            Object(NonMatching, "Network/NetworkPeerMcs.cpp",
                   cflags=[f for f in cflags_network if f != "-O4,p"] + ["-O3", "-pool off"]),
            # Flags: unit header of src/Network/network_socket_streams.cpp; measurements in docs/network.md.
            Object(NonMatching, "Network/network_socket_streams.cpp",
                   cflags=[f for f in cflags_network if f != "-O4,p"] + ["-O3", "-pool off"]),
            # Flags: unit header of src/Network/NetworkResolverWii.cpp; measurements in docs/network.md.
            Object(NonMatching, "Network/NetworkResolverWii.cpp",
                   cflags=[f for f in cflags_network if f != "-O4,p"] + ["-O3", "-pool off"]),
            # Flags: unit header of src/Network/NetworkSessionBase.cpp; measurements in docs/network.md.
            Object(Matching, "Network/NetworkSessionBase.cpp",
                   cflags=[f for f in cflags_network if f != "-O4,p"] + ["-O3", "-pool off"]),
            # Flags: unit header of src/Network/NetworkSessionStable.cpp; measurements in docs/network.md.
            Object(NonMatching, "Network/NetworkSessionStable.cpp",
                   cflags=[f for f in cflags_network if f != "-O4,p"] + ["-O3", "-pool off"]),
            # Flags: unit header of src/Network/network_shared_data.cpp; measurements in docs/network.md.
            Object(NonMatching, "Network/network_shared_data.cpp"),
            # Flags: unit header of src/Network/GameSpyInterfaceThread.cpp; measurements in docs/network.md.
            Object(NonMatching, "Network/GameSpyInterfaceThread.cpp",
                   cflags=[f for f in cflags_network if f not in ("-O4,p", "-inline auto")] + ["-O3", "-inline noauto", "-pool off"]),
        ],
    },
    {
        # The Nintendo Wi-Fi Connection (DWCi) SDK block of the retail link order: the only named
        # symbol of the block is `DWCi_Np_CPUCopyFast` (0x80507C40) and the next block up is the
        # NHTTP library (`NHTTPi_alloc`, 0x805145E8).  Registered once, at its final home (docs/plan.md 12):
        # proposal `805113B0_fn_805113B0.c` (`.text` 0x805113B0..0x805124F4, 21 functions / 4420 B).
        # Module `DWCi` from the library block the range's every callee lives in (0x8050A..0x8050E);
        # no `__FILE__` string covers the range and the dump answers only `zz_` placeholders, so the
        # file keeps the map's stem (brief section 2, class 3+4; see the unit header).  Sections: .text
        # only - the `.data`/`.bss` objects the range loads (lbl_8060EDB0, lbl_80795820, lbl_807625C0)
        # are not claimed here.
        "lib": "DWCi",
        "mw_version": "Wii/1.3",
        "cflags": cflags_dwc,
        "host": False,
        "objects": [
            # The DWCi SDK band head: the named `DWCi_Np_CPUCopyFast` (0x80507C40) and its 14
            # neighbours (.text 0x80507C40..0x80509DB0, 15 functions / 8560 B).  Right edge is the
            # strong `.sdata` run-jump cut at 0x80509DB0 (`tudiscover.py at 0x80507C40`); the left
            # edge is the named symbol's own start.  Claims .text only.  See the file header.
            Object(NonMatching, "DWCi/dwc_error.cpp"),
            Object(NonMatching, "DWCi/DWCi_Np_CPUCopyFast.c"),
            Object(NonMatching, "DWCi/dwc_nasfunc.cpp"),
            Object(NonMatching, "DWCi/fn_805113B0.c"),
            # The DWCi band tail (.text 0x80512490..0x805145B8, 17 functions / 8488 B).  The left
            # edge is the seam the recon resolved: `tudiscover.py at 0x80512490` pins `strong x2`
            # at 0x80512490, so the registered `DWCi/fn_805113B0.c` right edge was moved down from
            # 0x805124F4.  The right edge 0x805145B8 is the 16->4 byte function-packing change - the
            # DWCi/NHTTP library boundary.  `DWCi`, not NHTTP: the range calls only the DWCi
            # transport helpers and carries the GameSpy NATNEG pool.  Claims .text only.
            # `cflags_base` without the lib's `-func_align 4`: this unit's retail functions all start
            # 16-byte aligned (17/17, `gap_*` zero runs between them - unlike the 4-byte packing of the
            # rest of the lib), and with 4 our object misses the `nop` MWCC inserts to align
            # `DWCi_natNegTickIdleSockets`'s struct-copy loop (98.48 -> 100.00, the only row that moved for
            # this reason).  `-O4,p` implies `-func_align 16`.
            Object(NonMatching, "DWCi/DWCi_NatNeg.c", cflags=cflags_base),
        ],
    },
    {
        # The Revolution SDK NHTTP (HTTP) library, `.text` 0x805145B8..0x8051B7FC (the SSL library
        # begins at 0x8051B7FC).  Registered as its own SDK lib block because it is a separate
        # prebuilt library with its own source files: the three units' own `__FILE__` pool strings
        # are `NHTTP_bgnend.c` (0x80630A28), `NHTTP_os_RVL.c` (0x80630B18) and `d_nhttp.c`
        # (0x80630F04).  Internal TU boundaries are only weakly pinned by `tudiscover` (the NHTTP
        # cuts are weak pool signals), so the split follows the real file names + per-TU `.data`
        # fragment order and is expected to be re-cut when the SDK object list is recovered.
        # `Wii/1.3` + cflags_nhttp (4-byte packing is instruction-level evidence here; the flags
        # are otherwise the sibling SDK group's).  Each unit claims .text only.
        "lib": "NHTTP",
        "mw_version": "Wii/1.3",
        "cflags": cflags_nhttp,
        "host": False,
        "objects": [
            Object(NonMatching, "NHTTP/NHTTP_bgnend.c"),
            Object(NonMatching, "NHTTP/NHTTP_os_RVL.c"),
            Object(NonMatching, "NHTTP/d_nhttp.c"),
        ],
    },
    {
        # The WiiConnect24 (NWC24) SDK library of the retail link order: the strongest cut in the
        # band is `NWC24IsMsgLibOpened` (0x8051D8B0) on its right edge, 0x8051E864 (a `.sdata` pool
        # run jump, the NWC24 -> SO library seam), and the band really starts at 0x8051D710, not at
        # the weak left cut 0x8051CDD0 (that one is inside the NCD/REX band: `.sbss` 0x80795894 is
        # referenced only by 0x8051C554-0x8051CCE0, and NETMemCpy/NETMemSet have no data refs at all).
        # The band is registered as the TWO original translation units its own data proves - the
        # `.data` literal "/dev/net/kd/request" is emitted twice with disjoint referrer sets
        # (0x80631178 <- 0x8051DB4C/0x8051DCEC/0x8051DF08, 0x80631200 <- 0x8051E6D4) and `-str reuse`
        # merges identical literals inside one TU.  See both unit headers for the full cut evidence.
        # Module `NWC24` from the library's own version string in the span (`.data` 0x80631128,
        # "<< RVL_SDK - NWC24 release build: Jun  9 2009 11:59:51 (0x4199_60831) >>", registered by
        # fn_8051D878 at 0x8051D878) and from the named NWC24* API; no `__FILE__` string covers the
        # range, so the files keep the map's stems under a rule-7 deferral.
        "lib": "NWC24",
        "mw_version": "Wii/1.3",
        "cflags": cflags_nwc24,
        "host": False,
        "objects": [
            # The WiiConnect24 (NWC24) SDK library's two translation units, as the band's own data
            # proves: the message-library half (the MsgLib state API, the scheduler pair, the
            # script-mode/user-id requests, the version registration and the /dev/net/kd/request command
            # engine) and the device/utility half (the /dev/net/kd/* fd+ioctl wrappers with the async
            # command slot, the user-id CRC/unscramble pair, the RTC pair and the shutdown pair).  Both
            # file names are descriptive guesses marked as such in the unit headers - the image carries no
            # `__FILE__` string for the range and the runtime dump answers only zz_/fn_ placeholders.
            # The seam between them, the cut evidence and the 18 fn_XXXXXXXX symbols renamed to real
            # names are documented in the two headers and in .pi/notes/net-nwc24.md.
            Object(NonMatching, "NWC24/nwc24_msg.c"),  # 0x8051D710-0x8051E068
            Object(NonMatching, "NWC24/nwc24_io.c"),   # 0x8051E068-0x8051E864
        ],
    },
    {
        "lib": "OS",
        "mw_version": "Wii/1.3",
        "cflags": cflags_os,
        "host": False,
        "objects": [
            Object(NonMatching, "NAND/nand.c"),
            # Registered once, at its final home (docs/plan.md 12): proposal
            # `804C1760_FindContainHeap_.c` (`.text` 0x804C1760..0x804C6D68, 68 functions / 22024 B).  Module
            # `OS`: the nearest registered unit in splits.txt is OS/OSAlarm.c and the mem half's foreign
            # calls are all the OS library (OSInitMutex / OSLockMutex / OSUnlockMutex).  The run is the SDK
            # low-level runtime band (`tudiscover at` finds the mtx and vec clusters as separate certain TUs
            # inside it); the seam is a byte cap, not a boundary.  Real dump names are used where the map has
            # them; the rest keep their map stem under rule 7's deferral (see the file header).
            Object(NonMatching, "OS/FindContainHeap_.c"),
            Object(NonMatching, "MTX/mtxvec.c"),
            Object(NonMatching, "MTX/mtx44.c"),
            Object(NonMatching, "MTX/vec.c"),
            # Registered once, at its final home (docs/plan.md 12): proposal `80474CB0_AXFXReverbHiInit` -
            # the Revolution SDK AXFX reverb-hi effect pair (16 functions / 0x1174 B,
            # 0x80474CB0..0x80475E24): AXFXReverbHiInit/Shutdown/Callback + AXFXReverbHiExpInit and the
            # shared __AllocDelayLine/__BzeroDelayLines/__FreeDelayLine delay-line helpers.  Module `AX`:
            # the runtime dump names the range's head AXFXReverbHi* and its neighbours are the rest of the
            # AX library (`__AXVPBInit` below, `AXFXSetHooks` above), which is a game-independent SDK
            # library with no config.libs block of its own yet.  Lib `OS` + cflags_os: the range's link
            # neighbours in the same SDK run are the OS units (OSAlarm.c at 0x804CBC50 above it) and the
            # runtime block below it is `Runtime.PPCEABI.H`; the source restores -O4,p's 16-byte function
            # alignment with `#pragma function_align 16` (every start in the range is 16-aligned).
            # The discovery seam between AXFXReverbHi.c and AXFXReverbHiExp.c was not taken: the same
            # two-file split exists in a sister SDK build (MotoGP 08), but this run is one maximal
            # unclaimed run, so it lands as one unit.  Claims .text only; the size/coefficient tables it
            # reads (lbl_80612980 / lbl_80612A40) are an unclaimed auto .data range for the data pass.
            Object(NonMatching, "AX/AXFXReverbHi.c"),
            Object(NonMatching, "AX/AXFXReverbHiExp.c"),
            Object(NonMatching, "AX/AXFXReverbStd.cpp"),
            # Registered by the BTE-region survey (`worker/bte-survey-846f`, notes
            # `.pi/notes/bte-survey-846f.md`): `OS/PPCArch.c` (`.text` 0x804770E0..0x804772F0,
            # 22 functions / 528 B, plus its `.data` string at 0x80612CE0).  Module `OS`, lib `OS`:
            # the range is the SDK's PPCArch.c verbatim - every symbol already carries its real SDK
            # name and the roster matches the SDK file's order, so rule 7 needs no invention.  It is
            # one interior TU of the 238 KB unclaimed run 0x80475E24..0x804B17D0, whose left edge
            # (AXFXGetHooks, 0x804770D4 + 12 B pad) and right edge (fn_804772F0, not an SPR
            # accessor) are both non-PPCArch code.  Same lib block as the SDK bands above; the
            # source restores -O4,p's 16-byte function alignment with `#pragma function_align 16`
            # (every start in the range is 16-aligned).
            Object(NonMatching, "OS/PPCArch.c"),
            # Registered by the BTE-region survey (`worker/bte-survey-846f`): `EXI/EXIBios.c`
            # (`.text` 0x804AFED0..0x804B17D0, 20 functions / 6400 B).  Module `EXI`, lib `OS`, same
            # block as EXI/ProbeBarnacle.c.  The range is the SDK's EXIBios.c verbatim (every symbol
            # already carries its real SDK name, in the SDK file's own order), its right edge is
            # *proven* - the registered EXI/ProbeBarnacle.c starts at exactly 0x804B17D0 - and its
            # left edge is roster-proven (WriteUARTN, the UART layer above it, calls into the range,
            # so it consumes EXIBios and is not part of it).  Claims its own `.data` Ecb, `.sdata`
            # __EXIVersion and `.sbss` IDSerialPort1; `__OSInIPL` is left unclaimed on purpose (OS
            # shared global, 16 readers in DVD + EXI - see the unit header).
            Object(NonMatching, "EXI/EXIBios.c"),
            # Registered once, at its final home (docs/plan.md 12): proposal
            # `804B17D0_ProbeBarnacle.c` (`.text` 0x804B17D0..0x804B8020, 118 functions / 26704 B).  Module
            # `EXI` from the range's head (the dump names ProbeBarnacle / __OSEnableBarnacle / EXIWriteReg,
            # the EXI library's own entry points) and the sibling SDK modules (OS/, AX/, DWCi/); the band
            # holds three SDK libraries (EXI, FS/ISFS, GX) - see the file header.  Lib `OS` + cflags_os,
            # like the AX band above, and the source restores -O4,p's 16-byte function alignment with
            # `#pragma function_align 16` (every start in the range is 16-aligned).
            Object(NonMatching, "EXI/ProbeBarnacle.c"),
            # Phase 4 stubs (window fg): SDK candidate units with no bodies yet.
            Object(NonMatching, "TRK/TRK_flush_cache.cpp"),
            Object(NonMatching, "ARC/arc.cpp"),
            Object(NonMatching, "BTE/gki_buffer.cpp"),
            Object(NonMatching, "RVLGX/GXTexture_tail.cpp"),
            Object(NonMatching, "SC/sc.cpp"),
            Object(NonMatching, "WPAD/wpad.cpp"),
            Object(NonMatching, "nw4r/math_arithmetic.cpp"),
            Object(NonMatching, "nw4r/math_triangular.cpp"),
            Object(NonMatching, "nw4r/fn_805012C4.cpp"),
            Object(NonMatching, "nw4r/fn_80502828.cpp"),
            Object(NonMatching, "nw4r/fn_80504A3C.cpp"),
            Object(NonMatching, "nw4r/fn_8050661C.cpp"),
            Object(NonMatching, "SSL/ssl.cpp"),
            Object(NonMatching, "SO/soi.cpp"),
        ],
    },
    {
        "lib": "lobby",
        "mw_version": "Wii/1.3",
        "cflags": cflags_lobby,
        "host": False,
        "objects": [
            Object(Matching, "lobby/lobby_scene.c"),
            # Registered once, at its final home (docs/plan.md 12): proposal
            # `801E0ADC_fn_801E0ADC.cpp` (`.text` 0x801E0ADC..0x801E7530, 77 functions / 27220 B) -
            # the effect/flag bookkeeping group that precedes the lobby menu layer.  Module `lobby`:
            # 23 call sites go into the registered `lobby/fn_80212810.cpp`, the rest of the foreign
            # calls are the lobby/HUD API (`LbStr`, `GetMenuFontColor`, `get_lsp_data`,
            # `draw_sprite_*`), and the range's data is the lobby `.sbss`/`.data` run
            # (`lbl_80794880`); both link neighbours are the enemy/lobby units.  No `__FILE__` string
            # covers the range and the dump answers only `zz_` placeholders, so the file keeps the map
            # stem (brief section 2, class 3+4).  C++ from the range's mangled callees.
            # Sections: extab 0x800103D4..0x800105B4 (60 records), extabindex
            # 0x8002C4A8..0x8002C778 (60 x 12 B), .text 0x801E0ADC..0x801E7530 - each is exactly the
            # gap between the bracketing enemy/lobby claims.  The seam is unproven (`--max-bytes`
            # cap) and the run holds several original TUs (see the file header).
            Object(NonMatching, "lobby/fn_801E0ADC.cpp"),
            # Registered once, at its final home (docs/plan.md 12): proposal
            # `801E7530_fn_801E7530.cpp` (`.text` 0x801E7530..0x801EC9E0, 77 functions / 21680 B) -
            # the lobby menu-layer group.  Module `lobby` from the code (`lobby_w`, `lb_param_w`,
            # `LbStr`, `GetMenuFontColor`, `draw_font_idx`) and from the neighbour above; no `__FILE__`
            # string survives and the dump answers only `zz_` placeholders, so the file keeps the map
            # stem (brief section 2, class 3+4).  Sections: extab 0x800105B4..0x8001079C (61 records),
            # extabindex 0x8002C778..0x8002CA54 (61 x 12 B), .text 0x801E7530..0x801EC9E0.
            Object(NonMatching, "lobby/fn_801E7530.cpp"),
            Object(NonMatching, "lobby/lb_pane_ui.cpp"),
            Object(NonMatching, "lobby/lb_npc.cpp"),
            # `80212810_fn_80212810.cpp` (`.text` 0x80212810..0x80219260, 105 functions / 27216 B) -
            # the lobby item/equipment page layer.  Module `lobby` from the code (`LbStr`,
            # `LbPutAnaPageArrow`, `get_lsp_data`, `draw_sprite_ary`, `GetMenuFontColor`) and from the
            # `.bss` run it reads (`lobby_w`, `lb_npc`); no `__FILE__` string survives and the dump
            # answers only `zz_` placeholders, so the file keeps the map stem (brief section 2,
            # class 3+4).  `.text` only: the naive switch tables live in a `.data` run this range only
            # partly references, so no data range is claimed yet.
            Object(NonMatching, "lobby/fn_80212810.cpp"),
            # Registered once, at its final home (docs/plan.md 12): proposal
            # `80219260_fn_80219260.cpp` (`.text` 0x80219260..0x8021E1EC, 76 functions / 20364 B) -
            # the equipment-page group between the two item/equipment page units.  Module `lobby`
            # from the band (both bracketing registered units are `lobby`) and from the code: it
            # hands the player actor `_PLW` (`self->plw_0x34`) to the `Put_equip_dtl_*` /
            # `Put_status_equip_*` page and calls the `_EQUIP` accessors (`Gunner_opt_ok_ck`,
            # `Get_equip_rare`).  C++ from the range's mangled callees.  No `__FILE__` string covers
            # the range - the band's only source-name string (`enemy_control.cpp`, 0x805A1BB8) is
            # never addressed by the range's code - and the dump answers only `zz_` placeholders, so
            # the file keeps the map stem (brief section 2, class 4).
            # Sections: extab 0x80011724..0x800118CC (53 records), extabindex
            # 0x8002E1A0..0x8002E41C (53 x 12 B), .text 0x80219260..0x8021E1EC - each run is exactly
            # the gap between the two bracketing registered claims.  `.text` + the two unwind runs
            # only: the `.data` (0x805C0xxx) and `.bss` runs this range reads are shared and leak
            # outside it, so no data range is claimed yet.
            Object(NonMatching, "lobby/fn_80219260.cpp"),
            Object(NonMatching, "lobby/lb_menu_scratch.cpp"),
            Object(NonMatching, "lobby/lb_menu_pos_tbl.cpp"),
            Object(NonMatching, "lobby/lb_equip_page.cpp"),
            # Registered once, at its final home (docs/plan.md 12): the lobby event/status band
            # (`.text` 0x802FA9A0..0x8030121C, 120 symbols / 26748 B, plus its extab run
            # 0x800156B4..0x80015964 and extabindex run 0x80034074..0x8003447C - 86 records each, both
            # exactly the gap between the bracketing registered units' runs).  It links between the
            # `ef` bands (`ef/eft035.cpp` below it, `ef/fn_803066F0.c` above it), but the module is
            # `lobby` (class 3): the range's own predicates read the lobby work block `lobby_w`
            # (.bss 0x806AAB44) at +0x003/+0x15F/+0x161 and its `.sbss` run is the lobby pointer block
            # `lbl_80794880`; no `__FILE__` string covers the range, so the file keeps the map's stem
            # (class 4).  Same `cflags_lobby` as the band below, with the per-file
            # `#pragma exceptions on` the other lobby units use to emit the unwind records.
            Object(NonMatching, "lobby/fn_802FA9A0.cpp"),
            # Registered once, at its final home (docs/plan.md 12): the lobby UI band
            # (`.text` 0x8030121C..0x803066F0, 52 symbols / 21716 B, plus its extab run
            # 0x80015964..0x80015AAC and extabindex run 0x8003447C..0x80034668 - both exactly the gap
            # between the bracketing registered units' runs).  Module `lobby` (class 3): the range's
            # callees are the lobby/menu UI API (`LbStr`, `get_lsp_data`, `draw_sprite_ary`,
            # `draw_font_idx`, `put_menu_cursor`, `GetMenuFontColor`, `ItemName`) and its `.sbss`
            # reference is the lobby pointer block `lbl_80794880`; no `__FILE__` string covers the
            # range and the dump answers only `zz_` placeholders, so the file keeps the map's stem
            # (class 4).  Same `cflags_lobby` as the band below, with the per-file
            # `#pragma exceptions on` the other lobby units use to emit the unwind records.
            Object(NonMatching, "lobby/fn_8030121C.cpp"),
            # lobby/lb_companion_ui.cpp: the lobby companion/status UI band
            # (`.text` 0x80338808..0x8033F270, 133 functions / 27240 B, plus its extab run
            # 0x80016994..0x80016CA4 and extabindex run 0x80035CC4..0x8003615C - 98 records each,
            # both exactly the gap the bracketing split objects leave: `fn_803386C4`'s record ends
            # at 0x80016994 / 0x80035CC4 and the next function's begins at 0x80016CA4 /
            # 0x8003615C).  Module `lobby` (class 3): the range references the lobby work block
            # `lobby_w` (`.bss` 0x806AAB44), `lb_param_w` (`.bss` 0x806590B4) and the global
            # `lb_deli_data` (`.data` 0x8060DDB8), calls the lobby string helper `LbStr` 11 times,
            # and its callee profile is exactly the one the registered lobby bands document
            # (`LbStr`, `get_lsp_data`, `draw_sprite_ary`/`_idx`/`_anim_*`, `draw_font*`,
            # `GetMenuFontColor`, `sysSE_req`); no `__FILE__` string covers the range and the dump
            # answers only `zz_XXXXXXXX_` for it, so every symbol the file defines is named from its
            # own body (the naming pass of 2026-09-26, 79 names - see the unit header and
            # `.pi/notes/80338808-named.md`; the map's 78 `fn_` rows in the range moved with it and
            # the 79th follows main's `hud_key_lookup`).  Same
            # `cflags_lobby` as the bands above, with the per-file `#pragma exceptions on` the
            # other lobby units use to emit the unwind records.  Seam UNPROVEN (measured): the
            # decisive `__FILE__` class is absent for the whole band, no private pool crosses
            # either edge, and the `.data`/`.sdata` runs continue across both with ascending owners
            # (candidate-only class); see the unit header for the counts and the bracket.
            Object(NonMatching, "lobby/lb_companion_ui.cpp"),
            Object(NonMatching, "lobby/lb_screen_step.cpp"),  # phase 4 stub: new unit, cflags_lobby of the lib
            # Registered once, at its final home (docs/plan.md 12): the lobby menu page's frame step and its
            # info/text selector (`.text` 0x80365C84..0x80366618, 2 functions / 2452 B, plus their
            # extab run 0x800177C4..0x800177D4 and extabindex run 0x8003720C..0x80037224 - each run
            # is exactly the gap the bracketing split objects leave).  Module `lobby` and the names
            # from the code (class 3, GUESS marked in the unit header): the frame step reads
            # `lobby_w` (.bss 0x806AAB44) at +0x0AC - the menu pointer `lobby/fn_801E7530.cpp` uses -
            # and the selector reads the lobby page block `lbl_80794880`; every callee is a lobby/hud
            # symbol (set_zmode/set_blendmode, the 0x1877/0x1878/0x1879 panel setters fn_80214EF0/
            # fn_80214FB8/fn_802150DC/fn_80215170, fn_801E66A8/fn_801E677C/fn_801E68B4,
            # fn_801EF73C/fn_801F0834, fn_8033C1AC) plus the Pl icon queries
            # equip_kind_table_class/fn_8027F1B8/fn_8027F21C.  No `__FILE__` string covers the range and the
            # dump answers only `zz_` placeholders, so the unit and its two symbols are named for
            # what the bodies do - `lb_menu_page_step` (the frame step) and `lb_menu_info_update`
            # (the text selector); both map rows were renamed with the source via `symedit.py`.
            # C++; every plain `fn_` definition that remains is another unit's symbol.
            # `.data` 0x805EDAB4..0x805EDAE0 is claimed: it is `lb_menu_page_step`'s own 11-entry
            # jump table (`jumptable_805EDAB4`, `scope:local`), the only data either body emits.  Same
            # `cflags_lobby` as the band above, with the per-file `#pragma exceptions on` and
            # `#pragma peephole off` the other lobby units use (both measured, see the unit header).
            Object(NonMatching, "lobby/lb_menu_page.cpp"),
            # Registered once, at its final home (docs/plan.md 12): proposal
            # `80394038_fn_80394038.cpp` (`.text` 0x80394038..0x803967F0, 41 functions / 10168 B;
            # extab 0x8001833C..0x8001843C, extabindex 0x80038340..0x800384C0 - each run exactly the
            # gap the bracketing split objects leave).  Module `lobby` from the code (42 `lobby_w`
            # reads, `LbStr`, `lb_npc_Get_motion_no`, `get_now_areano`, `get_move_work_adrs`,
            # `get_option_cfg`, the lobby/HUD 2D layer) and from the module's own units; the file name
            # is derived from the range's one real map name `draw_quest_board` in the `lb_*` scheme of
            # its siblings (`lb_npc.cpp`, `lb_menu_page.cpp`, `lb_companion_ui.cpp`) - MARKED GUESS,
            # no `__FILE__` string covers the range and the dump answers `zz_` for 40 of 41 addresses.
            # C++: mangled callees (`LbStr__FUcUs`, `set_zmode__FbUcb`, `sysSE_req__Fl`,
            # `draw_font_idx__FUsPScUlPC10_mh_ivec2_`) reached through their real signatures (rule 9).
            # Same `cflags_lobby` as its siblings.  Same `cflags_lobby` as the band above.
            Object(NonMatching, "lobby/lb_quest_board.cpp"),
            # Registered: the lobby list/detail UI band
            # (`.text` 0x8038E8E8..0x80394038, 70 functions / 22352 B) plus its extab run
            # 0x8001819C..0x8001833C (52 x 8 B) and extabindex run 0x800380D0..0x80038340
            # (52 x 12 B) - both runs are exactly the gap the bracketing units leave
            # (`enemy/em009_act.cpp` ends at 0x8001819C / 0x800380D0).  Module `lobby`,
            # evidence class 3 (see the unit header): the range reads `lobby_w`/`lb_param_w`,
            # calls `LbStr` 28 times and the whole lobby/HUD 2D API, and the lobby menu
            # dispatcher `fn_80211E68` calls two of its functions as screen entry points.  The
            # file name is the marked GUESS `lb_quest_ui.cpp`; the seam is unproven with two
            # candidates (0x8038EF28 strong `.sdata2` cut, 0x8038EC44 where the six
            # `em009_prog_tbl` entry slots end) - both are in the unit header and the outbox.
            # Same `cflags_lobby` as its siblings with the per-file `#pragma exceptions on`.
            Object(NonMatching, "lobby/lb_quest_ui.cpp"),
            # Registered once, at its final home (`.text`
            # 0x803A3A50..0x803AA4A4, 95 functions / 27220 B; extab 0x8001882C..0x80018A6C and
            # extabindex 0x80038AA8..0x80038E08, both exactly the gap the bracketing registered
            # units leave; `.data` 0x805F2038..0x805F2090, the range's two private switch tables).
            # Module `lobby` and the file name `lb_quest_screen.cpp` are a MARKED GUESS (see the
            # unit header): no `__FILE__` string covers the range, the dump answers `zz_` for 92 of
            # its 95 functions, and the range is a `--max-bytes` cap over five different bands.  The
            # module comes from the range's globals and API (`lobby_w`, `lb_param_w`, `Screen_w`,
            # `lbl_80794880`, `LbStr`, `lb_item_get_data`, `subTransSet`) and the nearest registered
            # same-lib family (`lobby/lb_quest_ui.cpp`, `lobby/lb_quest_board.cpp`).  C++ with
            # exceptions on (every framed function of the range owns an extab record).  Same
            # `cflags_lobby` as its siblings; the seam is unproven and the unit header lists the
            # five bands and the bodies still to write.
            Object(NonMatching, "lobby/lb_quest_screen.cpp"),
        ],
    },
    {
        # The game root file (main.cpp). New lib because nothing registered is a game-root file.
        #
        # Flags, measured on the batch-2 source with the real command line (all numbers are that unit's
        # per-symbol match, before -> after):
        #   -O4,p -> -O3            main 71.12 -> 96.70, fn_8003FC64 65.62 -> 99.58,
        #                           change_widemode_req__FUc 58.00 -> 100.00, fn_8003F58C 79.78 -> 90.81
        #   -inline auto -> noauto   fn_8003F52C and fn_8003F564 74.00 -> 100.00 (retail keeps the out-of-line
        #                           call to fn_8003F554), change_widemode_req_default__Fv 21.18 -> 100.00
        #                           `-inline off` instead costs fn_8003F940 0.98 points (99.02): it turns
        #                           retail's inlined aggregate GXRenderModeObj copy into a call to the
        #                           implicit copy-assignment operator; `noauto` keeps that one inlined while
        #                           still refusing the auto inlines above
        #   no -func_align needed   with -O3 the functions pack on 4 B boundaries as the target does; -O4,p
        #                           implies -func_align 16 and would need a 4-byte override
        #   -use_lmw_stmw           absent from this 4448 B range in the target (no lmw/stmw)
        "lib": "main",
        "mw_version": "Wii/1.3",
        "cflags": cflags_main,
        "progress_category": "game",
        "host": False,
        "objects": [
            Object(NonMatching, "main.cpp"),
            # The C++ allocation group right after main.cpp (operator new/delete over main.cpp's heap). Same
            # lib because it is the same module and the same measured flags; its retail file name is not
            # evidenced - the unit is defined by its extab group (see the file header comment).
            Object(Matching, "sys_mem.cpp"),
            Object(NonMatching, "fn_80040598.cpp"),
            # Registered once, at its final home (docs/plan.md 12): proposal `800408A8` - the game-root
            # pad / mode file (initKPAD/shutdownKPAD/setWpadCallback, get_ControlType/get_rcSwitch_*,
            # GameModeExec/VsGameModeExec/ArenaSelExec, disp_beta, setSoftresetFlag).  Its original source
            # file name is evidenced by the `__FILE__` string `mh3_pad.cpp` (0x80580EF0) that its own
            # `fn_80041AA4` assert references; same lib and flags as the game-root system files beside it.
            Object(NonMatching, "mh3_pad.cpp"),
            Object(NonMatching, "pad_connect.cpp"),
            # Registered once, at its final home (docs/plan.md 12): proposal `80047398` - the character
            # face/skin TPL render unit (117 functions / 0x5610 B, 0x80047398..0x8004C9A0).  Game-root
            # band: the link neighbour `mh3_pad.cpp` ends at 0x80047398 and this unit's link neighbours
            # are the `main` lib's root files, so it takes their lib and flags.  The name stays the map's
            # `fn_80047398` stem - no `__FILE__` string and no runtime-dump name exists for the range
            # (class 3/4 in the brief; see the file header).
            Object(NonMatching, "fn_80047398.cpp"),
            Object(NonMatching, "userdata_item.cpp"),
            # Registered once, at its final home (docs/plan.md 12): proposal `8004CAD8` - the game-root
            # draw/gallery band (drawshape_exec, write/read_wpad_memory, gallery_open, set_mydata2vs, the
            # nw4r-math wrappers).  Same lib and flags as the game-root system files beside it
            # (cflags_main); the name stays the map's `fn_8004CAD8` stem - no `__FILE__` string exists for
            # the range (the `draw_shape.cpp` string at 0x8058178C belongs to the next unit) and the
            # runtime dump carries only `zz_` placeholders (class 3/4 in the brief; see the file header).
            Object(NonMatching, "fn_8004CAD8.cpp"),
            # Registered once, at its final home (docs/plan.md 12): proposal `80059550` - the game-root
            # system-message / save-create callback unit (0x80059550..0x8005AA28, 12 functions).  Game-root
            # band: its link neighbours are the `main` lib's root files and it is driven by the game-root
            # task registrar `fn_80046D34`, so it takes their lib and flags (cflags_main).  The name stays
            # the map's `fn_80059550` stem - no `__FILE__` string and no runtime-dump name exists for the
            # range (class 3 in the brief; see the file header).
            Object(NonMatching, "fn_80059550.cpp"),
            # Registered once, at its final home (docs/plan.md 12): proposal `800D45AC` - the
            # resource/"work" manager (RESmemAlloc/RESmemFree/pull_res_mem/push_res_mem, ckResourceName,
            # nwAddResource/nwDelResource, nwWorkInitialize/nwMoveStart/nwMoveEnd).  Same lib and flags as
            # the game-root system files beside it (cflags_main matches the sound/ef neighbours too).
            Object(NonMatching, "nw_resource.cpp"),
            # Registered once, at its final home (docs/plan.md 12): proposal `8004C9A0` - the user-data
            # equipment-slot selector (0x8004C9A0..0x8004CAD8), the inverse of the adjacent
            # `fn_8004CAD8` clear routine; same game-root band, lib and flags as the files above.
            Object(NonMatching, "fn_8004C9A0.cpp"),
            # Registered once, at its final home (docs/plan.md 12): proposal `80054C64` - the 2D
            # shape/texture draw layer.  Evidence class 1: every `nw4r::db::Panic` assert reachable from
            # the range passes the bare source name "draw_shape.cpp" (.data 0x8058178C), referenced
            # nowhere else in the image.  Same lib and flags as the game-root system files beside it
            # (cflags_main); the proposal covers only part of the TU - the drawshape_* half lives in the
            # previous proposal - see the file header.
            Object(NonMatching, "draw_shape.cpp"),
            Object(NonMatching, "draw_shape_arm.cpp"),
            # Registered once, at its final home (docs/plan.md 12): proposal `80056F24` - the screen
            # fade / filter / glare band (fade_set/fade_reset/get_fade_stat, GlareFilter_on,
            # filter_reset, setFilterPrio, the FIFO writers and the GX setup bodies), 59 symbols /
            # 0x262C bytes.  Same lib and flags as the game-root system files beside it (cflags_main);
            # the name stays the map's `fn_80056F24` stem - the range carries no `__FILE__` string (its
            # data refs are only the `.sdata2` float pool, the fade table and the two `.bss` blocks) and
            # the runtime dump answers only `FUN_`/`zz_` placeholders (class 3/4 in the brief; see the
            # file header).
            Object(NonMatching, "fn_80056F24.cpp"),
            # Flags: unit header of src/Network/network_pat_control.cpp; measurements in docs/network.md.
            Object(NonMatching, "Network/network_pat_control.cpp"),
            # Flags: unit header of src/Network/net_session_close.cpp; measurements in docs/network.md.
            Object(NonMatching, "Network/net_session_close.cpp"),
            # Flags: unit header of src/Network/NetworkSessionManagerPat.cpp; measurements in docs/network.md.
            Object(NonMatching, "Network/NetworkSessionManagerPat.cpp"),
            # Flags: unit header of src/Network/NetworkLayer.cpp; measurements in docs/network.md.
            Object(NonMatching, "Network/NetworkLayer.cpp", extra_cflags=["-pool off"]),
            # Phase 4 (window fg): the HOME-button (HBM) code in link order.  The units from 0x8052A040 to
            # 0x80542D8C are stubs; the keyboard units absorb the former fn_805482CC / homebutton/fn_8054E894 / fn_80555374 /
            # keyboard_ui / keyboard / gui / tiHKBManager sources (their evidence is in the unit and header comments).
            Object(NonMatching, "homebutton/fn_8052A040.cpp"),
            Object(NonMatching, "homebutton/fn_8052B004.cpp"),
            Object(NonMatching, "homebutton/fn_8052C880.cpp"),
            Object(NonMatching, "homebutton/fn_8052E0CC.cpp"),
            Object(NonMatching, "homebutton/fn_80530680.cpp"),
            Object(NonMatching, "homebutton/fn_8053072C.cpp"),
            Object(NonMatching, "homebutton/fn_80533474.cpp"),
            Object(NonMatching, "homebutton/fn_8053E808.cpp"),
            Object(NonMatching, "homebutton/hbm_text_panel.cpp"),
            Object(NonMatching, "homebutton/hbm_kb_widget.cpp"),
            Object(NonMatching, "homebutton/hbm_kb_list.cpp"),
            Object(NonMatching, "homebutton/hbm_kb_child.cpp"),
            Object(NonMatching, "homebutton/hbm_anim_record.cpp"),
            Object(NonMatching, "homebutton/hbm_value.cpp"),
            Object(NonMatching, "homebutton/hbm_hermite.cpp"),
            Object(NonMatching, "homebutton/fn_8055F728.cpp"),
            Object(NonMatching, "homebutton/fn_8055FB70.cpp"),
            Object(NonMatching, "homebutton/fn_8055FD58.cpp"),
            Object(NonMatching, "homebutton/hbm_kb_event.cpp"),
            Object(NonMatching, "homebutton/hbm_kb_cursor.cpp"),
            Object(NonMatching, "homebutton/gui.cpp"),
            Object(NonMatching, "homebutton/tiHKBManager.cpp"),
            Object(NonMatching, "homebutton/fn_8056D814.cpp"),
        ],
    },
]


# Optional callback to adjust link order. This can be used to add, remove, or reorder objects.
# This is called once per module, with the module ID and the current link order.
#
# For example, this adds "dummy.c" to the end of the DOL link order if configured with --non-matching.
# "dummy.c" *must* be configured as a Matching (or Equivalent) object in order to be linked.
def link_order_callback(module_id: int, objects: List[str]) -> List[str]:
    # Don't modify the link order for matching builds
    if not config.non_matching:
        return objects
    if module_id == 0:  # DOL
        return objects + ["dummy.c"]
    return objects


# Uncomment to enable the link order callback.
# config.link_order_callback = link_order_callback


# Optional extra categories for progress tracking
# Adjust as desired for your project
config.progress_categories = [

    ProgressCategory("auto", "Auto (bulk attribution)"),    ProgressCategory("game", "Game Code"),
    ProgressCategory("sdk", "SDK Code"),
]
config.progress_each_module = args.verbose
# Optional extra arguments to `objdiff-cli report generate`
config.progress_report_args = [
    # Marks relocations as mismatching if the target value is different
    # Default is "functionRelocDiffs=none", which is most lenient
    # "--config functionRelocDiffs=data_value",
]


def warn_if_original_missing() -> None:
    """Warn loudly when this tree has no original DOL, so `build.ninja` will be a stub (#9).

    `generate_build` emits the per-unit compile edges, the link edges and the real default target only
    when `build/<version>/config.json` exists. That file is produced by `dtk dol split`, which reads the
    `object:` DOL named in `config.yml`. A fresh git worktree has no `orig/` (it is gitignored), so
    `configure.py` there silently writes a `build.ninja` whose default target is the split edge and which
    has no way to compile a unit - measured 2026-09-2x, several workers read that stub as a configure bug
    and hand-copied the DOL. Name the problem and the fix instead of writing a build file that cannot build.
    """
    try:
        text = open(config.config_path, encoding="utf-8", errors="replace").read()
    except OSError:
        return
    m = re.search(r"^\s*object:\s*(\S+)\s*$", text, re.M)
    if m is None or os.path.exists(m.group(1)):
        return
    print(
        "WARNING: %s is missing, so this tree cannot split: build.ninja will carry no per-unit\n"
        "  rules (no `build ...: mwcc` edges, only the split edge), so `ninja` cannot build a unit\n"
        "  object and the score is never measured.\n"
        "  A fresh worktree has no `orig/` because it is gitignored; seed it from MAIN before\n"
        "  configuring - copy `orig/RMHE08/**` in, or run the claim seeder - then re-run\n"
        "  `python configure.py`. See `tools/units/claims.py seed_worktree_build`." % m.group(1),
        file=sys.stderr,
    )


if args.mode == "configure":
    # #9: a worktree without orig/ would get a stub build.ninja - warn loudly rather than write it silently
    warn_if_original_missing()
    # Write build.ninja and objdiff.json
    generate_build(config)
elif args.mode == "progress":
    # Print progress information
    calculate_progress(config)
else:
    sys.exit("Unknown mode: " + args.mode)
