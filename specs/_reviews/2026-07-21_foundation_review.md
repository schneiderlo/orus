# Foundation Specification Review — 2026-07-21

**Round:** Re-review after the first-round rework
**Scope:** `SPECS.md`, `specs/00-CHARTER.md`,
`specs/01-GLOSSARY.md`, `specs/02-DECISIONS.md`, and
`specs/03-RISKS.md`
**Verdict:** `needs_work`

All five required input files exist. Planned `specs/10+` domain files remain
Draft roadmap placeholders and were not treated as missing files.

The rework resolves nine of the ten prior blocking findings. It introduces one
new decision defect: D-016 treats the MIT notice identity as unanswered even
though `.factory/questions/q-0001.json` records the human answer, “Use Copyright
(c) 2026 Loic Schneider in the root MIT LICENSE and packaged copies,” at
2026-07-21T21:54:18Z. Because that answer existed before the rework completed,
it is authoritative rework input rather than an open clarification.

## File-by-file status

| File | Status | Summary |
|---|---|---|
| `specs/00-CHARTER.md` | READY | FND-001 through FND-003 are resolved. The diagnostics inventory, architecture-preservation checklist, CI applicability matrix, warning rule, goals, numeric metrics, M0-M11 milestones, constraints, and Definition of Done now have finite owners and observable pass rules. |
| `specs/01-GLOSSARY.md` | READY | FND-004 through FND-007 are resolved. Required abbreviations and formal states are defined, logical-time terms are related explicitly, circular definition pairs are separated, D-015 terminology is binding, and the M2/M4/M8 Trace sequence is explicit. |
| `specs/02-DECISIONS.md` | NEEDS-DETAIL | All 16 entries contain Status, Context, Decision, and Consequences, and D-001 through D-015 remain consistent and traceable. New D-016 contradicts an answered human clarification and is therefore neither a valid unresolved Assumption nor correctly sourced. |
| `specs/03-RISKS.md` | READY | FND-008 and FND-009 are resolved. Every risk has likelihood, impact, mitigation, validation, owner, milestone mapping, and an authoritative roadmap path; the M0-M11 cadence for R-003 and M2-M4 ownership for R-101 now agree with their validation text. |
| `SPECS.md` | NEEDS-DETAIL | The status legend, Domain Spec Template, phase grouping, explicit dependencies, and roadmap coverage of the Charter are complete. Its Foundation and `02-DECISIONS` Ready declarations remain unsupported while D-016 contradicts the recorded owner answer. |

## Prior finding disposition

| Prior ID | Disposition | Re-review evidence |
|---|---|---|
| FND-001 | Resolved | Charter Sections 2.1, G-06, and SM-06 define and reconcile the finite `M0-DOCTOR-v1` inventory and positive/per-row negative evidence. |
| FND-002 | Resolved | Charter Section 2.2 defines five `M0-ARCH-v1` rows with named decisions, roadmap owners, and binary pass rules. |
| FND-003 | Resolved | DOD-04 and its normative paragraph define `M0-CI-APPLICABILITY-v1`, scoped `not_applicable` rationales, and a zero-non-allowlisted-warning rule. |
| FND-004 | Resolved | The glossary adds the previously missing abbreviations, `Reported`, `Approved`, `Observed`, and distinct logical clock/position/ticks/time definitions. |
| FND-005 | Resolved | Branch and Recording now have independent base meanings; Counterfactual and Trace build on them without definition loops. |
| FND-006 | Resolved | Catalog and FlatBuffers now state D-015's accepted SQLite and FlatBuffers scopes. |
| FND-007 | Resolved | Trace identifies the M2 minimum and makes checkpoints optional until M4 and indexes optional until M8. |
| FND-008 | Resolved | Every `specs/*.md` path cited by the risk register matches an authoritative roadmap placeholder, including `61-debug-symbol-adapters.md` for R-204. |
| FND-009 | Resolved | R-003 maps its later-gate validation through M0-M11 owners; R-101 maps M2, M3, and M4 validation to `30`-`32`, `40`-`41`, and `50`. |
| FND-010 | **Carried** | `SPECS.md` still declares Foundation and `02-DECISIONS` Ready although the new D-016 defect prevents all foundation files from being READY. |

## Required Edits

| ID | Classification | File | Section / rework edit | Problem | Risk if left unaddressed | Proposed fix |
|---|---|---|---|---|---|---|
| FND-010 | carried | `SPECS.md` | Foundation status; Foundation specifications table | The top-level Foundation status and `02-DECISIONS` row assert Ready while FND-011 remains unresolved. | Consumers can authorize domain work using a foundation decision that contradicts the owner's recorded answer. | Do not submit the index with these Ready declarations while D-016 remains stale. Once FND-011 is corrected, retain or restore Ready and verify that the `specs/11-governance-release.md` roadmap row still reflects the accepted D-016 notice. |
| FND-011 | new-in-rework | `specs/02-DECISIONS.md` | Decision policy and new D-016 edit (“Require owner-supplied MIT notice identity before public packaging”) | The rework says the holder/year remain unresolved, marks D-016 `Assumption`, and cites the prior review instead of the answered clarification. `.factory/questions/q-0001.json` already contains the human answer `Copyright (c) 2026 Loic Schneider`; the answer predates completion of this rework. | The decision log can override an explicit owner answer with a lower-authority assumption, keep public packaging blocked unnecessarily, trigger duplicate questions, and give the release domain the wrong acceptance input. | Change D-016 to an `Accepted` decision sourced to the answered `q-0001`; state the exact required notice `Copyright (c) 2026 Loic Schneider`; remove the unresolved-value/default-block language from the policy introduction, Context, Decision, and Consequences; and require `specs/11-governance-release.md` plus root/package validation to match that exact approved notice and reject missing, placeholder, or different values. Keep alternatives only as historical rejected options. |

## Open questions

None. The only prior clarification has an authoritative human answer in
`.factory/questions/q-0001.json`; it must be consumed rather than asked again.

## Verdict

`needs_work`

Return FND-010 and FND-011 to the Spec Writer. The next re-review must verify
those two edits and inspect only their changed text and defects introduced by
that rework. The quality bar is unchanged.
