# M0-002 Rework Handoff r3

## Source and decision

- Rework base / failed-verification wrapper revision:
  `44cbe985bc9d111ceed2d5edbd957b8dfda84574`
- Authoritative verifier report:
  `.agents/M0-002/verification-report-r2.md`
- Exact checked Git revision:
  `44cbe985bc9d111ceed2d5edbd957b8dfda84574`
- Exact checked product source snapshot SHA-256:
  `6254a2434aaaa4cfd8dfefede9f42be084bd6a2aa173a4215d704a456c083391`
- Authoritative full Factory check run: `run-20260722T190809Z` (green)
- Rework status at handoff: ready for independent c-06/c-07 re-verification;
  this handoff does not mark the task `DONE`.

The source snapshot above is the c-05 provenance identity for the checked
working tree at the stated Git revision. The only product changes relative to
that revision are the two paths listed below.

## What changed

- Replaced the approval timestamp helper's broad day/month checks with the
  complete proleptic-Gregorian month-length relationship and the RFC 3339 leap
  year rule (divisible by 4, except centuries not divisible by 400).
- Preserved the exact 20-byte UTC-seconds profile (`YYYY-MM-DDTHH:MM:SSZ`) and
  clock bounds. `:60` is accepted only at `23:59` on the last calendar day of
  a month, matching RFC 3339's inserted-leap-second position; every other use
  of second 60 is rejected.
- Added production-validator fixtures for 30/31-day month endings, Gregorian
  leap and non-leap centuries, ordinary leap-day boundaries, the valid
  `2016-12-31T23:59:60Z` leap second, impossible calendar dates, zero month/day,
  invalid clock fields, and invalid leap-second day/minute placement.
- Every negative fixture asserts the frozen public governance diagnostic:
  schema `M0-GOV-ERROR-v1`, code `GOV_RELATIONSHIP_INVALID`, contract
  `M0-RELEASE-EVIDENCE-v1`, field path `$.approvals[]`, and release record ID.

## Blocker-to-fix mapping

| Verifier blocker | Fix and retained evidence |
|---|---|
| Approval `time` accepted impossible month/day combinations and did not apply Gregorian leap-year behavior. | `contracts/evidence_contracts.cc` now derives the exact maximum day from month/year and applies the RFC 3339 Gregorian rule. `tests/contracts/evidence_test.cc` proves February 2000/2024 acceptance and February 1900/2023 plus February 30/31 and April 31 rejection through `ValidateGovernanceDocument`. |
| Approval `time` rejected RFC 3339's leap-second form and lacked boundary/placement tests. | The production helper accepts second 60 only at `23:59` on the month's last date. The governance test accepts `2016-12-31T23:59:60Z` and rejects second 60 on the prior day or prior minute, plus second 61. |
| The negative corpus did not prove the literal typed governance error. | Each impossible-date/time fixture asserts the complete stable schema/code/contract/path/record diagnostic returned by the public production validator. |
| Append-only handoff and complete registered check refresh were required. | This `handoff-r3.md` records the exact source snapshot and green `run-20260722T190809Z`; retained stdout/stderr is under `.factory/logs/checks/M0-002/run-20260722T190809Z/`. |

## Commands run and outcomes

| Command | Outcome |
|---|---|
| `uv run --no-project /home/lschneid/workspace/schneiderlo/factory/factory.py status` | Workflow running at `11_rework`; no open question reported. |
| `uv run --no-project /home/lschneid/workspace/schneiderlo/factory/factory.py task status M0-002 "DOING (Rework)"` | `BLOCKED -> DOING (Rework)`. |
| `nix develop --command bazel test --config=dev //tests/contracts:evidence_test` | Passed the focused production governance target, including all new RFC 3339 fixtures. |
| `nix develop --command bazel run //tools:format` | Passed. |
| `uv run --no-project /home/lschneid/workspace/schneiderlo/factory/factory.py check run M0-002` | `run-20260722T190809Z` green: c-01 dev/release 242.0 s, c-02 ASan 98.4 s, c-03 UBSan 142.4 s, c-04 native fuzz 87.8 s, and c-05 fresh coverage 169.5 s. |
| `git diff --check` | Passed. |
| `uv run --no-project /home/lschneid/workspace/schneiderlo/factory/factory.py task status M0-002 IMPLEMENTED` | `DOING (Rework) -> IMPLEMENTED`; ready for independent re-verification. |

The authoritative c-05 run executed 9/9 tests and retained the existing
per-package threshold and scope:

| Package | Line coverage |
|---|---:|
| `contracts` | 85.824992% |
| `python/orus_contracts` | 85.593220% |
| `tools` | 79.411765% |
| `tools/build` | 75.321337% |
| `tools/coverage` | 71.038251% |

The c-05 provenance binds Git revision `44cbe985bc9d...`, source snapshot
`6254a2434aaa...`, manifest digest `acd0a4e88de1...`, and LCOV digest
`6c3fd27bbc7d...`.

## Changed paths

### Product implementation and tests

- `contracts/evidence_contracts.cc`
- `tests/contracts/evidence_test.cc`

### Audit and Factory evidence

- `.agents/M0-002/handoff-r3.md`
- `.factory/checks/M0-002/runs/run-20260722T190809Z.json`
- `.factory/logs/checks/M0-002/run-20260722T190809Z/`
- `.factory/run.json`
- `.factory/tasks/M0-002.json`
- `IMPLEMENTATION_PLAN.md`

## Remaining risks and ownership limits

- c-06 and c-07 remain independent verifier judgments; this handoff does not
  pre-approve them.
- The shared validator recognizes the RFC 3339 UTC calendar and the legal
  structural position of an inserted leap second. It does not fetch or freeze
  a volatile IERS announcement table; actual approval-time generation and
  approval workflow remain M0-004-owned.
- No governance producer, approval workflow, final scan, marker behavior,
  dependency pin, coverage command, package scope, threshold, or public API
  changed.
- No specification ambiguity, temporary substitute, Pending Healing, or
  blocking open question remains.

## Pending Healing

None.
