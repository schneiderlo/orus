# Native Concurrent Process-Tree Corpus

**Status:** Draft  
**Milestone:** M0  
**Owners:** Concurrent-corpus owner; test-infrastructure owner; performance owner  
**Last updated:** 2026-07-21  
**Depends on:** `specs/10-build-environment.md`, `specs/13-ci-quality.md`, and `specs/14-performance-foundation.md` (Draft; all are Ready prerequisites); Foundation specs (Ready)  
**Foundation decisions:** D-005  
**Risks addressed:** R-003, R-006, R-008, R-107, R-201

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
| CORP-FR-005 | Parent and child shall use one `AF_UNIX` `SOCK_SEQPACKET` socketpair carrying exact `M0-CORPUS-IPC-v1` byte frames across exec. | Only the child endpoint is duplicated to descriptor 3. Header and every READY/START/CHILD_RESULT/ACK/CANCEL payload match Section 6.2 offsets, widths, big-endian encoding, digest bytes, bits, enums, lengths, reserved-zero rules, direction, and sequence. Wrong descriptor/state or any malformed/truncated/extra/duplicate/closed frame fails with the exact protocol code before the affected state transition. | CORP-TEST-005 encoded-success golden, one-negative-per-wire-field, descriptor, and closure matrix. | G-05, C-09, R-008; resolved `q-0008`. |
| CORP-FR-006 | The five workers shall compute fixed disjoint partitions and the parent shall emit one deterministic logical result. | Parent workers cover 1-3000 in three contiguous 1000-value partitions; child workers cover 3001-5000 in two partitions; per-process sums are 4501500 and 8001000; combined value is exactly 12502500; output contains no time/random/PID/TID-dependent value in the logical result. | CORP-TEST-006 boundary/mutation/golden result tests. | G-05, SM-05. |
| CORP-FR-007 | Both process main threads shall execute the exact time/randomness observation pair in Section 6.3 once after START and before worker computation is released. | Parent main and child main each call `clock_gettime(CLOCK_REALTIME, &timespec)` exactly once and `getrandom(buf, 32, 0)` exactly once. Clock returns 0 with `0<=tv_nsec<1000000000`; getrandom returns exactly 32; EINTR, short read, or any other return is failure with no retry. Four operation rows/counts and the mapped success bits appear in evidence. Raw clock/random values cannot affect partition/result/branching and, if retained, appear only as SHA-256 diagnostic digests. | CORP-TEST-007 call-count/argument/return, failure, and value-mutation test. | Roadmap `15` purpose; D-005, R-003, R-006. |
| CORP-FR-008 | Every normal run shall join all worker threads, close owned descriptors, wait for the exec child, and validate statuses before success. | Both child workers and all three parent workers join exactly once; child exits 0 after ACK; parent wait observes that exact exit; parent exits 0 only after all cleanup; no zombie/open IPC endpoint remains. | CORP-TEST-008 lifecycle/resource inspection. | G-05, DOD-09, R-006. |
| CORP-FR-009 | The harness shall enforce one 10-second absolute per-run deadline and tear down the owned process group on failure, timeout, or cancellation. | The deadline starts at parent launch and includes cleanup; graceful teardown may consume at most 2 seconds of the then-remaining deadline before forced kill, and is skipped when no time remains; harness reaps every owned child and proves process-group absence before terminal result; timeout/cancellation never reports success. | CORP-TEST-009 timeout/cancellation/ignore-termination/deadline-boundary tests. | DOD-09, R-006; resolved `q-0009`. |
| CORP-FR-010 | The corpus shall expose finite, deterministic fault modes without external mutation. | `exec_failure`, `parent_worker_failure`, `child_worker_failure`, `child_exit_before_ready`, `malformed_ipc`, `ipc_close`, and `hang_until_timeout` each trigger at the named earliest point, emit exactly the Section 6.4 fault-mode/terminal pair, return non-zero, and leave zero process/resource leak. | CORP-TEST-010 fault-injection and report-mapping matrix. | DOD-08, DOD-09, R-006. |
| CORP-FR-011 | The native reliability gate shall run 100 fresh normal subprocess instances. | Runs are sequentially indexed 1-100; each creates a new process tree, meets topology/protocol/result/status/cleanup requirements, and is recorded; any failed/missing/duplicate run makes the report fail. | CORP-TEST-011 100-run end-to-end gate. | G-05, SM-05, R-003, R-006. |
| CORP-FR-012 | The corpus shall register as `m0_concurrent_native_v1` in the performance foundation without claiming an M0 product budget. | Workload declares equivalent topology, build/config, deadline, result predicate, raw duration/allocation fields, and authority; shared CI result is advisory; logical correctness remains blocking independent of measured duration. | CORP-TEST-012 registry/result schema integration. | G-04, D-006, D-010. |
| CORP-FR-013 | The corpus shall produce the normative `M0-CORPUS-RUN-v1` and `M0-CORPUS-RELIABILITY-v1` documents in Section 6.4. | Every required field/type/enum/bound/relationship, PID/TID ownership, distinct child image, fault mapping, failure-count type/order, and aggregate count equation validates. Reports are canonical JSON, at most depth 16 and 1 MiB/run or 16 MiB/aggregate, with a typed terminal outcome; missing/extra/type/enum/bound/digest/invariant mutations and forged success are rejected with the exact error code. | CORP-TEST-013 valid-example, one-negative-per-field/relationship, byte/depth bound, topology/fault/aggregate-forgery tests. | SM-05, R-008, R-201. |

