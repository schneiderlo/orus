# Orus Foundation Decision Log

**Status:** Ready  
**Last updated:** 2026-07-21

## 1. Decision policy and discovery traceability

This log applies the discovery precedence rule: **User Answer > confirmed
Assumption > project-seed Default**. `Accepted` means the choice is binding.
`Assumption` would mean a safe, reversible default was required because no
authoritative answer existed. Discovery and the answered post-discovery
clarification leave no unresolved foundation choice. D-016 records the exact
owner-approved MIT notice supplied in that clarification.

Every discovery answer is mapped exactly once in the first ten decisions:

| Discovery answer | Assumption log | Decision |
|---|---|---|
| Q-D01: begin with M0 | A-D01 | D-001 |
| Q-D02: MIT | A-D02 | D-002 |
| Q-D03: accept pinned-reference default | A-D03 | D-003 |
| Q-D06: confirm C++23/Nix/Bazel/no-CMake | A-D04 | D-004 |
| Q-D06: confirm concurrent architecture | A-D05 | D-005 |
| Q-D06: confirm correctness and performance together | A-D06 | D-006 |
| Q-D06: confirm fail-closed behavior | A-D07 | D-007 |
| Q-D06: confirm evidence authority and optional models | A-D08 | D-008 |
| Q-D04: close outside contributions during M0 | A-D09 | D-009 |
| Q-D05: GitHub CI plus controlled-runner contract | A-D10 | D-010 |

The remaining decisions preserve significant binding architecture choices from
the authoritative project seed. Detailed mechanisms still require the
milestone/domain ADRs named in `SPECS.md` before implementation.

---

## D-001 — Limit the current factory wave to M0

**Status:** Accepted  
**Source:** User answer Q-D01; A-D01  
**Decision owner:** Product owner  
**Decision scope:** Current factory run

### Context

The seed defines M0-M11, but treating the entire roadmap as one implementation
wave would make the scope, verification, and performance gates non-credible.

### Decision

This wave delivers M0 only. Later milestones remain roadmap context. M0 creates
the commercial repository, reproducible build, policy, benchmark foundation,
CI, CLI skeletons, and controlled concurrent examples and makes no
deterministic-record/replay claim.

### Alternatives considered

| Alternative | Pros | Cons |
|---|---|---|
| **M0 only (chosen)** | Smallest truthful gate; makes later implementation reproducible and measurable; avoids conflating infrastructure with deterministic semantics. | Does not deliver end-user record/replay capability in this wave. |
| M0 and M1 | Reaches process/thread control earlier. | Expands kernel lifecycle, signal, register, memory, and latency verification before the foundation is proven. |
| M0-M3 end-to-end | Delivers the first replayable slice sooner in calendar milestones. | Excessive integration risk for one wave; obscures intermediate failures and weakens gate credibility. |

**Recommendation and reasoning:** M0 only. This matches the explicit owner
answer and gives every later hot path a reproducible benchmark and policy base.

### Consequences

- Planning and Definition of Done stop at M0.
- M1+ specs can be drafted as roadmap placeholders but are not implementation
  authorization.
- Documentation, CLI, and releases must reject or omit record/replay claims.
- The concurrent workload is real and reusable, but M0 does not trace it.

### Validation

Verify every M0 item in `00-CHARTER.md`; scan release claims; confirm no M1+
domain spec becomes Active. A scope change requires explicit owner approval and
a superseding decision.

---

## D-002 — License Orus under MIT

**Status:** Accepted  
**Source:** User answer Q-D02; A-D02  
**Decision owner:** Product owner  
**Decision scope:** Project source license

### Context

M0 requires license, notice, dependency, and SBOM policy. The seed deliberately
did not choose a license; discovery supplied the legal posture.

### Decision

Use the standard MIT License for Orus-owned source. Do not modify the license
text with additional restrictions. Third-party components retain their own
licenses and notice obligations.

### Alternatives considered

