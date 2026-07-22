# M0-002 Verification Report r2

**Task:** `M0-002`  
**Verifier:** Factory step `10_verifier`, independent re-verification attempt `r2`  
**Latest handoff:** `.agents/M0-002/handoff-r2.md`  
**Judged check run:** `run-20260722T183843Z` (GREEN)  
**Implementation base:** `bdd18d1739f4d3e3e13a81c46fd5cd7e00cb1c9a`  
**Rework base:** `026d0d962eacdc5bcb79e67a5877a486f017c314`  
**Exact product source:** `14eeed8fd7ef2ddc2d6993894f42a582dc03651d`  
**Handoff wrapper revision:** `12c91ab2b67a1689d44ad3ef68f5fdfdfba9622a`

## Decision

**BLOCKED — required check c-06 fails.** The latest registered automated run is
complete and green, including every package's 70% coverage threshold, and the
manual scope/lifecycle audit c-07 passes. The frozen release-evidence schema is
not literal, however: the approval-time validator accepts impossible calendar
dates as UTC RFC3339 timestamps. This is an implementation and negative-test
defect, not a specification defect. No healing task is declared.

`specs/11-governance-release.md:220` requires each approval `time` to be UTC
RFC3339 seconds. `IsRfc3339Seconds` in
`contracts/evidence_contracts.cc:86-100` checks only the lexical separators,
`month` in 1..12, `day` in 1..31, and clock-field maxima. It never reconciles
the day with the month or leap year. Consequently a canonical manifest row
containing `2026-02-31T12:00:00Z` passes this predicate even though that value is
not an RFC3339 date-time. The helper also caps seconds at 59 without accounting
for RFC3339's valid UTC leap-second form. The governance tests construct only
`2026-07-22T12:00:00Z` (`tests/contracts/evidence_test.cc:148-155`) and contain
no invalid-calendar or leap-boundary mutation.

Required rework:

1. Validate the complete UTC RFC3339-seconds calendar relationship, including
   month-specific days and leap-year behavior, with the intended RFC3339
   leap-second semantics.
2. Add positive boundary fixtures and negative impossible-date/time fixtures
   through the production governance validator, preserving the literal typed
   governance error.
3. Refresh the append-only handoff and run the complete registered check suite.

## Registered machine checks

The registered commands were not re-executed individually. This report judges
the factory record and retained logs for `run-20260722T183843Z`; no stale or
flaky indication warranted a whole-suite rerun.

| Check | Result | Evidence and judgment |
|---|---|---|
| c-01 / V1 dev + release | PASS | Exit 0 in 307.1 s. All 6/6 `//tests/contracts/...` targets pass in dev and the separately configured release `build_test` passes 1/1. Evidence: `.factory/checks/M0-002/runs/run-20260722T183843Z.json` and `c-01.stdout.log`/`c-01.stderr.log` under the corresponding log directory. |
| c-02 / V2 ASan | PASS | Exit 0 in 132.5 s; 6/6 targets pass with `-fsanitize=address`, leak detection, and no sanitizer finding. Evidence: `c-02.stdout.log`, `c-02.stderr.log`. |
| c-03 / V3 UBSan | PASS | Exit 0 in 113.6 s; 6/6 targets pass with non-recovering UBSan and no undefined-behavior finding. Evidence: `c-03.stdout.log`, `c-03.stderr.log`. |
| c-04 / V4 fuzz | PASS | Exit 0 in 48.3 s; 1/1 native target passes under libFuzzer+ASan+UBSan. `tests/fuzz/BUILD.bazel:16-24` fixes `-runs=256` and the five-file checked-in corpus, so the passing target is neither empty nor a zero-execution registration. Evidence: `c-04.stdout.log`, `c-04.stderr.log`. |
| c-05 / V5 coverage | PASS | Exit 0 in 168.1 s; the producer executes 9/9 tests, validates revision/source/manifest/LCOV provenance, and every package exceeds the exact 70% threshold. Evidence: `c-05.stdout.log`, `c-05.stderr.log`, and `bazel-out/_coverage/orus-source-provenance.json`. |

Coverage from authoritative c-05:

| Package | Line coverage | Threshold | Result |
|---|---:|---:|---|
| `contracts` | 85.737439% | 70% | PASS |
| `python/orus_contracts` | 85.593220% | 70% | PASS |
| `tools` | 79.411765% | 70% | PASS |
| `tools/build` | 75.321337% | 70% | PASS |
| `tools/coverage` | 71.038251% | 70% | PASS |

