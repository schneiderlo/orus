# Native Concurrent Process-Tree Corpus

**Status:** Draft  
**Milestone:** M0  
**Owners:** Concurrent-corpus owner; test-infrastructure owner; performance owner  
**Last updated:** 2026-07-21  
**Depends on:** `specs/10-build-environment.md`, `specs/13-ci-quality.md`, and `specs/14-performance-foundation.md` (Draft; all are Ready prerequisites); Foundation specs (Ready)  
**Foundation decisions:** D-005  
**Risks addressed:** R-003, R-006, R-107

## 1. Purpose

This domain provides the first real Orus vertical-slice workload: a native
Linux parent process with multiple pthreads that launches and execs a child
process with multiple pthreads, coordinates bounded synchronization and IPC,
observes time/randomness, produces one deterministic logical result, joins and
waits, and tears down under success and injected failure.

The corpus is consumed by M0 CI/performance and later M1-M4 task-control,
record, replay, and validation specs. It prevents a single-process or
single-thread architecture from becoming the product baseline. It is a native
workload only: M0 does not trace, record, replay, virtualize, or claim
deterministic execution.

The M0 gate output is a machine-readable 100-run native reliability report with
the declared topology and result on every run, zero timeout, and zero surviving
child/process-group member, plus teardown evidence for each fault mode.

## 2. In Scope

- One parent executable and one distinct exec-child executable.
- Exactly three parent worker pthreads and exactly two child worker pthreads;
  each process also has its main thread.
- Start barriers/condition synchronization with bounded waits and no timing
  sleeps as correctness coordination.
- One bounded parent-child IPC channel surviving the exec boundary.
- Fixed per-thread work partitions whose combined logical result is
  `12502500`, the sum of integers 1 through 5000.
- Time and randomness observation in both processes, excluded from the
  deterministic logical value.
- Joins, child wait/status, descriptor closure, process-group cleanup, timeout,
  cancellation, and fault injection.
- Topology/result/cleanup report and performance-workload registration.

## 3. Out of Scope

- `ptrace`, pidfd-based target ownership semantics, virtual PID/TID,
  deterministic scheduling, trace events, recording, replay, or reverse
  execution; owned by M1-M4.
- Attach to an existing process, daemonization, grandchildren, more than one
  exec child, shared-memory IPC, networking, filesystem side effects, devices,
  or general syscall coverage.
- Using wall-clock sleeps, thread scheduling luck, observed time, or random
  values to determine success.
- Making native runtime an authoritative record/replay overhead claim.
- Stressing unbounded thread/process counts or production service semantics.

## 4. Functional Requirements