| Alternative | Pros | Cons |
|---|---|---|
| **MIT (chosen)** | Simple, permissive, widely understood, compatible with commercial use and redistribution. | Provides minimal reciprocal obligations and no patent grant beyond what the text implies. |
| Proprietary commercial license | Greater distribution control and explicit commercial terms. | Requires owner/counsel-authored legal text; conflicts with the explicit user choice. |
| Defer/publication closed | Avoids selecting terms prematurely. | Blocks a truthful release and contradicts the explicit user answer. |

**Recommendation and reasoning:** MIT is the explicit user choice and is
compatible with the desired commercial/open publication posture.

### Consequences

- M0 must add a root `LICENSE` with the standard MIT text and the exact
  owner-approved notice `Copyright (c) 2026 Loic Schneider`, as specified by
  D-016.
- Release SBOM and third-party notices remain required; MIT does not erase
  dependency obligations.
- Contribution governance is separate and governed by D-009.

### Validation

Compare `LICENSE` to the OSI-published MIT template, validate packaged license
presence, and reconcile dependency licenses/notices with the SBOM.

---

## D-003 — Validate one pinned Linux x86-64 reference environment in M0

**Status:** Accepted  
**Source:** User answer Q-D03 accepting the default; A-D03  
**Decision owner:** Product owner  
**Decision scope:** M0 compatibility claim

### Context

Linux tracing and performance depend on kernel, CPU, host policy, and toolchain.
A generic “Linux x86-64” statement would be untestable and misleading at M0.

### Decision

Continuously validate one Nix-pinned Linux x86-64 reference environment and
record its kernel and CPU facts. M0 makes no broader distribution, kernel, CPU,
glibc, musl, or container compatibility claim. The exact reference image and
facts belong in the M0 environment domain spec and CI provenance.

### Alternatives considered

| Alternative | Pros | Cons |
|---|---|---|
| **One pinned reference environment (chosen)** | Reproducible, bounded, and honest; sufficient for build/benchmark foundations. | Narrow user reach; does not reveal portability defects across kernels/distros. |
| One named enterprise distribution family | A clearer market support target. | Kernel and package drift still need pinning; no owner-selected distribution exists. |
| Multi-distribution/kernel matrix in M0 | Finds portability problems early and broadens support. | Multiplies CI, diagnosis, and host-policy work outside the M0 gate. |

**Recommendation and reasoning:** Use the pinned reference environment. It is
the accepted user default and creates evidence without promising unsupported
breadth.

### Consequences

- `orus doctor`, CI, benchmark results, and releases record actual environment
  facts and distinguish validated from unvalidated environments.
- GCC compatibility is a compiler check within the same platform contract, not
  a broader host claim.
- A public compatibility matrix requires later evidence and an ADR.

### Validation

Run canonical commands in the pinned environment; verify provenance fields;
exercise `doctor` with reference and mismatched facts; inspect release language.

---

## D-004 — Use C++23, Bazel/Bzlmod, and Nix flakes; prohibit CMake

**Status:** Accepted  
**Source:** User confirmation Q-D06; A-D04  
**Decision owner:** Product owner / build architecture  
**Decision scope:** Native language, build graph, environment, packaging

### Context

Record/replay requires low-level Linux control, predictable data layout, and a
reproducible commercial toolchain. Multiple build graphs would permit drift and
hidden dependencies.

### Decision

Use C++23 for native code. Nix flakes own pinned development, CI, benchmark,
toolchain, and packaging environments. Bazel owns source compilation, linking,
generation, tests, benchmarks, and release artifacts. Bzlmod is mandatory.
CMake is prohibited from repository, developer, and CI paths.

### Alternatives considered

| Alternative | Pros | Cons |
|---|---|---|
| **C++23 + Nix + Bazel/Bzlmod (chosen)** | Strong low-level control, explicit source graph, hermetic environments, shared local/CI/release path, scalable tests/benchmarks. | Toolchain integration and dependency packaging require deliberate work. |
| C++ with CMake and a package manager | Familiar to many native developers and common upstream projects. | Weaker single-graph hermeticity; risks host drift; explicitly rejected by the seed and owner. |
| Dual Bazel/CMake builds | More upstream/developer flexibility. | Two sources of truth, duplicated configuration, divergent releases, larger support burden. |

