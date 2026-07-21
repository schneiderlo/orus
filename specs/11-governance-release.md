# Governance, Dependency Admission, and Release Evidence

**Status:** Draft  
**Milestone:** M0  
**Owners:** Governance owner; release engineer; security reviewer  
**Last updated:** 2026-07-21  
**Depends on:** `specs/10-build-environment.md` (Draft; Ready prerequisite); Foundation specs (Ready)  
**Foundation decisions:** D-002, D-004, D-009, D-016  
**Risks addressed:** R-004, R-005, R-008, R-010

## 1. Purpose

This domain makes an Orus source and binary release legally identifiable,
dependency-complete, provenance-backed, and honest about its M0 capabilities.
It owns repository governance, architecture/dependency admission, the exact
MIT license, third-party notices, SBOM reconciliation, release evidence,
compatibility/rollback claims, and the pre-publication gate.

Its consumers are maintainers, security/release engineers, redistributors, and
reviewers. Build mechanics belong to `specs/10-build-environment.md`; CI
execution belongs to `specs/13-ci-quality.md`; secret and trust-boundary
policy belongs to `specs/16-security-foundations.md`.

The M0 gate output is a release-evidence bundle for the exact
`orus_executable_sha256` and `package_tree_sha256`
that passes license, dependency, SBOM, notice, provenance, contribution-policy,
and approved-claims checks before publication.

## 2. In Scope

- Standard MIT license with the exact D-016 notice.
- Closed outside-contribution policy for M0.
- Architecture-decision and dependency-admission records.
- Resolved dependency inventory, third-party notice handling, and SBOM.
- Artifact/source/toolchain provenance and release-evidence manifest.
- Repository/agent policy for scoped changes and evidence.
- Release claim allowlist; compatibility, migration, deprecation, and rollback
  claim rules.
- Fail-before-publication validation and evidence immutability.

## 3. Out of Scope

- Selecting another source license, CLA, DCO, or external contribution
  workflow; these require explicit owner approval superseding D-002/D-009.
- Legal advice or a new interpretation of third-party licenses. Ambiguous
  obligations stop admission for owner/counsel review.
- Build/package mechanics; owned by `specs/10-build-environment.md`.
- Credential storage, signing infrastructure, or remote-cache provisioning;
  M0 requires no signing service and `specs/16-security-foundations.md` owns
  secret boundaries.
- Record/replay, trace, protocol, or platform compatibility promises.
- Publishing a release. This spec defines the authorization gate; it does not
  authorize an external publication action.

## 4. Functional Requirements

