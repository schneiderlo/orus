# M0-002 Implementer Handoff

## Source identity

- Implementation base revision: `bdd18d1739f4d3e3e13a81c46fd5cd7e00cb1c9a`
- Exact tracked implementation tree snapshot before this handoff:
  `973b83d7106aa2bc2205edf6be210eea8ee242c6`
- Latest authoritative check run: `run-20260722T154456Z`
- The verifier should use the Factory-created revision containing this tree and
  handoff. The base revision does not contain the implementation.

## Summary of changes

- Added an Orus-owned C++23 contract library with strict, bounded canonical
  JSON parsing/emission, NFC normalization through private utf8proc types,
  SHA-256 through private OpenSSL EVP state, typed errors, subject-bound
  content identities, checked signed-integer arithmetic, and reusable resource
  guards. Glaze is used only as a private defensive parse after Orus byte
  prevalidation; no third-party or native layout crosses the public header.
- Added build-fact construction, the exact nine-predicate `M0-REFENV-v1`
  validator and ID recomputation, and streaming package-tree identity with
  sorted path/type/mode/link metadata, symlink confinement, hard-link handling,
  pre/post-read mutation checks, and finite entry/path/byte/RSS/time limits.
- Added shared governance validators for the SPDX descriptor and release
  evidence, including bounded identities/paths/timestamps, sorted validator
  and approval inventories, digest error preservation, and exact 12/12/3
  preapproval relationships. Added all five performance document validators
  with nested type/enum/bound/conditional relationships and exact fixed
  digests, integer-only median/MAD/nearest-rank primitives, both corpus report
  schemas with row-level topology/IPC/observation and aggregate validation,
  and the seven M0 cross-domain resource-contract rows.
- Added the independent cold-path Python parity surface for canonical bytes,
  SHA-256, median/MAD, and percentiles. Native and otherwise untrusted input
  remains authoritative only after acceptance by the C++ boundary.
- Added canonical fixed fixtures, cross-language byte/digest tests, subject and
  relationship mutation tests, exact/first-over resource tests, package-tree
  mutation/type/link tests, public-boundary inspection, and a real libFuzzer +
  ASan smoke target with five checked-in valid/malformed seeds.
- Extended Bazel/Nix integration for the already pinned OpenSSL and utf8proc
  closures. Bazel coverage now receives Nix's pinned LLVM `gcov` implementation
  so native LCOV records are collected rather than silently emitted as empty.
  The package gate discovers all task-owned native/Python/tool sources and
  fails closed per package.

## Frozen API, schema, error, and limit inventory

- C++ public surface: `JsonValue`, `ParseCanonicalJson`,
  `EmitCanonicalJson`, typed member helpers, `NormalizeNfc`, `IsValidNfc`,
  `CheckResourceUsage`, checked add/subtract/multiply, `Sha256Digest`,
  `Sha256Stream`, one-shot `Sha256`, `ContentIdentity`, `MakeBuildFacts`,
  `ValidateReferenceEnvironment`, `IdentifyPackageTree`, integer statistics,
  the three domain validators, and `M0SharedResourceContracts`.
- Python public surface: `ContractError`, `canonical_json`, `sha256_hex`, and
  `statistics`. It is an independent cold parity oracle, not an authority for
  untrusted/native bytes.
- Accepted schemas: `M0-BUILD-FACTS-v1`, `M0-REFENV-v1`,
  `M0-REFENV-OBSERVED-v1`, `M0-PACKAGE-TREE-v1`, `M0-SBOM-CONTRACT-v1`,
  `M0-RELEASE-EVIDENCE-v1`, `M0-PERF-WORKLOAD-v1`,
  `M0-PERF-RAW-SAMPLE-v1`, `M0-CONTROLLED-RUNNER-v1`,
  `M0-PERF-RESULT-v1`, `M0-PERF-COMPARISON-v1`, `M0-CORPUS-RUN-v1`, and
  `M0-CORPUS-RELIABILITY-v1`. The umbrella schema labels used for typed errors
  are `M0-CANONICAL-JSON-v1`, `M0-CONTENT-IDENTITY-v1`,
  `M0-GOVERNANCE-v1`, `M0-PERFORMANCE-v1`, and `M0-CORPUS-v1`.
