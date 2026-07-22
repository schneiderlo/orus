# Continuous Integration and Quality Gate

**Status:** Draft  
**Milestone:** M0  
**Owners:** CI/quality owner; build owner; security reviewer  
**Last updated:** 2026-07-21  
**Depends on:** `specs/10-build-environment.md`, `specs/11-governance-release.md`, and `specs/12-cli-diagnostics.md` (Draft; all are Ready prerequisites); Foundation specs (Ready)  
**Foundation decisions:** D-010  
**Risks addressed:** R-001, R-002, R-004, R-005, R-007, R-008, R-201

## 1. Purpose

This domain owns GitHub Actions orchestration and the machine-checkable M0
quality gate. It ensures every applicable M0 target actually executes under
the declared compiler/configuration, retains evidence after failure, and
distinguishes blocking functional evidence from advisory shared-runner
performance data.

Build semantics remain owned by `specs/10-build-environment.md`; release
policy by `specs/11-governance-release.md`; benchmark statistics and runner
authority by `specs/14-performance-foundation.md`; and workload behavior by
`specs/15-concurrent-corpus.md`.

The M0 gate output is one `M0-CI-GATE-v1` report that reconciles all actual
M0 targets to the finite `M0-CI-APPLICABILITY-v1` manifest and proves every
required blocking cell passed with zero non-allowlisted warnings.

## 2. In Scope

- GitHub Actions triggers, permissions, concurrency, job graph, and evidence
  upload policy.
- Clang dev, GCC compatibility, release build, ASan, UBSan, explicit TSan
  applicability, fuzz smoke, format/lint/static, dependency/SBOM, concurrent
  corpus, and advisory benchmark jobs.
- Finite target/check applicability and exact not-applicable rationales.
- Versioned warning allowlist and zero-unallowlisted-warning rule.
- Action/input pinning, least privilege, secret/cache policy, failure
  diagnostics, and final gate aggregation.
- The named hash-verifying `m0_input_acquisition` job/profile and the network-denied
  boundary for every repository build/test/package job.
- Matrix self-tests that detect silent skip, empty target selection, host
  fallback, or advisory-to-blocking promotion.

## 3. Out of Scope

- Provisioning or operating a controlled performance runner; D-010 excludes it
  from M0.
- Treating GitHub-hosted benchmark deltas as authoritative.
- Build/toolchain definitions, test implementation, dependency admission, or
  release publication.
- M1+ tracer-specific sanitizer exceptions. Future domains add scoped
  applicability rows; they cannot weaken M0 rows silently.
- Secrets/signing/remote-cache credentials. M0 workflows require none for
  untrusted changes and remain functional without them.

## 4. Functional Requirements

