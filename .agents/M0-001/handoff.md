# M0-001 Implementer Handoff

## Source identity

- Implementation base revision: `09a329dc1fb6363bb92f2453097f58a9bac85c37`
- Staged implementation tree before Factory/audit metadata and this handoff:
  `ed660ead584e1e4555d64755bbed857de1e7555d`
- Latest authoritative check run: `run-20260722T111448Z`
- The verifier should use the Factory-created source revision containing this
  tree and handoff; the base revision alone does not contain the implementation.

## Summary of changes

- Added the pinned x86-64 Linux Nix flake/lock and Bazel 8.6.0 Bzlmod
  root/lock, with Nix-owned Glaze v7.5.0, GoogleTest v1.17.0, and Google
  Benchmark v1.9.5 source admission and Nix-local Python toolchain.
- Added fail-closed Clang 21.1.8/LLD 21.1.8 and GCC 15.2.0 toolchain drivers,
  C++23 dev/release/GCC/sanitizer/fuzz/benchmark configurations, platform
  declarations, and native conformance/benchmark smoke targets.
- Recorded exact dependency coordinates, lowercase NAR/archive SHA-256 values,
  bootstrap admissions, acquisition limits/capabilities, configuration
  applicability, canonical wrapper roles, and the single narrow production
  `M0-REFENV-v1` document.
- Added bootstrap contract validation, acquisition exact/first-over negative
  cases, host-toolchain fallback rejection, prohibited CMake/alternate-path
  fixtures, default-denied Bazel action networking with a no-interface/no-route
  negative test, and a real release/GCC `aquery` audit for Clang, GCC, C++23,
  LLD, network policy, and host-path fallback.
- Added package-scoped LCOV gating and a deterministic formatter that respects
  no-trailing-LF canonical JSON while checking task-owned source text.
- Made `nix flake check` execute the Bazel build-contract tests and formatter
  from Nix-materialized BCR/source inputs without action-time fetching. Pure
  Nix builds patch generated/tool wrapper shebangs to pinned interpreters.

## Files changed

### Root build and environment

- `.bazelrc`
- `.bazelversion`
- `.gitignore`
- `BUILD.bazel`
- `MODULE.bazel`
- `MODULE.bazel.lock`
- `flake.nix`
- `flake.lock`

### Machine-readable contracts and documentation

- `config/BUILD.bazel`
- `config/bcr-distdir.json`
- `config/build-applicability.json`
- `config/dependency-inventory.json`
- `config/input-acquisition.json`
- `config/m0-reference-environment.json`
- `config/wrapper-map.json`
- `docs/adr/0001-m0-build-toolchain-and-dependencies.md`
- `docs/build.md`
- `third_party/BUILD.bazel`
- `third_party/bootstrap-admissions.json`

### Toolchains and tooling

- `toolchains/BUILD.bazel`
- `toolchains/bin/orus-clang`
- `toolchains/bin/orus-gcc`
- `tools/BUILD.bazel`
- `tools/format.py`
- `tools/build/BUILD.bazel`
- `tools/build/__init__.py`
- `tools/build/build_contract.py`
- `tools/build/hermeticity_audit.py`
- `tools/build/nix_repositories.bzl`
- `tools/build/prohibited_path_scan.py`
- `tools/coverage/BUILD.bazel`
- `tools/coverage/__init__.py`
- `tools/coverage/package_gate.py`
- `tools/coverage/packages.json`

### Automated tests and fixtures

- `tests/build/BUILD.bazel`
- `tests/build/contract_test.py`
- `tests/build/network_denial_test.py`
- `tests/build/toolchain_conformance_test.cc`
- `tests/build/fixtures/prohibited/BUILD.bazel`
- `tests/build/fixtures/prohibited/CMakeLists.txt`
- `tests/build/fixtures/prohibited/CMakePresets.json`
- `tests/build/fixtures/prohibited/docs/alternate-build.md`
- `tests/benchmarks/BUILD.bazel`
- `tests/benchmarks/benchmark_smoke_test.cc`

