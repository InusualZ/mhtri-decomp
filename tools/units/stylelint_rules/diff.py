"""The `--diff`/`--ref` judgement: identities new to a file, rename and move credits, the added-finding detail.
Spec: docs/tools/spec/stylelint_rules.md. CLI: none (the stylelint package; `stylelint.py` is the CLI)."""
from __future__ import annotations

import os
from dataclasses import dataclass

from tools.lib import findings as _findings
from tools.units.stylelint_rules.common import Source
from tools.units.stylelint_rules.r02_extern import header_declarations
from tools.units.stylelint_rules.r15_comments import is_advisory


# --------------------------------------------------------------------------------------------------
# `--diff`: an owed rename is not ownership the batch created
# --------------------------------------------------------------------------------------------------

def unresolved_declarations(src: Source, ownership: "Ownership") -> set[str]:
    """The declaration names in `src` that the map cannot resolve - the file's rule-2 **gaps**.

    Rule 2 reports *nothing* for these (`rule2_findings` counts them in `Ownership.gaps` and refuses to
    guess at a name with no row), so a file's gap names are invisible in the findings list - and they are
    exactly the signal `--diff` needs.  A file that stops spelling an unmapped name has **completed** a
    rename the base map had already made (the old spelling was the referrer half); a file that only ever
    adds declarations gives up no gap at all.  That is what keeps a credit from excusing a new violation.
    """
    out: set[str] = set()
    for name, _line in header_declarations(src):
        resolved = ownership.resolve(name)
        if resolved is None or resolved["kind"] == "dup":
            out.add(name)
    return out


def owed_rename_completion(finding: dict, base: "Ownership | None", working: "Ownership") -> bool:
    """Whether this rule-2 finding is the **referrer half of a rename the base map already made**.

    The incident (2026-09-28).  `cb7d49aaa` renamed four map rows (`getGameSpyInterfaceThread` ->
    `GameSpyInterfaceThread_getInstance`, `clearPatInterface` -> `PatInterface_clear`,
    `isPatInterfaceReady` -> `PatInterface_isReady`, `lbl_80794CE4` -> `sGameSpyInterfaceThread`) and
    landed the map alone; the referrers kept spelling the old names.  Completing that rename is the *only*
    way the two units can ever link - but `--diff` judges each side by the map it was written against, and
    the base copy's *old* spelling resolves to nothing at all, so the corrected spelling read as an
    addition: `+1 rule 2 include/Network/fn_8041A87C.h (3 -> 4)`, `+3 ... NetworkWiiMediator.cpp (39 ->
    42)` - a pure artefact, the same class as the `+62` rename artefact `load_ownership_at_ref` fixed.

    The judgement is therefore by the **ownership of the address**, not by the spelling of the name: a
    working-side declaration whose symbol resolves to an address that the **base map already carried, with
    the same owner** is not an addition - it is the other half of a rename the base made.  Three things
    still refuse, and they are the reason this is a narrow rule rather than a blanket one:

    * the address has **no row in the base map** - the batch newly registered the range (or added the row),
      so the ownership *is* the batch's, which is exactly what `--diff` exists to charge;
    * the base row at that address is owned by a **different** unit (or is in a different module) - the
      declaration is foreign for a reason the base did not have;
    * the name resolves to nothing in the working map either (a genuine gap, `Ownership.gaps`) - there is no
      address to compare, so the finding stands.
    """
    if base is None:
        return False
    name = finding.get("symbol")
    if not name:
        return False
    working_r = working.resolve(name)
    if working_r is None or working_r["kind"] not in ("owned", "unsplit"):
        return False
    section, address = working_r.get("section"), working_r.get("address")
    if section is None or address is None:
        return False
    base_r = base.resolution_at(section, address)
    if base_r is None or base_r["kind"] != working_r["kind"]:
        return False
    if working_r["kind"] == "owned":
        return base_r.get("unit") == working_r.get("unit")
    return base_r.get("module") == working_r.get("module")


