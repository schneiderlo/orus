# Orus Project Seed

**Status:** Authoritative project seed  
**Repository:** `schneiderlo/orus`  
**Revision:** 2 — commercial concurrent baseline  
**Original seed date:** 2026-07-19  
**Revision date:** 2026-07-19  
**Primary audience:** autonomous software factory, maintainers, reviewers, security engineers, performance engineers, and future contributors

> Read this document completely before creating code, issues, plans, or architecture. The autonomous factory will not have access to the conversation that produced it. This document is therefore the initial source of truth for product intent, scope, architecture, performance, and engineering policy. When implementation evidence requires a change, create an Architecture Decision Record (ADR), update this document deliberately, and preserve the reason for the change. Do not silently reinterpret or weaken it.

---

## 0. Factory directive

Build **Orus**, a commercial-grade deterministic record/replay and causal debugging platform for native C and C++ programs.

Orus must let humans and software agents:

1. Record a real multi-process, multi-thread execution.
2. Replay it deterministically without consulting the live outside world.
3. Navigate forward and backward through execution history.
4. Inspect process, thread, register, memory, syscall, signal, mapping, and synchronization state.
5. Ask causal questions such as “who last wrote this value?” and “where did the passing and failing runs first diverge?”
6. Fork history into controlled counterfactual experiments.
7. Produce evidence-backed findings and proof-carrying candidate fixes.

The product is not an LLM chat wrapper around GDB. The deterministic recording and verified replay are the source of truth. Models may plan investigations and explain results, but factual claims about an execution must be grounded in replayable machine evidence or explicitly marked as hypotheses.

### 0.1 Commercial posture

Orus is not a toy, classroom exercise, or disposable prototype. “Minimum viable” means the smallest **commercially credible** slice that preserves the architecture needed for correctness, concurrency, security, and performance. It does not mean a single-process or single-thread demonstration that must later be redesigned.

The factory must reject shortcuts that create predictable rewrites in:

- Process-tree and thread identity.
- Scheduler and synchronization modeling.
- Trace schema and execution coordinates.
- Checkpoint representation.
- Hot-path data layout.
- Build reproducibility.
- Protocol versioning.
- Security boundaries.
- Performance measurement.

A narrow syscall subset is acceptable for an early milestone. A single-process or single-thread architecture is not.

### 0.2 Priority order

The priorities are:

1. **Correct deterministic behavior and performance-by-design, together.** A fast divergent replay is invalid; a correct design with commercially unusable overhead is also not acceptable.
2. **Multi-process and multi-thread support in the first end-to-end product path.**
3. **Earliest divergence detection and diagnosability.**
4. **Safe isolation of untrusted programs, traces, symbols, and model-visible data.**
5. **Stable, versioned, typed boundaries.**
6. **Human debugging workflows.**
7. **Agentic causal investigation.**
8. **Counterfactual replay, autonomous patching, and verification.**

Performance is not a later optimization phase. Every milestone must include a benchmark, a budget, and a regression gate for the paths it introduces.

### 0.3 Explicit build-system decision

Orus uses **Nix and Bazel**.

- Nix provides the pinned developer, CI, packaging, and toolchain environment.
- Bazel owns the source build graph, dependency graph, code generation, tests, benchmarks, and release binaries.
- Bzlmod is the Bazel dependency mechanism.
- Nix flakes pin Nix inputs.
- **CMake is prohibited.** Do not add `CMakeLists.txt`, CMake presets, CMake-generated build metadata, or a developer/CI path that invokes CMake.

An upstream dependency that only ships a CMake build does not justify adding CMake to Orus. Consume a pinned artifact through Nix, add a Bazel-native build definition, replace the dependency, or document an isolated exception through an ADR that still keeps CMake out of Orus developer and CI workflows.

---

## 1. Mission

Orus turns a native program execution into an inspectable, forkable, and verifiable historical artifact.

Its north-star experience is:

> Record the run, find the first incorrect state, prove what caused it, test competing explanations against forked history, propose the smallest correction, and demonstrate that the correction works against the original failure.

A successful investigation should be able to produce a result such as:

```text
Claim
  A timeout callback used Session after its lifetime ended.

Evidence
  Process creation                    event 7,940
  Worker-thread creation              event 8,101
  Session allocation                  event 8,419
  Callback registration               event 11,204
  Session destruction                 event 18,992
  Cross-thread wakeup                 event 19,281
  Stale callback execution            event 19,307
  Invalid memory access               event 19,319
  Observable failure                  event 19,412

Counterfactual
  Cancelling the callback at destruction prevents the invalid access,
  preserves the recorded externally visible output, and leaves the
  process tree and thread completion behavior unchanged.

Verification
  Original failing trace              passes after the candidate patch
  Previously passing trace corpus     unchanged
  Relevant tests                      pass
  Performance budget                  satisfied
  Residual uncertainty                documented
```

Every referenced event, process, thread, value, stack frame, source location, replay branch, build, and verification result must be navigable.

---

## 2. Product definition

Orus consists of six capabilities:

1. **Record** native execution while capturing every relevant nondeterministic input, process transition, thread transition, and scheduling decision.
2. **Replay** the process tree and threads while preventing consultation of the live outside world.
3. **Navigate** forward and backward through source, instructions, processes, threads, memory, registers, syscalls, signals, mappings, and synchronization.
4. **Explain** value origins, object lifetimes, control flow, synchronization, and the earliest meaningful divergence between runs.
5. **Experiment** by forking replay at a historical point and applying a controlled intervention.
6. **Verify** findings and candidate fixes through replay, invariants, tests, sanitizers, and performance gates.

The product surfaces are:

- `orus` command-line interface.
- Orus Studio web application.
- Debug Adapter Protocol (DAP) endpoint.
- GDB Remote Serial Protocol compatibility endpoint where useful.
- A typed internal Replay Control Protocol (RCP).
- Model Context Protocol (MCP) tools and resources for external coding agents.
- Optional OpenTelemetry ingestion for correlating native execution with requests, model calls, tools, and agent handoffs.

---

## 3. Supported scope

### 3.1 Initial platform contract

The first product path is narrow in platform breadth but not in concurrency topology:

| Dimension | Initial decision |
|---|---|
| Host OS | Linux |
| Architecture | x86-64 |
| Target format | ELF |
| Target languages | C and C++ |
| C library | glibc first; musl after explicit validation |
| Program start | Launch under Orus; attach is deferred |
| Process model | **Process trees from the first end-to-end slice**; parent/child lifecycle, `fork`/process-forming `clone`, `exec`, wait, and exit within an explicit supported subset |
| Thread model | **Multiple pthreads from the first end-to-end slice**; thread `clone`, join/exit, TLS, and a defined futex/synchronization subset |
| Scheduling model | Strict deterministic mode serializes runnable target tasks and records every scheduling decision |
| Debug information | DWARF, with exact executable and shared-library retention |
| Replay environment | Same host or an identical pinned image with explicit compatibility validation |
| External effects | Recorded during record; suppressed, emulated, or virtualized during replay |
| Privilege model | Unprivileged operation wherever possible |
| Build system | Bazel with Bzlmod |
| Environment and packaging | Nix flakes |

No milestone may claim end-to-end deterministic record/replay based only on a one-process, one-thread target. Unit tests may be single process or single thread when testing an isolated component, but they do not satisfy a product gate.

### 3.2 Initial process and thread semantics

The initial commercially credible subset must model:

- Virtual process and thread IDs independent of live Linux numeric IDs.
- Parent/child process relationships.
- `fork` or process-forming `clone`.
- `execve` and executable identity changes.
- Process exit and wait semantics.
- Thread creation, exit, and join.
- Thread-local storage and per-thread registers.
- A defined futex subset sufficient for common pthread mutex, condition-variable, barrier, and join paths.
- Blocking and wakeup transitions.
- Per-thread signal masks and signal delivery within the declared subset.
- File-descriptor inheritance and process-visible offsets for supported descriptors.
- A deterministic scheduler trace that identifies the runnable task and reason for every switch.

