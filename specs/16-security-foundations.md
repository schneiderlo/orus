# Security Foundations and Future Trust Boundaries

**Status:** Draft  
**Milestone:** M0  
**Owners:** Security owner; build/release owner; CI owner; future domain owners  
**Last updated:** 2026-07-21  
**Depends on:** `specs/10-build-environment.md`, `specs/11-governance-release.md`, `specs/13-ci-quality.md`, `specs/14-performance-foundation.md`, and `specs/15-concurrent-corpus.md` (Draft; all are Ready prerequisites); Foundation specs (Ready)  
**Foundation decisions:** D-007, D-008, D-013, D-015  
**Risks addressed:** R-001, R-004, R-005, R-201, R-202, R-302, R-303

## 1. Purpose

This domain establishes the M0 security baseline and names the trust,
capability, integrity, sensitive-data, resource-limit, and fuzzing contracts
that later domains refine before accepting untrusted targets, traces,
symbols, browser data, or model-visible content.

M0 enforcement is limited to repository/build/dependency/CI/release inputs and
the structured parsers introduced by M0. This spec does not claim that a target,
trace, replay worker, symbol parser, Studio, or model is sandboxed because none
exists in M0.

The consumers are all engineering and release roles and every M1+ domain owner.
The M0 gate output is a versioned threat-boundary/capability/resource inventory,
zero-secret and least-privilege evidence, artifact/SBOM integrity evidence,
M0 parser/fuzz coverage, and a claim scan proving no unimplemented isolation
control is presented as available.

## 2. In Scope

- M0 threat model for repository, dependency, Nix/Bzlmod acquisition, build
  action, CI, cache, artifact, SBOM, release evidence, and diagnostics.
- Secret exclusion/scanning, CI least privilege, and cache/artifact integrity.
- Dependency/build trust admission and verified input digests.
- Stable capability vocabulary and default-deny ownership model.
- Future untrusted target, trace, symbol, browser, gateway, model, repository,
  network, and publication boundaries with named owner milestones.
- Resource-limit schema and requirement that limits precede allocation/side
  effect.
- Structured-input inventory, negative tests, fuzz-smoke/continuous-fuzz
  policy, corpus management, and sanitizer integration.
- Sensitive trace/model-content principles and honest security claims.

## 3. Out of Scope

- Implementing a target, trace, replay, symbol, gateway, browser, or model
  sandbox.
- Selecting Linux namespace/seccomp/LSM/container mechanisms for future
  workers; those require domain threat models and ADRs.
- Provisioning signing keys, remote caches, production secrets, encryption
  services, key management, or network services.
- Claiming M0 resists a malicious kernel, CPU, Nix daemon, or authorized
  maintainer.
- Defining trace redaction/encryption/retention behavior before trace/production
  domains; M0 records the boundary and owner.
- Granting an agent, workflow, or release process authority to publish or mutate
  external state.

## 4. Functional Requirements

| ID | Requirement | Acceptance criteria | Verification method | Traceability |
|---|---|---|---|---|
| SEC-FR-001 | The repository shall maintain `M0-SECURITY-BOUNDARIES-v1` covering every M0 and named future trust boundary in Section 6.1. | Each row identifies trusted/untrusted side, assets, entry format/capability, threat classes, M0 control/state, future owner, validation, residual risk, and claim wording; no row is omitted from roadmap-owner reconciliation. | SEC-TEST-001 schema and roadmap boundary audit. | C-09, D-013, R-201 through R-205, R-302/303. |
| SEC-FR-002 | Every byte source and every raw path/source-identity string in the finite pull-request and release populations of Section 6.5 shall contain zero unallowlisted secret/credential. | Gitleaks v8.30.1 is pinned by exact version/artifact digest and runs offline with committed rules plus bounded supplemental byte rules. Before each final scan, `M0-SECRET-SCAN-METADATA-v1` freezes every raw `logical_path` and `source_identity`, includes its own tuple, and is scanned as an ordinary generated/evidence entry. The excluded `M0-SECRET-SCAN-MANIFEST-v1` binds that metadata document by SHA-256 and contains only per-string SHA-256 identities plus fixed digest-only display values, never raw unmatched strings. Both contracts reconcile 100% of the named history/tree/generated/package-closure/log/artifact/cache/evidence sets; token/key/password/private-key fixtures, including canaries present only in filenames or source identities, are detected. The committed Gitleaks/supplemental fixture corpus has <=10,000 fixtures, <=1 MiB each, <=1 GiB total, and its scan job uses <=256 MiB RSS/300 seconds. Unavailable, unsupported, truncated, unmanifested, over-limit, unredacted, or metadata-unbound input fails the gate rather than shrinking the population. Allowlisting requires exact digest/path/rule, non-secret proof, owner, and review gate. | SEC-TEST-002 both profile metadata/manifests, scanner identity, raw-string/population reconciliation, fixture-corpus resource limits, incompleteness/bound/redaction, and fixture matrix. | C-11, R-005, R-202; resolved `q-0010`. |
| SEC-FR-003 | M0 CI/build shall use the exact least-privilege acquisition/build split and no protected secret on untrusted changes. | Pull-request and protected build/test/package jobs have read-only contents, no write/OIDC/package/publish/credential/privileged-cache/network capability. Only `m0_input_acquisition` has `network.client`; its profile ID is byte-identical to the workflow job/profile and spec-`10` ID, it fetches admitted hash-pinned inputs as data, executes no repository action, and has no credential. Canary, profile-alias, and network-boundary simulations report zero exposure/escape. | SEC-TEST-003 workflow capability/profile-identity/canary/acquisition-boundary test. | C-11, D-010, R-001, R-005. |
| SEC-FR-004 | Every acquired build/dependency input shall be immutable, hash-verified, admitted, and represented in release evidence. | Nix/Bzlmod locks include immutable identity/hash; 100% resolved dependencies reconcile to admission/SBOM; hash mismatch, mutable reference, network fetch during action, or unadmitted dependency fails before artifact approval. | SEC-TEST-004 supply-chain reconciliation/fault matrix. | G-02, SM-10, R-001, R-005. |
| SEC-FR-005 | Release objects shall use the subject-named lowercase SHA-256 identities and exact bytes from spec `10` Section 6.4. | Digest records use `algorithm=sha256`, `subject=orus_executable|package_tree|sbom|evidence_object`, and 64 lowercase hex digits. Raw executable, canonical package manifest (including/excluding exact metadata), completed external SBOM, and exact canonical evidence bytes are never interchangeable. Byte/metadata mutation, subject substitution, missing/mixed/malformed/alternate algorithm, self-reference, or cross-link mismatch fails; digest is never called a signature. | SEC-TEST-005 encoding, metadata, tamper, self-reference, and subject-substitution matrix. | DOD-03, R-005, R-008; resolved `q-0011`. |
| SEC-FR-006 | Security-sensitive operations and future interfaces shall use `M0-CAPABILITY-VOCABULARY-v1` and default deny. | Every M0 profile and future owner row declares grants/denials; absence/unknown fails. The only M0 `network.client` grant is `m0_input_acquisition` under Sections 6.2 and 6.5; all build/test/untrusted-code profiles deny network. No M0 profile has target-control, trace/replay, repository-write, credential, model, privileged-cache, or publish capability; test-owned subprocess launch is narrowly separate. | SEC-TEST-006 exact profile reconciliation and overgrant/network-escape fixtures. | C-09, D-008, D-013, R-001, R-303. |
| SEC-FR-007 | Every M0 structured/resource-bearing input shall match exactly one finite `M0-RESOURCE-LIMIT-v1` row in Section 6.3 before Ready. | Rows SEC-LIM-10-01 through SEC-LIM-16-04 cover every applicable build/reference/acquisition/package, governance/release, CLI, CI, performance, corpus, and security input/operation with numeric limits, pre-side-effect detection, error, cleanup, owner requirement, and test; explicit not-applicable rationales are finite. Missing/unbounded/duplicate/late-enforcement rows block readiness. | SEC-TEST-007 cross-domain owner-requirement/operation reconciliation matrix and mutations. | C-09, D-007, DOD-08, R-201. |
| SEC-FR-008 | Every untrusted structured parser shall register negative, boundary, resource, sanitizer, and fuzz verification before becoming Active. | `M0-STRUCTURED-INPUT-v1` maps format/schema, trust source, parser target, limits, seed corpus, dictionary when applicable, fuzz target, sanitizer configs, minimum smoke execution >0, continuous owner, crash retention, and promotion rule; missing mapping blocks applicability gate. | SEC-TEST-008 parser/fuzz inventory and zero-execution fixture. | C-09, D-015, R-201. |
| SEC-FR-009 | M0 native structured inputs shall receive the exact verification assigned in Section 6.4. | Performance result parser and corpus IPC frame parser have fuzz-smoke under ASan/UBSan-compatible fuzz config, positive execution count, malformed/resource corpus, and zero crash/sanitizer finding; their owner-spec resource contracts apply unchanged. Governance/CI schemas implemented through memory-safe validators have exhaustive schema negatives and must add fuzzing if native parsing is introduced. | SEC-TEST-009 M0 parser applicability, owner-limit reconciliation, and fuzz reports. | DOD-04, R-007, R-201. |
| SEC-FR-010 | Future target/trace/symbol/browser/model content shall remain untrusted data and shall not redefine policy or become factual authority. | Boundary inventory states content provenance, read/execute/parse capability, isolation owner, limit/fuzz owner, evidence authority, and prompt-injection rule; raw content cannot grant capabilities; model-visible content is data and factual claims require evidence under D-008. | SEC-TEST-010 future-boundary policy inspection and adversarial policy fixture. | D-008, R-202, R-302. |
| SEC-FR-011 | M0 documentation, CLI, package, and release metadata shall make no implemented sandbox/security claim for future components. | Claim scanner finds zero statement that target/trace/symbol/model isolation, encryption, redaction, sandbox, or deterministic fail-closed runtime is available in M0; policy/future-tense statements are permitted and tagged with owner milestone. | SEC-TEST-011 approved-claims scan and future-tense/prohibited fixtures. | D-001, R-004. |
| SEC-FR-012 | Security control failures shall block the affected CI/release gate and emit bounded non-secret typed diagnostics. | Secret, permission, digest, dependency, capability, resource, parser/fuzz, or claim failure produces a stable code with rule/boundary/artifact identity and redacted context, returns non-zero, creates no approved marker, and retains a report <=8 MiB with <=1,000 details, <=4 KiB per diagnostic, and zero raw secret. | SEC-TEST-012 end-to-end control failure, 8-MiB/detail/diagnostic boundaries, and redaction tests. | DOD-08, R-005, R-201. |
| SEC-FR-013 | Security exceptions shall be narrow, expiring, owner-approved records rather than silent suppression. | Each exception has stable ID, exact control/rule/resource and path/target/config scope, rationale, compensating control, evidence, owner, created revision, and mandatory review milestone; wildcard, ownerless, evidence-free, or overdue exception fails the gate. | SEC-TEST-013 exception-schema and invalid fixture suite. | DOD-07, Charter 11. |
| SEC-FR-014 | Changes to the primary secret scanner or canonical content digest shall use the governed admission/ADR path. | If Gitleaks upstream security maintenance ends, dependency admission reevaluates replacement/removal before a scanner change; any SHA-256 migration or hot-path adoption of BLAKE3 has an accepted ADR covering compatibility, dual-read/migration if applicable, dependency/security/performance evidence, and rollback; no tool silently emits a second canonical identity. | SEC-TEST-014 scanner-maintenance and digest-migration governance fixtures. | DOD-07, R-005; resolved `q-0010`, `q-0011`. |

