# M0-002 Verification Report r3

**Task:** `M0-002`  
**Verifier:** Factory step `10_verifier`, independent re-verification attempt `r3`  
**Latest handoff:** `.agents/M0-002/handoff-r3.md`  
**Judged check run:** `run-20260722T195725Z` (GREEN)  
**Implementation base:** `bdd18d1739f4d3e3e13a81c46fd5cd7e00cb1c9a`  
**Latest rework base:** `44cbe985bc9d111ceed2d5edbd957b8dfda84574`  
**Exact verified product revision:** `e79a331bbcfd959d23cb46626760a87beb832068`  
**Exact verified product source snapshot SHA-256:** `6254a2434aaaa4cfd8dfefede9f42be084bd6a2aa173a4215d704a456c083391`

## Decision

**DONE — all required checks pass.** The authoritative registered run is
complete and green, both manual checks pass, no healing task or temporary mock
is declared, and no unresolved Andon remains. The prior approval-timestamp
blocker is fixed in production code and covered through the public governance
validator with literal typed-error assertions.

The registered machine commands were not re-executed individually. This report
judges the factory record and retained logs for `run-20260722T195725Z`; its
source-bound coverage provenance identifies revision `e79a331...` and the same
product snapshot recorded by the latest handoff. No stale or flaky indication
warranted a whole-suite rerun.

## Registered machine checks

| Check | Result | Evidence and judgment |
|---|---|---|
| c-01 / V1 dev + release | PASS | Exit 0 in 255.7 s. Dev executes all 6/6 `//tests/contracts/...` targets and release executes `//tests/contracts:build_test` 1/1. Evidence: `.factory/checks/M0-002/runs/run-20260722T195725Z.json`, `c-01.stdout.log`, and `c-01.stderr.log` under the corresponding retained log directory. |
| c-02 / V2 ASan | PASS | Exit 0 in 109.4 s. All 6/6 contract targets execute with AddressSanitizer and leak detection; no sanitizer or leak finding is reported. Evidence: `c-02.stdout.log`, `c-02.stderr.log`. |
| c-03 / V3 UBSan | PASS | Exit 0 in 106.6 s. All 6/6 contract targets execute with non-recovering UndefinedBehaviorSanitizer; no undefined-behavior finding is reported. Evidence: `c-03.stdout.log`, `c-03.stderr.log`. |
| c-04 / V4 native fuzz smoke | PASS | Exit 0 in 46.4 s. The native libFuzzer+ASan+UBSan target executes 1/1. `tests/fuzz/BUILD.bazel` fixes 256 runs and supplies all five checked-in valid/malformed corpus entries; `boundary_test.py` fails zero-execution, empty/omitted corpus, crash, timeout, OOM, ASan, and UBSan simulations. Evidence: `c-04.stdout.log`, `c-04.stderr.log`, `tests/fuzz/BUILD.bazel`, and `tests/contracts/boundary_test.py`. |
| c-05 / V5 coverage | PASS | Exit 0 in 129.9 s. The producer executes 9/9 tests, emits current non-empty LCOV, and verifies revision/source/manifest/report provenance before applying the exact per-package 70% threshold. Evidence: `c-05.stdout.log`, `c-05.stderr.log`, and `bazel-out/_coverage/orus-source-provenance.json`. |

Authoritative c-05 package results:

| Package | Line coverage | Threshold | Result |
|---|---:|---:|---|
| `contracts` | 85.824992% | 70% | PASS |
| `python/orus_contracts` | 85.593220% | 70% | PASS |
| `tools` | 79.411765% | 70% | PASS |
| `tools/build` | 75.321337% | 70% | PASS |
| `tools/coverage` | 71.038251% | 70% | PASS |

The c-05 sidecar binds source revision
`e79a331bbcfd959d23cb46626760a87beb832068`, source snapshot
`6254a2434aaaa4cfd8dfefede9f42be084bd6a2aa173a4215d704a456c083391`,
package-manifest digest `acd0a4e88de16616c3f321eeac4ef33d7afac1fa2cb9414f954eb7ec5c2be28d`,
and LCOV digest `6c3fd27bbc7dfa1a1453dbbf60cd6336300a38211b85a609977fa6fa1a6eb9aa`.

