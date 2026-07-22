# Performance Measurement Foundation

**Status:** Draft  
**Milestone:** M0  
**Owners:** Performance owner; CI owner; build/toolchain owner  
**Last updated:** 2026-07-21  
**Depends on:** `specs/10-build-environment.md` and `specs/13-ci-quality.md` (Draft; both are Ready prerequisites); Foundation specs (Ready)  
**Foundation decisions:** D-006, D-010  
**Risks addressed:** R-002, R-006, R-106, R-201, R-304

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
| PERF-FR-002 | The harness shall separate setup/warmup from measured steady state and retain every raw sample. | A paired comparison runs exactly five warmup pairs (five baseline plus five candidate invocations, ten total) before at least 30 measured pairs; Section 6.4 defines initial/alternating order. Phase boundaries and every raw warmup/measured record are retained; timeout/failed samples remain typed invalid and make the attempt `inconclusive`, never silently discarded/replaced. | PERF-TEST-002 harness phase/order and raw-sample reconciliation. | SM-07, D-006; resolved `q-0006`. |
| PERF-FR-003 | Every performance document shall validate against the five complete schemas and canonical bytes in Sections 6.2-6.3. | Workload, raw sample, result, controlled-runner, and comparison documents contain every required typed field, enum, bound, conditional field, identity, and relationship. The five Section 6.3 examples reproduce their literal content digests; result raw-array order/count/digest and all summary selection/rounding rules reconcile. Missing/extra/type/enum/bound/noncanonical/float/native-layout/wrong-digest fixtures each return the exact typed parse error. | PERF-TEST-003 five-schema canonicalization, fixed-digest, raw-order/statistic, and one-negative-per-field/relationship corpus. | SM-07, R-002, R-201; resolved `q-0007` and `q-0011`. |
| PERF-FR-004 | A runner shall receive `authority=authoritative` only after `M0-CONTROLLED-RUNNER-v1` conformance succeeds for that run. | Contract verifies exact CPU/ISA, isolated affinity set, kernel/image, fixed governor/frequency policy, disabled/recorded boost policy, no CPU migration, no thermal throttling, background-load threshold, memory/NUMA policy, storage/device/filesystem/cache state when applicable, toolchain, workload, sampling, and noise policy; any unknown/failure makes result advisory or invalid, never authoritative. | PERF-TEST-004 runner conformance and one-negative-per-predicate matrix. | D-006, D-010, R-002. |
| PERF-FR-005 | Results from shared GitHub-hosted runners shall always be advisory. | Producer sets `runner_class=github_hosted_shared` and `authority=advisory`; comparator may describe delta but cannot return blocking regression; a forged authoritative label is rejected. | PERF-TEST-005 authority fixtures including simulated 10% delta. | SM-12, D-010, R-002. |
| PERF-FR-006 | The comparator shall reject inputs that are not semantically and environmentally comparable. | Schema/workload/metric/unit/direction, artifact role, runner contract/identity, CPU/ISA, affinity, kernel/image, toolchain except the intended candidate change, optimization, storage policy, sampling method, and declared comparison-key fields must match; each mismatch returns `PERF_INCOMPARABLE` naming all discovered mismatches and no regression verdict. | PERF-TEST-006 pairwise mismatch matrix. | SM-08, R-002. |
| PERF-FR-007 | The comparator shall apply the fully specified paired-bootstrap-median, strict greater-than-3.0% algorithm in Section 6.4. | Exact 30,000,000-ppb fixture does not flag; a fixture whose one-sided 99% lower bound is strictly greater returns `regression_requires_approval`; direction, role-ordered seed, SHA-256 counter sampling, rounding, overflow, median, and quantile rules are exact. The checked-in corpus records seed bytes, first accepted sample indices, per-pair degradations, selected resample medians, sorted index 99, and final bound/disposition. | PERF-TEST-007 deterministic intermediate/final golden corpus. | SM-08, D-006, R-002; resolved `q-0006`. |
| PERF-FR-008 | The comparator shall preserve authority in its decision. | Authoritative comparable results may produce blocking pass/regression/inconclusive; any advisory input yields `advisory_only`; mixed authority cannot produce a blocking verdict; result states whether the threshold and significance tests fired. | PERF-TEST-008 cross-product of baseline/candidate authorities. | D-010, R-002. |
| PERF-FR-009 | The allocation counter shall measure startup and designated steady-state regions separately. | `m0_noalloc_reference_v1` reports exactly 0 allocation calls and 0 allocated bytes in steady state after startup for all measured iterations; `m0_positive_alloc_reference_v1` reports at least 1 call and positive bytes; counter reentrancy/overflow becomes failure, not zero. | PERF-TEST-009 allocation positive/zero/reentrancy/overflow fixtures. | SM-09, future R-106 mitigation. |
| PERF-FR-010 | Noise policy shall run before and during measurement and preserve rather than trim samples. | Runner preflight records every predicate; each sample records migration, throttle, background load, and failure. Any preflight/sample violation sets `noise_state=failed`, `reason=PERF_NOISE_POLICY_FAILED`, and terminal comparison `state=inconclusive` regardless of authority; any timeout/failed sample sets `reason=PERF_SAMPLE_FAILED` and the same state. Outliers remain valid raw data and are never deleted. | PERF-TEST-010 terminal-state and injected migration/throttle/load/outlier/sample-failure fixtures. | R-002. |
| PERF-FR-011 | A material regression shall require retained diagnostic profile evidence and an approved ADR before acceptance. | For authoritative `regression_requires_approval`, evidence includes raw results, comparator report, workload/build IDs, and a profile bundle with elapsed time, cycles, instructions, branches, cache misses, context switches, and bounded symbolized hotspots where collection is supported; no tool automatically raises the threshold. | PERF-TEST-011 regression escalation and missing-profile/approval fixtures. | D-006, R-304. |
| PERF-FR-012 | Canonical JSON parsing and comparison shall enforce bounds and fail safely. | Before proportional allocation: each document is at most 16 MiB/depth 16, samples at most 100,000/result, workloads at most 1,024/bundle, strings at most 4 KiB, mismatch rows at most 256, integers within signed 64-bit/non-negative domains, bootstrap work exactly 10,000 resamples, comparator wall time at most 120 seconds, and process RSS at most 256 MiB. Corrupt/noncanonical/oversize/overflow/timeout/native-layout input returns the Section 6.3 typed error and no comparison state. | PERF-TEST-012 parser/resource/canonicalization/overflow/timeout corpus and fuzz target. | C-09, R-201 policy precursor; resolved `q-0007`. |
| PERF-FR-013 | Later domains shall declare when each Charter product objective becomes applicable and register its workload before implementation becomes Active. | Readiness audit covers greater-than-3%, 1.5x CPU recording, 2.5x syscall recording after fast path, 20M descriptors/s/core, 80% raw sequential writer throughput, and local pause/cancel p99 below 50 ms; each is either mapped to an introducing requirement/spec or explicitly not yet applicable. | PERF-TEST-013 roadmap objective reconciliation. | Charter Success Metrics after SM-12, C-05. |

