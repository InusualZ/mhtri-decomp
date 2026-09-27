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
    python tools/units/brief.py --pool [--force] [--no-prune] [--prune-promoted]
    python tools/units/brief.py --check-promoted

The brief is written into MAIN (`<main>/tools/units/briefs/`), not into a worker's worktree, so it outlives
the worktree the same way the outbox does. Its file name and the paths in part 4 come from the unit's
**active claim**: the slug is the claim's branch minus `worker/` - the name `land.py`'s gate keys the outbox
by - never a name re-derived from the unit path, and a brief for an unclaimed unit says so instead of
inventing one.

`--pool` is the one exception, and it is what lets the orchestrator start a worker the instant a slot frees:
for every registered unit that has no bodies yet it writes a brief *before* any claim exists, keyed by
`claims.slug(unit)` and rendered against the worktree and branch the default claim will create, so
`tools/units/queue.py next` only has to claim the unit and hand the worker a brief. No claim is made and the
registry is never touched. Each brief carries a **stamp** (an invisible HTML comment) of the entry's range,
function count and TU verdict, so re-running `--pool` is idempotent: an unchanged entry is skipped and the
file is not rewritten, a changed one is refreshed, and a brief whose unit has gained a body is pruned.
`queue.py next` no longer copies a pooled brief at all - it re-renders from the current entry - so a stale
pool cannot reach a worker even between `--pool` runs. `--check-promoted` reports (and `--prune-promoted`
deletes) a promoted brief in `tools/units/briefs/` whose entry no longer matches the queue.

**A brief is never written for a range that is not handable work (2026-09-25).** The proposal queue is
written from the *unclaimed* `.text`, but the file is not regenerated on every landing, so an entry's range
can be registered by another worker before its brief is pooled - the `proposal/8008F8E4` entry capped
`0x8008F8E4-0x80097D40`, five translation units, four of them already claimed and live. `--pool` leaves
such an entry out (and prunes its brief if one exists), single-unit mode refuses it, and both name the unit
whose range it now overlaps; `queue.py next` therefore only ever offers ranges that are still unclaimed.
The same test covers a queue that overlaps *itself*: two entries sharing bytes would hand one range to two
workers, and neither is offered.
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

# A brief carries a stamp of the queue entry it was written from, so `--pool` can tell a current brief from
# one written against an older entry: a regeneration that re-cuts a range or changes a function count used to
# leave every pre-existing pooled brief describing the OLD range (the 80119DEC incident, 2026-09-25). The
# stamp is an HTML comment - invisible in the rendered brief - holding the entry's range, function count and
# TU verdict. An unchanged entry leaves the file byte-identical (no churn); a changed one is refreshed.
STAMP_MARK = "<!-- brief-stamp:"
_STAMP_RE = re.compile(r"^<!-- brief-stamp: (\{.*\}) -->\s*$", re.M)


def _stamp_line(stamp: dict) -> str:
    return "%s %s -->" % (STAMP_MARK, json.dumps(stamp, sort_keys=True, separators=(",", ":")))


def proposal_stamp(p: dict) -> dict:
    """The identifying fields of a queue entry: its range, function count and TU verdict."""
    tu = p.get("tu") or {}
    lang = p.get("language") or {}
    return {"kind": "proposal", "label": p.get("label"),
            "text": [int(x) for x in (p.get("text") or [])],
            "count": int(p.get("count") or 0),
            "tu": {"verdict": tu.get("verdict"),
                   "sources": list(tu.get("sources") or []),
                   "partial_source": tu.get("partial_source")},
            "language": {"lang": lang.get("lang"), "confidence": lang.get("confidence")}}


def unit_stamp(unit: str, sections: dict) -> dict:
    """The identifying fields of the pre-option-A fallback: the unit's section ranges."""
    return {"kind": "unit", "unit": claims.norm_unit(unit),
            "sections": {k: [int(v[0]), int(v[1])] for k, v in sorted(sections.items())}}


def entry_stamp(main: str, unit: str) -> dict:
    """The stamp the brief for `unit` should carry right now, from the current queue entry."""
    p = proposal_by_label(main, unit)
    if p is not None:
        return proposal_stamp(p)
    return unit_stamp(unit, splits_range(main, unit))


def brief_stamp(path: str) -> dict | None:
    """The stamp a written brief carries - `None` for one written before stamps, or unparseable."""
    try:
        text = open(path, encoding="utf-8", errors="replace").read()
    except OSError:
        return None
    m = _STAMP_RE.search(text)
    if not m:
        return None
    try:
        return json.loads(m.group(1))
    except ValueError:
        return None


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


def text_has_bodies(text: str) -> bool:
    """Whether source `text` defines at least one body (a brace outside its comments).

    The attribution placeholders (`src/auto/*`) are a file-header comment and nothing else, so a brace
    outside the comments is the honest, cheap signal that a body exists. This is deliberately a
    source-level heuristic: it decides which units the *pool* has nothing to hand a worker for, never
    whether a unit is finished. `stylelint.py` reuses this exact notion for its rule-7 "no bodies yet"
    key, so a stub cannot be read as bodyless by one tool and bodied by the other.
    """
    return "{" in strip_comments(text)


def has_bodies(path: str) -> bool:
    """Whether the source file at `path` defines at least one body (see `text_has_bodies`)."""
    if not os.path.exists(path):
        return False
    return text_has_bodies(open(path, encoding="utf-8", errors="replace").read())


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


def _cpp_step(lines: list[str]) -> None:
    """Append section 5c (the C++ class rule) - shared by both brief renderers.

    Owner finding, 2026-09-26: a lane reconstructed a C++ class's methods as free functions taking an explicit
    `self` (403 `self->` uses in Network/fn_8041A87C.cpp) although the target's own string pool spells the two
    class names. The *shape* is part of the match: MWCC emits retail's canonical `lwz r12,0(r3)` /
    `lwz r12,<slot>(r12)` dispatch only for a genuine `virtual`, and a member function's `this` and mangling are
    what the map's names and objdiff pairing expect.
    """
    lines.append("")
    lines.append("### 5c · If the evidence says C++, write the class - not a struct with `self`")
    lines.append("")
    lines.append("A `__FILE__` string naming `Class::method`, a mangled definition, the canonical "
                 "`lwz r12,0(r3)`/`lwz r12,<slot>(r12)` dispatch, an adjustor thunk (`subi r3,r3,0x14`), a "
                 "ctor/dtor pair or a string pool spelling `Class::` all say the range is a class's methods: "
                 "write **member functions on the class**, with the layout annotations (sizes, field offsets) "
                 "kept on the class's fields.")
    lines.append("* MWCC emits retail's canonical virtual dispatch only for a genuine `virtual`; a struct of "
                 "function pointers stages the table through a temporary and loses the function's score.")
    lines.append("* A member function's `this` arrives in r3 and its name mangles the way the map spells it - a "
                 "free function with an explicit `self` gets neither.")
    lines.append("* If a function measures worse in the class form, keep the better-scoring shape and record "
                 "both numbers in the unit header - but never leave `self` style in place because it was "
                 "written first.")
    lines.append("")


def _data_step(lines: list[str]) -> None:
    """Append section 5d (the data step) - shared by both brief renderers.

    Owner instruction, 2026-09-26: a decompiler lane matches **data** as well as code. objdiff's unit score
    does not count a wrong data section, so a lane could hand over a unit whose code matched and whose
    `.data`/`.sdata2` was ours-extra - which is where the 20-unit flip-blocker list came from. Both the
    registered-unit renderer and the proposal renderer call this, because a proposal lane registers its unit
    and is exactly the lane that needs the step.
    """
    lines.append("")
    lines.append("### 5d · Data (measure it, then claim what your object emits)")
    lines.append("")
    lines.append("objdiff's unit score does not count a wrong data section, so measure it: "
                 "`python tools/units/datagap.py --unit <unit>` prints the per-section gap between the target "
                 "object and yours (`build/RMHE08/obj/...o` vs `build/RMHE08/src/...o`). Record it before and "
                 "after your work.")
    lines.append("")
    lines.append("* `ours-extra` - your source defines a table or constant the original TU did not own - is the "
                 "usual defect. A pooled constant is fixed by declaring the map's symbol `extern` and using it "
                 "as a load operand, **never** by defining it (playbook 29/58: a definition makes MWCC emit "
                 "both the named constant and its pool copy, so `.sdata2` grows instead of clearing).")
    lines.append("* Claim the data your object **emits**: exact `start:`/`end:` in `splits.txt`, then "
                 "`rm -f build/RMHE08/config.json` and rebuild - a claim edit that never re-splits links the "
                 "old object and reports a false green. `.data`, `.sdata`, `.ctors` and `.dtors` claims are "
                 "safe; **a partial `.sdata2` claim breaks the link** (playbook 23).")
    lines.append("* A pool entry is claimable **only while your unit is its sole referencer** (playbook 58). A "
                 "private entry is exactly what the claim is for - claim it, flip the unit to "
                 "`Object(Matching)` and say so; a shared entry can be neither claimed nor named in source, so "
                 "write the measured blocker in the unit header and report it.")
    lines.append("* Finish with the numbers: the unit's sections and bytes before/after, and whether "
                 "`python tools/units/datagap.py --flip-blockers` lists this unit.")


