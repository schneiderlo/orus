# M0-002 Rework Handoff r2

## Source and decision

- Rework base / failed-verification revision:
  `026d0d962eacdc5bcb79e67a5877a486f017c314`
- Authoritative verifier report:
  `.agents/M0-002/verification-report-r1.md`
- Contract implementation commit:
  `8679339224f76e08d3e95a8bbb098d0991c0bc29`
- Coverage-negative completion commit:
  `14eeed8fd7ef2ddc2d6993894f42a582dc03651d`
- Exact verified product source revision:
  `14eeed8fd7ef2ddc2d6993894f42a582dc03651d`
- Exact verified source snapshot SHA-256:
  `7659d97bb1e261d3086649c677db060a42fbe9ad260034c67a21d35fc17fd312`
- Authoritative full Factory check run: `run-20260722T182133Z` (green)
- Rework status at handoff: ready for independent c-06/c-07 re-verification;
  this handoff does not mark any task `DONE`.

## What changed

- Added `GovernanceError`/`GovernanceResult` as the exact public
  `M0-GOV-ERROR-v1` boundary and added `ValidateGovernanceBundle`. The bundle
  resolver requires every release-evidence row to resolve to canonical bytes
  with exact path, schema, byte length, lowercase SHA-256, and implicit
  `evidence_object_sha256` subject. It resolves the external SPDX document,
  checks its canonical bytes/counts/document identity, binds the descriptor and
  manifest source/executable/package/SBOM subjects, rejects extra/unresolved
  bytes, and preserves M0-004 producer/reconciliation ownership.
- Completed `M0-PERF-COMPARISON-v1` validation: statistical fields are
  all-or-none, the comparator seed is derived from the ordered result IDs,
  flags follow the strict lower-bound threshold, advisory results retain
  informational statistics, authoritative terminal states match the flags,
  and mismatch/noise precedence is literal.
- Expanded `ResourceContractRow` to represent the complete task-owned
  `M0-RESOURCE-LIMIT-v1` surface and added
  `ValidateM0SharedResourceContracts`. Every row now carries schema,
  domain/operation/parser, trust/status/units, numeric limits, derived memory,
  process/thread/FD, CPU/time, storage/queue/I/O, enforcement point, exact
  error, cleanup, tests, owner/requirement, and scoped N/A rationale. Missing,
  numeric/error, late-enforcement, cleanup, trust, and rationale drift all fail.
- Replaced the mtime-only LCOV check with `M0-LCOV-PROVENANCE-v1`. The
  registered producer records the exact Git revision, source inventory and
  snapshot, coverage package manifest, coverage command, and LCOV digest;
  the unchanged 70% package gate verifies all bindings before reading counts.
  Stale copied/touched LCOV, report mutation, absent/malformed provenance, and
  source/manifest/revision drift fail closed.
- Updated only c-05 through `factory check edit` so its registered command runs
  the same fixed coverage population through the provenance producer before
  the existing threshold gate. `.agents/M0-002/verification.md` contains the
  matching V5 command and pass rule.

## Blocker-to-fix mapping

| Verifier blocker | Fix and retained evidence |
|---|---|
| Governance references were syntax-only and not byte/subject-bound. | `contracts/include/orus/contracts/contracts.h` exposes `ValidateGovernanceBundle`; `contracts/evidence_contracts.cc` resolves evidence and SPDX bytes plus source/executable/package/SBOM cross-links; `tests/contracts/evidence_test.cc` covers missing bytes, one-byte mutation, schema mismatch, cross-subject digest substitution, package cross-link drift, SBOM mutation, and forbidden self/current-scan schemas. c-01/c-02/c-03 are green in `run-20260722T182133Z`. |
| Performance comparison authority/noise/state/statistics/threshold relationships were incomplete. | `contracts/evidence_contracts.cc` enforces derived seed, complete informational statistics for advisory input, exact authority/state/flags, noise-failure precedence, and mismatch/statistical exclusivity. `tests/contracts/evidence_test.cc` has positive advisory/clean/regression cases and one negative per conditional class. c-01 through c-04 are green. |
| Governance error/resource diagnostics and the shared resource-row surface drifted. | `GovernanceError` has exactly schema/code/contract/field_path/record_id/expected/observed/limit/message; governance RSS maps to literal `peak_rss_bytes`. The expanded rows and inventory validator carry every M0-002-owned `M0-RESOURCE-LIMIT-v1` column and reject missing/numeric/error/late/cleanup/trust/N/A-rationale drift. Exact governance error and all seven rows execute in `tests/contracts/evidence_test.cc`. |
| Coverage freshness was mtime-only and accepted touched stale LCOV. | `tools/coverage/package_gate.py` creates and verifies content/revision/report-digest provenance; `tests/build/contract_test.py` exercises stale touch/copy, report mutation, and absent/malformed provenance. c-05 first exposed the honest 69.40% post-change package result in red run `run-20260722T180203Z`; added fail-closed branch tests raised the same package to 71.04% without changing scope or threshold, and green run `run-20260722T182133Z` binds revision `14eeed8...`. |
| Prior handoff lacked the exact source/check run and complete frozen inventory. | This append-only `handoff-r2.md` records both implementation commits, exact verified source revision/snapshot, green run, changed paths, complete API/schema/error/limit/golden inventory, commands/outcomes, ownership boundary, and known limitations. |

