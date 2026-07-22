# M0-002 Verification Report r1

**Verdict:** BLOCKED — required manual check c-06 fails  
**Factory task:** `M0-002`  
**Verifier:** Factory step `10_verifier`, re-verification attempt `r1`  
**Author handoff:** `.agents/M0-002/handoff-r1.md`  
**Judged check run:** `run-20260722T172410Z` (GREEN)  
**Implementation base:** `bdd18d1739f4d3e3e13a81c46fd5cd7e00cb1c9a`  
**Initial implementation commit:** `43cb86260538dfefc3929ec384016ade901fbc37`  
**Prior verifier/result commit:** `fdf2f4ca13d87cc0e46b58083df126df70e5cb51`  
**Audited rework/source commit:** `2fef167f918209240ce2877ac939501b5527fb14`

## Decision

The latest registered run is complete and green, all five automated checks
pass, the 70% per-package coverage threshold is met, and manual scope check
c-07 passes. Acceptance is nevertheless blocked because manual frozen-contract
check c-06 finds required behavior that the automated tests do not cover and
that the implementation does not satisfy.

This is an implementation and negative-test failure, not a specification
defect. The A3, verification plan, and specs state the required subject
binding, conditional performance relationships, typed error/resource-row
surface, and source-consistent coverage behavior precisely. No healing task is
declared or pending.

The blocking findings are:

1. **Release-evidence references are not resolved or subject-bound.**
   `ValidateGovernanceDocument` accepts only manifest bytes and an optional
   resource measurement (`contracts/include/orus/contracts/contracts.h:237-239`).
   Its evidence loop checks the spelling of `path`, `schema`, `byte_length`, and
   a 64-hex digest but receives no referenced documents and does not recompute
   byte length, digest, schema, or subject from actual bytes
   (`contracts/evidence_contracts.cc:540-569`). The positive test deliberately
   accepts twelve synthetic rows with `byte_length=2`, repeated one-character
   hex digests, and the generic `M0-EVIDENCE-v1` schema without supplying the
   referenced objects (`tests/contracts/evidence_test.cc:82-127`). This fails
   TC-06, N3, N6, GOV-FR-008, GOV-NFR-004, SEC-FR-005, and SEC-NFR-003.

2. **The performance-comparison validator does not implement the exact
   conditional/state relationships.** The spec requires a valid advisory input
   to produce `advisory_only/PERF_ADVISORY_INPUT` with informational statistics
   (`specs/14-performance-foundation.md:382-389`). The validator instead permits
   statistical fields only for `no_regression_detected` or
   `regression_requires_approval`, so it rejects the required advisory form and
   accepts `advisory_only` with null statistics
   (`contracts/evidence_contracts.cc:1023-1051`). It also does not bind
   `authority` to the terminal state, `noise_state=failed` to
   `inconclusive/PERF_NOISE_POLICY_FAILED`, or regression/no-regression state to
   the two threshold flags. The only comparison mutation in the registered
   evidence suite empties an incomparable document's mismatch list
   (`tests/contracts/evidence_test.cc:323-327`); the required conditional-field
   and relationship matrix is absent. This fails TC-07, N7, PERF-FR-003, and
   PERF-NFR-003.

3. **The frozen governance error and security resource-row surfaces are not
   literal.** The public `Error` type exposes `document_schema` and has no
   `contract` or `record_id` (`contracts/include/orus/contracts/contracts.h:20-31`),
   while `M0-GOV-ERROR-v1` requires exact `contract` and `record_id` fields
   (`specs/11-governance-release.md:290-306`). Governance RSS errors report
   expected resource `rss_bytes` (`contracts/crypto_resource.cc:63-75`) rather
   than the required `peak_rss_bytes`. `ResourceContractRow` contains only ID,
   operation, numeric limits, error, and owner
   (`contracts/include/orus/contracts/contracts.h:262-269`); it cannot carry the
   required input trust/status, enforcement point, cleanup, tests, or
   not-applicable rationale defined by `M0-RESOURCE-LIMIT-v1`
   (`specs/16-security-foundations.md:151-165`). The test checks row uniqueness,
   numeric first-over behavior, and nonempty error/owner only
   (`tests/contracts/evidence_test.cc:516-575`). This fails TC-09, N9, and the
   M0-002 shared-core slice of SEC-FR-007.