## 5. Non-Functional Requirements

| ID | Quality attribute and target | Conditions / workload | Acceptance criteria | Verification method and evidence |
|---|---|---|---|---|
| PERF-NFR-001 | Comparator threshold: exact 30,000,000 ppb is accepted; only a one-sided 99% lower bound strictly greater flags regression. | Five paired warmup rounds, at least 30 alternating all-valid measured pairs, 10,000 Section 6.4 resamples, comparable authoritative inputs, zero migration/throttle, background utilization below 10,000 ppm, no trimming. | Intermediate and final boundary goldens repeat bit-for-bit across 100 runs and independent implementations. | PERF-TEST-007 `comparator-boundaries.json`. |
| PERF-NFR-002 | Allocation reference: exactly 0 calls and 0 bytes in designated steady state; positive control greater than 0. | Pinned Clang dev/release fixtures after startup, at least 30 measured iterations. | Zero fixture exact in both configs; positive fixture detected every iteration; no counter error. | PERF-TEST-009 `allocation-counter-report.json`. |
| PERF-NFR-003 | Schema completeness: 100% of required provenance/sample fields and 100% raw-sample reconciliation. | Every M0 workload result, advisory or authoritative. | Missing-field count zero; declared measured count equals valid+invalid retained raw records; all SHA-256 references resolve. | PERF-TEST-002/003 schema report. |
| PERF-NFR-004 | Authority safety: 0 shared-runner or nonconforming-runner blocking verdicts. | Authority cross-product, conformance negative matrix, simulated 10% shared-runner regression. | Every such result is advisory/inconclusive/invalid; none is blocking pass/regression. | PERF-TEST-004/005/008. |
| PERF-NFR-005 | Comparator resource bound: at most 16 MiB/document, depth 16, 100,000 samples/result, 10,000 resamples, 120 seconds, and 256 MiB peak process RSS. | Reference environment; maximum valid two-result synthetic inputs; startup/tool memory included. | Valid maximum completes within every cap; each exact-over fixture rejects before proportional excess allocation/work. | PERF-TEST-012 resource report. |
| PERF-NFR-006 | Workload timeout truthfulness: 0 timed-out or failed samples are counted as valid. | Registered per-sample timeout and timeout/failure injection. | Invalid count and diagnostic exact; authoritative comparison is inconclusive if fewer than 30 valid pairs. | PERF-TEST-002/010. |

No M0 product-speed number is authoritative merely because the tooling passes.
The CPU-bound recording, syscall-heavy recording, descriptor, writer, and
pause/cancel objectives become measurable only in their introducing specs as
mapped by PERF-FR-013.

## 6. Interfaces / Contracts

### 6.1 Workload registry

`M0-PERF-WORKLOAD-v1` is a committed finite table. The following are the five
complete normative M0 records; every string and array is literal. All records
have `schema=M0-PERF-WORKLOAD-v1`, `workload_version=1`,
`storage_applicable=false`, and the common
`registry_sha256=17949ac8fe9d6d2e303a95e5881cc6d4574fb99203bb1c845b06461cbfc134cc`.

| Workload ID / owner | Executable / literal arguments | Metric | Dataset / setup / measured operation | Concurrency / data size | Sampling / timeout | Allocation / introducing requirement |
|---|---|---|---|---|---|---|
| `m0_harness_reference_v1` / `performance.owner` | `//tests/benchmarks:m0_harness_reference` / `["--iterations=1000000"]` | `{name:duration_ns,unit:ns,direction:lower_is_better}` | `1000000 sequential uint64 additions; data_size unit is iterations` / `parse fixed arguments and initialize the result sink before timing` / `set the accumulator to zero, add integers 1 through 1000000 in order, and publish the uint64 sum to the result sink` | 1 / 1000000 | `{mode:paired,warmup_pairs:5,minimum_measured_pairs:30,resamples:10000}` / 60000 ms | `none` / `PERF-FR-002` |
| `m0_noalloc_reference_v1` / `performance.owner` | `//tests/benchmarks:m0_noalloc_reference` / `["--iterations=1000000"]` | `{name:allocation_calls,unit:count,direction:exact}` | `1000000 calls to the registered no-allocation fixture; data_size unit is calls` / `install the allocation counter and construct fixture state before entering steady state` / `invoke the fixture exactly 1000000 times inside the designated steady-state allocation region` | 1 / 1000000 | `{mode:single,warmup_pairs:0,minimum_measured_pairs:30,resamples:0}` / 60000 ms | `steady_state` / `PERF-FR-009` |
| `m0_positive_alloc_reference_v1` / `performance.owner` | `//tests/benchmarks:m0_positive_alloc_reference` / `["--allocations=1","--bytes=64"]` | `{name:allocation_calls,unit:count,direction:exact}` | `one 64-byte allocation and release per measured iteration; data_size unit is allocations` / `install the allocation counter before entering steady state` / `allocate one 64-byte block, observe non-null success, and release it inside the designated steady-state allocation region` | 1 / 1 | `{mode:single,warmup_pairs:0,minimum_measured_pairs:30,resamples:0}` / 60000 ms | `steady_state` / `PERF-FR-009` |
| `m0_comparator_boundary_v1` / `performance.owner` | `//tests/performance:emit_fixture_result` / `["--fixture=lower_boundary"]` | `{name:synthetic_value,unit:count,direction:lower_is_better}` | `checked-in 30-pair integer boundary corpus; data_size unit is pairs` / `load and schema-validate the checked-in boundary corpus before timing` / `emit the next declared integer sample without transformation` | 1 / 30 | `{mode:paired,warmup_pairs:5,minimum_measured_pairs:30,resamples:10000}` / 10000 ms | `none` / `PERF-FR-007` |
| `m0_concurrent_native_v1` / `corpus.owner` | `//tests/concurrent:corpus_parent` / `[]` | `{name:duration_ns,unit:ns,direction:lower_is_better}` | `one complete native parent plus exec-child corpus run; data_size unit is runs` / `prepare the process group, socketpair, absolute deadline, and report sink before parent launch` / `measure parent launch through validated joins, child wait, logical result, descriptor closure, and cleanup proof` | 7 / 1 | `{mode:single,warmup_pairs:0,minimum_measured_pairs:30,resamples:0}` / 10000 ms | `none` / `CORP-FR-012` |

