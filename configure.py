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
    "-I include",
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
    "-i include",
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

# g3d flags (src/g3d/g3d_resanmamblight.c). Evidence: the retail fn_800680A8 (0x24 B / 9 instructions)
# loads the field at +0xC *before* the epilogue's LR reload. Under -O4,p the nine instructions are the same
# multiset in the other order; under -O3 the object is byte-identical. The -O3 variants tried (-inline auto /
# -inline noauto / -opt nopeephole) all reproduce the retail order, so the level is the lever. Camellia and
# RSO settled on -O3 as well; -O4,p is cflags_base's default, not this build's.
cflags_g3d = [
    *[f for f in cflags_base if f not in ("-O4,p", "-inline auto")],
    "-O3",
    # `-inline noauto`, not cflags_base's `-inline auto`: the lib is taking in units whose reconstruction was
    # measured under `-inline noauto` (the same inlining as cflags_main), and under `-inline auto` they lose
    # 13.03 and 1.27 points. Measured 2026-09-24 with a scratch compile of the exact proposed command line,
    # official report metric plus a raw per-section byte compare, over the whole lib: `-inline noauto`
    # reproduces both units' baselines byte-for-byte (83.20733 / 94.86212) where `-inline auto` gives 70.17290
    # / 93.59429, and it makes this group token-identical to cflags_main. The lib's existing unit is
    # unaffected: g3d/g3d_resanmamblight.c's object is sha-identical under both flags, so the DOL hash cannot
    # move. Evidence: .pi/notes/g3d-flags.report.md.
    "-inline noauto",
    # Evidence: the retail object carries extab 0x8 + extabindex 0xC, and its single function fn_800680A8 is
    # byte-identical to ours (9 instructions, 0x24 B) - the records are unwind-only (a 4-byte flag word plus a
    # zero terminator, no PC-action ranges, no exception actions), so the flag alone reproduces them. Same
    # finding as cflags_pl and Gecko_ExceptionPPC.cp; analysis in .pi/notes/extab-gap.md.
    "-Cpp_exceptions on",
]

# Network and OS flags (src/Network/NetworkWiiMediator.cpp - formerly the 20-byte NetworkWiiMediator.c,
# retired by the worker/net-capcom batch - and src/OS/OSAlarm.c): cflags_base + -func_align 4.
# Evidence: both retail objects are `.text align 2**2`, while cflags_base's -O4,p implies -func_align 16, which
# is what our objects emit. Flipping NetworkWiiMediator with that alignment made the linker round the object's
# start up to the next 16-byte boundary: `dtk dol diff` reported fn_80413F3C expected at 0x80413F3C but found at
# 0x80413F40, every following symbol shifted by 4, and main.dol stopped matching build.sha1. Section sizes were
# already identical, so the alignment was the entire difference - the same finding as cflags_ppceabi above.
cflags_network = [*cflags_base, "-func_align", "4", "-Cpp_exceptions", "on"]
# `-Cpp_exceptions on` (flags-audit 2026-09-28): every one of the lib's 7 registered objects carries
# extab/extabindex in the target (24 B .. 1260 B), and 7 of 7 spelled it out as a file-wide
# `#pragma exceptions on`. Measured with the lib flag on and all 7 pragmas deleted: all 7 objects'
# allocatable sections byte-identical, every unit score identical, main.dol still
# BF4850739478CAAEDFE675949EB7C28595A7FDE9. The pragmas were the symptom of the lib default
# (`cflags_base`'s `off`) disagreeing with the lib's own bytes; the flag is the fix.
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

# Pl flags (src/Pl/*.cpp). Measured on this lib's three units with the real ninja command line:
#   -O4,p -> -O3          pl_master: 5/22 functions at 100 % under -O4,p, 18/22 under -O3 (and the target
#                         packs on 4 B, which -O4,p's implied -func_align 16 cannot produce);
#                         pl_skill fn_80270018 65.0 -> 87.6; pl_act fn_80276B58 85.9 -> 97.3
#   -inline auto -> noauto  pl_master fn_8026FA6C was inlined into fn_8026FB20 (892 B vs target 288);
#                         pl_skill fn_80270CA4 828 -> 684 B (= target); pl_act fn_802770E8 57 -> 229 ins
#   -opt nopeephole       pl_skill fn_80270018 87.6 -> 99.73; pl_act fn_80276B58 97.3 -> 100.0 (retail has
#                         exactly one record-form instruction in all 115 of its functions)
#   -Cpp_exceptions on    every Pl target object carries extab/extabindex and `off` emits none; with `on`
#                         the .text is unchanged and all 12 extab entries we can emit equal the target's
cflags_pl = [
    *[f for f in cflags_base if f not in ("-O4,p", "-inline auto", "-Cpp_exceptions off")],
    "-O3",
    "-inline noauto",
    "-opt nopeephole",
    "-Cpp_exceptions on",
]