4. **The coverage freshness negative is mtime-only, not source-revision
   consistent.** `validate_freshness` rejects sources newer than the LCOV file,
   then returns a digest of the current source tree; it never proves that the
   LCOV report was generated from that digest (`tools/coverage/package_gate.py:128-143`).
   The negative test only moves the report mtime before or after the source
   (`tests/build/contract_test.py:339-353`). A stale LCOV file copied or touched
   after a source change passes. The current c-05 report was freshly generated
   and its numeric gate is valid, but N11 and the verification plan's
   `source-revision-consistent` pass rule require this stale-report class to
   fail. This fails TC-10's negative-proof requirement, N11, and the 70%
   coverage acceptance item.

5. **The rework handoff is not a complete frozen-boundary inventory.** It
   records the rework base and an older green check run, but not the exact
   source commit `2fef167f...`; it claims the prior relationship/limit and N11
   blockers are fixed without disclosing the limitations above
   (`.agents/M0-002/handoff-r1.md:3-9,24-44,48-57`). It also lists changed paths,
   not a complete frozen schema/error/limit/golden inventory. This fails the
   final V6 handoff-inventory bullet. The commit range remains independently
   auditable, so it does not turn c-07 into a scope failure.

## Registered machine checks

The commands were not re-executed individually. This report judges the factory
record and logs for `run-20260722T172410Z`.

| Check | Result | Evidence and judgment |
|---|---|---|
| c-01 / V1 dev + release | PASS | Exit 0 in 170.9 s. The retained logs execute all six `//tests/contracts/...` targets and the release `//tests/contracts:build_test`; no target is skipped. Evidence: `.factory/checks/M0-002/runs/run-20260722T172410Z.json`, `c-01.stdout.log`, `c-01.stderr.log`. |
| c-02 / V2 ASan | PASS | Exit 0 in 79.5 s; all six contract targets pass under AddressSanitizer with leak detection and no finding. Evidence: `c-02.stdout.log`, `c-02.stderr.log`. |
| c-03 / V3 UBSan | PASS | Exit 0 in 73.9 s; all six contract targets pass under UndefinedBehaviorSanitizer with no finding. Evidence: `c-03.stdout.log`, `c-03.stderr.log`. |
| c-04 / V4 fuzz | PASS | Exit 0 in 37.6 s. The native target is built with libFuzzer, ASan, and UBSan; its Bazel target supplies `-runs=256` and the checked-in five-file corpus, and 1/1 target passes. Evidence: `tests/fuzz/BUILD.bazel:6-24`, `.bazelrc`, `c-04.stdout.log`, `c-04.stderr.log`. |
| c-05 / V5 coverage | PASS | Exit 0 in 113.5 s. Nine tests pass and every package exceeds the exact 70% threshold. Evidence: `c-05.stdout.log`, `c-05.stderr.log`. |

Coverage values from the authoritative c-05 log:

| Package | Line coverage | Threshold | Result |
|---|---:|---:|---|
| `contracts` | 85.1186% | 70% | PASS |
| `python/orus_contracts` | 85.5932% | 70% | PASS |
| `tools` | 79.4118% | 70% | PASS |
| `tools/build` | 75.3213% | 70% | PASS |
| `tools/coverage` | 75.0000% | 70% | PASS |

The report emitted source snapshot
`cfd7ad8e4f9b0fc7da7d6795c7f49ce49e1ff927b7998e6efab834661a1653cd`.
That value describes current sources but, per finding 4, is not bound into the
LCOV producer output.

