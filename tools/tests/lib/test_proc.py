"""lib.proc: the codec rule, the spawn retry, the process-tree kill and the trap scan."""
import sys, pathlib; sys.path.insert(0, str(next(p for p in pathlib.Path(__file__).resolve().parents if (p / "tools" / "__init__.py").is_file())))
import ast
import os
import subprocess

from tools.lib import proc, testing

TIER = "fixture"

DASH = "—"


def _winerr(code):
    exc = PermissionError(13, "Access is denied")
    exc.winerror = code
    return exc


def test_codec_rule(c):
    code = "import sys; sys.stdout.buffer.write(bytes.fromhex('%s'))" % DASH.encode("utf-8").hex()
    p = proc.run([sys.executable, "-c", code])
    c.check("a UTF-8 byte above ASCII decodes as UTF-8 whatever the locale", p.stdout, DASH)
    bad = proc.run([sys.executable, "-c", "import sys; sys.stdout.buffer.write(b'a\\xffb')"])
    c.check("a byte that is not UTF-8 never raises: it is replaced", bad.stdout, "a�b")
    c.check("output is captured by default", (p.returncode, p.stderr), (0, ""))
    b = proc.run_bytes([sys.executable, "-c", "import sys; sys.stdout.buffer.write(b'\\r\\n')"])
    c.check("run_bytes keeps the bytes (CRLF not translated)", b.stdout, b"\r\n")
    fed = proc.run([sys.executable, "-c", "import sys; print(sys.stdin.read().upper())"], input="abc")
    c.check("input is fed as text", fed.stdout.strip(), "ABC")
    c.raises("check raises CalledProcessError", subprocess.CalledProcessError, proc.run,
             [sys.executable, "-c", "raise SystemExit(3)"], check=True)
    routed = proc.run([sys.executable, "-c", "import sys; sys.stderr.write('e')"], stdout=subprocess.PIPE,
                      stderr=subprocess.STDOUT)
    c.check("a caller routing stdout/stderr itself is honoured", routed.stdout, "e")


def test_spawn_retry(c):
    def flaky(n, exc):
        calls = []

        def init(self, *a, **k):
            calls.append(1)
            if len(calls) <= n:
                raise exc
            return "started"
        return init, calls

    sleeps = []
    init, calls = flaky(2, _winerr(5))
    c.check("WinError 5 twice, then success", proc.retrying(init, sleep=sleeps.append)(None), "started")
    c.check("... three attempts with a growing backoff", (len(calls), sleeps), (3, [0.1, 0.2]))
    init, calls = flaky(99, _winerr(5))
    c.raises("a launch refused every time is raised", PermissionError, proc.retrying(init, sleep=lambda s: None), None)
    c.check("... after SPAWN_ATTEMPTS attempts", len(calls), proc.SPAWN_ATTEMPTS)
    init, calls = flaky(1, _winerr(2))
    c.raises("another PermissionError is raised at once", PermissionError, proc.retrying(init, sleep=lambda s: None), None)
    c.check("... after one attempt", len(calls), 1)
    init, calls = flaky(1, FileNotFoundError("nope"))
    c.raises("FileNotFoundError is raised at once", FileNotFoundError, proc.retrying(init, sleep=lambda s: None), None)
    c.check("is_transient is exactly WinError 5", (proc.is_transient(_winerr(5)), proc.is_transient(_winerr(2)),
                                                   proc.is_transient(OSError())), (True, False, False))
    proc.install_spawn_retry()
    if os.name == "nt":
        c.expect("install leaves Popen wrapped", getattr(subprocess.Popen.__init__, "__wrapped_by_spawnretry__", False))
    c.check("a second install is a no-op", proc.install_spawn_retry(), False)
    c.check("a real launch still works through the wrapper",
            proc.run([sys.executable, "-c", "print(7)"]).stdout.strip(), "7")


def test_kill_tree(c):
    child = subprocess.Popen([sys.executable, "-c", "import time; time.sleep(60)"])
    proc.kill_tree(child)
    try:
        rc = child.wait(timeout=20)
    except subprocess.TimeoutExpired:
        rc = None
        child.kill()
    c.expect("kill_tree ends the process", rc is not None, "the child was still running after 20 s")


def test_trap_scan(c):
    def traps(src):
        return [reason for _line, reason in proc.module_traps(ast.parse(src))]

    c.check("the F34 spelling is a trap",
            len(traps('import subprocess\nsubprocess.run(["git"], capture_output=True, text=True, errors="replace")\n')), 1)
    c.check("the pinned spelling is not",
            traps('import subprocess\nsubprocess.run(["git"], text=True,\n               encoding="utf-8")\n'), [])
    c.check("universal_newlines is the same trap", len(traps('subprocess.Popen(["git"], universal_newlines=True)\n')), 1)
    c.check("binary mode needs no codec", traps('subprocess.run(["git"], capture_output=True, text=False)\n'), [])
    c.check("a ** splat is not claimed", traps('kw = {}\nsubprocess.run(["git"], text=True, **kw)\n'), [])
    c.check("text=<expression> is not claimed", traps('subprocess.run(["git"], text=flag)\n'), [])
    with testing.FixtureTree() as tree:
        tree.write("tools/units/bad.py", 'import subprocess\np = subprocess.run(["git"], text=True)\n')
        tree.write("tools/units/good.py", 'import subprocess\np = subprocess.run(["git"], text=True, encoding="utf-8")\n')
        tree.write("tools/units/bad_selftest.py", 'import subprocess\np = subprocess.run(["git"], text=True)\n')
        tree.write("tools/m2c/vendored.py", 'import subprocess\np = subprocess.run(["git"], text=True)\n')
        tree.write("tools/units/broken.py", "def (:\n")
        sites = proc.trap_sites(tree.root)
        c.check("a tree scan names file and line; vendored code is skipped; a broken file is reported",
                sorted(s.split("tools/", 1)[1].split(":")[0] for s in sites),
                ["units/bad.py", "units/bad_selftest.py", "units/broken.py"])
        c.contains("... with its line", " ".join(sites), "tools/units/bad.py:2:")
        c.check("skip_selftests leaves the selftest out",
                [s for s in proc.trap_sites(tree.root, skip_selftests=True) if "bad_selftest" in s], [])


if __name__ == "__main__":
    raise SystemExit(testing.run(globals()))
