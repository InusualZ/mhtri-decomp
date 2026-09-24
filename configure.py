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
    *[f for f in cflags_base if f != "-O4,p"],
    "-O3",
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
        "lib": "auto",
        "mw_version": "Wii/1.3",
        "cflags": cflags_main,
        "progress_category": "auto",
        "objects": [
            Object(NonMatching, "auto/80040598_fn_80040598.cpp"),
            Object(NonMatching, "auto/80280050_fn_80280050.c"),
            Object(NonMatching, "auto/802843F0_fn_802843F0.c"),
            Object(NonMatching, "auto/80286DF8_fn_80286DF8.c"),
            Object(NonMatching, "auto/80287244_fn_80287244.c"),
            Object(NonMatching, "auto/802A0650_fn_802A0650.c"),
            Object(NonMatching, "auto/802B2978_fn_802B2978.c"),
            Object(NonMatching, "auto/802B2AA0_fn_802B2AA0.c"),
            Object(NonMatching, "auto/802B4E58_fn_802B4E58.c"),
            Object(NonMatching, "auto/802B535C_fn_802B535C.c"),
            Object(NonMatching, "auto/802C474C_fn_802C474C.c"),
            Object(NonMatching, "auto/802C58DC_fn_802C58DC.c"),
            Object(NonMatching, "auto/802C5ACC_fn_802C5ACC.c"),
            Object(NonMatching, "auto/802C5D10_fn_802C5D10.c"),
            Object(NonMatching, "auto/802CC57C_fn_802CC57C.c"),
            Object(NonMatching, "auto/802CC794_fn_802CC794.c"),
            Object(NonMatching, "auto/802D0C9C_fn_802D0C9C.c"),
            Object(Matching, "auto/802D0DCC_fn_802D0DCC.c"),
            Object(NonMatching, "auto/802D0F34_fn_802D0F34.cpp"),
            Object(NonMatching, "auto/802D4248_fn_802D4248.c"),
            Object(NonMatching, "auto/802E919C_fn_802E919C.c"),
            Object(NonMatching, "auto/802E932C_fn_802E932C.cpp"),
            Object(NonMatching, "auto/802EF0FC_fn_802EF0FC.c"),
            Object(NonMatching, "auto/802F12A4_fn_802F12A4.c"),
            Object(NonMatching, "auto/802F5138_fn_802F5138.c"),
            Object(NonMatching, "auto/802F9C2C_fn_802F9C2C.c"),
            Object(NonMatching, "auto/80073398_fn_80073398.cpp"),
            Object(NonMatching, "auto/800898B0_fn_800898B0.c"),
            Object(Matching, "auto/8009AA78_fn_8009AA78.c"),
            Object(NonMatching, "auto/800A99B4_fn_800A99B4.cpp"),
            Object(NonMatching, "auto/800AA18C_fn_800AA18C.cpp"),
            Object(NonMatching, "auto/800BFFD4_fn_800BFFD4.cpp"),
            Object(NonMatching, "auto/800C5DB8_fn_800C5DB8.cpp"),
            Object(NonMatching, "auto/800C9DD0_fn_800C9DD0.c"),
            Object(NonMatching, "auto/800CB948_fn_800CB948.c"),
            Object(NonMatching, "auto/800CC5B0_fn_800CC5B0.c"),
            Object(NonMatching, "auto/800CCCF8_fn_800CCCF8.c"),
            Object(NonMatching, "auto/800CCFB0_fn_800CCFB0.c"),
            Object(NonMatching, "auto/800CD584_fn_800CD584.c"),
            Object(NonMatching, "auto/800D7F54_fn_800D7F54.cpp"),
            Object(NonMatching, "auto/800DCFEC_fn_800DCFEC.c"),
            Object(NonMatching, "auto/800E46E8_fn_800E46E8.cpp"),
            Object(NonMatching, "auto/80300080_fn_80300080.c"),
            Object(NonMatching, "auto/80305C08_fn_80305C08.c"),
            Object(Matching, "auto/803066F0_fn_803066F0.c"),
            Object(NonMatching, "auto/8031D3B4_fn_8031D3B4.c"),
            Object(NonMatching, "auto/8031DA54_fn_8031DA54.c"),
            Object(NonMatching, "auto/803234EC_fn_803234EC.c"),
            Object(NonMatching, "auto/80324304_fn_80324304.c"),
            Object(Matching, "auto/80324F7C_fn_80324F7C.c"),
            Object(NonMatching, "auto/803250B0_fn_803250B0.c"),
            Object(NonMatching, "auto/803253BC_fn_803253BC.c"),
            Object(NonMatching, "auto/803386C4_fn_803386C4.c"),
            Object(NonMatching, "auto/80343130_fn_80343130.c"),
            Object(NonMatching, "auto/803589A8_fn_803589A8.c"),
            Object(NonMatching, "auto/8035F178_fn_8035F178.c"),
            Object(NonMatching, "auto/8036640C_fn_8036640C.c"),
            Object(NonMatching, "auto/800FCED4_fn_800FCED4.cpp"),
            Object(Matching, "auto/800FD520_fn_800FD520.c"),
            Object(Matching, "auto/800FD718_fn_800FD718.c"),
            Object(NonMatching, "auto/800FF8D4_fn_800FF8D4.cpp"),
            Object(NonMatching, "auto/80101DF4_fn_80101DF4.cpp"),
            Object(NonMatching, "auto/80101FA4_fn_80101FA4.cpp"),
            Object(NonMatching, "auto/80103D28_fn_80103D28.cpp"),
            Object(Matching, "auto/80104BD0_fn_80104BD0.c"),
            Object(NonMatching, "auto/8010D1A8_fn_8010D1A8.c"),
            Object(NonMatching, "auto/80114E34_fn_80114E34.cpp"),
            Object(Matching, "auto/8011722C_fn_8011722C.c"),
            Object(NonMatching, "auto/80119C44_fn_80119C44.c"),
            Object(NonMatching, "auto/8012BA00_fn_8012BA00.c"),
            Object(NonMatching, "auto/8012BDF4_fn_8012BDF4.cpp"),
            Object(NonMatching, "auto/80138074_fn_80138074.c"),
            Object(NonMatching, "auto/8013BE60_fn_8013BE60.c"),
            Object(NonMatching, "auto/80149D6C_fn_80149D6C.c"),
            Object(NonMatching, "auto/8014A1BC_fn_8014A1BC.c"),
            Object(NonMatching, "auto/801502C8_fn_801502C8.c"),
            Object(NonMatching, "auto/80154FAC_fn_80154FAC.c"),
            Object(NonMatching, "auto/801FBB64_fn_801FBB64.c"),
            Object(NonMatching, "auto/801FBF78_fn_801FBF78.cpp"),
            Object(NonMatching, "auto/80201C80_fn_80201C80.c"),
            Object(NonMatching, "auto/80201F60_fn_80201F60.c"),
            Object(NonMatching, "auto/802020FC_fn_802020FC.c"),
            Object(NonMatching, "auto/802027CC_fn_802027CC.c"),
            Object(NonMatching, "auto/802029B4_fn_802029B4.c"),
            Object(NonMatching, "auto/802076D4_fn_802076D4.c"),
            Object(NonMatching, "auto/802097E8_fn_802097E8.c"),
            Object(NonMatching, "auto/8020C588_fn_8020C588.c"),
            Object(NonMatching, "auto/8020FE18_fn_8020FE18.c"),
            Object(NonMatching, "auto/802310D4_fn_802310D4.c"),
            Object(NonMatching, "auto/80233448_fn_80233448.c"),
            Object(NonMatching, "auto/80234E9C_fn_80234E9C.c"),
            Object(NonMatching, "auto/802373AC_fn_802373AC.c"),
            Object(NonMatching, "auto/802399C8_fn_802399C8.c"),
            Object(NonMatching, "auto/8023C2D0_fn_8023C2D0.c"),
            Object(NonMatching, "auto/8023FC20_fn_8023FC20.c"),
            Object(NonMatching, "auto/80241558_fn_80241558.c"),
            Object(NonMatching, "auto/802430E8_fn_802430E8.c"),
            Object(NonMatching, "auto/802489D4_fn_802489D4.c"),
            Object(NonMatching, "auto/8024CA50_fn_8024CA50.c"),
            Object(NonMatching, "auto/8024CD8C_fn_8024CD8C.c"),
            Object(NonMatching, "auto/8024EF60_fn_8024EF60.c"),
            Object(NonMatching, "auto/8024F0DC_fn_8024F0DC.c"),
            Object(NonMatching, "auto/8025C09C_fn_8025C09C.c"),
            Object(NonMatching, "auto/80370274_fn_80370274.c"),
            Object(NonMatching, "auto/80382310_fn_80382310.cpp"),
            Object(NonMatching, "auto/80385CAC_fn_80385CAC.c"),
            Object(NonMatching, "auto/80385DEC_fn_80385DEC.c"),
            Object(NonMatching, "auto/80387620_fn_80387620.c"),
            Object(NonMatching, "auto/8038EFEC_fn_8038EFEC.c"),
            Object(NonMatching, "auto/803963F4_fn_803963F4.c"),
            Object(NonMatching, "auto/80396654_fn_80396654.c"),
            Object(NonMatching, "auto/803AFF34_fn_803AFF34.c"),
            Object(NonMatching, "auto/803B0F98_fn_803B0F98.c"),
            Object(NonMatching, "auto/803B177C_fn_803B177C.c"),
            Object(NonMatching, "auto/803B43B0_fn_803B43B0.c"),
            Object(NonMatching, "auto/803D6F98_fn_803D6F98.c"),
            Object(NonMatching, "auto/803E1A14_fn_803E1A14.c"),
            Object(NonMatching, "auto/803E3678_fn_803E3678.c"),
            Object(NonMatching, "auto/803E3CE8_fn_803E3CE8.c"),
            Object(NonMatching, "auto/803E44C8_fn_803E44C8.c"),
            Object(NonMatching, "auto/80430400_fn_80430400.c"),
            Object(NonMatching, "auto/804309E8_fn_804309E8.c"),
            Object(NonMatching, "auto/80440798_fn_80440798.c"),
            Object(NonMatching, "auto/80443958_fn_80443958.cpp"),
            Object(NonMatching, "auto/8044E340_fn_8044E340.c"),
            Object(NonMatching, "auto/804E01E0_fn_804E01E0.c"),
            Object(NonMatching, "auto/804E08F0_fn_804E08F0.c"),
            Object(NonMatching, "auto/804E6710_fn_804E6710.c"),
            Object(NonMatching, "auto/804E7280_fn_804E7280.c"),
            Object(NonMatching, "auto/80500E34_SinFIdx__Q24nw4r4mathFf.cpp"),
            Object(NonMatching, "auto/8050A770_fn_8050A770.c"),
            Object(NonMatching, "auto/8051E864_fn_8051E864.c"),
            Object(NonMatching, "auto/8051EB38_fn_8051EB38.c"),
            Object(NonMatching, "auto/80533B04_fn_80533B04.c"),
            Object(NonMatching, "auto/80537E50_fn_80537E50.c"),
            Object(NonMatching, "auto/8053D8D4_fn_8053D8D4.c"),
            Object(NonMatching, "auto/8053DD64_fn_8053DD64.c"),
            Object(NonMatching, "auto/8053DF20_fn_8053DF20.c"),
            Object(NonMatching, "auto/80541B4C_fn_80541B4C.c"),
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
            Object(Matching, "g3d/g3d_resanmamblight.c"),
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
