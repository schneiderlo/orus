# M0-001 Verification Plan

## Preconditions

1. Run every runnable command non-interactively from the repository root in
   the order below. The factory runner, not an ad hoc shell transcript, is the
   authoritative latest-run gate.
2. The implementation Author has stopped changing the task-owned surface and
   has written `.agents/M0-001/handoff.md` with the implementation base/source
   revision, changed-path inventory, self-check summary, produced evidence,
   and known limitations.
3. The Verifier is a different agent from the Author and starts from the exact
   source revision named in the handoff.
4. Required immutable Nix/Bzlmod inputs may be materialized only through the
   named bounded, hash-verifying acquisition profile. Build, test, benchmark,
   coverage, and package actions after acquisition must be network-denied.
5. These checks exercise the real local Nix/Bazel toolchain. No substituted
   database, auth service, provider, or GitHub Actions run is used, and no
   check is labeled end-to-end.
6. Do not make product test success depend on factory task/plan status, check
   ledger content, or `.factory/**` bytes. Those legitimately evolve between
   runs and are governed by the factory CLI.
7. For scope review, separate task-owned implementation paths from factory
   metadata. `.factory/run.json`, `.factory/results/**`,
   `.factory/questions/**`, `.factory/commits/**`, and `.factory/logs/**` are
   factory audit evidence and must not by themselves fail task scope. Historical
   `.agents/**` handoffs/reports are also evidence and are not scanned as
   product implementation.

## Commands/Steps

Run in this exact order. `V12` and `V13` are judgment-only checks registered as
manual factory checks; every other step is a registered runnable command.

### V1 — Nix root, locks, checks, and delegation

```bash
nix flake check
```

Proves that the committed flake/lock evaluate, the supported-system/check
surface is valid, locks do not mutate during the check, and flake checks
delegate source checks into the Bazel path.

### V2 — Bzlmod lock and dependency graph

```bash
nix develop --command bazel mod graph
```

Proves that the locked module graph resolves through the pinned development
environment without a host Bazel or lock rewrite.

### V3 — C++23 release toolchain and LLD link path

```bash
nix develop --command bazel build --config=release //...
```

Proves that all current release targets build through the declared C++23
Clang/LLVM toolchain and release link path. The retained action evidence must
identify pinned Clang and LLD and must not identify a host compiler/linker.

### V4 — Canonical M0-001 dev tests

```bash
nix develop --command bazel test --config=dev //tests/build/...
```

This is the primary automated test gate. It must execute, not merely discover,
the lock/root, C++23, toolchain, configuration-contract, dependency containment,
bootstrap reference-contract, wrapper-map, and positive/negative fixture tests
enumerated below.

### V5 — GCC compatibility tests

```bash
nix develop --command bazel test --config=gcc //tests/build/...
```

Proves that the same reference environment selects the pinned GCC C++23
compatibility toolchain for every applicable M0-001 test target, with explicit
applicability for any exclusion.

### V6 — AddressSanitizer build tests

```bash
nix develop --command bazel test --test_output=errors --config=asan //tests/build/...
```

Passes only when applicable task-owned native tests execute under ASan with no
finding and configuration reconciliation detects any silent skip/fallback.

### V7 — UndefinedBehaviorSanitizer build tests

```bash
nix develop --command bazel test --test_output=errors --config=ubsan //tests/build/...
```

Passes only when applicable task-owned native tests execute under UBSan with
no finding and configuration reconciliation detects any silent skip/fallback.

### V8 — Benchmark infrastructure smoke

```bash
nix develop --command bazel test --test_output=errors --config=benchmark //tests/benchmarks/...
```

Proves that the Google Benchmark-capable skeleton is a real Bazel test surface
under the declared benchmark configuration. This is a functional smoke test,
not authoritative performance evidence.

### V9 — Acquisition and hermeticity audit

```bash
nix develop --command bazel run //tools/build:hermeticity_audit
```

Passes only when the audit internally proves the admitted acquisition success
path, exact-bound and first-over limits, unadmitted/mutable/hash-mismatch
rejection, read-only promotion, minimal-host operation, complete Bazel action
ownership, zero undeclared host inputs, and zero action-time network access.

### V10 — CMake and alternate-path audit

```bash
nix develop --command bazel run //tools/build:prohibited_path_scan
```

