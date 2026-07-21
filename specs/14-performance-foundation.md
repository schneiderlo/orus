# Performance Measurement Foundation

**Status:** Draft  
**Milestone:** M0  
**Owners:** Performance owner; CI owner; build/toolchain owner  
**Last updated:** 2026-07-21  
**Depends on:** `specs/10-build-environment.md` and `specs/13-ci-quality.md` (Draft; both are Ready prerequisites); Foundation specs (Ready)  
**Foundation decisions:** D-006, D-010  
**Risks addressed:** R-002, R-006, R-106, R-304

## 1. Purpose

This domain makes performance evidence reproducible, comparable, and explicit
about authority before Orus introduces a record/replay hot path. It owns the
benchmark harness, workload registry, allocation counter, raw sample and result
schemas, controlled-runner conformance contract, statistical comparator,
greater-than-3% rule, advisory/authoritative classification, noise rejection,
and profile escalation policy.

Every later performance-sensitive domain consumes these contracts and adds
workloads/budgets without changing their meaning silently. CI orchestration is
owned by `specs/13-ci-quality.md`; the concurrent workload is registered by
`specs/15-concurrent-corpus.md`; release evidence is owned by
`specs/11-governance-release.md`.

The M0 gate output is validated raw/result evidence showing the harness and
comparator boundary fixtures work, shared-runner results stay advisory, and
the allocation counter both proves zero for its designated steady-state loop
and detects a positive fixture.

## 2. In Scope

- Versioned workload registry and benchmark invocation contract.
- Warmup, sample pairing/order, raw-sample retention, summary statistics, and
  comparison decision.
- Complete environment, build, runner, and storage provenance.
- Controlled-runner conformance and authority assignment without provisioning
  a runner.
- Advisory GitHub-runner behavior.
- Exact 3.0% boundary, significance rule, comparability rejection, and
  intentional-regression escalation.
- Process allocation counter with startup/steady-state phases and positive
  control.
- Noise/preflight checks and performance profile collection policy.
- Fixtures for lower-is-better, higher-is-better, incomparable, noisy,
  insufficient-sample, corrupt, and authority cases.

## 3. Out of Scope

- Procuring, administering, or securing a controlled runner.
- Authoritative M0 product speed claims; M0 validates tooling behavior only.
- Recorder/replay overhead, descriptor throughput, writer throughput, trace
  size, or control latency measurements before their owning milestones.
- Automatically approving a regression or changing a budget.
- Using shared GitHub-hosted measurements as blocking evidence.
- Removing correctness/validation work to meet a target.

## 4. Functional Requirements