## c-06 — Manual frozen-contract, API, and ownership review: PASS

| Required V6 item | Result | Evidence |
|---|---|---|
| Public surface uses Orus-owned types and hides Glaze, utf8proc, OpenSSL, and native layouts | PASS | `contracts/include/orus/contracts/contracts.h` includes only standard-library headers and exposes Orus-owned value/error/stream types. Third-party headers and handles remain private in `canonical_json.cc` and `crypto_resource.cc`. `public_header_test` passes in dev, ASan, and UBSan. |
| Strict byte prevalidation and canonical rules precede parse/identity | PASS | `ParseCanonicalJson` performs size/BOM checks, the Orus parser, NFC/integer/escape/name/order/duplicate/depth validation, exact re-emission, and byte comparison before the private `glz::read_json` defense. `canonical_test.cc`, `boundary_test.py`, and the five-entry native corpus cover BOM, whitespace/order, duplicate names, invalid/non-NFC Unicode, surrogate/escape defects, float/exponent/non-shortest/overflow integers, excess bounds, and terminal bytes. |
| C++ authority, pinned cold Python parity, and shared goldens | PASS | Native and untrusted parsing enters the C++ API. `python/orus_contracts/canonical.py` explicitly limits itself to a cold parity oracle, and `python_parity_test.py` proves canonical byte, SHA-256, statistics, and rejection parity over the shared fixture corpus without normalizing rejected untrusted bytes into acceptance. |
| Build facts, reference validation, identities, schemas, errors, and limits match the specs literally | PASS | `build_contracts.cc` and `evidence_contracts.cc` implement the exact build/reference/content, governance, five performance, two corpus, and seven shared resource rows described by specs `10`, `11`, `14`, `15`, and `16`. The previous gap is closed by `IsRfc3339Seconds`: Gregorian month lengths, leap years, clock bounds, and second 60 only at `23:59` on a month's last day. `ApprovalTimesEnforceUtcRfc3339CalendarAndLeapSeconds` accepts 2000/2024 leap dates and the valid leap-second form, rejects impossible dates/clock values/placement, and asserts schema `M0-GOV-ERROR-v1`, code `GOV_RELATIONSHIP_INVALID`, contract `M0-RELEASE-EVIDENCE-v1`, path `$.approvals[]`, and the release record ID. Exact resource rows and their enforcement/error/cleanup/test ownership are at `M0SharedResourceContracts`; the green suite checks exact-bound, first-over, overflow, and drift cases. |
| Downstream ownership boundaries remain intact | PASS | The implementation provides shared canonical types, validators, byte/subject relationships, and resource rows only. It adds no SPDX producer/license reconciliation/release gate or marker (M0-004), performance workload runner/comparator decision producer (M0-005), concurrent process corpus (M0-006), live CI provider behavior (M0-007/M0-009), or complete security inventory/secret-scan orchestration (M0-008). No broader platform, signature, sandbox, deterministic-record/replay, or M1+ claim appears. |
| Handoff inventory is complete, source-exact, and bounded in its claims | PASS | The append-only handoff chain is the inventory: `handoff-r2.md` lists the complete public API, schema/error/limit matrix, golden corpus, changed paths, ownership limits, and known limitations; `handoff-r3.md` records the RFC3339-only delta and product snapshot. The newer authoritative c-05 provenance binds that snapshot to its committed wrapper revision `e79a331...`, resolving the normal step-boundary commit created after handoff text was written. The handoff makes no downstream workflow or live-provider claim. |

Overall c-06 is **PASS**.

## c-07 — Manual task-scope and evidence-lifecycle audit: PASS

The complete committed task lifecycle range audited is
`bdd18d1739f4d3e3e13a81c46fd5cd7e00cb1c9a..e79a331bbcfd959d23cb46626760a87beb832068`.
The latest rework subrange is
`44cbe985bc9d111ceed2d5edbd957b8dfda84574..e79a331bbcfd959d23cb46626760a87beb832068`.
The latter contains exactly the two task-owned RFC3339 paths plus the handoff
and factory step-boundary metadata. This adapts the audit to the implementation
commit instead of the current verifier/result mutations.

