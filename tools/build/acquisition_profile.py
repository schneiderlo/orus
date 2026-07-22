"""Transactional implementation of the bounded M0 input-acquisition profile."""

from __future__ import annotations

import argparse
import json
import os
import shutil
import tarfile
import tempfile
import time
from dataclasses import dataclass
from hashlib import sha256
from pathlib import Path, PurePosixPath
from typing import Callable, Mapping, Protocol, Sequence
from urllib.parse import urlparse
from urllib.request import urlopen

from tools.build.build_contract import (
    ContractError,
    validate_acquisition_manifest,
    validate_resolved_input_population,
)


@dataclass(frozen=True)
class AcquisitionRequest:
    coordinate: str
    name: str
    role: str
    sha256: str
    url: str
    mutable: bool = False


@dataclass(frozen=True)
class TransferEvidence:
    bytes_fetched: int
    sha256: str


class Transport(Protocol):
    def fetch(
        self,
        request: AcquisitionRequest,
        destination: Path,
        *,
        byte_limit: int,
        seconds_remaining: float,
    ) -> TransferEvidence: ...


class UrlTransport:
    """Streaming production transport; bytes remain in quarantine."""

    def fetch(
        self,
        request: AcquisitionRequest,
        destination: Path,
        *,
        byte_limit: int,
        seconds_remaining: float,
    ) -> TransferEvidence:
        digest = sha256()
        size = 0
        try:
            with urlopen(request.url, timeout=max(seconds_remaining, 0.001)) as source, destination.open("xb") as sink:
                while chunk := source.read(1024 * 1024):
                    size += len(chunk)
                    if size > byte_limit:
                        raise ContractError(
                            "BUILD_ACQUISITION_DENIED",
                            f"{request.coordinate} exceeds the remaining byte budget",
                        )
                    sink.write(chunk)
                    digest.update(chunk)
        except ContractError:
            raise
        except (OSError, TimeoutError) as error:
            raise ContractError(
                "BUILD_ACQUISITION_DENIED",
                f"fetch failed for {request.coordinate}: {error}",
            ) from error
        return TransferEvidence(bytes_fetched=size, sha256=digest.hexdigest())