`vfork`, `clone3`, `execveat`, `posix_spawn`, robust futexes, restartable sequences, priority inheritance, process groups, and more complex signal cases must either be supported with tests or rejected precisely. They may be phased within the first commercial program, but the architecture and trace schema must accommodate them from the beginning.

### 3.3 Intended full scope

The long-term product should support:

- Broad Linux process-tree and pthread behavior.
- Precise asynchronous signal delivery.
- Futexes, blocking syscalls, condition variables, atomics, robust lists, and thread-local storage.
- Dynamic libraries and controlled `dlopen` behavior.
- Incremental checkpoints and fast reverse search.
- Optimized C++ and complex DWARF location expressions.
- C++ exceptions, RTTI, templates, coroutines, inline frames, and sanitizers.
- Trace comparison, causal slicing, lifetime analysis, and concurrency analysis.
- Counterfactual replay branches.
- Source patching, rebuilding, replay-based verification, and test execution.
- Cross-layer correlation with distributed and agent telemetry.
- Continuous or ring-buffer recording for production incidents.

### 3.4 Explicit early non-goals

Do not claim support for these until they have a design and deterministic test corpus:

- Attach to an already-running process.
- True parallel multicore deterministic replay.
- GPU or accelerator execution.
- Arbitrary device-backed mappings, DMA, or undocumented `ioctl` behavior.
- External shared memory modified by an unrecorded process.
- General `io_uring` target-program support.
- Kernel-space or whole-system replay.
- Cross-architecture replay.
- Transparent replay across materially different CPUs or kernels.
- JIT runtimes without explicit code-generation capture and validation.

True parallel multicore replay is deferred; multi-process and multi-thread support is not. The initial strict mode may serialize target task execution while preserving and replaying the complete concurrent schedule.

---

## 4. Non-negotiable engineering principles

### 4.1 The recording is authoritative

Models, logs, source comments, stack traces, heuristics, and static analysis are not the source of truth for what occurred. The recording and deterministic replay are authoritative.

### 4.2 Fail closed

When Orus encounters an unsupported syscall, instruction, mapping, synchronization path, signal case, kernel behavior, trace version, or state transition that could affect determinism, it must stop with a precise diagnostic. It must never continue with an approximation that could yield a convincing but false replay.

### 4.3 Detect the earliest divergence

Replay continuously validates progress. A mismatch stops at the earliest detectable point and reports expected versus observed process, thread, program counter, logical clock, syscall, signal, scheduler event, arguments, mappings, and relevant state.

### 4.4 Performance is a functional requirement

Performance design begins with the first data structure and syscall boundary. Every hot-path change carries benchmark evidence. “Optimize later” is not an acceptable rationale for an event representation, synchronization design, ownership model, storage layout, or process boundary that is predictably inefficient.

Correctness remains absolute. Performance work may not remove validation or silently weaken determinism.

### 4.5 Evidence before explanation

Every agent-generated finding includes structured evidence references. Natural-language explanations are views over those references, not replacements for them.

### 4.6 Stable typed boundaries

Recorder, replay worker, symbol service, gateway, indexer, and investigation service communicate through versioned typed protocols. Do not expose internal C++ layouts across process boundaries.

### 4.7 Isolate failure domains

A gateway crash must not corrupt a trace. A model timeout must not affect replay correctness. A malformed symbol file must not compromise the recorder. Each active replay session has one authoritative isolated worker.

### 4.8 Reproducible builds and traces

Build inputs, Nix inputs, Bazel modules, compiler identity, executable and library build IDs, configuration, kernel compatibility metadata, trace schema, and relevant CPU features are pinned or recorded.

### 4.9 No hidden environment dependencies

A linked library, generator, compiler, script, or tool must be declared through Bazel and/or the Nix flake. It may not work merely because it happens to be installed on a developer machine.

### 4.10 Provider-independent AI

The debugger remains fully useful with the AI layer disabled. Model adapters consume and produce a common typed investigation protocol.

### 4.11 Commercial quality

Every merged component must consider operability, upgrade compatibility, diagnostics, security, resource limits, licensing, supportability, and rollback. Demonstration-only implementations must be clearly isolated and may not become the default path by inertia.

---

## 5. System architecture

```text
+--------------------------- User surfaces ----------------------------+
| Orus Studio | CLI | IDE/DAP | GDB client | MCP clients | Incident API |
+-------------------------------+---------------------------------------+
                                |
                     HTTP / WebSocket / DAP / MCP
                                |
+-------------------------------v---------------------------------------+
|                         Interaction gateway                          |
| auth | sessions | routing | validation | streaming | backpressure    |
+-------------------------------+---------------------------------------+
                                | versioned internal IPC
                +---------------+----------------+
                |                                |
+---------------v----------------+  +------------v---------------------+
| Investigation / agent service |  | Session supervisor               |
| plans | hypotheses | evidence  |  | lifecycle | quotas | isolation   |
+---------------+----------------+  +------------+---------------------+
                | typed tools                    |
        +-------v-------------------+             |
        | Debug intelligence       |             |
        | slicing | lifetime | HB  |             |
        | diff | invariants | race |             |
        +-------+-------------------+             |
                |                                 |
        +-------v---------------------------------v---------------------+
        | Semantic execution graph and sparse indexes                  |
        +-------------------------+------------------------------------+
                                  |
                  +---------------v----------------+
                  | Isolated replay worker/session |
                  | seek | run | branch | validate |
                  +---------------+----------------+
                                  |
                  +---------------v----------------+
                  | Checkpoints and trace segments |
                  +--------------------------------+

Process tree and threads
  -> process/task supervisor
  -> deterministic scheduler
  -> syscall/signal interception
  -> recorder hot path
  -> trace writer and compression workers
  -> trace store

Symbol worker: pinned LLVM/LLDB/Clang, isolated behind a typed protocol.
```

### 5.1 Process boundaries

The default deployment uses separate processes for:

- Recorder and target-task supervisor.
- Replay worker, one authoritative owner per active replay session.
- Session supervisor.
- Symbol and C++ expression worker.
- Index and analysis workers.
- Web/API gateway.
- Agent investigation service.

The recorder and replay engine may share carefully designed core libraries. They must not be linked into the web gateway.

### 5.2 Ownership rules

- Each active replay session has exactly one authoritative replay worker.
- Each live target task has one controlling scheduler owner.
- Other components send commands and receive immutable results.
- Network callbacks, compression, symbol parsing, model work, and indexing never execute on a target-control or replay-control thread.
- Shared mutable state across services is prohibited.

### 5.3 Reference path and commercial fast path

Maintain two execution paths where useful:

1. **Reference correctness path:** simple, highly validated, and allowed to be slower. It establishes semantics and diagnoses divergence.
2. **Commercial fast path:** batching, selective interception, injected runtime support, shared-memory rings, and optimized encoding while producing the same logical events.

The reference path is a verification oracle, not the shipping performance target. The commercial fast path must be introduced early enough that trace schema, syscall policy, and ownership are not designed around per-event `ptrace` stops.

---

## 6. Deterministic execution model

### 6.1 Universal execution coordinate

All debugger state is addressable by a stable coordinate:

```cpp
struct ExecutionPoint {
    TraceId trace_id;
    BranchId branch_id;
    std::uint64_t event_id;
    std::uint64_t logical_ticks;
    VirtualProcessId process_id;
    VirtualThreadId thread_id;
    std::uint64_t program_counter;
    std::optional<SpanId> distributed_span_id;
    std::optional<AgentRunId> agent_run_id;
};
```

This is conceptual. Serialization uses explicit fixed-width fields and versioned schemas, never native ABI-dependent layout.

### 6.2 Event classes

The trace represents at least:

- Process creation, parent relationship, exec, wait, and exit.
- Thread creation, exit, join, and TLS lifecycle.
- Scheduler decisions, blocking, wakeups, and runnable-set changes.
- Syscall entry, exit, result, `errno`, and kernel-written memory.
- Futex and supported synchronization outcomes.
- Signal generation, delivery, disposition, mask, interruption, and restart.
- Address-space mapping changes.
- CPU nondeterministic instructions or emulated results.
- File-descriptor and virtual kernel-object transitions.
- Checkpoints.
- External input and observable output.
- Application markers and correlation IDs.
- Integrity, compatibility, and performance metadata.