**Recommendation and reasoning:** The chosen split assigns exactly one owner to
environment inputs and one owner to source actions, avoiding hidden alternate
paths.

### Consequences

- Required root files include `MODULE.bazel`, `MODULE.bazel.lock`,
  `BUILD.bazel`, `.bazelrc`, `.bazelversion`, `flake.nix`, and `flake.lock`.
- An upstream CMake-only dependency must be wrapped through pinned artifacts or
  a Bazel-native definition, replaced, or isolated by an accepted ADR without
  invoking CMake in Orus workflows.
- Clang is primary; GCC compatibility is continuously checked; LLD is the
  default release linker.

### Validation

Run all canonical commands from a clean minimal host, inspect Bazel actions and
module graph, verify lockfiles, compile a C++23 fixture, and scan for CMake.

---

## D-005 — Require multi-process and multi-thread architecture from the start

**Status:** Accepted  
**Source:** User confirmation Q-D06; A-D05  
**Decision owner:** Product architecture  
**Decision scope:** All product paths, including M0 examples

### Context

Process/thread identity, lifecycle, scheduling, synchronization, trace schemas,
and checkpoints become foundational. A single-task prototype would create a
predictable redesign.

### Decision

The first product path and its identifiers, ownership, interfaces, tests, and
performance model must support a process tree and multiple pthreads. M0 must
ship reusable controlled examples with a parent and exec'd child, at least
three parent threads and two child threads, synchronization, IPC, joins, wait,
known statuses, and deterministic output.

### Alternatives considered

| Alternative | Pros | Cons |
|---|---|---|
| **Concurrent architecture and examples now (chosen)** | Exposes lifecycle/ownership assumptions early; prevents schema and hot-path rewrites; matches commercial targets. | More complex fixtures and teardown from M0 onward. |
| Single process with multiple threads first | Covers thread scheduling while reducing lifecycle work. | Omits process-tree identity, inheritance, exec, wait, and cleanup—the exact areas most costly to retrofit. |
| Single-thread prototype | Fastest demonstration. | Cannot satisfy product intent and would validate the wrong architecture. |

**Recommendation and reasoning:** Concurrency is a structural requirement, not
a later feature. The explicit corpus gives every milestone a reusable vertical
slice.

### Consequences

- A single-task demo may test an isolated unit but cannot satisfy a product
  gate.
- APIs must name virtual process and thread identity where execution identity
  is relevant.
- Tests must verify cleanup and PID-reuse safety as soon as target control
  begins.

### Validation

At M0 run the native corpus 100 times and assert topology and cleanup. At later
milestones trace/replay the same declared topology and reject single-task gate
substitutes.

---

## D-006 — Treat correctness and performance-by-design as co-equal

**Status:** Accepted  
**Source:** User confirmation Q-D06; A-D06  
**Decision owner:** Engineering leadership  
**Decision scope:** Every applicable milestone and hot path

### Context

A divergent replay is invalid, while a predictably unusable correct design is
not commercially credible. Deferring measurement would lock trace, ownership,
and storage choices around unmeasured costs.

### Decision

Every milestone defines representative workloads, raw measurements, a pinned
baseline, applicable budgets, regression authority, and profiles before a hot
path is accepted. Correctness checks may not be removed to meet a budget.
Stable controlled-runner benchmarks block regressions; shared-runner results
are advisory. Applicable seed objectives are inherited by domain specs,
including the greater-than-3% approval threshold, 1.5x CPU-bound recording,
2.5x syscall-heavy recording after fast path, 20M descriptors/s/core, 80% raw
sequential writer throughput, and sub-50 ms local pause/cancel p99.

### Alternatives considered

| Alternative | Pros | Cons |
|---|---|---|
| **Correctness plus performance from each milestone (chosen)** | Makes cost visible before architecture hardens; prevents both false and unusable products. | Requires benchmark infrastructure and disciplined runner operations early. |
| Correctness first, optimize after replay works | Simplifies initial implementation. | Encourages per-event stops/allocations and storage layouts that require redesign; lacks early commercial evidence. |
| Performance first with reduced validation | Can improve headline throughput quickly. | Permits convincing but incorrect replay and violates the source-of-truth contract. |

