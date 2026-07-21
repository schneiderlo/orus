# M0 CLI Version and Diagnostics

**Status:** Draft  
**Milestone:** M0  
**Owners:** CLI owner; build owner; release engineer  
**Last updated:** 2026-07-21  
**Depends on:** `specs/10-build-environment.md` and `specs/11-governance-release.md` (Draft; both are Ready prerequisites); Foundation specs (Ready)  
**Foundation decisions:** D-001, D-003  
**Risks addressed:** R-004, R-008, R-009

## 1. Purpose

This domain owns the only M0 user-facing runtime behavior: `orus --version`
and `orus doctor`. It turns immutable build facts and bounded observed host
facts into complete, stable diagnostics without claiming record/replay or
broader platform support.

The consumers are native developers, CI, release reviewers, and operators.
`specs/10-build-environment.md` owns expected values in
`M0-BUILD-FACTS-v1` and `M0-REFENV-v1`; this spec owns rendering, the
finite `M0-DOCTOR-v1` inventory, exit status, and error behavior.

The M0 gate output is a positive fixture covering every mandatory check, one
negative fixture per mandatory check, golden version output, and an
inventory-to-test reconciliation proving there are no undocumented checks.

## 2. In Scope

- Parsing top-level M0 invocations for `--version` and `doctor`.
- Versioned data/output contracts for version facts and doctor results.
- The complete mandatory `M0-DOCTOR-v1` inventory.
- Stable process exit codes and bounded diagnostics.
- Reference/mismatch/unknown/internal-error fixtures.
- Approved-claim scanning of CLI help and results.

## 3. Out of Scope

- Record, replay, inspect, reverse, trace, session, server, Studio, DAP, GDB,
  MCP, or agent commands; these are M2+ or M5+.
- Repairing the host or installing missing components.
- Network calls, telemetry, privilege escalation, ptrace, target launch, or
  filesystem mutation.
- Choosing build/reference facts; owned by
  `specs/10-build-environment.md`.
- Broad compatibility inference from an unvalidated host.

## 4. Functional Requirements

| ID | Requirement | Acceptance criteria | Verification method | Traceability |
|---|---|---|---|---|
| CLI-FR-001 | `orus --version` shall emit one bounded JSON document containing exactly the five non-empty build facts owned by `M0-BUILD-FACTS-v1`. | The JSON object contains product version, source revision, build configuration, compiler identity, and target platform exactly once and with values equal to the embedded facts; it emits no parallel prose or extra document and accepts no M0 format flag; missing/invalid facts return one `CLI_BUILD_FACTS_INVALID` JSON error document and non-zero. | CLI-TEST-001 JSON golden/schema test across dev and release plus one negative fixture per field. | G-06, SM-06, R-008; resolved `q-0005`. |
| CLI-FR-002 | `orus doctor` shall execute every row of `M0-DOCTOR-v1` exactly once. | Result count and stable IDs exactly equal the seven-row inventory in Section 6.2; no row is omitted, duplicated, or added; each result is `pass` or `fail`, never implicit success. | CLI-TEST-002 inventory-to-result reconciliation. | G-06, Charter 2.1. |
| CLI-FR-003 | Doctor shall evaluate expected facts from `M0-REFENV-v1` against bounded observed fact sources. | Reference fixture passes all seven checks; OS, architecture, kernel, CPU/ISA, libc, environment-identity, and build-facts negative fixtures each fail only their intended row unless the fixture declares dependent failures. | CLI-TEST-003 positive and per-row negative integration matrix. | C-03, D-003, R-009. |
| CLI-FR-004 | `orus doctor` shall emit one bounded JSON document exposing the fields defined by `M0-DOCTOR-RESULT-v1`. | The JSON object contains every row with inventory version, stable ID, name, mandatory flag, status, expected predicate/value, observed value or `unavailable`, fact source, stable diagnostic code, and bounded message; unknown observation is `fail` for a mandatory row; no parallel prose/document or M0 format flag exists. | CLI-TEST-004 JSON schema and missing/oversize/unknown/extra-output fixtures. | Charter 2.1, C-06; resolved `q-0005`. |
| CLI-FR-005 | CLI process statuses shall follow the finite M0 exit-code contract. | Valid `--version` and all-pass doctor return 0; usage error returns 2; one or more mandatory doctor failures return 3; invalid embedded contract/internal evaluation failure returns 4; no other M0 exit code is emitted. | CLI-TEST-005 subprocess exit-code matrix. | G-06, DOD-08. |
| CLI-FR-006 | Doctor shall complete all independent checks even after a mandatory check fails, unless the doctor inventory/reference schema itself is invalid. | A host mismatch or invalid `M0-BUILD-FACTS-v1` payload reports all seven rows (with `M0-DOC-001` failed as applicable) and exits 3; an invalid doctor inventory/reference schema stops before host checks, emits one top-level typed error, and exits 4; no failed check is rendered as pass. | CLI-TEST-006 multi-failure, build-facts-failure, and invalid-contract fixtures. | G-06, R-008. |
| CLI-FR-007 | M0 commands shall be read-only, local, deterministic for identical facts, and non-privileged. | System-call/file-access test finds no network, process launch, ptrace, privilege change, persistent write, or host repair; repeated runs over identical fixture facts are byte-identical after excluding no fields (M0 output has no current-time field). | CLI-TEST-007 syscall/access-policy and repeatability test. | D-001, C-11. |
| CLI-FR-008 | Help, errors, and successful output shall make no future-capability or broad-support claim. | Approved-claims scan finds zero record/replay availability, deterministic-execution, or broad Linux support statement; mismatch output uses `unvalidated`, not `unsupported forever` or `supported`. | CLI-TEST-008 golden/claim-scan corpus. | G-07, SM-11, R-004, R-009. |
| CLI-FR-009 | Unsupported argument combinations shall fail before diagnostic evaluation. | No arguments, unknown option/subcommand, `--version` plus another operation, or extra `doctor` operands return exit 2, identify the invalid token without echoing unbounded input, and perform zero host checks. | CLI-TEST-009 argument boundary tests. | DOD-08. |
| CLI-FR-010 | Diagnostic collection shall enforce input/output bounds before allocation or rendering. | Contract/inventory bounds in Section 6 are checked first; strings over 4 KiB, inventory over 64 rows, duplicate IDs, or observed values over 4 KiB produce exit 4 with no partial success document. | CLI-TEST-010 resource-bound and malformed-contract tests. | C-09, R-008, R-201 policy precursor. |

