"""The build's own link: the linker, the link line from build.ninja, unit-name resolution, scratch links.
Spec: docs/tools/spec/mwlink.md. CLI: python tools/mwlink_debugger.py <subcommand> (this module has none of its own)."""
from __future__ import annotations

import re
from pathlib import Path

from tools.lib import proc as lib_proc
from tools.lib import units
from tools.mwlink.catalogue import classify_diagnostics, parse_timeline, phase_kinds


#: The repository root (tools/mwlink/ is two levels below it); a path, never read at import.
ROOT = Path(__file__).resolve().parents[2]


# The linker the build actually uses, if we can find it.
DEFAULT_LINKER = ROOT / "build/compilers/Wii/1.3/mwldeppc.exe"


def default_linker():
    """The linker the *build* uses, read from build.ninja's global mw_version.

    That variable is what the ``link`` rule expands, so it is the binary whose
    behaviour the build depends on - in this tree Wii/1.0, not the Wii/1.3 the
    compiler lane named.  Falls back to a local Wii build, then to 1.3.
    """
    ninja = ROOT / "build.ninja"
    if ninja.exists():
        m = re.search(r"^mw_version\s*=\s*(\S+)", ninja.read_text(errors="replace"), re.M)
        if m:
            cand = ROOT / "build/compilers" / m.group(1).replace("\\", "/") / "mwldeppc.exe"
            if cand.exists():
                return cand
    found = sorted((ROOT / "build/compilers").glob("Wii/*/mwldeppc.exe"))
    if found:
        return found[-1]
    return DEFAULT_LINKER if DEFAULT_LINKER.exists() else None


def find_gdb(explicit=None):
    if explicit:
        return explicit if Path(explicit).exists() else None
    for cand in (ROOT / "build/tools/gdb.exe", Path.home() / "tools/mwcc-dbg/mingw64/bin/gdb.exe"):
        if Path(cand).exists():
            return str(cand)
    from shutil import which
    return which("gdb.exe") or which("gdb")


def derive_rsp(out="build/RMHE08/main.elf", dest=None):
    """The link's list of inputs, read out of ``build.ninja``'s own statement.

    ``ninja`` writes ``$out.rsp`` for the link and deletes it again, so the
    response file a trace needs is derived from the build statement rather than
    kept: the tokens between ``link`` and the first ``|``/``||`` are exactly
    what ninja puts in the response file.  Nothing is transcribed - if the
    build's input list changes, so does this.
    """
    ninja = ROOT / "build.ninja"
    if not ninja.exists():
        return None
    lines = ninja.read_text(errors="replace").splitlines()
    want = out.replace("\\", "/")
    for i, line in enumerate(lines):
        m = re.match(r"^build\s+(\S+):\s+link\s+(.*)$", line)
        if not m or m.group(1).replace("\\", "/") != want:
            continue
        parts = [m.group(2)]
        j = i
        while parts[-1].rstrip().endswith("$") and j + 1 < len(lines):
            j += 1
            parts.append(lines[j])
        toks, stop = [], False
        for tok in " ".join(parts).replace("$", " ").split():
            if tok in ("|", "||"):
                stop = True
            if not stop:
                toks.append(tok)
        if dest is not None:
            Path(dest).write_text("\n".join(toks) + "\n", encoding="utf-8")
        return toks
    return None


def derive_link_line(rsp, ldscript=None):
    """The linker argument list for a link, with the build's own flags.

    ``ldflags`` is read from ``build.ninja`` (the global assignment *and* the
    per-build one that appends ``-lcf``), so the trace links the way the build
    does instead of the way a default would.
    """
    ninja = ROOT / "build.ninja"
    flags = None
    lcf = ldscript
    if ninja.exists():
        txt = ninja.read_text(errors="replace")
        m = re.search(r"^ldflags\s*=\s*(.*)$", txt, re.M)
        if m:
            flags = m.group(1).strip()
            for extra in re.findall(r"^\s+ldflags\s*=\s*\$ldflags\s*(.*)$", txt, re.M):
                seg = extra.strip()
                m2 = re.match(r"-lcf\s+(\S+)", seg)
                if m2:
                    lcf = m2.group(1)
                flags += " " + re.sub(r"-lcf\s+\S+", "", seg).strip()
    if flags is None:
        flags = "-fp hardware -nodefaults"
    argv = [flags]
    if lcf:
        argv.append("-lcf " + str(lcf))
    argv.append("-o {out} -map {map} @" + str(rsp))
    return " ".join(argv)


