"""The reports: the per-unit budget, the distinct-name counts, the rule-2 shape and the `--findings` listing.
Spec: docs/tools/spec/stylelint_rules.md. CLI: none (the stylelint package; `stylelint.py` is the CLI)."""
from __future__ import annotations

import collections
import json
import os
import re
import subprocess
import sys
import time

from tools.lib import findings as _findings
from tools.units.stylelint_rules.common import (AUDIT_RULES, EXEMPT, RULE7_NOTES, RULE_NAMES, UNCHECKED,
    UNSPLIT_UNRESOLVED)
from tools.units.stylelint_rules.lint import rule11_local_total
from tools.units.stylelint_rules.r15_comments import ADVISORY, REFUSING


def budget(findings: list[dict], rule10: dict | None = None) -> dict:
    """Per-unit rule counts plus the totals, both keyed by unit (path relative to the repo root).

    `rule10` is `{file: count}` from vtableaudit (`rule10_counts`) - plan 6.5 rule 10 is the audit's, the lint has
    no rule-10 finding - added to the rule-10 column and to `total`; `findings` stays the lint's own count. `None`
    (the audit was not run or could not read the tree) leaves the column at 0 and `rule10` null in the JSON."""
    per: dict[str, dict] = {}

    def row_of(rel: str) -> dict:
        return per.setdefault(rel, {"file": rel, "total": 0, "rules": {str(r): 0 for r in RULE_NAMES}})
    for f in findings:
        row = row_of(f["file"])
        row["rules"][str(f["rule"])] += 1
        row["total"] += 1
    for rel, n in sorted((rule10 or {}).items()):
        row = row_of(rel)
        row["rules"]["10"] += n
        row["total"] += n
    totals = {str(r): sum(row["rules"][str(r)] for row in per.values()) for r in RULE_NAMES}
    return {"units": [per[k] for k in sorted(per)], "totals": totals,
            "total": sum(totals.values()), "findings": len(findings), "unique": unique_names(findings),
            "rule10": None if rule10 is None else {"source": AUDIT_RULES[10], "violations": sum(rule10.values())},
            "rule15": rule15_classes(findings)}


def rule15_classes(findings: list[dict]) -> dict:
    """Rule 15's column split by class: `{"refusing": {check: n}, "advisory": {check: n}}`, each in the order
    `REFUSING` / `ADVISORY` name them (a class with no finding reads 0). The `r15` column is their sum."""
    counts = collections.Counter(f.get("check") for f in findings if f["rule"] == 15)
    return {"refusing": {c: counts.get(c, 0) for c in REFUSING}, "advisory": {c: counts.get(c, 0) for c in ADVISORY}}


def rule10_counts(root: str) -> tuple[dict | None, str]:
    """`({file: count}, note)` - plan 6.5 rule 10's violations per source file, read from `vtableaudit.py --json`
    (a subprocess: the audit is a tool, not a library of the lint). A run counts against `src/<unit>`, an own-range
    vtable write against its file - the same two kinds the gate's rule-10 row compares. `(None, why)` when the
    audit cannot run or read the tree; the note says how long it took and how many units were unbuilt."""
    audit = os.path.join(root, *AUDIT_RULES[10].split("/"))
    t0 = time.time()
    try:
        p = subprocess.run([sys.executable, audit, "--main", root, "--json"], cwd=root, capture_output=True,
                           text=True, encoding="utf-8", errors="replace")
        data = json.loads(p.stdout) if p.returncode == 0 else None
    except (OSError, ValueError):
        data = None
    if not isinstance(data, dict):
        return None, "rule 10: vtableaudit could not read this tree - the column stays 0"
    out: dict = collections.Counter()
    for run in data.get("violations") or []:
        out["src/" + run["unit"].replace("\\", "/")] += 1
    for ref in data.get("references") or []:
        if ref.get("kind") == "own":
            rel = ref["file"].replace("\\", "/")
            out[rel if rel.startswith("src/") else "src/" + rel] += 1
    return dict(out), ("rule 10 (vtableaudit, %.1f s): %d violation(s) over %d file(s); %d unit(s) unbuilt"
                       % (time.time() - t0, sum(out.values()), len(out), len(data.get("unbuilt") or [])))