| ID | Requirement | Acceptance criteria | Verification method | Traceability |
|---|---|---|---|---|
| CI-FR-001 | GitHub Actions shall run the M0 gate on every pull request and protected-branch update that changes a governed input. | Path/trigger test maps build files, source, tests, specs, policies, locks, workflows, and release metadata to at least the affected blocking jobs; protected-branch gate cannot pass with a required job absent. | CI-TEST-001 trigger simulation and required-check inspection. | G-03, SM-04, D-010. |
| CI-FR-002 | CI shall execute all Charter canonical commands through the pinned Nix environment and Bazel graph after the named acquisition job succeeds. | The workflow job ID, capability-profile ID, and spec-`10` profile ID are byte-identical `m0_input_acquisition`. That job materializes only admitted immutable lock entries under spec `10` BUILD-FR-008; each later canonical command has one job/step ID, consumes the read-only store manifest, runs with action/job network denied, and exits 0 on the reference fixture. Logs prove Nix+Bazel/toolchain delegation, zero CMake/host fallback, and zero post-acquisition fetch. | CI-TEST-002 workflow-to-command/acquisition identity reconciliation and clean run. | SM-01, DOD-03, R-001, R-005. |
| CI-FR-003 | `M0-CI-APPLICABILITY-v1` shall list every M0 Bazel target or finite target group against every M0 check within its fixed resource contract. | Canonical manifest is <=4 MiB/depth 16, every string is <=4 KiB, expansion is <=100,000 unique target/check cells, and validation uses <=256 MiB RSS/120 seconds; Bazel query and manifest label sets match; each cell is `required` or scoped `not_applicable`. Missing, duplicate, unknown, over-resource, or silent skip fails before job generation. | CI-TEST-003 schema/query/resource reconciliation and exact-bound mutation fixtures. | DOD-04, R-007, R-201. |
| CI-FR-004 | Blocking configurations shall cover Clang dev, GCC compatibility, release build, ASan, UBSan, formatting/lint/static checks, dependency/SBOM, fuzz smoke, and all other required matrix cells. | Every required cell executes at least one named target and reports pass/fail; ASan and UBSan execute applicable tests; fuzz smoke reports a positive execution count; release/SBOM operate on the release artifact. | CI-TEST-004 gate-result reconciliation. | SM-04, DOD-04. |
| CI-FR-005 | TSan applicability shall be explicit per target or group and never silently skipped. | Every TSan cell is required or cites a matrix rationale; required cells execute under TSan; non-applicable cells appear as such in the gate; an empty/fallback TSan fixture fails. | CI-TEST-005 TSan execution/applicability tests. | Charter 4.1, R-007. |
| CI-FR-006 | GitHub-hosted benchmark jobs shall emit complete results with `authority=advisory` and shall not fail solely because of measured delta. | Benchmark schema validation/tool failures block their own functional job; a simulated 10% regression with otherwise valid shared-runner provenance remains advisory and cannot fail `M0-CI-GATE-v1`; raw samples upload. | CI-TEST-006 authority and simulated-regression workflow test. | SM-12, D-010, R-002. |
| CI-FR-007 | Formatting, lint, static checks, and compiler warnings shall use zero non-allowlisted warnings within the manifest bound. | Canonical warning input/allowlist is <=4 MiB/depth 16 and <=100,000 entries; parsing permits exactly 256 MiB peak RSS and 120 seconds. Every warning has stable tool/rule and target/config scope; unmatched count zero; allowlist scope/owner/rationale/review exact. Wildcard, expired, input-bound, first attempted byte over 256 MiB, or first nanosecond after 120 seconds returns exactly `CI_WARNING_BLOCKED` before job/gate success. | CI-TEST-007 warning reconciliation, exact-bound/first-over parser resource, and injected-warning fixtures. | DOD-04, R-201. |
| CI-FR-008 | Workflow actions, permissions, capabilities, and external inputs shall be immutable and least-privileged. | Every third-party action is pinned to a full commit SHA. Workflow permissions default to `{contents: read}` and each job repeats the exact minimum; all M0 jobs have `actions`, `checks`, `deployments`, `id-token`, `issues`, `packages`, `pages`, `pull-requests`, `repository-projects`, `security-events`, and `statuses` set to `none` unless `contents:read` is the field. No M0 exception/release job may obtain write, OIDC, package, publish, or protected-secret access. Only `m0_input_acquisition` receives `network.client`, with the non-execution/hash/admission restrictions in Section 6.4; its workflow job ID and capability-profile ID must equal that literal byte-for-byte. | CI-TEST-008 workflow AST, capability/profile identity inventory, permission-mutation, and untrusted-PR simulation. | C-11, R-005, R-008. |
| CI-FR-009 | Cache use shall not alter correctness, expose secrets, or become an undeclared build input. | Clean no-cache run passes; cache-hit run produces equivalent test/gate outcome; cache keys include lock/toolchain/config/source inputs; untrusted changes cannot restore/write privileged cache; logs redact configured secret patterns. | CI-TEST-009 cold/warm cache and poisoning/redaction fixtures. | C-11, R-001. |
| CI-FR-010 | The final gate shall fail closed on missing, cancelled, failed, stale, or malformed required evidence. | Aggregator consumes only results for the same revision/run attempt and reference environment; any required non-pass prevents `passed`; advisory results are retained but excluded from blocking status; cancellation emits incomplete, not success. | CI-TEST-010 aggregator fault/cancellation matrix. | DOD-02, DOD-04, R-007. |
| CI-FR-011 | Failure paths shall retain the numerically bounded machine-readable evidence and diagnostics in Section 6.3. | Each job uploads its typed result or a typed evidence-limit failure even when tests fail. A report over 8 MiB is rejected/replaced, a diagnostic over 4 KiB is truncated at a UTF-8 scalar boundary, findings retain the first 1,000 in stable order, normalized logs retain 16-MiB head plus 16-MiB tail, and total per-job uncompressed evidence is at most 64 MiB. A bundle above 64 MiB produces `CI_EVIDENCE_LIMIT`: a blocking job is `failed`, while an advisory job is `incomplete` and cannot be represented as a blocking pass. Every job has an absolute provider timeout of 120 minutes. Completion at exactly 120 minutes is permitted; the first nanosecond over terminates the owned process group and produces terminal job state `failed` with code `CI_EVIDENCE_LIMIT`, never `incomplete` or `pass`. Any truncation/excess is explicit and prevents a blocking job from passing; secrets are absent. | CI-TEST-011 forced-failure, exact-bound/first-over timeout, cleanup, and every-boundary evidence test. | DOD-08, DOD-10, R-005, R-008, R-201. |
| CI-FR-012 | CI shall execute the concurrent corpus gate and verify teardown, including failure/cancellation fixtures. | The required corpus job consumes the 100-run report and post-run cleanup assertions defined by `specs/15-concurrent-corpus.md`; any timeout, wrong result/status, or leak blocks the gate. | CI-TEST-012 corpus report integration and forged-success rejection. | SM-05, DOD-09, R-006. |