## Frozen public/API inventory

The public surface is `contracts/include/orus/contracts/contracts.h`; it exposes
only standard-library and Orus-owned types.

- Canonical/typed JSON: `JsonValue`, `ParseLimits`, `ParseCanonicalJson`,
  `EmitCanonicalJson`, `FindMember`, `RequireObject`, `RequireArray`,
  `RequireString`, `RequireInteger`, and `RequireBoolean`.
- Unicode, hashing, arithmetic, and resources: `NormalizeNfc`, `IsValidNfc`,
  `Sha256Digest`, `Sha256Stream`, both `Sha256` overloads, `ResourceLimits`,
  `ResourceUsage`, `CheckResourceUsage`, `CheckedAdd`, `CheckedSubtract`, and
  `CheckedMultiply`.
- Subject/build/reference/package identity: `DigestSubject`, `ContentIdentity`,
  `SubjectName`, `ValidateIdentityBinding`, `BuildFacts`, `MakeBuildFacts`,
  `EmbeddedBuildFacts`, `ReferenceOutcome`, `ReferenceValidation`,
  `ValidateReferenceEnvironment`, `PackageLimits`, `PackageIdentity`, and
  `IdentifyPackageTree`.
- Shared domain contracts: `IntegerStatistics`, `ComputeIntegerStatistics`,
  `NearestRankPercentile`, `GovernanceError`, `GovernanceResult`,
  `ReferencedDocument`, `ValidateGovernanceDocument`,
  `ValidateGovernanceBundle`, `ValidatePerformanceDocument`,
  `ValidatePerformanceResultBundle`, `ValidateCorpusDocument`, and
  `ValidateCorpusReliabilityBundle`.
- Resource reconciliation: the complete `ResourceContractRow`,
  `M0SharedResourceContracts`, and `ValidateM0SharedResourceContracts`.

No Glaze, utf8proc, OpenSSL handle/header, or native serialized layout appears
in that surface.

## Frozen schema, error, limit, and golden inventory

### Schemas and byte profiles

- Core/build: `M0-CANONICAL-JSON-v1`, `M0-BUILD-FACTS-v1`, `M0-REFENV-v1`,
  `M0-CONTENT-IDENTITY-v1`, and `M0-PACKAGE-TREE-v1`.
- Governance: `M0-SBOM-CONTRACT-v1`, `M0-RELEASE-EVIDENCE-v1`, and exact
  `M0-GOV-ERROR-v1`; SPDX bytes use `SPDX-2.3-v1` layered on the canonical
  profile.
- Performance: `M0-PERF-WORKLOAD-v1`, `M0-PERF-RAW-SAMPLE-v1`,
  `M0-CONTROLLED-RUNNER-v1`, `M0-PERF-RESULT-v1`,
  `M0-PERF-COMPARISON-v1`, and `M0-PERF-ERROR-v1`.
- Corpus/security/coverage: `M0-CORPUS-RUN-v1`,
  `M0-CORPUS-RELIABILITY-v1`, `M0-RESOURCE-LIMIT-v1`,
  `M0-COVERAGE-PACKAGES-v2`, `M0-LCOV-PROVENANCE-v1`, and
  `M0-PACKAGE-COVERAGE-v1`.

