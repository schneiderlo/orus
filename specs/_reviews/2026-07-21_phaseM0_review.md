# M0 Domain Specification Review — 2026-07-21

**Round:** Fifth review, after rework commit `85141bb`
**Prior review:** Fourth-round report at this path, produced by commit `962b794`
**Scope:** `SPECS.md`, foundation files `00`-`03`, and all seven M0 domain
specifications (`10`-`16`)  
**Verdict:** `needs_work`

The rework resolves all four findings from the prior report. The CLI renderer
limit is now present in the finite security inventory; the advisory-bundle
state is consistent; decoded metadata fields reach the detector as their
original value bytes; and metadata findings carry a unique tuple locator and
field enum with actionable recovery tests.

One blocking defect was introduced while narrowing the CI terminal-state rule:
advisory typed-report and retained-log first-over cases no longer have any
defined terminal job state. The unchanged contracts still require every job to
emit a terminal result, and the security reconciliation still requires one
exact state for every resource-bearing operation. No owner clarification is
required because the prior contract already assigned these non-bundle evidence
limits `CI_EVIDENCE_LIMIT`/`failed`; the only established advisory exception is
the 64-MiB bundle's `incomplete` state.

The unchanged build, governance, CLI, performance, and corpus contracts remain
acceptable. Requirement-definition IDs remain unique, no M1+ capability or
scope was introduced, and every M0 spec still states that it has no open
question.

## Status table

| File | Status | Blocking findings | Re-review summary |
|---|---|---|---|
| `SPECS.md` | ACCEPTABLE | None | M0 remains the only delivery wave; all seven M0 specs remain Draft and M1-M11 remain roadmap-only. |
| `specs/10-build-environment.md` | ACCEPTABLE | None | Unchanged since the prior accepted review; build, reference-environment, acquisition, resource, and package-identity contracts remain exact. |
| `specs/11-governance-release.md` | ACCEPTABLE | None | Unchanged since the prior accepted review; its acyclic release order delegates the complete scan to spec `16` and remains compatible with the decoded-field step. |
| `specs/12-cli-diagnostics.md` | ACCEPTABLE | None | The owning collection/render limits remain exact, and spec `16` now inventories the renderer's 16-MiB ceiling, error, cleanup, and boundary fixtures. |
| `specs/13-ci-quality.md` | NEEDS WORK | M0-R5-001 | The advisory 64-MiB bundle and provider-timeout states are now exact, but advisory typed-report and retained-log over-limits lost their terminal state. |
| `specs/14-performance-foundation.md` | ACCEPTABLE | None | Unchanged since the prior accepted review; workloads, schemas, arithmetic, authority, and verification remain exact. |
| `specs/15-concurrent-corpus.md` | ACCEPTABLE | None | Unchanged since the prior accepted review; topology, wire contracts, failure mapping, aggregation, and cleanup remain exact. |
| `specs/16-security-foundations.md` | NEEDS WORK | M0-R5-001 | Decoded-field scanning and finding attribution are now exact, but the resource inventory/test still cannot reconcile advisory report/log first-over terminal states. |

## Prior-finding disposition

| Prior finding | Disposition | Re-review evidence |
|---|---|---|
| M0-010 resource-limit inventory | **RESOLVED** | `SEC-LIM-12-02` now includes the existing 16-MiB render-stage RSS ceiling, `CLI_RENDER_ERROR`/exit 4, cleanup, and CLI-TEST-010's exact-bound/first-over fixtures. |
| M0-R4-001 conflicting advisory-bundle states | **RESOLVED** | CI-FR-011, Section 6.3, CI-FAIL-007, CI-TEST-011, `SEC-LIM-13-03`, and SEC-TEST-007 now agree that an advisory 64-MiB-plus-one-byte bundle is `CI_EVIDENCE_LIMIT`/`incomplete`, while a blocking bundle is `failed` and a known provider timeout is `failed`. M0-R5-001 is a separate omission introduced by narrowing the failure row. |
| M0-R4-002 transformed metadata-string scan | **RESOLVED** | Spec `16` now sends every decoded `logical_path` and `source_identity` to the detector as an exact length-delimited UTF-8 byte sequence, binds each result to the metadata digest/tuple/field, rejects serialization-only scanning, and tests quote, backslash, U+0001, and LF boundaries. |
| M0-R4-003 unattributable metadata findings | **RESOLVED** | The report schema now distinguishes content/metadata origins and gives each metadata finding a deterministic tuple locator and exact field enum; validation, recovery, collision handling, duplicate-canary counts, and no-raw-name retention are explicit. |

