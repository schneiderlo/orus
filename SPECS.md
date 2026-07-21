# Orus Specification Index and Roadmap

**Foundation status:** Ready  
**Current delivery wave:** M0 only  
**Last updated:** 2026-07-21

This file is the authoritative index for Orus specifications. The foundation
files exist now. Domain-spec paths are planned placeholders: the files do not
exist until the Domain Spec Writer creates their full content. A placeholder's
Draft status is therefore expected and does not authorize implementation.

## Status Legend

| Status | Meaning |
|---|---|
| **Draft** | Planned or incomplete. Requirements are not accepted for implementation. |
| **Ready** | Self-contained, reviewed, measurable, and accepted; implementation may begin subject to milestone authorization. |
| **Active** | Accepted requirements are currently being implemented or operated. This does not mean the milestone gate has passed. |
| **Deprecated** | Retained for history or migration; it does not govern new implementation. A replacement must be named. |

## Source hierarchy

1. Explicit user answers in [Discovery Questions](specs/00-QUESTIONS.md).
2. Confirmed entries in [Discovery Assumptions](specs/00-ASSUMPTIONS.md).
3. Binding defaults in [Project Seed](specs/PROJECT-SEED.md).
4. Foundation decisions and domain ADRs that do not conflict with items 1-3.

Discovery is closed. Remaining implementation choices belong in the relevant
domain spec and ADR. A choice that changes product scope or a foundation
decision requires explicit owner approval, not a silent domain refinement.

## Foundation specifications

| Specification | Status | Purpose | Depends on |
|---|---|---|---|
| [00-CHARTER](specs/00-CHARTER.md) | Ready | Defines mission, current M0 boundary, full scope, measurable success, constraints, personas, Definition of Done, and M0-M11 gates. | Discovery questions, assumptions, and project seed. |
| [01-GLOSSARY](specs/01-GLOSSARY.md) | Ready | Defines shared product, platform, performance, data, protocol, and investigation terminology. | Charter and project seed. |
| [02-DECISIONS](specs/02-DECISIONS.md) | Ready | Maps every discovery answer and significant architecture tradeoff to an ADR-style Accepted decision or clearly labeled safe Assumption. | Discovery questions, assumptions, charter, project seed, and latest foundation review. |
| [03-RISKS](specs/03-RISKS.md) | Ready | Defines likelihood/impact, mitigation, concrete validation, ownership, and milestone mapping for delivery and roadmap risks. | Charter, glossary, decisions, and project seed. |

## Dependency and activation rules

- The milestone chain is **Foundation → M0 → M1 → M2 → M3 → M4 → M5 →
  M6 → M7 → M8 → M9 → M10 → M11**.
- A domain spec may become Ready before its phase begins, but it may become
  Active only when all listed prerequisite specs are Ready and the current
  factory wave authorizes that milestone.
- A phase gate passes only when every domain in that phase satisfies its
  accepted requirements and the Charter milestone gate. Partial domain
  completion does not advance the phase.
- Cross-cutting Build, Performance, Security, Data Compatibility, and
  Observability requirements remain dependencies after their originating
  phase; later domains must reference, refine, and verify them rather than copy
  or weaken them.
- M0 is the only currently authorized implementation phase. All M1-M11 entries
  remain Draft roadmap placeholders even if their foundation context is clear.

## Phase F — Foundation (current pack)

**Exit gate:** All four foundation files above exist, are Ready, contain no
unresolved blocking question, and this index covers every Charter milestone and
cross-cutting concern.

The Foundation Phase contains the four files listed above; it creates no
runtime behavior.

## Phase M0 — Commercial repository, build, and performance foundation

**Phase dependency:** Foundation.  
**Exit gate:** Charter M0 and DOD-01 through DOD-10. In particular, all
canonical Nix+Bazel commands pass from a clean clone, no CMake/host dependency
exists, the concurrent native corpus passes 100 runs without leaks, performance
tooling is authority-aware and tested, legal/release evidence is complete, and
no deterministic execution claim is made.

