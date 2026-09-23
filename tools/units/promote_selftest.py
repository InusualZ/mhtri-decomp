"""Self-test for `tools/units/promote.py` - the plan, the four-file edit and the byte comparator.

Three things here can silently cost a promotion, so each gets its own block of checks:

* the **plan** - which lib the module resolves to, whether the flag delta is read from the real
  `build.ninja` (and from a unit with no `cflags=` override, not from the first object in the lib),
  which symbol is the unit's own, and whether the pool/claim/lint consequences are found;
* the **edits** - `configure.py` (the object line leaves the source lib and lands *inside* the target
  lib's object list, flag and keyword arguments intact, or a new lib block is appended), `splits.txt`
  (the key line only), the source (whole-word only) and the map (through `symedit`'s plan/apply);
* `compare_objects` - against synthetic ELF objects written here, so the one judgement the whole tool
  exists for (is this the same object?) is tested directly: a rename is `names-only`, a changed
  section, a changed relocation *value* or a changed `.comment` is `differs`.

`apply` is exercised end to end against the fixture tree with `promote.git` replaced by a shim that
implements only `mv` (as `shutil.move`) and `status` (as empty) - git is used for nothing else, and the
selftest must not need a git repository. Everything runs in a temp directory; the real `config/`,
`configure.py` and `symbols.txt` are never read or written.

    python tools/units/promote_selftest.py
    python tools/units/promote.py --selftest
"""

from __future__ import annotations

import contextlib
import io
import json
import os
import shutil
import struct
import sys
import tempfile
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

import promote as pr  # noqa: E402

FIXTURE_SPLITS = (
    "Sections:\n"
    "\t.text       type:code align:32\n"
    "\n"
    "Pl/pl_act.cpp:\n"
    "\t.text       start:0x80003000 end:0x80003100\n"
    "\n"
    "auto/80001000_fn_80001000.c:\n"
    "\textab       start:0x80000100 end:0x80000108\n"
    "\t.text       start:0x80001000 end:0x80001080\n"
    "\n"
    "auto/80002000_fn_80002000.c:\n"
    "\t.text       start:0x80002000 end:0x80002080\n"
)

FIXTURE_CONF = (
    "config.progress_categories = [\n"
    '    ProgressCategory("auto", "Auto (bulk attribution)"),\n'
    '    ProgressCategory("game", "Game Code"),\n'
    "]\n\n"
    "config.libs = [\n"
    "    {\n"
    '        "lib": "auto",\n'
    '        "mw_version": "Wii/1.3",\n'
    '        "cflags": cflags_main,\n'
    '        "progress_category": "auto",\n'
    '        "objects": [\n'
    '            Object(NonMatching, "auto/80001000_fn_80001000.c"),\n'
    '            Object(Matching, "auto/80002000_fn_80002000.c"),\n'
    "        ],\n"
    "    },\n"
    "    {\n"
    '        "lib": "Pl",\n'
    '        "mw_version": "Wii/1.0",\n'
    '        "cflags": cflags_pl,\n'
    '        "progress_category": "game",\n'
    '        "objects": [\n'
    '            Object(NonMatching, "Pl/pl_skill.cpp", cflags=cflags_pl_skill),\n'
    '            Object(NonMatching, "Pl/pl_act.cpp"),\n'
    "        ],\n"
    "    },\n"
    "]\n"
)

FIXTURE_BUILD = (
    "build build\\RMHE08\\src\\auto\\80001000_fn_80001000.o: mwcc_sjis $\n"
    "    src\\auto\\80001000_fn_80001000.c | build\\compilers\n"
    "  mw_version = Wii\\1.3\n"
    "  cflags = -nodefaults -O3 -inline noauto $\n"
    "      -Cpp_exceptions on -lang=c\n"
    "build build\\RMHE08\\src\\Pl\\pl_skill.o: mwcc_sjis src\\Pl\\pl_skill.cpp\n"
    "  mw_version = Wii\\1.0\n"
    "  cflags = -nodefaults -O3 -opt nopeephole,level=4 -lang=c++\n"
    "build build\\RMHE08\\src\\Pl\\pl_act.o: mwcc_sjis src\\Pl\\pl_act.cpp\n"
    "  mw_version = Wii\\1.0\n"
    "  cflags = -nodefaults -O3 -inline noauto -opt nopeephole -lang=c++\n"
)

