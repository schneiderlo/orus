# M0-002 Verification Plan

## Preconditions

- Run every command from the repository root at the exact source revision
  named by `.agents/M0-002/handoff.md`. The Verifier must be a different agent
  from the Author.
- Use the real local Nix/Bazel substrate. The completed predecessor evidence is
  available at `.agents/M0-001/handoff.md` and
  `.agents/M0-001/verification-report-r2.md`; do not read an inherited
  `.factory/results/{step}/r{n}/result.json`.
- The pinned M0-001 dependency/toolchain set, including Glaze v7.5.0,
  utf8proc, OpenSSL 3 EVP, C++23 Clang, pinned Python, GoogleTest, sanitizer
  configurations, and the existing package gate, must be available without a
  pin or lock mutation.
- `//tests/contracts/...` must be the finite test population for the shared
  core, and `//tests/fuzz:canonical_json_parser_fuzz_smoke` must execute a real
  native parser corpus. An empty target, silent skip, or documented-only test
  is failure.
- The factory runs the registered checks before independent verification. A
  green factory run is lifecycle evidence only; product commands below do not
  assert task/plan status, ledger contents, or `.factory/**` metadata.
- No dependency-isolation healing rule applies and the outstanding
  `github-actions` substitute is outside this task. These steps are contract,
  unit, integration, fuzz, coverage, and manual-review evidence—not end-to-end
  evidence.

## Commands/Steps

Execute in this exact order.

### V1 — Canonical contract suite (dev)

```bash
nix develop --command bazel test --config=dev //tests/contracts/... && nix develop --command bazel test --config=release //tests/contracts:build_test
```

The suite must run the positive, malformed, relationship, cross-language,
identity, exact-bound/first-over, and coverage-gate unit fixtures described
below. The release clause must exercise the same Bazel-generated embedded
build-facts provider under the real release configuration, accepting a clean
full revision and rejecting an explicitly dirty revision. Tests must exercise
the production Orus libraries, not independent test-only reimplementations.

### V2 — Canonical contract suite (ASan)

```bash
nix develop --command bazel test --config=asan //tests/contracts/...
```

All applicable native contract/parser/package-walk tests execute under ASan
with leak detection and zero memory-safety or leak finding. Python-only tests
may be configuration-independent but must remain visible in the executed Bazel
population; no native test may be silently excluded.

### V3 — Canonical contract suite (UBSan)

```bash
nix develop --command bazel test --config=ubsan //tests/contracts/...
```

All applicable native integer, parser, identity, and resource-guard tests
execute under UBSan with zero undefined-behavior finding. Checked overflow
fixtures must return their typed error rather than invoke undefined behavior.

### V4 — Native canonical JSON parser fuzz smoke

```bash
nix develop --command bazel test --config=fuzz //tests/fuzz:canonical_json_parser_fuzz_smoke
```

The target must report a positive execution count over a checked-in seed and
malformed/resource corpus and finish with zero crash, timeout, OOM, ASan, or
UBSan finding. A zero-execution smoke run or corpus omission fails.

### V5 — Fresh package-scoped 70% coverage gate

```bash
nix develop --command bazel coverage --config=dev --combined_report=lcov --instrumentation_filter='^//(contracts|python/orus_contracts|tools)[/:]' //tests/build/... //tests/contracts/... && nix develop --command bazel run //tools/coverage:package_gate -- --threshold=70
```

The first clause must regenerate LCOV for the retained M0-001 tool packages and
all task-owned M0-002 C++/Python/core-tool packages. The second clause is the
task ledger's exact threshold gate. If the Author chooses a different in-scope
package path, the registered command and this document must be reconciled
before handoff; silently omitting it from the filter is failure.

### V6 — Manual frozen-contract, API, and ownership review

Inspect the implementation and retained evidence against specs `10`, `11`,
`14`, `15`, and `16`. Record PASS/FAIL with cited paths for every item:

- the public surface uses Orus-owned types and exposes no Glaze, utf8proc, or
  OpenSSL header/handle/native layout;
- strict byte prevalidation precedes parse/identity, and all canonical,
  Unicode, integer, escaping, unknown-field, duplicate-name, and terminal-byte
  rules match `M0-CANONICAL-JSON-v1`;
