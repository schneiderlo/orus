"""Fail-closed verifier for retained native fuzz-smoke outcomes."""

from __future__ import annotations

from collections.abc import Collection, Mapping


class FuzzVerificationError(ValueError):
    pass


def verify_fuzz_smoke(
    report: Mapping[str, object],
    *,
    expected_target: str,
    corpus_entries: Collection[str],
) -> None:
    if not corpus_entries:
        raise FuzzVerificationError("empty seed corpus")
    if report.get("target") != expected_target:
        raise FuzzVerificationError("fuzz target or corpus registration omitted")
    executions = report.get("executions")
    if not isinstance(executions, int) or isinstance(executions, bool) or executions <= 0:
        raise FuzzVerificationError("zero fuzz executions")
    if report.get("exit_code") != 0:
        raise FuzzVerificationError("fuzz crash or non-zero exit")
    for field in ("timed_out", "oom", "asan_finding", "ubsan_finding"):
        if report.get(field) is not False:
            raise FuzzVerificationError(f"fuzz outcome reports {field}")
    loaded_corpus = report.get("loaded_corpus")
    if not isinstance(loaded_corpus, list) or set(loaded_corpus) != set(corpus_entries):
        raise FuzzVerificationError("fuzz corpus population omitted or drifted")