### Task-owned implementation: 48 paths

- Build/config integration (7): `.bazelrc`, `MODULE.bazel`,
  `config/BUILD.bazel`, `flake.nix`, `tools/build/build_facts.bzl`,
  `tools/build/nix_repositories.bzl`, and
  `tools/build/workspace_status.sh`.
- C++ contract surface (7): `contracts/BUILD.bazel`, the four contract `.cc`
  files, `contracts/include/orus/contracts/contracts.h`, and
  `contracts/resource_monitor.h`.
- Python parity (3): the three files under `python/orus_contracts/`.
- Build/coverage support (4): `tests/build/BUILD.bazel`,
  `tests/build/contract_test.py`, `tools/coverage/package_gate.py`, and
  `tools/coverage/packages.json`.
- Contract tests (8): `tests/contracts/BUILD.bazel`, `boundary_test.py`,
  `build_test.cc`, `canonical_test.cc`, `evidence_test.cc`,
  `public_header_test.cc`, `python_parity_test.py`, and `test_support.h`.
- Golden fixtures (11): the two corpus, five performance, two reference,
  release-evidence, and SBOM-descriptor fixtures under
  `tests/contracts/fixtures/`.
- Fuzz target/corpus (8): `tests/fuzz/BUILD.bazel`, the native harness, the
  smoke verifier, and the five files under
  `tests/fuzz/corpus/canonical_json/`.

All 48 paths are within the A3 shared-core, test, fuzz, fixture, Bazel
registration, or backward-compatible coverage scope. Root/build changes expose
the already pinned OpenSSL/utf8proc/LLVM/build-fact inputs to Bazel. No
dependency coordinate, immutable digest, lockfile, compiler selection,
network/acquisition policy, CMake path, or other alternate build path changed.

### Factory/audit metadata and append-only history: 35 committed paths

- `.agents/M0-002/` (8): four handoffs, three prior verification reports,
  and `verification.md`.
- `.factory/checks/M0-002/` (13): two registered check definitions and eleven
  retained run records through `run-20260722T190809Z`.
- `.factory/results/` (11): four active step-result paths and seven archived
  append-only result records.
- Lifecycle metadata (3): `.factory/run.json`,
  `.factory/tasks/M0-002.json`, and the CLI-generated task row in
  `IMPLEMENTATION_PLAN.md`.

The judged `run-20260722T195725Z` record/logs, current task-status mutation,
and current verifier result lifecycle are newer factory-owned evidence. They
are reported separately and are not product-source changes. All
`.agents/**`/`.factory/**` historical handoffs, reports, logs, results, and
prior M0-001 evidence are append-only records exempt from product-source and
volatile-path guards. No historical evidence file is consumed as product
input, and no unexplained implementation path exists.

Overall c-07 is **PASS**.

## Target-condition results

| Target condition | Result | Evidence/reason |
|---|---|---|
| TC-01 | PASS | The strict canonical C++ boundary rejects forbidden forms before private Glaze/identity and emits exact no-LF canonical bytes with typed path/offset/limit errors. |
| TC-02 | PASS | Orus-owned NFC/SHA adapters hide utf8proc/OpenSSL, and C++/Python canonical bytes, digests, Unicode cases, and statistics agree. |
| TC-03 | PASS | Bazel-generated dev/release build facts contain the exact five non-empty fields; dirty, missing, empty, and wrong release metadata fail closed. |
| TC-04 | PASS | Fixed/production reference IDs, exactly nine outcomes, one supported contract, mismatch/unavailable behavior, and 64-MiB/10-s exact/first-over handling pass. |
| TC-05 | PASS | Executable/package/SBOM/evidence subjects are non-interchangeable; bounded package walk, fixed digest, metadata matrix, mutation, special/link, and first-over cases pass. |
| TC-06 | PASS | Governance descriptor/manifest bytes, referenced objects, source/package/SBOM subjects, exact 12/12/3 inventories, acyclic references, complete UTC RFC3339 approval timestamps, and literal governance errors pass while production/gate workflows remain deferred. |
| TC-07 | PASS | All five performance schemas/goldens, raw/result relationships, integer statistics, comparison conditional state, overflow, and resource negatives pass; execution/decision production stays M0-005-owned. |
| TC-08 | PASS | Both corpus schemas/examples plus topology, ownership, fault, digest, aggregate-count, resource, and forged-success negatives pass; runtime corpus ownership stays M0-006. |
| TC-09 | PASS | Seven complete shared resource rows and exact-bound/first-over/overflow/missing/duplicate/numeric/error/late/cleanup/trust/rationale drift checks pass. |
| TC-10 | PASS | Dev/release, ASan, UBSan, 256-run native fuzz smoke, and fresh per-package coverage all pass; every package is above 70%. |