| ID | Requirement | Acceptance criteria | Verification method | Traceability |
|---|---|---|---|---|
| GOV-FR-001 | The root and every packaged MIT `LICENSE` shall contain the standard MIT text and exactly one approved notice line, `Copyright (c) 2026 Loic Schneider`. | Each license input is <=64 KiB; root/package copies are byte-equivalent after permitted newline normalization; missing notice, placeholder token, holder variation, year variation, duplicate notice, modified text, or byte excess each fails before publication. | GOV-TEST-001 exact-text, bound, and negative-fixture suite. | G-03, C-10, D-002, D-016. |
| GOV-FR-002 | M0 contribution documentation and repository automation shall state that outside contributions are not accepted. | README/contribution policy are unambiguous; zero CLA/DCO requirement, external PR template, bot, or text implying acceptance exists; MIT redistribution rights are not restricted. | GOV-TEST-002 policy linter and manual legal-language review. | C-10, D-009, R-010. |
| GOV-FR-003 | Every significant architecture change shall have an ADR with decision, context, 2-3 alternatives and pros/cons, consequences, validation, rollback/migration, owner, and superseded decisions. | ADR linter passes every accepted domain decision; a change to a protected foundation decision identifies explicit product-owner approval. | GOV-TEST-003 ADR schema and protected-decision fixtures. | DOD-07, Charter 11, Decisions Section 2. |
| GOV-FR-004 | Every direct build/runtime dependency shall have one schema-valid, approved `M0-DEPENDENCY-ADMISSION-v1` record before it enters the lock graph. | Every required field, enum, bound, and relationship in Section 6.1.1 validates. `license_state=unknown_pending_legal` is represented only with `spdx_expression=null`, `status=blocked`, and a release-blocking notice disposition; it can never be admitted by default. Missing, denied, ambiguous, or relationship-invalid records fail reconciliation with a typed Section 6.1.5 error. | GOV-TEST-004 admission-schema, valid-example, unknown-license, and unrecorded-dependency fixtures. | G-03, R-005. |
| GOV-FR-005 | The release gate shall reconcile 100% of resolved build and runtime dependencies across locks, admission records, SBOM, and notice disposition. | Each resolved component maps exactly once to an `admitted` identity and SBOM component with exact version and declared SPDX expression; required notice is packaged; unknown license, stale/unresolved record, missing checksum, identity ambiguity, or invalid relationship fails. | GOV-TEST-005 graph reconciliation with unknown-license/missing/duplicate/stale fixtures. | SM-10, R-005. |
| GOV-FR-006 | Each release package shall include a canonical SPDX 2.3 JSON SBOM bound to the packaged `orus` executable and its resolved graph. | The completed SBOM and external `M0-SBOM-CONTRACT-v1` descriptor satisfy Section 6.1.2. The SBOM names `orus_executable_sha256`, includes every reconciled component/license/relationship, and contains neither its own digest nor `package_tree_sha256`; the external descriptor computes `sbom_sha256` over completed canonical SBOM bytes. Its generator is pinned, and mutable interpretation references are forbidden. | GOV-TEST-006 SPDX 2.3 schema, normalization, external-descriptor, generator-pin, graph, and artifact-binding tests. | G-03, SM-10, R-005, R-008; resolved `q-0004` and `q-0011`. |
| GOV-FR-007 | Third-party notice policy shall classify and package each admitted component's notice obligation. | Every dependency record has `notice_disposition` equal to `none_required`, `packaged`, or `blocked_pending_review`; the last state blocks release. All packaged documents appear in a canonical notice index <=16 MiB/depth 16 with <=100,000 entries and <=64-KiB notice file; excess blocks. | GOV-TEST-007 notice reconciliation and exact resource-bound fixtures. | D-002, R-005. |
| GOV-FR-008 | The release gate shall emit schema-valid `M0-RELEASE-EVIDENCE-v1` for one source revision and the subject identities in `M0-CONTENT-IDENTITY-v1`. | Manifest fields are exactly Section 6.1.3: `orus_executable_sha256`, `package_tree_sha256`, `sbom_sha256`, and typed references each named `evidence_object_sha256`. It covers build facts, reference environment, canonical commands, CI applicability/gate, sanitizers/fuzzing, corpus, performance tooling, license/notices, claims, security controls, and approvals; every path/length/schema/digest and cross-identity resolves. The manifest never contains its own digest; the final marker binds it externally. | GOV-TEST-008 evidence-bundle schema, relationship, and tamper tests. | DOD-02 through DOD-10, R-005, R-008. |
| GOV-FR-009 | M0 artifacts and documentation shall use an approved-claims allowlist and make no M1+ availability or compatibility claim. | Scanner covers source-visible help, README/docs, package files, release metadata, examples, and workflow descriptions; zero record/replay/reverse/trace/agent availability claim and zero broad Linux compatibility claim remains; roadmap language is clearly future-tense. | GOV-TEST-009 claim scanner and adversarial prose fixtures. | G-07, SM-11, C-12, R-004. |
| GOV-FR-010 | Publication shall be fail-closed and bound to one validated evidence bundle. | Any failed/missing validation, mismatched subject digest, unresolved dependency/license, unapproved protected decision, or non-allowlisted claim produces non-zero status and no publishable marker; success creates one immutable external `release_candidate_approved` marker binding release ID, `orus_executable_sha256`, `package_tree_sha256`, `sbom_sha256`, and the evidence manifest's `evidence_object_sha256`. | GOV-TEST-010 end-to-end gate with one fault fixture per validator and subject substitution. | DOD-01, DOD-06, R-004, R-005, R-008. |
| GOV-FR-011 | Repository and agent guidance shall constrain changes to authorized scope and require retained evidence. | Root `AGENTS.md` identifies canonical commands, no-CMake rule, M0 boundary, protected decisions, file ownership, secret policy, destructive-action caution, required tests, and prohibition on unsupported claims; policy check detects omission of each mandatory topic. | GOV-TEST-011 guidance inventory test. | G-03, D-001, D-004. |
| GOV-FR-012 | Release metadata shall state M0 support, compatibility, deprecation, and rollback posture without promising unavailable migration. | Metadata identifies exactly `M0-REFENV-v1`, says other environments are unvalidated, says no trace/protocol compatibility exists at M0, names artifact/source rollback coordinates, and requires an accepted ADR before a future compatibility promise changes. | GOV-TEST-012 release-metadata golden and mismatch tests. | C-03, C-12, D-003, R-009. |

## 5. Non-Functional Requirements

