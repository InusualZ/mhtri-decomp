"""Write the one file a worker is handed: `tools/units/briefs/<slug>.md`.

docs/plan.md 7.3, §5.2. Four workers in separate processes inherit nothing from the orchestrator's context, so
the brief has to be self-contained and has to say the same thing every time. It has exactly six parts:

1. the unit      - path, lib, mw_version, the real cflags, object and target paths, the `.text` range,
   the **language** (C/C++, from `tools/units/langcheck.py` - the extension decides the front-end, so a
   worker has to be told which one it is and what that costs), and the shared headers (`include/**`)
   that already declare what this unit needs - so it reuses them instead of re-creating them
   (`tools/units/typeregistry.py`)
2. the inventory - every symbol the unit owns, its address, size and current measured %
3. the residuals - the unit's file-header comment, so a re-brief never re-derives settled work
4. the decided   - the flags landed for this lib, and the data ranges deliberately not claimed
5. the task      - the functions still under the bar, in address order (or an explicit --task)
6. the rules     - `docs/plan.md` §6.5 and §8 verbatim, plus the measurement loop

    python tools/units/brief.py <unit> [--task "..."] [--stdout] [--json] [--selftest]
    python tools/units/brief.py --pool [--force] [--no-prune]

The brief is written into MAIN (`<main>/tools/units/briefs/`), not into a worker's worktree, so it outlives
the worktree the same way the outbox does. Its file name and the paths in part 4 come from the unit's
**active claim**: the slug is the claim's branch minus `worker/` - the name `land.py`'s gate keys the outbox
by - never a name re-derived from the unit path, and a brief for an unclaimed unit says so instead of
inventing one.

`--pool` is the one exception, and it is what lets the orchestrator start a worker the instant a slot frees:
for every registered unit that has no bodies yet it writes a brief *before* any claim exists, keyed by
`claims.slug(unit)` and rendered against the worktree and branch the default claim will create, so
`tools/units/queue.py next` only has to claim the unit and copy the file. No claim is made and the registry
is never touched. Re-running is idempotent (existing briefs are skipped) and prunes a brief whose unit has
gained a body.
"""

from __future__ import annotations

import argparse
import json
import os
import re
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
# the brief carries the plan's text verbatim (<=, >=, em dashes), and a Windows console is cp1252: without this
# `--stdout` dies on the first such character while the file write (UTF-8) is fine
try:
    sys.stdout.reconfigure(encoding="utf-8")
    sys.stderr.reconfigure(encoding="utf-8")
except Exception:
    pass
sys.path.insert(0, os.path.dirname(HERE))

import unitutil  # noqa: E402
from units import recompile as rc  # noqa: E402
from units import claims  # noqa: E402
from units import langcheck as lc  # noqa: E402
from units import typeregistry  # noqa: E402
from units import dossier as dossier_mod  # noqa: E402

SRC_EXT = (".c", ".cpp", ".cp", ".cxx", ".cc")
BAR = 80.0


def source_name(unit: str, *roots: str) -> str:
    """`Pl/pl_act` -> `Pl/pl_act.cpp`; a full name is left alone.

    A unit's *identity* is extensionless (`claims.norm_unit`), so a caller that passes the short spelling must
    not get `.cpp` by default: `RSO/runtime` is `runtime.c` and `Camellia/camellia` is `camellia.c`. When a
    root is given, the real extension is resolved from the tree (`src/<unit>.<ext>`); with no root the old
    `.cpp` default stays, so the pure spelling rules are unchanged for callers that do not need a file.
    """
    unit = claims.norm_unit(unit.strip("/"))
    if unit.endswith(SRC_EXT):
        return unit
    for root in roots:
        if not root:
            continue
        for ext in SRC_EXT:
            if os.path.exists(os.path.join(root, "src", *unit.split("/")) + ext):
                return unit + ext
    return unit + ".cpp"


def strip_comments(text: str) -> str:
    """Source with its comments removed - a placeholder's whole content is a header comment."""
    text = re.sub(r"/\*.*?\*/", "", text, flags=re.S)
    return re.sub(r"//[^\n]*", "", text)


def has_bodies(path: str) -> bool:
    """Whether a source file defines at least one function body.

    The attribution placeholders (`src/auto/*`) are a file-header comment and nothing else, so a brace outside
    the comments is the honest, cheap signal that a body exists. This is deliberately a source-level
    heuristic: it decides which units the *pool* has nothing to hand a worker for, never whether a unit is
    finished.
    """
    if not os.path.exists(path):
        return False
    return "{" in strip_comments(open(path, encoding="utf-8", errors="replace").read())


def registered_units(main: str) -> list[str]:
    """Every source file registered in `configure.py`'s `config.libs`, in registration order.

    Parsed from the file rather than from `build/` so it does not depend on a generated build tree: a unit
    whose object has never been built still counts as registered.
    """
    text = open(os.path.join(main, "configure.py"), encoding="utf-8", errors="replace").read()
    return re.findall(r'Object\([^,]+,\s*"([^"]+)"', text)


def pool_units(main: str) -> list[str]:
    """Registered units whose source exists and has no bodies yet, extensionless, sorted by name.

    This is the pool's definition: a unit is pooled exactly while there is nothing to hand a worker except
    the inventory and the rules. A unit that gains a body (a worker's source lands) drops out on the next
    `--pool` run, and a registered unit with no source at all is reported separately, never pooled.
    """
    out = []
    for src in registered_units(main):
        path = os.path.join(main, "src", *src.split("/"))
        if os.path.exists(path) and not has_bodies(path):
            out.append(claims.norm_unit(src))
    return sorted(set(out))


def config_schema_lines() -> list[str]:
    """The outbox `config_requests` schema as the brief renders it, from handoff.py's one definition.

    The brief is what the worker is told to follow, so it has to state the vocabulary the validator accepts:
    a worker that invents `data`/`flags`/`tool` (or a non-object `flags_probed`) produces a handoff the gate
    refuses, which is exactly what happened to `RSO/runtime`'s first outbox. Imported lazily because
    `handoff.py` imports this module at load time.
    """
    from units import handoff as handoff_mod  # local: handoff imports brief, so a top-level import would cycle
    lines = ["",
             "The validator accepts exactly these kinds - the fields marked * are required:",
             "",
             "| kind | required | also | what it is |",
             "| --- | --- | --- | --- |"]
    for row in handoff_mod.config_schema_rows():
        needs = ", ".join("`%s`*" % f for f in row["needs"]) or "(none)"
        also = ", ".join("`%s`" % f for f in row["also"]) or ""
        lines.append("| `%s` | %s | %s | %s |" % (row["kind"], needs, also, row["means"]))
    lines.append("")
    lines.append("`flags_probed` is a list of `{ \"flags\", \"effect\", \"verdict\" }` objects, with the"
                 " verdict one of `%s`. `python tools/units/handoff.py <unit> --template` prints the whole"
                 " skeleton, and `handoff.py <unit> --check <file>` validates what you wrote."
                 % "/".join(handoff_mod.FLAG_PROBE_VERDICTS))
    return lines