- Error families are `CANONICAL_JSON_*`, `DIGEST_INVALID`,
  `BUILD_METADATA_INVALID`, `BUILD_PACKAGE_IDENTITY_INVALID`,
  `BUILD_REFENV_*`, `BUILD_UNVALIDATED_ENVIRONMENT`, `GOV_*`, `PERF_*`, and
  `CORP_REPORT_*`. Every public `Error` carries code, document schema, field
  path, expected/observed values, optional limit/offset, and message.
- Shared finite rows are `SEC-LIM-10-02` reference environment (96 KiB total,
  128 inputs, depth 8, 64 MiB RSS, 10 s), `SEC-LIM-10-03` package identity
  (100,000 entries, 16 GiB regular bytes, 256 MiB RSS, 1,200 s),
  `SEC-LIM-11-03` SPDX descriptor (16 MiB, 200,000 rows, depth 32, 256 MiB
  RSS, 120 s), `SEC-LIM-11-04` release evidence (16 MiB, 12 evidence rows,
  depth 16, 256 MiB RSS, 120 s), `SEC-LIM-14-01` performance documents
  (16 MiB, 100,000 rows, depth 16), `SEC-LIM-14-02` comparator (10,000
  samples, 256 MiB RSS, 120 s), and `SEC-LIM-15-03` corpus reports (16 MiB,
  100 runs, depth 16).

## Files changed

### Contract implementations and public boundary

- `.bazelrc`
- `MODULE.bazel`
- `config/BUILD.bazel`
- `contracts/BUILD.bazel`
- `contracts/build_contracts.cc`
- `contracts/canonical_json.cc`
- `contracts/crypto_resource.cc`
- `contracts/evidence_contracts.cc`
- `contracts/include/orus/contracts/contracts.h`
- `flake.nix`
- `python/orus_contracts/BUILD.bazel`
- `python/orus_contracts/__init__.py`
- `python/orus_contracts/canonical.py`
- `tools/build/nix_repositories.bzl`
- `tools/coverage/package_gate.py`
- `tools/coverage/packages.json`

### Automated tests, fuzz corpus, and fixed fixtures

- `tests/build/BUILD.bazel`
- `tests/build/contract_test.py`
- `tests/contracts/BUILD.bazel`
- `tests/contracts/boundary_test.py`
- `tests/contracts/build_test.cc`
- `tests/contracts/canonical_test.cc`
- `tests/contracts/evidence_test.cc`
- `tests/contracts/public_header_test.cc`
- `tests/contracts/python_parity_test.py`
- `tests/contracts/test_support.h`
- `tests/contracts/fixtures/corpus-reliability.json`
- `tests/contracts/fixtures/corpus-run.json`
- `tests/contracts/fixtures/perf-comparison.json`
- `tests/contracts/fixtures/perf-raw-sample.json`
- `tests/contracts/fixtures/perf-result.json`
- `tests/contracts/fixtures/perf-runner.json`
- `tests/contracts/fixtures/perf-workload.json`
- `tests/contracts/fixtures/reference-fixed.json`
- `tests/contracts/fixtures/reference-observed-fixed.json`
- `tests/contracts/fixtures/release-evidence-assembled.json`
- `tests/contracts/fixtures/sbom-descriptor.json`
- `tests/fuzz/BUILD.bazel`
- `tests/fuzz/canonical_json_parser_fuzz.cc`
- `tests/fuzz/corpus/canonical_json/invalid-duplicate`
- `tests/fuzz/corpus/canonical_json/invalid-number`
- `tests/fuzz/corpus/canonical_json/invalid-resource-depth`
- `tests/fuzz/corpus/canonical_json/valid-empty-object`
- `tests/fuzz/corpus/canonical_json/valid-nested`

Factory-generated status, authoritative check-run, and implementation-plan
metadata changed through the CLI and is separate from the task-owned paths.

## Commands run and outcomes

