"""The build rows: each command's exit code, the compile gate scoped to the batch's objects, the `ok` stamp.
Spec: docs/tools/spec/landing.md. CLI: none (a module of the `land.py` gate)."""
from __future__ import annotations

import os
import sys
import time

from tools.lib import project as _project
from tools.lib.lanes import naming
from tools.units.landing.common import (Batch, _OBJECT_TARGET, command_detail, compile_targets,
    failed_compile_outputs, run)


def _added_object_calls(diff: str) -> list:
    """The `Object(...)` calls a diff's added lines spell (`lib.project.object_calls`, comments ignored)."""
    return [c for line in (diff or "").splitlines() if line.startswith("+") and not line.startswith("+++")
            for c in _project.object_calls(line[1:])]


def flips_objects(main: str) -> bool:
    """True when configure.py gains `Object(Matching, ...)` relative to HEAD - the batch flips something."""
    p = run(["git", "diff", "HEAD", "--", "configure.py"], main)
    return any(c.flag == "Matching" for c in _added_object_calls(p.stdout))


def flipped_units(main: str) -> list[str]:
    """Units whose `Object(...)` line gains `Matching` in configure.py relative to HEAD (a flip, or a new Matching unit)."""
    p = run(["git", "diff", "HEAD", "-U0", "--", "configure.py"], main)
    out = [naming.norm_unit(c.path) for c in _added_object_calls(p.stdout) if c.flag == "Matching"]
    return sorted(dict.fromkeys(out))


def batch_compile_failures(units: list[str], output: str) -> tuple[list[str], list[str]]:
    """-> (the batch's units whose object FAILED, foreign FAILED outputs).

    Scoping: only a failed output that is one of the batch's own `build/RMHE08/src/<unit>.o` targets is a
    failure of *this* batch. Every other failed output is another stream's dirty work in MAIN, which the
    batch did not touch and must not answer for - named, never counted, so a compile gate cannot make a
    passing batch fail for a reason that is not its own.
    """
    want: dict[str, str] = {}
    for unit in units:
        norm = naming.norm_unit(unit.strip("/"))
        if norm:
            want[_OBJECT_TARGET % norm] = norm
    bad: list[str] = []
    foreign: list[str] = []
    for path in failed_compile_outputs(output):
        unit = want.get(path)
        if unit is None:
            foreign.append(path)
        elif unit not in bad:
            bad.append(unit)
    return bad, foreign


def compile_check(main: str, units: list[str], runner=None) -> tuple[bool, str]:
    """Do the batch's own units still compile? -> (ok, detail) after one `ninja -k 0`.

    `ninja build/RMHE08/ok` is deliberately not asked (that is the DOL check); this runs the compile itself,
    once: `-k 0` keeps going past the first error, so every failure is visible in one run and a foreign
    failure cannot hide a batch unit's. The "did it compile" verdict is scoped by object target through
    `batch_compile_failures`, so another stream's broken dirty unit is reported, not counted.

    `runner` is a `run([...])`-shaped callable and exists for the selftest: it lets the scoping be exercised
    in both directions without a real build tree.
    """
    if not units:
        return True, "no batch unit named"
    if not os.path.exists(os.path.join(main, "build.ninja")):
        return True, "no build.ninja - the configure.py gate owns that"
    run_fn = runner or (lambda args: run(args, main))
    p = run_fn(["ninja", "-k", "0"])
    output = (p.stdout or "") + (p.stderr or "")
    bad, foreign = batch_compile_failures(units, output)
    if bad:
        return False, "FAILED to compile: %s" % ", ".join(bad)
    if p.returncode != 0 and not foreign:
        # ninja failed without naming an output: not a compile failure of a unit we can scope, so it is not
        # silently read as a pass - the anomaly is the detail.
        tail = [l.strip() for l in output.splitlines() if l.strip()]
        return False, "ninja -k 0 exited %d without a FAILED target: %s" % (p.returncode, tail[-1] if tail else "")
    detail = "all %d batch unit object(s) compiled" % len(compile_targets(units))
    if foreign:
        detail += (" (tolerated: %d FAILED foreign dirty target(s), not this batch's: %s)"
                   % (len(foreign), ", ".join(foreign[:3])))
    return True, detail


# --- the rows -------------------------------------------------------------------------------------------------

def command_row(b: Batch, name: str, args: list[str]) -> bool:
    """One command's exit code as a row (`<name> (exit N)`); -> whether it succeeded."""
    p = run(args, b.main)
    b.check(name + " (exit %d)" % p.returncode, p.returncode == 0, command_detail(p))
    return p.returncode == 0


def ok_paths(main: str) -> tuple[str, str]:
    return (os.path.join(main, "build", "RMHE08", "ok"), os.path.join(main, "build", "RMHE08", "main.elf"))


def clear_stamps(b: Batch) -> None:
    """Delete `ok` (and `main.elf` when the batch flips an object) so the later rows prove THIS run made them."""
    ok_file, elf_file = ok_paths(b.main)
    for stale in [ok_file] + ([elf_file] if b.extra.get("flip") else []):
        if os.path.exists(stale):
            os.remove(stale)
    b.extra["started"] = time.time_ns()


def configure_rows(b: Batch) -> bool:
    """10a. `configure.py`, then the split and `configure.py` again so the graph carries the batch's own units.

    The per-unit rules are generated from build/RMHE08/config.json and build.ninja depends on it, so a
    configure.py that precedes the split regenerates the graph from the *previous* analyzed config (2026-09-26,
    .pi/notes/8030681c-gate-finding.md): run the split (a no-op when current), then configure.py again."""
    built = command_row(b, "configure.py", [sys.executable, "configure.py"])
    if built and os.path.exists(os.path.join(b.main, "build.ninja")):
        command_row(b, "split (config.json)", ["ninja", "build/RMHE08/config.json"])
        built = command_row(b, "configure.py (after the split)", [sys.executable, "configure.py"]) and built
    return built


def compile_row(b: Batch) -> None:
    """10b. the compile gate: every batch unit's own object compiles (`compile_check`, one `ninja -k 0`)."""
    ok_compile, compile_detail = compile_check(b.main, b.unit_units)
    b.check("every batch unit compiles (compile gate)", ok_compile, compile_detail,
            remedy="make the batch unit's source compile (`ninja -k 0` names the error above); "
                   "`ninja build/RMHE08/ok` cannot see this because a `NonMatching` unit is never linked")


def ok_fresh_row(b: Batch) -> bool:
    """19. `ninja build/RMHE08/ok`, and the stamp was recreated by THIS run; -> whether it is fresh."""
    command_row(b, "ok (main.dol verified)", ["ninja", "build/RMHE08/ok"])
    ok_file, _elf = ok_paths(b.main)
    fresh = os.path.exists(ok_file) and os.stat(ok_file).st_mtime_ns >= b.extra.get("started", 0)
    b.check("ok was recreated by THIS run", fresh, "the ok stamp predates the run - it proves nothing")
    return fresh