## c-06 — Manual frozen-contract, API, and ownership review: FAIL

| Required V6 item | Result | Evidence |
|---|---|---|
| Public surface is Orus-owned and hides Glaze/utf8proc/OpenSSL/native layout | PASS | `contracts/include/orus/contracts/contracts.h` exposes only standard-library and Orus-owned types. `tests/contracts/public_header_test.cc` and registered c-01/c-02/c-03 pass. |
| Strict prevalidation and exact canonical/Unicode/integer/escape/member/terminal-byte rules | PASS | `contracts/canonical_json.cc`, `tests/contracts/canonical_test.cc`, and `tests/contracts/python_parity_test.py`; c-01 through c-03 pass. |
| C++ authority, pinned cold Python parity, and shared goldens | PASS | `python/orus_contracts/canonical.py` is the parity surface; native/untrusted parsing remains in C++; `//tests/contracts:python_parity_test` passes in c-01 through c-03. |
| Build facts, reference validation, identities, schemas, typed errors, and resource bounds match the specs literally | FAIL | Governance referenced bytes are not resolved; performance terminal relationships are incomplete; governance error fields/resource literals and resource-row fields drift. Findings 1-3. |
| Downstream producer/workflow ownership boundaries remain intact | PASS | No CLI, governance producer/gate, performance harness/comparator, corpus runtime, CI workflow/provider, or security reconciler was added. A3 ownership for M0-003 through M0-009 remains intact. |
| Handoff inventories frozen API/schema/error/limit/goldens, exact source revision, and limitations without overclaim | FAIL | `.agents/M0-002/handoff-r1.md` omits the exact source commit and does not disclose findings 1-4 or provide the complete claimed inventory. |

Overall c-06 is **FAIL**.

## c-07 — Manual task-scope and evidence-lifecycle audit: PASS

The exact audited implementation range is
`bdd18d1739f4d3e3e13a81c46fd5cd7e00cb1c9a..2fef167f918209240ce2877ac939501b5527fb14`.
The rework-only subrange is
`fdf2f4ca13d87cc0e46b58083df126df70e5cb51..2fef167f918209240ce2877ac939501b5527fb14`.
This adapts V7's handoff range to the latest implementation/rework commit at
HEAD and audits all 69 paths, rather than treating the intervening verifier and
factory-result commit as product implementation.

### Task-owned implementation: 48 paths

- Build/config integration (7): `.bazelrc`, `MODULE.bazel`,
  `config/BUILD.bazel`, `flake.nix`, `tools/build/build_facts.bzl`,
  `tools/build/nix_repositories.bzl`, `tools/build/workspace_status.sh`.
- C++ contracts (7): `contracts/BUILD.bazel`,
  `contracts/build_contracts.cc`, `contracts/canonical_json.cc`,
  `contracts/crypto_resource.cc`, `contracts/evidence_contracts.cc`,
  `contracts/include/orus/contracts/contracts.h`,
  `contracts/resource_monitor.h`.
- Python parity (3): `python/orus_contracts/BUILD.bazel`,
  `python/orus_contracts/__init__.py`, `python/orus_contracts/canonical.py`.
- Build/coverage support (4): `tests/build/BUILD.bazel`,
  `tests/build/contract_test.py`, `tools/coverage/package_gate.py`,
  `tools/coverage/packages.json`.
- Contract tests (8): `tests/contracts/BUILD.bazel`,
  `tests/contracts/boundary_test.py`, `tests/contracts/build_test.cc`,
  `tests/contracts/canonical_test.cc`, `tests/contracts/evidence_test.cc`,
  `tests/contracts/public_header_test.cc`,
  `tests/contracts/python_parity_test.py`, `tests/contracts/test_support.h`.