Passes only when the real task-owned product tree has zero prohibited finding
and every injected CMake/invocation/independent-wrapper fixture is detected.
The scan scope is product sources and task-owned deliverables; it excludes
`.factory/**` and historical `.agents/**` evidence.

### V11 — Package-scoped baseline coverage gate

```bash
nix develop --command bazel coverage --config=dev --combined_report=lcov //tests/build/... && nix develop --command bazel run //tools/coverage:package_gate -- --threshold=70
```

The second command performs the semantic assertion that the factory runner
cannot infer from exit status alone. It must fail if coverage data is absent,
if any task-owned logic package is below 70% line coverage, if a package is
silently omitted, or if an exclusion is not present in the reviewed finite
manifest. Repository-wide averaging must not mask a failing package.

### V12 — Manual pin, ADR, admission, and reference-contract review

Inspect the exact toolchain/dependency inventory, immutable digests, bootstrap
admission records, compatibility evidence, action/toolchain selection, and the
single production `M0-REFENV-v1` document plus ADR. Confirm:

- Glaze is exactly v7.5.0 and has no CMake exposure;
- utf8proc, OpenSSL 3 EVP, GoogleTest, Google Benchmark, pinned Python,
  Nixpkgs, Bazel, Clang/LLVM/LLD, and GCC are exact, mutually compatible, and
  digest-pinned;
- every admitted dependency records purpose, license, closure/ABI/build cost,
  containment, alternatives, and rollback/removal;
- tool and release-link action evidence names the selected pinned tools;
- exactly one reference document declares Linux x86-64,
  `validated_reference`, the complete selected coordinate set, exact host
  predicates, and no broader support claim; and
- this task does not duplicate the reusable canonical validator owned by
  M0-002 or claim completed BUILD-FR-009/BUILD-FR-011 behavior.

### V13 — Manual task-scope and evidence audit

Using the base/source revision recorded in `.agents/M0-001/handoff.md`, inspect
the complete change set and classify it into two buckets:

1. **Task-owned implementation:** root Nix/Bazel/Bzlmod files and locks,
   toolchains/configuration, dependency/reference ADR and bootstrap admissions,
   build/test/benchmark/format/coverage skeletons, task build tests/fixtures,
   acquisition/hermeticity/prohibited-path tooling, and the minimum
   wrapper/documentation mapping required by BUILD-FR-012.
2. **Factory/audit metadata:** `.factory/run.json`,
   `.factory/results/**`, `.factory/questions/**`, `.factory/commits/**`,
   `.factory/logs/**`, and `.agents/**` evidence.

Factory/audit metadata is reported separately and does not fail scope by
itself. Fail the manual check for any implementation path outside the In scope
list, any hidden generated source/build path, any unexplained lock mutation,
or any task-owned deliverable missing from the handoff.

## Expected Results

| Step | Required result |
|---|---|
| V1 | Exit 0; flake/lock remain byte-identical; x86-64 checks resolve; no secret or alternate source action is required. |
| V2 | Exit 0; module graph contains the exact locked dependency set and no unapproved/floating module. |
| V3 | Exit 0; all release targets use C++23, pinned Clang/LLVM and LLD, with no host fallback. |
| V4 | Exit 0; all positive and negative build-contract tests execute; no skipped required target or unexpected failure. |
| V5 | Exit 0; applicable tests compile/run with pinned GCC as C++23; every exclusion has the exact scoped reason. |
| V6-V7 | Exit 0; applicable native tests execute under the named sanitizer with zero finding; non-applicability is explicit. |
| V8 | Exit 0; at least one real benchmark smoke target executes through Google Benchmark and the Bazel benchmark configuration. No performance authority is claimed. |
| V9 | Exit 0; audit reports zero undeclared host input/network action and all acquisition negative/boundary fixtures return their required typed failure before promotion. |
| V10 | Exit 0; zero real-tree findings and 100% detection of injected prohibited fixtures. |
| V11 | Exit 0; coverage data exists and every task-owned logic package is at least 70%; no unreviewed exclusion or aggregate-only pass. |
| V12 | PASS with cited file/action evidence for every bullet; no floating pin, CMake path, broad support claim, or M0-002 ownership violation. |
| V13 | PASS with separate task-owned and factory-metadata path inventories; no unexplained out-of-scope implementation. |

## Negative/Edge Tests

The runnable suites/audits must exercise these cases internally so their
registered commands return 0 only when the expected negative behavior occurs:

| ID | Fixture / edge | Required outcome | Covered by |
|---|---|---|---|
| N1 | Missing, floating, mutated, or resolution-rewritten Nix/Bzlmod input. | Reject before source action with `BUILD_LOCK_INVALID`; leave committed locks unchanged. | V1, V2, V4 |
| N2 | Deliberately broken toolchain, host compiler/linker fallback, or silent empty required configuration. | Test detects and rejects with `BUILD_CONFIG_INVALID`; no passing applicability result. | V3-V7 |
| N3 | CMakeLists/preset/cache/generator metadata, executable CMake invocation, documentation for a second path, or wrapper-only source action. | Every injected fixture produces `BUILD_PROHIBITED_PATH`; real tree remains zero findings. | V4, V10 |
| N4 | Unadmitted, mutable, or hash-mismatched acquisition coordinate. | `BUILD_ACQUISITION_DENIED`; bytes are quarantined/discarded and never promoted or interpreted. | V9 |
| N5 | Independent acquisition fixtures at exactly 128 coordinates, one 4-GiB blob, 16 GiB total, and 1,200 seconds; then the first requested unit over each applicable bound. | Each exact-bound fixture succeeds when otherwise valid; its first-over counterpart fails before promotion with `BUILD_ACQUISITION_DENIED`. | V9 |
| N6 | Build/test/package action attempts network or an undeclared host path after successful acquisition. | Stop the action with `BUILD_UNDECLARED_INPUT`; no valid action/package evidence. | V9 |
| N7 | Wrong reference schema/ID/member bytes, incomplete production coordinate inventory, or a second/broader supported-environment claim. | Bootstrap contract test/manual review rejects it; no validated or broad support outcome. Full reusable validator/resource behavior remains M0-002-owned. | V1, V4, V12 |
| N8 | Glaze is reachable through CMake, a dependency lacks an immutable digest/admission, or pins are ABI/closure incompatible. | Module/check/manual review fails; pinset is not admitted. | V1, V2, V4, V10, V12 |
| N9 | Task-owned logic package at exactly 70%, below 70%, missing coverage data, missing from the report, or excluded without finite approval. | Exactly 70% passes; every other listed defect fails the package gate. | V4, V11 |
| N10 | ASan/UBSan failure, non-executed benchmark/configuration target, or unrecorded not-applicable cell. | Named test command fails; no green factory run. | V4-V8 |

## Evidence to attach

The Verifier links or summarizes these stable artifacts in
`.agents/M0-001/verification-report.md`:

- `.agents/M0-001/handoff.md` and the exact verified source revision;
- ordered factory check-run summary plus stdout/stderr or retained log link for
  V1-V11;
- exact pin inventory, lock digests, module graph, compatibility matrix,
  dependency/reference ADR, bootstrap admission records, and production
  `M0-REFENV-v1` bytes/digest;
- toolchain/action conformance evidence showing Clang, LLD, GCC, C++23, and
  absence of host fallback;
- configuration/applicability and negative-fixture test reports;
- acquisition-boundary and hermeticity reports, including exact-bound/
  first-over and zero-network/zero-undeclared-input evidence;
- prohibited-path report showing zero real findings and injected-fixture
  detection;
- LCOV data, per-package threshold report, and reviewed finite exclusion
  manifest;
- separate task-owned implementation and factory/audit metadata path
  inventories; and
- a criterion-by-criterion sign-off table with PASS/FAIL and reasons.

Do not use an overwritable `.factory/results/{step}/r{n}/result.json` as
inherited evidence. If a later task consumes this result, it must cite the
append-only `.agents/M0-001/` handoff/verification artifacts or
`.factory/results/_archive/`.

## Sign-off checklist (PASS/FAIL + reasons)

The Verifier must record `PASS` or `FAIL`, a concise reason, and an evidence
link for every item:

- [ ] Latest factory-run execution of V1-V11 is green; no command is stale,
  skipped, optional, or replaced by an unregistered command.
- [ ] V12 and V13 are both addressed explicitly in the verification report.
- [ ] TC-01 through TC-10 and every task-ledger requirement pass with the
  traceability below.
- [ ] All N1-N10 negative/edge expectations were actually exercised and
  produced the required outcome.
- [ ] Nix/Bazel ownership is singular; CMake, host fallback, undeclared inputs,
  and action-time network are absent.
- [ ] Exact pins/admissions and the single reference document/ADR are complete,
  compatible, immutable, and narrowly worded.