The sidecar binds revision
`12c91ab2b67a1689d44ad3ef68f5fdfdfba9622a`, source snapshot
`7659d97bb1e261d3086649c677db060a42fbe9ad260034c67a21d35fc17fd312`,
the fixed instrumentation filter, the package manifest, all 12 owned source
files, and the LCOV digest. The source snapshot equals the handoff's product
snapshot for `14eeed8...`; the intervening commit is audit metadata only.

## c-06 — Manual frozen-contract, API, and ownership review: FAIL

| Required V6 item | Result | Evidence |
|---|---|---|
| Public surface uses Orus-owned types and hides Glaze, utf8proc, OpenSSL, and native layouts | PASS | `contracts/include/orus/contracts/contracts.h` includes only standard-library headers and exposes Orus-owned values; third-party integration remains private in `canonical_json.cc` and `crypto_resource.cc`. `public_header_test` passes under dev/ASan/UBSan. |
| Strict byte prevalidation and canonical/Unicode/integer/escape/member/terminal rules are exact | PASS | `contracts/canonical_json.cc`, `canonical_test.cc`, and the five fuzz seeds reject BOM, whitespace/reordering/duplicates, malformed/non-NFC Unicode, forbidden numbers/escapes, excess bounds, and terminal bytes before Glaze/identity. |
| C++ authority, pinned cold Python parity, and shared goldens | PASS | Native and untrusted parsing enters the C++ API. `python/orus_contracts/canonical.py` is a pinned cold parity surface; `python_parity_test.py` proves the shared byte/digest/statistics corpus in c-01 through c-03. |
| Build facts, reference validation, subject identities, schemas, typed errors, and numeric limits match cited specs literally | **FAIL** | Build/reference/identity/error/resource behavior passes, but the release-evidence approval timestamp is not a literal UTC RFC3339-seconds validator. `contracts/evidence_contracts.cc:86-100,656-668`; `specs/11-governance-release.md:220`. |
| Governance/performance/corpus/security work stops at the shared-core boundary | PASS | No SPDX producer/release gate, performance runner/comparator producer, concurrent corpus runtime, complete security reconciler, CI provider, marker, signature, or M1+ capability was added. The ownership limits in `a3.md:92-97` remain intact. |
| Handoff inventories API/schema/error/limit/goldens, exact source, and limitations without overclaim | **FAIL** | `handoff-r2.md` contains the requested inventory and exact product source, but it claims complete document/relationship acceptance and no blocking ambiguity while omitting the accepted invalid RFC3339 calendar values (`handoff-r2.md:31-35,218-233`). |

Overall c-06 is **FAIL**.

## c-07 — Manual task-scope and evidence-lifecycle audit: PASS

The exact full task lifecycle range audited is
`bdd18d1739f4d3e3e13a81c46fd5cd7e00cb1c9a..12c91ab2b67a1689d44ad3ef68f5fdfdfba9622a`.
The latest product-rework subrange is
`026d0d962eacdc5bcb79e67a5877a486f017c314..14eeed8fd7ef2ddc2d6993894f42a582dc03651d`;
`14eeed8...12c91ab` contains only the append-only handoff and factory/audit
metadata. This adapts the audit to the task's latest implementation/rework
source rather than treating the wrapper commit or current verifier mutations
as product implementation.

### Task-owned implementation: 48 paths

- Build/config integration (7): `.bazelrc`, `MODULE.bazel`,
  `config/BUILD.bazel`, `flake.nix`, `tools/build/build_facts.bzl`,
  `tools/build/nix_repositories.bzl`, `tools/build/workspace_status.sh`.
- C++ contracts (7): `contracts/BUILD.bazel`, `build_contracts.cc`,
  `canonical_json.cc`, `crypto_resource.cc`, `evidence_contracts.cc`,
  `include/orus/contracts/contracts.h`, and `resource_monitor.h`.
- Python parity (3): the three files under `python/orus_contracts/`.
- Build/coverage support (4): `tests/build/BUILD.bazel`,
  `tests/build/contract_test.py`, `tools/coverage/package_gate.py`, and
  `tools/coverage/packages.json`.
- Contract tests (8): `tests/contracts/BUILD.bazel`, `boundary_test.py`,
  `build_test.cc`, `canonical_test.cc`, `evidence_test.cc`,
  `public_header_test.cc`, `python_parity_test.py`, and `test_support.h`.
- Golden fixtures (11): both corpus fixtures, all five performance fixtures,
  both reference fixtures, the release-evidence fixture, and the SPDX
  descriptor under `tests/contracts/fixtures/`.
- Fuzz target/corpus (8): `tests/fuzz/BUILD.bazel`, the C++ target, the smoke
  verifier, and all five files under `tests/fuzz/corpus/canonical_json/`.

All 48 paths are within the A3 shared-core/test/coverage registration scope.
The root/config changes expose the already pinned Nix OpenSSL, utf8proc, LLVM,
and build facts to Bazel; no dependency coordinate, digest, lock, compiler,
network/acquisition policy, or alternate/CMake build path changed.