### Stable task-owned error codes

- Build/core: `BUILD_METADATA_INVALID`, `BUILD_PACKAGE_IDENTITY_INVALID`,
  `BUILD_REFENV_FIELD_INVALID`, `BUILD_REFENV_ID_MISMATCH`,
  `BUILD_REFENV_NONCANONICAL`, `BUILD_REFENV_OPERATOR_UNKNOWN`,
  `BUILD_REFENV_RESOURCE_LIMIT`, `BUILD_REFENV_SCHEMA_UNKNOWN`,
  `BUILD_UNVALIDATED_ENVIRONMENT`, `CANONICAL_JSON_NONCANONICAL`,
  `CANONICAL_JSON_RESOURCE_LIMIT`, and `DIGEST_INVALID`.
- Governance: `GOV_SCHEMA_UNKNOWN`, `GOV_NONCANONICAL`, `GOV_FIELD_MISSING`,
  `GOV_FIELD_TYPE`, `GOV_FIELD_BOUND`, `GOV_ENUM_INVALID`,
  `GOV_REFERENCE_UNRESOLVED`, `GOV_DIGEST_INVALID`,
  `GOV_RELATIONSHIP_INVALID`, and `GOV_RESOURCE_LIMIT`. The exact error object
  additionally fixes `contract`, nullable `record_id`, and literal
  `peak_rss_bytes|wall_time_ns` resource diagnostics.
- Performance: `PERF_SCHEMA_UNKNOWN`, `PERF_NONCANONICAL`,
  `PERF_FIELD_MISSING`, `PERF_FIELD_TYPE`, `PERF_FIELD_BOUND`,
  `PERF_DIGEST_INVALID`, `PERF_RELATIONSHIP_INVALID`,
  `PERF_INTEGER_OVERFLOW`, `PERF_RESOURCE_LIMIT`, plus terminal reasons
  `PERF_INCOMPARABLE`, `PERF_INSUFFICIENT_SAMPLES`, `PERF_SAMPLE_FAILED`,
  `PERF_NOISE_POLICY_FAILED`, `PERF_ADVISORY_INPUT`, and
  `PERF_REGRESSION_REQUIRES_APPROVAL`.
- Corpus/resource: `CORP_REPORT_SCHEMA_UNKNOWN`,
  `CORP_REPORT_NONCANONICAL`, `CORP_REPORT_FIELD_MISSING`,
  `CORP_REPORT_FIELD_TYPE`, `CORP_REPORT_FIELD_BOUND`,
  `CORP_REPORT_ENUM_INVALID`, `CORP_REPORT_DIGEST_INVALID`,
  `CORP_REPORT_RELATIONSHIP_INVALID`, `CORP_REPORT_FORGED_SUCCESS`, and
  `SEC_RESOURCE_LIMIT` for inventory drift.

### Task-owned limits

`M0SharedResourceContracts()` freezes seven rows:
`SEC-LIM-10-02`, `SEC-LIM-10-03`, `SEC-LIM-11-03`, `SEC-LIM-11-04`,
`SEC-LIM-14-01`, `SEC-LIM-14-02`, and `SEC-LIM-15-03`. Their exact primary and
alternate byte/count/work/depth/RSS/time values, enforcement points, errors,
cleanup, tests, owners, and N/A rationales live in
`contracts/evidence_contracts.cc` and are compared byte-for-byte by
`ValidateM0SharedResourceContracts`.

### Golden corpus

- Canonical/reference: the five files under
  `tests/fuzz/corpus/canonical_json/` plus `reference-fixed.json` and
  `reference-observed-fixed.json`.
- Governance: `sbom-descriptor.json`, `release-evidence-assembled.json`, and
  the resolved 12-evidence/SPDX positive and mutation corpus constructed in
  `tests/contracts/evidence_test.cc` from those fixed bytes.
- Performance: `perf-workload.json`, `perf-raw-sample.json`,
  `perf-runner.json`, `perf-result.json`, and `perf-comparison.json`, including
  their five fixed SHA-256 values and complete relationship mutation matrix.