Factory-generated status, check-run, and implementation-plan metadata changed
through the CLI and is intentionally separate from the implementation paths
above.

## Commands run and outcomes

| Command | Outcome |
|---|---|
| `uv run --no-project /home/lschneid/workspace/schneiderlo/factory/factory.py check run M0-001` | Initial run `run-20260722T105731Z` was red only at c-01 because pure Nix lacked `/usr/bin/env` for copied/generated launchers; c-02 through c-11 passed. |
| `nix flake check` | Repeated while repairing pure-build shebangs and nested sandbox behavior; final run passed and executed Bazel tests plus the formatter. |
| `nix develop --command bazel test --config=dev //tests/build/...` | Passed: contract, denied-network, and native toolchain tests all executed. |
| `nix develop --command bazel run //tools/build:hermeticity_audit` | Passed; report retained exact acquisition outcomes plus release/GCC action counts and pinned driver/linker identities. |
| `nix develop --command bazel run //tools:format` | Passed over the task-owned source/configuration surface. |
| `nix develop --command bazel coverage --config=dev --combined_report=lcov //tests/build/... && nix develop --command bazel run //tools/coverage:package_gate -- --threshold=70` | Passed; `tools/build` line coverage was 73.64%. |
| `nix flake metadata --json` and `nix path-info --json` inspection | Confirmed the recorded Nix-input and selected package-output SHA-256 values match their locked NAR identities. |
| Release and GCC `bazel aquery` inspection | Confirmed command lines use `toolchains/bin/orus-clang`, `toolchains/bin/orus-gcc`, `-std=c++23`, and the exact Nix-store LLD with no host compiler/linker path. |
| `nix develop --command bazel run //tools/build:prohibited_path_scan` | Passed through the final Factory run: zero real-tree findings and all three injected fixtures detected. |
| `uv run --no-project /home/lschneid/workspace/schneiderlo/factory/factory.py check run M0-001` | Final run `run-20260722T111448Z` green: c-01 through c-11 all passed; c-12/c-13 remain independent manual verifier checks. |

## Known risks and assumptions

- `M0-REFENV-v1` intentionally validates only the exact recorded WSL2/Linux
  x86-64 host predicates. It makes no broader WSL, distribution, kernel, CPU,
  libc, container, or Linux compatibility claim.
- M0-001 performs only the allowed bootstrap canonical/reference byte checks.
  M0-002 still owns the reusable bounded canonical JSON/NFC/SHA-256 and
  reference validation implementation; M0-004 still owns automated
  dependency/SBOM/notice reconciliation.
- `fuzz` and `tsan` are present as explicit configurations but are scoped
  `not_applicable` for this task in `config/build-applicability.json`; the first
  native canonical parser and concurrent corpus belong to later ledger tasks.
- The flake's nested Bazel sandbox enables its own network default because the
  enclosing pure Nix derivation already has no network namespace and omits
  `/sys`. Normal canonical Bazel actions retain
  `--sandbox_default_allow_network=false`, exercised by
  `network_denial_test.py`.

## Verifier focus areas

- Re-run the registered checks from the exact handed-off revision and confirm
  locks remain byte-identical.
- Inspect release and GCC action command lines, especially the Clang driver,
  exact LLD path, GCC driver, C++23 flag, and absence of host paths.
- Independently compare every inventory/reference digest to `flake.lock`,
  `MODULE.bazel.lock`, the BCR distdir manifest, or the selected Nix output.
- Confirm the no-CMake scan excludes only its explicit injected fixture tree
  and still catches filenames, invocation text, and alternate documentation.
- Confirm the 70% report is current, per-package, and fail-closed for missing,
  omitted, below-threshold, or unreviewed package data.
- Audit the task-owned paths separately from Factory/audit metadata as required
  by c-13.

## Pending Healing

None. M0-001 introduced no temporary mock, fake provider, or stand-in boundary.
