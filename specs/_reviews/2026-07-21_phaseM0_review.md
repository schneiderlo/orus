# M0 Domain Specification Review — 2026-07-21

**Round:** Third review, after rework commit `9caf57f`
**Prior review:** Second-round report at this path, produced by commit `0b707a1`
**Scope:** `SPECS.md`, foundation files `00`-`03`, and all seven M0 domain
specifications (`10`-`16`)
**Verdict:** `needs_work`

The rework resolves seven of the eight findings in the prior report. One prior
finding remains partially unresolved, and one blocking defect was introduced by
the new secret-scan self-output contract. No owner clarification is required:
both findings can be corrected by making the existing M0 contracts exact,
without changing product scope.

Mechanical checks reproduced the performance registry digest, all five new
performance example content digests, the raw-array digest/length, and the
derived runner/result/comparison IDs. The higher-is-better boundary now yields
exactly 30,000,000 ppb at 100000/97000 and 30,010,000 ppb at 100000/96999.

## Status table

| File | Status | Blocking findings | Re-review summary |
|---|---|---|---|
| `SPECS.md` | ACCEPTABLE | None | M0 remains the only delivery wave, all seven M0 specs remain Draft, dependencies are accurate, and M1-M11 remain roadmap-only. |
| `specs/10-build-environment.md` | NEEDS WORK | M0-010 | The acquisition identity and new owner-side resource numbers are correct, but the reference validator still has no exact resource-error mapping. |
| `specs/11-governance-release.md` | NEEDS WORK | M0-010 | Pin grammars, SPDX normalization, release inventories, and the acyclic marker order are now exact; validator RSS/deadline failures still delegate their exact code choice. |
| `specs/12-cli-diagnostics.md` | NEEDS WORK | M0-010 | The closed doctor-row union resolves M0-R2-001; the new RSS/deadline cases do not select one exact top-level error code. |
| `specs/13-ci-quality.md` | NEEDS WORK | M0-010 | The acquisition profile is byte-identical across specs; the new provider-timeout outcome conflicts with the security inventory's required code. |
| `specs/14-performance-foundation.md` | ACCEPTABLE | None | Complete workloads, schemas, raw ordering/statistics, fixed examples, and baseline-relative arithmetic resolve M0-006 and M0-R2-002. |
| `specs/15-concurrent-corpus.md` | ACCEPTABLE | None | PID/TID ownership, child-image distinction, fault mapping, and aggregate equations resolve M0-008. |
| `specs/16-security-foundations.md` | NEEDS WORK | M0-010, M0-R3-001 | Numeric owner limits and the acyclic scan order are substantially corrected, but exact resource outcomes do not reconcile and the excluded final manifest can contain unscanned attacker-derived strings. |

All FR/NFR IDs remain unique within their requirement sections, every FR/NFR
is named in its domain test-plan table, and every domain states no open
question. No M1+ availability claim, CMake path, license change, broad platform
promise, or conflicting Accepted ADR was introduced.

## Prior-finding disposition

| Prior finding | Disposition | Re-review evidence |
|---|---|---|
| M0-003 governance record schemas | RESOLVED | Spec `11` now defines every pin grammar/relationship, a complete SPDX array/generated-value normalization, 12 evidence types, 12 validators, three approval roles, and producer identity/version in each evidence row. |
| M0-006 performance schemas | RESOLVED | Spec `14` now publishes five complete workload records, exact sampling/count/order/statistic rules, and one mechanically reproducible canonical example for each schema. |
| M0-008 corpus report relationships | RESOLVED | Spec `15` now enforces parent/child PID sets, TID ownership/uniqueness, distinct images, the seven fault mappings, typed sorted failure counts, and exact aggregate equations with forged fixtures. |
| M0-010 resource-limit inventory | **CARRIED** | The missing numeric caps now appear in owner requirements/tests, but several new owner-side resource failures still do not name the same exact typed outcome required by the security reconciliation contract. |
| M0-011 network capability model | RESOLVED | `m0_input_acquisition` is now byte-identical in specs `10`, `13`, and `16`, including workflow/profile reconciliation and alias/case mutation tests. |
| M0-R2-001 doctor aggregate cardinality | RESOLVED | Spec `12` now defines a closed build-facts/component/aggregate row union; the aggregate carries one complete nine-outcome validation object and the tests reject wrong variants/cardinalities/order. |
| M0-R2-002 higher-is-better boundary | RESOLVED | Both metric directions now divide by the positive baseline, and the exact/adjacent higher-is-better goldens state the correct ppb values. |
| M0-R2-003 secret-scan release cycle | RESOLVED | Specs `11` and `16` now freeze variable evidence, exclude only the current control pair/future marker, validate the pair, and create the marker last. M0-R3-001 concerns unsafe content newly permitted inside that excluded pair. |

## Cross-spec consistency check