## 5. Non-Functional Requirements

| ID | Quality attribute and target | Conditions / workload | Acceptance criteria | Verification method and evidence |
|---|---|---|---|---|
| CI-NFR-001 | Blocking coverage: 100% of required applicability cells execute and pass. | One protected-branch run for the release revision in `M0-REFENV-v1`. | Executed required cells / declared required cells = 100%; zero missing/skip/failure. | CI-TEST-003/004 `ci-applicability-reconciliation.json`. |
| CI-NFR-002 | Required quality jobs: 100% pass for Clang dev, GCC, release, ASan, UBSan, format/lint/static, SBOM, fuzz smoke, and corpus. | Release revision; exact finite gate inventory. | Every named blocking job is pass and carries current revision/environment identity. | CI-TEST-010 `m0-ci-gate-v1.json`. |
| CI-NFR-003 | Warning policy: 0 non-allowlisted warnings. | All blocking compiler/analyzer/linter outputs. | Unmatched warning count is zero; every allowlisted warning matches exact rule/target/config and has required metadata. | CI-TEST-007 warning report. |
| CI-NFR-004 | Fuzz smoke execution: greater than 0 completed inputs for every required fuzz target, with 0 crash, sanitizer finding, timeout, or OOM. | CI fuzz configuration and committed seed corpus; bounded smoke duration is declared per target in the applicability manifest. | Each required target reports positive executions and zero failure. | CI-TEST-004 fuzz report. |
| CI-NFR-005 | Advisory integrity: 100% shared-runner benchmark results label `advisory`; 0 gate failures are caused solely by their delta. | All GitHub-hosted benchmark jobs and 10% simulated regression. | Authority label exact; aggregator exclusion proven. | CI-TEST-006 comparison/gate report. |
| CI-NFR-006 | Secret/privilege exposure: 0 protected secret values and 0 M0 write/OIDC/package/publish permission. | Untrusted-fork and protected-branch simulations with canary values; full workflow, capability, cache, log, and artifact scan. | Canary count zero outside protected harness; permission inventory exactly matches Section 6.4; acquisition has network but no credential/build execution. | CI-TEST-008/009 security report. |

Precise performance deltas from these shared runners are Advisory regardless
of sample count. The greater-than-3% blocking policy becomes operational only
on a conforming controlled runner as defined by
`specs/14-performance-foundation.md`.

## 6. Interfaces / Contracts

### 6.1 `M0-CI-APPLICABILITY-v1`

The canonical committed manifest is at most 4 MiB/depth 16 and expands to at
most 100,000 target/check cells. It contains `schema`, source revision policy, check
definitions, target groups with explicit Bazel labels, cells, and rationale
records. Dynamic query is used only to detect omissions; it does not silently
add a target. A group is valid only when its expanded labels are committed and
each member has identical applicability. A divergent target becomes its own
row.

Cell values below are the required initial matrix. `R` means required.
`N-n` means not applicable under the rationale immediately below.

