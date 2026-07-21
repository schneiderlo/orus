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

The M0 gate output is a release-evidence bundle for the exact artifact SHA-256 digest
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
| GOV-FR-001 | The root and every packaged MIT `LICENSE` shall contain the standard MIT text and exactly one approved notice line, `Copyright (c) 2026 Loic Schneider`. | Root/package copies are byte-equivalent after permitted newline normalization; missing notice, placeholder token, holder variation, year variation, duplicate notice, or modified license text each fails before publication. | GOV-TEST-001 exact-text and negative-fixture suite. | G-03, C-10, D-002, D-016. |
| GOV-FR-002 | M0 contribution documentation and repository automation shall state that outside contributions are not accepted. | README/contribution policy are unambiguous; zero CLA/DCO requirement, external PR template, bot, or text implying acceptance exists; MIT redistribution rights are not restricted. | GOV-TEST-002 policy linter and manual legal-language review. | C-10, D-009, R-010. |
| GOV-FR-003 | Every significant architecture change shall have an ADR with decision, context, 2-3 alternatives and pros/cons, consequences, validation, rollback/migration, owner, and superseded decisions. | ADR linter passes every accepted domain decision; a change to a protected foundation decision identifies explicit product-owner approval. | GOV-TEST-003 ADR schema and protected-decision fixtures. | DOD-07, Charter 11, Decisions Section 2. |
| GOV-FR-004 | Every direct build/runtime dependency shall have one approved admission record before it enters the lock graph. | Record covers purpose, exact pin/hash, license, notices, maintainer/security health, transitives, build/runtime cost, ABI/boundary exposure, artifact size/memory/performance evidence where material, containment/removal, owner, and review milestone; missing/denied record fails resolution reconciliation. | GOV-TEST-004 admission-schema and unrecorded-dependency fixture. | G-03, R-005. |
| GOV-FR-005 | The release gate shall reconcile 100% of resolved build and runtime dependencies across locks, admission records, SBOM, and notice disposition. | Each resolved component maps exactly once to an admitted identity and SBOM component with version and license field; required notice is packaged; stale/unresolved records and missing checksums fail. | GOV-TEST-005 graph reconciliation with missing/duplicate/stale fixtures. | SM-10, R-005. |
| GOV-FR-006 | Each release package shall include a canonical SPDX 2.3 JSON SBOM bound to the packaged `orus` executable and its resolved graph. | The SBOM validates as SPDX 2.3 JSON, is non-empty, names the Orus executable lowercase hexadecimal SHA-256 content digest, includes every reconciled dependency identity/version/license field and relationship, and contains no secret or mutable network reference needed for interpretation; its admitted generator is pinned by exact version and artifact digest, and the governance/release ADR records deterministic normalization. The external release-evidence manifest binds the completed package digest, avoiding a circular package/SBOM identity. | GOV-TEST-006 SPDX 2.3 schema, normalization, generator-pin, and artifact-binding tests. | G-03, SM-10; resolved `q-0004` and `q-0011`. |
| GOV-FR-007 | Third-party notice policy shall classify and package each admitted component's notice obligation. | Every dependency record has `notice_disposition` equal to `none_required`, `packaged`, or `blocked_pending_review`; the last state blocks release; all `packaged` documents appear in a deterministic notice index. | GOV-TEST-007 notice reconciliation. | D-002, R-005. |
| GOV-FR-008 | The release gate shall emit `M0-RELEASE-EVIDENCE-v1` for the exact source revision and artifact SHA-256 digest. | Manifest references build facts, reference environment, canonical-command results, CI applicability/gate result, sanitizer/fuzz results, corpus report, performance-tool result, license/SBOM/notices SHA-256 digests, claim scan, security controls, and approvals; every reference resolves and digests match. | GOV-TEST-008 evidence-bundle schema and tamper tests. | DOD-02 through DOD-10, R-008. |
| GOV-FR-009 | M0 artifacts and documentation shall use an approved-claims allowlist and make no M1+ availability or compatibility claim. | Scanner covers source-visible help, README/docs, package files, release metadata, examples, and workflow descriptions; zero record/replay/reverse/trace/agent availability claim and zero broad Linux compatibility claim remains; roadmap language is clearly future-tense. | GOV-TEST-009 claim scanner and adversarial prose fixtures. | G-07, SM-11, C-12, R-004. |
| GOV-FR-010 | Publication shall be fail-closed and bound to one validated evidence bundle. | Any failed/missing validation, mismatched digest, unresolved dependency/license, unapproved protected decision, or non-allowlisted claim produces non-zero status and no publishable marker; success creates one immutable `release_candidate_approved` marker for the artifact SHA-256 digest. | GOV-TEST-010 end-to-end gate with one fault fixture per validator. | DOD-01, DOD-06, R-004, R-005. |
| GOV-FR-011 | Repository and agent guidance shall constrain changes to authorized scope and require retained evidence. | Root `AGENTS.md` identifies canonical commands, no-CMake rule, M0 boundary, protected decisions, file ownership, secret policy, destructive-action caution, required tests, and prohibition on unsupported claims; policy check detects omission of each mandatory topic. | GOV-TEST-011 guidance inventory test. | G-03, D-001, D-004. |
| GOV-FR-012 | Release metadata shall state M0 support, compatibility, deprecation, and rollback posture without promising unavailable migration. | Metadata identifies exactly `M0-REFENV-v1`, says other environments are unvalidated, says no trace/protocol compatibility exists at M0, names artifact/source rollback coordinates, and requires an accepted ADR before a future compatibility promise changes. | GOV-TEST-012 release-metadata golden and mismatch tests. | C-03, C-12, D-003, R-009. |