| Command | Outcome |
|---|---|
| `nix develop --command bazel test --config=dev //tests/contracts/...` | The first recovered-tree diagnostic run exposed two stricter evidence-test fixture defects; after correcting the fixture and preserving exact digest errors, the direct rerun passed all six targets. The current tree also passed as c-01 in the authoritative Factory run. |
| `nix develop --command bazel test --config=dev //tests/contracts:build_test` | Passed after adding exact tool/input/target and per-predicate type/operator/bound validation. |
| `nix develop --command bazel test --config=dev //tests/contracts:evidence_test` | Passed after adding governance, performance, and corpus relationship hardening and negative mutations. |
| `nix develop --command bazel test --config=asan //tests/contracts/...` | Passed all six targets as c-02 with leak detection and fail-fast ASan. |
| `nix develop --command bazel test --config=ubsan //tests/contracts/...` | Passed all six targets as c-03 with non-recovering UBSan. |
| `nix develop --command bazel test --config=fuzz //tests/fuzz:canonical_json_parser_fuzz_smoke` | Passed as c-04 over five checked-in seeds and 256 executions with no crash, timeout, OOM, ASan, or UBSan finding. |
| `nix develop --command bazel coverage --config=dev --combined_report=lcov --instrumentation_filter='^//(contracts|python/orus_contracts|tools)[/:]' //tests/build/... //tests/contracts/... && nix develop --command bazel run //tools/coverage:package_gate -- --threshold=70` | Passed directly and as c-05. Current coverage: `contracts` 85.34%, `python/orus_contracts` 85.59%, `tools` 79.41%, `tools/build` 75.32%, and `tools/coverage` 74.04%. |
| `nix develop --command bazel run //tools:format` | Passed over the complete task-owned text/configuration surface. |
| `nix develop --command bazel build --config=release //contracts:contracts //python/orus_contracts:orus_contracts` | Passed; native shared/static libraries and the Python library build in release configuration. |
| `nix flake check` | Passed the pure bootstrap derivation and formatter checks on the current tree. |
| `nix develop --command bazel build --config=release //contracts:contracts //python/orus_contracts:orus_contracts //tests/fuzz:canonical_json_parser_fuzz_smoke` | Diagnostic-only probe: production libraries built, while the fuzz executable correctly could not link without the registered `--config=fuzz` libFuzzer `main`. The corrected production-only release build and the registered fuzz check both passed. |
| `git diff --check` | Passed with no whitespace errors. |
| `uv run --no-project /home/lschneid/workspace/schneiderlo/factory/factory.py check run M0-002` | Authoritative current-tree run `run-20260722T154456Z` green: c-01 through c-05 passed. c-06/c-07 remain independent manual verifier checks. |

## Known risks and assumptions

- This task freezes shared contracts and fixtures; it does not implement the
  downstream CLI, governance producer, performance harness, corpus runtime, CI
  aggregator, security orchestration, or release approval marker owned by
  M0-003 through M0-009.
- Package traversal is intentionally local-filesystem and fail-closed. Any
  special file, escaping/invalid symlink, invalid NFC path, hard-link
  inconsistency, or pre/post-read metadata change rejects the identity rather
  than attempting recovery.
- Python parity intentionally uses the pinned standard library only. It may
  cross-check trusted/cold artifacts, but consumer tasks must not promote it to
  authority for native or untrusted parsing.
- The fixed governance fixture is an assembled synthetic contract object only;
  no approval marker, real release claim, or downstream producer evidence was
  created.

## Verifier focus areas

- Perform c-06 against the frozen inventory above: confirm public headers hide
  Glaze, utf8proc, and OpenSSL types; compare schema literals, derived IDs,
  relationship rules, typed errors, and every finite limit to the approved
  specs and question ledger.
- Stress the newly explicit reference input/tool/predicate typing and the
  governance approval timestamp/order/digest paths; verify malformed content
  cannot be collapsed into a generic relationship error or accepted after ID
  recomputation.
- Independently compare the five performance fixture digests and the package
  manifest/reference-environment identities with their specification literals;
  mutate nested workload, raw-sample, runner, result, and comparison fields to
  confirm enum, uint32, conditional-null, ordering, and authority rules fail
  closed.
- Inspect package traversal around symlink confinement, hard-link
  representation, sorted unsigned-byte paths, and read-race rejection.
- Confirm the Python surface is truly independent and cold-path only, and that
  the parity tests never normalize a rejected untrusted byte stream into an
  accepted artifact.
- Inspect the real fuzz log for five loaded seeds and `Done 256 runs`; confirm
  no empty/skipped target or product test asserts Factory lifecycle metadata.
- Perform c-07 over the Factory-created handoff revision, keeping task-owned
  implementation separate from `.factory/**`, generated plan status, and
  append-only audit history.

## Pending Healing

None. M0-002 introduced no temporary mock, fake provider, in-memory boundary,
or other substrate stand-in.