Do not record every normal instruction merely to implement reverse execution. Re-execute deterministic instructions, record determinism-boundary inputs and schedule, and use checkpoints plus optional control-flow indexes.

### 6.3 Determinism boundary

Capture every value that can alter future user-space execution, including:

- Syscall results and kernel-written buffers.
- Time, randomness, process identity, and environment observations.
- Signals and precise delivery positions.
- Thread scheduling and synchronization outcomes.
- Address-space layout and mapping content where necessary.
- CPU instructions with nondeterministic or environment-dependent results.
- File descriptors, offsets, readiness, and virtual kernel-object state.
- Inputs from files, sockets, pipes, pseudo-terminals, and supported devices.

### 6.4 Syscall model

Every supported syscall has one explicit policy:

1. **Emulate** from recorded data.
2. **Execute under control** and validate.
3. **Virtualize** against debugger-owned state.
4. **Reject** as unsupported.

Each syscall implementation documents:

- Inputs affecting determinism.
- Output memory ranges.
- Process/thread lifecycle effects.
- File-descriptor effects.
- Blocking and wakeup behavior.
- Signal interactions.
- Mapping effects.
- Replay side-effect policy.
- Success, error, interrupted, race, and boundary tests.

### 6.5 Scheduling

Strict deterministic mode runs one target task at a time and records every scheduling decision. “Task” means a virtual process/thread execution context, not only a pthread.

The scheduler records:

- Runnable set before and after a decision.
- Selected virtual process and thread.
- Logical clock position.
- Reason for the switch.
- Blocking object or syscall when applicable.
- Wakeup source and target.
- Signal-related transitions.

A later concurrency-exploration mode may vary switch points to expose failures, but every discovered failure must be captured and replayable in strict mode.

### 6.6 Signals

An asynchronous signal requires an exact logical position, target virtual thread, mask state, and disposition. The implementation may use deterministic hardware counters or instrumentation and single-step near the target to compensate for counter skid. Debugger-generated traps must be distinguished from target-generated traps.

### 6.7 Address space and process identity

Replay reproduces mappings at the same virtual addresses. The trace identifies exact executables and libraries through build IDs and content hashes. Preserve the artifacts or mapped bytes needed for replay.

Use virtual PIDs and TIDs. Translate process identity consistently wherever the supported program can observe it, including syscalls, signals, waits, locks, `/proc` behavior within the declared contract, and generated names.

### 6.8 Checkpoints

Reverse execution restores an earlier checkpoint and deterministically replays forward.

A checkpoint captures or reconstructs:

- Every virtual process and thread register set.
- Parent/child and thread-group relationships.
- Signal masks and pending signals.
- Mapping topology, permissions, and relevant content.
- Dirty memory pages.
- File descriptors and virtual kernel objects.
- Futex, blocking, runnable, and scheduling state.
- Virtual PID/TID state.
- Replay logical time and trace cursor.

A simple `fork()` is not a complete checkpoint for a process tree with multiple threads. Incremental checkpoints use parent relationships, dirty-page data, changed mappings, changed task state, and changed kernel-object state.

### 6.9 Required reverse operations

- Seek by event and execution point.
- Reverse instruction step.
- Reverse continue to breakpoint or condition.
- Previous source-line execution.
- Previous function call or return.
- Previous expression or value change.
- Last write to an address or object field.
- First event satisfying an invariant violation.
- First meaningful divergence between two runs.

The earliest correct implementation may replay from program start, but performance budgets begin immediately and checkpoints must arrive before commercial interactive release.

---

## 7. Performance architecture and gates

### 7.1 Performance contract

Every milestone includes:

1. Representative microbenchmarks and end-to-end workloads before major implementation choices become fixed.
2. A baseline recorded on a pinned benchmark environment.
3. CPU, wall time, memory, allocation, trace-size, and latency measurements as applicable.
4. A regression threshold enforced on stable dedicated runners.
5. Profiles for any material regression or unexplained variance.
6. An ADR for a regression that is intentionally accepted.

A performance result without workload, host CPU, CPU affinity, kernel, compiler, optimization configuration, storage device, and sample statistics is not a valid product claim.

### 7.2 Initial commercial objectives

These objectives seed the performance budget. M0 must convert them into documented, reproducible gates on designated hardware:

- Zero steady-state dynamic allocations in the recorder event-descriptor path after startup.
- Zero contended mutexes in the target-control and replay-control hot paths.
- No statistically significant regression above 3% in a gated hot benchmark without explicit approval and evidence.
- CPU-bound strict recording objective: no more than 1.5x native wall time when compared with the same CPU affinity and runnable-core topology.
- Syscall-heavy strict recording objective: no more than 2.5x after the commercial interception fast path is enabled.
- Event encoding pipeline objective: at least 20 million compact descriptors per second per modern benchmark core, independent of kernel stop overhead.
- Uncompressed sequential writer objective: at least 80% of measured raw sequential storage throughput for equivalent block sizes.
- Local pause and cancel p99 latency below 50 ms while bulk timeline or memory data is streaming.
- Trace opening proportional to manifest and index size, never total event volume.
- Timeline query work proportional to viewport resolution and selected filters, not total trace length.
- Replay and recording memory remain bounded by configured queues, checkpoint policy, and cache budgets.

These are engineering objectives, not permission to hide unfavorable comparisons. For strict serialized concurrency, publish both:

- Native execution pinned to equivalent runnable-core topology.
- Unrestricted native execution using all available cores.

### 7.3 Hot-path rules

- No heap allocation per event.
- No formatted logging per event.
- No JSON, database calls, source parsing, model calls, general-purpose reflection, or WebSocket work in recorder/replay hot paths.
- FlatBuffers objects are boundary representations, not hot-path event objects.
- Prefer fixed-size descriptors, compact tagged unions, preallocated arenas, and bounded SPSC queues.
- Use single-owner state; avoid shared mutable maps.
- Separate hot and cold fields.
- Align frequently written counters and queues to avoid false sharing.
- Use structure-of-arrays for large scan-oriented indexes where benchmarks support it.
- Batch target-memory reads, event serialization, compression, checksums, and writes.
- Use `process_vm_readv`/`process_vm_writev` for bulk transfer when valid and measured.
- Reuse compression, hash, and parser contexts.
- Index asynchronously or in bounded background work.
- Virtualize debugger breakpoints so trap bytes do not contaminate logical state.
- Keep diagnostic counters lock-free or per-thread and defer formatting.
- Design for NUMA awareness in large recording and analysis deployments, but do not add NUMA complexity without measured benefit.

### 7.4 Commercial recording fast path

The architecture must not depend permanently on stopping in `ptrace` at every syscall.

Evaluate and implement, as correctness permits:

- `ptrace` for lifecycle, registers, exceptional transitions, and the reference path.
- seccomp-BPF for selective interception.
- A small injected Orus runtime for syscall buffering and cooperation where transparent semantics can be preserved.
- Shared `memfd` rings and `eventfd` wakeups.
- Batched event descriptors and large blob transfer.
- `pidfd` APIs for task identity and lifecycle safety.
- `perf_event_open` for validated deterministic progress counters.
- `userfaultfd` or alternative page tracking for checkpoint deltas.
- `io_uring` for Orus-owned storage only when it wins a representative benchmark; never adopt it by fashion.

The reference path and fast path must emit equivalent logical events for an overlap corpus.

### 7.5 SIMD and ISA dispatch

Use SIMD wherever measured data-parallel work justifies it, including checksums, hashing, delta/varint scanning, bitmap operations, page comparison, memory search, trace filtering, and timeline aggregation.

Policy:

- Preserve a portable scalar implementation as the semantic reference.
- Prefer **Google Highway** as the default portable SIMD and runtime-dispatch candidate.
- Direct x86 intrinsics are allowed for proven hotspots behind a small tested abstraction and runtime CPU dispatch.
- Initialize dispatch tables once; never execute repeated feature discovery in the hot path.
- Ship a compatible x86-64 baseline and dispatch to SSE4.2, AVX2, AVX-512, CRC, AES, or other facilities only when present and beneficial.
- Test every vectorized implementation against the scalar reference with randomized and boundary inputs.
- Benchmark downclocking, code size, alignment, and tail behavior; AVX-512 is not automatically faster.
- Do not force the entire binary to the highest build-machine ISA.