- [ ] Every task-owned logic package meets the 70% line gate without masking
  or an unreviewed exclusion.
- [ ] Task-owned implementation is within scope; factory metadata is reported
  separately and did not create a false scope failure.
- [ ] No M0-002 canonical-core implementation, completed package/CLI claim,
  live GitHub claim, broad Linux support, or M1+ behavior was introduced.
- [ ] Verifier identity differs from Author identity, evidence names the exact
  source revision, and no unresolved Andon remains.

Any `FAIL` prevents acceptance and must include the failed criterion, observed
result, retained evidence, and required rework. A prose caveat cannot convert a
failed runnable check into PASS.

## Acceptance Criteria Traceability (AC -> verification step)

| Acceptance criterion | Verification step(s) | Specific proof |
|---|---|---|
| BUILD-FR-001 | V1, V2, V4, V12; N1 | Required roots/locks, exact coordinate inventory, stable resolution, and module graph. |
| BUILD-FR-002 | V1, V4, V9 | Nix-only environment on minimal host, no secret, no undeclared host tool. |
| BUILD-FR-003 | V2, V3, V4, V9, V13 | Bazel owns every source action; query/aquery and scope evidence find no independent action. |
| BUILD-FR-004 | V3, V4, V6, V7, V12; N2 | C++23 conformance, pinned Clang action, pinned LLD release link, fallback rejection. |
| BUILD-FR-005 | V5, V12; N2 | Pinned GCC C++23 compatibility and applicability reconciliation in the same reference environment. |
| BUILD-FR-006 | V3-V8, V12; N2, N10 | Named-config contract, actual execution, explicit non-applicability, broken fallback/empty rejection. |
| BUILD-FR-007 | V4, V10, V13; N3, N8 | Zero CMake/alternate-path findings and positive detection fixtures. |
| BUILD-FR-008 | V1, V4, V9, V12; N4-N6 | Bounded hash-verifying acquisition, read-only promotion, network-denied hermetic actions, untrusted-input rejection. |
| BUILD-FR-010 (M0-001 selection/publication slice) | V1, V4, V12, V13; N7 | Exactly one production contract and ADR with complete pins/predicates/compatibility/rollback; reusable validator remains M0-002-owned. |
| BUILD-FR-012 | V1, V4, V10, V12, V13; N3 | Machine-readable wrapper-to-canonical mapping and zero wrapper-only build/support claim. |
| BUILD-NFR-002 | V4, V9; N4-N6 | Zero undeclared host inputs and zero action-time network after acquisition. |
| BUILD-NFR-003 | V4, V10, V13; N3 | Zero CMake/independent paths and complete injected-fixture detection. |
| BUILD-NFR-005 | V3-V8, V12; N2, N10 | Every named configuration resolves with real execution or exact scoped non-applicability; no host fallback/silent skip. |
| q-0003/q-0012 and task dependency-stack deliverable | V1, V2, V4, V8, V10, V12; N8 | Exact compatible Glaze/utf8proc/OpenSSL/GoogleTest/Google Benchmark/Python/toolchain pins, digests, containment, admissions, and no CMake. |
| Factory baseline coverage | V4, V11; N9 | Per-package 70% line threshold with fail-closed missing/exclusion behavior. |

## Coverage Gate (command, scope, threshold, pass rule)

**Command:**

```bash
nix develop --command bazel coverage --config=dev --combined_report=lcov //tests/build/... && nix develop --command bazel run //tools/coverage:package_gate -- --threshold=70
```

**Scope:** Every task-owned logic package exercised by the M0-001 build tests,
including executable configuration, acquisition/hermeticity, prohibited-path,
wrapper-map, and reference bootstrap validation logic. Declarative Nix/
Starlark/configuration, vendored/generated code, and fixture-only data may be
excluded only by a finite reviewed manifest; moving logic into an excluded
path fails review.

**Threshold:** At least 70% line coverage for every included package. This is
the factory baseline applicable to this first build-contract unit; later plan
gates raise the same package-scoped policy to 75% at M0-009 and 80% at M0-010.

**Pass rule:** Both command clauses exit 0, coverage data is current and
non-empty, every in-scope package is represented, each package is at or above
70%, and every exclusion matches the reviewed finite manifest. Exact 70%
passes. A lower package, missing/stale data, package omission, unauthorized
exclusion, or repository-wide average that masks a lower package fails.
