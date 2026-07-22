"""Fail-closed bootstrap checks for the M0 build contract.

This module deliberately validates only M0-001's selected bytes, pins, and
configuration surface. The reusable canonical JSON and reference-environment
validator remains owned by M0-002.
"""

from __future__ import annotations

import hashlib
import json
import os
import re
import subprocess
import unicodedata
from dataclasses import dataclass
from pathlib import Path
from typing import Any, Iterable, Mapping, Sequence


class ContractError(RuntimeError):
    """A stable build-contract failure."""

    def __init__(self, code: str, detail: str):
        super().__init__(f"{code}: {detail}")
        self.code = code
        self.detail = detail


REQUIRED_ROOTS = (
    "flake.nix",
    "flake.lock",
    "MODULE.bazel",
    "MODULE.bazel.lock",
    "BUILD.bazel",
    ".bazelrc",
    ".bazelversion",
)
REQUIRED_CONFIGS = (
    "dev",
    "release",
    "asan",
    "ubsan",
    "tsan",
    "fuzz",
    "benchmark",
    "gcc",
)
REFERENCE_ROOT_FIELDS = {
    "schema",
    "environment_id",
    "nix_system",
    "support_level",
    "target_triple",
    "inputs",
    "tools",
    "host",
}
REFERENCE_TOOL_FIELDS = {"bazel", "clang", "llvm", "lld", "gcc"}
REFERENCE_HOST_FIELDS = {
    "os_family",
    "architecture",
    "kernel_release",
    "cpu_vendor",
    "libc_name",
    "libc_version",
    "cpu_family",
    "cpu_model",
    "required_isa",
}
HEX64 = re.compile(r"^[0-9a-f]{64}$")
TOKEN = re.compile(r"^[a-z0-9][a-z0-9._-]{0,63}$")


def workspace_root() -> Path:
    """Locate the repository both under `bazel run` and Bazel test runfiles."""

    candidates: list[Path] = []
    if value := os.environ.get("BUILD_WORKSPACE_DIRECTORY"):
        candidates.append(Path(value))
    if value := os.environ.get("TEST_SRCDIR"):
        source_root = Path(value)
        candidates.extend([source_root / "_main", source_root / "orus"])
    candidates.extend([Path.cwd(), Path(__file__).resolve().parents[2]])
    for candidate in candidates:
        if (candidate / "MODULE.bazel").is_file() and (candidate / "flake.nix").is_file():
            return candidate.resolve()
    raise ContractError("BUILD_LOCK_INVALID", "unable to locate repository root")


def _reject_duplicate(pairs: Sequence[tuple[str, Any]]) -> dict[str, Any]:
    result: dict[str, Any] = {}
    for key, value in pairs:
        if key in result:
            raise ContractError("BUILD_REFENV_FIELD_INVALID", f"duplicate object name {key!r}")
        result[key] = value
    return result


def _reject_float(value: str) -> None:
    raise ContractError("BUILD_REFENV_FIELD_INVALID", f"floating-point value {value!r} is forbidden")


def _reject_constant(value: str) -> None:
    raise ContractError("BUILD_REFENV_FIELD_INVALID", f"non-finite value {value!r} is forbidden")


def strict_json_loads(data: bytes) -> Any:
    if data.startswith(b"\xef\xbb\xbf"):
        raise ContractError("BUILD_REFENV_NONCANONICAL", "UTF-8 BOM is forbidden")
    try:
        text = data.decode("utf-8", errors="strict")
    except UnicodeDecodeError as error:
        raise ContractError("BUILD_REFENV_FIELD_INVALID", f"invalid UTF-8: {error}") from error
    try:
        value = json.loads(
            text,
            object_pairs_hook=_reject_duplicate,
            parse_float=_reject_float,
            parse_constant=_reject_constant,
        )
    except ContractError:
        raise
    except json.JSONDecodeError as error:
        raise ContractError("BUILD_REFENV_FIELD_INVALID", f"malformed JSON: {error.msg}") from error
    _validate_json_value(value)
    return value