### Factory/audit metadata and historical evidence: 29 paths

- `.agents/M0-002/` (6): three handoffs, two prior verification reports, and
  `verification.md`.
- `.factory/checks/M0-002/` (11): two registered check definitions and nine
  retained historical run records through `run-20260722T182133Z`.
- `.factory/results/` (9): four active step results and five append-only
  archived results.
- Lifecycle metadata (3): `.factory/run.json`,
  `.factory/tasks/M0-002.json`, and the CLI-generated task row in
  `IMPLEMENTATION_PLAN.md`.

The authoritative `run-20260722T183843Z` record/logs and the current task/result
mutations are newer uncommitted factory-owned evidence. They are judged and
reported separately, not product source. All `.agents/**` and `.factory/**`
handoffs/reports/logs/results are append-only historical or step-boundary
records and are exempt from volatile-path/product-source guards. No inherited
`.agents/M0-001/**` evidence changed or became a product input. No unexplained
product path exists.

Overall c-07 is **PASS**.

The substrate ledger still lists only `github-actions=substrate`. M0-002's real
local Nix/Bazel/parser/fuzz/coverage checks do not claim or exercise the live
GitHub Actions boundary, so no substrate transition is warranted.

## Target-condition results

| Target condition | Result | Evidence/reason |
|---|---|---|
| TC-01 | PASS | Strict canonical C++ boundary, exact emission, and typed rejections pass c-01 through c-04 and source review. |
| TC-02 | PASS | Orus-owned NFC/SHA adapters and C++/Python byte/digest/statistics parity pass. |
| TC-03 | PASS | Bazel-generated five-field dev/release build facts and explicit dirty/missing release rejection pass. |
| TC-04 | PASS | Fixed/production reference IDs, exactly nine outcomes, one supported contract, and exact/first-over resources pass. |
| TC-05 | PASS | Subject-named executable/package/SBOM/evidence identities and referenced-byte substitution/tamper checks pass. |
| TC-06 | **FAIL** | The release-evidence shared schema accepts an impossible approval calendar date as UTC RFC3339 seconds; the claimed frozen manifest relationship is incomplete. |
| TC-07 | PASS | Five performance schemas/digests, bundle reconciliation, statistics, comparison conditions, and malformed/resource negatives pass; decision production remains M0-005-owned. |
| TC-08 | PASS | Corpus examples, topology/fault/aggregate/reference/resource mutations, and forged-success rejection pass. |
| TC-09 | PASS | Seven full reusable resource rows and exact/first-over/overflow/drift checks pass. |
| TC-10 | PASS | Dev, ASan, UBSan, 256-run native fuzz smoke, and fresh per-package coverage all pass. |

## Acceptance-criterion results

| Acceptance criterion | Result | Evidence/reason |
|---|---|---|
| BUILD-FR-009 | PASS | Generated dev/release build facts and dirty/missing/wrong-fact negatives pass. |
| BUILD-FR-010 | PASS | Canonical reference bytes/ID, nine ordered outcomes, content errors, and resource boundaries pass. |
| BUILD-FR-011 | PASS | Executable/package identities, bounded streaming walk, and metadata/link/mutation matrix pass. |
| BUILD-NFR-004 | PASS | Exactly one complete reference is validated; mismatches remain unvalidated and no broad-support claim exists. |
| GOV-FR-006 shared-core slice | PASS | Descriptor bytes, counts, SPDX identity, digest, and subject relationships resolve without implementing the producer. |
| GOV-FR-008 shared-core slice | **FAIL** | Referenced bytes and 12/12/3 inventories resolve, but an approval row can carry an impossible date despite the literal UTC RFC3339-seconds field contract. |
| GOV-NFR-004 | PASS | All subject-named references are recomputed and substitution/tamper/self-reference is rejected. |
| GOV-NFR-006 shared-core slice | PASS | Deterministic canonical/SPDX primitives and fixed bytes pass; producer reproducibility remains M0-004-owned. |
| PERF-FR-003 shared-core slice | PASS | Five schemas, fixed digests, raw/result relationships, integer summaries, and comparison conditional relationships pass. |
| PERF-FR-012 shared-core slice | PASS | Byte/depth/count/arithmetic/work/RSS/time guards and parser fuzzing pass. |
| PERF-NFR-003 | PASS | Required provenance/sample fields and raw-result bundle reconciliation pass. |
| PERF-NFR-005 shared-core slice | PASS | 16 MiB/depth/count, 10,000-work, 120 s, and 256 MiB guard behavior pass. |
| CORP-FR-013 shared-core slice | PASS | Canonical run/reliability schemas, resolved reports, topology/fault/count relationships, and forged-success/resource rejection pass. |
| SEC-FR-005 | PASS | Lowercase SHA-256 subject identities remain explicit, recomputed, and non-interchangeable. |
| SEC-FR-007 shared-core slice | PASS | All task-owned owner/operation rows freeze literal bounds, enforcement, error, cleanup, test, and ownership fields. |
| SEC-NFR-003 | PASS | Referenced subjects and included metadata are verified; byte mutation/substitution/self-reference negatives pass. |
| q-0012/q-0014 and dependency policy | PASS | Glaze v7.5.0, utf8proc, and OpenSSL EVP remain private behind C++ authority; Python stays cold parity; no pin/CMake/ownership drift. |
| 70% contract-branch coverage ratchet | PASS | Fresh provenance-bound LCOV includes each retained package and every package exceeds 70%; fail-closed provenance/scope negatives pass. |