## 5. Non-Functional Requirements

| ID | Quality attribute and target | Conditions / workload | Acceptance criteria | Verification method and evidence |
|---|---|---|---|---|
| GOV-NFR-001 | License accuracy: 100% exact copies and 0 invalid notice values. | Root plus every release-package license copy. | Exact standard text and exact D-016 notice; all six negative classes fail. | GOV-TEST-001 `license-validation.json`. |
| GOV-NFR-002 | Dependency completeness: 100% resolved components reconciled; 0 unadmitted, unpinned, or missing-license-field components. | Full build/runtime graph for release configuration, including transitive components represented by the selected SBOM schema. | Reconciliation has no missing, duplicate, stale, or ambiguous identity. | GOV-TEST-005 `dependency-reconciliation.json`. |
| GOV-NFR-003 | Claim truthfulness: 0 M1+ availability and 0 broad compatibility claims. | Tracked source, CLI help strings, docs, package, and release metadata. | Claim scan has zero non-allowlisted findings; every injected prohibited phrase is detected. | GOV-TEST-009 SARIF/JSON report. |
| GOV-NFR-004 | Evidence integrity: 100% of manifest references validate by lowercase hexadecimal SHA-256 digest for one artifact and revision. | Complete M0 release candidate bundle. | Zero missing/unreadable/mismatched reference; one-byte mutation of each artifact class is detected. | GOV-TEST-008 tamper matrix. |
| GOV-NFR-005 | Governance closure: 0 implied outside-contribution paths and 0 CLA/DCO controls during M0. | Repository policy, templates, automation, and public-facing docs. | Policy linter and reviewer both report zero conflict. | GOV-TEST-002 signed review record. |
| GOV-NFR-006 | SBOM reproducibility: 100% byte-identical canonical SPDX 2.3 JSON from identical resolved graph, artifact, generator, and normalization inputs. | Two clean release-candidate assemblies for one source revision and locked dependency graph. | Both documents schema-validate and have the same lowercase hexadecimal SHA-256 digest; a generator/version, graph, artifact, or normalization change changes declared provenance and is never silently accepted. | GOV-TEST-006 `sbom-reproducibility.json`. |

Performance-hot-path objectives are not applicable: this domain executes on
the cold release path. It must retain performance evidence from
`specs/14-performance-foundation.md`, but does not set or waive its targets.

## 6. Interfaces / Contracts

### 6.1 Versioned governance records