| ID | Requirement | Acceptance criteria | Verification method | Traceability |
|---|---|---|---|---|
| PERF-FR-001 | Every benchmark shall be declared in `M0-PERF-WORKLOAD-v1` before its result is accepted. | Registry row defines stable workload ID/version, owner, executable/arguments, metric name/unit/direction, setup, warmup, measured operation, concurrency/data size, sample rule, timeout, applicable storage, allocation phase, and introducing requirement; unknown/mismatched workload is rejected. | PERF-TEST-001 registry schema and unknown/version-mismatch fixtures. | G-04, C-05, D-006. |
| PERF-FR-002 | The harness shall separate setup/warmup from measured steady state and retain every raw sample. | Each execution records phase boundaries; a comparison runs 5 warmup iterations before at least 30 valid measured baseline/candidate pairs, with pair order alternating and the initial order recorded; no raw measured sample is silently discarded or replaced by a summary; timeout/failed samples remain typed invalid records. | PERF-TEST-002 harness phase/order and raw-sample reconciliation. | SM-07, D-006; resolved `q-0006`. |
| PERF-FR-003 | Each result shall validate against canonical normalized JSON `M0-PERF-RESULT-v1` and include all Charter provenance. | Workload/version, source revision, artifact/build ID, CPU, affinity, kernel, compiler, optimization config, storage facts when applicable, warmup/sample method/counts, raw-sample references and lowercase hexadecimal SHA-256 digests, statistics in integer base units, runner ID, conformance, and authority are present; each missing-field, noncanonical, floating-point, native-layout, or wrong-digest fixture fails. | PERF-TEST-003 schema/canonicalization positive and per-field negative fixtures. | SM-07; resolved `q-0007` and `q-0011`. |
| PERF-FR-004 | A runner shall receive `authority=authoritative` only after `M0-CONTROLLED-RUNNER-v1` conformance succeeds for that run. | Contract verifies exact CPU/ISA, isolated affinity set, kernel/image, fixed governor/frequency policy, disabled/recorded boost policy, no CPU migration, no thermal throttling, background-load threshold, memory/NUMA policy, storage/device/filesystem/cache state when applicable, toolchain, workload, sampling, and noise policy; any unknown/failure makes result advisory or invalid, never authoritative. | PERF-TEST-004 runner conformance and one-negative-per-predicate matrix. | D-006, D-010, R-002. |
| PERF-FR-005 | Results from shared GitHub-hosted runners shall always be advisory. | Producer sets `runner_class=github_hosted_shared` and `authority=advisory`; comparator may describe delta but cannot return blocking regression; a forged authoritative label is rejected. | PERF-TEST-005 authority fixtures including simulated 10% delta. | SM-12, D-010, R-002. |
| PERF-FR-006 | The comparator shall reject inputs that are not semantically and environmentally comparable. | Schema/workload/metric/unit/direction, artifact role, runner contract/identity, CPU/ISA, affinity, kernel/image, toolchain except the intended candidate change, optimization, storage policy, sampling method, and declared comparison-key fields must match; each mismatch returns `PERF_INCOMPARABLE` naming all discovered mismatches and no regression verdict. | PERF-TEST-006 pairwise mismatch matrix. | SM-08, R-002. |
| PERF-FR-007 | The comparator shall apply the exact paired-bootstrap-median, strict greater-than-3.0% statistical policy in Section 6.4. | Exact 3.0% fixture does not flag; a fixture whose one-sided 99% lower confidence bound is strictly greater than 3.0% returns `regression_requires_approval`; an improvement uses the correct metric direction; insufficient/noisy data is inconclusive, never pass. | PERF-TEST-007 deterministic statistical boundary corpus. | SM-08, D-006; resolved `q-0006`. |
| PERF-FR-008 | The comparator shall preserve authority in its decision. | Authoritative comparable results may produce blocking pass/regression/inconclusive; any advisory input yields `advisory_only`; mixed authority cannot produce a blocking verdict; result states whether the threshold and significance tests fired. | PERF-TEST-008 cross-product of baseline/candidate authorities. | D-010, R-002. |
| PERF-FR-009 | The allocation counter shall measure startup and designated steady-state regions separately. | `m0_noalloc_reference_v1` reports exactly 0 allocation calls and 0 allocated bytes in steady state after startup for all measured iterations; `m0_positive_alloc_reference_v1` reports at least 1 call and positive bytes; counter reentrancy/overflow becomes failure, not zero. | PERF-TEST-009 allocation positive/zero/reentrancy/overflow fixtures. | SM-09, future R-106 mitigation. |
| PERF-FR-010 | Noise policy shall run before and during measurement and preserve rather than trim samples. | Runner preflight records every contract predicate; each sample records migration, throttle, background-load, and failure state; violation marks whole authoritative comparison `PERF_NOISE_POLICY_FAILED`; outlier values remain in raw data and no post-hoc outlier deletion is allowed. | PERF-TEST-010 injected migration/throttle/background-load/outlier fixtures. | R-002. |
| PERF-FR-011 | A material regression shall require retained diagnostic profile evidence and an approved ADR before acceptance. | For authoritative `regression_requires_approval`, evidence includes raw results, comparator report, workload/build IDs, and a profile bundle with elapsed time, cycles, instructions, branches, cache misses, context switches, and bounded symbolized hotspots where collection is supported; no tool automatically raises the threshold. | PERF-TEST-011 regression escalation and missing-profile/approval fixtures. | D-006, R-304. |
| PERF-FR-012 | Canonical JSON result parsing and comparison shall enforce bounds and fail safely. | Before proportional allocation: JSON document size at most 16 MiB, at most 100,000 samples/result, at most 1,024 workloads/bundle, strings at most 4 KiB, integer values within declared widths, units from declared inventory, and bootstrap work exactly capped at 10,000 resamples; corrupt/noncanonical/oversize/native-layout input returns typed error with no partial authoritative result. | PERF-TEST-012 parser/resource/canonicalization corpus and fuzz target. | C-09, R-201 policy precursor; resolved `q-0007`. |
| PERF-FR-013 | Later domains shall declare when each Charter product objective becomes applicable and register its workload before implementation becomes Active. | Readiness audit covers greater-than-3%, 1.5x CPU recording, 2.5x syscall recording after fast path, 20M descriptors/s/core, 80% raw sequential writer throughput, and local pause/cancel p99 below 50 ms; each is either mapped to an introducing requirement/spec or explicitly not yet applicable. | PERF-TEST-013 roadmap objective reconciliation. | Charter Success Metrics after SM-12, C-05. |