**Recommendation and reasoning:** Co-equal gates preserve correctness while
forcing early evidence about commercially significant costs.

### Consequences

- M0 must build the measurement and comparison foundation before record/replay
  hot paths exist.
- No performance claim is valid without workload, CPU, affinity, kernel,
  compiler/configuration, applicable storage, samples, and authority.
- An intentional regression requires approval, evidence, and an ADR; it may not
  silently raise a budget.

### Validation

Audit every domain spec and milestone report for tests, numeric budgets,
provenance, raw data, regression result, and profiles for material regressions.

---

## D-007 — Fail closed on unsupported determinism-affecting behavior

**Status:** Accepted  
**Source:** User confirmation Q-D06; A-D07  
**Decision owner:** Deterministic-core architecture  
**Decision scope:** Record, replay, trace, synchronization, signals, mappings,
and compatibility

### Context

Approximating an unsupported syscall, instruction, mapping, signal, scheduler
transition, or trace version can produce a plausible but false replay.

### Decision

Stop at the earliest detected unsupported or inconsistent determinism-affecting
behavior. Return a typed diagnostic with operation, process/thread/execution
identity, expected/observed context where applicable, support/capability state,
and recoverability. Do not emit an authoritative success artifact or continue
with an approximation.

### Alternatives considered

| Alternative | Pros | Cons |
|---|---|---|
| **Fail closed (chosen)** | Preserves trust and localizes the unsupported boundary; makes support measurable. | Narrow early workloads stop more often. |
| Best-effort replay with warnings | Demonstrates more workloads and may aid exploratory diagnosis. | Warnings can be missed; later state is not trustworthy; creates false causal claims. |
| Silent emulation/fallback | Smoothest apparent UX. | Unverifiable and unsafe; hides exactly the evidence needed to expand support. |

**Recommendation and reasoning:** A deterministic debugger is valuable only
when a successful result is trustworthy. Precise rejection is preferable to a
convincing approximation.

### Consequences

- Every supported surface needs success, error, interrupted, boundary,
  concurrency, and unsupported-path tests.
- Capability negotiation and compatibility errors are explicit.
- Unsupported-path frequency becomes an observable product metric.

### Validation

Mutation and negative tests must prove the operation stops at the relevant
boundary, includes stable diagnostic fields, performs no forbidden side effect,
and cannot be interpreted as success.

---

## D-008 — Make recording and verified replay authoritative; keep models optional

**Status:** Accepted  
**Source:** User confirmation Q-D06; A-D08  
**Decision owner:** Product and investigation architecture  
**Decision scope:** Facts, analysis, agents, and product claims

### Context

Models, logs, stack traces, static analysis, and source comments can generate
useful hypotheses but can also be wrong or adversarial. Orus must distinguish
execution evidence from explanation.

### Decision

The immutable recording and continuously validated replay are the source of
truth for execution facts. Model adapters are optional, provider-independent
planners over typed tools. Every factual finding must reference structured,
navigable evidence or be labeled as a hypothesis. Disabling models must leave
record, replay, navigation, and deterministic analysis usable.

### Alternatives considered

| Alternative | Pros | Cons |
|---|---|---|
| **Evidence authority with optional models (chosen)** | Trustworthy claims, provider flexibility, deterministic no-model product, testable agent boundaries. | Requires typed evidence models and more disciplined UI/state handling. |
| Model-first debugger over logs/GDB | Faster conversational prototype. | Hallucinated facts, provider coupling, and no replayable proof; explicitly outside product intent. |
| No model integration | Smallest trust surface. | Gives up useful planning/explanation and agent workflows in later milestones. |

**Recommendation and reasoning:** Separate fact production from probabilistic
planning so models add leverage without becoming a correctness dependency.

### Consequences