## 5. Non-Functional Requirements

| ID | Quality attribute and target | Conditions / workload | Acceptance criteria | Verification method and evidence |
|---|---|---|---|---|
| CLI-NFR-001 | Version completeness: 5/5 required fields, all non-empty and exact. | Dev and release binaries for the current revision. | 100% schema/golden agreement; every missing/wrong-field fixture fails. | CLI-TEST-001 `version-contract.json`. |
| CLI-NFR-002 | Doctor coverage: 7/7 inventory rows in every evaluable invocation and 100% negative-row coverage. | Reference fixture, one negative fixture per mandatory row, and multi-failure fixture. | Positive result has 7 passes; each row has a dedicated failing test and exit 3; inventory/test reconciliation is exact. | CLI-TEST-002/003 `doctor-coverage.json`. |
| CLI-NFR-003 | Truthfulness: 0 unknown-as-pass and 0 unapproved capability/support claims. | All outputs/help plus unknown-source and claim corpus. | Unknown mandatory facts fail; claim scanner has zero real findings and detects all injected findings. | CLI-TEST-004/008 reports. |
| CLI-NFR-004 | Resource bound: at most 64 inventory rows, 4 KiB per JSON string value, and 256 KiB total UTF-8 JSON output. | Malformed and maximum-sized fixture contracts. | Limits reject before excess allocation; maximum valid fixture completes as one JSON document of at most 256 KiB. | CLI-TEST-010 allocation/size report. |
| CLI-NFR-005 | Repeatability: 100/100 identical-fixture invocations are byte-identical and leak no resource. | Pinned reference fixture; fresh subprocess per invocation. | 100 equal outputs/statuses; zero child process, persistent file, or open-descriptor growth after each process exits. | CLI-TEST-007 repetition report. |

The Charter's future local pause/cancel p99 target is not applicable: M0
introduces no replay session or bulk path. It first becomes applicable in
`specs/60-replay-control-protocol.md` and
`specs/70-gateway-studio.md`.

## 6. Interfaces / Contracts

### 6.1 Invocation and exit contract

| Invocation | Success stdout | Failure output | Exit |
|---|---|---|---|
| `orus --version` | One UTF-8 JSON `M0-VERSION-RESULT-v1` document. | One top-level UTF-8 JSON typed error document to stderr; no partial success or prose. | 0 or 4. |
| `orus doctor` | One UTF-8 JSON `M0-DOCTOR-RESULT-v1` document containing all seven rows. | Ordinary compatibility failures remain one complete JSON result on stdout; invalid contract/internal failure is one JSON typed error on stderr. | 0, 3, or 4. |
| Invalid syntax | No success document. | Bounded usage diagnostic to stderr. | 2. |

