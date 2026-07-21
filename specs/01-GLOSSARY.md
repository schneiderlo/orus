# Orus Glossary

**Status:** Ready  
**Scope:** Foundation terminology for M0 and the M1-M11 roadmap  
**Last updated:** 2026-07-21

Terms in specifications, code, tests, diagnostics, and product UI must use the
meanings below. A domain specification may introduce a narrower term, but it
must not redefine one of these terms without an accepted superseding decision.

## A

**Acceptance criterion**  
A binary or measurable condition that must hold for a requirement to be
accepted. It states observable behavior; its verification method states how
evidence is obtained.

**Active**  
A domain-spec status meaning its accepted requirements are currently being
implemented or operated. Active does not mean the milestone gate has passed.

**ADR (Architecture Decision Record)**  
A durable record of a significant engineering choice, its context,
alternatives, consequences, validation, and rollback or migration plan.

**Advisory result**  
A measurement retained for information that cannot by itself pass or fail a
performance gate. Performance data from shared GitHub-hosted runners is
advisory.

**Agent**  
Optional software that plans an investigation and interprets structured
results. An agent is not an authority for facts about an execution.

**Andon**  
A stop condition used by the factory when continuing would be unsafe or the
required result cannot be produced. It is not a synonym for an ordinary open
question.

**Artifact identity**  
A stable identification of an executable, library, trace component, or build
output using recorded build metadata and a content hash. It is distinct from a
filename.

**ASan (AddressSanitizer)**  
A compiler instrumentation mode that detects classes of memory-safety defects.

## B

**Backpressure**  
A bounded-flow mechanism by which a slower consumer makes a producer pause,
batch, reject, or shed work according to policy instead of allowing unbounded
memory growth.

**Baseline**  
Versioned raw measurements plus provenance against which later performance
measurements are compared. A baseline is authoritative only when both runs
satisfy the controlled-runner contract and are comparable.

**Bazel**  
The build system that owns Orus source compilation, linking, code generation,
tests, benchmarks, web builds, and release binaries.

**BLAKE3**  
A candidate cryptographic hash for artifact/content identity, subject to the
dependency-admission and benchmark process.

**Blocking result**  
Evidence whose failure prevents a requirement or milestone from passing.

**Branch**  
A counterfactual execution history derived from a parent execution point and a
recorded set of interventions. A source-control branch is called a Git branch
when ambiguity is possible.

**Build ID**  
An identifier embedded in or associated with an ELF binary or shared library.
Orus records it together with stronger content identity where required.

**Bzlmod**  
Bazel's module-based dependency system. It is Orus's required Bazel dependency
mechanism.

## C

**Capability**  
An explicitly granted permission or supported protocol operation. Security
capabilities (for example, `memory.read`) and negotiated protocol capabilities
must be identified by context.

**Catalog**  
Bounded trace metadata stored separately from the event stream. SQLite may be
used for the catalog; product language must not call the entire trace store a
database.

**Checkpoint**  
A restorable snapshot or delta chain sufficient to reconstruct the registers,
memory, mappings, process/thread relationships, signals, file descriptors,
virtual kernel objects, scheduler state, logical time, and trace cursor for the
complete supported process tree.

**CI (Continuous Integration)**  
Automated build, test, analysis, packaging, and advisory benchmark workflows
run for repository changes.

**CMake**  
A build-system family that is prohibited from Orus developer, CI, generated
metadata, and source build paths unless a later accepted ADR defines an
isolated exception that still keeps CMake out of those paths.

**Commercial fast path**  
The measured, shipping-oriented record/replay path that may use batching,
selective interception, runtime cooperation, shared-memory rings, and compact
encoding while producing the same logical events as the overlap reference
path.

**Compatibility claim**  
A documented promise that a host, trace version, protocol version, artifact, or
client is supported. Observing that something happens to work is not a
compatibility claim.

**Controlled runner**  
A performance host operated under the pinned CPU, affinity, kernel, governor,
storage, toolchain, workload, repetition, noise, and provenance rules in the
performance domain spec. Only its conforming measurements can be authoritative
for precise regression decisions.

**Counterfactual**  
A replay branch that changes one or more recorded conditions to test whether a
claimed causal mechanism changes the outcome.