- Natural-language explanations are views, not stored proof.
- Prompt-injection provenance and capability boundaries apply to all target and
  model-visible content.
- Agent findings, counterfactuals, and patches carry structured evidence and
  residual uncertainty.

### Validation

Run no-model product tests; validate evidence-reference schemas and navigation;
inject unsupported model claims and confirm they remain hypotheses rather than
facts; verify read-only agent capabilities cannot mutate state.

---

## D-009 — Keep outside contributions closed during M0

**Status:** Accepted  
**Source:** User answer Q-D04 option 2; A-D09  
**Decision owner:** Product owner / repository governance  
**Decision scope:** M0 contribution acceptance

### Context

MIT governs use and redistribution of source but does not define contribution
provenance or the project's review agreement. No owner-approved CLA or DCO
workflow exists.

### Decision

Publish under MIT if desired, but do not accept outside contributions during
M0. Document the closure visibly and provide no external pull-request path that
implies acceptance. Revisit governance after M0 through an owner-approved
decision.

### Alternatives considered

| Alternative | Pros | Cons |
|---|---|---|
| **Closed during M0 (chosen)** | Avoids inventing legal terms; lets the team establish policy and architecture first. | Forfeits outside patches and community feedback in M0. |
| MIT plus DCO sign-off | Lightweight, automatable provenance assertion. | Creates a contributor workflow the owner did not choose. |
| Contributor License Agreement | Explicit rights grant and potential commercial flexibility. | Requires legal text, signing operations, and owner approval; highest friction. |

**Recommendation and reasoning:** Closure is the explicit user choice and is
reversible after a deliberate governance decision.

### Consequences

- Contribution documentation must state that external patches are not accepted
  in M0 and provide a non-contribution issue/contact policy only if separately
  approved.
- Do not add DCO/CLA bots, templates, or implied acceptance language.
- MIT redistribution rights remain unaffected.

### Validation

Review CONTRIBUTING/README/PR templates and repository automation; simulate an
external contribution path and confirm the documented policy is unambiguous.

---

## D-010 — Use GitHub Actions for functional CI and advisory benchmarks; reserve authority for controlled runners

**Status:** Accepted  
**Source:** User answer Q-D05 accepting the default; A-D10  
**Decision owner:** CI and performance operations  
**Decision scope:** M0 CI and performance-result authority

### Context

Shared runners are suitable for functional checks but too noisy and
under-specified to adjudicate a precise 3% regression threshold. A controlled
self-hosted performance runner is not yet provisioned.

### Decision

Use GitHub Actions for blocking functional, build, sanitizer, formatting/lint,
dependency/SBOM, fuzz-smoke, and integration checks. Run benchmark smoke and
retain results there, but label them advisory and never block solely on their
measured delta. Define a controlled-runner contract and result provenance in
M0; only conforming controlled measurements become authoritative when a runner
is provisioned.

### Alternatives considered

| Alternative | Pros | Cons |
|---|---|---|
| **GitHub CI plus controlled-runner contract (chosen)** | Immediate functional automation; honest performance authority; does not block M0 on hardware procurement. | Precise performance gating remains operationally inactive until a runner exists. |
| Provision self-hosted runner in M0 | Enforces the 3% gate immediately on stable hardware. | Adds hardware, security, maintenance, and scheduling ownership not authorized in discovery. |
| Treat shared runners as authoritative | Lowest operational cost. | Noise and changing hardware make precise regressions unreliable and irreproducible. |

**Recommendation and reasoning:** Separate correctness automation from
measurement authority. This is the accepted user default and preserves all raw
data without presenting noisy values as product evidence.

### Consequences

- Every result declares runner authority and complete environment provenance.
- The comparator rejects authoritative comparisons when provenance or runner
  contract does not match.
- M0 can pass without provisioning a controlled runner, but later applicable
  performance gates cannot pass without one.

### Validation

Inspect required GitHub jobs; run comparator authority/provenance fixtures;
confirm a shared-runner 10% delta remains advisory; validate the controlled
runner policy contains all seed-required environment and sampling fields.

---

