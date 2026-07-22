# M0 Domain Specification Review — 2026-07-21

**Round:** Re-review after rework commit `e2d82d5`
**Prior review:** First-round report at this path, produced by commit `6cfcca1`
**Scope:** `SPECS.md`, foundation files `00`-`03`, and all seven M0 domain
specifications (`10`-`16`)  
**Verdict:** `needs_work`

The rework fully resolves seven of the twelve first-round findings. Five prior
findings remain partially unresolved, and three blocking defects were
introduced by the new contract text. No owner clarification is required: all
eight remaining findings can be corrected by making the existing M0 contracts
internally exact and consistent, without adding product scope.

## Status table

| File | Status | Blocking findings | Re-review summary |
|---|---|---|---|
| `SPECS.md` | ACCEPTABLE | None | M0 remains the only delivery wave, all seven M0 specs remain Draft, dependencies are accurate, and M1-M11 remain roadmap-only. |
| `specs/10-build-environment.md` | ACCEPTABLE | None directly | The canonical JSON, reference-environment, content-identity, and acquisition-boundary additions resolve M0-001/M0-002. Its canonical acquisition profile name is not used consistently by spec `13` (M0-011). |
| `specs/11-governance-release.md` | NEEDS WORK | M0-003 | Unknown-license and SBOM self-digest states are resolved, but three governance record contracts still delegate normative grammar, normalization, and approval inventory to undefined text. |
| `specs/12-cli-diagnostics.md` | NEEDS WORK | M0-R2-001 | Help and top-level error behavior now meet the one-document decision, but the new doctor result schema cannot represent its own aggregate row. |
| `specs/13-ci-quality.md` | NEEDS WORK | M0-011 | Evidence bounds and permission rules are now exact; the sole-network profile has a different identifier from specs `10` and `16`. |
| `specs/14-performance-foundation.md` | NEEDS WORK | M0-006, M0-R2-002 | Canonical bytes and most comparator arithmetic are now explicit, but result/workload schema details remain incomplete and the new higher-is-better formula violates the stated exact 3% boundary. |
| `specs/15-concurrent-corpus.md` | NEEDS WORK | M0-008 | Wire layouts and observation workload resolve M0-009 and the wire half of M0-008; report relationship invariants remain incomplete. |
| `specs/16-security-foundations.md` | NEEDS WORK | M0-010, M0-011, M0-R2-003 | A finite inventory and finite scan populations now exist, but the inventory does not reconcile to several owning specs and the release population is circular. |

All FR/NFR IDs remain unique within their requirement sections, every FR/NFR
is named in its domain test-plan table, and every domain continues to state no
open question. No M1+ availability claim, CMake path, license change, broad
platform promise, or conflicting Accepted ADR was introduced.

## Prior-finding disposition

| Prior finding | Disposition | Re-review evidence |
|---|---|---|
| M0-001 reference-environment contract | RESOLVED | Spec `10` 6.3 now fixes byte canonicalization, fields/types/operators, observed facts, validation outcomes, errors, and a verified fixed-ID example; spec `12` maps exact fact paths. |
| M0-002 digest subjects/canonical bytes | RESOLVED | Spec `10` 6.4 defines executable, package tree, SBOM, and evidence bytes plus correct fixed digests; specs `11` and `16` use the same subjects. |
| M0-003 governance record schemas | **CARRIED** | The new tables resolve unknown-license and self-digest states but still omit normative pin grammars, the named SPDX normalization profile, and the finite release evidence/approval inventory. |
| M0-004 CLI error/help contract | RESOLVED | Spec `12` now defines the exit/stream matrix, `M0-CLI-ERROR-v1`, bounded argv encoding, renderer fallback, and explicit no-help behavior. M0-R2-001 concerns a different result-schema edit. |
| M0-005 CI evidence/permission exceptions | RESOLVED | Spec `13` 6.3 supplies numeric evidence limits and exact over-limit behavior; CI-FR-008 removes all M0 write/OIDC/package/publish exceptions. |
| M0-006 performance document schemas | **CARRIED** | The rework supplies canonical byte rules and field tables, but several result/workload fields and relationships remain implementation-defined. |
| M0-007 comparator reproducibility | RESOLVED | Spec `14` 6.4 now fixes schedule/seed bytes, counter sampling, arithmetic, median, quantile, precedence, and intermediate goldens. M0-R2-002 is a semantic error introduced in that algorithm. |
| M0-008 corpus wire/report schemas | **CARRIED** | All wire payloads are concrete; the newly added run/reliability schemas still permit evidence that violates the required topology/fault relationships. |
| M0-009 corpus observations/cardinality | RESOLVED | Spec `15` now fixes the exact clock/random calls and counts, and CORP-OBS-001 correctly uses seven identities. |
| M0-010 resource-limit inventory | **CARRIED** | The finite table exists, but multiple rows add numbers absent from their cited owner requirements, so the mandated exact reconciliation cannot pass. |
| M0-011 network capability model | **CARRIED** | The acquisition/build split is now coherent, but its supposedly single profile has two different stable identifiers. |
| M0-012 secret-scan population | RESOLVED | PR and release sets, manifests, limits, and incomplete-input behavior are finite. M0-R2-003 is a gate-order cycle introduced by those new sets. |