## 5. Non-Functional Requirements

| ID | Quality attribute and target | Conditions / workload | Acceptance criteria | Verification method and evidence |
|---|---|---|---|---|
| PERF-NFR-001 | Comparator threshold: exact 3.0% is accepted; only a one-sided 99% lower bound strictly greater than 3.0% flags regression. | Five warmups; at least 30 alternating valid pairs; 10,000 deterministic paired-bootstrap resamples of median degradation; comparable authoritative inputs; zero migration and throttle events; benchmark-core background utilization below 1.0%; no sample trimming. | Boundary corpus returns expected disposition bit-for-bit across 100 comparator runs. | PERF-TEST-007 `comparator-boundaries.json`. |
| PERF-NFR-002 | Allocation reference: exactly 0 calls and 0 bytes in designated steady state; positive control greater than 0. | Pinned Clang dev/release fixtures after startup, at least 30 measured iterations. | Zero fixture exact in both configs; positive fixture detected every iteration; no counter error. | PERF-TEST-009 `allocation-counter-report.json`. |
| PERF-NFR-003 | Schema completeness: 100% of required provenance/sample fields and 100% raw-sample reconciliation. | Every M0 workload result, advisory or authoritative. | Missing-field count zero; declared measured count equals valid+invalid retained raw records; all SHA-256 references resolve. | PERF-TEST-002/003 schema report. |
| PERF-NFR-004 | Authority safety: 0 shared-runner or nonconforming-runner blocking verdicts. | Authority cross-product, conformance negative matrix, simulated 10% shared-runner regression. | Every such result is advisory/inconclusive/invalid; none is blocking pass/regression. | PERF-TEST-004/005/008. |
| PERF-NFR-005 | Comparator resource bound: at most 16 MiB input, 100,000 samples/result, 10,000 resamples, and 256 MiB peak process RSS for maximum valid two-result comparison. | Reference environment; maximum valid synthetic inputs; startup/tool memory included. | Valid maximum completes within caps; each over-limit fixture rejects before proportional excess allocation. | PERF-TEST-012 resource report. |
| PERF-NFR-006 | Workload timeout truthfulness: 0 timed-out or failed samples are counted as valid. | Registered per-sample timeout and timeout/failure injection. | Invalid count and diagnostic exact; authoritative comparison is inconclusive if fewer than 30 valid pairs. | PERF-TEST-002/010. |

No M0 product-speed number is authoritative merely because the tooling passes.
The CPU-bound recording, syscall-heavy recording, descriptor, writer, and
pause/cancel objectives become measurable only in their introducing specs as
mapped by PERF-FR-013.

## 6. Interfaces / Contracts

### 6.1 Workload registry

`M0-PERF-WORKLOAD-v1` is a committed finite table. Initial rows are:

| Workload ID | Purpose | Metric / direction | Required measurement |
|---|---|---|---|
| `m0_harness_reference_v1` | Prove phase/sample/result plumbing with deterministic synthetic work. | `duration_ns`, lower is better. | 5 warmups; at least 30 measured samples; no allocation assertion. |
| `m0_noalloc_reference_v1` | Prove zero steady-state allocation counting. | `allocation_calls` and `allocated_bytes`, exact zero. | Startup excluded; at least 30 measured iterations. |
| `m0_positive_alloc_reference_v1` | Positive control for allocation counter. | Calls/bytes, positive detection. | At least one allocation per measured iteration. |
| `m0_comparator_boundary_v1` | Deterministic lower/higher, 3.0%, >3%, noise, authority fixtures. | Synthetic declared units/directions. | Exact checked-in raw sample corpus. |
| `m0_concurrent_native_v1` | Register native concurrent corpus measurements without asserting a product budget. | `duration_ns`, lower is better. | Added/owned by `specs/15-concurrent-corpus.md`; advisory on shared CI. |

Registry changes require owner, introducing requirement, schema validation,
and review. Workload identity changes whenever measured semantics, dataset,
concurrency, phase, metric, or sample rule changes.

### 6.2 Raw sample and result schema

`M0-PERF-RAW-SAMPLE-v1` contains result/workload/run IDs, pair/index/order,
phase, monotonic start/end/duration, primary metric value/unit, validity and
typed invalid reason, allocation counters when applicable, CPU/core, migration
count, throttle state, background-load observation, and storage observation
when applicable.

`M0-PERF-RESULT-v1` contains:

- schema/workload ID and version, metric name/unit/direction, dataset and
  concurrency;
- source revision, artifact identity/SHA-256 digest, build configuration, compiler and
  optimization facts;
- runner ID/class, `M0-CONTROLLED-RUNNER-v1` contract SHA-256 digest and conformance
  result, authority;
- CPU identity/ISA, affinity, NUMA/memory, kernel/image, governor/frequency and
  boost policy;
- storage device/model, filesystem, mount/cache and equivalent raw sequential
  storage result when the workload uses storage;
- warmup method/count, measured pair/sample method/counts, timeouts,
  noise-policy result;
- raw-sample artifact reference/SHA-256 digest, median, minimum, maximum, median absolute
  deviation, and percentile values only when sample population/method is
  stated;
- tool version, creation attempt identity, and validation state.

The workload, raw-sample, result, and comparison contracts use canonical
normalized UTF-8 JSON. Object members are serialized in deterministic
lexicographic key order with no insignificant whitespace; duplicate keys,
floating-point/exponent values, non-shortest integer spellings, and invalid
UTF-8 are rejected. Quantities use declared integer base units (for example,
nanoseconds and bytes); integer counters are unsigned 64-bit with overflow
failure. Result IDs are lowercase 64-character hexadecimal SHA-256 digests of
the canonical schema content excluding only the ID field. Raw samples may be
embedded or stored in separate canonical JSON documents referenced by relative
bundle path, schema ID, byte length, and SHA-256 digest. Native layouts are
never accepted or serialized.

### 6.3 Controlled runner

`M0-CONTROLLED-RUNNER-v1` defines expected exact/predicate values, measurement
method, and conformance status for CPU/ISA, isolated cores and affinity,
kernel/image, governor/frequency/boost, thermal throttling, CPU migration,
background utilization, NUMA/memory, storage state, toolchains, and clock
source. The selected M0 noise contract requires:

- the benchmark process is pinned to the declared isolated logical CPU set;
- observed migration count is 0;
- observed hardware thermal-throttle events are 0;
- background utilization on benchmark CPUs is below 1.0% during a 5-second
  preflight and each measured interval;
- after 5 warmup iterations, at least 30 valid baseline/candidate pairs run in
  strict alternating order, with the starting member selected by and recorded
  from a deterministic seed;
- no measured sample is trimmed; any predicate failure makes the comparison
  non-authoritative.