## Acceptance-criterion results

| Acceptance criterion | Result | Evidence/reason |
|---|---|---|
| BUILD-FR-009 | PASS | Exact generated dev/release build facts and dirty/missing/wrong-fact negatives execute in c-01 through c-03. |
| BUILD-FR-010 | PASS | Canonical reference bytes/identity, nine ordered outcomes, one validated reference, and exact content/resource errors pass. |
| BUILD-FR-011 | PASS | Executable/package identities, bounded streaming walk, fixed digest, and metadata/link/type/read-mutation matrix pass. |
| BUILD-NFR-004 | PASS | Only the complete exact reference yields `validated_reference`; mismatch/unavailable remains unvalidated and no broad support claim exists. |
| GOV-FR-006 shared-core slice | PASS | Canonical descriptor/SPDX byte/count/document/subject primitives and resolved external bytes pass without implementing the producer. |
| GOV-FR-008 shared-core slice | PASS | Canonical release-evidence types, 12/12/3 state relationships, UTC RFC3339 approvals, typed limits, referenced byte/subject binding, and no-self/current-scan rules pass. |
| GOV-NFR-004 | PASS | All subject-named references are recomputed and mutation/substitution/self-reference is rejected. |
| GOV-NFR-006 shared-core slice | PASS | Deterministic canonical/SPDX primitives and fixed bytes pass; graph-to-SPDX reproducibility remains M0-004-owned. |
| PERF-FR-003 shared-core slice | PASS | Five schemas/digests, raw order/count/digest, statistics, and comparison relationships pass. |
| PERF-FR-012 shared-core slice | PASS | Byte/depth/count/arithmetic/work/RSS/time guards and native canonical parser fuzzing pass. |
| PERF-NFR-003 | PASS | Required provenance/sample fields and complete raw/result bundle reconciliation pass. |
| PERF-NFR-005 shared-core slice | PASS | 16-MiB/depth/count, 10,000-work, 256-MiB, and 120-s exact/first-over behavior passes. |
| CORP-FR-013 shared-core slice | PASS | Canonical run/reliability schemas, referenced reports, topology/fault/count relationships, resource limits, and forged-success rejection pass. |
| SEC-FR-005 | PASS | Lowercase SHA-256 subject identities are explicit, recomputed, and non-interchangeable. |
| SEC-FR-007 shared-core slice | PASS | Every task-owned owner/operation row freezes the literal bounds, enforcement point, error, cleanup, tests, and owner fields; full reconciliation remains M0-008. |
| SEC-NFR-003 | PASS | Referenced subjects and included metadata are verified; byte mutation, substitution, and self-reference negatives pass. |
| q-0012/q-0014 and dependency policy | PASS | Glaze v7.5.0, utf8proc, and OpenSSL EVP remain private behind C++ authority; pinned Python stays cold parity; no CMake or pin/ownership drift exists. |
| 70% contract-branch coverage ratchet | PASS | Fresh source-bound LCOV includes each retained package once; every package exceeds 70%, and missing/stale/empty/mutated/omitted/excluded/masked cases fail closed. |

## Negative and edge results

