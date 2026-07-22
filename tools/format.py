"""Deterministic whitespace and terminal-newline formatter check."""

from __future__ import annotations

import json
import os
from pathlib import Path


TEXT_SUFFIXES = {".bzl", ".cc", ".h", ".json", ".md", ".nix", ".py", ".txt"}
TEXT_FILENAMES = {".bazelrc", ".bazelversion", "BUILD.bazel", "MODULE.bazel", "orus-clang", "orus-gcc"}


def main() -> int:
    root = Path(os.environ.get("BUILD_WORKSPACE_DIRECTORY", Path.cwd()))
    bad: list[str] = []
    excluded = {".git", ".factory", ".agents", "bazel-bin", "bazel-out", "bazel-testlogs", "bazel-orus"}
    for path in sorted(root.rglob("*")):
        relative = path.relative_to(root)
        if (
            excluded.intersection(relative.parts)
            or relative.parts[:1] == ("specs",)
            or relative.parts[:3] == ("tests", "build", "fixtures")
            or relative in {Path("IMPLEMENTATION_PLAN.md"), Path("SPECS.md")}
            or not path.is_file()
            or (path.suffix not in TEXT_SUFFIXES and path.name not in TEXT_FILENAMES)
        ):
            continue
        data = path.read_bytes()
        if path.suffix == ".json":
            try:
                canonical = json.dumps(
                    json.loads(data),
                    ensure_ascii=False,
                    allow_nan=False,
                    separators=(",", ":"),
                    sort_keys=True,
                ).encode("utf-8")
            except (UnicodeDecodeError, ValueError):
                bad.append(str(relative))
                continue
            if data != canonical:
                bad.append(str(relative))
        elif data and (
            not data.endswith(b"\n")
            or any(line.endswith((b" ", b"\t")) for line in data.splitlines())
        ):
            bad.append(str(relative))
    if bad:
        print("format violations: " + ", ".join(bad), file=os.sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