class ProfileAcquirer:
    """One fail-closed admission, quarantine, verification, and promotion owner."""

    def __init__(
        self,
        manifest: Mapping[str, object],
        requests: Sequence[AcquisitionRequest],
        transport: Transport,
        *,
        clock: Callable[[], float] = time.monotonic,
    ) -> None:
        self.manifest = manifest
        self.requests = tuple(requests)
        self.transport = transport
        self.clock = clock

    def _preflight(self) -> None:
        limits = self.manifest["limits"]
        admitted_rows = self.manifest["admitted_inputs"]
        assert isinstance(limits, Mapping)
        assert isinstance(admitted_rows, list)
        admitted = {str(row["coordinate"]): row for row in admitted_rows}
        if len(admitted) > int(limits["coordinates"]):
            raise ContractError("BUILD_ACQUISITION_DENIED", "coordinate limit exceeded before transport")
        coordinates = [request.coordinate for request in self.requests]
        names = [request.name for request in self.requests]
        output_names = [Path(urlparse(request.url).path).name for request in self.requests if request.role != "registry"]
        if len(coordinates) != len(set(coordinates)) or len(names) != len(set(names)):
            raise ContractError("BUILD_ACQUISITION_DENIED", "requested acquisition population is not unique")
        if len(output_names) != len(set(output_names)):
            raise ContractError("BUILD_ACQUISITION_DENIED", "distdir output names are not unique")
        for request in self.requests:
            admission = admitted.get(request.coordinate)
            if admission is None or request.mutable:
                raise ContractError("BUILD_ACQUISITION_DENIED", f"unadmitted or mutable input: {request.coordinate}")
            if request.sha256 != admission["sha256"] or admission["store_mode"] != "read_only":
                raise ContractError("BUILD_ACQUISITION_DENIED", f"admission differs for {request.coordinate}")
            if request.role not in {"asset", "module", "registry"}:
                raise ContractError("BUILD_ACQUISITION_DENIED", f"invalid acquisition role: {request.role}")

    @staticmethod
    def _extract_registry(source: Path, destination: Path) -> None:
        with tarfile.open(source, mode="r:*") as archive:
            members = archive.getmembers()
            roots = {PurePosixPath(member.name).parts[0] for member in members if PurePosixPath(member.name).parts}
            if len(roots) != 1:
                raise ContractError("BUILD_ACQUISITION_DENIED", "registry archive has an invalid root")
            root = next(iter(roots))
            for member in members:
                parts = PurePosixPath(member.name).parts
                relative = PurePosixPath(*parts[1:]) if parts and parts[0] == root else PurePosixPath()
                if not relative.parts:
                    continue
                if relative.is_absolute() or ".." in relative.parts or not (member.isdir() or member.isfile()):
                    raise ContractError("BUILD_ACQUISITION_DENIED", "registry archive contains an unsafe member")
                target = destination.joinpath(*relative.parts)
                if member.isdir():
                    target.mkdir(parents=True, exist_ok=True)
                    continue
                target.parent.mkdir(parents=True, exist_ok=True)
                extracted = archive.extractfile(member)
                if extracted is None:
                    raise ContractError("BUILD_ACQUISITION_DENIED", "registry file cannot be extracted")
                with extracted, target.open("xb") as sink:
                    shutil.copyfileobj(extracted, sink)

    @staticmethod
    def _make_read_only(root: Path) -> None:
        for path in sorted(root.rglob("*"), reverse=True):
            os.chmod(path, 0o555 if path.is_dir() else 0o444)
            os.utime(path, (1, 1), follow_symlinks=False)
        os.chmod(root, 0o555)
        os.utime(root, (1, 1))

    def _promote(self, quarantine: Path, output: Path, report: Mapping[str, object]) -> None:
        if output.exists():
            raise ContractError("BUILD_ACQUISITION_DENIED", f"promotion target already exists: {output}")
        try:
            (output / "distdir").mkdir(parents=True)
            for request in self.requests:
                source = quarantine / request.name
                if request.role == "registry":
                    self._extract_registry(source, output / "registry")
                else:
                    shutil.copyfile(source, output / "distdir" / Path(urlparse(request.url).path).name)
            (output / "profile-report.json").write_text(
                json.dumps(report, separators=(",", ":"), sort_keys=True),
                encoding="utf-8",
            )
            self._make_read_only(output)
        except Exception:
            shutil.rmtree(output, ignore_errors=True)
            raise

    def acquire(self, output: Path) -> dict[str, object]:
        self._preflight()
        limits = self.manifest["limits"]
        assert isinstance(limits, Mapping)
        started = self.clock()
        total_bytes = 0
        with tempfile.TemporaryDirectory(prefix="orus-m0-acquisition-") as directory:
            quarantine = Path(directory)
            try:
                for request in self.requests:
                    elapsed = self.clock() - started
                    remaining_seconds = float(limits["wall_seconds"]) - elapsed
                    remaining_bytes = int(limits["total_bytes"]) - total_bytes
                    if remaining_seconds < 0 or remaining_bytes < 0:
                        raise ContractError("BUILD_ACQUISITION_DENIED", "whole-profile budget exhausted")
                    evidence = self.transport.fetch(
                        request,
                        quarantine / request.name,
                        byte_limit=min(int(limits["per_blob_bytes"]), remaining_bytes),
                        seconds_remaining=remaining_seconds,
                    )
                    total_bytes += evidence.bytes_fetched
                    if evidence.bytes_fetched > int(limits["per_blob_bytes"]):
                        raise ContractError("BUILD_ACQUISITION_DENIED", "per-blob limit exceeded")
                    if total_bytes > int(limits["total_bytes"]):
                        raise ContractError("BUILD_ACQUISITION_DENIED", "whole-profile byte limit exceeded")
                    if self.clock() - started > float(limits["wall_seconds"]):
                        raise ContractError("BUILD_ACQUISITION_DENIED", "whole-profile deadline exceeded")
                    if evidence.sha256 != request.sha256:
                        raise ContractError("BUILD_ACQUISITION_DENIED", f"digest mismatch for {request.coordinate}")
                elapsed = self.clock() - started
                report: dict[str, object] = {
                    "coordinates": [request.coordinate for request in self.requests],
                    "deadline_enforced": True,
                    "profile": self.manifest["profile"],
                    "schema": "M0-ACQUISITION-TRANSACTION-v1",
                    "state": "promoted_read_only",
                    "total_bytes": total_bytes,
                    "wall_limit_seconds": limits["wall_seconds"],
                }
                self._promote(quarantine, output, report)
                return report | {"elapsed_seconds": elapsed}
            except ContractError:
                shutil.rmtree(output, ignore_errors=True)
                raise


