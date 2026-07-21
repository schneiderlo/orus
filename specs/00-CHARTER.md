# Orus Product Charter

**Status:** Ready  
**Current delivery wave:** M0 only  
**Authoritative inputs:** `PROJECT-SEED.md`, `00-QUESTIONS.md`, and
`00-ASSUMPTIONS.md`  
**Last updated:** 2026-07-21

## 1. Purpose and authority

Orus is a commercial-grade deterministic record/replay and causal debugging
platform for native C and C++ programs. Its product mission is to turn a real
multi-process, multi-thread execution into an inspectable, replayable,
forkable, and verifiable historical artifact.

This factory wave is authorized to deliver **M0 only**: the commercial
repository, hermetic build, engineering policy, performance-measurement
foundation, and controlled concurrent example programs. M0 must preserve the
architecture needed by later milestones, but it must not claim that recording,
replay, reverse debugging, or causal analysis works.

When sources conflict, apply this precedence order:

1. An explicit user answer in `00-QUESTIONS.md`.
2. A confirmed entry in `00-ASSUMPTIONS.md`.
3. A binding default in `PROJECT-SEED.md`.

The discovery decisions that apply this rule are recorded in
`02-DECISIONS.md`.

## 2. Product goals

The full product goals define the roadmap. Only the M0 acceptance criteria are
delivery commitments in the current wave.

| ID | Goal | M0 acceptance criterion | Verification method |
|---|---|---|---|
| G-01 | Establish one reproducible source-to-release path. | From a clean clone in the pinned `x86_64-linux` environment, every canonical M0 command in Section 7 exits 0 without undeclared host tools, headers, or libraries. | Run the commands in an environment containing only declared Nix inputs; inspect Bazel action inputs and sandbox failures. |
| G-02 | Make the build graph and toolchain explicit and reviewable. | The repository pins Nix inputs, Bazel, Bzlmod modules, Clang/LLVM/LLD, and direct dependencies; Bazel owns compilation, code generation, tests, benchmarks, and the release binary. | Review committed lockfiles and toolchain targets; run `bazel mod graph`; build with Bazel sandboxing enabled. |
| G-03 | Create a commercially credible engineering baseline. | MIT license text, contribution policy, dependency-admission template, ADR template, SBOM generation, formatting, linting, CI, and agent guidance exist and their automated checks pass. Outside contributions are explicitly closed during M0. | Repository inspection plus CI execution; compare MIT text to the OSI-published MIT template; generate and validate a non-empty SBOM for the release artifact. |
| G-04 | Make performance evidence part of correctness from the first implementation wave. | A benchmark harness, allocation counter, versioned result schema, controlled-runner contract, and regression comparator exist. The comparator detects a fixture regression greater than 3%; shared-runner results are labeled advisory. | Run benchmark unit/integration tests and comparator fixtures; schema-validate emitted results; inspect runner provenance and authority fields. |
| G-05 | Prevent a single-process or single-thread architecture from becoming the baseline. | The M0 example corpus includes a parent and an exec'd child, with at least three parent pthreads and two child pthreads, synchronization, IPC, deterministic output, joins, waits, and clean exit. It runs successfully 100 consecutive times under the native test harness. | Run the Bazel integration target 100 times; assert topology, expected logical result, all joins/waits, zero timeout, and zero remaining child process after each run. |
| G-06 | Provide truthful operator diagnostics. | `orus --version` reports product version, source revision, build configuration, compiler identity, and target platform. `orus doctor` implements exactly the mandatory checks in the versioned `M0-DOCTOR-v1` inventory required by Section 2.1, reports each as pass/fail, exits 0 only when all pass, and exits non-zero when any fail. | Schema/golden tests validate all five version fields. A compatible fixture covers every inventory row and exits 0; one negative fixture per mandatory check proves that check's stable ID is reported as failed and the command exits non-zero. |
| G-07 | Preserve the future deterministic core boundary. | Every row of the `M0-ARCH-v1` checklist in Section 2.2 has both an accepted foundation decision and a named future owning domain in `SPECS.md`; M0 contains no implementation or availability claim for those future capabilities. | Run the checklist inspection against the decision log and `SPECS.md`; run the approved-claims scan over source, CLI help, README, package contents, and release metadata and find zero M1+ availability claims or runtime targets. |
| G-08 | Ultimately provide deterministic, evidence-backed debugging. | M1-M11 deliver the gated roadmap in Section 10; each milestone must meet its correctness, concurrency, security, compatibility, and performance gate before the next dependent milestone is Active. | Milestone-specific domain specifications and their test/benchmark reports; not an M0 completion criterion. |