def unique_names(findings: list[dict]) -> dict:
    """Distinct names behind the name-based rules - the burn-down number a rename closes in one edit."""
    def names(rule: int, prefix: str) -> set:
        return {m.group(1) for f in findings if f["rule"] == rule and f["detail"].startswith(prefix)
                for m in [re.search(r"`([^`]+)`", f["detail"])] if m}
    return {"fn_names": len(names(7, "auto")), "unk_identifiers": len(names(7, "bare")),
            "label_names": len(names(7, "data")), "address_names": len(names(7, "address")),
            "unk_fields": len(names(5, "")), "types": len(names(3, "")),
            "shared_types": len(names(1, "")), "extern_symbols": len(names(2, "")),
            "mangled_names": len(names(9, "")), "unowned_data_symbols": len(names(12, "")),
            "c_spelled_methods": len(names(13, ""))}


# --------------------------------------------------------------------------------------------------
# reporting
# --------------------------------------------------------------------------------------------------
def print_rule2_report(ownership: "Ownership | None") -> None:
    """The rule-2 backlog shape: owners needing a header, the unsplit modules, and the counted gaps."""
    if ownership is None:
        print("rule 2: unchecked (config/RMHE08/symbols.txt or splits.txt is absent)")
        return
    if ownership.foreign_units:
        print("rule 2: %d declaration site(s) into %d other owner unit(s); top: %s"
              % (sum(ownership.foreign_units.values()), len(ownership.foreign_units),
                 ", ".join("%s (%d)" % (u, n) for u, n in ownership.foreign_units.most_common(5))))
    if ownership.unsplit_modules:
        print("rule 2: the unsplit band (src/unsplit/*.h) would carry %d declaration site(s) for %d symbol(s): %s"
              % (sum(ownership.unsplit_modules.values()),
                 sum(len(v) for v in ownership.unsplit_symbols.values()),
                 ", ".join("%s %d site(s)/%d symbol(s)"
                           % ("unresolved band" if m == UNSPLIT_UNRESOLVED else m, n,
                              len(ownership.unsplit_symbols.get(m, ())))
                           for m, n in sorted(ownership.unsplit_modules.items()))))
    for reason, n in sorted(ownership.gaps.items()):
        print("rule 2 gap: %d declaration site(s) - %s (documented, not guessed)" % (n, reason))
    if not ownership.foreign_units and not ownership.unsplit_modules and not ownership.gaps:
        print("rule 2: every extern declaration was judged")


def print_budget(findings: list[dict], ownership: "Ownership | None" = None,
                 root: str | None = None, rule10: dict | None = None, rule10_note: str = "") -> None:
    b = budget(findings, rule10)
    width = max([len(row["file"]) for row in b["units"]] + [len("TOTAL")])
    head = "".join("  r%d" % r for r in RULE_NAMES)
    print("%-*s  %4s%s" % (width, "unit", "tot", head))
    for row in b["units"]:
        print("%-*s  %4d%s" % (width, row["file"], row["total"],
                               "".join("  %2d" % row["rules"][str(r)] for r in RULE_NAMES)))
    print("%-*s  %4d%s" % (width, "TOTAL", b["total"],
                           "".join("  %2d" % b["totals"][str(r)] for r in RULE_NAMES)))
    print("")
    u = unique_names(findings)
    print("distinct names: rule 1 %d shared type(s), rule 2 %d extern symbol(s), rule 3 %d type(s), "
          "rule 5 %d field(s), rule 7 %d fn_* + %d unk identifier(s) + %d data label(s) + %d address-named, "
          "rule 9 %d mangled name(s)"
          % (u["shared_types"], u["extern_symbols"], u["types"], u["unk_fields"], u["fn_names"],
             u["unk_identifiers"], u["label_names"], u["address_names"], u["mangled_names"]))
    print("%d finding(s) over %d unit(s), %d file(s) with findings"
          % (b["findings"], len(b["units"]), len(source_files_of(findings))))
    if rule10_note:
        print(rule10_note + " - the r10 column; plan 6.5 rule 10 is vtableaudit's, the codegen-pragma check is r14")
    r11 = [f for f in findings if f["rule"] == 11]
    print("rule 11 (banned outright): %d finding(s) over %d file(s) with a `void *` parameter/return type"
          % (len(r11), len(source_files_of(r11))))
    if root is not None:
        print("rule 11 note: %d `void *` local variable(s) - out of the rule's scope, counted so the owner "
              "can decide" % rule11_local_total(root))
    r13 = [f for f in findings if f["rule"] == 13]
    print("rule 13 (a method is a member): %d finding(s) over %d file(s), %d distinct function name(s)"
          % (len(r13), len(source_files_of(r13)), len({f["token"] for f in r13})))
    print("rule 13 static form (`<Type>_<name>` with no `Type* self`, a static member): %d finding(s) over %d "
          "file(s), %d distinct name(s)" % (len([f for f in r13 if f.get("static")]),
                                            len(source_files_of([f for f in r13 if f.get("static")])),
                                            len({f["token"] for f in r13 if f.get("static")})))
    r15 = rule15_classes(findings)
    print("rule 15 (comments): %d refusing (%s) + %d advisory (%s) - an advisory finding is counted here and never "
          "refused" % (sum(r15["refusing"].values()), ", ".join("%s %d" % kv for kv in r15["refusing"].items()),
                       sum(r15["advisory"].values()), ", ".join("%s %d" % kv for kv in r15["advisory"].items())))
    print_rule2_report(ownership)
    for num, what in UNCHECKED:
        print("not checked (cross-file): rule %d - %s" % (num, what))
    for rule, prefix, why in EXEMPT:
        print("not enforced: rule %d under %s (%s)" % (rule, prefix, why))
    for cond, why in RULE7_NOTES:
        print("not enforced: rule 7 for %s (%s)" % (cond, why))