The registry digest is SHA-256 of the canonical object
`{schema:"M0-PERF-WORKLOAD-REGISTRY-v1",workloads:[...]}` where `workloads`
contains the five table records in table order with `registry_sha256` omitted
from each. Inserting or changing a record changes the computed digest; each
materialized record then carries that same digest. PERF-TEST-001 reconstructs
the object and must obtain the literal digest above. Higher-is-better comparator
tests use a separately identified test-only schema fixture; they do not mutate
these five production registry rows.

Registry changes require owner, introducing requirement, schema validation,
and review. Workload identity changes whenever measured semantics, dataset,
concurrency, phase, metric, or sample rule changes.

### 6.2 Raw sample and result schema

All five schemas use `M0-CANONICAL-JSON-v1` from spec `10`. Unknown fields are
rejected; every listed field is required, with `null` used only where a row
explicitly permits it. `id` is ASCII
`[a-z0-9][a-z0-9._:-]{0,127}`, `hex64` is `[0-9a-f]{64}`, `relpath` has the
spec-`10` package-path rules, strings are NFC and at most 4 KiB, non-negative
integers are 0 through `9223372036854775807`, and `ratio_ppb` is signed 64-bit.
Each document is at most 16 MiB and depth 16. Metric `unit` is exactly one of
`ns|bytes|count|descriptors_per_second|bytes_per_second|ratio_ppb`; `direction`
is `lower_is_better|higher_is_better|exact`.

#### 6.2.1 `M0-PERF-WORKLOAD-v1`

| Field | Type / bound | Relationship |
|---|---|---|
| `schema`, `workload_id`, `workload_version`, `owner` | literal schema; `id`; uint32 >=1; `id` | ID/version unique in registry. |
| `executable`, `arguments` | Bazel label string 1-512; array 0-64 strings each <=1 KiB | Exact invocation; no shell expansion. |
| `metric` | object | Exactly `name:id`, `unit` enum, `direction` enum. `exact` is not bootstrapped. |
| `dataset`, `setup`, `measured_operation` | strings 1-4096 | Semantics change requires version change. |
| `concurrency`, `data_size` | uint32 >=1; non-negative int64 plus declared unit in `dataset` | Exact workload parameters. |
| `sampling` | object | Exactly `mode:single|paired`, `warmup_pairs:uint32`, `minimum_measured_pairs:uint32`, `resamples:uint32`. Paired comparator rows require 5, >=30, and 10,000; single rows require `resamples=0`. |
| `timeout_ms` | uint32 1-600000 | Per invocation. |
| `storage_applicable` | boolean | Governs storage fields in sample/result. |
| `allocation_phase` | `none|startup|steady_state` | Governs allocation fields. |
| `introducing_requirement`, `registry_sha256` | requirement token `[A-Z][A-Z0-9]*-(?:FR|NFR)-[0-9]{3}`; `hex64` | Requirement resolves; digest identifies canonical registry excluding every row's own digest field. |

#### 6.2.2 `M0-PERF-RAW-SAMPLE-v1`

| Field | Type / bound | Relationship |
|---|---|---|
| `schema`, `run_id`, `workload_id`, `workload_version` | literal; `id`; `id`; uint32 | Resolve to one workload/run. Raw samples do not contain result ID, avoiding a digest cycle. |
| `phase`, `pair_index`, `order` | `warmup|measured`; uint32; `baseline_first|candidate_first|standalone` | Paired runs have one baseline and candidate row per index/order. |
| `start_ns`, `end_ns`, `duration_ns` | non-negative int64 | Monotonic; `end_ns >= start_ns`; duration equals subtraction without overflow. |
| `metric` | object | Exactly workload `name`, `unit`, and signed-int64 `value`; value positive for ratio denominator metrics. |
| `valid`, `invalid_reason` | boolean; `none|timeout|process_failure|migration|throttle|background_load|counter_error|cancelled` | `valid=true` iff reason is `none`. |
| `allocation` | `null` or object | Null iff workload phase is `none`; otherwise exactly non-negative `calls` and `bytes`. |
| `cpu`, `migration_count`, `throttled`, `background_utilization_ppm` | uint32; uint32; boolean; uint32 0-1000000 | Conformance uses migration=0, false, and <10000 ppm. |
| `storage` | `null` or object | Null iff storage not applicable; otherwise exactly `device:id`, `filesystem:id`, `cache_state:cold|warm|mixed`, `bytes:non-negative int64`. |

#### 6.2.3 `M0-PERF-RESULT-v1`

