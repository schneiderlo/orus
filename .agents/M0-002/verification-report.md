# M0-002 Verification Report

**Verdict:** BLOCKED — required manual check c-06 fails  
**Factory task:** `M0-002`  
**Verifier:** Factory step `10_verifier`, attempt `r1`  
**Author handoff:** Factory step `09_implementer`, attempt `r2`  
**Judged check run:** `run-20260722T155248Z` (GREEN)  
**Implementation base:** `bdd18d1739f4d3e3e13a81c46fd5cd7e00cb1c9a`  
**Audited implementation commit:** `43cb86260538dfefc3929ec384016ade901fbc37`  
**Handoff pre-metadata tree:** `973b83d7106aa2bc2205edf6be210eea8ee242c6`

## Decision

The registered machine run is current, complete, and green, and c-07's scope
audit passes. Acceptance is nevertheless blocked because c-06 found contract
behavior that does not satisfy the frozen specifications or the task's own
verification plan. This is implementation/test rework, not a specification
defect: the A3 and `verification.md` state the missing behavior precisely.

The principal blockers are:

1. Production resource limits are not enforced by production observations.
   `ValidateReferenceEnvironment`, the three domain validators, and
   `IdentifyPackageTree` accept default-zero or optional caller-injected
   `ResourceUsage`; the ordinary production paths do not measure current RSS,
   and only the package walk samples real elapsed time. Its elapsed-time check
   occurs after each whole entry, not during a potentially long file read.
   Therefore the exact 64/256 MiB and 10/120/1,200-second ceilings are test
   seams, not real guards. Evidence:
   `contracts/build_contracts.cc:197-207,453-475,523-599`,
   `contracts/evidence_contracts.cc:331-345,1265-1305`, and
   `tests/contracts/build_test.cc:309-315`.
2. Performance result validation is not a raw-result reconciliation. The
   public validator accepts only one document, validates the `raw_samples`
   descriptor syntactically, and checks only that reported statistics lie
   between reported minimum/maximum. It never receives the raw array, verifies
   its byte length/SHA-256/order/count, derives statistics from its measured
   values, or enforces workload-dependent allocation/storage conditions.
   Evidence: `contracts/include/orus/contracts/contracts.h:237-239` and
   `contracts/evidence_contracts.cc:746-894`. The fixed-fixture test validates
   the raw and result documents independently at
   `tests/contracts/evidence_test.cc:151-166`.
3. Corpus validation is incomplete. All nested topology, IPC, partition,
   observation, lifecycle, cleanup, and diagnostic validation is inside
   `if (*passed)`, so non-success reports may carry wrong nested types and
   bounds. The registered fault test proves this gap by changing only
   `fault_mode`, `passed`, and `terminal` in a success report and expecting the
   otherwise success-shaped document to pass. The reliability validator never
   resolves the referenced run reports, so it cannot recompute digest validity,
   per-terminal counts, `passed_runs`, or source/environment binding. Evidence:
   `contracts/evidence_contracts.cc:1031-1153,1156-1222` and
   `tests/contracts/evidence_test.cc:316-340`.
4. Exact literal limits and relationships drift from the specs. The common
   domain parser permits 16 MiB for an `M0-SBOM-CONTRACT-v1` descriptor whose
   maximum is 64 KiB, and 16 MiB for an `M0-CORPUS-RUN-v1` document whose
   maximum is 1 MiB. `SEC-LIM-14-02` records only a 10,000 count and omits the
   separate 100,000-pair maximum. Release-evidence cycle detection is a path
   substring heuristic; it accepts a current-scan schema under a neutral path
   and rejects unrelated paths containing those words. Evidence:
   `contracts/evidence_contracts.cc:331-345,475-485,1265-1317`.
5. `M0-BUILD-FACTS-v1` is built only from arbitrary caller-supplied strings.
   No task-owned Bazel status/generated provider binds the five facts to the
   selected build, and the test fabricates the values directly. This does not
   establish the authoritative embedded dev/release facts required by
   BUILD-FR-009 and TC-03. Evidence: `contracts/build_contracts.cc:163-195`,
   `tests/contracts/build_test.cc:51-79`, and the absence of any non-test call
   site in the audited tree.