FIXTURE_MAP = (
    "fn_80001000 = .text:0x80001000; // type:function size:0x80 scope:global\n"
    "fn_80001040 = .text:0x80001040; // type:function size:0x40 scope:local\n"
    "lbl_80000100 = .text:0x80000100; // type:label size:0x8\n"
    "fn_80002000 = .text:0x80002000; // type:function size:0x80 scope:global\n"
)

FIXTURE_SOURCE = (
    "/* auto/80001000_fn_80001000.c - fixture.  `fn_80001000` calls `fn_80001040`. */\n"
    '#include "types.h"\n'
    "\n"
    "extern void fn_80001040(int value);\n"
    "\n"
    "void fn_80001000(int value)\n"
    "{\n"
    "    fn_80001040(value);\n"
    "}\n"
)

# A second unit, registered as `Matching`: the move must keep that flag on the line it carries over.
FIXTURE_SOURCE_MATCHING = "void fn_80002000(void)\n{\n}\n"

FIXTURE_POOL = "# Brief: auto/80001000_fn_80001000\n\nbody\n"


def build_fixture(root: Path) -> pr.Ctx:
    (root / "config" / "RMHE08").mkdir(parents=True, exist_ok=True)
    (root / "src" / "auto").mkdir(parents=True, exist_ok=True)
    (root / "src" / "Pl").mkdir(parents=True, exist_ok=True)
    (root / "tools" / "units" / "briefs" / "pool").mkdir(parents=True, exist_ok=True)
    (root / "build" / "RMHE08" / "src").mkdir(parents=True, exist_ok=True)
    (root / "build.ninja").write_text(FIXTURE_BUILD, encoding="utf-8", newline="")
    (root / "configure.py").write_text(FIXTURE_CONF, encoding="utf-8", newline="")
    (root / "config" / "RMHE08" / "splits.txt").write_text(FIXTURE_SPLITS, encoding="utf-8",
                                                           newline="")
    (root / "config" / "RMHE08" / "symbols.txt").write_text(FIXTURE_MAP, encoding="utf-8",
                                                            newline="")
    (root / "src" / "auto" / "80001000_fn_80001000.c").write_text(FIXTURE_SOURCE, encoding="utf-8",
                                                                  newline="")
    (root / "src" / "auto" / "80002000_fn_80002000.c").write_text(FIXTURE_SOURCE_MATCHING,
                                                                  encoding="utf-8", newline="")
    (root / "src" / "Pl" / "pl_act.cpp").write_text("// fixture\n", encoding="utf-8")
    (root / "tools" / "units" / "briefs" / "pool" / "80001000-fn-80001000-abcd.md").write_text(
        FIXTURE_POOL, encoding="utf-8", newline="")
    return pr.Ctx(root=root)


# --------------------------------------------------------------------------------------------------
# synthetic ELF objects - the comparator's fixtures
# --------------------------------------------------------------------------------------------------
def write_elf(path: Path, text: bytes = b"\x10\x00\x00\x00" * 4, comment: bytes = b"CW\x00\x0f",
              syms=None, relocs=None) -> Path:
    """A minimal ELF32 BE object with `.text`, `.rela.text`, `.comment`, `.symtab`, `.strtab`."""
    syms = syms if syms is not None else [
        ("", 0, 0, 0, 0),
        ("fixture.c", 0, 0, 4, 0xFFF1),
        ("main_fn", 0, len(text), 0x12, 1),
    ]
    relocs = relocs if relocs is not None else [(0, 2, 0)]
    strtab, offsets = b"\x00", []
    for name, *_ in syms:
        offsets.append(len(strtab))
        strtab += name.encode() + b"\x00"
    symtab = b""
    for (name, value, size, info, shndx), off in zip(syms, offsets):
        symtab += struct.pack(">IIIBBH", off if name else 0, value, size, info, 0, shndx)
    rela = b"".join(struct.pack(">IIi", off, (idx << 8) | 1, addend) for off, idx, addend in relocs)
    sections = [
        (".text", 1, 0, text),
        (".rela.text", 4, 0, rela),
        (".comment", 1, 0, comment),
        (".symtab", 2, 0, symtab),
        (".strtab", 3, 0, strtab),
    ]
    shstr, shname_off = b"\x00", []
    for name, *_ in sections:
        shname_off.append(len(shstr))
        shstr += name.encode() + b"\x00"
    shstrtab_off = len(shstr)
    shstr += b".shstrtab\x00"
    shnum = len(sections) + 2                       # + null + .shstrtab
    shstrndx = shnum - 1
    ehsize, shentsize = 52, 40
    body, body_off = b"", []
    for _name, _type, _flags, data in sections:
        body_off.append(len(body))
        body += data
    shstr_off = len(body)
    body += shstr
    shoff = ehsize + len(body)
    ehdr = (b"\x7fELF\x01\x02\x01" + b"\x00" * 9 +
            struct.pack(">HHIIIIIHHHHHH", 1, 20, 1, 0, 0, shoff, 0, ehsize, 0, 0,
                        shentsize, shnum, shstrndx))
    shdrs = struct.pack(">10I", 0, 0, 0, 0, 0, 0, 0, 0, 0, 0)
    for i, (name, typ, flags, data) in enumerate(sections):
        link = shnum - 1 if name == ".symtab" else (2 if name == ".rela.text" else 0)
        info = 1 if name == ".rela.text" else 0
        shdrs += struct.pack(">10I", shname_off[i], typ, flags, 0, ehsize + body_off[i], len(data),
                             link, info, 1, 0)
    shdrs += struct.pack(">10I", shstrtab_off, 3, 0, 0, ehsize + shstr_off, len(shstr), 0, 0, 1, 0)
    path.write_bytes(ehdr + body + shdrs)
    return path