| Field | Type / bound | Relationship |
|---|---|---|
| `schema`, `result_id`, `run_id`, `workload_id`, `workload_version`, `role` | literal; `hex64`; `id`; `id`; uint32; `baseline|candidate|standalone` | `result_id` is SHA-256 of canonical object with only `result_id` removed. |
| `metric`, `dataset`, `concurrency` | exact workload metric object; string; uint32 | Equal registry values. |
| `source_revision`, `artifact_subject`, `artifact_sha256`, `build_id` | 40/64 lower hex; `orus_executable|benchmark_fixture`; `hex64`; `hex64` | Role artifacts differ only by intended candidate change recorded in comparison key. |
| `configuration`, `compiler`, `optimization` | strings 1-256 | Exact declared build facts. |
| `runner` | object | Exactly `runner_id:id`, `runner_class:controlled|github_hosted_shared|other`, `contract_sha256:hex64`, `conformance:pass|fail|unknown`, `authority:authoritative|advisory|invalid`; authoritative iff controlled/pass. |
| `host` | object | Exactly `cpu:string`, `isa:string[1..128]`, `affinity:uint32[1..1024]` sorted/unique, `numa:string`, `kernel:string`, `image_sha256:hex64`, `governor:string`, `frequency_khz:non-negative int64`, `boost:enabled|disabled|unknown`. |
| `storage` | `null` or object | Null iff not applicable; otherwise sample storage identity plus `sequential_reference_bytes_per_second:non-negative int64`. |
| `sampling` | object | Exactly `mode:single|paired`, `warmup_pairs_required:uint32`, `warmup_pairs_completed:uint32`, `minimum_measured_pairs:uint32>=1`, `measured_pairs_attempted:uint32`, `valid_pairs:uint32`, `invalid_pairs:uint32`, `resamples:uint32`, and `timeout_ms:uint32[1..600000]`; equations are below. |
| `noise_state` | `pass|failed` | Failed if any preflight/measured predicate or sample fails. |
| `raw_samples` | object | Exactly `storage:embedded|external`, `path:relpath|null`, `byte_length:non-negative int64`, `sha256:hex64`, `count:uint32<=100000`; external requires path, embedded requires null; digest covers the exact canonical array order below. |
| `statistics` | object | Exactly signed-int64 `minimum`, `maximum`, `median`, `median_absolute_deviation`, and `percentiles` array 0-32 of `{rank_ppm:uint32[1..999999],value:int64}` sorted unique; derivation is exact below. |
| `tool_version`, `attempt_id`, `validation_state` | string 1-128; `id`; `schema_valid|invalid` | Accepted result is schema_valid. |

Sampling relationships are normative. Every result copies `mode`,
`warmup_pairs_required`, `minimum_measured_pairs`, `resamples`, and `timeout_ms`
from its workload (`warmup_pairs` maps to `warmup_pairs_required`). In paired
mode, accepted results require required/completed warmups = 5,
`measured_pairs_attempted >= minimum_measured_pairs >= 30`,
`valid_pairs + invalid_pairs = measured_pairs_attempted`, and
`resamples=10000`; baseline and candidate results have identical counts and one
sample per role for every pair index. In single mode, required/completed
warmups are 0, `measured_pairs_attempted >= minimum_measured_pairs`,
`valid_pairs + invalid_pairs = measured_pairs_attempted`, `resamples=0`, and
`role=standalone`. A result's raw array contains exactly
`warmup_pairs_completed + measured_pairs_attempted` objects; its warmup indices
are contiguous `0..completed-1`, measured indices are contiguous
`0..attempted-1`, and `raw_samples.count` equals that sum. A valid pair is one
whose baseline and candidate raw objects are both valid; both results report
the same resulting pair counts. Any count, role, index, or workload mismatch is
`PERF_RELATIONSHIP_INVALID`.

The raw-sample array order is fixed: all `phase=warmup` rows in increasing
`pair_index`, then all `phase=measured` rows in increasing `pair_index`. For a
result document there is only its declared role's row at each index. Across a
paired run, the `order` value at an index is identical in the two result arrays
and follows Section 6.4. The identified raw bytes are the
`M0-CANONICAL-JSON-v1` serialization of that array with no terminal LF; every
embedded/external representation must have the declared length and SHA-256.

Statistics use only valid measured primary `metric.value` integers from that
ordered array. Sort a copy ascending. `minimum`/`maximum` select the first/last.
Median uses the Section 6.4 odd/even selection and half-away-from-zero rule.
For MAD, compute `abs(value-median)` in checked >=128-bit arithmetic for every
selected value, sort those integer deviations, and apply the same median rule.
For percentile `rank_ppm`, select the one-based nearest-rank element
`ceil(rank_ppm*N/1000000)`, implemented as checked integer
`(rank_ppm*N + 999999) / 1000000` and clamped to 1..N; no interpolation occurs.
The emitted percentile `value` is that selected original metric integer. Empty
valid measured populations cannot emit a result; overflow is
`PERF_INTEGER_OVERFLOW`.

#### 6.2.4 `M0-CONTROLLED-RUNNER-v1`

| Field | Type / bound | Relationship |
|---|---|---|
| `schema`, `runner_id`, `contract_id`, `contract_sha256`, `run_id` | literal; two `id`; `hex64`; `id` | Digest is SHA-256 of this canonical runner document with only `contract_sha256` removed, including per-run observations/conformance. |
| `runner_class` | `controlled|github_hosted_shared|other` | Only controlled may pass conformance. |
| `predicates` | 1-256 rows | Each row exactly `path:string[1..256]`, `operator:eq|u32_eq|set_eq|max_exclusive`, `expected`, `observed`, `status:pass|mismatch|unavailable`; paths unique/sorted. `eq` values are same-type string<=4KiB or boolean; `u32_eq`/`max_exclusive` use uint32; `set_eq` uses sorted unique `id[1..1024]`; observed may be null only with unavailable. |
| `measurement` | object | Exactly `affinity:uint32[1..1024]`, `preflight_ms=5000`, `migration_count:uint32`, `throttle_events:uint32`, `max_background_utilization_ppm:uint32`, `clock:id`, `storage_applicable:boolean`. |
| `conformance`, `authority` | `pass|fail|unknown`; `authoritative|advisory|invalid` | Authoritative iff controlled, every predicate pass, migration/throttle zero, utilization <10000 ppm. Shared is always advisory. |

#### 6.2.5 `M0-PERF-COMPARISON-v1`