6. N10's required failure simulations are absent. `boundary_test.py` checks
   source strings for corpus registration and `__builtin_trap`; it does not
   simulate an empty corpus, zero executions, crash, timeout, OOM, ASan/UBSan
   finding, or corpus omission. The real fuzz run did load five seeds and
   complete 256 executions, but `--config=fuzz` enables libFuzzer plus ASan,
   not UBSan. Evidence: `tests/contracts/boundary_test.py:20-35`,
   `tests/fuzz/BUILD.bazel:8-19`, `.bazelrc`, and c-04's retained run log.

## Registered checks

| Check | Result | Evidence and judgment |
|---|---|---|
| c-01 / V1 dev | PASS | Factory record reports exit 0 in 46.7 s. The retained stdout lists all six contract targets and `Executed 6 out of 6 tests: 6 tests pass`. |
| c-02 / V2 ASan | PASS | Exit 0 in 54.5 s; all six targets executed. The log shows `-fsanitize=address` and `ASAN_OPTIONS=detect_leaks=1:halt_on_error=1`; no finding appears. |
| c-03 / V3 UBSan | PASS | Exit 0 in 46.6 s; all six targets executed. The log shows non-recovering undefined-behavior sanitizer options; no finding appears. |
| c-04 / V4 fuzz | PASS (registered command) | Exit 0 in 17.9 s and 1/1 target passed. The Bazel test log loaded five seed files and finished 256 runs with no crash or ASan finding. N10 remains FAIL because its verifier simulations and fuzz-time UBSan evidence are absent. |
| c-05 / V5 coverage | PASS | Exit 0 in 61.7 s; 9/9 tests executed. Fresh package results are `contracts` 85.34%, `python/orus_contracts` 85.59%, `tools` 79.41%, `tools/build` 75.32%, and `tools/coverage` 74.04%, all above the exact 70% floor. |
| c-06 / V6 frozen contract | **FAIL** | The six blockers above violate literal build/resource, governance, performance, corpus, and negative-evidence requirements. Bullet-by-bullet results follow below. |
| c-07 / V7 scope/lifecycle | PASS | The complete `bdd18d1..43cb862` range contains 44 task-owned implementation paths and 8 factory/audit paths. No dependency lock/pin, acquisition semantics, alternate build path, inherited M0-001 evidence, or unrelated product source changed. |

Machine logs are retained under
`.factory/logs/checks/M0-002/run-20260722T155248Z/`; the run record is
`.factory/checks/M0-002/runs/run-20260722T155248Z.json`.

## c-06 / V6 manual review

| Required review item | Result | Evidence |
|---|---|---|
| Orus-owned public types; no Glaze/utf8proc/OpenSSL/native layout exposure | PASS | `contracts/include/orus/contracts/contracts.h` contains only standard-library and Orus types. Third-party headers occur only in private `.cc` files. Both the public consumer compile test and boundary scan passed. |
| Strict canonical prevalidation and exact canonical byte rules | PASS | `contracts/canonical_json.cc` performs byte/depth/BOM/token/UTF-8/NFC/number/order/duplicate/escape checks, exact re-emission comparison, then private Glaze parsing. The fixed negative corpus and canonical tests passed. |
| C++ authority and pinned Python cold-path parity | PASS | The Python module labels itself a cold parity oracle; no non-test product consumer promotes it to authority. Both implementations reproduce the five performance byte/digest goldens, and noncanonical byte inputs reject. |
| Build facts, reference validation, identities, schemas, typed errors, and numeric limits match the specs literally | **FAIL** | Build facts are not bound to Bazel build inputs; real RSS/deadline enforcement is absent; descriptor/run limits are widened; performance and corpus cross-document relationships are not validated. |
| Downstream governance/performance/corpus/security ownership is preserved | PASS | No release producer/gate, performance harness/comparator workflow, native corpus runtime, CI provider behavior, or security reconciler was implemented or claimed complete. |
| Handoff inventory and limitations are exact | **FAIL** | The file/surface inventory and tree identity are complete, but the handoff claims raw ordering/digest/statistic reconciliation and production-bounded validators that the implementation does not provide, and it omits these known limitations. |