| ID | Requirement | Acceptance criteria | Verification method | Traceability |
|---|---|---|---|---|
| CORP-FR-001 | The corpus parent shall create exactly three joinable worker pthreads in addition to its main thread. | Each run's topology evidence contains one parent PID, three distinct worker TIDs created after process start, all three reaching `ready`, `running`, and `joined`; creation or join failure is non-zero. | CORP-TEST-001 topology integration test. | G-05, D-005, R-003. |
| CORP-FR-002 | The parent shall launch one child process that successfully replaces its image with the distinct corpus child executable. | Evidence records fork/launch PID, pre-exec intent, child executable build/artifact identity after exec handshake, and exactly one direct child; wrong/missing child artifact or exec failure cannot pass. | CORP-TEST-002 exec identity and failure fixture. | G-05, D-005. |
| CORP-FR-003 | The exec child shall create exactly two joinable worker pthreads in addition to its main thread. | Child READY is sent only after two distinct worker TIDs exist and are blocked at the declared start condition; both later run and join; failure returns child non-zero and parent reports it. | CORP-TEST-003 child topology test. | G-05, D-005, R-003. |
| CORP-FR-004 | Parent and child shall coordinate through bounded synchronization rather than timing assumptions. | Worker readiness uses mutex/condition or barrier state with predicate loops; START occurs only after all five workers are ready; every wait has the per-run deadline; source/test audit finds no sleep used to order correctness. | CORP-TEST-004 synchronization state and no-sleep audit. | G-05, R-006. |
| CORP-FR-005 | Parent and child shall use one `AF_UNIX` `SOCK_SEQPACKET` socketpair carrying bounded `M0-CORPUS-IPC-v1` frames across exec. | Only the child endpoint is intentionally duplicated to descriptor 3 across exec; exactly that one inherited endpoint is open in the exec child; READY(1), START(2), CHILD_RESULT(3), and ACK(4) occur in sequence; wrong descriptor type/state and frames over 256 bytes, wrong magic/version/type/sequence, duplicate, truncation, or peer close fail closed with typed context before the affected state transition. | CORP-TEST-005 protocol/descriptor success, malformed, and closure matrix. | G-05, C-09 precursor; resolved `q-0008`. |
| CORP-FR-006 | The five workers shall compute fixed disjoint partitions and the parent shall emit one deterministic logical result. | Parent workers cover 1-3000 in three contiguous 1000-value partitions; child workers cover 3001-5000 in two partitions; per-process sums are 4501500 and 8001000; combined value is exactly 12502500; output contains no time/random/PID/TID-dependent value in the logical result. | CORP-TEST-006 boundary/mutation/golden result tests. | G-05, SM-05. |
| CORP-FR-007 | Both processes shall execute time and randomness observations for later capture without allowing them to affect correctness. | Parent and child each call a declared realtime clock source and kernel randomness source after START; success booleans/counts appear in evidence; raw values may be retained only in a diagnostic field excluded from logical-result comparison; failure is typed and non-zero. | CORP-TEST-007 syscall-observation and value-mutation test. | Roadmap `15` purpose; D-005. |
| CORP-FR-008 | Every normal run shall join all worker threads, close owned descriptors, wait for the exec child, and validate statuses before success. | Both child workers and all three parent workers join exactly once; child exits 0 after ACK; parent wait observes that exact exit; parent exits 0 only after all cleanup; no zombie/open IPC endpoint remains. | CORP-TEST-008 lifecycle/resource inspection. | G-05, DOD-09, R-006. |
| CORP-FR-009 | The harness shall enforce one 10-second absolute per-run deadline and tear down the owned process group on failure, timeout, or cancellation. | The deadline starts at parent launch and includes cleanup; graceful teardown may consume at most 2 seconds of the then-remaining deadline before forced kill, and is skipped when no time remains; harness reaps every owned child and proves process-group absence before terminal result; timeout/cancellation never reports success. | CORP-TEST-009 timeout/cancellation/ignore-termination/deadline-boundary tests. | DOD-09, R-006; resolved `q-0009`. |
| CORP-FR-010 | The corpus shall expose finite, deterministic fault modes without external mutation. | `exec_failure`, `parent_worker_failure`, `child_worker_failure`, `child_exit_before_ready`, `malformed_ipc`, `ipc_close`, and `hang_until_timeout` each trigger at the named earliest point, return expected typed failure/non-zero status, and leave zero process/resource leak. | CORP-TEST-010 fault-injection matrix. | DOD-08, DOD-09, R-006. |
| CORP-FR-011 | The native reliability gate shall run 100 fresh normal subprocess instances. | Runs are sequentially indexed 1-100; each creates a new process tree, meets topology/protocol/result/status/cleanup requirements, and is recorded; any failed/missing/duplicate run makes the report fail. | CORP-TEST-011 100-run end-to-end gate. | G-05, SM-05, R-003, R-006. |
| CORP-FR-012 | The corpus shall register as `m0_concurrent_native_v1` in the performance foundation without claiming an M0 product budget. | Workload declares equivalent topology, build/config, deadline, result predicate, raw duration/allocation fields, and authority; shared CI result is advisory; logical correctness remains blocking independent of measured duration. | CORP-TEST-012 registry/result schema integration. | G-04, D-006, D-010. |
| CORP-FR-013 | The corpus shall produce `M0-CORPUS-RUN-v1` and aggregate `M0-CORPUS-RELIABILITY-v1` evidence. | Schemas include run/build IDs, topology counts/roles, IPC sequence, partition sums, observation-success counts, joins/wait/status, deadline/elapsed time, cleanup checks, fault mode, result, and bounded diagnostics; invalid/forged success is rejected. | CORP-TEST-013 schema and forged-report tests. | SM-05, R-008. |