## D-011 — Start with a narrow Linux x86-64 ELF target contract

**Status:** Accepted  
**Source:** `PROJECT-SEED.md` Sections 3.1 and 25  
**Decision owner:** Product architecture  
**Decision scope:** Initial M1+ target platform

### Context

Deterministic control depends on OS, ISA, executable format, libc, launch mode,
and kernel behavior. Platform breadth competes directly with correctness depth.

### Decision

The first product path targets Linux x86-64 ELF C/C++ programs using glibc,
launched under Orus, and replayed on the same host or an identically pinned
image after compatibility validation. musl and attach are deferred pending
explicit validation. Cross-architecture and materially different host replay
are unsupported.

### Alternatives considered

| Alternative | Pros | Cons |
|---|---|---|
| **Narrow Linux x86-64 launch path (chosen)** | Concentrates tests on one kernel/ISA/ABI and supports commercially common native services. | Excludes other platforms, musl, and attach workflows initially. |
| Linux multi-architecture from first slice | Earlier ARM64 reach. | Doubles register, syscall, signal, instruction, toolchain, and benchmark work. |
| Cross-platform Linux/macOS/Windows | Broadest market. | Fundamentally different tracing and process models make early correctness evidence infeasible. |

**Recommendation and reasoning:** Prove deterministic semantics deeply on one
contract before expanding breadth.

### Consequences

Every unsupported platform or launch mode must be diagnosed before target
execution. The exact kernel matrix and artifact compatibility rules require
later ADRs and continuous evidence.

### Validation

Platform capability tests, ELF/glibc identity checks, cross-build negative
tests, and exact-host/image replay compatibility tests at the introducing
milestones.

---

## D-012 — Use virtual task identity and a strict serialized deterministic schedule

**Status:** Accepted  
**Source:** `PROJECT-SEED.md` Sections 3, 6.5, and 25  
**Decision owner:** Deterministic-core architecture  
**Decision scope:** M1-M4 scheduling and identity semantics

### Context

Live PIDs/TIDs are unstable and kernel scheduling of multiple runnable tasks is
nondeterministic. Orus must preserve a concurrent execution without initially
relying on true parallel replay.

### Decision

Assign stable virtual process/thread identities and translate observable
identity within the supported contract. Strict deterministic mode runs one
target task at a time and records each runnable-set transition, selected task,
logical position, switch reason, blocking object/syscall, and wakeup source and
target. True parallel multicore replay is deferred.

### Alternatives considered

| Alternative | Pros | Cons |
|---|---|---|
| **Virtual identity plus serialized strict schedule (chosen)** | Stable trace identity and repeatable complete concurrency order; simplest correctness oracle. | Alters wall-time parallelism and can impose substantial overhead. |
| Record native parallel scheduling directly | Better native performance/topology fidelity. | Races, hardware ordering, and precise event ordering greatly complicate first replay semantics. |
| Model only pthread IDs in one process | Smaller identity model. | Cannot represent process trees, exec, wait, descriptor inheritance, or host PID reuse safely. |

**Recommendation and reasoning:** Serialize execution while preserving every
task and schedule decision; optimize interception without weakening the logical
model.

### Consequences

Publish both equivalent-topology and unrestricted-native performance
comparisons. Scheduling quantum/logical-clock details require an ADR and
calibration tests before M1/M2 implementation.

### Validation

Concurrent topology/schedule tests, PID-reuse tests, schedule mutation tests,
100-repeat replay at M3, and complete decision-log reconciliation.

---

## D-013 — Isolate failure domains behind stable typed protocols and single ownership

**Status:** Accepted  
**Source:** `PROJECT-SEED.md` Sections 4.6-4.7 and 5  
**Decision owner:** System architecture  
**Decision scope:** M5+ processes and all earlier ownership models

### Context

Target control, replay state, untrusted symbol parsing, gateway traffic,
indexing, and models have different security, latency, and failure properties.
Shared mutable ownership would undermine isolation and deterministic ordering.

### Decision

