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
| SEC-FR-002 | Source, flake, locks, build definitions, logs, fixtures, packages, and release evidence shall contain no secret or credential. | Gitleaks v8.30.1 is the primary scanner, pinned by exact version and artifact digest through Nix/Bazel; it runs offline with committed versioned rules plus narrow repository-specific supplemental rules and reports zero unallowlisted finding; injected token/key/password/private-key fixtures are detected; any allowlist entry identifies exact content digest/path/rule, non-secret proof, owner, and review gate. | SEC-TEST-002 Gitleaks identity/offline repository/package/history-scope scan and fixture matrix. | C-11; resolved `q-0010`. |
| SEC-FR-003 | M0 CI and build actions shall operate with least privilege and no protected secret on untrusted changes. | Pull-request workflow has read-only contents, no write/id-token/package capability, no protected secret, no privileged cache write, and no action-network access during build; canary simulation finds zero exposure. | SEC-TEST-003 workflow capability/canary test. | C-11, D-010, R-001. |
| SEC-FR-004 | Every acquired build/dependency input shall be immutable, hash-verified, admitted, and represented in release evidence. | Nix/Bzlmod locks include immutable identity/hash; 100% resolved dependencies reconcile to admission/SBOM; hash mismatch, mutable reference, network fetch during action, or unadmitted dependency fails before artifact approval. | SEC-TEST-004 supply-chain reconciliation/fault matrix. | G-02, SM-10, R-001, R-005. |
| SEC-FR-005 | Release artifacts and evidence shall use lowercase hexadecimal SHA-256 as the canonical M0 content digest and verify it before trust or publication. | Artifact, SBOM, license/notices, and each evidence reference carry `algorithm=sha256` and exactly 64 lowercase hexadecimal digits; one-byte mutation, substitution, missing/mixed-case/malformed digest, unknown/alternate algorithm, or artifact/evidence cross-link mismatch fails; the digest is identity/integrity evidence and is never described as a signature. | SEC-TEST-005 SHA-256 encoding/tamper/substitution matrix. | DOD-03, R-005, R-008; resolved `q-0011`. |
| SEC-FR-006 | Security-sensitive operations and future interfaces shall use the `M0-CAPABILITY-VOCABULARY-v1` identifiers and default deny. | Every M0 component/job and every future owner row declares granted/denied capabilities; absence means denied; unknown capability fails validation; no component is granted target/replay/network/repository-write/model/publish capability in M0. | SEC-TEST-006 capability inventory and overgrant fixtures. | C-09, D-008, D-013, R-303. |
| SEC-FR-007 | Every domain accepting structured or resource-bearing input shall register an enforced `M0-RESOURCE-LIMIT-v1` row before becoming Ready. | Row names parser/operation, input size/count/depth, memory, process/thread/FD, CPU/time, storage/queue limits as applicable, detection point before unsafe allocation/side effect, typed error, cleanup, owner, and tests; `not_applicable` requires scoped rationale. | SEC-TEST-007 domain/resource inventory reconciliation. | C-09, D-007, R-201. |
| SEC-FR-008 | Every untrusted structured parser shall register negative, boundary, resource, sanitizer, and fuzz verification before becoming Active. | `M0-STRUCTURED-INPUT-v1` maps format/schema, trust source, parser target, limits, seed corpus, dictionary when applicable, fuzz target, sanitizer configs, minimum smoke execution >0, continuous owner, crash retention, and promotion rule; missing mapping blocks applicability gate. | SEC-TEST-008 parser/fuzz inventory and zero-execution fixture. | C-09, D-015, R-201. |
| SEC-FR-009 | M0 native structured inputs shall receive the exact verification assigned in Section 6.4. | Performance result parser and corpus IPC frame parser have fuzz-smoke under ASan/UBSan-compatible fuzz config, positive execution count, malformed/resource corpus, and zero crash/sanitizer finding; governance/CI schemas implemented through memory-safe validators have exhaustive schema negatives and must add fuzzing if native parsing is introduced. | SEC-TEST-009 M0 parser applicability reconciliation and fuzz reports. | DOD-04, R-007, R-201. |
| SEC-FR-010 | Future target/trace/symbol/browser/model content shall remain untrusted data and shall not redefine policy or become factual authority. | Boundary inventory states content provenance, read/execute/parse capability, isolation owner, limit/fuzz owner, evidence authority, and prompt-injection rule; raw content cannot grant capabilities; model-visible content is data and factual claims require evidence under D-008. | SEC-TEST-010 future-boundary policy inspection and adversarial policy fixture. | D-008, R-202, R-302. |
| SEC-FR-011 | M0 documentation, CLI, package, and release metadata shall make no implemented sandbox/security claim for future components. | Claim scanner finds zero statement that target/trace/symbol/model isolation, encryption, redaction, sandbox, or deterministic fail-closed runtime is available in M0; policy/future-tense statements are permitted and tagged with owner milestone. | SEC-TEST-011 approved-claims scan and future-tense/prohibited fixtures. | D-001, R-004. |
| SEC-FR-012 | Security control failures shall block the affected CI/release gate and emit bounded non-secret typed diagnostics. | Secret, permission, digest, dependency, capability, resource, parser/fuzz, or claim failure produces a stable code with rule/boundary/artifact identity and redacted context, returns non-zero, creates no approved marker, and retains safe evidence. | SEC-TEST-012 end-to-end control failure matrix and diagnostic redaction test. | DOD-08, R-005, R-201. |
| SEC-FR-013 | Security exceptions shall be narrow, expiring, owner-approved records rather than silent suppression. | Each exception has stable ID, exact control/rule/resource and path/target/config scope, rationale, compensating control, evidence, owner, created revision, and mandatory review milestone; wildcard, ownerless, evidence-free, or overdue exception fails the gate. | SEC-TEST-013 exception-schema and invalid fixture suite. | DOD-07, Charter 11. |
| SEC-FR-014 | Changes to the primary secret scanner or canonical content digest shall use the governed admission/ADR path. | If Gitleaks upstream security maintenance ends, dependency admission reevaluates replacement/removal before a scanner change; any SHA-256 migration or hot-path adoption of BLAKE3 has an accepted ADR covering compatibility, dual-read/migration if applicable, dependency/security/performance evidence, and rollback; no tool silently emits a second canonical identity. | SEC-TEST-014 scanner-maintenance and digest-migration governance fixtures. | DOD-07, R-005; resolved `q-0010`, `q-0011`. |

