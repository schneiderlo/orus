# M0-001 Verification Report r1

## Decision

**Verdict:** FAIL / BLOCKED  
**Task:** `M0-001` — Hermetic Build, Toolchain, and Test Skeleton  
**Original implementation base:** `09a329dc1fb6363bb92f2453097f58a9bac85c37`  
**Rework base:** `60478f3dc4d9a14812efc3152f5bb4e2df115af8`  
**Verified source revision:** `ea4f5679fa4787700ce198b460de368fc81ad954`  
**Full task scope range:** `09a329dc1fb6363bb92f2453097f58a9bac85c37..ea4f5679fa4787700ce198b460de368fc81ad954`  
**Rework commit audited:** `ea4f5679fa4787700ce198b460de368fc81ad954`  
**Authoritative check run:** `run-20260722T115238Z` (GREEN transcript)

The latest registered run covers all eleven runnable checks and is current for
the reworked source, so it was judged without re-executing the commands. The
rework closes the complete resolved-input inventory/admission mismatch and the
source-coverage omission reported in the first verification. Acceptance still
fails because c-09's live acquisition path does not enforce the declared
whole-profile bounds or rejection-before-promotion semantics. The command's
green report combines a real below-limit materialization with pure decision
fixtures; it does not prove the required first-over behavior on the real
materialization boundary.

There is no healing-task link. The only substrate ledger entry is the later
`github-actions` substitute; M0-001 made no live-GitHub claim and exercised the
real local Nix/Bazel boundary, so no substrate ledger change is appropriate.

## Registered machine checks

| Check | Result | Evidence |
|---|---|---|
| c-01 `nix flake check` | PASS | Exit 0 in `.factory/logs/checks/M0-001/run-20260722T115238Z/c-01.*.log`; the bootstrap Bazel contract and formatter passed. |
| c-02 module graph | PASS | Exit 0; `c-02.stdout.log` contains the 23-module selected graph. |
| c-03 release build | PASS | Exit 0; 19 targets and 51 sandboxed actions completed. |
| c-04 dev tests | PASS | Exit 0; contract, denied-network, and native toolchain targets all executed. |
| c-05 GCC tests | PASS | Exit 0; the same three build-test targets executed with the GCC configuration. |
| c-06 ASan tests | PASS | Exit 0; all three applicable targets executed. |
| c-07 UBSan tests | PASS | Exit 0; all three applicable targets executed. |
| c-08 benchmark smoke | PASS | Exit 0; the Google Benchmark smoke target executed. |
| c-09 hermeticity audit | **FAIL acceptance semantics** | The command exited 0 and its report proves the current 39-coordinate population is read-only and below limits, with 23 modules, 25 archives, 151 registry files, 13 Nix paths, zero network-enabled Bazel actions, and zero undeclared host inputs. It does not enforce unadmitted rejection or the aggregate/time first-over cases before real store promotion; see F-01. |
| c-10 prohibited-path scan | PASS | `c-10.stdout.log` reports `real_findings=0`, `fixture_detection=3`, and `fixture_expected=3`. |
| c-11 coverage gate | PASS | `c-11.stdout.log` reports `tools=79.41176470588235%`, `tools/build=75.40983606557377%`, and `tools/coverage=73.7864077669903%`, each above 70%. |

## Blocking finding

### F-01 — Actual acquisition is not bounded as one pre-promotion profile

BUILD-FR-008 and the build-entry contract require `m0_input_acquisition` to
fetch only admitted coordinates, bound the complete profile to 128
coordinates/4 GiB each/16 GiB total/1,200 seconds, and reject every excess or
unadmitted input before read-only-store promotion. V9 and N4/N5 require those
negative cases on the acquisition boundary, not only as arithmetic decisions.

The rework's real path is materially improved but remains ordered incorrectly:

1. `flake.nix:86-113` reads `config/bcr-distdir.json` directly and constructs
   `pkgs.fetchurl`/`pkgs.fetchzip` fixed-output derivations for its rows. It
   does not reconcile those rows against `admitted_inputs` before constructing
   or realizing the fetches. An added immutable but unadmitted distdir row can
   therefore be fetched into the Nix store before the later Python contract
   rejects the population.