| ID | Quality attribute and target | Conditions / workload | Acceptance criteria | Verification method and evidence |
|---|---|---|---|---|
| GOV-NFR-001 | License accuracy: 100% exact copies and 0 invalid notice values. | Root plus every release-package license copy. | Exact standard text and exact D-016 notice; all six negative classes fail. | GOV-TEST-001 `license-validation.json`. |
| GOV-NFR-002 | Dependency completeness: 100% resolved components reconciled; 0 unadmitted, unpinned, unknown-license, or missing-license components. | Full build/runtime graph for release configuration, including transitive components represented by the selected SBOM schema. | Reconciliation has no missing, duplicate, stale, ambiguous identity, or `unknown_pending_legal` state. | GOV-TEST-005 `dependency-reconciliation.json`. |
| GOV-NFR-003 | Claim truthfulness: 0 M1+ availability and 0 broad compatibility claims. | Tracked source, CLI help strings, docs, package, and release metadata. | Claim scan has zero non-allowlisted findings; every injected prohibited phrase is detected. | GOV-TEST-009 SARIF/JSON report. |
| GOV-NFR-004 | Evidence integrity: 100% of subject-named executable/package/SBOM/evidence references validate by lowercase hexadecimal SHA-256 for one revision. | Complete M0 release candidate bundle. | Zero missing/unreadable/mismatched/cross-subject reference; one-byte or included-package-metadata mutation of each class is detected under Section 6.4 of spec `10`. | GOV-TEST-008 tamper and subject-substitution matrix. |
| GOV-NFR-005 | Governance closure: 0 implied outside-contribution paths and 0 CLA/DCO controls during M0. | Repository policy, templates, automation, and public-facing docs. | Policy linter and reviewer both report zero conflict. | GOV-TEST-002 signed review record. |
| GOV-NFR-006 | SBOM reproducibility: 100% byte-identical canonical SPDX 2.3 JSON from identical resolved graph, artifact, generator, and normalization inputs. | Two clean release-candidate assemblies for one source revision and locked dependency graph. | Both documents schema-validate and have the same lowercase hexadecimal SHA-256 digest; a generator/version, graph, artifact, or normalization change changes declared provenance and is never silently accepted. | GOV-TEST-006 `sbom-reproducibility.json`. |

Performance-hot-path objectives are not applicable: this domain executes on
the cold release path. It must retain performance evidence from
`specs/14-performance-foundation.md`, but does not set or waive its targets.

## 6. Interfaces / Contracts

### 6.1 Versioned governance records

Every contract is `M0-CANONICAL-JSON-v1` from spec `10`; YAML may be an authoring
view only and is never identified or consumed by the gate. Unless a field is
marked optional, it is required; unknown fields are rejected. `id` is an ASCII
string matching `[a-z0-9][a-z0-9._:-]{0,127}`, `hex64` is
`[0-9a-f]{64}`, `schema_id` is ASCII
`[A-Za-z0-9][A-Za-z0-9._:-]{0,127}`, `text` is NFC UTF-8 of 1-4096 bytes, and `relpath` is an NFC
relative `/`-separated path of 1-4096 bytes with no empty, `.`, or `..`
component.

#### 6.1.1 `M0-DEPENDENCY-ADMISSION-v1`

The document is at most 1 MiB and has exactly these fields:

| Field | Type / enum / bound | Relationship rule |
|---|---|---|
| `schema`, `record_id`, `component_id` | literal schema; two unique `id` values | Schema is exactly `M0-DEPENDENCY-ADMISSION-v1`; one active record/component. |
| `purpose`, `source`, `boundary`, `containment`, `removal_plan`, `owner` | `text`; `source` at most 2048 bytes | Source is an immutable origin description, not a required live reference. |
| `version`, `pin_value` | strings 1-256 bytes | Exact, never a range or floating branch/tag. |
| `pin_type` | `sha256|git_commit|nix_store` | `pin_value` conforms to the selected type. |
| `artifact_sha256` | `hex64` | Digest of acquired component artifact. |
| `dependency_scope` | `direct|transitive` | Direct records may name transitives; resolved transitives also have their own record. |
| `linkage_scope` | `build|runtime|build_and_runtime` | Matches lock/SBOM use. |
| `license_state` | `declared|unknown_pending_legal` | Unknown is a first-class blocking state. |
| `spdx_expression` | string 1-512 bytes or `null` | Required non-null iff license state is `declared`; required null otherwise. |
| `notice_disposition` | `none_required|packaged|blocked_pending_review` | Unknown license requires `blocked_pending_review`. |
| `health_status`, `security_status` | each `reviewed|blocked` | Both `reviewed` for admission. |
| `transitive_ids` | sorted unique `id` array, 0-256 | No self-reference; every ID resolves; larger graphs use separately admitted records, never truncation. |
| `cost` | object | Exactly non-negative-int64 `build_time_ms`, `artifact_bytes`, `peak_memory_bytes`, and `evidence_ref:id`. |
| `abi_exposure` | `none|private|public` | Public exposure requires non-empty boundary/removal plan; other states retain them for audit. |
| `review_milestone` | milestone token | Exactly `M0` for this gate. |
| `status` | `proposed|admitted|blocked|removed` | `admitted` requires declared license, reviewed health/security, and non-blocking notice. Unknown license requires `blocked`. |
| `supersedes` | `id` or `null` | If non-null, resolves to an immutable prior record for the same component. |