## 5. Non-Functional Requirements

| ID | Quality attribute and target | Conditions / workload | Acceptance criteria | Verification method and evidence |
|---|---|---|---|---|
| CORP-NFR-001 | Reliability: 100/100 fresh normal runs pass. | Pinned reference environment; Clang dev configuration; sequential run harness; 10-second absolute deadline each. | Exactly 100 successes, 0 timeout, 0 crash, 0 wrong topology/result/status/sequence. | CORP-TEST-011 `m0-corpus-reliability-v1.json`. |
| CORP-NFR-002 | Cleanup: 0 remaining owned processes, zombies, IPC endpoints, or harness temporary resources after every normal and fault test. | All 100 normal runs and one execution of each seven fault modes plus cancellation/escalation. | Post-run process-group probe reports absent, all wait obligations satisfied, and descriptor/temp inventory returns to baseline. | CORP-TEST-008 through -010 cleanup report. |
| CORP-NFR-003 | Topology: 100% of valid runs contain 1 parent process + 3 parent worker pthreads and 1 exec child + 2 child worker pthreads. | Normal reliability population. | Every run exact; main threads are separately identified and do not count toward worker minima. | CORP-TEST-001 through -003 topology report. |
| CORP-NFR-004 | Logical determinism: 100/100 normal runs produce exactly one logical value `12502500` and exact process sums. | Same reliability population; OS scheduling/time/random values unconstrained. | Parent=4501500, child=8001000, combined=12502500 each run; one output record. | CORP-TEST-006 golden/reliability report. |
| CORP-NFR-005 | Bounded protocol/resources: frame <=256 bytes (fixed success frames <=56), at most 4 success frames, exactly 2 processes/7 total threads, at most 4 corpus-owned live FDs during descriptor transfer (1 socketpair plus optional pidfd/dup transient), and absolute deadline 10 seconds. | Normal and malformed/resource fixtures. | Bounds enforced before read/allocation/creation; graceful teardown uses at most 2 seconds of remaining time; over-limit path is typed failure with cleanup. | CORP-TEST-005/009. |

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

| Header byte offset | Width | Encoding / required value |
|---:|---:|---|
| 0 | 4 | ASCII `ORUS` (`4f 52 55 53`). |
| 4 | 2 | Unsigned big-endian version, exactly 1. |
| 6 | 2 | Unsigned big-endian type: READY=1, START=2, CHILD_RESULT=3, ACK=4, CANCEL=5. |
| 8 | 4 | Unsigned big-endian sequence. Success is exactly 1,2,3,4. |
| 12 | 4 | Unsigned big-endian payload byte length; must equal the type's fixed length below and received datagram length minus 16. |

All multibyte payload integers are unsigned big-endian; SHA-256 values are 32
raw digest bytes, never hexadecimal text. Bits not assigned below and every
reserved byte must be zero.

