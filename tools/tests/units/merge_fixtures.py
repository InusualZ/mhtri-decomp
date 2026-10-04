"""Shared fixtures of the merge-family tests: the two real prose conflicts, the duplicated-prose signature, the
pre-fix union (to pin the defect), and a hermetic git environment for the tools' own git calls."""
from __future__ import annotations

import os

# The real region from `include/unsplit/lobby.h` at the recorded revisions (base `1fb32e620`, main `093017eaf`,
# branch `aa1c85431`) - the merge `d21bc02d5` on `worker/menu-num-8725`, where both sides rewrote the same
# comment paragraph and the old append-union duplicated it mid-sentence.
REAL_LOBBY_CONFLICT = (
    "<<<<<<< ours\n"
    "/* `fn_802A7C04`/`fn_802A8EC0`/`fn_802A8ED8`/`menu_cursor_step`/`fn_802A8F50` (0x802A7C04-0x802A8F50) were\n"
    " * declared here while the menu band had no registered unit.  `menu/fn_802A6624.cpp` owns that range\n"
    " * now, so its header `include/menu/menu_message.h` declares them and this header includes it (rule 2).\n"
    "||||||| base\n"
    "/* `fn_802A7C04`/`fn_802A8EC0`/`fn_802A8ED8`/`menu_cursor_step`/`fn_802A8F50` (0x802A7C04-0x802A8F50) were\n"
    " * declared here while the menu band had no registered unit.  `menu/fn_802A6624.cpp` owns that range\n"
    " * now, so its header `include/menu/fn_802A6624.h` declares them and this header includes it (rule 2).\n"
    "=======\n"
    "/* `menu_hold_row_draw_by_lsp`/`menu_cursor_step_fixed_tail`/`menu_cursor_step_open_last`/`menu_cursor_step`/`toggle_word_step` (0x802A7C04-0x802A8F50) were\n"
    " * declared here while the menu band had no registered unit.  `menu/menu_message.cpp` owns that range\n"
    " * now, so its header `include/menu/menu_message.h` declares them and this header includes it (rule 2).\n"
    ">>>>>>> theirs\n"
    " * mismatch is the `(10505) illegal overloading` this move clears. */\n")
REAL_LOBBY_SUPERSET = (
    "/* `menu_hold_row_draw_by_lsp`/`menu_cursor_step_fixed_tail`/`menu_cursor_step_open_last`/`menu_cursor_step`/`toggle_word_step` (0x802A7C04-0x802A8F50) were\n"
    " * declared here while the menu band had no registered unit.  `menu/menu_message.cpp` owns that range\n"
    " * now, so its header `include/menu/menu_message.h` declares them and this header includes it (rule 2).\n"
    " * mismatch is the `(10505) illegal overloading` this move clears. */\n")

# The old generated names a union reintroduces (the branch's rename sweep removed them).
OLD_GENERATED_NAMES = ("fn_802A7C04", "fn_802A8EC0", "fn_802A8ED8", "fn_802A8F50")
OLD_FILES = ("menu/fn_802A6624.cpp", "include/menu/fn_802A6624.h")

# The second real conflict of that merge (`include/lobby/fn_801F3294.h`): its hunk opens mid-comment (the
# paragraph's `/*` is above the hunk), which is why the classifier carries the block state in.
REAL_FN_CONFLICT = (
    "/* `menu_cursor_step` (0x802A8EFC) stood here as `s32 (s16, s16, u16, s32, s32)` - the call site's narrow\n"
    " * view - while its band had no registered unit, and the two spellings could not both be visible\n"
    "<<<<<<< ours\n"
    " * ((10505) illegal overloading).  `menu/fn_802A6624.cpp` owns the address and its header\n"
    " * `include/menu/menu_message.h` (included below) declares the definition's `s32`/`u16` spelling\n"
    "||||||| base\n"
    " * ((10505) illegal overloading).  `menu/fn_802A6624.cpp` owns the address and its header\n"
    " * `include/menu/fn_802A6624.h` (included below) declares the definition's `s32`/`u16` spelling\n"
    "=======\n"
    " * ((10505) illegal overloading).  `menu/menu_message.cpp` owns the address and its header\n"
    " * `include/menu/menu_message.h` (included below) declares the definition's `s32`/`u16` spelling\n"
    ">>>>>>> theirs\n"
    " * (docs/plan.md 6.5 rule 2). */\n")
REAL_FN_SUPERSET = (
    "/* `menu_cursor_step` (0x802A8EFC) stood here as `s32 (s16, s16, u16, s32, s32)` - the call site's narrow\n"
    " * view - while its band had no registered unit, and the two spellings could not both be visible\n"
    " * ((10505) illegal overloading).  `menu/menu_message.cpp` owns the address and its header\n"
    " * `include/menu/menu_message.h` (included below) declares the definition's `s32`/`u16` spelling\n"
    " * (docs/plan.md 6.5 rule 2). */\n")