## 5. Non-Functional Requirements

| ID | Quality attribute and target | Conditions / workload | Acceptance criteria | Verification method and evidence |
|---|---|---|---|---|
| SEC-NFR-001 | Secret exposure: 0 unallowlisted secrets in tracked source, relevant history/diff, build/package outputs, logs, caches, or release evidence. | Gitleaks v8.30.1 offline rules plus supplemental-rule set; release revision, untrusted-PR canary simulation, and injected fixture corpus. | Scanner version/digest/rules identity is exact; real finding count zero; 100% injected fixtures detected; canary appears nowhere unauthorized. | SEC-TEST-002/003 `secret-scan-report.json`. |
| SEC-NFR-002 | Supply-chain completeness: 100% resolved build/runtime components are hash-pinned, admitted, and SBOM-reconciled. | Release configuration dependency graph. | Zero mutable, unverified, unadmitted, or unreconciled component. | SEC-TEST-004 supply-chain report. |
| SEC-NFR-003 | Artifact integrity: 100% release-evidence references verify with lowercase hexadecimal SHA-256 and 100% one-byte mutation classes are detected. | Orus binary/package, SBOM, licenses/notices, CI/corpus/performance/security reports. | Zero missing/malformed/mismatch; every mutation/substitution/alternate-algorithm fixture blocks. | SEC-TEST-005 tamper matrix. |
| SEC-NFR-004 | Capability least privilege: 0 M0 grants for target control, trace/replay, network client, repository write, model invocation, or publication. | Complete M0 component/job capability inventory. | Forbidden-grant count zero; every injected overgrant/unknown capability fails. | SEC-TEST-006 capability report. |
| SEC-NFR-005 | Structured-parser coverage: 100% registered untrusted M0 native parsers have limits, negative/resource tests, ASan/UBSan-compatible fuzz smoke with >0 executions and 0 crash/sanitizer finding. | `M0-STRUCTURED-INPUT-v1` versus Bazel/parser/fuzz target query. | No unmapped native parser; all required fuzzers execute and pass. | SEC-TEST-008/009 parser coverage report. |
| SEC-NFR-006 | Security-claim truthfulness: 0 implemented-isolation/encryption/redaction claim for M1+ behavior. | CLI/help/docs/package/release surfaces. | Zero non-allowlisted claim; all prohibited fixtures detected. | SEC-TEST-011 claim report. |
| SEC-NFR-007 | Diagnostic bound: <=4 KiB per security diagnostic, <=1,000 detailed findings/report before `truncated=true`, and 0 raw secret value. | Maximum and over-limit scan/control fixtures. | Summary/count/first bounded findings retained; limits enforced; secret canary redacted. | SEC-TEST-012 resource/redaction report. |

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
| `target.launch`, `target.control`, `target.memory.read`, `target.memory.write` | Future native target operations. |
| `trace.read`, `trace.create`, `trace.mutate` | Future trace operations; published traces are immutable, so mutate is normally denied. |
| `replay.session.read`, `replay.session.mutate` | Future replay-state access/mutation. |
| `symbol.parse`, `model.invoke` | Future isolated symbol/expression and optional model operations. |
| `external.publish` | Release/PR/deploy or other externally visible publication. |

