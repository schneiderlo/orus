# M0-002 Rework Handoff r1

## Source and decision

- Rework base revision: `fdf2f4ca13d87cc0e46b58083df126df70e5cb51`
- Authoritative verifier report: `.agents/M0-002/verification-report.md`
- Latest full Factory check run: `run-20260722T171208Z` (green)
- Rework status at handoff: ready for independent c-06/c-07 re-verification;
  no task is marked `DONE` by this handoff.

## What changed

- Added Linux production peak-RSS and monotonic elapsed-time observation and
  retained deterministic injected minima. Reference validation observes around
  parsing and predicate work; package identity observes before traversal,
  between entries, and during streaming file reads; governance, performance,
  corpus, and cross-document resolvers observe before and after proportional
  work.
- Added a Bazel workspace-status-backed generated C++ provider for the five
  `M0-BUILD-FACTS-v1` fields. Configuration and compiler values are selected
  from declared Bazel settings; the full Git revision and explicit dirty state
  come from the declared stable status input. V1/c-01 now also runs the real
  release configuration.
- Added `ValidatePerformanceResultBundle`, which resolves workload, canonical
  raw-array bytes, and result together; it verifies byte length, SHA-256,
  count, context, storage/allocation conditions, phase/index order,
  valid/invalid counts, integer statistics, percentiles, and noise state.
- Validated corpus nested fields for both success and non-success reports,
  rejected success-shaped injected failures, implemented exact cleanup-failure
  precedence, and added `ValidateCorpusReliabilityBundle` to resolve referenced
  run bytes, digests, indices, source/environment context, terminal counts,
  passed counts, cleanup, and aggregate booleans.
- Replaced release-evidence path-substring cycle checks with exact forbidden
  schema/DAG checks. Corrected the 64-KiB SBOM descriptor, 1-MiB corpus-run,
  16-MiB corpus-aggregate, 100,000-pair, and 10,000-work-unit literals; the
  schema-specific byte limits now reject before full document parsing.
- Enabled ASan and UBSan together for the fuzz configuration and added
  fail-closed simulations for empty/omitted corpus, zero execution, crash,
  timeout, OOM, and sanitizer findings. A rework fuzz run exposed a Glaze
  numeric fast-path over-read on a one-byte input; the private defensive parse
  now uses Glaze's mutable padded-buffer path, and the fuzz target is green.
- Added LCOV source-freshness validation and a source snapshot digest, plus a
  stale-report negative fixture. The package/source manifest and 70% per-package
  threshold remain unchanged in scope.
- Kept `resource_monitor.h` private to the contract library and preserved the
  downstream M0-003 through M0-009 producer/workflow ownership boundaries.

## Blocker-to-fix mapping

| Verifier blocker | Fix and evidence |
|---|---|
| Production paths relied on injected resource counters and package reads checked deadlines too late. | `contracts/resource_monitor.h`, `contracts/crypto_resource.cc`, `contracts/build_contracts.cc`, and `contracts/evidence_contracts.cc` observe real peak RSS/elapsed time; `tests/contracts/build_test.cc` and `tests/contracts/evidence_test.cc` cover observed and injected boundaries, including checks inside streamed file reads. |
| Performance result validation did not resolve raw bytes or derive relationships/statistics. | Public bundle API in `contracts/include/orus/contracts/contracts.h`, implementation in `contracts/evidence_contracts.cc`, and raw digest/order/context/statistic/count/workload mutations in `tests/contracts/evidence_test.cc`. |
| Corpus failure documents and aggregates were not fully reconciled. | Universal nested validation, injected-fault/cleanup mapping, and the aggregate resolver in `contracts/evidence_contracts.cc`; failure-shape, referenced digest/context, and aggregate tests in `tests/contracts/evidence_test.cc`. |
| Literal limits drifted and release-evidence cycle detection depended on path substrings. | Schema-specific pre-parse byte ceilings, distinct pair/work-unit limits, alternate run/aggregate limits, and exact forbidden schema edges in `contracts/evidence_contracts.cc`; literal, oversized, and neutral-path/forbidden-schema fixtures in `tests/contracts/evidence_test.cc`. |
| Build facts were arbitrary caller strings rather than Bazel-bound inputs. | `tools/build/workspace_status.sh`, `tools/build/build_facts.bzl`, `.bazelrc`, `contracts/BUILD.bazel`, generated `EmbeddedBuildFacts`, and dev/release tests in `tests/contracts/build_test.cc`. Registered c-01 was edited through Factory to include the release facts target. |
| N10/N11 negative simulations, fuzz-time UBSan, and coverage freshness were absent. | `.bazelrc`, `tests/fuzz/fuzz_smoke_verifier.py`, `tests/fuzz/BUILD.bazel`, `tests/contracts/boundary_test.py`, `tools/coverage/package_gate.py`, and `tests/build/contract_test.py`. |