# The same two files as whole blobs at the three revisions, for the `git merge-file` / `resolve` paths.
REAL_LOBBY_BASE = (
    "/* `fn_802A7C04`/`fn_802A8EC0`/`fn_802A8ED8`/`menu_cursor_step`/`fn_802A8F50` (0x802A7C04-0x802A8F50) were\n"
    " * declared here while the menu band had no registered unit.  `menu/fn_802A6624.cpp` owns that range\n"
    " * now, so its header `include/menu/fn_802A6624.h` declares them and this header includes it (rule 2).\n"
    " * They stood here with `s16` returns and `void*`/`s32` tails while the owner defines `s32` - that\n"
    " * mismatch is the `(10505) illegal overloading` this move clears. */\n"
    "\n"
    "/* `GetMenuFontColor` (0x802AA3EC, the map's `GetMenuFontColor__Fbbbb`) was declared here while the\n"
    " * menu band had no registered unit; `menu/fn_802A6624.cpp` owns the address now and the owner's header\n"
    " * `include/menu/fn_802A6624.h`, included above, declares it with this same spelling (rule 2). */\n")
REAL_LOBBY_MAIN = REAL_LOBBY_BASE.replace("include/menu/fn_802A6624.h", "include/menu/menu_message.h")
REAL_LOBBY_BRANCH = (
    "/* `menu_hold_row_draw_by_lsp`/`menu_cursor_step_fixed_tail`/`menu_cursor_step_open_last`/`menu_cursor_step`/`toggle_word_step` (0x802A7C04-0x802A8F50) were\n"
    " * declared here while the menu band had no registered unit.  `menu/menu_message.cpp` owns that range\n"
    " * now, so its header `include/menu/menu_message.h` declares them and this header includes it (rule 2).\n"
    " * They stood here with `s16` returns and `void*`/`s32` tails while the owner defines `s32` - that\n"
    " * mismatch is the `(10505) illegal overloading` this move clears. */\n"
    "\n"
    "/* `GetMenuFontColor` (0x802AA3EC, the map's `GetMenuFontColor__Fbbbb`) was declared here while the\n"
    " * menu band had no registered unit; `menu/menu_message.cpp` owns the address now and the owner's header\n"
    " * `include/menu/menu_message.h`, included above, declares it with this same spelling (rule 2). */\n")
REAL_FN_BASE = (
    "/* `menu_cursor_step` (0x802A8EFC) stood here as `s32 (s16, s16, u16, s32, s32)` - the call site's narrow\n"
    " * view - while its band had no registered unit, and the two spellings could not both be visible\n"
    " * ((10505) illegal overloading).  `menu/fn_802A6624.cpp` owns the address and its header\n"
    " * `include/menu/fn_802A6624.h` (included below) declares the definition's `s32`/`u16` spelling\n"
    " * (docs/plan.md 6.5 rule 2). */\n")
REAL_FN_MAIN = REAL_FN_BASE.replace("include/menu/fn_802A6624.h", "include/menu/menu_message.h")
REAL_FN_BRANCH = REAL_FN_MAIN.replace("`menu/fn_802A6624.cpp`", "`menu/menu_message.cpp`")


def nested_comment(text: str) -> bool:
    """True when a `/*` opens while a block comment is already open - the duplicated-prose signature."""
    in_block = False
    for line in text.split("\n"):
        j = 0
        while j < len(line):
            if line.startswith("*/", j) and in_block:
                in_block, j = False, j + 2
                continue
            if line.startswith("/*", j):
                if in_block:
                    return True
                in_block, j = True, j + 2
                continue
            j += 1
    return False


def old_union(text: str) -> str:
    """The pre-fix behaviour (every hunk ours-then-theirs, base dropped), kept only to pin the defect."""
    out, i, src = [], 0, text.splitlines(keepends=True)
    while i < len(src):
        if not src[i].startswith("<<<<<<<"):
            out.append(src[i])
            i += 1
            continue
        ours, theirs, mode = [], [], "ours"
        i += 1
        while i < len(src) and not src[i].startswith(">>>>>>>"):
            if src[i].startswith("|||||||"):
                mode = "base"
            elif src[i].startswith("======="):
                mode = "theirs"
            elif mode == "ours":
                ours.append(src[i])
            elif mode == "theirs":
                theirs.append(src[i])
            i += 1
        out.extend(ours)
        out.extend(t for t in theirs if t not in ours)
        i += 1
    return "".join(out)


def hermetic_git() -> None:
    """The tools' own git calls inherit the environment: no global/system config, and an identity for commits."""
    for key in [k for k in os.environ if k.startswith("GIT_")]:
        del os.environ[key]
    os.environ.update(GIT_CONFIG_GLOBAL=os.devnull, GIT_CONFIG_NOSYSTEM="1", GIT_TERMINAL_PROMPT="0",
                      GIT_AUTHOR_NAME="fixture", GIT_AUTHOR_EMAIL="fixture@example.invalid",
                      GIT_COMMITTER_NAME="fixture", GIT_COMMITTER_EMAIL="fixture@example.invalid")