Use separate processes by default for recorder/task supervision, one
authoritative replay worker per session, session supervision, symbol/expression
work, indexing/analysis, gateway, and investigation services. Cross-process
messages use versioned typed schemas, not native C++ layouts. Each mutable
session or live task has exactly one authoritative owner; other components send
commands and receive immutable results.

### Alternatives considered

| Alternative | Pros | Cons |
|---|---|---|
| **Isolated processes with single owners (chosen)** | Contains crashes/untrusted parsing, clarifies state order, supports quotas and cancellation. | IPC and lifecycle complexity; serialization overhead must be measured. |
| Monolithic process with internal modules | Simpler calls and deployment; lower serialization cost. | One failure can corrupt all state; symbol/network/model work shares the deterministic failure domain. |
| Shared mutable state across service workers | High apparent concurrency and direct access. | Races, ambiguous authority, difficult cancellation/recovery, and untestable state ordering. |

**Recommendation and reasoning:** Strong ownership and process boundaries make
correctness, failure recovery, and least privilege testable. Large data uses
shared immutable buffers to control IPC cost.

### Consequences

RCP/local IPC needs schema identity, request IDs, capabilities, progress,
cancellation, typed errors, and large-payload transfer. Gateway/model callbacks
never run on target/replay control threads.

### Validation

Worker crash isolation, concurrent mutation rejection, schema incompatibility,
backpressure/cancellation, least-privilege, and IPC latency/throughput tests.

---

## D-014 — Maintain a reference correctness path and a logically equivalent commercial fast path

**Status:** Accepted  
**Source:** `PROJECT-SEED.md` Sections 5.3 and 7.4  
**Decision owner:** Recorder/replay architecture  
**Decision scope:** M2-M7 interception and replay

### Context

A simple `ptrace`-centric path is valuable for semantic validation but cannot
be assumed to meet commercial event/syscall overhead. A fast path without an
oracle makes subtle divergence harder to detect.

### Decision

Keep a simple, heavily validated reference path and introduce a measured fast
path using selective interception, batching, bounded rings, runtime cooperation
or other accepted mechanisms. On an overlap corpus, both paths must emit the
same logical events and outcomes. The trace schema and ownership model must not
assume a stop at every event.

### Alternatives considered

| Alternative | Pros | Cons |
|---|---|---|
| **Reference plus equivalent fast path (chosen)** | Independent semantic oracle plus commercial optimization path; enables differential testing. | Two implementations and equivalence corpus increase maintenance. |
| Reference/`ptrace` path only | Simplest correctness implementation. | Predictably poor syscall-heavy overhead and architecture biased toward per-event stops. |
| Fast path only | Lower early overhead and one shipping path. | No independent oracle; optimization bugs can become trace semantics. |

**Recommendation and reasoning:** Differential equivalence makes aggressive
performance work safe without treating the slow path as the shipping target.

### Consequences

Fast-path candidates require security and performance ADRs. An optimization
cannot change logical trace meaning or remove validation. Overlap corpus results
are milestone-gate evidence.

### Validation

Differential event/outcome comparison, mutation tests, representative overhead
benchmarks, allocation/contention checks, and reference fallback diagnostics.

---

## D-015 — Use versioned explicit schemas and segmented trace storage

**Status:** Accepted  
**Source:** `PROJECT-SEED.md` Sections 6.1, 9, 10, and 25  
**Decision owner:** Data/protocol architecture  
**Decision scope:** M2+ trace and M5+ process boundaries

### Context

Native C++ layouts are ABI-dependent and unsafe for durable or untrusted data.
Per-event relational or JSON storage would conflict with event throughput and
safe bounded parsing.

### Decision

Use explicit fixed-width/versioned schemas, FlatBuffers for typed boundary
messages, independently checksummed event segments with separately stored large
blobs, and SQLite only for bounded catalog metadata. Use custom indexed block
storage for event streams. No native pointer, implicit padding, or C++ object
layout crosses a persistence/process boundary.

### Alternatives considered