Provisioning and exact runner hardware remain outside M0. A contract can exist
without a conforming runner.

### 6.4 Statistical comparison contract

For each valid pair, normalize degradation as:

- lower-is-better: `candidate / baseline - 1`;
- higher-is-better: `baseline / candidate - 1`.

Zero/negative denominators are invalid unless a workload defines exact-count
comparison instead. At least 30 valid pairs are required. Using a deterministic
seed derived from the two result IDs, perform exactly 10,000 paired bootstrap
resamples, each drawing the original pair count with replacement, and calculate
the median degradation of each resample. The point estimate is the original
sample median. Sort the 10,000 bootstrap medians in ascending order and define
the one-sided 99% lower confidence bound as the zero-based element at index 99
(the nearest-rank 1st percentile). Even-sized sample medians are the arithmetic
mean of the two middle fixed-point values; all arithmetic and rounding rules
are versioned and covered by golden tests.

Disposition for comparable authoritative inputs:

- `regression_requires_approval` only when the lower bound is strictly
  greater than `0.030000`;
- `no_regression_detected` when the lower bound is at or below 3.0% and all
  conformance/sample rules pass; this is not proof of equality;
- `inconclusive` for insufficient valid pairs or failed noise policy.

Advisory inputs return `advisory_only` with descriptive statistics, never a
blocking disposition. The comparator emits all comparability fields, point
estimate, confidence bound, threshold, resample count/seed, authority, and
required next action.

### 6.5 Design choice: regression method

| Alternative | Pros | Cons |
|---|---|---|
| Alternating paired samples and one-sided bootstrap median bound (selected) | Robust to skew, uses local pairing to reduce drift, expresses strict 3% confidence rule directly. | More runner time; bootstrap/fixed-point implementation needs golden tests. |
| Independent Mann-Whitney test plus median ratio | Established rank test and simple libraries. | Significance does not directly bound the magnitude threshold; pairing benefit lost. |
| Compare aggregate means only | Easy and fast. | Noise/outlier sensitive and not a statistically valid 3% gate. |

**Recommendation:** Use the paired-bootstrap median method in Section 6.4:
5 warmups, at least 30 alternating valid pairs, exactly 10,000 deterministic
resamples, a one-sided 99% lower bound strictly greater than 3.0%, no trimming,
zero migration/throttle events, and benchmark-core background utilization
below 1.0%. This implements the human answer to `q-0006`.

### 6.6 Design choice: performance-document serialization

| Alternative | Pros | Cons |
|---|---|---|
| Canonical normalized JSON with integer base units and optional digest-referenced raw-sample JSON (selected) | Inspectable, straightforward schema validation, deterministic content identity, and sufficient for cold-path M0 tooling. | Larger and slower to parse than binary formats. |
| FlatBuffers | Strong typed binary contract and efficient parsing. | Adds cold-path complexity and conflicts with the explicit low-volume M0 choice. |
| CSV samples plus provenance sidecar | Simple tabular raw samples. | Weak nested provenance/integrity and creates a two-format drift surface. |

**Recommendation:** Use canonical normalized JSON for workload, raw-sample,
result, and comparison contracts; encode quantities in integer base units;
permit separate canonical JSON raw-sample documents only through SHA-256
references; never serialize native layouts. This implements `q-0007`.

## 7. Data Model