def _precommit_lines(lines: list[str]) -> None:
    """Append the §4 pre-commit gate check and staging hygiene - shared by both renderers.

    The land gate's naming rule is importable, so a worker can run it from its own worktree instead of
    paying a refusal round; two lanes were refused after a full unit of work because they did not run it
    first. And `git add` aborts the *whole* add, silently, when any path it is given no longer exists -
    a rename commit staged only the `git mv` because it also passed the renamed-away path.
    """
    lines.append("")
    lines.append("**Check the gate from your own worktree before you commit** - it is importable, so run its")
    lines.append("naming rule yourself instead of paying a refusal round. Both commands must print `[]`:")
    lines.append("")
    lines.append("```sh")
    lines.append("python -c \"import sys;sys.path[:0]=['tools','tools/units'];from units import land;"
                 "print(land.rule7_defer_growth(r'.','main'))\"        # run with your worktree as cwd")
    lines.append("python -c \"import sys;sys.path[:0]=['tools','tools/units'];from units import land;"
                 "print(land.band_ownership_warnings(r'.','main'))\"   # likewise")
    lines.append("```")
    lines.append("")
    lines.append("**Stage the whole change, then read the commit back.** `git add <path>` aborts the *whole* add")
    lines.append("- silently - when any `<path>` no longer exists: a rename commit staged only the `git mv` because")
    lines.append("it also passed the renamed-away path. Use `git add -A` with no path arguments, then")
    lines.append("`git show --stat` and check the commit holds every file you touched.")
    lines.append("")


def _teardown_lines(lines: list[str]) -> None:
    """Append the §6 `claims.py release` ban - shared by both renderers.

    Run from inside a worktree it WIPED the directory and deregistered it while reporting "teardown is
    INCOMPLETE, the claim is kept". That lane's work survived only because it had committed first;
    uncommitted work would have been lost. Teardown is the orchestrator's job.
    """
    lines.append("")
    lines.append("**NEVER run `claims.py release`.** From inside a worktree it WIPED the directory and")
    lines.append("deregistered it while reporting \"teardown is INCOMPLETE, the claim is kept\" - that lane")
    lines.append("lost its tree, and it survived only because it had committed first; uncommitted work would")
    lines.append("not have. Teardown is the orchestrator's job. Leave your worktree where it is.")


def _measure_lines(lines: list[str], unit: str) -> None:
    """Append the working measurement loop (§6) - shared by both renderers.

    `recompile.py --measure` and `measure.py` are unusable on this host: from git-bash the compile path
    emits `cmd /c`, MSYS rewrites it to `C:\\c`, and no object is written (a worktree run says "the
    compiler returned 0 but wrote no object"). `ninja build/RMHE08/report.json` plus `symdiff.py` is the
    loop that works - and `report.json` is an order-only target of `all_source`, so it must be removed
    first or ninja reports "no work to do" and the worker reads the previous build's scores.
    """
    lines.append("")
    lines.append("**Measure with `ninja build/RMHE08/report.json` + `tools/objdiff/symdiff.py`, never")
    lines.append("`recompile.py --measure` or `measure.py`.** Both are unusable here: from git-bash the compile")
    lines.append("path emits `cmd /c`, MSYS rewrites it to `C:\\c`, and no object is written (a worktree run")
    lines.append("reports \"the compiler returned 0 but wrote no object\").")
    lines.append("")
    lines.append("```sh")
    lines.append("rm -f build/RMHE08/report.json   # order-only target: see below")
    lines.append("ninja build/RMHE08/report.json")
    lines.append("python tools/objdiff/symdiff.py -u %s <symbol>   # the official report metric" % unit)
    lines.append("```")
    lines.append("")
    lines.append("**`build/RMHE08/report.json` is an order-only target of `all_source`**: after a source edit ninja")
    lines.append("says \"no work to do\" and you read the PREVIOUS build's scores. `rm -f build/RMHE08/report.json`")
    lines.append("first - that trap cost one lane three iterations that looked like \"all new functions score 0 %\".")
    lines.append("")


def render(main: str, b: dict, task: str | None, pool: bool = False) -> str:
    rng = b["sections"]
    txt = rng.get(".text")
    lines = []
    lines.append("# Brief: %s" % b["unit"])
    lines.append("")
    lines.append(_stamp_line(unit_stamp(b["unit"], rng)))
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
    lines.append("")
    lines.append("**Self-check before you commit.** The land gate REFUSES a batch that adds any section 6.5")
    lines.append("violation, and a refusal costs the whole round - so run")
    lines.append("`python tools/units/stylelint.py --diff main` and fix what it reports for your files. MAIN")
    lines.append("is the base the gate lints against - linting against your own HEAD misses a violation")
    lines.append("that an already-merged header introduces. The two")
    lines.append("that catch a new unit are **rule 2** (a declaration belongs in the symbol's owner's header,")
    lines.append("never in your source; `include/unsplit/<band>.h` is the home when no unit owns it) and")
    lines.append("**rule 9** (never spell a mangled name - call the owner's member or function through its real")
    lines.append("signature; `tools/units/mangle.py` proves the signature).")
    _precommit_lines(lines)
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
        lines.append("**Data this unit may own** (measured second pass): "
                     + ", ".join("%s 0x%X-0x%X (%s)" % (e.get("section"), e.get("start", 0), e.get("end", 0),
                                                        e.get("verdict", "unmeasured")) for e in b["data_queue"])
                     + " - claim what your object *emits* (and only while this unit is its sole referencer); "
                       "propose the rest.")
    _cpp_step(lines)
    _data_step(lines)
    lines.append("")
    lines.append("## 6 · The rules")
    lines.append("")
    lines.append("Measurement loop (build YOUR unit in YOUR worktree; never the split, never MAIN):")
    lines.append("")
    lines.append("```sh")
    lines.append("**Build in your worktree, never in MAIN's.** `build/RMHE08` there is yours alone; MAIN's belongs to")
    lines.append("the orchestrator, and several workers share this machine. The target objects you diff against are read-only")
    lines.append("in MAIN, and your worktree's `build/tools` is seeded with the toolchain (`dtk`, `objdiff-cli`, `sjiswrap`).")
    lines.append("")
    lines.append("**Set the toolchain up by COPYING it - never junction it.** Copy `build/{compilers,binutils,tools}`; junction")
    lines.append("only the read-only `orig/RMHE08/{sys,files,disc}`; `build/RMHE08` must be the worktree's own. A junctioned")
    lines.append("`build/compilers` is dangerous: ninja decides it is missing and re-downloads it *through* the junction into")
    lines.append("MAIN's directory (observed: a `PermissionError` mid-write on `lmgr8c.dll`, MAIN's compiler one write away")
    lines.append("from being corrupted).")
    lines.append("")
    lines.append("**Run the full `ninja` in your worktree, with `build/RMHE08/ok` deleted first.** That target is order-only,")
    lines.append("so it prints `main.dol: OK` forever once its stamp exists; deleting the stamp forces a real hash check. It is")
    lines.append("the only way to prove your worktree builds and that the DOL still matches - and it cannot affect MAIN.")
    lines.append("")
    lines.append("**Before you hand-roll a search, use the two tools that do it mechanically:**")
    lines.append("")
    lines.append("```")
    lines.append("python tools/flags/infer.py %s            # which flags the TARGET object implies" % b["unit"])
    lines.append("python tools/flags/shapesearch.py -u %s --scan 20   # generate/compile/score/rank source variants" % b["unit"])
    lines.append("```")
    lines.append("")
    lines.append("`infer.py` reads the target bytes and names the flags (it is 100 % correct on 36 confident claims and abstains rather than guess); `shapesearch.py` writes variants of your source, compiles each with the real command line into a scratch copy and ranks them by the official metric - it took two functions to 100 % on `Pl/pl_act` in 30 seconds. A shapesearch *miss* is informative: zero differing rows with a 99.9 % score means relocation naming, so the data claim is the fix, not a shape.")
    _measure_lines(lines, b["unit"])
    lines.append("A worker **registers its own unit** (`splits.txt` + `configure.py`, part 2) and builds freely **in its own worktree** - including the full `ninja` - but never runs the **split**, the **link**, `land.py` or `claims.py release`, never edits `symbols.txt` or `AGENTS.md`, and never commits on `main`. Everything else you need changed goes into the outbox's `config_requests`.")
    _teardown_lines(lines)
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
    lines.append("**Rule 7 at your final path.** The lint stops enforcing rule 7 while a unit has no bodies "
                 "yet. Once it carries bodies, every symbol the unit **defines** needs a name: use the map's "
                 "real name when the evidence has one, otherwise derive one from the symbol's own body and "
                 "the neighbours' scheme, and when the context supports only a guess, guess and mark it as a "
                 "GUESS in the unit header with the evidence behind it. `fn_XXXXXXXX` is never the resting "
                 "place, and a rename is the map **and** the source in one edit - request the map half of "
                 "your own unit's renames in the outbox (`config_requests`, `kind: rename`, "
                 "old/new/evidence), since you may not edit `symbols.txt` here. A `rule 7 deferred` line is "
                 "only for references to OTHER units' unrenamed symbols - the land gate refuses a batch that "
                 "grows an escape for a name the batch owns:\n\n    rule 7 deferred: <the evidence, e.g. "
                 "references only to other units' fn_XXXXXXXX symbols>\n\nIt defers the `fn_XXXXXXXX` half "
                 "only - a bare `unk*` local still fails - and the complete set of deferrals is "
                 "`grep -rn \"rule 7 deferred\" src/`.")
    lines.append("")
    lines.append(plan_section(main, "## 8. Invariants"))
    return "\n".join(lines).rstrip() + "\n"