| Field | Type / bound | Relationship |
|---|---|---|
| `schema`, `comparison_id`, `comparator_version` | literal; `hex64`; literal `M0-PAIRED-BOOTSTRAP-v1` | ID hashes canonical object excluding only `comparison_id`. |
| `baseline_result_id`, `candidate_result_id` | two distinct `hex64` | Role order is semantic and fixed. |
| `state` | `incomparable|inconclusive|advisory_only|no_regression_detected|regression_requires_approval` | Terminal precedence is Section 6.4; invalid inputs emit an error instead of this document. |
| `reason` | `none|PERF_INCOMPARABLE|PERF_INSUFFICIENT_SAMPLES|PERF_SAMPLE_FAILED|PERF_NOISE_POLICY_FAILED|PERF_ADVISORY_INPUT|PERF_REGRESSION_REQUIRES_APPROVAL` | Must match state. |
| `mismatches` | 0-256 sorted rows `{path,baseline,candidate}` strings <=1024 | Nonempty iff incomparable. |
| `seed_sha256`, `resamples`, `valid_pairs` | `hex64|null`; uint32; uint32 | Present seed and 10,000 only when statistics execute; otherwise seed null/resamples 0. |
| `point_estimate_ppb`, `lower_bound_ppb` | `ratio_ppb|null` | Both non-null only after statistical execution. |
| `threshold_ppb` | int64 | Exactly 30000000. |
| `threshold_fired`, `significance_fired` | boolean or `null` | Non-null with statistics; both true iff lower bound > threshold. |
| `authority`, `noise_state`, `next_action` | `authoritative|advisory`; `pass|failed|unknown`; `none|rerun|approval|informational` | An invalid input emits an error rather than a comparison; valid state/action mapping follows Section 6.4. |
| `golden_corpus_sha256` | `hex64` | Digest of intermediate-golden corpus version used by comparator tests. |

State relationships are exact: `incomparable` uses
`PERF_INCOMPARABLE/next_action=rerun`; `inconclusive` uses one of
`PERF_INSUFFICIENT_SAMPLES|PERF_SAMPLE_FAILED|PERF_NOISE_POLICY_FAILED` and
`next_action=rerun`; `advisory_only` uses
`PERF_ADVISORY_INPUT/informational`; `no_regression_detected` uses
`none/none`; and `regression_requires_approval` uses
`PERF_REGRESSION_REQUIRES_APPROVAL/approval`.

### 6.3 Controlled runner

The runner schema's predicate paths cover CPU/ISA, isolated cores/affinity,
kernel/image, governor/frequency/boost, NUMA/memory, storage when applicable,
toolchains, workload, sampling, and clock. The selected M0 contract requires:

- the benchmark process is pinned to the declared isolated logical CPU set;
- observed migration count is 0;
- observed hardware thermal-throttle events are 0;
- background utilization on benchmark CPUs is below 1.0% during a 5-second
  preflight and each measured interval;
- exactly five paired warmup rounds (ten invocations), then at least 30
  all-valid measured pairs in strict alternating pair order, with starting
  member derived and recorded under Section 6.4;
- no measured sample is trimmed; any predicate failure makes the comparison
  `inconclusive` with `PERF_NOISE_POLICY_FAILED`.

Provisioning and exact runner hardware remain outside M0. A contract can exist
without a conforming runner.

#### 6.3.1 Canonicalization and typed errors

Before schema parsing, the byte profile rejects BOM, whitespace/member-order
differences, duplicate names, non-NFC/invalid Unicode, floats/exponents,
non-shortest/out-of-range integers, unknown fields, and excess size/depth.
Errors use canonical `M0-PERF-ERROR-v1` with exactly `schema`, `code`,
`document_schema`, `field_path` (<=512 bytes), `expected` and redacted
`observed` (<=1024 bytes), `limit:int64|null`, `offset:int64|null`, and
`message` (<=4 KiB). `code` is one of `PERF_SCHEMA_UNKNOWN`,
`PERF_NONCANONICAL`, `PERF_FIELD_MISSING`, `PERF_FIELD_TYPE`,
`PERF_FIELD_BOUND`, `PERF_ENUM_INVALID`, `PERF_RELATIONSHIP_INVALID`,
`PERF_DIGEST_INVALID`, `PERF_INTEGER_OVERFLOW`, or `PERF_RESOURCE_LIMIT`.
No comparison document is emitted for these parse/identity failures.

#### 6.3.2 Fixed byte goldens

This valid canonical raw-sample fixture has content SHA-256
`be68aca7cc59414f245daaf3dca7bfa22b6463d97e33e80771655a6bd58d8b78`.
The one-element canonical raw array containing it is 420 bytes with SHA-256
`0a3055e4061248fec8f1fb2ee50ded4668e2b08515a5ebc956b1b60e76c1c9e1`:

```json
{"allocation":null,"background_utilization_ppm":0,"cpu":2,"duration_ns":100,"end_ns":200,"invalid_reason":"none","metric":{"name":"duration_ns","unit":"ns","value":100},"migration_count":0,"order":"standalone","pair_index":0,"phase":"measured","run_id":"run.fixture","schema":"M0-PERF-RAW-SAMPLE-v1","start_ns":100,"storage":null,"throttled":false,"valid":true,"workload_id":"fixture_duration_v1","workload_version":1}
```

The complete canonical workload example has registry SHA-256
`3e6b1322718c773227c66926a57ebc5e473cf6892bd43707e7af37bac15d12bb`
and content SHA-256
`6ae68e519f47866d9aca4574dbda964e3557b9536bb0819a6314d564aed745d0`.
For this test-only example, the registry digest uses the Section 6.1 algorithm
over a one-row `workloads` array containing this fixture with its
`registry_sha256` member omitted:

```json
{"allocation_phase":"none","arguments":[],"concurrency":1,"data_size":1,"dataset":"one fixture operation; data_size unit is operations","executable":"//tests/performance:fixture_duration","introducing_requirement":"PERF-FR-003","measured_operation":"execute one fixture operation","metric":{"direction":"lower_is_better","name":"duration_ns","unit":"ns"},"owner":"performance.owner","registry_sha256":"3e6b1322718c773227c66926a57ebc5e473cf6892bd43707e7af37bac15d12bb","sampling":{"minimum_measured_pairs":1,"mode":"single","resamples":0,"warmup_pairs":0},"schema":"M0-PERF-WORKLOAD-v1","setup":"initialize fixture state before timing","storage_applicable":false,"timeout_ms":1000,"workload_id":"fixture_duration_v1","workload_version":1}
```

