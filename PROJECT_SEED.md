# Orus Project Seed

**Status:** Authoritative project seed  
**Repository:** `schneiderlo/orus`  
**Seed date:** 2026-07-19  
**Primary audience:** autonomous software factory, maintainers, reviewers, and future contributors

> Read this document before creating code, issues, plans, or architecture. It is the initial source of truth for product intent and engineering constraints. When implementation discoveries require a change, record the decision in an Architecture Decision Record (ADR) and update this document deliberately. Do not silently reinterpret it.

---

## 0. Factory directive

Build **Orus**, a production-oriented deterministic record/replay and causal debugging platform for native C and C++ programs. Orus must let humans and software agents record a failing execution, replay it exactly, navigate backward through time, ask causal questions, run counterfactual experiments, and produce evidence-backed findings and fixes.

The product is not an LLM chat wrapper around GDB. The deterministic recording is the source of truth. Models may plan investigations and explain results, but all factual claims about an execution must be grounded in replayable machine evidence or explicitly marked as hypotheses.

The order of priorities is:

1. Correctness and deterministic replay.
2. Early divergence detection and diagnosability.
3. Safe isolation of untrusted programs and traces.
4. Performance of the recorder, replay engine, and trace store.
5. Stable typed interfaces.
6. Human debugger workflows.
7. Agentic causal investigation.
8. Autonomous patching and verification.

Do not build the polished web interface, autonomous patch generation, distributed tracing integrations, or production flight recorder before the deterministic core has passed its milestone gates.

---

## 1. Mission

Orus turns a program execution into an inspectable, forkable, and verifiable historical artifact.

Its north-star experience is:

> Record the run, find the first incorrect state, prove what caused it, test competing explanations against forked history, propose the smallest correction, and demonstrate that the correction works against the original failure.

A successful investigation should be able to produce a result such as:

```text
Claim
  A timeout callback used Session after its lifetime ended.

Evidence
  Session allocation                 event 8,419
  Callback registration              event 11,204
  Session destruction                event 18,992
  Stale callback execution           event 19,307
  Invalid memory access              event 19,319
  Observable failure                 event 19,412

Counterfactual
  Cancelling the callback at destruction prevents the invalid access
  and preserves the recorded externally visible output.

Verification
  Original failing trace             passes after the candidate patch
  Previously passing trace corpus    unchanged
  Relevant tests                     pass
  Residual uncertainty               documented
```

Every referenced event, value, stack frame, source location, and replay branch must be navigable in the debugger.

---

## 2. Product definition

Orus consists of six capabilities:

1. **Record** native execution while capturing every relevant nondeterministic input.
2. **Replay** the execution while preventing consultation of the live outside world.
3. **Navigate** forward and backward through source, instructions, threads, memory, registers, syscalls, signals, and process state.
4. **Explain** value origins, object lifetimes, control flow, synchronization, and the earliest meaningful divergence between runs.
5. **Experiment** by forking replay at a historical point and applying a controlled intervention.
6. **Verify** findings and candidate fixes through replay, invariants, tests, sanitizers, and performance checks.

The product surfaces are:

- `orus` command-line interface.
- Orus Studio web application.
- Debug Adapter Protocol (DAP) endpoint for IDE integration.
- GDB Remote Serial Protocol compatibility endpoint where useful.
- A typed native/internal Replay Control Protocol (RCP).
- Model Context Protocol (MCP) tools and resources for external coding agents.
- Optional OpenTelemetry ingestion for correlating native execution with requests, model calls, tools, and agent handoffs.

---

## 3. Non-negotiable principles

### 3.1 The recording is authoritative

The model, logs, source comments, stack traces, heuristics, and static analysis are not the source of truth. The recording and deterministic replay are authoritative for claims about what happened.

### 3.2 Fail closed

When Orus encounters an unsupported syscall, CPU instruction, mapping type, kernel behavior, trace version, or state transition that could affect determinism, it must stop with a precise diagnostic. It must never continue with an approximation that could yield a convincing but false replay.

### 3.3 Detect the earliest divergence

Replay must continuously validate its progress against the recording. A mismatch must stop at the earliest detectable event and report expected versus observed thread, program counter, logical clock, syscall, signal, arguments, mappings, and relevant state.

### 3.4 Evidence before explanation

Every agent-generated finding must include structured evidence references. Natural-language explanations are views over those references, not replacements for them.

### 3.5 Optimize the deterministic core, not the presentation layer

No JSON, WebSocket work, source parsing, model interaction, database query, or general-purpose logging is permitted in the recorder's event hot path. The core should use compact records, preallocated memory, bounded queues, sequential writes, and single-owner state.

### 3.6 Stable typed boundaries

Recorder, replay worker, symbol service, gateway, indexer, and agent service must communicate through versioned typed protocols. Do not expose internal C++ object layouts across process boundaries.

### 3.7 Isolate failure domains

A gateway crash must not corrupt a trace. A model timeout must not affect replay correctness. A malformed symbol file must not compromise the recorder. Each replay session should be owned by an isolated worker process.

### 3.8 Reproducible builds and traces

Build inputs, compiler identity, executable and shared-library build IDs, configuration, kernel compatibility metadata, trace schema, and relevant CPU features must be recorded or pinned.

### 3.9 Progressive scope

Start with a narrow, explicitly supported execution contract. Expand only after deterministic tests cover the previous contract.

### 3.10 Provider-independent AI

The agent layer must not depend on one model provider. Model adapters consume and produce a common investigation protocol. The debugger must remain useful with the entire AI layer disabled.

---

## 4. Supported scope

### 4.1 Initial platform contract

The first production path is deliberately narrow:

| Dimension | Initial decision |
|---|---|
| Host OS | Linux |
| Architecture | x86-64 |
| Target format | ELF |
| Target languages | C and C++ |
| C library | glibc first; musl after explicit validation |
| Program start | Launch under Orus; no attach |
| Process model | One process initially |
| Thread model | One thread initially |
| Debug information | DWARF, with exact binary and library retention |
| Replay environment | Same host or an identical pinned container/image |
| External effects | Recorded during record; suppressed or virtualized during replay |
| Privilege model | Unprivileged operation wherever possible |

