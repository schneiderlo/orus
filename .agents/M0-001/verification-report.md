# M0-001 Verification Report

## Decision

**Verdict:** FAIL / BLOCKED  
**Task:** `M0-001` — Hermetic Build, Toolchain, and Test Skeleton  
**Implementation base:** `09a329dc1fb6363bb92f2453097f58a9bac85c37`  
**Verified source revision:** `6056821ca3631278780bfb47bc636f4d3b86a623`  
**Scope range:** `09a329dc1fb6363bb92f2453097f58a9bac85c37..6056821ca3631278780bfb47bc636f4d3b86a623`  
**Authoritative check run:** `run-20260722T112100Z` (GREEN)  
**Prior stale run:** `run-20260722T111936Z` (RED because the pre-commit tree did not yet track `flake.nix`)

The complete registered suite was rerun once because the supplied red run
predated the Factory-created implementation commit. All runnable checks passed
on the committed source. Acceptance still fails because the manual dependency
audit found unresolved graph inputs outside the admission/acquisition
inventory, and the coverage gate masks task-owned logic omitted from LCOV.
There is no healing-task link and no substituted service was used by these
checks.

## Registered machine checks

| Check | Result | Evidence |
|---|---|---|
| c-01 `nix flake check` | PASS | Exit 0 in `.factory/logs/checks/M0-001/run-20260722T112100Z/c-01.*.log`. |
| c-02 module graph | PASS | Exit 0; resolved graph retained in `c-02.stdout.log`. |
| c-03 release build | PASS | Exit 0. |
| c-04 dev tests | PASS | Exit 0; three targets executed. |
| c-05 GCC tests | PASS | Exit 0. |
| c-06 ASan tests | PASS | Exit 0. |
| c-07 UBSan tests | PASS | Exit 0. |
| c-08 benchmark smoke | PASS | Exit 0. |
| c-09 hermeticity audit | PASS command, FAIL acceptance semantics | Exit 0 and pinned-action report retained, but its acquisition results are calls to the pure `acquisition_outcome` function and do not reconcile or exercise the actual Nix/Bzlmod acquisition population. See F-01. |
| c-10 prohibited-path scan | PASS | `real_findings=0`, `fixture_detection=3`, `fixture_expected=3`. |
| c-11 coverage gate | PASS command, FAIL acceptance semantics | Command reports `tools/build=73.63636363636364%`, but omitted task-owned files/packages are not detected. See F-02. |

## Blocking findings

### F-01 — Resolved Bzlmod acquisition is not completely admitted

`c-02.stdout.log` proves the selected graph includes, in addition to the four
direct BCR modules, `abseil-cpp`, `bazel_features`, `bazel_skylib`,
`googletest@1.14.0.bcr.1`, `jsoncpp`, `package_metadata`, `protobuf`,
`pybind11_bazel`, `re2`, `rules_android`, `rules_fuzzing`, `rules_java`,
`rules_jvm_external`, `rules_kotlin`, `rules_license`, `rules_pkg`,
`rules_proto`, `toml.bzl`, and `zlib`. Those selected module identities do not
have corresponding rows in `config/dependency-inventory.json`,
`config/input-acquisition.json`, or
`third_party/bootstrap-admissions.json`. The existing `googletest` admission
is for the separate Nix-fetched v1.17.0 source, not the BCR v1.14.0 module.

`flake.nix` materializes every archive in `config/bcr-distdir.json`; that set
also contains fetched buildozer and supply-chain assets not represented by the
declared acquisition/admission population. Thus real input acquisition is not
one-to-one with the admitted immutable coordinates required by BUILD-FR-008.

The named `m0_input_acquisition` profile is otherwise only a JSON document and
a pure decision function. The checks pass caller-supplied byte/time/digest
numbers into `acquisition_outcome`; they do not execute a bounded fetch,
quarantine, digest verification, or read-only promotion boundary around the
actual Nix/Bzlmod materialization. Consequently the retained report cannot
prove the exact-bound/first-over side effects it claims.

Required rework: reconcile every actually resolved/fetched input one-to-one
with immutable inventory, acquisition, and bootstrap-admission records; enforce
the named bounded acquisition phase on actual materialization; and make the
audit fail for any graph/distdir/admission mismatch.

### F-02 — The package coverage gate masks omitted task-owned logic

The gate hardcodes only `tools/build` in
`tools/coverage/packages.json`, `.bazelrc` instruments only
`^//tools/build[/:]`, and `parse_lcov` recognizes only paths containing
`/tools/build/`. `tests/build/contract_test.py` imports and exercises
`tools/coverage/package_gate.py`, but that task-owned logic is excluded from
the report. Within `tools/build`, the gate aggregates only LCOV records that
happen to exist and has no source-file inventory, so omitted executable logic
such as `hermeticity_audit.py` and `prohibited_path_scan.py` cannot make the
package fail. `tools/format.py` is also absent without a reviewed exclusion
rationale.