A grant record contains component/profile, capability, scope, resource limits,
owner, reason, evidence, and milestone. Unknown/absent is denied. M0 untrusted
CI may receive repository.read, build.input.read, build.action.execute, and
scoped artifact.write only. This vocabulary is a policy contract, not proof of
future enforcement.

### 6.3 Resource and exception contracts

`M0-RESOURCE-LIMIT-v1` fields: schema, domain/operation/parser, input trust,
units, maximum input bytes/count/depth, maximum derived allocation/memory,
process/thread/FD, CPU/time, storage/queue/I/O as applicable, enforcement
point, typed error, cleanup, tests, owner, and introducing requirement.
Unbounded is invalid unless the operation is proven not to consume that
resource and cites a rationale.

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

### 6.5 Design choice: policy enforcement shape

| Alternative | Pros | Cons |
|---|---|---|
| Versioned repository contracts validated locally and in CI (recommended) | Reviewable, reproducible, works without network/secrets, shared with future domains. | Requires schema/reconciliation tooling. |
| Prose policy only | Fast to author. | Cannot detect omission, overgrant, or stale exception. |
| CI-provider settings only | Centralized operational enforcement. | Not reproducible locally and insufficient for source/build/parser boundaries. |

**Recommendation:** Keep versioned repository contracts authoritative and use
CI/provider settings as additional enforcement evidence.

### 6.6 Design choice: primary M0 secret scanner

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

### 6.7 Design choice: canonical M0 content digest

| Alternative | Pros | Cons |
|---|---|---|
| Lowercase hexadecimal SHA-256 (selected) | Ubiquitous Nix, SBOM, and release-tool support with no new algorithm dependency. | Slower than newer hashes, though M0 identity work is cold-path. |
| BLAKE3 | Fast and parallel, attractive for a future measured hot path. | Adds/adopts a dependency and needs compatibility/migration policy. |
| Store SHA-256 and BLAKE3 as co-equal identities | Eases experimentation and migration. | Creates conflicting canonical identities and disagreement handling. |

**Recommendation:** Use exactly one canonical M0 identity: `sha256` plus 64
lowercase hexadecimal digits. Call it a digest, never a signature. Any future
migration or hot-path use of BLAKE3 requires an accepted ADR. This implements
`q-0011`.

## 7. Data Model