### 4.2 Intended full product scope

The long-term product should support:

- Multiple processes across `fork`, `vfork`, `clone`, and `exec`.
- Multiple threads under a deterministic serialized scheduler.
- Precise asynchronous signal delivery.
- Futexes, blocking syscalls, condition variables, atomics, and thread-local storage.
- Dynamic libraries and controlled `dlopen` behavior.
- Incremental checkpoints and fast reverse search.
- Optimized C++ and complex DWARF location expressions.
- C++ exceptions, RTTI, templates, coroutines, inline frames, and sanitizers.
- Trace comparison, causal slicing, lifetime analysis, and concurrency analysis.
- Counterfactual replay branches.
- Source patching, rebuilding, replay-based verification, and test execution.
- Cross-layer correlation with distributed and agent telemetry.
- Continuous or ring-buffer recording for production incidents.

### 4.3 Explicit early non-goals

Do not claim support for these until they have their own design and deterministic test corpus:

- Attach to an already-running process.
- True parallel multicore deterministic replay.
- GPU or accelerator execution.
- Arbitrary device-backed mappings, DMA, or undocumented `ioctl` behavior.
- External shared memory modified by an unrecorded process.
- General `io_uring` support.
- Kernel-space or whole-system replay.
- Cross-architecture replay.
- Transparent replay across materially different CPUs or kernels.
- JIT runtimes without explicit code-generation capture and validation.

---

## 5. Canonical user experiences

### 5.1 CLI

The intended command vocabulary is:

```bash
orus record --output ./traces/failure.orus -- ./program arg1 arg2
orus replay ./traces/failure.orus
orus inspect ./traces/failure.orus
orus serve ./traces/failure.orus
orus compare passing.orus failing.orus
orus verify --trace failure.orus --invariant invariants/order.yaml
orus doctor
```

Additional commands may be added, but names and semantics should remain composable and scriptable. Human-readable output must have a stable machine-readable equivalent, preferably JSON only at the CLI boundary rather than inside the core.

### 5.2 Web application

Orus Studio is an evidence workspace, not merely a chat surface. It should combine:

- Source and disassembly.
- Timeline and thread lanes.
- Call stacks and variable trees.
- Registers and memory.
- Syscalls, signals, exceptions, and mappings.
- Hypotheses, evidence, contradictions, and findings.
- Replay branch tree and experiment results.
- Agent action log and human approval queue.
- Inferior terminal and debugger console.

Every agent claim displayed in the interface must be clickable and navigate to its evidence.

### 5.3 IDE integration

DAP should expose ordinary debugger operations, including backward operations where clients support them. Orus-specific causal search and branch operations should use documented extension requests rather than overloading unrelated DAP fields.

### 5.4 Agent integration

External agents should receive typed tools such as `find_last_write`, not an unconstrained shell that emits GDB commands. MCP is an interoperability facade; bulk trace data remains behind the optimized native interfaces.

---

## 6. System architecture

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

Tracee -- ptrace/seccomp/perf --> Recorder --> trace writer --> trace store

