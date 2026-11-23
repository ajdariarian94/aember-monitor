#!/usr/bin/env python3
"""The repository's `aember` CLI (sdk, build, pack, install, test, all):
the shared one from the aember-build-helper submodule, told what this
repository is. `aember-cli/run.py --help`."""

import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools" / "aember-build-helper" / "project-cli"))
from aember_project import Project, main  # noqa: E402

NAME = "aember-monitor"
# aember-resources release with the SDKs; v0.1.13 adds Qt.
RESOURCES_VERSION = "0.1.13"
TESTS = "sdk"  # built with the x64 SDK, run through its loader: needs its libraries

if __name__ == "__main__":
    sys.exit(main(Project(NAME, RESOURCES_VERSION, tests=TESTS, root=ROOT)))