def claim_for(main: str, unit: str) -> dict:
    """The unit's active claim from `MAIN/.pi/claims.json` - `{}` when it is unclaimed.

    Read through `claims.registry_record`, so the unit may be spelled with or without its source extension:
    the registry key is the unit's *name* (extensionless), and a claim written before `norm_unit` existed may
    still carry the extension (`Gecko/Gecko_ExceptionPPC.cp`).
    """
    return claims.registry_record(main, unit)


def claim_slug(claim: dict) -> str | None:
    """The handoff slug `land.py` keys the outbox by: the claim's branch minus `worker/`.

    The one implementation is `claims.slug_of_branch`; this is the dict-shaped wrapper `handoff_paths` uses.
    Read from the branch instead of re-deriving it from the unit path: the branch *is* the claim's identity (it
    is the lock), a round may name it with its own suffix, and the gate looks the outbox up by the branch. An
    unclaimed unit has no branch and therefore no slug: never invent one.
    """
    return claims.slug_of_branch((claim or {}).get("branch"))


def unclaimed_notice(main: str, unit: str) -> str:
    """The plain statement a brief for an unclaimed unit carries in place of an invented slug."""
    return ("**This unit has no active claim.** `%s` carries no `branch` for `%s`, so there is no slug to key "
            "the outbox by - the gate reads `MAIN/.pi/outbox/<branch minus worker/>.json`, and a made-up name "
            "is refused. Claim the unit first (`python tools/units/claims.py claim %s`) and use the brief "
            "written afterwards." % (claims.registry_path(main), unit, unit))


def handoff_paths(main: str, unit: str, assume_claim: bool = False) -> dict:
    """Where a worker's artefacts go, derived from the claim (see `claim_slug`).

    The ack file and the rescue ref stay `claims.py`'s own (`claims.ack_path`/`claims.slug`): the heartbeat is
    written by `claims.py ack`, so the brief has to name the file that command actually writes.

    `assume_claim` is for the pool: a unit with no claim yet is rendered against the branch `claims.py claim`
    *will* make (`worker/<slug(unit)>`), so the pooled brief already carries the outbox and notes paths its
    real claim will produce. The registry is never written - the assumed claim exists only in the returned
    dict.
    """
    unit = unit.strip("/")
    claim = claim_for(main, unit)
    if not claim and assume_claim:
        claim = {"branch": claims.branch_for(claims.norm_unit(unit))}
    slug = claim_slug(claim)
    return {
        "claimed": bool(slug),
        "branch": claim.get("branch"),
        "slug": slug,
        "ack": claims.ack_path(main, unit),
        "rescue": "refs/rescue/%s" % claims.slug(unit),
        "outbox": claims.outbox_path(main, unit) if slug else None,
        "notes": claims.notes_path(main, unit) if slug else None,
    }


def splits_range(main: str, unit: str) -> dict:
    """The unit's section ranges from `splits.txt`, e.g. {'.text': (start, end, size)}.

    Blocks are matched by *stem*, so either spelling of the unit finds them: the map and the registry key a
    unit by its extensionless name, while `splits.txt` names the source file (`Camellia/camellia.c`).
    """
    path = os.path.join(main, "config", "RMHE08", "splits.txt")
    want = os.path.splitext(source_name(unit, main))[0]
    out, current = {}, None
    if not os.path.exists(path):
        return out
    for line in open(path, encoding="utf-8", errors="replace"):
        if line[:1] not in (" ", "\t") and line.rstrip().endswith(":") and not line.startswith("#"):
            current = line.strip()[:-1]
            continue
        m = re.match(r"\s+(\S+)\s+start:(0x[0-9A-Fa-f]+)\s+end:(0x[0-9A-Fa-f]+)", line)
        if m and current and os.path.splitext(current.lstrip("/"))[0] == want:
            start, end = int(m.group(2), 16), int(m.group(3), 16)
            out[m.group(1)] = (start, end, end - start)
    return out


_MAP_CACHE: dict = {}


def map_rows(main: str) -> list[dict] | None:
    """Every symbol in the map, parsed once per (path, mtime) - `None` when the map is missing.

    `--pool` briefs every stub in one process and the map is 4.5 MB; parsing it per unit would read it
    hundreds of times. The cache is keyed on the mtime, so a rename or a re-split invalidates it.
    """
    path = os.path.join(main, "config", "RMHE08", "symbols.txt")
    if not os.path.exists(path):
        return None
    key = (path, os.path.getmtime(path))
    if key not in _MAP_CACHE:
        pattern = re.compile(r"^(\S+) = (\S+):(0x[0-9A-Fa-f]+); // type:(\w+)( size:(0x[0-9A-Fa-f]+))?")
        rows = []
        with open(path, encoding="utf-8", errors="replace") as fh:
            for line in fh:
                m = pattern.match(line)
                if not m:
                    continue
                rows.append({"name": m.group(1), "section": m.group(2), "address": int(m.group(3), 16),
                             "size": int(m.group(6), 16) if m.group(6) else 0, "type": m.group(4)})
        _MAP_CACHE.clear()
        _MAP_CACHE[key] = rows
    return _MAP_CACHE[key]


def symbols_in_range(main: str, start: int, end: int) -> list[dict]:
    """The unit's symbols from the map, parsed in-process.

    `symbols.txt` must never reach an agent's context, but a tool reading it programmatically is exactly what
    `attribute.py` and `tudiscover` do. Parsing it here instead of shelling out to `symedit.py --json` also
    avoids that command's multi-document output, which is not a single JSON value.

    A failure is reported, never swallowed: an empty inventory would let a worker believe a unit is finished.
    """
    path = os.path.join(main, "config", "RMHE08", "symbols.txt")
    if not os.path.exists(path):
        print("WARNING: no %s" % path, file=sys.stderr)
        return []
    rows = [dict(r) for r in map_rows(main) if start <= r["address"] < end]
    if not rows:
        print("WARNING: no symbols found in 0x%X-0x%X - check the split range" % (start, end), file=sys.stderr)
    return rows