Symbol worker: pinned LLVM/LLDB/Clang, isolated behind a typed protocol.
```

### 6.1 Process boundaries

The default deployment should use separate processes for:

- Recorder.
- Replay worker, one authoritative owner per active replay session.
- Session supervisor.
- Symbol and C++ expression worker.
- Index and analysis workers.
- Web/API gateway.
- Agent investigation service.

The recorder and replay engine may share internal libraries, but they must not be linked into the web gateway.

### 6.2 Ownership rule

Each active replay session has exactly one authoritative replay worker. Other components send commands and receive immutable results. No two threads or services independently mutate the same replay state.

---

## 7. Deterministic execution model

### 7.1 Universal execution coordinate

All debugger state must be addressable by a stable coordinate:

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

The actual implementation must use fixed-width, serialized identifiers rather than C++ ABI-dependent layouts. `ExecutionPoint` is conceptual.

### 7.2 Event classes

The trace format should be able to represent at least:

- Process and thread lifecycle.
- Syscall entry, exit, result, `errno`, and kernel-written memory.
- Signal generation, delivery, disposition, and interruption/restart.
- Scheduling decisions and blocking/wakeup transitions.
- Address-space mapping changes.
- CPU nondeterministic instructions or emulated results.
- Debugger-independent checkpoints.
- External input and observable output.
- Application-defined markers and correlation IDs.
- Integrity and version metadata.

Do not record every normal instruction merely to make reverse execution easy. Re-execute deterministic instructions and record only what is needed to reproduce their effects, supplemented by checkpoints and optional control-flow indexes.

### 7.3 Determinism boundary

The recorder must capture every value that can alter future user-space execution. This includes:

- Syscall results and kernel-written buffers.
- Time, randomness, process identity, and environment observations.
- Signals and precise delivery position.
- Thread scheduling and synchronization outcomes.
- Address-space layout and mapping content where necessary.
- CPU instructions with nondeterministic or environment-dependent results.
- Relevant file-descriptor and virtual kernel-object state.
- Inputs from files, sockets, pipes, pseudo-terminals, and devices within the supported contract.

### 7.4 Syscall model

Every supported syscall must have an explicit replay policy:

1. **Emulate** from recorded data.
2. **Execute under control** and validate the result.
3. **Virtualize** against debugger-owned kernel objects.
4. **Reject** as unsupported.

A syscall implementation must describe:

- Inputs that affect determinism.
- Output memory ranges.
- File-descriptor effects.
- Blocking and wakeup behavior.
- Signal interactions.
- Mapping effects.
- Replay side-effect policy.
- Tests for success, error, interruption, and edge cases.

### 7.5 Signals

An asynchronous signal requires a precise logical position, not only an approximate event number. The implementation may initially single-step to prove correctness, then use deterministic hardware counters or instrumentation and single-step near the target to compensate for counter skid.

Debugger-generated traps must be distinguished from traps generated by the target.

### 7.6 Address space

Replay must reproduce mappings at the same virtual addresses, including executable, libraries, heap, stacks, thread-local storage, anonymous mappings, file-backed mappings, vDSO/vvar behavior, and generated code when supported.

The trace manifest must identify exact binaries and libraries through build IDs and content hashes. Retain the exact artifacts or mapped bytes needed for replay.

### 7.7 Virtual process identity

Use virtual PIDs and TIDs in the trace. Do not assume Linux will allocate identical numeric process identifiers during replay. Translate observable process identity consistently wherever the supported program can see it.

### 7.8 Scheduling

The first multithreaded implementation should run only one target thread at a time and record every scheduling decision. This is the strict deterministic mode.

A later concurrency-exploration mode may deliberately vary switch points to expose failures, but any discovered failure must be captured and replayed under the strict mode.

True parallel multicore deterministic replay is a distinct research and architecture tier and must not block a correct serialized implementation.

### 7.9 Checkpoints

Reverse execution restores an earlier checkpoint and deterministically replays forward. It does not attempt to invert arbitrary machine instructions.

A checkpoint must capture or reconstruct:

- All virtual thread register states.
- Signal masks and pending signals.
- Mapping topology, permissions, and relevant content.
- Dirty memory pages.
- File-descriptor and virtual kernel-object model.
- Blocking, futex, and scheduling state.
- Virtual PID/TID state.
- Replay logical time and trace cursor.

A simple `fork()` is not a complete multithreaded checkpoint. Incremental checkpoints should use a parent relationship, dirty-page bitmap, changed mappings, changed pages, and changed thread/kernel state. `userfaultfd` write protection may be evaluated, but it is not assumed to be the only mechanism.

### 7.10 Reverse operations

Required operations, in increasing order of sophistication:

- Reverse instruction step.
- Reverse continue to breakpoint.
- Previous source-line execution.
- Previous function call or return.
- Previous expression/value change.
- Last write to an address or object field.
- First event satisfying an invariant violation.
- First meaningful divergence between two runs.

The earliest implementation may replay from program start for every reverse operation. It will be slow but semantically valid. Add checkpoints only after replay correctness is established.

---

## 8. Agentic causal debugging

### 8.1 Agent role

The agent is a planner and interpreter. It may:

- Formulate falsifiable hypotheses.
- Choose deterministic analysis tools.
- Compare evidence.
- Request counterfactual experiments.
- Explain findings.
- Propose source changes.

It may not treat its own probability or prose as execution evidence.

### 8.2 Investigation state machine

Investigations should be persisted as structured state:

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

Persist tool calls, typed results, evidence accepted or rejected, concise rationale, confidence, and next action. Do not depend on hidden chain-of-thought text as product state.

### 8.3 Evidence strengths

Use explicit evidence levels:

| Level | Meaning |
|---|---|
| Observed | The event occurred before or near the failure. |
| Dependency-supported | Dynamic data or control dependencies connect it to the failure. |
| Concurrency-supported | Happens-before or conflicting-access evidence connects it. |
| Counterfactually validated | Changing the event or condition changes or prevents the failure. |
| Regression-verified | The correction succeeds across a defined trace and test corpus. |

Do not label temporal proximity as causation.

### 8.4 Typed investigation tools

The internal agent-facing API should eventually include:

```text
open_trace
summarize_trace
describe_failure
get_execution_context
seek
read_memory
evaluate_expression
get_stack
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