**CRoaring**  
A candidate compressed-bitmap library for sparse event sets, subject to
dependency admission and benchmarks.

## D

**DAP (Debug Adapter Protocol)**  
A client-facing debugger protocol to be implemented as an adapter over Orus's
internal Replay Control Protocol, not as the core internal API.

**Deprecated**  
A domain-spec status meaning the specification remains available for migration
or history but must not govern new implementation.

**Determinism boundary**  
The boundary across which every value or transition capable of changing future
user-space execution must be captured, controlled, virtualized, validated, or
rejected.

**Deterministic replay**  
Re-execution from a trace that reproduces declared task topology, scheduling,
nondeterministic inputs, and validated state without consulting the live
outside world or repeating forbidden external effects.

**DCO (Developer Certificate of Origin)**  
A contribution sign-off mechanism. Orus does not adopt a DCO during M0 because
outside contributions are closed.

**Domain spec**  
An implementation-ready specification for one bounded product or engineering
domain. It contains testable requirements, measurable non-functional targets,
contracts, data, flows, failures, observability, and a verification plan.

**Draft**  
A domain-spec status meaning content is incomplete or not yet accepted for
implementation.

**DWARF**  
The debug-information format used for native symbol, type, source, and variable
information in the initial product path.

## E

**ELF (Executable and Linkable Format)**  
The native executable and shared-object format in Orus's initial Linux target
contract.

**Event**  
An ordered occurrence at the determinism boundary or a trace metadata
occurrence. A normal instruction is not automatically an event.

**Event descriptor**  
A compact in-memory representation passed through the recorder's hot pipeline.
It is not a FlatBuffers wire object and must not require a steady-state heap
allocation after startup.

**Event ID**  
A trace-stable ordered identifier for an event. It is not a host timestamp or
native pointer.

**Evidence reference**  
A typed, navigable link to trace state, analysis, experiment, source/build,
benchmark, or test evidence. Prose alone is not an evidence reference.

**Execution point**  
A stable coordinate consisting conceptually of trace, branch, event, logical
ticks, virtual process, virtual thread, program counter, and optional external
correlation identities.

**External effect**  
An interaction observable outside the replay worker, such as a file write,
network transmission, terminal output, or process creation. Replay must
suppress, emulate, virtualize, or explicitly control such effects.

## F

**Fail closed**  
Stop before an authoritative success claim when unsupported or inconsistent
behavior could affect determinism, and emit a precise diagnostic. Continuing
with an approximation is not fail-closed behavior.

**Finding**  
A structured, evidence-backed investigation claim with stated confidence,
scope, and residual uncertainty.

**First bad state**  
The earliest state in an execution that violates a specified invariant, not
merely the state nearest an observed failure.

**First meaningful divergence**  
The earliest semantically relevant difference between comparable executions,
reported with the process/thread/schedule and expected-versus-observed context.

**FlatBuffers**  
The candidate versioned binary serialization for process boundaries. A
FlatBuffers object is a boundary representation, not a hot-path event object or
native ABI contract.

**Fuzzing**  
Automated generation and mutation of inputs to discover crashes, resource
violations, and semantic failures. All untrusted structured parsers require an
applicable fuzz target.

**Futex (fast userspace mutex)**  
A Linux synchronization primitive used by pthread implementations. Orus must
declare the exact supported futex operations and reject unsupported
determinism-affecting paths.

## G-H

**Gate**  
The complete set of blocking criteria and evidence required to declare a
milestone passed.

**GCC (GNU Compiler Collection)**  
The secondary compiler used for M0 compatibility builds. Clang is the primary
compiler.

**GDB RSP (GNU Debugger Remote Serial Protocol)**  
An optional compatibility interface layered over Orus's internal protocol.
Textual GDB commands are never Orus's internal API.

**GitHub Actions**  
The M0 CI service for blocking functional/quality checks and advisory
performance measurements. A shared GitHub-hosted runner is not a controlled
performance runner.

**glibc (GNU C Library)**  
The first C library in the initial target contract. musl requires separate
validation before a support claim.

**Happens-before**  
A partial order derived from program order and synchronization that is used to
reason about visibility and concurrency; temporal proximity alone does not
establish it.