_REPORT_CACHE: dict = {}


def report_scores(main: str, unit: str) -> dict:
    """Per-symbol scores for this unit out of `report.json`, keyed by symbol name.

    Cached once per (path, mtime) for the same reason as `map_rows`: the report is 7 MB and `--pool` briefs
    hundreds of units in one process.
    """
    path = os.path.join(main, "build", "RMHE08", "report.json")
    if not os.path.exists(path):
        return {}
    key = (path, os.path.getmtime(path))
    if key not in _REPORT_CACHE:
        _REPORT_CACHE.clear()
        _REPORT_CACHE[key] = json.loads(open(path, encoding="utf-8").read())
    data = _REPORT_CACHE[key]
    want = source_name(unit.strip("/"))
    stem = os.path.splitext(want)[0]
    best = {}
    for u in data.get("units", []):
        name = (u.get("name") or "").replace("\\", "/")
        if not (name.endswith(want) or name.endswith(stem) or name.endswith("/" + stem)):
            continue
        for fn in u.get("functions") or []:
            pct = fn.get("fuzzy_match_percent", fn.get("match_percent"))
            if fn.get("name") and pct is not None:
                best[fn["name"]] = float(pct)
    return best


def header_comment(main: str, wt: str, unit: str) -> str:
    """The unit's file-header comment - the place its residuals live."""
    name = source_name(unit, wt, main)
    for root in (wt, main):
        path = os.path.join(root, "src", *name.split("/"))
        if not os.path.exists(path):
            continue
        text = open(path, encoding="utf-8", errors="replace").read()
        m = re.match(r"\s*(/\*.*?\*/|//[^\n]*(?:\n//[^\n]*)*)", text, re.S)
        if m:
            return m.group(1).strip()
    return "(no source file yet)"


def flags_for(main: str, unit: str) -> tuple[list[str], str]:
    """The unit's real flags, **values included** - `-proc gekko`, not `-proc`.

    The old filter (`[t for t in tokens if t.startswith("-")]`) dropped every value token, so the brief's
    "real command line" rendered `-proc -align -enum -fp -Cpp_exceptions -pragma -pragma -maxerrors ...
    -i -i ... -inline -lang=c++ -MMD -c -o` - a line that tells a worker nothing about the flags it is
    reading the brief for. `unitutil.split_flags` is the one definition of where the flags start and end, so
    the rendering cannot drift from the command line `recompile.py` and the flag tools run. The
    `-c <src> -o <dir>` tail is dropped on purpose: `recompile.py` builds it, and it is MAIN's.
    """
    try:
        tokens = rc.ninja_command(main, unit)
    except (SystemExit, OSError) as exc:
        return [], str(exc)
    _head, flags, _tail = unitutil.split_flags(tokens)
    return flags, ""


def lib_for(main: str, unit: str) -> str:
    text = open(os.path.join(main, "configure.py"), encoding="utf-8", errors="replace").read()
    want = "\"%s\"" % source_name(unit, main)
    for block in re.finditer(r"\{\s*\n\s*\"lib\": \"([^\"]+)\"[^{}]*?\"objects\": \[(.*?)\]\s*,\s*\n\s*\}", text, re.S):
        if want in block.group(2):
            return block.group(1)
    return "(unknown)"


def shared_headers(main: str, wt: str, unit: str, symbol_names) -> list:
    """The shared headers this unit should reuse, from `typeregistry`.

    The unit's own source (the worktree's copy wins, so a worker re-briefed mid-unit sees its own state) is
    matched against every `include/**` declaration by three signals: it includes the header, it defines one
    of the header's names (the duplication), or it / one of its owned map symbols names one. The result is
    what part 1 renders; an empty list still carries the rule, so a worker with no match knows to ask rather
    than define locally (`docs/plan.md` 6.5 rule 1).
    """
    try:
        reg = typeregistry.registry(main)
    except OSError:
        return []
    name = source_name(unit, wt, main)
    rel = os.path.join("src", *name.split("/")).replace("\\", "/")
    text = ""
    for root in (wt, main):
        path = os.path.join(root, "src", *name.split("/"))
        if os.path.exists(path):
            text = open(path, encoding="utf-8", errors="replace").read()
            break
    return typeregistry.relevant_headers(reg, text, symbol_names, unit_file=rel)


def plan_section(main: str, heading: str) -> str:
    """One section of docs/plan.md, verbatim - the rules have exactly one source."""
    path = os.path.join(main, "docs", "plan.md")
    if not os.path.exists(path):
        return ""
    text = open(path, encoding="utf-8", errors="replace").read()
    start = text.find(heading)
    if start < 0:
        return ""
    nxt = re.search(r"\n## ", text[start + len(heading):])
    return text[start:start + len(heading) + (nxt.start() if nxt else len(text))].strip()


def data_queue_entries(main: str, unit: str) -> list[dict]:
    path = os.path.join(main, "tools", "units", "data-queue.json")
    if not os.path.exists(path):
        return []
    try:
        data = json.loads(open(path, encoding="utf-8").read())
    except json.JSONDecodeError:
        return []
    return [e for e in (data if isinstance(data, list) else data.get("entries", []))
            if e.get("unit") == unit]