def _validate_json_value(value: Any) -> None:
    if isinstance(value, str):
        if unicodedata.normalize("NFC", value) != value:
            raise ContractError("BUILD_REFENV_NONCANONICAL", "all strings must be NFC")
    elif isinstance(value, bool) or value is None:
        return
    elif isinstance(value, int):
        if value < -(2**63) or value > 2**63 - 1:
            raise ContractError("BUILD_REFENV_FIELD_INVALID", "integer is outside signed 64-bit")
    elif isinstance(value, list):
        for item in value:
            _validate_json_value(item)
    elif isinstance(value, dict):
        for key, item in value.items():
            _validate_json_value(key)
            _validate_json_value(item)
    else:
        raise ContractError("BUILD_REFENV_FIELD_INVALID", f"unsupported JSON value {type(value).__name__}")


def canonical_json(value: Any) -> bytes:
    _validate_json_value(value)
    return json.dumps(
        value,
        ensure_ascii=False,
        allow_nan=False,
        separators=(",", ":"),
        sort_keys=True,
    ).encode("utf-8")


def load_canonical_json(path: Path, *, max_bytes: int | None = None) -> Any:
    data = path.read_bytes()
    if max_bytes is not None and len(data) > max_bytes:
        raise ContractError("BUILD_REFENV_FIELD_INVALID", f"{path} exceeds {max_bytes} bytes")
    value = strict_json_loads(data)
    if canonical_json(value) != data:
        raise ContractError("BUILD_REFENV_NONCANONICAL", f"{path} is not canonical JSON")
    return value


def derive_environment_id(document: Mapping[str, Any]) -> str:
    payload = dict(document)
    payload.pop("environment_id", None)
    return hashlib.sha256(canonical_json(payload)).hexdigest()


def validate_reference_bootstrap(path: Path, expected_inputs: Iterable[str]) -> dict[str, Any]:
    document = load_canonical_json(path, max_bytes=64 * 1024)
    if not isinstance(document, dict) or set(document) != REFERENCE_ROOT_FIELDS:
        raise ContractError("BUILD_REFENV_FIELD_INVALID", "reference root fields are incomplete or unknown")
    if document["schema"] != "M0-REFENV-v1":
        raise ContractError("BUILD_REFENV_SCHEMA_UNKNOWN", str(document["schema"]))
    if document["nix_system"] != "x86_64-linux" or document["support_level"] != "validated_reference":
        raise ContractError("BUILD_REFENV_FIELD_INVALID", "reference support identity is not narrow and exact")
    if document["target_triple"] != "x86_64-unknown-linux-gnu":
        raise ContractError("BUILD_REFENV_FIELD_INVALID", "unexpected target triple")
    if not HEX64.fullmatch(str(document["environment_id"])):
        raise ContractError("BUILD_REFENV_FIELD_INVALID", "environment_id must be lowercase hex64")
    derived = derive_environment_id(document)
    if document["environment_id"] != derived:
        raise ContractError("BUILD_REFENV_ID_MISMATCH", f"expected {derived}")

    inputs = document["inputs"]
    if not isinstance(inputs, list) or not (1 <= len(inputs) <= 128):
        raise ContractError("BUILD_REFENV_FIELD_INVALID", "inputs must contain 1..128 rows")
    names = [row.get("name") for row in inputs if isinstance(row, dict)]
    if names != sorted(names) or len(names) != len(set(names)):
        raise ContractError("BUILD_REFENV_FIELD_INVALID", "input rows must be sorted and unique")
    if set(names) != set(expected_inputs):
        raise ContractError("BUILD_REFENV_FIELD_INVALID", "reference inputs do not match flake inputs")
    for row in inputs:
        if set(row) != {"name", "coordinate", "sha256"}:
            raise ContractError("BUILD_REFENV_FIELD_INVALID", "input row fields are invalid")
        if not TOKEN.fullmatch(str(row["name"])) or not HEX64.fullmatch(str(row["sha256"])):
            raise ContractError("BUILD_REFENV_FIELD_INVALID", "input identity is malformed")
        if not 1 <= len(str(row["coordinate"]).encode()) <= 256:
            raise ContractError("BUILD_REFENV_FIELD_INVALID", "input coordinate bound is invalid")

    tools = document["tools"]
    if not isinstance(tools, dict) or set(tools) != REFERENCE_TOOL_FIELDS:
        raise ContractError("BUILD_REFENV_FIELD_INVALID", "tool inventory must contain exactly five tools")
    for name, tool in tools.items():
        if set(tool) != {"version", "artifact_sha256"} or not HEX64.fullmatch(str(tool["artifact_sha256"])):
            raise ContractError("BUILD_REFENV_FIELD_INVALID", f"invalid {name} tool identity")

    host = document["host"]
    if not isinstance(host, dict) or set(host) != REFERENCE_HOST_FIELDS:
        raise ContractError("BUILD_REFENV_FIELD_INVALID", "host predicates must contain exactly nine leaves")
    for name, predicate in host.items():
        expected_op = "set_contains_all" if name == "required_isa" else "u32_eq" if name in {"cpu_family", "cpu_model"} else "eq"
        if set(predicate) != {"op", "expected"} or predicate["op"] != expected_op:
            raise ContractError("BUILD_REFENV_OPERATOR_UNKNOWN", name)
    return document


