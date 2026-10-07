"""A function's leading local declarations (plain or initialised) and the orders to try them in.
Spec: docs/tools/spec/lib-declperm.md. CLI: none (library)."""
from __future__ import annotations

import itertools
import math
import random
import re
from dataclasses import dataclass

#: a declaration with an initialiser (`    int x = f();`): skipped by the plain-only reading, a unit of the run otherwise
_INITIALISED = re.compile(r"^    [A-Za-z_][^;]*=[^;]*;$")
#: an initialised declaration as one statement: `    Type [*]name[N] = expr;` (group 1 the name, group 3 the initialiser)
_INIT_DECL = re.compile(r"^    [A-Za-z_][\w:]*(?: [\w:]+)*[\s\*]+(\w+)(\[[^\]]*\])?\s*=\s*([^;]*);$")
#: a leading word that makes a line a statement, not a declaration
_STATEMENT_WORDS = frozenset(("return", "else", "goto", "case", "break", "continue", "do", "delete", "new", "throw",
                              "typedef", "using"))
_IDENT = re.compile(r"[A-Za-z_]\w*")
#: a plain declaration: `    Type name;`, `    Type *name;`, `    u8 buf[4];`
_PLAIN = re.compile(r"^    [A-Za-z_][\w:]*(?: [\w:]+)*[\s\*]+\w+(\[[^\]]*\])?;$")
#: the default length of the run: more than this many leading lines are cut to the first ones
DEFAULT_MAX_LINES = 6


@dataclass(frozen=True)
class Run:
    """The plain declarations of one function, as they sit in the (LF) source: `text[start:end]` is the lines."""
    function: str
    start: int
    end: int
    lines: tuple[str, ...]
    #: `deps[i]`: the earlier lines whose declared name line `i`'s initialiser uses (it cannot move before them)
    deps: tuple[tuple[int, ...], ...] = ()

    @property
    def block(self) -> str:
        return "".join(line + "\n" for line in self.lines)


def _definition(text: str, function: str) -> list[re.Match]:
    pat = r'(?m)^(?:extern "C" )?[A-Za-z][^\n;]*\b%s\([^\n;]*\)\s*(?:const\s*)?\{\n' % re.escape(function)
    return list(re.finditer(pat, text))


def declared(line: str) -> tuple[str, str] | None:
    """`(name, initialiser text)` of a single-line local declaration (`""` when plain), else `None`."""
    word = line.split(None, 1)[0] if line.strip() else ""
    if word in _STATEMENT_WORDS or "return" in line:
        return None
    m = _INIT_DECL.match(line)
    if m:
        return m.group(1), m.group(3)
    if _PLAIN.match(line):
        return re.search(r"(\w+)(?:\[[^\]]*\])?;$", line).group(1), ""
    return None


def dependencies(lines: tuple[str, ...]) -> tuple[tuple[int, ...], ...]:
    """Per line, the indices of the earlier lines whose declared name its initialiser mentions."""
    decls = [declared(l) for l in lines]
    out = []
    for i, d in enumerate(decls):
        used = set(_IDENT.findall(d[1])) if d else set()
        out.append(tuple(j for j in range(i) if decls[j] and decls[j][0] in used))
    return tuple(out)