def build(main: str, wt: str, unit: str, task: str | None, assume_claim: bool = False) -> dict:
    unit = claims.norm_unit(unit.strip("/"))
    name = source_name(unit, wt, main)
    rng = splits_range(main, unit)
    text_range = rng.get(".text")
    syms = symbols_in_range(main, text_range[0], text_range[1]) if text_range else []
    scores = report_scores(main, unit)
    tokens, err = flags_for(main, unit)
    lib = lib_for(main, unit)
    obj = os.path.join(wt, "build", "RMHE08", "src", *name.split("/"))
    obj = os.path.splitext(obj)[0] + ".o"
    target = os.path.join(main, "build", "RMHE08", "obj", *name.split("/"))
    target = os.path.splitext(target)[0] + ".o"
    for sym in syms:
        sym["percent"] = scores.get(sym["name"])
    below = [s for s in syms if (s.get("percent") is None or s["percent"] < BAR)]
    handoff = handoff_paths(main, unit, assume_claim)
    return {
        "unit": unit, "slug": handoff["slug"], "claimed": handoff["claimed"],
        "branch": handoff["branch"], "handoff": handoff, "lib": lib, "worktree": wt, "main": main,
        "source": os.path.join(wt, "src", *name.split("/")),
        "object": obj, "target": target,
        "sections": rng, "flags": tokens, "flag_error": err,
        # the language verdict: the extension is what picks the front-end, so this is not cosmetic (see
        # langcheck). `object_verdict` returns lang=None for a missing target and never raises.
        "language": lc.object_verdict(target),
        "symbols": syms, "below_bar": len(below),
        "task": task, "task_symbols": [s["name"] for s in below] if not task else [],
        "header": header_comment(main, wt, unit),
        "data_queue": data_queue_entries(main, unit),
        # the binary dossier: the traces the split object already carries (source file, asserts, pool,
        # jump tables) - rendered as part 2b so a worker starts from the maximum the binary gives
        "dossier": dossier_mod.build(main, unit, target, ranges=rng, map_symbols=syms),
        "shared_headers": shared_headers(main, wt, unit, [s["name"] for s in syms]),
    }