# pl_skill flags (src/Pl/pl_skill.cpp). cflags_pl plus `-opt nopeephole,level=4`: at level 3 the
# allocator rematerialises fn_8027350C's `plw + i*4` base across the fn_802693C4/fn_80269474 call;
# at level 4 it keeps it in r24 like the target (416 -> 404 B, 95.50 -> 99.21). Measured on the
# whole TU: the pragma-free source under this flag set is codegen-identical to the pragma build -
# 0 of 197 symbols move and every allocatable section is byte-equal (only MWCC's generated local
# `@NNN` names in .strtab shift by 2). The level is per-object, not per-lib: pl_master loses
# fn_8026CC70 at level 4 (100 -> 33.33) and is a flipped Matching unit, so cflags_pl stays level 3.
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
        # Promoted from auto/802D0DCC_fn_802D0DCC.c (docs/plan.md: an auto unit stops being scaffolding).
        "lib": "ai",
        "mw_version": "Wii/1.3",
        "cflags": cflags_main,
        "progress_category": "game",
        "objects": [
                        # Phase 4 (docs/splits/phase4, window d): fold of ai/fn_802D0DCC.c (was Matching, demoted), fn_802CC794, fn_802C474C, fn_802C5D10, fn_802D0F34
                        # and the head of fn_802D44F4, plus the tail of light/light.cpp; the lib's cflags_main, the same for every absorbed unit.
                        Object(NonMatching, "ai/ai_npc.cpp"),
                        # Registered from proposal/802D44F4_fn_802D44F4.cpp (`.text`
                        # 0x802D44F4..0x802DDC04, 165 functions / 38672 B).  See the unit header for
                        # the seam, module and language evidence.
                        # Phase 4 (window d): the band's tail (0x802D9EA4..0x802E0740), recut from ai/fn_802D44F4.cpp; same cflags_main.
                        Object(NonMatching, "hud/cockpit.cpp"),
        ],
    },

    {
        # Promoted from auto/802B2978_fn_802B2978.c (docs/plan.md: an auto unit stops being scaffolding).
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
            # Camera band 0x802B5C58-0x802BEAAC (132 functions, 36436 B), registered from
            # proposal/802B5C58_fn_802B5C58.cpp.  The lib and cflags are the neighbours' ("stage"
            # and "ai" both build with cflags_main / Wii/1.3) and the module is `camera`: the range's
            # own named exports are get_camera_pos / get_camera_direction / get_current_view_mtx /
            # set_quake_sub, and the seam at 0x802B5C58 is a .sdata2 pool jump.
            # Phase 4 (window d): fold of the head of stage/fn_802B2AA0.cpp, camera/fn_802B5C58.cpp and the head of light/light.cpp; the lib's cflags_main.
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
        # Promoted from auto/80324F7C_fn_80324F7C.c (docs/plan.md: an auto unit stops being scaffolding).
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
            # Registered from proposal/802E7408_fn_802E7408.cpp (`.text` 0x802E7408..0x802EBED8, 64
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
            # Phase 4 (window d): fold of menu/fn_802E4978.cpp, this unit, hud/fn_802EBED8.cpp and the head of ef/eft035.cpp; `-pool off` is this unit's own and now covers the absorbed bodies (see the unit header).
            Object(NonMatching, "hud/cockpit_quest.cpp", cflags=[*cflags_hud, "-pool off"]),
            # Registered from proposal/803250B0_fn_803250B0.cpp, at its final home (docs/plan.md 12):
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
            # Phase 4 (window d): fold of enemy/em_pl_frame.cpp (no bodies) and hud/net_char_sync.cpp, cflags_hud of the lib.
            Object(NonMatching, "hud/pl_frame_sync.cpp"),
        ],
    },

    {
        # New module, registered from proposal/8029F3C8_body_set__FP7_BODY_WP10_BODY_DATAUcUlUc: the
        # item menu (`.text` 0x8029F3C8..0x802A6624, 99 functions, 0x725C B; extab
        # 0x8001360C..0x800137D4 and extabindex 0x80030F90..0x8003123C for the 57 framed functions in
        # that range - both runs are exactly the gap between the bracketing auto objects).  Module
        # `menu` and file name `menu_item.cpp` come from the range's own `__FILE__` string
        # (`.data` 0x805CDFC8, 0xE B = "menu_item.cpp"; the dump's local symbol for it is
        # `_802a22a4s_menu_item.cpp_805cdfc8`, i.e. it is emitted by 0x802A22A4, a function of this
        # range, and every `nw4r::db::Panic` assert of the range passes it - including the
        # 0x802A5444..0x802A6624 half the 2026-09-26 fold brought in).  cflags_main: the range keeps
        # `bl`s to its tiny same-file helpers
        # (`GetItemData` from `fn_8029F704`/`fn_8029F73C`, `hit_flag_set` from `fn_8029F4C4`), which is
        # cflags_main's `-inline noauto`, and it carries 0 record-form instructions like the stage and
        # Pl bands (the peephole is not proven off here - `infer.py` reads absence as no evidence).
        "lib": "menu",
        "mw_version": "Wii/1.3",
        "cflags": cflags_menu,
        "progress_category": "game",
        "objects": [
            Object(NonMatching, "menu/menu_item.cpp"),
            Object(NonMatching, "menu/menu_item_sub.cpp"),  # phase 4 stub (window d): new unit after ef/eft053, cflags_menu of the lib
            # The continuation of the menu band: proposal `802A6624_fn_802A6624.cpp` (`.text`
            # 0x802A6624..0x802AD9C0, 139 functions / 29596 B; extab 0x800137D4..0x80013B0C and
            # extabindex 0x8003123C..0x80031710).  Module `menu` from the left neighbour and from the
            # range's own entry points (`put_message`, `put_frame_dialog`, `GetMenuFontColor`); no
            # `__FILE__` string covers the range and the dump answers `zz_` for most of it, so the file
            # name is derived from what the range does (see its header - `menu_message.cpp`,
            # re-homed 2026-09-29 from the map's stem by `worker/menu-num-8725`).  Same `cflags_menu`
            # as `menu_item.cpp`: the band carries 0 record-form instructions and keeps its tiny
            # same-file `bl`s.
            Object(NonMatching, "menu/menu_message.cpp"),
            # Registered from proposal/8030D338_Put_equip_dtl_basis_sword_colorX__FP4_PLWP12_EQU.cpp
            # and re-drawn by the seam round (`.text` 0x80308FB4..0x8031A6C0, 149 functions /
            # 71436 B).  Module `menu` and file name `menu_infomation.cpp` are class-1 evidence:
            # `.data` 0x805DCCDC is the bare `__FILE__` string, it has exactly one copy in the DOL
            # and its referrers span 0x8030A328..`Set_equip_column_arrangement` (0x8031A244), so the
            # whole run is one TU (the old 0x8030D338/0x80313E24 edges were proven false; see the
            # unit header and `.pi/notes/seam-round.md`).  Sections: .text 0x80308FB4..0x8031A6C0,
            # extab 0x80015B54..0x80015F3C (125 records), extabindex 0x80034764..0x80034D40
            # (125 x 12 B).
            Object(NonMatching, "menu/menu_infomation.cpp"),
            # Registered from proposal/8031A6C0_fn_8031A6C0.cpp (the `.text` 0x8031A6C0..0x8031EA8C
            # run, 59 functions / 17356 B): the item/equipment selection screen.  Module `menu`,
            # map stem as file name (brief section 2, class 4 - the band's own `.data` run carries
            # only the *neighbouring* TU's `menu_infomation.cpp` string, never this range's).  The
            # extab/extabindex runs are 0x80015F3C..0x80016074 / 0x80034D40..0x80034F14, contiguous
            # with the previous proposal's, so the 0x8031A6C0 seam is a discovery size cap.  Same
            # `cflags_menu` as its menu siblings.  This pass registers the range and measures it;
            # the unit header names the bodies still to write.
            Object(NonMatching, "menu/fn_8031A6C0.cpp"),
            Object(NonMatching, "menu/menu_item_effect.cpp"),  # phase 4 recut of the line above (its tail), same cflags_menu
            Object(NonMatching, "menu/menu_effect_slot.cpp"),  # phase 4 stub (window d): tail recut of ef/eft_slot.cpp, cflags_menu of the lib
            # Registered from proposal/8031EA8C_fn_8031EA8C.cpp: the continuation of the
            # item/equipment selection-screen band above `menu/fn_8031A6C0.cpp` (`.text`
            # 0x8031EA8C..0x80324F7C, 67 functions / 25840 B; extab 0x80016074..0x8001621C and
            # extabindex 0x80034F14..0x80035190, both runs contiguous with the predecessor's and the
            # successor's).  Module `menu` from the left neighbour and the range's own callees (the
            # menu/HUD 2D library); no `__FILE__` string covers the range (every `.data` reference is
            # a mask/sprite table, a jumptable or a pool float) and the dump answers `zz_`, so the
            # file keeps the map's stem (brief section 2, class 4).  Same `cflags_menu` as the band
            # below: `infer.py` reads the peephole off on its objects (`fn_80324CC4`, 0 record forms
            # with 2 fold-shaped pairs) and `-use_lmw_stmw off`.
            Object(NonMatching, "menu/fn_8031EA8C.cpp"),
            # Registered from proposal/80349DD8_fn_80349DD8.cpp (`.text` 0x80349DD8..0x8034C0C4, 22
            # functions / 8940 B; extab 0x80016FE4..0x80017094, extabindex 0x8003663C..0x80036744,
            # `.data` 0x805E91E8..0x805E91F8, `.sdata` 0x807932F0..0x80793308).  Module `menu` from the
            # band its own `.data`/`.sdata` fragments sit in and from its entry points
            # (`get_str_tbl`/`get_menu_lsp_tbl`/`ItemName`/`ItemExp`/`font_print_ex`); no `__FILE__`
            # string covers the range (the flanking `menu_note.cpp` string at 0x805E91F8 belongs to the
            # 0x8034C0C4 TU) and the dump answers `zz_` for all 22 addresses, so the file and its
            # symbols are named for what they do - the menu's item page draw layer (naming pass
            # 2026-09-27; every name is a guess recorded in the file header).  Same `cflags_menu` as
            # its menu siblings.  `.data`/`.sdata` are claimed: both are this object's own and
            # byte-identical to the target's (`datagap.py` reports no data gap).
            Object(NonMatching, "menu/menu_item_page.cpp"),
            # Registered from proposal/8034C0C4_fn_8034C0C4.cpp (`.text` 0x8034C0C4..0x8034C1D0, one
            # function / 268 B): the note-list entry table.  Module `menu` and file name
            # `menu_note.cpp` are class-1 evidence - the range's `__FILE__` string `.data` 0x805E91F8
            # reads "menu_note.cpp", it has exactly ONE copy in the DOL, and it is referenced by
            # exactly this range's object, so no neighbour shares the TU.  Sections: `.text` plus
            # `.data` 0x805E9220..0x805E9248 (the unit's own 10-entry switch jump table); the two
            # `.data` literals the body loads stay unowned (declared, never defined).  Same
            # `cflags_menu` as its file family (`menu_item.cpp` and `menu_infomation.cpp` carry the
            # same `menu_*` name pattern); the body keeps no fold-shaped pair.
            Object(Matching, "menu/menu_note.cpp"),
            # Re-cut 2026-09-29 out of `enemy/em024_ai.cpp`: `.text` 0x8034C1D0..0x8034D2B0 (27
            # functions / 4320 B), extab 0x80017094..0x800170F4, extabindex 0x80036744..0x800367D4
            # and `.sdata2` 0x8079B368..0x8079B3A8.  Module `menu`: the range calls
            # `get_note_item_slot`/`item_page_option_*`/`draw_sprite`, works the `MenuRowData` table
            # and the `MenuSlot::place_entries` list and references no enemy symbol; the seam to the
            # player band is the `extabindex` and `.sdata2` runs breaking at 0x8034D2B0.  File name
            # a GUESS from the dominant type; evidence in the file header.  `cflags_menu` (this lib).
            Object(NonMatching, "menu/menu_row.cpp"),
            # Registered once, at its final home, from proposal/803967F0_fn_803967F0.cpp: the
            # quest-result screen band (`.text` 0x803967F0..0x8039D278, 85 functions / 0x6A88 B).
            # Module `menu` (evidence class 3): the `.data` band its own tables sit in carries
            # `menu_note.cpp` (0x805E91F8) below and `menu_placeinfo.cpp` (0x80604780) above, and
            # every callee is the menu library's (`get_menu_lsp_tbl`, `put_menu_cursor`,
            # `GetMenuFontColor`, `ItemName`, `PutPageArrow`, `font_print_ex`).  C++ because the
            # bodies reach genuinely mangled callees (`get_joint_wpos__6MHcharFUlPQ34nw4r4math4VEC3`).
            # The seam at 0x803967F0 is a real TU edge, not the brief's `--max-bytes` cap: the
            # `.data` referrer runs below and above it are disjoint (last below 0x803960BC, first
            # above 0x80396BBC) and this unit's `.data` run ends where `em029_prog_tbl` begins.
            # Same `cflags_menu` as its menu siblings: the band keeps unfused narrow-load pairs.
            # This pass writes 24 of the 85 bodies; the rest are map stems measuring 0 %, and the
            # unit's `.data`/`.sdata`/`.sdata2` runs are not claimed until the bodies that emit
            # them land (see the unit header's residual list).
            Object(NonMatching, "menu/menu_result.cpp"),
            # Registered once, at its final home, from proposal/8039D278_fn_8039D278.cpp: the
            # multiplayer-result screen's box cursor band plus the enemy action/substate dispatchers
            # that share its address range (`.text` 0x8039D278..0x803A3A50, 80 functions / 0x67D8 B).
            # Module `menu` (evidence class 3): the range's head is the multi-result screen, its
            # neighbours are `menu_result.cpp` below and the `menu/*` band above, and its two
            # original manglings are the box helpers on `_multi_result_work`.  C++ because those
            # manglings are defined by the range (`...__FP18_multi_result_work`).  The seam is
            # UNPROVEN - it is the discovery `--max-bytes` cap, no `__FILE__` string reaches the
            # range, and the range is really a *sequence* of objects: `tudiscover at 0x8039FD4C`
            # gives a 40-function `.sdata2`-sharing enemy cluster in the middle, and the runtime dump
            # names SDK objects interleaved with the game ones (`DBClose` 0x8039E714,
            # `gdev_cc_shutdown` 0x803A1108, `GoalOverlay::SceneCreated` 0x803A1680,
            # `homebutton::MotorCallback` 0x803A26A8).  The unit header carries the full evidence and
            # the residual list; a seam re-draw is the follow-up.  Same `cflags_menu` as its menu
            # siblings.
            Object(NonMatching, "menu/multi_result.cpp"),
            # Registered once, at its final home, from proposal/803BE30C_get_pop_dat_ptr__Fv.cpp: the
            # pop-data / option / demo system file (`.text` 0x803BE30C..0x803C4BA0, 110 functions /
            # 26772 B).  Module `menu` (evidence class 3): the range's callee surface is the menu 2D
            # library's and it shares 48-66 callees with `menu_result.cpp`/`menu_item.cpp`/
            # `menu_item_page.cpp`; the file name keeps the range's first symbol, which is the runtime
            # dump's own real name (`get_pop_dat_ptr`).  The seam is UNPROVEN - the discovery
            # `--max-bytes` cap, with `tudiscover`'s best cut at 0x803BF294 only weak; the unit header
            # carries the full evidence.  Only `.text` is claimed this pass: the unit's extab
            # 0x80019164..0x800193D4, extabindex 0x8003987C..0x80039C24 and its own `.data`/`.sdata`/
            # `.sdata2`/`.bss`/`.sbss` runs are claimed by the pass that writes the bodies emitting
            # them (docs/plan.md 8.4).  Same `cflags_menu` as its menu siblings.
            Object(NonMatching, "menu/get_pop_dat_ptr.cpp"),
            Object(NonMatching, "lobby/lb_server_sel_trans.cpp"),  # phase 4 recut of the line above (its tail), same cflags_menu
            # Registered once, at its final home, from proposal/803AA4A4_fn_803AA4A4.cpp: the quest
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
            # Registered once, at its final home, from proposal/803B465C_fn_803B465C.cpp: the field-side
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
            # Phase 4 stubs (docs/splits/phase4, window e): candidate units with no bodies yet, `cflags_menu` of the lib.
            Object(NonMatching, "menu/menu_placeinfo.cpp"),
            Object(NonMatching, "menu/movie.cpp"),
            Object(NonMatching, "menu/menu_plsearch.cpp"),
            Object(NonMatching, "menu/menu_sysmsg.cpp"),
        ],
    },

    {
        # Promoted from auto/8012BA00_fn_8012BA00.c (docs/plan.md: an auto unit stops being scaffolding).
        "lib": "enemy",
        "mw_version": "Wii/1.3",
        "cflags": cflags_main,
        "progress_category": "game",
        "objects": [
            # Registered once, at its final home (docs/plan.md 12).  The `proposal/8011D448_fn_8011D448.cpp`
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
                        # proposal/801251D0_fn_801251D0.cpp: the enemy control unit
            # (0x801251D0..0x8012BA00, 145 symbols). Registered once, at its final home
            # (docs/plan.md 12); cflags are this lib's.
            Object(NonMatching, "enemy/em_common.cpp"),
            # Registered from proposal/80137604_fn_80137604 (a 0x80137604 run discovery
            # proposed): an enemy's per-motion action/rotation update set, 20 functions / 0x2670
            # bytes.  Module `enemy` from the link band (both bracketing units are `enemy`) and
            # from the code (it calls `em_act_ck(_ENEMY_WORK*)`, `get_enemy_data`, ...); C++
            # because every mangled callee must be declared at its real signature (rule 9).
            # No `__FILE__` string survives in the range and the dump answers only `zz_`
            # placeholders, so the file keeps the map's own stem (see the unit's header).
            Object(NonMatching, "enemy/em_motion_update.cpp"),
            Object(NonMatching, "enemy/fn_80138074.c"),
            # Registered from proposal/8013ACC4_fn_8013ACC4.cpp (a 0x8013ACC4 run discovery
            # proposed): the enemy user-data command interpreter and its 0x100-entry dispatch
            # table, 3 functions / 0x119C bytes.  Module `enemy` from the link band (both
            # bracketing units are `enemy`) and from the code (`_ENEMY_WORK`, `fn_8013A900`);
            # C++ because the range calls the mangled `ran_suu__Fl`.  No `__FILE__` string
            # survives and the dump answers only `zz_` placeholders, so the file keeps the map's
            # own stem (see the unit's header).
            Object(NonMatching, "enemy/fn_8013ACC4.cpp"),
            Object(NonMatching, "enemy/em_kind.cpp"),
            # Registered from proposal/801411B8_fn_801411B8.cpp: the enemy control TU's lower half,
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
            # proposal/8015D860_fn_8015D860.cpp: the em008 per-action state-step band
            # (0x8015D860..0x8015E854, 28 functions), the run between the two units above and
            # below.  C++ (the range's callees are manglings and it allocates with `operator
            # new`); registered once, at its final home - the file keeps the map's `fn_XXXXXXXX`
            # stem because no `__FILE__` string names it (rule 7 deferred, see the unit header).
            Object(NonMatching, "enemy/em008_prog.cpp"),
            # Registered from proposal/80165FC8_fn_80165FC8.cpp: the enemy per-area seat/action unit
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
            # Registered from proposal/80181C88_fn_80181C88.cpp (the 0x80181C88 run discovery
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
            # Registered from proposal/80191598_fn_80191598.cpp (a 0x80191598 run discovery
            # proposed): the enemy aim/action group the per-enemy class tables at 0x805AA960..
            # 0x805AAA60 hold, 26 functions / 0x1154 bytes plus its extab/extabindex run.
            # Module `enemy` from the link band and the code (`em_act_ck(_ENEMY_WORK*)`,
            # `get_move_work_adrs(3)`, the enemy work's aim record); C++ because every callee out
            # of the range is a mangled symbol.  No `__FILE__` string survives in the range and
            # the dump answers only `zz_` placeholders, so the file keeps the map's own stem (see
            # the unit's header).
            Object(NonMatching, "enemy/em018_prog.cpp"),
            # Registered once, at its final home (docs/plan.md 12) from
            # proposal/801CCBC4_fn_801CCBC4.cpp (the 0x801CCBC4 run discovery proposed; 58
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
            # Registered from proposal/801B0010_fn_801B0010.cpp (a 0x801B0010 run discovery
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
            # Registered once, at its final home (docs/plan.md 12) from
            # proposal/801D80EC_fn_801D80EC: the enemy action band 0x801D80EC..0x801DB8E0, 49
            # functions / 0x37F4 bytes, plus the extab run 0x800101E4..0x8001031C (39 records) and
            # the extabindex run 0x8002C1C0..0x8002C394 (39 records) its framed functions carry.
            # Module `enemy` from the link band (the unit below is `enemy/fn_801D428C.cpp` and the
            # unit above `enemy/fn_801DB8E0.cpp`) and from the code (every function takes the shared
            # `_ENEMY_WORK`).  C++ because the range reaches mangled callees
            # (`em_frame_check__FP...`, `calcVecAng2__FP...`, `rotVecY__FP...`).  No `__FILE__`
            # string is reachable and the runtime dump answers only `zz_` placeholders, so the file
            # keeps the map's own stem (see the unit header).
            Object(NonMatching, "enemy/em007_act.cpp"),
            # Registered once, at its final home (docs/plan.md 12) from
            # proposal/8019ED34_fn_8019ED34.cpp (the 0x8019ED34 gap between this unit and
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
            # Registered from proposal/801BD6C0_fn_801BD6C0.cpp: the enemy motion/act-instruction
            # band's continuation, 128 functions / 0xC944 bytes (0x801BD6C0..0x801CA004) plus the
            # extab run 0x8000FACC..0x8000FDFC (102 records) and the extabindex run
            # 0x8002B71C..0x8002BBE4 (102 x 12 B) its framed functions carry.  Module `enemy` from
            # the link band (the unit below ends exactly at this range's start; the unit above starts
            # later at 0x801D428C after the still-unclaimed 0x801CA004..0x801D428C run) and from the
            # code (every callee is the enemy work API).  C++ because the range reaches mangled
            # callees.  No `__FILE__` string survives and the dump answers only `zz_` placeholders,
            # so the file keeps the map's own stem (see the unit header).
            Object(NonMatching, "enemy/em006_prog.cpp"),
            # Registered once, at its final home (docs/plan.md 12) from
            # proposal/801CA004_fn_801CA004.cpp: the enemy action/state band, 47 functions /
            # 0x2BC0 bytes (0x801CA004..0x801CCBC4), plus the extab run 0x8000FDFC..0x8000FF4C and
            # the extabindex run 0x8002BBE4..0x8002BDDC its 42 framed functions carry.  Module
            # `enemy` from the link band (both bracketing registered units are `enemy/*`) and from
            # the code (every function takes the shared `_ENEMY_WORK`); C++ because the range's
            # callees are mangled (`getTevKColor__6MHchar...`, `__nw__FUl`).  No `__FILE__` string
            # is reachable and the dump answers only `zz_` placeholders, so the file keeps the
            # map's own stem (see the unit header).
            Object(NonMatching, "enemy/em004_act.cpp"),
            # Registered once, at its final home (docs/plan.md 12) from
            # proposal/801A9540_fn_801A9540.cpp: the enemy per-action state-machine band
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
            # Registered once, at its final home (docs/plan.md 12) from proposal/8035E034_fn_8035E034.cpp:
            # the em033/em035 enemy-program handlers (`.text` 0x8035E034..0x8035F2B4, 16 functions /
            # 0x1280 bytes) plus their extab/extabindex group.  Module `enemy` from the `.data` program
            # tables `em033_prog_tbl`/`em035_prog_tbl` that list the range's entry points and from the
            # shared `_ENEMY_WORK` record every body drives; the name keeps the map's `fn_` stem (no
            # `__FILE__` string is reachable and the dump answers only `zz_` placeholders).  Sections:
            # extab 0x80017574..0x800175DC (13 records), extabindex 0x80036E94..0x80036F30 (13 x 12 B),
            # `.text` 0x8035E034..0x8035F2B4.  C++; every plain `fn_` definition is `extern "C"`.
            # Phase 4 (window d): fold of the head of ef/eft052.cpp (no bodies) and enemy/fn_8035E034.cpp; cflags_main of the lib.
            Object(NonMatching, "enemy/em033_prog.cpp"),
            # Registered once, at its final home (docs/plan.md 12) from proposal/8034C1D0 and re-cut
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
            # Registered from proposal/8035F2B4_fn_8035F2B4.cpp, re-cut to the em035 program's own
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
            # Registered once, at its final home (docs/plan.md 12) from
            # proposal/8036CF64_fn_8036CF64.cpp: the em020 enemy program
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
            # Registered once, at its final home (docs/plan.md 12) from
            # proposal/80375424_fn_80375424.cpp: the em019 monster-AI file's body
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
            # Phase 4 (window d): folded into enemy/em020_prog.cpp (was Matching, demoted).
            # Registered once, at its final home (docs/plan.md 12) from proposal/80387844_fn_80387844.cpp, renamed in phase 4 from `enemy/fn_80387844`:
            # the em009 enemy's monster-AI action band, `.text` 0x803868DC..0x8038EC44 (2026-09-30 recut of the
            # range first registered as 0x80387844..0x8038E8E8; see the unit's header).  Module `enemy` from the code (every body drives the shared `_ENEMY_WORK` record
            # through `em_frame_check__FP11_ENEMY_WORKUsff`, `em_after_frame_check`, `get_joint_wpos_em`,
            # `em_magma_check`, `get_em_chg_scale`) and from the `.data` `em0XX_prog_tbl` program tables
            # of the bracketing enemy bands; C++ because the range reaches genuinely mangled callees
            # (`setVector3__FPQ34nw4r4math4VEC3fff`, `mulVecMatAddTrans`, `rotVecY`) through their real
            # signatures (rule 9).  No `__FILE__` string is reachable from the range and the runtime dump
            # answers only `zz_` placeholders, so the file stem is derived from the map's `em009_act_*` names.
            Object(NonMatching, "enemy/em009_act.cpp"),
            # Registered once, at its final home (docs/plan.md 12) from proposal/80382310_fn_80382310.cpp:
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
            # Phase 4 (window d): the old range is recut into enemy/em019_ai.cpp (head), enemy/em_prog_support.cpp and enemy/em_prog_tail.cpp; same cflags_main.
            Object(NonMatching, "enemy/em_prog_support.cpp"),
            Object(NonMatching, "enemy/em_prog_tail.cpp"),
            # Registered from proposal/802F5138_fn_802F5138.cpp (`.text` 0x802F5138..0x802FA9A0, 72
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
            # Registered once, at its final home (docs/plan.md 12) from
            # proposal/8032C920_fn_8032C920.cpp, whose range 0x8032C920..0x80334568 turned out to be
            # two TUs (`--max-bytes` had cut them in one).  FIRST TU 0x8032C920..0x8033041C, 74
            # functions / 15100 B: the enemy work record's action band - it hands its r3 record to
            # `em_frame_check`/`em_act_ck`/`em_die_ck`/`em_after_frame_check`/`em_get_mot_no`
            # (67 sites) and every field it reads on that pointer is one `include/enemy/ENEMY_WORK.h`
            # names.  It owns the class vtable `lbl_805E04E0`, the table run `.data`
            # 0x805DFC9C..0x805E0510, extab 0x80016464..0x8001662C, extabindex
            # 0x800354FC..0x800357A8, the `.ctors` word 0x8056F3A0 -> `fn_80330128` and its
            # `.sdata2` pool half 0x8079B108..0x8079B210 (declared, not claimed - playbook 23).
            # Module `enemy` from the code and the link band; C++ because every out-of-range callee
            # is a mangled symbol.  No `__FILE__` string is reachable and the runtime dump answers
            # only `zz_` placeholders, so the file name and its own symbols come from the bodies and
            # the module's `em_<noun>_<verb>` scheme (the merger lane's naming pass, 2026-09-26).
            Object(NonMatching, "enemy/em_act_step.cpp"),
            Object(NonMatching, "enemy/em_act_step_tail.cpp"),  # phase 4 stub (window d): tail recut of the line above, same cflags_main
        ],
    },

    {
        # Promoted from auto/800D7F54_fn_800D7F54.cpp (docs/plan.md: an auto unit stops being scaffolding).
        "lib": "sound",
        "mw_version": "Wii/1.3",
        "cflags": cflags_main,
        "progress_category": "game",
        "objects": [
                        Object(NonMatching, "sound/fn_800D7F54.cpp"),
            # Registered from proposal/800DD1F0_fn_800DD1F0.cpp (125 symbols / 0x6ACC bytes) and recut in phase 4: the head is folded into
            # sound/fn_800D7F54.cpp, this row is the SE request cluster's middle, then the `MHchar` model class and the job request.
            # `sound` module (include/unsplit/sound.h is the band) and C++ (the `* __FP...` / `move__6MHcharFUs` manglings).
            Object(NonMatching, "sound/se_req.cpp"),
            Object(NonMatching, "sound/mhchar.cpp"),
            Object(NonMatching, "sound/sound_job.cpp"),
            # Registered from proposal/800E3CBC_fn_800E3CBC.cpp (the 0x800E3CBC run discovery
            # proposed).  Module from the placed link-neighbour sound/fn_800E46E8.cpp; the dump's
            # prim_init_all/set_blendmode/set_zmode name the functions, not the TU (see the file header).
            Object(NonMatching, "sound/fn_800E3CBC.cpp"),
            Object(NonMatching, "sound/fn_800E46E8.cpp"),
            Object(NonMatching, "sound/sound_obj.cpp"),
            Object(NonMatching, "sound/snd_stream_reloc.cpp"),
            Object(NonMatching, "sound/snd_level_tbl.cpp"),
            # Registered from proposal/800E8E60_fn_800E8E60.cpp (a 0x800E8E60 run discovery proposed).
            # The quest/challenge sound work system: 149 functions / 0x6978 bytes.  C++ from the range's
            # own mangled symbols; no `__FILE__` string survives, so the map's stem is the file name.
            Object(NonMatching, "sound/fn_800E8E60.cpp"),
            Object(NonMatching, "sound/snd_stream_mgr.cpp"),
            Object(NonMatching, "sound/snd_voice_pool.cpp"),
            Object(NonMatching, "sound/quest_snd.cpp"),
            Object(NonMatching, "sound/snd_bank_loader.cpp"),
            # Registered from proposal/800F2A94_fn_800F2A94.cpp (a 0x800F2A94 run discovery proposed,
            # 88 functions / 27408 B).  Same lib and same cflags as its neighbours: the range continues the
            # sound band up to the effect (eft_control) block at 0x800F6520.  Flags are this lib's
            # cflags_main; the per-symbol measurements are in the worker's outbox.
            Object(NonMatching, "sound/fn_800F2A94.cpp"),
        ],
    },

    {
        # Promoted from auto/800BFFD4_fn_800BFFD4.cpp (docs/plan.md: an auto unit stops being scaffolding).
        "lib": "ef",
        "mw_version": "Wii/1.3",
        "cflags": cflags_main,
        "progress_category": "game",
        "objects": [
            # Registered once, at its final home (docs/plan.md 12) from
            # proposal/80358624_fn_80358624.cpp: the `eft052` effect family (its `_EFT` tag is 52)
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
            # Registered once, at its final home (docs/plan.md 12).  The `proposal/8009B374_fn_8009B374`
            # range (`.text` 0x8009B374..0x8009CD64, 16 functions / 6640 B): the NW4R effect library's
            # shared math/utility file.  The range's own `__FILE__` string (`ef_util.cpp` at 0x80591948,
            # read out of orig/RMHE08/sys/main.dol) names the TU - see the unit's file header.  Sections:
            # extab 0x80009A38..0x80009A98, extabindex 0x80022B30..0x80022BC0, .text 0x8009B374..0x8009CD64.
            # The lib is `ef` (the file's own module); its cflags_main is token-identical to the
            # neighbouring g3d/g3d_gpu.cpp's cflags_g3d, and dtk links by address, so the lib choice
            # cannot move the object or change its codegen.
            Object(NonMatching, "ef/ef_util.cpp"),
            # Registered once, at its final home (docs/plan.md 12).  The
            # `proposal/8009CDBC_fn_8009CDBC` range (0x8009CDBC..0x800A3044, 26 functions): the
            # effect library's key-frame animation curve.  The range's own `__FILE__` string
            # (`ef_animcurve.cpp` at 0x80591E68) names the TU - see the unit's file header.  Sections:
            # extab 0x80009A98..0x80009B40, extabindex 0x80022BC0..0x80022CBC,
            # .text 0x8009CDBC..0x800A3044.
            Object(NonMatching, "ef/ef_animcurve.cpp"),
            # Registered once, at its final home (docs/plan.md 12).  The
            # `proposal/800A3044_fn_800A3044` range (0x800A3044..0x800A388C, 7 functions): the NW4R
            # effect library's creation queue.  The range's own `__FILE__` string (`ef_creationqueue.cpp`
            # at 0x805922C0) names the TU - see the unit's file header.
            Object(NonMatching, "ef/ef_creationqueue.cpp"),
            # Registered once, at its final home (docs/plan.md 12).  The
            # `proposal/800A388C_fn_800A388C` range (0x800A388C..0x800A40F4, 9 functions): the NW4R
            # effect library's draw-order helpers.  The range's own `__FILE__` string
            # (`ef_draworder.cpp` at 0x805923A0) names the TU - see the unit's file header.  Sections:
            # extab 0x80009B60..0x80009BA0, extabindex 0x80022CEC..0x80022D4C,
            # .text 0x800A388C..0x800A40F4.
            Object(NonMatching, "ef/ef_draworder.cpp"),
            # Registered once, at its final home (docs/plan.md 12).  The
            # `proposal/800A40F4_fn_800A40F4` range (0x800A40F4..0x800A56B0, 39 functions): the NW4R
            # effect library's `nw4r::ef::Effect` object (its table lbl_80592588, the create/retire
            # paths, the emitter sweeps and the EffectSystem constructor/destructor).  The range's own
            # `__FILE__` string (`ef_effect.cpp` at 0x80592430) names the TU - see the unit's file
            # header.  Sections: extab 0x80009BA0..0x80009C38, extabindex 0x80022D4C..0x80022E30,
            # .text 0x800A40F4..0x800A56B0.
            Object(NonMatching, "ef/ef_effect.cpp"),
            # Registered once, at its final home (docs/plan.md 12).  The
            # `proposal/800A56B0_fn_800A56B0` range (0x800A56B0..0x800A6350, 23 functions): the game side
            # of the NW4R effect library's system object.  The range's own `__FILE__` string
            # (`ef_effectsystem.cpp` at 0x80592698) names the TU - see the unit's file header.  Sections:
            # extab 0x80009C38..0x80009CD4, extabindex 0x80022E30..0x80022EE4,
            # .text 0x800A56B0..0x800A6350, .ctors 0x8056F2D8..0x8056F2DC.
            Object(NonMatching, "ef/ef_effectsystem.cpp"),
                        Object(NonMatching, "ef/ef_drawstripestrategy.cpp"),
            Object(NonMatching, "ef/ef_particlemanager.cpp"),
            # Registered once, at its final home (docs/plan.md 12).  The `proposal/800AEE48_fn_800AEE48`
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
            # Registered from proposal/800C9540_fn_800C9540.cpp (a 0x800C9540 run discovery proposed).
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
            # Registered from proposal/800CDB2C_fn_800CDB2C.cpp (a 0x800CDB2C run discovery proposed at a --max-bytes cap, recut
            # 2026-09-30 and renamed in phase 4) - the game-system core; see the unit's file header.
            Object(NonMatching, "ef/system_core.cpp"),
            Object(NonMatching, "ef/eft001.cpp"),
            # Registered once, at its final home (docs/plan.md 12).  The
            # `proposal/800F6520_fn_800F6520` range (0x800F6520..0x800F95A4, 43 functions): the game's
            # eft resource manager (eft_control, the 256-slot proID table, the load/create path and the
            # effect-heap push).  Module `ef` from both bracketing units; the name follows the siblings'
            # scheme - see the unit's file header for the evidence.
            Object(NonMatching, "ef/eft_res.cpp"),
            # Registered once, at its final home (docs/plan.md 12).  The
            # `proposal/800F95A4_fn_800F95A4` range: the game's `effect.cpp` manager, named from the
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
            # Registered once, at its final home (docs/plan.md 12).  proposal/80105314_fn_80105314:
            # a maximal unclaimed run, seam unproven; class 4 decided the name (the map's
            # fn_80105314 stem, the scheme the bracketing fn_80104BD0/fn_8010D1A8 units use) and
            # the range holds several original effect families - see the unit's file header.
            Object(NonMatching, "ef/fn_80105314.cpp"),
            Object(NonMatching, "ef/eft013_fx.cpp"),
            # Registered from proposal/8010BDE4_fn_8010BDE4.cpp (a 0x8010BDE4 run discovery proposed).
            # The range's own symbols are plain fn_XXXXXXXX (rule 7 deferred); it is built as C++
            # because every callee it reaches is a C++ mangling (rule 9) - see the unit's header.
            Object(NonMatching, "ef/fn_8010BDE4.cpp"),
            Object(NonMatching, "ef/fn_8010D1A8.c"),
            # Registered once, at its final home (docs/plan.md 12).  The
            # `proposal/801121DC_eft019_set__FPQ34nw4r4math4VEC3UcUc` range: named from the runtime
            # dump's own `eft019_set` (dumpmap.py), C++ from its mangled definition.
            Object(NonMatching, "ef/eft019.cpp"),
            Object(NonMatching, "ef/fn_80114E34.cpp"),
            Object(NonMatching, "ef/eft022_fx.cpp"),
            Object(NonMatching, "ef/fn_8011722C.c"),
            # Registered once, at its final home (docs/plan.md 12).  The
            # `proposal/801173AC_fn_801173AC.cpp` range (`.text` 0x801173AC..0x80119C44, 30 functions
            # / 10392 B): the tail of the eft024 job machine, the whole eft025 player family, the whole
            # eft026 enemy family and the head of eft028.  Two of the range's own definitions are
            # manglings (`eft026_set__FP4_PLWUcUlUl`, `eft028_set_koware__FUcPQ34nw4r4math4VEC3Ucl`), so
            # it is built as C++ and every plain `fn_XXXXXXXX` definition is `extern "C"`.  Sections:
            # extab 0x8000C49C..0x8000C554, extabindex 0x800265D4..0x800266E8,
            # .text 0x801173AC..0x80119C44.
            Object(NonMatching, "ef/fn_801173AC.cpp"),
            Object(NonMatching, "ef/eft026_fx.cpp"),
            Object(NonMatching, "ef/fn_80119C44.c"),
            # Registered once, at its final home (docs/plan.md 12).  The `proposal/80119DEC_fn_80119DEC`
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
            # `proposal/8033F270_fn_8033F270` range (`.text` 0x8033F270..0x803432B4, 46 functions /
            # 16452 B): the runtime dump's own `eft050_set` at 0x80342F34 names the TU (dumpmap.py;
            # every other address is the dump's `zz_XXXXXXXX_` placeholder), so the module is `ef`
            # and the file follows the `eft00X.cpp` scheme of the neighbours.  Sections: extab
            # 0x80016CA4..0x80016DB4, extabindex 0x8003615C..0x800362F4, .text
            # 0x8033F270..0x803432B4 - exactly the bytes the bracketing units leave unclaimed.  The
            # seam is unproven (one maximal unclaimed run); see the unit's file header.
            Object(NonMatching, "ef/eft050.cpp"),
            Object(NonMatching, "ef/fn_803066F0.c"),
            # Registered once, at its final home (docs/plan.md 12) from
            # proposal/8030681C_fn_8030681C.cpp, and re-drawn by the seam round (`.text`
            # 0x8030681C..0x80308FB4, 32 functions / 10136 B): the eft041/042 effect machine, whose
            # first body drives fn_803066F0's `_EFT` record and which defines `eft042_set2`.  The
            # `menu_infomation.cpp` `__FILE__` string's referrer set puts the seam at 0x80308FB4 (the
            # screen bodies above it belong to `menu/menu_infomation.cpp`), so this range keeps only
            # the two screen entry points below it (`fn_80308EC0`/`fn_80308F1C`).  Class 4 decided
            # the name (the map's own fn_8030681C stem) and class 2 the module (`ef`, the left
            # bracket).  C++; every plain `fn_` definition is `extern "C"`.  Sections: extab
            # 0x80015AB4..0x80015B54 (20 records), extabindex 0x80034674..0x80034764 (20 x 12 B).
            Object(NonMatching, "ef/fn_8030681C.cpp"),
            # proposal/803432B4_fn_803432B4.cpp: the `_EFT` family at `.text` 0x803432B4..0x80349DD8
            # (92 functions / 0x6B24 bytes).  Module `ef` from the code: the range's `self` is the
            # 0x48-byte `_EFT` field for field (`flag_0x01`, `state_0x05`, `field_0x06`,
            # `timer_0x0C`, `pos_0x18`, `work_0x38`, `area_0x44` - the `include/ef.h` layout), it
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
            # Registered from proposal/80366618_fn_80366618.cpp, whose 0x80366618..0x8036CF64 range is a
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
        # Promoted from auto/8009AA78_fn_8009AA78.c (docs/plan.md: an auto unit stops being scaffolding).
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
            # Phase 4 stubs (docs/splits/phase4, window e): MSL/runtime candidate units with no bodies yet.
            Object(NonMatching, "MSL/strlen.cpp"),
            Object(NonMatching, "Runtime.PPCEABI.H/__va_arg.cpp"),
            Object(NonMatching, "Runtime.PPCEABI.H/global_destructor_chain.c"),
            Object(NonMatching, "Runtime.PPCEABI.H/CPlusLibPPC.cpp"),
            Object(NonMatching, "Runtime.PPCEABI.H/runtime.cpp"),
            Object(NonMatching, "MSL_C/alloc.cpp"),
            # Phase 4 stubs (docs/splits/phase4, window fg): the MSL libm (fdlibm) units after MSL_C/alloc.
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
        # Method: docs/matching.md 17 (cross-family version matrix).
        "mw_version": "GC/3.0a3",
        "cflags": cflags_rso,
        "host": False,
        "objects": [
            Object(NonMatching, "RSO/runtime.c"),
        ],
    },
{
        "lib": "Pl",
        # Wii/1.0 (mwcc 4.3 build 145): game code, neither runtime-style code (Wii/1.3) nor a REL.
        # Both ranges come from tools/splits/tudiscover.py: the must-link anchors are .sdata2 pools
        # (`lbl_8079A03C` for pl_skill.c, `lbl_8079A0AC` for pl_act.c), so the extents are lower
        # bounds and `NonMatching` keeps the original bytes in the link until a unit actually matches.
        # 50 of the region's 372 functions are pinned; the rest have no layout evidence and stay in
        # auto units on purpose (see the `tu-boundary-discovery` skill).
        "mw_version": "Wii/1.0",
        "cflags": cflags_pl,
        "progress_category": "game",
        "host": False,
        "objects": [
            # Registered once, at its final home (docs/plan.md 12): proposal
            # `80230FBC_fn_80230FBC` - the player work's motion-kind dispatch plus four of its nine
            # per-kind motion banks (`.text` 0x80230FBC-0x802373AC, 4 functions, 25584 B) with
            # extab 0x80011C84-0x80011CA4, extabindex 0x8002E9B0-0x8002E9E0 and the four
            # compiler-emitted jump tables in `.data` 0x805C1F94-0x805C2C60.  Home is `Pl`: the
            # first argument goes straight to `Get_motion_no(_PLW*)`, the third/fourth fields are
            # the player's `_se_w` works (+0xAF4/+0xAF8/+0xAFC) and the banks' callees are the Pl
            # SE helpers.  No `__FILE__` string covers the range (its own .data pool is jump tables
            # only), so the stem is the map's `fn_80230FBC` with a rule-7 deferral.  It uses
            # `cflags_pl` (this lib).
            Object(Matching, "Pl/fn_80230FBC.cpp"),
            # Registered once, at its final home (docs/plan.md 12): proposal
            # `802373AC_fn_802373AC` - the third and fourth of the player work's per-motion SE banks
            # (`.text` 0x802373AC-0x8023C2D0, TWO functions, 20260 B) with extab
            # 0x80011CA4-0x80011CB4, extabindex 0x8002E9E0-0x8002E9F8 and their two
            # compiler-emitted jump tables in `.data` 0x805C2C60-0x805C34D4 (271 + 270 entries).
            # Home is `Pl`: both functions pass their first argument straight to
            # `Get_motion_no(_PLW*)`, the three `_se_w` fields they load are `_PLW`+0xAF4/+0xAF8/
            # +0xAFC (the same pair of banks as the sibling `Pl/fn_80230FBC.cpp` next door) and
            # their callees are the Pl SE helpers.  No `__FILE__` string covers the range (its own
            # .data pool is the two jump tables and nothing else), so the stem is the map's
            # `fn_802373AC` with a rule-7 deferral.  It uses `cflags_pl` (this lib).
            Object(NonMatching, "Pl/fn_802373AC.cpp"),
            Object(Matching, "Pl/fn_80229ECC.cpp"),
            # `8023C2D0_fn_8023C2D0` - the other half of the player motion -> SE frame dispatcher
            # family (0x8023C2D0-0x80241558, 0x50B8 B, TWO functions).  Home is `Pl` and the stem is
            # the map's `fn_8023C2D0` with a rule-7 deferral: both functions dispatch on
            # `Get_motion_no__FP4_PLW` and `fn_8023C2D0` gates two arms on `Pl_act_ck__FP4_PLWUcUs`,
            # no `__FILE__` string covers the range and `dumpmap.py` has only `zz_` placeholders.
            # The unit owns its two `.data` jump tables (270 entries at 0x805C34D4, 261 entries at
            # 0x805C390C = 0x805C34D4-0x805C3D20) and its extab/extabindex pair.  It uses
            # `cflags_pl` (this lib), and the object is byte-identical to the target (.text 0x5288,
            # .data 0x84C, extab 0x10, extabindex 0x18; both functions 100.0 %).  It stays
            # `NonMatching` for a LINK reason, not a code one: MWCC emits the two jump tables as an
            # 8-byte-aligned `.data` section while retail's first table sits at the 4-mod-8 address
            # 0x805C34D4 (the split warns about it), so mwld pads the section up to 0x805C34D8 and the
            # DOL goes red - measured: `Matching` -> main.dol sha1 5324C567..., 403822 bytes differ.
            # See the unit header and this claim's outbox (`shared-file`).
            Object(Matching, "Pl/fn_8023C2D0.cpp"),
            # Registered once, at its final home (docs/plan.md 12): proposal
            # `80241558_fn_80241558` - the player motion -> SE frame dispatcher, ONE function
            # (0x80241558-0x802430E8, 0x1B90 B) whose ~58-case switch on `Get_motion_no(_PLW*)` is
            # compiled to a 261-entry `.data` jump table (`jumptable_805C3D20`, 0x805C3D20-0x805C4134).
            # Home is `Pl`: the first argument is passed straight to `Get_motion_no`, whose map
            # spelling is `Get_motion_no__FP4_PLW`, and the sibling switch `fn_8023C2D0` calls
            # `Pl_act_ck__FP4_PLWUcUs`.  No `__FILE__` string covers the range (the .data pool around
            # the jump table carries none), so the stem is the map's `fn_80241558` with a rule-7
            # deferral.  It uses `cflags_pl` (this lib).
            Object(Matching, "Pl/fn_80241558.cpp"),
            Object(NonMatching, "Pl/player_control.cpp"),
            # Registered once, at its final home (docs/plan.md 12): proposal
            # `8024F200_fn_8024F200` - the player's per-act state machine cluster
            # (0x8024F200-0x80258FCC, 92 functions, 40396 B).  Home is `Pl`: every function takes the
            # player work (`_PLW*`) and the per-part index byte, reads the act step byte at `_PLW`+0x005
            # and drives one step through `Pl_Skill_ck`/`Pl_cat_skill_ck`/`Pl_frame_check` and the
            # `Pl` motion helpers.  No `__FILE__` string covers the range (the `.data` pool between
            # `enemy_control.cpp` at 0x805A1BB8 and `menu_item.cpp` at 0x805CDFC8 carries none for the
            # whole band), so the stem is the map's `fn_8024F200` with a rule-7 deferral.  It uses
            # `cflags_pl` (this lib).
            Object(NonMatching, "Pl/pl_act_step.cpp"),
            Object(NonMatching, "Pl/pl_coll.cpp"),
            Object(NonMatching, "Pl/pl_act.cpp"),
            Object(NonMatching, "Pl/pl_act_step_data.cpp"),
            # Registered once, at its final home (docs/plan.md 12): proposal `802840DC_fn_802840DC`.
            # 54 functions, 0x802840DC-0x80288CEC (0x4C10 B).  Home is `Pl`: the band's callees are
            # all Pl API (`Get_motion_no__FP4_PLW`, `Pl_get_gunner_pos`/`Pl_get_gunner_vec`,
            # `Pl_atk_act_flag_ck`, `Pl_Skill_ck`, `Pl_frame_check`, `Pl_master_ck`) and its two
            # registered neighbours (`Pl/pl_act.cpp` below 0x8027D684, `Pl/fn_80288CEC.cpp` at
            # 0x80288CEC) are Pl units.  No `__FILE__` string covers the range (its own `.data` pool
            # is jump tables only) and the runtime dump answers `zz_XXXXXXXX_` placeholders
            # (`tools/symbols/dumpmap.py`), so the stem is the map's `fn_802840DC` with a rule-7
            # deferral - the sibling class-4 pattern of `Pl/fn_8026FFBC.cpp` / `Pl/fn_80288CEC.cpp`.
            # It uses `cflags_pl` (this lib).
            Object(NonMatching, "Pl/fn_802840DC.cpp"),
            Object(NonMatching, "Pl/pl_motion.cpp"),
            Object(NonMatching, "Pl/pl_hit_sphere.cpp"),
            # Re-cut 2026-09-29 out of `enemy/em024_ai.cpp`: the player act-state handlers of
            # `_PLW::field_0x002` class 3 (`Pl/fn_80258FCC.cpp`'s `jumptable_805C5648` entry 3 is
            # this band's dispatcher `fn_8034EE18`), `.text` 0x8034D2B0..0x8034F138 (24 functions /
            # 7816 B), extab 0x800170F4..0x800171A4, extabindex 0x800367D4..0x800368DC, `.data`
            # 0x805E9248..0x805EBBE0, `.sdata` 0x80793308..0x80793330 and `.sdata2` 0x8079B3A8..
            # 0x8079B3C8.  The file name is a GUESS (the selector's meaning is unproven); evidence
            # and the data caveat are in the file header.  It uses `cflags_pl` (this lib).
            Object(NonMatching, "Pl/pl_act_class3.cpp"),
            # The equipment sway band below the shell pool: `.text` 0x802ABD28..0x802AD9C0, extab
            # 0x80013AC4..0x80013B0C, extabindex 0x800316A4..0x80031710, `.ctors` 0x8056F378 (its own
            # static initialiser), `.data` 0x805CE638..0x805CED40, `.sdata` 0x807922C0..0x807922E0,
            # `.bss` 0x806B8798..0x806B87C0, `.sdata2` 0x8079A410..0x8079A448 - see `Pl/pl_yure.cpp`.
            Object(NonMatching, "Pl/pl_yure.cpp"),
        ],
    },
    {
        "lib": "g3d",
        "mw_version": "Wii/1.3",
        "cflags": cflags_g3d,
        "host": False,
        "objects": [
            # 36-byte `ResAnmAmbLight`-cluster accessor.  Re-homed from the mis-named
            # `g3d/g3d_resanmamblight.c` when the wQ-recut carved the real `g3d_resanmamblight.cpp`
            # (0x80089F94) out of auto/800898B0: its neighbours cite `g3d_anmscn.cpp` and the region's
            # data fragment is g3d_anmscn.cpp's, so the file took its own TU's name.  Real C++ since the
            # wR-mangling pass: the map's `fn_800680A8`/`fn_80066C8C` were placeholders, so the source mangles
            # and the MAP was renamed to `fn_800680A8__FPv`/`fn_80066C8C__FPv` (playbook 48); the object is
            # byte-identical, so the DOL hash holds.
            Object(Matching, "g3d/g3d_anmscn.cpp"),
            # Registered once, at its final home (docs/plan.md 12): proposal
            # `8005ABD8_disp_beta_tex__FP3VecP4Vec28_GXColorP9_GXTexObj` - the nw4r g3d character-
            # animation TU plus the game's font/debug-print accessors (183 functions / 0x8CB0 B,
            # 0x8005ABD8..0x80063888).  Its own `nw4r::db::Panic` asserts pass the bare source name
            # `"g3d_anmchr.cpp"` (`.data` 0x8058B410, 98 sites, 0x8005CF50..0x8006385C) with the
            # `g3d_resnode_ac.h`/`g3d_anmobj.h`/`g3d_resanmchr_ac.h` cluster beside it, so the lib is
            # `g3d` and the file takes its evidenced TU name (class 1 in the brief) rather than the map's
            # `disp_beta_tex` stem.  The range is a capped slice: its first ~0x2338 B (0x8005ABD8..
            # 0x8005CF10) carry no g3d assert and read a different `.data` pool, so the seam is a proposal
            # cap, not a proven TU boundary - the header records it.
            Object(NonMatching, "g3d/g3d_anmchr.cpp"),     # 0x8005ABD8-0x80063888
            # Registered once, at its final home (docs/plan.md 12): proposal `800680CC` - the nw4r g3d
            # animation-object cluster that spans `g3d_anmscn.cpp` -> `g3d_anmshp.cpp` ->
            # `g3d_anmtexpat.cpp` -> `g3d_anmtexsrt.cpp` (each body's cited `__FILE__` string settles
            # the TU).  The first TU name already has the provisional `g3d/g3d_anmscn.cpp` home, so the
            # cluster keeps the map's own stem, as `g3d/fn_80063888.cpp` did beside it; internal seam
            # near 0x8006946C/0x800697D4/0x80069CF4 (see the file header).
            Object(NonMatching, "g3d/fn_800680CC.cpp"),   # 0x800680CC-0x8006EAC0
            # Registered from proposal/8006EE78_fn_8006EE78 (the 0x8006EE78 range).  The range spans
            # two original TUs, proven by their own `__FILE__` strings read out of orig/RMHE08/sys/main.dol:
            # fn_8006EE78's Panic cites `lbl_8058D7E0` = "g3d_calcmaterial.cpp", and fn_8006F738 - the
            # file's first body - cites `lbl_8058D938` = "g3d_calcview.cpp".  Each is registered once at
            # its final home; the seam is 0x8006F738 and is also the extab/extabindex boundary.
            Object(NonMatching, "g3d/g3d_calcmaterial.cpp"), # 0x8006EE78-0x8006F738
            Object(NonMatching, "g3d/g3d_calcview.cpp"),     # 0x8006F738-0x8007270C
            # Registered from proposal/8006EAC0_fn_8006EAC0 (the 0x8006EAC0-0x8006EE78 maximal
            # unclaimed run).  `g3d` from the region's own `__FILE__` string (`lbl_8058D6C0` =
            # "g3d_anmvis.cpp", reached by fn_8006EAC0/ECB4/ED84's nw4r::db::Panic asserts).
            Object(NonMatching, "g3d/g3d_anmvis.cpp"),     # 0x8006EAC0-0x8006EE78
            # The two boundary-defective auto/ units re-cut at their real TU seams - each was a bulk
            # attribution spanning three original TUs.  The lib's cflags are cflags_g3d; the source was
            # authored under cflags_main's `-inline noauto`, which cflags_g3d lacks, so the per-function
            # scores here are measured with the flag lane's fix still outstanding (see the cflags_g3d
            # note above and .pi/notes/g3d-flags.probe.py).
            Object(NonMatching, "g3d/g3d_calcvtx.cpp"),      # 0x8007270C-0x800736F8 (widened 2026-09-24 from 0x80073398; 0x8007270C-0x80073398 reconstructed, incl. the 80073180 handover, 0x80073398-0x800736F8 earlier cut)
            Object(NonMatching, "g3d/g3d_calcworld.cpp"),    # 0x800736F8-0x800746DC
            Object(NonMatching, "g3d/g3d_camera.cpp"),       # 0x800746DC-0x80075DCC
            # Registered once, at its final home (docs/plan.md 12): proposal `8008452C` - the maximal
            # unclaimed run 0x8008452C-0x800898B0 (202 functions).  Module `g3d` and source name
            # `g3d_state.cpp` come from the run's own `__FILE__` string (`.data` 0x8058F750 =
            # "g3d_state.cpp", 18 references - the file argument of the `nw4r::db::Panic` asserts from
            # fn_80084630 on).  The run's data fragment starts at 0x8058F750, exactly where
            # `g3d_scnroot.cpp`'s ends (0x8058F530-0x8058F74A), so the left seam 0x8008452C is the
            # scnroot|state TU boundary; the right seam is the registered `g3d/g3d_resanm.c` at
            # 0x800898B0 (tudiscover: strong).  Interior: a second `.data` fragment starts at
            # 0x8058FCE8 (`g3d_resvtx_ac.h`/`ResVtxFurVec`/`ResVtxTexCoord` strings plus the static
            # constructor fn_80088AD0 at .ctors 0x8056F2D4), so the last ~52 functions may be a second
            # TU; both interior cut candidates (0x80088AD0 / 0x80089330) are tudiscover "weak" only, so
            # the run is registered whole and the seam is left to settle (brief 8.3).
            Object(NonMatching, "g3d/g3d_state.cpp"),        # 0x8008452C-0x80088E24
            Object(NonMatching, "g3d/g3d_resvtx.cpp"),       # 0x80088E24-0x800898B0
            Object(NonMatching, "g3d/g3d_resanm.c"),         # 0x800898B0-0x80089F94
            Object(NonMatching, "g3d/g3d_resanmamblight.c"), # 0x80089F94-0x8008A220
            Object(NonMatching, "g3d/g3d_resanmcamera.cpp"),   # 0x8008A220-0x8008A664 (merged 2026-09-24 from the 0x8008A220-0x8008A28C `.c` cut: same `__FILE__` fragment and contiguous sections, one TU)
            # Registered once, at its final home (docs/plan.md 12): proposal `8008F6E8` - the nw4r
            # g3d fog animation-channel evaluator (2 functions, 0x1FC B, 0x8008F6E8-0x8008F8E4).
            # `g3d`/`.cpp` from the body's own `__FILE__` string (`.data` 0x805903F0 =
            # "g3d_resanmfog.cpp"); the right edge 0x8008F8E4 is the next TU's first body, which
            # cites "g3d_resanmlight.cpp" (.data 0x80590440), so the seam is proven.  See the file
            # header for the sections and the rule-7 deferral.
            Object(NonMatching, "g3d/g3d_resanmfog.cpp"),    # 0x8008F6E8-0x8008F8E4
            # Registered once, at its final home (docs/plan.md 12): proposal `8008F8E4` - the nw4r g3d
            # light-animation channel evaluator and the `ResAnmScn` light channel accessors
            # (31 functions / 0x1018 B, 0x8008F8E4-0x800908FC).  `g3d`/`.cpp` from the body's own
            # `__FILE__` string (`.data` 0x80590440 = "g3d_resanmlight.cpp", fn_8008F8E4's Panic file
            # argument).  The discovery cap 0x8008F8E4-0x80097D40 spans several original TUs (whose own
            # `__FILE__` strings - g3d_resanmscn.cpp at 0x800908FC, g3d_resanmtexsrt.cpp at 0x800916FC,
            # g3d_resfile.cpp at 0x80093990, g3d_resmat.cpp at 0x800947A4 - are the pinned seams of the
            # sibling proposals 800908FC/800916FC/80093990/800947A4), so this unit is registered at its
            # own evidenced extent only.  See the file header for the sections and the rule-7 deferral.
            Object(NonMatching, "g3d/g3d_resanmlight.cpp"),   # 0x8008F8E4-0x800908FC
            # Registered once, at its final home (docs/plan.md 12): proposal
            # `800908FC_fn_800908FC` - the nw4r `ResAnmScn` channel getters followed by the
            # `ResAnmTexPat` accessor/bind cluster (24 functions / 0xE00 B, 0x800908FC-0x800916FC).
            # The range's own `__FILE__` string is `g3d_resanmscn.cpp` (`.data` 0x80590700, the file
            # argument of every getter's Panic) and the next TU's is `g3d_resanmtexsrt.cpp`, so the
            # module is `g3d` and the name is the evidenced TU name; the seam between the two TUs sits
            # inside the range (see the unit's file header).  Flags are this lib's `cflags_g3d`.
            Object(NonMatching, "g3d/g3d_resanmscn.cpp"),  # 0x800908FC-0x800916FC
            # Registered once, at its final home (docs/plan.md 12): proposal
            # `80093990_fn_80093990` - the nw4r g3d `ResFile` translation unit (53 functions /
            # 0xE14 B, 0x80093990..0x800947A4).  Its own `nw4r::db::Panic` assert passes the bare
            # source name "g3d_resfile.cpp" (`.data` 0x80590BA0, the file argument of the
            # CheckRevision assert at the head of fn_80093990), so the lib is `g3d` and the file
            # takes its evidenced TU name (class 1 in the brief).  `langcheck` agrees: the name is a
            # `.cpp` and the `Panic__Q24nw4r2dbFPCciPCce` relocation is a C++ mangling, so the unit
            # is `src/g3d/g3d_resfile.cpp` in this lib.  Sections: `.text` 0x80093990-0x800947A4,
            # `extab` 0x800093A8-0x800094E0, `extabindex` 0x80022158-0x8002232C; the boundaries are
            # fn_800938EC before and fn_800947A4 after (the first body of the next TU, which cites
            # "g3d_resmat.cpp").
            Object(Matching, "g3d/g3d_resfile.cpp"),      # 0x80093990-0x800947A4
            # Registered once, at its final home (docs/plan.md 12): proposal `800947A4` - the nw4r g3d
            # `ResMat`/`ResTexSrt` resource TU (140 functions / 0x45B8 B, 0x800947A4-0x80098D5C).  `g3d`/
            # `.cpp` from the range's own `__FILE__` string (`.data` 0x80590D78 = "g3d_resmat.cpp", the
            # file argument of every `nw4r::db::Panic` assert in the range); the right edge 0x80098D5C is
            # `g3d_resnode.cpp`'s first body (tudiscover seam, class `source`), so the seam is proven.
            # Sections: `.text` 0x800947A4-0x80098D5C, `extab` 0x800094E0-0x80009850 (110 8-byte
            # unwind-only records) and `extabindex` 0x8002232C-0x80022854 (110 12-byte records).
            # See the file header for the sections, the rebuilt bodies and the rule-7 deferral.
            Object(NonMatching, "g3d/g3d_resmat.cpp"),      # 0x800947A4-0x80098D5C

            # Registered once, at its final home (docs/plan.md 12): proposal
            # `80098D5C_fn_80098D5C` - the nw4r g3d `ResNode` animation-result TU
            # (7 functions / 0x6A4 B, 0x80098D5C-0x80099400).  `g3d`/`.cpp` from the range's own
            # `__FILE__` string (`.data` 0x805915C0 = "g3d_resnode.cpp", the file argument of
            # fn_80098D5C/F6C/9178/9278's `nw4r::db::Panic` asserts; `langcheck` returns C++
            # conclusive).  The discovery cap 0x80098D5C-0x800997E0 is NOT one TU: it spans this file
            # and `g3d_resshp.cpp` (see below), so the unit is registered at its own evidenced extent
            # only.  Sections: `.text` 0x80098D5C-0x80099400, `extab` 0x80009850-0x80009880
            # (6 records), `extabindex` 0x80022854-0x8002289C (6 entries).
            Object(NonMatching, "g3d/g3d_resnode.cpp"),      # 0x80098D5C-0x80099400
            # Registered once, at its final home (docs/plan.md 12): the nw4r g3d `ResShp` TU -
            # proposal `80098D5C_fn_80098D5C`'s head plus proposal `800997E0_fn_800997E0`'s tail,
            # which are one file.  fn_80099400 - the range's first body - calls `fn_80077674` =
            # `ResShp::ref` on its own `this` (its assert names `.sdata` 0x807911F0 = "ResShp"), while
            # fn_800993B4 calls `fn_8005D218` = `ResNode::ref` (0x80791148 = "ResNode"), so 0x80099400
            # is the resnode|resshp boundary; the `g3d_resshp.cpp` `.data` fragment starts at
            # 0x80591618 and every assert string the two halves pass ("g3d_resshp.cpp" 0x80591618,
            # "g3d_resshp_ac.h" 0x80591708/748, "g3d_rescommon_ac.h" 0x80591780/7EC,
            # "g3d_restev_ac.h" 0x80591820, "g3d_restex_ac.h" 0x80591850) sits inside it, so both
            # proposals are registered here as ONE unit (one Object line, one splits.txt block).  The
            # tail's own four `__FILE__`-level asserts pass "g3d_resshp.cpp" (0x80591618) and its
            # bodies keep calling the head's `ResShp`/`ResTagDL` accessors.  The right edge 0x8009A748
            # is `g3d_cpu.cpp`'s first body (the next registered unit).  Sections: `.text`
            # 0x80099400-0x8009A748 (58 functions / 0x1348 B), `extab` 0x80009880-0x800099E0
            # (44 records, 0x160 B), `extabindex` 0x8002289C-0x80022AAC (44 entries, 0x210 B).
            Object(NonMatching, "g3d/g3d_resshp.cpp"),       # 0x80099400-0x8009A748
            # Registered once, at its final home (docs/plan.md 12): proposal
            # `8008A664_fn_8008A664` - the nw4r g3d `ResAnmChr` character-animation TU
            # (101 functions / 0x5084 B, 0x8008A664..0x8008F6E8).  Its own `nw4r::db::Panic` asserts pass
            # the bare source name "g3d_resanmchr.cpp" (`.data` 0x80590010, the file argument of every
            # assert in the range) with the value-type format strings and the `g3d_resanmchr_ac.h`
            # inlined-assert header beside it, so the lib is `g3d` and the file takes its evidenced TU
            # name (class 1 in the brief).  `langcheck` agrees: the name is a `.cpp` and the `Panic`
            # relocation is a C++ mangling, so the unit is `src/g3d/g3d_resanmchr.cpp` in this lib.
            # Sections: `.text` 0x8008A664-0x8008F6E8, `extab` 0x80008F10-0x80009100 (62 records),
            # `extabindex` 0x80021A74-0x80021D5C (62 entries); the boundaries are the functions before
            # (fn_8008A644) and after (fn_8008F6E8).
            Object(NonMatching, "g3d/g3d_resanmchr.cpp"),   # 0x8008A664-0x8008F6E8
            # Registered once, at its final home (docs/plan.md 12): proposal
            # `800916FC_fn_800916FC` - the nw4r g3d `ResAnmTexSrt` SRT-animation TU and the inline
            # resource-accessor bodies emitted beside it (75 functions / 0x2294 B,
            # 0x800916FC-0x80093990).  Its own `nw4r::db::Panic` asserts pass the bare source name
            # "g3d_resanmtexsrt.cpp" (`.data` 0x80590928, the file argument of all four asserts in
            # fn_800916FC), so the lib is `g3d` and the file takes its evidenced TU name (class 1 in
            # the brief).  `langcheck` agrees: the name is a `.cpp` and the `Panic` relocation is a
            # C++ mangling, so the unit is `src/g3d/g3d_resanmtexsrt.cpp` in this lib.  The run also
            # cites the `g3d_res*_ac.h` inlined-assert headers, which share the same `.data`
            # fragment, so they are this TU's inlined accessor bodies, not a second object.  Sections:
            # `.text` 0x800916FC-0x80093990, `extab` 0x80009208-0x800093A8 (52 records),
            # `extabindex` 0x80021EE8-0x80022158 (52 entries).  See the file header.
            Object(NonMatching, "g3d/g3d_resanmtexsrt.cpp"), # 0x800916FC-0x80093990
            # Registered from proposal/800D77B0_fn_800D77B0 (the 0x800D77B0 two-function run).  `g3d`
            # from `g3d/g3d_calcworld.cpp`, which calls fn_800D77B0 (`fn_800737CC`'s per-node matrix
            # builder) and names it; the range's own data has no `__FILE__` string, so the name stays
            # the map's `fn_` stem (docs/plan.md 12, the register-once rule).
            # The `g3d_xsi.cpp` TU (0x800D6C00-0x800D77B0), carved out of `nw_resource.cpp`'s tail on 2026-09-30 (its own `__FILE__` string).
            Object(NonMatching, "g3d/g3d_xsi.cpp"),         # 0x800D6C00-0x800D77B0
            Object(NonMatching, "g3d/fn_800D77B0.cpp"),     # 0x800D77B0-0x800D79B4
            # The `g3d_basic.cpp` SRT/matrix cluster, named by its own `__FILE__` string
            # (`lbl_80595840` = "g3d_basic.cpp", reached by fn_800D79B4's `nw4r::db::Panic` asserts).
            Object(NonMatching, "g3d/g3d_basic.cpp"),        # 0x800D79B4-0x800D7F54
            # Registered once, at its final home (docs/plan.md 12): proposal `80063888` - the nw4r g3d
            # animation-object cluster (164 functions / 0x4820 B, 0x80063888..0x800680A8).  The run spans
            # more than one original TU (`g3d_anmobj.cpp`/`g3d_anmclr.cpp` in its head, `g3d_anmscn.cpp`
            # from 0x800649CC), so it keeps the map's `fn_80063888` stem (brief evidence class 4); the
            # module is `g3d` and the lib's flags are cflags_g3d.  See the file header for the seam and
            # the rule-2 owner header `include/g3d/fn_80063888.h`.
            Object(NonMatching, "g3d/fn_80063888.cpp"),       # 0x80063888-0x800680A8
            # Registered once, at its final home (docs/plan.md 12): proposal `8005AA28` - the nw4r g3d
            # `ResMat` accessor cluster (0x8005AA28..0x8005ABD8, 8 functions).  Its own `__FILE__`
            # string is the accessor header `g3d_resnode_ac.h` (lbl_8058B370, the Panic file argument of
            # fn_8005AA44), which names the header an inline assert was written in, not the unit - no
            # `.cpp` string exists for the range, so the file keeps the map's `fn_` stem (class 4).  The
            # module is `g3d`: every caller is nw4r g3d (`ScnMdl::CopiedMatAccess`, g3d_calcworld.cpp's
            # fn_80073E8C, g3d_basic.cpp's fn_800D7ED0 twin).  See the file header.
            Object(NonMatching, "g3d/fn_8005AA28.cpp"),       # 0x8005AA28-0x8005ABD8
            # Registered once, at its final home (docs/plan.md 12): proposal `80075DCC` - the nw4r g3d
            # render/dispatch cluster (216 functions / 0x6774 B, 0x80075DCC..0x8007C540).  The run spans
            # five original TUs (g3d_dcc.cpp, g3d_draw1mat1shp.cpp, g3d_draw.cpp, g3d_fog.cpp,
            # g3d_light.cpp - `tools/units/attribution-queue.json`), so it keeps the map's `fn_80075DCC`
            # stem (brief evidence class 4); the module is `g3d` and the lib's flags are cflags_g3d.  The
            # left edge 0x80075DCC is a tudiscover strong cut, the right edge 0x8007C540 is the proposal
            # cap, not a seam.  See the file header and `include/g3d/fn_80075DCC.h` (rule 2).
            Object(NonMatching, "g3d/fn_80075DCC.cpp"),      # 0x80075DCC-0x8007C540
            # Registered once, at each real TU's own home (docs/plan.md 12), from the pooled proposal
            # `8007C540` (185 functions / 0x7FEC B, 0x8007C540-0x8008452C).  The proposal is NOT one TU:
            # its own `.data` pool pins four different `__FILE__` strings to four disjoint function runs
            # (`g3d_scnmdl.cpp` at 0x8058EDA0, `g3d_scnmdlsmpl.cpp` at 0x8058F0A0, `g3d_scnobj.cpp` at
            # 0x8058F3D8, `g3d_scnroot.cpp` at 0x8058F530 - four consecutive per-TU `.data` fragments),
            # so each is registered at its own home with its own section ranges.  The three small runs
            # between the anchors (0x8007EF1C, 0x800810DC, 0x80082668) each hold the *name-record reader*
            # of the class the neighbouring file defines (ScnMdl / ScnMdlSimple / ScnObj+ScnLeaf+ScnGroup),
            # so each is allocated to that class's file; the extab, extabindex and `.text` ranges are then
            # contiguous and gapless across the four units.  See each file's header for the seams.
            Object(NonMatching, "g3d/g3d_scnmdl.cpp"),       # 0x8007C540-0x8007F0E4
            Object(NonMatching, "g3d/g3d_scnmdlsmpl.cpp"),   # 0x8007F0E4-0x800813B8
            Object(NonMatching, "g3d/g3d_scnobj.cpp"),       # 0x800813B8-0x800827E4
            Object(NonMatching, "g3d/g3d_scnroot.cpp"),      # 0x800827E4-0x8008452C
            # Registered once, at its final home (docs/plan.md 12): proposal
            # `8009A748_fn_8009A748` - the nw4r g3d CPU-side display-list copy/fill helpers
            # (2 functions / 0x330 B, 0x8009A748..0x8009AA78).  `g3d`/`.cpp` from the range's own
            # `__FILE__` string (`.data` 0x80591860 = "g3d_cpu.cpp", the file argument of every
            # `nw4r::db::Panic` assert in both bodies), so the lib is `g3d` and the file takes its
            # evidenced TU name (class 1 in the brief).  `langcheck` agrees: the name is a `.cpp` and
            # the `Panic__Q24nw4r2dbFPCciPCce` relocation is a C++ mangling, so the unit is
            # `src/g3d/g3d_cpu.cpp` in this lib.  Every caller of the range is nw4r g3d
            # (fn_80075DCC, g3d_state.cpp, g3d_resfile.cpp and the g3d_resmat band).  Sections:
            # `.text` 0x8009A748-0x8009AA78, `extab` 0x800099E0-0x800099F0, `extabindex`
            # 0x80022AAC-0x80022AC4; the boundaries are fn_8009A720 before (a different TU) and
            # gx/fn_8009AA78.c at 0x8009AA78 after.
            Object(NonMatching, "g3d/g3d_cpu.cpp"),           # 0x8009A748-0x8009AA78
            # Registered once, at its final home (docs/plan.md 12): proposal
            # `8009B140_fn_8009B140` - the nw4r g3d GPU-path texgen helpers (2 functions / 0x234 B,
            # 0x8009B140..0x8009B374).  `g3d`/`.cpp` from the range's own `__FILE__` string
            # (`.data` 0x80591900 = "g3d_gpu.cpp", the `pFile` argument of fn_8009B140's
            # `nw4r::db::Panic` assert), the same class-1 evidence its sibling `g3d_cpu.cpp` used, so
            # the lib is `g3d` and the file takes its evidenced TU name.  Section claim:
            # `.text` 0x8009B140..0x8009B374, `extab` 0x80009A28..0x80009A38, `extabindex`
            # 0x80022B18..0x80022B30 (gapless against gx/fn_8009ACE4.c below them).  The right edge
            # 0x8009B374 is the discovery byte cap, not a proven TU end - the file's header records it.
            Object(NonMatching, "g3d/g3d_gpu.cpp"),           # 0x8009B140-0x8009B374
        ],
    },
    {
        "lib": "Network",
        "mw_version": "Wii/1.3",
        "cflags": cflags_network,
        "host": False,
        "objects": [
            # Registered once, at its final home (docs/plan.md 12), the Capcom `Network` reconnaissance
            # lane (branch worker/net-capcom).  Five units; `constructNetworkWiiMediator` flipped and was later
            # folded into `Network/sNetworkLibraryWii.cpp` (its TU), `initNetworkSessionStable` was folded into `Network/NetworkSessionManagerPat.cpp` at
            # phase 4 (its per-unit -O3 evidence below is kept), the other three are `Object(NonMatching, ...)`.
            #   Network/initNetworkSessionStable.cpp (folded)  .text 0x803DEA30..0x803DEB38 (264 B, 1 fn)
            #   Network/network_state.cpp              .text 0x803FE8E4..0x804006A8 (8900 B, 21 fn)
            #   Network/NetworkWiiMediator.cpp         .text 0x80413C64..0x804155D4 (6490 B, 79 fn)
            #   Network/constructNetworkWiiMediator.cpp  .text 0x80418988..0x804189C8 (64 B, 1 fn)
            #   Network/NetworkPat.cpp                 .text 0x80419EC4..0x8041A170 (328 B, 11 fn)
            # The *seams*: NetworkWiiMediator's left edge 0x80413C64 is the lane's only two-signal
            # (strong) cut (`.data` jumptable run jump); every other edge is the weak closure edge and
            # is recorded as unproven in each file's header.  `Network/NetworkWiiMediator.cpp` folds in
            # the retired `Network/NetworkWiiMediator.c` (`fn_80413F3C` at 0x80413F3C) so the whole
            # class band has one owner; the fold-in is part of this batch, not a later one.
            # Per-unit flag deviation (brief section 8.2), instruction-level evidence: the two
            # session-manager units are `-O3` scheduling, like the sibling `Network/NetworkSessionManager.cpp`
            # below.  With the lib's `-O4,p` every framed function hoists its constant-argument
            # setup into the prologue's `mflr`->`stw` latency slot (sendReqCommonKey: the target is
            # `stw r0,20; stw r31,12; stw r30,8; mr r30,r3; li r4,18; li r5,0; bl` while ours puts
            # the two `li` before the saves), and the FMP/request switch tails are laid out
            # unsorted.  Measured over the same source: `Network/network_state.cpp` 69.42 % at
            # `-O4,p` -> 81.93 % at `-O3` (five more functions at 100 %),
            # `Network/initNetworkSessionStable.cpp` 81.39 % -> 100.00 % (re-measured on the landed
            # source; the numbers as first landed were 63.26 % -> 92.88 %).  `-func_align 4` is kept
            # from `cflags_network` (the range packs on 4-byte boundaries).
            # Matching: the 264-byte `.text`, `extab` 0x18 and `extabindex` 0xC are all byte-identical
            # to the target object and all ten relocations name the same symbols at the same offsets
            # (`__nw__FUl` at .text+0x34, `constructNetworkSessionObject` at +0x44,
            # `networkSessionReflectCallback` at +0x66/+0x6A, the three float constants at
            # +0x8C/+0xA4/+0xBC; `__dl__FPv` at extab+0x14; `initNetworkSessionStable` and
            # `@etb_8001A630` in extabindex).  The extab record and the last `.text` byte are source
            # shapes, not flags - the `new` expression and the file-scope `#pragma peephole off`, both
            # with their measured evidence in the unit header.
            Object(NonMatching, "Network/network_state.cpp",
                   cflags=[f for f in cflags_network if f != "-O4,p"] + ["-O3"]),
            # Registered once, at its final home: proposal `803E44C8_fn_803E44C8` (`.text`
            # 0x803E44C8..0x803E4888, 1 function / 960 B) - `NetworkLayerPat`'s request state machine
            # (vtable slot +0x104 of `lbl_805FC1E0`), with its own extab/extabindex record.  The name is a
            # GUESS (unit header).  `-O3` like the sibling session units: measured with the lib's
            # `-O4,p` (everything else equal) the unit's one function scores 90.14167 %, against 100.00000 % at `-O3`.
            Object(NonMatching, "Network/NetworkLayerPatStep.cpp",
                   cflags=[f for f in cflags_network if f != "-O4,p"] + ["-O3"]),
            # Phase 4 stubs (docs/splits/phase4, window e): units of the reconciled candidate with no bodies yet.
            # Each source's header says what the range is and what is unknown; flags are the lib default, unmeasured.
            Object(NonMatching, "Network/NetworkCommunityPat.cpp"),
            Object(NonMatching, "Network/PatInterface.cpp"),
            Object(NonMatching, "Network/network_layer_io.cpp"),
            # Per-object flag deviation (brief section 8.2), instruction-level evidence: measured over
            # the unit's 79 rows, the lib's `-O4,p` leaves getAccountBan/Warning/WaitQueue at 42.86 and
            # getReflectName3C at 51.28 where `-O3` puts all four at 100.00, and `-inline auto` scores
            # validateReflectName 0.00 where `-inline noauto` scores it 88.28 - no other function moves under
            # either setting.  Same finding as the lib's other units.  Evidence: the unit header of
            # src/Network/NetworkWiiMediator.cpp.
            Object(NonMatching, "Network/NetworkWiiMediator.cpp",
                   cflags=[f for f in cflags_network if f not in ("-O4,p", "-inline auto")] + ["-O3", "-inline noauto"]),
            # Per-object flag deviation (brief section 8.2), instruction-level evidence: measured over the 22
            # `sNetworkLibrary` rows, the lib's `-O4,p` scores ctor/dtor/final/the pool helpers 77-93 % (the
            # constant setup hoisted above the callee-save stores) and `-O3` puts all of them at 100 %.
            Object(NonMatching, "Network/network_opening.cpp",
                   cflags=[f for f in cflags_network if f != "-O4,p"] + ["-O3"]),
            # Registered once, at its final home: split out of `Network/network_opening.cpp` (the `.data` order
            # seam, unit header).  The base-class rows the `-O3` evidence above was measured on are this unit's.
            Object(NonMatching, "Network/sNetworkLibrary.cpp",
                   cflags=[f for f in cflags_network if f != "-O4,p"] + ["-O3"]),
            # Per-object flag deviation (brief section 8.2), instruction-level evidence: the sibling units' `-O3`
            # schedule.  Measured over the same first-pass source: 75.54058 % at the lib's `-O4,p` (every framed
            # body hoists its constant setup above the callee-save stores) against 91.81757 % at `-O3`, every row
            # equal or higher.  Evidence: the unit header of src/Network/sNetworkLibraryWii.cpp.
            # The folded-in `constructNetworkWiiMediator` (0x80418988) had the same deviation as its own unit:
            # 75.00000 at `-O4,p`, 100.00000 at `-O3` (`li r3,0x1408` after the callee-save stores).
            # `-inline noauto` since the fold (measured 2026-10-04): retail's `initNetworkLibrary` *calls*
            # `constructNetworkWiiMediator`; in one TU `-inline auto` folds it and the inlined constructor in
            # (256 B against 120 B, 100.00 -> 0.00); with `noauto` every one of the unit's 41 rows is unchanged.
            Object(NonMatching, "Network/sNetworkLibraryWii.cpp",
                   cflags=[f for f in cflags_network if f not in ("-O4,p", "-inline auto")] + ["-O3", "-inline noauto"]),
            # Per-object flag deviation (brief section 8.2), instruction-level evidence: the three
            # `deleteNetwork*Pat` helpers get the target's block order and the target's `bl` to the
            # sibling `clearNetwork*Pat` (retail calls it; `-inline auto` folds the 36-byte callee in
            # and costs the unit 4 bytes per helper), and the eight already-matching rows do not move.
            # Measured on the freshly written bodies: 2.85714 at the lib's `-O4,p`/`-inline auto`,
            # 99.71429 at `-O3`/`-inline noauto` (140 B, the target size).  The file's
            # `#pragma exceptions on` is not a flag change: the lib's `-Cpp_exceptions off` leaves our
            # object with no `extab`/`extabindex` while the target has 24 B / 36 B, and the pragma
            # reproduces both exactly without moving a `.text` byte.  The unit's `.text` right edge is
            # 0x8041A194, not the pre-fix 0x8041A170: the map's `clearNetworkLayerPat` starts at
            # 0x8041A170 (+0x24), so the old edge orphaned that function into an `auto_*` unit.
            Object(Matching, "Network/NetworkPat.cpp",
                   cflags=[f for f in cflags_network if f not in ("-O4,p", "-inline auto")] + ["-O3", "-inline noauto"]),
            # Per-object flag deviation (brief section 8.2), instruction-level evidence, the sibling units'
            # pair: measured over the unit's 13 written rows, the lib's `-O4,p`/`-inline auto` gives 68.95 %,
            # `-O3` 81.77 % (`reflectServiceStart` then inlines `resetReflectServiceTask`, which retail calls:
            # 40.54 -> 30.54), and `-O3`/`-inline noauto` puts 12 of them at 100 %.
            Object(NonMatching, "Network/NetworkReflectService.cpp",
                   cflags=[f for f in cflags_network if f not in ("-O4,p", "-inline auto")] + ["-O3", "-inline noauto"]),
            # Registered once, at its final home (docs/plan.md 12): proposal
            # `803D3CE8_NetworkSessionManager.cpp` (`.text` now 0x803D4904..0x803D70B8: the twelve `NetworkSessionStable`
            # op-code writers that opened the range moved to `Network/NetworkSessionStable.cpp`, whose
            # `.data` order proved the seam at 0x803D4904) - the Network session band: the
            # `NetworkSessionManager` request pool/state machine and the first `NetworkSessionManagerPat`
            # virtual slots.  Module `Network` from the class names and the registered neighbour
            # `Network/NetworkWiiMediator.cpp`; no `__FILE__` string and only `zz_` dump names cover the
            # range, and the tile spans more than one original TU, so the file keeps the map stem
            # (brief section 2, class 3 module + class 4 name).  C++ (mangled `__nw__FUl`/`__dl__FPv`),
            # exceptions off, so `.text` only.  The seam is unproven (discovery byte cap).
            # Per-unit flag deviation (brief section 8.2), instruction-level evidence: every framed
            # function in the range is -O3 scheduling - fn_803D53B0's prologue is `stw r31,28; stw r30,24;
            # mr r30,r3; mr r31,r4` and its global-descriptor copy is the plain `lwz/stw` block, both of
            # which `-O4,p` destroys (it interleaves the saves and folds the copy into `lwzu`).  Measured:
            # the same source scores fn_803D53B0 59.79 % at `-O4,p` and 95.15 % at `-O3`, fn_803D4904
            # 70.42 % -> 93.24 %, NetworkSessionManager 81.63 % -> 92.23 %.  `-func_align 4` is kept (the 4-byte
            # functions `fn_803D4B5C`/`fn_803D5D64` prove it).  This is the object's cflags, not the lib's:
            # the sibling `NetworkWiiMediator.cpp` is byte-identical at `-O4,p`.
            Object(NonMatching, "Network/NetworkSessionManager.cpp",
                   cflags=[f for f in cflags_network if f != "-O4,p"] + ["-O3"]),
            # The Network transport band, `.text` 0x803CCDF8..0x803D3CE8, registered as eight units -
            # one per translation unit (docs/network-transport-split.md holds the evidence and the
            # confidence of each cut: the retail `.data` interleaves each class's vtable with its own
            # strings, which one TU cannot emit, and the `extabindex` entries tile in the same order).
            # Module `Network` from the class names and the registered neighbours
            # (`Network/NetworkSessionManager.cpp` abuts the last one exactly in extab/extabindex); no `__FILE__`
            # string names any of them, so every file name is derived from the classes it holds and is a
            # GUESS (recorded in each unit's header).  C++ (`__dl__FPv`, virtual dispatch); the lib's
            # `-Cpp_exceptions on` supplies the target's extab/extabindex.
            # Per-unit flag deviation (brief section 8.2), instruction-level evidence, unchanged by the
            # split: `-O3` like the two sibling session units - measured on the combined unit, the lib's
            # `-O4,p` put 14 rows at 100 % (unit score 4.05), `-O3` put 37 of 38 there (unit score 7.37);
            # and `-pool off` on `network_socket_streams` alone (playbook 43) - the target materialises each log string with its own
            # `lis`/`addi` pair (`NetworkMultipleUdp_receive`: `lis r4,lbl_805F988C@ha` / `addi
            # r4,r4,lbl_805F988C@l`, then `lis r4,lbl_805F98C4@ha`), where the default pooling addresses
            # every string of a function through one `@stringBase0` register; toggling it moved one row
            # (`NetworkMultipleUdp_receive` 89.53 % -> 94.06 %) and lowered none.  Measured 2026-09-29: `-pool`
            # (not `-str`) is the lever - with it on, a function referencing three or more distinct `@NNN` strings addresses the
            # second off the first's base (`addi r28,r5,<first>` then `addi r4,r28,<delta>`); it is a no-op
            # on the other seven units of this group TODAY (each object is byte-identical without it), but they are
            # TUs of the same retail family and the session units still have ~60 unwritten rows, several with three or more
            # log strings in one function, so the flag stays on all eight rather than being re-discovered per row.
            Object(Matching, "Network/NetworkPeerBase.cpp",
                   cflags=[f for f in cflags_network if f != "-O4,p"] + ["-O3", "-pool off"]),
            Object(Matching, "Network/NetworkPeerBuffer.cpp",
                   cflags=[f for f in cflags_network if f != "-O4,p"] + ["-O3", "-pool off"]),
            Object(Matching, "Network/NetworkPeerUdp.cpp",
                   cflags=[f for f in cflags_network if f != "-O4,p"] + ["-O3", "-pool off"]),
            Object(NonMatching, "Network/NetworkPeerMcs.cpp",
                   cflags=[f for f in cflags_network if f != "-O4,p"] + ["-O3", "-pool off"]),
            Object(NonMatching, "Network/network_socket_streams.cpp",
                   cflags=[f for f in cflags_network if f != "-O4,p"] + ["-O3", "-pool off"]),
            Object(NonMatching, "Network/NetworkResolverWii.cpp",
                   cflags=[f for f in cflags_network if f != "-O4,p"] + ["-O3", "-pool off"]),
            Object(Matching, "Network/NetworkSessionBase.cpp",
                   cflags=[f for f in cflags_network if f != "-O4,p"] + ["-O3", "-pool off"]),
            Object(NonMatching, "Network/NetworkSessionStable.cpp",
                   cflags=[f for f in cflags_network if f != "-O4,p"] + ["-O3", "-pool off"]),
            # A **data-only** unit: the Network band's shared small-data pool, `.sdata2`
            # 0x8079C690-0x8079C758 (200 B) and `.sdata` 0x80793900-0x80793930 (48 B), whose source
            # defines nothing (playbook 23/53 route 2, playbook 54; the model is
            # `Pl/pl_frame_data.cpp`).  Both runs are read by more than one party -
            # the Network transport units and `Network/NetworkSessionManager.cpp` share
            # 0x8079C6EC..0x8079C754, and unsplit (Network) code reads 0x8079C690 and 0x8079C750 -
            # so no consumer may claim them without taking rows only the other consumer reads.
            # One owner is what lets every consumer include `Network/network_shared_data.h`
            # instead of declaring the words into its own file, which is the rule-12 finding those
            # 14 declarations were in `include/Network/NetworkSessionManager.h`.  Registered once, at its
            # final home; the `-O3` override above does not apply (no code in this unit).
            Object(NonMatching, "Network/network_shared_data.cpp"),
            # Registered once, at its final home (docs/plan.md 12): proposal
            # `8041A87C_fn_8041A87C.cpp` (`.text` 0x8041A87C..0x8041DF10, 71 functions / 13972 B).
            # Per-object flags (brief 8.2), instruction-level evidence: retail *calls* the small
            # file-static helpers from the big state machines - fn_8041DCDC's target body is
            # `lwz r3,0x6634; lwz r4,0x6638; bl fn_8041C9D8; extsb` (72 B) while `-inline auto` folds
            # the 128-byte callee into it (156 B), and fn_8041DD58/fn_8041CA94 grow the same way.
            # With `-inline noauto` the sizes land on the target and the unit's .text gap shrinks.
            # `-pool off` (playbook 43), instruction-level evidence from claiming the unit's `.data` string
            # run (2026-09-30): once the log strings are this TU's own literals the default pooling
            # addresses them off one `...data.0` base (`lis r5,...data.0@ha ; addi r30,r5,...@l`, then
            # `addi r4,r30,0x218`), where the target gives every string its own `lis`/`addi` pair
            # (`lis r4,lbl_806033C8@ha ; addi r4,r4,lbl_806033C8@l`).  Unit-level, measured 2026-09-30: 99.87 % / 64 rows at 100 with the flag, 98.21 % / 59 without it
            # (natNegCompletedCallback 89.78, gt2ConnectAttemptCallback 88.74, runNasLogin 92.46, startMatch 84.70 lower, none higher);
            # an object that owns log strings after a recut needs the same flag.
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
            # Phase 4 stubs (docs/splits/phase4, window fg): SDK candidate units with no bodies yet.
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
            # Registered once, at its final home (docs/plan.md 12) from
            # proposal/802FA9A0_fn_802FA9A0.cpp: the lobby event/status band
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
            # Registered once, at its final home (docs/plan.md 12) from
            # proposal/8030121C_fn_8030121C.cpp: the lobby UI band
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
            # proposal/80338808_fn_80338808.cpp: the lobby companion/status UI band
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
            Object(NonMatching, "lobby/lb_screen_step.cpp"),  # phase 4 stub (window d): new unit, cflags_lobby of the lib
            # Registered once, at its final home (docs/plan.md 12) from
            # proposal/80365C84_fn_80365C84.cpp: the lobby menu page's frame step and its
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
            # Registered from proposal/8038E8E8_fn_8038E8E8.cpp: the lobby list/detail UI band
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
            # Registered once, at its final home from proposal/803A3A50_fn_803A3A50 (`.text`
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
            # Phase 4 fold: the unit is 0x80423E74-0x80432104 - it absorbed the former `fn_80423E74.cpp`
            # work-record / PatCamellia band (0x80423E74-0x80429B94, 77 functions; same lib and `cflags_main`).
            # Registered once, at its final home (docs/plan.md 12): proposal
            # `80429B94_fn_80429B94` - the 0x80429B94-0x8043065C network pat-control band (114
            # functions, 27336 B) with extab 0x8001D368-0x8001D558 and extabindex 0x8003DE54-0x8003E0DC.
            # Game code that drives getPatsObject/getNetworkSessionManagerPat and reads the lobby
            # singleton `lobby_w`; no `__FILE__` string and no runtime-dump source name cover the
            # range, so the name is evidence class 3 - the band's own vocabulary (`getPatsObject`,
            # `getNetworkSessionManagerPat`, `updatePatInterface180`, `setPatField854`) plus the
            # module its four sibling units share - and the generated stem `fn_80429B94.cpp` is gone.
            # It takes the game-root `main` lib and cflags_main (Wii/1.3, -O3, -inline noauto,
            # -Cpp_exceptions on - the target object carries extab/extabindex).
            Object(NonMatching, "Network/network_pat_control.cpp"),
            Object(NonMatching, "Network/net_session_close.cpp"),  # phase 4 stub (link neighbour's lib and cflags_main)
            # Registered once, at its final home (docs/plan.md 12): the key function of
            # `NetworkSessionManagerPat` - `move` (0x803D70B8, 572 B), the head of the queue's
            # `803D70B8_fn_803D70B8` band.  MWCC emits a class's vtable in the TU that defines its key
            # function, and the target's `__vt__24NetworkSessionManagerPat` (0x805FB0F0) sits in this
            # band's own `.data` run (0x805FAAD0.., right after `Network/NetworkSessionManagerPat`'s
            # base table), so the unit claims the key function's `.text` extent, the table's `.data`
            # extent and the 1.0f circle-info interval at 0x8079C758.  Same game-root `main` lib and
            # cflags_main as its link neighbours (the object carries extab/extabindex for the rest of
            # the band, and C++ virtual dispatch).
            Object(NonMatching, "Network/NetworkSessionManagerPat.cpp"),
            # Phase 4 (docs/splits/phase4, window fg): the HOME-button (HBM) code in link order.  The units from 0x8052A040 to
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