### 2.1 Normative M0 diagnostics inventory contract

`specs/12-cli-diagnostics.md` is the authoritative owner of a versioned, finite
table named `M0-DOCTOR-v1`. Before that domain spec can become Ready, the table
must enumerate every M0 `orus doctor` check and, for each row, define:

- a stable check ID and human-readable name;
- the fact source and expected value or predicate;
- whether the check is mandatory or informational;
- the structured pass/fail representation and diagnostic fields; and
- whether failure contributes to the process exit status.

The Ready version of `M0-DOCTOR-v1` is the complete inventory: the executable
must not add undocumented mandatory checks, omit an inventory row, or report an
unknown result as a pass. The positive fixture must exercise every mandatory
row. The negative suite must cover every mandatory row by producing the stated
failure result and non-zero process exit. Exact pinned environment values are
owned by `specs/10-build-environment.md` and referenced, not duplicated, by the
inventory.

### 2.2 Normative M0 architecture-preservation checklist

M0 preserves future boundaries through explicit ownership and accepted
decisions, not through empty directories or placeholder runtime code. Every row
below must pass at the M0 gate.

| Check ID | Boundary assertion | Required observable M0 artifacts | Binary pass rule |
|---|---|---|---|
| M0-ARCH-01 | Process and task identity is not modeled as one host PID or one implicit thread. | D-005 and D-012; planned owner `specs/21-task-identity-scheduler.md` in `SPECS.md`. | Both decisions are Accepted and the named roadmap row explicitly owns virtual process/thread identity. |
| M0-ARCH-02 | Deterministic scheduling has one explicit future owner and a strict serialized initial policy. | D-012; planned owner `specs/21-task-identity-scheduler.md`. | D-012 is Accepted and the roadmap row explicitly owns scheduler state, logical progress, and unsupported scheduling cases. |
| M0-ARCH-03 | Durable trace data uses versioned schemas and segmented storage rather than native layouts. | D-015; planned owner `specs/30-trace-format-store.md`. | D-015 is Accepted and the roadmap row explicitly owns trace schema/version, integrity, and storage compatibility. |
| M0-ARCH-04 | Failure-prone debugger components are isolated behind typed process boundaries with one mutable-state owner. | D-013; planned owners `specs/60-replay-control-protocol.md` and `specs/61-debug-symbol-adapters.md`. | D-013 is Accepted and both roadmap rows explicitly own the worker protocol and symbol-worker isolation boundary. |
| M0-ARCH-05 | The reference correctness path and commercial fast path share one logical model and require differential validation. | D-014; planned owners `specs/31-recorder-pipeline.md` and `specs/41-validation-divergence.md`. | D-014 is Accepted and both roadmap rows explicitly own the fast/reference overlap and equivalence evidence. |

If any required decision is not Accepted, any named owner is absent, or an M0
artifact claims the future capability is available, G-07 fails.

## 3. Non-goals

The following are explicitly outside M0:

- Recording, deterministic replay, reverse execution, checkpoints, trace
  formats, syscall interception, deterministic scheduling, or claims that any
  of those capabilities are available.
- Attaching to an already-running process, cross-architecture behavior,
  whole-system replay, GPU/accelerator execution, or true parallel multicore
  deterministic replay.
- Orus Studio, gateway services, DAP, GDB RSP, RCP, MCP, model integration,
  causal analysis, counterfactual execution, patch generation, or production
  flight recording.
- A broad Linux distribution, kernel, CPU, glibc, or musl compatibility claim.
  M0 validates only one pinned Linux x86-64 reference environment.
- Provisioning a dedicated performance runner. M0 defines its contract and
  result format; precise performance results become authoritative only when a
  conforming controlled runner is available.
- Accepting outside contributions during M0. The source license is MIT, but the
  contribution workflow remains closed until a later governance decision.
- Creating empty future-component directories or placeholder runtime code.
- Adding CMake or an alternate source build path.

The following remain early product non-goals until a later design and
deterministic corpus explicitly admit them: arbitrary device mappings and
`ioctl` behavior, external shared memory modified by an unrecorded process,
general target-program `io_uring`, kernel-space replay, materially different
CPU/kernel replay, and uncaptured JIT execution.

## 4. Scope

### 4.1 In scope for M0

- Root Nix flake and lockfile, supported-system declaration, developer shell,
  checks, and release package.