The reported 73.64% therefore does not establish the required per-package
coverage population. This violates c-11's explicit rule that missing data,
omitted task-owned logic, and unreviewed exclusions fail.

Required rework: define a finite reviewed package/source and exclusion
manifest, collect current non-empty LCOV for every included task-owned logic
package/file, test missing/omitted/unauthorized-exclusion cases, and apply the
70% threshold independently to each package.

## Manual checks

### c-12 — FAIL: pins, admissions, actions, and reference contract

| Review item | Result | Evidence |
|---|---|---|
| Glaze exactly v7.5.0, Nix/Bzlmod-only, no CMake exposure | PASS | `flake.lock` pins revision `8b60d82...`; `MODULE.bazel` consumes the Nix source through `nix_source_repository`; `tools/build/nix_repositories.bzl` exposes only headers through an Orus Bazel target; c-10 reports zero real findings. |
| Direct tool/library versions and immutable digests | PASS | Inventory/locks record Bazel 8.6.0; Clang/LLVM/LLD 21.1.8; GCC 15.2.0; Python 3.14.6 + Coverage 7.14.1; OpenSSL 3.6.3; utf8proc 2.11.3; GoogleTest 1.17.0; Google Benchmark 1.9.5; and Glaze 7.5.0. |
| Complete resolved dependency inventory and bootstrap admissions | **FAIL** | F-01: the selected Bzlmod graph and fetched distdir contain un-inventoried/unadmitted inputs. Existing 17 admissions are structurally complete but do not cover the actual graph. |
| Compatibility and pinned-tool action evidence | PASS | ADR-0001 records selection, alternatives, compatibility matrix, consequences, and rollback. `c-09.stdout.log` names Bazel 8.6.0, Clang/LLD 21.1.8, GCC 15.2.0, C++23 drivers, release LLD `/nix/store/...-lld-21.1.8/bin/ld.lld`, zero host inputs, and zero network-enabled Bazel actions. |
| Single narrow `M0-REFENV-v1` document and ADR | PASS | Only `config/m0-reference-environment.json` is a production document. Removing `environment_id` and hashing canonical bytes recomputes `19b6d0378bbc6cd715ac806f8a8d7b9171b5b7ff387440ebe1de15d19ed87545`. It declares `x86_64-linux`, `validated_reference`, exact host predicates, five tool identities, and no broader support claim. |
| M0-002 ownership and excluded BUILD-FR-009/-011 behavior | PASS | `tools/build/build_contract.py` labels its work bootstrap-only; no product canonical API/reference validator or completed package/build-facts claim was introduced. M0-002 remains the reusable canonical/NFC/SHA-256 owner. |

Because one mandatory review item fails, c-12 fails overall.

### c-13 — PASS: task scope and audit metadata

Audited range: `09a329dc1fb6363bb92f2453097f58a9bac85c37..6056821ca3631278780bfb47bc636f4d3b86a623`.

Task-owned implementation bucket (44 paths), matching the handoff:

- Roots: `.bazelrc`, `.bazelversion`, `.gitignore`, `BUILD.bazel`,
  `MODULE.bazel`, `MODULE.bazel.lock`, `flake.nix`, `flake.lock`.
- Contracts/docs: `config/BUILD.bazel`, `config/bcr-distdir.json`,
  `config/build-applicability.json`, `config/dependency-inventory.json`,
  `config/input-acquisition.json`, `config/m0-reference-environment.json`,
  `config/wrapper-map.json`,
  `docs/adr/0001-m0-build-toolchain-and-dependencies.md`, `docs/build.md`,
  `third_party/BUILD.bazel`, `third_party/bootstrap-admissions.json`.
- Toolchains/tools: `toolchains/BUILD.bazel`, both `toolchains/bin/orus-*`
  drivers, `tools/BUILD.bazel`, all six files under `tools/build/`, all four
  files under `tools/coverage/`, and `tools/format.py`.
- Tests: both files under `tests/benchmarks/`, the three top-level
  `tests/build/` sources plus its `BUILD.bazel`, and the four prohibited
  fixture files.

Factory/audit metadata bucket (8 paths):

- `.agents/M0-001/handoff.md`;
- the two prior `.factory/checks/M0-001/runs/*.json` records;
- `.factory/results/09_implementer/r2/result.json` and its archived result;
- `.factory/run.json`, `.factory/tasks/M0-001.json`, and the CLI-generated
  `IMPLEMENTATION_PLAN.md` status change.

Current uncommitted changes are also only Factory/verifier metadata and the
new check-run records. No unexplained out-of-scope implementation, hidden
generated source path, or handoff omission was found.

## Acceptance criteria