def rename_credits(findings: list[dict], base: "Ownership | None", working: "Ownership",
                   base_symbols: "dict[str, set] | None" = None,
                   freed: "dict[str, int] | None" = None) -> dict[tuple[int, str], int]:
    """`(rule, file) -> how many of that pair's findings are an owed rename's referrer half.

    Three conditions, each of which a genuinely new declaration fails:

    * `base_symbols` - the BASE copy's rule-2 symbols per file: a symbol the file already declared is part
      of `before`, not one of this batch's findings, so it can never be credited;
    * `owed_rename_completion` - the symbol must resolve to an address the base map owned, for the same
      owner (or the same unsplit module);
    * `freed` - how many unmapped names the file **stopped spelling** (`unresolved_declarations`): each
      credit costs one, so a file that adds a foreign declaration while giving up no gap is refused.
    """
    credits: dict[tuple[int, str], int] = {}
    if base is None:
        return credits
    known = base_symbols or {}
    for f in findings:
        if f.get("rule") != 2 or not owed_rename_completion(f, base, working):
            continue
        if f.get("symbol") in known.get(f["file"], ()):
            continue
        key = (2, f["file"])
        if credits.get(key, 0) >= (freed or {}).get(f["file"], 0):
            continue
        credits[key] = credits.get(key, 0) + 1
    return credits


def apply_rename_credits(added: list[dict], findings: list[dict], base: "Ownership | None",
                         working: "Ownership",
                         base_symbols: "dict[str, set] | None" = None,
                         freed: "dict[str, int] | None" = None) -> tuple[list[dict], dict]:
    """`(added, credits)`: subtract the owed-rename credits from `added`, dropping pairs that reach zero.

    Only rule 2 is credited (`rename_credits`), so no other rule's growth is ever excused; the credit can
    never exceed what the count comparison reported as added (a subtraction from a positive delta); and a
    pair is kept, with its reduced count, as soon as one genuine finding remains - a file that completes an
    owed rename *and* adds a foreign declaration still refuses, and the refusal names the file.
    """
    credits = rename_credits(findings, base, working, base_symbols, freed)
    if not credits:
        return added, {}
    out, used = [], {}
    for a in added:
        key = (a["rule"], a["file"])
        c = credits.get(key, 0)
        if c:
            # report only what was actually subtracted: a file whose credit found no addition to cancel
            # (its count fell, or never rose) must not appear in the credit lines
            used[key] = min(c, a["added"])
        left = a["added"] - c
        if left > 0:
            out.append(dict(a, added=left))
    return out, used


def rename_credit_lines(credits: dict[tuple[int, str], int],
                        freed: "dict[str, set] | None" = None) -> list[str]:
    """The credited findings as printable lines - a credit is **said**, never a silent tolerance.

    The line names the unmapped names the file stopped spelling, so a reader can check the judgement
    instead of trusting it: those names are the base's half of the rename.
    """
    lines = []
    for (rule, path), n in sorted(credits.items(), key=lambda kv: (kv[0][1], kv[0][0])):
        gave_up = sorted((freed or {}).get(path, ()))[:3]
        more = ""
        gone = ""
        if gave_up:
            gone = "; stopped spelling %s%s" % (", ".join(gave_up),
                                                 " (+%d more)" % (len((freed or {})[path]) - 3)
                                                 if len((freed or {})[path]) > 3 else "")
        lines.append("  ~%d rule %d  %s  (completing a rename the base map already made: same address, "
                     "same owner at base%s)" % (n, rule, path, gone))
    return lines


def rule_counts(findings: list[dict]) -> dict[tuple[int, str], int]:
    out: dict[tuple[int, str], int] = {}
    for f in findings:
        out[(f["rule"], f["file"])] = out.get((f["rule"], f["file"]), 0) + 1
    return out


def diff_deltas(before: dict[tuple[int, str], int], after: dict[tuple[int, str], int]) -> list[dict]:
    """The (rule, file) pairs whose count rose, with the delta. Pure function - the `--diff` core."""
    added = []
    for key in sorted(set(before) | set(after)):
        delta = after.get(key, 0) - before.get(key, 0)
        if delta > 0:
            added.append({"rule": key[0], "file": key[1], "added": delta,
                          "before": before.get(key, 0), "after": after.get(key, 0)})
    return added


def finding_identity(f: dict) -> tuple:
    """A line-independent identity for a finding: rule, file, at-fault token and detail text
    (`lib.findings.identity`; the credit model is `lib.findings.added`)."""
    return _findings.identity(f)


def _covering_range(own: "Ownership", section: str, address: int) -> "tuple | None":
    """The `(start, end)` of the registered range covering `address` in `section`, or None.

    The range, not the unit path, is what tells one ownership from another across a rename: a batch that
    renames a unit file (`menu/fn_802A6624.cpp` -> `menu/menu_message.cpp`) keeps every boundary, so an
    address's ownership is unchanged even though the unit the map spells changed.
    """
    hit = own.covering(section, address)
    return (hit[0], hit[1]) if hit else None