| Planned domain specification | Status | Purpose and boundary | Direct dependencies |
|---|---|---|---|
| `specs/10-build-environment.md` | Draft | Nix flake, Bazel/Bzlmod graph, hermetic Clang/LLD toolchain, GCC compatibility, configurations, locks, supported reference environment, clean-build and packaging contracts. Does not define target tracing. | Foundation D-003/D-004/D-011. |
| `specs/11-governance-release.md` | Draft | MIT license packaging, owner-supplied holder/year gate, outside-contribution closure, ADR/dependency admission, third-party notices, SBOM, release provenance, repository/agent policy, and rollback/compatibility claim rules. | `10`; Foundation D-002/D-009/D-016. |
| `specs/12-cli-diagnostics.md` | Draft | Minimal `orus --version` and `orus doctor` commands, build/host fact schema, exit codes, diagnostics, and false-claim prevention. No record/replay commands. | `10`, `11`; Foundation D-001/D-003. |
| `specs/13-ci-quality.md` | Draft | GitHub Actions topology for Clang/GCC, dev/release, tests, ASan/UBSan, TSan applicability, fuzz smoke, formatting/lint/static checks, SBOM, cache/secret policy, and advisory benchmarks. | `10`, `11`, `12`; Foundation D-010. |
| `specs/14-performance-foundation.md` | Draft | Benchmark workloads/harness, allocation counter, versioned result/provenance schema, raw-sample/statistical method, controlled-runner contract, comparison tool, 3% boundary, advisory/authoritative rules, noise and profile policy. | `10`, `13`; Foundation D-006/D-010. |
| `specs/15-concurrent-corpus.md` | Draft | Native parent/exec-child workload with ≥3 parent and ≥2 child pthreads, bounded synchronization/IPC, time/random observations for later capture, deterministic result, joins/wait, fault injection, 100-run reliability, and cleanup. No record/replay behavior. | `10`, `13`, `14`; Foundation D-005. |
| `specs/16-security-foundations.md` | Draft | M0 secret handling, dependency/build threat boundary, CI least privilege, artifact/SBOM integrity, future untrusted target/trace/symbol/model trust boundaries, capability vocabulary, resource-limit and fuzzing policy. No unimplemented sandbox claim. | `10`, `11`, `13`; Foundation D-007/D-008/D-013/D-015. |

## Phase M1 — Linux process-tree and thread-control core

**Phase dependency:** M0 gate.  
**Exit gate:** A parent/child tree with multiple threads is repeatedly launched,
controlled, diagnosed, and cleaned up across normal exit, crash, exec failure,
child/thread exit, signal, tracer failure, cancellation, and PID churn; traps
are classified; no task leaks or identity confusion occur; accepted control and
memory-transfer budgets pass. No recording claim.

| Planned domain specification | Status | Purpose and boundary | Direct dependencies |
|---|---|---|---|
| `specs/20-linux-task-control.md` | Draft | Launch-under-trace, process/thread discovery, clone/exec/stop/exit/wait/signal lifecycle subset, pidfd-aware ownership, teardown, stop reasons, debugger-vs-target traps, and fail-closed capability checks. | M0 `10`, `14`, `15`, `16`. |
| `specs/21-task-identity-scheduler.md` | Draft | Virtual process/thread identity, parent/thread-group model, task states, one controlling owner, initial strict one-runnable-task scheduler, logical clock/switch reasons, and unsupported scheduling cases. | `20`; Foundation D-005/D-007/D-012. |
| `specs/22-x86-state-memory.md` | Draft | Typed x86-64 registers for every task, continue/single-step behavior, bulk target-memory read/write, mapping awareness, transfer limits, errors, and latency/throughput benchmarks. | `20`, `21`, M0 `14`, `16`. |

## Phase M2 — Concurrent recording and trace foundation

**Phase dependency:** M1 gate.  
**Exit gate:** The declared multi-process/multi-thread slice produces a complete
topology/schedule trace; unsupported operations fail closed; truncated,
corrupt, and resource-hostile traces fail safely; reference and initial
buffered architecture use the same logical model; event, allocation, writer,
compression, and trace-size budgets pass.

| Planned domain specification | Status | Purpose and boundary | Direct dependencies |
|---|---|---|---|
| `specs/30-trace-format-store.md` | Draft | Versioned manifest, independently decodable/checksummed segments, event identity/order, blobs, binary/artifact capture, catalog/index hints, commit/recovery, bounds/quotas, compatibility posture, inspector, parser fuzzing. | M1 `20`-`22`; M0 `11`, `14`, `16`; Foundation D-015. |
| `specs/31-recorder-pipeline.md` | Draft | Reference interception semantics, fixed descriptors, bounded rings/arenas, merge ordering, blob pipeline, compression/hash/write batching, backpressure, allocation/contention constraints, crash-safe publication, and fast-path boundary. | `30`, M1 `20`-`22`; M0 `14`; Foundation D-006/D-014. |
| `specs/32-syscall-sync-policy.md` | Draft | Per-operation emulate/control/virtualize/reject contract; initial process, exec, wait, thread, futex, time, randomness, IPC, read/write, mapping, FD, signal, and exit subset; output-memory/side-effect/blocking tests and coverage inventory. | `30`, `31`, M1 `21`; Foundation D-007/D-012. |

