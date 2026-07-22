# M0-001 Verification Report r2

## Decision

**Verdict:** PASS / DONE  
**Task:** `M0-001` — Hermetic Build, Toolchain, and Test Skeleton  
**Original implementation base:** `09a329dc1fb6363bb92f2453097f58a9bac85c37`  
**Latest rework base:** `b5b67acc2bd7100cd95c6e7cf794a4fd0391896d`  
**Verified source revision:** `d2e447ed07519160a404fe3f8158a715a785afdb`  
**Full task scope range:** `09a329dc1fb6363bb92f2453097f58a9bac85c37..d2e447ed07519160a404fe3f8158a715a785afdb`  
**Latest rework commit audited:** `d2e447ed07519160a404fe3f8158a715a785afdb`  
**Authoritative check run:** `run-20260722T135616Z` (GREEN)

The supplied run `run-20260722T135021Z` was red only because its c-01
derivation copied stale Nix source `/nix/store/k48cc1wl3bxbnhw8gjg994qaw3pspw0l-source`,
which omitted the newly committed `tools/build/acquisition_profile.py` even
though `git ls-tree` showed that file in `d2e447e`. A current flake evaluation
resolved `/nix/store/byafn3m6pgh1vsfwb45ifqx8jkdj2g0c-source`, which contains
the file. Under the stale-evidence rule, the complete factory suite was rerun
once. Its new authoritative run is green and c-01 uses the current derivation
`/nix/store/sbx5h4yz6w32gdqnwz4yr5s0pgnm71z0-orus-m0-001-bootstrap-contract.drv`.

The latest rework closes the prior pre-promotion acquisition blocker. One
admission-gated state machine now owns preflight, quarantine, whole-profile
byte/deadline enforcement, digest verification, read-only promotion, and the
exact/first-over negative matrix. Both required manual checks pass. There is
no healing-task link and no applicable substituted boundary.

## Registered machine checks

All logs below are under
`.factory/logs/checks/M0-001/run-20260722T135616Z/`.

| Check | Result | Evidence |
|---|---|---|
| c-01 `nix flake check` | PASS | Exit 0; the current flake source includes `acquisition_profile.py`, the bootstrap Bazel contract ran, and Nix reports `all checks passed`. |
| c-02 module graph | PASS | Exit 0; the retained graph resolves the complete selected Bzlmod population. |
| c-03 release build | PASS | Exit 0; 20 targets and 51 sandboxed actions completed under the release configuration. |
| c-04 dev tests | PASS | Exit 0; contract, network-denial, and native toolchain targets all passed. |
| c-05 GCC tests | PASS | Exit 0; the native conformance target rebuilt with the GCC configuration and all three test targets passed. The two Python targets were valid config-independent cache hits, not skips. |
| c-06 ASan tests | PASS | Exit 0; all three applicable build-test targets executed with ASan. |
| c-07 UBSan tests | PASS | Exit 0; all three applicable build-test targets executed with UBSan. |
| c-08 benchmark smoke | PASS | Exit 0; the real Google Benchmark smoke target executed under the benchmark/release configuration. |
| c-09 hermeticity audit | PASS | Exit 0; the report proves one 26-fetch transaction, 39 admitted coordinates, 58,273,138 fetched bytes, a shared 1,200-second deadline, read-only promotion, 23 modules, 151 registry files, 13 Nix paths, pinned Clang/LLD/GCC actions, zero network-enabled Bazel actions, and zero undeclared host inputs. |
| c-10 prohibited-path scan | PASS | `real_findings=0`, `fixture_detection=3`, and `fixture_expected=3`. |
| c-11 coverage gate | PASS | Current LCOV is non-empty; `tools=79.41176470588235%`, `tools/build=75.32133676092545%`, and `tools/coverage=73.7864077669903%`, each above the exact 70% per-package threshold. |

## Manual checks

### c-12 — PASS: pins, admissions, actions, reference contract, and ownership