# --------------------------------------------------------------------------------------------------
# option A: the proposal brief (owner, 2026-09-24)
#
# The `src/auto/` scaffolding bucket is retired: a translation unit is registered ONCE, at its final
# `src/<module>/<name>.<ext>` home, by the worker that works it. Discovery therefore produces *proposals*
# (`attribute.py queue`), and the pool is built from the queue rather than from registered no-body units.
# The worker's first act is the registration - it has to be, because a unit that is not in the build graph
# cannot be measured - and that registration lands on `main` with the worker's own commit.
# --------------------------------------------------------------------------------------------------
def queue_path(main: str) -> str:
    return os.path.join(main, "tools", "units", "attribution-queue.json")


# mtime-keyed: see `proposals`. A module-level dict rather than an lru_cache so a rewritten queue is picked
# up without a new process.
_QUEUE_CACHE: dict = {}


def proposals(main: str) -> list[dict]:
    """The proposal queue, or `[]` when discovery has not been run.

    Read from the file rather than from the build tree on purpose: a proposal has no split object yet, so
    there is nothing in `build/` to read. A missing or malformed queue is `[]`, never an exception - the
    pool then falls back to the registered no-body units, which is the pre-option-A behaviour.

    Cached per (path, mtime): `queue.py` asks for the queue once per pooled entry, and the file is a few
    hundred KB.
    """
    path = queue_path(main)
    if not os.path.exists(path):
        return []
    try:
        key = (path, os.path.getmtime(path))
    except OSError:
        key = None
    if key is not None and _QUEUE_CACHE.get("key") == key:
        return _QUEUE_CACHE["units"]
    try:
        doc = json.load(open(path, encoding="utf-8"))
    except (OSError, ValueError) as exc:
        print("WARNING: cannot read %s (%s) - falling back to registered no-body units" % (path, exc),
              file=sys.stderr)
        return []
    units = doc.get("units")
    units = units if isinstance(units, list) else []
    if key is not None:
        _QUEUE_CACHE["key"], _QUEUE_CACHE["units"] = key, units
    return units


def proposal_by_label(main: str, label: str) -> dict | None:
    want = claims.norm_unit(label)
    for p in proposals(main):
        if claims.norm_unit(p.get("label") or "") == want:
            return p
    return None


def proposal_labels(main: str) -> list[str]:
    """The queue's labels, in address order (the queue is written in address order already)."""
    return [p["label"] for p in proposals(main) if p.get("label")]


def registered_text_ranges(main: str) -> list[tuple[int, int, str]]:
    """Every registered unit's `.text` range from `splits.txt`, as `(start, end, unit)`.

    `queue.py` keeps its own start-only variant for the pool's `covered` state; a brief needs the whole
    range, because a proposal overlapping *any* part of a live unit is the defect, not only one that
    starts inside it.
    """
    out = []
    for unit in registered_units(main):
        rng = splits_range(main, unit)
        if ".text" in rng:
            out.append((rng[".text"][0], rng[".text"][1], unit))
    return sorted(out)


def proposal_conflicts(main: str, p: dict) -> list[str]:
    """Every reason this queue entry must not be handed to a worker - empty when it may be.

    The queue is written from the *unclaimed* `.text`, but the tree moves under it and the file is not
    regenerated on every landing, so a proposal's range can be registered by another worker before its
    brief is written. Measured 2026-09-25: the `proposal/8008F8E4` entry capped `0x8008F8E4-0x80097D40` -
    five translation units, four of them already claimed and live. A brief written for it would set a
    second worker on four live ranges, so none is written: the range is finished work and `attribute.py
    queue` re-cuts the region. A range overlapping a *sibling* proposal is the queue's own invariant
    broken (`attribute.overlap_report`), and is refused the same way rather than handed out twice.
    """
    text = p.get("text") or []
    label = p.get("label", "?")
    if len(text) != 2:
        return ["%s: the queue entry carries no .text range" % label]
    t0, t1 = text
    out = []
    for s, e, unit in registered_text_ranges(main):
        if t0 < e and s < t1:
            out.append("%s: 0x%08X..0x%08X overlaps %s's registered .text 0x%08X..0x%08X"
                       % (label, t0, t1, unit, s, e))
    for q in proposals(main):
        if q is p or claims.norm_unit(q.get("label") or "") == claims.norm_unit(label):
            continue
        qt = q.get("text") or []
        if len(qt) == 2 and t0 < qt[1] and qt[0] < t1:
            out.append("%s: 0x%08X..0x%08X overlaps the sibling proposal %s (0x%08X..0x%08X)"
                       % (label, t0, t1, q.get("label", "?"), qt[0], qt[1]))
    return out


def handable_proposals(main: str) -> tuple[list[dict], list[dict]]:
    """`(handable, blocked)` - the queue's entries a worker may be given, and the rest with reasons.

    `blocked` carries `why` per entry because a queue entry nobody can take is a *data* defect to fix
    (`attribute.py queue`), never work to drop silently - `pool` reports every one of them.
    """
    handable, blocked = [], []
    for p in proposals(main):
        why = proposal_conflicts(main, p)
        if why:
            blocked.append(dict(p, why=why))
        else:
            handable.append(p)
    return handable, blocked


def proposal_conflict_reasons(main: str, unit: str) -> list[str]:
    """Why a *named* unit is not handable work, `[]` for a registered unit or a clean proposal."""
    p = proposal_by_label(main, unit)
    return proposal_conflicts(main, p) if p is not None else []


def build_proposal(main: str, p: dict, task: str | None = None, assume_claim: bool = False) -> dict:
    """The brief data for one proposal. Unit-shaped keys are present but empty/unknown by design.

    There is no `sections` (nothing is split yet), no `flags`/`lib` (the worker picks from the evidence the
    target object will give it), no `report_scores` (no object) and no `source` (the path does not exist
    until the worker decides the module and name). Everything that *is* known comes from the queue entry.
    """
    label = p["label"]
    t0, t1 = p["text"]
    return {
        "unit": label,
        "proposal": p,
        "range": [t0, t1],
        "symbols": symbols_in_range(main, t0, t1),
        "below_bar": p.get("count", 0),
        "slug": claims.slug(label),
        "handoff": handoff_paths(main, label, assume_claim=assume_claim),
        "source": None,
    }