### 7.6 High-performance library candidates

The following are preferred candidates, subject to benchmark, maintenance, license, and security review:

| Need | Preferred candidate or policy |
|---|---|
| Portable SIMD and runtime dispatch | Google Highway |
| Fast non-cryptographic checksums | XXH3; hardware CRC32C where measured |
| Artifact/content identity | BLAKE3 or another approved cryptographic hash |
| High-ratio trace compression | Zstandard with reusable contexts and independent frames |
| Lowest-latency compression option | LZ4, selected adaptively when it beats Zstandard for the workload |
| Sparse event sets | CRoaring |
| Dense high-performance hash tables | Abseil Swiss tables or a benchmarked equivalent |
| Catalog and metadata | SQLite in WAL mode, never per-event storage |
| Boundary serialization | FlatBuffers |
| General service allocator | Evaluate mimalloc and jemalloc per component; hot paths still use owned arenas |
| Asynchronous storage submission | liburing only after benchmark evidence |
| Networking | Boost.Asio and Boost.Beast outside deterministic hot paths |
| TLS | OpenSSL through a narrow boundary |
| C++ and DWARF semantics | Pinned LLVM/LLDB/Clang |

Do not add a large framework because it contains one useful container. Dependency weight, transitive build cost, ABI surface, support burden, and license obligations are part of performance and commercial evaluation.

### 7.7 Compiler and linker optimization

Bazel release configurations should support:

- Clang as the primary compiler and GCC compatibility builds.
- LLD as the default linker; mold may be benchmarked for developer build speed.
- Optimized release builds with measured `-O2` versus `-O3` choices.
- ThinLTO where it improves shipped binaries without unacceptable build cost.
- PGO using a versioned representative workload corpus.
- Optional post-link optimization such as BOLT when reproducible and beneficial.
- Split debug information and retained build IDs.
- Sanitizer configurations separate from release performance measurements.
- Component-specific exception and RTTI policy. Performance-critical leaf libraries may disable them only through an ADR and typed error design.

### 7.8 Performance observability

Collect without polluting hot paths:

- Event counts and bytes by event type.
- Queue depth and backpressure.
- Scheduler decisions and stop reasons.
- Time in kernel control, encoding, compression, hashing, writing, replay, validation, and indexing.
- Allocations and peak resident memory.
- Checkpoint dirty pages and restore cost.
- Hardware counters on controlled benchmark runs.
- p50, p95, p99, and tail outliers for interactive operations.

---

## 8. Nix, Bazel, and dependency management

### 8.1 Responsibility split

**Nix owns:**

- Pinned `nixpkgs` and other environment inputs.
- Developer shells.
- CI and benchmark machine environments.
- Clang/LLVM/LLD, Bazel, Node, pnpm, Python, formatters, profilers, and packaging tools.
- Reproducible release packaging and optional container/image outputs.

**Bazel owns:**

- All source compilation and linking.
- The complete declared target dependency graph.
- Code generation.
- Unit, integration, deterministic replay, fuzz, and benchmark targets.
- Web application build and tests.
- Release binaries and test artifacts.
- Build Event Protocol output, profiles, remote cache, and optional remote execution.

A Nix package being present does not automatically make it a Bazel dependency. Linked and generated inputs must be exposed through explicit Bazel targets and labels.

### 8.2 Required root files

```text
MODULE.bazel
MODULE.bazel.lock
BUILD.bazel
.bazelrc
.bazelversion
flake.nix
flake.lock
README.md
PROJECT_SEED.md
AGENTS.md
```

Add `bazel/` for toolchain and repository definitions and `nix/` for flake modules or packaging when needed.

### 8.3 Bazel policy

- Bzlmod is mandatory; do not introduce a legacy `WORKSPACE` dependency flow unless a temporary compatibility ADR requires it.
- Pin the Bazel version.
- Pin direct dependency versions and commit the module lockfile.
- Prefer Bazel-native dependencies from trustworthy registries or pinned archives.
- Verify checksums for downloaded artifacts.
- Use hermetic Clang toolchains declared to Bazel.
- Use sandboxed actions and minimize network access during builds.
- Keep generated files reproducible; prefer generating them in Bazel actions rather than committing them.
- Define `dev`, `release`, `asan`, `ubsan`, `tsan`, `fuzz`, `benchmark`, `coverage`, `pgo-generate`, and `pgo-use` configurations as they become applicable.
- Support remote cache from the beginning; remote execution may follow after toolchains are fully hermetic.
- Never use undeclared host headers or libraries.

### 8.4 Nix policy

- Use a flake with committed lockfile.
- `nix develop` must provide the exact documented toolchain.
- `nix flake check` must exercise repository checks suitable for a clean environment.
- `nix build .#orus` must produce a release artifact through the declared Bazel build path.
- Avoid impure environment lookups.
- Keep secrets, remote-cache credentials, and signing material outside the flake and repository.
- Provide supported-system outputs explicitly.

### 8.5 Canonical developer commands

The bootstrap milestone must make these commands work:

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

CI and contributor documentation use the same commands. Wrappers may improve ergonomics but may not hide a different build path.

### 8.6 Dependency admission policy

Before adding a dependency, document:

- Purpose and measured benefit.
- Exact version and pinning mechanism.
- License and notice obligations.
- Maintainer and release health.
- Security and vulnerability process.
- Transitive dependency and build cost.
- ABI and upgrade risk.
- Binary-size and runtime-memory impact.
- Replacement and removal cost.
- Boundary that prevents the dependency from spreading through the architecture.

Generate an SBOM for releases and retain third-party notices. Commercial redistribution requirements must be reviewed before release.

### 8.7 Initial dependency set

Expected initial dependencies include:

- LLVM/LLDB/Clang, pinned and isolated.
- Boost.Asio, Boost.Beast, Boost.URL, and low-volume Boost.JSON.
- FlatBuffers.
- Zstandard.
- XXH3.
- BLAKE3 or an approved equivalent for artifact identity.
- SQLite.
- CRoaring.
- Google Highway.
- OpenSSL.
- GoogleTest and Google Benchmark.
- LLVM libFuzzer and sanitizers.
- TypeScript, React, Monaco, xterm.js, TanStack Virtual, and a measured Canvas/WebGL renderer for Studio.

The web build should be represented in Bazel, with a committed pnpm lockfile consumed through Bazel JavaScript rules. Direct ad hoc `pnpm` commands are not the canonical CI build path.

---

## 9. Trace format and storage

### 9.1 Logical layout

```text
failure.orus/
  manifest.fb
  catalog.sqlite
  events/
    segment-000000.zst
    segment-000001.zst
  checkpoints/
    checkpoint-000001/
  indexes/
    event-time/
    processes.roar
    threads.roar
    pc-executions/
    memory-writes/
    scheduling/
  blobs/
  binaries/
  symbols/
  sources/
  diagnostics/
  performance/
```

A later packed format may be added without changing the logical model.

### 9.2 Segment design

Segments are independently checksummed and decodable. Use:

- Delta-coded event IDs, clocks, process IDs, thread IDs, and addresses.
- Varints only where benchmarks justify their branch and decode cost.
- Interned repeated metadata.
- Separate large blobs for syscall memory output and file data.
- Explicit byte order and schema version.
- No native pointers or implicit C++ padding.
- Crash-safe append and commit markers.
- Index hints produced with minimal hot-path work.

Benchmark block sizes and compression algorithms. Do not compress each event independently.

### 9.3 Recorder pipeline

```text
target-control/scheduler owner
  -> preallocated descriptors
  -> per-producer or SPSC rings
  -> merge/order stage with explicit ownership
  -> trace writer
  -> compression/hash worker pool
  -> large aligned sequential writes
```

Variable-size data uses preallocated arenas or a dedicated blob pipeline referenced by offsets.

### 9.4 Trace safety

