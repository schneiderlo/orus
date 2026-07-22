# M0 Domain Specification Review — 2026-07-21

**Round:** Sixth review, after rework commit `657f418`

**Prior review:** Fifth-round report at this path, produced by commit `8211ec3`

**Scope:** `SPECS.md`, foundation files `00`-`03`, and all seven M0 domain
specifications (`10`-`16`)

**Verdict:** `pass`

The rework resolves the sole fifth-round finding. Advisory typed-report and
retained-log first-over cases now have the same exact functional outcome as
their blocking counterparts: `CI_EVIDENCE_LIMIT` with terminal job state
`failed`. The text continues to distinguish that functional evidence failure
from an advisory benchmark's measured delta, so D-010 and SM-12 authority are
preserved. The separate 64-MiB advisory-bundle outcome remains
`CI_EVIDENCE_LIMIT`/`incomplete`.

Spec `13` defines the outcome consistently in CI-FR-006, CI-FR-011, Section
6.3, CI-FAIL-007, and CI-TEST-011. Spec `16` mirrors the same numeric bounds,
errors, states, and exact-bound/first-over fixtures in `SEC-LIM-13-02`,
`SEC-LIM-13-03`, and SEC-TEST-007. The changed text introduces no new defect.

The unchanged build, governance, CLI, performance, and corpus contracts remain
acceptable based on their previously completed review. Requirement-definition
IDs remain unique, all seven M0 specifications retain explicit scope and
testable verification plans, no M1+ capability or deterministic-execution
claim was introduced, and every M0 spec states that it has no open question.

## Status table

| File | Status | Blocking findings | Re-review summary |
|---|---|---|---|
| `SPECS.md` | ACCEPTABLE | None | M0 remains the only delivery wave; its seven specs are present and M1-M11 remain roadmap-only. |
| `specs/10-build-environment.md` | ACCEPTABLE | None | Unchanged; the previously accepted build, acquisition, reference-environment, resource, and identity contracts remain exact. |
| `specs/11-governance-release.md` | ACCEPTABLE | None | Unchanged; governance, release evidence, final-scan ordering, and rollback/claim contracts remain compatible with the M0 set. |
| `specs/12-cli-diagnostics.md` | ACCEPTABLE | None | Unchanged; CLI schemas, typed errors, collection/render limits, observability, and boundary tests remain exact. |
| `specs/13-ci-quality.md` | ACCEPTABLE | None | M0-R5-001 is resolved throughout the owner requirement, evidence table, failure catalog, and blocking/advisory boundary fixtures. |
| `specs/14-performance-foundation.md` | ACCEPTABLE | None | Unchanged; advisory/authoritative provenance, workload schemas, comparator arithmetic, and verification remain compatible with the CI correction. |
| `specs/15-concurrent-corpus.md` | ACCEPTABLE | None | Unchanged; topology, wire contracts, failure mapping, aggregation, cleanup, and advisory performance registration remain exact. |
| `specs/16-security-foundations.md` | ACCEPTABLE | None | The finite resource inventory and reconciliation test now select and verify one exact state for every CI report/log authority case. |

## Prior-finding disposition

| Prior finding | Disposition | Re-review evidence |
|---|---|---|
| M0-R5-001 advisory report and log over-limits have no terminal job state | **RESOLVED** | CI-FR-006/-011, Section 6.3, CI-FAIL-007, and CI-TEST-011 now assign `CI_EVIDENCE_LIMIT`/`failed` to blocking and advisory 8-MiB report and 32-MiB retained-log first-over cases. `SEC-LIM-13-02/-03` and SEC-TEST-007 repeat the same bounds, outcomes, and authority-paired fixtures. The 64-MiB advisory-bundle exception remains `incomplete`, and provider timeout remains `failed`. |

## Cross-spec consistency check

| Contract / concern | Result | Evidence |
|---|---|---|
| M0 scope and foundation decisions | CONSISTENT | The rework changes only CI evidence-limit precision, stays within M0, and preserves the Charter hierarchy and accepted decisions. |
| Advisory benchmark authority | CONSISTENT | Specs `13`-`16` distinguish functional schema/tool/resource failure from a measured delta; a shared-runner delta remains advisory and cannot alone block the M0 gate under D-010 and SM-12. |
| CI typed-report resource outcome | CONSISTENT | Specs `13` and `16` agree on the 8-MiB exact bound and `CI_EVIDENCE_LIMIT`/`failed` first-over outcome for both authorities, with paired fixtures. |
| CI retained-log resource outcome | CONSISTENT | Specs `13` and `16` agree on the 32-MiB exact bound, 16-MiB head/tail retention, and `CI_EVIDENCE_LIMIT`/`failed` first-over outcome for both authorities, with paired fixtures. |
| CI bundle and timeout outcomes | CONSISTENT | Blocking/advisory bundle first-over remains `failed`/`incomplete` respectively; known provider timeout remains `failed`; no changed row weakens either rule. |
| Security finite-limit reconciliation | CONSISTENT | `SEC-LIM-13-02/-03` and SEC-TEST-007 map every changed CI operation once with byte-identical bounds, exact errors/states, and owner tests. |
| Build, governance, CLI, performance, and corpus interfaces | CONSISTENT | No interface outside specs `13` and `16` changed; existing environment, artifact, performance-authority, corpus, and release dependencies remain compatible. |
| Risk and Definition-of-Done application | CONSISTENT | Exact failure classification and boundary tests restore the DOD-08 and R-008/R-201 mitigation expected by the prior finding without weakening R-002 authority controls. |

## Required edits

None. No carried finding remains and the rework introduced no new-in-rework
finding.

## Open questions

None.

## Verdict

`pass`

M0 domain specifications meet the precision, completeness, testability, and
cross-spec consistency gate. M1-M11 specifications are not needed before
planning this M0-only factory wave, so `more_phases` is false.