## 5. Non-Functional Requirements

| ID | Quality attribute and target | Conditions / workload | Acceptance criteria | Verification method and evidence |
|---|---|---|---|---|
| CORP-NFR-001 | Reliability: 100/100 fresh normal runs pass. | Pinned reference environment; Clang dev configuration; sequential run harness; 10-second absolute deadline each. | Exactly 100 successes, 0 timeout, 0 crash, 0 wrong topology/result/status/sequence. | CORP-TEST-011 `m0-corpus-reliability-v1.json`. |
| CORP-NFR-002 | Cleanup: 0 remaining owned processes, zombies, IPC endpoints, or harness temporary resources after every normal and fault test. | All 100 normal runs and one execution of each seven fault modes plus cancellation/escalation. | Post-run process-group probe reports absent, all wait obligations satisfied, and descriptor/temp inventory returns to baseline. | CORP-TEST-008 through -010 cleanup report. |
| CORP-NFR-003 | Topology: 100% of valid runs contain 1 parent process + 3 parent worker pthreads and 1 exec child + 2 child worker pthreads. | Normal reliability population. | Every run exact; main threads are separately identified and do not count toward worker minima. | CORP-TEST-001 through -003 topology report. |
| CORP-NFR-004 | Logical determinism: 100/100 normal runs produce exactly one logical value `12502500` and exact process sums. | Same reliability population; OS scheduling/time/random values unconstrained. | Parent=4501500, child=8001000, combined=12502500 each run; one output record. | CORP-TEST-006 golden/reliability report. |
| CORP-NFR-005 | Bounded protocol/resources: frame <=256 bytes, at most 4 success frames, exactly 1 child, 5 worker pthreads, and absolute run deadline <=10 seconds. | Normal and malformed/resource fixtures. | Bounds enforced before read/allocation/creation; graceful teardown uses at most 2 seconds of remaining time; over-limit path is typed failure with cleanup. | CORP-TEST-005/009. |

Recorder overhead, descriptor throughput, writer throughput, and replay
control-latency targets are not applicable because the corpus executes natively
without Orus control. Later owning specs benchmark this same topology and make
those objectives applicable.

## 6. Interfaces / Contracts

### 6.1 Process/thread roles and work partitions

| Role | Count | Work partition | Lifecycle owner |
|---|---:|---|---|
| Parent main | 1 | Launch, IPC, start publication, aggregation, waits, cleanup. | Harness owns parent process; parent main owns its workers/child. |
| Parent worker `P0..P2` | 3 pthreads | Inclusive ranges 1-1000, 1001-2000, 2001-3000. | Parent main creates/joins. |
| Exec-child main | 1 | IPC readiness/result/ACK, child aggregation, joins, exit. | Parent main waits; child main owns child workers. |
| Child worker `C0..C1` | 2 pthreads | Inclusive ranges 3001-4000, 4001-5000. | Child main creates/joins. |

Worker results use unsigned 64-bit integers and overflow-checked addition.
Each worker writes one exclusively owned result slot then publishes completion
under the declared synchronization primitive. A worker never writes another
slot.

### 6.2 `M0-CORPUS-IPC-v1`

The selected transport is one Linux `AF_UNIX` `SOCK_SEQPACKET`
socketpair. Both endpoints start close-on-exec; the launch child duplicates
only its endpoint to fixed descriptor 3 without close-on-exec immediately
before exec, closes all other owned descriptors, and invokes the child with
`--ipc-fd=3`. The child validates descriptor type and peer state before
thread creation.

Each frame is a versioned byte sequence, not a native struct:

| Field | Encoding / bound |
|---|---|
| Magic | 4 ASCII bytes `ORUS`. |
| Version | unsigned 16-bit big-endian; exactly 1. |
| Type | unsigned 16-bit big-endian: READY=1, START=2, CHILD_RESULT=3, ACK=4, CANCEL=5. |
| Sequence | unsigned 32-bit big-endian; success sequence 1 through 4. |
| Payload length | unsigned 32-bit big-endian; 0-240; total frame <=256 bytes. |
| Payload | Type-specific fixed-width big-endian integers; no native padding/string. |

