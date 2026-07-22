# M0 Domain Specification Review — 2026-07-21

**Round:** Fourth review, after rework commit `444768c`  
**Prior review:** Third-round report at this path, produced by commit `176125f`  
**Scope:** `SPECS.md`, foundation files `00`-`03`, and all seven M0 domain
specifications (`10`-`16`)  
**Verdict:** `needs_work`

The rework resolves the prior secret-scan-manifest exposure and most of the
carried resource-outcome finding. One part of the carried finding remains: the
security inventory still omits the CLI renderer's 16-MiB limit. Three blocking
defects were introduced by the new CI terminal-state wording and secret-scan
metadata design. No owner clarification is required; all four findings are
precision or consistency corrections within existing M0 requirements.

The unchanged build, governance, performance, and corpus contracts remain
acceptable. Requirement-definition IDs remain unique, no M1+ capability or
scope was introduced, and every M0 spec still states that it has no open
question.

## Status table

| File | Status | Blocking findings | Re-review summary |
|---|---|---|---|
| `SPECS.md` | ACCEPTABLE | None | M0 remains the only delivery wave; all seven M0 specs remain Draft and M1-M11 remain roadmap-only. |
| `specs/10-build-environment.md` | ACCEPTABLE | None | Reference-validator and package-identity resource faults now have exact codes, exact boundaries, and first-over fixtures. |
| `specs/11-governance-release.md` | ACCEPTABLE | None | Admission, SBOM, and release-evidence validators now consistently use `GOV_RESOURCE_LIMIT`; gate ordering correctly consumes the new security metadata contract. |
| `specs/12-cli-diagnostics.md` | ACCEPTABLE | None in owner spec | Collection and rendering now select exact codes and tests. The remaining mismatch is in spec `16`'s reconciliation row. |
| `specs/13-ci-quality.md` | NEEDS WORK | M0-R4-001 | Provider timeout is exact, but the broadened failure-mode row conflicts with the existing advisory evidence-bundle state. |
| `specs/14-performance-foundation.md` | ACCEPTABLE | None | Unchanged since the prior accepted review: workloads, schemas, arithmetic, authority, and verification remain exact. |
| `specs/15-concurrent-corpus.md` | ACCEPTABLE | None | Unchanged since the prior accepted review: topology, wire contracts, failure mapping, aggregation, and cleanup remain exact. |
| `specs/16-security-foundations.md` | NEEDS WORK | M0-010, M0-R4-002, M0-R4-003 | The final manifest no longer retains raw attacker-derived names, but the CLI limit inventory is incomplete and the new metadata scan neither defines byte-exact raw-string inspection nor an attributable metadata finding. |

## Prior-finding disposition

| Prior finding | Disposition | Re-review evidence |
|---|---|---|
| M0-010 resource-limit inventory | **CARRIED, PARTIALLY RESOLVED** | Specs `10`, `11`, `12`, and `13` now select exact resource codes/states and exact-bound/first-over tests. Spec `16` still omits the renderer's 16-MiB limit from `SEC-LIM-12-02`, even though its own SEC-TEST-007 requires that owner value to reconcile. |
| M0-R3-001 excluded manifest can retain an unscanned secret | **RESOLVED** | Spec `16` now scans a frozen metadata document, binds it by digest, and permits only digest-derived path/source displays in the excluded manifest. Specs `11` and `16` agree on the acyclic order. M0-R4-002 and M0-R4-003 concern defects introduced inside that new mechanism, not the prior raw-manifest exposure. |

## Cross-spec consistency check

| Contract / concern | Result | Evidence |
|---|---|---|
| M0 scope and foundation decisions | CONSISTENT | The rework remains within M0, preserves the Charter hierarchy, and adds no availability, platform, license, build-system, or publication-authority change. |
| Build/reference/governance resource outcomes | CONSISTENT | Exact codes and boundary tests now agree across specs `10`, `11`, and the corresponding spec-`16` rows. |
| CLI resource ownership | BLOCKED | Spec `12` makes 16 MiB normative for collection and rendering, while `SEC-LIM-12-02` lists the render deadline and output limit but not render RSS (M0-010). |
| CI evidence terminal states | BLOCKED | Spec `13` Section 6.3 makes an advisory bundle over-limit `incomplete`, while CI-FAIL-007 now makes every numeric evidence/log/bundle excess `failed` (M0-R4-001). |
| Secret-scan gate DAG and final-pair content | CONSISTENT WITH FINDINGS | Metadata precedes the final pair; the final manifest uses digest-only displays; the marker remains last. Raw-value scan semantics and finding attribution remain invalid (M0-R4-002/-003). |
| Performance, corpus, and CI applicability | CONSISTENT | Specs `13`-`15` retain compatible workload, corpus, sanitizer/fuzz, authority, and evidence dependencies apart from M0-R4-001. |
| Risk and ADR application | CONSISTENT WITH FINDINGS | Accepted decisions remain intact. The findings prevent complete verification of DOD-08 and mitigations for R-005, R-008, R-201, and R-202. |

## Required edits

### M0-010 — carried — the renderer RSS limit is absent from its security inventory row

**Locations:** spec `12` lines 63, 177-184, 246, and 287; spec `16` lines
175-176, 190-195, and 546.