- Bazel/Bzlmod root project, lockfile, pinned Bazel version, hermetic Clang
  toolchain, LLD release link path, and GCC compatibility configuration.
- Dev, release, ASan, UBSan, TSan-applicable, fuzz, and benchmark
  configuration entry points. A configuration may contain no target only when
  the corresponding feature is not yet applicable, but invoking it must fail
  clearly or perform its documented checks; it must never silently select a
  host toolchain.
- Minimal `orus` CLI with real `--version` and `doctor` behavior.
- GoogleTest/Google Benchmark-capable test and benchmark infrastructure.
- The controlled concurrent workload described by G-05.
- GitHub Actions for functional, sanitizer, lint/format, dependency/SBOM, fuzz
  smoke, and advisory benchmark checks, including Clang and GCC compatibility.
- Controlled-runner policy, benchmark provenance/result schema, raw sample
  retention, allocation counting, and a regression comparison tool.
- MIT license, README, contribution policy, third-party dependency admission,
  SBOM/notice process, ADR template, security/performance policies, and
  `AGENTS.md`.

### 4.2 Product scope preserved for later milestones

The roadmap covers Linux x86-64 ELF C/C++ programs launched under Orus;
multi-process and multi-pthread topology; virtual process/thread identity;
strict deterministic scheduling; process, syscall, synchronization, mapping,
signal, and external-input capture; replay without live external effects;
reverse navigation; typed debugger interfaces; Studio; causal analysis;
optional provider-independent models; counterfactual branches; proof-carrying
patches; and production recording. These items are context for M0 architecture,
not M0 deliverables.

## 5. Personas and use cases

| Persona | Need | M0 use case | Later product use case |
|---|---|---|---|
| Native developer | A dependable local workflow and actionable diagnostics. | Enter the pinned shell, build/test, run the concurrent corpus, and diagnose host incompatibility with `orus doctor`. | Record a failure, navigate history, inspect any process/thread, and verify a fix. |
| Maintainer/reviewer | Changes that remain reproducible, supportable, and within policy. | Review declared dependencies, ADRs, test evidence, CI, SBOM, and performance evidence from one canonical build path. | Evolve trace/protocol behavior without silent compatibility or determinism regressions. |
| Performance engineer | Comparable raw measurements and clear authority. | Run the same benchmarks on a conforming controlled runner, retain provenance/samples, and detect regressions above the approved 3% threshold. | Measure recorder/replay overhead, latency, trace volume, checkpoint cost, and query performance. |
| Security/release engineer | Known inputs, artifact provenance, and constrained untrusted-data boundaries. | Reproduce the release artifact, inspect its SBOM and notices, and confirm no secret or host dependency is embedded. | Isolate target, trace, symbol, and model inputs with quotas and least privilege. |
| CI/operator | Stable automation with useful failures. | Run the documented Nix+Bazel commands in GitHub Actions and distinguish functional failures from advisory performance noise. | Operate dedicated performance runners and isolated replay/session services. |
| Debugging investigator or coding agent | Claims backed by navigable execution evidence. | No M0 agent/debugging capability. | Use typed tools to collect evidence, test hypotheses, and produce a proof-carrying finding or patch. |

## 6. Success metrics

All M0 metrics are release-blocking unless their Authority column says
Advisory. A metric is satisfied only by retained machine-readable evidence from
the current source revision.