| Entity | Identity / relationships | Lifecycle / invariants |
|---|---|---|
| Asset | Stable ID, owner, sensitivity/integrity/availability class. | Every boundary references at least one protected asset. |
| Trust boundary | Stable boundary ID, sides, inputs, assets, controls, owner. | `policy_only -> implemented -> validated`; M1+ rows remain policy_only in M0. |
| Capability | Stable vocabulary ID and semantics. | Default denied; scoped grant only; no unknown implicit capability. |
| Grant profile | Component/profile plus capability scopes and limits. | Immutable per evidence run; least privilege; M0 forbidden capabilities absent. |
| Resource limit | Domain operation/input and numeric bounds. | Validated before Ready and enforced before unsafe allocation/side effect. |
| Structured input | Format/schema/trust/parser/owner. | Maps to limits and verification; native untrusted parser maps to fuzzer. |
| Security exception | Stable ID and exact scope/expiry review milestone. | `proposed -> approved -> expired|closed`; expired never matches. |
| Security finding | Stable control/rule, bounded context SHA-256 digest, severity, affected identity. | Secret values redacted; blocks according to owning requirement; retained as evidence. |

Recorded lowercase hexadecimal SHA-256 digests and validator reports are
authoritative for mechanical integrity checks; they are not signatures, legal judgments, or execution
evidence. Future model text and target content remain untrusted regardless of
provenance label.

## 8. Key Flows

1. **M0 secure build/release (SEC-FR-002 through SEC-FR-006, SEC-FR-012).**
   An untrusted change enters read-only/no-secret CI; locked inputs resolve;
   sandboxed actions run; secret, dependency, capability, and parser checks
   pass; artifacts/SBOM/evidence are digested and reconciled; claim scan passes;
   security report joins the non-publishing release candidate.
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
| SEC-FAIL-001 | Gitleaks or a supplemental rule finds a secret/canary in source, input, log, cache, package, or evidence. [R-202] | Offline scan/redaction boundary before upload or release approval. | `SEC_SECRET_DETECTED`; scanner/rule identity, content SHA-256 digest, scoped location, redacted preview; never raw value. | Gate fails; artifact/evidence not approved; exposed real secret requires owner incident response/rotation outside automation. | Remove/rotate and rerun the complete pinned scan. | SEC-FR-002, -012 / SEC-TEST-002, -012. |
| SEC-FAIL-002 | Mutable/unverified/unadmitted dependency or action network access. [R-001, R-005] | Input resolution/action sandbox/reconciliation. | `SEC_SUPPLY_CHAIN_INVALID`; component/action, source, hash/admission state. | Build/release stops; no trusted artifact. | Pin/admit/remove dependency and rerun. | SEC-FR-004 / SEC-TEST-004. |
| SEC-FAIL-003 | Artifact/evidence SHA-256 digest is missing, malformed, mixed-case, alternate-algorithm, mismatched, or substituted. [R-005, R-201] | Before artifact/evidence consumption. | `SEC_INTEGRITY_FAILED`; identity, algorithm, expected/observed digest. | Input not trusted/approved; committed valid evidence unchanged. | Regenerate/retrieve correct artifact; never ignore mismatch. | SEC-FR-005 / SEC-TEST-005. |
| SEC-FAIL-010 | Primary scanner maintenance ends or a digest/scanner change is attempted without admission/ADR evidence. [R-005] | Dependency admission and governance validation before lock/schema change. | `SEC_SECURITY_PRIMITIVE_CHANGE_BLOCKED`; component/algorithm, current/proposed identity, missing evidence/approval. | Existing validated identities remain authoritative; candidate change and release gate fail. | Complete dependency admission or accepted migration ADR, then rerun full evidence. | SEC-FR-014 / SEC-TEST-014. |
| SEC-FAIL-004 | Unknown/implicit/overbroad capability or forbidden M0 grant. [R-303] | Grant/profile validation before operation/job. | `SEC_CAPABILITY_DENIED`; component/profile, capability, scope, rule. | Operation/job not started; no external side effect. | Narrow approved profile; future capability requires owning implementation/evidence. | SEC-FR-006 / SEC-TEST-006. |
| SEC-FAIL-005 | Resource limit missing or exceeded. [R-201] | Domain readiness audit or runtime pre-allocation/side-effect boundary. | `SEC_RESOURCE_LIMIT`; domain/operation, resource, limit, observed/requested. | Reject input/operation; domain-defined bounded cleanup; no success claim. | Use bounded input or approved contract change with evidence. | SEC-FR-007 / SEC-TEST-007. |
| SEC-FAIL-006 | Unregistered parser, missing fuzzer, zero fuzz executions, crash/sanitizer finding. [R-201, R-007] | Inventory reconciliation/fuzz job. | `SEC_PARSER_VERIFICATION_FAILED`; input/parser/target/config, execution/finding. | CI gate fails; crashing input minimized/retained; no release approval. | Fix/map/run and retain regression seed. | SEC-FR-008, -009 / SEC-TEST-008, -009. |
| SEC-FAIL-007 | Target/model/browser content attempts to grant authority or fabricate fact. [R-302] | Future typed tool/policy boundary; M0 policy fixture validation. | `SEC_UNTRUSTED_CONTENT_POLICY_ATTEMPT`; provenance, requested capability/claim, evidence state. | Content retained only as data; no grant/factual promotion. | None automatically; authorized owner may separately act. | SEC-FR-010 / SEC-TEST-010. |
| SEC-FAIL-008 | M0 artifact claims an unimplemented sandbox/control. [R-004] | Claim scan before release approval. | `SEC_UNIMPLEMENTED_CLAIM`; surface, boundary/control, owner milestone. | Candidate blocked; no runtime capability created. | Correct to policy/future-tense or deliver in authorized future milestone. | SEC-FR-011 / SEC-TEST-011. |
| SEC-FAIL-009 | Exception is wildcard, stale, ownerless, evidence-free, or attempts forbidden waiver. | Exception validation before matching. | `SEC_EXCEPTION_INVALID`; exception/control/scope/reason. | Exception does not match; underlying finding blocks. | Create narrow reviewed exception if permitted, or fix finding. | SEC-FR-013 / SEC-TEST-013. |

