"""Scan the task-owned product tree and exercise prohibited-path fixtures."""

from __future__ import annotations

import json
import os
from pathlib import Path

from tools.build.build_contract import ContractError, scan_prohibited, workspace_root


def main() -> int:
    try:
        root = workspace_root()
        real_findings = scan_prohibited(root, exclude_fixtures=True)
        if real_findings:
            raise ContractError(
                "BUILD_PROHIBITED_PATH",
                "; ".join(f"{item.path}:{item.rule}" for item in real_findings),
            )
        fixture_root = root / "tests/build/fixtures/prohibited"
        fixture_findings = scan_prohibited(fixture_root, exclude_fixtures=False)
        expected = {
            "CMakeLists.txt",
            "CMakePresets.json",
            "docs/alternate-build.md",
        }
        detected = {item.path for item in fixture_findings}
        if not expected.issubset(detected):
            raise ContractError("BUILD_PROHIBITED_PATH", f"fixtures not detected: {sorted(expected - detected)}")
        report = {
            "schema": "M0-PROHIBITED-PATHS-v1",
            "real_findings": 0,
            "fixture_detection": len(expected),
            "fixture_expected": len(expected),
        }
        print(json.dumps(report, separators=(",", ":"), sort_keys=True))
        return 0
    except ContractError as error:
        print(str(error), file=os.sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