def find_run(text: str, function: str, max_lines: int = DEFAULT_MAX_LINES, initialised: bool = True) -> Run | None:
    """The first `max_lines` local declarations after the function's opening brace, or `None` when fewer than two
    exist. With `initialised` a declaration with an initialiser is a member (its dependencies on earlier members
    are recorded in `Run.deps`); without, only plain declarations count and leading initialised ones are skipped.
    A name defined twice is refused (`ValueError`): the run would be a guess."""
    found = _definition(text, function)
    if len(found) > 1:
        raise ValueError("%s has %d definitions in the source: cannot tell which run to permute"
                         % (function, len(found)))
    if not found:
        return None
    pos = found[0].end()

    def line_at(p: int) -> tuple[str, int]:
        e = text.find("\n", p)
        return (text[p:], len(text)) if e < 0 else (text[p:e], e + 1)

    while not initialised:
        line, nxt = line_at(pos)
        if _INITIALISED.match(line):
            pos = nxt
        else:
            break
    start = pos
    lines: list[str] = []
    while len(lines) < max_lines:
        line, nxt = line_at(pos)
        ok = declared(line) is not None if initialised else (_PLAIN.match(line) and "return" not in line)
        if ok:
            lines.append(line)
            pos = nxt
        else:
            break
    if len(lines) < 2:
        return None
    return Run(function, start, pos, tuple(lines), dependencies(tuple(lines)) if initialised else ())


def _topological(n: int, deps: tuple[tuple[int, ...], ...], limit: int) -> list[tuple[int, ...]]:
    """Up to `limit` orders of `range(n)` that keep every line after its dependencies, in lexicographic order."""
    out: list[tuple[int, ...]] = []

    def walk(prefix: list[int], placed: set[int]) -> None:
        if len(out) >= limit:
            return
        if len(prefix) == n:
            out.append(tuple(prefix))
            return
        for i in range(n):
            if i not in placed and all(d in placed for d in deps[i]):
                prefix.append(i)
                placed.add(i)
                walk(prefix, placed)
                placed.discard(i)
                prefix.pop()

    walk([], set())
    return out


def _random_topological(n: int, deps: tuple[tuple[int, ...], ...], rng: random.Random) -> tuple[int, ...]:
    placed: list[int] = []
    left = set(range(n))
    while left:
        ready = sorted(i for i in left if all(d in placed for d in deps[i]))
        pick = rng.choice(ready)
        placed.append(pick)
        left.discard(pick)
    return tuple(placed)


def orders(n: int, cap: int, seed: int = 0,
           deps: tuple[tuple[int, ...], ...] = ()) -> tuple[list[tuple[int, ...]], bool]:
    """`(orders, sampled)`: every order of `range(n)` but the identity when there are at most `cap` of them, else
    `cap` distinct ones drawn with a fixed `seed` (never the identity), so a rerun tries the same set. With `deps`
    (`Run.deps`) only orders that keep each line after the lines it depends on are produced."""
    identity = tuple(range(n))
    if any(deps):
        every = _topological(n, deps, cap + 2)
        if len(every) <= cap + 1:
            return [o for o in every if o != identity], False
        rng = random.Random(seed)
        chosen: set[tuple[int, ...]] = set()
        for _ in range(cap * 50):
            if len(chosen) >= cap:
                break
            o = _random_topological(n, deps, rng)
            if o != identity:
                chosen.add(o)
        return sorted(chosen), True
    total = math.factorial(n) - 1
    if total <= cap:
        return [p for p in itertools.permutations(range(n)) if p != identity], False
    rng = random.Random(seed)
    picked: set[tuple[int, ...]] = set()
    while len(picked) < cap:
        p = list(range(n))
        rng.shuffle(p)
        if tuple(p) != identity:
            picked.add(tuple(p))
    return sorted(picked), True


def name_of(order: tuple[int, ...]) -> str:
    return "order-" + ",".join(str(i) for i in order)


def reorder(text: str, run: Run, order: tuple[int, ...]) -> str | None:
    """`text` with the run's lines in `order`; `None` when the text no longer holds the run where it was found."""
    if text[run.start:run.end] != run.block:
        return None
    new = "".join(run.lines[i] + "\n" for i in order)
    return text[:run.start] + new + text[run.end:]


def variants(run: Run, ords: list[tuple[int, ...]]) -> list[tuple[str, object]]:
    """`[(name, rewrite)]` in `tryvar`'s variant shape; each rewrite is a callable so the run is replaced where
    it was found, never at the first place the same lines occur."""
    return [(name_of(o), (lambda text, o=o: reorder(text, run, o))) for o in ords]