| Finite target group | Dev test | GCC test | Release build | ASan | UBSan | TSan | Fuzz smoke | Format/lint/static | SBOM | Advisory benchmark |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| `m0_cpp_unit` | R | R | R | R | R | R | N-3 | R | N-5 | N-4 |
| `m0_cli_integration` | R | R | R | R | R | N-2 | N-3 | R | N-5 | N-4 |
| `m0_governance_tools` | R | N-1 | R | N-1 | N-1 | N-1 | N-3 | R | N-5 | N-4 |
| `m0_ci_self_tests` | R | N-1 | N-6 | N-1 | N-1 | N-1 | N-3 | R | N-5 | N-4 |
| `m0_performance_tools` | R | R | R | R | R | R | N-3 | R | N-5 | R |
| `m0_concurrent_corpus` | R | R | R | R | R | R | N-3 | R | N-5 | N-4 |
| `m0_structured_input_fuzzers` | R | R | R | R | R | N-2 | R | R | N-5 | N-4 |
| `m0_security_policy_tests` | R | N-1 | N-6 | N-1 | N-1 | N-1 | N-3 | R | N-5 | N-4 |
| `m0_release_package` | N-6 | N-1 | R | N-1 | N-1 | N-1 | N-3 | R | R | N-4 |

Rationales:

- **N-1:** The listed target is a non-C/C++ policy/workflow/package action, so
  compiler or sanitizer instrumentation has no applicable executable.
- **N-2:** The target contains no concurrent mutable code in this group or is a
  fuzz process whose TSan combination is not admitted; separate concurrent
  components remain required under `m0_cpp_unit`,
  `m0_performance_tools`, or `m0_concurrent_corpus`.
- **N-3:** The target exposes no structured untrusted parser entry point;
  fuzzing by arbitrary bytes would not exercise a defined contract.
- **N-4:** The target is not a benchmark workload; smoke/runtime correctness is
  covered by its functional cells.
- **N-5:** SBOM is generated once for the release package's resolved graph, not
  once per test group.
- **N-6:** The target is an orchestration/package action rather than a runtime
  test or does not define a separate release build beyond its inspected files.

The actual label list is an implementation artifact under CI-FR-003. A
`not_applicable` entry may not cite only a group-wide rationale if one member
has an applicable surface.

### 6.2 Warning allowlist

`M0-WARNING-ALLOWLIST-v1` entries contain stable `tool`, `rule_id`,
exact `target`, exact `configuration`, owner, rationale, evidence reference,
created revision, and mandatory `review_milestone`. Matching is exact; no
empty or wildcard rule/target/config. An entry does not suppress the raw
warning and cannot convert an error or sanitizer finding into a pass.
The canonical warning input and allowlist are each at most 4 MiB/depth 16 and
100,000 entries; parsing uses at most 256 MiB RSS and 120 seconds. Bound excess
for the applicability manifest is `CI_APPLICABILITY_INVALID`. Any warning-input
or allowlist byte/count/depth/string excess, warning-parser peak RSS above 256
MiB, or warning-parser wall time above 120 seconds is exactly
`CI_WARNING_BLOCKED` before job/gate success. The exact-bound parser fixtures
at 256 MiB and 120 seconds are valid; the first-over fixtures are 256 MiB + 1
byte and 120 seconds + 1 nanosecond.

### 6.3 Gate and evidence contracts

`M0-CI-CHECK-RESULT-v1` contains schema, run/attempt/revision,
`M0-REFENV-v1` ID, check ID, configuration, expanded target labels, command,
start/end/exit status, executed-test/input counts, warning counts, authority,
and lowercase hexadecimal SHA-256 evidence digests. `M0-CI-GATE-v1` contains all expected check IDs,
terminal states, advisory exclusions, and aggregate `passed|failed|incomplete`.

Results are immutable per run attempt. The aggregator accepts exactly one
terminal result for each required check and the same revision/environment.
Retries have distinct attempt IDs. Workflow cancellation propagates to
incomplete.

Provider timeout is a resource failure, not workflow cancellation. At exactly
120 minutes a job may complete normally; at 120 minutes + 1 nanosecond the
provider timeout terminates the owned process group. The job finalizer, or the
aggregator from the authenticated provider terminal event if the process can no
longer upload, emits the bounded `M0-CI-EVIDENCE-LIMIT-v1` record with
`code=CI_EVIDENCE_LIMIT`, `class=provider_wall_time`, `limit=7200000000000`
nanoseconds, and terminal job state `failed`. `incomplete` remains reserved for
missing/cancelled evidence, never a known provider-timeout event.

Evidence limits are normative over uncompressed bytes:

| Class | Maximum | Exact over-limit behavior |
|---|---:|---|
| `M0-CI-CHECK-RESULT-v1`, gate, or other typed report | 8 MiB/document | Do not truncate a schema. Replace it with one <=64-KiB `M0-CI-EVIDENCE-LIMIT-v1` containing run/attempt/check, class, original byte count, limit, full-stream SHA-256 if safely available, `truncated=false`, and `code=CI_EVIDENCE_LIMIT`; blocking job/gate fails. |
| One diagnostic string | 4 KiB | Retain the first 4 KiB at a UTF-8 scalar boundary, set `diagnostic_truncated=true`, include original byte count; any omitted failure context causes job failure, never success. |
| Detailed findings | 1,000/report | Sort by `(rule_id,target,path,line)`; retain first 1,000, exact total, and `truncated=true`; findings already block and truncation cannot reduce count. |
| Retained normalized job log | 32 MiB | Retain first 16 MiB and last 16 MiB at line/scalar boundaries plus omitted-byte count and full-stream SHA-256; set `truncated=true` and fail a blocking job even if its command exited 0. Raw provider log is not release evidence. |
| Per-job evidence bundle | 64 MiB | Reject before upload and emit the limit document naming sorted artifact sizes; blocking job fails. Advisory job becomes `incomplete`, not a blocking pass. |

The limit document is itself `M0-CANONICAL-JSON-v1`; if its fixed 64-KiB
bound cannot be met, upload is absent and the aggregator returns
`CI_GATE_INCOMPLETE`. Retained summaries always include job/check/config/target,
exit status, original/retained bytes, truncation state, and artifact name.

### 6.4 Acquisition, permission, and capability profile

The only M0 network-capable profile is `m0_input_acquisition`. Its workflow job
ID, capability-profile ID in this spec, and profile ID in spec `10` are the
same UTF-8 byte sequence with no alias or normalization. It runs before any
repository-defined build/test/package action, reads the committed lock and
admission manifests as data, and fetches only allowlisted immutable coordinates
with expected hashes. It may write only quarantined downloads and the scoped
read-only input-store artifact. It has `contents:read`, `repository.read`,
`build.input.read`, `artifact.write`, and `network.client`; it has no
`build.action.execute`, credential, privileged-cache-write, repository-write,
OIDC, package, or publication capability. Lock changes not already admitted
are not acquired in untrusted-PR context.

Every other M0 job has `contents:read` plus only the local capabilities needed
for declared inputs, build actions, test-owned subprocesses, and scoped
artifacts; `network.client` is denied at job and Bazel-action boundaries. No
permission exception exists in M0. The capability inventory and workflow AST
must agree exactly; absence/unknown means denied.

### 6.5 Design choice: job topology

| Alternative | Pros | Cons |
|---|---|---|
| Matrix-driven independent jobs plus one fail-closed aggregator (recommended) | Parallel feedback, precise applicability, retained partial evidence, explicit advisory separation. | More workflow/report plumbing. |
| One monolithic gate job | Simple status wiring and shared setup. | Slow feedback; cancellation loses later evidence; hidden skip is harder to detect. |
| One workflow per target | Maximum isolation. | Excessive maintenance and difficult global reconciliation. |

**Recommendation:** Use independent jobs generated from the committed finite
manifest, followed by one non-publishing aggregator. This implements D-010
without requiring a controlled runner.

### 6.6 Design choice: CI acquisition placement

| Alternative | Pros | Cons |
|---|---|---|
| One named pre-build acquisition job/profile (recommended) | Sole network grant is reviewable; downloaded bytes are hash/admission checked once; build jobs stay offline. | Requires immutable store handoff and manifest validation. |
| Prepopulate the runner image/store outside CI | Build jobs need no network or handoff. | Clean-clone evidence depends on an externally maintained image and omits acquisition proof. |
| Let each matrix job fetch inputs | Simple fan-out setup. | Multiplies network grants and permits repository actions to hide fetches. |

**Recommendation:** Use the one named acquisition profile defined by spec `10`
and Section 6.4; no matrix/release-job permission escape exists.

## 7. Data Model

| Entity | Stable identity | Lifecycle / invariants |
|---|---|---|
| Check definition | Versioned check ID and command-template SHA-256 digest. | Immutable within applicability schema version; blocking/advisory explicit. |
| Target group | Stable group ID and ordered explicit Bazel label set. | No empty group; every queried M0 target appears exactly once unless a documented target legitimately participates in multiple check types. |
| Applicability cell | Group/target ID plus check ID. | Exactly one of required/not_applicable; latter references one scoped rationale. |
| Job attempt | GitHub run ID, attempt, check ID, revision. | `queued -> running -> pass|fail|cancelled`; rerun is new attempt. |
| Evidence artifact | Type, lowercase hexadecimal SHA-256 content digest, producer/check attempt, size, truncation state. | Immutable and correlated to one revision/environment. |
| Gate | Revision/environment plus expected check-set SHA-256 digest. | `collecting -> passed|failed|incomplete`; passed iff all blocking cells pass and evidence validates. |