## 5. Non-Functional Requirements

| ID | Quality attribute and target | Conditions / workload | Acceptance criteria | Verification method and evidence |
|---|---|---|---|---|
| SEC-NFR-001 | Secret exposure: 0 unallowlisted secrets across 100% of both Section 6.5 scan profiles, including retained metadata and excluded final control-pair fields. | Gitleaks v8.30.1 offline rules plus supplemental bytes rules; exact PR range/tree/run outputs and release reachable-history/tree/generated/package-closure/log/artifact/cache/evidence populations; frozen pre-scan raw-string metadata. | Scanner/rules identities exact; every metadata/population entry scans once; final manifest binds the scanned metadata digest and contains zero raw path/source string; zero unavailable/truncated/unmanifested/over-limit set; real count zero; 100% content/filename/source-identity fixtures detected and final-pair displays remain digest-only. | SEC-TEST-002/003 `secret-scan-report.json`, metadata, manifests. |
| SEC-NFR-002 | Supply-chain completeness: 100% resolved build/runtime components are hash-pinned, admitted, and SBOM-reconciled. | Release configuration dependency graph. | Zero mutable, unverified, unadmitted, or unreconciled component. | SEC-TEST-004 supply-chain report. |
| SEC-NFR-003 | Artifact integrity: 100% subject-named release references verify and 100% byte/included-metadata/substitution mutations are detected. | Executable, package tree, SBOM, license/notices, CI/corpus/performance/security evidence. | Zero missing/malformed/mismatch/generic subject; every mutation/substitution/alternate-algorithm/self-reference fixture blocks. | SEC-TEST-005 tamper matrix. |
| SEC-NFR-004 | Capability least privilege: exactly 1 scoped M0 `network.client` profile and 0 forbidden grants elsewhere. | Complete Section 6.2 component/job profile inventory. | `m0_input_acquisition` is the sole network grant and cannot execute repository code/read credential; target control, trace/replay, repository write, model, privileged cache, credential, and publication grant counts are zero; every mutation fails. | SEC-TEST-006 capability report. |
| SEC-NFR-005 | Structured-parser coverage: 100% registered untrusted M0 native parsers have limits, negative/resource tests, ASan/UBSan-compatible fuzz smoke with >0 executions and 0 crash/sanitizer finding. | `M0-STRUCTURED-INPUT-v1` versus Bazel/parser/fuzz target query. | No unmapped native parser; all required fuzzers execute and pass. | SEC-TEST-008/009 parser coverage report. |
| SEC-NFR-006 | Security-claim truthfulness: 0 implemented-isolation/encryption/redaction claim for M1+ behavior. | CLI/help/docs/package/release surfaces. | Zero non-allowlisted claim; all prohibited fixtures detected. | SEC-TEST-011 claim report. |
| SEC-NFR-007 | Diagnostic bound: <=4 KiB per security diagnostic, <=1,000 detailed findings/report before `findings_truncated=true`, and 0 raw secret value. | Maximum and over-limit scan/control fixtures. | Summary/count/first bounded findings retained; limits enforced; secret canary redacted. | SEC-TEST-012 resource/redaction report. |

Future sandbox overhead, trace-data volume, and model budgets are not
applicable in M0 because the components do not exist. Each introducing domain
must define measurable security/resource/performance targets before Ready;
this spec cannot waive them.

## 6. Interfaces / Contracts

### 6.1 `M0-SECURITY-BOUNDARIES-v1`

| Boundary ID | Untrusted/less-trusted input -> owner | Protected assets / M0 control | Future enforcement owner |
|---|---|---|---|
| `SEC-BND-001` | Nix/Bzlmod/upstream dependency -> build graph | Source/release integrity; immutable locks, hashes, admission, offline actions. | M0 `10`, `11`, this spec. |
| `SEC-BND-002` | Repository change/untrusted PR -> CI job | Secrets, workflow token, cache, release evidence; read-only/no-secret jobs, pinned actions. | M0 `13`, this spec. |
| `SEC-BND-003` | Build action -> release artifact/evidence | Artifact identity, license/SBOM/provenance; sandbox, digest, reconciliation. | M0 `10`, `11`, this spec. |
| `SEC-BND-004` | M0 structured bytes -> parser/tool | Process memory/availability and truthful result; pre-allocation limits, negatives, fuzz/sanitizers. | M0 `14`, `15`, this spec. |
| `SEC-BND-101` | Untrusted native target -> task-control/recorder | Host, other processes, trace truth; fail-closed capabilities/limits/isolation. Policy only in M0. | M1 `20`; M2 `31`, `32`; M7 `80`. |
| `SEC-BND-102` | Untrusted/corrupt/sensitive trace -> trace/replay/index | Worker memory/CPU/storage, execution truth, data confidentiality. Policy only in M0. | M2 `30`; M3 `40`, `41`; M8 `90`. |
| `SEC-BND-103` | Untrusted ELF/DWARF/source/expression -> symbol worker | Worker/session integrity and availability. Policy only in M0. | M5 `61`. |
| `SEC-BND-104` | Browser/network input -> gateway/session | Replay control, trace data, credentials, availability. Policy only in M0. | M6 `70`. |
| `SEC-BND-105` | Target/source/log/tool/model content -> investigation/model adapter | Policy, capabilities, evidence truth, repository/network/publication. Policy only in M0. | M9 `100`, `101`; M10 `111`. |
| `SEC-BND-106` | Production trace/telemetry -> storage/correlation | Credentials, personal/proprietary data, encryption/retention/access. Policy only in M0. | M11 `120`, `121`. |