def render(main: str, b: dict, task: str | None, pool: bool = False) -> str:
    rng = b["sections"]
    txt = rng.get(".text")
    lines = []
    lines.append("# Brief: %s" % b["unit"])
    lines.append("")
    if pool:
        lines.append("> **Pooled brief** - prepared by `brief.py --pool` before the claim. `queue.py next` claims")
        lines.append("> this unit and hands you this file; the worktree and outbox paths below are the ones your")
        lines.append("> claim will have. Do not act on a pooled brief you were not handed.")
        lines.append("")
    lines.append("Read this file, do the task, write your report where §4 says. Nothing outside this file is a rule.")
    lines.append("")
    lines.append("## 0 · Acknowledge first, then heartbeat")
    lines.append("")
    lines.append("Before anything else, say you are alive:")
    lines.append("")
    lines.append("```sh")
    lines.append("python tools/units/claims.py ack %s --agent <your-name> --pane <your-pane>" % b["unit"])
    lines.append("```")
    lines.append("")
    lines.append("That writes `%s`. Re-run it **with `--progress <symbol>` every time you finish a "
                 "function** - it is the heartbeat by which the orchestrator tells a stalled worker from a "
                 "working one, and it takes a second." % b["handoff"]["ack"])
    lines.append("")
    lines.append("**The timeout policy:** if there is no ack within **%d seconds**, or no progress for "
                 "**%d minutes**, the orchestrator reclaims the unit - your commits are copied to "
                 "`%s` first, then the worktree goes away and the unit is re-briefed to someone "
                 "else. Talk to the orchestrator with the ack and the outbox, not by being busy."
                 % (120, 20, b["handoff"]["rescue"]))
    lines.append("")
    lines.append("## 1 · The unit")
    lines.append("")
    lines.append("| | |")
    lines.append("| --- | --- |")
    lines.append("| unit | `%s` |" % b["unit"])
    lines.append("| lib | `%s` |" % b["lib"])
    lines.append("| source (yours) | `%s` |" % b["source"])
    lines.append("| object (yours) | `%s` |" % b["object"])
    lines.append("| target (read-only, MAIN) | `%s` |" % b["target"])
    lines.append("| worktree | `%s` |" % b["worktree"])
    lines.append("| build | **yours**: `build/RMHE08` in your worktree - compile and measure there, never in MAIN's `build/` |")
    lines.append("| edits | **yours only**: change files inside your worktree. MAIN's working tree belongs to the orchestrator, and other workers are often mid-round in it - an edit there blocks every batch gate |")
    lines.append("| sections | %s |" % (", ".join("%s 0x%X-0x%X" % (s, a, e) for s, (a, e, _n) in sorted(rng.items()))
                                        or "(none in splits.txt)"))
    lines.append("| language | %s |" % lc.language_cell(b.get("language")))
    if b["flags"]:
        lines.append("")
        lines.append("The real command line (flags only; `recompile.py` builds the full one):")
        lines.append("")
        lines.append("```")
        # a valued flag can be one token with a space in it (`-pragma cats off`); quote it back so the line
        # reads as the tokens the compiler actually gets
        lines.append(" ".join('"%s"' % t if " " in t else t for t in b["flags"]))
        lines.append("```")
        lines.append("")
        lines.append("The `-i` directories are MAIN's and are searched **in the order shown**, so never run "
                     "this line with MAIN as the working directory: `recompile.py` rewrites them to your "
                     "worktree's (yours first) and that is the compile a measurement has to come from - a "
                     "header you edited in your worktree is otherwise shadowed by MAIN's copy, silently.")
    lines.append("")
    lines.append(lc.brief_paragraph(b.get("language")))
    sh = b.get("shared_headers") or []
    lines.append("")
    lines.append("**Shared headers this unit should reuse** (`docs/plan.md` \u00a76.5 rule 1, AGENTS.md -> "
                 "Repository layout): a type or helper another unit already declares belongs under `include/` - "
                 "include it, never copy it. `include/**` is read-only for you: a change there goes in the outbox's "
                 "`config_requests`, not in your branch.")
    if sh:
        lines.append("")
        lines.append("| header | why | declaration(s) |")
        lines.append("| --- | --- | --- |")
        for h in sh:
            why = []
            if "duplicated" in h["reasons"]:
                why.append("**you define these too - include the header and delete your copies**")
            if "included" in h["reasons"]:
                why.append("already included")
            if "used" in h["reasons"]:
                why.append("you name these")
            if "mentioned" in h["reasons"]:
                why.append("your file header names these")
            names = h["duplicated"] + [n for n in h["used"] if n not in h["duplicated"]]
            shown = ", ".join("`%s`" % n for n in names[:12])
            if len(names) > 12:
                shown += " (+%d more)" % (len(names) - 12)
            lines.append("| `%s` | %s | %s |" % (h["header"], "; ".join(why), shown))
    else:
        lines.append("")
        lines.append("No shared header declares anything this unit names yet. If you need a type or helper another "
                     "unit already uses, do not define it here - put it in the outbox's `config_requests` so it can "
                     "move to `include/` first.")
    lines.append("")
    lines.append("## 2 · The inventory (every symbol the unit owns, and where it stands)")
    lines.append("")
    lines.append("| symbol | address | size | measured % |")
    lines.append("| --- | --- | --- | --- |")
    for s in b["symbols"]:
        pct = s.get("percent")
        mark = "" if (pct is not None and pct >= BAR) else " **<- open**"
        lines.append("| `%s` | 0x%X | %s | %s%s |"
                     % (s["name"], s["address"] or 0, s["size"], "n/a" if pct is None else "%.2f" % pct, mark))
    lines.append("")
    if b.get("dossier"):
        lines.append(dossier_mod.render(b["dossier"]).rstrip())
        lines.append("")
    lines.append("## 3 · What is already known (the unit's own header, verbatim)")
    lines.append("")
    lines.append("```c")
    lines.append(b["header"])
    lines.append("```")
    lines.append("")
    lines.append("## 4 · Where your output goes")
    lines.append("")
    lines.append("* your source, committed **on your branch** (one commit): `%s`" % b["source"])
    if b["handoff"]["claimed"]:
        lines.append("* `%s` - the outbox `land.py`'s gate reads. It is named after your claim's branch "
                     "(`%s` minus `worker/`), so write it exactly here; do not invent a name."
                     % (b["handoff"]["outbox"], b["handoff"]["branch"]))
        lines.append("* `%s`" % b["handoff"]["notes"])
        lines.append("* a ≤ 15-line digest in your reply")
    else:
        lines.append("* %s" % unclaimed_notice(main, b["unit"]))
        lines.append("* a ≤ 15-line digest in your reply")
    lines.append("")
    lines.append("**End your turn with your report as the final message.** The orchestrator runs you through the "
                 "`subagent` tool, which returns to it when your process exits, so your last assistant message "
                 "*is* the handoff; a pane-launched worker delivers the same message plus its outbox, and the "
                 "orchestrator reads them once the pane is idle. There is no completion tool to call - ending on a "
                 "tool call (or saying nothing) hands back an empty result, so make the digest the last thing you "
                 "write.")
    lines.append("")
    lines.append("## 5 · The task")
    lines.append("")
    if task:
        lines.append(task)
    elif b["task_symbols"]:
        lines.append("%d of this unit's symbols are below the %.0f %% bar. Work them **in address order**, "
                     "biggest first where two are equal:" % (b["below_bar"], BAR))
        lines.append("")
        for name in b["task_symbols"][:40]:
            lines.append("* `%s`" % name)
        if len(b["task_symbols"]) > 40:
            lines.append("* ... and %d more" % (len(b["task_symbols"]) - 40))
    elif not b["symbols"]:
        lines.append("**Do not start: the inventory came back empty** - the unit's range is not in splits.txt, its symbols are not in the map, or the map proxy failed. Report it instead of guessing; the warning printed when this brief was generated says which of the three it was.")
    else:
        lines.append("Every symbol is at or above the bar. Confirm it, then improve the worst one if you can do so "
                     "without regressing anything.")
    if b["data_queue"]:
        lines.append("")
        lines.append("**Data this unit may own** (measured second pass - propose, do not claim): "
                     + ", ".join("%s 0x%X-0x%X (%s)" % (e.get("section"), e.get("start", 0), e.get("end", 0),
                                                        e.get("verdict", "unmeasured")) for e in b["data_queue"]))
    lines.append("")
    lines.append("## 6 · The rules")
    lines.append("")
    lines.append("Measurement loop (never `ninja`, never the link, never the split):")
    lines.append("")
    lines.append("```sh")
    lines.append("**Build in your worktree, never in MAIN's.** `build/RMHE08` there is yours alone; MAIN's belongs to")
    lines.append("the orchestrator, and several workers share this machine. The target objects you diff against are read-only")
    lines.append("in MAIN, and your worktree's `build/tools` is seeded with the toolchain (`dtk`, `objdiff-cli`, `sjiswrap`).")
    lines.append("")
    lines.append("**Before you hand-roll a search, use the two tools that do it mechanically:**")
    lines.append("")
    lines.append("```")
    lines.append("python tools/flags/infer.py --unit %s      # which flags the TARGET object implies" % b["unit"])
    lines.append("python tools/flags/shapesearch.py -u %s --scan 20   # generate/compile/score/rank source variants" % b["unit"])
    lines.append("```")
    lines.append("")
    lines.append("`infer.py` reads the target bytes and names the flags (it is 100 %% correct on 36 confident claims and abstains rather than guess); `shapesearch.py` writes variants of your source, compiles each with the real command line into a scratch copy and ranks them by the official metric - it took two functions to 100 % on `Pl/pl_act` in 30 seconds. A shapesearch *miss* is informative: zero differing rows with a 99.9 % score means relocation naming, so the data claim is the fix, not a shape.")
    lines.append("")
    lines.append("python tools/units/recompile.py %s --measure <symbol>   # compiles YOUR source in YOUR worktree" % b["unit"])
    lines.append("```")
    lines.append("")
    lines.append("A worker never touches `splits.txt`, `configure.py`, `symbols.txt` or `AGENTS.md`, never runs "
                 "`ninja`/the split/the link/`ok`, and never commits on `main`. Everything you need changed goes "
                 "into the outbox's `config_requests`.")
    lines.extend(config_schema_lines())
    lines.append("")
    lines.append("**You may fan out subagents** for parallel work. They run in *your* worktree, on *your* branch; "
                 "they never commit (you make the one commit); you assign them disjoint files or functions; and you "
                 "are accountable for what they produce - **re-measure every claim they make**, exactly as the "
                 "orchestrator re-measures yours. Hand each of them this whole part verbatim: a subagent that has "
                 "not read it will name a field `unk4`, reach it with a pointer cast, or use a `goto`, and that "
                 "becomes repair work charged to you.")
    lines.append("")
    lines.append(plan_section(main, "### 6.5 Type and naming discipline"))
    lines.append("")
    lines.append(plan_section(main, "## 8. Invariants"))
    return "\n".join(lines).rstrip() + "\n"


def brief_unit(path: str) -> str | None:
    """The unit a written brief names, from its `# Brief: <unit>` title - `None` when it is not a brief."""
    try:
        with open(path, encoding="utf-8", errors="replace") as fh:
            for line in fh:
                m = re.match(r"# Brief: (\S+)", line)
                if m:
                    return m.group(1)
    except OSError:
        return None
    return None


def pool_dir(main: str) -> str:
    return os.path.join(main, "tools", "units", "briefs", "pool")