| Alternative | Pros | Cons |
|---|---|---|
| **FlatBuffers boundaries + segmented event storage + SQLite catalog (chosen)** | Typed zero/low-copy boundaries, crash-safe independent segments, bounded catalog queries, hot-path-specific encoding. | Multiple formats and migration responsibilities; custom event tooling required. |
| Put all events in SQLite | Familiar transactions and query language. | Per-event storage/write amplification and scan layout conflict with throughput goals. |
| JSON files/messages | Human-readable and easy to prototype. | Size, parse cost, weak numeric/layout discipline, and unsafe hot-path temptation. |

**Recommendation and reasoning:** Separate stable service contracts, compact
append-oriented events, and low-volume catalog queries according to their
workloads.

### Consequences

Trace/protocol compatibility and migration require explicit ADRs before public
promises. All parsers enforce lengths, offsets, counts, nesting, and allocation
limits and are fuzzed. Compression/checksum/hash choices remain benchmarked
dependency decisions within this layout.

### Validation

Schema round-trip/golden tests, cross-version rejection/migration tests,
truncation/corruption tests, parser fuzzing, crash-commit recovery, resource
limit tests, and throughput/trace-size benchmarks.

---

## D-016 — Use the owner-approved MIT copyright notice

**Status:** Accepted
**Source:** Human answer to factory clarification `q-0001`, 2026-07-21
**Decision owner:** Product owner / release owner
**Decision scope:** Copyright notice in root and packaged MIT license copies

### Context

D-002 selects the MIT License. A post-discovery clarification requested the
exact holder name and year because repository metadata is not authoritative
legal identity data. The human owner answered: “Use Copyright (c) 2026 Loic
Schneider in the root MIT LICENSE and packaged copies.”

Factory question `q-0002` later repeated the same holder/year clarification.
It is not an independent requirement or a second source of authority; it is
resolved by the existing human answer in `q-0001`.

### Decision

The root MIT `LICENSE` and every packaged copy of that license must contain this
exact notice line:

`Copyright (c) 2026 Loic Schneider`

`specs/11-governance-release.md` must treat that literal line as a binding
release input. Root and packaged-license validation must fail before
publication if the notice is missing, contains placeholder text, or differs in
holder name or year.

### Alternatives considered

| Alternative | Pros | Cons |
|---|---|---|
| **Use the exact human-approved notice (chosen)** | Preserves authoritative owner intent; gives root and package checks one deterministic expected value; unblocks packaging. | A future holder or year change requires explicit owner approval and a superseding decision. |
| Continue to require owner input at packaging time | Avoids inferring identity and can accommodate a late legal change. | The owner has already answered; keeping the value open contradicts a higher-priority human decision and needlessly blocks packaging. |
| Infer holder/year from repository metadata and current date | Can be automated without a stored policy value. | Repository metadata may not name the legal rights holder; the inferred year may be wrong; violates the no-invented-requirement rule. |

**Recommendation and reasoning:** Use the exact human-approved notice. It has
the highest source priority, removes interpretation from release tooling, and
is directly verifiable in every distributed license copy.

### Consequences

- `specs/11-governance-release.md` must state the exact approved notice and
  define a failing validation for missing, placeholder, or different values.
- Root and packaged MIT license copies use one identical notice value.
- Changing the holder name or year requires explicit owner approval and an
  accepted decision that supersedes D-016; implementation or repository
  metadata alone cannot change it.
- Duplicate factory question `q-0002` is reconciled to the authoritative human
  answer in `q-0001`; it cannot supply a divergent notice value.

### Validation

Before public packaging, compare the root and every packaged `LICENSE` copy to
the standard MIT text and assert that each contains exactly one
`Copyright (c) 2026 Loic Schneider` notice line. Fixtures with a missing line,
placeholder token, different year, or different holder must each fail the
release gate before artifact publication.

## 2. Superseding a decision

A superseding ADR must identify the decision ID, new evidence, affected
requirements/milestones, compatibility and migration effects, security and
performance effects, validation, and rollback. Every decision sourced from an
explicit human or owner answer requires product-owner approval, including
D-001 through D-010 and D-016; an implementation team may not supersede such a
decision by code or documentation alone.