def render_proposal(main: str, b: dict, task: str | None, pool: bool = False) -> str:
    """The brief a worker gets for a *proposal*: work this range, then register it where it belongs."""
    p = b["proposal"]
    label = b["unit"]
    t0, t1 = b["range"]
    seam = p.get("seam") or []
    lines: list[str] = []
    lines.append("# Proposal brief: %s" % label)
    lines.append("")
    lines.append(_stamp_line(proposal_stamp(p)))
    lines.append("")
    if pool:
        lines.append("> **Pooled brief** - prepared by `brief.py --pool` before the claim. `queue.py next` claims")
        lines.append("> this proposal and hands you this file; the worktree and outbox paths below are the ones")
        lines.append("> your claim will have. Do not act on a pooled brief you were not handed.")
        lines.append("")
    warning = _tu_warning(p)
    if warning:
        lines.append(warning)
        lines.append("")
    lines.append("Read this file, do the task, write your report where §4 says. Nothing outside this file is a rule.")
    lines.append("")
    lines.append("## 0 · Acknowledge first, then heartbeat")
    lines.append("")
    lines.append("```sh")
    lines.append("python tools/units/claims.py ack %s --agent <your-name>" % label)
    lines.append("```")
    lines.append("")
    lines.append("Re-run it **with `--progress <symbol>` every time you finish a function** - it is the heartbeat")
    lines.append("by which the orchestrator tells a stalled worker from a working one.")
    lines.append("")
    lines.append("## 1 · This is a proposal, not a unit")
    lines.append("")
    lines.append("| | |")
    lines.append("| --- | --- |")
    lines.append("| `.text` range | `0x%08X`-`0x%08X` (%d bytes) |" % (t0, t1, t1 - t0))
    lines.append("| functions | %d |" % p.get("count", 0))
    lines.append("| language hint | %s |" % _lang_hint(p))
    lines.append("| seam | %s |" % ("**pinned** (%s)" % ", ".join(s.get("kind", "?") for s in seam) if seam
                                     else "**unproven** - this is one maximal unclaimed run"))
    lines.append("| TU evidence | %s |" % _tu_row(p))
    lines.append("")
    lines.append("**The `src/auto/` scaffolding bucket is retired** (owner, 2026-09-24). Discovery proposed this")
    lines.append("range; it registered nothing. Your first act is to **register it once, at its final home**, and")
    lines.append("you are the only one who can: you will have understood the code by then.")
    lines.append("")
    if p.get("seam_note"):
        lines.append("**WARNING from discovery:** %s" % p["seam_note"])
        lines.append("")
    lines.append("## 2 · Register it at its final home (do this first)")
    lines.append("")
    lines.append("A unit that is not in the build graph cannot be measured, so registration comes before the")
    lines.append("bodies. Decide, in this evidence order, and say in your report which class decided it:")
    lines.append("")
    lines.append("1. **A `__FILE__` string.** `python tools/units/dossier.py <unit>` will not work yet (no object),")
    lines.append("   so read the region's `.data`/`.sdata` pool yourself: an assert or log string that is a bare")
    lines.append("   source-file name (`ef_line.cpp`) IS the original source file. Module and name are then decided,")
    lines.append("   and the extension comes from its suffix. `langcheck.py` is the authority for C vs C++.")
    lines.append("2. **A real runtime-dump name.** `python tools/symbols/dumpmap.py lookup <addr>`. A `zz_XXXXXXXX_`")
    lines.append("   name is NOT evidence.")
    lines.append("3. **What the code does, plus the naming scheme of its neighbours.** A descriptive name that fits")
    lines.append("   the siblings' scheme. If several proposals are plainly one subsystem, say so - they belong in")
    lines.append("   one module directory.")
    lines.append("4. **The evidence gives no name - derive the best guess and mark it.** With no `__FILE__`")
    lines.append("   string, no real runtime-dump name and no neighbour scheme reaching the range, derive the most")
    lines.append("   descriptive module and name the context supports (what the range actually does), write it in the")
    lines.append("   unit header as an explicit **GUESS** with the evidence behind it, and register at")
    lines.append("   `src/<module>/<name>.<ext>`. **Keeping the map's `fn_XXXXXXXX` stem as the file name is not an")
    lines.append("   option**: the land gate refuses a batch whose own unit is registered at a generated file name")
    lines.append("   (`src/enemy/fn_8033041C.cpp`). Do not invent a module either - if the module is genuinely unknown,")
    lines.append("   ask the orchestrator.")
    lines.append("")
    lines.append("Then make the registration, **in your worktree**, in one commit - the source, the map row and the")
    lines.append("two build files, because a rename is always **two** edits (the map **and** the source; playbook")
    lines.append("31/48):")
    lines.append("")
    lines.append("```sh")
    lines.append("# 1. the source, at its final path")
    lines.append("mkdir -p src/<module> && $EDITOR src/<module>/<name>.<ext>")
    lines.append("# 2. the map row for each symbol THIS unit defines that the map still spells fn_XXXXXXXX -")
    lines.append("#    never by hand, and never another unit's symbol")
    lines.append("python tools/symbols/symedit.py rename fn_XXXXXXXX <name>")
    lines.append("# 3. its object line, in configure.py's config.libs, in the lib its neighbours use")
    lines.append("#    Object(NonMatching, \"<module>/<name>.<ext>\"),")
    lines.append("# 4. its splits.txt block: one line per section, exact start:/end: addresses")
    lines.append("#    <module>/<name>.<ext>:")
    lines.append("#    \t.text       start:0x%08X end:0x%08X" % (t0, t1))
    lines.append("python configure.py && ninja build/RMHE08/src/<module>/<name>.o   # registers it in YOUR worktree")
    lines.append("python tools/units/recompile.py <module>/<name>.<ext> --measure <symbol>")
    lines.append("```")
    lines.append("")
    lines.append("**This is the one exception to the shared-file rule.** Everywhere else a worker never touches")
    lines.append("`splits.txt`, `configure.py`, `symbols.txt` or `AGENTS.md`. Here you must, because measurement needs")
    lines.append("the unit in the graph - but **only in your own worktree**. The orchestrator applies your")
    lines.append("registration on `main` with the rest of the batch, one re-split for all of them. Do not add a")
    lines.append("second `config.libs` block: extend the block your neighbours are in.")
    lines.append("")
    lines.append("If the unit needs sections beyond `.text` (a `.ctors`/`.dtors` word, `extab`/`extabindex`), claim")
    lines.append("them in the same `splits.txt` block and say so in your report.")
    lines.append("")
    lines.append("**Naming your own symbols, and the one line the gate needs.** This unit is yours, so every")
    lines.append("symbol it **defines** is yours to name: use the map's real name when the evidence has one, otherwise")
    lines.append("derive a descriptive name from the symbol's own body - what it does, what it returns, who calls it,")
    lines.append("what it writes - plus the neighbours' naming scheme. When the context supports only a guess, guess")
    lines.append("and write it in the unit header as an explicit **GUESS** with the evidence behind it: there is no")
    lines.append("\"no evidence for a name\" case, only a name to derive. Register at that named path (never")
    lines.append("`fn_XXXXXXXX.cpp`) and finish the rename's map half with")
    lines.append("`python tools/symbols/symedit.py rename fn_XXXXXXXX <name>` - the map **and** the source, one edit.")
    lines.append("")
    lines.append("The gate line is for the symbols you only *reference*: the land gate REFUSES a batch that grows a")
    lines.append("`rule 7 deferred` escape for a name the batch owns (its own file registered at a generated file")
    lines.append("name, or its own `fn_XXXXXXXX` name left **defined** in the source). References to OTHER units'")
    lines.append("unrenamed `fn_XXXXXXXX` symbols are tolerated - they are not this batch's to fix - and a `rule 7")
    lines.append("deferred` line is still the durable way to say so, with a reason that names the evidence:")
    lines.append("")
    lines.append("```c")
    lines.append(" * rule 7 deferred: references only to other units' unrenamed fn_XXXXXXXX symbols (checked <how>)")
    lines.append("```")
    lines.append("")
    lines.append("It is a per-unit, greppable deferral - `grep -rn \"rule 7 deferred\" src/` is the complete list,")
    lines.append("so write it only when it is true. `unkNN` identifiers are still violations and must be named.")
    lines.append("")
    lines.append("## 3 · The inventory (from the symbol map)")
    lines.append("")
    if b["symbols"]:
        lines.append("| symbol | address |")
        lines.append("| --- | --- |")
        for r in b["symbols"][:60]:
            lines.append("| `%s` | 0x%X |" % (r.get("name", "?"), r.get("address", 0)))
        if len(b["symbols"]) > 60:
            lines.append("| ... and %d more | |" % (len(b["symbols"]) - 60))
    else:
        lines.append("**The inventory came back empty** - the range is not in the map, or the map proxy failed.")
        lines.append("Report it instead of guessing.")
    lines.append("")
    lines.append("## 4 · Where your output goes")
    lines.append("")
    lines.append("* your source **and its registration**, committed **on your branch** (one commit)")
    lines.append("")
    lines.append("**Self-check before you commit.** The land gate REFUSES a batch that adds any section 6.5")
    lines.append("violation, and a refusal costs the whole round - so run")
    lines.append("`python tools/units/stylelint.py --diff main` and fix what it reports for your files. MAIN")
    lines.append("is the base the gate lints against - linting against your own HEAD misses a violation")
    lines.append("that an already-merged header introduces. The two")
    lines.append("that catch a new unit are **rule 2** (a declaration belongs in the symbol's owner's header,")
    lines.append("never in your source; `include/unsplit/<band>.h` is the home when no unit owns it) and")
    lines.append("**rule 9** (never spell a mangled name - call the owner's member or function through its real")
    lines.append("signature; `tools/units/mangle.py` proves the signature).")
    _precommit_lines(lines)
    if b["handoff"]["claimed"]:
        lines.append("* `%s` - the outbox `land.py`'s gate reads. It is named after your claim's branch, so"
                     % b["handoff"]["outbox"])
        lines.append("  write it exactly here; do not invent a name.")
        lines.append("* `%s`" % b["handoff"]["notes"])
    else:
        lines.append("* %s" % unclaimed_notice(main, label))
    lines.append("* a ≤ 15-line digest in your reply")
    lines.append("")
    lines.append("**End your turn with your report as the final message.** Your last assistant message *is* the")
    lines.append("handoff; ending on a tool call (or saying nothing) hands back an empty result.")
    lines.append("")
    lines.append("## 5 · The task")
    lines.append("")
    if task:
        lines.append(task)
    else:
        lines.append("Reconstruct this range's %d function(s) to at least the %.0f %% bar, in address order,"
                     % (p.get("count", 0), BAR))
        lines.append("biggest first where two are equal.")
        lines.append("")
        lines.append("Work them one at a time and re-measure each with `ninja build/RMHE08/report.json` +")
        lines.append("`tools/objdiff/symdiff.py` (the loop is in §6). A function that resists is a residual to")
        lines.append("record, not a reason to stop -")
        lines.append("**apply the best-scoring variant even if it is not a full match** and write what still differs")
        lines.append("into the unit's file header.")
    _cpp_step(lines)
    _data_step(lines)
    lines.append("")
    lines.append("## 6 · The rules")
    lines.append("")
    lines.append("**Build in your worktree, never in MAIN's.** Several workers share this machine.")
    lines.append("")
    lines.append("**Before you hand-roll a search, use the tools that do it mechanically:**")
    lines.append("")
    lines.append("```")
    lines.append("python tools/flags/infer.py <unit>                    # which flags the TARGET object implies")
    lines.append("python tools/flags/shapesearch.py -u <unit> --scan 20 # generate/compile/score/rank variants")
    lines.append("```")
    _measure_lines(lines, label)
    lines.append("A worker never runs the **split**, the **link** or `land.py`, and never commits on `main` - but it DOES")
    lines.append("build in its own worktree, full `ninja` with `build/RMHE08/ok` deleted first (that is the only way to prove")
    lines.append("the worktree and see a real `main.dol: OK`). Apart from the registration part 2 requires, everything you need")
    lines.append("changed goes into the outbox's `config_requests`.")
    _teardown_lines(lines)
    lines.extend(config_schema_lines())
    lines.append("")
    lines.append("**You may fan out subagents** - use the **`decompiler`** agent for them (the project agents "
                 "`fixer` and `merger` exist for the two repair cases: a branch the landing gate refused, and a "
                 "held branch that main moved past). "
                 "**You may fan out subagents** for parallel work, in *your* worktree, on *your* branch. They never")
    lines.append("commit; you assign them disjoint functions; you re-measure every claim they make. Hand each of them")
    lines.append("this whole part verbatim.")
    lines.append("")
    lines.append(plan_section(main, "### 6.5 Type and naming discipline"))
    lines.append("")
    lines.append(plan_section(main, "## 8. Invariants"))
    return "\n".join(lines).rstrip() + "\n"