2. `flake.nix:88-103` applies `--max-time 1200` independently to every archive
   and the registry. This allows the complete profile to exceed 1,200 seconds;
   there is no one-profile wall clock/deadline.
3. `flake.nix:113-133` realizes each fixed-output archive first, then sums file
   sizes inside `bcrDistdir`. A 16-GiB first-over population fails before the
   distdir is exposed to Bazel, but the constituent fixed-output paths have
   already been hash-verified and promoted into `/nix/store`, contrary to the
   required before-promotion rule.
4. `tools/build/hermeticity_audit.py:114-164` audits only the already-realized,
   below-limit current population. Its first-over and unadmitted results at
   lines 212-274 call the pure `acquisition_outcome` function with supplied
   numbers. `tests/build/contract_test.py:64-141` does the same. Neither path
   exercises a real rejected fetch/quarantine/promotion transition.
5. The audit reports `wall_limit_seconds:1200` but does not measure profile
   elapsed time. The handoff itself notes that the exact/first-over time case
   is decision evidence rather than a real acquisition-boundary execution.

Required rework: make one admitted-manifest-driven acquisition owner reconcile
the complete Nix/Bzlmod population before any fetch; enforce one aggregate
coordinate/byte/deadline budget while bytes remain quarantined; hash-verify and
promote only after all relevant checks pass; and make c-09 exercise retained
negative evidence showing unadmitted and aggregate/time first-over inputs do
not enter the promoted store. A bounded synthetic transport is acceptable for
the large/time boundary fixtures if it executes the same promotion state
machine; a pure outcome function is not.

## Manual checks

### c-12 — PASS: pins, admissions, actions, and reference contract

| Review item | Result | Evidence |
|---|---|---|
| Exact Glaze route | PASS | `config/dependency-inventory.json` pins Glaze 7.5.0 at Git revision `8b60d82c...` with digest `10d0fd9a...`; `flake.nix` fetches it as a locked non-flake Nix input, `MODULE.bazel` exposes it only through `nix_source_repository`, and `tools/build/nix_repositories.bzl` publishes only an Orus-owned header target. c-10 finds no real CMake path. |
| Exact mutually compatible tool/library pins and digests | PASS | The inventory records Nixpkgs `fd146203...`, Bazel 8.6.0, Clang/LLVM/LLD 21.1.8, GCC 15.2.0, Python 3.14.6 + Coverage 7.14.1, OpenSSL 3.6.3, utf8proc 2.11.3, GoogleTest 1.17.0, Google Benchmark 1.9.5, and Glaze 7.5.0 with exact coordinates and 64-hex digests. ADR-0001 binds these as one compatibility unit; c-01 through c-08 exercised that unit. |
| Complete bootstrap admissions | PASS | The dependency inventory, acquisition manifest, and admission ledger each contain the same 39 named inputs; the inventory groups 23 selected Bzlmod modules, 2 BCR assets, 1 pinned registry, and 13 Nix/Nix+Bzlmod inputs. All 39 admission rows are sorted, unique, non-empty, and contain purpose, license, closure/ABI/build cost, containment, alternatives, health, and rollback/removal. |
| Live graph/materialization reconciliation | PASS for current population | c-02 shows all 23 selected modules. c-09 hashes 25 archives and 151 locked registry files, verifies 13 Nix paths, and reports all current materializations read-only. This does not waive F-01's boundary-order failure. |
| Pinned-tool action evidence | PASS | c-09 reports Bazel 8.6.0, Clang/LLD 21.1.8, GCC 15.2.0, Python 3.14.6, the release `orus-clang` driver, exact Nix-store `ld.lld`, the GCC driver, zero network-enabled actions, and zero undeclared host inputs. |
| Single narrow `M0-REFENV-v1` document and ADR | PASS | Only `config/m0-reference-environment.json` is a production document. Removing `environment_id` and hashing its canonical bytes recomputes `19b6d0378bbc6cd715ac806f8a8d7b9171b5b7ff387440ebe1de15d19ed87545`. It declares `x86_64-linux`, `validated_reference`, the four locked flake inputs, all five required tool identities, exact nine host predicates, and no broader support claim. ADR-0001 records selection date, compatibility evidence, alternatives, consequences, and rollback. |
| M0-002 and deferred ownership | PASS | `tools/build/build_contract.py` labels its JSON/reference work bootstrap-only, and ADR-0001 explicitly retains reusable canonicalization/reference validation under M0-002. No completed BUILD-FR-009 build-facts implementation, BUILD-FR-011 release package, product canonical API, or live-GitHub behavior was introduced. |