| Contract | Required fields and bounds | Owner / consumer |
|---|---|---|
| `M0-DEPENDENCY-ADMISSION-v1` | Stable component ID; purpose; exact version/hash; source; direct/transitive scope; build/runtime scope; license/SPDX expression when known; notice disposition; health/security review; cost evidence; ABI/boundary; containment/removal; owner; status; at most 256 transitives per direct record, with larger graphs referenced by hash. | Governance owner / build and release gates. |
| `M0-SBOM-CONTRACT-v1` | Canonical serialization `SPDX-2.3` JSON; document namespace/ID; packaged Orus executable SHA-256 digest; pinned creation-tool identity/version/artifact digest; component identities, versions, license fields, and relationships; deterministic-normalization profile ID; document SHA-256 digest; validation result. | Release owner / redistributor and security reviewer. |
| `M0-RELEASE-EVIDENCE-v1` | Schema, release ID, source revision, artifact SHA-256 digests, environment/build IDs, typed evidence references and SHA-256 digests, validator versions/results, approval identity/time, claim scope. At most 10,000 evidence references and 4 KiB per diagnostic. | Release gate / maintainers and auditors. |
| `M0-APPROVED-CLAIMS-v1` | Stable rule ID, permitted literal/pattern, file/surface scope, rationale, owner, and review milestone. | Product/release owner / claim scanner. |

Records are UTF-8, deterministic, schema-validated low-volume data. Records
other than the SBOM may use the selected reviewable JSON/YAML source form, but
their normalized canonical form is digested. The SBOM has exactly one canonical
M0 serialization: SPDX 2.3 JSON. Native C++ layouts are never a record format.
Unknown major schema versions, duplicate stable IDs, excess bounds, unresolved
references, a non-SPDX SBOM, or a noncanonical SBOM is a typed failure.

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

## 7. Data Model

| Entity / state | Identity and relationships | Lifecycle and invariants |
|---|---|---|
| Dependency component | Ecosystem/source-qualified ID plus exact version/hash. | `proposed -> admitted -> active -> removed` or `blocked`; only admitted components may resolve in release graph. |
| Admission record | Stable record ID and normalized-content SHA-256 digest; points to component and evidence. | Immutable once accepted; revision supersedes prior record and explains removal/migration. |
| Notice item | Component identity plus notice-content SHA-256 digest. | Either none required, deterministically packaged, or release-blocking. |
| SBOM | SPDX 2.3 document identity and lowercase hexadecimal SHA-256 digest, bound to the packaged Orus executable digest and reconciled component graph. | Regenerated canonically for each executable; never copied across differing digests or generator/normalization identities. |
| Release candidate | Release ID, source revision, build/environment IDs, artifact SHA-256 digests. | `assembled -> validating -> rejected|approved`; publication is outside this state machine. |
| Evidence reference | Type, schema, URI/path within bundle, lowercase hexadecimal SHA-256 content digest, producer/version. | Resolves inside the retained bundle or an immutable approved store. |
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
| GOV-FAIL-002 | Unadmitted/unpinned/unreconciled dependency or ambiguous license. [R-005] | Lock-to-admission/SBOM reconciliation. | `GOV_DEPENDENCY_BLOCKED`; component, version/hash, missing relation, review owner. | Candidate rejected; lock/build evidence retained. | Admit/remove dependency; legal ambiguity requires owner/counsel input. | GOV-FR-004 through -007 / GOV-TEST-004 through -007. |
| GOV-FAIL-003 | Repository implies outside contribution acceptance or DCO/CLA. [R-010] | Policy scan before release approval. | `GOV_CONTRIBUTION_POLICY_CONFLICT`; surface, rule, excerpt hash. | Candidate rejected; no workflow mutation. | Remove conflict or obtain superseding D-009 decision. | GOV-FR-002 / GOV-TEST-002. |
| GOV-FAIL-004 | M1+ availability or broad platform claim found. [R-004, R-009] | Approved-claims scan. | `GOV_UNAPPROVED_CLAIM`; surface, rule, milestone, bounded excerpt. | Candidate rejected; artifacts retained only as unapproved. | Correct wording or obtain explicit scope/compatibility decision. | GOV-FR-009, -012 / GOV-TEST-009. |
| GOV-FAIL-005 | Evidence reference missing, digest-mismatched, wrong revision, or wrong artifact. [R-008] | Bundle validation before marker. | `GOV_EVIDENCE_INVALID`; reference type/path, expected/observed digest or identity. | Candidate rejected; no partial marker. | Regenerate complete candidate evidence. | GOV-FR-008, -010 / GOV-TEST-008. |
| GOV-FAIL-006 | Gate cancelled or concurrent candidate wins marker creation. | Before atomic approval marker. | `GOV_GATE_CANCELLED` or `GOV_CANDIDATE_CONFLICT`; release ID, artifact SHA-256 digest, attempt ID. | No marker for cancelled/losing attempt; temporary index removed. | Retry after checking immutable winning candidate. | GOV-FR-010 / GOV-TEST-010. |