| Entity | Identity and relationship | Lifecycle / invariants |
|---|---|---|
| Workload | Stable ID plus semantic version and normalized-registry SHA-256 digest. | Version changes on measured-semantics change; result references exact version. |
| Runner contract | Runner ID plus normalized-contract SHA-256 digest. | Expected configuration immutable per result; conformance observed per run. |
| Benchmark run | Run/attempt ID, source/build/runner/workload identities. | `preflight -> warmup -> measuring -> validating -> valid|invalid`; invalid never becomes authoritative. |
| Raw sample | Result ID plus phase/pair/index/order. | Append-only; no silent deletion; invalid reason retained. |
| Result | SHA-256 content ID referencing an embedded or separate canonical JSON raw-sample document. | `assembled -> schema_valid -> advisory|authoritative|invalid`; authority cannot be upgraded without rerun/conformance. |
| Baseline | An authoritative result selected by reviewed baseline metadata. | Immutable; replacement records predecessor and rationale. |
| Comparison | Ordered baseline/candidate IDs and comparator version. | `validating -> incomparable|advisory_only|inconclusive|no_regression_detected|regression_requires_approval`. |
| Profile bundle | Comparison ID plus tool/config and artifact SHA-256 digest. | Required for material authoritative regression acceptance; diagnostic, not authority by itself. |

Raw samples and controlled-runner facts are authoritative measurement evidence
only when conformance passes. Summary prose, profiles, and shared-runner data
are derived/advisory and cannot overwrite samples or authority.

## 8. Key Flows

1. **Controlled comparison (PERF-FR-001 through PERF-FR-011).** Resolve
   workload/builds; validate runner preflight; alternate five warmups then at
   least 30 measured pairs; record every sample; validate result schemas;
   compare exact provenance; bootstrap paired degradation; emit blocking
   disposition only for authoritative conforming inputs; retain raw and
   summary evidence.
2. **Shared CI smoke (PERF-FR-003, -005, -008).** Run registered workload on
   shared host; record available provenance and raw samples; label advisory;
   comparator may report delta; CI blocks only on tool/schema/crash failure,
   never measured delta.
3. **Cancellation/timeout (PERF-FR-002, -006, -010).** Terminate owned
   benchmark process group; record incomplete/invalid sample and attempt;
   publish no authoritative result; a retry uses a new attempt ID and fresh
   preflight.
4. **Incomparable/malformed/resource path (PERF-FR-006, -012).** Validate
   schema/bounds before loading all samples; enumerate bounded comparison-key
   mismatches; return typed invalid/incomparable result without statistical
   verdict or allocation beyond caps.
5. **Regression escalation (PERF-FR-007, -011).** Strict bound fires; tool
   returns `regression_requires_approval`; collect profile; owner investigates;
   acceptance requires evidence and ADR and cannot mutate historical results
   or threshold.

## 9. Failure Modes

| ID | Trigger | Required detection point | Typed outcome / diagnostic fields | Side effects and cleanup | Retry / recovery | Verifying requirements/tests |
|---|---|---|---|---|---|---|
| PERF-FAIL-001 | Unknown/mismatched workload or incomplete result provenance. [R-002] | Registry/schema validation before comparison. | `PERF_SCHEMA_INVALID`; result/workload, field, expected contract. | Input retained as invalid; no verdict/baseline. | Regenerate with valid registry/result. | PERF-FR-001, -003 / PERF-TEST-001, -003. |
| PERF-FAIL-002 | Runner predicate unknown/fails or shared runner claims authority. [R-002] | Preflight/authority validation. | `PERF_RUNNER_NONCONFORMING` or `PERF_AUTHORITY_INVALID`; runner, predicate, expected/observed. | Result advisory/invalid; never blocking. | Correct runner and rerun; label shared result advisory. | PERF-FR-004, -005 / PERF-TEST-004, -005. |
| PERF-FAIL-003 | Provenance/sampling/metric mismatch. [R-002] | Comparability phase before statistics. | `PERF_INCOMPARABLE`; baseline/candidate IDs and bounded mismatch list. | No regression/pass verdict. | Rerun comparable pair or explicitly create a new baseline. | PERF-FR-006 / PERF-TEST-006. |
| PERF-FAIL-004 | Fewer than 30 valid pairs, migration/throttle/load violation, timeout, or failed sample. [R-002, R-006] | Per sample and pre-statistics validation. | `PERF_INCONCLUSIVE` or `PERF_NOISE_POLICY_FAILED`; counts and predicates. | All samples retained; no authoritative conclusion. | Fix condition and rerun complete attempt. | PERF-FR-002, -010 / PERF-TEST-002, -010. |
| PERF-FAIL-005 | Allocation counter reenters, overflows, or reports false zero. [R-106] | Counter operation/finalization. | `PERF_ALLOCATION_COUNTER_INVALID`; phase, counter, overflow/reentrancy state. | Workload result invalid; process exits cleanly. | Fix instrumentation and rerun positive/zero controls. | PERF-FR-009 / PERF-TEST-009. |
| PERF-FAIL-006 | Regression exceeds strict statistical boundary without approval/profile. [R-304] | Comparator then release evidence gate. | `PERF_REGRESSION_REQUIRES_APPROVAL`; point/bound/threshold, comparison IDs, missing evidence. | Candidate evidence blocked; baseline/result immutable. | Investigate, fix, or approve via ADR with profile/evidence. | PERF-FR-007, -011 / PERF-TEST-007, -011. |
| PERF-FAIL-007 | Input is corrupt, oversize, deeply populated, non-finite, or unit-unknown. [R-201] | Header/bounds/schema parse before unsafe allocation. | `PERF_INPUT_INVALID`; field/offset, limit, observed; no partial verdict. | Bounded diagnostics; allocated buffers released. | Supply valid bounded input. | PERF-FR-012 / PERF-TEST-012/fuzzer. |
| PERF-FAIL-008 | Benchmark cancelled. | Harness process-control boundary. | `PERF_RUN_CANCELLED`; attempt, workload, phase/pair/index. | Owned children terminated; partial samples marked incomplete; no authoritative result. | Manual/CI retry creates new attempt. | PERF-FR-002 / PERF-TEST-002. |