- C++ is authoritative for native/untrusted parsing, Python is pinned and
  limited to parity/cold validated data, and all shared goldens agree;
- build facts, reference validation, subject-named identities, package-tree
  rules, schemas, typed errors, and numeric resource bounds match the cited
  specs literally;
- governance, performance, corpus, and security work stops at the shared-core
  boundary stated in the A3; their downstream producer/workflow owners remain
  intact; and
- the handoff inventories the frozen API, schema/error/limit matrix, golden
  corpus, exact verified source revision, and any known limitations without a
  broader platform, signature, sandbox, or M1+ claim.

### V7 — Manual task-scope and evidence-lifecycle audit

Using the base and source revisions recorded in `.agents/M0-002/handoff.md`,
inspect the complete change set and classify every changed path into two
separate buckets:

1. **Task-owned implementation:** canonical/NFC/SHA-256/error/resource code,
   build facts, reference and subject/package identity code, shared evidence
   schema/types, contract/fuzz tests and goldens, Bazel registrations, and the
   minimum backward-compatible coverage/config registration named by the A3.
2. **Factory/audit metadata:** `.factory/run.json`,
   `.factory/results/**`, `.factory/questions/**`, `.factory/commits/**`,
   `.factory/logs/**`, task JSON/CLI-generated plan-row changes, and
   `.agents/**` evidence.

Factory/audit metadata is reported separately and does not fail scope by
itself. Step-boundary commits may include it. Fail for any unexplained
implementation path outside the first bucket, dependency pin/toolchain/
acquisition behavior change without owner evidence, hidden alternate build
path, missing handoff deliverable, or historical evidence/hand-off file used as
a volatile-path/scope-guard input. Prior `.agents/M0-001/` evidence is an
append-only historical record and is exempt from product-source path guards.

## Expected Results

| Step | Required result |
|---|---|
| V1 | Exit 0; all contract test targets execute; fixed bytes/digests, positive relationships, malformed inputs, exact-bound/first-over outcomes, cross-language parity, and coverage-gate mutation fixtures match their literal expectations. |
| V2 | Exit 0; native contract/parser/identity/resource tests execute under ASan with zero finding or leak; no applicable native target is skipped. |
| V3 | Exit 0; native tests execute under UBSan with zero finding; overflow and invalid arithmetic fail through typed errors. |
| V4 | Exit 0; positive fuzz execution over the declared corpus, with zero crash, timeout, OOM, ASan, or UBSan finding. |
| V5 | Exit 0; current non-empty LCOV represents every retained/in-scope package and each reports at least 70%; no aggregate-only pass or unreviewed exclusion. |
| V6 | PASS with a path/evidence citation for every bullet; exact Orus-owned boundary and downstream ownership are preserved. |
| V7 | PASS with separate task-owned and factory-metadata inventories; no out-of-scope implementation or volatile inherited-evidence dependency. |

## Negative/Edge Tests

The runnable suites must contain and execute the following cases internally.
Their registered command returns 0 only when the negative input produces the
required failure.