| ID | Metric and target | Authority | Measurement and evidence |
|---|---|---|---|
| SM-01 | 100% of the canonical commands in Section 7 exit 0 from a clean clone. | Blocking | GitHub CI logs plus clean-environment release evidence. |
| SM-02 | 0 CMake files, commands, generated metadata, or alternate canonical build paths. | Blocking | Repository scan for `CMakeLists.txt`, presets, CMake cache/generator files, and executable invocations; documentation review. |
| SM-03 | 0 undeclared host headers, libraries, compilers, generators, or scripts in a sandboxed build. | Blocking | Build with an intentionally minimal host and Bazel sandbox debug/aquery inspection. |
| SM-04 | 100% pass for Clang dev tests, GCC compatibility tests, release build, ASan tests, UBSan tests, format/lint, SBOM generation, and fuzz smoke in GitHub Actions. | Blocking | Required GitHub Actions job results for the current revision. |
| SM-05 | Concurrent example completes 100/100 consecutive runs with the declared topology, one expected result per run, expected exit statuses, and zero leaked child processes. | Blocking | Repetition harness report and process cleanup assertions. |
| SM-06 | `orus --version` contains five non-empty facts: version, revision, configuration, compiler, and target. `orus doctor` covers 100% of `M0-DOCTOR-v1`: the all-compatible fixture reports every mandatory row as pass and exits 0; each mandatory-check negative fixture reports its row as fail and exits non-zero. | Blocking | Version output-schema tests; inventory-to-test reconciliation; positive and per-row negative CLI integration reports. |
| SM-07 | Every emitted benchmark result contains workload/version, source revision, CPU, affinity, kernel, compiler, optimization configuration, storage facts when applicable, warmup/sample statistics, units, and runner authority. | Blocking | Result-schema validation with positive and missing-field fixtures. |
| SM-08 | The regression comparator passes changes at or below 3.0%, flags a statistically significant change above 3.0%, rejects incomparable provenance, and never treats shared GitHub-runner data as authoritative. | Blocking for tool behavior; shared-runner measurements are Advisory | Comparator boundary fixtures at 3.0% and above 3.0%, provenance mismatch tests, and authority-label tests. Exact statistical sampling is specified by the M0 performance domain spec. |
| SM-09 | Allocation counter reports zero steady-state allocations for the benchmark harness's designated no-allocation reference loop after startup and detects a positive-allocation fixture. | Blocking | Allocation-counter integration tests with zero and injected-allocation fixtures. |
| SM-10 | MIT license is present; every resolved runtime/build dependency appears in the generated SBOM with version and license field; every admitted dependency has an admission record. | Blocking | License-text check, dependency graph-to-SBOM reconciliation, and admission-record linter. |
| SM-11 | No M0 artifact, command help, README, or release metadata claims recording or replay support. | Blocking | Approved-claims allowlist plus repository/release scan. |
| SM-12 | Shared-runner benchmark values are retained and labeled `advisory`; they never block solely on a measured performance delta. | Advisory | CI workflow inspection and simulated comparator invocation with shared-runner provenance. |

The following product objectives become blocking only when a milestone
introduces the applicable hot path: zero steady-state event-descriptor
allocations; zero contended mutexes in target/replay control; no statistically
significant regression above 3% without approved evidence; CPU-bound recording
at no more than 1.5x equivalent-topology native time; syscall-heavy recording
at no more than 2.5x after the commercial fast path; at least 20 million compact
descriptors/second/core; writer throughput of at least 80% of equivalent raw
sequential storage; and local pause/cancel p99 below 50 ms during bulk streaming.

## 7. Canonical M0 commands

These commands define the only supported build/test path at M0:

```bash
nix flake check

nix develop --command bazel build --config=dev //...
nix develop --command bazel test --config=dev //...
nix develop --command bazel test --config=asan //...
nix develop --command bazel test --config=ubsan //...
nix develop --command bazel build --config=release //...
nix develop --command bazel test --config=benchmark //tests/benchmarks/...

nix develop --command bazel run //tools:format
nix develop --command bazel run //cli:orus -- --version

nix build .#orus
```

CI and documentation may add convenience wrappers, but wrappers must delegate
to these Nix+Bazel paths and must not introduce an independent build graph.

## 8. Constraints