c-12 passes overall. F-01 is a c-09/BUILD-FR-008 execution-boundary failure,
not a pin, admission-record, reference-document, or ownership failure.

### c-13 — PASS: task scope and audit metadata

Audited the full task range
`09a329dc1fb6363bb92f2453097f58a9bac85c37..ea4f5679fa4787700ce198b460de368fc81ad954`
and separately inspected the rework commit
`ea4f5679fa4787700ce198b460de368fc81ad954`. The current `HEAD` is the rework
implementation/factory commit, not a later verifier commit.

Task-owned implementation bucket: 44 paths, exactly the original handoff
population; rework modifies 17 of those paths without adding another product
surface.

- Roots: `.bazelrc`, `.bazelversion`, `.gitignore`, `BUILD.bazel`,
  `MODULE.bazel`, `MODULE.bazel.lock`, `flake.nix`, and `flake.lock`.
- Contracts/docs: seven `config/` JSON/build paths, ADR-0001, `docs/build.md`,
  and the two `third_party/` paths.
- Toolchains/tools: three `toolchains/` paths, six `tools/build/` paths, four
  `tools/coverage/` paths, `tools/BUILD.bazel`, and `tools/format.py`.
- Tests: two benchmark paths, four top-level build-test paths, and four
  prohibited-fixture paths.

Factory/audit metadata bucket in the committed range: the original and rework
handoffs, prior verification report, five prior check-run records, active and
archived implementer/verifier/rework results, `.factory/run.json`,
`.factory/tasks/M0-001.json`, and the CLI-generated plan-row status in
`IMPLEMENTATION_PLAN.md`. The supplied authoritative run record and current
verifier status/result changes are uncommitted factory metadata. These paths do
not fail scope.

No unexplained out-of-scope implementation, hidden generated source path,
lock mutation outside the declared root surface, or handoff omission was
found. c-13 passes.

## Acceptance criteria

| Criterion | Result | Reason |
|---|---|---|
| BUILD-FR-001 | PASS | Required roots/locks exist; c-01/c-02 pass; the 39-coordinate inventory reconciles the selected graph and materializations. |
| BUILD-FR-002 | PASS | Nix supplies the environment and pinned tools; no secret or host compiler was selected. |
| BUILD-FR-003 | PASS | Bazel accounts for the current compile/link/test/benchmark actions; no independent source action was found. |
| BUILD-FR-004 | PASS | C++23 Clang 21.1.8 and LLD 21.1.8 actions passed with host fallback rejected. |
| BUILD-FR-005 | PASS | GCC 15.2.0 C++23 compatibility tests passed in the same reference environment. |
| BUILD-FR-006 | PASS | Required configurations execute; task-scoped TSan/fuzz non-applicability remains explicit. |
| BUILD-FR-007 | PASS | Zero real CMake/alternate-path findings; all three injected fixtures detected. |
| BUILD-FR-008 | **FAIL** | The real acquisition path is not admitted-manifest-gated before fetch, has no whole-profile 1,200-second deadline, and applies the aggregate-byte check only after fixed-output paths are in the Nix store. |
| BUILD-FR-010 (M0-001 slice) | PASS | Exactly one narrow production reference document and one complete selection/validation ADR; M0-002 ownership is preserved. |
| BUILD-FR-012 | PASS | Both executable toolchain wrappers map to canonical Nix+Bazel entry points. |
| BUILD-NFR-002 | PASS | Post-acquisition Bazel action evidence shows zero undeclared host inputs and zero action-time network capability. |
| BUILD-NFR-003 | PASS | Scanner reports zero real findings and complete fixture detection. |
| BUILD-NFR-005 | PASS | Required named configurations passed and non-applicability is explicit. |
| q-0003/q-0012 dependency-stack deliverable | PASS | The complete stack is exact, digest-pinned, admitted, compatible in the green suite, and Glaze is Nix/Bzlmod-only. |
| Factory 70% package coverage baseline | PASS | All three finite task-owned logic packages exceed 70%; only two empty package markers have reviewed exclusions. |

