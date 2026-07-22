"""Executable acquisition, environment, and tool-identity audit."""

from __future__ import annotations

import json
import os
import subprocess
from pathlib import Path

from tools.build.build_contract import (
    ContractError,
    acquisition_outcome,
    command_version,
    validate_acquisition_manifest,
    validate_nix_environment,
    validate_repository,
    workspace_root,
)


def action_query(root: Path, configuration: str) -> str:
    process = subprocess.run(
        [
            os.environ["ORUS_BAZEL"],
            "aquery",
            f"--config={configuration}",
            'mnemonic("CppCompile|CppLink", //tests/build:toolchain_conformance_test)',
            "--include_commandline",
            "--output=text",
        ],
        cwd=root,
        env=os.environ,
        check=False,
        capture_output=True,
        text=True,
        timeout=120,
    )
    if process.returncode:
        raise ContractError(
            "BUILD_CONFIG_INVALID",
            f"{configuration} action query failed: {process.stderr.strip()[-1000:]}",
        )
    return process.stdout


def audit_actions(root: Path) -> dict[str, object]:
    release = action_query(root, "release")
    gcc = action_query(root, "gcc")
    if "/toolchains/bin/orus-clang" not in release or "'-std=c++23'" not in release:
        raise ContractError("BUILD_CONFIG_INVALID", "release actions do not select the pinned C++23 Clang driver")
    lld_flag = f"'-fuse-ld={os.environ['ORUS_LLD']}'"
    if lld_flag not in release:
        raise ContractError("BUILD_CONFIG_INVALID", "release link action does not select the pinned LLD")
    if "/toolchains/bin/orus-gcc" not in gcc or "'-std=c++23'" not in gcc:
        raise ContractError("BUILD_CONFIG_INVALID", "GCC actions do not select the pinned C++23 GCC driver")
    for forbidden in ("/usr/bin/clang", "/usr/bin/gcc", "/usr/bin/g++", "/usr/bin/ld", "/usr/local/"):
        if forbidden in release or forbidden in gcc:
            raise ContractError("BUILD_UNDECLARED_INPUT", f"action graph contains host path {forbidden}")

    bazelrc = (root / ".bazelrc").read_text(encoding="utf-8")
    if "build --sandbox_default_allow_network=false" not in bazelrc.splitlines():
        raise ContractError("BUILD_UNDECLARED_INPUT", "sandboxed actions do not default to network denial")
    network_exceptions = []
    for path in root.rglob("BUILD.bazel"):
        if "requires-network" in path.read_text(encoding="utf-8"):
            network_exceptions.append(str(path.relative_to(root)))
    if network_exceptions:
        raise ContractError("BUILD_UNDECLARED_INPUT", f"network-enabled actions found: {network_exceptions}")

    return {
        "gcc_action_count": gcc.count("Command Line:"),
        "gcc_driver": "toolchains/bin/orus-gcc",
        "network_enabled_actions": 0,
        "network_negative_fixture": "//tests/build:network_denial_test",
        "release_action_count": release.count("Command Line:"),
        "release_driver": "toolchains/bin/orus-clang",
        "release_linker": os.environ["ORUS_LLD"],
    }


def main() -> int:
    try:
        root = workspace_root()
        validate_repository(root)
        validate_nix_environment(os.environ)
        manifest = validate_acquisition_manifest(root / "config/input-acquisition.json")
        first = manifest["admitted_inputs"][0]
        digest = first["sha256"]
        limits = manifest["limits"]
        boundary_matrix = {
            "exact_blob": acquisition_outcome(
                manifest,
                coordinate=first["coordinate"],
                blob_bytes=limits["per_blob_bytes"],
                total_bytes=limits["total_bytes"],
                elapsed_seconds=limits["wall_seconds"],
                observed_sha256=digest,
            ),
            "blob_first_over": acquisition_outcome(
                manifest,
                coordinate=first["coordinate"],
                blob_bytes=limits["per_blob_bytes"] + 1,
                total_bytes=1,
                elapsed_seconds=1,
                observed_sha256=digest,
            ),
            "total_first_over": acquisition_outcome(
                manifest,
                coordinate=first["coordinate"],
                blob_bytes=1,
                total_bytes=limits["total_bytes"] + 1,
                elapsed_seconds=1,
                observed_sha256=digest,
            ),
            "time_first_over": acquisition_outcome(
                manifest,
                coordinate=first["coordinate"],
                blob_bytes=1,
                total_bytes=1,
                elapsed_seconds=limits["wall_seconds"] + 1,
                observed_sha256=digest,
            ),
            "hash_mismatch": acquisition_outcome(
                manifest,
                coordinate=first["coordinate"],
                blob_bytes=1,
                total_bytes=1,
                elapsed_seconds=1,
                observed_sha256="0" * 64,
            ),
            "mutable": acquisition_outcome(
                manifest,
                coordinate=first["coordinate"],
                blob_bytes=1,
                total_bytes=1,
                elapsed_seconds=1,
                observed_sha256=digest,
                mutable=True,
            ),
            "unadmitted": acquisition_outcome(
                manifest,
                coordinate="https://invalid.example/unadmitted",
                blob_bytes=1,
                total_bytes=1,
                elapsed_seconds=1,
                observed_sha256=digest,
            ),
        }
        if boundary_matrix["exact_blob"] != "promoted_read_only":
            raise ContractError("BUILD_ACQUISITION_DENIED", "exact acquisition boundary did not pass")
        if any(value != "BUILD_ACQUISITION_DENIED" for key, value in boundary_matrix.items() if key != "exact_blob"):
            raise ContractError("BUILD_ACQUISITION_DENIED", "a negative acquisition fixture passed")

        versions = {
            "bazel": command_version([os.environ["ORUS_BAZEL"], "--version"]),
            "clang": command_version([os.environ["ORUS_CLANG"], "--version"]),
            "gcc": command_version([os.environ["ORUS_GCC"], "--version"]),
            "lld": command_version([os.environ["ORUS_LLD"], "--version"]),
            "python": command_version([os.environ["ORUS_PYTHON"], "--version"]),
        }
        action_audit = audit_actions(root)
        report = {
            "schema": "M0-HERMETICITY-REPORT-v1",
            "acquisition_boundary": boundary_matrix,
            "action_audit": action_audit,
            "capability_profile": "network_denied_build_actions",
            "tool_versions": versions,
            "undeclared_host_inputs": 0,
        }
        print(json.dumps(report, separators=(",", ":"), sort_keys=True))
        return 0
    except ContractError as error:
        print(str(error), file=os.sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