## Phase M3 — Deterministic process-tree and thread replay

**Phase dependency:** M2 gate.  
**Exit gate:** The concurrent vertical slice reconstructs topology, identity,
mappings, supported virtual state, nondeterministic results, and strict schedule
identically for 100 replays without forbidden external writes; mutations are
localized at their earliest detectable boundary; reference/fast paths agree;
record/replay overhead budgets pass.

| Planned domain specification | Status | Purpose and boundary | Direct dependencies |
|---|---|---|---|
| `specs/40-replay-virtualization.md` | Draft | Replay startup, task/mapping/VPID/VTID reconstruction, supported FD/kernel-object state, schedule replay, syscall/synchronization result injection/validation, live-world isolation, and output suppression/display. | M2 `30`-`32`; M1 `21`-`22`; M0 `16`. |
| `specs/41-validation-divergence.md` | Draft | Continuous event-boundary validation, execution-point context, expected-vs-observed typed errors, artifact/mapping/schedule/data mutation corpus, reference/fast equivalence, teardown, and 100-repeat evidence. | `40`, M2 `30`-`32`; M0 `14`; Foundation D-007/D-014. |

## Phase M4 — Reverse execution and multi-task checkpoints

**Phase dependency:** M3 gate.  
**Exit gate:** Seek/reverse operations return correct historical state for every
virtual task; reverse then forward restores identical validated state;
checkpoints restore process, thread, scheduler, descriptor, mapping, and signal
state; interactive latency and checkpoint memory/restore budgets pass.

| Planned domain specification | Status | Purpose and boundary | Direct dependencies |
|---|---|---|---|
| `specs/50-checkpoints-reverse.md` | Draft | Execution-point seek, reverse step/continue, historical registers/memory, complete process-tree checkpoint model, dirty-page/delta strategy, restore validation, checkpoint lineage, resource limits, and latency/memory budgets. | M3 `40`-`41`; M2 `30`; M0 `14`, `16`. |

## Phase M5 — Stable debugger interfaces

**Phase dependency:** M4 gate.  
**Exit gate:** CLI and adapters control isolated workers only through versioned
contracts; one owner serializes session mutation; cancellation/progress and
typed errors work; worker failures cannot corrupt traces; compatibility,
control-latency, and bulk-throughput budgets pass.

| Planned domain specification | Status | Purpose and boundary | Direct dependencies |
|---|---|---|---|
| `specs/60-replay-control-protocol.md` | Draft | Versioned RCP operations, IDs/capabilities, one-owner session model, Unix socket framing, FlatBuffers validation, `memfd`/FD passing for bulk data, progress/cancellation, errors, quotas, compatibility and recovery. | M4 `50`; M3 `40`-`41`; M0 `16`; Foundation D-013/D-015. |
| `specs/61-debug-symbol-adapters.md` | Draft | CLI-over-RCP, DAP, optional GDB RSP, isolated pinned LLVM/LLDB/Clang symbol/expression worker, DWARF/artifact contracts, resource limits, malformed-input handling, and adapter conformance. | `60`; M2 `30`; M0 `10`, `16`; Foundation D-011/D-013. |

## Phase M6 — Orus Studio vertical slice

**Phase dependency:** M5 gate.  
**Exit gate:** The browser operates on bounded viewport/bulk data, not a full
event stream; gateway owns no target/replay/DWARF state; bulk transfers cannot
block pause/cancel; bounded queue, browser memory, rendering/query, and local
control-latency budgets pass.

| Planned domain specification | Status | Purpose and boundary | Direct dependencies |
|---|---|---|---|
| `specs/70-gateway-studio.md` | Draft | Asio/Beast authentication/session gateway, separate control/bulk paths, HTTP ranges, bounded WebSockets/backpressure, React/TypeScript evidence workspace, source/disassembly/tasks/state/timeline, Web Worker decoding, viewport tiles, virtualization, cancellation and quotas. | M5 `60`-`61`; M4 `50`; M0 `10`, `14`, `16`. |