## Target conditions TC-01 through TC-10

| TC | Result | Reason |
|---|---|---|
| TC-01 | PASS | Roots, locks, exact inventory, and module graph reconcile. |
| TC-02 | **FAIL** | Post-acquisition actions are hermetic, but the acquisition phase itself does not enforce all profile bounds before promotion. |
| TC-03 | PASS | Pinned Clang/LLD/GCC and C++23 action selection passed. |
| TC-04 | PASS | Required configuration commands passed and scoped non-applicability is recorded. |
| TC-05 | PASS | All 39 resolved/fetched inputs have exact inventory, acquisition, digest, and admission coverage. |
| TC-06 | **FAIL** | The build/test/benchmark/format/coverage skeletons execute, but the acquisition negative skeleton does not exercise the real promotion boundary. |
| TC-07 | PASS | Prohibited-path and wrapper-map checks pass. |
| TC-08 | PASS | One narrow production reference document and ADR are retained. |
| TC-09 | PASS | Every finite task-owned logic package exceeds the exact 70% threshold and omission/exclusion cases fail closed. |
| TC-10 | **FAIL** | c-09 fails required semantics despite a green command transcript; c-12 and c-13 pass. |

## Negative and edge expectations

| ID | Result | Evidence / gap |
|---|---|---|
| N1 | PASS | Live graph reconciliation covers all 23 selected modules; missing/mismatched selected populations and invalid lock identities are rejected with `BUILD_LOCK_INVALID`. |
| N2 | PASS | Broken/host toolchain and silent-configuration fixtures return `BUILD_CONFIG_INVALID`; required configurations executed. |
| N3 | PASS | All three prohibited fixtures are detected while the task-owned real tree remains clean. |
| N4 | **FAIL** | The pure function rejects unadmitted/mutable/hash-mismatch values, and Nix rejects a wrong fixed-output hash, but a correctly hashed unadmitted distdir row can be realized before downstream inventory reconciliation. No real quarantine/non-promotion proof exists for that case. |
| N5 | **FAIL** | Exact/first-over coordinate/blob/total/time values are exercised by `acquisition_outcome`; the real path has only a per-fetch timeout and a post-realization aggregate sum, so the required before-promotion boundary is not exercised. |
| N6 | PASS | The network-negative fixture executes; action audit reports zero enabled network actions and zero undeclared host inputs. |
| N7 | PASS | Wrong reference identity is rejected; manual review confirms one exact narrow production document and no broader support claim. |
| N8 | PASS | All 39 inputs have exact digests and admissions, compatibility commands pass, and Glaze has no CMake route. |
| N9 | PASS | Exact 70% passes; first-below, absent LCOV, missing source, omitted owned source, and invented exclusion cases fail. The repository manifest covers all task-owned Python logic. |
| N10 | PASS | ASan, UBSan, and benchmark targets executed; TSan/fuzz task non-applicability is recorded. |

## Required disposition

Set `M0-001` to `BLOCKED`. Rework F-01, then run the complete registered suite
so c-09 retains evidence from the corrected acquisition state machine. Do not
change the accepted pinset, coverage gate, reference document, M0-002
ownership, task scope, or substrate ledger unless that rework independently
requires it.