## Cross-spec consistency check

| Contract / concern | Result | Evidence |
|---|---|---|
| M0 scope and foundation decisions | CONSISTENT | All changes stay inside M0 and apply D-001 through D-016 without weakening the Charter. |
| Build/reference/CLI facts | BLOCKED | Exact fact paths now agree, but the new CLI result cardinality cannot encode the aggregate reference row (M0-R2-001). |
| Artifact/package/SBOM/evidence identity | CONSISTENT | Subject names, canonical bytes, external digest placement, and fixed digest examples align across `10`, `11`, and `16`. |
| Governance/release state | BLOCKED | Approval depends on an undefined required-evidence/approval inventory, and the SPDX normalization profile is only named (M0-003). |
| CI/security capability inventory | BLOCKED | `m0_input_acquisition` in `10`/`16` conflicts with `input_acquisition` in `13` (M0-011). |
| Performance authority and comparator | BLOCKED | Authority/noise precedence is aligned, but incomplete schema fields and the higher-is-better denominator prevent an exact interoperable 3% decision (M0-006, M0-R2-002). |
| Concurrent corpus into CI/performance/security | BLOCKED | Topology, wire values, calls, deadline, and sums agree; report schemas do not enforce the topology and fault relationships required by those consumers (M0-008). |
| Resource-limit ownership | BLOCKED | The security table is finite but several limits have no identical owning requirement or allowed N/A rationale (M0-010). |
| Secret scan into release gate | BLOCKED | The release population requires bytes that cannot exist until the scan/release gate has already passed (M0-R2-003). |
| Risk and ADR application | CONSISTENT WITH FINDINGS | Existing choices remain within the Accepted decisions. The defects below prevent complete validation of R-001, R-002, R-003, R-005, R-006, R-008, R-201, and R-202. |

## Required edits

### M0-003 — carried — governance schemas still delegate normative decisions

**Locations:** `11` lines 109, 130-147, and 154-178.
**Finding:** The rework correctly makes unknown licenses blocking and moves the
SBOM digest outside the SBOM, but the contracts are still not independently
implementable:

- `pin_value conforms to the selected type` gives no exact grammar for
  `git_commit` or `nix_store` and does not state the SHA-256 relationship for
  `pin_type=sha256`;
- `M0-CANONICAL-JSON-v1+SPDX-2.3-v1` is named but never defines the SPDX-specific
  ordering/normalization of arrays and generated fields, so two valid SPDX
  producers can produce different canonical bytes from the same graph; and
- `M0-RELEASE-EVIDENCE-v1` says approval requires “all required evidence
  types” and “required approvals,” while their counts/roles live in an
  undefined “release inventory.” The schema also says evidence producer/version
  is retained elsewhere but provides only `producer` in the row.

GOV-TEST-004/006/008/010 therefore cannot derive one expected result solely
from this spec.

**Required edit:** Define the exact `pin_value` grammar and digest relationship
for each `pin_type`; define the complete `SPDX-2.3-v1` normalization rules,
including every unordered collection and generated value; and add the finite
M0 evidence-type cardinality plus exact approval role/count requirements
directly to Section 6.1.3. Align the evidence row with the retained producer
identity/version invariant and extend the existing negative fixtures for each
newly exact relationship.