Every row also carries threat classes, concrete validation, residual risk, and
claim state in the machine-readable inventory. Future status remains
`policy_only_not_implemented` until the owning gate supplies evidence.

### 6.2 `M0-CAPABILITY-VOCABULARY-v1`

Capabilities are stable lower-case dot-separated identifiers. Initial finite
vocabulary:

| Capability | Meaning |
|---|---|
| `repository.read`, `repository.write` | Read or mutate the scoped source workspace. |
| `build.input.read`, `build.action.execute`, `artifact.write` | Consume declared inputs, execute declared local build actions, create scoped artifacts. |
| `network.client`, `credential.read`, `cache.privileged_write` | External network, protected credential, or privileged shared-cache authority. |
| `test.process.launch` | Launch only an owned benchmark/corpus test subprocess under its declared process/deadline limits; never an Orus-controlled target. |
| `target.launch`, `target.control`, `target.memory.read`, `target.memory.write` | Future native target operations. |
| `trace.read`, `trace.create`, `trace.mutate` | Future trace operations; published traces are immutable, so mutate is normally denied. |
| `replay.session.read`, `replay.session.mutate` | Future replay-state access/mutation. |
| `symbol.parse`, `model.invoke` | Future isolated symbol/expression and optional model operations. |
| `external.publish` | Release/PR/deploy or other externally visible publication. |

A grant record contains component/profile, capability, scope, resource limits,
owner, reason, evidence, and milestone. Unknown/absent is denied. The complete
M0 profiles are:

| Profile | Granted | Explicitly denied / invariant |
|---|---|---|
| `m0_input_acquisition` | `repository.read` for lock/admission manifests, `build.input.read`, scoped `artifact.write`, `network.client` | No `build.action.execute`, `test.process.launch`, credential/cache privilege, repository write, target/trace/replay/model/publish. Only admitted immutable coordinates and hash-verified store promotion. |
| `m0_untrusted_pr_build`, `m0_protected_build` | `repository.read`, `build.input.read`, `build.action.execute`, scoped `artifact.write`; `test.process.launch` only for registered perf/corpus jobs | No network/credential/cache privilege/repository write/target/trace/replay/model/publish. Read-only acquired store. |
| `m0_release_validator` | `repository.read`, `build.input.read`, scoped `artifact.write` | No network, build execution, credential, repository write, package/sign/publish, or future capability. Creates only a local approval marker. |
| `m0_cli` | `build.input.read` limited to embedded/packaged contracts | All mutation, process-launch, network, credential, target/trace/replay/model/publish denied. |

The acquisition profile is trusted tooling, not untrusted repository code; an
untrusted lock change without an admitted record cannot select its network
destination. This vocabulary is a policy contract, not proof of future
enforcement. The `m0_input_acquisition` spelling is a stable identity, not a
display name: specs `10`, `13`, and `16`, the workflow job ID, and every grant
record must contain that exact UTF-8 byte sequence.

### 6.3 Resource and exception contracts

`M0-RESOURCE-LIMIT-v1` fields: schema, domain/operation/parser, input trust,
status `applicable|not_applicable`, units, maximum input bytes/count/depth,
maximum derived allocation/memory, process/thread/FD, CPU/time,
storage/queue/I/O as applicable, enforcement point, typed error, cleanup,
tests, owner, and introducing requirement. Unbounded is invalid unless the row
is `not_applicable` and proves that the operation cannot consume that resource.

The following table is the complete M0 inventory. `MiB`/`GiB` are powers of
1024. Limits are checked before proportional allocation, process creation,
store promotion, upload, or approval. A cited owner section is normative; the
security row may not weaken it.