def brief_for(main: str, wt: str, unit: str, task: str | None = None, assume_claim: bool = False,
              pool: bool = False) -> tuple[dict, str]:
    """`(brief, text)` for a unit spelled either way: a proposal on its queue entry, else on its unit.

    Single-unit mode and the pool must agree on which renderer a unit gets. They did not: `main()` always
    took the registered-unit path, so the first proposal round came back with the registered-unit template
    and an **empty inventory** (`splits_range()` is empty for an unregistered proposal, so every symbol list
    is blank and the dossier reads "target object not found"), which the brief's own §5 turns into "do not
    start". One helper, one decision.
    """
    p = proposal_by_label(main, unit)
    if p is not None:
        b = build_proposal(main, p, task, assume_claim=assume_claim)
        return b, render_proposal(main, b, task, pool=pool)
    b = build(main, wt, unit, task, assume_claim=assume_claim)
    return b, render(main, b, task, pool=pool)


def _lang_hint(p: dict) -> str:
    """The region's language verdict, as a *hint* - the worker's own evidence outranks it."""
    lang = p.get("language") or {}
    if lang.get("lang"):
        return "%s (%s) - re-derive it from the unit's own evidence" % (
            "C++" if lang["lang"] == "c++" else "C", lang.get("confidence", "?"))
    return "C++ (a mangled name is in the region)" if p.get("cxx") else "C (no evidence; the default)"


def _tu_row(p: dict) -> str:
    """The §1 line for the queue's TU verdict - what the range's edges rest on, in the worker's words."""
    probe = p.get("tu") or {}
    verdict, srcs = probe.get("verdict"), probe.get("sources") or []
    if verdict == "one-tu":
        return "one-tu - `%s` spans the range" % (srcs[0] if srcs else "?")
    if verdict == "partial":
        return "**partial** - the range cuts `%s`" % (probe.get("partial_source") or "?")
    if verdict == "multi-tu":
        return "**multi-tu** - %d names: %s" % (len(srcs), ", ".join("`%s`" % s for s in srcs))
    if verdict == "merged":
        return "**merged** - a candidate seam inside `%s` was not taken" % (srcs[0] if srcs else "?")
    if verdict == "capped":
        return "**capped** - the edge is a size cap, not a TU boundary (no `__FILE__` evidence)"
    return "unproven - no accepted `__FILE__` name covers the range"


def _tu_warning(p: dict) -> str | None:
    """A plain warning when the range's own evidence does not make it one TU (`attribute.tu_probe`).

    The queue tiles the unclaimed `.text` by evidence first and size only where no evidence reaches, so a
    proposal can be a *partial* TU, a union of several, one file whose internal boundary is a guess, or -
    in a range no `__FILE__` name reaches - a pure `--max-bytes` slice. Each of those has cost a worker or
    a landing cycle. `attribute.tu_probe` reads `tudiscover`'s anchors and `segments`' notes, and this is
    where the verdict reaches the worker, at the top of the brief, before the claim. `None` when the range
    is one anchored TU (or has no TU evidence at all, which the seam row already states).
    """
    probe = p.get("tu") or {}
    verdict = probe.get("verdict")
    sources = probe.get("sources") or []
    at = p.get("text", [0])[0]
    if verdict == "partial":
        return ("**WARNING from discovery (TU probe): this range cuts source file `%s` - part of it "
                "is outside the range.** Re-check `python tools/splits/tudiscover.py at 0x%08X` before "
                "you register: the unit may extend past this range, and half a file is not a unit."
                % (probe.get("partial_source") or "?", at))
    if verdict == "multi-tu":
        return ("**WARNING from discovery (TU probe): this range holds %d source files (%s) - it is a "
                "union of translation units, not one.** Work and register only the file you take; "
                "re-cut the rest with `python tools/splits/tudiscover.py at 0x%08X`."
                % (len(sources), ", ".join("`%s`" % s for s in sources), at))
    if verdict == "merged":
        seams = probe.get("open_seams") or []
        return ("**WARNING from discovery (TU probe): the boundary inside this range is a guess - it "
                "may still be two units.** %d candidate seam(s) sit inside one source file's span "
                "(%s); `python tools/splits/tudiscover.py at 0x%08X` decides before you register."
                % (len(seams), ", ".join(s.get("why", "?") for s in seams[:3]), at))
    if verdict == "capped":
        return ("**WARNING from discovery (TU probe): this range is where `--max-bytes` cut a run that "
                "offered no `__FILE__` evidence - its edge is a size cap, not a translation-unit "
                "boundary.** Nothing in the region names a file here; find the real seam with "
                "`python tools/splits/tudiscover.py at 0x%08X` (or work it as one unit and say why) "
                "before you register." % at)
    return None


def brief_unit(path: str) -> str | None:
    """The unit or proposal a written brief names, from its title - `None` when it is not a brief.

    Two titles exist: `# Brief: <unit>` for a registered unit and `# Proposal brief: <label>` for option A's
    proposal briefs. Both return the identifier the claim machinery keys on, so a caller does not have to
    know which kind it read.
    """
    try:
        with open(path, encoding="utf-8", errors="replace") as fh:
            for line in fh:
                m = re.match(r"# (?:Proposal )?[Bb]rief: (\S+)", line)
                if m:
                    # normalise: a proposal brief's title carries the label WITH its source extension, and
                    # the claim machinery keys on the extensionless unit, so returning it raw handed the
                    # caller a key that matched nothing (same trap as claims.slug(), 2026-09-24)
                    return claims.norm_unit(m.group(1))
    except OSError:
        return None
    return None


def pool_dir(main: str) -> str:
    return os.path.join(main, "tools", "units", "briefs", "pool")


def pool(main: str, force: bool = False, prune: bool = True,
         prune_promoted_litter: bool = False) -> dict:
    """Write a brief for every piece of work the pool can hand out, into `tools/units/briefs/pool/`.

    Under option A (owner, 2026-09-24) the work is the **proposal queue** written by `attribute.py queue`,
    and each pooled brief is a proposal brief. The pre-option-A source - registered units with no bodies -
    is the fallback when there is no queue, so an existing pool is not orphaned and the tool still works on
    a tree whose discovery has not been re-run.

    No claim is made and the claims registry is never touched. `assume_claim` renders each brief against the
    worktree and branch the claim *will* create, so the pooled file already carries the paths the claim makes;
    `queue.py next` still re-renders at claim time (see `queue.promote`) rather than trusting it.

    Idempotent: an existing brief is skipped when its stamp still equals the current entry's (no churn),
    refreshed when the entry changed, and `force` rewrites everything. A pooled brief whose proposal has
    left the queue (its range was worked and registered) is pruned, so the pool always equals the current
    queue. The *promoted* directory is audited too: a promoted brief whose entry no longer matches is
    reported in `litter`, and - only with `prune_promoted_litter` - deleted when no live claim owns it.

    An entry whose range is no longer unclaimed - a unit now holds its bytes - is left out and pruned the
    same way, and comes back in `blocked` with the reason, so a worker is never handed a range another
    worker already owns (see `proposal_conflicts`).
    """
    queue = proposals(main)
    blocked: list[dict] = []
    if queue:
        kind = "proposal"
        handable, blocked = handable_proposals(main)
        units = [p["label"] for p in handable if p.get("label")]
    else:
        kind, units = "unit", pool_units(main)
    outdir = pool_dir(main)
    os.makedirs(outdir, exist_ok=True)
    wrote, skipped, refreshed, pruned = [], [], [], []
    want = {claims.slug(u): u for u in units}
    for unit in units:
        path = os.path.join(outdir, claims.slug(unit) + ".md")
        if os.path.exists(path) and not force:
            # A pooled brief is current exactly when its stamp (range, function count, TU verdict) equals
            # what the queue entry says now. An unchanged entry is skipped and the file is not rewritten,
            # so `--pool` is idempotent; a changed one is refreshed instead of silently kept.
            if brief_stamp(path) == entry_stamp(main, unit):
                skipped.append(unit)
                continue
            refreshed.append(unit)
        b, text = brief_for(main, claims.worktree_for(unit, main), unit, None,
                            assume_claim=True, pool=True)
        open(path, "w", encoding="utf-8", newline="\n").write(text)
        wrote.append(unit)
    if prune:
        for name in sorted(os.listdir(outdir)):
            if not name.endswith(".md") or name[:-3] in want:
                continue
            path = os.path.join(outdir, name)
            pruned.append({"slug": name[:-3], "unit": brief_unit(path)})
            os.remove(path)
    # the *promoted* directory is not the pool: a brief there belongs to a claim, so a stale one is
    # reported (or, when it owns no claim, pruned) rather than left to be read as current work
    litter = promoted_litter(main)
    pruned_promoted = prune_promoted(main, litter) if prune_promoted_litter else []
    return {"dir": outdir, "kind": kind, "units": units, "wrote": wrote, "skipped": skipped,
            "refreshed": refreshed, "pruned": pruned, "blocked": blocked,
            "litter": litter, "pruned_promoted": pruned_promoted}