## 10. Observability

| Signal ID | Metric/event/log | Type, unit, and labels | Emission point / owner | Cardinality and cost bound | Alert/gate use | Verification |
|---|---|---|---|---|---|---|
| PERF-OBS-001 | `benchmark_sample` | Raw event; workload/result, phase/pair/index, integer value/unit, validity | Harness sample boundary / performance owner | At most 100,000/result; no formatted per-operation event inside measured loop. | Raw authority/evidence. | PERF-TEST-002. |
| PERF-OBS-002 | `runner_conformance` | Report; runner/contract, predicate, expected/observed, status | Preflight/finalization / performance owner | At most 256 predicates/run. | Authority assignment. | PERF-TEST-004. |
| PERF-OBS-003 | `allocation_phase_total` | Counters; calls and bytes by startup/steady phase and workload | Counter finalization, outside measured operation | Fixed phase/workload cardinality; instrumentation avoids diagnostic formatting in measured loop. | SM-09 gate. | PERF-TEST-009. |
| PERF-OBS-004 | `performance_comparison` | Event/report; result IDs, authority, point/bound/threshold, disposition | Comparator / performance owner | One per comparison; mismatch list max 256. | Regression decision/escalation. | PERF-TEST-006 through -008. |
| PERF-OBS-005 | `performance_objective_coverage` | Gauge/report; objective, owner spec/requirement, applicability | Spec readiness audit / performance owner | Fixed Charter objective inventory. | Domain readiness and roadmap coverage. | PERF-TEST-013. |

Results correlate by source revision, artifact/build ID, configuration,
workload/version, runner/contract ID, and run/attempt. Later domains add trace,
branch, execution, VPID/VTID only when applicable; M0 omits them rather than
using null identities as claims. Raw samples, result documents, comparison,
runner conformance, and required profiles are retained together.

## 11. Test & Verification Plan

Copy/paste from the repository root after implementation:

```bash
nix develop --command bazel test --config=dev //tests/performance/...
nix develop --command bazel test --config=asan //tests/performance/...
nix develop --command bazel test --config=ubsan //tests/performance/...
nix develop --command bazel test --config=benchmark //tests/benchmarks/...
nix develop --command bazel run --config=benchmark //tests/benchmarks:m0_noalloc_reference
nix develop --command bazel run --config=benchmark //tests/benchmarks:m0_positive_alloc_reference
nix develop --command bazel run //tools/performance:compare -- --baseline=tests/performance/fixtures/baseline.json --candidate=tests/performance/fixtures/candidate_over_3pct.json
nix develop --command bazel run //tools/performance:validate_result -- --input=tests/performance/fixtures/valid_result.json
nix develop --command bazel test --config=fuzz //tests/fuzz:performance_result_parser_fuzz_smoke
```