Treat traces as sensitive and untrusted. They may contain credentials, keys, file contents, network payloads, environment variables, prompts, model outputs, personal data, and proprietary source.

Requirements:

- Bounds-check every length, offset, count, and nesting level.
- Fuzz every parser.
- Enforce resource limits before allocation.
- Support redaction and retention policy.
- Design for encryption at rest and enterprise key management.
- Never execute embedded content as instructions to an agent.
- Preserve provenance labels for source, runtime, network, and model-derived data.

---

## 10. Debug semantics and protocols

### 10.1 Debug semantics

- Pin LLVM/LLDB/Clang through Nix and Bazel.
- Use LLVM object/DWARF facilities, LLDB debugger components, Clang expression parsing, and Clang LibTooling.
- Isolate symbol and expression handling in a worker process because symbol inputs are untrusted and internal APIs may change.
- Maintain a GDB RSP adapter where useful, but never use textual GDB commands as Orus’s internal API.

### 10.2 Replay Control Protocol

Define a versioned RCP independent of DAP, GDB, WebSocket, and model providers.

Representative operations:

```text
OpenTrace
CloseTrace
GetCapabilities
GetExecutionPoint
Seek
RunForward
RunBackward
Pause
GetProcesses
GetThreads
GetRegisters
ReadMemory
GetMappings
GetStack
GetDisassembly
SetStopConditions
CreateCheckpoint
ForkBranch
ApplyIntervention
FindLastWrite
FindCondition
CompareExecution
VerifyInvariant
CancelRequest
```

Requirements:

- Unique request IDs.
- Progress and cancellation.
- Trace, branch, process, thread, execution point, and schema identity in responses.
- Shared-memory/file-descriptor transfer for large payloads.
- Typed errors with recoverability and diagnostic context.
- Explicit capability negotiation.
- Documented version compatibility.

### 10.3 Internal IPC

Use:

- Unix-domain sockets for commands and small responses.
- FlatBuffers for typed boundary messages.
- `memfd` for large immutable buffers.
- `eventfd` for wakeups.
- `SCM_RIGHTS` for file-descriptor passing.

Do not use Beast or WebSocket between local core processes.

---

## 11. Web gateway and Orus Studio

### 11.1 Gateway

Use Boost.Asio and Boost.Beast for the browser-facing gateway.

The gateway handles:

- Authentication and authorization.
- HTTP assets and immutable range downloads.
- WebSocket lifecycle.
- Request validation and session routing.
- Bounded queues, quotas, cancellation, and backpressure.
- Translation between browser messages and RCP.

It must not control target tasks, parse DWARF, restore checkpoints, evaluate C++, compress traces on an I/O thread, or own authoritative replay state.

### 11.2 Network paths

```text
/ws/control/{session-id}
/ws/data/{session-id}
/blob/{artifact-id}
```

Use separate control and bulk paths so a timeline or memory transfer cannot block pause or cancel. Large immutable artifacts use HTTP range requests.

Each WebSocket connection uses serialized access, one outstanding read, and a bounded write queue.

### 11.3 Studio

Studio is an evidence workspace, not merely a chat panel. It combines:

- Source and disassembly.
- Process and thread trees.
- Timeline and thread/process lanes.
- Stacks, variables, registers, and memory.
- Syscalls, signals, exceptions, mappings, and synchronization.
- Hypotheses, evidence, contradictions, and findings.
- Replay branch tree and experiment results.
- Agent action log and approval queue.
- Inferior terminal and debugger console.

The browser never receives the entire event stream. It requests viewport-specific aggregated tiles rendered with Canvas/WebGL. Large lists and trees are virtualized. Binary decoding and decompression run in Web Workers.

Every agent claim is clickable and navigates to its evidence.

---

## 12. Agentic causal debugging

### 12.1 Agent role

The agent is a planner and interpreter. It may formulate hypotheses, select deterministic tools, compare evidence, request experiments, explain findings, and propose source changes. Its probability or prose is never execution evidence.

### 12.2 Investigation state

Persist structured state:

```text
Reported
  -> Failure localized
  -> Hypotheses generated
  -> Evidence collected
  -> Hypotheses eliminated
  -> Root cause identified
  -> Counterfactual validated
  -> Patch proposed
  -> Patch verified
  -> Human approved or rejected
```

Persist tool calls, typed results, evidence accepted or rejected, concise rationale, confidence, budgets, and next action. Do not rely on hidden model reasoning as product state.

### 12.3 Evidence strengths

| Level | Meaning |
|---|---|
| Observed | The event occurred before or near the failure. |
| Dependency-supported | Dynamic data or control dependencies connect it. |
| Concurrency-supported | Happens-before or conflicting-access evidence connects it. |
| Counterfactually validated | Changing the event or condition changes the failure. |
| Regression-verified | The correction succeeds across a defined trace, test, and performance corpus. |

Temporal proximity alone is not causation.

### 12.4 Typed tools

The agent-facing API should include:

```text
open_trace
summarize_trace
describe_failure
get_execution_context
seek
read_memory
evaluate_expression
get_stack
get_processes
get_threads
get_mappings
find_first_condition
find_last_condition
find_last_write
find_previous_call
find_previous_exception
find_first_bad_state
trace_value_origin
build_backward_slice
build_forward_slice
analyze_happens_before
find_data_races
find_lock_cycles
find_use_after_free
find_out_of_bounds_access
compare_timepoints
compare_runs
compare_branches
fork_replay
apply_intervention
run_to_condition
verify_invariant
apply_patch
run_test_suite
run_performance_suite
create_hypothesis
reject_hypothesis
create_finding
attach_evidence
export_reproduction
```

Tools return structured facts and evidence references, not only prose.

### 12.5 Counterfactual replay

A branch forks at an execution point and may:

- Override a syscall result or input buffer.
- Change a scheduling decision.
- Delay or advance a task.
- Change signal-delivery position.
- Override a function return value.
- Modify memory.
- Enable an assertion.
- Replace a model/tool result.
- Apply an ABI-compatible change or rebuilt executable.

Every branch records parent, fork point, interventions, build identity, source revision, environment, and verification status.

### 12.6 Proof-carrying patches

An agent-generated patch is incomplete without:

- Root-cause claim.
- Evidence references.
- Counterfactual result where applicable.
- Patch and affected source locations.
- Explanation of how it addresses the causal mechanism.
- Original failing traces replayed.
- Passing traces checked.
- Tests and sanitizers.
- Performance comparison against the budget.
- Known limitations and residual uncertainty.

Use Clang AST and LibTooling for C++ transformations; do not use regex editing for semantic changes.

---

## 13. Security model

### 13.1 Isolation

Recording and replay sessions run with least privilege and, where practical:

- User, PID, and mount namespaces.
- Private temporary filesystem.
- Cgroup CPU, memory, process, storage, and I/O limits.
- `no_new_privs`.
- Narrow seccomp policy.
- No gateway or repository credentials.
- No direct catalog access from untrusted workers.
- No external network during replay by default.
- Explicitly passed file descriptors and artifacts only.

Seccomp is one layer, not the whole sandbox.

### 13.2 Capabilities

```text
trace.read
memory.read
replay.seek
replay.fork
replay.modify
source.read
source.patch
build.execute
tests.execute
performance.execute
network.access
repository.commit
repository.publish
```

Read-only investigation does not receive patch, build, network, commit, or publish capabilities.

### 13.3 Prompt-injection boundary

Treat source, comments, process memory, logs, filenames, network data, database fields, tool output, and model output from the debugged program as untrusted. Preserve provenance and prevent untrusted content from redefining policy.

### 13.4 Human approval

Publishing commits, opening non-draft pull requests, deploying, accessing external credentials, or broadening network access requires explicit policy and visible authorization.

---

## 14. Repository layout