**Finding:** Spec `12` now makes a 16-MiB peak-RSS ceiling normative in both
collection and render stages and assigns render excess to `CLI_RENDER_ERROR`.
`SEC-LIM-12-01` records 16 MiB only for embedded/input collection.
`SEC-LIM-12-02`, the result/error-output row, records the 256-KiB document and
10-second render bounds but omits the 16-MiB render ceiling. Yet SEC-TEST-007
requires every owner operation to have a byte-identical memory number, exact
error, and owner test. The finite inventory therefore cannot reconcile the
render RSS case it claims to test.

**Required edit:** Add the existing 16-MiB render-stage peak-RSS ceiling to
`SEC-LIM-12-02`, retain `CLI_RENDER_ERROR`/exit 4 and the existing cleanup, and
name CLI-TEST-010's exact-16-MiB and 16-MiB-plus-one-byte render fixtures in the
row. Do not change the numeric limit or owner behavior.

**Risk:** R-201 and DOD-08; the security readiness matrix can pass while
omitting an owner-defined resource limit.

### M0-R4-001 — new-in-rework — CI assigns two terminal states to an advisory bundle over-limit

**Edit that introduced it:** The CI-FAIL-007 rewrite that assigns terminal
state `failed` to every numeric evidence/log/bundle excess while fixing the
provider-timeout state.

**Locations:** spec `13` lines 166-183, 279, and 329; spec `16` lines 179 and
546.

**Finding:** Section 6.3 says a per-job evidence bundle above 64 MiB makes a
blocking job fail but makes an advisory job `incomplete`. CI-FAIL-007 now says
any numeric evidence/log/bundle excess has code `CI_EVIDENCE_LIMIT` and job
state `failed`. CI-TEST-011 says only that evidence over-limits follow their
exact table rule, while SEC-TEST-007 requires exact terminal-state
reconciliation. An advisory bundle first-over fixture therefore has two
normative expected states.

**Required edit:** Narrow CI-FAIL-007's unconditional `failed` state to the
cases already defined as failed, including the known provider timeout and
blocking evidence limits. Preserve Section 6.3's existing advisory-bundle
`incomplete` outcome and state that exact outcome in CI-TEST-011 and the
SEC-LIM-13-03 reconciliation text/fixture. Keep `CI_EVIDENCE_LIMIT` and the
numeric limits unchanged.

**Risk:** R-008, R-201, and DOD-08; CI, the gate aggregator, and security
reconciliation can disagree on the same typed evidence record.

### M0-R4-002 — new-in-rework — scanning canonical JSON does not define a scan of the raw metadata strings

**Edit that introduced it:** The new `M0-SECRET-SCAN-METADATA-v1` document and
the rule that it is scanned as one ordinary population byte source.

**Locations:** spec `16` lines 218-234, 267-270, 355-374, and 541.

**Finding:** SEC-FR-002 requires every raw `logical_path` and
`source_identity` string to be scanned. The new contract instead serializes
those values into canonical JSON and scans that document as an ordinary byte
stream. Canonical JSON escapes quotes, backslashes, and control characters, so
the scanner is not necessarily presented with the original UTF-8 value bytes.
The test matrix has filename/source-identity canaries but no escaping mutation
and does not select a semantic decoded-field scan. Consequently an accepted
implementation can scan only escaped JSON bytes while claiming that every raw
string was inspected.

**Required edit:** Define one exact byte-preserving scan input for each decoded
metadata value (for example, scan the decoded UTF-8 field bytes under a
length-delimited field contract bound to the canonical metadata digest) and
state how its results reconcile to the metadata row. Add filename and
`source_identity` fixtures containing quote, backslash, and escaped control/LF
boundaries; the detector must receive the original value bytes and produce the
existing secret outcome. Preserve the canonical metadata document, digest,
population, and final-pair exclusion rules.

**Risk:** R-005 and R-202; the new proof can say raw names were scanned when
only a transformed serialization was examined.

### M0-R4-003 — new-in-rework — metadata-only findings cannot identify the offending tuple or field

**Edit that introduced it:** The new metadata-only canary flow combined with
the rewritten digest-only `M0-SECRET-SCAN-REPORT-v1` finding schema.

**Locations:** spec `16` lines 221-230, 333-344, 372-374, 489-502, and 541.

**Finding:** A secret occurring only in an original filename or
`source_identity` is detected inside the single metadata document. Its report
row contains the metadata document's `logical_path_sha256` and content digest,
not a digest locator for the original metadata tuple or an enum identifying
`logical_path` versus `source_identity`. Multiple metadata-only findings can
therefore have the same report identity after redaction, and the prescribed
recovery cannot tell the owner which source name must be removed or reviewed.
The interface is concrete but does not carry enough information to verify the
eight metadata-only canaries independently or execute SEC-FAIL-001 recovery.

**Required edit:** Extend the existing finding contract for a metadata-origin
finding with a deterministic non-secret locator for the original tuple and an
exact field enum (`logical_path` or `source_identity`); define its derivation
from the scanned metadata and its one-to-one manifest mapping. Add a fixture
with the same rule/canary in two different metadata rows and prove distinct
locators, exact counts, no raw name/secret in the final pair, and actionable
recovery. Do not restore raw path/source values to the excluded pair.

**Risk:** R-202 and DOD-08; a gate can detect a secret but cannot attribute or
remediate it from the retained bounded evidence.

## Open questions

None. These are exactness, consistency, and recovery defects within accepted
M0 scope; they do not require a product or policy choice.

## Verdict

`needs_work`

M0 cannot be marked READY until carried finding M0-010 and new-in-rework
findings M0-R4-001 through M0-R4-003 are resolved. M1-M11 specs are not needed
before planning this M0-only factory wave, so `more_phases` is false.
