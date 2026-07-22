# M0-001 Rework Handoff r2

## Source and check identity

- Rework base revision: `b5b67acc2bd7100cd95c6e7cf794a4fd0391896d`
- Authoritative rework check run: `run-20260722T134530Z`
- Check evidence: `.factory/logs/checks/M0-001/run-20260722T134530Z/`
- Registered check definitions were not edited.

## Blocker-to-fix mapping

### F-01 — actual acquisition was not one bounded pre-promotion profile

1. **Unadmitted BCR rows could fetch before reconciliation.**
   `tools/build/acquisition_profile.py` is now the single BCR acquisition
   owner. Before opening any transport, it calls the repository population
   validator, which reconciles all 39 Nix/Bzlmod coordinates across the
   dependency inventory, acquisition admissions, flake lock, selected BCR
   archives, and registry. Its own preflight rejects an unadmitted, mutable,
   duplicate, digest-mismatched, or over-count request before a transport call.
2. **The 1,200-second limit was per fetch rather than whole-profile.**
   One monotonic deadline now covers the complete transaction. Each transport
   receives only the profile time remaining, and the state machine checks the
   deadline again before any promotion. The deterministic fixed-output report
   records that the 1,200-second deadline is enforced.
3. **Aggregate bytes were checked after constituent Nix-store promotion.**
   `flake.nix` now creates one fixed-output acquisition derivation instead of
   25 `fetchurl` paths plus a separate `fetchzip`. All raw bytes stay in the
   derivation's quarantine directory while the per-blob and 16-GiB aggregate
   budgets and every digest are checked. Only the complete verified set is
   copied into one read-only output containing `distdir/` and `registry/`;
   failed derivations expose no output path to Bazel.
4. **c-09 negative evidence used a pure arithmetic decision helper.**
   c-09 now drives exact and first-over coordinate, blob, aggregate-byte, and
   time cases through `ProfileAcquirer`, the same admission/quarantine/
   verification/promotion state machine used by the real Nix transaction.
   Its bounded synthetic transport avoids 16-GiB allocations and 20-minute
   waits while still creating quarantine bytes and proving the promotion
   target absent after every rejection. Unadmitted and over-count cases also
   prove zero transport calls.
5. **The live report did not demonstrate one shared transaction.**
   `tools/build/hermeticity_audit.py` now requires `ORUS_BCR_DISTDIR` and
   `ORUS_BCR_REGISTRY` to be children of the same Nix-store acquisition
   profile, validates its retained transaction report, and reports 26 fetched
   BCR coordinates, 58,273,138 aggregate fetched bytes, deadline enforcement,
   read-only mode, 23 selected modules, and 151 locked registry files.

The registry coordinate and revision are unchanged. Its recorded SHA-256 was
corrected from the former `fetchzip` recursive-output hash to the raw archive
SHA-256 `af1981be211b9d95f4fc4f35e3d50f8413e25436ac669a538f608345ab7bd83a`
so the transaction can verify the downloaded blob before interpreting or
extracting it.

## Files changed

### Task-owned implementation

- `.bazelrc`
- `config/bcr-distdir.json`
- `config/dependency-inventory.json`
- `config/input-acquisition.json`
- `flake.nix`
- `tests/build/BUILD.bazel`
- `tests/build/contract_test.py`
- `tools/build/BUILD.bazel`
- `tools/build/acquisition_profile.py`
- `tools/build/hermeticity_audit.py`
- `tools/coverage/packages.json`

### Factory/audit metadata

- `.agents/M0-001/handoff-r2.md`
- `.factory/checks/M0-001/runs/run-20260722T134530Z.json`
- `.factory/logs/checks/M0-001/run-20260722T134530Z/`
- `.factory/run.json`
- `.factory/tasks/M0-001.json`
- `IMPLEMENTATION_PLAN.md`

## Commands run and outcomes

| Command | Outcome |
|---|---|
| `python3 -m py_compile tools/build/acquisition_profile.py tools/build/hermeticity_audit.py tests/build/contract_test.py` | PASS. Changed Python sources compile. |
| `python3 tools/format.py` and `git diff --check` | PASS. Canonical JSON and source whitespace are clean. |
| `nix develop --command bazel test --test_output=errors --config=dev //tests/build:contract_test` | PASS. 31 tests pass, including transactional promotion/non-promotion and complete request population. |
| `nix develop --command bazel run //tools/build:hermeticity_audit` | PASS. The live transaction and all same-state-machine boundary fixtures reconcile. |
| `nix develop --command bazel coverage --config=dev --combined_report=lcov //tests/build/... && nix develop --command bazel run //tools/coverage:package_gate -- --threshold=70` | PASS. `tools=79.41176470588235%`, `tools/build=75.32133676092545%`, and `tools/coverage=73.7864077669903%`. |
| `uv run --no-project /home/lschneid/workspace/schneiderlo/factory/factory.py check run M0-001` | PASS. Full registered run `run-20260722T134530Z`; c-01 through c-11 green. |

## Remaining risks

- c-12 and c-13 remain independent verifier judgments. The accepted toolchain,
  dependency coordinates, reference document, coverage policy, M0-002
  ownership, task scope, and substrate ledger were not changed.
- The large byte and long wall-clock boundaries use the verifier-authorized
  bounded synthetic transport, but they execute the production promotion state
  machine and retain explicit transport-call and promotion-state evidence.

No verifier-reported blocker remains open.