def link_inputs(out="build/RMHE08/main.elf"):
    """The objects the build's own link statement consumes, in its own order.

    This is ``build.ninja``'s answer, not ours: the same token list ninja
    writes into the response file.  It is what makes a *unit name* a usable
    argument - the object that belongs to a unit is the one the link names.
    """
    toks = derive_rsp(out)
    if toks is None:
        return None
    return [t for t in toks if re.search(r"\.(o|a)$", t, re.I)]


def unit_of_object(path):
    """The *unit name* of a build input: its path under ``build/<...>/{src,obj}/``.

    ``build/RMHE08/src/Network/NetworkWiiMediator.o`` ->
    ``Network/NetworkWiiMediator``; same for ``obj/`` (the split target
    object).  A path that does not have that shape returns its stem.
    """
    s = str(path).replace("\\", "/")
    m = re.search(r"/build/[^/]+/(?:src|obj)/(.+\.o)$", "/" + s)
    if m:
        return units.stem(m.group(1))
    return Path(s).name[:-2] if s.lower().endswith(".o") else Path(s).name


def resolve_object(spec, link_out="build/RMHE08/main.elf"):
    """Resolve a path **or a unit name** to the object a link consumes.

    Order, and why: an existing path is honoured first (a lane that already
    has one knows what it wants); then the build's *own* input list, which is
    ground truth for "which object is this unit" - a unit that is `linked
    False` appears there as ``obj/<unit>.o`` (the split target object) and a
    flipped one as ``src/<unit>.o`` (ours), and picking the wrong one silently
    traces a different object; then the two conventional paths.  ``how`` says
    which of those answered, so the report never implies more than it checked.
    """
    want = str(spec).replace("\\", "/").strip()
    stem = re.sub(r"\.o$", "", want, flags=re.I)
    stem = re.sub(r"^.*?/?(?:build/[^/]+/)?(?:src|obj)/", "", stem)

    cand = Path(want)
    if cand.exists():
        return {"path": cand, "unit": unit_of_object(cand), "how": "the path given",
                "in_link": None}
    root_cand = ROOT / want
    if root_cand.exists():
        return {"path": root_cand, "unit": unit_of_object(root_cand),
                "how": "the path given", "in_link": None}

    inputs = link_inputs(link_out) or []
    hits = [t for t in inputs if unit_of_object(t).lower() == stem.lower()]
    if not hits:
        # The link may name the same unit in more than one place (a `link`
        # statement and the objdiff list); keep only the first, and say how
        # many there were.
        hits = [t for t in inputs
                if unit_of_object(t).lower().endswith("/" + stem.lower())
                or Path(t.replace("\\", "/")).name.lower() == stem.lower() + ".o"]
    if hits:
        p = Path(hits[0].replace("\\", "/"))
        if not p.is_absolute():
            p = ROOT / p
        return {"path": p, "unit": unit_of_object(hits[0]),
                "how": f"the object the build's link names ({len(hits)} entry/entries)",
                "in_link": True}

    for sub in ("src", "obj"):
        p = ROOT / "build" / "RMHE08" / sub / (stem + ".o")
        if p.exists():
            return {"path": p, "unit": stem,
                    "how": f"build/RMHE08/{sub}/<unit>.o - NOT in the link's "
                           f"input list (this object is not linked)",
                    "in_link": False}
    known = sorted({unit_of_object(t) for t in inputs})
    import difflib
    near = difflib.get_close_matches(stem, known, n=5, cutoff=0.6)
    return {"path": None, "unit": stem, "how": "not found", "in_link": None,
            "known": known[:0], "near": near, "inputs": len(inputs)}