**Hermetic build**  
A build whose declared, pinned inputs fully determine its actions and that does
not rely on undeclared host files, tools, libraries, environment state, or
network access.

**Hot path**  
Latency- or throughput-critical target-control, event production, replay
control, or equivalent work whose cost occurs frequently enough to determine
product performance.

**Hypothesis**  
A falsifiable investigation claim that has not yet reached the evidence
strength required to be a finding.

## I-L

**IPC (Interprocess Communication)**  
Communication between separate operating-system processes. Orus core services
use versioned typed IPC rather than sharing native C++ object layouts.

**ISA (Instruction Set Architecture)**  
The processor instruction contract. Orus initially targets a compatible
x86-64 baseline and may dispatch to optional measured extensions at runtime.

**Invariant**  
A machine-checkable property expected to hold at specified execution points or
across a replay/branch.

**Intervention**  
An explicit, recorded change applied at a branch fork point, such as replacing
an input, schedule choice, signal position, function result, or memory value.

**JIT (Just-In-Time compilation)**  
Runtime generation of executable code. JIT targets are unsupported until code
generation and identity can be captured and validated.

**LLD**  
LLVM's linker and the default M0 release linker.

**LLDB**  
LLVM's debugger libraries and components, planned behind an isolated symbol and
expression-worker boundary.

**Logical ticks**  
A deterministic progress counter used to identify execution position
independently of wall-clock time.

**LZ4**  
A candidate low-latency trace compression algorithm, selected only when an
accepted adaptive policy and representative benchmarks prefer it.

## M-N

**M0-M11**  
Sequential Orus product milestones defined in `00-CHARTER.md`. M0 is the only
milestone in the current delivery wave.

**MCP (Model Context Protocol)**  
A future external facade exposing typed investigation tools/resources to coding
agents. It does not grant authority beyond the underlying Orus capabilities.

**memfd**  
A Linux anonymous file-descriptor mechanism planned for transferring large
immutable buffers between local processes.

**Milestone**  
A coherent product capability increment with explicit deliverables and a
blocking gate. Completion of tasks alone does not pass a milestone.

**NFR (Non-Functional Requirement)**  
A measurable quality constraint, such as latency, throughput, memory,
reproducibility, security, or compatibility.

**Nix flake**  
The pinned declaration that owns Orus development, CI, benchmark, toolchain,
and packaging environments and their supported systems.

**NUMA (Non-Uniform Memory Access)**  
A hardware topology where memory cost depends on processor locality. Orus may
add NUMA-aware behavior only after a representative benchmark demonstrates the
benefit.

## O-P

**Observable output**  
Target-program output captured as an execution fact. Replay may display the
recorded value separately but must not repeat the original outside-world side
effect.

**Open question**  
A bounded unresolved clarification or design choice with impact, options,
default, and blocking status. It is not permission to silently choose a new
product requirement.

**Orus**  
The product and CLI name for this repository.

**Orus Studio (Studio)**  
The future browser evidence workspace for execution state, timelines,
investigations, and replay branches.

**p50/p95/p99**  
The 50th, 95th, and 99th percentile of a declared measurement distribution.
Every use must name the population, unit, sample method, and environment.

**PGO (Profile-Guided Optimization)**  
Compiler optimization using profiles from a versioned representative workload
corpus.

**PID/TID**  
Linux process/thread identifiers. Host PID/TID values are distinct from Orus
virtual process/thread IDs and may be reused by the kernel.

**pidfd**  
A Linux file descriptor referring to a process identity, used where supported
to avoid races caused by numeric PID reuse.

**Pinned reference environment**  
The single Nix-identified Linux x86-64 environment continuously validated by
M0. It includes toolchain and image inputs and records actual kernel/CPU facts;
it does not imply broader distro/kernel compatibility.

**Proof-carrying patch**  
A source change accompanied by a root-cause claim, evidence references,
counterfactual result where applicable, exact build identity, failing/passing
trace results, tests, sanitizers, performance comparison, and residual risk.

**Provenance**  
Structured metadata describing the origin and transformation of a result or
content, including source revision, build, environment, runner, trace, tool,
and whether content came from an untrusted source or model.

## R