## Cross-spec consistency check

| Contract / concern | Result | Evidence |
|---|---|---|
| M0 scope and foundation decisions | CONSISTENT | The rework stays within M0, preserves the Charter hierarchy and D-010 authority split, and adds no platform, license, build-system, publication, or M1+ availability change. |
| CLI resource ownership | CONSISTENT | Specs `12` and `16` now agree on collection versus rendering codes, 16-MiB/10-second ceilings, exit 4, cleanup, and exact-bound/first-over fixtures. |
| CI advisory bundle and provider timeout | CONSISTENT | Specs `13` and `16` now assign one state to blocking/advisory bundle first-over and to known provider timeout. |
| CI advisory report/log resource outcomes | **BLOCKED** | Spec `13` requires terminal results for expected checks and makes schema/tool failures functional failures, but CI-FAIL-007 now defines report/log excess only for blocking jobs and defines an advisory state only for bundles. `SEC-LIM-13-02/-03` and SEC-TEST-007 likewise omit the advisory report/log state (M0-R5-001). |
| Secret-scan byte preservation and attribution | CONSISTENT | Canonical metadata remains an ordinary scanned entry; every decoded field has a byte-exact scan unit, deterministic locator, exact field enum, one-to-one manifest mapping, bounded failure behavior, and retained verification artifacts. |
| Secret-scan/release gate DAG | CONSISTENT | Spec `11`'s evidence-to-metadata-to-complete-scan-to-final-pair-to-marker order remains acyclic and delegates the expanded complete-scan semantics to spec `16`. |
| Performance, corpus, and CI applicability | CONSISTENT WITH FINDING | Specs `13`-`15` retain compatible workload, corpus, sanitizer/fuzz, authority, and evidence dependencies apart from M0-R5-001. |
| Risk and ADR application | CONSISTENT WITH FINDING | Accepted decisions remain intact. M0-R5-001 prevents complete verification of DOD-08 and the R-008/R-201 mitigations. |

## Required edits

### M0-R5-001 — new-in-rework — advisory report and log over-limits have no terminal job state

**Edit that introduced it:** The CI-FAIL-007 rewrite narrowed the previous
`CI_EVIDENCE_LIMIT`/`failed` rule from every numeric evidence/log/bundle excess
to excesses "in a blocking job," then restored an advisory outcome only for
the 64-MiB bundle.

**Locations:** spec `13` lines 65, 70, 155-183, 279, and 328-329; spec `16`
lines 178-179 and 614.

**Finding:** CI-FR-006 says an advisory benchmark's schema or tool failure
blocks that functional job even though its measured delta cannot block the M0
gate. Section 6.3 requires `M0-CI-GATE-v1` to carry terminal states and defines
first-over handling for an 8-MiB typed report and a 32-MiB retained log.
CI-FAIL-007 now assigns `CI_EVIDENCE_LIMIT`/`failed` to those cases only when
the job is blocking; its only advisory rule is the distinct 64-MiB bundle
exception. CI-TEST-011 has advisory fixtures only for bundles. Consequently an
advisory report or log at first-over can be represented as `failed`,
`incomplete`, or an advisory success without violating the changed rows.
Spec `16` nevertheless claims every applicable operation reconciles to one
exact error and terminal state, while `SEC-LIM-13-02/-03` and SEC-TEST-007 do
not select or test those two advisory states.

**Required edit:** Preserve the existing 64-MiB advisory-bundle
`CI_EVIDENCE_LIMIT`/`incomplete` exception. In CI-FAIL-007, state that the
previous `CI_EVIDENCE_LIMIT`/`failed` outcome continues for typed-report and
retained-log first-over regardless of blocking/advisory authority; do not make
a measured benchmark delta blocking. Add advisory 8-MiB report and 32-MiB log
exact-bound/first-over cases to CI-TEST-011 with that exact state. Mirror those
owner outcomes and fixtures in `SEC-LIM-13-02/-03` and SEC-TEST-007 so the
finite reconciliation has one state per operation. Do not change any numeric
limit, provider-timeout state, or bundle state.

**Risk:** R-008, R-201, and DOD-08; the producer, gate aggregator, and security
readiness matrix can classify the same advisory evidence-limit event
differently or accept incomplete evidence.

## Open questions

None. The required correction restores an already-defined non-bundle resource
outcome and does not require a product, authority, or policy choice.

## Verdict

`needs_work`

M0 cannot be marked READY until new-in-rework finding M0-R5-001 is resolved.
M1-M11 specs are not needed before planning this M0-only factory wave, so
`more_phases` is false.