class VirtualClock:
    def __init__(self) -> None:
        self.value = 0.0

    def __call__(self) -> float:
        return self.value

    def advance(self, seconds: float) -> None:
        self.value += seconds


class SyntheticTransport:
    """Bounded fixture transport that drives the production state machine."""

    def __init__(
        self,
        evidence: Mapping[str, TransferEvidence],
        clock: VirtualClock,
        elapsed: Mapping[str, float] | None = None,
    ) -> None:
        self.evidence = evidence
        self.clock = clock
        self.elapsed = elapsed or {}
        self.calls: list[str] = []

    def fetch(
        self,
        request: AcquisitionRequest,
        destination: Path,
        *,
        byte_limit: int,
        seconds_remaining: float,
    ) -> TransferEvidence:
        del byte_limit, seconds_remaining
        self.calls.append(request.coordinate)
        destination.write_bytes(b"bounded-synthetic-transport\n")
        self.clock.advance(self.elapsed.get(request.coordinate, 0.0))
        return self.evidence[request.coordinate]


def _synthetic_manifest(count: int, digest: str) -> dict[str, object]:
    return {
        "admitted_inputs": [
            {"coordinate": f"fixture:{index:03}", "sha256": digest, "store_mode": "read_only"}
            for index in range(count)
        ],
        "capabilities": ["network.client"],
        "credentials": False,
        "limits": {
            "coordinates": 128,
            "per_blob_bytes": 4 * 1024**3,
            "total_bytes": 16 * 1024**3,
            "wall_seconds": 1200,
        },
        "profile": "m0_input_acquisition",
        "schema": "M0-INPUT-ACQUISITION-v1",
    }


def _synthetic_requests(count: int, digest: str) -> list[AcquisitionRequest]:
    return [
        AcquisitionRequest(
            coordinate=f"fixture:{index:03}",
            name=f"fixture-{index:03}",
            role="asset",
            sha256=digest,
            url=f"https://fixture.invalid/fixture-{index:03}.tar.gz",
        )
        for index in range(count)
    ]