## Phase M7 — Broad Linux concurrency and process semantics

**Phase dependency:** M6 gate; may be prepared after M3 but cannot weaken
earlier interfaces.  
**Exit gate:** Representative real C++ services repeatedly replay across the
expanded declared subset; schedule mutations are detected; every unsupported
path fails precisely; CPU, syscall, memory, process, synchronization, trace,
and application-tail budgets pass.

| Planned domain specification | Status | Purpose and boundary | Direct dependencies |
|---|---|---|---|
| `specs/80-linux-semantics.md` | Draft | Expanded futex/mutex/condition/barrier/robust-list/blocking behavior; fork/vfork/clone/clone3/exec/wait/posix_spawn; per-thread signals/restart; TLS/dlopen; descriptor/process identity semantics; syscall/scheduler fast paths and real-service corpus. | M3 `40`-`41`; M2 `32`; M1 `20`-`22`; M0 `14`, `16`. |

## Phase M8 — Causal analysis

**Phase dependency:** M7 gate and stable M5 protocol.  
**Exit gate:** A known-bug corpus returns the responsible event and source
region, distinguishes evidence strength, rejects at least one plausible wrong
hypothesis, and meets commercial index/search latency and memory budgets at
declared trace sizes.

| Planned domain specification | Status | Purpose and boundary | Direct dependencies |
|---|---|---|---|
| `specs/90-execution-indexes.md` | Draft | Sparse event/task/PC/memory-write/scheduling indexes, viewport/search proportionality, asynchronous bounded construction, corruption validation, manifest-only open behavior, storage/memory/query budgets. | M2 `30`; M4 `50`; M7 `80`; M0 `14`, `16`. |
| `specs/91-causal-analysis.md` | Draft | Memory object/lifetime model, last-write, first-bad-state, run alignment/earliest semantic divergence, bounded backward/forward slicing, happens-before/conflicting access, invariants, evidence levels, structured findings and known-bug corpus. | `90`; M5 `60`-`61`; M7 `80`; Foundation D-008. |

## Phase M9 — Read-only debugging agent

**Phase dependency:** M8 gate and M6 evidence UI.  
**Exit gate:** Read-only mode cannot mutate replay/source/repository or access
network; every factual claim resolves to structured evidence; adversarial
target/model content cannot redefine policy; models can be disabled without
losing deterministic debugger behavior; budgets prevent agent work from
degrading recorder/replay hot paths.

| Planned domain specification | Status | Purpose and boundary | Direct dependencies |
|---|---|---|---|
| `specs/100-investigation-service.md` | Draft | Provider-independent adapters, typed deterministic tools, Reported→Approved investigation state, hypotheses/findings/evidence, budgets/cancellation, read-only capabilities, provenance/prompt-injection controls, audit and no-model behavior. | M8 `90`-`91`; M5 `60`; M0 `16`; Foundation D-008/D-013. |
| `specs/101-agent-interfaces.md` | Draft | MCP facade/resources and Studio investigation UI with clickable evidence, contradictions, action log, approvals, tool-result validation, and provider-independent transport. | `100`; M6 `70`; M5 `60`. |

## Phase M10 — Counterfactual branches and patch verification

**Phase dependency:** M9 gate.  
**Exit gate:** Branches and experiments reproduce from recorded provenance; a
patch cannot pass because a crash merely disappears; accepted patches include
causal/counterfactual evidence, exact build identity, failing/passing traces,
tests, sanitizers, performance comparison, limitations, and human publication
approval.

| Planned domain specification | Status | Purpose and boundary | Direct dependencies |
|---|---|---|---|
| `specs/110-branches-interventions.md` | Draft | Branch identity/lineage, fork points, controlled syscall/input/schedule/signal/function/memory interventions, build identity, branch comparison, invariants, reproducibility and resource budgets. | M9 `100`; M8 `91`; M4 `50`; M3 `40`-`41`. |
| `specs/111-patch-verification.md` | Draft | Clang AST/LibTooling patch workspace, rebuild, original/passing trace corpus, tests/sanitizers/performance bundle, proof-carrying result schema, residual risk, sandbox/capabilities, and human publish approval. | `110`; M9 `100`-`101`; M0 `10`, `13`, `14`, `16`. |