| Review item | Result | Evidence |
|---|---|---|
| Glaze exactly v7.5.0 and Nix/Bzlmod-only | PASS | `flake.nix` pins `github:stephenberry/glaze/v7.5.0`; the inventory binds revision `8b60d82c...` and digest `10d0fd9a...`; `MODULE.bazel` exposes the Nix-store source through `nix_source_repository`; `tools/build/nix_repositories.bzl` publishes only an Orus-owned header target. c-10 reports zero real CMake/alternate-path findings. |
| Exact tool/library pins and immutable digests | PASS | The inventory records Nixpkgs `fd146203...`, Bazel 8.6.0, Clang/LLVM/LLD 21.1.8, GCC 15.2.0, Python 3.14.6 + Coverage 7.14.1, OpenSSL 3.6.3 EVP, utf8proc 2.11.3, GoogleTest 1.17.0, Google Benchmark 1.9.5, and Glaze 7.5.0. All 39 inventory digests are lowercase 64-hex values. |
| Complete bootstrap admissions | PASS | Dependency inventory, acquisition manifest, and admission ledger contain 39 matching inputs. Inventory/acquisition coordinate+digest sets match exactly; inventory/admission name sets match exactly. Every admission has non-empty purpose, license, closure/ABI/build cost, containment, alternatives, health, and rollback/removal fields. |
| Compatibility and live materialization | PASS | ADR-0001 treats the stack as one compatibility unit. c-01 through c-08 exercise the Nix/Bazel graph, both compilers, sanitizers, GoogleTest, and Google Benchmark. c-09 reconciles 23 modules, 26 BCR fetches, 151 registry files, and 13 Nix paths into read-only materializations. |
| Pinned-tool action evidence | PASS | c-09 reports Bazel 8.6.0, Clang/LLD 21.1.8, GCC 15.2.0, Python 3.14.6, `toolchains/bin/orus-clang`, the exact Nix-store `ld.lld`, `toolchains/bin/orus-gcc`, zero host inputs, and zero network-enabled Bazel actions. |
| Single narrow `M0-REFENV-v1` and ADR | PASS | `config/m0-reference-environment.json` is the only production JSON with that schema. Hashing the canonical document without `environment_id` recomputes the recorded `19b6d0378bbc6cd715ac806f8a8d7b9171b5b7ff387440ebe1de15d19ed87545`. It declares `x86_64-linux`, `validated_reference`, four locked flake inputs, all five tool identities, nine exact host predicates, and no broad distro/kernel/CPU/container/WSL claim. ADR-0001 records the selection date, compatibility evidence, alternatives, consequences, and rollback. |
| M0-002 and deferred ownership | PASS | `tools/build/build_contract.py` explicitly scopes its JSON/reference logic to bootstrap checking. ADR-0001 retains the reusable canonical JSON/NFC/SHA-256 and reference-validation boundary under M0-002. No completed BUILD-FR-009 build-facts implementation, BUILD-FR-011 package identity/release package, product canonical API, CLI behavior, or live-GitHub claim was introduced. |

c-12 passes overall.

### c-13 — PASS: task scope and factory/audit metadata

Audited the full task range
`09a329dc1fb6363bb92f2453097f58a9bac85c37..d2e447ed07519160a404fe3f8158a715a785afdb`
and separately inspected latest rework commit
`d2e447ed07519160a404fe3f8158a715a785afdb`. `HEAD` is that implementation
commit, so no `HEAD~1` adaptation was needed.

Task-owned implementation bucket: 45 paths, all within the A3/handoff scope.

- Roots: `.bazelrc`, `.bazelversion`, `.gitignore`, `BUILD.bazel`,
  `MODULE.bazel`, `MODULE.bazel.lock`, `flake.nix`, and `flake.lock`.
- Contracts/docs: seven `config/` build/JSON paths, ADR-0001,
  `docs/build.md`, and both `third_party/` paths.
- Toolchains/tools: three `toolchains/` paths, seven `tools/build/` paths
  including `acquisition_profile.py`, four `tools/coverage/` paths,
  `tools/BUILD.bazel`, and `tools/format.py`.