def silent(fn):
    """Run `fn` with stdout captured, returning its value - the selftest's own output stays readable."""
    return captured(fn)[0]


def captured(fn):
    """`(value, stdout)` for a call whose printed output is part of what is checked."""
    buf = io.StringIO()
    with contextlib.redirect_stdout(buf):
        value = fn()
    return value, buf.getvalue()


def selftest() -> int:
    fails, checks = [], 0

    def check(name, got, want):
        nonlocal checks
        checks += 1
        if got != want:
            fails.append("%s: got %r want %r" % (name, got, want))

    def message(fn):
        try:
            fn()
            return ""
        except SystemExit as exc:
            return str(exc)
        except Exception as exc:                      # noqa: BLE001 - the selftest wants the text
            return "%s: %s" % (type(exc).__name__, exc)

    # -- pure helpers --------------------------------------------------------------------------
    check("norm_unit strips src/ and the extension", pr.norm_unit("src/auto/X.c"), "auto/X")
    check("norm_unit keeps a bare path", pr.norm_unit("auto/X"), "auto/X")
    check("norm_unit strips .cpp", pr.norm_unit("Pl/f.cpp"), "Pl/f")
    check("lang_flag .c", pr.lang_flag(".c"), "-lang=c")
    check("lang_flag .cpp", pr.lang_flag(".cpp"), "-lang=c++")
    check("normalise_path turns ninja's backslashes", pr.normalise_path("Wii\\1.3"), "Wii/1.3")
    check("flag_units pairs an option with its value",
          pr.flag_units("-O3 -Cpp_exceptions off -lang=c"), ["-O3", "-Cpp_exceptions off"])
    check("flag_units keeps a quoted value whole",
          pr.flag_units('-pragma "cats off" -O3'), ['-pragma "cats off"', "-O3"])
    check("fmt_delta names both directions",
          pr.fmt_delta(["-O3", "-a"], ["-O3", "-b"]), ["-a", "+-b"])

    tmp = Path(tempfile.mkdtemp(prefix="promote-selftest-"))
    try:
        ctx = build_fixture(tmp)

        # -- build.ninja reading ---------------------------------------------------------------
        got = pr.ninja_unit_flags(ctx.build_ninja, "build/RMHE08/src/auto/80001000_fn_80001000.o")
        check("ninja: mw_version", pr.normalise_path(got["mw_version"]), "Wii/1.3")
        check("ninja: multi-line cflags joined",
              got["cflags"], "-nodefaults -O3 -inline noauto -Cpp_exceptions on -lang=c")
        check("ninja: a missing object is None",
              pr.ninja_unit_flags(ctx.build_ninja, "build/RMHE08/src/nope.o"), None)

        libs = pr.configure_libs(pr.read_text(ctx.configure))
        check("configure: two libs", sorted(libs), ["Pl", "auto"])
        check("configure: the Pl lib's group", libs["Pl"]["cflags"], "cflags_pl")
        check("configure: the auto lib's objects", len(libs["auto"]["objects"]), 2)
        check("configure: lib_of_module finds Pl", pr.lib_of_module(libs, "Pl"), "Pl")
        check("configure: lib_of_module is None for a new module",
              pr.lib_of_module(libs, "Game"), None)
        check("configure: preserving_libs finds the same command line",
              pr.preserving_libs(libs, "Wii/1.3", "cflags_main"), ["auto"])

        # -- the plan --------------------------------------------------------------------------
        p = pr.plan(ctx, "auto/80001000_fn_80001000", "move_indicator", "Pl",
                    None, ["fn_80001000=move_indicator_draw"])
        check("plan: new unit path", p["new_unit"], "Pl/move_indicator.c")
        check("plan: the module's lib", p["lib"], "Pl")
        check("plan: not creating a lib", p["creating_lib"], False)
        check("plan: the flag is preserved", p["flag"], "NonMatching")
        check("plan: the object row was found", p["object_row"]["path"],
              "auto/80001000_fn_80001000.c")
        check("plan: splits key line", p["splits_block"]["key_line"], 6)
        check("plan: splits ranges kept", len(p["splits_block"]["ranges"]), 2)
        check("plan: one map rename planned", len(p["map_changed"]), 1)
        check("plan: the map rename is not applied yet", p["map_applied"], [])
        check("plan: the source edit is whole-word",
              [(i, o, n) for i, o, n, _t in p["source_edits"]],
              [(1, "fn_80001000", "move_indicator_draw"),
               (6, "fn_80001000", "move_indicator_draw")])
        check("plan: the callee was not renamed", "fn_80001040(value)" in p["text"], True)
        check("plan: rule 7 counts the fn_ names left in code", p["lint"]["count"], 2)
        check("plan: rule 7 names them", p["lint"]["names"], ["fn_80001040"])
        check("plan: a comment mention is not a finding",
              (p["text"].count("fn_80001040"), p["lint"]["count"]), (3, 2))
        check("plan: rule 7 is enforced outside src/auto/", p["lint"]["enforced"], True)
        check("plan: renaming every fn_ name leaves rule 7 clean",
              pr.plan(ctx, "auto/80001000_fn_80001000", "move_indicator", "Pl", None,
                      ["fn_80001000=move_indicator_draw", "fn_80001040=helper_draw"])["lint"]["count"],
              0)
        check("plan: the pooled brief was found", [x.name for x in p["pool"]],
              ["80001000-fn-80001000-abcd.md"])
        check("plan: the flag delta came from the unit without an override",
              p["flags"]["after"]["from"], "Pl/pl_act.cpp")
        check("plan: mw_version delta", p["flags"]["changes"][0], "mw_version Wii/1.3 -> Wii/1.0")
        check("plan: cflags delta names the added flag",
              "cflags +-opt nopeephole" in p["flags"]["changes"], True)
        check("plan: a bare --symbol names the unit's own symbol",
              pr.plan(ctx, "auto/80001000_fn_80001000", "x_fn", "Pl", None, ["x_fn"])["pairs"],
              [("fn_80001000", "x_fn")])
        for spelling in ("auto/80001000_fn_80001000", "src/auto/80001000_fn_80001000",
                         "auto/80001000_fn_80001000.c"):
            check("plan: accepts the %s spelling" % spelling,
                  pr.plan(ctx, spelling, "move_indicator", "Pl", None, [])["unit"],
                  "auto/80001000_fn_80001000.c")
        check("plan: no --lib for the src root is refused",
              "pass --lib" in message(lambda: pr.plan(ctx, "auto/80001000_fn_80001000", "x",
                                                      ".", None, [])), True)
        check("plan: --module '' with --lib Pl plans to the src root",
              pr.plan(ctx, "auto/80001000_fn_80001000", "move_indicator", ".", "Pl",
                      [])["new_unit"], "move_indicator.c")
        check("plan: renders without error", silent(lambda: pr.human_plan(p, ctx)) is None, True)
        check("plan: --json is serialisable",
              json.dumps(pr.plan_json(p, ctx))[:1], "{")

        # a new lib defaults to the moved unit's own flags, so the command line is unchanged
        g = pr.plan(ctx, "auto/80001000_fn_80001000", "move_indicator", "Game", None, [])
        check("plan: a new lib keeps the command line", g["flags"]["changes"], [])
        check("plan: a new lib keeps the mw_version", g["new_lib_settings"]["mw_version"], "Wii/1.3")
        check("plan: a new lib keeps the cflags group", g["new_lib_settings"]["cflags"], "cflags_main")
        check("plan: a new lib is declared new", g["creating_lib"], True)
        check("plan: no --symbol leaves the map alone", g["map_changed"], [])

        # the refusals
        check("refuse: unregistered unit",
              "not a registered source" in message(lambda: pr.plan(ctx, "auto/nope", "x", "Pl",
                                                                    None, [])), True)
        check("refuse: the auto bucket itself",
              "auto bucket" in message(lambda: pr.plan(ctx, "auto/80001000_fn_80001000", "x", "auto",
                                                        None, [])), True)
        check("refuse: extension change",
              "changes the language" in message(lambda: pr.plan(
                  ctx, "auto/80001000_fn_80001000", "x.cpp", "Pl", None, [])), True)
        check("refuse: a bad stem",
              "not a valid source stem" in message(lambda: pr.plan(
                  ctx, "auto/80001000_fn_80001000", "9x", "Pl", None, [])), True)

        # -- configure.py ----------------------------------------------------------------------
        out = pr.configure_move(p)
        check("configure: the old object line is gone",
              'Object(NonMatching, "auto/80001000_fn_80001000.c")' in out, False)
        check("configure: the new object line is in",
              'Object(NonMatching, "Pl/move_indicator.c")' in out, True)
        auto_block = out[out.index('"lib": "auto"'):out.index('"lib": "Pl"')]
        pl_block = out[out.index('"lib": "Pl"'):]
        check("configure: the source lib keeps its other object",
              '"auto/80002000_fn_80002000.c"' in auto_block, True)
        check("configure: it landed inside the Pl objects list",
              pl_block.index('"objects": [') < pl_block.index('"Pl/move_indicator.c"')
              < pl_block.index("],"), True)
        check("configure: it lands after the objects already there",
              pl_block.index('"Pl/pl_act.cpp"') < pl_block.index('"Pl/move_indicator.c"'), True)

        m = pr.plan(ctx, "auto/80002000_fn_80002000", "second", "Pl", None, [])
        check("configure: a Matching flag is preserved", m["flag"], "Matching")
        check("configure: Matching stays Matching",
              'Object(Matching, "Pl/second.c")' in pr.configure_move(m), True)

        k = pr.plan(ctx, "auto/80001000_fn_80001000", "kept_kwargs", "Pl", "Pl", [])
        k["object_row"] = dict(k["object_row"], rest=", cflags=cflags_pl_skill")
        check("configure: a cflags= override travels with the line",
              'Object(NonMatching, "Pl/kept_kwargs.c", cflags=cflags_pl_skill)' in pr.configure_move(k),
              True)

        new_lib = pr.configure_move(g)
        check("configure: a new lib block is appended", '        "lib": "Game",' in new_lib, True)
        check("configure: the new lib takes the unit's flags",
              '"cflags": cflags_main,' in new_lib, True)
        check("configure: the new lib holds the moved object",
              'Object(NonMatching, "Game/move_indicator.c")' in new_lib, True)
        check("configure: the source lib is still intact", '"lib": "auto",' in new_lib, True)

        g_bad = dict(g)
        g_bad["conf_text"] = FIXTURE_CONF.replace("config.libs = [", "config.libs =")
        check("configure: a missing anchor is refused",
              "config.libs = [" in message(lambda: pr.configure_move(g_bad)), True)

        # -- apply performs the whole change ---------------------------------------------------
        watched = [ctx.configure, ctx.splits, ctx.mapfile,
                   ctx.src / "auto" / "80001000_fn_80001000.c"]
        before = [path.read_bytes() for path in watched]
        silent(lambda: pr.apply_plan(ctx, p, dry_run=True))
        check("apply --dry-run: writes nothing", [path.read_bytes() for path in watched], before)

        real_git = pr.git

        def fake_git(root, *args, check=True):
            if args[:1] == ("mv",):
                src, dst = os.path.join(str(root), args[1]), os.path.join(str(root), args[2])
                os.makedirs(os.path.dirname(dst), exist_ok=True)
                shutil.move(src, dst)
                return ""
            if args[:1] == ("status",):
                return ""
            raise AssertionError("unexpected git call: %r" % (args,))

        pr.git = fake_git
        try:
            rc = silent(lambda: pr.apply_plan(ctx, p))
        finally:
            pr.git = real_git
        check("apply: returns 0", rc, 0)
        check("apply: the source moved", (ctx.src / "Pl" / "move_indicator.c").is_file(), True)
        check("apply: the old source is gone",
              (ctx.src / "auto" / "80001000_fn_80001000.c").exists(), False)
        moved = (ctx.src / "Pl" / "move_indicator.c").read_text(encoding="utf-8")
        check("apply: the source defines the new name", "void move_indicator_draw(int value)" in moved,
              True)
        check("apply: the comment was renamed too", "`move_indicator_draw` calls" in moved, True)
        check("apply: the callee is untouched", "fn_80001040(value)" in moved, True)
        check("apply: the map renamed the symbol",
              "move_indicator_draw = .text:0x80001000" in pr.read_text(ctx.mapfile), True)
        check("apply: the map kept the other symbols",
              "fn_80001040 = .text:0x80001040" in pr.read_text(ctx.mapfile), True)
        check("apply: the splits key moved", "Pl/move_indicator.c:" in pr.read_text(ctx.splits), True)
        check("apply: the old splits key is gone",
              "auto/80001000_fn_80001000.c:" in pr.read_text(ctx.splits), False)
        check("apply: the splits ranges were kept",
              pr.splits_block(pr.read_text(ctx.splits), "Pl/move_indicator.c")["ranges"],
              ["extab       start:0x80000100 end:0x80000108",
               ".text       start:0x80001000 end:0x80001080"])
        conf_now = pr.read_text(ctx.configure)
        check("apply: configure lost the old row", '"auto/80001000_fn_80001000.c"' in conf_now, False)
        check("apply: configure gained the new row", '"Pl/move_indicator.c"' in conf_now, True)
        check("apply: the pooled brief was removed", list(ctx.pool.glob("*.md")), [])
        check("apply: a second plan of the old unit refuses",
              "not a registered source" in message(lambda: pr.plan(ctx, "auto/80001000_fn_80001000",
                                                                    "again", "Pl", None, [])), True)
        check("apply: the new path plans cleanly",
              pr.plan(ctx, "Pl/move_indicator", "again", "Pl", None, [])["new_unit"], "Pl/again.c")
        same = pr.plan(ctx, "Pl/move_indicator", "again", "Pl", None, [])
        check("configure: a same-lib move is flagged", same["same_lib"], True)
        same_out = pr.configure_move(same)
        check("configure: a same-lib move rewrites the line in place",
              'Object(NonMatching, "Pl/again.c")' in same_out
              and '"Pl/move_indicator.c"' not in same_out, True)
        check("configure: a same-lib move keeps the line's position",
              same_out.index('"Pl/pl_act.cpp"') < same_out.index('"Pl/again.c"'), True)

        # -- a failed write undoes everything that was already done ----------------------------
        tmp2 = Path(tempfile.mkdtemp(prefix="promote-selftest-rollback-"))
        ctx_r = build_fixture(tmp2)
        p_r = pr.plan(ctx_r, "auto/80001000_fn_80001000", "move_indicator", "Pl",
                      None, ["fn_80001000=move_indicator_draw"])

        def bad_rename(src, dst):
            if str(dst).endswith("configure.py"):
                raise OSError("injected write failure")
            os.replace(src, dst)

        pr.git = fake_git
        try:
            rc = silent(lambda: pr.apply_plan(ctx_r, p_r, rename=bad_rename))
        finally:
            pr.git = real_git
        check("apply: a failed write returns 1", rc, 1)
        check("apply: the failure undid the move",
              (ctx_r.src / "auto" / "80001000_fn_80001000.c").is_file(), True)
        check("apply: the failure undid the source edit",
              (ctx_r.src / "auto" / "80001000_fn_80001000.c").read_text(encoding="utf-8"),
              FIXTURE_SOURCE)
        check("apply: the failure undid the map rename",
              "fn_80001000 = .text:0x80001000" in pr.read_text(ctx_r.mapfile), True)
        check("apply: the failure undid the splits key",
              pr.read_text(ctx_r.splits), FIXTURE_SPLITS)
        check("apply: the failure undid configure.py",
              pr.read_text(ctx_r.configure), FIXTURE_CONF)
        shutil.rmtree(tmp2, ignore_errors=True)

        # -- compare_objects -------------------------------------------------------------------
        objs = tmp / "objs"
        objs.mkdir(exist_ok=True)
        a = write_elf(objs / "a.o")
        check("compare: identical objects",
              pr.compare_objects(a, write_elf(objs / "b.o"))["verdict"], "identical")
        check("compare: a file rename is names-only",
              pr.compare_objects(a, write_elf(objs / "c.o", syms=[
                  ("", 0, 0, 0, 0), ("moved.c", 0, 0, 4, 0xFFF1), ("main_fn", 0, 16, 0x12, 1)]))["verdict"],
              "names-only")
        check("compare: a symbol rename is names-only",
              pr.compare_objects(a, write_elf(objs / "d.o", syms=[
                  ("", 0, 0, 0, 0), ("fixture.c", 0, 0, 4, 0xFFF1), ("draw", 0, 16, 0x12, 1)]))["verdict"],
              "names-only")
        check("compare: a one-byte .text change differs",
              pr.compare_objects(a, write_elf(objs / "e.o",
                                              text=b"\x10\x00\x00\x00" * 3 + b"\x11"))["verdict"],
              "differs")
        check("compare: a .comment change differs",
              pr.compare_objects(a, write_elf(objs / "f.o", comment=b"CW\x00\x0e"))["verdict"],
              "differs")
        check("compare: a relocation target rename is names-only",
              pr.compare_objects(a, write_elf(objs / "g.o", syms=[
                  ("", 0, 0, 0, 0), ("fixture.c", 0, 0, 4, 0xFFF1),
                  ("main_fn2", 0, 16, 0x12, 1)]))["verdict"], "names-only")
        check("compare: a relocation target with another value differs",
              pr.compare_objects(a, write_elf(objs / "h.o", syms=[
                  ("", 0, 0, 0, 0), ("fixture.c", 0, 0, 4, 0xFFF1),
                  ("main_fn", 8, 16, 0x12, 1)]))["verdict"], "differs")
        check("compare: an added symbol differs",
              pr.compare_objects(a, write_elf(objs / "i.o", syms=[
                  ("", 0, 0, 0, 0), ("fixture.c", 0, 0, 4, 0xFFF1), ("main_fn", 0, 16, 0x12, 1),
                  ("@9", 0, 0, 0x10, 0)]))["verdict"], "differs")
        check("compare: an empty relocation section differs",
              pr.compare_objects(a, write_elf(objs / "j.o", relocs=[]))["verdict"], "differs")
        check("compare: the diff names the section",
              pr.compare_objects(a, write_elf(objs / "k.o", text=b"\x00" * 16))["diffs"][0].startswith(
                  ".text"), True)
        (objs / "notelf.o").write_bytes(b"not an elf")
        check("compare: not an ELF is refused",
              "not a 32-bit big-endian ELF" in message(
                  lambda: pr.compare_objects(a, objs / "notelf.o")), True)

        # -- the check subcommand --------------------------------------------------------------
        p2 = pr.plan(ctx, "Pl/move_indicator", "again", "Pl", None, [])
        check("check: a missing after-object exits 2",
              silent(lambda: pr.check(ctx, p2, None, None, False, False)), 2)
        ctx.object_dir.joinpath("Pl").mkdir(parents=True, exist_ok=True)
        shutil.copyfile(a, ctx.object_dir / "Pl" / "move_indicator.o")       # the "before" object
        shutil.copyfile(write_elf(objs / "m.o", syms=[
            ("", 0, 0, 0, 0), ("moved.c", 0, 0, 4, 0xFFF1), ("main_fn", 0, 16, 0x12, 1)]),
            ctx.object_dir / "Pl" / "again.o")                               # the "after" object
        check("check: defaults resolve old_obj/new_obj and exit 0 on names-only",
              silent(lambda: pr.check(ctx, p2, None, None, False, False)), 0)
        check("check: a changed section exits 1",
              silent(lambda: pr.check(ctx, p2, str(a), str(objs / "e.o"), False, False)), 1)
        rc, text = captured(lambda: pr.check(ctx, p2, None, None, False, True))
        check("check: the JSON has the verdict", json.loads(text or "{}").get("verdict"), "names-only")
        check("check: the JSON exit code is 0 for names-only", rc, 0)
    finally:
        shutil.rmtree(tmp, ignore_errors=True)

    print("%d checks, %d failed" % (checks, len(fails)))
    for f in fails:
        print("  FAIL %s" % f)
    return 1 if fails else 0


if __name__ == "__main__":
    sys.exit(selftest())
