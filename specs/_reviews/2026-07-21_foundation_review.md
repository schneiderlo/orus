# Foundation Specification Review — 2026-07-21

**Round:** First review (no prior foundation review existed under
`specs/_reviews/`)  
**Scope:** `SPECS.md`, `00-CHARTER.md`, `01-GLOSSARY.md`,
`02-DECISIONS.md`, and `03-RISKS.md`  
**Verdict:** `needs_work`

All five required input files exist. The planned domain specifications under
`specs/10+` are correctly represented as Draft roadmap placeholders and were
not treated as missing files.

## File-by-file status

| File | Status | Summary |
|---|---|---|
| `specs/00-CHARTER.md` | NEEDS-DETAIL | Goals, non-goals, numeric success metrics, M0-M11 milestone table, constraints, and most Definition of Done rows are strong. Three M0 gates still depend on undefined inventories or subjective review language. |
| `specs/01-GLOSSARY.md` | NEEDS-DETAIL | Broad coverage is present, but abbreviations/formal states are missing, two definition pairs are circular, one accepted technology decision is described as a candidate, and the Trace definition conflicts with milestone sequencing. |
| `specs/02-DECISIONS.md` | READY | Every D-001 through D-015 entry has Status, Context, Decision, and Consequences. Discovery-derived decisions trace to the answered question and confirmed assumption logs; seed-derived decisions cite the relevant seed sections. No conflict among the decisions was found. |
| `specs/03-RISKS.md` | NEEDS-DETAIL | Every risk has likelihood, impact, mitigation, validation, ownership, and a nominal mapping, and high-impact risks have concrete mitigation directions. Many roadmap paths and some milestone mappings are stale or incomplete, so traceability is not reliable. |
| `SPECS.md` | NEEDS-DETAIL | The status legend and Domain Spec Template are present; phases and dependencies are logical; and the roadmap covers the Charter's M0-M11 scope. Its top-level Foundation status and four Ready rows are not accurate while the blocking findings in this report remain. |

## Required Edits