- Corpus: `corpus-run.json` and `corpus-reliability.json` plus the retained
  aggregate/reference/fault mutations in `tests/contracts/evidence_test.cc`.
- Coverage/fuzz registration negatives live in `tests/build/contract_test.py`
  and `tests/contracts/boundary_test.py`; generated LCOV/provenance remains
  run evidence, not a checked-in golden.

## Commands run and outcomes

| Command | Outcome |
|---|---|
| `uv run --no-project /home/lschneid/workspace/schneiderlo/factory/factory.py status` | Workflow running at `11_rework`; no open question. |
| `uv run --no-project /home/lschneid/workspace/schneiderlo/factory/factory.py task status M0-002 "DOING (Rework)"` | `BLOCKED -> DOING (Rework)`. |
| `nix develop --command bazel test --config=dev //tests/contracts:evidence_test //tests/build:contract_test` | Passed both focused targets after the four production blockers were fixed. |
| `nix develop --command bazel run //tools:format` | Passed. |
| `uv run --no-project /home/lschneid/workspace/schneiderlo/factory/factory.py check edit M0-002 c-05 ...` | Registered the revision-bound LCOV producer and matching description; no threshold/scope change. |
| `uv run --no-project /home/lschneid/workspace/schneiderlo/factory/factory.py check run M0-002` | `run-20260722T180203Z`: c-01 through c-04 passed; c-05 honestly failed because `tools/coverage` was 69.40%. |
| `nix develop --command bazel test --config=dev //tests/build:contract_test` | Passed after adding absent/malformed provenance negatives. |
| `uv run --no-project /home/lschneid/workspace/schneiderlo/factory/factory.py check run M0-002` | `run-20260722T182133Z` green: c-01 289.9 s, c-02 176.3 s, c-03 141.8 s, c-04 69.6 s, c-05 158.9 s. |
| `git diff --check` | Passed before the implementation commits and handoff. |

The authoritative c-05 result is source-revision-bound and records:

| Package | Line coverage |
|---|---:|
| `contracts` | 85.74% |
| `python/orus_contracts` | 85.59% |
| `tools` | 79.41% |
| `tools/build` | 75.32% |
| `tools/coverage` | 71.04% |

The provenance binds source revision `14eeed8fd7ef...`, source snapshot
`7659d97bb1e...`, manifest digest `acd0a4e88de1...`, and LCOV digest
`99fead141a51...`.

## Changed task-owned paths

- `contracts/evidence_contracts.cc`
- `contracts/include/orus/contracts/contracts.h`
- `tests/build/contract_test.py`
- `tests/contracts/evidence_test.cc`
- `tools/coverage/package_gate.py`

## Audit/Factory paths

- `.agents/M0-002/verification.md`
- `.agents/M0-002/handoff-r2.md`
- `.factory/checks/M0-002/c-05.json`
- `.factory/checks/M0-002/runs/run-20260722T180203Z.json`
- `.factory/checks/M0-002/runs/run-20260722T182133Z.json`
- `.factory/logs/checks/M0-002/run-20260722T180203Z/`
- `.factory/logs/checks/M0-002/run-20260722T182133Z/`
- `.factory/run.json`
- `.factory/tasks/M0-002.json`
- `IMPLEMENTATION_PLAN.md`

The c-05 command was edited only because the verifier proved its mtime-only
freshness assertion could not satisfy N11. All status/plan/task metadata was
changed by Factory CLI lifecycle commands, not by hand.

## Remaining risks and ownership limits

- c-06 and c-07 remain independent verifier judgments; this handoff does not
  pre-approve them.
- The governance resolver freezes shared canonical bytes, identities, and
  cross-document relationships. Full graph/license/notice reconciliation,
  SPDX production, approvals, gate transitions, final scan, and marker creation
  remain M0-004-owned.
- Performance workload execution and comparator decision production remain
  M0-005-owned; this task freezes only document/relationship acceptance.
- Corpus execution remains M0-006-owned; complete cross-domain resource
  reconciliation remains M0-008-owned.
- The outstanding `github-actions` substitute is unrelated to M0-002 and no
  live provider claim is made.
- No specification ambiguity, temporary substitute, Pending Healing, or
  blocking open question remains.

## Pending Healing

None.