| Limit ID | Domain operation / input | Exact limits (N/A rationale where scoped) | Detection, typed outcome, cleanup | Owner requirement / reconciliation test |
|---|---|---|---|---|
| `SEC-LIM-10-01` | Nix/Bzlmod input acquisition | <=128 coordinates, <=4 GiB/fetched blob, <=16 GiB total, <=1,200 s; one acquisition process; repository build execution N/A because forbidden in profile | Validate manifest/admission/hash/size before store promotion; `BUILD_ACQUISITION_DENIED`; delete quarantine | Spec `10` 6.1/BUILD-FR-008; BUILD-TEST-008, SEC-TEST-004/007 |
| `SEC-LIM-10-02` | Reference/observed environment JSON | contract <=64 KiB/depth 8/128 inputs; observed <=32 KiB; strings <=256 bytes; validator <=64 MiB RSS/10 s; process/thread/FD N/A: in-process cold parser opens only declared input | Byte/count/depth before parse; RSS/deadline first-over is exactly `BUILD_REFENV_RESOURCE_LIMIT`; release buffers | Spec `10` BUILD-FR-010/6.3; BUILD-TEST-010, CLI-TEST-003/010 |
| `SEC-LIM-10-03` | Package-tree identity walk | <=100,000 entries, path/target <=4 KiB, regular bytes <=16 GiB, RSS <=256 MiB, one streaming process, <=1,200 s | Check type/path/count/size before hash; `BUILD_PACKAGE_IDENTITY_INVALID`; close handles/delete temporary manifest | Spec `10` 6.4/BUILD-FR-011; BUILD-TEST-011 |
| `SEC-LIM-11-01` | License/notice validation | each file <=64 KiB, <=100,000 notice/package entries, index <=16 MiB/depth 16, diagnostic <=4 KiB | Size/count before read/index; `GOV_FIELD_BOUND`; no approval marker | Spec `11` GOV-FR-001/-007; GOV-TEST-001/-007 |
| `SEC-LIM-11-02` | Dependency-admission record | <=1 MiB/depth 16, <=256 transitives/record, strings <=4 KiB, validator <=64 MiB RSS/30 s | Canonical bytes/schema before graph insertion; RSS/deadline first-over is exactly `GOV_RESOURCE_LIMIT`; discard candidate record | Spec `11` GOV-FR-004/6.1.1/6.1.5; GOV-TEST-004 |
| `SEC-LIM-11-03` | SPDX SBOM/descriptor | descriptor <=64 KiB; SBOM <=16 MiB/depth 32, <=100,000 components/files, <=200,000 relationships, validator <=256 MiB/120 s | Bounds before graph allocation; encoded field/count/depth excess is `GOV_FIELD_BOUND`, RSS/deadline first-over is exactly `GOV_RESOURCE_LIMIT`; no descriptor/approval | Spec `11` GOV-FR-006/6.1.2/6.1.5; GOV-TEST-006 |
| `SEC-LIM-11-04` | Release evidence | <=16 MiB/depth 16, <=12 evidence refs, <=12 validators, <=3 approvals, <=4 KiB diagnostic, validator <=256 MiB/120 s | Bounds/identity before final scan/marker; encoded field/count/depth excess is `GOV_FIELD_BOUND`, RSS/deadline first-over is exactly `GOV_RESOURCE_LIMIT`; remove temporary index | Spec `11` GOV-FR-008/-010 and 6.1.3/6.1.5; GOV-TEST-008/-010 |
| `SEC-LIM-11-05` | Approved claims | <=1 MiB/depth 16, <=4,096 rules, <=64 paths/rule, pattern <=1 KiB, scanner report <=1,000 findings | Validate schema/RE2 before scan; Section 6.1.5 error; candidate blocked | Spec `11` 6.1.4; GOV-TEST-009 |
| `SEC-LIM-12-01` | Embedded build/reference/observed CLI inputs | build strings <=4 KiB, reference limits from `SEC-LIM-10-02`, inventory <=64 rows, input depth <=16, allocation <=16 MiB, one process/no child/FD growth, <=10 s | Validate before observation/allocation; `CLI_CONTRACT_INVALID`; process-local memory released | Spec `12` CLI-FR-006/-010 and 6.3; CLI-TEST-006/-010 |
| `SEC-LIM-12-02` | CLI result/error output | <=4 KiB/string, argument token <=256 bytes, seven doctor rows (absolute schema cap 64), document <=256 KiB, exactly one LF, one process <=10 s | Bound before atomic stream commit; `CLI_RENDER_ERROR`/exit 4; no partial output | Spec `12` 6.1/6.3; CLI-TEST-004/-009/-010 |
| `SEC-LIM-13-01` | CI applicability/warning manifests | each <=4 MiB/depth 16, <=100,000 expanded target cells, <=100,000 warning entries, strings <=4 KiB, parser <=256 MiB/120 s | Validate before matrix/job generation; applicability excess is exactly `CI_APPLICABILITY_INVALID`; warning input/allowlist or warning-parser RSS/deadline excess is exactly `CI_WARNING_BLOCKED`; no jobs generated | Spec `13` CI-FR-003/-007 and 6.1/6.2; CI-TEST-003/-007 |
| `SEC-LIM-13-02` | CI typed report/diagnostic/findings | report <=8 MiB, diagnostic <=4 KiB, details <=1,000, limit doc <=64 KiB | Producer limit before upload; `CI_EVIDENCE_LIMIT`; blocking job fails | Spec `13` 6.3; CI-TEST-011 |
| `SEC-LIM-13-03` | CI retained log/evidence bundle | log <=32 MiB retained as 16-MiB head+tail, bundle <=64 MiB uncompressed; one job process group and provider timeout <=120 min | Bound before upload; truncate/reject exactly per 6.3; known provider timeout first-over is exactly `CI_EVIDENCE_LIMIT` with terminal job state `failed`; terminate group/remove staged upload | Spec `13` CI-FR-011/6.3; CI-TEST-010/-011 |
| `SEC-LIM-14-01` | Any performance JSON/document bundle | <=16 MiB/document/depth 16, <=100,000 samples/result, <=1,024 workloads/bundle, <=256 mismatches, string <=4 KiB | Byte/depth/count before proportional allocation; `M0-PERF-ERROR-v1`; no result/comparison | Spec `14` PERF-FR-003/-012, 6.2/6.3; PERF-TEST-003/-012 |
| `SEC-LIM-14-02` | Performance comparator | exactly 10,000 resamples, <=100,000 pairs, <=256 MiB RSS, <=120 s, one owned worker process group, no network/FD growth | Checked arithmetic/work counter/deadline before commit; `PERF_INTEGER_OVERFLOW|PERF_RESOURCE_LIMIT`; terminate/release buffers | Spec `14` PERF-FR-007/-012; PERF-TEST-007/-012 |
| `SEC-LIM-15-01` | Corpus IPC | datagram <=256 bytes; fixed success 56/56/40/32, CANCEL 24; <=5 frames; parser fixed stack buffer <=256 bytes | Header length before payload; exact `CORP_IPC_*`; close endpoint/teardown | Spec `15` 6.2/CORP-FR-005; CORP-TEST-005 |
| `SEC-LIM-15-02` | Corpus topology/lifecycle | exactly 2 processes/7 threads, <=4 owned live FDs, one socketpair, deadline 10 s with <=2 s remaining grace, no storage queue | Check before create/dup/wait; `CORP_THREAD_LIFECYCLE_FAILED|CORP_TIMEOUT|CORP_CLEANUP_FAILED`; kill/reap/close | Spec `15` CORP-FR-001/-003/-009 and CORP-NFR-005; CORP-TEST-001/-003/-009 |
| `SEC-LIM-15-03` | Corpus run/reliability reports | run <=1 MiB/depth 16; aggregate <=16 MiB; <=100 refs; <=32 diagnostics/run at <=4 KiB | Bounds before parse/aggregate; `CORP_REPORT_FIELD_BOUND`; retain valid reports/no aggregate pass | Spec `15` 6.4/CORP-FR-013; CORP-TEST-013 |
| `SEC-LIM-16-01` | Security boundary/capability/resource/structured-input/exception inventories | each <=4 MiB/depth 16, <=4,096 rows, strings <=4 KiB, parser <=256 MiB/120 s | Validate before grant/readiness; `SEC_RESOURCE_LIMIT|SEC_CAPABILITY_DENIED|SEC_EXCEPTION_INVALID`; no grant/gate pass | This spec SEC-FR-001/-006/-008/-013; SEC-TEST-001/-006/-008/-013 |
| `SEC-LIM-16-02` | Secret scan population | <=100,000 commits, <=100,000 manifested files, <=64 MiB/file, <=16 GiB total expanded bytes, archive depth <=8, scanner <=1 GiB RSS/1,800 s | Manifest/size/support before/while scan; `SEC_SECRET_SCAN_INCOMPLETE|SEC_SECRET_SCAN_LIMIT`; no approval, delete expansion temp | This spec 6.5/SEC-FR-002; SEC-TEST-002 |
| `SEC-LIM-16-03` | Security report/diagnostic | report <=8 MiB, <=1,000 details, <=4 KiB/diagnostic, zero raw secret; process/FD N/A: cold in-process renderer | Bound/redact before write; `SEC_DIAGNOSTIC_LIMIT`; blocking summary retained | SEC-NFR-007/SEC-FR-012; SEC-TEST-012 |
| `SEC-LIM-16-04` | Gitleaks/supplemental fixture corpus | <=10,000 fixtures, each <=1 MiB, total <=1 GiB, scan job <=256 MiB/300 s; network/secret capability N/A and denied | Validate corpus manifest before scan; `SEC_PARSER_VERIFICATION_FAILED`; retain minimized non-secret fixture only | SEC-FR-002; SEC-TEST-002 |

SEC-TEST-007 constructs the finite cross-domain reconciliation matrix with one
row for every owner operation above and columns `limit_id`, requirement,
parser/operation, byte/count/depth, memory, process/thread/FD, time,
storage/queue, enforcement point, error, cleanup, and tests. It fails on a
missing owner reference, numeric disagreement, duplicate operation, late
enforcement, or an `N/A` without the exact rationale shown here.

`M0-SECURITY-EXCEPTION-v1` has the fields in SEC-FR-013. Exceptions never
allow a secret, unknown-as-pass, unsupported deterministic success,
unauthorized publication, or silent skip. Foundation-decision changes require
the separate superseding process.

### 6.4 Structured input/fuzz inventory

| Input ID | Trust/source | M0 owner | Required verification |
|---|---|---|---|
| `SEC-IN-001` | Performance result/comparison documents from files/artifacts | `specs/14-performance-foundation.md` | Bounds before allocation, schema negatives, resource corpus, ASan/UBSan-compatible native parser fuzzer and >0 CI smoke executions. |
| `SEC-IN-002` | Corpus IPC frames from exec-child peer | `specs/15-concurrent-corpus.md` | <=256-byte bound, malformed frame matrix, native frame-parser fuzzer under ASan/UBSan-compatible config and >0 CI smoke executions. |
| `SEC-IN-003` | Governance/CI manifests from repository | `specs/11-governance-release.md`, `specs/13-ci-quality.md` | Schema and mutation/resource negatives; if a native custom parser is introduced, add native fuzz target before Active. |
| `SEC-IN-004` | Embedded build/reference/CLI contracts | `specs/10-build-environment.md`, `specs/12-cli-diagnostics.md` | Compile/generation-time schema validation, runtime bounds/negative tests; add fuzzing if runtime accepts externally replaceable bytes. |

Fuzz findings retain minimized input, parser/toolchain/config/revision,
sanitizer report, and stable issue identity. A crash corpus entry becomes a
permanent regression seed. Fuzzing never operates with network, secret,
publication, or out-of-scope filesystem capability.

### 6.5 Exact secret-scan populations