The complete canonical shared-runner example has
`contract_sha256=1cbc14832a9639dd2a811c95639692d3582da41499baae3e58ad09401be61f90`
and content SHA-256
`29751cbe816b7edad016dddef96f5dbd28a9e5290e5c4b664e5f5effae1f3e62`:

```json
{"authority":"advisory","conformance":"unknown","contract_id":"runner.fixture.contract","contract_sha256":"1cbc14832a9639dd2a811c95639692d3582da41499baae3e58ad09401be61f90","measurement":{"affinity":[2],"clock":"monotonic_raw","max_background_utilization_ppm":0,"migration_count":0,"preflight_ms":5000,"storage_applicable":false,"throttle_events":0},"predicates":[{"expected":"github_hosted_shared","observed":"github_hosted_shared","operator":"eq","path":"runner.class","status":"pass"}],"run_id":"run.fixture","runner_class":"github_hosted_shared","runner_id":"runner.fixture","schema":"M0-CONTROLLED-RUNNER-v1"}
```

The complete canonical result example resolves the raw-array and runner
digests above. Its derived
`result_id=0c7dc602e1a6cf701f6d9cb818b9b5189c783d4649b3627c7ccde2161eb65a4e`
and full content SHA-256 is
`53bf9f36b775d084120636acbc80490257e8f32d3e9e662de3fd0dae3a751f4b`:

```json
{"artifact_sha256":"aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa","artifact_subject":"benchmark_fixture","attempt_id":"attempt.fixture","build_id":"bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb","compiler":"fixture-compiler-1","concurrency":1,"configuration":"dev","dataset":"one fixture operation; data_size unit is operations","host":{"affinity":[2],"boost":"unknown","cpu":"fixture-cpu","frequency_khz":0,"governor":"unknown","image_sha256":"cccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccc","isa":"sse2","kernel":"fixture-kernel","numa":"node0"},"metric":{"direction":"lower_is_better","name":"duration_ns","unit":"ns"},"noise_state":"pass","optimization":"O0","raw_samples":{"byte_length":420,"count":1,"path":"samples/fixture.json","sha256":"0a3055e4061248fec8f1fb2ee50ded4668e2b08515a5ebc956b1b60e76c1c9e1","storage":"external"},"result_id":"0c7dc602e1a6cf701f6d9cb818b9b5189c783d4649b3627c7ccde2161eb65a4e","role":"standalone","run_id":"run.fixture","runner":{"authority":"advisory","conformance":"unknown","contract_sha256":"1cbc14832a9639dd2a811c95639692d3582da41499baae3e58ad09401be61f90","runner_class":"github_hosted_shared","runner_id":"runner.fixture"},"sampling":{"invalid_pairs":0,"measured_pairs_attempted":1,"minimum_measured_pairs":1,"mode":"single","resamples":0,"timeout_ms":1000,"valid_pairs":1,"warmup_pairs_completed":0,"warmup_pairs_required":0},"schema":"M0-PERF-RESULT-v1","source_revision":"0123456789abcdef0123456789abcdef01234567","statistics":{"maximum":100,"median":100,"median_absolute_deviation":0,"minimum":100,"percentiles":[{"rank_ppm":500000,"value":100}]},"storage":null,"tool_version":"fixture-1","validation_state":"schema_valid","workload_id":"fixture_duration_v1","workload_version":1}
```

The complete canonical incomparable comparison example has derived
`comparison_id=381537010c45b765b28b9dfcb2dfa34bdc0e080e4b9644a60aad33b58b594b4b`
and full content SHA-256
`8aede2e8179c52f9d6c96618510ebe6e4311030b6d91f292a58bca4d7a4c94c6`:

```json
{"authority":"advisory","baseline_result_id":"1111111111111111111111111111111111111111111111111111111111111111","candidate_result_id":"2222222222222222222222222222222222222222222222222222222222222222","comparator_version":"M0-PAIRED-BOOTSTRAP-v1","comparison_id":"381537010c45b765b28b9dfcb2dfa34bdc0e080e4b9644a60aad33b58b594b4b","golden_corpus_sha256":"3333333333333333333333333333333333333333333333333333333333333333","lower_bound_ppb":null,"mismatches":[{"baseline":"fixture-a","candidate":"fixture-b","path":"host.cpu"}],"next_action":"rerun","noise_state":"unknown","point_estimate_ppb":null,"reason":"PERF_INCOMPARABLE","resamples":0,"schema":"M0-PERF-COMPARISON-v1","seed_sha256":null,"significance_fired":null,"state":"incomparable","threshold_fired":null,"threshold_ppb":30000000,"valid_pairs":0}
```

Whitespace/member-reordered versions of each example are deliberately
noncanonical and return `PERF_NONCANONICAL`; PERF-TEST-003 retains their exact
byte hashes and never normalizes them into acceptance.

### 6.4 Statistical comparison contract

`M0-PAIRED-BOOTSTRAP-v1` is exact:

1. **Schedule.** Derive `schedule_seed = SHA256(ASCII("M0-PERF-SCHEDULE-v1\n")
   || hex-decode(baseline artifact SHA-256) || hex-decode(candidate artifact
   SHA-256) || UTF8(workload_id))`. Bit 0 of byte 0 selects baseline-first when
   0, candidate-first when 1. Run five warmup pair rounds, alternating the
   first member each round; then continue that alternation for at least 30
   measured pairs. Thus each role has exactly five warmups. Warmup failure
   stops before measurement; any measured invalid sample makes the attempt
   inconclusive, with no replacement/trimming.