## 10. Observability

| Signal ID | Metric/event/log | Type, unit, and labels | Emission point / owner | Cardinality and cost bound | Alert/gate use | Verification |
|---|---|---|---|---|---|---|
| SEC-OBS-001 | `security_control_result` | Event/report; control/rule, boundary, status, finding count | Each M0 validator / security owner | At most 1,000 detailed findings then truncated; one summary/control. | CI/release gate. | SEC-TEST-012. |
| SEC-OBS-002 | `capability_inventory` | Report; component/profile/capability/scope/status | Capability reconciliation / security owner | Bounded by finite component/vocabulary inventory; no raw credential. | Least-privilege gate. | SEC-TEST-006. |
| SEC-OBS-003 | `structured_parser_coverage` | Gauge/report; registered/mapped/limited/fuzzed/executed/crashed counts | CI reconciliation / security owner | One row/input ID and aggregate; finite schema inventory. | DOD-04 and domain readiness. | SEC-TEST-008/009. |
| SEC-OBS-004 | `artifact_integrity_result` | Event; artifact/evidence ID, algorithm, digest status | Bundle validation / release/security owner | One/reference; no content logged. | Release evidence gate. | SEC-TEST-005. |
| SEC-OBS-005 | `security_boundary_state` | Gauge/report; boundary, owner milestone, policy/implemented/validated state | Spec readiness audit / security owner | Fixed roadmap boundary inventory. | Prevent false claim/owner omission. | SEC-TEST-001/011. |

Signals correlate by source revision, build/reference-environment ID, CI
run/attempt, artifact/evidence SHA-256 digest, boundary, control, component, and
capability profile. Future trace/branch/execution/VPID/VTID fields are added by
their owners; M0 does not synthesize them. Reports retain digests and redacted
context, never raw secrets or sensitive future trace content.