READY contains the child artifact's lowercase hexadecimal SHA-256 digest plus
child worker count. START contains parent worker count and the fixed workload
ID's SHA-256 digest. CHILD_RESULT
contains child sum, child observation success bits, and join count. ACK
contains combined sum and accepted child status. CANCEL has a stable reason
and is best-effort during teardown. Unknown version/type/length/sequence fails;
no capability negotiation exists inside this fixed M0 fixture.

### 6.3 Synchronization, timeout, and cancellation

Each process uses a mutex-protected state predicate and condition variable (or
an equivalent barrier proven to preserve the states) for
`creating -> ready -> running -> completed -> joined`. The absolute harness
deadline is propagated as monotonic-clock deadline to IPC and condition waits.
Spurious wakeups recheck predicates. EINTR retries only while deadline and
state permit. No unbounded wait and no sleep-based ordering is allowed.

The harness creates a dedicated process group for each run and, where supported
by the pinned environment, opens a pidfd for the parent to reduce numeric-PID
races. On terminal failure it sends graceful termination to the group, waits
for at most 2 seconds and never past the absolute 10-second deadline, escalates
immediately to kill when that grace bound or deadline is reached, reaps, then verifies
`kill(-pgid, 0)` reports no member (with EPERM treated as failure, not
absence). M1 owns the stronger general task-control model.

### 6.4 Result schemas

`M0-CORPUS-RUN-v1` contains schema, run index/attempt, workload/build/artifact
IDs, fault mode, parent/child PID evidence (diagnostic, not stable identity),
role/TID counts, ready/run/join states, IPC frame types/sequences, partition
sums, time/random observation-success bits, child wait status, parent status,
deadline/elapsed milliseconds, cancellation/escalation, process-group and
resource cleanup checks, logical result, and typed terminal outcome.

`M0-CORPUS-RELIABILITY-v1` contains schema, source/build/environment IDs,
required/attempted/passed run counts, failure counts by code, run-report
references/SHA-256 digests, aggregate topology/result/timeout/cleanup assertions, and
`passed`. It passes only if all 100 unique run reports validate.

### 6.5 Design choice: exec IPC

| Alternative | Pros | Cons |
|---|---|---|
| `AF_UNIX SOCK_SEQPACKET` socketpair with one intentional inherited FD (selected) | Bidirectional, preserves message boundaries, simple bounded malformed-frame tests. | Linux-specific and requires careful close-on-exec handling. |
| Two unidirectional pipes | Minimal primitives and simple stream behavior. | Needs framing across partial reads and doubles descriptor cleanup. |
| Shared memory plus eventfd | Representative high-throughput IPC. | Adds mapping/lifetime complexity and exceeds M0 fixture need. |

**Recommendation:** Use exactly one socketpair because the initial target is
Linux and the corpus benefits from explicit message boundaries. Duplicate only
the child endpoint to descriptor 3 across exec, bound versioned frames to 256
bytes, and fail closed on malformed framing or descriptor state. This
implements `q-0008`.

### 6.6 Design choice: absolute per-run deadline

| Alternative | Pros | Cons |
|---|---|---|
| 10 seconds from parent launch through cleanup, with at most 2 seconds of remaining time for graceful teardown (selected) | Tolerates shared-CI variance while bounding hangs and the 100-run gate. | A repeated pathological failure can still consume material CI time. |
| 5 seconds total | Faster failure and lower worst-case CI duration. | Higher false-timeout risk on shared runners. |
| 30 seconds total | Most tolerant of slow hosts. | Excessive failure latency for a 100-run gate. |

**Recommendation:** Use the selected 10-second absolute deadline. Graceful
teardown consumes no more than 2 seconds of the time remaining when teardown
starts; forced kill is immediate once either bound is exhausted. This
implements `q-0009`.

## 7. Data Model