| Criterion | Result | Reason |
|---|---|---|
| BUILD-FR-001 | FAIL | Roots/locks resolve, but the exact-coordinate dependency inventory is incomplete for the selected graph. |
| BUILD-FR-002 | PASS | Nix supplies the environment and pinned tools; no host tool was selected. |
| BUILD-FR-003 | PASS | Bazel action evidence accounts for current compilation/link/test actions. |
| BUILD-FR-004 | PASS | C++23 Clang 21.1.8 and LLD 21.1.8 actions passed, with host fallback rejected. |
| BUILD-FR-005 | PASS | GCC 15.2.0 C++23 compatibility tests passed. |
| BUILD-FR-006 | PASS | Required configurations executed; TSan/fuzz have scoped M0-001 non-applicability rows. |
| BUILD-FR-007 | PASS | Zero real CMake/alternate-path findings; all three injected fixtures detected. |
| BUILD-FR-008 | FAIL | Actual resolved/fetched inputs exceed the admitted manifest, and the named acquisition boundary is simulated rather than enforced on materialization. |
| BUILD-FR-010 (M0-001 slice) | PASS | Exactly one narrow reference document and one ADR; identity and predicates are exact. |
| BUILD-FR-012 | PASS | Both executable toolchain wrappers have canonical Nix+Bazel mappings. |
| BUILD-NFR-002 | PASS | Retained Bazel action evidence shows zero undeclared host inputs and zero action-time network capability. |
| BUILD-NFR-003 | PASS | Scanner reports zero real findings and complete fixture detection. |
| BUILD-NFR-005 | PASS | Required named configurations passed and non-applicability is explicit. |
| q-0003/q-0012 dependency-stack deliverable | FAIL | Direct stack is pinned, but complete resolved-input admission is missing. |
| Factory 70% package coverage baseline | FAIL | 73.64% is reported only for a narrowed population that omits task-owned logic. |

## Target conditions TC-01 through TC-10

| TC | Result | Reason |
|---|---|---|
| TC-01 | FAIL | Root/lock commands pass, but complete exact-coordinate inventory is absent. |
| TC-02 | FAIL | Post-acquisition Bazel actions are network-denied, but the real acquisition population/profile is not proved. |
| TC-03 | PASS | Pinned Clang/LLD/GCC and C++23 action selection passed. |
| TC-04 | PASS | Required configuration commands passed; scoped non-applicability is recorded. |
| TC-05 | FAIL | Direct admissions are complete, but resolved Bzlmod dependencies are not admitted. |
| TC-06 | PASS | Build/test/benchmark/audit skeletons execute through Nix+Bazel. |
| TC-07 | PASS | Prohibited-path and wrapper-map checks pass. |
| TC-08 | PASS | One narrow production reference document and ADR are retained. |
| TC-09 | FAIL | The 70% report omits task-owned logic/source population. |
| TC-10 | FAIL | c-12 and coverage-population review fail despite a green runnable run. |

## Negative and edge expectations

| ID | Result | Evidence / gap |
|---|---|---|
| N1 | FAIL | Locks stayed unchanged in the green run, but no missing/floating/mutated resolved-input fixture reconciles the complete Bzlmod population; un-inventoried selected modules pass. |
| N2 | PASS | Host Clang/GCC fallback fixtures return `BUILD_CONFIG_INVALID`; required configuration commands executed. |
| N3 | PASS | All three prohibited fixtures return findings while the real tree remains clean. |
| N4 | FAIL | The pure-function unadmitted/mutable/hash cases return `BUILD_ACQUISITION_DENIED`, but actual unadmitted resolved inputs are fetched/materialized and no quarantine/promotion boundary is exercised. |
| N5 | FAIL | Exact/first-over values are integer arguments to a pure function; no real 128-coordinate/4-GiB/16-GiB/1,200-second acquisition side effects are proved. |
| N6 | PASS | Network-denial test has only loopback/no default route and an external connect returns unreachable; action audit finds no host compiler/linker path. |
| N7 | PASS | Wrong reference ID is rejected; manual review confirms one canonical narrow production document and exact identity. |
| N8 | FAIL | Glaze containment passes, but selected Bzlmod dependencies without admissions do not fail the suite. |
| N9 | FAIL | Exact 70% and below-threshold unit cases exist, but omitted package/source and unreviewed-exclusion defects are masked by the hardcoded population. |
| N10 | PASS | ASan/UBSan and benchmark commands executed successfully; scoped TSan/fuzz non-applicability is present. |

## Substrate and healing

`factory substrate list` contains only the later `github-actions` substitute.
M0-001 exercised the real local Nix/Bazel boundary and made no live GitHub
claim, so no substrate ledger change is appropriate. `factory task show
M0-001` lists no healing task or temporary mock.

## Required disposition

Set `M0-001` to `BLOCKED`. Rework must close F-01 and F-02, add the missing
negative evidence, and then rerun the complete registered suite. A green
command transcript alone cannot waive either manual/semantic failure.