def pool(main: str, force: bool = False, prune: bool = True) -> dict:
    """Write a brief for every registered unit with no bodies yet into `tools/units/briefs/pool/`.

    The point (owner's ask, 2026-09-23): the brief's content is unit-derived, so it can be prepared before any
    worker exists. Only its *file name* used to need a claim, and the pooled name is the same
    `claims.slug(unit)` the default claim's branch yields - so the orchestrator starts a worker the moment a
    slot frees with `queue.py next` and no derivation.

    No claim is made and the claims registry is never touched. `assume_claim` renders each brief against the
    worktree and branch the claim *will* create, so promoting it later is a copy.

    Idempotent: an existing brief is skipped unless `force`. A pooled brief whose unit has gained a body (a
    worker's source landed) is pruned, so the pool always equals the current no-body set.
    """
    units = pool_units(main)
    outdir = pool_dir(main)
    os.makedirs(outdir, exist_ok=True)
    wrote, skipped, pruned = [], [], []
    want = {claims.slug(u): u for u in units}
    for unit in units:
        path = os.path.join(outdir, claims.slug(unit) + ".md")
        if os.path.exists(path) and not force:
            skipped.append(unit)
            continue
        b = build(main, claims.worktree_for(unit, main), unit, None, assume_claim=True)
        open(path, "w", encoding="utf-8", newline="\n").write(render(main, b, None, pool=True))
        wrote.append(unit)
    if prune:
        for name in sorted(os.listdir(outdir)):
            if not name.endswith(".md") or name[:-3] in want:
                continue
            path = os.path.join(outdir, name)
            pruned.append({"slug": name[:-3], "unit": brief_unit(path)})
            os.remove(path)
    return {"dir": outdir, "units": units, "wrote": wrote, "skipped": skipped, "pruned": pruned}