- Tests: both benchmark paths, four top-level `tests/build/` paths, and the
  four prohibited-fixture paths.

Factory/audit metadata bucket: 23 committed paths in the range—the three
handoffs, two prior verification reports, seven prior check-run records,
active and archived implementer/verifier/rework result records,
`.factory/run.json`, `.factory/tasks/M0-001.json`, and the CLI-generated status
row in `IMPLEMENTATION_PLAN.md`. The supplied/new check records and current
verifier task/run/result changes are uncommitted factory metadata. They do not
count as implementation scope.

Every task-owned path is named by the cumulative handoffs. The latest rework
commit changes only 11 task-owned paths plus factory/audit metadata, exactly as
`handoff-r2.md` records. No unexplained out-of-scope implementation, hidden
generated source path, unexplained lock mutation, or missing handoff
deliverable was found. c-13 passes.

## Acceptance criteria

| Criterion | Result | Reason |
|---|---|---|
| BUILD-FR-001 | PASS | Required roots/locks exist; c-01/c-02 pass; 39 exact inventory inputs reconcile the selected graph and materializations. |
| BUILD-FR-002 | PASS | Nix supplies the declared environment and tools without a secret or host-tool fallback. |
| BUILD-FR-003 | PASS | Bazel owns compile/link/test/benchmark actions; c-09 and c-13 find no independent source action. |
| BUILD-FR-004 | PASS | C++23 Clang 21.1.8 and exact LLD 21.1.8 actions pass; host fallback is rejected. |
| BUILD-FR-005 | PASS | GCC 15.2.0 C++23 compatibility passes in the same reference environment. |
| BUILD-FR-006 | PASS | Required configurations execute; TSan/fuzz have exact task-scoped non-applicability records. |
| BUILD-FR-007 | PASS | Zero real CMake/alternate-path findings and complete injected-fixture detection. |
| BUILD-FR-008 | PASS | One admitted-manifest-driven state machine preflights before transport, quarantines all bytes, enforces aggregate limits/deadline, verifies digests, promotes only complete read-only output, and retains exact/first-over non-promotion evidence. |
| BUILD-FR-010 (M0-001 slice) | PASS | Exactly one narrow production contract and one complete ADR; M0-002 owns the reusable validator. |
| BUILD-FR-012 | PASS | Both executable toolchain wrappers have machine-readable canonical Nix+Bazel mappings. |
| BUILD-NFR-002 | PASS | c-09 reports zero undeclared host inputs and zero action-time network capability. |
| BUILD-NFR-003 | PASS | c-10 reports zero real findings and detects all prohibited fixtures. |
| BUILD-NFR-005 | PASS | All required named configurations pass; scoped non-applicability is explicit; no host fallback/silent empty cell is accepted. |
| q-0003/q-0012 dependency-stack deliverable | PASS | The complete stack is exact, digest-pinned, admitted, compatible in the green suite, and Glaze is Nix/Bzlmod-only. |
| Factory 70% package coverage baseline | PASS | All three finite task-owned logic packages exceed 70%; omission, missing-data, first-below, and unauthorized-exclusion cases fail closed. |

## Target conditions TC-01 through TC-10

| TC | Result | Reason |
|---|---|---|
| TC-01 | PASS | Roots, locks, exact inventory, and module graph reconcile. |
| TC-02 | PASS | One real bounded acquisition transaction precedes hermetic network-denied Bazel actions. |
| TC-03 | PASS | Pinned Clang/LLD/GCC and C++23 action selection pass. |
| TC-04 | PASS | Required configuration commands execute and scoped non-applicability is recorded. |
| TC-05 | PASS | All 39 resolved/fetched inputs have exact inventory, acquisition, digest, and admission coverage; Glaze is CMake-free. |
| TC-06 | PASS | Build/test/benchmark/format/coverage and transactional acquisition skeletons execute with positive and negative fixtures. |
| TC-07 | PASS | Prohibited-path and wrapper-map checks pass. |
| TC-08 | PASS | One narrow hash-valid production reference document and complete ADR are retained. |
| TC-09 | PASS | Every finite task-owned logic package exceeds the 70% line threshold without masking. |
| TC-10 | PASS | The authoritative registered run is green; c-12/c-13 and the independent sign-off pass. |