| ID | Fixture / edge | Required outcome | Covered by |
|---|---|---|---|
| N1 | BOM; whitespace/member reorder; duplicate object name; invalid UTF-8/lone surrogate; non-NFC name/value; float/exponent; signed-64 overflow; leading plus/zero/negative zero; invalid escape; unknown field; excess depth/bytes; terminal LF. | Reject before identity with the exact schema-specific `*_NONCANONICAL`, field, integer-overflow, or resource code/path/offset; never normalize into acceptance. | V1-V4 |
| N2 | NFC expansion/composition, embedded control characters, quote/backslash escaping, invalid scalar, and identical typed input through C++ and Python; permissive Glaze acceptance case. | Valid cases produce byte-identical canonical bytes/digests; invalid/non-NFC/permissive-parser cases fail at the Orus boundary; no third-party type escapes. | V1-V3, V6 |
| N3 | Raw executable mutation; package mtime-only, mode, path, symlink-target, special-file, hard-link ambiguity, read-mutation, and content changes; malformed/generic/alternate digest algorithm; subject substitution; self-reference. | Fixed package digest matches; mtime preserves tree identity; included metadata/content changes it as specified; invalid/generic/substituted/self-referential input fails before acceptance with the literal build/governance/security code. | V1-V3, V6 |
| N4 | Fixed `M0-REFENV-v1` example; production document; every observed-fact mismatch/unavailable; wrong schema/operator/ID; noncanonical bytes; exact 64 MiB/10 s guards and first attempted byte/nanosecond over. | Fixed ID and production ID recompute; exact reference yields nine ordered passes; each fact defect is `unvalidated`; content faults return their exact code; exact resource bounds pass and first-over returns `BUILD_REFENV_RESOURCE_LIMIT` before validation. | V1-V3 |
| N5 | Dev/release/dirty/missing/empty/wrong build facts. | Five fields are exact and non-empty in valid builds; dirty is explicit; dirty release or missing/wrong fact returns `BUILD_METADATA_INVALID` and cannot be presented as clean. | V1-V3, V6 |
| N6 | Canonical SPDX descriptor and release-evidence shared fixtures; array order/duplicate/generated identity mutation; missing/wrong executable/SBOM/package/evidence subject; self/current-scan/final-marker reference; a `preapproval_validated` fixture with 11 evidence, 11 validators, or 2 approvals; and over-limit 13/13/4 inventories. | Shared validators enforce canonical identity, subject binding, the state-dependent exact 12/12/3 preapproval contract, finite maxima, and acyclic/no-self relationships with exact typed failures; no actual release approval/marker is created by this task. | V1-V3, V6 |
| N7 | Five performance document examples and content digests; reordered/gapped/duplicate raw arrays; wrong count/digest/conditional field; odd/even negative/tie median and MAD; nearest-rank percentile; 16 MiB/depth/string/sample/workload/mismatch/int/RSS/time exact and first-over fixtures. | All examples/digests and integer summary primitives match; each malformed relationship/overflow/first-over returns the exact `M0-PERF-ERROR-v1` code before a comparison state or proportional excess work. | V1-V4 |
| N8 | Both corpus report examples; missing/extra/type/enum/digest; split parent PIDs; reused/cross-owned TIDs; equal parent/child image digest; wrong fault terminal; duplicate/unsorted/zero failure row; aggregate count mismatch; forged scalar-correct success; byte/depth/count first-over. | Valid examples parse; each invalid mutation returns the exact `CORP_REPORT_*` code before aggregate pass; release-profile success is impossible without exact 100-run relationships. | V1-V4 |
| N9 | Every M0-002 resource row at its exact input/count/depth/memory/time bound and first unit over; checked-integer overflow; cancellation/cleanup seam; missing/duplicate/numeric-drift/late-enforcement row. | Exact bounds pass; each first-over/overflow fails before allocation, identity, descriptor, result, or other authoritative side effect with the literal owner code and cleanup; invalid resource mapping fails. | V1-V4 |
| N10 | Empty seed corpus, zero fuzz execution, crash, timeout, OOM, ASan finding, UBSan finding, and corpus omission simulations inside the test harness. | Each simulation makes the fuzz-registration/verification test fail; the real V4 run reports positive execution and zero finding. | V1, V4 |
| N11 | Package exactly 70%, first below 70%, missing/stale/empty LCOV, omitted owned source/package, repository-average masking, duplicate ownership, and unreviewed exclusion. | Exact 70% passes; every other listed defect fails `package_gate`; current real report includes every M0-002 package and retains M0-001 package coverage. | V1, V5 |
| N12 | Public-header compile fixture without direct third-party deps; downstream consumer attempting native layout/third-party handle use; CMake/alternate build-path reference; M1+/broad-support/signature/security claim. | Valid consumer compiles against Orus API only; each boundary leak, alternate path, or prohibited claim is rejected by test/manual audit. | V1-V3, V6-V7 |

## Evidence to attach

The Verifier links or summarizes these stable artifacts in
`.agents/M0-002/verification-report.md`:

- `.agents/M0-002/handoff.md`, exact implementation base/source revisions,
  Author identity, Verifier identity, and the latest ordered factory check run;