| ID | Result | Evidence/reason |
|---|---|---|
| N1 | PASS | BOM, order/whitespace/duplicate, invalid/non-NFC Unicode, forbidden integer/float/escape forms, unknown fields, depth/bytes, and terminal LF reject before identity in c-01 through c-04. |
| N2 | PASS | NFC/scalar/control/escape, permissive-parser defense, typed emission, and C++/Python byte/digest cases pass without leaking third-party types. |
| N3 | PASS | Executable/package metadata/content/link/type/read mutation, malformed digest, subject substitution, tamper, and self-reference cases reject precisely. |
| N4 | PASS | Fixed/production reference documents, nine outcomes, mismatches/unavailable, schema/operator/ID/noncanonical faults, and exact/first-over limits pass. |
| N5 | PASS | Generated dev/release plus dirty/missing/empty/wrong build-fact cases pass their literal outcomes. |
| N6 | PASS | Governance canonical bytes/subjects/cardinalities/cycles pass; invalid month/day/leap-year/clock/leap-second placement now rejects with the complete stable governance diagnostic, while valid leap boundaries pass. |
| N7 | PASS | Five performance goldens, order/count/digest/conditional mutations, median/MAD/percentile/overflow, and exact/first-over resources pass. |
| N8 | PASS | Corpus missing/extra/type/enum/digest/topology/PID/TID/fault/count/aggregate/forged-success/bound mutations reject before aggregate pass. |
| N9 | PASS | Seven resource rows pass exact limits and reject first-over, checked overflow, missing/duplicate/numeric/error/late/cleanup/trust/N/A-rationale drift. |
| N10 | PASS | Boundary tests reject empty/omitted corpus, zero execution, crash, timeout, OOM, ASan, and UBSan simulations; real c-04 runs the 256-iteration native target over five entries. |
| N11 | PASS | Exact 70% passes; below-70, missing/stale/empty/mutated LCOV/provenance, omitted source/package, duplicate ownership, masking, and unreviewed exclusion fail; current c-05 is source-bound. |
| N12 | PASS | Public-header/boundary tests and V7 reject third-party/native leaks, CMake/alternate build paths, and prohibited broad platform/security/M1+ claims. |

## Sign-off checklist

| Item | Result | Reason |
|---|---|---|
| Latest V1-V5 run is complete, current, nonempty, and green | PASS | `run-20260722T195725Z` covers c-01 through c-05 with exit 0 and current source provenance. |
| V6 and V7 explicitly addressed | PASS | c-06 and c-07 are both recorded above with path/range evidence and separate metadata classification. |
| TC-01 through TC-10 and every ledger AC pass | PASS | All target-condition and acceptance rows pass for the M0-002 shared-core slice. |
| N1-N12 produce their literal outcomes | PASS | Positive, malformed, relationship, resource, fuzz, and coverage negatives execute in the green registered population, including the RFC3339 rework cases. |
| C++ authority and Python byte/error parity | PASS | The public boundary and parity tests preserve the q-0014 language split. |
| Public API hiding plus literal byte/error/identity/resource/schema contracts | PASS | No third-party/native type escapes, and all reviewed literals including approval time now match. |
| Build facts/reference validation truthful | PASS | Exact generated facts, one reference, nine outcomes, and dirty/mismatch negatives pass. |
| Digest subjects/package behavior exact | PASS | Referenced bytes and package mutations cannot substitute subjects or self-reference. |
| Downstream ownership and no broad capability claim | PASS | All deferred producer/workflow owners remain intact and no live-provider, sandbox, signature, broad-platform, or M1+ claim is made. |
| Every owned package has fresh coverage >=70% | PASS | Lowest is `tools/coverage` at 71.038251%; provenance, inventory, and scope are bound. |
| Product scope separated from factory/history metadata | PASS | 48 product paths and 35 committed metadata/history paths are independently inventoried; newer verifier metadata is separate. |
| Independent identity, exact source, pins, and unresolved blockers | PASS | Author/rework and verifier roles are distinct, revision/snapshot are exact, pins/locks are unchanged, no healing is pending, and no blocker remains. |

## Substrate disposition

The substrate ledger still lists only `github-actions=substitute`
evidence. M0-002's checks exercise real local Nix/Bazel, native parser,
sanitizer, fuzz, and coverage boundaries but do not claim or exercise a live
GitHub Actions run. No substrate transition is warranted.

## Final disposition

`M0-002` is accepted and moved from `CHECKING` to `DONE`. Verdict: **pass**.
The authoritative judged check run is `run-20260722T195725Z`.