## Negative and edge results

| ID | Result | Evidence/reason |
|---|---|---|
| N1 | PASS | Canonical BOM/member/Unicode/number/escape/depth/bytes/terminal mutations execute and reject under c-01 through c-04. |
| N2 | PASS | NFC/scalar/control/escape/permissive-parser and C++/Python parity cases pass. |
| N3 | PASS | Executable/package metadata/content/link/type, subject substitution, tamper, and self-reference cases pass. |
| N4 | PASS | Fixed/production reference documents, nine outcomes, schema/operator/ID/content faults, and exact/first-over limits pass. |
| N5 | PASS | Generated dev/release/dirty/missing/empty/wrong build-fact cases pass. |
| N6 | **FAIL** | The governance corpus covers byte/subject/cardinality/cycle mutations but omits invalid calendar timestamps, and production validation accepts `2026-02-31T12:00:00Z`. |
| N7 | PASS | Five performance goldens, raw/result/comparison relationships, statistics/overflow, and exact/first-over resource cases pass. |
| N8 | PASS | Corpus type/enum/digest/topology/TID/PID/fault/count/aggregate/forged-success/bound mutations pass. |
| N9 | PASS | Seven resource rows pass exact boundaries and reject first-over, overflow, missing/duplicate/drift/late/cleanup mutations. |
| N10 | PASS | `boundary_test.py` rejects empty/omitted corpus, zero execution, crash, timeout, OOM, ASan, and UBSan simulations; real c-04 runs 256 iterations over five entries. |
| N11 | PASS | Exact-70, below-70, missing/stale/empty/mutated LCOV/provenance, omitted source/package, duplicate ownership, masking, and exclusion negatives pass; current c-05 is source-bound. |
| N12 | PASS | Public-header/boundary tests and V7 reject third-party/native leaks, alternate build paths, and prohibited broad/security/M1+ claims. |

## Sign-off checklist

| Item | Result | Reason |
|---|---|---|
| Latest V1-V5 run is complete, current, nonempty, and green | PASS | `run-20260722T183843Z` covers c-01 through c-05 with exit 0. |
| V6 and V7 explicitly addressed | **FAIL** | Both are recorded; V7 passes and V6 fails on the RFC3339 calendar relationship. |
| TC-01 through TC-10 and every ledger AC pass | **FAIL** | TC-06 and GOV-FR-008 shared-core fail. |
| N1-N12 produce their literal outcomes | **FAIL** | N6 lacks and fails the invalid-calendar timestamp outcome. |
| C++ authority and Python byte/error parity | PASS | Public boundary and parity evidence pass. |
| Public API hiding plus literal byte/error/identity/resource/schema contracts | **FAIL** | Hiding and all other reviewed literals pass, but the approval-time schema literal does not. |
| Build facts/reference validation truthful | PASS | Generated facts, one reference, nine outcomes, and dirty/mismatch negatives pass. |
| Digest subjects/package behavior exact | PASS | Referenced bytes and package mutations cannot substitute subjects or self-reference. |
| Downstream ownership and no broad capability claim | PASS | All deferred workflows remain with M0-004 through M0-008. |
| Every owned package has fresh coverage >=70% | PASS | Lowest is `tools/coverage` at 71.038251%; provenance and scope are bound. |
| Product scope separated from factory/history metadata | PASS | 48 product paths and 29 metadata/history paths are independently inventoried. |
| Independent identity, exact source, pins, and unresolved blockers | **FAIL** | Implementer and verifier roles are independent, source and pins are exact, but this report records an unresolved implementation blocker. |

## Final disposition

`M0-002` was moved from `CHECKING` to `BLOCKED`. Verdict: **fail**. The judged
check run remains `run-20260722T183843Z`; its green machine evidence is valid
but cannot override required manual check c-06.
