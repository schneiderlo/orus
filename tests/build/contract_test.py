from __future__ import annotations

import copy
import json
import os
import subprocess
import tempfile
import unittest
from pathlib import Path
from unittest import mock

from tools.build.acquisition_profile import requests_for_root, run_boundary_fixtures
from tools.build.build_contract import (
    ContractError,
    acquisition_outcome,
    canonical_json,
    derive_environment_id,
    parse_module_graph,
    scan_prohibited,
    strict_json_loads,
    validate_acquisition_manifest,
    validate_nix_environment,
    validate_reference_bootstrap,
    validate_repository,
    validate_resolved_input_population,
    workspace_root,
)
from tools.build.hermeticity_audit import (
    audit_actions,
    audit_bcr_materialization,
    audit_nix_materialization,
    selected_module_graph,
)
from tools.build.prohibited_path_scan import main as prohibited_path_main
from tools.coverage.package_gate import evaluate, locate_lcov, parse_lcov, validate_manifest
from tools.format import find_violations, main as format_main


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

    def test_transactional_boundary_matrix_promotes_only_exact_cases(self) -> None:
        matrix = run_boundary_fixtures()
        positives = {"coordinates_exact", "blob_exact", "total_exact", "time_exact"}
        self.assertEqual({name for name, result in matrix.items() if result["promoted"]}, positives)
        self.assertEqual(matrix["unadmitted"]["transport_calls"], 0)
        self.assertEqual(matrix["coordinates_first_over"]["transport_calls"], 0)

    def test_transaction_requests_cover_every_bcr_materialization(self) -> None:
        manifest, requests = requests_for_root(workspace_root())
        admitted = {row["coordinate"] for row in manifest["admitted_inputs"]}
        self.assertEqual(len(requests), 26)
        self.assertTrue({request.coordinate for request in requests}.issubset(admitted))

    def test_selected_module_population_is_complete(self) -> None:
        population = validate_resolved_input_population(workspace_root())
        self.assertIn("bcr:protobuf@29.0", population["module_coordinates"])
        with self.assertRaisesRegex(ContractError, "selected Bzlmod graph mismatch"):
            validate_resolved_input_population(workspace_root(), selected_modules={"bcr:missing@1.0"})

    def test_module_graph_parser_rejects_empty_and_extracts_cycles(self) -> None:
        graph = "<root> (orus@0.0.0-m0)\n├───rules_cc@0.2.22\n│   └───platforms@1.0.0 (*)\n"
        self.assertEqual(parse_module_graph(graph), {"bcr:platforms@1.0.0", "bcr:rules_cc@0.2.22"})
        with self.assertRaisesRegex(ContractError, "empty or malformed"):
            parse_module_graph("<root> (orus@0.0.0-m0)\n")


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


class HermeticityAuditTest(unittest.TestCase):
    def test_action_audit_accepts_only_pinned_drivers(self) -> None:
        release = "Command Line: '/workspace/toolchains/bin/orus-clang' '-std=c++23' '-fuse-ld=/nix/store/lld/bin/ld.lld'"
        gcc = "Command Line: '/workspace/toolchains/bin/orus-gcc' '-std=c++23'"
        with mock.patch.dict(os.environ, {"ORUS_LLD": "/nix/store/lld/bin/ld.lld"}), mock.patch(
            "tools.build.hermeticity_audit.action_query", side_effect=[release, gcc]
        ):
            report = audit_actions(workspace_root())
        self.assertEqual(report["network_enabled_actions"], 0)
        self.assertEqual(report["release_action_count"], 1)

    def test_action_audit_rejects_host_path(self) -> None:
        release = (
            "Command Line: '/workspace/toolchains/bin/orus-clang' '-std=c++23' "
            "'-fuse-ld=/nix/store/lld/bin/ld.lld' '/usr/bin/ld'"
        )
        gcc = "Command Line: '/workspace/toolchains/bin/orus-gcc' '-std=c++23'"
        with mock.patch.dict(os.environ, {"ORUS_LLD": "/nix/store/lld/bin/ld.lld"}), mock.patch(
            "tools.build.hermeticity_audit.action_query", side_effect=[release, gcc]
        ), self.assertRaisesRegex(ContractError, "host path"):
            audit_actions(workspace_root())

    def test_selected_graph_command_is_parsed(self) -> None:
        completed = subprocess.CompletedProcess(
            args=[], returncode=0, stdout="<root> (orus@0)\n└───rules_cc@0.2.22\n", stderr=""
        )
        with mock.patch.dict(os.environ, {"ORUS_BAZEL": "/nix/store/bazel/bin/bazel"}), mock.patch(
            "tools.build.hermeticity_audit.subprocess.run", return_value=completed
        ):
            self.assertEqual(selected_module_graph(workspace_root()), {"bcr:rules_cc@0.2.22"})

    def test_actual_bcr_materialization_is_hash_verified_and_read_only(self) -> None:
        population = validate_resolved_input_population(workspace_root())
        report = audit_bcr_materialization(workspace_root(), population["module_coordinates"])
        self.assertEqual(report["archive_count"], 25)
        self.assertEqual(report["fetched_coordinate_count"], 26)
        self.assertGreater(report["registry_file_count"], 0)

    def test_nix_materialization_requires_every_inventoried_store_path(self) -> None:
        population = validate_resolved_input_population(workspace_root())
        inventory_names = {
            entry["name"]
            for entry in population["inventory"]["dependencies"]
            if not entry["owner"].startswith("bzlmod_")
        }
        environment = {
            name: f"/nix/store/fixture-{name}"
            for name in (
                "ORUS_BAZEL",
                "ORUS_BENCHMARK_SRC",
                "ORUS_CLANG",
                "ORUS_GCC",
                "ORUS_GLAZE_SRC",
                "ORUS_GOOGLETEST_SRC",
                "ORUS_LLD",
                "ORUS_LLVM",
                "ORUS_NIXPKGS_SRC",
                "ORUS_OPENSSL",
                "ORUS_OPENSSL_DEV",
                "ORUS_PYTHON",
                "ORUS_UTF8PROC",
            )
        }
        with mock.patch.dict(os.environ, environment), mock.patch(
            "tools.build.hermeticity_audit.Path.exists", return_value=True
        ), mock.patch("tools.build.hermeticity_audit._require_read_only"):
            report = audit_nix_materialization(population)
        self.assertEqual(report["path_count"], len(inventory_names))


