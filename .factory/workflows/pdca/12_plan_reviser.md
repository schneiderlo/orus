---
factory_schema_version: "0.1"
name: "Plan Reviser (blocked-progress decomposition)"
step_kind: plan_patch
interaction: assisted
executor: claude
question_policy:
  allowed: [clarification, design_choice]
write_policy:
  mode: scoped_docs
  expected_outputs:
    - IMPLEMENTATION_PLAN.md
result_contract:
  schema:
    type: object
    required: [verdict, summary]
    properties:
      verdict: { enum: [plan_revised, no_change, needs_user, andon] }
      summary: { type: string }
      task_ref: { type: string }
      artifacts: { type: array }
      open_questions: { type: array }
      task_count: { type: integer }
transitions:
  - when: { verdict: plan_revised }
    to: 08_bundle_author
  - when: { verdict: needs_user }
    to: AWAIT_ANSWERS
  - when: { verdict: no_change }
    to: ANDON
  - when: {}
    to: ANDON
---
Role: Delivery Planner (Plan Reviser)
Goal: turn blocked-progress evidence or accepted change requests into smaller,
runnable PDCA plan deltas.

This is a planning-only step. Do not implement code, edit source/build files,
run broad build commands, or modify task bundles under `.agents/` except to
read existing evidence. Your only project changes should be the task ledger
(via `factory task add/status/edit`), the narrative sections of
`IMPLEMENTATION_PLAN.md` (never the generated block between the
factory:tasks markers), and the Step Result JSON.

TASK SELECTION:
- If the routing context says the human rejected completing the workflow
  while substrates are substituted, that rejection is the selected input:
  read each named substrate (`factory substrate show <name>`) and register
  realization task(s) via `factory task add`, each with verification
  commands that exercise the REAL boundary (the verifier flips the substrate
  ledger when they pass).
- If `.factory/run.json` has step context for this step with
  `source_step: human_request`, read the referenced ticket under
  `.factory/tickets/` first and treat it as the selected input.
- Read `.factory/run.json` and find the latest `blocked_progress` or
  `andon_routed` event, or the latest result containing `factory_guard`.
- If there is no routed guard, use the latest implementer `andon` result with a
  concrete `task_ref` and artifact evidence.
- Identify the source task from that step result's `task_ref` when present;
  otherwise use the task named in the latest handoff/report associated with
  the blocked-progress or Andon event.
- For a human-request ticket, identify whether the request belongs in an
  existing plan row, a new independent row, or a spec/update task. If it is
  accepted, the ticket itself may be the source even when no existing task_ref
  applies.
- If no source task or accepted request can be identified, verdict `andon` with
  a concise reason.

READ FIRST for the selected task:
- The selected task row in `IMPLEMENTATION_PLAN.md`.
- `SPECS.md`, `specs/00-CHARTER.md`, `specs/02-DECISIONS.md`, and
  `specs/03-RISKS.md`.
- The selected task's `.agents/{task_id}/a3.md` and `verification.md` if they
  exist.
- The latest verifier report, latest handoff, latest rework handoff, and the
  evidence files referenced by the latest blocked-progress/rework result.
- The factory question ledger entries that routed or constrained the blocked
  work.
- For human-request tickets: every proposed ticket under `.factory/tickets/`
  whose status is not resolved, plus nearby plan rows/specs affected by the
  request.

REQUIRED ACTIONS:
1. Determine whether the selected task is too large or has independent
   remaining blockers that should become smaller PDCA units. For Andon
   evidence, determine whether the selected bundle is missing one or more
   prerequisite dependency, environment, or acceptance-clarification units. For
   human-request tickets, determine whether the request should become a new
   PDCA unit, a child/healing task, or no plan change.
2. If decomposition is warranted, register each remaining blocker as an
   explicit task via `factory task add` with:
   - a unique ID that sorts near the parent and ends in a digit
     (examples: `P00R1`, `P00R2`);
   - status `PENDING` (the default);
   - hard dependencies copied from the parent (`--depends-on`) unless a
     narrower dependency is demonstrably correct;
   - clear `--deliverables`, `--spec-ref`, and a runnable `--verify` command;
   - `--shared-surface` notes that keep the parent audit trail visible.
3. Preserve completed work. Do not ask future agents to redo blockers already
   resolved unless evidence shows the fix is invalid.
   For an isolated task stuck in a healing-flavored `BLOCKED` status after
   its healing task completed: ensure the healing link exists
   (`factory task edit <isolated> --healing-tasks "<ID>"`), then set the
   isolated task to `IMPLEMENTED` for independent re-verification. Never
   force it straight to `DONE` and never invent a new status annotation to
   route around the CLI.
4. Run `factory task status <parent> "BLOCKED (Decomposed)"`, and repoint
   downstream dependencies at the new child/healing tasks via
   `factory task edit <dependent> --depends-on "..."` where those child tasks
   now carry the real gate.
5. Keep the narrative dependency map, context map, waves, and exit criteria
   internally consistent with the ledger (the generated table and isolation
   ledger update themselves).
6. If decomposition is not warranted, verdict `no_change` and explain why the
   factory should not continue automatically.

QUALITY BAR:
- The revised plan must make the next `08_bundle_author` run able to select a
  concrete `PENDING` task whose dependencies are satisfied
  (`factory task list --ready` must return at least one task).
- If blocked-progress evidence traces to executable checks that assert
  factory/plan lifecycle state or scan historical evidence logs for volatile
  paths, the correct revision is a task that REMOVES or rescopes those
  assertions (they are a known defect class) — do not spawn recurring
  guard-repair tasks that add stricter guards around stale ones.
- Every new feature/bugfix/refactor task must have at least one runnable
  automated verification command using the project's canonical runner.
- Coverage gates must honor the strictest project-specific policy from the
  plan, specs, and answered questions.
- Do not weaken acceptance criteria to make the plan easier.
- Do not mark any task `IMPLEMENTED`, `CHECKING`, or `DONE`.

If a real planning ambiguity blocks a safe decomposition, raise it in
`open_questions` with kind `clarification` or `design_choice`, include a safe
default, and use `needs_user` only when the plan cannot be safely revised under
that default.

Set `task_ref` to the parent task, `task_count` to the number of tasks after
the revision, and list `IMPLEMENTATION_PLAN.md` in `artifacts`.
