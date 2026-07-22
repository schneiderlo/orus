"""Executable acquisition, environment, and tool-identity audit."""

from __future__ import annotations

import json
import os
import subprocess
from hashlib import sha256
from pathlib import Path
from urllib.parse import urlparse

from tools.build.build_contract import (
    ContractError,
    acquisition_outcome,
    command_version,
    parse_module_graph,
    validate_acquisition_manifest,
    validate_nix_environment,
    validate_repository,
    validate_resolved_input_population,
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


def selected_module_graph(root: Path) -> set[str]:
    process = subprocess.run(
        [os.environ["ORUS_BAZEL"], "mod", "graph"],
        cwd=root,
        env=os.environ,
        check=False,
        capture_output=True,
        text=True,
        timeout=120,
    )
    if process.returncode:
        raise ContractError("BUILD_LOCK_INVALID", f"module graph failed: {process.stderr.strip()[-1000:]}")
    return parse_module_graph(process.stdout)


def _sha256_file(path: Path) -> str:
    digest = sha256()
    with path.open("rb") as source:
        while chunk := source.read(1024 * 1024):
            digest.update(chunk)
    return digest.hexdigest()


def _require_read_only(path: Path) -> None:
    resolved = path.resolve()
    if resolved.stat().st_mode & 0o222:
        raise ContractError("BUILD_ACQUISITION_DENIED", f"materialized input is writable: {resolved}")


def audit_bcr_materialization(root: Path, selected_modules: set[str]) -> dict[str, object]:
    population = validate_resolved_input_population(root, selected_modules=selected_modules)
    manifest = population["acquisition"]
    limits = manifest["limits"]
    distdir = Path(os.environ.get("ORUS_BCR_DISTDIR", ""))
    registry = Path(os.environ.get("ORUS_BCR_REGISTRY", ""))
    if not str(distdir).startswith("/nix/store/") or not str(registry).startswith("/nix/store/"):
        raise ContractError("BUILD_UNDECLARED_INPUT", "BCR inputs are not materialized through Nix")
    if not distdir.is_dir() or not registry.is_dir():
        raise ContractError("BUILD_ACQUISITION_DENIED", "BCR Nix materialization is absent")

    inventory_by_name = {entry["name"]: entry for entry in population["inventory"]["dependencies"]}
    archive_bytes = 0
    for row in population["archives"]:
        archive = distdir / Path(urlparse(row["url"]).path).name
        if not archive.is_file():
            raise ContractError("BUILD_ACQUISITION_DENIED", f"materialized archive is absent: {row['name']}")
        size = archive.stat().st_size
        archive_bytes += size
        if size > limits["per_blob_bytes"]:
            raise ContractError("BUILD_ACQUISITION_DENIED", f"materialized archive exceeds blob limit: {row['name']}")
        if _sha256_file(archive) != inventory_by_name[row["name"]]["sha256"]:
            raise ContractError("BUILD_ACQUISITION_DENIED", f"materialized archive digest differs: {row['name']}")
        _require_read_only(archive)
    if archive_bytes > limits["total_bytes"]:
        raise ContractError("BUILD_ACQUISITION_DENIED", "materialized archive population exceeds total limit")

    lock = json.loads((root / "MODULE.bazel.lock").read_text(encoding="utf-8"))
    locked_registry_files = {
        url.removeprefix("https://bcr.bazel.build/"): digest
        for url, digest in lock.get("registryFileHashes", {}).items()
        if url.startswith("https://bcr.bazel.build/")
    }
    if not locked_registry_files:
        raise ContractError("BUILD_LOCK_INVALID", "locked BCR metadata population is empty")
    for relative, digest in locked_registry_files.items():
        path = registry / relative
        if not path.is_file() or _sha256_file(path) != digest:
            raise ContractError("BUILD_LOCK_INVALID", f"materialized BCR metadata differs: {relative}")
        _require_read_only(path)
    _require_read_only(registry)

    return {
        "archive_bytes": archive_bytes,
        "archive_count": len(population["archives"]),
        "coordinate_count": len(manifest["admitted_inputs"]),
        "module_count": len(selected_modules),
        "registry_file_count": len(locked_registry_files),
        "store_mode": "read_only",
        "wall_limit_seconds": limits["wall_seconds"],
    }


def audit_nix_materialization(population: dict[str, object]) -> dict[str, object]:
    environment_by_name = {
        "bazel": "ORUS_BAZEL",
        "clang": "ORUS_CLANG",
        "gcc": "ORUS_GCC",
        "glaze": "ORUS_GLAZE_SRC",
        "google_benchmark": "ORUS_BENCHMARK_SRC",
        "googletest": "ORUS_GOOGLETEST_SRC",
        "lld": "ORUS_LLD",
        "llvm": "ORUS_LLVM",
        "nixpkgs": "ORUS_NIXPKGS_SRC",
        "openssl": "ORUS_OPENSSL",
        "openssl_dev": "ORUS_OPENSSL_DEV",
        "python": "ORUS_PYTHON",
        "utf8proc": "ORUS_UTF8PROC",
    }
    inventory = population["inventory"]
    nix_names = {
        entry["name"]
        for entry in inventory["dependencies"]
        if not str(entry["owner"]).startswith("bzlmod_")
    }
    if nix_names != set(environment_by_name):
        raise ContractError("BUILD_LOCK_INVALID", "Nix materialization map and inventory differ")
    for name, variable in environment_by_name.items():
        path = Path(os.environ.get(variable, ""))
        if not str(path).startswith("/nix/store/") or not path.exists():
            raise ContractError("BUILD_UNDECLARED_INPUT", f"{name} is not an actual Nix-store materialization")
        _require_read_only(path)
    return {"path_count": len(environment_by_name), "store_mode": "read_only"}


def main() -> int:
    try:
        root = workspace_root()
        validate_repository(root)
        validate_nix_environment(os.environ)
        selected_modules = selected_module_graph(root)
        materialization_audit = audit_bcr_materialization(root, selected_modules)
        population = validate_resolved_input_population(root, selected_modules=selected_modules)
        nix_materialization_audit = audit_nix_materialization(population)
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
            "acquisition_materialization": materialization_audit,
            "nix_materialization": nix_materialization_audit,
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
