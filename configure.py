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

# Network and OS flags (src/Network/NetworkWiiMediator.c, src/OS/OSAlarm.c): cflags_base + -func_align 4.
# Evidence: both retail objects are `.text align 2**2`, while cflags_base's -O4,p implies -func_align 16, which
# is what our objects emit. Flipping NetworkWiiMediator with that alignment made the linker round the object's
# start up to the next 16-byte boundary: `dtk dol diff` reported fn_80413F3C expected at 0x80413F3C but found at
# 0x80413F40, every following symbol shifted by 4, and main.dol stopped matching build.sha1. Section sizes were
# already identical, so the alignment was the entire difference - the same finding as cflags_ppceabi above.
cflags_network = [*cflags_base, "-func_align", "4"]
cflags_os = [*cflags_base, "-func_align", "4"]

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
    # flag - the same finding as Pl, g3d and camellia. sys_mem.cpp in this lib already turns exceptions on
    # with a per-file pragma, so this only adds what that pragma would have (analysis: .pi/notes/extab-gap.md).
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
#     => -pool off
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
#     register each (r19, r21-r30) and never a shared base
#     => -pool off
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
    "-pool off",
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
                        Object(Matching, "ai/fn_802D0DCC.c"),
                        Object(NonMatching, "ai/fn_802CC794.cpp"),
                        Object(NonMatching, "ai/fn_802C474C.cpp"),
                # Registered once, at its final home (docs/plan.md 12): proposal
                # `802C5D10_fn_802C5D10.cpp` (`.text` 0x802C5D10..0x802CC794, 76 functions / 27268 B).
                # Module `ai` from the code - every function takes an `_AINPC_W` in r3, calls
                # `ai_skill_ck__FP8_AINPC_WUc` and drives the same `+0x170`/`+0x442`/`+0x444` offsets the
                # band above writes - and from the bracketing units (both `ai`).  No `__FILE__` string and
                # no runtime-dump name covers the range, so the file keeps the map's stem (brief section 2,
                # class 4; see the unit header).  Sections: .text 0x802C5D10..0x802CC794, extab
                # 0x80014484..0x8001467C (63 records), extabindex 0x8003252C..0x80032820 - both runs are
                # exactly the gap between the bracketing objects' runs, and the extabindex entries for
                # 0x802C5D10..0x802CC57C all point into the extab run.  No .data: the range's switch tables
                # (0x805D4964..0x805D4D38) sit between the two neighbours' runs and a data claim has to be
                # measured before and after (playbook 55).
                        Object(NonMatching, "ai/fn_802C5D10.cpp"),
                        # Registered from proposal/802D0F34_fn_802D0F34.cpp: the AI-NPC motion band
                        # (90 functions / 13760 B) directly above `ai/fn_802D0DCC.c`.  Module `ai`
                        # and the map's stem as file name (class 4 - the range's own manglings are
                        # `*_AINPC_W`, so the module is certain but no source name is evidenced).
                        # Sections: .text 0x802D0F34..0x802D44F4, extab 0x80014844..0x8001496C,
                        # extabindex 0x80032ACC..0x80032C88, .data 0x805D4E80..0x805D5000.
                        Object(NonMatching, "ai/fn_802D0F34.cpp"),
                        # Registered from proposal/802D44F4_fn_802D44F4.cpp (`.text`
                        # 0x802D44F4..0x802DDC04, 165 functions / 38672 B).  See the unit header for
                        # the seam, module and language evidence.
                        Object(NonMatching, "ai/fn_802D44F4.cpp"),
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
            Object(NonMatching, "stage/stg_w.cpp"),
            Object(NonMatching, "stage/fn_802B2978.c"),
            # Registered from proposal/802B2AA0_fn_802B2AA0.cpp (a 0x802B2AA0 run discovery
            # proposed): the stage band's per-area runtime state - the `stage_w` block's flag
            # byte/bit mask/4-second timers, the two 0x4F8-byte per-area objects at
            # `lbl_806BB7E0` and the area colour/effect drivers.  Module `stage` (the lib and the
            # left neighbour `stage/fn_802B2978.c`); no `__FILE__` string covers the range and the
            # dump answers `zz_` for every row, so the file keeps the map's stem (see its header).
            # Sections: .text 0x802B2AA0..0x802B5C58, extab 0x80013D34..0x80013E3C (33 8-byte
            # records), extabindex 0x80031A4C..0x80031BD8.
            Object(NonMatching, "stage/fn_802B2AA0.cpp"),
            # Camera band 0x802B5C58-0x802BEAAC (132 functions, 36436 B), registered from
            # proposal/802B5C58_fn_802B5C58.cpp.  The lib and cflags are the neighbours' ("stage"
            # and "ai" both build with cflags_main / Wii/1.3) and the module is `camera`: the range's
            # own named exports are get_camera_pos / get_camera_direction / get_current_view_mtx /
            # set_quake_sub, and the seam at 0x802B5C58 is a .sdata2 pool jump.
            Object(NonMatching, "camera/fn_802B5C58.cpp"),
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
            Object(NonMatching, "hud/cockpit_quest.cpp"),
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
            # The continuation of the menu band: proposal `802A6624_fn_802A6624.cpp` (`.text`
            # 0x802A6624..0x802AD9C0, 139 functions / 29596 B; extab 0x800137D4..0x80013B0C and
            # extabindex 0x8003123C..0x80031710).  Module `menu` from the left neighbour and from the
            # range's own entry points (`put_message`, `put_frame_dialog`, `GetMenuFontColor`); no
            # `__FILE__` string covers the range and the dump answers `zz_` for most of it, so the file
            # keeps the map's stem (see its header).  Same `cflags_menu` as `menu_item.cpp`: the band
            # carries 0 record-form instructions and keeps its tiny same-file `bl`s.
            Object(NonMatching, "menu/fn_802A6624.cpp"),
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
            Object(NonMatching, "enemy/fn_801251D0.cpp"),
                        Object(NonMatching, "enemy/fn_8012BA00.c"),
            Object(NonMatching, "enemy/fn_8012BDF4.cpp"),
            # Registered from proposal/8012E968_fn_8012E968 (a 0x8012E968 run discovery
            # proposed): the enemy area/group timer set, 3 functions / 0x30C bytes, plus the
            # extab/extabindex entry its whole first function carries.  Module `enemy` from the
            # link band and the code (`_ENEMY_WORK`, `get_move_work_adrs(3)`); the boundary at
            # 0x8012E968 is the band's one strong `tudiscover` seam, so this is not a
            # continuation of the unit above.  No `__FILE__` string survives and the dump answers
            # only `zz_` placeholders, so the file keeps the map's own stem (see its header).
            Object(NonMatching, "enemy/fn_8012E968.cpp"),
            # Registered from proposal/8012EC74_fn_8012EC74 (a 0x8012EC74 run discovery
            # proposed): the enemy per-motion frame-window/area set, 281 functions / 0x8990 bytes.  The
            # seam at 0x8012EC74 is the band's strong `.sdata2` pool-run jump (label_80796CB4 ->
            # label_80796CB8): the left neighbour's pool run ends at 0x80796CB4 and this range reads the
            # next one, so it is a different TU, not a continuation.  Module `enemy` from the link band
            # and the code; no `__FILE__` string survives and the dump answers `zz_` for most rows, so
            # the file keeps the map's stem (see the unit's header).
            Object(NonMatching, "enemy/fn_8012EC74.cpp"),
            # Registered from proposal/80137604_fn_80137604 (a 0x80137604 run discovery
            # proposed): an enemy's per-motion action/rotation update set, 20 functions / 0x2670
            # bytes.  Module `enemy` from the link band (both bracketing units are `enemy`) and
            # from the code (it calls `em_act_ck(_ENEMY_WORK*)`, `get_enemy_data`, ...); C++
            # because every mangled callee must be declared at its real signature (rule 9).
            # No `__FILE__` string survives in the range and the dump answers only `zz_`
            # placeholders, so the file keeps the map's own stem (see the unit's header).
            Object(NonMatching, "enemy/fn_80137604.cpp"),
            Object(NonMatching, "enemy/fn_80138074.c"),
            # Registered from proposal/8013ACC4_fn_8013ACC4.cpp (a 0x8013ACC4 run discovery
            # proposed): the enemy user-data command interpreter and its 0x100-entry dispatch
            # table, 3 functions / 0x119C bytes.  Module `enemy` from the link band (both
            # bracketing units are `enemy`) and from the code (`_ENEMY_WORK`, `fn_8013A900`);
            # C++ because the range calls the mangled `ran_suu__Fl`.  No `__FILE__` string
            # survives and the dump answers only `zz_` placeholders, so the file keeps the map's
            # own stem (see the unit's header).
            Object(NonMatching, "enemy/fn_8013ACC4.cpp"),
            Object(NonMatching, "enemy/fn_8013BE60.c"),
            # Registered from proposal/8013F764_fn_8013F764 (a 0x8013F764 run discovery proposed):
            # the enemy program interpreter's second half - the run driver, the 0x4E..0x6A stream
            # readers and the command-length/stream-walk helpers, 45 functions / 0x1A54 bytes.
            # Module `enemy` from the link band (both bracketing units are `enemy`) and from the
            # code (it calls `get_enemy_data(_ENEMY_WORK*)`, `get_move_work_adrs`, and the
            # neighbour unit's `fn_8013BE60`/`fn_8013C244`); C++ because four of its callees are
            # mangled and rule 9 forbids spelling a mangling at the call site.  No `__FILE__`
            # string survives in the range and the dump answers only `zz_` placeholders, so the
            # file keeps the map's own stem (see the unit's header).
            Object(NonMatching, "enemy/fn_8013F764.cpp"),
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
            # Registered from proposal/80147CE0_fn_80147CE0.cpp (a 0x80147CE0 run discovery
            # proposed): the enemy action-handler band - the per-tick entry, the state-machine
            # family and the three sub-state dispatchers, 36 functions / 0x208C bytes, plus the
            # extab/extabindex entries its 30 framed functions carry.
            # Module `enemy` from the link band (both bracketing registered units are `enemy`) and
            # from the code (every function takes a `_ENEMY_WORK*`, pinned by
            # `em_frame_check__FP11_ENEMY_WORKUsff`); C++ because four callees are mangled and rule
            # 9 forbids spelling a mangling at the call site.  No `__FILE__` string survives in the
            # range and the dump answers only `zz_` placeholders, so the file keeps the map's own
            # stem.  The right edge (0x80149D6C) is evidence (the registered unit above starts
            # there, and this unit's extabindex run ends exactly where that unit's begins); the left
            # edge (0x80147CE0) is `attribute.py`'s `--max-bytes` cut, not evidence - see the unit
            # header and the outbox.
            Object(NonMatching, "enemy/fn_80147CE0.cpp"),
            Object(Matching, "enemy/fn_80149D6C.c"),
            Object(NonMatching, "enemy/fn_8014A1BC.c"),
            # Registered from proposal/801502C8_fn_801502C8.cpp: the enemy em00x action band,
            # 30 functions / 0x4E34 bytes (0x801502C8..0x801550FC), plus the extab/extabindex runs
            # its framed functions carry.  Module `enemy` from the link band (both bracketing
            # registered units are `enemy`) and from the code (every function takes the
            # `_ENEMY_WORK`, pinned by `em_after_frame_check__FP11_ENEMY_WORKUsff`); C++ because the
            # callees are mangled and rule 9 forbids spelling a mangling at the call site.  No
            # `__FILE__` string is reachable from the range and the dump answers only `zz_`
            # placeholders, so the file keeps the map's own stem.  See the unit header.
            Object(NonMatching, "enemy/fn_801502C8.cpp"),
            # proposal/801550FC_fn_801550FC.cpp: the em003 action unit (0x801550FC..0x8015D860,
            # 104 functions).  C++ (the range defines three em003_* manglings).  The boundary is
            # provisional - see the unit header.
            Object(NonMatching, "enemy/fn_801550FC.cpp"),
            # proposal/8015D860_fn_8015D860.cpp: the em008 per-action state-step band
            # (0x8015D860..0x8015E854, 28 functions), the run between the two units above and
            # below.  C++ (the range's callees are manglings and it allocates with `operator
            # new`); registered once, at its final home - the file keeps the map's `fn_XXXXXXXX`
            # stem because no `__FILE__` string names it (rule 7 deferred, see the unit header).
            Object(NonMatching, "enemy/fn_8015D860.cpp"),
            # proposal/8015E854_fn_8015E854.cpp: the enemy action/state unit that follows the em003
            # block (0x8015E854..0x80165FC8, 55 functions).  C++ (every callee is a mangling reached
            # through its real signature).  Not a continuation of fn_801550FC.cpp (that unit ends at
            # 0x8015D860); the boundary is provisional - see the unit header.
            Object(NonMatching, "enemy/fn_8015E854.cpp"),
            # Registered from proposal/80165FC8_fn_80165FC8.cpp: the enemy per-area seat/action unit
            # (0x80165FC8..0x801679B0, 21 functions / 0x19E8 bytes), the exact unclaimed gap between
            # the two registered units above and below (each ends where this range begins/ends), plus
            # the extab/extabindex runs its 18 framed functions carry and the one .ctors word for its
            # static initializer `fn_80166330`.  Module `enemy` from the link band (both bracketing
            # units are `enemy`) and from the code (`_ENEMY_WORK`, `fn_802B0668`, `em_frame_check`);
            # C++ because the range reaches mangled callees through their real signatures (rule 9).
            # No `__FILE__` string survives in the range and the dump answers only `zz_` placeholders,
            # so the file keeps the map's own stem (see the unit's header).
            Object(NonMatching, "enemy/fn_80165FC8.cpp"),
            Object(NonMatching, "enemy/fn_801679B0.cpp"),
            Object(NonMatching, "enemy/fn_80171194.cpp"),
            Object(NonMatching, "enemy/fn_80178128.cpp"),
            # Registered from proposal/80178378_fn_80178378.cpp (a 0x80178378 run discovery proposed):
            # the enemy action arming/stepping band 0x80178378..0x80181C88, 64 functions / 0x9920 bytes,
            # plus the 58 extab/extabindex entries its functions carry.  Module `enemy` from the link
            # band (the unit below ends exactly at this range's start) and from the code (`_ENEMY_WORK`,
            # the enemy action callees).  The seam is unproven - see the unit header.
            Object(NonMatching, "enemy/fn_80178378.cpp"),
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
            Object(NonMatching, "enemy/fn_80181C88.cpp"),
            Object(NonMatching, "enemy/fn_80177608.cpp"),
            # proposal/80177890_fn_80177890: the enemy motion-state update set (0x80177890..0x80178128,
            # 12 functions). The neighbour TU's dispatch evidence pins the seam; cflags are this lib's.
            Object(NonMatching, "enemy/fn_80177890.cpp"),
            # Registered from proposal/80170FA8_fn_80170FA8.cpp (a 0x80170FA8 run discovery proposed).
            Object(NonMatching, "enemy/fn_80170FA8.cpp"),
            Object(NonMatching, "enemy/fn_80176C58.cpp"),
            Object(NonMatching, "enemy/fn_80170600.cpp"),
            # Registered from proposal/80182D5C_fn_80182D5C.cpp (the 0x80182D5C run discovery
            # proposed; 115 functions / 0x868C bytes, plus the extab/extabindex entries its 91
            # framed functions carry).  Module `enemy` from the link band (the unit below ends at
            # 0x80178378 and every callee in the range is enemy-band) and from the code;
            # C++ because the range reaches mangled callees through their real signatures.
            # No `__FILE__` string survives in the range and the dump answers only `zz_`
            # placeholders, so the file keeps the map's own stem (see the unit's header).
            Object(NonMatching, "enemy/fn_80182D5C.cpp"),
            # Registered from proposal/8018B3B8_fn_8018B3B8.cpp (the 0x8018B3B8 run discovery
            # proposed; 24 functions / 0x61E0 bytes).  Module `enemy` from the link band (both
            # bracketing registered units are `enemy/*`) and from the code; C++ because the range
            # reaches mangled callees.  No `__FILE__` string survives in the range and the dump
            # answers only `zz_` placeholders, so the file keeps the map's own stem (see the unit
            # header).
            Object(NonMatching, "enemy/fn_8018B3B8.cpp"),
            # Registered from proposal/80191598_fn_80191598.cpp (a 0x80191598 run discovery
            # proposed): the enemy aim/action group the per-enemy class tables at 0x805AA960..
            # 0x805AAA60 hold, 26 functions / 0x1154 bytes plus its extab/extabindex run.
            # Module `enemy` from the link band and the code (`em_act_ck(_ENEMY_WORK*)`,
            # `get_move_work_adrs(3)`, the enemy work's aim record); C++ because every callee out
            # of the range is a mangled symbol.  No `__FILE__` string survives in the range and
            # the dump answers only `zz_` placeholders, so the file keeps the map's own stem (see
            # the unit's header).
            Object(NonMatching, "enemy/fn_80191598.cpp"),
            # Registered from proposal/801926EC_fn_801926EC.cpp (the 0x801926EC run discovery
            # proposed): the enemy "em" action band's per-motion step group, 93 functions /
            # 0x6CF4 bytes plus its extab/extabindex run.  Module `enemy` from the link band (the
            # unit below ends at 0x801926EC, the unit above starts at 0x801993E0, and every callee
            # out of the range is enemy-band); C++ because the range reaches the mangled
            # `em_frame_check`.  No `__FILE__` string survives and the dump answers only `zz_`
            # placeholders, so the file keeps the map's own stem (see the unit's header).
            Object(NonMatching, "enemy/fn_801926EC.cpp"),
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
            Object(NonMatching, "enemy/fn_801CCBC4.cpp"),
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
            Object(NonMatching, "enemy/fn_801B0010.cpp"),
            # Registered from proposal/801D428C_fn_801D428C.cpp (the 0x801D428C run discovery
            # proposed): the enemy action band 0x801D428C..0x801D80EC, 42 functions / 0x3E60 bytes,
            # plus the 34 extab/extabindex entries its framed functions carry.  Module `enemy` from
            # the link band (the unit below ends at 0x80191598 and every callee out of the range is
            # enemy-band) and from the code (`_ENEMY_WORK` state machines, the enemy action
            # dispatchers).  C++ because the range reaches mangled callees (`setVector3__FP...`,
            # `em_frame_check__FP11_ENEMY_WORKUsff`, `getTevKColor__6MHchar...`) and a class
            # descriptor.  No `__FILE__` string survives in the range and the dump answers only
            # `zz_` placeholders, so the file keeps the map's own stem (see the unit's header).
            Object(NonMatching, "enemy/fn_801D428C.cpp"),
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
            Object(NonMatching, "enemy/fn_801D80EC.cpp"),
            # Registered once, at its final home (docs/plan.md 12) from proposal/801DB8E0_fn_801DB8E0.cpp
            # (the 0x801DB8E0 run discovery proposed): the enemy effect/part band 0x801DB8E0..0x801E0ADC,
            # 28 functions / 0x51FC bytes, plus the 23 extab records 0x8001031C..0x800103D4 and the 23
            # extabindex records 0x8002C394..0x8002C4A8 its framed functions carry.  Module `enemy` from
            # the link band (the unit below is `enemy/fn_801D428C.cpp`, the module's naming scheme is the
            # map stem) and from the code (every callee is `_ENEMY_WORK`-based).  C++ because the range
            # reaches mangled callees (`setVector3__FP...`, `eft009_set_pos__FUcP...`, `__nw__FUl`).  No
            # `__FILE__` string survives and the runtime dump answers only `zz_` placeholders, so the file
            # keeps the map's own stem (see the unit's header).
            Object(NonMatching, "enemy/fn_801DB8E0.cpp"),
            # Registered from proposal/801993E0_fn_801993E0.cpp (a 0x801993E0 run discovery
            # proposed): the enemy "em" action band 0x801993E0..0x8019ED34, 35 functions / 0x5954
            # bytes, plus the 25 extab/extabindex records its framed functions carry.
            # Module `enemy` from the link band (the unit below ends at 0x801926EC) and from the
            # code (every callee out of the range is `_ENEMY_WORK`-based); C++ because the range
            # reaches mangled callees through their real signatures.  No `__FILE__` string is
            # referenced by the range and the dump answers only `zz_` placeholders, so the file
            # keeps the map's own stem (see the unit's header).
            Object(NonMatching, "enemy/fn_801993E0.cpp"),
            # Registered once, at its final home (docs/plan.md 12) from
            # proposal/8019ED34_fn_8019ED34.cpp (the 0x8019ED34 gap between this unit and
            # `enemy/fn_801A4504.cpp`): the enemy motion/action band 0x8019ED34..0x801A4504, 65
            # functions / 0x57D0 bytes, plus the extab run 0x8000F14C..0x8000F304 (55 records) and
            # the extabindex run 0x8002A8DC..0x8002AB70 (55 records).  Module `enemy` from the link
            # band (both bracketing units are `enemy`) and the code (every callee out of the range
            # is enemy-band, every state machine switches on `_ENEMY_WORK::state`); the name keeps
            # the map's `fn_` stem (no `__FILE__` string, the dump answers only `zz_`/`FUN_`).  C++,
            # every plain `fn_XXXXXXXX` definition `extern "C"` (see the unit's header).
            Object(NonMatching, "enemy/fn_8019ED34.cpp"),
            # Registered once, at its final home (docs/plan.md 12) from
            # proposal/801B7020_fn_801B7020.cpp (a 0x801B7020 run discovery proposed): the enemy
            # motion/act-instruction group, 123 functions / 0x669C bytes plus its extab/extabindex
            # run and one `.ctors` word.  Module `enemy` from the link band (both bracketing
            # registered units are `enemy/*`, and every callee out of the range is the enemy work
            # API); C++ because the range reaches mangled members (`setMatColor__6MHchar...`,
            # `rotVecY__FPQ34nw4r4math4VEC3Ul`).  No `__FILE__` string survives and the runtime dump
            # answers only `zz_` placeholders, so the file keeps the map's own stem (see header).
            # Registered once, at its final home (docs/plan.md 12) from
            # `proposal/801B4458_fn_801B4458.cpp`: the enemy seat/effect-action band
            # 0x801B4458..0x801B7020, 48 functions / 0x2BC8 bytes, plus the extab run
            # 0x8000F6B4..0x8000F7EC (39 records), the extabindex run 0x8002B0F8..0x8002B2CC
            # (39 records) and one `.ctors` word at 0x8056F348 for the static initializer
            # `fn_801B6FB0`.  Module `enemy` from the link band (the unit below ends at 0x801B4458,
            # the unit above starts at 0x801B7020, both `enemy/*`) and from the code (every callee
            # out of the range is enemy-band: `_ENEMY_WORK`, `em_act_ck`, `fn_8012F5B8`,
            # `get_move_work_adrs(3)`).  C++ because the range reaches mangled callees through
            # their real signatures (rule 9).  No `__FILE__` string survives in the range and the
            # runtime dump answers only `zz_` placeholders, so the file keeps the map's own stem
            # (see the unit's header).
            Object(NonMatching, "enemy/fn_801B4458.cpp"),
            Object(NonMatching, "enemy/fn_801B7020.cpp"),
            # Registered from proposal/801BD6C0_fn_801BD6C0.cpp: the enemy motion/act-instruction
            # band's continuation, 128 functions / 0xC944 bytes (0x801BD6C0..0x801CA004) plus the
            # extab run 0x8000FACC..0x8000FDFC (102 records) and the extabindex run
            # 0x8002B71C..0x8002BBE4 (102 x 12 B) its framed functions carry.  Module `enemy` from
            # the link band (the unit below ends exactly at this range's start; the unit above starts
            # later at 0x801D428C after the still-unclaimed 0x801CA004..0x801D428C run) and from the
            # code (every callee is the enemy work API).  C++ because the range reaches mangled
            # callees.  No `__FILE__` string survives and the dump answers only `zz_` placeholders,
            # so the file keeps the map's own stem (see the unit header).
            Object(NonMatching, "enemy/fn_801BD6C0.cpp"),
            # Registered once, at its final home (docs/plan.md 12) from
            # proposal/801CA004_fn_801CA004.cpp: the enemy action/state band, 47 functions /
            # 0x2BC0 bytes (0x801CA004..0x801CCBC4), plus the extab run 0x8000FDFC..0x8000FF4C and
            # the extabindex run 0x8002BBE4..0x8002BDDC its 42 framed functions carry.  Module
            # `enemy` from the link band (both bracketing registered units are `enemy/*`) and from
            # the code (every function takes the shared `_ENEMY_WORK`); C++ because the range's
            # callees are mangled (`getTevKColor__6MHchar...`, `__nw__FUl`).  No `__FILE__` string
            # is reachable and the dump answers only `zz_` placeholders, so the file keeps the
            # map's own stem (see the unit header).
            Object(NonMatching, "enemy/fn_801CA004.cpp"),
            # Registered from proposal/801A4504_fn_801A4504.cpp (a 0x801A4504 run discovery proposed):
            # the enemy per-motion action dispatcher and its four helpers, 5 functions / 0x503C bytes
            # plus the extab run 0x8000F304..0x8000F324 and the extabindex run 0x8002AB70..0x8002ABA0.
            # Module `enemy` from `em_get_mot_no(_ENEMY_WORK*)` and every callee; the name keeps the
            # map's `fn_` stem (no `__FILE__` string, the dump answers only `zz_`/`FUN_`).  C++.
            Object(NonMatching, "enemy/fn_801A4504.cpp"),
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
            Object(NonMatching, "enemy/fn_801A9540.cpp"),
            # Registered once, at its final home (docs/plan.md 12) from proposal/8035E034_fn_8035E034.cpp:
            # the em033/em035 enemy-program handlers (`.text` 0x8035E034..0x8035F2B4, 16 functions /
            # 0x1280 bytes) plus their extab/extabindex group.  Module `enemy` from the `.data` program
            # tables `em033_prog_tbl`/`em035_prog_tbl` that list the range's entry points and from the
            # shared `_ENEMY_WORK` record every body drives; the name keeps the map's `fn_` stem (no
            # `__FILE__` string is reachable and the dump answers only `zz_` placeholders).  Sections:
            # extab 0x80017574..0x800175DC (13 records), extabindex 0x80036E94..0x80036F30 (13 x 12 B),
            # `.text` 0x8035E034..0x8035F2B4.  C++; every plain `fn_` definition is `extern "C"`.
            Object(NonMatching, "enemy/fn_8035E034.cpp"),
            # Registered once, at its final home (docs/plan.md 12) from proposal/80387844_fn_80387844.cpp:
            # the enemy monster-AI action band (`.text` 0x80387844..0x8038E8E8, 43 functions / 0x7084
            # bytes).  Module `enemy` from the code (every body drives the shared `_ENEMY_WORK` record
            # through `em_frame_check__FP11_ENEMY_WORKUsff`, `em_after_frame_check`, `get_joint_wpos_em`,
            # `em_magma_check`, `get_em_chg_scale`) and from the `.data` `em0XX_prog_tbl` program tables
            # of the bracketing enemy bands; C++ because the range reaches genuinely mangled callees
            # (`setVector3__FPQ34nw4r4math4VEC3fff`, `mulVecMatAddTrans`, `rotVecY`) through their real
            # signatures (rule 9).  No `__FILE__` string is reachable from the range and the runtime dump
            # answers only `zz_` placeholders, so the file keeps the map's own `fn_80387844` stem.
            Object(NonMatching, "enemy/fn_80387844.cpp"),
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
            Object(NonMatching, "sound/fn_800DCFEC.cpp"),
            # Registered once, at its final home (docs/plan.md 12) from proposal/800DD1F0_fn_800DD1F0.cpp:
            # the SE (`se_w`) request cluster's tail plus the `MHchar` model class, 125 symbols /
            # 0x6ACC bytes.  `sound` module (both bracketing units are `sound`, include/unsplit/sound.h
            # is their band) and C++ (the `* __FP...` / `move__6MHcharFUs` manglings).
            Object(NonMatching, "sound/fn_800DD1F0.cpp"),
            # Registered from proposal/800E3CBC_fn_800E3CBC.cpp (the 0x800E3CBC run discovery
            # proposed).  Module from the placed link-neighbour sound/fn_800E46E8.cpp; the dump's
            # prim_init_all/set_blendmode/set_zmode name the functions, not the TU (see the file header).
            Object(NonMatching, "sound/fn_800E3CBC.cpp"),
            Object(NonMatching, "sound/fn_800E46E8.cpp"),
            # Registered from proposal/800E8E60_fn_800E8E60.cpp (a 0x800E8E60 run discovery proposed).
            # The quest/challenge sound work system: 149 functions / 0x6978 bytes.  C++ from the range's
            # own mangled symbols; no `__FILE__` string survives, so the map's stem is the file name.
            Object(NonMatching, "sound/fn_800E8E60.cpp"),
            # Registered from proposal/800EF7D8_fn_800EF7D8 (a 0x800EF7D8 run discovery proposed):
            # the SE/BGM loader cluster, 0x800EF7D8..0x800F2A94 (62 functions).  Module `sound` from
            # the link band and the neighbours; the name is the map's own stem (no `__FILE__` string
            # in the range, and `dumpmap.py lookup` answers `zz_XXXXXXXX_`).
            Object(NonMatching, "sound/fn_800EF7D8.cpp"),
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
            # range: 119 functions / 0xABA0 bytes of `nw4r::ef`, three original TUs kept whole
            # (ef_postfield.cpp, ef_resource.cpp, ef_drawstripestrategy.cpp) because the discovery's
            # seam was capped at --max-bytes.  C++ from the `.cpp` __FILE__ strings and
            # Panic__Q24nw4r2dbFPCciPCce.
            Object(NonMatching, "ef/fn_800AEE48.cpp"),
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
            # Registered from proposal/800CDB2C_fn_800CDB2C.cpp (a 0x800CDB2C run discovery
            # proposed at a --max-bytes cap; the seam is a guess and the range is several
            # original TUs - see the unit's file header).
            Object(NonMatching, "ef/fn_800CDB2C.cpp"),
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
            Object(NonMatching, "ef/eft002.cpp"),
            Object(Matching, "ef/fn_800FD520.c"),
            Object(Matching, "ef/fn_800FD718.c"),
            Object(NonMatching, "ef/fn_800FD864.cpp"),
            # Registered once, at its final home (docs/plan.md 12).  proposal/800FE978_fn_800FE978.
            Object(NonMatching, "ef/fn_800FE978.cpp"),
            Object(NonMatching, "ef/eft004.cpp"),
            Object(NonMatching, "ef/fn_80101DF4.cpp"),
            Object(NonMatching, "ef/eft007.cpp"),
            Object(NonMatching, "ef/eft009.cpp"),
            Object(Matching, "ef/fn_80104BD0.c"),
            # Registered once, at its final home (docs/plan.md 12).  proposal/80105314_fn_80105314:
            # a maximal unclaimed run, seam unproven; class 4 decided the name (the map's
            # fn_80105314 stem, the scheme the bracketing fn_80104BD0/fn_8010D1A8 units use) and
            # the range holds several original effect families - see the unit's file header.
            Object(NonMatching, "ef/fn_80105314.cpp"),
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
            Object(Matching, "ef/fn_8011722C.c"),
            # Registered once, at its final home (docs/plan.md 12).  The
            # `proposal/801173AC_fn_801173AC.cpp` range (`.text` 0x801173AC..0x80119C44, 30 functions
            # / 10392 B): the tail of the eft024 job machine, the whole eft025 player family, the whole
            # eft026 enemy family and the head of eft028.  Two of the range's own definitions are
            # manglings (`eft026_set__FP4_PLWUcUlUl`, `eft028_set_koware__FUcPQ34nw4r4math4VEC3Ucl`), so
            # it is built as C++ and every plain `fn_XXXXXXXX` definition is `extern "C"`.  Sections:
            # extab 0x8000C49C..0x8000C554, extabindex 0x800265D4..0x800266E8,
            # .text 0x801173AC..0x80119C44.
            Object(NonMatching, "ef/fn_801173AC.cpp"),
            Object(NonMatching, "ef/fn_80119C44.c"),
            # Registered once, at its final home (docs/plan.md 12).  The `proposal/80119DEC_fn_80119DEC`
            # range, at the TU-bounded 0x80119DEC..0x8011D448 the attribution queue carries: the runtime
            # dump's own `eft029_set_scale` / `eft029_set_kaihou` name the TU (dumpmap.py); C++ from
            # their mangled definitions.  See the unit's file header for the stale-brief record.
            Object(NonMatching, "ef/eft029.cpp"),
            Object(Matching, "ef/fn_803066F0.c"),
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
            Object(Matching, "Runtime.PPCEABI.H/global_destructor_chain.c"),
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
            # Registered once, at its final home (docs/plan.md 12): proposal
            # `80224AC4_fn_80224AC4.cpp` - the player actor's per-model SE/motion rig update
            # (0x80224AC4-0x80229ECC, 44 functions, 0x8408 B) with its own exception tables
            # (extab 0x80011B34-0x80011C7C, extabindex 0x8002E7B8-0x8002E9A4, 41 records).  Home is
            # `Pl`: every actor parameter is a `_PLW*` (`Get_motion_no__FP4_PLW`,
            # `Pl_master_ck__FP4_PLW`, `Pl_act_ck__FP4_PLWUcUs`), the tail helpers
            # (`fn_80229CB4`/`fn_80229E10`/`fn_80229EA8`) are already homed in `unsplit/Pl.h`, and the
            # target object's extab/extabindex rules out the `lobby` lib next door
            # (`-Cpp_exceptions off`).  No `__FILE__` string and no runtime-dump name cover the range,
            # so the stem is the map's `fn_80224AC4` with a rule-7 deferral.  It uses `cflags_pl`
            # (this lib).
            Object(NonMatching, "Pl/fn_80224AC4.cpp"),
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
            # Registered once, at its final home (docs/plan.md 12): proposal
            # `802430E8_fn_802430E8` - the second half of the player motion -> SE frame dispatcher
            # family (0x802430E8-0x802489D4, 30 functions, 0x58EC B) with extab
            # 0x80011CCC-0x80011DB4 and extabindex 0x8002EA1C-0x8002EB78 (29 framed functions, one
            # 8-byte extab and one 12-byte extabindex record each - the left edge is the byte after
            # `Pl/fn_80241558.cpp`'s own extabindex record and the right edge is `Pl/fn_802489D4.cpp`'s
            # extab, so both runs are exactly this unit's).  Home is `Pl` and the stem is the map's
            # `fn_802430E8` with a rule-7 deferral: the unit's first function dispatches on
            # `Get_motion_no__FP4_PLW` and arms `se_req_frame_set__FP5_se_wllll` on the
            # `_PLW`+0xAF4/+0xAF8/+0xAFC `_se_w` works, i.e. the same shape as the matching sibling
            # `Pl/fn_80241558.cpp` (52 of its 59 switch arms are instruction-identical), no `__FILE__`
            # string covers the range, and `dumpmap.py` answers only `zz_<addr>_` for all 30 symbols.
            # It uses `cflags_pl` (this lib).
            Object(NonMatching, "Pl/fn_802430E8.cpp"),
            # Registered once, at its final home (docs/plan.md 12): proposal
            # `8025F088_fn_8025F088` - the player per-frame control cluster
            # (0x8025F088-0x80262940, 17 functions, 0x38B8 B) with its own exception tables
            # (extab 0x8001239C-0x8001241C, extabindex 0x8002F454-0x8002F514 - the runs start and end
            # exactly at this range, so both seams are real TU boundaries).  Home is `Pl`: every actor
            # parameter is a `_PLW` (`Pl_master_ck`, `Pl_act_ck`, `Pl_Skill_ck`, `Pl_cat_skill_ck`,
            # `Get_motion_no`), it reads the move work `get_move_work_adrs`/`get_move_work_max` and
            # the `lbl_806AB848` chunk table, and its siblings are `Pl/fn_80241558.cpp` (before) and
            # `Pl/fn_80262940.cpp` (after).  No `__FILE__` string covers the range and `dumpmap.py`
            # answers only `zz_` placeholders, so the stem is the map's `fn_8025F088` with a rule-7
            # deferral.  It uses `cflags_pl` (this lib).
            Object(NonMatching, "Pl/fn_8025F088.cpp"),
            # `80258FCC_fn_80258FCC.cpp` - the player act state-machine band (0x80258FCC-0x8025F088,
            # 74 functions, 0x60BC B) with extab 0x8001219C-0x8001239C and extabindex
            # 0x8002F154-0x8002F454 (the run is exactly this unit's 64 framed functions).  Home is
            # `Pl`: every function's first argument is the player work `_PLW*` and the gates are the
            # Pl siblings (`Pl_master_ck`, `Pl_act_ck`, `Pl_Skill_ck`, `Pl_frame_check`); both
            # bracketing registered units are Pl.  No `__FILE__` string is reachable from the range
            # and the dump answers only `zz_0258fcc_`, so the stem is the map's `fn_80258FCC` with a
            # rule-7 deferral.  It uses `cflags_pl` (this lib).
            Object(NonMatching, "Pl/fn_80258FCC.cpp"),
            # Registered once, at its final home (docs/plan.md 12): proposal
            # `80262940_fn_80262940` - the player main/control cluster (0x80262940-0x802693C4, 59
            # functions, 0x6A84 B) with its own exception tables (extab 0x8001241C-0x80012554,
            # extabindex 0x8002F514-0x8002F6E8).  Home is `Pl`: the actors are `_PLW` and every gate
            # is a Pl sibling (`Pl_master_ck`, `Pl_Skill_ck`, `Pl_act_ck`, `Pl_cat_skill_ck`,
            # `Pl_condition_ck`), and the runtime dump names 8 of the 59
            # (`player_control_move`/`init_player_work`/`player_move_start`/...).  No `__FILE__`
            # string covers the band, so the stem is the map's `fn_80262940` with a rule-7 deferral.
            # Registered once, at its final home (docs/plan.md 12): proposal
            # `802489D4_fn_802489D4` - the player action/handler cluster (0x802489D4-0x8024F200,
            # 42 functions, 0x6D2C B) with its own exception tables (extab
            # 0x80011DB4-0x80011EFC, extabindex 0x8002EB78-0x8002ED64), both runs bracketed
            # exactly by the neighbouring functions' records.  Home is `Pl`: the actor is
            # `_PLW`, every sibling unit is `Pl/*.cpp`, and the outbound calls are the Pl gates
            # (`Pl_master_ck`/`Pl_Skill_ck`/`Pl_act_ck`/`Pl_cat_skill_ck`/`Pl_frame_check`/
            # `Pl_chr_setX`/`PlayMode_ck`).  No `__FILE__` string covers the band and no
            # runtime-dump name exists for any of the 42 symbols, so the stem is the map's
            # `fn_802489D4` with a rule-7 deferral.  It uses `cflags_pl` (this lib).
            Object(NonMatching, "Pl/fn_802489D4.cpp"),
            Object(NonMatching, "Pl/fn_80262940.cpp"),
            # Registered once, at its final home (docs/plan.md 12): proposal
            # `8024F200_fn_8024F200` - the player's per-act state machine cluster
            # (0x8024F200-0x80258FCC, 92 functions, 40396 B).  Home is `Pl`: every function takes the
            # player work (`_PLW*`) and the per-part index byte, reads the act step byte at `_PLW`+0x005
            # and drives one step through `Pl_Skill_ck`/`Pl_cat_skill_ck`/`Pl_frame_check` and the
            # `Pl` motion helpers.  No `__FILE__` string covers the range (the `.data` pool between
            # `enemy_control.cpp` at 0x805A1BB8 and `menu_item.cpp` at 0x805CDFC8 carries none for the
            # whole band), so the stem is the map's `fn_8024F200` with a rule-7 deferral.  It uses
            # `cflags_pl` (this lib).
            Object(NonMatching, "Pl/fn_8024F200.cpp"),
            # Registered once, at its final home (docs/plan.md 12): proposal
            # `802693C4_fn_802693C4` - the player part/motion cluster (0x802693C4-0x8026BA1C, 63
            # functions, 9816 B) with its own exception tables (extab 0x80012554-0x8001265C,
            # extabindex 0x8002F6E8-0x8002F808 - the two runs the link order puts between
            # `Pl/fn_80262940.cpp` and `Pl/pl_master.cpp`).  Home is `Pl`: every actor parameter is
            # the `_PLW` the siblings take, the table it walks is `lbl_80794B28` (`_PLGLOBAL`) and
            # its callees are `Pl_chr_set_attr`/`Pl_chr_setX`/`Pl_frame_check`/`Get_motion_no`.
            # No `__FILE__` string covers the band and the runtime dump answers only `zz_`
            # placeholders for 58 of the 63 addresses, so the stem is the map's `fn_802693C4` with
            # a rule-7 deferral.  It uses `cflags_pl` (this lib).
            Object(NonMatching, "Pl/fn_802693C4.cpp"),
            Object(NonMatching, "Pl/pl_skill.cpp", cflags=cflags_pl_skill),
            Object(NonMatching, "Pl/pl_act.cpp"),
            # Registered once, at its final home (docs/plan.md 12): proposal
            # `80273B14_fn_80273B14.cpp` - the player act-entry/parameter unit
            # (0x80273B14-0x80276B58, 68 functions, 0x3044 B), the gap between pl_skill and pl_act.
            # It owns extab 0x8001280C-0x8001294C and extabindex 0x8002FA90-0x8002FC70: each run is
            # exactly 40 records and every extabindex record's function address (0x80273B14..
            # 0x80276A3C) is one of this unit's, read out of the DOL.  Home is `Pl`: every function
            # takes the player work (`_PLW*`) or an equipment slot out of it, and the gates are the
            # Pl siblings (`Pl_Skill_ck`, `Pl_master_ck`, `Pl_act_ck`, `Pl_cat_skill_ck`,
            # `Pl_condition_ck`, `Pl_dm_condition_ck`, `Pl_suimen_ck`, `Pl_chr_setX`).  No `__FILE__`
            # string covers the range and `dumpmap.py` answers only `zz_0273b14_`, so the stem is the
            # map's `fn_80273B14` with a rule-7 deferral.  It uses `cflags_pl` (this lib).
            Object(NonMatching, "Pl/fn_80273B14.cpp"),
            # Cluster C (`Pl_master_ck`, `Pl_act_ck`): pinned by the .sdata2 run jump
            # `lbl_8079A02C -> lbl_8079A030` at the right edge; the left edge is the closure edge.
            Object(Matching, "Pl/pl_master.cpp"),
            # Registered once, at its final home (docs/plan.md 12): proposal
            # `8026FFBC_fn_8026FFBC` - the player work's health-ratio gate, ONE function
            # (0x8026FFBC-0x80270018, 92 B) with extab 0x800126CC-0x800126D4 and extabindex
            # 0x8002F8B0-0x8002F8BC.  Home is `Pl`: the argument is the `_PLW*` both callers (in
            # `Pl/pl_skill.cpp`) hand `Pl_Skill_ck(_PLW*, u16)`.  No `__FILE__` string covers the
            # range (its only data operands are the two `.sdata2` pool words) and `dumpmap.py`
            # answers `zz_026ffbc_`, so the stem is the map's `fn_8026FFBC` with a rule-7 deferral.
            # It uses `cflags_pl` (this lib).
            Object(NonMatching, "Pl/fn_8026FFBC.cpp"),
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
            # Registered once, at its final home (docs/plan.md 12): proposal `8027D684_fn_8027D684` -
            # the player act/equipment cluster (0x8027D684-0x802840DC, 138 functions, 0x6A58 B) with
            # extab 0x80012B54-0x80012E7C and extabindex 0x8002FF7C-0x80030438 (101 framed functions,
            # one 8-byte extab and one 12-byte extabindex record each - the record count is exactly the
            # framed functions in the range, which is what pins both ranges).  Home is `Pl`: every
            # function's first argument is the player work `_PLW`, the gates are the Pl siblings
            # (`Pl_master_ck`/`Pl_frame_check`/`Pl_Skill_ck`), the equipment helpers take the `_EQUIP`
            # record `include/pl.h` owns, and both bracketing registered units are Pl.  The proposal's
            # edge is a `--max-bytes` cap rather than a TU boundary (no `__FILE__` evidence anywhere in
            # the band) and the dump answers `zz_<addr>_` for it, so the stem is the map's
            # `fn_8027D684` with a rule-7 deferral.  It uses `cflags_pl` (this lib).
            Object(NonMatching, "Pl/fn_8027D684.cpp"),
            # Registered once, at its final home (docs/plan.md 12): proposal `80288CEC_fn_80288CEC`.
            # 78 functions, 0x80288CEC-0x8028F66C (0x697C B), with extab 0x80012FFC-0x8001323C and
            # extabindex 0x80030678-0x800309D8.  Home is `Pl`: the whole unit operates on the player
            # work (`Pl_frame_check`/`Pl_zanzo_set`/`Pl_act_ck`/`eft029_set_scale` all take `_PLW*`),
            # it defines `pl_motion_set`, and its accessors read the record `get_move_work_adrs(0)`
            # returns.  Extent pinned by the `.sdata2` run 0x8079A270-0x8079A314 (the run boundary is
            # the left edge).  No `__FILE__` string and no runtime-dump name cover the range, so the
            # stem is the map's `fn_80288CEC` with a rule-7 deferral (the sibling class-4 pattern of
            # `Pl/fn_80229ECC.cpp` / `Pl/fn_80241558.cpp`).  It uses `cflags_pl` (this lib).
            Object(NonMatching, "Pl/fn_80288CEC.cpp"),
            # Registered once, at its final home (docs/plan.md 12): proposal `8028F66C_fn_8028F66C` -
            # the ground/hit collision cluster of the Pl band (0x8028F66C-0x80295EF4, 53 functions,
            # 0x6888 B) with extab 0x8001323C-0x800133CC and extabindex 0x800309D8-0x80030C30 (50
            # framed functions, one 8-byte extab and one 12-byte extabindex record each).  Home is
            # `Pl`: the right edge of the preceding Pl unit's `.text` is this range's left edge and
            # this unit's `.sdata2` pool starts exactly where that unit's ends (0x8079A314), the unit
            # reads the Pl-band global `lbl_80794B58`, and its exported entry points are the ones the
            # Pl/ef/enemy units call (`GetGroundHit2` from `Pl/pl_act.cpp`, `GetGroundHit` from
            # `ef/eft001.cpp`, `findInterSection*` from `enemy/*`).  The proposal's right edge is a
            # `--max-bytes` cap rather than a TU boundary (no `__FILE__` string covers the band and
            # the dump answers `zz_<addr>_` for 47 of the 53 addresses), so the stem is the map's
            # `fn_8028F66C` with a rule-7 deferral.  It uses `cflags_pl` (this lib).
            Object(NonMatching, "Pl/fn_8028F66C.cpp"),
            # Registered once, at its final home (docs/plan.md 12): proposal `80295EF4_fn_80295EF4` -
            # the hit/land query band (0x80295EF4-0x8029F3C8, 99 functions, 0x94D4 B).  Home is `Pl`:
            # the band hits the Pl family (`get_move_work_adrs`, `_PLW` fields, `Pl_frame_check`) and
            # its record vocabulary (`_HIT_W`, `LandData`, `get_hit_id`) is the one the map already
            # names inside the Pl band, whose registered `Pl/fn_80288CEC.cpp` ends at 0x8028F66C
            # straight before it.  The seam at 0x80295EF4 is a `--max-bytes` cut, not a TU boundary:
            # the band's `.sdata2` run 0x8079A330-0x8079A3C8 has no break across it, and the static
            # initializer inside this range (0x80297C30) constructs arrays out of the *previous*
            # proposal's `fn_80295544`.  No `__FILE__` string covers the band and `dumpmap.py` answers
            # only `zz_XXXXXXXX_` placeholders, so the stem is the map's `fn_80295EF4` with a rule-7
            # deferral (the sibling class-4 pattern of `Pl/fn_80229ECC.cpp` / `Pl/fn_80288CEC.cpp`).
            # `.text` only: the band owns no emitted data, and its `.ctors` word is not claimed
            # because this source states the static initializer as an explicit function (invariant
            # 8.4).  It uses `cflags_pl` (this lib).
            Object(NonMatching, "Pl/fn_80295EF4.cpp"),
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
            Object(NonMatching, "g3d/g3d_state.cpp"),        # 0x8008452C-0x800898B0
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
            Object(NonMatching, "g3d/g3d_resfile.cpp"),      # 0x80093990-0x800947A4
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
            Object(Matching, "Network/NetworkWiiMediator.c"),
        ],
    },
    {
        "lib": "OS",
        "mw_version": "Wii/1.3",
        "cflags": cflags_os,
        "host": False,
        "objects": [
            Object(Matching, "OS/OSAlarm.c"),
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
            # Registered once, at its final home (docs/plan.md 12): proposal
            # `801EC9F8_fn_801EC9F8.cpp` (`.text` 0x801EC9F8..0x801F3294, 67 functions / 26780 B).
            Object(NonMatching, "lobby/fn_801EC9F8.cpp"),
            # `801F3294_fn_801F3294.cpp` (`.text` 0x801F3294..0x801F9CD4, 48 functions / 27200 B) -
            # the lobby page/panel group between the menu layer below and the NPC group above.  Module
            # `lobby` from the code (`LbStr`, `get_lsp_data`, `chk_pointer`, `PutPageArrow`,
            # `LbPutAnaPageArrow`, `draw_sprite*`, `sysSE_req`) and from both bracketing registered
            # units; no `__FILE__` string covers the range and the dump answers only `zz_`
            # placeholders, so the file keeps the map stem (brief section 2, class 3+4).  Both edges
            # are unproven (the right one is the discovery byte cap; the next proposal 0x801F9CD4
            # continues the band).  Sections: extab 0x8001094C..0x80010A7C (38 records), extabindex
            # 0x8002CCDC..0x8002CEA4 (38 x 12 B), .text 0x801F3294..0x801F9CD4.
            Object(NonMatching, "lobby/fn_801F3294.cpp"),
            # `801F9CD4_fn_801F9CD4.cpp` (`.text` 0x801F9CD4..0x801FBF78, 14 functions / 8868 B) -
            # the lobby character-edit (hair/inner colour) screen group.  Module `lobby` from the code
            # (`lobby_w`/`lb_param_w`/`Screen_w`/`system_w`, `get_lsp_data`, `draw_sprite_ary`,
            # `GetMenuFontColor`, `LbStr`) and from both bracketing registered units; no `__FILE__`
            # string covers the range (the only `.cpp` string in the region's data, `enemy_control.cpp`,
            # is referenced from 0x801411DC, a different band) and the dump answers only `zz_`
            # placeholders, so the file keeps the map stem (brief section 2, class 3+4).  Sections:
            # extab 0x80010A7C..0x80010ADC (12 records), extabindex 0x8002CEA4..0x8002CF34 (12 x 12 B),
            # .text 0x801F9CD4..0x801FBF78.
            Object(NonMatching, "lobby/fn_801F9CD4.cpp"),
            # Registered once, at its final home (docs/plan.md 12): proposal
            # `801FBF78_fn_801FBF78.cpp` (`.text` 0x801FBF78..0x802029B4, 127 functions / 27196 B) -
            # the lobby NPC / world-update group.  Module `lobby` from the code (the range owns
            # `lb_npc`, `npc_data_town`, `npc_data_village`, `npc_lp_tbl`, `npc_model_*`, `npc_sub_data`
            # and defines `lb_npc_Get_motion_no`/`get_talk_npc_data_ptr`) and from the neighbour
            # below; no `__FILE__` string covers the range and the dump answers only `zz_`
            # placeholders, so the file name is the subsystem's own `lb_npc` (brief section 2,
            # class 3).  Sections: extab 0x80010ADC..0x80010DBC, extabindex 0x8002CF34..0x8002D384
            # (92 x 12 B), .text 0x801FBF78..0x802029B4, .ctors 0x8056F35C..0x8056F360.
            Object(NonMatching, "lobby/lb_npc.cpp"),
            # Registered once, at its final home (docs/plan.md 12): proposal
            # `802076D4_fn_802076D4.cpp` (`.text` 0x802076D4..0x8020C588, 82 functions / 20148 B) -
            # the lobby NPC/character action layer.  Module `lobby` from the code (`LbStr`,
            # `lb_npc_Get_motion_no`, `fn_801FE0AC`/`fn_801FDE3C` out of `lb_npc.cpp`) and from the
            # band (both bracketing registered units are `lobby`); no `__FILE__` string covers the
            # range and the dump answers only `zz_` placeholders, so the file keeps the map stem
            # (brief section 2, class 4).  C++ from the range's mangled callees.  Sections: extab
            # 0x80010F7C..0x80011184 and extabindex 0x8002D624..0x8002D930 (65 records, both runs
            # abutting the bracketing units), .text 0x802076D4..0x8020C588.
            Object(NonMatching, "lobby/fn_802076D4.cpp"),
            # `802029B4_fn_802029B4.cpp` (`.text` 0x802029B4..0x802076D4, 68 functions / 19744 B) -
            # the lobby NPC work band above `lobby/lb_npc.cpp`'s range: the same `_LB_NPC` state
            # machines and motion-table helpers.  Module `lobby` from the link band (both bracketing
            # registered units are `lobby`) and from the code (it takes `_LB_NPC`, calls
            # `lb_npc_Get_motion_no` and reads `lb_npc_move_data`/`lobby_w`); no `__FILE__` string is
            # reachable from the range and the dump answers only `zz_` placeholders, so the file keeps
            # the map stem (brief section 2, class 3+4).  Sections: extab 0x80010DBC..0x80010F7C,
            # extabindex 0x8002D384..0x8002D624 (56 x 8 / 56 x 12 B: the range has 56 framed
            # functions, and the per-function split objects' extabindex relocations hand
            # `@etb_80010F7C` to the next proposal's first framed function), .text
            # 0x802029B4..0x802076D4.
            Object(NonMatching, "lobby/fn_802029B4.cpp"),
            # Registered once, at its final home (docs/plan.md 12) from
            # proposal/8020C588_fn_8020C588.cpp: the lobby player-character control band
            # (`.text` 0x8020C588..0x80212810, 112 functions / 25224 B, plus its extab run
            # 0x80011184..0x80011434 and extabindex run 0x8002D930..0x8002DD38 - 86 records).
            # It sits between this unit and `lobby/fn_80212810.cpp`, so the module is `lobby`
            # (class 3: the link band); the range **defines** `LbStr__FUcUs`, the lobby string
            # helper `include/unsplit/lobby.h` declares and `lobby/fn_801E7530.cpp` calls, and
            # calls the lobby UI API.  No `__FILE__` string survives, so the file keeps the
            # map's `fn_8020C588` stem (class 4; see the unit's header).
            Object(NonMatching, "lobby/fn_8020C588.cpp"),
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
            # Registered once, at its final home (docs/plan.md 12): proposal
            # `8021E1EC_fn_8021E1EC.cpp` (`.text` 0x8021E1EC..0x80224AC4, 108 functions / 26840 B) -
            # the lobby item/equipment page family.  Module `lobby` from the code (`LbStr`,
            # `draw_sprite_ary`, `draw_font_idx`, `get_lsp_data`, `ItemName`, `put_menu_cursor`) and
            # from the band (both bracketing registered units are `lobby`); no `__FILE__` string
            # survives and the dump answers only `zz_` placeholders, so the file keeps the map stem
            # (brief section 2, class 4).  C++ from the range's mangled callees.  The extab /
            # extabindex runs agree with both edges exactly (77 records, `fn_8021E1EC` first,
            # `fn_80224A28` last), which is why the capped range is registered whole.  `.text` only:
            # the `.data` run this range partly references leaks outside it, so no data range is
            # claimed yet.
            Object(NonMatching, "lobby/fn_8021E1EC.cpp"),
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
            # Registered once, at its final home (docs/plan.md 12): proposal `80047398` - the character
            # face/skin TPL render unit (117 functions / 0x5610 B, 0x80047398..0x8004C9A0).  Game-root
            # band: the link neighbour `mh3_pad.cpp` ends at 0x80047398 and this unit's link neighbours
            # are the `main` lib's root files, so it takes their lib and flags.  The name stays the map's
            # `fn_80047398` stem - no `__FILE__` string and no runtime-dump name exists for the range
            # (class 3/4 in the brief; see the file header).
            Object(NonMatching, "fn_80047398.cpp"),
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
            # Registered once, at its final home (docs/plan.md 12): proposal `80056F24` - the screen
            # fade / filter / glare band (fade_set/fade_reset/get_fade_stat, GlareFilter_on,
            # filter_reset, setFilterPrio, the FIFO writers and the GX setup bodies), 59 symbols /
            # 0x262C bytes.  Same lib and flags as the game-root system files beside it (cflags_main);
            # the name stays the map's `fn_80056F24` stem - the range carries no `__FILE__` string (its
            # data refs are only the `.sdata2` float pool, the fade table and the two `.bss` blocks) and
            # the runtime dump answers only `FUN_`/`zz_` placeholders (class 3/4 in the brief; see the
            # file header).
            Object(NonMatching, "fn_80056F24.cpp"),
            # Registered once, at its final home (docs/plan.md 12): proposal `8056BBF0` - the
            # tiHKBManager.cpp listener/observer manager (its own `__FILE__` string at .data:0x80658408).
            # Evidence class 1; un-moduled game file at the repository root in the `main` (game) lib,
            # cflags_main.  Claims .text 0x8056BBF0-0x8056F2B4 + the .ctors word 0x8056F428-0x8056F42C.
            Object(NonMatching, "tiHKBManager.cpp"),
            # Registered once, at its final home (docs/plan.md 12): proposal
            # `80429B94_fn_80429B94` - the 0x80429B94-0x8043065C network/server-control band (114
            # functions, 27336 B) with extab 0x8001D368-0x8001D558 and extabindex 0x8003DE54-0x8003E0DC.
            # Game code that drives getPatsObject/getNetworkSessionManagerPat and reads the lobby
            # singleton `lobby_w`; no `__FILE__` string and no runtime-dump source name cover the
            # range, so the stem is the map's `fn_80429B94` with a rule-7 deferral (classes 3/4).  It
            # takes the game-root `main` lib and cflags_main (Wii/1.3, -O3, -inline noauto,
            # -Cpp_exceptions on - the target object carries extab/extabindex).
            Object(NonMatching, "fn_80429B94.cpp"),
            # Registered once, at its final home (docs/plan.md 12): proposal `80569DAC` - the
            # homebutton::gui component/manager band (`.text` 0x80569DAC-0x8056BBF0, 52 functions /
            # 7748 B).  Evidence class 2: the shared runtime dump names six functions with the real
            # `homebutton::gui::` spellings (Manager::~Manager, drawLine_, getComponent,
            # PaneManager::getPaneComponentByPane, PaneComponent::contain), and the RSO export table
            # config/RMHE08/hbm_data/symbols.txt carries the same class hierarchy.  Link neighbour
            # `tiHKBManager.cpp` is in `main`, so the unit takes the `main` lib and cflags_main; the
            # file is `homebutton/gui.cpp` (the namespace is `gui`).  Claims .text only.
            # Registered once, at its final home (docs/plan.md 12): proposal
            # `805632BC_fn_805632BC.cpp` - the software-keyboard band of the home-button GUI
            # (`.text` 0x805632BC-0x80569DAC, 142 functions / 27376 B).  Evidence class 3: the range's
            # own `.data` pool is the keyboard layout vocabulary of the home-button menu (fs_VK_*.brlyt
            # layouts, T_hiragana/B_hiragana/T_katakana/P_dakuten/B_Gkey_handaku/T_hankaku/T_zenkaku/
            # T_Mode_roma_hira, N_Header/N_Footer/T_Nigaoe/B_Nigaoe/T_Letter/T_TouchLetter,
            # P_txtScrll_UP/DOWN/N_txt_scrl/N_TopBtn_00/N_MemoRoot/G_ArwRoop), and the module comes
            # from the two link neighbours: `homebutton/gui.cpp` immediately above and
            # `tiHKBManager.cpp` immediately below (its "HKB" is this keyboard's manager).  No
            # `__FILE__` string covers the range and the runtime dump answers only `zz_`/`FUN_`
            # placeholders, so the file name is descriptive (class 3) and the symbols keep the map's
            # `fn_805632BC` stem with a rule-7 deferral in the file header.  The left edge is a
            # `--max-bytes` cap (the brief warns; the code before it references the same data pool),
            # the right edge is the registered homebutton/gui.cpp.  Link neighbour tiHKBManager.cpp is
            # in `main`, so the unit takes the `main` lib and cflags_main; the file is
            # `homebutton/keyboard.cpp`.  Claims .text only.
            Object(NonMatching, "homebutton/keyboard.cpp"),
            Object(NonMatching, "homebutton/gui.cpp"),
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

if args.mode == "configure":
    # Write build.ninja and objdiff.json
    generate_build(config)
elif args.mode == "progress":
    # Print progress information
    calculate_progress(config)
else:
    sys.exit("Unknown mode: " + args.mode)