def run_boundary_fixtures() -> dict[str, dict[str, object]]:
    digest = "a" * 64

    def run_case(
        manifest: Mapping[str, object],
        requests: Sequence[AcquisitionRequest],
        evidence: Mapping[str, TransferEvidence],
        *,
        elapsed: Mapping[str, float] | None = None,
    ) -> dict[str, object]:
        clock = VirtualClock()
        transport = SyntheticTransport(evidence, clock, elapsed)
        with tempfile.TemporaryDirectory(prefix="orus-m0-boundary-") as directory:
            output = Path(directory) / "promoted"
            try:
                ProfileAcquirer(manifest, requests, transport, clock=clock).acquire(output)
                outcome = "promoted_read_only"
            except ContractError as error:
                outcome = error.code
            return {
                "outcome": outcome,
                "promoted": output.exists(),
                "transport_calls": len(transport.calls),
                "transport": "bounded_synthetic",
            }

    def evidence_for(requests: Sequence[AcquisitionRequest], sizes: Sequence[int]) -> dict[str, TransferEvidence]:
        return {
            request.coordinate: TransferEvidence(bytes_fetched=size, sha256=request.sha256)
            for request, size in zip(requests, sizes, strict=True)
        }

    one_manifest = _synthetic_manifest(1, digest)
    one = _synthetic_requests(1, digest)
    exact_coordinates = _synthetic_requests(128, digest)
    first_over_coordinates = _synthetic_requests(129, digest)
    exact_total = _synthetic_requests(4, digest)
    first_over_total = _synthetic_requests(5, digest)
    unadmitted = [AcquisitionRequest("fixture:unadmitted", "unadmitted", "asset", digest, "https://fixture.invalid/u")]
    mutable = [AcquisitionRequest(one[0].coordinate, one[0].name, one[0].role, digest, one[0].url, mutable=True)]

    matrix = {
        "coordinates_exact": run_case(
            _synthetic_manifest(128, digest),
            exact_coordinates,
            evidence_for(exact_coordinates, [0] * 128),
        ),
        "coordinates_first_over": run_case(
            _synthetic_manifest(129, digest),
            first_over_coordinates,
            evidence_for(first_over_coordinates, [0] * 129),
        ),
        "blob_exact": run_case(one_manifest, one, evidence_for(one, [4 * 1024**3])),
        "blob_first_over": run_case(one_manifest, one, evidence_for(one, [4 * 1024**3 + 1])),
        "total_exact": run_case(
            _synthetic_manifest(4, digest),
            exact_total,
            evidence_for(exact_total, [4 * 1024**3] * 4),
        ),
        "total_first_over": run_case(
            _synthetic_manifest(5, digest),
            first_over_total,
            evidence_for(first_over_total, [4 * 1024**3] * 4 + [1]),
        ),
        "time_exact": run_case(
            one_manifest,
            one,
            evidence_for(one, [1]),
            elapsed={one[0].coordinate: 1200},
        ),
        "time_first_over": run_case(
            one_manifest,
            one,
            evidence_for(one, [1]),
            elapsed={one[0].coordinate: 1201},
        ),
        "hash_mismatch": run_case(
            one_manifest,
            one,
            {one[0].coordinate: TransferEvidence(bytes_fetched=1, sha256="b" * 64)},
        ),
        "mutable": run_case(one_manifest, mutable, evidence_for(mutable, [1])),
        "unadmitted": run_case(one_manifest, unadmitted, evidence_for(unadmitted, [1])),
    }
    positive = {"coordinates_exact", "blob_exact", "total_exact", "time_exact"}
    for name, result in matrix.items():
        expected = "promoted_read_only" if name in positive else "BUILD_ACQUISITION_DENIED"
        if result["outcome"] != expected or result["promoted"] is not (name in positive):
            raise ContractError("BUILD_ACQUISITION_DENIED", f"invalid boundary fixture result: {name}")
    return matrix


def requests_for_root(root: Path) -> tuple[dict[str, object], list[AcquisitionRequest]]:
    population = validate_resolved_input_population(root)
    inventory = {row["coordinate"]: row for row in population["inventory"]["dependencies"]}
    rows = [*population["archives"], population["registry"]]
    requests = [
        AcquisitionRequest(
            coordinate=row["coordinate"],
            name=row["name"],
            role=row["role"],
            sha256=inventory[row["coordinate"]]["sha256"],
            url=row["url"],
        )
        for row in rows
    ]
    return population["acquisition"], requests


def main(argv: Sequence[str] | None = None) -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--root", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    arguments = parser.parse_args(argv)
    try:
        validate_acquisition_manifest(arguments.root / "config/input-acquisition.json")
        manifest, requests = requests_for_root(arguments.root)
        ProfileAcquirer(manifest, requests, UrlTransport()).acquire(arguments.output)
        return 0
    except ContractError as error:
        print(str(error), file=os.sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
