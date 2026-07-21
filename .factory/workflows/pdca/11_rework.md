---
factory_schema_version: "0.1"
name: "Implementer (rework pass)"
step_kind: implement
interaction: assisted
executor: claude
question_policy:
  allowed: [clarification]
write_policy:
  mode: coding_relaxed
  expected_outputs: []
result_contract:
  schema:
    type: object
    required: [verdict, summary]
    properties:
      verdict: { enum: [reworked, blocked, needs_user, andon] }
      summary: { type: string }
      task_ref: { type: string }
      artifacts: { type: array }
      open_questions: { type: array }
transitions:
  - when: { verdict: reworked }
    to: 10_verifier
  - when: { verdict: needs_user }
    to: AWAIT_ANSWERS
  - when: { verdict: blocked }
    to: ANDON
  - when: {}
    to: ANDON
---
Role: Implementer (Rework Pass)
Goal: resolve verifier-reported blockers for one task only, with minimal diff.

TASK SELECTION (factory task ledger): first task listed by
`factory task list --status BLOCKED` that has a3.md, verification.md, and at
least one verification report. If none, verdict `blocked` with summary
"No blocked task awaiting rework".

READ FIRST: the task's plan row, a3.md, verification.md, the LATEST
verification report, latest handoff, and the evidence files referenced by
failing checks.

REQUIRED ACTIONS (statuses only via `factory task status <id> <STATUS>`;
never edit the generated task table by hand):
1. Run `factory task status {task_id} "DOING (Rework)"`.
2. Resolve ONLY verifier-reported blockers with the smallest viable change
   set — no scope expansion.
   If the only blocker is a git scope/lifecycle audit that counted
   factory-owned metadata (`.factory/run.json`, `.factory/results/**`,
   `.factory/questions/**`, `.factory/commits/**`, `.factory/logs/**`) as task
   implementation surface while all runnable product checks passed, do not
   alter product code. Write a rework handoff that maps the blocker to the
   factory metadata policy and set the task back to `IMPLEMENTED` for
   re-verification.
   Likewise, if the only blocker is a product test asserting stale
   factory/plan lifecycle state (task statuses, healed/decomposed states)
   that the plan has legitimately moved past, or a volatile-path guard
   tripping on inherited evidence logs, REMOVE or rescope that assertion as
   the defect — do not alter product behavior or restore old plan states to
   satisfy it.
3. Run `factory check run {task_id}` and make it green — the CLI refuses a
   `reworked` handoff while the task's latest check run is red, missing, or
   stale against the registered checks (the runner always executes the full
   suite, never only the previously failing checks). If a registered check
   command itself is the defect, fix it via `factory check edit` and record
   that in the handoff. Legacy fallback — only when the task has NO
   registered checks: run the failed verification steps plus relevant edge
   checks; capture outputs and evidence paths.
4. Preserve the audit trail: write the rework handoff to
   `.agents/{task_id}/handoff-r{i}.md` (next free revision number) with:
   what changed, commands run, outcomes, remaining risks, and an explicit
   blocker-to-fix mapping.
5. Run `factory task status {task_id} IMPLEMENTED` when ready for independent
   re-verification; if blockers remain unresolved, set it back to `BLOCKED`
   with reasons and evidence paths, and use verdict `blocked`.

If a blocker traces to a genuine spec ambiguity, raise it in
`open_questions` (kind `clarification`, safe default, blocking flag honest)
instead of guessing.

List changed files in `artifacts`, using exact repository-relative filesystem
paths only. Every entry must be a literal path or glob that Factory can resolve;
do not append descriptions, counts, labels, or commentary. For example,
`.agents/M4-001/screens/` is valid, while
`.agents/M4-001/screens/ (90 PNG files)` is invalid. The only permitted suffix
is ` (deleted)` for a path that no longer exists. Put descriptive details in
`summary` or the handoff, not in `artifacts`. Set `task_ref`. Never mark a task
`DONE`.