Valid minimal fixture:

```json
{"abi_exposure":"none","artifact_sha256":"aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa","boundary":"private tool","component_id":"tool.fixture","containment":"build only","cost":{"artifact_bytes":1,"build_time_ms":1,"evidence_ref":"ev.cost.fixture","peak_memory_bytes":1},"dependency_scope":"direct","health_status":"reviewed","license_state":"declared","linkage_scope":"build","notice_disposition":"none_required","owner":"governance","pin_type":"sha256","pin_value":"aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa","purpose":"schema fixture","record_id":"dep.fixture.v1","removal_plan":"remove lock row","review_milestone":"M0","schema":"M0-DEPENDENCY-ADMISSION-v1","security_status":"reviewed","source":"fixture origin","spdx_expression":"MIT","status":"admitted","supersedes":null,"transitive_ids":[],"version":"1.0.0"}
```

#### 6.1.2 `M0-SBOM-CONTRACT-v1`

This is an external descriptor for one completed canonical SPDX 2.3 JSON
document, not a field embedded in that document. The descriptor is at most 64
KiB; the referenced SBOM is at most 16 MiB, 100,000 packages/files, 200,000
relationships, and nesting depth 32.

| Field | Type / bound | Rule |
|---|---|---|
| `schema`, `descriptor_id` | literal schema; `id` | Schema exactly `M0-SBOM-CONTRACT-v1`. |
| `spdx_path`, `spdx_document_id`, `document_namespace` | `relpath`; string 1-256; string 1-2048 | Path resolves within bundle; namespace is immutable text and needs no network lookup. |
| `orus_executable_sha256`, `sbom_sha256` | `hex64` | First covers raw executable bytes; second covers completed canonical SPDX bytes. |
| `generator` | object | Exactly `component_id:id`, `version:string[1..128]`, `artifact_sha256:hex64`. |
| `normalization_profile` | literal | Exactly `M0-CANONICAL-JSON-v1+SPDX-2.3-v1`. |
| `component_count`, `relationship_count` | uint32 | Equal parsed SPDX counts and stay within bounds. |
| `validation_status` | enum `valid` | Any schema/license/graph/digest failure prevents descriptor creation. |

The SPDX document contains neither `sbom_sha256` nor `package_tree_sha256`.
Valid descriptor fixture:

```json
{"component_count":1,"descriptor_id":"sbom.fixture.v1","document_namespace":"https://example.invalid/orus/fixture","generator":{"artifact_sha256":"bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb","component_id":"tool.sbom.fixture","version":"1"},"normalization_profile":"M0-CANONICAL-JSON-v1+SPDX-2.3-v1","orus_executable_sha256":"aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa","relationship_count":1,"sbom_sha256":"cccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccc","schema":"M0-SBOM-CONTRACT-v1","spdx_document_id":"SPDXRef-DOCUMENT","spdx_path":"orus.spdx.json","validation_status":"valid"}
```

#### 6.1.3 `M0-RELEASE-EVIDENCE-v1`

The manifest is at most 16 MiB and has exactly these fields:

| Field | Type / bound | Rule |
|---|---|---|
| `schema`, `release_id` | literal schema; `id` | One immutable candidate identity. |
| `source_revision` | 40- or 64-character lowercase hex | Clean revision only. |
| `environment_id`, `build_id`, `orus_executable_sha256`, `package_tree_sha256`, `sbom_sha256` | five `hex64` values | Resolve to the same source/build/package candidate and Section 6.4 subjects from spec `10`. |
| `evidence` | 1-10,000 rows | Each row has exactly `type` (finite enum below), `schema:schema_id`, `path:relpath`, `byte_length:non-negative int64`, `evidence_object_sha256:hex64`, and `producer:id`; paths unique and digest/length exact. |
| `validators` | 1-128 rows | Exactly `validator_id:id`, `version:string[1..128]`, `status:pass|fail`, `finding_count:uint32`, `diagnostic:text|null`; any fail prevents approval. |
| `approvals` | 0-32 rows | Exactly `identity:id`, `role:product_owner|release_owner|security_owner`, `decision:approved`, `time` as UTC RFC3339 seconds; identities unique. |
| `claim_scope` | enum | Exactly `m0_only`. |
| `state` | `assembled|validating|rejected|release_candidate_approved` | Approved requires all required evidence types, all validator passes, required approvals, and an external marker. |

Evidence `type` is exactly one of `build_facts`, `reference_environment`,
`canonical_commands`, `ci_gate`, `sanitizer_fuzz`, `corpus_reliability`,
`performance_tooling`, `license_notice`, `sbom_descriptor`, `claim_scan`,
`security_controls`, or `adr_approval`; required type counts are declared by
the release inventory and never inferred from filenames. The manifest never
references or hashes itself. Valid pre-approval fixture:

```json
{"approvals":[],"build_id":"bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb","claim_scope":"m0_only","environment_id":"aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa","evidence":[{"byte_length":2,"evidence_object_sha256":"ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff","path":"evidence/build.json","producer":"tool.build","schema":"M0-BUILD-FACTS-v1","type":"build_facts"}],"orus_executable_sha256":"cccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccc","package_tree_sha256":"dddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddd","release_id":"orus.fixture.1","sbom_sha256":"eeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeee","schema":"M0-RELEASE-EVIDENCE-v1","source_revision":"0123456789abcdef0123456789abcdef01234567","state":"assembled","validators":[{"diagnostic":null,"finding_count":0,"status":"pass","validator_id":"schema.fixture","version":"1"}]}
```

#### 6.1.4 `M0-APPROVED-CLAIMS-v1`

The document is at most 1 MiB and contains `schema` plus 1-4096 `rules`, sorted
by unique `rule_id`. Each row has exactly `rule_id:id`,
`match_kind:literal|re2`, `pattern:string[1..1024]`, 1-8 unique `surfaces` from
`cli_string|readme|documentation|package|release_metadata|example|workflow`,
1-64 `paths:relpath`, `rationale:text`, `owner:id`, and
`review_milestone:M0`. RE2 means RE2 syntax with UTF-8 mode, unanchored search;
literal means exact Unicode-scalar substring. No pattern changes availability
or support facts; nonmatching claims remain denied.

```json
{"rules":[{"match_kind":"literal","owner":"product.owner","paths":["SPECS.md"],"pattern":"roadmap context","rationale":"allows explicitly future roadmap wording","review_milestone":"M0","rule_id":"claim.roadmap-context","surfaces":["documentation"]}],"schema":"M0-APPROVED-CLAIMS-v1"}
```

#### 6.1.5 Validation errors

All four validators emit one canonical `M0-GOV-ERROR-v1` object containing
exactly `schema=M0-GOV-ERROR-v1`, `code`, `contract` (one of the four exact
contract schema IDs in Sections 6.1.1-6.1.4), `field_path` (0-512 bytes), `record_id`
(`id|null`), `expected` (0-1024-byte string), `observed` (0-1024-byte redacted
string), `limit` (non-negative int64 or `null`), and `message` (`text`, at most 4 KiB). `code`
is one of `GOV_SCHEMA_UNKNOWN`, `GOV_NONCANONICAL`, `GOV_FIELD_MISSING`,
`GOV_FIELD_TYPE`, `GOV_FIELD_BOUND`, `GOV_ENUM_INVALID`, `GOV_ID_DUPLICATE`,
`GOV_REFERENCE_UNRESOLVED`, `GOV_DIGEST_INVALID`,
`GOV_RELATIONSHIP_INVALID`, or `GOV_LICENSE_UNRESOLVED`. Validation stops
before approval side effects but may return at most 1,000 sorted errors with a
total count and `truncated=true` in the surrounding report.

Gate invocation is idempotent for an unchanged artifact and evidence set.
Concurrent attempts for the same release ID serialize at marker creation.
Cancellation leaves no approved marker. Evidence is immutable once referenced;
a correction creates a new candidate ID and supersedes, never mutates, the old
record.

### 6.2 Design choice: release validation ordering