def rename_map(base: "Ownership | None", after: "Ownership | None",
               names: "set[str] | None" = None) -> dict:
    """`{name_at_base: name_now}` for the map rows whose **address** is unchanged and whose **name** changed.

    The symbol sibling of `renames_of` (which maps a *path* at the ref to the path now): a batch that
    renames a `symbols.txt` row keeps the address, so the two sides must read as *the same symbol,
    renamed*, never as one removal plus one addition.  This is the same address-is-the-identity view
    `owed_rename_completion` uses for its credit, and the same `+62 rule-2 violations for a pure rename`
    artefact that `load_ownership_at_ref` fixed on the map side - the token comparison needs it too.

    A pair is admitted only when both maps resolve the shared address to the **same** kind and the same
    ownership - the same covering `(start, end)` for an owned range (the unit *path* may itself have been
    renamed) or the same unsplit module - so a range the batch re-registered to a different unit, or
    moved, is not read as a rename of the old row.  `names` bounds the walk to the tokens the caller
    actually compares (a `--diff` supplies its base side's tokens) instead of the whole 4.5 MB map.
    """
    out: dict[str, str] = {}
    if base is None or after is None:
        return out
    for name in (names if names is not None else list(base.symbols)):
        entries = base.symbols.get(name)
        if not entries or len(entries) != 1:
            continue
        section, address, _type = entries[0]
        now = after.name_at(section, address)
        if not now or now == name:
            continue
        was, is_ = base.resolution_at(section, address), after.resolution_at(section, address)
        if not was or not is_ or was.get("kind") != is_.get("kind"):
            continue
        if was.get("kind") == "owned":
            if _covering_range(base, section, address) != _covering_range(after, section, address):
                continue
        elif was.get("module") != is_.get("module"):
            continue
        out[name] = now
    return out


def renamed_finding(f: dict, symbols: dict, files: "dict | None" = None) -> dict:
    """`f` with its token - and the owner path its detail spells - translated: the same token, renamed.

    Two translations, because a rename changes two things a base-side finding names.  `symbols`
    (`rename_map`) renames the **token** and its backticked occurrence in the prose, so a renamed
    declaration presents the identity the working side does.  `files` (`renames_of`) renames any
    **path** in the prose: a rule-2 finding's detail names the *owner unit* ("`x` is owned by
    `src/menu/fn_802A6624.cpp`"), and when the batch re-homes that unit too the same finding would look
    like a new one for no reason but the unit's new name.

    The map holds only map-row names, so a finding whose token is not a renamed row comes back unchanged.
    """
    tok = f.get("token")
    detail = f["detail"]
    token2 = tok
    if tok and tok in (symbols or {}):
        token2 = symbols[tok]
        detail = detail.replace("`%s`" % tok, "`%s`" % token2)
    for old, new in (files or {}).items():
        if old != new:
            detail = detail.replace(old, new)
    if token2 == tok and detail == f["detail"]:
        return f
    return dict(f, token=token2, detail=detail)


def added_identities(before_findings: list[dict], after_findings: list[dict],
                     symbols: "dict | None" = None, files: "dict | None" = None) -> dict:
    """`{(rule, file): [finding, ...]}` - one entry per after-side identity **new to that file**
    (`lib.findings.added`), each base identity admitted under every spelling a rename gives it
    (`renamed_finding`, driven by the symbol map and the file map)."""
    return _findings.added(before_findings, after_findings,
                           lambda f: (renamed_finding(f, symbols or {}, files),))


def removed_identities(before_findings: list[dict], after_findings: list[dict],
                       symbols: "dict | None" = None, files: "dict | None" = None) -> dict:
    """`{(rule, token, detail): [file, ...]}` - one entry per base identity a file **stopped** carrying under any
    spelling a rename gives it (`lib.findings.removed`); the mirror of `added_identities`."""
    return _findings.removed(before_findings, after_findings,
                             lambda f: (renamed_finding(f, symbols or {}, files),))


def _unit_stem(path: str) -> "str | None":
    """`src/enemy/fn_x.cpp` -> `enemy/fn_x` (the unit name `splits.txt` uses); None outside `src/`."""
    return os.path.splitext(path[4:])[0] if path.startswith("src/") else None