**Risk:** R-005 and R-008; dependency identity, reproducible SBOM bytes, and
release authorization can be interpreted differently.

### M0-006 — carried — performance schemas remain incomplete

**Locations:** `14` lines 96-108, 150-165, and 235-251.
**Finding:** The rework adds strong common types and five field tables, but it
still leaves identified bytes under implementation control. The initial
workload table omits the required invocation/dataset/concurrency/sampling fields
for each named row. In `M0-PERF-RESULT-v1`, `sampling` refers to unspecified
“warmup/measured method strings” instead of named fields/enums/types, the
canonical order of the referenced raw-sample array is not defined, and the
median/MAD/percentile derivation used in `statistics` has no selection or
rounding rule. Only the raw-sample schema has a complete fixed byte example,
although PERF-TEST-003 claims one valid example for all five schemas.

**Required edit:** Publish complete normative records for the five initial
workloads; replace the prose `sampling` summary with exact field names, enums,
integer types/bounds, and count equations; specify raw-sample array order and
summary-statistic selection/rounding; and include one complete canonical valid
example for each remaining schema with its expected content digest. Update
PERF-TEST-003 to exercise those exact examples and relationships.

**Risk:** R-002 and R-201; conforming producers can emit different identities,
summaries, or comparability keys.

### M0-008 — carried — corpus report relationships do not prove the required run

**Locations:** `15` lines 191-211 and 219-235.
**Finding:** The wire contract is now exact, but the report schema does not
enforce the behavior it claims to evidence. A success report can state
`process_count=2` while assigning arbitrary host PIDs to its seven role rows;
there is no rule that all parent roles share one PID, all child roles share one
different PID, main/worker TIDs have the required uniqueness/ownership, or the
child image is distinct. Fault reports have no exact `fault_mode` to `terminal`
mapping. In the reliability schema, a `failure_counts` row has no terminal enum
or count type/bound and no complete equation reconciling failures with
attempted/passed reports.

**Required edit:** Add the missing cross-field topology/identity invariants,
the finite fault-mode-to-terminal mapping, and a fully typed/sorted
`failure_counts` schema with exact count equations. Add forged fixtures that
keep all scalar counts correct while violating PID/TID ownership, fault
mapping, or aggregate equations.

**Risk:** R-003, R-006, and R-008; forged or malformed evidence can pass while
the required process/thread topology or fault result did not occur.

### M0-010 — carried — the resource table cannot reconcile to its owners

**Locations:** `16` lines 149-193, especially SEC-LIM-10-02/03,
SEC-LIM-11-02/03/04, SEC-LIM-12-01/02, and SEC-LIM-13-03.
**Finding:** SEC-FR-007 and SEC-TEST-007 require every number to equal its
owning requirement, but the new table adds uncited caps that those owners do
not state. Examples include the 64-MiB/10-second reference validator, the
1,200-second package walk, 64/256-MiB governance validators and 30/120-second
times, 16-MiB/10-second CLI processing, and a 120-minute CI provider timeout.
The cited owner sections contain no matching requirements or verification
criteria for those numbers. By the table's own “numeric disagreement” rule,
SEC-TEST-007 must fail.

**Required edit:** For every SEC-LIM column, either point to an identical
numeric value and enforcement/test rule already stated in the owning spec, or
make that owning FR/NFR's existing resource requirement exact before citing
it. Remove security-only numbers that are neither owner refinements nor valid
`not_applicable` rationales. Add one reconciliation fixture per currently
unmatched value.

**Risk:** R-201 and DOD-08; the phase can claim complete pre-allocation limits
that its actual owner tests neither require nor verify.

### M0-011 — carried — the sole acquisition profile has two identities

**Locations:** `10` lines 70/104/259; `13` lines 39/61/67/178; `16` lines
65/68/85/139.
**Finding:** Specs `10` and `16` define the sole network-capable profile as
`m0_input_acquisition`; spec `13` defines the supposedly same job/profile as
`input_acquisition`. Because profile identity is the key used by the capability
inventory and exact reconciliation tests, the text either declares two
profiles or leaves the CI grant unmapped.