def link_argv(args, out_base="trace", verbose=False):
    """The argv for a link that can never write ``build/RMHE08/main.elf``.

    Either an explicit link line (``--args``, with any ``-o``/``-map`` removed
    and replaced by the scratch paths) or the build's own, derived from
    ``build.ninja``.  Returns ``(argv, elf_out, map_out, work, rsp)`` or
    ``None`` when the build statement cannot be read.
    """
    work = Path(args.out).resolve()
    work.mkdir(parents=True, exist_ok=True)
    elf_out = work / f"{out_base}.elf"
    map_out = work / f"{out_base}.MAP"
    if getattr(args, "args", None):
        toks, skip = [], False
        for tk in args.args.split():
            if skip:
                skip = False
                continue
            if tk in ("-o", "-map"):
                skip = True
                continue
            toks.append(tk)
        return ([str(args.linker)] + toks + ["-o", str(elf_out), "-map", str(map_out)],
                elf_out, map_out, work, Path(args.rsp) if args.rsp else None)
    rsp = Path(args.rsp) if args.rsp else None
    if rsp is None:
        rsp = work / "trace.rsp"
        if derive_rsp(args.link_out, rsp) is None:
            return None
    if not rsp.exists():
        return None
    args.rsp = str(rsp)
    line = derive_link_line(rsp, args.ldscript or "build/RMHE08/ldscript.lcf")
    if verbose:
        line = "-v " + line
    line = line.format(out=str(elf_out), map=str(map_out))
    return [str(args.linker)] + line.split(), elf_out, map_out, work, rsp


def run_link(args, out_base="trace", verbose=False):
    """Run the build's own link into ``build/scratch/``; never ``main.elf``.

    Returns ``(rc, argv, stdout, stderr, elf_out, map_out)``, or ``None`` when
    the link cannot even be derived.
    """
    got = link_argv(args, out_base, verbose)
    if got is None:
        return None
    argv, elf_out, map_out, _work, _rsp = got
    proc = lib_proc.run(argv, cwd=str(ROOT))
    return proc.returncode, argv, proc.stdout, proc.stderr, elf_out, map_out


def report_link_failure(args, argv, stdout, stderr, catalogue):
    """The production case: a link that errors.  Say what it said, and where.

    A flip whose DOL hash moves is a *failing* link, so the tool has to be
    useful on the failure rather than refusing it: the run is repeated once
    with ``-v`` (the phase stream the build does not ask for), the diagnostics
    are classified against the message catalogue, and each one is attributed to
    the last phase the linker announced before printing it.  Nothing is
    invented: a diagnostic the catalogue does not hold is reported without an
    id, and a diagnostic with no phase line before it says ``(no phase seen)``.
    """
    print(f"# link FAILED (the link's own diagnostics follow)")
    tail = (stdout or "") + (stderr or "")
    for line in tail.strip().splitlines()[-40:]:
        print("#   " + line)
    # The phase attribution needs the verbose stream, which the build's own
    # ldflags do not ask for; one retry into scratch is worth it here.
    again = run_link(args, out_base="diagnose", verbose=True)
    if again is None:
        return []
    rc2, argv2, out2, err2, _elf2, _map2 = again
    # Classify the *verbose* run's output when it produced any: concatenating
    # both runs would report every diagnostic twice.
    text = "\n".join(t for t in (out2, err2) if t) or "\n".join(
        t for t in (stdout, stderr) if t)
    timeline = parse_timeline(text, phase_kinds(catalogue))
    diags = classify_diagnostics(text, catalogue, timeline)
    print()
    print("the link's phase stream, up to the failure:")
    for kind, _line in timeline:
        print(f"  {kind}")
    if not timeline:
        print("  (no phase line was printed)")
    print("diagnostics (classified against the message catalogue):")
    if not diags:
        print("  (the linker printed no banner this tool recognises)")
    for d in diags:
        who = f"phase {d['phase']}" if d["phase"] else "(no phase seen)"
        mid = f"msgid={d['msgid']}" if d["msgid"] is not None else "msgid=(not in the catalogue)"
        print(f"  [{who}] {mid}: {d['text']}")
    return diags