| ID | File | Section | Problem | Risk if left unaddressed | Proposed fix |
|---|---|---|---|---|---|
| FND-001 | `specs/00-CHARTER.md` | G-06 and SM-06 | The pass condition refers to “every mandatory M0 check,” but no finite mandatory-check inventory or normative owner/reference is given. The “compatible” and “deliberately incompatible” fixtures are likewise not tied to that inventory. | `orus doctor` could omit an important check while still satisfying the words “every mandatory check,” and two implementations could claim success using different check sets. | Enumerate the mandatory M0 check IDs and their pass/fail/exit behavior, or make an explicit normative reference to the versioned finite inventory that `specs/12-cli-diagnostics.md` must define. Require the positive and negative fixtures to cover that inventory. |
| FND-002 | `specs/00-CHARTER.md` | G-07 | “Reserve” five future architecture concerns and verify them by “architecture-policy review” does not identify observable M0 artifacts or a binary pass rule. The no-false-claim portion is testable, but the preservation portion is not. | A reviewer can pass or fail the goal subjectively, and M0 can claim to preserve future boundaries without evidence that constrains later domain work. | Replace the preservation half with a finite checklist of the M0 artifacts/assertions that demonstrate each named boundary, or name a committed policy checklist whose individual entries have binary review/test evidence. Retain the existing release-claim test for the no-fake-capability half. |
| FND-003 | `specs/00-CHARTER.md` | DOD-04 | “Relevant ASan/UBSan runs” and “no unexplained warning” have no applicability inventory or objective warning disposition rule. | A failing target can be declared irrelevant, or warnings can be accepted or rejected inconsistently, making Definition of Done non-repeatable. | Point DOD-04 to the finite CI target/applicability matrix owned by `specs/13-ci-quality.md`, and define passing as zero non-allowlisted warnings (or an equivalent objective rule) with each exception carrying a written scoped rationale. |
| FND-004 | `specs/01-GLOSSARY.md` | Whole glossary; roadmap terminology | Abbreviation and formal-term coverage is incomplete. At minimum, foundation text uses ABI, API, ARM64, AST, CLA, CLI, CPU, FD, GPU, HTTP, I/O, JSON, OS, OSI, PC, PR, SLO, TLS, UI, UX, and the formal states `Reported`, `Approved`, and `Observed` without an expansion/definition. `logical clock`, `logical position`, `logical time`, and the defined `logical ticks` are also not related explicitly. TLS is especially ambiguous because it can mean thread-local storage here or a transport-security protocol elsewhere. | Domain writers can give the same token different meanings, schemas can use inconsistent time coordinates or state names, and requirements will not share a testable vocabulary. | Add concise entries or expand each term at first normative use. Define the formal investigation/evidence states and state whether the logical-time phrases are synonyms or distinct fields. Where a product/library name is not officially an acronym, say so instead of inventing an expansion. |
| FND-005 | `specs/01-GLOSSARY.md` | Branch / Counterfactual; Recording / Trace | `Branch` is defined through “counterfactual,” while `Counterfactual` is defined through “replay branch.” `Recording` is defined as an artifact whose concrete artifact is a `Trace`, while `Trace` is defined as the recorded execution artifact. These pairs do not provide independent base meanings. | Domain specs can disagree on whether all branches are counterfactual and whether “recording” means the capture operation, logical evidence set, or stored trace artifact. | Give each base term an independent definition. In particular, distinguish a branch's lineage/state from the special case that applies interventions, and distinguish capture/logical recording from the concrete stored trace representation. |
| FND-006 | `specs/01-GLOSSARY.md` | Catalog and FlatBuffers | The glossary says SQLite “may be used” for the Catalog and calls FlatBuffers a “candidate,” while accepted D-015 requires SQLite for bounded catalog metadata and FlatBuffers for typed boundary messages. | A domain writer could reopen or substitute an already accepted foundation choice without a superseding ADR. | Update both entries to reflect D-015's accepted status and retain the existing scope limits: SQLite only for bounded catalog metadata and FlatBuffers only for typed boundary messages, not hot-path events/native layouts. |
| FND-007 | `specs/01-GLOSSARY.md` | Trace | The Trace definition lists checkpoints and indexes as constituents without marking them milestone- or schema-optional, while the Charter and roadmap require a valid trace in M2 and introduce checkpoints only in M4. | M2 acceptance can be read as impossible without an M4 capability, or implementations can disagree on the minimum valid trace structure. | State which trace components are universally required and which are present only when introduced/applicable under the trace schema; explicitly preserve the M2-before-M4 sequence. |
| FND-008 | `specs/03-RISKS.md` | R-101, R-102, R-104 through R-106, R-201 through R-205, and R-301 through R-304 | The register cites seven paths that do not exist in the authoritative roadmap: `30-trace-recording`, `40-deterministic-replay`, `70-studio-gateway`, `90-causal-analysis`, `100-agent-investigation`, `110-counterfactual-verification`, and `120-production-causality`. The roadmap instead splits or renames these responsibilities across `30`-`32`, `40`-`41`, `70-gateway-studio`, `90`-`91`, `100`-`101`, `110`-`111`, and `120`-`121`. | Risk ownership will not reach the domain spec that must mitigate and validate the risk; automated traceability and gate reviews will follow nonexistent files. | Audit each affected row by responsibility rather than doing blind string replacement. Map storage, pipeline, policy, replay, validation, indexing, analysis, agent, intervention, patch, production-recording, and cross-layer concerns to the exact authoritative paths in `SPECS.md`. Include `61-debug-symbol-adapters.md` for symbol risk R-204. |
| FND-009 | `specs/03-RISKS.md` | R-003 and R-101; milestone/spec mappings | R-003's validation says “at every later gate,” but its mapping ends at M3. R-101 maps to M2-M4 while naming only a nonexistent M3 replay file; it names neither the M2 owning specs nor M4 `50-checkpoints-reverse.md`. | A risk can disappear from the gate cadence before its own stated validation ends, or a gate can have no domain owner responsible for its validation. | Make the prose, milestone range, and owning spec list agree. For R-003 either bound the validation to M0-M3 or extend its mapping to the later gates/specs actually intended. For R-101 add the actual M2/M3/M4 owners if the M2-M4 range is retained, or narrow the range to its real scope. |
| FND-010 | `SPECS.md` | Foundation status; Foundation specifications table | `Foundation status: Ready` and all four Ready rows assert the readiness condition that this review disproves. The roadmap itself otherwise passes scope, phase, dependency, legend, and template checks. | Consumers may authorize implementation on the basis of a failed foundation gate. | During rework, represent affected foundation items as Draft/not Ready (or avoid publishing the interim index). Restore the top-level and row statuses to Ready only after FND-001 through FND-009 are resolved and verification passes. Do not create any `specs/10+` files as part of this correction. |

## Open clarification

The following does not change this round's `needs_work` verdict and is not
required to complete this review, but the exact value cannot be derived from the
existing files:

- **Licensing notice:** What exact copyright year and legal holder name should
  appear in the root MIT `LICENSE` and packaged copies? Until answered, the safe
  assumption is to require an owner-supplied notice in the release domain spec
  and block public packaging rather than invent a holder or ship a placeholder.

## Verdict

`needs_work`

All foundation files are not READY. Return FND-001 through FND-010 to the Spec
Writer in one rework pass. On re-review, verify every finding, then inspect only
the changed text and defects introduced by those edits under the convergence
rules.