| Type / direction / sequence | Payload bytes | Payload byte offsets and values |
|---|---:|---|
| READY child->parent / 1 | 40 | 0-31 `child_artifact_sha256`; 32-33 `child_worker_count=2`; 34-35 `ready_bits=0x0003` where bit 0 means descriptor validated and bit 1 means both workers ready; 36-39 reserved zero. |
| START parent->child / 2 | 40 | 0-1 `parent_worker_count=3`; 2-3 `observation_request_bits=0x0003` where bit 0 requests realtime and bit 1 randomness; 4-35 raw `SHA256(UTF8("m0_concurrent_native_v1"))`; 36-39 reserved zero. |
| CHILD_RESULT child->parent / 3 | 24 | 0-7 `child_sum=8001000`; 8-11 observation bits (bit 0 clock success, bit 1 random success; exactly `0x00000003` on success); 12-13 `child_join_count=2`; 14-15 status `0=success,1=observation_failed,2=worker_failed,3=join_failed`; 16-23 reserved zero. |
| ACK parent->child / 4 | 16 | 0-7 `combined_sum=12502500`; 8-9 `accepted_child_status=0`; 10-11 `parent_join_count=3`; 12-15 observation bits (0 parent clock, 1 parent random, 2 child clock, 3 child random; exactly `0x0000000f` on success). |
| CANCEL either direction / next expected sequence 1-5 | 8 | 0-1 reason `1=caller_cancel,2=deadline,3=peer_failure,4=protocol_error`; 2-3 origin `1=parent,2=child`; 4-7 reserved zero. Sequence equals the receiver's next success sequence, or 5 only after ACK and before peer exit. |

The fixed total wire sizes are READY=56, START=56, CHILD_RESULT=40, ACK=32,
and CANCEL=24 bytes. `recvmsg` must detect `MSG_TRUNC`; a datagram with fewer or
more bytes than header-declared/fixed size is rejected. Direction, state, and
sequence are validated before payload use. Error code is exactly one of
`CORP_IPC_DESCRIPTOR_INVALID`, `CORP_IPC_MAGIC`, `CORP_IPC_VERSION`,
`CORP_IPC_TYPE`, `CORP_IPC_DIRECTION`, `CORP_IPC_SEQUENCE`,
`CORP_IPC_LENGTH`, `CORP_IPC_RESERVED`, `CORP_IPC_FIELD`,
`CORP_IPC_TRUNCATED`, `CORP_IPC_EXTRA_BYTES`, or `CORP_IPC_PEER_CLOSED`.

Encoded READY success with an all-zero fixture digest is:

```text
4f52555300010001000000010000002800000000000000000000000000000000000000000000000000000000000000000002000300000000
```

Changing the last eight hex digits to `00000001` is a same-length malformed
fixture and must return `CORP_IPC_RESERVED` before `threads_ready`. No
capability negotiation exists inside this fixed M0 fixture.

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

After START is received/sent, each process main performs exactly one
`clock_gettime(CLOCK_REALTIME, &timespec)` then exactly one
`getrandom(buffer, 32, 0)`, in that order, before publishing `running` to its
workers. Each call is attempted once: clock succeeds only on return 0 and a
normalized `timespec`; getrandom succeeds only on return 32. EINTR/short reads
are typed failures with no retry, so the syscall count is invariant. The four
successes map to ACK bits 0-3; child bits also appear in CHILD_RESULT. Raw
values are wiped after optional SHA-256 diagnostic derivation and never enter a
work partition, IPC decision, result, or success predicate.

### 6.4 Result schemas

Both documents use `M0-CANONICAL-JSON-v1`. Unknown fields are rejected. `id`
is ASCII `[a-z0-9][a-z0-9._:-]{0,127}`, `hex64` is `[0-9a-f]{64}`, paths use
spec-`10` relative-path rules, strings are <=4 KiB, and all integers are
non-negative signed-64 unless narrowed below.

#### 6.4.1 `M0-CORPUS-RUN-v1`

The document is at most 1 MiB and has exactly:

| Field | Type / bound | Success invariant |
|---|---|---|
| `schema`, `attempt_id`, `run_index` | literal; `id`; uint8 1-100 | Unique attempt/index. |
| `source_revision`, `environment_id`, `parent_artifact_sha256`, `child_artifact_sha256` | 40/64 lower hex; three `hex64` | Exact build/environment/artifact identities. |
| `fault_mode` | `none|exec_failure|parent_worker_failure|child_worker_failure|child_exit_before_ready|malformed_ipc|ipc_close|hang_until_timeout` | Success requires `none`. |
| `terminal` | `success|thread_lifecycle_failed|exec_failed|child_identity_mismatch|ipc_protocol_error|result_mismatch|observation_failed|timeout|cancelled|cleanup_failed` | `passed=true` iff `success`. |
| `topology` | object | Exactly `process_count:uint8`, `thread_count:uint8`, `parent_worker_count:uint8`, `child_worker_count:uint8`, and seven unique role rows. A role row is `{role,parent_main|P0|P1|P2|child_main|C0|C1; host_pid:uint32>=1; host_tid:uint32>=1; final_state:exited|joined}`. Success counts are 2/7/3/2; mains exited, workers joined; all parent roles share the parent-main PID, all child roles share one different child-main PID, and the distinct-PID count equals 2. Each main has `host_tid=host_pid`; all seven TIDs are unique; P0-P2 TIDs differ from parent main and C0-C1 differ from child main. |
| `ipc` | array 0-5 | Row exactly `direction:child_to_parent|parent_to_child`, `type` enum, `sequence:uint32`, `wire_bytes:uint16`, `status:accepted|rejected`; success is four accepted rows with sequences 1-4 and sizes 56/56/40/32. |
| `partitions` | array 0-5 | Row exactly `role:P0|P1|P2|C0|C1`, `first:uint32`, `last:uint32`, `sum:non-negative int64`; success has the Section 6.1 five rows/sums. |
| `observations` | array 0-4 | Row exactly `process:parent|child`, `operation:clock_gettime_realtime|getrandom_32`, `call_count:uint8`, `success:boolean`, `value_sha256:hex64|null`; success has all four unique rows, count 1, true. |
| `lifecycle` | object | Exactly `parent_join_count:uint8`, `child_join_count:uint8`, `child_waited:boolean`, `child_wait_status:int32`, `parent_status:int32`, `cancel_sent:boolean`, `forced_kill:boolean`; success 3/2/true/0/0/false/false. |
| `deadline_ms`, `elapsed_ms` | uint32 | Deadline exactly 10000; success elapsed <=10000. |
| `cleanup` | object | Exactly booleans `group_absent`, `all_waits_complete`, `ipc_closed`, `fd_baseline_restored`, `temporary_resources_removed`; all true before any success. |
| `sums` | object | Exactly non-negative-int64 `parent`, `child`, `combined`; success 4501500/8001000/12502500. |
| `logical_result` | non-negative int64 | Success exactly 12502500 and equals combined. |
| `diagnostics` | array 0-32 strings | Each <=4 KiB; empty on ordinary success; no raw random bytes. |
| `passed` | boolean | True iff every success invariant above holds. |

For every success report, `parent_artifact_sha256` and
`child_artifact_sha256` are unequal, proving the exec child image is distinct.
The parent role set is exactly `parent_main,P0,P1,P2`; the child role set is
exactly `child_main,C0,C1`; no host PID/TID may supply role ownership contrary
to those sets. Scalar counts never override these row-level equations.

Injected fault reports use this exact mapping after cleanup succeeds:

| `fault_mode` | Required `terminal` |
|---|---|
| `exec_failure` | `exec_failed` |
| `parent_worker_failure` | `thread_lifecycle_failed` |
| `child_worker_failure` | `thread_lifecycle_failed` |
| `child_exit_before_ready` | `thread_lifecycle_failed` |
| `malformed_ipc` | `ipc_protocol_error` |
| `ipc_close` | `ipc_protocol_error` |
| `hang_until_timeout` | `timeout` |

An injected-fault report with another terminal is
`CORP_REPORT_RELATIONSHIP_INVALID`. `cleanup_failed` supersedes the mapped
terminal only when a cleanup boolean is false, and that report is never the
expected passing fault-fixture result. With `fault_mode=none`, `success` is
permitted only when every success invariant holds; naturally detected
non-injected failures use their detected terminal and remain non-pass.

Canonical valid success example (fixture identities/PIDs are diagnostic):