The CLI is single-request/single-process. There is no concurrency,
backpressure, retry, or cancellation protocol beyond normal process
termination. Termination creates no cleanup obligation because the commands
own no persistent state. Schemas are M0-local and carry an exact schema ID;
unknown schemas fail. Native C++ layouts are not output contracts.

### 6.2 Normative `M0-DOCTOR-v1` inventory

This table is complete and authoritative. All rows are mandatory and contribute
to exit status. Expected predicates are references into the single
`M0-REFENV-v1` contract; exact version coordinates are not duplicated here.

| Stable check ID | Human name | Fact source | Expected predicate | Structured failure code |
|---|---|---|---|---|
| `M0-DOC-001` | Build facts integrity | Embedded `M0-BUILD-FACTS-v1` | Schema valid; all five facts non-empty; target/config/toolchain values internally consistent. | `DOCTOR_BUILD_FACTS_INVALID` |
| `M0-DOC-002` | Operating system | Kernel `uname` equivalent, without shelling out | Exact OS family predicate from `M0-REFENV-v1` (Linux for M0). | `DOCTOR_OS_MISMATCH` |
| `M0-DOC-003` | Machine architecture | Kernel machine fact, without shelling out | Exact architecture predicate from `M0-REFENV-v1` (x86-64 for M0). | `DOCTOR_ARCH_MISMATCH` |
| `M0-DOC-004` | Kernel contract | Kernel release/version facts | Satisfies the exact pinned reference predicate in `M0-REFENV-v1`. | `DOCTOR_KERNEL_MISMATCH` |
| `M0-DOC-005` | CPU/ISA contract | Bounded CPUID/kernel-exposed CPU facts | Vendor/model and required ISA predicates in `M0-REFENV-v1`; optional extensions do not imply support. | `DOCTOR_CPU_MISMATCH` |
| `M0-DOC-006` | C library contract | Loaded libc identity/version through declared runtime API | Satisfies the libc predicate in `M0-REFENV-v1`; unavailable is fail. | `DOCTOR_LIBC_MISMATCH` |
| `M0-DOC-007` | Reference environment identity | Embedded build environment ID plus normalized observed facts from rows 2-6 | Embedded ID equals the contract ID and all contributing observed facts are available and match. | `DOCTOR_REFENV_MISMATCH` |

`M0-DOC-007` is an explicit aggregate check: a component mismatch may cause
its component row and the aggregate row to fail. Dedicated negative fixtures
declare that dependency; this is not duplicate or accidental failure.

### 6.3 Result fields and bounds

`M0-VERSION-RESULT-v1` is one UTF-8 JSON object containing `schema` plus the
five strings from `M0-BUILD-FACTS-v1`, each at most 4 KiB and exactly once.

`M0-DOCTOR-RESULT-v1` is one UTF-8 JSON object containing `schema`, `inventory`,
`overall_status: pass|fail`, and `checks`. Each check contains
`id`, `name`, `mandatory=true`, `status: pass|fail`,
`fact_source`, `expected`, `observed` (string or `unavailable`),
`code`, and `message`. The renderer uses one deterministic key order so
identical facts are byte-identical. No current time, hostname, username, path,
raw environment block, secret, trailing prose, or second JSON value is emitted.

### 6.4 Design choice: output representation

| Alternative | Pros | Cons |
|---|---|---|
| One JSON document by default (selected) | Exact schema, robust golden/consumer parsing, complete doctor aggregation, and one public surface. | Less conventional for a human-only `--version`; correct escaping needs dedicated tests. |
| Stable `key=value` lines | Easy to read and shell parse. | Nested doctor rows and escaping/typing are awkward. |
| Human prose plus separate format flag | Best interactive presentation and machine contract. | Adds a public option not requested for M0 and doubles golden surfaces. |

**Recommendation:** Use one bounded JSON document per invocation without an
additional M0 format flag or parallel human-text contract. This implements the
human answer to `q-0005` and keeps one golden-test surface.

## 7. Data Model

