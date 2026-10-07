"""A function's leading plain declarations and the orders to try them in.
Spec: docs/tools/spec/lib-declperm.md. CLI: none (library)."""
from __future__ import annotations

import itertools
import math
import random
import re
from dataclasses import dataclass

#: a declaration with an initialiser (`    int x = f();`): skipped, never permuted
_INITIALISED = re.compile(r"^    [A-Za-z_][^;]*=[^;]*;$")
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

    @property
    def block(self) -> str:
        return "".join(line + "\n" for line in self.lines)


def _definition(text: str, function: str) -> list[re.Match]:
    pat = r'(?m)^(?:extern "C" )?[A-Za-z][^\n;]*\b%s\([^\n;]*\)\s*(?:const\s*)?\{\n' % re.escape(function)
    return list(re.finditer(pat, text))


def find_run(text: str, function: str, max_lines: int = DEFAULT_MAX_LINES) -> Run | None:
    """The first `max_lines` plain declarations after the function's opening brace (skipping any initialised
    declarations that lead), or `None` when fewer than two exist. A name defined twice is refused (`ValueError`):
    the run would be a guess."""
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

    while True:
        line, nxt = line_at(pos)
        if _INITIALISED.match(line):
            pos = nxt
        else:
            break
    start = pos
    lines: list[str] = []
    while len(lines) < max_lines:
        line, nxt = line_at(pos)
        if _PLAIN.match(line) and "return" not in line:
            lines.append(line)
            pos = nxt
        else:
            break
    if len(lines) < 2:
        return None
    return Run(function, start, pos, tuple(lines))


def orders(n: int, cap: int, seed: int = 0) -> tuple[list[tuple[int, ...]], bool]:
    """`(orders, sampled)`: every order of `range(n)` but the identity when there are at most `cap` of them, else
    `cap` distinct ones drawn with a fixed `seed` (never the identity), so a rerun tries the same set."""
    total = math.factorial(n) - 1
    identity = tuple(range(n))
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