2. **Pair degradation.** Inputs and denominators are positive signed-64-bit
   integer base units. Lower-is-better numerator is `candidate-baseline`;
   higher-is-better numerator is `baseline-candidate`. Both directions use the
   positive `baseline` as denominator, so the percentage is always relative to
   baseline. Compute the mathematical integer numerator times
   `1,000,000,000` using unbounded or checked >=128-bit intermediates, divide
   by the positive denominator, and round to signed `ratio_ppb` with ties away
   from zero. Specifically truncate quotient toward zero; if
   `2*abs(remainder) >= denominator`, increment quotient one unit away from
   zero. Final signed-64 overflow is `PERF_INTEGER_OVERFLOW`. `exact` metrics
   use their declared equality test and never enter this algorithm.
3. **Median.** Sort signed ppb values ascending. Odd count selects the middle.
   Even count adds the two middle values in checked >=128-bit arithmetic,
   divides by two, and rounds a half away from zero. The original-pair median
   is `point_estimate_ppb`.
4. **Comparator seed and sampling.** The baseline/candidate roles are never
   lexically reordered. `seed_sha256 = SHA256(ASCII("M0-PAIRED-BOOTSTRAP-v1\n")
   || hex-decode(baseline_result_id) || hex-decode(candidate_result_id))`.
   For resample `r=0..9999` and draw `d=0..N-1`, set rejection counter `c=0`
   and compute `H=SHA256(hex-decode(seed_sha256) || u64be(r) || u64be(d) ||
   u32be(c))`; interpret the first eight bytes as unsigned big-endian `x`.
   Let `L=floor(2^64/N)*N`. Accept index `x mod N` only when `x<L`; otherwise
   increment `c` (overflow is failure) and retry. Median the N selected pair
   degradations. This is exactly 10,000 resamples; no library PRNG is allowed.
5. **Bound.** Sort the 10,000 resample medians ascending; the one-sided 99%
   lower bound is zero-based element 99. Threshold is exactly 30,000,000 ppb.
   Both threshold/significance flags are true iff the bound is strictly
   greater. Equality is `no_regression_detected`, not proof of equality.
6. **Terminal precedence.** Schema/digest/resource/overflow faults emit only
   `M0-PERF-ERROR-v1`. Valid but mismatched comparison keys yield
   `incomparable/PERF_INCOMPARABLE`. Any insufficient/invalid pair yields
   `inconclusive/PERF_INSUFFICIENT_SAMPLES` or `PERF_SAMPLE_FAILED`; any noise
   failure yields `inconclusive/PERF_NOISE_POLICY_FAILED`. Only then, if either
   input is advisory, state is `advisory_only/PERF_ADVISORY_INPUT` with
   informational statistics. Comparable authoritative clean inputs return
   `regression_requires_approval` above the strict bound or
   `no_regression_detected` otherwise.

The checked-in `tests/performance/fixtures/paired_bootstrap_v1/` corpus contains
canonical inputs and `intermediates.json` with schedule/comparator seed input
bytes, both seed digests, all pair ppb values, first 16 accepted indices for
resamples 0/1/9999, those resample medians, sorted position 99, point/bound,
flags, and state. The fixture directory's canonical manifest SHA-256 populates
`golden_corpus_sha256`; any intermediate disagreement fails PERF-TEST-007.
Its higher-is-better boundary contains baseline 100000/candidate 97000 and must
produce exactly 30000000 ppb. The adjacent first-above integer case is baseline
100000/candidate 96999 and produces 30010000 ppb before bootstrap aggregation;
the corpus asserts both pair values and terminal strict-bound behavior.

### 6.5 Design choice: regression method

| Alternative | Pros | Cons |
|---|---|---|
| Alternating paired samples and one-sided bootstrap median bound (selected) | Robust to skew, uses local pairing to reduce drift, expresses strict 3% confidence rule directly. | More runner time; bootstrap/fixed-point implementation needs golden tests. |
| Independent Mann-Whitney test plus median ratio | Established rank test and simple libraries. | Significance does not directly bound the magnitude threshold; pairing benefit lost. |
| Compare aggregate means only | Easy and fast. | Noise/outlier sensitive and not a statistically valid 3% gate. |

**Recommendation:** Use the paired-bootstrap median method in Section 6.4:
five warmup pairs, at least 30 alternating all-valid measured pairs, exactly
10,000 SHA-256 counter-sampled resamples, a one-sided 99% lower bound strictly
greater than 30,000,000 ppb, no trimming, zero migration/throttle events, and
benchmark-core background utilization below 10,000 ppm. This implements the
human answer to `q-0006`.

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
| Raw sample | Run ID plus phase/pair/index/order; it does not reference result ID. | Append-only; no digest cycle or silent deletion; invalid reason retained. |
| Result | SHA-256 content ID referencing an embedded or separate canonical JSON raw-sample document. | `assembled -> schema_valid -> advisory|authoritative|invalid`; authority cannot be upgraded without rerun/conformance. |
| Baseline | An authoritative result selected by reviewed baseline metadata. | Immutable; replacement records predecessor and rationale. |
| Comparison | Role-ordered baseline/candidate IDs, comparator version, and golden-corpus digest. | `validating -> incomparable|inconclusive|advisory_only|no_regression_detected|regression_requires_approval` under one precedence. |
| Profile bundle | Comparison ID plus tool/config and `evidence_object_sha256`. | Required for material authoritative regression acceptance; diagnostic, not authority by itself. |

Raw samples and controlled-runner facts are authoritative measurement evidence
only when conformance passes. Summary prose, profiles, and shared-runner data
are derived/advisory and cannot overwrite samples or authority.

## 8. Key Flows