class FormatTest(unittest.TestCase):
    def test_finite_tree_format_scan(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            (root / "good.py").write_text("value = 1\n", encoding="utf-8")
            (root / "bad.py").write_text("value = 1  \n", encoding="utf-8")
            (root / "bad.json").write_text('{"z": 1, "a": 2}\n', encoding="utf-8")
            self.assertEqual(find_violations(root), ["bad.json", "bad.py"])

    def test_main_accepts_a_clean_tree(self) -> None:
        with tempfile.TemporaryDirectory() as directory, mock.patch.dict(
            os.environ, {"BUILD_WORKSPACE_DIRECTORY": directory}
        ):
            self.assertEqual(format_main(), 0)

    def test_prohibited_path_command_exercises_real_and_fixture_trees(self) -> None:
        self.assertEqual(prohibited_path_main(), 0)


class CoverageGateTest(unittest.TestCase):
    def test_exact_threshold_passes(self) -> None:
        self.assertEqual(
            evaluate({"tools/build/a.py": (7, 10)}, {"tools/build": ["tools/build/a.py"]}, 70.0),
            {"tools/build": 70.0},
        )

    def test_first_below_threshold_fails(self) -> None:
        with self.assertRaisesRegex(ValueError, "below 70.00%"):
            evaluate({"tools/build/a.py": (6999, 10000)}, {"tools/build": ["tools/build/a.py"]}, 70.0)

    def test_missing_source_data_fails(self) -> None:
        with self.assertRaisesRegex(ValueError, "missing coverage data"):
            evaluate(
                {"tools/build/a.py": (10, 10)},
                {"tools/build": ["tools/build/a.py", "tools/build/b.py"]},
                70.0,
            )

    def test_lcov_parser_retains_each_declared_source(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            report = Path(directory) / "coverage.dat"
            report.write_text(
                "SF:/workspace/tools/build/a.py\nDA:1,1\nDA:2,0\nend_of_record\n"
                "SF:tools/coverage/b.py\nDA:1,1\nend_of_record\n",
                encoding="utf-8",
            )
            self.assertEqual(
                parse_lcov(report, {"tools/build/a.py", "tools/coverage/b.py"}),
                {"tools/build/a.py": (1, 2), "tools/coverage/b.py": (1, 1)},
            )

    def test_absent_lcov_fails_closed(self) -> None:
        with tempfile.TemporaryDirectory() as directory, self.assertRaisesRegex(ValueError, "coverage data is absent"):
            locate_lcov(Path(directory))

    def test_manifest_requires_every_owned_source_or_reviewed_exclusion(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            (root / "tools/build").mkdir(parents=True)
            (root / "tools/coverage").mkdir(parents=True)
            (root / "tools/build/a.py").write_text("pass\n", encoding="utf-8")
            (root / "tools/build/__init__.py").write_text("", encoding="utf-8")
            (root / "tools/coverage/b.py").write_text("pass\n", encoding="utf-8")
            (root / "tools/format.py").write_text("pass\n", encoding="utf-8")
            manifest = {
                "schema": "M0-COVERAGE-PACKAGES-v2",
                "packages": [
                    {"name": "tools", "sources": ["tools/format.py"]},
                    {"name": "tools/build", "sources": ["tools/build/a.py"]},
                    {"name": "tools/coverage", "sources": ["tools/coverage/b.py"]},
                ],
                "exclusions": [{"path": "tools/build/__init__.py", "reason": "empty package marker"}],
            }
            self.assertEqual(set(validate_manifest(root, manifest)), {"tools", "tools/build", "tools/coverage"})
            manifest["exclusions"] = []
            with self.assertRaisesRegex(ValueError, "omits or invents"):
                validate_manifest(root, manifest)
            manifest["exclusions"] = [{"path": "tools/missing.py", "reason": "not a real owned source"}]
            with self.assertRaisesRegex(ValueError, "omits or invents"):
                validate_manifest(root, manifest)

    def test_repository_coverage_manifest_is_complete(self) -> None:
        root = workspace_root()
        manifest = json.loads((root / "tools/coverage/packages.json").read_text(encoding="utf-8"))
        packages = validate_manifest(root, manifest)
        self.assertEqual(set(packages), {"tools", "tools/build", "tools/coverage"})


if __name__ == "__main__":
    unittest.main()