**Required edit:** Rename every spec-`13` occurrence to the canonical
`m0_input_acquisition` identifier, including CI-FR-002/008, Sections 2/6.4,
failure diagnostics, and CI-TEST-002/008 evidence. Assert that workflow job ID,
capability profile ID, and the spec-`10` profile ID are byte-identical.

**Risk:** R-001 and R-005; the network grant can evade or fail exact
least-privilege reconciliation.

### M0-R2-001 — new-in-rework — the doctor aggregate row exceeds its schema

**Edit that exposed it:** The rework replaced the doctor result fields with
`fact_paths`/`outcomes` derived from the new reference-validation schema.
**Locations:** `12` lines 102-110 and 128-136.
**Finding:** `M0-DOC-007` aggregates all nine reference outcomes, but every
doctor row is limited to 1-4 `fact_paths` and 1-4 outcomes. The same schema also
permits “one build-fact outcome” for M0-DOC-001 without defining that outcome's
fields, types, status, or code. A valid seven-row document therefore cannot
represent the authoritative inventory exactly, and schema/golden tests have no
single expected shape.

**Required edit:** Define a finite row union for build-fact, component, and
aggregate checks. Give each variant exact fields/types/codes and let the
aggregate variant carry all nine ordered predicate outcomes (or an exact typed
reference to the one validation object). Make `fact_paths` cardinality match
each inventory row and add aggregate/build-fact schema goldens.

**Risk:** R-008; `doctor` can omit evidence or emit a document that contradicts
its complete-inventory claim.

### M0-R2-002 — new-in-rework — higher-is-better misclassifies the exact 3% boundary

**Edit that introduced it:** The new Section 6.4 pair-degradation formula.
**Locations:** `14` lines 68/80 and 265-274.
**Finding:** Lower-is-better uses baseline as the percentage denominator, but
higher-is-better divides `(baseline-candidate)` by the candidate. A candidate
that is exactly 3% below its baseline computes as
`3% / 97% = 30,927,835 ppb`, which is strictly above 30,000,000 and therefore
blocks. That contradicts PERF-FR-007/PERF-NFR-001 and Charter SM-08, which say
the exact 3.0% boundary does not flag.

**Required edit:** Use the baseline value as the denominator for both metric
directions while retaining the sign convention, or otherwise rewrite the
formula so an exact 3% change relative to baseline yields exactly 30,000,000
ppb. Add a higher-is-better golden with baseline 100,000 and candidate 97,000,
plus the adjacent first-above-bound case.

**Risk:** R-002; a permitted boundary change is falsely reported as a
release-blocking regression.

### M0-R2-003 — new-in-rework — the secret-scan release population is circular

**Edit that introduced it:** The new exact PR/release population in spec `16`
Section 6.5.
**Locations:** `11` lines 66-68 and 215-224; `16` lines 232-291, especially
release set 7 at 273-275.
**Finding:** Release set `release_evidence` requires the secret scan to cover
the security report and final external approval marker. The security report
cannot reach its passing final bytes until the scan completes, and the marker
is created only after every validator—including the secret scan—passes. PR
generated/log/artifact sets can similarly include the current scan job's own
outputs. A manifest that hashes every such byte cannot be completed before it
must be validated, so SEC-TEST-002 and GOV-TEST-010 cannot both pass.

**Required edit:** Define an acyclic gate order. Make the pre-approval scan
population include every variable input/evidence byte but explicitly exclude
only the current scan manifest/report and not-yet-created approval marker;
constrain those generated control artifacts to fixed schemas, redacted source
fields, and scanned/validated inputs; then create the marker last. Apply the
same exact self-output rule to PR generated/log/artifact sets and add a gate
ordering fixture proving no required byte depends on its own scan result.

**Risk:** R-005 and R-202; the release gate is impossible to reproduce, or an
implementation must silently omit bytes while claiming 100% coverage.

## Open questions

None. These are contract corrections within existing accepted M0 requirements;
no user product choice is needed.

## Verdict

`needs_work`

M0 cannot be marked READY until carried findings M0-003, M0-006, M0-008,
M0-010, and M0-011 plus new-in-rework findings M0-R2-001 through M0-R2-003 are
resolved. M1-M11 specs are not needed before planning this M0-only factory wave,
so `more_phases` is false.