```text
orus/
  MODULE.bazel
  MODULE.bazel.lock
  BUILD.bazel
  .bazelrc
  .bazelversion
  flake.nix
  flake.lock
  LICENSE                         # only after owner chooses a license
  README.md
  PROJECT_SEED.md
  AGENTS.md

  bazel/
    toolchains/
    config/
    repositories/
  nix/
  docs/
    architecture/
    adr/
    protocols/
    trace-format/
    security/
    performance/
    operations/

  protocol/
    flatbuffers/

  core/
    base/
    simd/
    linux/
    process-control/
    task-supervisor/
    record/
    replay/
    scheduler/
    syscall-model/
    checkpoint/
    trace-format/
    trace-store/

  runtime/
    injected/

  debug/
    replay-protocol/
    symbol-worker/
    lldb-adapter/
    dap-adapter/
    gdb-rsp-adapter/

  analysis/
    event-index/
    memory-write-index/
    execution-diff/
    slicing/
    lifetime/
    concurrency/
    invariants/

  server/
    supervisor/
    gateway/
    artifact-service/
    investigation-service/
    mcp-server/

  cli/
  web/
  examples/
  tests/
    deterministic-programs/
    process-tree/
    concurrency/
    syscall-corpus/
    replay-divergence/
    trace-format/
    trace-fuzzing/
    protocol/
    integration/
    benchmarks/
  tools/
    trace-inspect/
    trace-verify/
    trace-repair/
    performance-report/
```

Create directories only when the first owned component and test enter the repository.

---

## 15. Engineering workflow for the autonomous factory

### 15.1 Planning

For each milestone:

1. Restate correctness, concurrency, performance, security, and compatibility acceptance criteria.
2. Create ADRs before irreversible choices.
3. Divide work into vertical slices that compile and test.
4. Identify unsupported paths and how they fail closed.
5. Define the benchmark and regression gate before implementing the hot path.
6. Define how failure and divergence are diagnosed.

### 15.2 Change discipline

- One coherent change per pull request.
- Keep the repository buildable at every merged commit.
- Add tests with behavior, not after behavior.
- Add benchmark coverage with hot-path behavior.
- Do not introduce a dependency without the admission record.
- No TODO without an issue or milestone reference.
- No silent fallback for deterministic behavior.
- No generated artifact without a reproducible Bazel action.
- No CMake files or commands.
- No performance claim without reproducible evidence.
- Do not mass-format unrelated code.

### 15.3 ADR template

Use `docs/adr/NNNN-title.md`:

```text
Status
Context
Decision
Alternatives considered
Consequences
Correctness impact
Concurrency impact
Performance impact and benchmark
Security impact
Commercial/support impact
Compatibility and migration
Validation plan
Rollback plan
```

### 15.4 Definition of done

Every applicable change has:

- Unit tests.
- Multi-process/multi-thread integration or deterministic replay coverage.
- Negative and unsupported-path tests.
- Documentation of externally visible behavior.
- Trace/protocol compatibility consideration.
- Benchmark and before/after results for hot paths.
- Fuzz target for untrusted structured input.
- Relevant sanitizer-clean execution.
- No unexplained warnings.
- Resource-bound and failure diagnostics.
- Rollback or migration story.

---

## 16. Milestones and gates

Every milestone gate includes performance results. Passing functional tests without the milestone’s performance report does not pass the gate.

### M0 — Commercial repository, build, and performance foundation

Deliver:

- Nix flake and lockfile.
- Bazel/Bzlmod project, module lockfile, and hermetic Clang toolchain.
- `orus --version` and `orus doctor` skeletons with real build metadata.
- Dev, release, sanitizer, fuzz, and benchmark configurations.
- Linux x86-64 CI for Clang and GCC compatibility.
- Dedicated or controlled performance-runner design and baseline format.
- Benchmark harness, allocation counter, benchmark result schema, and regression comparison tool.
- Formatting, linting, SBOM, dependency admission, ADR, and `AGENTS.md` policies.
- Minimal multi-process and multi-thread target programs.

Gate:

- A clean clone builds and tests only through documented Nix+Bazel commands.
- `nix flake check` and `nix build .#orus` succeed.
- No undeclared host dependency and no CMake.
- Benchmark baselines are reproducible enough to establish regression thresholds.
- No deterministic execution claim yet.

### M1 — Linux process-tree and thread-control core

Deliver:

- Launch a target under tracing.
- Discover and control multiple processes and threads.
- Observe process/thread clone, exec, stops, exits, waits, and signals in the supported subset.
- Virtual process/thread identity model.
- Read/write registers for every task through a typed x86-64 abstraction.
- Bulk target-memory access.
- Continue and single-step selected tasks.
- Initial strict scheduler that runs one target task at a time.
- Precise stop-reason model.

Gate:

- A test process tree with at least two processes and multiple threads per test run can be repeatedly controlled and cleaned up.
- Normal exit, crash, exec failure, child exit, thread exit, signal, and tracer failure are covered.
- Debugger traps and target traps are distinguishable.
- No task leaks or PID reuse confusion.
- Control latency and memory-transfer benchmarks meet the M1 budget.
- No recording yet.

### M2 — Concurrent recording and trace foundation

Deliver:

- Versioned trace manifest and event segments.
- Process, thread, scheduler, syscall, mapping, and file-descriptor event schemas.
- Initial syscall and futex policy framework.
- Capture return values and kernel-written memory.
- Capture exact binaries, libraries, process tree, mappings, and task identity.
- Crash-safe trace writer and inspector.
- Reference interception path and initial commercial buffered path architecture.

Gate:

- The first multi-process/multi-thread vertical-slice trace records the complete declared topology and schedule.
- Unsupported syscalls and synchronization paths fail closed.
- Truncated and corrupt traces fail safely.
- Parser fuzz targets exist.
- Event pipeline, writer, compression, allocation, and trace-size benchmarks pass.

### M3 — Deterministic process-tree and thread replay

Deliver:

- Replay from process-tree start.
- Recreate task topology, mappings, virtual identity, and supported file-descriptor state.
- Replay the recorded strict schedule.
- Prevent repeated external side effects.
- Inject or validate recorded syscall and synchronization outcomes.
- Event-boundary state validation for the selected task and relevant global state.
- Earliest-divergence report including process/thread/scheduler context.

Gate:

- The first concurrent vertical slice replays identically 100 times.
- Mutated traces, binaries, mappings, or scheduling events cause localized failure.
- Replay performs no forbidden external writes.
- Reference and fast paths agree on the overlap corpus.
- Recording and replay overhead meet the M3 budget.

### M4 — Reverse execution and multi-task checkpoints

Deliver:

- Seek by execution point.
- Reverse instruction step and reverse continue.
- Historical registers and memory for any virtual task.
- Checkpoint abstraction covering the process tree and threads.
- Initial incremental memory tracking.

Gate:

- Reverse then forward returns to the identical state across the concurrent corpus.
- Checkpoint restore preserves process, thread, scheduler, descriptor, and signal state.
- Interactive latency and checkpoint memory meet the M4 budget.

### M5 — Stable debugger interfaces

Deliver:

- RCP schema and local IPC.
- CLI over RCP rather than internal object access.
- DAP adapter.
- Optional GDB RSP adapter.
- Cancellation, progress, typed errors, and capability negotiation.

Gate:

- Replay workers can fail without corrupting traces.
- Two clients cannot concurrently mutate one session.
- Protocol compatibility and throughput/latency tests pass.

### M6 — Orus Studio vertical slice

Deliver:

- Beast/Asio gateway.
- Session and trace opening.
- Source/disassembly, process/thread tree, stack, registers, memory, and timeline.
- Forward/reverse controls.
- Bounded WebSocket queues and cancellation.

Gate:

- Bulk requests do not block pause/cancel.
- Browser receives viewport tiles, not the complete stream.
- Gateway has no direct target control or DWARF parsing.
- Studio latency and memory budgets pass.

### M7 — Broad Linux concurrency and process semantics

Deliver:

- Expanded futex, mutex, condition-variable, barrier, robust-list, and blocking behavior.
- Broader `fork`, `vfork`, `clone`, `clone3`, `exec`, wait, and `posix_spawn` paths.
- Precise per-thread signals and restart behavior.
- Dynamic libraries and controlled `dlopen`.
- Additional scheduler and syscall fast paths.

Gate:

- Representative real C++ service workloads replay repeatedly.
- Unsupported paths fail precisely.
- Schedule mutation is detected.
- Commercial overhead budgets pass across CPU, syscall, memory, process, and synchronization workloads.