| ID | Constraint | Acceptance criterion | Verification method |
|---|---|---|---|
| C-01 | Native implementation language is C++23. | Every M0 C++ target compiles under the declared C++23 toolchain; no target silently downgrades the language level. | Inspect Bazel toolchain/features and compile a C++23 conformance fixture. |
| C-02 | Nix owns environments/packages; Bazel with Bzlmod owns the source graph; CMake is prohibited. | Responsibility boundaries match Sections 2 and 4, and repository scan finds no prohibited path. | Lockfile/target review and SM-02/SM-03 tests. |
| C-03 | M0 support is one pinned Linux x86-64 reference environment. | CI records the pinned image/input identity, kernel and CPU facts; all other environments are reported as unvalidated, not supported. | Validate CI provenance and `orus doctor` output on reference and non-reference fixtures. |
| C-04 | Architecture is multi-process and multi-thread from the first product path. | IDs, schemas, and APIs introduced after M0 cannot identify execution solely by a host PID or a single thread; M0 corpus meets G-05. | Architecture review plus concurrent corpus test. |
| C-05 | Correctness and performance-by-design are co-equal. | Every milestone/domain spec identifies functional tests, applicable numeric performance budgets, benchmark method, and regression authority before implementation becomes Active. | Spec readiness checklist and milestone gate review. |
| C-06 | Determinism-affecting unsupported behavior fails closed. | Every later supported surface has a negative test proving unsupported cases stop with typed context and do not emit a success claim. | Negative/unsupported-path test reports at the introducing milestone. |
| C-07 | Recordings and verified replay are authoritative; models are optional planners. | Later factual execution claims carry typed evidence; disabling model integration leaves deterministic debugger operations usable. | Evidence-reference contract tests and no-model end-to-end tests at M8-M9. |
| C-08 | Stable process boundaries use versioned typed protocols; native C++ layout is never a wire format. | Each cross-process contract declares schema/version/capabilities and rejects incompatible input. | Protocol conformance and compatibility tests at the introducing milestone. |
| C-09 | Untrusted programs, traces, symbols, browser input, and model-visible content are resource-bounded and isolated. | Each introducing domain spec defines limits, least-privilege capabilities, malformed-input tests, and fuzzing where structured input is accepted. | Security review, sandbox tests, limit tests, and fuzz reports. |
| C-10 | The project is MIT-licensed; outside contributions are closed during M0. | `LICENSE` contains MIT text and contribution documentation rejects external submissions during M0 without defining a CLA or DCO workflow. | License and policy checks. |
| C-11 | Secrets and signing/remote-cache credentials remain outside repository and flake. | Secret scan passes; flake evaluation succeeds without secret inputs; CI injects credentials only through protected runtime configuration. | Secret scanning, flake evaluation, and workflow review. |
| C-12 | No public compatibility promise precedes evidence and migration policy. | M0 release documentation names only its reference environment and explicitly says record/trace/protocol compatibility is not yet offered. | Release documentation/metadata check. |

## 9. Definition of Done for this wave

M0 is done only when every row below is satisfied. “Not applicable” requires a
written rationale linked to the owning domain spec; it may not waive a listed
M0 deliverable.

| ID | Done condition | Required evidence |
|---|---|---|
| DOD-01 | All M0 domain specs in `SPECS.md` are Ready, their accepted requirements are implemented, and no M1+ runtime capability is represented as complete. | Spec status index, requirement-to-test traceability, and release-claim scan. |
| DOD-02 | SM-01 through SM-11 pass on the current revision; SM-12 is emitted and correctly labeled. | Machine-readable CI/release evidence bundle. |
| DOD-03 | Clean Nix+Bazel builds are reproducible through the canonical commands and produce an `orus` release artifact with real build metadata. | Clean-clone logs, artifact hash(es), and `orus --version` output. |
| DOD-04 | Every cell marked required in the versioned `M0-CI-APPLICABILITY-v1` target/check matrix owned by `specs/13-ci-quality.md` executes and passes, including unit, integration, negative-path, ASan, UBSan, fuzz-smoke, format, lint/static, and concurrent-corpus checks where the matrix applies them. All produced warnings are allowlisted under the rule below. | Matrix-to-CI reconciliation, machine-readable job/test reports, warning report, and the concurrent 100-run report. |
| DOD-05 | Benchmark schema, raw samples, controlled-runner contract, comparator, allocation counter, and advisory CI benchmark path are documented and tested. | Schema/comparator tests and a sample performance report containing all SM-07 fields. |
| DOD-06 | MIT license, outside-contribution closure, dependency admission records, third-party notices process, and release SBOM are complete. | Policy review and SBOM/license reconciliation. |
| DOD-07 | Every significant M0 architectural choice has an accepted ADR or an explicit assumption with alternatives, consequences, validation, and rollback/migration plan. | Decision/ADR audit against `02-DECISIONS.md` and domain specs. |
| DOD-08 | Resource bounds, failures, and externally visible behavior introduced in M0 have precise diagnostics and documentation. | Negative tests, CLI docs, and operator-facing failure catalog. |
| DOD-09 | No test leaves child processes or temporary owned resources behind, including after forced failure or timeout. | Teardown/fault-injection integration tests and post-test process/resource checks. |
| DOD-10 | A reviewer can reproduce the gate from zero repository context using only committed documentation. | Independent clean-clone gate execution. |

`M0-CI-APPLICABILITY-v1` must be a finite matrix of every M0 Bazel test target
or target group against every required CI check/configuration. Each cell is
`required` or `not_applicable`; a `not_applicable` cell must carry a scoped
rationale and the requirement or tool limitation that justifies it. Silent
skip, empty execution, and host-toolchain fallback fail DOD-04. A warning passes
only when a versioned allowlist entry matches its stable tool/rule ID and exact
target/configuration scope and records an owner, written rationale, and review
milestone. Zero non-allowlisted warnings are permitted.

