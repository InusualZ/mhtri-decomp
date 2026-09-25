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
    *[f for f in cflags_lobby if f != "-inline auto"],
    "-inline noauto",
    # Evidence: the retail main.o carries extab 0x90 + extabindex 0xD8 (18 unwind-only records, one per
    # function with a frame) and our object emitted none, while every function's .text is unaffected by the
    # flag - the same finding as Pl, g3d and camellia. sys_mem.cpp in this lib already turns exceptions on
    # with a per-file pragma, so this only adds what that pragma would have (analysis: .pi/notes/extab-gap.md).
    "-Cpp_exceptions on",
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
        ],
    },

    {
        # Promoted from auto/802B2978_fn_802B2978.c (docs/plan.md: an auto unit stops being scaffolding).
        "lib": "stage",
        "mw_version": "Wii/1.3",
        "cflags": cflags_main,
        "progress_category": "game",
        "objects": [
                        Object(NonMatching, "stage/fn_802B2978.c"),
        ],
    },

    {
        # Promoted from auto/80324F7C_fn_80324F7C.c (docs/plan.md: an auto unit stops being scaffolding).
        "lib": "hud",
        "mw_version": "Wii/1.3",
        "cflags": cflags_main,
        "progress_category": "game",
        "objects": [
                        Object(Matching, "hud/fn_80324F7C.c"),
        ],
    },

    {
        # Promoted from auto/8012BA00_fn_8012BA00.c (docs/plan.md: an auto unit stops being scaffolding).
        "lib": "enemy",
        "mw_version": "Wii/1.3",
        "cflags": cflags_main,
        "progress_category": "game",
        "objects": [
                        Object(NonMatching, "enemy/fn_8012BA00.c"),
            Object(NonMatching, "enemy/fn_8012BDF4.cpp"),
            Object(NonMatching, "enemy/fn_80138074.c"),
            Object(NonMatching, "enemy/fn_8013BE60.c"),
            Object(Matching, "enemy/fn_80149D6C.c"),
            Object(NonMatching, "enemy/fn_8014A1BC.c"),
            Object(NonMatching, "enemy/fn_801679B0.cpp"),
            Object(NonMatching, "enemy/fn_80171194.cpp"),
            Object(NonMatching, "enemy/fn_80178128.cpp"),
            Object(NonMatching, "enemy/fn_80177608.cpp"),
            # proposal/80177890_fn_80177890: the enemy motion-state update set (0x80177890..0x80178128,
            # 12 functions). The neighbour TU's dispatch evidence pins the seam; cflags are this lib's.
            Object(NonMatching, "enemy/fn_80177890.cpp"),
            # Registered from proposal/80170FA8_fn_80170FA8.cpp (a 0x80170FA8 run discovery proposed).
            Object(NonMatching, "enemy/fn_80170FA8.cpp"),
            Object(NonMatching, "enemy/fn_80176C58.cpp"),
            Object(NonMatching, "enemy/fn_80170600.cpp"),
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
            Object(NonMatching, "sound/fn_800DCFEC.c"),
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
            Object(NonMatching, "ef/fn_80119C44.c"),
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
            Object(NonMatching, "Pl/pl_skill.cpp", cflags=cflags_pl_skill),
            Object(NonMatching, "Pl/pl_act.cpp"),
            # Cluster C (`Pl_master_ck`, `Pl_act_ck`): pinned by the .sdata2 run jump
            # `lbl_8079A02C -> lbl_8079A030` at the right edge; the left edge is the closure edge.
            # `fn_8026FFBC` (0x8026FFBC..0x80270018) sits on the ambiguous side of that seam and is
            # deliberately left unclaimed rather than guessed in.
            Object(Matching, "Pl/pl_master.cpp"),
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
            Object(NonMatching, "g3d/g3d_resanm.c"),         # 0x800898B0-0x80089F94
            Object(NonMatching, "g3d/g3d_resanmamblight.c"), # 0x80089F94-0x8008A220
            Object(NonMatching, "g3d/g3d_resanmcamera.c"),   # 0x8008A220-0x8008A28C
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