## Negative and edge expectations

| ID | Result | Evidence |
|---|---|---|
| N1 | PASS | c-01/c-02/c-04 pass; contract tests reject incomplete selected-module populations, invalid locks, duplicates, and mutable resolution with `BUILD_LOCK_INVALID`, while committed locks remain unchanged. |
| N2 | PASS | c-03 through c-07 select pinned toolchains; broken/host-toolchain and silent-configuration fixtures return `BUILD_CONFIG_INVALID`. |
| N3 | PASS | c-10 reports zero real findings and detects all three injected filename/invocation/documentation fixtures. |
| N4 | PASS | c-09 reports `BUILD_ACQUISITION_DENIED`, `promoted=false` for unadmitted, mutable, and hash-mismatch cases; unadmitted/mutable preflight uses zero transport calls. |
| N5 | PASS | c-09 drives 128-coordinate, 4-GiB blob, 16-GiB total, and 1,200-second exact/first-over fixtures through `ProfileAcquirer`; all exact cases promote read-only and every first-over case is denied with no promoted output. |
| N6 | PASS | The network-denial test executes; c-09 reports `network_enabled_actions=0` and `undeclared_host_inputs=0`. |
| N7 | PASS | Contract tests reject wrong reference identity/noncanonical input; manual review confirms one exact narrow production document and no broader claim. |
| N8 | PASS | All 39 inputs have immutable digest/admission coverage, compatibility commands pass, and c-10 confirms Glaze has no CMake route. |
| N9 | PASS | c-04 exercises exact-70, first-below, absent LCOV, missing source, omitted owned source, and unauthorized-exclusion cases; c-11 proves current per-package coverage above 70%. |
| N10 | PASS | ASan, UBSan, and benchmark targets execute in c-06/c-07/c-08; TSan/fuzz non-applicability is recorded and reconciled. |

## Sign-off checklist

| Item | Result | Reason |
|---|---|---|
| Latest V1-V11 run is complete and green | PASS | `run-20260722T135616Z` covers all registered runnable checks; none is skipped, optional, or substituted. |
| V12 and V13 addressed | PASS | c-12 and c-13 are explicitly audited above with file/action/diff evidence. |
| TC-01 through TC-10 and ledger requirements | PASS | Every target condition and acceptance criterion passes. |
| N1 through N10 | PASS | Every required negative/boundary outcome is exercised and retained. |
| Singular Nix/Bazel ownership | PASS | No CMake, host fallback, undeclared input, independent action, or action-time network capability is present. |
| Exact pins/admissions/reference | PASS | 39/39 rows reconcile; the one reference document is hash-valid and narrowly worded. |
| Exact 70% package coverage | PASS | Every included package is above threshold and exclusion/omission behavior fails closed. |
| Scope and metadata policy | PASS | 45 task-owned paths are in scope; factory/audit metadata is separately bucketed. |
| Downstream ownership/claims | PASS | No M0-002 core, completed package/CLI, live-GitHub, broad Linux, or M1+ behavior is claimed. |
| Independence/source/Andon | PASS | Verifier role differs from Author role; evidence names `d2e447e`; no healing link or unresolved Andon exists. |

## Substrate and healing

`factory substrate list` contains only the later `github-actions` substitute.
M0-001 exercises the real local Nix/Bazel boundary, makes no live-GitHub claim,
and does not exercise a ledger boundary marked substituted; no substrate flip
is appropriate. `factory task show M0-001` lists no healing task or temporary
mock.

## Required disposition

Set `M0-001` to `DONE`. All required machine and manual checks pass, the
coverage threshold is met exactly as declared, and no pending healing or scope
defect remains.