def parse_bazel_configs(path: Path) -> set[str]:
    configs: set[str] = set()
    pattern = re.compile(r"^(?:common|build|test|run):([a-z0-9_-]+)\s")
    for line in path.read_text(encoding="utf-8").splitlines():
        if match := pattern.match(line):
            configs.add(match.group(1))
    return configs


def validate_dependency_inventory(path: Path) -> dict[str, Any]:
    inventory = load_canonical_json(path)
    if inventory.get("schema") != "M0-DEPENDENCY-INVENTORY-v1":
        raise ContractError("BUILD_LOCK_INVALID", "unknown dependency inventory schema")
    entries = inventory.get("dependencies")
    if not isinstance(entries, list) or not entries:
        raise ContractError("BUILD_LOCK_INVALID", "dependency inventory is empty")
    names = [entry.get("name") for entry in entries]
    if names != sorted(names) or len(names) != len(set(names)):
        raise ContractError("BUILD_LOCK_INVALID", "dependency inventory must be sorted and unique")
    for entry in entries:
        required = {"name", "version", "coordinate", "sha256", "owner"}
        if set(entry) != required or not HEX64.fullmatch(str(entry["sha256"])):
            raise ContractError("BUILD_LOCK_INVALID", f"invalid dependency row {entry.get('name')!r}")
        if not entry["version"] or not entry["coordinate"] or not entry["owner"]:
            raise ContractError("BUILD_LOCK_INVALID", f"incomplete dependency row {entry['name']!r}")
    return inventory


def validate_admissions(path: Path, inventory: Mapping[str, Any]) -> None:
    admissions = load_canonical_json(path)
    if admissions.get("schema") != "M0-BOOTSTRAP-ADMISSIONS-v1":
        raise ContractError("BUILD_LOCK_INVALID", "unknown admission schema")
    records = admissions.get("records")
    dependency_names = {row["name"] for row in inventory["dependencies"]}
    record_names = {record.get("name") for record in records or []}
    if dependency_names != record_names:
        raise ContractError("BUILD_LOCK_INVALID", "admissions do not match the dependency inventory")
    required = {
        "name",
        "purpose",
        "license",
        "health",
        "closure_abi_build_cost",
        "containment",
        "alternatives",
        "rollback_removal",
    }
    for record in records:
        if set(record) != required or any(not record[field] for field in required):
            raise ContractError("BUILD_LOCK_INVALID", f"incomplete admission for {record.get('name')!r}")


def validate_configuration_applicability(path: Path) -> None:
    applicability = load_canonical_json(path)
    if applicability.get("schema") != "M0-001-CONFIGURATION-APPLICABILITY-v1":
        raise ContractError("BUILD_CONFIG_INVALID", "unknown configuration applicability schema")
    rows = applicability.get("configurations")
    if not isinstance(rows, list):
        raise ContractError("BUILD_CONFIG_INVALID", "configuration applicability rows are absent")
    names = [row.get("name") for row in rows if isinstance(row, dict)]
    if names != sorted(REQUIRED_CONFIGS) or len(names) != len(set(names)):
        raise ContractError("BUILD_CONFIG_INVALID", "configuration applicability is incomplete or unsorted")
    for row in rows:
        status = row.get("status")
        if status == "required":
            if set(row) != {"name", "scope", "status"} or not row["scope"]:
                raise ContractError("BUILD_CONFIG_INVALID", f"required configuration row is invalid: {row!r}")
        elif status == "not_applicable":
            if set(row) != {"name", "scope", "status", "reason"} or not row["scope"] or not row["reason"]:
                raise ContractError("BUILD_CONFIG_INVALID", f"not-applicable row lacks a scoped reason: {row!r}")
        else:
            raise ContractError("BUILD_CONFIG_INVALID", f"unknown configuration status: {status!r}")


