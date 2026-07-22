"""Independent Python parity oracle for M0 canonical bytes and statistics.

This module is intentionally cold-path only. Native or otherwise untrusted
bytes remain authoritative only after the C++ contract boundary accepts them.
"""

from __future__ import annotations

from dataclasses import dataclass
import hashlib
import json
import unicodedata
from typing import Any, Iterable


@dataclass(frozen=True)
class ContractError(ValueError):
    code: str
    path: str
    message: str

    def __str__(self) -> str:
        return f"{self.code}:{self.path}:{self.message}"


def _pairs(pairs: list[tuple[str, Any]]) -> dict[str, Any]:
    result: dict[str, Any] = {}
    for name, value in pairs:
        if name in result:
            raise ContractError("CANONICAL_JSON_NONCANONICAL", "$", "duplicate object name")
        result[name] = value
    return result


def _integer(token: str) -> int:
    value = int(token)
    if value < -(2**63) or value > 2**63 - 1:
        raise ContractError("CANONICAL_JSON_NONCANONICAL", "$", "integer outside signed 64-bit")
    return value


def _float(_: str) -> float:
    raise ContractError("CANONICAL_JSON_NONCANONICAL", "$", "floating-point value is forbidden")


def _constant(_: str) -> float:
    raise ContractError("CANONICAL_JSON_NONCANONICAL", "$", "non-finite value is forbidden")


def _validate(value: Any, path: str = "$", depth: int = 1) -> None:
    if depth > 16:
        raise ContractError("CANONICAL_JSON_RESOURCE_LIMIT", path, "depth exceeds 16")
    if isinstance(value, str):
        try:
            value.encode("utf-8", errors="strict")
        except UnicodeEncodeError as error:
            raise ContractError("CANONICAL_JSON_NONCANONICAL", path, "invalid Unicode scalar") from error
        if unicodedata.normalize("NFC", value) != value:
            raise ContractError("CANONICAL_JSON_NONCANONICAL", path, "string is not NFC")
    elif isinstance(value, bool) or value is None:
        return
    elif isinstance(value, int):
        if value < -(2**63) or value > 2**63 - 1:
            raise ContractError("CANONICAL_JSON_NONCANONICAL", path, "integer outside signed 64-bit")
    elif isinstance(value, list):
        for index, member in enumerate(value):
            _validate(member, f"{path}[{index}]", depth + 1)
    elif isinstance(value, dict):
        for name, member in value.items():
            _validate(name, path, depth + 1)
            _validate(member, f"{path}.{name}", depth + 1)
    else:
        raise ContractError("CANONICAL_JSON_NONCANONICAL", path, "unsupported JSON value")


def canonical_json(value_or_bytes: Any, maximum_bytes: int = 16 * 1024 * 1024) -> bytes:
    """Return exact M0 canonical bytes, rejecting noncanonical byte inputs."""

    original: bytes | None = None
    value = value_or_bytes
    if isinstance(value_or_bytes, (bytes, bytearray, memoryview)):
        original = bytes(value_or_bytes)
        if len(original) > maximum_bytes:
            raise ContractError("CANONICAL_JSON_RESOURCE_LIMIT", "$", "document byte limit exceeded")
        if original.startswith(b"\xef\xbb\xbf"):
            raise ContractError("CANONICAL_JSON_NONCANONICAL", "$", "BOM is forbidden")
        try:
            text = original.decode("utf-8", errors="strict")
        except UnicodeDecodeError as error:
            raise ContractError("CANONICAL_JSON_NONCANONICAL", "$", "invalid UTF-8") from error
        try:
            value = json.loads(
                text,
                object_pairs_hook=_pairs,
                parse_int=_integer,
                parse_float=_float,
                parse_constant=_constant,
            )
        except ContractError:
            raise
        except (json.JSONDecodeError, ValueError) as error:
            raise ContractError("CANONICAL_JSON_NONCANONICAL", "$", "invalid JSON") from error

    _validate(value)
    try:
        encoded = json.dumps(
            value,
            ensure_ascii=False,
            allow_nan=False,
            separators=(",", ":"),
            sort_keys=True,
        ).encode("utf-8")
    except (TypeError, ValueError, UnicodeEncodeError) as error:
        raise ContractError("CANONICAL_JSON_NONCANONICAL", "$", "cannot emit canonical JSON") from error
    if len(encoded) > maximum_bytes:
        raise ContractError("CANONICAL_JSON_RESOURCE_LIMIT", "$", "document byte limit exceeded")
    if original is not None and encoded != original:
        raise ContractError("CANONICAL_JSON_NONCANONICAL", "$", "input bytes are not canonical")
    return encoded


def sha256_hex(data: bytes | bytearray | memoryview) -> str:
    return hashlib.sha256(bytes(data)).hexdigest()


def _median(values: list[int]) -> int:
    if not values:
        raise ContractError("PERF_RELATIONSHIP_INVALID", "$.statistics", "empty population")
    ordered = sorted(values)
    middle = len(ordered) // 2
    if len(ordered) % 2:
        return ordered[middle]
    total = ordered[middle - 1] + ordered[middle]
    quotient = abs(total) // 2
    if abs(total) % 2:
        quotient += 1
    return -quotient if total < 0 else quotient


def statistics(values: Iterable[int], percentiles_ppm: Iterable[int] = ()) -> dict[str, Any]:
    population = list(values)
    if any(isinstance(value, bool) or value < -(2**63) or value > 2**63 - 1 for value in population):
        raise ContractError("PERF_INTEGER_OVERFLOW", "$.statistics", "invalid signed integer")
    ordered = sorted(population)
    median = _median(ordered)
    deviations: list[int] = []
    for value in ordered:
        deviation = abs(value - median)
        if deviation > 2**63 - 1:
            raise ContractError("PERF_INTEGER_OVERFLOW", "$.statistics", "MAD overflow")
        deviations.append(deviation)
    percentile_rows: list[dict[str, int]] = []
    prior = 0
    for rank in percentiles_ppm:
        if rank <= prior or rank < 1 or rank > 999_999:
            raise ContractError("PERF_FIELD_BOUND", "$.statistics.percentiles", "invalid rank")
        prior = rank
        position = max(1, min(len(ordered), (rank * len(ordered) + 999_999) // 1_000_000))
        percentile_rows.append({"rank_ppm": rank, "value": ordered[position - 1]})
    return {
        "maximum": ordered[-1],
        "median": median,
        "median_absolute_deviation": _median(deviations),
        "minimum": ordered[0],
        "percentiles": percentile_rows,
    }