## c-07 / V7 revision and lifecycle audit

The audited range is exactly
`bdd18d1739f4d3e3e13a81c46fd5cd7e00cb1c9a..43cb86260538dfefc3929ec384016ade901fbc37`.
The handoff's pre-metadata tree `973b83d7...` differs from commit `43cb862...`
only by the eight audit paths listed below, so the handoff identity is
reconciled. The later `run-20260722T155248Z` run record/logs and current
verifier status changes are also factory metadata and are outside product
scope.

### Task-owned implementation (44 paths)

- Build/config integration: `.bazelrc`, `MODULE.bazel`, `config/BUILD.bazel`,
  `flake.nix`, `tools/build/nix_repositories.bzl`.
- Contract sources: `contracts/BUILD.bazel`,
  `contracts/include/orus/contracts/contracts.h`,
  `contracts/build_contracts.cc`, `contracts/canonical_json.cc`,
  `contracts/crypto_resource.cc`, `contracts/evidence_contracts.cc`.
- Python parity: `python/orus_contracts/BUILD.bazel`,
  `python/orus_contracts/__init__.py`,
  `python/orus_contracts/canonical.py`.
- Retained build/coverage integration: `tests/build/BUILD.bazel`,
  `tests/build/contract_test.py`, `tools/coverage/package_gate.py`,
  `tools/coverage/packages.json`.
- Contract tests: `tests/contracts/BUILD.bazel`,
  `tests/contracts/boundary_test.py`, `tests/contracts/build_test.cc`,
  `tests/contracts/canonical_test.cc`, `tests/contracts/evidence_test.cc`,
  `tests/contracts/public_header_test.cc`,
  `tests/contracts/python_parity_test.py`,
  `tests/contracts/test_support.h`.
- Eleven fixed fixtures: `tests/contracts/fixtures/corpus-reliability.json`,
  `corpus-run.json`, `perf-comparison.json`, `perf-raw-sample.json`,
  `perf-result.json`, `perf-runner.json`, `perf-workload.json`,
  `reference-fixed.json`, `reference-observed-fixed.json`,
  `release-evidence-assembled.json`, and `sbom-descriptor.json`.
- Fuzz target/corpus: `tests/fuzz/BUILD.bazel`,
  `tests/fuzz/canonical_json_parser_fuzz.cc`, and the five files under
  `tests/fuzz/corpus/canonical_json/`.

### Factory/audit metadata (8 paths)

- `.agents/M0-002/handoff.md`
- `.factory/checks/M0-002/runs/run-20260722T145320Z.json`
- `.factory/checks/M0-002/runs/run-20260722T154456Z.json`
- `.factory/results/09_implementer/r2/result.json`
- `.factory/results/_archive/20260722T155248Z_09_implementer_r2_M0-002.json`
- `.factory/run.json`
- `.factory/tasks/M0-002.json`
- `IMPLEMENTATION_PLAN.md` (CLI-generated status row only)

No `.agents/M0-001/**` historical evidence changed or was treated as product
source. No `flake.lock`, `MODULE.bazel.lock`, dependency coordinate/digest, or
toolchain pin changed. The `MODULE.bazel`/Nix edits only expose already pinned
OpenSSL/utf8proc closures and LLVM coverage tooling to Bazel.

## Target conditions

| Target | Result | Reason |
|---|---|---|
| TC-01 | PASS | Canonical parsing/emission rejects the declared byte forms before private Glaze parsing and emits no LF. |
| TC-02 | PASS | utf8proc/OpenSSL are private; SHA-256/NFC and Python cold parity goldens pass. |
| TC-03 | **FAIL** | `MakeBuildFacts` validates arbitrary caller values; no authoritative Bazel-generated dev/release facts are embedded or tested. |
| TC-04 | **FAIL** | Fixed/production IDs and nine fact outcomes pass, but production RSS/deadline use is not observed and therefore not enforced. |
| TC-05 | **FAIL** | Subject/package mutation checks pass, but package traversal does not measure RSS and does not enforce its deadline during file reads. |
| TC-06 | **FAIL** | Inventory cardinalities pass, but current-scan/self-reference detection is path-name based and literal descriptor/resource bounds drift. |
| TC-07 | **FAIL** | Five fixed documents validate independently, but raw array bytes/order/digest/statistics and workload conditional relationships are not reconciled. |
| TC-08 | **FAIL** | Success fixture mutations are checked, but non-success nested documents and referenced aggregate run documents are not validated. |
| TC-09 | **FAIL** | Limit rows and injection seams exist; real production usage observation/cleanup enforcement and several literal per-schema limits do not. |
| TC-10 | **FAIL** | Dev/ASan/UBSan/fuzz/coverage commands pass, but N10 simulations and fuzz-time UBSan evidence are missing. |