**RCP (Replay Control Protocol)**  
The future versioned typed internal protocol for controlling an isolated replay
worker. DAP, GDB RSP, browser, CLI, and agent interfaces adapt to RCP.

**Ready**  
A domain-spec status meaning its scope, requirements, measurable NFRs,
contracts, failures, and verification plan are accepted and implementation may
begin.

**Recording**  
An immutable artifact that captures the declared determinism boundary,
topology, schedule, metadata, and external observations of a target execution.
The concrete stored artifact is a trace.

**Reference correctness path**  
A simple, strongly validated implementation used to establish semantics and
diagnose divergence. It may be slower than the commercial fast path but is not
allowed to define a different logical event model.

**Regression**  
A statistically valid degradation relative to a comparable baseline. Orus's
general hot-benchmark threshold is greater than 3%, subject to authority and
the exact performance-domain method.

**Replay session**  
The isolated authoritative worker state navigating one trace and branch. One
worker owns mutable state for a session at a time.

**Resource bound**  
A configured, enforced maximum on memory, processes, CPU, storage, I/O, queue
depth, input size, nesting, or time. A warning without enforcement is not a
resource bound.

**Reverse execution**  
Restoring an earlier checkpoint (or program start) and deterministically
replaying forward to an earlier requested execution point; it does not imply
executing machine instructions backward.

## S

**SBOM (Software Bill of Materials)**  
A machine-readable inventory of components and dependency identities in a
release, including versions and available license metadata.

**Schema version**  
An explicit identifier for a serialized data contract. Compatibility must be
negotiated or rejected; native ABI layout is not a schema.

**SPSC (Single-Producer/Single-Consumer)**  
A queue ownership pattern with exactly one producer and one consumer, preferred
for bounded recorder pipelines where it fits the measured design.

**Source of execution truth**  
The recording plus continuously validated replay. Source text, logs, model
output, or explanation can support an investigation but cannot override that
evidence.

**Strict deterministic mode**  
A scheduling mode that runs one target task at a time, records every scheduling
decision, and replays the complete concurrent schedule.

**Supported**  
Covered by an explicit contract and continuously passing evidence. “Not
observed to fail” and “compatible in one experiment” do not mean supported.

**Syscall policy**  
The per-system-call declaration to emulate, execute under control and validate,
virtualize, or reject, including determinism inputs, output memory, lifecycle,
descriptor, blocking, signal, mapping, side-effect, and test behavior.

## T-Z

**Task**  
A schedulable virtual process/thread execution context. It is not synonymous
with a pthread, host TID, work item, or factory task.

**ThinLTO (Thin Link-Time Optimization)**  
A scalable LLVM whole-program optimization mode that may be used in measured,
reproducible release configurations.

**Trace**  
The immutable recorded execution artifact: manifest, event segments,
checkpoints, indexes, blobs, exact binary/symbol/source identities,
diagnostics, and performance metadata as applicable.

**Trace store**  
The logical storage of trace artifacts. It is not called a database in product
language even if its catalog uses SQLite.

**TSan (ThreadSanitizer)**  
A compiler instrumentation mode for detecting data races. Some tracer behavior
may be incompatible; applicability must be explicit rather than silently
skipped.

**Typed error**  
A versioned error value containing a stable category/code, relevant identities
and context, recoverability, and human-readable diagnostic. Text alone is not
the contract.

**UBSan (UndefinedBehaviorSanitizer)**  
A compiler instrumentation mode that detects selected undefined C/C++ behavior.

**Virtual process ID / virtual thread ID (VPID/VTID)**  
Stable trace identities assigned by Orus and translated independently of live
Linux numeric IDs.

**Viewport tile**  
A bounded, aggregated timeline response for a visible time range, resolution,
lanes, and filters. It avoids sending a complete event stream to a browser.

**XXH3**  
A candidate fast non-cryptographic checksum for integrity/error detection. It
is not a substitute for cryptographic artifact identity.

**x86-64**  
The initial 64-bit x86 host and target architecture. Optional ISA extensions
require runtime dispatch and cannot be assumed from the build machine.

**Zstandard (Zstd)**  
A candidate high-ratio framed compression algorithm for trace segments,
subject to the adaptive compression policy and representative benchmarks.