| Alternative | Pros | Cons |
|---|---|---|
| Validate complete immutable bundle before creating an approval marker (recommended) | No partial release can appear approved; simple artifact-digest binding; supports offline review. | Requires all evidence to be available before gate completion. |
| Validate incrementally and publish as checks pass | Faster apparent release path. | Partial external state and rollback ambiguity; violates fail-before-publication intent. |
| Manual checklist without machine-readable bundle | Low tooling cost. | Cannot prove reconciliation or tamper resistance and fails DOD-10. |

**Recommendation:** Validate first and create one marker last. This directly
implements GOV-FR-010 and introduces no external publication authorization.

### 6.3 Design choice: canonical M0 SBOM serialization

| Alternative | Pros | Cons |
|---|---|---|
| SPDX 2.3 JSON with a pinned generator and deterministic normalization (selected) | Mature component/license vocabulary, machine-readable relationships, and one redistributable release contract. | Verbose and requires exact normalization to make byte-level evidence reproducible. |
| CycloneDX JSON | Strong security-component ecosystem and compact component modeling. | Uses different license/relationship semantics and would create a second policy surface. |
| Emit SPDX and CycloneDX | Broad consumer interoperability. | Duplicates reconciliation and permits the two documents to drift. |

**Recommendation:** Use SPDX 2.3 JSON as the only canonical M0 SBOM. Pin the
generator by version and artifact digest through dependency admission, and
record the deterministic normalization profile and golden validation in the
governance/release ADR. This implements the human answer to `q-0004`.

### 6.4 Design choice: SBOM/package identity placement

| Alternative | Pros | Cons |
|---|---|---|
| External subject-named descriptor and release manifest (recommended) | No self-reference; executable, package tree, SBOM, and evidence bytes stay distinguishable. | Requires the descriptor and final marker to travel with the package. |
| Put the SBOM digest inside the SBOM | One document appears self-describing. | Circular identity or an arbitrary excluded-field algorithm. |
| Use one generic `artifact` digest | Smaller schema. | Cannot say whether bytes mean executable, tree, SBOM, or evidence and permits substitution. |

**Recommendation:** Use the external descriptor and the four subject names
defined by spec `10`; no generic artifact digest is accepted.

## 7. Data Model

| Entity / state | Identity and relationships | Lifecycle and invariants |
|---|---|---|
| Dependency component | Ecosystem/source-qualified ID plus exact version/hash. | `proposed -> admitted -> active -> removed` or `blocked`; only admitted components may resolve in release graph. |
| Admission record | Stable record ID and normalized-content SHA-256 digest; points to component and evidence. | Immutable once accepted; revision supersedes prior record and explains removal/migration. |
| Notice item | Component identity plus notice-content SHA-256 digest. | Either none required, deterministically packaged, or release-blocking. |
| SBOM | SPDX 2.3 document ID plus external `sbom_sha256`, bound to `orus_executable_sha256` and reconciled graph. | Regenerated canonically; document contains no self/package digest. |
| Release candidate | Release ID, revision, build/environment IDs, `orus_executable_sha256`, `package_tree_sha256`, and `sbom_sha256`. | `assembled -> validating -> rejected|approved`; publication is outside this state machine. |
| Evidence reference | Type, schema, relative path, byte length, `evidence_object_sha256`, producer/version. | Resolves inside the retained bundle; never uses a generic artifact label. |
| Approved claim | Stable rule scoped to surface and milestone. | Default deny; future-scope wording remains non-availability language. |

Legal approval is authoritative for license ambiguity. Automated outputs are
authoritative only for exact mechanical checks. A scanner pass cannot supply a
legal interpretation or authorize publication.

## 8. Key Flows

1. **Dependency admission (GOV-FR-004 through GOV-FR-007).** A maintainer
   proposes an exact dependency and evidence; governance reviews license,
   health, security, cost, ABI, and removal; an approved immutable record is
   committed; build locks may then change; release reconciliation maps the
   resolved component to admission, SBOM, and notice disposition.
2. **Release candidate validation (GOV-FR-001, -002, -008 through -012).**
   The build owner supplies an artifact identity; the gate assembles typed
   evidence; validators run with no publish capability; all digests, policies,
   dependencies, claims, and approvals pass; the gate atomically creates the
   approval marker bound to that artifact.
3. **Cancellation/rejection (GOV-FR-010).** Cancellation or any validator
   failure records a rejected/incomplete candidate; no approval marker exists;
   already produced immutable evidence remains diagnostic; retry uses a new
   validation attempt and creates a new candidate if content changes.