## Acceptance criteria

| Criterion | Result | Evidence-based judgment |
|---|---|---|
| BUILD-FR-009 | **FAIL** | No authoritative build-status/generated source supplies the five facts; only caller literals are validated. |
| BUILD-FR-010 | **FAIL** | Canonical/refenv semantics pass, but the 64 MiB/10 s production observation is absent. |
| BUILD-FR-011 | **FAIL** | Identity and mutation behavior is present; real 256 MiB/1,200 s package-walk enforcement is not. |
| BUILD-NFR-004 | PASS | Exactly one production reference contract remains and no broader compatibility claim was added. |
| GOV-FR-006 shared-core | **FAIL** | Descriptor structure/golden passes, but the descriptor accepts the generic 16 MiB parser cap instead of 64 KiB and does not have a real resource guard. |
| GOV-FR-008 shared-core | **FAIL** | 12/12/3 cardinality passes; exact current-scan/no-self relationship validation is not semantic. |
| GOV-NFR-004 | **FAIL** | Syntax/subject classes are checked, but referenced evidence bytes and semantic current-scan/self relationships cannot be reconciled by the one-document API. |
| GOV-NFR-006 shared-core | PASS | Canonical JSON/SHA-256 primitives and fixed descriptor bytes are deterministic; producer reproducibility remains downstream. |
| PERF-FR-003 shared-core | **FAIL** | The five fixtures/digests pass, but raw/result/workload relationships and derived statistics do not reconcile. |
| PERF-FR-012 shared-core | **FAIL** | Byte/depth parsing exists; real RSS/time enforcement and complete relationship/overflow work bounds do not. |
| PERF-NFR-003 | **FAIL** | Required fields are structural only; raw samples are not resolved or reconciled. |
| PERF-NFR-005 shared-core | **FAIL** | The comparator row omits the distinct 100,000-pair cap, and production RSS/time use is caller-injected only. |
| CORP-FR-013 shared-core | **FAIL** | Valid examples/success mutations pass, but failure reports and referenced aggregate runs are not fully validated; run size uses 16 MiB rather than 1 MiB. |
| SEC-FR-005 | PASS | Lowercase SHA-256 parsing, four distinct subject enum values, exact-byte binding, and substitution rejection are present. |
| SEC-FR-007 shared-core | **FAIL** | Rows are finite, but enforcement and several per-operation numeric distinctions do not match the owner specs. |
| SEC-NFR-003 | **FAIL** | Core tamper/substitution checks exist, but complete subject/reference/self-link reconciliation is not demonstrated. |
| q-0012/q-0014 | PASS | Glaze v7.5.0/utf8proc/OpenSSL are private, no CMake path was introduced, and Python remains a pinned cold parity surface. |
| 70% coverage ratchet | PASS | All five included packages have fresh non-empty per-package line coverage above 70%; the manifest retains the prior tool packages. |

## Negative and edge cases