| Entity / state | Identity and relationships | Invariants |
|---|---|---|
| Corpus attempt | Build/environment ID plus attempt/run index. | Fresh process group and IPC endpoints; one terminal report. |
| Process role | Attempt-local `parent` or `exec_child`; host PID diagnostic only. | Exactly one each; child artifact identity proves exec image. |
| Worker role | Process role plus fixed `P0..P2` or `C0..C1`. | Exactly five; fixed disjoint partition; one result slot; joined once. |
| Process state | `launching -> threads_ready -> running -> joining -> exited`, or `failed -> tearing_down -> reaped`. | Success only follows joins, ACK, child wait, and cleanup. |
| IPC session | Attempt ID, endpoint owners, next sequence. | One endpoint/process, total frame <=256, success sequence exact. |
| Logical result | Parent sum, child sum, combined value. | Independent of PID/TID, time/random value, scheduling, or duration. |
| Observation | Process role, source `realtime|random`, success, optional diagnostic SHA-256 digest. | Cannot feed partitions/result/branching; failure is explicit. |
| Run evidence | `M0-CORPUS-RUN-v1` identified by its lowercase hexadecimal SHA-256 digest. | Host identities are not VPID/VTID and are never reused as stable product identity. |

The mathematical result and validated lifecycle are authoritative test
evidence. Host PID/TID, observed clock/random values, and elapsed time are
diagnostic/performance fields, not execution identities or deterministic
outputs.

## 8. Key Flows

1. **Successful native run (CORP-FR-001 through CORP-FR-008).** Harness
   creates group/socketpair/deadline and launches parent; parent creates three
   ready workers and exec child; child validates FD/artifact, creates two ready
   workers, sends READY; parent sends START and releases its workers; both
   processes observe time/randomness and compute fixed partitions; child joins
   and sends CHILD_RESULT; parent joins, validates/sums, sends ACK; child exits
   0; parent waits, closes, verifies cleanup, emits one success result.
2. **100-run gate (CORP-FR-011, -013).** Harness runs the successful flow in
   100 fresh sequential attempts, validates each SHA-256-identified report,
   reconciles unique indices/topology/result/cleanup, and emits aggregate pass
   only for 100/100.
3. **Cancellation/teardown (CORP-FR-009).** Harness or injected cancellation
   marks attempt failed, sends best-effort CANCEL/graceful group signal,
   performs bounded escalation, reaps parent and descendants, verifies group
   absence/resource baseline, and emits non-success.
4. **Unsupported/malformed/resource flow (CORP-FR-005, -010).** Earliest
   endpoint detects wrong FD/frame/version/sequence/size or deadline; it stops
   computation/acceptance, reports typed context, and enters the same teardown;
   no partial sum becomes success.

## 9. Failure Modes

