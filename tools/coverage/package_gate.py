"""Fail-closed per-package LCOV threshold gate for M0 logic."""

from __future__ import annotations

import argparse
import json
import os
from pathlib import Path


def parse_lcov(path: Path) -> dict[str, tuple[int, int]]:
    packages: dict[str, list[int]] = {}
    current: str | None = None
    for line in path.read_text(encoding="utf-8").splitlines():
        if line.startswith("SF:"):
            source = line[3:].replace("\\", "/")
            marker = "/tools/build/"
            current = "tools/build" if marker in f"/{source}" else None
        elif current is not None and line.startswith("DA:"):
            _, count = line[3:].split(",", maxsplit=1)
            totals = packages.setdefault(current, [0, 0])
            totals[1] += 1
            if int(count) > 0:
                totals[0] += 1
        elif line == "end_of_record":
            current = None
    return {name: (values[0], values[1]) for name, values in packages.items()}


def evaluate(packages: dict[str, tuple[int, int]], expected: list[str], threshold: float) -> dict[str, float]:
    result: dict[str, float] = {}
    for package in expected:
        if package not in packages:
            raise ValueError(f"missing coverage data for {package}")
        covered, total = packages[package]
        if total <= 0:
            raise ValueError(f"empty coverage data for {package}")
        percent = 100.0 * covered / total
        if percent + 1e-12 < threshold:
            raise ValueError(f"{package} is {percent:.2f}%, below {threshold:.2f}%")
        result[package] = percent
    return result


def locate_lcov(workspace: Path) -> Path:
    candidates = [
        workspace / "bazel-out/_coverage/_coverage_report.dat",
        workspace / "bazel-out/_coverage/coverage.dat",
    ]
    for candidate in candidates:
        if candidate.is_file() and candidate.stat().st_size:
            return candidate
    for candidate in (workspace / "bazel-out").glob("**/_coverage_report.dat"):
        if candidate.stat().st_size:
            return candidate
    raise ValueError("coverage data is absent")


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--threshold", required=True, type=float)
    arguments = parser.parse_args()
    workspace = Path(os.environ.get("BUILD_WORKSPACE_DIRECTORY", Path.cwd()))
    manifest = json.loads((workspace / "tools/coverage/packages.json").read_text(encoding="utf-8"))
    if manifest != {"schema": "M0-COVERAGE-PACKAGES-v1", "packages": ["tools/build"]}:
        raise SystemExit("coverage exclusion/package manifest is not the reviewed finite manifest")
    try:
        report = evaluate(parse_lcov(locate_lcov(workspace)), manifest["packages"], arguments.threshold)
    except ValueError as error:
        raise SystemExit(str(error)) from error
    print(json.dumps({"schema": "M0-PACKAGE-COVERAGE-v1", "packages": report}, separators=(",", ":"), sort_keys=True))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