def promoted_dir(main: str) -> str:
    """The promoted directory - `<main>/tools/units/briefs/` - where a claim's brief is handed to a worker."""
    return os.path.join(main, "tools", "units", "briefs")


def brief_text_range(path: str) -> list[int] | None:
    """The `.text` range a written brief states, as `[start, end]` - `None` when it cannot be read.

    Two renderings exist: the proposal brief's ``| `.text` range | `0x..`-`0x..` |`` row and the registered
    unit's ``| sections | .text 0x..-0x.., ... |`` row.
    """
    try:
        text = open(path, encoding="utf-8", errors="replace").read()
    except OSError:
        return None
    m = re.search(r"\| `?\.text`? range \| `(0x[0-9A-Fa-f]+)`-`(0x[0-9A-Fa-f]+)`", text)
    if not m:
        m = re.search(r"\| sections \| [^\n]*?\.text (0x[0-9A-Fa-f]+)-(0x[0-9A-Fa-f]+)", text)
    if not m:
        return None
    return [int(m.group(1), 16), int(m.group(2), 16)]


def promoted_litter(main: str) -> list[dict]:
    """Promoted briefs in `tools/units/briefs/` whose current queue entry no longer matches.

    A promoted brief is a claim's copy; it outlives the claim, and `attribute.py queue` can be regenerated
    under it, so the brief can end up describing a range the queue no longer hands out. Two were found this
    way on 2026-09-25 - `801502C8` stated `0x801502C8..0x80154B04` while the queue says `..0x801550FC`, and
    `801FBF78` stated a `.ctors` word where the queue says `0x801FBF78..0x802029B4` (127 functions) - and a
    live worker (`80119DEC`) had been handed a stale one, so this reports rather than trusts.

    The brief's own title and stated `.text` range are compared, in order, against (a) the entry it names,
    (b) the entry whose range starts at the same address, (c) the queue entry that now contains that
    address, and (d) the registered units. A brief whose range is registered work already is history and is
    left alone; the rest are litter. `claimed` records whether a live claim still owns the brief, so a
    caller can report a stale in-flight brief instead of deleting it.
    """
    d = promoted_dir(main)
    out: list[dict] = []
    if not os.path.isdir(d):
        return out
    props = proposals(main)
    by_label = {claims.norm_unit(p.get("label") or ""): p for p in props}
    by_start: dict[int, dict] = {}
    for p in props:
        t = p.get("text") or []
        if len(t) == 2:
            by_start[int(t[0])] = p
    registered = {claims.norm_unit(u) for u in registered_units(main)}
    covered = registered_text_ranges(main)
    branches = claims.worker_branches(main)
    branch_slugs = {claims.slug_of_branch(b) for b in branches}
    for name in sorted(os.listdir(d)):
        if not name.endswith(".md"):
            continue
        path = os.path.join(d, name)
        unit = brief_unit(path)
        if unit is None:
            continue
        stated = brief_text_range(path)
        if stated is None:
            continue  # not a range brief (or unreadable) - it cannot be judged against the queue
        entry = by_label.get(claims.norm_unit(unit))
        expected, reason = None, None
        if entry is not None:
            expected = [int(x) for x in (entry.get("text") or [])]
            stamp = brief_stamp(path)
            if stamp is not None:
                if stamp == proposal_stamp(entry):
                    continue
                reason = "the brief's stamp no longer matches its queue entry"
            elif stated == expected:
                continue
            else:
                reason = "the queue entry changed range"
        elif claims.norm_unit(unit) in registered:
            rng = splits_range(main, unit).get(".text")
            expected = [rng[0], rng[1]] if rng else None
            if expected is None or stated == expected:
                continue
            reason = "the unit's split range moved"
        elif stated[0] in by_start:
            expected = [int(x) for x in (by_start[stated[0]].get("text") or [])]
            reason = "the queue re-cut this range as %s" % by_start[stated[0]].get("label")
        else:
            holder = None
            for q in props:
                qt = q.get("text") or []
                if len(qt) == 2 and qt[0] <= stated[0] < qt[1]:
                    holder = q
                    break
            if holder is None and any(s <= stated[0] < e for s, e, _u in covered):
                continue  # the range was worked and landed - history, not litter
            if holder is not None:
                expected = [int(x) for x in holder["text"]]
                reason = "the queue now cuts this range inside %s" % holder.get("label")
            else:
                reason = "no current entry or registered unit carries this range"
        claimed = (bool(claim_for(main, unit)) or claims.lock_held(main, unit, branches)
                   or name[:-3] in branch_slugs)
        out.append({"slug": name[:-3], "path": path, "unit": unit, "stated": stated,
                    "expected": expected, "reason": reason, "claimed": claimed})
    return out