## 11. Test & Verification Plan

Copy/paste from the repository root after implementation:

```bash
nix develop --command bazel test --config=dev //tests/security/...
nix develop --command bazel run //tools/security:secret_scan -- --require-scanner=gitleaks@8.30.1 --offline
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
| SEC-FR-002, SEC-NFR-001 | SEC-TEST-002 | Security/static | Source/diff/package/log/cache/evidence scan plus token/key/password/private-key fixtures. | Zero real finding; 100% fixtures detected/redacted. | `secret-scan-report.json`. |
| SEC-FR-003, SEC-NFR-001 | SEC-TEST-003 | Security/integration | Untrusted PR with canary and permission/cache assertions. | Zero canary exposure and no forbidden capability. | `ci-least-privilege.json`. |
| SEC-FR-004, SEC-NFR-002 | SEC-TEST-004 | Integration/security | Resolved graph plus mutable/hash-mismatch/unadmitted/network fixtures. | 100% valid reconciliation; all faults block before trusted artifact. | `supply-chain-report.json`. |
| SEC-FR-005, SEC-NFR-003 | SEC-TEST-005 | Security/integration | Complete bundle plus one-byte mutation/substitution/missing/unknown-algorithm fixtures. | 100% valid references; every fault detected. | `artifact-integrity-matrix.json`. |
| SEC-FR-006, SEC-NFR-004 | SEC-TEST-006 | Schema/security | Complete component profiles plus unknown/implicit/forbidden/overbroad grants. | Zero forbidden M0 grant; every fixture denied. | `capability-audit.json`. |
| SEC-FR-007 | SEC-TEST-007 | Schema/inspection/resource | All domain operations against limit inventory plus missing/unbounded/late-enforcement fixtures. | 100% applicable mapping; invalid fixtures fail. | `resource-limit-reconciliation.json`. |
| SEC-FR-008, SEC-NFR-005 | SEC-TEST-008 | Schema/analysis | Parser query against input/fuzz/limit inventory plus unmapped/zero-execution fixture. | Exact mapping and positive smoke count; fixtures block. | `structured-input-coverage.json`. |
| SEC-FR-009, SEC-NFR-005 | SEC-TEST-009 | Fuzz/sanitizer/resource | Performance document and corpus frame seed/malformed corpora under fuzz config. | >0 executions/target; 0 crash, timeout, OOM, ASan/UBSan finding. | Fuzz reports and minimized regression corpus index. |
| SEC-FR-010 | SEC-TEST-010 | Policy/security | Future boundary records and adversarial content that requests capability/fabricates evidence. | Content remains data; zero grant/factual promotion; owner mappings complete. | `untrusted-content-policy.json`. |
| SEC-FR-011, SEC-NFR-006 | SEC-TEST-011 | Static/negative | M0 release surfaces, valid future policy prose, injected implemented-control claims. | Zero real finding; prohibited fixtures detected; policy prose allowed. | `security-claims.sarif`. |
| SEC-FR-012, SEC-NFR-007 | SEC-TEST-012 | End-to-end/resource/redaction | One failing control each, over-1,000 findings, over-4KiB diagnostic, secret canary. | Non-zero/no approval; bounded/truncated reports; zero raw canary. | `security-failure-matrix.json`. |
| SEC-FR-013 | SEC-TEST-013 | Schema/negative | Valid exact exception plus wildcard/stale/ownerless/evidence-free/forbidden-waiver fixtures. | Valid only in scope; all invalid fixtures rejected. | `security-exception-tests.xml`. |
| SEC-FR-014 | SEC-TEST-014 | Governance/negative | Maintained and end-of-maintenance scanner states; unauthorized scanner replacement; SHA-256-to-BLAKE3 change with and without complete ADR/admission evidence. | Current pinned choices pass; every ungoverned change blocks; complete future evidence follows the explicit migration path without creating two canonical M0 identities. | `security-primitive-change.json`. |

## 12. Open Questions

No open questions.