```json
{"attempt_id":"attempt.1","child_artifact_sha256":"cccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccc","cleanup":{"all_waits_complete":true,"fd_baseline_restored":true,"group_absent":true,"ipc_closed":true,"temporary_resources_removed":true},"deadline_ms":10000,"diagnostics":[],"elapsed_ms":10,"environment_id":"aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa","fault_mode":"none","ipc":[{"direction":"child_to_parent","sequence":1,"status":"accepted","type":"READY","wire_bytes":56},{"direction":"parent_to_child","sequence":2,"status":"accepted","type":"START","wire_bytes":56},{"direction":"child_to_parent","sequence":3,"status":"accepted","type":"CHILD_RESULT","wire_bytes":40},{"direction":"parent_to_child","sequence":4,"status":"accepted","type":"ACK","wire_bytes":32}],"lifecycle":{"cancel_sent":false,"child_join_count":2,"child_wait_status":0,"child_waited":true,"forced_kill":false,"parent_join_count":3,"parent_status":0},"logical_result":12502500,"observations":[{"call_count":1,"operation":"clock_gettime_realtime","process":"child","success":true,"value_sha256":null},{"call_count":1,"operation":"getrandom_32","process":"child","success":true,"value_sha256":null},{"call_count":1,"operation":"clock_gettime_realtime","process":"parent","success":true,"value_sha256":null},{"call_count":1,"operation":"getrandom_32","process":"parent","success":true,"value_sha256":null}],"parent_artifact_sha256":"bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb","partitions":[{"first":3001,"last":4000,"role":"C0","sum":3500500},{"first":4001,"last":5000,"role":"C1","sum":4500500},{"first":1,"last":1000,"role":"P0","sum":500500},{"first":1001,"last":2000,"role":"P1","sum":1500500},{"first":2001,"last":3000,"role":"P2","sum":2500500}],"passed":true,"run_index":1,"schema":"M0-CORPUS-RUN-v1","source_revision":"0123456789abcdef0123456789abcdef01234567","sums":{"child":8001000,"combined":12502500,"parent":4501500},"terminal":"success","topology":{"child_worker_count":2,"parent_worker_count":3,"process_count":2,"roles":[{"final_state":"joined","host_pid":2,"host_tid":6,"role":"C0"},{"final_state":"joined","host_pid":2,"host_tid":7,"role":"C1"},{"final_state":"joined","host_pid":1,"host_tid":3,"role":"P0"},{"final_state":"joined","host_pid":1,"host_tid":4,"role":"P1"},{"final_state":"joined","host_pid":1,"host_tid":5,"role":"P2"},{"final_state":"exited","host_pid":2,"host_tid":2,"role":"child_main"},{"final_state":"exited","host_pid":1,"host_tid":1,"role":"parent_main"}],"thread_count":7}}
```

#### 6.4.2 `M0-CORPUS-RELIABILITY-v1`

The aggregate is at most 16 MiB. It has exactly `schema`, `gate_profile`
(`unit_fixture|m0_release`), `source_revision`, `build_id:hex64`,
`environment_id:hex64`, `required_runs:uint8`, `attempted_runs:uint8`,
`passed_runs:uint8`, `failure_counts` (0-9 rows),
`run_reports` (1-100 sorted rows with exactly `run_index:uint8`,
`path:relpath`, `byte_length:non-negative int64`, `sha256:hex64`), `aggregate` (exact
booleans `indices_unique`, `topology_exact`, `result_exact`, `zero_timeout`,
`cleanup_complete`, `digests_valid`), and `passed:boolean`.
`m0_release` requires all three counts exactly 100, indices 1-100, no failure
row, all aggregate booleans true, and 100 run reports; `unit_fixture` permits
1-100 but otherwise uses the same reconciliation.

Each `failure_counts` row has exactly `terminal` from the nine non-success
`M0-CORPUS-RUN-v1` terminal values and `count:uint8` in 1-100. Rows are unique
and sorted by terminal's ASCII bytes; zero-count rows are forbidden. For both
profiles, `attempted_runs=run_reports.length`, `passed_runs` equals the number
of referenced run documents with `passed=true` and `terminal=success`, and
`sum(failure_counts.count)=attempted_runs-passed_runs`. For every non-success
terminal, its count equals the number of digest-valid referenced run documents
with that terminal; an unlisted terminal therefore has count zero. Every
referenced run's index/path/digest/length, source revision, and environment ID
must equal its aggregate row/context. `passed=true` iff
`attempted_runs=required_runs`, `passed_runs=required_runs`, the failure sum is
zero, and every aggregate boolean is true. This schema-valid unit
example does not substitute for the M0 gate:

```json
{"aggregate":{"cleanup_complete":true,"digests_valid":true,"indices_unique":true,"result_exact":true,"topology_exact":true,"zero_timeout":true},"attempted_runs":1,"build_id":"bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb","environment_id":"aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa","failure_counts":[],"gate_profile":"unit_fixture","passed":true,"passed_runs":1,"required_runs":1,"run_reports":[{"byte_length":2,"path":"runs/1.json","run_index":1,"sha256":"cccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccc"}],"schema":"M0-CORPUS-RELIABILITY-v1","source_revision":"0123456789abcdef0123456789abcdef01234567"}
```

#### 6.4.3 Validation errors

Reports fail with canonical `M0-CORPUS-ERROR-v1`, exactly `schema`, `code`,
`document_schema`, `field_path` (<=512 bytes), `expected`, redacted `observed`
(each <=1024), `limit:int64|null`, and `message` (<=4 KiB). Codes are
`CORP_REPORT_SCHEMA_UNKNOWN`, `CORP_REPORT_NONCANONICAL`,
`CORP_REPORT_FIELD_MISSING`, `CORP_REPORT_FIELD_TYPE`,
`CORP_REPORT_FIELD_BOUND`, `CORP_REPORT_ENUM_INVALID`,
`CORP_REPORT_DIGEST_INVALID`, `CORP_REPORT_RELATIONSHIP_INVALID`, or
`CORP_REPORT_FORGED_SUCCESS`. Validation happens before aggregate acceptance;
there is no best-effort success.

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

### 6.7 Design choice: reusable nondeterministic observations

| Alternative | Pros | Cons |
|---|---|---|
| One `clock_gettime(CLOCK_REALTIME)` and one 32-byte `getrandom(...,0)` call per process (recommended) | Direct Linux kernel-facing time/random results, fixed syscall/count/size, useful to later capture policy. | Short/EINTR behavior must fail rather than retry to preserve count. |
| C/C++ time and PRNG library APIs | Portable and simple. | May be userspace-only, seeded/deterministic, or expand into implementation-dependent calls. |
| Read `/dev/urandom` plus filesystem timestamp | Exercises FDs/filesystem metadata. | Adds descriptor/path/permission state outside the bounded one-channel corpus. |

**Recommendation:** Use the first alternative exactly at the declared
post-START/pre-worker-release point. Its raw values remain diagnostic-only.

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
   workers, sends READY; parent sends START; both main threads perform their
   exact observation pair, then release workers to compute fixed partitions; child joins
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
| CORP-FAIL-003 | Descriptor 3 invalid, or wire magic/version/type/direction/sequence/length/reserved/field/truncation/extra bytes/peer state invalid. [R-006, R-008] | Descriptor before thread creation; header before payload allocation; payload before state transition. | Exact Section 6.2 `CORP_IPC_*` code; endpoint/state, byte offset, expected/observed integer/size. | No partial frame/state accepted; tear down/reap. | Fresh attempt after defect correction. | CORP-FR-005 / CORP-TEST-005. |
| CORP-FAIL-004 | Partition/result/status differs. | Worker aggregation, CHILD_RESULT, or wait validation. | `CORP_RESULT_MISMATCH`; role/partition, expected/observed sum/status. | Attempt fails after complete cleanup; wrong value retained diagnostically. | Fix workload defect and rerun full gate. | CORP-FR-006, -008 / CORP-TEST-006, -008. |
| CORP-FAIL-005 | Exact clock/getrandom call returns error, EINTR, short count, or invalid timespec. | Main-thread observation boundary before worker release. | `CORP_OBSERVATION_FAILED`; process role, operation, call_count=1, arguments, return/errno, expected return/range. | No raw value/result use; workers are not released or are cancelled; attempt tears down. | Fresh attempt; no in-attempt retry. | CORP-FR-007 / CORP-TEST-007. |
| CORP-FAIL-006 | Absolute deadline expires or cancellation occurs. [R-006] | Harness monotonic deadline/cancel boundary. | `CORP_TIMEOUT` or `CORP_CANCELLED`; attempt, state, elapsed/deadline, escalation. | Graceful then forced group teardown, reap, resource check. | Fresh attempt; never resume partial run. | CORP-FR-009 / CORP-TEST-009. |
| CORP-FAIL-007 | Process/group/resource remains after wait. [R-006, R-107] | Post-run cleanup verification before terminal success. | `CORP_CLEANUP_FAILED`; PID/PGID diagnostic, wait/descriptor/temp state, probe result. | Escalate cleanup; gate remains failed even if later cleanup succeeds. | Investigate harness/lifecycle; rerun complete gate. | CORP-FR-008 through -011 / CORP-TEST-008 through -011. |
| CORP-FAIL-008 | Run/aggregate report is missing/extra/duplicate/malformed/over-limit/digest-mismatched or violates a success invariant. [R-008, R-201] | Canonical schema/reconciliation before aggregate pass. | Exact Section 6.4.3 `CORP_REPORT_*` code; report/index, path/digest/limit, expected/observed. | Aggregate fails; raw valid reports retained. | Regenerate fresh attempts; do not edit reports. | CORP-FR-013 / CORP-TEST-013. |