- stdout/stderr or retained logs for V1-V5 and an executed Bazel target list
  proving no required test/fuzzer was empty or skipped;
- public API/target inventory and third-party-boundary compile evidence;
- canonical JSON/NFC/SHA-256 cross-language differential report, fixed byte/
  digest goldens, and malformed-input/error matrix;
- build-facts and reference-environment reports, including fixed/production
  IDs, nine-outcome mismatch matrix, and exact-bound/first-over results;
- subject/package identity mutation matrix and bounded package-walk evidence;
- governance, performance, corpus, and resource shared-schema validation
  reports with explicit downstream ownership notes;
- fuzz execution/sanitizer report and minimized regression-corpus index;
- fresh LCOV, per-package threshold report, complete package/source inventory,
  and reviewed finite exclusions;
- V6 frozen-contract/API/ownership review and V7 separate task-owned versus
  factory/audit metadata inventories; and
- TC-01 through TC-10, every cited acceptance criterion, N1-N12, and every
  sign-off item below with PASS/FAIL, reason, and evidence link.

Do not use an overwritable `.factory/results/{step}/r{n}/result.json` as
inherited evidence. Later tasks must cite `.agents/M0-002/` handoff,
verification-report, or Andon artifacts, or `.factory/results/_archive/`.

## Sign-off checklist (PASS/FAIL + reasons)

The Verifier must record `PASS` or `FAIL`, a concise reason, and an evidence
link for every item:

- [ ] Latest factory execution of V1-V5 is green; no command or target is
  stale, empty, skipped, optional, or replaced by an unregistered command.
- [ ] V6 and V7 are both addressed explicitly with the required evidence and
  metadata separation.
- [ ] TC-01 through TC-10 and every task-ledger acceptance criterion pass for
  the M0-002 shared-core scope.
- [ ] N1-N12 were actually exercised and produced their literal expected
  outcome; negative fixtures were not merely inspected.
- [ ] C++ is the authoritative native/untrusted parser and all permitted Python
  outputs/errors are byte-identical over the shared corpus.
- [ ] Public APIs hide third-party/native layouts; canonical byte, typed error,
  subject identity, and resource contracts match the specs.
- [ ] Build facts and reference validation are truthful; exactly one M0
  reference contract is supported; dirty/unknown/mismatch cannot pass.
- [ ] Digest subjects cannot substitute or self-reference; package metadata
  inclusion/exclusion and bounded identity behavior are exact.
- [ ] Downstream governance/performance/corpus/security producer workflows
  remain outside scope and no future/platform/security capability is claimed.
- [ ] Every M0-002 core/business package has fresh line coverage >=70% without
  masking, omission, or unreviewed exclusion; retained M0-001 rows are not
  weakened.
- [ ] Task-owned implementation is within scope; factory metadata is reported
  separately; historical evidence was not treated as product source input.
- [ ] Author and Verifier identities differ, the exact source revision is
  recorded, no dependency pin/lock changed without owner evidence, and no
  unresolved Andon remains.

Any `FAIL` prevents acceptance. The report must name the failed criterion,
observed result, retained evidence, and required rework; prose cannot convert a
red runnable check into PASS.

## Acceptance Criteria Traceability (AC -> verification step)