| Contract / concern | Result | Evidence |
|---|---|---|
| M0 scope and foundation decisions | CONSISTENT | All rework remains within M0 and applies D-001 through D-016 without weakening the Charter. |
| Build/reference/CLI facts | CONSISTENT EXCEPT RESOURCE OUTCOME | Fact paths, nine validation outcomes, and seven doctor rows agree; exact RSS/deadline error identity remains blocked by M0-010. |
| Artifact/package/SBOM/evidence identity | CONSISTENT | Subject names, canonical bytes, external digest placement, pin relationships, SPDX normalization, and finite release inventories align across `10`, `11`, and `16`. |
| CI/security capability inventory | CONSISTENT | Every acquisition job/profile identity is the literal `m0_input_acquisition`; alias/case/byte differences fail. |
| Performance authority and comparator | CONSISTENT | Workload/result schemas, raw evidence, authority precedence, and both 3% directions are exact; published hashes reproduced. |
| Concurrent corpus into CI/performance/security | CONSISTENT | Topology, wire values, observation calls, deadlines, sums, fault terminals, report relationships, and resource bounds agree. |
| Resource-limit ownership | BLOCKED | Numeric caps now align, but exact owner error/state mappings do not yet match the security inventory in every named case (M0-010). |
| Secret scan into release gate | BLOCKED | The dependency order is acyclic, but the excluded final manifest still admits unredacted variable path/source strings (M0-R3-001). |
| Risk and ADR application | CONSISTENT WITH FINDINGS | Accepted decisions remain intact; the two findings prevent complete validation of R-005, R-008, R-201, and R-202. |

## Required edits

### M0-010 — carried — resource outcomes still cannot reconcile exactly

**Locations:** spec `10` lines 72, 181-186, and 360; spec `11` lines 62,
64, 66, 288-300, and 429-433; spec `12` lines 63, 151-175, and 275; spec
`13` lines 70, 145-146, 265, 311, and 315; spec `16` lines 168-179,
190-195, and 482.

**Finding:** The rework correctly moved the previously security-only numbers
into their owning specs and added boundary fixtures. Exact reconciliation still
cannot produce one expected result for several resource faults:

- BUILD-FR-010 says memory/deadline excess returns an “applicable
  `BUILD_REFENV_*` error,” but the finite Section 6.3 list contains no resource
  code and the requirement does not select one existing code.
- GOV-FR-004/006/008 and their tests require an “exact Section 6.1.5 code” for
  RSS/deadline excess, but Section 6.1.5 does not map either condition to one
  enum value; `GOV_FIELD_BOUND` is explicitly asserted only for some security
  rows.
- CLI-FR-010 promises the “exact exit-4 error” for RSS/deadline excess but does
  not choose `CLI_CONTRACT_INVALID`, `CLI_RENDER_ERROR`, or
  `CLI_INTERNAL_ERROR`; SEC-LIM-12-01/02 independently choose the first two.
- CI-FR-011 permits provider timeout to yield either `failed` or `incomplete`,
  while SEC-LIM-13-03 requires `CI_EVIDENCE_LIMIT`; CI-TEST-011 proves only
  non-pass. The warning-parser 256-MiB/120-second owner contract also lacks
  explicit exact/first-over cases in CI-TEST-007 even though SEC-TEST-007
  requires the owner test to prove every number.

SEC-TEST-007 therefore cannot compare identical error/state/test columns for
all rows, and several owner tests cannot know which exact error golden to
assert.

**Required edit:** For each newly added RSS/deadline/timeout bound, add one
explicit condition-to-code/state mapping in the owning FR or contract, use that
same literal in the corresponding SEC-LIM row, and name the exact-bound and
first-over fixture plus expected literal in the owner test-plan row. In
particular, select one reference-validator resource code, one governance
resource code (or an explicit existing-code mapping), one CLI code per
collection/render resource case, and one CI provider-timeout terminal code;
add the warning parser's 256-MiB/120-second exact/first-over fixture to
CI-TEST-007. Keep the numeric caps unchanged.

**Risk:** R-201 and DOD-08; implementations can report different typed failures
for the same resource limit while all claiming reconciliation success.

### M0-R3-001 — new-in-rework — the excluded scan manifest can carry an unscanned secret

**Edit that introduced it:** The new Section 6.5 self-output exclusion and
fixed control-pair schema used to resolve M0-R2-003.

**Locations:** spec `16` lines 218-234, 245-255, 271-311, and 477.

**Finding:** The final `M0-SECRET-SCAN-MANIFEST-v1` is deliberately excluded
from its own population, but each entry stores raw `logical_path` (up to 4 KiB)
and raw `source_identity` (up to 512 bytes). Those strings can be derived from
an attacker-controlled Git filename, generated output name, artifact name, or
provider identity. The final report redacts its `logical_path`, but the
excluded manifest does not. Scanning the referenced file bytes does not prove
that their path/source strings contain no credential. Therefore a canary placed
only in a filename or source identity can survive in retained release evidence
while SEC-FR-002/SEC-NFR-001 still claim zero unallowlisted secrets.

**Required edit:** Freeze a canonical pre-scan metadata document containing the
raw path/source strings, include and scan that document as an ordinary
generated/evidence entry, and bind its digest from the final manifest. In the
excluded manifest itself, replace raw path/source values with deterministic
digests plus a bounded redacted display value; forbid raw unmatched source
strings. Add PR and release fixtures that place canaries only in a tracked
filename, generated-output path, log/artifact name, and `source_identity`, and
prove either detection or redaction before the final pair is retained.

**Risk:** R-005 and R-202; the artifact intended to prove zero secret exposure
can itself retain an unscanned secret.

## Open questions

None. Both findings are exactness/security corrections inside already accepted
M0 requirements; no user product choice is needed.

## Verdict

`needs_work`

M0 cannot be marked READY until carried finding M0-010 and new-in-rework
finding M0-R3-001 are resolved. M1-M11 specs are not needed before planning
this M0-only factory wave, so `more_phases` is false.