## Phase M11 — Production recording and cross-layer causality

**Phase dependency:** M10 gate.  
**Exit gate:** Production recording publishes explicit overhead, application
tail-latency, storage, quota, and loss measurements; sensitive-data controls
are enabled by default; incident packages are reproducible; cross-layer and
organizational evidence preserves provenance and revalidates old patterns
against new traces.

| Planned domain specification | Status | Purpose and boundary | Direct dependencies |
|---|---|---|---|
| `specs/120-production-recording.md` | Draft | Ring-buffer/flight recording, storage/process/I/O quotas, retention/redaction/encryption, incident packaging, loss/backpressure semantics, operability, and production overhead/tail/storage budgets. | M10 `110`-`111`; M7 `80`; M2 `30`-`31`; M0 `14`, `16`. |
| `specs/121-cross-layer-causality.md` | Draft | OpenTelemetry request/model/tool/agent correlation, span-to-execution identity, failure clustering, organizational incident memory, policy-controlled autonomous investigation, revalidation and data lifecycle. | `120`; M9 `100`-`101`; M8 `90`-`91`; M0 `16`. |

## Roadmap coverage matrix

This matrix is a completeness check, not a substitute for domain requirements.

| Charter capability / constraint | Owning roadmap specs |
|---|---|
| Reproducible Nix+Bazel build, configurations, packaging | `10`, `13` |
| Licensing, dependencies, SBOM, release/support policy | `11`, `16` |
| CLI diagnostics and truthful claims | `12` |
| Performance workloads, budgets, authority, regression evidence | `14`, then every hot-path domain |
| Multi-process/multi-thread corpus and architecture | `15`, `20`-`22`, `30`-`32`, `40`-`41`, `80` |
| Security, isolation, sensitive data, capabilities, resource limits | `16`, then `30`, `60`-`61`, `70`, `100`-`101`, `111`, `120`-`121` |
| Linux target control, identity, scheduling, registers/memory | `20`-`22` |
| Record, trace format/store, syscall/synchronization policy | `30`-`32` |
| Replay, live-world isolation, continuous validation/divergence | `40`-`41` |
| Checkpoints and reverse navigation | `50` |
| RCP, IPC, CLI/DAP/GDB, symbols/DWARF | `60`-`61` |
| Browser gateway and Studio evidence workspace | `70` |
| Broad Linux process/thread/signal/futex/library semantics | `80` |
| Indexing, lifetime, divergence, slicing, concurrency analysis | `90`-`91` |
| Provider-independent agent, typed tools, MCP, evidence UI | `100`-`101` |
| Counterfactual replay and proof-carrying patches | `110`-`111` |
| Production/flight recording, telemetry, incident memory | `120`-`121` |

---

# Domain Spec Template

Copy this template into each planned domain file. Replace every bracketed item.
Normative `must`/`shall` statements belong in a requirement table with an
acceptance criterion and verification method. Narrative sections explain the
model; they do not create untracked requirements.

