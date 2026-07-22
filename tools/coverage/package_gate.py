"""Fail-closed per-package LCOV threshold gate for M0 logic."""

from __future__ import annotations

import argparse
import json
import os
from pathlib import Path
from typing import Any, Mapping


def task_owned_sources(workspace: Path) -> set[str]:
    roots = (
        (workspace / "contracts", "*.cc"),
        (workspace / "python" / "orus_contracts", "*.py"),
        (workspace / "tools", "*.py"),
    )
    return {
        str(path.relative_to(workspace))
        for root, pattern in roots
        if root.is_dir()
        for path in root.rglob(pattern)
        if path.is_file()
    }


def validate_manifest(workspace: Path, manifest: Mapping[str, Any]) -> dict[str, list[str]]:
    if set(manifest) != {"exclusions", "packages", "schema"} or manifest.get("schema") != "M0-COVERAGE-PACKAGES-v2":
        raise ValueError("coverage manifest schema or fields are invalid")
    packages = manifest.get("packages")
    exclusions = manifest.get("exclusions")
    if not isinstance(packages, list) or not packages or not isinstance(exclusions, list):
        raise ValueError("coverage package or exclusion population is invalid")

    package_sources: dict[str, list[str]] = {}
    included: list[str] = []
    names = [row.get("name") for row in packages if isinstance(row, dict)]
    if names != sorted(names) or len(names) != len(set(names)):
        raise ValueError("coverage packages must be sorted and unique")
    for row in packages:
        if set(row) != {"name", "sources"} or not isinstance(row["sources"], list) or not row["sources"]:
            raise ValueError("coverage package row is invalid")
        sources = row["sources"]
        if sources != sorted(sources) or len(sources) != len(set(sources)):
            raise ValueError(f"coverage sources for {row['name']} must be sorted and unique")
        package_sources[row["name"]] = sources
        included.extend(sources)

    excluded: list[str] = []
    exclusion_paths = [row.get("path") for row in exclusions if isinstance(row, dict)]
    if exclusion_paths != sorted(exclusion_paths) or len(exclusion_paths) != len(set(exclusion_paths)):
        raise ValueError("coverage exclusions must be sorted and unique")
    for row in exclusions:
        if set(row) != {"path", "reason"} or not row["path"] or not row["reason"]:
            raise ValueError("coverage exclusion lacks a finite path and rationale")
        excluded.append(row["path"])

    if len(included) != len(set(included)) or set(included).intersection(excluded):
        raise ValueError("coverage source population is duplicated or both included and excluded")
    owned = task_owned_sources(workspace)
    declared = set(included).union(excluded)
    if owned != declared:
        raise ValueError(
            f"coverage manifest omits or invents task-owned logic: missing={sorted(owned - declared)}, "
            f"unknown={sorted(declared - owned)}"
        )
    return package_sources


def parse_lcov(path: Path, expected_sources: set[str]) -> dict[str, tuple[int, int]]:
    sources: dict[str, list[int]] = {}
    current: str | None = None
    for line in path.read_text(encoding="utf-8").splitlines():
        if line.startswith("SF:"):
            source = line[3:].replace("\\", "/")
            matches = [expected for expected in expected_sources if source == expected or source.endswith(f"/{expected}")]
            current = matches[0] if len(matches) == 1 else None
            if current is not None and current in sources:
                raise ValueError(f"duplicate coverage record for {current}")
            if current is not None:
                sources[current] = [0, 0]
        elif current is not None and line.startswith("DA:"):
            _, count = line[3:].split(",", maxsplit=1)
            sources[current][1] += 1
            if int(count) > 0:
                sources[current][0] += 1
        elif line == "end_of_record":
            current = None
    return {name: (values[0], values[1]) for name, values in sources.items()}


def evaluate(
    source_coverage: Mapping[str, tuple[int, int]],
    package_sources: Mapping[str, list[str]],
    threshold: float,
) -> dict[str, float]:
    result: dict[str, float] = {}
    for package, sources in package_sources.items():
        missing = sorted(set(sources) - set(source_coverage))
        if missing:
            raise ValueError(f"missing coverage data for {package}: {missing}")
        covered = sum(source_coverage[source][0] for source in sources)
        total = sum(source_coverage[source][1] for source in sources)
        if total <= 0 or any(source_coverage[source][1] <= 0 for source in sources):
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
    try:
        packages = validate_manifest(workspace, manifest)
        coverage = parse_lcov(locate_lcov(workspace), {source for sources in packages.values() for source in sources})
        report = evaluate(coverage, packages, arguments.threshold)
    except ValueError as error:
        raise SystemExit(str(error)) from error
    print(json.dumps({"schema": "M0-PACKAGE-COVERAGE-v1", "packages": report}, separators=(",", ":"), sort_keys=True))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