## 10. Observability

| Signal ID | Metric/event/log | Type, unit, and labels | Emission point / owner | Cardinality and cost bound | Alert/gate use | Verification |
|---|---|---|---|---|---|---|
| CORP-OBS-001 | `corpus_role_state` | Event; attempt, process/worker role, prior/new state | State transitions / owning main thread | Exactly seven identities (2 mains + 5 workers), at most 8 transitions each: <=56 rows/run. | Topology/lifecycle gate. | CORP-TEST-001 through -004. |
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
| CORP-FR-005, CORP-NFR-005 | CORP-TEST-005 | Protocol/resource/negative/golden | Encoded success frames plus one mutation for every header/payload offset, digest byte, bit, enum, fixed length, reserved field, direction/state/sequence, truncation/extra/duplicate/close, FD, and fifth-owned-FD fixture. | Golden bytes decode exactly; every mutation returns its exact code before transition; <=256 bytes/<=4 live owned FDs and cleanup hold. | `corpus-ipc-matrix.json`, wire goldens. |
| CORP-FR-006, CORP-NFR-004 | CORP-TEST-006 | Unit/integration/golden | Partition boundaries, mutated boundary/result, normal output. | Exact sums/result; mutations fail; one deterministic logical record. | `corpus-result-golden.json`. |
| CORP-FR-007 | CORP-TEST-007 | Integration/syscall/mutation | Both process mains; exact clock/getrandom calls; return 0/32, EINTR, short/random, invalid-timespec, extra-call, and substituted-value fixtures. | Exactly four operations/count 1/arguments exact; any nonexact return fails/no retry before compute; value substitutions preserve logical result. | `corpus-observations.json`. |
| CORP-FR-008, CORP-NFR-002 | CORP-TEST-008 | Integration/resource | Normal and child non-zero paths under descriptor/process observation. | All joins/wait/status exact; resource baseline restored. | `corpus-lifecycle-cleanup.json`. |
| CORP-FR-009 | CORP-TEST-009 | Cancellation/timeout | Harness cancel, hang, ignored graceful termination. | Non-success; bounded escalation; zero group member/resource. | `corpus-timeout-cancel.json`. |
| CORP-FR-010 | CORP-TEST-010 | Fault injection/schema | All seven finite fault modes plus one wrong-terminal report per mode. | Expected earliest typed failure/non-zero, exact Section 6.4 mapping, and zero leak for each; every wrong mapping is rejected. | `corpus-fault-matrix.json`. |
| CORP-FR-011, CORP-NFR-001, CORP-NFR-003, CORP-NFR-004 | CORP-TEST-011 | End-to-end/reliability | 100 fresh sequential runs in reference environment. | 100/100 exact topology/result/status/cleanup; 0 timeout/leak. | `m0-corpus-reliability-v1.json`, 100 run reports. |
| CORP-FR-012 | CORP-TEST-012 | Schema/benchmark | Workload registry and shared-runner benchmark result. | Registry valid; correctness blocking; measured delta advisory. | `m0-concurrent-native-result.json`. |
| CORP-FR-013 | CORP-TEST-013 | Schema/security/resource | Both Section 6.4 valid examples; one missing/extra/type/enum/bound/relationship/digest mutation per field; 1-MiB/16-MiB edges; forged scalar-correct reports with split parent PIDs, reused/cross-owned TIDs, equal parent/child image digests, wrong fault terminal, zero/duplicate/unsorted failure row, and mismatched attempted/passed/failure sums. | Valid examples parse; release profile accepts only exact 100; every topology, fault, aggregate, and other invalid mutation returns exact report code before aggregate pass. | `corpus-evidence-schema.json`, `corpus-report-forgery-matrix.json`. |

## 12. Open Questions

No open questions.