## 10. Observability

| Signal ID | Metric/event/log | Type, unit, and labels | Emission point / owner | Cardinality and cost bound | Alert/gate use | Verification |
|---|---|---|---|---|---|---|
| GOV-OBS-001 | `dependency_reconciliation` | Gauge/report; resolved, admitted, SBOM, notice, blocked counts | Release reconciliation / governance owner | One row per resolved component; bound by dependency graph; no source content. | SM-10 release gate. | GOV-TEST-005. |
| GOV-OBS-002 | `release_validator_result` | Event; candidate ID, validator ID/version, status, finding count | Each cold-path validator / release owner | At most one terminal row per validator/attempt; validator inventory max 128. | DOD evidence. | GOV-TEST-008. |
| GOV-OBS-003 | `release_claim_finding` | Event; stable rule, surface, bounded excerpt hash | Claim scanner / product owner | At most 1,000 findings then truncated=true and gate fails; no secrets in excerpts. | SM-11 gate. | GOV-TEST-009. |
| GOV-OBS-004 | `release_candidate_state` | Event; candidate/artifact/revision, prior/new state, reason | Gate state transition / release owner | At most four transitions per candidate. | Publication prerequisite; not publication authorization. | GOV-TEST-010. |
| GOV-OBS-005 | `license_validation` | Event; path, template hash, notice count, status | License validator / governance owner | One per license copy; package file count bound. | D-016 gate. | GOV-TEST-001. |

All signals correlate by release candidate ID, source revision, artifact SHA-256 digest,
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
| GOV-FR-004 | GOV-TEST-004 | Schema/negative | Admission records plus unrecorded and denied component. | Valid records pass; both fixtures block. | `dependency-admission.json`. |
| GOV-FR-005, GOV-NFR-002 | GOV-TEST-005 | Integration | Release lock graphs against admissions/SBOM/notices. | 100% one-to-one reconciliation; missing/duplicate/stale fixtures fail. | `dependency-reconciliation.json`. |
| GOV-FR-006, GOV-NFR-006 | GOV-TEST-006 | Schema/integration/reproducibility | Two generated SPDX 2.3 JSON SBOMs plus wrong-format, unpinned-generator, noncanonical, missing-component, and wrong-artifact fixtures. | Both valid outputs are byte-identical with equal SHA-256 digest, non-empty, and exactly graph/artifact bound; every negative fixture fails. | `orus.spdx.json`, `sbom-validation.json`, `sbom-reproducibility.json`. |
| GOV-FR-007 | GOV-TEST-007 | Integration | Each notice disposition including blocked fixture. | Required notices indexed; blocked state rejects release. | `third-party-notice-index.json`. |
| GOV-FR-008, GOV-NFR-004 | GOV-TEST-008 | End-to-end/security | Complete evidence bundle and one-byte mutation per artifact class. | All references resolve; every mutation is detected. | `release-evidence-v1.json`, tamper report. |
| GOV-FR-009, GOV-NFR-003 | GOV-TEST-009 | Static/negative | All release surfaces plus prohibited/future-tense fixtures. | Zero real finding; prohibited fixtures found; valid roadmap prose allowed. | `approved-claims-report.sarif`. |
| GOV-FR-010 | GOV-TEST-010 | End-to-end/concurrency/cancellation | Valid candidate, fault per validator, concurrent attempts, cancellation before commit. | Only valid candidate gets one marker; all other paths get none. | `release-gate-report.json`. |
| GOV-FR-011 | GOV-TEST-011 | Inspection/negative | Root agent guidance and one omitted-topic fixture per inventory row. | Every topic present; omission fixtures fail. | `agent-policy-audit.json`. |
| GOV-FR-012 | GOV-TEST-012 | Golden/negative | M0 release metadata plus broad-support and compatibility fixtures. | Exact M0 posture passes; unsupported claims fail. | `release-metadata-test.json`. |

## 12. Open Questions

No open questions.