1. **Controlled comparison (PERF-FR-001 through PERF-FR-011).** Resolve
   workload/builds; validate runner preflight; alternate five warmup pairs then at
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
| PERF-FAIL-004 | Fewer than 30 pairs, any invalid sample, or migration/throttle/load violation. [R-002, R-006] | Per sample and terminal-precedence validation before statistics. | One comparison state `inconclusive` with reason `PERF_INSUFFICIENT_SAMPLES`, `PERF_SAMPLE_FAILED`, or `PERF_NOISE_POLICY_FAILED`; counts/predicates. | All samples retained; no pass/regression/advisory-only state. | Fix condition and rerun complete attempt. | PERF-FR-002, -010 / PERF-TEST-002, -010. |
| PERF-FAIL-005 | Allocation counter reenters, overflows, or reports false zero. [R-106] | Counter operation/finalization. | `PERF_ALLOCATION_COUNTER_INVALID`; phase, counter, overflow/reentrancy state. | Workload result invalid; process exits cleanly. | Fix instrumentation and rerun positive/zero controls. | PERF-FR-009 / PERF-TEST-009. |
| PERF-FAIL-006 | Regression exceeds strict statistical boundary without approval/profile. [R-304] | Comparator then release evidence gate. | `PERF_REGRESSION_REQUIRES_APPROVAL`; point/bound/threshold, comparison IDs, missing evidence. | Candidate evidence blocked; baseline/result immutable. | Investigate, fix, or approve via ADR with profile/evidence. | PERF-FR-007, -011 / PERF-TEST-007, -011. |
| PERF-FAIL-007 | Input is corrupt/noncanonical, oversize/deep, float/unit-unknown, arithmetic overflows, or comparator exceeds 120 seconds. [R-201] | Byte/bounds/schema/arithmetic/work boundary before unsafe allocation or result commit. | Exact Section 6.3 `M0-PERF-ERROR-v1` code/path/offset/limit; no comparison document. | Bounded diagnostic; allocated buffers/owned worker released. | Supply valid bounded input or fix comparator; never accept partial statistics. | PERF-FR-012 / PERF-TEST-012/fuzzer. |
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
| PERF-FR-001 | PERF-TEST-001 | Schema/unit/golden | The five complete Section 6.1 records plus unknown, version-mismatch, missing invocation/dataset/concurrency/sampling field, digest, and duplicate-ID fixtures. | Reconstructed registry digest equals `17949ac8fe9d6d2e303a95e5881cc6d4574fb99203bb1c845b06461cbfc134cc`; every field equals the normative table and every invalid fixture fails. | `workload-registry-tests.xml`, canonical registry golden. |
| PERF-FR-002, PERF-NFR-003, PERF-NFR-006 | PERF-TEST-002 | Integration/cancellation | Five-warmup-pair + 30-measured-pair fixture, timeout/failure/cancel attempts. | Ten warmup invocations and phase/order/count exact; every sample retained; invalid makes comparison inconclusive. | `harness-phase-report.json`, raw samples. |
| PERF-FR-003, PERF-NFR-003 | PERF-TEST-003 | Schema/negative/golden | All five Section 6.3 valid examples and literal content digests; canonical/noncanonical byte goldens; raw arrays reordered/duplicated/gapped; every sampling count equation; odd/even negative/tie median and MAD; nearest-rank percentiles; one missing/extra/type/enum/bound/conditional/relationship/digest mutation per field. | Every valid document reproduces its stated digest; every mutation returns its exact typed error; raw/result references, order, counts, and statistics reconcile without a cycle or implementation-defined rounding. | `performance-schema-coverage.json`, canonical byte/statistic goldens. |
| PERF-FR-004, PERF-NFR-004 | PERF-TEST-004 | Integration/negative | Conforming runner simulation and one failed/unknown predicate each. | Only fully conforming simulation can be authoritative. | `runner-conformance-matrix.json`. |
| PERF-FR-005 | PERF-TEST-005 | Integration | Shared-runner provenance, forged authority, simulated 10% delta. | Valid result advisory; forged rejected; no delta-only block. | `shared-runner-authority.json`. |
| PERF-FR-006 | PERF-TEST-006 | Unit/boundary | One fixture per comparison key mismatch and multiple mismatch. | `PERF_INCOMPARABLE`; complete bounded mismatch identity; no verdict. | `comparability-matrix.json`. |
| PERF-FR-007, PERF-NFR-001 | PERF-TEST-007 | Statistical/intermediate-golden | Checked-in Section 6.4 corpus: exact 30,000,000 ppb, above-bound, improvement, both directions including higher-is-better 100000/97000 and adjacent 96999, odd/even/tie rounding, rejection sampling, overflow. | Both directions use baseline denominator; 97000 yields exactly 30000000 and does not flag, 96999 yields 30010000 before aggregation; schedule/seed bytes, first indices, pair/resample medians, index-99 bound, flags and state equal goldens 100/100 and in an independent reference implementation. | `comparator-boundaries.json`, `intermediates.json`. |
| PERF-FR-008, PERF-NFR-004 | PERF-TEST-008 | Unit | Authority cross-product and mixed authority. | Blocking dispositions only when both inputs authoritative. | `authority-cross-product.json`. |
| PERF-FR-009, PERF-NFR-002 | PERF-TEST-009 | Integration/benchmark | Zero/positive controls in dev/release, reentrancy and overflow injection. | Exact zero and positive targets met; error fixtures invalidate. | `allocation-counter-report.json`. |
| PERF-FR-010 | PERF-TEST-010 | Integration/fault/state | Migration, throttle, >=10,000-ppm background load, outlier, timeout/process-failure, insufficient-pair, and advisory-with-noise fixtures. | Every noise/sample failure has state `inconclusive` and exact reason before authority handling; outlier retained; no conflicting terminal vocabulary. | `noise-policy-fixtures.json`. |
| PERF-FR-011 | PERF-TEST-011 | End-to-end/inspection | Material regression with/without profile and ADR approval. | Missing evidence blocks; complete reviewed bundle can be accepted without changing threshold/history. | `regression-escalation.json`. |
| PERF-FR-012, PERF-NFR-005 | PERF-TEST-012 | Resource/fuzz | Exact/max-over document/depth/string/sample/workload/mismatch/integer/RSS/time fixtures and corrupt/noncanonical bytes under parser fuzzing. | Max valid stays <=256 MiB/120 s; every over-limit/overflow returns exact typed error before partial state; fuzz has zero crash/sanitizer finding. | `performance-parser-resource.json`, fuzz report. |
| PERF-FR-013 | PERF-TEST-013 | Inspection | Charter objectives against roadmap/spec requirements. | Every objective has introducing owner or explicit not-yet-applicable status. | `performance-objective-coverage.json`. |

## 12. Open Questions

No open questions.