GitHub job status is not sufficient evidence without the typed result.
Shared-runner measurements are advisory facts; they are never authoritative
performance baselines.

## 8. Key Flows

1. **Successful CI gate (CI-FR-001 through CI-FR-012).** Trigger resolves
   governed paths; the committed manifest expands explicit labels; Nix enters
   the reference environment; independent jobs execute required cells and
   upload reports; advisory benchmark job labels results; aggregator validates
   identities/evidence, sees every blocking pass and zero non-allowlisted
   warning, then emits `passed`.
2. **Failure with evidence (CI-FR-007, CI-FR-011).** A job observes a test,
   warning, sanitizer, fuzz, policy, or build failure; it records the first
   typed failure plus bounded raw evidence; uploads even on failure; aggregator
   emits `failed`; no release approval follows.
3. **Cancellation/teardown (CI-FR-010, CI-FR-012).** Workflow cancellation
   terminates job process groups; job/aggregator marks cancelled/incomplete;
   corpus teardown checks execute where the platform permits finalizers and
   the next run begins clean; absence of cleanup evidence cannot pass.
4. **Silent-skip/resource/malformed path (CI-FR-003 through CI-FR-005,
   CI-FR-010).** Empty group, unknown cell, absent result, wrong revision,
   truncated mandatory summary, or host fallback is rejected before aggregate
   pass.

## 9. Failure Modes

| ID | Trigger | Required detection point | Typed outcome / diagnostic fields | Side effects and cleanup | Retry / recovery | Verifying requirements/tests |
|---|---|---|---|---|---|---|
| CI-FAIL-001 | M0 target missing/duplicated, empty group, or cell lacks status/rationale. [R-007] | Manifest/query reconciliation before jobs. | `CI_APPLICABILITY_INVALID`; target/group, check, mismatch class. | No gate pass; no target silently executed/skipped. | Correct manifest or target classification. | CI-FR-003 / CI-TEST-003. |
| CI-FAIL-002 | Toolchain fallback or canonical command bypass. [R-001] | Job setup/action audit. | `CI_BUILD_CONTRACT_VIOLATION`; job, command, expected/observed toolchain/path. | Job fails; evidence retained. | Fix build/workflow mapping, rerun. | CI-FR-002 / CI-TEST-002. |
| CI-FAIL-003 | Required sanitizer/fuzz/TSan cell executes zero work or silently skips. [R-007] | Check-result validation. | `CI_EMPTY_EXECUTION`; check, labels, execution count, applicability. | Job/gate fail. | Correct target/config or explicit scoped rationale. | CI-FR-004, -005 / CI-TEST-004, -005. |
| CI-FAIL-004 | Shared-runner delta influences blocking status. [R-002] | Benchmark result and aggregator validation. | `CI_AUTHORITY_VIOLATION`; result ID, authority, attempted gate effect. | Gate fails as tool-policy defect, not performance regression. | Fix authority routing and rerun. | CI-FR-006 / CI-TEST-006. |
| CI-FAIL-005 | Non-allowlisted warning or invalid allowlist match. | Warning reconciliation. | `CI_WARNING_BLOCKED`; tool/rule, target/config, allowlist reason. | Blocking job fails; raw warning retained. | Fix warning or add narrowly reviewed entry. | CI-FR-007 / CI-TEST-007. |
| CI-FAIL-006 | Unpinned action, acquisition job/profile ID differs from `m0_input_acquisition`, permission outside exact M0 set, network outside acquisition, acquisition executes untrusted code, or secret/cache exposure. [R-005] | Workflow/capability static validation before affected job and network sandbox at runtime. | `CI_SECURITY_POLICY_VIOLATION`; workflow/job/profile, expected/observed profile bytes, action/permission/capability/coordinate. | Workflow/gate fails; invalid acquisition output quarantined; no secret use. | Pin/reduce/admit and rerun; no M0 permission exception or profile alias. | CI-FR-002, -008, -009 / CI-TEST-002, -008, -009. |
| CI-FAIL-007 | Missing/stale/malformed/cancelled required evidence, a Section 6.3 numeric evidence limit, or known provider timeout. [R-008, R-201] | Producer resource guard, authenticated provider terminal event, and final aggregation. | A known provider timeout or a numeric report/log/bundle excess in a blocking job is exactly `CI_EVIDENCE_LIMIT` with job state `failed`. A per-job advisory bundle above 64 MiB is exactly `CI_EVIDENCE_LIMIT` with job state `incomplete`. Missing/cancelled/stale/malformed evidence is `CI_GATE_INCOMPLETE`. Diagnostics include expected check/class, byte/count/time limit, observed state/identity, and run/attempt. | No blocking pass; bounded partial/limit evidence retained; timed-out process group terminated and staged upload removed. | Fix producer output or retry missing/cancelled jobs; never reinterpret truncation, an advisory incomplete bundle, or timeout as pass. | CI-FR-010, -011 / CI-TEST-010, -011. |
| CI-FAIL-008 | Corpus timeout/leak/wrong result or forged report. [R-006, R-008] | Corpus job and evidence SHA-256 digest/schema validation. | `CI_CORPUS_GATE_FAILED`; run index, fault mode, result/cleanup mismatch. | Job process group torn down; gate fails. | Diagnose/fix; rerun complete 100-run gate. | CI-FR-012 / CI-TEST-012. |

