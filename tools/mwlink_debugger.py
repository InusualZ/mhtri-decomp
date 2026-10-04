#!/usr/bin/env python3
"""Interrogate the Metrowerks linker (mwldeppc.exe) about a real link - the CLI entry point of the tools/mwlink/ package.
Spec: docs/tools/spec/mwlink_debugger.md. CLI: mwlink_debugger.py info|messages|order|anchors|records|align|timeline|verify|trace|diagnose|phases."""
import sys, pathlib; sys.path.insert(0, str(next(p for p in pathlib.Path(__file__).resolve().parents if (p / "tools" / "__init__.py").is_file())))

from tools.mwlink.cli import main

if __name__ == "__main__":
    sys.exit(main())