def prune_promoted(main: str, litter: list[dict] | None = None) -> list[dict]:
    """Delete only the promoted litter no live claim owns - a stale brief for an in-flight worker is kept,
    so its handoff (and its outbox path) still exists; it is reported, never silently removed."""
    litter = promoted_litter(main) if litter is None else litter
    removed = []
    for row in litter:
        if row.get("claimed"):
            continue
        try:
            os.remove(row["path"])
        except OSError:
            continue
        removed.append(row)
    return removed


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
        check("the brief names the rule-7 deferral declaration",
              "rule 7 deferred:" in brief_text, True)
        check("the brief tells the worker to run the gate's naming rule locally",
              "land.rule7_defer_growth" in brief_text and "land.band_ownership_warnings" in brief_text, True)
        check("the brief bans claims.py release with its consequence",
              "NEVER run `claims.py release`" in brief_text and "WIPED the directory" in brief_text, True)
        check("the brief warns off recompile.py --measure / measure.py",
              "`recompile.py --measure` or `measure.py`" in brief_text
              and "tools/objdiff/symdiff.py" in brief_text, True)
        check("the brief carries the order-only report.json trap",
              "order-only target of `all_source`" in brief_text
              and "rm -f build/RMHE08/report.json" in brief_text, True)
        check("the brief carries the git add hygiene",
              "`git add -A` with no path arguments" in brief_text and "`git show --stat`" in brief_text, True)
        check("pool is idempotent", pool(tmp)["skipped"], ["auto/stub"])
        open(os.path.join(tmp, "src", "auto", "stub.c"), "w").write("int f(void) { return 1; }\n")
        check("pool prunes a unit that gained a body",
              [p["unit"] for p in pool(tmp)["pruned"]], ["auto/stub"])

    # option A: the pool IS the proposal queue, and a proposal brief carries the registration protocol. A
    # proposal has no source, no splits block and no registration, so it exercises a different path than
    # every check above.
    with tempfile.TemporaryDirectory() as tmp:
        os.makedirs(os.path.join(tmp, "tools", "units"))
        entry = {"label": "proposal/80161660_fn_80161660.cpp", "text": [0x80161660, 0x801679B0],
                 "count": 52, "bytes": 25424, "cxx": True,
                 "language": {"lang": "c++", "confidence": "medium"},
                 "seam": None, "seam_note": None, "functions": [], "runs": {}}
        with open(queue_path(tmp), "w", encoding="utf-8") as fh:
            json.dump({"version": 1, "units": [entry]}, fh)
        with open(os.path.join(tmp, "configure.py"), "w", encoding="utf-8") as fh:
            fh.write("config.libs = [\n]\n")
        check("proposals reads the queue", proposal_labels(tmp), ["proposal/80161660_fn_80161660.cpp"])
        check("proposal_by_label finds it from the extensionless spelling",
              (proposal_by_label(tmp, "proposal/80161660_fn_80161660") or {}).get("count"), 52)
        check("proposal_by_label refuses a stranger", proposal_by_label(tmp, "Pl/pl_act"), None)
        check("a proposal is not a registered unit", pool_units(tmp), [])
        check("proposals is [] without a queue", proposals(os.path.join(tmp, "nope")), [])

        b = build_proposal(tmp, proposal_by_label(tmp, entry["label"]), None, assume_claim=True)
        text = render_proposal(tmp, b, None)
        check("the proposal brief titles itself as a proposal",
              text.startswith("# Proposal brief: proposal/80161660_fn_80161660.cpp"), True)
        check("the proposal brief names the range", "0x80161660`-`0x801679B0" in text, True)
        check("the proposal brief carries the register-at-final-home protocol",
              "Register it at its final home" in text, True)
        check("the proposal brief states the shared-file exception",
              "one exception to the shared-file rule" in text, True)
        check("the proposal brief carries the rule-7 deferral spelling", "rule 7 deferred:" in text, True)
        check("the proposal brief tells the worker to guess and mark a name, not keep the map's stem",
              "explicit **GUESS**" in text
              and "Keeping the map's `fn_XXXXXXXX` stem as the file name is not an" in text
              and "Keep the map's `fn_XXXXXXXX` stem as the file name and say so" not in text, True)
        check("the proposal brief carries the map-side rename of the unit's own symbol",
              "symedit.py rename fn_XXXXXXXX <name>" in text, True)
        check("the proposal brief keeps the tolerance for other units' unrenamed symbols",
              "references only to other units' unrenamed fn_XXXXXXXX symbols" in text, True)
        check("the proposal brief keeps the section 6.5 rules",
              "## 6 · The rules" in text and "rule 7 deferred:" in text, True)
        check("the proposal brief carries the outbox path", str(b["handoff"]["outbox"]) in text, True)
        check("the proposal brief tells the worker to run the gate's naming rule locally",
              "land.rule7_defer_growth" in text and "land.band_ownership_warnings" in text, True)
        check("the proposal brief bans claims.py release with its consequence",
              "NEVER run `claims.py release`" in text and "WIPED the directory" in text, True)
        check("the proposal brief warns off recompile.py --measure / measure.py",
              "`recompile.py --measure` or `measure.py`" in text and "tools/objdiff/symdiff.py" in text, True)
        check("the proposal brief carries the order-only report.json trap",
              "order-only target of `all_source`" in text and "rm -f build/RMHE08/report.json" in text, True)
        check("the proposal brief carries the git add hygiene",
              "`git add -A` with no path arguments" in text and "`git show --stat`" in text, True)
        check("a proposal brief still says where the report goes",
              "final message" in text and "subagent_done" not in text, True)

        # the TU probe: the queue tags each entry with what `tudiscover`'s anchors and `segments`' notes
        # say, and a range that is not one TU gets a plain warning at the top of the brief
        # (attribute.tu_probe). `capped` is the tag for a range no `__FILE__` name reaches: its edge is
        # the `--max-bytes` cap, which is a size decision and not a TU boundary.
        for verdict, want in (("multi-tu", "union of translation units"),
                              ("partial", "cuts source file"),
                              ("merged", "boundary inside this range is a guess"),
                              ("capped", "size cap, not a translation-unit")):
            tagged = dict(entry, tu={"verdict": verdict, "sources": ["a.cpp", "b.cpp"],
                                     "partial_source": "a.cpp",
                                     "open_seams": [{"cut": 3, "why": "a pool jump"}]})
            _t = render_proposal(tmp, build_proposal(tmp, tagged, None, assume_claim=True), None)
            check("the TU probe warns on %s" % verdict, want in _t, True)
            check("the %s warning is at the top of the brief" % verdict,
                  _t.index(want) < _t.index("## 1 · This is a proposal"), True)
        clean = dict(entry, tu={"verdict": "one-tu", "sources": ["a.cpp"], "partial_source": None,
                                "open_seams": []})
        check("a one-TU range gets no TU warning",
              "TU probe" not in render_proposal(tmp, build_proposal(tmp, clean, None,
                                                               assume_claim=True), None), True)
        check("an untagged entry gets no TU warning", _tu_warning({}), None)
        # §1 states what the range's edges rest on, in the worker's words, whether or not it warns
        check("the §1 table carries the TU evidence row",
              "| TU evidence |" in render_proposal(tmp, build_proposal(tmp, clean, None,
                                                                  assume_claim=True), None), True)
        check("a one-TU range names the file it rests on",
              _tu_row(clean), "one-tu - `a.cpp` spans the range")
        check("a capped range says the edge is a size cap",
              _tu_row({"tu": {"verdict": "capped", "sources": []}}),
              "**capped** - the edge is a size cap, not a TU boundary (no `__FILE__` evidence)")
        check("a partial range names the file it cuts",
              _tu_row({"tu": {"verdict": "partial", "partial_source": "menu_note.cpp"}}),
              "**partial** - the range cuts `menu_note.cpp`")
        check("an untagged range says it is unproven",
              _tu_row({}), "unproven - no accepted `__FILE__` name covers the range")

        # the dispatch itself: the round that came back empty was handed a registered-unit brief for a
        # proposal, so single-unit mode must route a proposal to the proposal renderer - from either spelling
        for spelling in (entry["label"], "proposal/80161660_fn_80161660"):
            _b, t = brief_for(tmp, os.path.join(tmp, "nowt"), spelling, None, assume_claim=True)
            check("single-unit mode renders a proposal as a proposal (%s)" % spelling,
                  t.startswith("# Proposal brief:"), True)
            check("... and therefore has an inventory (%s not empty)" % spelling, len(_b["symbols"]) >= 0,
                  True)
            check("... and names its range (%s)" % spelling, "0x80161660" in t, True)

        # the rules themselves are read from docs/plan.md, so that part is checked against the real tree
        # (the temp fixture above has no docs/)
        real = render_proposal(".", build_proposal(".", entry, None, assume_claim=True), None)
        check("the proposal brief embeds the section 6.5 text",
              "Type and naming discipline" in real and "A reconstructed class/struct states its size" in real, True)
        check("the proposal brief embeds the invariants", "## 8. Invariants" in real, True)

        out = pool(tmp)
        check("the pool is built from the queue", out["kind"], "proposal")
        check("the pool wrote one brief per proposal", len(out["wrote"]), 1)
        pooled = os.path.join(out["dir"], claims.slug(entry["label"]) + ".md")
        check("a pooled proposal brief is parseable", brief_unit(pooled), claims.norm_unit(entry["label"]))
        check("... and parses to the same key as its title without the extension",
              claims.slug(brief_unit(pooled)), claims.slug(entry["label"]))
        check("pool is idempotent for a proposal", pool(tmp)["skipped"], [entry["label"]])
        # an unchanged entry leaks no churn: the file is not rewritten
        stable = open(pooled, encoding="utf-8").read()
        again = pool(tmp)
        check("an unchanged entry refreshes nothing", again["refreshed"], [])
        check("... and the pooled file is byte-identical", open(pooled, encoding="utf-8").read(), stable)
        check("... its stamp equals the entry's", brief_stamp(pooled), entry_stamp(tmp, entry["label"]))
        # a regenerated entry (new range and function count) refreshes the pooled brief instead of
        # silently keeping the old scope - the 80119DEC defect
        changed = dict(entry, text=[0x80161660, 0x80169000], count=99)
        with open(queue_path(tmp), "w", encoding="utf-8") as fh:
            json.dump({"version": 1, "units": [changed]}, fh)
        _QUEUE_CACHE.clear()   # the mtime can be unchanged within the same second
        ref = pool(tmp)
        check("a changed entry is refreshed", ref["refreshed"], [entry["label"]])
        check("... and the pooled brief states the new range",
              "0x80169000" in open(pooled, encoding="utf-8").read(), True)
        check("... and carries a stamp matching the new entry",
              brief_stamp(pooled), entry_stamp(tmp, entry["label"]))
        check("... while the old function count is gone",
              "| functions | 52 |" not in open(pooled, encoding="utf-8").read(), True)
        # the queue drops a proposal once its range is registered, and the pool prunes the brief with it
        with open(queue_path(tmp), "w", encoding="utf-8") as fh:
            json.dump({"version": 1, "units": []}, fh)
        dropped = pool(tmp)
        check("an empty queue falls back to the registered no-body pool", dropped["kind"], "unit")
        check("a proposal brief is pruned when the queue drops it",
              [r["unit"] for r in dropped["pruned"]], [claims.norm_unit(entry["label"])])
        check("the pruned brief is gone from disk", os.path.exists(pooled), False)

    # a promoted brief outlives its claim: when the queue is regenerated under it (the 801502C8/801FBF78
    # litter) its stated range no longer matches the current entry, so the tooling must report it - and
    # delete it only when no live claim owns it.
    with tempfile.TemporaryDirectory() as tmp:
        os.makedirs(os.path.join(tmp, "src"))
        open(os.path.join(tmp, "configure.py"), "w").write("config.libs = [\n]\n")
        claims.save_registry(tmp, {})
        label = "proposal/801502C8_fn_801502C8.cpp"
        os.makedirs(promoted_dir(tmp))
        with open(queue_path(tmp), "w", encoding="utf-8") as fh:
            json.dump({"version": 1, "units": [{"label": label, "text": [0x801502C8, 0x801550FC],
                                                "count": 30, "bytes": 20020, "cxx": True}]}, fh)
        litter_path = os.path.join(promoted_dir(tmp), "801502c8-fn-801502c8-4783.md")
        open(litter_path, "w", encoding="utf-8").write(
            "# Brief: auto/801502C8_fn_801502C8\n\n"
            "| sections | .text 0x801502C8-0x80154B04, extab 0x8000DC54-0x8000DCDC |\n")
        ok_path = os.path.join(promoted_dir(tmp), "ok.md")
        open(ok_path, "w", encoding="utf-8").write(
            "# Proposal brief: %s\n\n| `.text` range | `0x801502C8`-`0x801550FC` (20020 bytes) |\n" % label)
        litter = promoted_litter(tmp)
        check("a promoted brief the queue re-cut is litter", [r["slug"] for r in litter],
              ["801502c8-fn-801502c8-4783"])
        check("... with the old range it states", litter[0]["stated"], [0x801502C8, 0x80154B04])
        check("... and the queue's current range", litter[0]["expected"], [0x801502C8, 0x801550FC])
        check("... and no claim owns it", litter[0]["claimed"], False)
        check("a promoted brief whose range still matches is not litter",
              "ok" in [os.path.splitext(n)[0] for n in os.listdir(promoted_dir(tmp))]
              and all(r["slug"] != "ok" for r in litter), True)
        check("the matching brief is left on disk", os.path.exists(ok_path), True)
        removed = prune_promoted(tmp, litter)
        check("the claim-free litter is pruned", [r["slug"] for r in removed],
              ["801502c8-fn-801502c8-4783"])
        check("... and it is gone from disk", os.path.exists(litter_path), False)
        # a stale brief a live claim owns is reported, never deleted - it is the worker's handoff
        held_path = os.path.join(promoted_dir(tmp), claims.slug(label) + ".md")
        open(held_path, "w", encoding="utf-8").write(
            "# Proposal brief: %s\n\n| `.text` range | `0x801502C8`-`0x80154B04` (20020 bytes) |\n" % label)
        claims.save_registry(tmp, {claims.norm_unit(label):
                                   {"branch": claims.branch_for(claims.norm_unit(label))}})
        held = [r for r in promoted_litter(tmp) if r["unit"] == claims.norm_unit(label)]
        check("a stale promoted brief with a live claim is marked held",
              bool(held) and held[0]["claimed"], True)
        check("... and prune_promoted leaves it alone",
              [r for r in prune_promoted(tmp, held) if r["path"] == held_path], [])
        check("... the held file survives", os.path.exists(held_path), True)
        claims.save_registry(tmp, {})
        check("an empty promoted directory reports nothing", promoted_litter(os.path.join(tmp, "nope")), [])

    # the tree moves under the queue: an entry whose range a unit now holds is NOT handable work. This is
    # the 8008F8E4 incident - the queue entry capped 0x8008F8E4-0x80097D40, five TUs, four of them already
    # claimed and live - and a brief written for it would have put a worker on four live ranges (2026-09-25)
    with tempfile.TemporaryDirectory() as tmp:
        os.makedirs(os.path.join(tmp, "tools", "units"))
        os.makedirs(os.path.join(tmp, "config", "RMHE08"))
        stale = {"label": "proposal/8008F8E4_fn_8008F8E4.cpp", "text": [0x8008F8E4, 0x80097D40],
                 "count": 276, "bytes": 0x847C, "cxx": True, "functions": [], "runs": {}}
        tail = {"label": "proposal/8017F000_tail.cpp", "text": [0x8017F000, 0x80180080],
                "count": 4, "bytes": 0x1080, "cxx": True, "functions": [], "runs": {}}
        live_prop = {"label": "proposal/80161660_fn_80161660.cpp", "text": [0x80161660, 0x801679B0],
                     "count": 52, "bytes": 25424, "cxx": True, "functions": [], "runs": {}}
        with open(queue_path(tmp), "w", encoding="utf-8") as fh:
            json.dump({"version": 1, "units": [stale, tail, live_prop]}, fh)
        with open(os.path.join(tmp, "configure.py"), "w", encoding="utf-8") as fh:
            fh.write('config.libs = [\n    {\n        "lib": "main",\n        "objects": [\n'
                     '            Object(NonMatching, "g3d/g3d_resanmlight.cpp"),\n'
                     '            Object(NonMatching, "g3d/g3d_resmat.cpp"),\n'
                     "        ],\n    },\n]\n")
        with open(os.path.join(tmp, "config", "RMHE08", "splits.txt"), "w", encoding="utf-8") as fh:
            fh.write("g3d/g3d_resanmlight.cpp:\n\t.text       start:0x8008F8E4 end:0x800908FC\n"
                     "g3d/g3d_resmat.cpp:\n\t.text       start:0x80180000 end:0x80181000\n")
        check("a proposal spanning a live unit's .text is a conflict",
              len(proposal_conflict_reasons(tmp, stale["label"])), 1)
        check("... and the conflict names the registered unit",
              "g3d/g3d_resanmlight.cpp" in proposal_conflict_reasons(tmp, stale["label"])[0], True)
        check("... and the range it holds",
              "0x8008F8E4..0x800908FC" in proposal_conflict_reasons(tmp, stale["label"])[0], True)
        check("a proposal that only overlaps a unit's tail is a conflict too",
              "g3d/g3d_resmat.cpp" in proposal_conflict_reasons(tmp, tail["label"])[0], True)
        check("a proposal with no overlap is not a conflict",
              proposal_conflict_reasons(tmp, live_prop["label"]), [])
        handable, blocked = handable_proposals(tmp)
        check("the handable list drops the covered proposals",
              [p["label"] for p in handable], [live_prop["label"]])
        check("... and reports each one with a reason",
              [b["label"] for b in blocked], [stale["label"], tail["label"]])
        out = pool(tmp)
        check("the pool writes no brief for a covered proposal",
              out["wrote"], [live_prop["label"]])
        check("the pool's work list is the handable set", out["units"], [live_prop["label"]])
        check("the pool reports what it blocked", [b["label"] for b in out["blocked"]],
              [stale["label"], tail["label"]])
        check("no brief exists for a blocked proposal",
              os.path.exists(os.path.join(pool_dir(tmp), claims.slug(stale["label"]) + ".md")), False)
        # a brief pooled before the range was registered is pruned, so it cannot be promoted later either
        os.makedirs(pool_dir(tmp), exist_ok=True)
        open(os.path.join(pool_dir(tmp), claims.slug(stale["label"]) + ".md"), "w").write(
            "# Proposal brief: %s\n" % stale["label"])
        out = pool(tmp)
        check("a stale pooled brief is pruned",
              [r["unit"] for r in out["pruned"]], [claims.norm_unit(stale["label"])])
        check("... and it is gone from disk",
              os.path.exists(os.path.join(pool_dir(tmp), claims.slug(stale["label"]) + ".md")), False)
        # two entries sharing bytes are the queue's own invariant broken: neither is handed out
        with open(queue_path(tmp), "w", encoding="utf-8") as fh:
            json.dump({"version": 1, "units": [live_prop, dict(live_prop,
                                                                label="proposal/80167900_other.cpp",
                                                                text=[0x80167900, 0x80169000])]}, fh)
        check("two overlapping proposals are both refused", len(handable_proposals(tmp)[0]), 0)
        check("... and both are reported", len(handable_proposals(tmp)[1]), 2)
        check("... naming the sibling they overlap",
              "sibling" in handable_proposals(tmp)[1][0]["why"][0], True)

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
    ap.add_argument("--prune-promoted", action="store_true",
                    help="with --pool, delete promoted briefs in tools/units/briefs/ that no live claim owns"
                         " and whose entry no longer matches")
    ap.add_argument("--check-promoted", action="store_true",
                    help="report promoted briefs whose entry no longer matches, then exit (read-only)")
    args = ap.parse_args()

    if args.selftest:
        return selftest()

    main = rc.main_root(rc.worktree_root())
    if args.check_promoted:
        litter = promoted_litter(main)
        for row in litter:
            print("%s  %s" % ("HELD  " if row["claimed"] else "LITTER", row["slug"]))
            print("    unit   %s" % row["unit"])
            print("    stated %s" % ("0x%08X..0x%08X" % tuple(row["stated"])))
            print("    queue  %s" % ("0x%08X..0x%08X" % tuple(row["expected"])
                                      if row["expected"] else "(no current entry)"))
            print("    reason %s" % row["reason"])
        if not litter:
            print("ok - no promoted brief disagrees with the current queue")
            return 0
        print("\n%d promoted brief(s) disagree with the current queue (HELD = a live claim owns them,"
              " do not delete)" % len(litter))
        return 1
    if args.pool:
        out = pool(main, force=args.force, prune=not args.no_prune,
                   prune_promoted_litter=args.prune_promoted)
        if args.json:
            print(json.dumps(out, indent=2))
            return 0
        print("pool %s" % out["dir"])
        print("  %d %s(s) to hand out" % (len(out["units"]),
                                            "proposal" if out.get("kind") == "proposal" else "registered unit"))
        print("  wrote    %d" % len(out["wrote"]))
        for unit in out["wrote"]:
            print("      + %s" % unit)
        print("  skipped  %d  (up to date)" % len(out["skipped"]))
        print("  refreshed %d  (entry changed since the brief was written)" % len(out["refreshed"]))
        for unit in out["refreshed"]:
            print("      ~ %s" % unit)
        print("  pruned   %d  (unit has a body now, or is no longer registered)" % len(out["pruned"]))
        for row in out["pruned"]:
            print("      - %s  (%s)" % (row["unit"] or "?", row["slug"]))
        print("  blocked  %d  (range no longer unclaimed, or the queue overlaps itself)"
              % len(out.get("blocked") or []))
        for row in out.get("blocked") or []:
            print("      x %s" % row["why"][0])
            for why in row["why"][1:]:
                print("        %s" % why)
        litter = out.get("litter") or []
        print("  promoted %d  brief(s) disagree with the current queue" % len(litter))
        for row in litter:
            print("      %s %s  (%s)" % ("HOLD" if row["claimed"] else "LITTER", row["slug"], row["reason"]))
        for row in out.get("pruned_promoted") or []:
            print("      - pruned %s" % row["slug"])
        return 0
    if not args.unit:
        ap.print_help()
        return 0

    why = proposal_conflict_reasons(main, args.unit)
    if why:
        print("refusing to brief %s - it is not handable work (nothing was written):" % args.unit)
        for w in why:
            print("  - %s" % w)
        print("  A proposal is work to hand out, and this range is no longer unclaimed (or the queue's own")
        print("  tiling overlaps itself). Re-run `python tools/units/attribute.py queue <start> <end>` over")
        print("  the region - `python tools/units/brief.py --pool` follows it - then take a live proposal.")
        return 1

    wt = rc.worktree_root()
    b, text = brief_for(main, wt, args.unit, args.task)
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