def validate_wrapper_map(path: Path, root: Path) -> None:
    mapping = load_canonical_json(path)
    if mapping.get("schema") != "M0-WRAPPER-MAP-v1":
        raise ContractError("BUILD_PROHIBITED_PATH", "unknown wrapper map schema")
    wrappers = mapping.get("wrappers")
    if not isinstance(wrappers, list) or not wrappers:
        raise ContractError("BUILD_PROHIBITED_PATH", "wrapper map is empty")
    for wrapper in wrappers:
        if set(wrapper) != {"path", "role", "canonical_entry_points"}:
            raise ContractError("BUILD_PROHIBITED_PATH", "wrapper mapping is incomplete")
        if wrapper["role"] != "bazel_toolchain_driver" or not wrapper["canonical_entry_points"]:
            raise ContractError("BUILD_PROHIBITED_PATH", "wrapper role or entry points are invalid")
        for command in wrapper["canonical_entry_points"]:
            if not command.startswith("nix develop --command bazel "):
                raise ContractError("BUILD_PROHIBITED_PATH", f"independent command {command!r}")
    mapped = {wrapper["path"] for wrapper in wrappers}
    executable = {
        str(candidate.relative_to(root))
        for candidate in (root / "toolchains/bin").iterdir()
        if candidate.is_file() and candidate.stat().st_mode & 0o111
    }
    if mapped != executable:
        raise ContractError("BUILD_PROHIBITED_PATH", "executable toolchain drivers and wrapper map differ")


@dataclass(frozen=True)
class Finding:
    path: str
    rule: str
    detail: str


PROHIBITED_FILENAMES = {
    "CMakeLists.txt",
    "CMakePresets.json",
    "CMakeUserPresets.json",
    "CMakeCache.txt",
    "cmake_install.cmake",
}
PROHIBITED_COMMAND = re.compile(
    r"(?:^|[`$;&|]\s*)cmake\s+(?:--build|--install|-S|-B|\.|/)",
    re.IGNORECASE,
)


def scan_prohibited(root: Path, *, exclude_fixtures: bool) -> list[Finding]:
    findings: list[Finding] = []
    excluded_parts = {".git", ".factory", ".agents", "bazel-bin", "bazel-out", "bazel-testlogs", "bazel-orus"}
    for path in sorted(root.rglob("*")):
        relative = path.relative_to(root)
        if excluded_parts.intersection(relative.parts) or relative.parts[:1] == ("specs",):
            continue
        if exclude_fixtures and relative.parts[:3] == ("tests", "build", "fixtures"):
            continue
        if path.name in PROHIBITED_FILENAMES:
            findings.append(Finding(str(relative), "prohibited_filename", path.name))
            continue
        if not path.is_file() or (path.is_symlink() and exclude_fixtures) or path.stat().st_size > 1024 * 1024:
            continue
        if relative == Path("tools/build/build_contract.py"):
            continue
        try:
            text = path.read_text(encoding="utf-8")
        except UnicodeDecodeError:
            continue
        for number, line in enumerate(text.splitlines(), start=1):
            if PROHIBITED_COMMAND.search(line):
                findings.append(Finding(str(relative), "prohibited_invocation", f"line {number}"))
    return findings


def acquisition_outcome(
    manifest: Mapping[str, Any],
    *,
    coordinate: str,
    blob_bytes: int,
    total_bytes: int,
    elapsed_seconds: int,
    observed_sha256: str,
    mutable: bool = False,
) -> str:
    limits = manifest["limits"]
    admitted = {entry["coordinate"]: entry for entry in manifest["admitted_inputs"]}
    entry = admitted.get(coordinate)
    if entry is None or mutable:
        return "BUILD_ACQUISITION_DENIED"
    if observed_sha256 != entry["sha256"]:
        return "BUILD_ACQUISITION_DENIED"
    if blob_bytes > limits["per_blob_bytes"] or total_bytes > limits["total_bytes"]:
        return "BUILD_ACQUISITION_DENIED"
    if elapsed_seconds > limits["wall_seconds"]:
        return "BUILD_ACQUISITION_DENIED"
    if len(admitted) > limits["coordinates"]:
        return "BUILD_ACQUISITION_DENIED"
    return "promoted_read_only"