Before either final scan, the enumerator freezes one canonical
`M0-SECRET-SCAN-METADATA-v1` document containing exactly `schema`,
`profile:pull_request|release`, `revision` (40/64 lower hex), `base_revision`
(same or `null`), and `entries`. Each entry contains exactly `set` from the
finite profile sets below, raw `logical_path` as an NFC UTF-8 relative display
path of 1-4,096 bytes, and raw `source_identity` as an NFC UTF-8 string of
1-512 bytes. The raw tuple is unique and sorted by unsigned UTF-8 bytes. The
document contains one tuple for every population byte source and exactly one
self tuple: set `pr_generated` for a pull request or `release_evidence` for a
release, its actual output path, and
`source_identity=m0_secret_scan_metadata`. It contains no byte length, content
digest, scan status, report identity, manifest identity, or marker identity, so
its own tuple creates no content-digest cycle. After these bytes freeze, the
metadata document is hashed and scanned as an ordinary population entry. A
release metadata document belongs only to `release_evidence`, and a PR metadata
document belongs only to `pr_generated`, even if an upload/output inventory
would otherwise list the same bytes again.

The excluded final `M0-SECRET-SCAN-MANIFEST-v1` is canonical JSON with exactly
`schema`, `profile`, `revision`, `base_revision`, `rules_sha256:hex64`,
`scanner_artifact_sha256:hex64`, `metadata`, `entries`, and `summary`.
`metadata` contains exactly `set`, `logical_path_sha256`,
`source_identity_sha256`, `byte_length`, and `metadata_sha256:hex64`; those values
identify the one scanned metadata entry, and `metadata_sha256` equals that
entry's content `sha256`. Every manifest entry has exactly:

- `set` from the owning profile;
- `logical_path_sha256`, SHA-256 of the raw metadata-row `logical_path` UTF-8
  bytes, and `logical_path_display`, exactly
  `sha256:<logical_path_sha256>` (71 ASCII bytes);
- `source_identity_sha256`, SHA-256 of the raw metadata-row
  `source_identity` UTF-8 bytes, and `source_identity_display`, exactly
  `sha256:<source_identity_sha256>` (71 ASCII bytes);
- `byte_length` as a non-negative int64, content `sha256:hex64`,
  `engine:gitleaks|supplemental_bytes`, and
  `status:scanned|unsupported|unavailable|truncated`.

Keys named raw `logical_path` or raw `source_identity`, any display not exactly
the digest-derived value above, and any unmatched/free-form source string are
forbidden anywhere in the final manifest. The tuple
`(set,logical_path_sha256,source_identity_sha256)` is unique and entries are
sorted by its unsigned ASCII bytes. Manifest validation reads the scanned
metadata bytes, recomputes every string digest, requires a one-to-one mapping
between metadata and manifest entries, and requires the top-level `metadata`
object to equal that entry's locator/length/content digest. `summary` contains
total entries/bytes and counts per set/status. Archives and package containers
are expanded recursively to depth 8; symlinks are recorded and resolved only
when their target stays inside the owning tree/closure. Special files, escaping
links, encrypted/opaque archives, unsupported bytes, or a non-`scanned` status
fail. Gitleaks processes every supported text/binary stream with redaction; the
committed supplemental scanner covers byte streams Gitleaks cannot interpret.
The two engines' manifests are disjoint and their union must equal the metadata
and population.

#### Pull-request profile

Let `B` be the Git merge base of the fetched protected target ref and PR head
`H`; both full revisions are recorded. The exact sets are:

1. `pr_history`: every commit patch reachable from `H` but not `B`, including
   added, modified, renamed, and deleted bytes and merge diffs;
2. `pr_tree`: every tracked blob/symlink at `H`; a gitlink additionally expands
   its pinned checked-out submodule tree or makes the gate incomplete;
3. `pr_generated`: every file in each required M0 job's declared Bazel/Nix
   output manifest, including generated source/config, binaries, test reports,
   fuzz/crash output, SBOM, corpus, performance, security, and candidate
   evidence produced by that attempt, plus the frozen
   `M0-SECRET-SCAN-METADATA-v1` document, except the current final PR scan's own
   manifest/report control pair defined below;
4. `pr_package_closure`: every regular file/symlink in the Orus package and
   every path returned by the package's Nix runtime-closure manifest when the
   release-package cell runs;
5. `pr_logs_artifacts`: every normalized job log and every uploaded evidence
   artifact named by `M0-CI-GATE-v1` for the same run/attempt, except byte-for-
   byte copies of the current final PR scan's own manifest/report control pair;
6. `pr_cache`: every file in a restored cache before consumption and every file
   in a cache payload before upload. A cache not locally enumerable is denied.

An untrusted lock/admission change may be scanned as repository bytes but does
not enter acquisition. All six sets must appear; a legitimately empty set has
count zero plus the producing job/check identity, not omission.

#### Release profile

For clean release revision `R`, the exact sets are:

1. `release_history`: patches for every commit reachable from `R`, including
   merge/deleted bytes, up to the 100,000-commit hard limit;
2. `release_tree`: every tracked blob/symlink at `R` and each pinned submodule
   tree as above;
3. `release_generated`: every file in every blocking M0 job's declared
   Bazel/Nix output manifest for the accepted CI attempt, except the current
   final release scan's own manifest/report control pair;
4. `release_package_closure`: every Orus package and transitive Nix closure
   regular file/symlink enumerated by the retained closure manifest;
5. `release_logs_artifacts`: every normalized log and uploaded artifact for all
   required blocking/advisory job IDs referenced by the release evidence,
   except byte-for-byte copies of the current final release scan control pair;
6. `release_cache`: every restored/created cache payload used by the accepted
   run, scanned before use/upload;
7. `release_evidence`: every file reachable from `M0-RELEASE-EVIDENCE-v1`, the
   frozen `preapproval_validated` manifest itself, license/notices, SBOM
   descriptor/document, claim report, the pre-scan security-controls report,
   and the frozen `M0-SECRET-SCAN-METADATA-v1` document. It excludes only the
   current final release scan manifest/report control pair and the
   not-yet-created `M0-RELEASE-APPROVAL-v1` marker.

