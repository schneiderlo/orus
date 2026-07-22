from __future__ import annotations

import copy
import json
import os
import subprocess
import tempfile
import unittest
from pathlib import Path
from unittest import mock

from tools.build.build_contract import (
    ContractError,
    acquisition_outcome,
    canonical_json,
    derive_environment_id,
    scan_prohibited,
    strict_json_loads,
    validate_acquisition_manifest,
    validate_nix_environment,
    validate_reference_bootstrap,
    validate_repository,
    workspace_root,
)
from tools.coverage.package_gate import evaluate


class JsonBootstrapTest(unittest.TestCase):
    def test_rejects_duplicate_names(self) -> None:
        with self.assertRaisesRegex(ContractError, "duplicate object name"):
            strict_json_loads(b'{"a":1,"a":2}')

    def test_rejects_floating_point_and_non_nfc(self) -> None:
        with self.assertRaisesRegex(ContractError, "floating-point"):
            strict_json_loads(b'{"a":1.0}')
        with self.assertRaisesRegex(ContractError, "NFC"):
            strict_json_loads('{"a":"e\u0301"}'.encode())

    def test_canonical_bytes_are_sorted_and_compact(self) -> None:
        self.assertEqual(canonical_json({"z": 1, "a": "ok"}), b'{"a":"ok","z":1}')

    def test_reference_identity_mismatch_fails(self) -> None:
        root = workspace_root()
        document = json.loads((root / "config/m0-reference-environment.json").read_text())
        self.assertEqual(document["environment_id"], derive_environment_id(document))
        document["environment_id"] = "0" * 64
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "reference.json"
            path.write_bytes(canonical_json(document))
            with self.assertRaisesRegex(ContractError, "BUILD_REFENV_ID_MISMATCH"):
                validate_reference_bootstrap(path, ["glaze", "google_benchmark", "googletest", "nixpkgs"])


class AcquisitionTest(unittest.TestCase):
    def setUp(self) -> None:
        self.manifest = validate_acquisition_manifest(workspace_root() / "config/input-acquisition.json")
        self.entry = self.manifest["admitted_inputs"][0]
        self.limits = self.manifest["limits"]

    def outcome(self, **changes: object) -> str:
        values: dict[str, object] = {
            "coordinate": self.entry["coordinate"],
            "blob_bytes": self.limits["per_blob_bytes"],
            "total_bytes": self.limits["total_bytes"],
            "elapsed_seconds": self.limits["wall_seconds"],
            "observed_sha256": self.entry["sha256"],
        }
        values.update(changes)
        return acquisition_outcome(self.manifest, **values)  # type: ignore[arg-type]

    def test_exact_limits_pass(self) -> None:
        self.assertEqual(self.outcome(), "promoted_read_only")

    def test_every_first_over_limit_fails(self) -> None:
        for change in (
            {"blob_bytes": self.limits["per_blob_bytes"] + 1},
            {"total_bytes": self.limits["total_bytes"] + 1},
            {"elapsed_seconds": self.limits["wall_seconds"] + 1},
        ):
            with self.subTest(change=change):
                self.assertEqual(self.outcome(**change), "BUILD_ACQUISITION_DENIED")

    def test_unadmitted_mutable_and_hash_mismatch_fail(self) -> None:
        self.assertEqual(self.outcome(coordinate="unadmitted"), "BUILD_ACQUISITION_DENIED")
        self.assertEqual(self.outcome(mutable=True), "BUILD_ACQUISITION_DENIED")
        self.assertEqual(self.outcome(observed_sha256="0" * 64), "BUILD_ACQUISITION_DENIED")

    def test_coordinate_count_exact_and_first_over(self) -> None:
        exact = copy.deepcopy(self.manifest)
        seed = exact["admitted_inputs"][0]
        exact["admitted_inputs"] = [
            {"coordinate": f"fixture:{index:03}", "sha256": seed["sha256"], "store_mode": "read_only"}
            for index in range(128)
        ]
        self.assertEqual(
            acquisition_outcome(
                exact,
                coordinate="fixture:000",
                blob_bytes=1,
                total_bytes=1,
                elapsed_seconds=1,
                observed_sha256=seed["sha256"],
            ),
            "promoted_read_only",
        )
        exact["admitted_inputs"].append(
            {"coordinate": "fixture:128", "sha256": seed["sha256"], "store_mode": "read_only"}
        )
        self.assertEqual(
            acquisition_outcome(
                exact,
                coordinate="fixture:000",
                blob_bytes=1,
                total_bytes=1,
                elapsed_seconds=1,
                observed_sha256=seed["sha256"],
            ),
            "BUILD_ACQUISITION_DENIED",
        )