- Golden fixtures (11): `tests/contracts/fixtures/corpus-reliability.json`,
  `corpus-run.json`, `perf-comparison.json`, `perf-raw-sample.json`,
  `perf-result.json`, `perf-runner.json`, `perf-workload.json`,
  `reference-fixed.json`, `reference-observed-fixed.json`,
  `release-evidence-assembled.json`, and `sbom-descriptor.json` under the same
  fixture directory.
- Fuzz target/corpus (8): `tests/fuzz/BUILD.bazel`,
  `tests/fuzz/canonical_json_parser_fuzz.cc`,
  `tests/fuzz/fuzz_smoke_verifier.py`, and the five files under
  `tests/fuzz/corpus/canonical_json/` (`invalid-duplicate`, `invalid-number`,
  `invalid-resource-depth`, `valid-empty-object`, `valid-nested`).

These paths are within A3 scope. `MODULE.bazel`, `flake.nix`,
`tools/build/nix_repositories.bzl`, and `.bazelrc` expose the already pinned Nix
OpenSSL/utf8proc/LLVM tools and register real targets/status; no coordinate,
digest, lock, compiler, acquisition profile, network policy, or alternate build
path changes. No CMake path was added.

### Factory/audit metadata and historical evidence: 21 paths

- `.agents/M0-002/handoff.md`, `.agents/M0-002/handoff-r1.md`,
  `.agents/M0-002/verification.md`,
  `.agents/M0-002/verification-report.md`.
- `.factory/checks/M0-002/c-01.json` and the six retained run records
  `run-20260722T145320Z.json`, `run-20260722T154456Z.json`,
  `run-20260722T155248Z.json`, `run-20260722T162639Z.json`,
  `run-20260722T163837Z.json`, `run-20260722T171208Z.json`.
- `.factory/results/09_implementer/r2/result.json`,
  `.factory/results/10_verifier/r1/result.json`,
  `.factory/results/11_rework/r1/result.json`,
  `.factory/results/11_rework/r3/result.json`, and the three corresponding
  append-only files under `.factory/results/_archive/` dated
  `20260722T155248Z`, `20260722T160653Z`, and `20260722T172410Z`.
- `.factory/run.json`, `.factory/tasks/M0-002.json`, and the CLI-generated
  `IMPLEMENTATION_PLAN.md` task row.

The current `run-20260722T172410Z.json` and its `.factory/logs/**` are newer
factory-owned evidence outside the audited source commit. They are judged as
machine evidence, not product source. All `.agents/**` reports/handoffs and
`.factory/**` evidence are append-only/historical or step-boundary metadata and
are exempt from product-source volatile/scope guards. No `.agents/M0-001/**`
path changed. No unexplained product path exists.

Overall c-07 is **PASS**.

The substrate ledger still lists only `github-actions=substrate`. M0-002's
contract/unit/fuzz/coverage checks did not claim or exercise that real external
boundary, so no substrate transition is warranted.

## Target-condition results

| Target condition | Result | Reason |
|---|---|---|
| TC-01 | PASS | Strict canonical C++ boundary and typed parse rejection are covered by c-01 through c-04 and source review. |
| TC-02 | PASS | Orus-owned NFC/SHA interfaces and C++/Python byte parity pass. |
| TC-03 | PASS | Bazel-generated five-field build facts cover dev/release and explicit dirty rejection. |
| TC-04 | PASS | Fixed/production reference IDs, nine outcomes, supported-contract posture, and resource boundaries pass registered tests. |
| TC-05 | FAIL | Local content/package identities pass, but the target condition includes SBOM/evidence subject non-interchangeability; governance references are accepted without referenced bytes or subject verification. |
| TC-06 | FAIL | Release evidence has finite 12/12/3/cardinality and cycle syntax checks, but no referenced-byte/canonical subject binding. |
| TC-07 | FAIL | Performance comparison conditional/state relationships are incomplete and the required negative matrix is absent. |
| TC-08 | PASS | Reworked corpus run/reliability resolvers cover nested types, referenced run digests/counts, fault mapping, aggregates, and resource bounds in the registered suite. |
| TC-09 | FAIL | Reusable rows cannot represent enforcement point, cleanup, tests, trust/status, or N/A rationale; governance error literals also drift. |
| TC-10 | FAIL | Machine configurations and numeric package coverage pass, but the mandatory stale/source-consistency negative can be bypassed by touching stale LCOV. |