def selftest() -> int:
    fails, checks = [], 0

    def check(name, got, want):
        nonlocal checks
        checks += 1
        if got != want:
            fails.append("%s: got %r want %r" % (name, got, want))

    check("source_name adds the extension", source_name("Pl/pl_act"), "Pl/pl_act.cpp")
    check("source_name keeps one", source_name("main.cpp"), "main.cpp")
    check("source_name's .cpp default is unchanged without a root", source_name("RSO/runtime"), "RSO/runtime.cpp")
    check("source_name resolves the real extension from the tree", source_name("RSO/runtime", "."), "RSO/runtime.c")
    check("source_name strips an extension first (norm_unit)", source_name("Camellia/camellia.c", "."),
          "Camellia/camellia.c")
    check("splits parses a block", splits_range(".", "Pl/pl_act").get(".text") is not None
          and len(splits_range(".", "Pl/pl_act")[".text"]) == 3, True)
    check("splits finds a .c unit from the extensionless spelling",
          splits_range(".", "RSO/runtime").get(".text") is not None, True)
    check("splits finds it from the .c spelling too",
          splits_range(".", "RSO/runtime.c") == splits_range(".", "RSO/runtime"), True)
    check("splits finds Camellia both ways",
          splits_range(".", "Camellia/camellia") == splits_range(".", "Camellia/camellia.c"), True)
    check("splits ignores an unknown unit", splits_range(".", "Nope/nothing"), {})

    # the pool's definition: a registered unit whose source exists and has no bodies yet. A temp fixture keeps
    # the checks independent of which stubs the real tree happens to have written
    import tempfile
    with tempfile.TemporaryDirectory() as tmp:
        os.makedirs(os.path.join(tmp, "src", "auto"))
        open(os.path.join(tmp, "src", "auto", "stub.c"), "w").write("/* header only */\n")
        open(os.path.join(tmp, "src", "auto", "done.c"), "w").write("/* header */\nint f(void) { return 1; }\n")
        open(os.path.join(tmp, "configure.py"), "w").write(
            'config.libs = [\n    {\n        "lib": "auto",\n        "objects": [\n'
            '            Object(NonMatching, "auto/stub.c"),\n'
            '            Object(NonMatching, "auto/done.c"),\n'
            '            Object(NonMatching, "auto/missing.c"),\n'
            "        ],\n    },\n]\n")
        check("registered_units parses the object list", registered_units(tmp),
              ["auto/stub.c", "auto/done.c", "auto/missing.c"])
        check("has_bodies: a header-only placeholder", has_bodies(os.path.join(tmp, "src", "auto", "stub.c")), False)
        check("has_bodies: a written body", has_bodies(os.path.join(tmp, "src", "auto", "done.c")), True)
        check("has_bodies: a missing file", has_bodies(os.path.join(tmp, "src", "auto", "missing.c")), False)
        check("pool_units keeps only the registered no-body source", pool_units(tmp), ["auto/stub"])
        check("strip_comments does not count a brace in a comment",
              has_bodies(os.path.join(tmp, "src", "auto", "stub.c")), False)
        # a pooled brief assumes the claim `queue.py next` will make, so its outbox path is already right
        h = handoff_paths(tmp, "auto/stub", assume_claim=True)
        check("assume_claim marks the brief claimed", h["claimed"], True)
        check("assume_claim's slug is the default branch's", h["slug"], claims.slug("auto/stub"))
        check("assume_claim's outbox is the fallback path", h["outbox"], claims.outbox_path(tmp, "auto/stub"))
        check("assume_claim writes no registry", claims.load_registry(tmp), {})
        claims.save_registry(tmp, {"auto/stub": {"branch": "worker/" + claims.slug("auto/stub") + "-zz"}})
        check("a real claim wins over assume_claim",
              handoff_paths(tmp, "auto/stub", assume_claim=True)["slug"], claims.slug("auto/stub") + "-zz")
        claims.save_registry(tmp, {})
        # the pool writes one brief, is idempotent, and prunes a unit that gained a body
        out = pool(tmp)
        check("pool writes the stub's brief", out["wrote"], ["auto/stub"])
        brief_path = os.path.join(pool_dir(tmp), claims.slug("auto/stub") + ".md")
        check("the pooled brief is named by the unit slug", os.path.exists(brief_path), True)
        check("the pooled brief is parseable", brief_unit(brief_path), "auto/stub")
        check("the pooled brief carries the claim's outbox", "outbox" in open(brief_path, encoding="utf-8").read()
              and "no active claim" not in open(brief_path, encoding="utf-8").read(), True)
        brief_text = open(brief_path, encoding="utf-8").read()
        check("the handoff is the final message, not a subagent_done sidecar",
              "final message" in brief_text and "subagent_done" not in brief_text, True)
        check("pool is idempotent", pool(tmp)["skipped"], ["auto/stub"])
        open(os.path.join(tmp, "src", "auto", "stub.c"), "w").write("int f(void) { return 1; }\n")
        check("pool prunes a unit that gained a body",
              [p["unit"] for p in pool(tmp)["pruned"]], ["auto/stub"])

    # the brief's own schema table is what a worker follows, so an outbox shaped by it must validate clean
    from units import handoff as handoff_mod
    table = "\n".join(config_schema_lines())
    check("the brief names every config kind", all(k in table for k in handoff_mod.CONFIG_KINDS), True)
    check("the brief names the required fields",
          all(f in table for row in handoff_mod.config_schema_rows() for f in row["needs"]), True)
    check("the brief names every flags_probed field",
          all(f in table for f in handoff_mod.FLAG_PROBE_FIELDS), True)
    brief_shaped = {"unit": "Pl/pl_act", "worker": "a", "finished_at": "2026-01-01T00:00:00",
                    "unit_percent": 50.0, "symbols": [{"name": "fn_1", "percent": 50.0}],
                    "residual": "none", "measured_with": "recompile.py", "blockers": [],
                    "config_requests": [], "flags_probed": []}
    for row in handoff_mod.config_schema_rows():
        brief_shaped["config_requests"].append(
            dict({"kind": row["kind"]}, **{f: "<%s>" % f for f in row["needs"]}))
    brief_shaped["flags_probed"].append({"flags": "<flags>", "effect": "<symbol: before -> after>",
                                          "verdict": "reject"})
    check("a brief-shaped outbox validates clean", handoff_mod.validate(brief_shaped, {"fn_1"})[0], [])
    check("plan_section finds §6.5", "Type and naming discipline" in plan_section(".", "### 6.5 Type and naming discipline"), True)
    check("§6.5 now carries the goto rule", "`goto` is forbidden" in plan_section(".", "### 6.5 Type and naming discipline"), True)
    check("the plan has the acknowledgement section",
          "Acknowledgement" in plan_section(".", "### 5.6 Acknowledgement"), True)
    check("the subagent contract is in §5.5",
          "fan out subagents" in plan_section(".", "### 5.5 A worker may fan out subagents"), True)
    check("plan_section finds §8", "Invariants" in plan_section(".", "## 8. Invariants"), True)
    check("plan_section is empty for nonsense", plan_section(".", "### 99 nope"), "")

    # the slug is the claim's branch minus worker/, because that is what land.py's gate keys the outbox by
    import tempfile
    from units import handoff as handoff_mod
    with tempfile.TemporaryDirectory() as tmp:
        main = os.path.join(tmp, "mhtri-dtk")
        claims.save_registry(main, {
            "Pl/pl_act": {"branch": claims.branch_for("Pl/pl_act"), "worktree": os.path.join(tmp, "ws"),
                          "base": "0" * 40},
            "Pl/pl_skill": {"branch": claims.branch_for("Pl/pl_skill") + "-dd6e"},
        })
        check("an unclaimed unit has no claim", claim_for(main, "RSO/runtime"), {})
        check("no branch means no slug", claim_slug({}), None)
        check("the slug is the branch minus worker/", claim_slug(claim_for(main, "Pl/pl_act")),
              claims.slug("Pl/pl_act"))
        check("claim_slug is claims.py's one rule", claim_slug(claim_for(main, "Pl/pl_skill")),
              claims.claim_slug(main, "Pl/pl_skill"))
        h = handoff_paths(main, "Pl/pl_act")
        check("a claimed unit is marked claimed", h["claimed"], True)
        check("the outbox is <slug>.json", os.path.basename(h["outbox"]), claims.slug("Pl/pl_act") + ".json")
        check("the outbox is the one handoff.py names", os.path.basename(h["outbox"]),
              os.path.basename(handoff_mod.outbox_path(main, "Pl/pl_act")))
        check("the outbox is claims.py's own outbox path", h["outbox"], claims.outbox_path(main, "Pl/pl_act"))
        check("the outbox lives in MAIN/.pi/outbox", os.path.dirname(h["outbox"]),
              os.path.join(main, ".pi", "outbox"))
        check("the notes path uses the same slug", os.path.basename(h["notes"]), claims.slug("Pl/pl_act") + ".md")
        check("the notes path is claims.py's own", h["notes"], claims.notes_path(main, "Pl/pl_act"))
        check("a branch suffix survives into the slug", handoff_paths(main, "Pl/pl_skill")["slug"],
              claims.slug("Pl/pl_skill") + "-dd6e")
        check("the ack stays claims.py's own path", h["ack"], claims.ack_path(main, "Pl/pl_act"))
        check("the rescue ref stays claims.py's slug", h["rescue"], "refs/rescue/%s" % claims.slug("Pl/pl_act"))
        u = handoff_paths(main, "RSO/runtime")
        check("an unclaimed unit has no slug", u["slug"], None)
        check("an unclaimed unit has no outbox path", u["outbox"], None)
        check("an unclaimed unit is marked", u["claimed"], False)
        check("the unclaimed notice says so plainly",
              "no active claim" in unclaimed_notice(main, "RSO/runtime"), True)

    # the brief must name the shared headers the unit should reuse instead of re-creating (typeregistry)
    with tempfile.TemporaryDirectory() as tmp:
        os.makedirs(os.path.join(tmp, "include", "nw4r"))
        os.makedirs(os.path.join(tmp, "src", "auto"))
        os.makedirs(os.path.join(tmp, "config", "RMHE08"))
        open(os.path.join(tmp, "configure.py"), "w").write(
            'config.libs = [{"lib": "auto", "objects": [Object(NonMatching, "auto/copies.c")]}]')
        open(os.path.join(tmp, "include", "ef.h"), "w").write(
            "typedef struct Vec { f32 x; f32 y; f32 z; } Vec;\n"
            "#define EF_ASSERT_PTR(p) do { } while (0)\n")
        open(os.path.join(tmp, "include", "nw4r", "math.h"), "w").write(
            "namespace nw4r { namespace math { struct VEC3 { f32 x; f32 y; f32 z; }; } }\n")
        # the unit copies `Vec` (debt) and owns a symbol whose mangled name encodes `VEC3`
        open(os.path.join(tmp, "src", "auto", "copies.c"), "w").write(
            "typedef struct Vec { f32 x; f32 y; f32 z; } Vec;\n"
            "void f(Vec* v) { EF_ASSERT_PTR(v); }\n")
        open(os.path.join(tmp, "src", "auto", "plain.c"), "w").write("void g(void) { }\n")
        open(os.path.join(tmp, "config", "RMHE08", "splits.txt"), "w").write(
            "auto/copies.c:\n\t.text start:0x80000000 end:0x80000004\n")
        open(os.path.join(tmp, "config", "RMHE08", "symbols.txt"), "w").write(
            "fn_1 = .text:0x80000000; // type:func size:0x4\n"
            "make__FPQ34nw4r4math4VEC3 = .text:0x80000002; // type:func size:0x2\n")
        typeregistry.clear_cache()
        b = build(tmp, tmp, "auto/copies", None)
        check("build carries the shared-header advice",
              [h["header"] for h in b["shared_headers"]],
              ["include/ef.h", "include/nw4r/math.h"])
        check("the copying header is marked duplicated",
              b["shared_headers"][0]["duplicated"], ["Vec"])
        check("a mangled owned symbol names the second header's type",
              b["shared_headers"][1]["used"], ["VEC3"])
        text = render(tmp, b, None)
        check("the brief prints the shared-header block", "Shared headers this unit should reuse" in text, True)
        check("the brief names the header the unit copies", "`include/ef.h`" in text, True)
        check("the brief names the header the owned symbol needs", "`include/nw4r/math.h`" in text, True)
        check("the brief flags the duplication", "you define these too" in text, True)
        check("the brief tells the worker include/ is read-only",
              "`include/**` is read-only for you" in text, True)
        # the language verdict rides part 1: the extension picks the front-end, so a worker has to be
        # told. A fixture with no target object must degrade to 'not on record', never guess or raise.
        check("an unreadable target leaves the language unrecorded", b["language"]["lang"], None)
        check("the brief has a language row", "| language | not on record" in text, True)
        check("the brief still states the language rule", "language is not on record" in text, True)
        check("the language paragraph cites the language rule's home",
              "The language comes from the symbol" in text, True)
        plain = build(tmp, tmp, "auto/plain", None)
        check("a unit with no match has an empty shared-header table", plain["shared_headers"], [])
        check("a unit with no match still carries the rule",
              "No shared header declares anything" in render(tmp, plain, None), True)
        # a bodyless stub is steered by its own file-header comment (typeregistry's 'mentioned')
        open(os.path.join(tmp, "src", "auto", "hinted.c"), "w").write(
            "/* the nw4r::math VEC3 shape, no body yet */\n")
        hb = build(tmp, tmp, "auto/hinted", None)
        check("a stub's brief carries the header its comment names",
              [h["header"] for h in hb["shared_headers"]], ["include/nw4r/math.h"])
        check("the brief labels a comment-only hint",
              "your file header names these" in render(tmp, hb, None), True)

    # the real end-to-end case: `auto/800CCFB0` is `ef_line.cpp` (its target object's `__FILE__` string),
    # registered `.c`. The brief has to name it C++ and hand over the two consequences that cost score.
    real_target = os.path.join("build", "RMHE08", "obj", "auto", "800CCFB0_fn_800CCFB0.o")
    if os.path.exists(real_target):
        rb = build(".", ".", "auto/800CCFB0_fn_800CCFB0", None)
        check("the real 800CCFB0 brief reads C++/high", (rb["language"]["lang"], rb["language"]["confidence"]),
              ("c++", "high"))
        check("it carries the __FILE__ evidence", "ef_line.cpp" in rb["language"]["sources"], True)
        rtext = render(".", rb, None)
        check("the brief says this unit is C++", "**This unit is C++**" in rtext, True)
        check("the brief asks for extern \"C\"", 'extern "C"' in rtext, True)
    if fails:
        print("FAIL (%d)" % len(fails))
        for f in fails:
            print("  " + f)
        return 1
    print("ok - %d checks" % checks)
    return 0


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    ap.add_argument("unit", nargs="?", help="unit path from the repository root, e.g. Pl/pl_act")
    ap.add_argument("--task", default=None, help="override part 5 with your own task text")
    ap.add_argument("--out", default=None)
    ap.add_argument("--stdout", action="store_true")
    ap.add_argument("--json", action="store_true")
    ap.add_argument("--selftest", action="store_true")
    ap.add_argument("--pool", action="store_true",
                    help="write a brief for every registered unit with no bodies yet into tools/units/briefs/pool/")
    ap.add_argument("--force", action="store_true", help="with --pool, rewrite briefs that already exist")
    ap.add_argument("--no-prune", action="store_true",
                    help="with --pool, keep briefs whose unit has gained a body")
    args = ap.parse_args()

    if args.selftest:
        return selftest()

    main = rc.main_root(rc.worktree_root())
    if args.pool:
        out = pool(main, force=args.force, prune=not args.no_prune)
        if args.json:
            print(json.dumps(out, indent=2))
            return 0
        print("pool %s" % out["dir"])
        print("  %d registered unit(s) with no bodies" % len(out["units"]))
        print("  wrote    %d" % len(out["wrote"]))
        for unit in out["wrote"]:
            print("      + %s" % unit)
        print("  skipped  %d  (already written)" % len(out["skipped"]))
        print("  pruned   %d  (unit has a body now, or is no longer registered)" % len(out["pruned"]))
        for row in out["pruned"]:
            print("      - %s  (%s)" % (row["unit"] or "?", row["slug"]))
        return 0
    if not args.unit:
        ap.print_help()
        return 0

    wt = rc.worktree_root()
    b = build(main, wt, args.unit, args.task)
    text = render(main, b, args.task)
    if args.json:
        print(json.dumps({k: v for k, v in b.items() if k != "header"}, indent=2))
        return 0
    if args.stdout:
        print(text)
        return 0
    if not b["slug"]:
        # the brief still has to have a home, but the file name must not pass itself off as the handoff slug:
        # it falls back to claims.py's own slug and the brief says the unit is unclaimed
        print("WARNING: %s has no active claim in %s - the brief says so and offers no outbox path; the file "
              "name falls back to the registry slug" % (args.unit, claims.registry_path(main)), file=sys.stderr)
    out = args.out or os.path.join(main, "tools", "units", "briefs",
                                   (b["slug"] or claims.slug(args.unit)) + ".md")
    os.makedirs(os.path.dirname(out), exist_ok=True)
    open(out, "w", encoding="utf-8", newline="\n").write(text)
    print("wrote %s (%d lines, %d symbols, %d below the bar)"
          % (out, text.count("\n"), len(b["symbols"]), b["below_bar"]))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