| Requirement ID | Test/benchmark/review ID | Level | Fixture/workload and environment | Pass criterion | Evidence artifact |
|---|---|---|---|---|---|
| PERF-FR-001 | PERF-TEST-001 | Schema/unit | Initial registry plus unknown, version-mismatch, missing-field, duplicate-ID fixtures. | Valid inventory passes; every invalid fixture fails. | `workload-registry-tests.xml`. |
| PERF-FR-002, PERF-NFR-003, PERF-NFR-006 | PERF-TEST-002 | Integration/cancellation | 5 warmup + 30-pair fixture, timeout/failure/cancel attempts. | Phase/order/count exact; every sample retained; invalid not counted valid. | `harness-phase-report.json`, raw samples. |
| PERF-FR-003, PERF-NFR-003 | PERF-TEST-003 | Schema/negative | Complete result and one missing/invalid fixture per required field. | Complete passes; every missing/invalid field rejected. | `performance-schema-coverage.json`. |
| PERF-FR-004, PERF-NFR-004 | PERF-TEST-004 | Integration/negative | Conforming runner simulation and one failed/unknown predicate each. | Only fully conforming simulation can be authoritative. | `runner-conformance-matrix.json`. |
| PERF-FR-005 | PERF-TEST-005 | Integration | Shared-runner provenance, forged authority, simulated 10% delta. | Valid result advisory; forged rejected; no delta-only block. | `shared-runner-authority.json`. |
| PERF-FR-006 | PERF-TEST-006 | Unit/boundary | One fixture per comparison key mismatch and multiple mismatch. | `PERF_INCOMPARABLE`; complete bounded mismatch identity; no verdict. | `comparability-matrix.json`. |
| PERF-FR-007, PERF-NFR-001 | PERF-TEST-007 | Statistical/golden | Exact 3%, above 3% significant, noisy, improvement, higher/lower-is-better corpora. | Expected disposition and numeric goldens repeat 100/100. | `comparator-boundaries.json`. |
| PERF-FR-008, PERF-NFR-004 | PERF-TEST-008 | Unit | Authority cross-product and mixed authority. | Blocking dispositions only when both inputs authoritative. | `authority-cross-product.json`. |
| PERF-FR-009, PERF-NFR-002 | PERF-TEST-009 | Integration/benchmark | Zero/positive controls in dev/release, reentrancy and overflow injection. | Exact zero and positive targets met; error fixtures invalidate. | `allocation-counter-report.json`. |
| PERF-FR-010 | PERF-TEST-010 | Integration/fault | Migration, throttle, >=1% background load, outlier, timeout fixtures. | Each policy violation is retained and prevents authoritative conclusion; outlier not deleted. | `noise-policy-fixtures.json`. |
| PERF-FR-011 | PERF-TEST-011 | End-to-end/inspection | Material regression with/without profile and ADR approval. | Missing evidence blocks; complete reviewed bundle can be accepted without changing threshold/history. | `regression-escalation.json`. |
| PERF-FR-012, PERF-NFR-005 | PERF-TEST-012 | Resource/fuzz | Max-valid and over-limit/non-finite/corrupt inputs under RSS measurement and parser fuzzing. | Max valid <=256 MiB RSS; all over-limit reject safely; fuzz has zero crash/sanitizer finding. | `performance-parser-resource.json`, fuzz report. |
| PERF-FR-013 | PERF-TEST-013 | Inspection | Charter objectives against roadmap/spec requirements. | Every objective has introducing owner or explicit not-yet-applicable status. | `performance-objective-coverage.json`. |

## 12. Open Questions

No open questions.