The current scan control pair is exactly one
`M0-SECRET-SCAN-MANIFEST-v1` plus one canonical
`M0-SECRET-SCAN-REPORT-v1`. The report is canonical JSON, <=8 MiB/depth 16,
and has exactly `schema=M0-SECRET-SCAN-REPORT-v1`,
`profile:pull_request|release`, `revision` (40/64 lowercase hex),
`manifest_sha256:hex64`, `scanner_artifact_sha256:hex64`,
`rules_sha256:hex64`, `entry_count:uint32`, `scanned_count:uint32`,
`finding_count:uint32`, `findings_truncated:boolean`,
`status:pass|fail|incomplete`, and `findings` (0-1,000 rows). Each finding has
exactly `rule_id:id`, `set` from its profile, `logical_path_sha256:hex64`,
`logical_path_display` copied from the matching manifest entry,
`content_sha256:hex64`, and `redacted_preview` as 0-256 NFC bytes; it contains
no raw path, source identity, source bytes, match, or credential. A raw
`logical_path` or `source_identity` key is forbidden. `finding_count` is the
full count even when details stop at 1,000. Findings are sorted by the unsigned
canonical bytes of the complete row; `findings.length=min(finding_count,1000)`,
and `findings_truncated` is true exactly when `finding_count>1000`. A truncated
report cannot pass. Counts and digests reconcile to the manifest; `pass`
requires all entries `scanned`, equal entry/scanned counts,
`findings_truncated=false`, and zero unallowlisted findings.
The scan job's normalized log and uploaded scan artifact are byte-for-byte the
report document (with at most the provider's terminal LF) rather than a second
variable diagnostic format. Thus excluding the current pair excludes every
self-dependent output and nothing else. All of their variable inputs, scanner,
rules, schemas, redaction rules, and producer executable are themselves in an
earlier scanned/validated set.

The same self-output rule applies to PR and release profiles. An earlier
pre-use cache/history scan report is ordinary input to the final scan and is
included; only the currently executing final scan pair is excluded. Gate order
is exact: scan any cache before consumption; run network-denied jobs; enumerate
and freeze every declared output/log/artifact/evidence byte and the pre-approval
release manifest when applicable; freeze the raw-string metadata document with
its self tuple; scan the complete population including that metadata document;
construct the digest/redaction-only final manifest and report; validate their
one-to-one metadata binding, redaction, completeness, and passing status; then,
for release only, create the approval marker last. A metadata/manifest entry
that depends on its own content digest, the current report/manifest result, or
the future marker is `SEC_SECRET_SCAN_INCOMPLETE`.

The only standing raw-byte exclusion is `.git/` object/storage bytes because
its logical history is scanned by `*_history`. No raw logical path or source
identity is excluded: all appear in the scanned metadata document. The current
scan control pair is a temporal self-output exclusion and the final marker does
not yet exist; neither permits excluding another generated/evidence path.
Compiler scratch that is neither an input, declared output, cache, log,
package/closure member, artifact, nor evidence is not retained and therefore is
outside the release population. Secret fixtures are not excluded: their exact
raw metadata strings and content bytes are scanned, and their digest/rule
findings are then matched to narrow non-secret allowlist records. There is no
wildcard, generated-directory, binary, dependency-closure, metadata-string, or
old-history exclusion.

Both profiles enforce `SEC-LIM-16-02`: <=100,000 commits, <=100,000 metadata
and manifest entries, <=64 MiB per expanded entry (including the metadata
document), <=16 GiB total, depth 8, <=1 GiB scanner RSS, and <=1,800 seconds.
Hitting a bound yields `SEC_SECRET_SCAN_LIMIT`; missing provider
log/artifact/cache/closure/history/metadata bytes, provider-side truncation,
metadata/manifest mapping or digest drift, forbidden raw final-pair fields,
scan-engine skip, or status other than `scanned` yields
`SEC_SECRET_SCAN_INCOMPLETE`. Either code blocks the gate with zero approval;
it is never converted to zero findings.

### 6.6 Design choice: policy enforcement shape

| Alternative | Pros | Cons |
|---|---|---|
| Versioned repository contracts validated locally and in CI (recommended) | Reviewable, reproducible, works without network/secrets, shared with future domains. | Requires schema/reconciliation tooling. |
| Prose policy only | Fast to author. | Cannot detect omission, overgrant, or stale exception. |
| CI-provider settings only | Centralized operational enforcement. | Not reproducible locally and insufficient for source/build/parser boundaries. |

**Recommendation:** Keep versioned repository contracts authoritative and use
CI/provider settings as additional enforcement evidence.

### 6.7 Design choice: primary M0 secret scanner

| Alternative | Pros | Cons |
|---|---|---|
| Gitleaks v8.30.1, pinned through Nix/Bazel, with offline versioned rules and narrow supplemental rules (selected) | Mature secret-pattern coverage, local/CI parity, reproducible offline evidence. | Adds an admitted tool dependency and requires maintenance monitoring. |
| Repository-native pattern/entropy scanner | Complete local control and no external scanner binary. | High rule/corpus maintenance burden and weaker ecosystem coverage. |
| GitHub-hosted scanning only | Managed service with low local tooling burden. | Not locally reproducible, not offline, and unavailable as the sole M0 gate. |

**Recommendation:** Use Gitleaks v8.30.1 as the primary M0 scanner. Lock its
version and artifact digest through Nix/Bazel, commit the offline ruleset,
retain only narrow repository-specific supplemental rules, and revisit it
through dependency admission if upstream security maintenance ends. This
implements `q-0010`.

### 6.8 Design choice: canonical M0 content digest

| Alternative | Pros | Cons |
|---|---|---|
| Lowercase hexadecimal SHA-256 (selected) | Ubiquitous Nix, SBOM, and release-tool support with no new algorithm dependency. | Slower than newer hashes, though M0 identity work is cold-path. |
| BLAKE3 | Fast and parallel, attractive for a future measured hot path. | Adds/adopts a dependency and needs compatibility/migration policy. |
| Store SHA-256 and BLAKE3 as co-equal identities | Eases experimentation and migration. | Creates conflicting canonical identities and disagreement handling. |

**Recommendation:** Use one algorithm, `sha256` plus 64 lowercase hexadecimal
digits, with the four non-interchangeable subject names and byte contracts in
spec `10` Section 6.4. Call each a digest, never a signature. Any future
migration or hot-path BLAKE3 use requires an accepted ADR. This implements
`q-0011`.

### 6.9 Design choice: secret-scan self-output metadata

| Alternative | Pros | Cons |
|---|---|---|
| Scan a frozen raw-string metadata document, then retain only digest-derived displays in the excluded control pair (selected) | Raw attacker-derived names are scanned; final pair is acyclic and cannot retain an unscanned name; one-to-one digest mapping is mechanical. | Adds one pre-scan document and reconciliation step; digest-only displays require the scanned metadata for human lookup. |
| Retain raw paths/source identities in the excluded manifest | Most readable standalone manifest. | The excluded bytes can contain a secret that no scan covered; violates SEC-FR-002 and R-202 mitigation. |
| Recursively rescan each newly rendered manifest until stable | Could retain readable names while scanning prior generations. | No finite fixed point is guaranteed because each manifest digest changes; complicates the release DAG and failure evidence. |

**Recommendation:** Use the selected pre-scan metadata contract and exact
digest-only displays from Section 6.5. The metadata is ordinary scanned input;
the final pair is safe to exclude from its own population only after its schema,
mapping, and redaction validate.

## 7. Data Model

| Entity | Identity / relationships | Lifecycle / invariants |
|---|---|---|
| Asset | Stable ID, owner, sensitivity/integrity/availability class. | Every boundary references at least one protected asset. |
| Trust boundary | Stable boundary ID, sides, inputs, assets, controls, owner. | `policy_only -> implemented -> validated`; M1+ rows remain policy_only in M0. |
| Capability | Stable vocabulary ID and semantics. | Default denied; scoped grant only; no unknown implicit capability. |
| Grant profile | Component/profile plus capability scopes and limits. | Immutable per evidence run; only acquisition has network; M0 forbidden capabilities absent. |
| Resource limit | Stable SEC-LIM ID, domain operation/input, owner requirement, numeric/N/A bounds. | Exactly one mapping per applicable M0 operation; validated before Ready and enforced before unsafe allocation/side effect. |
| Structured input | Format/schema/trust/parser/owner. | Maps to limits and verification; native untrusted parser maps to fuzzer. |
| Security exception | Stable ID and exact scope/expiry review milestone. | `proposed -> approved -> expired|closed`; expired never matches. |
| Security finding | Stable control/rule, bounded context SHA-256 digest, severity, affected identity. | Secret values redacted; blocks according to owning requirement; retained as evidence. |
| Secret scan metadata | Profile/revision/base plus the complete sorted raw `(set,logical_path,source_identity)` tuples, including its own tuple. | Frozen before scanning; contains no content/result digest; scanned as one ordinary population entry; immutable until the next attempt. |
| Secret scan manifest | Profile/revision/base, scanner/rules identities, scanned metadata locator/digest, digest/redaction-only entry identities, byte digests, and statuses. | One-to-one with scanned metadata; contains no raw path/source identity; any unavailable/truncated/unsupported/over-limit/unbound/unredacted state blocks. |

Recorded subject-named lowercase hexadecimal SHA-256 digests and validator reports are
authoritative for mechanical integrity checks; they are not signatures, legal judgments, or execution
evidence. Future model text and target content remain untrusted regardless of
provenance label.

## 8. Key Flows

1. **M0 secure build/release (SEC-FR-002 through SEC-FR-006, SEC-FR-012).**
   An untrusted change enters read-only/no-secret CI; the isolated acquisition
   profile verifies admitted locked inputs; network-denied sandboxed actions
   run; cache pre-use scans and all non-scan checks pass; the gate freezes every
   variable output/evidence byte and the pre-approval manifest; it freezes and
   scans the complete raw path/source metadata document as ordinary input; the
   final digest/redaction-only control pair binds that metadata and excludes
   only itself and the future marker; the pair validates complete/pass; the
   release gate creates the bound marker last.
2. **Structured input admission (SEC-FR-007 through SEC-FR-009).** Domain
   identifies schema/trust; registers numeric limits and parser; adds negative,
   resource, sanitizer, and fuzz tests as applicable; CI reconciles inventory;
   only complete passing mapping can become Active.
3. **Cancellation/security-control failure (SEC-FR-012).** Validator stops,
   redacts bounded diagnostic, returns non-zero, uploads safe evidence, and
   creates no gate/approval success; owned processes/temp artifacts are cleaned
   by the domain contract.
4. **Future boundary review (SEC-FR-001, -006, -007, -010).** Before a future
   domain becomes Ready, owner changes its row from policy-only with concrete
   isolation/capabilities/limits/validation; security review rejects missing
   owner, unbounded input, implicit capability, or content-as-policy.
5. **Malformed/resource/overgrant path (SEC-FR-006 through SEC-FR-009,
   SEC-FR-013).** Validate vocabulary/schema/bounds before grant/allocation;
   unknown/oversize/expired/wildcard input fails closed and cannot be
   suppressed by an invalid exception.

## 9. Failure Modes

| ID | Trigger | Required detection point | Typed outcome / diagnostic fields | Side effects and cleanup | Retry / recovery | Verifying requirements/tests |
|---|---|---|---|---|---|---|
| SEC-FAIL-001 | Scanner finds a secret/canary, or a Section 6.5 population entry is missing/unsupported/truncated/over-limit. [R-202] | Manifest reconciliation and offline scan before cache use/upload/release approval. | `SEC_SECRET_DETECTED`, `SEC_SECRET_SCAN_INCOMPLETE`, or `SEC_SECRET_SCAN_LIMIT`; profile/set, scanner/rule, content digest, redacted path/preview, status/count/limit; never raw secret. | Gate fails; cache/artifact/evidence not consumed/approved; expansion temp deleted; real secret requires owner incident response/rotation. | Restore complete bounded population, remove/rotate, and rerun entire profile; never scan a subset. | SEC-FR-002, -012 / SEC-TEST-002, -012. |
| SEC-FAIL-002 | Mutable/unverified/unadmitted dependency, acquisition code execution/credential use, or network outside acquisition. [R-001, R-005] | Acquisition profile, store promotion, action sandbox, capability reconciliation. | `SEC_SUPPLY_CHAIN_INVALID` or `SEC_CAPABILITY_DENIED`; profile/component/action, source, hash/admission/network state. | Quarantine deleted; build/release stops; no trusted artifact. | Pin/admit/remove and rerun; no build-network exception. | SEC-FR-003, -004, -006 / SEC-TEST-003, -004, -006. |
| SEC-FAIL-003 | Subject digest is missing/malformed/mixed/alternate/mismatched/self-referential or substituted across executable/package/SBOM/evidence. [R-005, R-201] | Before object consumption/approval. | `SEC_INTEGRITY_FAILED`; subject, exact byte contract, algorithm, expected/observed digest. | Input not trusted/approved; committed valid evidence unchanged. | Regenerate/retrieve correct subject; never relabel or ignore mismatch. | SEC-FR-005 / SEC-TEST-005. |
| SEC-FAIL-010 | Primary scanner maintenance ends or a digest/scanner change is attempted without admission/ADR evidence. [R-005] | Dependency admission and governance validation before lock/schema change. | `SEC_SECURITY_PRIMITIVE_CHANGE_BLOCKED`; component/algorithm, current/proposed identity, missing evidence/approval. | Existing validated identities remain authoritative; candidate change and release gate fail. | Complete dependency admission or accepted migration ADR, then rerun full evidence. | SEC-FR-014 / SEC-TEST-014. |
| SEC-FAIL-004 | Unknown/implicit/overbroad grant, second M0 network profile, or forbidden capability. [R-303] | Grant/profile validation before operation/job. | `SEC_CAPABILITY_DENIED`; component/profile, capability, scope, expected sole-network profile. | Operation/job not started; no external side effect. | Narrow to exact Section 6.2 profile; future capability requires owning implementation/evidence. | SEC-FR-006 / SEC-TEST-006. |
| SEC-FAIL-005 | Resource limit missing or exceeded. [R-201] | Domain readiness audit or runtime pre-allocation/side-effect boundary. | `SEC_RESOURCE_LIMIT`; domain/operation, resource, limit, observed/requested. | Reject input/operation; domain-defined bounded cleanup; no success claim. | Use bounded input or approved contract change with evidence. | SEC-FR-007 / SEC-TEST-007. |
| SEC-FAIL-006 | Unregistered parser, missing fuzzer, zero fuzz executions, crash/sanitizer finding. [R-201, R-007] | Inventory reconciliation/fuzz job. | `SEC_PARSER_VERIFICATION_FAILED`; input/parser/target/config, execution/finding. | CI gate fails; crashing input minimized/retained; no release approval. | Fix/map/run and retain regression seed. | SEC-FR-008, -009 / SEC-TEST-008, -009. |
| SEC-FAIL-007 | Target/model/browser content attempts to grant authority or fabricate fact. [R-302] | Future typed tool/policy boundary; M0 policy fixture validation. | `SEC_UNTRUSTED_CONTENT_POLICY_ATTEMPT`; provenance, requested capability/claim, evidence state. | Content retained only as data; no grant/factual promotion. | None automatically; authorized owner may separately act. | SEC-FR-010 / SEC-TEST-010. |
| SEC-FAIL-008 | M0 artifact claims an unimplemented sandbox/control. [R-004] | Claim scan before release approval. | `SEC_UNIMPLEMENTED_CLAIM`; surface, boundary/control, owner milestone. | Candidate blocked; no runtime capability created. | Correct to policy/future-tense or deliver in authorized future milestone. | SEC-FR-011 / SEC-TEST-011. |
| SEC-FAIL-009 | Exception is wildcard, stale, ownerless, evidence-free, or attempts forbidden waiver. | Exception validation before matching. | `SEC_EXCEPTION_INVALID`; exception/control/scope/reason. | Exception does not match; underlying finding blocks. | Create narrow reviewed exception if permitted, or fix finding. | SEC-FR-013 / SEC-TEST-013. |
| SEC-FAIL-011 | Pre-scan metadata is missing/unscanned/unbound, its raw tuple does not map one-to-one to the final manifest, or the excluded manifest/report contains a raw/unmatched path or source identity. [R-005, R-202] | Metadata and final control-pair validation before retaining the pair or creating approval. | `SEC_SECRET_SCAN_INCOMPLETE`; profile/set, metadata digest, hashed locator, mismatch/redaction class; never the raw suspect string. | Unsafe pair is discarded or retained only as a separately quarantined non-release test artifact; no gate pass or marker. | Re-enumerate/freeze/rescan the complete metadata and regenerate the pair; no self-output exclusion expansion. | SEC-FR-002 / SEC-TEST-002. |

## 10. Observability

| Signal ID | Metric/event/log | Type, unit, and labels | Emission point / owner | Cardinality and cost bound | Alert/gate use | Verification |
|---|---|---|---|---|---|---|
| SEC-OBS-001 | `security_control_result` | Event/report; control/rule, boundary, status, finding count | Each M0 validator / security owner | At most 1,000 detailed findings then truncated; one summary/control. | CI/release gate. | SEC-TEST-012. |
| SEC-OBS-002 | `capability_inventory` | Report; component/profile/capability/scope/status | Capability reconciliation / security owner | Bounded by finite component/vocabulary inventory; no raw credential. | Least-privilege gate. | SEC-TEST-006. |
| SEC-OBS-003 | `structured_parser_coverage` | Gauge/report; registered/mapped/limited/fuzzed/executed/crashed counts | CI reconciliation / security owner | One row/input ID and aggregate; finite schema inventory. | DOD-04 and domain readiness. | SEC-TEST-008/009. |
| SEC-OBS-004 | `artifact_integrity_result` | Event; subject, object ID, algorithm, digest status | Bundle validation / release/security owner | One/reference; no content logged. | Release evidence gate. | SEC-TEST-005. |
| SEC-OBS-005 | `security_boundary_state` | Gauge/report; boundary, owner milestone, policy/implemented/validated state | Spec readiness audit / security owner | Fixed roadmap boundary inventory. | Prevent false claim/owner omission. | SEC-TEST-001/011. |

Signals correlate by source revision, build/reference-environment ID, CI
run/attempt, digest subject plus subject-named SHA-256, boundary, control,
component, and capability profile. Future trace/branch/execution/VPID/VTID fields are added by
their owners; M0 does not synthesize them. Reports retain digests and redacted
context, never raw secrets or sensitive future trace content.

## 11. Test & Verification Plan

Copy/paste from the repository root after implementation:

```bash
nix develop --command bazel test --config=dev //tests/security/...
nix develop --command bazel run //tools/security:secret_scan -- --profile=pull_request --base-ref=origin/main --head=HEAD --require-scanner=gitleaks@8.30.1 --offline
nix develop --command bazel run //tools/security:secret_scan -- --profile=release --revision=HEAD --require-scanner=gitleaks@8.30.1 --offline
nix develop --command bazel run //tools/security:capability_audit
nix develop --command bazel run //tools/security:boundary_reconcile
nix develop --command bazel run //tools/security:resource_limit_reconcile
nix develop --command bazel run //tools/security:structured_input_reconcile
nix develop --command bazel test --config=fuzz //tests/fuzz:performance_result_parser_fuzz_smoke
nix develop --command bazel test --config=fuzz //tests/fuzz:corpus_ipc_parser_fuzz_smoke
nix develop --command bazel run //tools/governance:claim_scan -- --scope=security
nix develop --command bazel run //tools/governance:release_gate -- --candidate-dir=bazel-bin/release/candidate
```

| Requirement ID | Test/benchmark/review ID | Level | Fixture/workload and environment | Pass criterion | Evidence artifact |
|---|---|---|---|---|---|
| SEC-FR-001 | SEC-TEST-001 | Schema/inspection | Boundary inventory against M0-M11 roadmap and one missing-owner/control fixture. | All rows complete/mapped; fixture fails. | `security-boundary-audit.json`. |
| SEC-FR-002, SEC-NFR-001 | SEC-TEST-002 | Security/static/resource/DAG | Exact PR/release profile sets; valid raw-string metadata/manifests/reports; metadata self tuple and one-to-one digest mapping; empty-set rows; token/key/password/private-key fixtures; and missing/truncated/unsupported/digest/unbound/raw-final-field/100001-file/64-MiB+1/16-GiB+1/depth-9/history-limit mutations. For each of both profiles, add four canaries whose bytes occur only in (1) a tracked filename, (2) a generated-output path, (3) a log/artifact name, or (4) `source_identity`, with benign file content. Add 10,001-fixture, 1-MiB+1, 1-GiB+1, 256-MiB+1, and 300-second+1-nanosecond scanner-corpus/job fixtures. Include forged cycles from `pr_generated`, `pr_logs_artifacts`, and `release_evidence` to the current pair/marker, plus a trace proving cache pre-scan -> jobs -> frozen raw metadata -> complete scan -> digest/redaction-only final pair -> marker. | 100% frozen variable entries and the metadata document scan once with zero real finding. Each of the eight metadata-only canaries is detected in the scanned metadata; any retained failing pair contains only the exact digest displays and redacted preview, never the canary, and no marker exists. Only the current fixed pair/future marker are absent; raw/unmatched final-pair fields, metadata drift, and every cycle/incomplete/over-limit mutation block with the exact code, never zero. | `secret-scan-pr-metadata.json`, `secret-scan-release-metadata.json`, manifests, reports, metadata-canary matrix, `secret-scan-gate-order.json`. |
| SEC-FR-003, SEC-NFR-001 | SEC-TEST-003 | Security/integration | Untrusted PR/protected builds with canary; sole acquisition network; case/alias/byte-different acquisition IDs; unadmitted lock, acquisition code-exec/credential, and build-network mutations. | Zero canary/forbidden permission; workflow job and all profile IDs are byte-identical `m0_input_acquisition`; only that admitted acquisition fetches; every alias, escape, or extra grant fails. | `ci-least-privilege.json`, `acquisition-capability.json`. |
| SEC-FR-004, SEC-NFR-002 | SEC-TEST-004 | Integration/security | Resolved graph plus mutable/hash-mismatch/unadmitted/network fixtures. | 100% valid reconciliation; all faults block before trusted artifact. | `supply-chain-report.json`. |
| SEC-FR-005, SEC-NFR-003 | SEC-TEST-005 | Security/integration | Complete subject-named bundle; raw-byte and package mtime/mode/path/symlink/content changes; self-digest, cross-subject substitution, missing/mixed/alternate algorithm. | Exact included/excluded metadata behavior; 100% valid subjects; every invalid mutation/substitution detected. | `artifact-integrity-matrix.json`. |
| SEC-FR-006, SEC-NFR-004 | SEC-TEST-006 | Schema/security | Exact Section 6.2 profiles plus unknown/implicit/forbidden/overbroad grants, zero/two network profile, acquisition repository-code/credential fixtures. | Exactly one scoped network profile; zero forbidden grants; every fixture denied. | `capability-audit.json`. |
| SEC-FR-007 | SEC-TEST-007 | Schema/inspection/resource | Finite SEC-LIM table against every owner requirement/operation plus missing/duplicate/numeric-drift/error-drift/state-drift/unbounded/late-enforcement and N/A-rationale mutations. Dedicated owner exact-bound/first-over fixtures cover spec-`10` reference 64 MiB/10 seconds -> `BUILD_REFENV_RESOURCE_LIMIT` and package 1,200 seconds -> `BUILD_PACKAGE_IDENTITY_INVALID`; spec-`11` admission 64 MiB/30 seconds, SBOM 256 MiB/120 seconds, and release 256 MiB/120 seconds -> `GOV_RESOURCE_LIMIT`; spec-`12` collection 16 MiB/10 seconds -> `CLI_CONTRACT_INVALID` and rendering 16 MiB/10 seconds -> `CLI_RENDER_ERROR`; spec-`13` warning parser 256 MiB/120 seconds -> `CI_WARNING_BLOCKED` and provider 120 minutes + 1 nanosecond -> `CI_EVIDENCE_LIMIT`/`failed`; and release inventory counts 12/12/3. | Every applicable operation maps exactly once with byte-identical number, exact error and terminal state, cleanup, and owner test. Each owner test proves the exact bound succeeds and first-over returns the stated literal; every numeric/error/state mutation blocks Ready. | `resource-limit-reconciliation.json`, cross-domain matrix. |
| SEC-FR-008, SEC-NFR-005 | SEC-TEST-008 | Schema/analysis | Parser query against input/fuzz/limit inventory plus unmapped/zero-execution fixture. | Exact mapping and positive smoke count; fixtures block. | `structured-input-coverage.json`. |
| SEC-FR-009, SEC-NFR-005 | SEC-TEST-009 | Fuzz/sanitizer/resource | Performance document and corpus frame seed/malformed corpora under fuzz config. | >0 executions/target; 0 crash, timeout, OOM, ASan/UBSan finding. | Fuzz reports and minimized regression corpus index. |
| SEC-FR-010 | SEC-TEST-010 | Policy/security | Future boundary records and adversarial content that requests capability/fabricates evidence. | Content remains data; zero grant/factual promotion; owner mappings complete. | `untrusted-content-policy.json`. |
| SEC-FR-011, SEC-NFR-006 | SEC-TEST-011 | Static/negative | M0 release surfaces, valid future policy prose, injected implemented-control claims. | Zero real finding; prohibited fixtures detected; policy prose allowed. | `security-claims.sarif`. |
| SEC-FR-012, SEC-NFR-007 | SEC-TEST-012 | End-to-end/resource/redaction | One failing control each, over-1,000 findings, over-4KiB diagnostic, secret canary. | Non-zero/no approval; bounded/truncated reports; zero raw canary. | `security-failure-matrix.json`. |
| SEC-FR-013 | SEC-TEST-013 | Schema/negative | Valid exact exception plus wildcard/stale/ownerless/evidence-free/forbidden-waiver fixtures. | Valid only in scope; all invalid fixtures rejected. | `security-exception-tests.xml`. |
| SEC-FR-014 | SEC-TEST-014 | Governance/negative | Maintained and end-of-maintenance scanner states; unauthorized scanner replacement; SHA-256-to-BLAKE3 change with and without complete ADR/admission evidence. | Current pinned choices pass; every ungoverned change blocks; complete future evidence follows the explicit migration path without creating two canonical M0 identities. | `security-primitive-change.json`. |

## 12. Open Questions

No open questions.