def file_absorbers(absorbs: dict, before_paths: "list[str]", after_paths: "list[str]") -> dict:
    """`{F: [G, ...]}` - for each source file F, the files whose unit received F's bytes. Pure.

    `absorbs` is `datagap.derive_absorption`'s `{NEW unit: [OLD unit, ...]}` (an OLD's base byte range now claimed by
    NEW: a fold, a shrunk unit's moved part, a rename).  `before_paths` are the files that existed at the base
    (deleted, renamed or edited), `after_paths` the files the batch leaves; units map to files by stem.
    """
    by_old: dict = {}
    for path in before_paths:
        stem = _unit_stem(path)
        if stem:
            by_old[stem] = path
    by_new: dict = {}
    for path in after_paths:
        stem = _unit_stem(path)
        if stem:
            by_new[stem] = path
    out: dict = {}
    for new, olds in absorbs.items():
        for old in olds:
            if old in by_old and new in by_new and by_old[old] != by_new[new]:
                out.setdefault(by_old[old], []).append(by_new[new])
    return {f: sorted(set(gs)) for f, gs in out.items()}


def derive_file_absorbers(root: str, base: str, ref: "str | None", pairs: list, deleted: "list[str]") -> dict:
    """The absorber map for this comparison: the datagap fold map over `splits.txt` at `base` vs `ref` (or the tree).

    Evidence only (address ranges), never a name guess; an unreadable map yields no absorbers, so the credit degrades
    to the plain one-per-removal rule.
    """
    try:
        from tools.units import dataclosure as dg  # noqa: PLC0415 - the fold map is the data closure's
        base_table = dg.unit_claim_table(dg.splits_at_ref(root, base))
        now_table = dg.unit_claim_table(dg.splits_at_ref(root, ref) if ref else dg.load_claims(root))
        absorbs = dg.derive_absorption(base_table, now_table)
    except Exception:                                                        # noqa: BLE001 - evidence only
        return {}
    before_paths = [b for b, _a in pairs if b] + list(deleted)
    return file_absorbers(absorbs, before_paths, [a for _b, a in pairs])


def apply_move_credits(fresh: dict, before_findings: list[dict], after_findings: list[dict],
                       symbols: "dict | None" = None, files: "dict | None" = None,
                       absorbers: "dict | None" = None) -> tuple[dict, list[dict]]:
    """Credit an added identity that another file of the same batch **gave up**: a move, not growth.

    `--diff` grandfathers per file, so functions moved from an old file into a new one read as additions
    in the new file although the old file lost the same findings.  Here every identity new to a file
    (`fresh`, `added_identities`' shape) is matched against the identities other files lost
    (`removed_identities`), keyed `(rule, token, detail)` - the same rule and the same at-fault token, never
    a look-alike of another rule.  A removal is keyed by its **renamed** spelling (`lib.findings.removed` through
    `renamed_finding`), so a finding that moved file while the batch renamed its map row is still a move.
    **One credit per removal** (a multiset match): a copy that leaves the
    original intact removes nothing and earns nothing, and a name that grew across the batch (two files
    gained it, one lost it) still leaves the surplus refused.  Returns `(fresh_left, moves)`, each move
    `{rule, token, detail, from, to}`; a batch without a move returns `fresh` unchanged and `[]`.
    """
    removed = removed_identities(before_findings, after_findings, symbols, files)
    pool = {k: list(v) for k, v in removed.items()}
    left: dict = {}
    moves: list[dict] = []
    # SPLIT CREDIT (owner's delegate, 2026-09-30): a source file that lost an identity (deleted, or it stopped
    # carrying it) whose bytes were absorbed by N files (`absorbers`, the derived fold map) earns up to N credits
    # for it - one per absorber that newly carries it - because the one base identity is now legitimately needed
    # in each. Only real absorbers qualify, a rule-1 duplicate never does, and the origin's own removal is
    # consumed once, so no arbitrary file can also draw on it.
    used_pairs: set = set()
    split_done: set = set()                 # (key, id(finding)): credited by the absorber pass
    for key in sorted(fresh):               # absorber pass FIRST, so no arbitrary file draws on an origin before them
        rule, file = key
        if not absorbers or rule == 1:
            continue
        for f in fresh[key]:
            ident = (rule, f.get("token"), f["detail"])
            for cand in removed.get(ident, ()):
                if file in absorbers.get(cand, ()) and (cand, file, ident) not in used_pairs:
                    used_pairs.add((cand, file, ident))
                    if cand in pool.get(ident, []):
                        pool[ident].remove(cand)
                    moves.append({"rule": rule, "token": f.get("token"), "detail": f["detail"],
                                  "from": cand, "to": file, "split": True})
                    split_done.add((key, id(f)))
                    break
    for key in sorted(fresh):
        rule, file = key
        keep = []
        for f in fresh[key]:
            if (key, id(f)) in split_done:
                continue
            ident = (rule, f.get("token"), f["detail"])
            src = pool.get(ident)
            if src:
                origin = src.pop(0)
                moves.append({"rule": rule, "token": f.get("token"), "detail": f["detail"],
                              "from": origin, "to": file})
            else:
                keep.append(f)
        if keep:
            left[key] = keep
    return left, moves