4. **Unsupported legal or compatibility claim (GOV-FR-005, -009, -012).**
   The earliest scanner/reconciler detects ambiguity or an unapproved promise,
   returns typed context, and blocks the candidate without guessing a license
   obligation or migration policy.

## 9. Failure Modes

| ID | Trigger | Required detection point | Typed outcome / diagnostic fields | Side effects and cleanup | Retry / recovery | Verifying requirements/tests |
|---|---|---|---|---|---|---|
| GOV-FAIL-001 | Root/package license missing, altered, placeholder, duplicate, wrong holder, or wrong year. [R-005] | License validation before bundle approval. | `GOV_LICENSE_INVALID`; copy path, expected notice hash/text ID, mismatch class. | Candidate rejected; no approval marker. | Correct only with D-016 value or superseding owner decision. | GOV-FR-001 / GOV-TEST-001. |
| GOV-FAIL-002 | Unadmitted/unpinned/unreconciled dependency or `unknown_pending_legal` license. [R-005] | Admission validation and lock-to-SBOM reconciliation. | `GOV_DEPENDENCY_BLOCKED` plus Section 6.1.5 cause; component, version/hash, license state, missing relation, review owner. | Candidate rejected; lock/build evidence retained. | Admit/remove dependency; unknown license requires owner/counsel resolution and a superseding immutable record. | GOV-FR-004 through -007 / GOV-TEST-004 through -007. |
| GOV-FAIL-003 | Repository implies outside contribution acceptance or DCO/CLA. [R-010] | Policy scan before release approval. | `GOV_CONTRIBUTION_POLICY_CONFLICT`; surface, rule, excerpt hash. | Candidate rejected; no workflow mutation. | Remove conflict or obtain superseding D-009 decision. | GOV-FR-002 / GOV-TEST-002. |
| GOV-FAIL-004 | M1+ availability or broad platform claim found. [R-004, R-009] | Approved-claims scan. | `GOV_UNAPPROVED_CLAIM`; surface, rule, milestone, bounded excerpt. | Candidate rejected; artifacts retained only as unapproved. | Correct wording or obtain explicit scope/compatibility decision. | GOV-FR-009, -012 / GOV-TEST-009. |
| GOV-FAIL-005 | Evidence is missing/digest-mismatched, uses a generic/wrong subject, or binds the wrong revision/package/SBOM. [R-005, R-008] | Bundle validation before marker. | `GOV_EVIDENCE_INVALID`; subject, reference type/path, expected/observed digest or identity. | Candidate rejected; no partial marker. | Regenerate complete candidate evidence; never relabel a digest. | GOV-FR-008, -010 / GOV-TEST-008. |
| GOV-FAIL-006 | Gate cancelled or concurrent candidate wins marker creation. | Before atomic approval marker. | `GOV_GATE_CANCELLED` or `GOV_CANDIDATE_CONFLICT`; release ID, `package_tree_sha256`, evidence-manifest `evidence_object_sha256`, attempt ID. | No marker for cancelled/losing attempt; temporary index removed. | Retry after checking immutable winning candidate. | GOV-FR-010 / GOV-TEST-010. |

## 10. Observability

| Signal ID | Metric/event/log | Type, unit, and labels | Emission point / owner | Cardinality and cost bound | Alert/gate use | Verification |
|---|---|---|---|---|---|---|
| GOV-OBS-001 | `dependency_reconciliation` | Gauge/report; resolved, admitted, SBOM, notice, blocked counts | Release reconciliation / governance owner | One row per resolved component; bound by dependency graph; no source content. | SM-10 release gate. | GOV-TEST-005. |
| GOV-OBS-002 | `release_validator_result` | Event; candidate ID, validator ID/version, status, finding count | Each cold-path validator / release owner | At most one terminal row per validator/attempt; validator inventory max 128. | DOD evidence. | GOV-TEST-008. |
| GOV-OBS-003 | `release_claim_finding` | Event; stable rule, surface, bounded excerpt hash | Claim scanner / product owner | At most 1,000 findings then truncated=true and gate fails; no secrets in excerpts. | SM-11 gate. | GOV-TEST-009. |
| GOV-OBS-004 | `release_candidate_state` | Event; candidate/artifact/revision, prior/new state, reason | Gate state transition / release owner | At most four transitions per candidate. | Publication prerequisite; not publication authorization. | GOV-TEST-010. |
| GOV-OBS-005 | `license_validation` | Event; path, template hash, notice count, status | License validator / governance owner | One per license copy; package file count bound. | D-016 gate. | GOV-TEST-001. |

