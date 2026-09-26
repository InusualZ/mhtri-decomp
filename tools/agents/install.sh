#!/usr/bin/env bash
# install.sh - install the tracked subagent profiles to user scope.
#
# WHY THIS EXISTS (measured 2026-09-25, and the reason a launch failed):
#
#   Every campaign lane runs in a `git worktree` cut from the `main` of its
#   claim time. The harness discovers project agents in `<cwd>/.agents`, so a
#   worktree only sees profiles that were already committed when its branch was
#   cut. A worktree created before the profile commit therefore has a project
#   agent dir that is "present but empty", and launching `{ agent: "fixer" }`
#   fails with `Unknown agent: fixer`. Running `subagent list` from MAIN hides
#   this completely - the profiles are there, and they work.
#
#   The user scope (`~/.pi/agent/agents/`) is consulted for every cwd, so the
#   fix is to install the tracked copies there. `.agents/agents/*.md` stays the
#   reviewed source of truth; this makes it visible everywhere.
#
# Run it after editing any profile, then re-test with tools/agents/profileprobe.py.
set -eu
cd "$(dirname "$0")/../.." || exit 1
src=".agents/agents"
dst="${HOME}/.pi/agent/agents"
[ -d "$src" ] || { echo "no $src - wrong directory?"; exit 1; }
mkdir -p "$dst"
for f in "$src"/*.md; do
  [ -e "$f" ] || continue
  cp "$f" "$dst/"
  echo "installed $(basename "$f")"
done
echo
echo "profiles in $dst:"
ls -1 "$dst" | sed 's/^/  /'