def move_credit_lines(moves: list[dict]) -> list[str]:
    """`--diff`'s report of what it credited as moved: never silent, names the old and the new file."""
    out = []
    if moves:
        plain = [m for m in moves if not m.get("split")]
        splits: dict = {}
        for m in moves:
            if m.get("split"):
                splits.setdefault((m["rule"], m["token"] or m["detail"], m["from"]), []).append(m["to"])
        out.append("  moved (credited, one per removal from another file of the batch): %d finding(s)"
                   % len(moves))
        for m in plain:
            out.append("    moved rule %d %s: %s -> %s" % (m["rule"], m["token"] or m["detail"], m["from"], m["to"]))
        for (rule, token, origin), dests in sorted(splits.items()):
            if len(dests) == 1:
                out.append("    moved rule %d %s: %s -> %s" % (rule, token, origin, dests[0]))
                continue
            out.append("    moved (split across %d absorbers) rule %d: %s: %s -> %s"
                       % (len(dests), rule, token, origin, ", ".join(sorted(dests))))
    return out


def added_rows(fresh: dict, before_counts: dict, after_counts: dict) -> list[dict]:
    """The `+N rule R <file> (before -> after)` rows the message prints, from `added_identities`.

    `N` is the number of identities new to the file (what counts as an addition), not the count delta:
    a file that spells an already-flagged name 30 more times has no row at all.  `before`/`after` stay the
    (rule, file) counts, so the row still says how the file's debt moved while `N` says what is new.
    """
    out = []
    for key in sorted(fresh):
        rule, file = key
        out.append({"rule": rule, "file": file, "added": len(fresh[key]),
                    "before": before_counts.get(key, 0), "after": after_counts.get(key, 0)})
    return out


def added_finding_detail(added: list[dict], after_findings: list[dict], before_findings: list[dict],
                         symbols: "dict | None" = None, files: "dict | None" = None,
                         fresh: "dict | None" = None) -> list[dict]:
    """The findings behind `added`'s `+N rule R <file>` rows, named: rule, file, line and token.

    `added` is `added_rows`' new-identity rows, so by itself it says how many new tokens a file gained but
    not which.  Re-deriving `added_identities` (with the same symbol/file rename maps) names each one - the
    first occurrence of each new identity by line - and the list is capped at `row["added"]` because a
    rename credit can reduce a row below what the identity comparison finds.

    The returned rows carry only what a reader needs - the file and line that locate the occurrence and the
    token that names it - so the `--diff --json` `detail` key is stable whatever the internal finding adds.
    """
    if fresh is None:
        fresh = added_identities(before_findings, after_findings, symbols, files)
    out = []
    for row in sorted(added, key=lambda a: (a["rule"], -a["added"], a["file"])):
        key = (row["rule"], row["file"])
        for f in fresh.get(key, ())[:row["added"]]:
            out.append({"rule": f["rule"], "file": f["file"], "line": f["line"],
                        "token": f.get("token"), "detail": f["detail"]})
    return out