## Commands run and outcomes

| Command | Outcome |
|---|---|
| `uv run --no-project /home/lschneid/workspace/schneiderlo/factory/factory.py status` | Run active at `11_rework`; no open question reported. |
| `uv run --no-project /home/lschneid/workspace/schneiderlo/factory/factory.py task status M0-002 "DOING (Rework)"` | Confirmed the routed task was already in the required rework status. |
| `nix develop --command bazel test --config=fuzz //tests/fuzz:canonical_json_parser_fuzz_smoke` | Initially reproduced an ASan heap-buffer-overflow in Glaze on the one-byte input `8`; passed after routing the private defensive parse through Glaze's padded mutable buffer. |
| `nix develop --command bazel run //tools:format` | Passed. |
| `nix develop --command bazel test --config=dev //tests/contracts:evidence_test //tests/contracts:canonical_test //tests/contracts:boundary_test //tests/contracts:build_test` | Passed all four focused targets after the final relationship/boundary changes. |
| `uv run --no-project /home/lschneid/workspace/schneiderlo/factory/factory.py check run M0-002` | `run-20260722T171208Z` green: c-01 183.9 s, c-02 132.4 s, c-03 91.4 s, c-04 40.7 s, c-05 117.7 s. |
| `git diff --check` | Passed. |

The authoritative c-05 report is fresh and records `contracts` 85.12%,
`python/orus_contracts` 85.59%, `tools` 79.41%, `tools/build` 75.32%, and
`tools/coverage` 75.00%, with source snapshot
`cfd7ad8e4f9b0fc7da7d6795c7f49ce49e1ff927b7998e6efab834661a1653cd`.

Earlier partial rework runs are retained for audit: `run-20260722T162639Z`
was red while the new contracts were being completed, and
`run-20260722T163837Z` isolated the fuzz over-read while its other four machine
checks passed.

## Changed task-owned paths

- `.bazelrc`
- `contracts/BUILD.bazel`
- `contracts/build_contracts.cc`
- `contracts/canonical_json.cc`
- `contracts/crypto_resource.cc`
- `contracts/evidence_contracts.cc`
- `contracts/include/orus/contracts/contracts.h`
- `contracts/resource_monitor.h`
- `tests/build/contract_test.py`
- `tests/contracts/BUILD.bazel`
- `tests/contracts/boundary_test.py`
- `tests/contracts/build_test.cc`
- `tests/contracts/canonical_test.cc`
- `tests/contracts/evidence_test.cc`
- `tests/fuzz/BUILD.bazel`
- `tests/fuzz/fuzz_smoke_verifier.py`
- `tools/build/build_facts.bzl`
- `tools/build/workspace_status.sh`
- `tools/coverage/package_gate.py`

## Audit/Factory paths

- `.agents/M0-002/verification.md`
- `.agents/M0-002/handoff-r1.md`
- `.factory/checks/M0-002/c-01.json`
- `.factory/checks/M0-002/runs/run-20260722T162639Z.json`
- `.factory/checks/M0-002/runs/run-20260722T163837Z.json`
- `.factory/checks/M0-002/runs/run-20260722T171208Z.json`
- `.factory/run.json`
- `.factory/tasks/M0-002.json`
- `IMPLEMENTATION_PLAN.md`

The c-01 registration and verification-plan command changed because the
original command could not exercise the verifier-required real release build
facts. Factory lifecycle metadata and prior failed-attempt result cleanup are
not product implementation surface.

## Remaining risks and assumptions

- c-06 and c-07 remain independent manual verifier judgments; this handoff does
  not pre-approve them.
- Workspace status intentionally reports `-dirty` until the Factory captures
  this worktree in its step-boundary revision. The release-config test rejects
  that state; a clean committed verification revision exercises the clean
  embedded release facts path.
- RSS observation uses Linux `getrusage(RUSAGE_SELF)`, matching the sole M0
  Linux x86-64 reference environment. Broader platform support is not claimed.
- No downstream CLI, release producer/gate, performance harness/comparator,
  corpus runtime, CI provider, or security reconciler was implemented.
- No specification ambiguity or blocking open question remains.

## Pending Healing

None.