class EnvironmentAndScannerTest(unittest.TestCase):
    def test_environment_rejects_host_tool_or_network(self) -> None:
        environment = {name: f"/nix/store/hash-{name.lower()}" for name in (
            "ORUS_BAZEL", "ORUS_CLANG", "ORUS_CLANGXX", "ORUS_GCC", "ORUS_GCC_LIB", "ORUS_GXX",
            "ORUS_LLD", "ORUS_LLVM", "ORUS_PYTHON", "ORUS_OPENSSL", "ORUS_OPENSSL_DEV", "ORUS_UTF8PROC",
        )}
        environment["ORUS_NETWORK_ALLOWED"] = "0"
        validate_nix_environment(environment)
        environment["ORUS_GCC"] = "/usr/bin/gcc"
        with self.assertRaisesRegex(ContractError, "BUILD_UNDECLARED_INPUT"):
            validate_nix_environment(environment)
        environment["ORUS_GCC"] = "/nix/store/hash-gcc"
        environment["ORUS_NETWORK_ALLOWED"] = "1"
        with self.assertRaisesRegex(ContractError, "network"):
            validate_nix_environment(environment)

    def test_injected_prohibited_fixtures_are_detected(self) -> None:
        fixture = workspace_root() / "tests/build/fixtures/prohibited"
        found = {finding.path for finding in scan_prohibited(fixture, exclude_fixtures=False)}
        self.assertTrue({"CMakeLists.txt", "CMakePresets.json", "docs/alternate-build.md"}.issubset(found))

    def test_toolchain_drivers_reject_host_compilers(self) -> None:
        root = workspace_root()
        cases = (
            ("orus-clang", {"ORUS_CLANG": "/usr/bin/clang"}),
            ("orus-gcc", {"ORUS_GCC": "/usr/bin/gcc"}),
        )
        baseline = {
            "ORUS_CLANG": "/nix/store/fixture/bin/clang",
            "ORUS_GCC": "/nix/store/fixture/bin/gcc",
            "ORUS_GCC_LIB": "/nix/store/fixture-gcc-1-lib",
            "ORUS_LLD": "/nix/store/fixture/bin/ld.lld",
        }
        for driver, override in cases:
            with self.subTest(driver=driver):
                environment = os.environ | baseline | override
                process = subprocess.run(
                    [root / "toolchains/bin" / driver, "--version"],
                    env=environment,
                    check=False,
                    capture_output=True,
                    text=True,
                )
                self.assertEqual(process.returncode, 78)
                self.assertIn("BUILD_CONFIG_INVALID", process.stderr)

    def test_real_repository_contract(self) -> None:
        validate_repository(workspace_root())


class CoverageGateTest(unittest.TestCase):
    def test_exact_threshold_passes(self) -> None:
        self.assertEqual(evaluate({"tools/build": (7, 10)}, ["tools/build"], 70.0), {"tools/build": 70.0})

    def test_first_below_threshold_fails(self) -> None:
        with self.assertRaisesRegex(ValueError, "below 70.00%"):
            evaluate({"tools/build": (6999, 10000)}, ["tools/build"], 70.0)


if __name__ == "__main__":
    unittest.main()