### M8 — Causal analysis

Deliver:

- Memory object and lifetime model.
- Last-write and first-bad-state search.
- Passing/failing alignment and earliest semantic divergence.
- Dynamic backward slicing for a bounded subset.
- Happens-before and conflicting-access analysis.
- Structured findings and evidence references.

Gate:

- Known-bug corpus returns the responsible event and source region.
- At least one plausible incorrect hypothesis is rejected by evidence.
- Search and index latency budgets pass at commercial trace sizes.

### M9 — Read-only debugging agent

Deliver:

- Provider-independent investigation service.
- Typed tools over RCP and analysis services.
- Hypothesis/evidence state machine.
- MCP server.
- Studio investigation panel with clickable claims.
- Prompt-injection provenance controls and budgets.

Gate:

- Read-only mode cannot mutate source, replay state, repository, or network.
- Every factual benchmark claim cites structured evidence.
- Disabling models leaves deterministic debugger functions operational.
- Agent activity does not degrade recorder/replay hot paths.

### M10 — Counterfactual branches and patch verification

Deliver:

- Replay branch model and provenance.
- Controlled memory, input, schedule, signal, and function interventions.
- Branch comparison.
- Clang-based patch workspace.
- Rebuild and replay against original traces.
- Test, sanitizer, and performance verification bundle.
- Human approval gates for publication.

Gate:

- Experiments are reproducible.
- A patch is never verified merely because a crash disappeared.
- Evidence, test results, performance comparison, residual risk, and exact build identity accompany it.

### M11 — Production recording and cross-layer causality

Deliver only after previous gates:

- Ring-buffer/flight-recorder mode.
- Storage quotas and retention.
- Incident packaging.
- OpenTelemetry and agent-span correlation.
- Failure clustering and organizational incident memory.
- Policy-controlled autonomous investigation.

Gate:

- Production recorder has explicit overhead, tail-latency, storage, and loss measurements.
- Sensitive-data controls are enabled by default.
- Incident memory revalidates old patterns against new traces.

---

## 17. First vertical slice — implement this first

The first end-to-end slice is intentionally small in syscall breadth but **must be multi-process and multi-thread**.

### 17.1 Example workload

Create a controlled C++ test application with this topology:

1. A parent process creates a pipe or socket pair.
2. The parent creates or forks a child and the child executes a small helper binary.
3. The parent creates at least three pthreads.
4. The child helper creates at least two pthreads.
5. Threads synchronize through a deliberately limited pthread mutex/condition-variable or barrier path implemented by the supported futex subset.
6. One parent thread calls `clock_gettime` and `getrandom`.
7. Another parent thread performs deterministic CPU work and sends a compact message to the child.
8. Child threads process the message, synchronize, and return a deterministic response.
9. Parent threads join; the parent waits for the child; all processes exit with known statuses.
10. The program emits one deterministic logical result.

Forking may occur before parent thread creation in the first slice to avoid unsupported fork-from-multithreaded-state semantics, but both parent and child execution must contain multiple threads and the trace must contain multiple processes.

### 17.2 Required commands

```bash
orus record --output test.orus -- ./orus-example-concurrent
orus inspect test.orus
orus replay test.orus
```

### 17.3 Acceptance criteria

- Trace contains exact executable identities, process tree, virtual process/thread IDs, task lifecycle, mappings, scheduler decisions, synchronization events, nondeterministic values, and exit statuses.
- Replay reconstructs the same process/thread topology and strict schedule.
- Replay observes recorded time and random bytes, not current values.
- Replay suppresses duplicate external output; recorded output may be displayed separately.
- Event-boundary validated state matches on 100 repeated replays.
- Mutating a lifecycle, scheduler, synchronization, or data event causes the earliest relevant divergence.
- Replacing either executable with another build fails before authoritative replay.
- Truncating or corrupting a segment fails safely.
- No target task remains after recorder or replay teardown.
- Event, scheduler, writer, trace-size, recording-overhead, and replay benchmarks exist and pass the current budget.
- Zero steady-state event-path allocations are demonstrated.

A one-process or one-thread substitute does not satisfy this slice.

---

## 18. Initial work packages

Execute roughly in this order:

1. Add `README.md`, `AGENTS.md`, ADR template, contribution guide, and license-decision placeholder.
2. Add Nix flake, Bazel/Bzlmod skeleton, hermetic Clang toolchain, release/sanitizer configs, GoogleTest, Google Benchmark, and CI.
3. Add benchmark result schema, controlled runner policy, allocation counter, and regression comparison tool.
4. Add multi-process/multi-thread deterministic example programs and test harness.
5. Add Linux launcher, task discovery, pidfd-aware lifecycle, and cleanup.
6. Add x86-64 register abstraction and stop-reason model for every task.
7. Add bulk target-memory access.
8. Add virtual process/thread identity and strict scheduler skeleton.
9. Specify process/thread/scheduler-aware trace manifest and segment framing in ADRs and schemas.
10. Implement crash-safe append-only segment writer and reader.
11. Add trace inspection, corruption tests, and fuzzers.
12. Define syscall and synchronization policy interfaces.
13. Implement minimum process creation, exec, wait, thread creation, futex, time, randomness, pipe/socket, read/write, and exit handling for the vertical slice.
14. Implement reference recording and establish logical event equivalence tests.
15. Introduce the commercial buffered/interception path without changing event semantics.
16. Implement process-tree replay, virtual identity, mappings, and schedule replay.
17. Inject recorded outcomes and suppress repeated side effects.
18. Add event-boundary validation and divergence reports.
19. Complete the concurrent vertical-slice gate.
20. Add replay-from-start seeking and reverse stepping.
21. Add multi-task checkpoints and measure them.
22. Specify RCP and move CLI control across the process boundary.
23. Add Studio only after M5.
24. Add agent-facing tools only after deterministic analysis operations work without a model.

Parallel work is allowed for independent docs, tests, build, benchmarking, and security tooling. Later layers may not invent semantics absent from the deterministic core.

---

## 19. Test and benchmark strategy

### 19.1 Determinism corpus

Maintain small programs for:

- Process creation, exec, wait, and exit.
- Thread creation, join, TLS, and exit.
- Time and randomness.
- Files, offsets, pipes, socket pairs, and controlled sockets.
- Futexes, mutexes, condition variables, barriers, and atomics.
- Blocking and wakeup order.
- Signals at deterministic and asynchronous positions.
- Mapping, protection, unmapping, and remapping.
- Process identity and descriptor inheritance.
- C++ exceptions and unwinding.
- Use-after-free, double-free, out-of-bounds, and stale callbacks.
- Data races, lost wakeups, deadlocks, and atomic-order bugs.
- Optimized debug information and inline variables.
- Trace corruption and version mismatch.

Every supported feature needs success, error, interrupted, boundary, concurrency, and unsupported-path tests.

### 19.2 Core invariants

- Replaying the same trace reaches identical validated states.
- Every task transition is captured or rejected.
- Every kernel-written memory range is captured or rejected.
- Every mapping appears at the expected address or replay stops.
- Every signal reaches the expected virtual thread at the expected logical position.
- Every scheduler decision is reproduced or replay stops.
- Replay repeats no forbidden external side effect.
- Reverse then forward returns to identical state.
- Corrupt traces never produce silently trusted state.
- Debugger breakpoints do not alter stored logical memory.
- Reference and fast paths agree where both support the workload.

### 19.3 Fuzzing

Fuzz:

- Manifest and segment parsers.
- Varint, delta, and SIMD decoders.
- FlatBuffers validation boundaries.
- Protocol framing.
- Syscall, scheduler, and synchronization event deserialization.
- DWARF/symbol-worker boundaries.
- WebSocket binary messages.
- Agent tool-result validation.

### 19.4 Benchmarks

Maintain reproducible benchmarks for:

- Process/task discovery and lifecycle.
- Stop/continue and scheduler-switch latency.
- Register and target-memory transfer.
- Event descriptor production.
- Ring throughput, merge ordering, and backpressure.
- Scalar versus SIMD codecs and scans.
- Trace write, hash, and compression throughput.
- Trace bytes by workload.
- Recording slowdown and application tail latency.
- Replay speed and validation cost.
- Checkpoint creation, restore, and dirty-page volume.
- Last-write and condition search.
- Timeline tile queries.
- Gateway control latency under bulk streaming.
- Build time, incremental build time, and remote-cache effectiveness.

Use warmups, repeated samples, confidence intervals, CPU affinity, fixed governor where possible, and noise detection. Keep raw benchmark data.

---

## 20. CI and release policy

Initial CI includes:

- `nix flake check`.
- Bazel Clang dev build and tests.
- GCC compatibility build and tests.
- Release build.
- ASan/UBSan tests.
- TSan-compatible tests where tracer behavior permits.
- Formatting and static analysis.
- Trace/parser fuzz smoke tests.
- Multi-process/multi-thread integration tests.
- Deterministic replay repetition tests as soon as available.
- Bazel dependency and lockfile verification.
- SBOM and license-notice generation.
- Frontend typecheck, tests, and Bazel build when Studio exists.

Performance gating runs on controlled dedicated machines. Shared CI runners may produce advisory data but must not be used for precise regression decisions.

Nightly jobs add long fuzzing, repeated replay, benchmark matrices, kernel matrices, PGO corpus validation, trace compatibility, and large-trace tests.

Release artifacts include:

- Version and commit.
- Nix flake and Bazel module identities.
- Compiler, linker, and dependency identity.
- Supported host/target/kernel matrix.
- Trace and protocol versions.
- Known deterministic limitations.
- Security and sensitive-data notes.
- SBOM and required notices.
- Benchmark environment and results.
- Separate debug-symbol artifacts where appropriate.

Do not promise trace/protocol compatibility until migration and version policy exist.

---

## 21. Licensing and legal constraints

The repository has no license decision in this seed. The owner must choose the project license before accepting outside contributions or distributing releases.

The factory must:

- Track third-party licenses and notices.
- Prefer permissive, maintained dependencies with clear provenance.
- Evaluate commercial distribution obligations.
- Keep optional components separable when licenses differ.
- Avoid copying proprietary debugger implementations or non-redistributable code.
- Use public OS interfaces, standards, published research, and appropriately licensed code.
- Record source and license for imported tests and corpora.
- Produce an SBOM for releases.

---

## 22. Principal risks

| Risk | Required response |
|---|---|
| Silent replay divergence | Continuous validation and fail-closed behavior. |
| Single-thread architecture hidden in early code | Concurrent vertical slice and process/thread-aware schemas from M1. |
| Commercially unusable overhead | Performance budgets, fast path, dedicated benchmarks, and regression gates from M0. |
| Syscall and synchronization surface explosion | Explicit policies, generated coverage inventory, and precise unsupported diagnostics. |
| Signals and performance-counter skid | Logical clocks, calibration tests, and stepping near targets. |
| Multithread nondeterminism | Strict deterministic scheduler before parallel replay ambitions. |
| Address-space mismatch | Exact artifacts, mapping validation, and compatibility checks. |
| Trace size and write pressure | Compact events, batching, SIMD, adaptive compression, sparse indexes, and bounded queues. |
| Build/environment drift | Nix flakes, Bazel/Bzlmod locks, hermetic toolchains, and no hidden host dependencies. |
| Dependency bloat or license risk | Admission policy, boundaries, SBOM, and review. |
| Corrupt or malicious traces | Bounds checks, fuzzing, isolation, and quotas. |
| C++ semantic complexity | Pinned LLVM/LLDB/Clang behind an isolated worker. |
| LLM hallucination | Structured evidence and deterministic verification. |
| Prompt injection | Provenance, capability boundaries, and isolation. |
| Autonomous side effects | Sandboxes, explicit capabilities, and human approval. |
| Architectural overreach | Milestone gates and a concurrent first vertical slice. |

---

## 23. Success metrics

Core metrics:

- Replay success rate within the declared contract.
- Earliest-divergence localization accuracy.
- Process/thread topology and schedule reproduction rate.
- Recording slowdown by workload class.
- Application p99 latency impact.
- Trace size and write bandwidth.
- Replay and reverse-operation latency.
- Checkpoint memory and restore cost.
- Parser safety and fuzz coverage.
- Hot-path allocation and contention counts.
- Performance regression escape rate.

Debug-intelligence metrics:

- First-bad-state accuracy.
- Last-write/root-cause localization accuracy.
- Evidence precision.
- Hypothesis elimination rate.
- Counterfactual reproducibility.
- Passing/failing alignment quality.

Agent metrics:

- Percentage of factual claims carrying valid evidence.
- Confidence calibration.
- Time and cost to first useful evidence.
- Patch correctness and regression escape rate.
- Performance-regression detection for proposed patches.
- Safety-policy compliance.
- Replay CPU, branch count, model calls, and token budget per investigation.

---

## 24. Canonical terminology

- **Trace:** immutable recorded execution artifact.
- **Event:** ordered occurrence at the determinism boundary or trace metadata event.
- **Task:** a schedulable virtual process/thread execution context.
- **Execution point:** stable coordinate in a trace and branch.
- **Logical ticks:** deterministic progress counter.
- **Checkpoint:** restorable process-tree and thread state.
- **Replay session:** worker state navigating one trace/branch.
- **Branch:** counterfactual history derived from a parent execution point.
- **Intervention:** explicit branch change.
- **Finding:** evidence-backed investigation claim.
- **Hypothesis:** falsifiable claim not yet established.
- **Evidence reference:** typed link to trace state, analysis, experiment, build, benchmark, or test.
- **Invariant:** machine-checkable expected property.
- **RCP:** Replay Control Protocol.
- **Studio:** Orus web application.

Avoid calling the trace store a database in product language. Avoid using “cause” for temporal predecessors without supporting evidence.

---

## 25. Open decisions with binding defaults

The factory proceeds with:

- Product name: **Orus**.
- Host/target: Linux x86-64.
- Native language: C++23.
- Build: **Bazel with Bzlmod**.
- Environment/package reproducibility: **Nix flakes**.
- CMake: **prohibited**.
- Initial topology: **multiple processes and multiple threads** under strict deterministic scheduling.
- Core license: undecided; do not invent one.
- Symbol engine: pinned LLVM/LLDB/Clang in an isolated worker.
- Browser gateway: Boost.Asio + Boost.Beast.
- Browser client: TypeScript + React, built through Bazel.
- Binary boundary messages: FlatBuffers.
- Portable SIMD candidate: Google Highway.
- Compression: adaptive Zstandard/LZ4 based on benchmark and workload policy.
- Fast checksum: XXH3; content identity: BLAKE3 or approved equivalent.
- Catalog: SQLite; events remain custom block storage.
- Agent API: typed tools plus MCP facade.
- Model provider: pluggable.
- Repository publication by agents: human approval by default.

ADRs are required before commitment for:

- Exact kernel support matrix.
- Trace compatibility and migration policy.
- Packaging and ABI strategy for LLVM.
- Checkpoint dirty-page mechanism.
- Syscall interception and injected-runtime security model.
- Strict scheduling quantum and logical-clock mechanism.
- SIMD abstraction and supported ISA baseline.
- Adaptive compression policy.
- PGO/BOLT release process.
- Artifact retention and encryption/key management.
- Public project license.

---

## 26. Final instruction to the factory

Begin with M0 and the concurrent first vertical slice. Build the smallest system that can truthfully say:

> Orus used a reproducible Nix+Bazel toolchain to record a real multi-process, multi-thread native C++ execution, captured its nondeterministic inputs and deterministic schedule, replayed the complete process tree and threads repeatedly without repeating external side effects, verified execution at event boundaries, detected altered data or scheduling at the earliest relevant point, and met its published performance budget.

A single-process or single-thread demonstration does not satisfy this statement.

Do not defer performance architecture, process trees, threads, deterministic scheduling, or reproducible dependency management to a later rewrite. Once the statement above is demonstrably true, proceed to checkpoints, richer interfaces, Studio, causal analysis, model-driven investigation, counterfactual replay, and proof-carrying patches.