## 10. CRITICAL Milestones

Milestones are sequential product gates. Work may be explored in parallel, but
a dependent milestone cannot become Active until its predecessor's gate passes.

| Milestone | Description |
|---|---|
| M0 | **Commercial repository, build, and performance foundation.** Deliver pinned Nix+Bazel/Bzlmod toolchains, CLI metadata/doctor skeletons, CI and quality configurations, MIT/governance/SBOM policy, performance harness and controlled-runner contract, and real multi-process/multi-thread examples. Gate: clean canonical commands pass, no hidden host dependency or CMake exists, benchmark tooling is reproducible and authority-aware, and no deterministic-execution claim is made. |
| M1 | **Linux process-tree and thread-control core.** Launch and control multiple processes/threads; model virtual identities, registers, memory, stop reasons, lifecycle, signals, and a strict single-runnable-task scheduler. Gate: repeated concurrent control and cleanup tests pass without leaks/PID confusion, and M1 latency/transfer budgets pass. No recording claim. |
| M2 | **Concurrent recording and trace foundation.** Add versioned manifests/segments, process/thread/scheduler/syscall/mapping/FD events, policy frameworks, crash-safe I/O, parser fuzzing, and reference/buffered paths. Gate: the concurrent slice's full declared topology/schedule is recorded, unsupported paths fail closed, corrupt traces fail safely, and pipeline/writer/size budgets pass. |
| M3 | **Deterministic process-tree and thread replay.** Recreate topology, mappings, identities, descriptor state, schedule, and supported outcomes without repeating external effects; continuously validate and localize divergence. Gate: the concurrent slice replays identically 100 times, mutations fail locally, reference/fast paths agree, and overhead budgets pass. |
| M4 | **Reverse execution and multi-task checkpoints.** Add execution-point seek, reverse step/continue, historical state, complete process-tree checkpoints, and incremental memory tracking. Gate: reverse-then-forward identity holds and interactive/checkpoint budgets pass. |
| M5 | **Stable debugger interfaces.** Introduce versioned RCP/local IPC, CLI-over-RCP, DAP, optional GDB RSP, cancellation/progress/errors/capabilities, and single-owner session mutation. Gate: worker isolation, concurrency control, compatibility, latency, and throughput tests pass. |
| M6 | **Orus Studio vertical slice.** Add Asio/Beast gateway and evidence workspace for source, tasks, state, timeline, and forward/reverse control with bounded bulk/control paths. Gate: bulk traffic cannot block pause/cancel, browser work is viewport-bounded, boundaries remain isolated, and latency/memory budgets pass. |
| M7 | **Broad Linux concurrency and process semantics.** Expand futex/pthread, process creation/exec/wait, signal/restart, dynamic-library, scheduler, and syscall paths. Gate: representative services replay repeatedly, schedule mutation is detected, unsupported paths remain precise, and commercial workload budgets pass. |
| M8 | **Causal analysis.** Add object/lifetime modeling, last-write/first-bad-state search, run alignment, dynamic slicing, happens-before/conflict analysis, and structured evidence. Gate: known-bug localization and false-hypothesis rejection succeed within commercial query/index budgets. |
| M9 | **Read-only debugging agent.** Add provider-independent investigation state/tools, MCP, Studio evidence UI, provenance controls, and budgets. Gate: read-only mode cannot mutate or access unauthorized capabilities, every factual claim cites evidence, models are optional, and hot paths are unaffected. |
| M10 | **Counterfactual branches and patch verification.** Add reproducible interventions, branch comparison, Clang-based patch workspaces, rebuild/replay, test/sanitizer/performance bundles, and human publication approval. Gate: experiments reproduce and every verified patch carries causal, build, test, performance, and residual-risk evidence. |
| M11 | **Production recording and cross-layer causality.** Add flight recording, quotas/retention, incident packaging, OpenTelemetry/agent correlation, clustering/memory, and policy-controlled autonomous investigation. Gate: production overhead/tail/storage/loss measurements pass and sensitive-data controls default on. |

## 11. Change control

Discovery is closed. A domain spec may refine implementation details only when
the refinement is consistent with this charter and `02-DECISIONS.md`. A change
to current-wave scope, license, build system, platform claim, concurrency
posture, correctness/performance priority, failure policy, evidence authority,
contribution posture, or CI authority requires a new superseding decision and
explicit owner approval. Implementation evidence may motivate such a change;
it may not silently redefine the requirement.