All signals correlate by release candidate ID, source revision,
`orus_executable_sha256`, `package_tree_sha256`, and `sbom_sha256`,
build ID, and reference-environment ID. M0 has no trace, branch, execution,
VPID, or VTID. The complete normalized manifest, validator reports, SBOM,
notices, and claim scan are retained with the candidate.

## 11. Test & Verification Plan

Copy/paste from the repository root after implementation:

```bash
nix develop --command bazel test --config=dev //tests/governance/...
nix develop --command bazel run //tools/governance:license_check
nix develop --command bazel run //tools/governance:dependency_reconcile
nix develop --command bazel run //tools/governance:sbom -- --artifact=result/bin/orus --output=bazel-bin/release/orus.spdx.json
nix develop --command bazel run //tools/governance:claim_scan
nix develop --command bazel run //tools/governance:release_gate -- --candidate-dir=bazel-bin/release/candidate
nix build .#orus
```

| Requirement ID | Test/benchmark/review ID | Level | Fixture/workload and environment | Pass criterion | Evidence artifact |
|---|---|---|---|---|---|
| GOV-FR-001, GOV-NFR-001 | GOV-TEST-001 | Unit/integration | Root/package license plus missing, placeholder, holder, year, duplicate, altered-text fixtures. | Valid copies pass; every negative fixture fails before approval. | `license-validation.json`. |
| GOV-FR-002, GOV-NFR-005 | GOV-TEST-002 | Static/inspection | Docs/templates/automation plus injected CLA/DCO/acceptance phrases. | Real tree has zero conflict; fixtures detected. | `contribution-policy.sarif`, signed review. |
| GOV-FR-003 | GOV-TEST-003 | Schema/inspection | All ADRs plus missing-field/protected-decision fixtures. | 100% valid; unauthorized protected change rejected. | `adr-audit.json`. |
| GOV-FR-004 | GOV-TEST-004 | Schema/negative | Section 6.1.1 valid example; one missing/type/enum/bound/relationship negative per field; unrecorded, denied, and unknown-license components. | Valid passes; every mutation emits the exact Section 6.1.5 code; unknown license is blocked, never admitted. | `dependency-admission.json`. |
| GOV-FR-005, GOV-NFR-002 | GOV-TEST-005 | Integration | Release lock graphs against admissions/SBOM/notices, including explicit unknown-license state. | 100% one-to-one declared-license reconciliation; missing/duplicate/stale/unknown fixtures fail. | `dependency-reconciliation.json`. |
| GOV-FR-006, GOV-NFR-006 | GOV-TEST-006 | Schema/integration/reproducibility | Section 6.1.2 example; two generated SPDX documents; wrong format/generator/canonicalization/graph/executable, self-digest, oversize/count/depth fixtures. | Valid outputs/descriptor are byte-identical with equal external `sbom_sha256`; no self/package digest exists; every negative fails. | `orus.spdx.json`, `sbom-descriptor.json`, `sbom-validation.json`, `sbom-reproducibility.json`. |
| GOV-FR-007 | GOV-TEST-007 | Integration | Each notice disposition including blocked fixture. | Required notices indexed; blocked state rejects release. | `third-party-notice-index.json`. |
| GOV-FR-008, GOV-NFR-004 | GOV-TEST-008 | End-to-end/schema/security | Section 6.1.3 example, complete evidence bundle, one-byte/included-metadata mutation and cross-subject substitution per class. | All fields/relationships resolve; every mutation/substitution is detected; manifest has no self-reference. | `release-evidence-v1.json`, subject tamper report. |
| GOV-FR-009, GOV-NFR-003 | GOV-TEST-009 | Static/schema/negative | Section 6.1.4 example; all release surfaces plus invalid RE2/scope/bound and prohibited/future-tense fixtures. | Claims schema exact; zero real finding; prohibited fixtures found; valid allowlisted roadmap prose allowed. | `approved-claims-report.sarif`. |
| GOV-FR-010 | GOV-TEST-010 | End-to-end/concurrency/cancellation | Valid candidate, fault per validator, concurrent attempts, cancellation before commit. | Only valid candidate gets one marker; all other paths get none. | `release-gate-report.json`. |
| GOV-FR-011 | GOV-TEST-011 | Inspection/negative | Root agent guidance and one omitted-topic fixture per inventory row. | Every topic present; omission fixtures fail. | `agent-policy-audit.json`. |
| GOV-FR-012 | GOV-TEST-012 | Golden/negative | M0 release metadata plus broad-support and compatibility fixtures. | Exact M0 posture passes; unsupported claims fail. | `release-metadata-test.json`. |

## 12. Open Questions

No open questions.