## 10. Observability

| Signal ID | Metric/event/log | Type, unit, and labels | Emission point / owner | Cardinality and cost bound | Alert/gate use | Verification |
|---|---|---|---|---|---|---|
| CI-OBS-001 | `ci_check_terminal` | Event; run/attempt, check, config, status, duration_ms, executed count | Job finalization / CI owner | One per check attempt; check inventory finite. | Blocking/advisory gate input. | CI-TEST-004. |
| CI-OBS-002 | `ci_applicability_coverage` | Gauge; declared/executed/missing cells | Reconciliation / CI owner | One aggregate and failures capped at 1,000. | DOD-04 gate. | CI-TEST-003. |
| CI-OBS-003 | `ci_warning_count` | Counter; tool/rule, target/config, allowlisted boolean | Warning normalizer / quality owner | Rule/target/config bounded by build graph; raw duplicate count aggregated. | Zero-unallowlisted gate. | CI-TEST-007. |
| CI-OBS-004 | `ci_benchmark_authority` | Event; result ID, runner, authority, comparator disposition | Benchmark job / performance owner | One per workload/result, workload inventory bounded. | Advisory policy audit. | CI-TEST-006. |
| CI-OBS-005 | `ci_gate_state` | Event; revision/environment/check-set SHA-256 digest, state, missing/failing counts | Aggregator / CI owner | One terminal event per attempt. | Required GitHub check and release evidence. | CI-TEST-010. |

Signals correlate by GitHub run/attempt, source revision, reference-environment
ID, build configuration, target label, `orus_executable_sha256`, and
`package_tree_sha256`. M0 has no trace,
branch, execution, VPID, or VTID. Typed reports and raw test/benchmark samples
are retained with the release-candidate evidence; logs are diagnostic views.

## 11. Test & Verification Plan

Copy/paste from the repository root after implementation:

```bash
nix flake check
nix develop --command bazel test --config=dev //tests/ci/...
nix develop --command bazel test --config=dev //...
nix develop --command bazel test --config=gcc //...
nix develop --command bazel test --config=asan //...
nix develop --command bazel test --config=ubsan //...
nix develop --command bazel test --config=tsan //...
nix develop --command bazel test --config=fuzz //tests/fuzz/...
nix develop --command bazel build --config=release //...
nix develop --command bazel run //tools:format
nix develop --command bazel run //tools/ci:applicability_reconcile
nix develop --command bazel run //tools/ci:workflow_policy_check
```