def added_detail_lines(detail: list[dict]) -> list[str]:
    """`--list-added`'s report: grouped by rule, then by file with its count, biggest offender first.

    The per-file count is the weight `backlog.py` ranks by, so the file that grew the most surfaces first;
    each finding is then named the way a lane acts on it - `rule R <file>:<line> <token>` - so a `+N rule R`
    summary can be read down to the identifiers behind it without re-deriving them from the diff.
    """
    lines = []
    for rule in sorted({d["rule"] for d in detail}):
        by_file: dict[str, list[dict]] = {}
        for d in detail:
            if d["rule"] == rule:
                by_file.setdefault(d["file"], []).append(d)
        lines.append("rule %d:" % rule)
        for path in sorted(by_file, key=lambda p: (-len(by_file[p]), p)):
            lines.append("  %s (%d)" % (path, len(by_file[path])))
            for d in by_file[path]:
                token = (" %s" % d["token"]) if d.get("token") else ""
                lines.append("    rule %d %s:%d%s" % (d["rule"], d["file"], d["line"], token))
    return lines


@dataclass(frozen=True)
class Judgement:
    """What one comparison found: the `added_rows` left after both credits, the rename credits per (rule, file), the
    moves, the named detail of the added rows, the printable credit lines, and the gaps each file gave up."""
    added: list
    credits: dict
    moves: list
    detail: list
    credit_lines: list
    freed_gaps: dict


def base_rule2_symbols(base_findings: list[dict]) -> dict[str, set]:
    """The base copies' rule-2 symbols per file: a rename credit can only ever touch a name *new* to the file - one it
    already declared is part of `before`, never one of the batch's additions."""
    out: dict[str, set] = {}
    for f in base_findings:
        if f.get("rule") == 2 and f.get("symbol"):
            out.setdefault(f["file"], set()).add(f["symbol"])
    return out


def judge(before_findings: list[dict], after_findings: list[dict], touched: list[dict], base_findings: list[dict],
          base_ownership: "Ownership | None", after_ownership: "Ownership | None", base_gaps: dict[str, set],
          after_sources: list, rename: dict, absorbers: dict) -> Judgement:
    """The one judgement `--diff` and `--ref` share, from both sides' findings.

    `touched` is the after side of the changed files (the only findings a credit may consider), `base_findings` their
    base copies, `base_gaps` the unmapped names the base copies spelled (`unresolved_declarations_at_ref`) and
    `after_sources` the changed files now. A declaration the base spelled under a name the batch renamed is the
    *same* declaration: `rename_map` (symbols) and `rename` (files) let `added_identities` admit a base identity
    under both spellings, so a rename reads as a rename, never as removal plus addition.

    A rule-15 **advisory** finding (`advisory: True`: a narrative marker, a date, a percentage, a self-name) is
    dropped from every side first: it is a `--budget` count, never an addition, a move or a credit.
    """
    before_findings, after_findings, touched, base_findings = (
        [f for f in fs if not is_advisory(f)] for fs in (before_findings, after_findings, touched, base_findings))
    before, after = rule_counts(before_findings), rule_counts(after_findings)
    # the gaps each file gave up: the old spelling of a renamed row is an *unmapped* name, so a file that completed a
    # rename has strictly fewer of them; one credit costs one freed gap (`rename_credits`)
    freed = {src.rel: base_gaps.get(src.rel, set()) - unresolved_declarations(src, after_ownership)
             for src in after_sources}
    symbol_rename = rename_map(base_ownership, after_ownership,
                               {f.get("token") for f in before_findings if f.get("token")})
    fresh, moves = apply_move_credits(
        added_identities(before_findings, after_findings, symbol_rename, rename),
        before_findings, after_findings, symbol_rename, rename, absorbers=absorbers)
    added, credits = apply_rename_credits(
        added_rows(fresh, before, after), touched, base_ownership, after_ownership,
        base_rule2_symbols(base_findings), {p: len(names) for p, names in freed.items()})
    return Judgement(added, credits, moves,
                     added_finding_detail(added, after_findings, before_findings, symbol_rename, rename, fresh),
                     rename_credit_lines(credits, freed) + move_credit_lines(moves), freed)


def merge_counts(*counts: dict) -> dict:
    """Sum several `rule_counts` dicts (each `{(rule, file): n}`) into one."""
    out: dict = {}
    for c in counts:
        for key, n in c.items():
            out[key] = out.get(key, 0) + n
    return out


def renames_of(pairs: list[tuple[str | None, str]]) -> dict:
    """`{path_at_ref: path_now}` for the renames in a changed-pairs list (`changed_src_files`).

    A rename is one pair carrying both names, so a walk that reads the *ref* tree can key its findings by
    the path the working tree now spells - the rename-stable key `--diff` compares against.
    """
    return {before: after for before, after in pairs if before and before != after}