| ID | Result | Judgment |
|---|---|---|
| N1 | PASS | Canonical BOM/order/duplicate/UTF-8/NFC/number/escape/depth/byte/LF cases execute and reject. |
| N2 | PASS | NFC, invalid Unicode, escaping, private dependency containment, and fixed cross-language bytes/digests execute. |
| N3 | **FAIL** | Package and subject mutations execute, but semantic self/current-scan references can pass under neutral paths. |
| N4 | **FAIL** | IDs/nine outcomes/injected exact-over seams execute; ordinary validator calls do not observe real RSS/time. |
| N5 | **FAIL** | Literal valid/dirty/missing values are tested, but no real Bazel dev/release fact source is exercised. |
| N6 | **FAIL** | Cardinalities execute; semantic current-scan/self relationships and referenced bytes do not. |
| N7 | **FAIL** | Fixed documents and isolated integer primitives execute; raw array order/digest/statistics and workload conditionals are not integrated. |
| N8 | **FAIL** | Success mutations execute; non-success nested fields and aggregate referenced-run reconciliation do not. |
| N9 | **FAIL** | `CheckResourceUsage` exact/first-over seams execute, but production measurement, timely enforcement, and the exact limit inventory are incomplete. |
| N10 | **FAIL** | Real fuzz execution is positive, but the required empty/zero/crash/timeout/OOM/sanitizer/corpus-omission simulations are absent and fuzz is not UBSan-instrumented. |
| N11 | **FAIL** | Exact-70/below/missing/empty/omission/exclusion cases execute and c-05 is fresh, but the gate has no source-revision/freshness check and the required stale-report negative simulation is absent. |
| N12 | PASS | Public consumer compiles against Orus types; third-party/native names are absent; V7 finds no CMake, alternate path, or prohibited future capability claim. |

## Coverage gate

The exact registered command regenerated LCOV before invoking the gate and
passed. The judged package results are:

| Package | Line coverage | Threshold |
|---|---:|---:|
| `contracts` | 85.34% | 70% |
| `python/orus_contracts` | 85.59% | 70% |
| `tools` | 79.41% | 70% |
| `tools/build` | 75.32% | 70% |
| `tools/coverage` | 74.04% | 70% |

Coverage is not the blocker. The failing contract behavior occurs in covered
code whose tests assert weaker behavior than the verification plan requires.

## Sign-off checklist

| Item | Result | Reason |
|---|---|---|
| Latest V1-V5 execution is green/current/complete | PASS | `run-20260722T155248Z` covers c-01 through c-05 and all registered targets execute. |
| V6 and V7 addressed explicitly | PASS | V6 is FAIL with source evidence; V7 is PASS with separate inventories. |
| TC-01 through TC-10 and all ledger criteria pass | **FAIL** | TC-03 through TC-09 and TC-10's negative-evidence portion fail. |
| N1-N12 actually exercised with literal outcomes | **FAIL** | N3-N11 contain missing or weaker evidence as detailed above. |
| C++ authority and Python parity | PASS | Authority boundary and fixed parity goldens are present. |
| Public API hides dependencies and exact contracts match | **FAIL** | Dependency hiding passes; exact schemas/resources/relationships do not. |
| Build facts/reference truthfulness | **FAIL** | Reference outcomes are truthful, but authoritative build fact binding and real resource guards are missing. |
| Digest subjects/package behavior exact | **FAIL** | Core subject distinction passes; self/current-scan and production package resource behavior do not. |
| Downstream workflows remain outside scope; no future claim | PASS | Ownership boundary is preserved. |
| Every package has fresh coverage >=70% | PASS | All five packages exceed the threshold. |
| Task scope and metadata lifecycle are clean | PASS | c-07 inventories all 52 changed paths and exempts audit/history correctly. |
| Different author/verifier stages, exact source, no pin drift/Andon | PASS | `09_implementer/r2` and `10_verifier/r1` are separate stages; tree/range reconcile; no lock/pin changed and no healing link exists. |

## Required rework

- Bind build facts to declared Bazel-generated/status inputs and test real
  dev/release outputs.
- Add production RSS and monotonic-deadline observation/enforcement to every
  applicable validator and package walk, with checks during long operations,
  while retaining deterministic injection seams.
- Introduce relationship APIs/resolvers that validate workload + raw samples +
  result together and aggregate + referenced corpus runs together; recompute
  byte lengths, digests, ordering, statistics, counts, and contextual fields.
- Validate all corpus nested fields for success and failure reports; use the
  exact per-schema byte/count/depth limits and checked aggregate arithmetic.
- Replace path-substring cycle detection with exact schema/type/identity DAG
  rules and correct the shared resource-row literals.
- Add the required N10/N11 negative verifier fixtures and fuzz-time UBSan
  evidence, then rerun the complete registered suite through the Factory.

Until all required checks pass, `M0-002` must remain `BLOCKED`.