| Requirement ID | Test/benchmark/review ID | Level | Fixture/workload and environment | Pass criterion | Evidence artifact |
|---|---|---|---|---|---|
| CI-FR-001 | CI-TEST-001 | Integration/inspection | Governed-path change matrix and protected-branch rules. | Every change triggers affected gate; required status cannot be absent. | `ci-trigger-matrix.json`. |
| CI-FR-002 | CI-TEST-002 | End-to-end/security/analysis | Admitted/invalid acquisition fixtures, job/profile-ID alias and byte-mismatch fixtures, reference clean run, workflow/Charter commands, post-acquisition network denial. | Workflow job ID, CI capability-profile ID, and spec-`10` ID are exactly `m0_input_acquisition`; 100% command mapping/success follows valid acquisition; invalid input quarantined; expected Nix/toolchain; zero bypass/fetch in build jobs. | `canonical-command-reconciliation.json`, `ci-acquisition-boundary.json`. |
| CI-FR-003, CI-NFR-001 | CI-TEST-003 | Schema/analysis/negative/resource | Manifest, Bazel query, missing/duplicate/empty/rationale mutations, and 4-MiB/depth-16/4-KiB/100,000-cell/256-MiB/120-second exact and first-over fixtures. | Exact target/cell equality; exact-bound inputs validate; all semantic and first-over mutations fail before job generation. | `ci-applicability-reconciliation.json`. |
| CI-FR-004, CI-NFR-002, CI-NFR-004 | CI-TEST-004 | End-to-end | Full required job set plus zero-execution fuzz fixture. | All real blocking jobs pass and execute; fixture fails. | `m0-ci-check-results`, fuzz reports. |
| CI-FR-005 | CI-TEST-005 | Integration/negative | Required and not-applicable TSan cells plus empty/fallback fixture. | Every cell explicit; required work executes; invalid fixture fails. | `tsan-applicability.json`. |
| CI-FR-006, CI-NFR-005 | CI-TEST-006 | Integration | Shared-runner valid result and simulated 10% delta. | Authority advisory; no delta-only gate failure; raw samples retained. | `advisory-benchmark-ci.json`. |
| CI-FR-007, CI-NFR-003 | CI-TEST-007 | Static/integration/resource | Real warnings plus unmatched, wildcard, stale-scope, and warning input/allowlist bound fixtures; parser peak RSS at exactly 256 MiB and 256 MiB + 1 byte; parser wall time at exactly 120 seconds and 120 seconds + 1 nanosecond. | Zero unmatched real warnings; exact-bound parser fixtures complete normally; both first-over parser fixtures and every invalid warning fixture return exactly `CI_WARNING_BLOCKED` before job/gate success. | `warning-reconciliation.json`, `warning-parser-resource.json`. |
| CI-FR-008, CI-NFR-006 | CI-TEST-008 | Security/static/negative | Workflow AST, exact permission/capability inventory, profile alias/case/byte mutations, untrusted PR canary, and one mutation for every forbidden permission/capability plus acquisition code-execution/network-scope faults. | Full SHA pins; the only network job/profile is byte-identical `m0_input_acquisition`; zero M0 write/OIDC/package/publish and zero canary. | `workflow-security.sarif`, `ci-capability-matrix.json`. |
| CI-FR-009 | CI-TEST-009 | Security/end-to-end | Cold/warm/poisoned cache and log redaction fixtures. | Equivalent valid outcomes; poisoned entry rejected; zero secret output. | `cache-security.json`. |
| CI-FR-010 | CI-TEST-010 | Unit/integration/cancellation | Pass, failure, missing, stale, malformed, advisory, and cancellation result sets. | Aggregate state matches contract in every case; only complete blocking pass succeeds. | `ci-gate-fixture-matrix.json`. |
| CI-FR-011 | CI-TEST-011 | Integration/resource/timeout | Forced failure; 4-KiB diagnostic, 1,000-finding, 8-MiB report, 32-MiB log, blocking and advisory 64-MiB bundles, and job wall-time fixtures at exactly 120 minutes and 120 minutes + 1 nanosecond; each numeric fixture has a first-over value of one byte or one nanosecond; UTF-8 split, owned-process cleanup, and secret fixtures. | Exact-bound artifacts and the exactly-120-minute completion pass structurally; every evidence over-limit follows its exact reject/truncate rule. A blocking 64-MiB + 1-byte bundle emits `CI_EVIDENCE_LIMIT`/`failed`; an advisory 64-MiB + 1-byte bundle emits `CI_EVIDENCE_LIMIT`/`incomplete`; the first-over timeout terminates the group and emits/synthesizes exactly `CI_EVIDENCE_LIMIT`/`failed`. Head/tail/count/digest are exact and no secret is retained. | `failure-evidence-report.json`, `ci-evidence-limit-matrix.json`. |
| CI-FR-012 | CI-TEST-012 | End-to-end/fault | Valid corpus report plus wrong digest/schema/status/leak/cancel fixtures. | Valid 100-run report accepted; every forged/failing fixture blocks. | `corpus-ci-integration.json`. |

## 12. Open Questions

No open questions.
