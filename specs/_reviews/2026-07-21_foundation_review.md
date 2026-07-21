# Foundation Specification Review — 2026-07-21

**Round:** Re-review after the D-016 rework
**Scope:** `SPECS.md`, `specs/00-CHARTER.md`,
`specs/01-GLOSSARY.md`, `specs/02-DECISIONS.md`, and
`specs/03-RISKS.md`
**Verdict:** `needs_work`

All five required input files exist. Planned `specs/10+` domain files remain
Draft roadmap placeholders and were not treated as missing files.

The rework resolves prior finding FND-011: D-016 is now an Accepted decision,
is traceable to the human answer in `q-0001`, states the exact approved notice,
and gives concrete positive and negative release validation. The corresponding
`specs/11-governance-release.md` roadmap edit preserves full Charter coverage.

Two contradictions introduced by that edit still block READY. The general
supersession policy has not been updated to include newly human-answer-sourced
D-016, and D-016 asserts that no MIT-notice clarification remains open while
`.factory/questions/q-0002.json` is still open with the same question already
answered in `q-0001`. Because both defects are resolvable from existing
evidence, no user answer is required.

## File-by-file status

| File | Status | Summary |
|---|---|---|
| `specs/00-CHARTER.md` | READY | Unchanged since the prior review. Its measurable goals, numeric success metrics, M0-M11 milestones, finite diagnostic and architecture inventories, explicit constraints, and actionable Definition of Done remain complete. |
| `specs/01-GLOSSARY.md` | READY | Unchanged since the prior review. Required terms and abbreviations remain defined without the previously corrected circularities or phase ambiguity. |
| `specs/02-DECISIONS.md` | NEEDS-DETAIL | FND-011 is resolved and all ADRs retain Status, Context, Decision, and Consequences, but the D-016 edit introduced two decision-policy/state contradictions (FND-012 and FND-013). |
| `specs/03-RISKS.md` | READY | Unchanged since the prior review. Every risk retains likelihood, impact, mitigation, validation, owner, and consistent spec/milestone mappings. |
| `SPECS.md` | NEEDS-DETAIL | The changed governance/release roadmap row correctly consumes D-016 and all Charter scope, phase groupings, dependencies, the Status Legend, and the Domain Spec Template remain covered. Foundation and `02-DECISIONS` cannot yet be declared Ready while FND-012 and FND-013 remain. |

## Prior finding disposition

| Prior ID | Disposition | Re-review evidence |
|---|---|---|
| FND-001 through FND-009 | Resolved and unchanged | Only `SPECS.md` and `specs/02-DECISIONS.md` changed after the prior review; the edits do not disturb the previously accepted Charter, Glossary, Risks, or roadmap coverage. |
| FND-010 | **Carried** | `SPECS.md` still declares Foundation and `02-DECISIONS` Ready while the new D-016-related contradictions prevent every foundation file from being READY. |
| FND-011 | Resolved | D-016 is Accepted, cites the answered `q-0001`, uses the literal `Copyright (c) 2026 Loic Schneider`, removes the unresolved/default-block language, and specifies exact-copy and negative-fixture validation. `SPECS.md` now gives the same binding input to the governance/release domain. |

## Required Edits

| ID | Classification | File | Section / rework edit | Problem | Risk if left unaddressed | Proposed fix |
|---|---|---|---|---|---|---|
| FND-010 | carried | `SPECS.md` | Foundation status; Foundation specifications table | The top-level Foundation status and `02-DECISIONS` row assert Ready while FND-012 and FND-013 remain unresolved. | Consumers can authorize domain work from a decision log whose authority and clarification state are internally inconsistent. | Do not submit these Ready declarations while the decision defects remain. Once FND-012 and FND-013 are corrected, the declarations may remain Ready; the revised `specs/11-governance-release.md` roadmap row itself needs no scope expansion. |
| FND-012 | new-in-rework | `specs/02-DECISIONS.md` | Section 2, “Superseding a decision”; exposed by changing D-016 to `Accepted` with source “Human answer to factory clarification `q-0001`” | The general policy says decisions sourced from an explicit user answer are D-001 through D-010 only. D-016 is now also explicitly sourced to a human owner answer. D-016's local Consequences require owner approval, but the supposedly general enumeration omits it. | A later ADR author can read the general policy as excluding D-016 from protected user-answer decisions, creating inconsistent supersession rules and a path to silently replace an approved legal notice. | Update the general rule to cover D-016 as well as D-001 through D-010, or state the rule generically for every decision sourced from an explicit human/owner answer and list D-016 as an included example. Preserve the existing owner-approval rule; do not add a new approval class. |
| FND-013 | new-in-rework | `specs/02-DECISIONS.md` and `.factory/questions/q-0002.json` | D-016 Consequences bullet “No MIT-notice clarification remains open”; exposed by adding that absolute state claim | The statement is false against the current factory record: `q-0002` is `open` and asks the same holder/year question answered authoritatively by human `q-0001`. This is not a product ambiguity; it is an unreconciled duplicate question state. | The factory can route a duplicate question, accept a divergent second answer, or leave consumers unable to verify which clarification state the decision describes. | Reconcile `q-0002` to the existing `q-0001` answer without asking the user again (for example, mark it answered, superseded, or withdrawn as the workflow supports). In D-016, remove the absolute workflow-state bullet or replace it with a precise trace statement that the duplicate is resolved by `q-0001`; keep the exact approved notice unchanged. |

## Open questions

None. `q-0001` already contains the authoritative human answer, so `q-0002`
is a duplicate state-reconciliation defect rather than a clarification to route.

## Verdict

`needs_work`

Return FND-010, FND-012, and FND-013 to the Spec Writer. The next re-review
must verify those findings and inspect only the edits made to resolve them and
any defects introduced by that rework. The quality bar is unchanged.