## Acceptance-criterion results

| Acceptance criterion | Result | Evidence/reason |
|---|---|---|
| BUILD-FR-009 | PASS | Generated dev/release build-fact provider and dirty/missing fact negatives pass. |
| BUILD-FR-010 | PASS | Canonical reference bytes/ID, nine outcomes, mismatch/content/resource cases pass. |
| BUILD-FR-011 | PASS | Subject/package identity and bounded mutation matrix pass. |
| BUILD-NFR-004 | PASS | Exactly one reference environment is presented as validated; mismatch/broad-support negatives pass. |
| GOV-FR-006 shared-core slice | FAIL | Descriptor syntax is validated, but the completed SBOM bytes and descriptor digest cannot be resolved through the public governance validator. |
| GOV-FR-008 shared-core slice | FAIL | 12/12/3 structure passes; path/length/schema/digest and subject references are not resolved. |
| GOV-NFR-004 | FAIL | One-byte/cross-subject referenced-evidence mutations cannot be detected without actual referenced bytes. |
| GOV-NFR-006 shared-core slice | PASS | Canonical primitives and fixed deterministic bytes are present; actual producer reproducibility remains M0-004-owned. |
| PERF-FR-003 shared-core slice | FAIL | The comparison schema does not enforce exact authority/noise/state/statistics relationships. |
| PERF-FR-012 shared-core slice | PASS | Parser byte/depth/count/arithmetic/work/RSS/time guards and fuzz coverage pass. |
| PERF-NFR-003 | FAIL | Raw-result bundle reconciliation exists, but comparison provenance/conditional relationships remain forgeable. |
| PERF-NFR-005 shared-core slice | PASS | Corrected 16 MiB, 100,000 pair, 10,000 work, 256 MiB, and 120 s bounds pass. |
| CORP-FR-013 shared-core slice | PASS | Canonical run/reliability documents and referenced aggregate relationships pass reworked tests. |
| SEC-FR-005 | FAIL | Local typed identities pass; governance can accept arbitrary syntactically valid evidence digests without referenced-byte subject validation. |
| SEC-FR-007 shared-core slice | FAIL | Numeric rows pass, but the required reusable enforcement/error/cleanup/test row surface is incomplete and governance resource diagnostic literal drifts. |
| SEC-NFR-003 | FAIL | The required 100% subject-reference mutation/substitution detection is not established for governance evidence. |
| q-0012/q-0014 and dependency policy | PASS | Pinned Glaze/utf8proc/OpenSSL remain private behind C++; Python is cold parity; no pin/CMake/ownership drift. |
| 70% contract-branch coverage ratchet | FAIL | The authoritative numeric report passes every package threshold, but N11's stale/source-revision consistency gate is not fail-closed. |

## Negative and edge results

