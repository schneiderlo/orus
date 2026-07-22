# M0-001 Rework Handoff r1

## Source and check identity

- Rework base revision: `60478f3dc4d9a14812efc3152f5bb4e2df115af8`
- Authoritative rework check run: `run-20260722T114834Z`
- Check evidence: `.factory/logs/checks/M0-001/run-20260722T114834Z/`
- Registered check definitions were not edited.

## Blocker-to-fix mapping

### F-01 — resolved acquisition population was incomplete and simulated

- Added one-to-one immutable inventory, acquisition, and bootstrap-admission
  rows for all 23 modules selected by `bazel mod graph`, the two additional
  fetched BCR assets, and the pinned BCR registry. The full admitted population
  is now 39 coordinates, including the existing 13 Nix inputs/outputs.
- Extended `config/bcr-distdir.json` with the unique name, coordinate, and role
  of every actual BCR materialization. Repository validation now fails when the
  dependency inventory, acquisition manifest, admissions, flake lock, distdir,
  or selected module graph differs.
- Made Nix fetch the registry and archives with the acquisition profile's
  4-GiB per-blob and 1,200-second limits, enforce the 128-coordinate and 16-GiB
  aggregate guards before exposing the distdir to Bazel, and expose only the
  resulting read-only Nix-store materializations.
- Changed the hermeticity audit from pure decision evidence alone to live
  reconciliation. It hashes all 25 materialized archives, verifies 151 locked
  registry metadata files, compares the 23-module live graph, checks 13 Nix
  materialization paths, and rejects writable, missing, over-limit, or
  mismatched inputs. The exact/first-over and rejection matrix remains as
  boundary evidence alongside the real materialization report.
- Added negative coverage for an absent/mismatched selected module population;
  the existing unadmitted, mutable, hash-mismatch, exact-limit, and first-over
  acquisition cases remain active.

Evidence: `c-02.stdout.log` contains the live graph and `c-09.stdout.log`
reports `module_count=23`, `archive_count=25`, `registry_file_count=151`,
`coordinate_count=39`, `path_count=13`, and read-only store modes.

### F-02 — coverage could silently omit task-owned Python logic

- Replaced the one-package allowlist with a finite reviewed manifest of every
  task-owned Python source under `tools/**`. The only exclusions are the two
  empty package-marker files, each with an explicit rationale.
- Widened instrumentation to `//tools/...`, made LCOV parsing source-exact, and
  require non-empty current data for every included source before independently
  applying the 70% threshold to `tools`, `tools/build`, and `tools/coverage`.
- Added tests for an omitted owned source, an invented/unauthorized exclusion,
  missing LCOV source data, absent LCOV, exact 70%, first-below 70%, and the
  complete repository manifest. Command-only audit/format logic is now imported
  and exercised by the build contract suite rather than silently omitted.

Evidence: `c-11.stdout.log` reports `tools=79.41176470588235%`,
`tools/build=75.40983606557377%`, and
`tools/coverage=73.7864077669903%`.

## Files changed

### Task-owned implementation

- `.bazelrc`
- `config/BUILD.bazel`
- `config/bcr-distdir.json`
- `config/dependency-inventory.json`
- `config/input-acquisition.json`
- `flake.nix`
- `tests/build/BUILD.bazel`
- `tests/build/contract_test.py`
- `third_party/bootstrap-admissions.json`
- `tools/BUILD.bazel`
- `tools/build/BUILD.bazel`
- `tools/build/build_contract.py`
- `tools/build/hermeticity_audit.py`
- `tools/coverage/BUILD.bazel`
- `tools/coverage/package_gate.py`
- `tools/coverage/packages.json`
- `tools/format.py`

### Factory/audit metadata

- `.agents/M0-001/handoff-r1.md`
- `.factory/checks/M0-001/runs/run-20260722T114834Z.json`
- `.factory/run.json`
- `.factory/tasks/M0-001.json`
- `IMPLEMENTATION_PLAN.md`

## Commands run and outcomes

| Command | Outcome |
|---|---|
| `python3 tools/format.py` and `git diff --check` | PASS. Canonical JSON and source formatting are clean. |
| `nix develop --command bazel test --test_output=errors --config=dev //tests/build:contract_test` | PASS. 29 contract/negative tests pass, including real BCR materialization and coverage-population checks. |
| `nix develop --command bazel run //tools/build:hermeticity_audit` | PASS. Live graph, actual Nix materializations, hashes, permissions, limits, tool actions, and network policy reconcile. |
| `nix develop --command bazel coverage --config=dev --combined_report=lcov //tests/build/... && nix develop --command bazel run //tools/coverage:package_gate -- --threshold=70` | PASS. All three finite packages exceed 70%. |
| `uv run --no-project /home/lschneid/workspace/schneiderlo/factory/factory.py check run M0-001` | PASS. Full registered run `run-20260722T114834Z`; c-01 through c-11 green. |

## Remaining risks

- The 1,200-second wall limit is wired into the actual Nix fixed-output fetchers
  and its exact/first-over decision behavior is tested without waiting 20 real
  minutes. Actual archive byte counts, aggregate bytes, hashes, registry bytes,
  graph membership, and read-only promotion are exercised directly.
- c-12 and c-13 remain independent verifier judgments. No product behavior,
  downstream task surface, healing link, or GitHub substitute was changed.

No verifier-reported blocker remains open.