create_hypothesis
reject_hypothesis
create_finding
attach_evidence
export_reproduction
```

Tools return structured facts and evidence references, not only prose.

### 8.5 Semantic execution graph

The intelligence layer should expose an on-demand graph with node types such as execution event, instruction, source statement, function invocation, stack frame, variable, memory object, memory version, thread, lock, syscall, signal, exception, request, span, model call, tool call, agent handoff, invariant, hypothesis, experiment, patch, and test result.

Useful edges include:

```text
executed-before
control-depends-on
data-depends-on
read-from
wrote-to
allocated-by
freed-by
happens-before
synchronized-with
called-by
spawned-by
belongs-to-request
belongs-to-agent-run
violated-invariant
supports-hypothesis
contradicts-hypothesis
tested-by
fixed-by
```

Do not eagerly materialize a graph node for every event. Keep the compact event stream authoritative, build sparse indexes, materialize neighborhoods on demand, and cache derived slices by immutable trace identity and analysis version.

### 8.6 Counterfactual replay

A replay branch forks at an `ExecutionPoint` and records explicit interventions, for example:

- Override a syscall result or input buffer.
- Change a scheduling decision.
- Delay or advance a thread.
- Change signal-delivery position.
- Override a function return value.
- Modify memory.
- Enable an assertion.
- Replace a model/tool result.
- Apply an ABI-compatible code change or rebuilt executable.

Every branch must record parent branch, fork point, interventions, build identity, source revision, environment, and verification status.

### 8.7 Proof-carrying patches

An agent-generated patch is not complete without:

- Root-cause claim.
- Evidence references.
- Counterfactual result where applicable.
- Patch and affected source locations.
- Explanation of how the patch addresses the causal mechanism.
- Original failing traces replayed.
- Passing traces checked for regression.
- Tests, sanitizers, and performance checks executed.
- Known limitations and residual uncertainty.

Use Clang AST and LibTooling for source transformations where possible; do not rely on regex editing for C++ semantics.

---

## 9. Technology defaults

These are defaults, not excuses to couple the architecture to a library. Replace a default only through an ADR that compares correctness, performance, maintenance, license, and portability.

### 9.1 Core and build

- **Language:** C++23 for native components.
- **Build:** CMake with CMake Presets and Ninja.
- **Dependency management:** vcpkg manifest mode with a pinned baseline for ordinary third-party libraries. LLVM/LLDB may use a separately pinned distribution or reproducible source build because of its size and ABI sensitivity.
- **Compilers:** Clang is the primary developer/CI compiler; GCC is a compatibility build.
- **Standard library policy:** avoid implementation-specific assumptions unless isolated and tested.
- **Formatting/linting:** `clang-format`, `clang-tidy`, compiler warnings as errors in owned code.

### 9.2 Linux execution control

Use direct Linux APIs and narrow wrappers:

- `ptrace` for process execution and register control.
- `waitid`/`waitpid` for state transitions.
- `pidfd` APIs where they improve identity and lifecycle safety.
- `process_vm_readv` and `process_vm_writev` for bulk target memory transfer.
- `perf_event_open` for deterministic progress counters when validated.
- seccomp-BPF for selective syscall interception and policy enforcement.
- `userfaultfd` experiments for incremental checkpoint dirty-page tracking.
- `memfd`, `eventfd`, Unix-domain sockets, and `SCM_RIGHTS` for local IPC and large payloads.

Do not add a portability abstraction that hides Linux semantics from the deterministic core.

### 9.3 Debug semantics

- Pin an LLVM/LLDB/Clang version.
- Use LLVM object and DWARF facilities, LLDB debugger components, Clang expression parsing, and Clang LibTooling.
- Isolate the integration behind a symbol/expression worker process because internal plugin APIs may change and symbol inputs are untrusted.
- Maintain a GDB RSP adapter for compatibility where valuable, but do not make GDB textual commands the internal API.

### 9.4 Protocols and serialization

- **Internal typed messages:** FlatBuffers.
- **Internal transport:** Unix-domain sockets for commands, shared `memfd` regions for large immutable buffers, `eventfd` for wakeups.
- **IDE:** DAP.
- **GDB compatibility:** GDB Remote Serial Protocol.
- **Browser:** HTTP plus WebSocket.
- **Agent interoperability:** MCP.
- **Distributed telemetry:** OpenTelemetry/OTLP adapters.

### 9.5 Trace storage and indexing

- Custom append-only, block-oriented trace format.
- Zstandard for independently decodable compressed blocks.
- XXH3 for fast accidental-corruption checksums.
- A cryptographic hash for artifact identity and tamper-sensitive use cases.
- SQLite in WAL mode for catalogs, sessions, annotations, manifests, and index metadata; never one row per instruction/event.
- CRoaring for sparse event-ID sets and indexes.

### 9.6 Gateway

- Boost.Asio.
- Boost.Beast.
- Boost.URL.
- Boost.JSON for low-volume boundary messages only.
- OpenSSL for TLS.

Boost.Beast is the HTTP/WebSocket foundation, not the application architecture and never part of the recorder hot path.

### 9.7 Browser application

- TypeScript.
- React.
- Vite.
- pnpm with a committed lockfile.
- Monaco Editor for source/disassembly presentation.
- xterm.js for inferior I/O and command console.
- TanStack Virtual for large lists and trees.
- PixiJS or a measured custom Canvas/WebGL renderer for the timeline.
- Web Workers for binary decoding, decompression, indexing, and layout.

### 9.8 Testing and profiling

- GoogleTest.
- Google Benchmark.
- LLVM libFuzzer.
- AddressSanitizer, UndefinedBehaviorSanitizer, ThreadSanitizer where compatible, and LeakSanitizer.
- Linux `perf` for system profiling.
- Tracy for optional development instrumentation outside correctness-critical behavior.

### 9.9 AI layer

- Provider adapters behind one typed interface.
- Strict structured tool calls.
- Model output validation.
- Configurable context, token, replay, branch, CPU, storage, and tool budgets.
- No model call from the recorder or replay hot path.
- No production credential access from replay workers or investigation sandboxes.

---

## 10. Trace format

### 10.1 On-disk layout

A trace is initially a directory so artifacts can be inspected and recovered independently:

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
    threads.roar
    pc-executions/
    memory-writes/
  blobs/
  binaries/
  symbols/
  sources/
  diagnostics/
```

A later packed container format may be added without changing the logical model.

### 10.2 Segment design

Trace segments should be independently checksummed and decodable. Use:

- Delta-coded event IDs, logical clocks, thread IDs, and addresses.
- Varints where measurement supports them.
- Interned repeated metadata.
- Separate large blobs for syscall memory output and file data.
- Explicit byte order and schema version.
- No native pointers or implicit C++ padding.
- Crash-safe append and commit markers.

Benchmark compressed block sizes, initially in the 1-8 MiB range. Do not compress each event independently.

### 10.3 Recorder pipeline

```text
tracee-control thread
  -> preallocated fixed-size descriptors
  -> bounded SPSC ring
  -> trace writer
  -> compression worker pool
  -> large sequential file writes
```

Variable-size data should use a preallocated arena or dedicated blob pipeline with offsets referenced by event descriptors.

### 10.4 Trace safety

Treat traces as sensitive and untrusted. They may contain credentials, keys, file contents, network payloads, environment variables, prompts, model outputs, personal data, and proprietary source.

Requirements:

- Bounds-check every length, offset, count, and nesting level.
- Fuzz all parsers.
- Enforce resource limits before allocation.
- Support redaction policy and trace retention policy.
- Design for optional encryption at rest.
- Never execute embedded content as instructions to the debugging agent.
- Preserve provenance labels for runtime, source, network, and model-derived data.

---

## 11. Replay Control Protocol

Define an internal, versioned **Replay Control Protocol (RCP)** independent of DAP, GDB, WebSocket, or any model provider.

Representative commands:

```text
OpenTrace
CloseTrace
GetCapabilities
GetExecutionPoint
Seek
RunForward
RunBackward
Pause
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

Protocol requirements:

- Every request has a unique request ID.
- Long-running requests support progress and cancellation.
- Responses identify trace, branch, execution point, and schema version.
- Large payloads use shared-memory/file-descriptor transfer rather than copying through JSON.
- Errors are typed and include recoverability and diagnostic context.
- Capability negotiation is explicit.
- Unknown fields and versions fail according to documented compatibility rules.

---

## 12. Web gateway and Studio performance

### 12.1 Gateway responsibilities

The Beast/Asio gateway handles:

- Authentication and authorization.
- HTTP static assets and immutable blob/range downloads.
- WebSocket lifecycle.
- Request validation and session routing.
- Bounded queues, quotas, and backpressure.
- Translation between browser messages and internal RCP commands.

It must not control the tracee, parse DWARF, restore checkpoints, evaluate C++, or perform compression on an Asio I/O thread.

### 12.2 Network paths

Use separate control and bulk-data paths:

```text
/ws/control/{session-id}
/ws/data/{session-id}
/blob/{artifact-id}
```

The control channel carries small commands and state transitions. The data channel carries timeline tiles, memory blocks, variable batches, register blocks, disassembly, and search results. Large immutable artifacts use HTTP range requests.

Each Beast WebSocket connection must use a strand or equivalent serialized executor. Maintain one outstanding read and a serialized bounded write queue. Never allow a large data transfer to block a pause or cancel command on the same logical channel.

### 12.3 Backpressure

- Never discard command responses or authoritative state transitions.
- Coalesce superseded progress updates.
- Cancel obsolete viewport, memory, or disassembly requests.
- Drop replaceable timeline tiles when a newer request supersedes them.
- Disconnect clients that persistently exceed configured quotas.

### 12.4 Timeline

The browser never receives the full instruction stream. It requests viewport-specific tiles containing aggregation appropriate to zoom level:

- Far zoom: event density, thread activity, signals, checkpoints, requests.
- Medium zoom: calls, returns, syscalls, context switches, exceptions.
- Near zoom: source lines and instruction ranges.
- Maximum zoom: individual events/instructions.

Render large timelines with Canvas/WebGL, not one DOM element per event.

---

## 13. Performance requirements

Correctness gates all performance work. Once a milestone is correct, the factory must add benchmarks before optimizing it.

### 13.1 Hot-path rules

- No heap allocation per recorded event.
- No formatted logging in the event path.
- No JSON, FlatBuffers construction, database calls, compression, source parsing, or model work on the tracee-control thread.
- Avoid contended locks; prefer single-owner state and bounded SPSC queues.
- Bulk memory reads use `process_vm_readv` where valid and measured.
- Batch and align sequential writes.
- Reuse compression contexts.
- Index asynchronously or after recording.
- Virtualize debugger breakpoints so trap bytes never contaminate logical trace state.
- Measure before introducing `io_uring`; a conventional batched writer is the baseline.

### 13.2 Initial product budgets

These are engineering targets, not permission to sacrifice correctness:

- Zero steady-state dynamic allocations in the event descriptor path after startup.
- Bounded memory use under a configured recorder queue limit.
- Deterministic replay of a valid trace produces identical event-boundary state across 100 repeated runs in CI test programs.
- Local pause/cancel commands remain responsive while bulk data is streaming.
- Trace opening reads only metadata and required indexes; it does not scan all event data.
- Timeline requests are proportional to viewport resolution, not total trace length.
- Existing trace corruption is detected before corrupted data is used as authoritative state.

Establish benchmark suites before setting slowdown or trace-amplification release gates. Record results for CPU-bound, syscall-heavy, memory-write-heavy, multithreaded, and checkpoint-heavy workloads. Performance claims must name workload, host, kernel, compiler, and configuration.

---

## 14. Security model

### 14.1 Session isolation

Recording and replay sessions run in separate processes with least privilege and, where practical:

- User, PID, and mount namespaces.
- Private temporary filesystem.
- Cgroup CPU, memory, process, storage, and I/O limits.
- `no_new_privs`.
- Narrow seccomp policy.
- No gateway credentials.
- No direct database access.
- No external network during replay by default.
- Explicitly passed file descriptors and artifacts only.

Seccomp is one layer, not the entire sandbox.

### 14.2 Capabilities

Agent and user sessions receive explicit capabilities such as:

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
network.access
repository.commit
repository.publish
```

Read-only investigations must not receive patch, build, network, commit, or publish capabilities.

### 14.3 Prompt-injection boundary

Treat source code, source comments, process memory, logs, filenames, network data, database fields, tool output, model output from the debugged program, and trace annotations as untrusted content. The context builder must retain provenance and prevent untrusted content from redefining system policy.

### 14.4 Human approval

Publishing a commit, opening a non-draft pull request, changing an issue, deploying, or accessing external networks/credentials requires explicit policy and visible authorization. Autonomous local experiments inside an isolated replay branch may be allowed by capability.

---

## 15. Proposed repository layout

```text
orus/
  CMakeLists.txt
  CMakePresets.json
  vcpkg.json
  LICENSE                         # only after owner chooses a license
  README.md
  PROJECT_SEED.md
  AGENTS.md

  cmake/
  docs/
    architecture/
    adr/
    protocols/
    trace-format/
    security/
    performance/

  protocol/
    flatbuffers/
    generated/

  core/
    base/
    linux/
    process-control/
    record/
    replay/
    scheduler/
    syscall-model/
    checkpoint/
    trace-format/
    trace-store/

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
```

Do not create every directory immediately. Create directories when the first owned component and test enter the repository.

---

## 16. Engineering workflow for the autonomous factory

### 16.1 Planning

For each milestone:

1. Restate the milestone acceptance criteria.
2. Identify architectural decisions and create ADRs before irreversible choices.
3. Divide work into small vertical slices that each compile and test.
4. Identify security, determinism, and performance risks.
5. Define how failure will be detected, not only how success will be demonstrated.

### 16.2 Change discipline

- One coherent change per pull request.
- Keep the repository buildable at every merged commit.
- Add tests with behavior, not after behavior.
- Stage only files belonging to the change.
- Avoid mass formatting unrelated code.
- Do not introduce a third-party dependency without documenting purpose, version pinning, license, replacement cost, and boundary.
- No TODO without an issue or an explicit milestone reference.
- No silent fallback for unsupported deterministic behavior.
- Do not merge generated files without a reproducible generation command.

### 16.3 ADRs

Use `docs/adr/NNNN-title.md` with:

```text
Status
Context
Decision
Alternatives considered
Consequences
Correctness impact
Performance impact
Security impact
Compatibility/migration
Validation plan
```

### 16.4 Definitions of done

Every implementation change must have, as applicable:

- Unit tests.
- Integration or deterministic replay test.
- Negative/error-path test.
- Documentation of externally visible behavior.
- Trace/protocol compatibility consideration.
- Benchmark when touching a hot path.
- Fuzz target when parsing untrusted structured input.
- Sanitizer-clean execution for the relevant test.
- No unexplained warnings.
- A clear rollback or compatibility story.

### 16.5 Canonical developer commands

The bootstrap milestone should make these commands work:

```bash
cmake --preset dev
cmake --build --preset dev -j
ctest --preset dev

cmake --preset asan
cmake --build --preset asan -j
ctest --preset asan

cmake --preset release
cmake --build --preset release -j

pnpm --dir web install --frozen-lockfile
pnpm --dir web test
pnpm --dir web build
```

Presets may evolve, but CI and contributor documentation must use the same commands.

---

## 17. Milestones and gates

### M0 - Repository and quality bootstrap

Deliver:

- CMake/C++ project skeleton.
- `orus --version` and `orus doctor` placeholders with real build metadata.
- Formatting, linting, unit-test, sanitizer, and release presets.
- CI for Clang and GCC on Linux x86-64.
- Dependency manifest and pinning policy.
- ADR template, contribution guidance, and `AGENTS.md` derived from this seed.
- Minimal example target programs.

Gate:

- Clean clone builds and tests through documented commands.
- CI is reproducible and contains no hidden local steps.
- No deterministic execution claims yet.

### M1 - Conventional Linux process-control core

Deliver:

- Launch a child under tracing.
- Observe exec, stops, exits, and signals.
- Read/write registers through a typed architecture abstraction.
- Read/write target memory, using bulk APIs where possible.
- Continue and single-step.
- Precise stop reason model.
- Minimal CLI diagnostic commands.

Gate:

- Tests cover normal exit, crash, signal, exec failure, and tracer cleanup.
- Debugger-generated versus target-generated traps are distinguishable.
- No recording yet.

### M2 - Single-thread recording

Deliver:

- Versioned trace manifest and event segments.
- Explicit syscall model framework.
- Initial syscall support required by the vertical-slice program.
- Capture syscall return values and kernel-written memory.
- Capture exact binary/library identity and mappings.
- Crash-safe trace writer and trace inspector.

Gate:

- Unsupported syscalls fail closed with a useful diagnostic.
- Truncated/corrupt segments fail safely.
- Parser fuzz target exists.

### M3 - Deterministic replay and divergence detection

Deliver:

- Replay from process start.
- Prevent replay from repeating external side effects.
- Inject or validate recorded syscall outcomes.
- Reproduce mappings and process identity within the supported scope.
- Event-boundary register/state validation.
- Earliest-divergence report.

Gate:

- The first vertical slice replays identically 100 times.
- Deliberately mutated traces or binaries cause deterministic, localized failure.
- Replay performs no external writes in the test sandbox.

### M4 - Reverse execution

Deliver:

- Seek by event.
- Reverse instruction step by restart/replay.
- Reverse continue to an address or condition.
- Initial checkpoint abstraction.
- Historical register and memory inspection.

Gate:

- Reverse-step followed by forward-step returns to the original state for the deterministic corpus.
- Checkpoint use does not alter semantics.

### M5 - Stable debugger interfaces

Deliver:

- RCP schema and local IPC.
- CLI client using RCP rather than internal object access.
- Initial DAP adapter.
- Optional minimal GDB RSP adapter.
- Cancellation, progress, typed errors, and capability negotiation.

Gate:

- Replay worker can be killed without corrupting the trace.
- Two independent clients cannot concurrently mutate one session.
- Protocol compatibility tests exist.

### M6 - Orus Studio vertical slice

Deliver:

- Beast/Asio gateway.
- Session creation and trace opening.
- Source/disassembly view.
- Threads, stack, registers, memory, and basic timeline.
- Forward/reverse controls.
- Bounded WebSocket queues and cancellation.

Gate:

- Large timeline requests do not block pause/cancel.
- Browser receives viewport tiles, not the full event stream.
- Gateway has no direct tracee control or DWARF parsing.

### M7 - Deterministic multithreading

Deliver:

- Virtual threads and serialized scheduler.
- `clone`/thread exit.
- Futex and blocking/wakeup model for an explicitly defined subset.
- Precise per-thread signal behavior.
- Thread schedule in the trace.

Gate:

- Multithread test corpus replays identically across repeated runs.
- Schedule mutation is detected.
- Unsupported synchronization paths fail closed.

### M8 - Causal analysis

Deliver:

- Memory object/lifetime model.
- Last-write search.
- First-bad-state search.
- Passing/failing run alignment and earliest semantic divergence.
- Dynamic backward slicing for a bounded supported subset.
- Happens-before and conflicting-access analysis.
- Structured findings and evidence references.

Gate:

- Known-bug corpus returns the correct responsible event and source region.
- At least one plausible incorrect hypothesis can be rejected by evidence.

### M9 - Read-only debugging agent

Deliver:

- Provider-independent investigation service.
- Typed tools over RCP and analysis services.
- Hypothesis/evidence state machine.
- MCP server.
- Web investigation panel with clickable claims.
- Prompt-injection provenance controls and budgets.

Gate:

- Agent cannot mutate source, replay state, repository, or network in read-only mode.
- Every factual claim in the benchmark report cites structured evidence.
- Disabling the model leaves all deterministic debugger functions operational.

### M10 - Counterfactual branches and patch verification

Deliver:

- Replay branch model and provenance.
- Controlled memory/input/schedule interventions.
- Branch comparison.
- Clang-based patch workspace.
- Rebuild and replay against original traces.
- Test/sanitizer/performance verification bundle.
- Human approval gates for repository publishing.

Gate:

- Counterfactual experiments are reproducible.
- A patch is never marked verified only because a crash disappeared.
- Evidence, test results, residual risk, and exact build identity accompany the patch.

### M11 - Production recording and cross-layer causality

Deliver only after the previous gates:

- Ring-buffer/flight-recorder mode.
- Storage quotas and retention.
- Incident packaging.
- OpenTelemetry and agent-span correlation.
- Automated failure clustering and organizational incident memory.
- Policy-controlled autonomous investigation.

Gate:

- Production recorder has explicit overhead and storage measurements.
- Sensitive-data controls are enabled by default.
- Incident memory revalidates old patterns against new traces rather than trusting old summaries.

---

## 18. First vertical slice: implement this first

The first end-to-end target is deliberately small and must precede the web UI.

### 18.1 Example program

Create a tiny single-thread C++ program that:

1. Calls `clock_gettime`.
2. Calls `getrandom` for a small buffer.
3. Performs deterministic CPU work over the returned values.
4. Writes a deterministic formatted result to stdout.
5. Exits with a known status.

Avoid hidden dependencies that expand the syscall surface unnecessarily. Provide a static or minimal-runtime variant if helpful for early tests.

### 18.2 Required commands

```bash
orus record --output test.orus -- ./orus-example-nondeterminism
orus inspect test.orus
orus replay test.orus
```

### 18.3 Acceptance criteria

- Recording completes and creates a versioned, inspectable trace directory.
- The trace contains the exact executable identity and the nondeterministic results.
- Replay reaches the same supported event sequence and final exit status.
- The replayed program observes the recorded time and random bytes, not current values.
- Replay does not duplicate the original stdout side effect; the CLI may display recorded output separately.
- Event-boundary register hashes or equivalent validated state match on 100 repeated replays.
- Changing one recorded byte causes the earliest relevant divergence or checksum error.
- Replacing the executable with a different build fails before authoritative replay.
- Truncating a segment fails safely.
- The core event path has a benchmark and allocation count.

This slice establishes the architecture. Do not add broad syscall support before this path is reliable and tested.

---

## 19. Initial work packages

The factory should create and execute work packages in roughly this order:

1. Add `README.md`, `AGENTS.md`, ADR template, contribution guide, and license decision placeholder.
2. Add CMake presets, vcpkg manifest, CLI skeleton, GoogleTest, and CI.
3. Add Linux process launcher and tracer lifecycle abstraction.
4. Add x86-64 register file abstraction and stop-reason model.
5. Add target bulk memory access abstraction.
6. Add deterministic example programs and a test harness that captures host/kernel metadata.
7. Specify trace manifest and segment framing in an ADR and FlatBuffers schema where appropriate.
8. Implement crash-safe append-only segment writer and reader.
9. Add trace inspection and corruption tests/fuzzer.
10. Define syscall policy interface and implement the minimum process-start syscalls.
11. Add `clock_gettime`, `getrandom`, `write`, and exit-path handling for the vertical slice.
12. Implement replay process launch and address-space validation.
13. Inject recorded syscall outputs and suppress repeated external effects.
14. Add event-boundary validation and divergence reports.
15. Complete the first vertical-slice gate.
16. Add replay-from-start seeking and reverse stepping.
17. Specify RCP and move CLI control across the process boundary.
18. Add checkpoints only after replay-from-start behavior is stable.
19. Add the minimal Beast gateway and Studio only after M5.
20. Add agent-facing tools only after deterministic analysis operations exist without a model.

Parallel work is allowed for independent documentation, tests, and tooling, but no later layer may invent semantics that the deterministic core does not provide.

---

## 20. Test strategy

### 20.1 Determinism corpus

Maintain small programs for:

- Time and randomness.
- File reads and offsets.
- Pipes and sockets inside a controlled harness.
- Signals at deterministic and asynchronous positions.
- Mapping creation, protection, unmapping, and remapping.
- Process identity.
- C++ exceptions and unwinding.
- Thread creation, futexes, condition variables, and atomics.
- Use-after-free, double-free, out-of-bounds, and stale callbacks.
- Data races, lost wakeups, deadlocks, and atomic-order bugs.
- Optimized debug information and inline variables.
- Trace corruption and version mismatch.

Each supported feature needs positive, error, interrupted, boundary, and unsupported-path tests.

### 20.2 Core invariants

- Replaying the same trace repeatedly reaches identical validated states.
- Every kernel-written memory range is captured or rejected.
- Every mapping appears at the expected address or replay stops.
- Every signal reaches the expected virtual thread at the expected logical position.
- Replay repeats no forbidden external side effect.
- Reverse then forward returns to the original state.
- A corrupted trace never produces silently trusted state.
- Debugger breakpoints do not alter stored logical memory.

### 20.3 Fuzzing

Fuzz:

- Trace manifest and segment parser.
- Varint/delta decoders.
- FlatBuffers validation boundaries.
- Protocol frame parser.
- Syscall event deserialization.
- DWARF/symbol-worker request boundary.
- WebSocket binary message decoder.
- Agent tool-result validation.

### 20.4 Benchmarks

Maintain reproducible benchmarks for:

- Process-control stop/continue latency.
- Register and target-memory transfer.
- Event descriptor production.
- Ring throughput and backpressure.
- Trace write/compress throughput.
- Trace bytes per workload.
- Replay speed.
- Checkpoint creation, restore, and dirty-page volume.
- Last-write and condition search.
- Timeline tile query.
- Gateway control latency under bulk streaming.

---

## 21. CI and release policy

Initial CI should include:

- Clang debug build and tests.
- GCC debug build and tests.
- ASan/UBSan build and tests.
- Formatting and static analysis.
- Trace parser fuzz smoke tests.
- Release build.
- Frontend typecheck, unit tests, and build when the web project exists.

Later CI should add pinned kernel/container matrices and deterministic replay tests. Nightly jobs may run long fuzzing, repeated replay, benchmarks, TSan-compatible tests, and trace compatibility suites.

Release artifacts must include:

- Version and commit.
- Compiler and dependency identity.
- Supported host/target matrix.
- Trace/protocol versions.
- Known deterministic limitations.
- Security and sensitive-data notes.
- Benchmark environment and results when performance is claimed.

No trace format or protocol compatibility promise should be made until migration and version policy are implemented.

---

## 22. Observability

Orus must be observable without polluting its hot path.

Use:

- Lock-free or per-thread counters in hot components.
- Deferred binary diagnostics.
- Structured logs outside the tracee-control path.
- Trace and session IDs in every service event.
- Metrics for queue depth, dropped/coalesced replaceable messages, compression ratio, checkpoint cost, replay divergence, and tool budgets.
- Optional OpenTelemetry export from services, never as a determinism dependency.

Do not log target secrets by default. Diagnostic bundles need explicit redaction and export controls.

---

## 23. Licensing and legal constraints

The repository currently has no license decision in this seed. The owner must choose the project license before accepting outside contributions or distributing releases.

The factory must:

- Track third-party licenses and notices.
- Prefer permissive, maintained dependencies with clear provenance.
- Keep optional components separable when licenses differ.
- Avoid copying proprietary debugger implementations or non-redistributable code.
- Use public operating-system interfaces, standards, published research, and appropriately licensed open-source code.
- Record source and license for imported test cases or corpora.

---

## 24. Principal risks

| Risk | Required response |
|---|---|
| Silent replay divergence | Continuous validation and fail-closed behavior. |
| Syscall surface explosion | Explicit syscall policies and narrow milestone contracts. |
| Signals and performance-counter skid | Logical clocks, calibration tests, and single-step near targets. |
| Multithread nondeterminism | Serialized strict scheduler before multicore ambitions. |
| Address-space mismatch | Exact artifacts, mapping validation, and compatibility checks. |
| Trace size and recorder overhead | Compact events, batching, compression, sparse indexes, benchmarks. |
| Corrupt or malicious traces | Bounds checks, fuzzing, isolation, quotas, typed validation. |
| C++ semantic complexity | Pinned LLVM/LLDB/Clang behind an isolated worker. |
| LLM hallucination | Structured evidence and deterministic verification. |
| Prompt injection from runtime data | Provenance labels, capability boundaries, and content isolation. |
| Autonomous side effects | Sandboxes, explicit capabilities, and human publication approval. |
| Architectural overreach | Milestone gates and first vertical slice. |

---

## 25. Success metrics

Measure the product by investigation outcomes, not the eloquence of explanations.

Core metrics:

- Replay success rate within the declared support contract.
- Earliest-divergence localization accuracy.
- Recording slowdown by workload class.
- Trace size by workload class.
- Replay and reverse-operation latency.
- Checkpoint memory and restore cost.
- Trace parser safety and fuzz coverage.

Debug-intelligence metrics:

- First-bad-state accuracy.
- Last-write/root-cause localization accuracy.
- Evidence precision.
- Hypothesis elimination rate.
- Counterfactual reproducibility.
- Passing/failing run alignment quality.

Agent metrics:

- Percentage of factual claims carrying valid evidence.
- Confidence calibration.
- Time and cost to first useful evidence.
- Patch correctness and regression escape rate.
- Safety-policy compliance.
- Replay CPU, branch count, model calls, and token budget per investigation.

---

## 26. Canonical terminology

Use these terms consistently:

- **Trace:** immutable recorded execution artifact.
- **Event:** ordered recorded occurrence at the determinism boundary or a trace metadata event.
- **Execution point:** stable coordinate in a trace and branch.
- **Logical ticks:** deterministic progress counter used to locate events precisely.
- **Checkpoint:** restorable execution state used to accelerate replay.
- **Replay session:** live worker state navigating one trace/branch.
- **Branch:** counterfactual history derived from a parent execution point.
- **Intervention:** explicit change applied to a replay branch.
- **Finding:** evidence-backed claim produced by an investigation.
- **Hypothesis:** falsifiable claim not yet established as a finding.
- **Evidence reference:** typed link to trace state, analysis result, experiment, build, or test.
- **Invariant:** machine-checkable expected property.
- **RCP:** internal Replay Control Protocol.
- **Studio:** Orus web application.

Avoid calling the trace store a database in product language. Avoid using “cause” for mere temporal predecessors.

---

## 27. Open decisions with safe defaults

The factory may proceed with these defaults while documenting decisions:

- Product name: **Orus**.
- Initial host/target: Linux x86-64.
- Native language: C++23.
- Build: CMake + Ninja + vcpkg manifest.
- Core license: undecided; do not invent one.
- Symbol engine: pinned LLVM/LLDB/Clang in an isolated worker.
- Browser gateway: Boost.Asio + Boost.Beast.
- Browser client: TypeScript + React + Vite.
- Binary messages: FlatBuffers.
- Compression: Zstandard.
- Catalog: SQLite; event data remains custom block storage.
- Agent API: typed internal tools plus MCP facade.
- Model provider: pluggable.
- Repository publication by agents: human approval required by default.

Questions that require an ADR before implementation commitment include:

- Exact trace compatibility and migration policy.
- Linux kernel/version support matrix.
- Packaging strategy for pinned LLVM.
- Checkpoint dirty-page mechanism.
- Syscall interception transition from correctness-first tracing to optimized buffering.
- Source/binary artifact retention policy.
- Encryption and enterprise key-management design.
- Public project license.

---

## 28. Final instruction to the factory

Begin with M0 and the first vertical slice. Build the smallest system that can truthfully say:

> Orus recorded nondeterministic inputs from a native C++ program, replayed the same execution repeatedly without repeating external side effects, verified the execution at event boundaries, and stopped precisely when the trace or executable was altered.

Only after that statement is demonstrably true should the project add checkpoints, richer debugger protocols, the web application, multithreading, causal analysis, model-driven investigation, or autonomous patching.

The durable product advantage will not be a chat interface. It will be high-fidelity recordings, fast temporal and causal indexes, forkable replay, proof-carrying findings, and a disciplined verification system that humans and agents can trust.