| ID | Trigger | Required detection point | Typed outcome / diagnostic fields | Side effects and cleanup | Retry / recovery | Verifying requirements/tests |
|---|---|---|---|---|---|---|
| CORP-FAIL-001 | pthread create/readiness/join failure in either process. [R-006] | Exact pthread operation or deadline. | `CORP_THREAD_LIFECYCLE_FAILED`; process role, worker role, operation, error, state. | Stop acceptance; release/start cancellation; join created workers; tear down group. | Fresh attempt only. | CORP-FR-001, -003, -004, -008 / CORP-TEST-001, -003, -008. |
| CORP-FAIL-002 | Exec fails or wrong child artifact answers. [R-003, R-006] | Exec error channel or READY validation. | `CORP_EXEC_FAILED` or `CORP_CHILD_IDENTITY_MISMATCH`; path/errno or expected/observed artifact ID. | Close endpoints, terminate/reap group; no success. | Correct artifact/build then fresh attempt. | CORP-FR-002 / CORP-TEST-002. |
| CORP-FAIL-003 | Descriptor 3 has wrong type/state, or IPC is truncated, oversize, wrong version/type/sequence, duplicate, or closed. [R-006] | Child descriptor validation before thread creation, or frame header/payload/peer-close boundary before state transition. | `CORP_IPC_DESCRIPTOR_INVALID` or `CORP_IPC_PROTOCOL_ERROR`; endpoint role, descriptor state, expected/observed sequence/type, size/version. | No partial frame or state transition accepted; tear down/reap. | Fresh attempt after defect correction. | CORP-FR-005 / CORP-TEST-005. |
| CORP-FAIL-004 | Partition/result/status differs. | Worker aggregation, CHILD_RESULT, or wait validation. | `CORP_RESULT_MISMATCH`; role/partition, expected/observed sum/status. | Attempt fails after complete cleanup; wrong value retained diagnostically. | Fix workload defect and rerun full gate. | CORP-FR-006, -008 / CORP-TEST-006, -008. |
| CORP-FAIL-005 | Time/random observation fails. | Declared observation call. | `CORP_OBSERVATION_FAILED`; process role, source, errno. | No value used; attempt fails and tears down. | Fresh attempt; environment defect diagnostic. | CORP-FR-007 / CORP-TEST-007. |
| CORP-FAIL-006 | Absolute deadline expires or cancellation occurs. [R-006] | Harness monotonic deadline/cancel boundary. | `CORP_TIMEOUT` or `CORP_CANCELLED`; attempt, state, elapsed/deadline, escalation. | Graceful then forced group teardown, reap, resource check. | Fresh attempt; never resume partial run. | CORP-FR-009 / CORP-TEST-009. |
| CORP-FAIL-007 | Process/group/resource remains after wait. [R-006, R-107] | Post-run cleanup verification before terminal success. | `CORP_CLEANUP_FAILED`; PID/PGID diagnostic, wait/descriptor/temp state, probe result. | Escalate cleanup; gate remains failed even if later cleanup succeeds. | Investigate harness/lifecycle; rerun complete gate. | CORP-FR-008 through -011 / CORP-TEST-008 through -011. |
| CORP-FAIL-008 | Run/aggregate report is missing, duplicate, malformed, digest-mismatched, or claims success without evidence. [R-008] | Report schema/reconciliation before aggregate pass. | `CORP_EVIDENCE_INVALID`; report/index, field/digest, expected/observed. | Aggregate fails; raw valid reports retained. | Regenerate fresh missing attempts; do not edit reports. | CORP-FR-013 / CORP-TEST-013. |

## 10. Observability

| Signal ID | Metric/event/log | Type, unit, and labels | Emission point / owner | Cardinality and cost bound | Alert/gate use | Verification |
|---|---|---|---|---|---|---|
| CORP-OBS-001 | `corpus_role_state` | Event; attempt, process/worker role, prior/new state | State transitions / owning main thread | At most 8 transitions per role; fixed six role/main identities. | Topology/lifecycle gate. | CORP-TEST-001 through -004. |
| CORP-OBS-002 | `corpus_ipc_frame` | Event; attempt, direction, type, sequence, size, status | Send/validated receive, outside worker compute | At most 5 frames/normal run including optional cancel; no payload formatting on compute path. | Protocol diagnosis. | CORP-TEST-005. |
| CORP-OBS-003 | `corpus_run_result` | Event/report; build, run, topology, sums, statuses, elapsed_ms, cleanup, outcome | Harness finalization / corpus owner | One per attempt; diagnostic strings <=4 KiB. | SM-05 input. | CORP-TEST-013. |
| CORP-OBS-004 | `corpus_reliability_summary` | Gauge/report; required/attempted/passed, failures/timeouts/leaks | 100-run aggregation / corpus owner | One per revision/config/environment. | M0 release gate. | CORP-TEST-011. |
| CORP-OBS-005 | `corpus_cleanup_result` | Event; attempt/fault, group_absent, waits_complete, descriptor/temp deltas | Post-run boundary / harness | One per attempt; PID details bounded and diagnostic only. | DOD-09. | CORP-TEST-008 through -010. |

Evidence correlates by source revision, artifact/build IDs, reference
environment, corpus attempt/run, and fixed role identity. Host PID/TID fields
are explicitly diagnostic; M0 emits no trace, branch, execution point, VPID,
or VTID. Per-run reports and the 100-run aggregate are retained with CI/release
evidence.

## 11. Test & Verification Plan

Copy/paste from the repository root after implementation:

```bash
nix develop --command bazel test --config=dev //tests/concurrent/...
nix develop --command bazel test --config=gcc //tests/concurrent/...
nix develop --command bazel test --config=asan //tests/concurrent/...
nix develop --command bazel test --config=ubsan //tests/concurrent/...
nix develop --command bazel test --config=tsan //tests/concurrent/...
nix develop --command bazel run //tests/concurrent:corpus_parent
nix develop --command bazel run //tests/concurrent:corpus_reliability -- --runs=100 --timeout-seconds=10 --output=bazel-bin/test-evidence/m0-corpus-reliability-v1.json
nix develop --command bazel test //tests/concurrent:corpus_fault_matrix
nix develop --command bazel test --config=benchmark //tests/benchmarks:m0_concurrent_native
```

| Requirement ID | Test/benchmark/review ID | Level | Fixture/workload and environment | Pass criterion | Evidence artifact |
|---|---|---|---|---|---|
| CORP-FR-001 | CORP-TEST-001 | Integration | Normal parent run with topology observation. | Exactly 3 ready/running/joined parent workers. | `parent-topology.json`. |
| CORP-FR-002 | CORP-TEST-002 | Integration/negative | Valid child, missing executable, wrong artifact handshake. | One valid exec child; both negatives fail and clean. | `exec-child-matrix.json`. |
| CORP-FR-003 | CORP-TEST-003 | Integration/fault | Normal child and second-thread creation failure. | Exactly 2 workers valid; injected failure typed/clean. | `child-topology.json`. |
| CORP-FR-004 | CORP-TEST-004 | Concurrency/inspection | Spurious wakeup/readiness-order mutations and source audit. | Predicates preserve order; zero sleep-based correctness. | `synchronization-report.json`. |
| CORP-FR-005, CORP-NFR-005 | CORP-TEST-005 | Protocol/resource/negative | Success frames and truncation/oversize/version/type/sequence/duplicate/close fixtures. | Success exact; every invalid frame rejected <=256-byte bound and cleanup passes. | `corpus-ipc-matrix.json`. |
| CORP-FR-006, CORP-NFR-004 | CORP-TEST-006 | Unit/integration/golden | Partition boundaries, mutated boundary/result, normal output. | Exact sums/result; mutations fail; one deterministic logical record. | `corpus-result-golden.json`. |
| CORP-FR-007 | CORP-TEST-007 | Integration/mutation | Successful/failed time/random calls and substituted raw values. | Both processes observe; failures typed; substituted values do not change logical result. | `corpus-observations.json`. |
| CORP-FR-008, CORP-NFR-002 | CORP-TEST-008 | Integration/resource | Normal and child non-zero paths under descriptor/process observation. | All joins/wait/status exact; resource baseline restored. | `corpus-lifecycle-cleanup.json`. |
| CORP-FR-009 | CORP-TEST-009 | Cancellation/timeout | Harness cancel, hang, ignored graceful termination. | Non-success; bounded escalation; zero group member/resource. | `corpus-timeout-cancel.json`. |
| CORP-FR-010 | CORP-TEST-010 | Fault injection | All seven finite fault modes. | Expected earliest typed failure/non-zero and zero leak for each. | `corpus-fault-matrix.json`. |
| CORP-FR-011, CORP-NFR-001, CORP-NFR-003, CORP-NFR-004 | CORP-TEST-011 | End-to-end/reliability | 100 fresh sequential runs in reference environment. | 100/100 exact topology/result/status/cleanup; 0 timeout/leak. | `m0-corpus-reliability-v1.json`, 100 run reports. |
| CORP-FR-012 | CORP-TEST-012 | Schema/benchmark | Workload registry and shared-runner benchmark result. | Registry valid; correctness blocking; measured delta advisory. | `m0-concurrent-native-result.json`. |
| CORP-FR-013 | CORP-TEST-013 | Schema/security | Valid reports and missing/duplicate/digest/forged-success fixtures. | Valid reconciles; every invalid/forged fixture fails. | `corpus-evidence-schema.json`. |

## 12. Open Questions

No open questions.