| Entity | Identity / fields | Lifecycle and invariants |
|---|---|---|
| Invocation | Process-local operation enum and bounded argv. | `parsed -> evaluating -> rendered -> exited`; syntax failure skips evaluation. |
| Build facts | Embedded `M0-BUILD-FACTS-v1`. | Immutable; validation precedes rendering. |
| Reference contract | Embedded/packaged `M0-REFENV-v1` and content ID. | Immutable for binary; exactly one contract accepted. |
| Observed fact | Check ID, source, bounded typed/string value or unavailable. | Read once per invocation; never persisted; unavailable is not pass. |
| Check result | Inventory ID, expected, observed, status, code, message. | Exactly one per inventory row when contract is valid. |
| Doctor result | Inventory version, ordered check results, aggregate status. | Pass iff every mandatory row passes; order is inventory order. |

Expected contract data is authoritative for the M0 support claim. Observed host
facts are evidence for one invocation. Human messages are views over typed
codes and do not change result state.

## 8. Key Flows

1. **Version success (CLI-FR-001).** Parse exact invocation; validate embedded
   schema/bounds; copy the five facts into the result contract; render once;
   exit 0. Invalid build facts produce the top-level version error and exit 4.
2. **Doctor success (CLI-FR-002 through CLI-FR-007).** Parse; validate the
   doctor inventory/reference schema; evaluate build facts and inventory rows in stable order using
   local bounded sources; assemble seven pass rows; render aggregate pass; exit
   0.
3. **Ordinary mismatch (CLI-FR-003 through CLI-FR-006).** A fact differs or is
   unavailable; mark its row fail; continue independent checks; mark aggregate
   reference row as applicable; render all rows; exit 3 without mutation.
4. **Cancellation/termination (CLI-FR-007).** The OS terminates the process at
   any point; no child/persistent resource exists; a new invocation starts
   from immutable contracts and recollects facts.
5. **Malformed/unsupported invocation (CLI-FR-009, CLI-FR-010).** Reject
   syntax before checks with exit 2, or reject invalid internal schema/bounds
   before partial rendering with exit 4.

## 9. Failure Modes

| ID | Trigger | Required detection point | Typed outcome / diagnostic fields | Side effects and cleanup | Retry / recovery | Verifying requirements/tests |
|---|---|---|---|---|---|---|
| CLI-FAIL-001 | Embedded build fact is empty, malformed, or inconsistent. [R-008] | Version contract validation or doctor row `M0-DOC-001`. | Version: `CLI_BUILD_FACTS_INVALID`, field/reason, exit 4 with no success document. Doctor: failed `M0-DOC-001` row with `DOCTOR_BUILD_FACTS_INVALID`, all independent rows, exit 3. | Read-only; no persistent state. | Rebuild from valid declared inputs. | CLI-FR-001, -003, -006 / CLI-TEST-001, -003, -006. |
| CLI-FAIL-002 | Mandatory observed fact differs or is unavailable. [R-009] | Owning inventory row. | Row-specific `DOCTOR_*_MISMATCH`; expected, observed/unavailable, source; exit 3 after full inventory. | Read-only; all independent results retained in output. | Use reference environment or treat host as unvalidated. | CLI-FR-003, -004, -006 / CLI-TEST-003. |
| CLI-FAIL-003 | Inventory/reference schema has unknown version, duplicate ID, or exceeds bound. [R-008, R-201] | Before fact collection/allocation. | `CLI_CONTRACT_INVALID`; schema, field/index, limit; exit 4. | No partial result; bounded error only. | Install/rebuild a valid artifact. | CLI-FR-010 / CLI-TEST-010. |
| CLI-FAIL-004 | Invalid argument or combination. | Parse boundary. | `CLI_USAGE_ERROR`; bounded token and valid operations; exit 2. | No checks or writes. | Correct invocation. | CLI-FR-009 / CLI-TEST-009. |
| CLI-FAIL-005 | Renderer cannot encode a bounded valid result. | Before stdout commit. | `CLI_RENDER_ERROR`; schema and cause; exit 4. | No partial success document; temporary memory released on exit. | Internal defect; rebuild after fix. | CLI-FR-004 / CLI-TEST-004. |
| CLI-FAIL-006 | Help/output contains prohibited future claim. [R-004] | CI/release claim scan, before publication. | `GOV_UNAPPROVED_CLAIM`; CLI surface and rule. | Release candidate rejected; local binary unchanged. | Correct wording or supersede scope decision. | CLI-FR-008 / CLI-TEST-008. |

## 10. Observability