```markdown
# [Domain name]

**Status:** Draft | Ready | Active | Deprecated  
**Milestone:** [M0-M11]  
**Owners:** [role(s)]  
**Last updated:** [YYYY-MM-DD]  
**Depends on:** [spec IDs/paths and required statuses]  
**Foundation decisions:** [D-NNN references]  
**Risks addressed:** [R-NNN references]

## 1. Purpose

[What product/team outcome this domain owns, who consumes it, and the boundary
that distinguishes it from adjacent domains. State the milestone gate output.]

## 2. In Scope

- [Concrete behavior/data/interface delivered by this domain.]

## 3. Out of Scope

- [Explicitly deferred or owned elsewhere; name the owning spec/milestone.]

## 4. Functional Requirements

Use stable IDs of the form `[DOMAIN]-FR-001`. Each row describes one observable
behavior. “All,” “supported,” and “valid” require a referenced finite inventory
or contract.

| ID | Requirement | Acceptance criteria | Verification method | Traceability |
|---|---|---|---|---|
| [DOM]-FR-001 | [Actor/system shall do one observable thing under named preconditions.] | [Exact input/state and expected output/state/side effect, including boundary and negative behavior.] | [Named automated test, inspection, analysis, or reproducible procedure and retained evidence.] | [Charter goal/decision/risk/milestone.] |

## 5. Non-Functional Requirements

Use stable IDs of the form `[DOMAIN]-NFR-001`. Give a number, unit, population,
workload, environment, measurement window/sample method, threshold, and
authority. If a seed objective is not yet applicable, say which introducing
requirement makes it applicable; do not silently omit it.

| ID | Quality attribute and target | Conditions / workload | Acceptance criteria | Verification method and evidence |
|---|---|---|---|---|
| [DOM]-NFR-001 | [e.g., p99 latency < N ms] | [Pinned environment, data size, concurrency, warmup/samples.] | [Exact pass/fail rule and allowed exceptions.] | [Benchmark/security/reliability method, schema, raw evidence location.] |

## 6. Interfaces / Contracts

For every public or cross-process interface, specify:

- caller/consumer and authoritative owner;
- operation/message names, schema and schema version;
- request/response fields with types, units, optionality, bounds, and identity;
- ordering, idempotency, concurrency, timeout, cancellation, and backpressure;
- capability negotiation, compatibility, typed errors, and recoverability;
- large-payload ownership/lifetime and security boundary;
- requirements/tests that verify the contract.

Do not expose native C++ layouts across process or persistence boundaries.

## 7. Data Model

Define entities, stable identities, relationships, lifecycle/state machine,
invariants, serialization, versioning, ownership, retention, limits, and
integrity. Include tables or schemas where ambiguity is possible. Distinguish
host identities from Orus virtual identities and authoritative evidence from
derived/advisory data.

## 8. Key Flows

For each primary flow, give numbered steps from precondition through terminal
state, naming owner transitions and emitted evidence. Include at least:

1. [Successful primary flow.]
2. [Cancellation/teardown flow.]
3. [Unsupported, malformed, or resource-limit flow.]

Reference the requirement IDs verified by each flow.

## 9. Failure Modes

| ID | Trigger | Required detection point | Typed outcome / diagnostic fields | Side effects and cleanup | Retry / recovery | Verifying requirements/tests |
|---|---|---|---|---|---|---|
| [DOM]-FAIL-001 | [Precise fault.] | [Earliest observable boundary.] | [Stable code plus required context.] | [What is committed, rolled back, retained, or suppressed.] | [Never/manual/automatic with limits.] | [[DOM]-FR-NNN / test.] |

Determinism-affecting unsupported behavior must fail closed. Resource limits
must be enforced before unsafe allocation or side effect.

## 10. Observability

| Signal ID | Metric/event/log | Type, unit, and labels | Emission point / owner | Cardinality and cost bound | Alert/gate use | Verification |
|---|---|---|---|---|---|---|
| [DOM]-OBS-001 | [Name.] | [Counter/gauge/histogram/event and units.] | [Cold path or bounded collection point.] | [Exact bound; no per-event formatting on hot paths.] | [Diagnostic, SLO, benchmark, or release gate.] | [Test/inspection.] |

State how request, trace, branch, execution, virtual process/thread, build, and
runner identities correlate where applicable. Preserve raw benchmark/test
evidence required by the domain gate.

## 11. Test & Verification Plan

| Requirement ID | Test/benchmark/review ID | Level | Fixture/workload and environment | Pass criterion | Evidence artifact |
|---|---|---|---|---|---|
| [DOM]-FR-001 | [DOM]-TEST-001 | Unit / integration / end-to-end / fuzz / benchmark / inspection | [Exact fixture and pinned facts.] | [Binary or numeric criterion.] | [Machine-readable report/log/corpus.] |

The plan must cover success, error, boundary, cancellation, resource limit,
concurrency, and unsupported paths where applicable; sanitizer and fuzz
coverage for untrusted structured inputs; before/after performance evidence for
hot paths; and clean teardown. Every FR and NFR must map to at least one row,
and every row must name a retained pass/fail artifact.

## 12. Open Questions

Use no open question for a requirement already answered by discovery. A
non-blocking safe default must be labeled Assumption and added to the decision
log before the spec becomes Ready. A blocking question prevents Ready status.

| ID | Kind | Question | Why it matters / unlocks | Options with pros/cons | Default | Blocking | Owner / due gate |
|---|---|---|---|---|---|---|---|
| [DOM]-Q-001 | clarification / design_choice | [Bounded question.] | [Impact and unlocked requirement.] | [2-3 concrete choices.] | [Safe reversible default or none.] | true / false | [Decision owner and latest gate.] |

If none: `No open questions.`
```