| ID | Result | Evidence/reason |
|---|---|---|
| N1 | PASS | Canonical malformed/byte/depth/integer/terminal cases execute under c-01 through c-04. |
| N2 | PASS | NFC/scalar/escape/permissive-parser and C++/Python parity cases pass. |
| N3 | FAIL | Package/content mutation coverage passes, but governance subject substitution can use an arbitrary valid-looking digest because referenced bytes are unavailable. |
| N4 | PASS | Fixed/production reference, nine outcomes, fault and exact/first-over resources pass. |
| N5 | PASS | Generated dev/release/dirty/missing/wrong build-fact cases pass. |
| N6 | FAIL | Cardinality and forbidden schema cycles pass; canonical referenced-byte identity and subject substitution are not exercised or enforceable. |
| N7 | FAIL | Fixed performance examples and many result/raw/resource mutations pass, but advisory/authority/noise/threshold comparison relationships are neither correctly enforced nor negatively tested. |
| N8 | PASS | Run/reliability nested, fault, aggregate, forged-success, and bound mutations execute in c-01 through c-04. |
| N9 | FAIL | Numeric exact/first-over checks pass; missing field, late-enforcement, cleanup, and literal governance error-row mutations cannot be expressed by the implemented resource-row type. |
| N10 | PASS | Real V4 runs 256 inputs over the five checked-in seeds with ASan+UBSan; the V1 suite covers empty/omitted/zero/crash/timeout/OOM/sanitizer simulation rejection. |
| N11 | FAIL | Exact-70/below/missing/empty/omission/ownership/exclusion cases pass, but a stale report touched newer than changed source is accepted. |
| N12 | PASS | Public-header isolation, no CMake/alternate path, scope review, and no prohibited capability/support claim pass. |

## Sign-off checklist

| Item | Result | Reason |
|---|---|---|
| Latest V1-V5 run is green/current/complete | PASS | `run-20260722T172410Z` covers c-01 through c-05 and is GREEN. |
| V6 and V7 explicitly addressed | PASS | c-06 is explicitly FAIL; c-07 explicitly PASS with complete path buckets. |
| TC-01 through TC-10 and every cited AC pass | FAIL | TC-05/-06/-07/-09/-10 and related ACs fail above. |
| N1-N12 produce literal outcomes | FAIL | N3/N6/N7/N9/N11 are incomplete or permit forbidden acceptance. |
| C++ authority and Python parity | PASS | Public boundary and parity tests pass. |
| Public API hides dependencies and exact byte/error/identity/resource contracts match | FAIL | Dependencies are hidden, but findings 1-3 show error/identity/resource mismatch. |
| Build facts/reference validation truthful | PASS | Generated provider and exact reference negatives pass. |
| Digest subjects cannot substitute/self-reference | FAIL | Governance evidence references can be syntactically substituted without actual bytes. |
| Downstream ownership intact and no future claim | PASS | c-06 ownership review and c-07 scope audit pass. |
| Fresh package coverage >=70% with all negatives | FAIL | Current generated report exceeds 70%; stale/source-revision consistency is not fail-closed. |
| Task scope and metadata separation | PASS | c-07 classifies all 69 committed-range paths and exempts historical evidence. |
| Independent identities/exact revision/no pin drift/no Andon | PASS | Implementer/rework and verifier steps are distinct; exact source is recorded here; no pin drift or open question exists. |

## Required rework

1. Add an Orus-owned governance bundle resolver that accepts the referenced
   evidence/SBOM documents and verifies canonical bytes, exact schema, length,
   lowercase SHA-256, subject identity, source/package/SBOM cross-links, and
   self/substitution mutations. Retain a negative fixture for each mutation.
2. Enforce every performance-comparison authority/noise/state/action/statistics/
   threshold relationship literally, including advisory informational
   statistics and noise-failure precedence, and add one negative per
   conditional/relationship required by N7.
3. Freeze exact domain error objects and expand `ResourceContractRow` (or an
   equivalent Orus-owned type) to carry all M0-002-owned
   `M0-RESOURCE-LIMIT-v1` fields. Use the exact governance
   `peak_rss_bytes` diagnostic literal and test missing/drift/late-enforcement/
   cleanup mutations.
4. Bind LCOV provenance to the exact source snapshot/revision rather than file
   mtimes. Add a negative where stale LCOV is copied/touched after a source
   change and must still fail.
5. Produce a new append-only handoff that records the exact implementation
   commit/check run and a complete frozen API/schema/error/limit/golden and
   known-limitation inventory.

Until the full registered suite is green again and c-06 passes on these items,
M0-002 must remain `BLOCKED`.