| Signal ID | Metric/event/log | Type, unit, and labels | Emission point / owner | Cardinality and cost bound | Alert/gate use | Verification |
|---|---|---|---|---|---|---|
| CLI-OBS-001 | `cli_invocation_result` | Process result; operation, exit code, output schema | Process exit / CLI owner | Exactly one terminal result; not networked or persisted by CLI. | Integration diagnosis. | CLI-TEST-005. |
| CLI-OBS-002 | `doctor_check_result` | Structured output row; inventory/check ID, status, code | Check evaluation / CLI owner | Exactly seven rows for valid contract; strings bounded 4 KiB. | SM-06 gate and operator diagnosis. | CLI-TEST-002/003. |
| CLI-OBS-003 | `doctor_inventory_coverage` | Test gauge; inventory_rows, tested_positive, tested_negative | CI reconciliation / quality owner | One aggregate per revision; stable seven-row inventory. | Release-blocking coverage. | CLI-TEST-002. |
| CLI-OBS-004 | `cli_output_size_bytes` | Test histogram/gauge, bytes by operation/fixture | Test harness / CLI owner | Two operations times bounded fixture set; not runtime telemetry. | Resource-limit gate. | CLI-TEST-010. |

Build/environment identities appear in the structured result; source revision
correlates it to release evidence. M0 has no request, trace, branch, execution,
VPID, or VTID. Golden documents and subprocess results are retained as M0 test
evidence; the CLI itself sends no telemetry.

## 11. Test & Verification Plan

Copy/paste from the repository root after implementation:

```bash
nix develop --command bazel build --config=dev //cli:orus
nix develop --command bazel test --config=dev //tests/cli/...
nix develop --command bazel test --config=asan //tests/cli/...
nix develop --command bazel test --config=ubsan //tests/cli/...
nix develop --command bazel run //cli:orus -- --version
nix develop --command bazel run //cli:orus -- doctor
nix develop --command bazel run //tools/governance:claim_scan -- --scope=cli
```

| Requirement ID | Test/benchmark/review ID | Level | Fixture/workload and environment | Pass criterion | Evidence artifact |
|---|---|---|---|---|---|
| CLI-FR-001, CLI-NFR-001 | CLI-TEST-001 | Unit/golden/integration | Dev/release build facts and one missing/wrong fixture per field. | 5/5 exact on valid; every invalid fixture exits 4. | `version-contract.json`, golden outputs. |
| CLI-FR-002, CLI-NFR-002 | CLI-TEST-002 | Unit/analysis | Inventory, result, and registered negative tests. | Exactly seven unique rows/results/tests. | `doctor-inventory-reconciliation.json`. |
| CLI-FR-003 | CLI-TEST-003 | Integration | Reference and dedicated negative fixture for each of seven rows. | Reference exits 0; each negative reports intended failures and exits 3. | `doctor-fixture-matrix.json`. |
| CLI-FR-004 | CLI-TEST-004 | Schema/negative | Pass/fail/unavailable, missing, invalid enum, oversize, renderer-fault fixtures. | Valid schemas pass; every malformed result rejected without partial output. | `doctor-schema-tests.xml`. |
| CLI-FR-005 | CLI-TEST-005 | Integration | Every documented exit path. | Only 0, 2, 3, 4 as specified. | `cli-exit-codes.json`. |
| CLI-FR-006 | CLI-TEST-006 | Integration | Multi-mismatch, invalid-build-facts, and invalid doctor inventory/reference-schema fixtures. | Mismatch/build-facts failure renders all seven rows/exit 3; invalid doctor contract renders no success/exit 4. | `doctor-completion-policy.json`. |
| CLI-FR-007, CLI-NFR-005 | CLI-TEST-007 | End-to-end/security/reliability | 100 reference invocations under syscall/file-access observation. | Byte-identical output/status; zero forbidden capability or leaked resource. | `cli-repeatability.json`, access report. |
| CLI-FR-008, CLI-NFR-003 | CLI-TEST-008 | Static/golden/negative | Help/output/error corpus plus injected claims. | Zero real findings; all prohibited fixtures detected; unknown never pass. | `cli-claims.sarif`. |
| CLI-FR-009 | CLI-TEST-009 | Boundary/integration | Empty, unknown, combined, extra-operand, and overlong token invocations. | Exit 2, bounded diagnostic, zero check evaluation. | `cli-argument-boundaries.json`. |
| CLI-FR-010, CLI-NFR-004 | CLI-TEST-010 | Resource/security | Maximum-valid and over-limit contract/string/row fixtures under allocation instrumentation. | Max valid stays within 256 KiB output; over-limit rejects before excess allocation. | `cli-resource-limits.json`. |

## 12. Open Questions

No open questions.