def validate_acquisition_manifest(path: Path) -> dict[str, Any]:
    manifest = load_canonical_json(path)
    expected_limits = {
        "coordinates": 128,
        "per_blob_bytes": 4 * 1024**3,
        "total_bytes": 16 * 1024**3,
        "wall_seconds": 1200,
    }
    if manifest.get("schema") != "M0-INPUT-ACQUISITION-v1" or manifest.get("profile") != "m0_input_acquisition":
        raise ContractError("BUILD_ACQUISITION_DENIED", "unknown acquisition profile")
    if manifest.get("limits") != expected_limits:
        raise ContractError("BUILD_ACQUISITION_DENIED", "acquisition limits differ from the contract")
    if manifest.get("capabilities") != ["network.client"] or manifest.get("credentials") is not False:
        raise ContractError("BUILD_ACQUISITION_DENIED", "acquisition capabilities are too broad")
    for entry in manifest.get("admitted_inputs", []):
        if set(entry) != {"coordinate", "sha256", "store_mode"} or entry["store_mode"] != "read_only":
            raise ContractError("BUILD_ACQUISITION_DENIED", "invalid admitted input")
        if not HEX64.fullmatch(str(entry["sha256"])):
            raise ContractError("BUILD_ACQUISITION_DENIED", "invalid admitted digest")
    return manifest


def validate_nix_environment(environment: Mapping[str, str]) -> None:
    required = {
        "ORUS_BAZEL",
        "ORUS_CLANG",
        "ORUS_CLANGXX",
        "ORUS_GCC",
        "ORUS_GCC_LIB",
        "ORUS_GXX",
        "ORUS_LLD",
        "ORUS_LLVM",
        "ORUS_PYTHON",
        "ORUS_OPENSSL",
        "ORUS_OPENSSL_DEV",
        "ORUS_UTF8PROC",
    }
    for name in required:
        value = environment.get(name, "")
        if not value.startswith("/nix/store/"):
            raise ContractError("BUILD_UNDECLARED_INPUT", f"{name} is not a Nix-store path")
    if environment.get("ORUS_NETWORK_ALLOWED") != "0":
        raise ContractError("BUILD_UNDECLARED_INPUT", "build action network must be denied")


def command_version(command: Sequence[str]) -> str:
    process = subprocess.run(command, check=False, text=True, capture_output=True, timeout=10)
    if process.returncode != 0:
        raise ContractError("BUILD_CONFIG_INVALID", f"{command[0]} failed: {process.stderr.strip()}")
    return (process.stdout or process.stderr).splitlines()[0]


def validate_repository(root: Path) -> None:
    for relative in REQUIRED_ROOTS:
        if not (root / relative).is_file():
            raise ContractError("BUILD_LOCK_INVALID", f"missing {relative}")
    if (root / ".bazelversion").read_text(encoding="utf-8").strip() != "8.6.0":
        raise ContractError("BUILD_LOCK_INVALID", "unexpected Bazel version")
    configs = parse_bazel_configs(root / ".bazelrc")
    missing_configs = set(REQUIRED_CONFIGS) - configs
    if missing_configs:
        raise ContractError("BUILD_CONFIG_INVALID", f"missing configurations {sorted(missing_configs)}")
    inventory = validate_dependency_inventory(root / "config/dependency-inventory.json")
    validate_admissions(root / "third_party/bootstrap-admissions.json", inventory)
    validate_configuration_applicability(root / "config/build-applicability.json")
    validate_wrapper_map(root / "config/wrapper-map.json", root)
    expected_inputs = inventory["reference_inputs"]
    validate_reference_bootstrap(root / "config/m0-reference-environment.json", expected_inputs)
    validate_acquisition_manifest(root / "config/input-acquisition.json")
    findings = scan_prohibited(root, exclude_fixtures=True)
    if findings:
        raise ContractError("BUILD_PROHIBITED_PATH", "; ".join(f"{item.path}:{item.rule}" for item in findings))