| Acceptance criterion | Verification step(s) | Specific proof |
|---|---|---|
| BUILD-FR-009 | V1-V3, V6; N5 | Exact five build facts across dev/release, explicit dirty state, missing/dirty release rejection. |
| BUILD-FR-010 | V1-V3, V6; N1, N2, N4 | Canonical bytes/ID, one production contract, nine outcomes, exact mismatch/content/resource behavior. |
| BUILD-FR-011 | V1-V3, V6; N3, N9 | Same Bazel release subject boundary, exact executable/package identities, bounded walk, metadata mutation matrix. |
| BUILD-NFR-004 | V1, V6; N4, N12 | Exactly one `validated_reference`; all mismatches unvalidated; zero broad support claim. |
| GOV-FR-006 (shared-core slice) | V1-V3, V6; N2, N3, N6 | Canonical SPDX descriptor/byte/subject/generated-identity primitives available to M0-004; producer/reconciliation deferred. |
| GOV-FR-008 (shared-core slice) | V1-V3, V6; N3, N6, N9 | Canonical release-evidence types, 12/12/3 inventory relationships, typed limits, and no-self/current-scan references. |
| GOV-NFR-004 | V1-V3, V6; N3, N6 | All subject-named references validate; mutation/substitution/self-reference detected. |
| GOV-NFR-006 (shared-core slice) | V1, V6; N2, N6 | Deterministic canonical primitives and byte goldens for repeatable SPDX emission; actual graph-to-SPDX reproducibility remains M0-004-owned. |
| PERF-FR-003 (shared-core slice) | V1-V4, V6; N1, N2, N7 | Five complete schemas/fixed digests, raw order/count/digest and integer-summary validation, exact errors. |
| PERF-FR-012 (shared-core slice) | V1-V4; N1, N7, N9 | Pre-allocation byte/depth/count/arithmetic/work guards, typed error and fuzz coverage. |
| PERF-NFR-003 | V1, V6; N7 | Required provenance/sample fields and complete raw-sample reconciliation. |
| PERF-NFR-005 (shared-core slice) | V1-V4; N7, N9 | 16 MiB/depth/count, 10,000-work, 120 s, and 256 MiB guard behavior; comparator workflow deferred. |
| CORP-FR-013 (shared-core slice) | V1-V4, V6; N8, N9 | Run/reliability schemas, canonical examples, topology/fault/count relationships, forged-success/resource rejection. |
| SEC-FR-005 | V1-V3, V6; N3, N6 | Exact lowercase SHA-256 subject identities, bytes, non-interchangeability, tamper/self-reference rejection. |
| SEC-FR-007 (shared-core slice) | V1-V4, V6; N4, N7-N9 | Resource guard schema/types and exact owner bounds for M0-002 operations; full inventory reconciliation deferred to M0-008. |
| SEC-NFR-003 | V1-V3, V6; N3, N6 | 100% subject-named reference verification and byte/included-metadata/substitution/self-reference mutation detection. |
| q-0012/q-0014 and task dependency policy | V1-V4, V6-V7; N2, N12 | Glaze v7.5.0/utf8proc/OpenSSL hidden behind C++ authority; pinned Python cold parity; no CMake/ownership drift. |
| 70% contract-branch coverage ratchet | V1, V5; N11 | Fresh per-package LCOV, exact-70 pass, fail-closed below/missing/omitted/excluded behavior. |

## Coverage Gate (command, scope, threshold, pass rule)

**Command:**

```bash
nix develop --command bazel coverage --config=dev --combined_report=lcov --instrumentation_filter='^//(contracts|python/orus_contracts|tools)[/:]' //tests/build/... //tests/contracts/... && nix develop --command bazel run //tools/coverage:package_gate -- --threshold=70
```

**Scope:** Every M0-002-owned core/business package exercised by
`//tests/contracts/...`, including C++ canonical parsing/emission, NFC/SHA-256,
typed errors/checked integers/resource guards, build facts, reference
validation, subject/package identity, shared evidence schema/relationship
logic, and pinned Python parity logic. The growing package manifest must retain
the M0-001 logic packages exercised by `//tests/build/...`. Vendor, generated,
declarative Bazel/configuration, and fixture-only files may be excluded only
through a finite reviewed path-and-reason manifest; public or business logic
cannot be moved into an excluded path.

**Threshold:** At least 70% line coverage for every included package. This is
the accepted M0-002 contract-branch gate in the implementation plan. The same
package population ratchets to 75% at M0-009 and 80% at M0-010; those later
thresholds do not replace or weaken this gate.

**Pass rule:** Both command clauses exit 0; LCOV is freshly generated,
non-empty, and source-revision-consistent; every in-scope package and source is
represented exactly once; each package is at or above 70%; and every exclusion
matches the reviewed finite manifest. Exactly 70% passes. A lower package,
missing/stale/empty data, omitted source/package, duplicate ownership,
unauthorized exclusion, narrowed instrumentation filter, or repository-wide
average that masks a failing package fails.