def source_files_of(findings: list[dict]) -> set[str]:
    return {f["file"] for f in findings}


def select_findings(findings: list[dict], paths: "list[str] | None" = None,
                    rules: "list[int] | None" = None) -> list[dict]:
    """The `--findings` filter: a finding whose file matches any `--path` glob (`fnmatch`, `/` separators; a
    glob with no wildcard is a directory or file prefix) and whose rule is any `--rule`; no filter keeps all.
    Sorted by (file, line, rule) so a listing reads top-down per file."""
    import fnmatch

    def path_ok(rel: str) -> bool:
        if not paths:
            return True
        for glob in paths:
            glob = glob.replace("\\", "/")
            if any(c in glob for c in "*?["):
                if fnmatch.fnmatchcase(rel, glob):
                    return True
            elif rel == glob or rel.startswith(glob.rstrip("/") + "/"):
                return True
        return False

    out = [f for f in findings if path_ok(f["file"]) and (not rules or f["rule"] in rules)]
    return sorted(out, key=lambda f: (f["file"], f["line"], f["rule"]))


def print_findings_listing(rows: list[dict], scope: str, as_json: bool, paths=None, rules=None) -> None:
    """`--findings`: one `file:line  rule N  token` line per finding, or the `lib.findings` JSON schema
    (`{tool, rows, ok, summary}` + `scope`/`filters`/`by_rule`), each row a `Finding.to_dict()`."""
    by_rule = dict(sorted(collections.Counter(f["rule"] for f in rows).items()))
    if as_json:
        verdict = _findings.Verdict.of(_findings.Finding.from_dict(f) for f in rows)
        print(_findings.render_json("stylelint", verdict, scope=scope,
                                    filters={"path": list(paths or []), "rule": list(rules or [])},
                                    by_rule={str(k): v for k, v in by_rule.items()}))
        return
    for f in rows:
        print("%s:%d  rule %d  %s" % (f["file"], f["line"], f["rule"], f.get("token") or "-"))
    print("stylelint: %d finding(s) in the %s set%s" % (
        len(rows), scope, (" (" + ", ".join("rule %d: %d" % kv for kv in by_rule.items()) + ")") if rows else ""))


def print_findings(findings: list[dict]) -> None:
    for f in findings:
        print("%s:%d: rule %d: %s [%s]" % (f["file"], f["line"], f["rule"], f["detail"], f["text"]))
    for num, what in UNCHECKED:
        print("not checked (cross-file): rule %d - %s" % (num, what))
    for rule, prefix, why in EXEMPT:
        print("not enforced: rule %d under %s (%s)" % (rule, prefix, why))
    for cond, why in RULE7_NOTES:
        print("not enforced: rule 7 for %s (%s)" % (cond, why))
