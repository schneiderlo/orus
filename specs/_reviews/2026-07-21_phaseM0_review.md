# M0 Domain Specification Review — 2026-07-21

**Round:** First review  
**Scope:** `SPECS.md`, foundation files `00`-`03`, and all seven M0 domain
specifications (`10`-`16`)  
**Verdict:** `needs_work`

This was the first M0 review. The complete checklist was applied to every M0
domain file. The phase is correctly bounded to M0 and the specs preserve the
foundation decisions, but twelve contract, testability, and consistency
defects block READY. No owner clarification is required; every finding can be
resolved without adding product scope.

## Status table

| File | Status | Blocking findings | Review summary |
|---|---|---|---|
| `SPECS.md` | ACCEPTABLE | None | The M0 target, dependencies, exit gate, and Draft statuses are accurate. M1-M11 remain roadmap-only and are not needed before M0 planning. |
| `specs/10-build-environment.md` | NEEDS WORK | M0-001, M0-002 | Purpose, scope, requirements, flows, failures, observability, and test mapping are strong, but the reference-environment and package identity contracts are not implementable unambiguously. |
| `specs/11-governance-release.md` | NEEDS WORK | M0-002, M0-003 | License, contribution, dependency, SBOM, claim, and release-gate requirements are well traced, but governance record schemas and digest/license states are incomplete. |
| `specs/12-cli-diagnostics.md` | NEEDS WORK | M0-001, M0-004 | The seven-row doctor inventory and exit-code matrix are finite and testable, but error/help output conflicts with the owner-selected one-JSON-document contract. |
| `specs/13-ci-quality.md` | NEEDS WORK | M0-005, M0-011 | The applicability matrix and blocking/advisory separation are concrete, but evidence limits and the permission exception are not testable, and the CI capability policy conflicts with dependency acquisition. |
| `specs/14-performance-foundation.md` | NEEDS WORK | M0-006, M0-007 | Workloads, authority, samples, resource limits, and tests are comprehensive, but canonical serialization and the statistical decision cannot be reproduced independently from the current text. |
| `specs/15-concurrent-corpus.md` | NEEDS WORK | M0-008, M0-009 | Topology, partitions, deadline, cleanup, faults, and the 100-run gate are precise, but the wire/report payloads and nondeterministic observation workload remain ambiguous. |
| `specs/16-security-foundations.md` | NEEDS WORK | M0-010, M0-011, M0-012 | Boundaries, digests, fuzzing, and future claim limits are well scoped, but a stated READY prerequisite is absent, network capability rules conflict with the build, and the zero-secret scan population is undefined. |

All FR and NFR IDs are unique within their domains and every FR/NFR appears in
the corresponding test-plan table. No M1+ availability claim, CMake path, new
license requirement, or unresolved product choice was found in the specs.

## Cross-spec consistency check

| Contract / concern | Result | Evidence |
|---|---|---|
| M0 scope and foundation decisions | CONSISTENT | All seven specs remain Draft, deliver only M0 behavior, and apply D-001 through D-016 without weakening the Charter. |
| Build facts into CLI | PARTIAL | The five build facts agree across `10` and `12`; `M0-REFENV-v1` predicate and identity semantics do not (M0-001). |
| Artifact/package/SBOM/evidence identity | BLOCKED | All owners select lowercase hexadecimal SHA-256, but they do not identify the same canonical bytes for a package/tree and one SBOM digest field is potentially self-referential (M0-002, M0-003). |
| CLI output | BLOCKED | Success documents are JSON, while syntax/help/error behavior is not consistently JSON despite the authoritative answer requiring one JSON document per invocation (M0-004). |
| CI and performance authority | PARTIAL | Shared-runner data is consistently advisory, but the result encoding and comparator algorithm are not reproducible enough to verify a blocking decision (M0-006, M0-007). |
| Concurrent corpus into CI/performance/security | PARTIAL | Topology, sums, deadline, and fault inventory agree; exact IPC payloads, report schemas, observation calls, and one cardinality count do not (M0-008, M0-009). |
| Security resource and capability policy | BLOCKED | `16` requires limit rows before READY but supplies no finite M0 row set, and its zero-network-grant rule conflicts with the acquisition stage in `10` (M0-010, M0-011). |
| Risk and ADR application | CONSISTENT WITH FINDINGS | M0 risks R-001 through R-010 are referenced by their owning specs, and future risks are treated as policy-only. The defects below prevent the named mitigations from being verifiable; no conflicting Accepted ADR was found. |

## Required edits

### M0-001 — `M0-REFENV-v1` is not a concrete cross-spec contract

**Locations:** `10` lines 72 and 120-130; `12` lines 93-124.  
**Finding:** The reference environment is called a structured document and its
ID is the SHA-256 of a normalized document excluding `environment_id`, but the
serialization, canonicalization, field types, predicate grammar, predicate
operators, observed-fact representation, and schema-version rejection rules
are not defined. In particular, two implementations can disagree on kernel,
CPU/ISA, libc, and environment-ID matching while both appear to satisfy the
prose. That makes BUILD-TEST-010 and CLI-TEST-003/010 non-reproducible.

**Required edit:** In `10` Section 6.3, add the exact serialization and
canonicalization rule; enumerate every required field with type, width/bound,
optionality, allowed predicate operators, and comparison semantics; define the
observed-fact/result shape and typed schema/predicate errors; and include one
reference plus one mismatch example. Update `12` to reference those exact
field paths and comparison outcomes rather than “normalized observed facts.”

**Risk:** R-008 and R-009; a host can be mislabeled validated or the same
contract can receive different identities.

### M0-002 — The canonical bytes represented by package/artifact digests are undefined

**Locations:** `10` lines 73, 80, 165-166; `11` lines 63-65 and 93-94; `16`
lines 67 and 217-219.  
**Finding:** The specs distinguish executable, package, SBOM, and evidence
digests but do not define how the SHA-256 of a multi-file Nix package/output
tree is calculated. “Normalized release package” is undefined, so BUILD-NFR-001
cannot prove equal package digests and the release/security manifests can bind
different byte subjects under the same generic `artifact` label.

**Required edit:** Name each digest field by subject (`orus_executable`,
`package`, `sbom`, and evidence object); define the exact byte representation
for each, including the package/tree serialization or content-tree algorithm;
state whether metadata such as mode, path, timestamp, and symlink target is
included; and align `10`, `11`, and `16` on those field names and rules. Add
one fixture that changes package metadata without changing the executable and
asserts the specified identity behavior.

**Risk:** R-005 and R-008; release evidence can validate the wrong object or
fail reproducibility despite identical intended content.

### M0-003 — Governance records have unresolved schema and digest/license states

**Locations:** `11` lines 61-64 and 88-108.  
**Finding:** The four governance contracts are field summaries rather than
concrete schemas: types, required/optional fields, enums, identity encoding,
relationship constraints, and error payloads are absent. Two material
ambiguities remain: `license/SPDX expression when known` does not say how an
unknown license is represented or whether it blocks admission/release, and
`M0-SBOM-CONTRACT-v1` lists the SBOM document's own SHA-256 digest without
stating whether that digest is external or excludes a field.

**Required edit:** Add a concrete schema (or normative field tables) for
`M0-DEPENDENCY-ADMISSION-v1`, `M0-SBOM-CONTRACT-v1`,
`M0-RELEASE-EVIDENCE-v1`, and `M0-APPROVED-CLAIMS-v1`, including types,
requiredness, enums, bounds, identity/relationship invariants, and typed
validation errors. Define the exact unknown-license state and gate effect.
Place the SBOM document digest in an external descriptor or explicitly define
the excluded-field calculation. Add one valid example per contract.

**Risk:** R-005 and R-008; dependency/license reconciliation and tamper checks
can accept incompatible interpretations.

### M0-004 — CLI error and help behavior violates or omits the one-document contract

**Locations:** `12` lines 51-60, 79-91, 113-124, and 172-181.  
**Finding:** The owner-selected contract is one bounded JSON document per
invocation. Version failures are JSON, but invalid syntax is only a “bounded
usage diagnostic,” and no normative JSON schema exists for usage, contract,
or renderer errors. `help` is included in claim scanning but has no invocation,
output, or exit contract. The specs therefore permit non-JSON output and
cannot golden-test all advertised CLI surfaces.

**Required edit:** Define one versioned top-level JSON error schema with exact
fields, types, bounds, code enum, and stdout/stderr placement; apply it to exit
2 and exit 4 paths. Either define the finite `--help` behavior with its JSON
schema and exit status or explicitly state that M0 exposes no help invocation
and remove help-specific claims/tests. Reconcile FR-009, Section 6.1, failure
modes, and CLI-TEST-004/005/009.

**Risk:** R-008; callers cannot reliably parse failures and an implementation
can contradict the explicit human output decision.

### M0-005 — CI evidence bounds and privileged-job exception are untestable

**Locations:** `13` lines 65, 68, and 142-155.  
**Finding:** CI-FR-011 requires bounded reports and CI-TEST-011 tests a report
size boundary, but the only bound is “by job configuration”; no byte/count
limits or truncation rules for each artifact/log class are specified. CI-FR-008
also permits write, OIDC, or package permissions when an unnamed “separately
reviewed release job” requires them, although publication and signing are out
of M0 scope and no exception schema, job, permission set, or test exists.

**Required edit:** Add numeric per-report, per-diagnostic, and retained-log
bounds with exact reject/truncate behavior and required summary fields. Remove
the M0 permission escape hatch, or bind it to a named in-scope non-publishing
job with exact permissions, approval record, secret exposure rule, and
negative tests. Update CI-TEST-008/011 pass criteria to those exact values.

**Risk:** R-005, R-007, and R-008; CI can leak/exhaust evidence storage or gain
untestable privilege while still satisfying the prose.

### M0-006 — Canonical performance JSON and result schemas are incomplete

**Locations:** `14` lines 64, 73, and 110-147.  
**Finding:** The custom canonical JSON rules do not fix string escaping/Unicode
normalization, all integer domains, required versus optional fields, enum
values, or the complete schemas for raw sample, result, comparison, runner,
and workload documents. Content IDs and byte-identical golden evidence can
therefore differ between conforming producers, and parser tests cannot provide
one-negative-per-field coverage from the prose alone.

**Required edit:** Reference a complete canonicalization standard/profile or
finish the custom byte-level rules, including Unicode/string escaping and
integer domains. Add normative schemas for all five performance document
types with field types, requiredness, enums, bounds, conditional storage
fields, relationship constraints, and typed parse errors. Include canonical
and deliberately noncanonical examples with fixed SHA-256 goldens.

**Risk:** R-002 and R-201; comparator inputs can acquire different identities
or be interpreted differently.

### M0-007 — The “exact” comparator is not independently reproducible

**Locations:** `14` lines 63, 68, 71, 80, and 151-200.  
**Finding:** Section 6.4 does not select the seed byte encoding/hash extraction,
pseudorandom generator, sampling algorithm, fixed-point scale, overflow rule,
or rounding rule; it merely says some arithmetic rules are versioned. It also
does not say whether five warmups means total or per baseline/candidate, and
noise failure has three conflicting outcomes: `PERF_NOISE_POLICY_FAILED`,
`inconclusive`, or either. These differences can change the 99% bound and the
blocking verdict.

**Required edit:** Specify the complete deterministic algorithm: paired
warmup schedule, result-ID ordering and seed derivation, PRNG and integer
sampling method, fixed-point representation/scale, division and median
rounding, overflow handling, quantile selection, and terminal state for each
noise/sample failure. Make FR-007/010, Section 6.4, the data-model state
machine, PERF-FAIL-004, and PERF-TEST-007/010 use one state vocabulary. Add a
checked-in sample corpus with intermediate seed/resample/bound goldens, not
only final dispositions.

**Risk:** R-002; different implementations can make opposite release-blocking
3% decisions.

### M0-008 — Corpus IPC and evidence payloads are not concrete schemas

**Locations:** `15` lines 66, 107-133, and 152-164.  
**Finding:** The 16-byte frame header is defined, but READY, START,
CHILD_RESULT, ACK, and CANCEL payloads only name fields. They omit byte widths,
offset/order, digest encoding, bit assignments, reason enum, permitted payload
length per type, and concrete error behavior. The run/reliability documents
likewise list fields without types, requiredness, bounds, enums, or a valid
example. Protocol and forged-report tests are therefore not independently
implementable.

**Required edit:** Add a byte-offset table for every frame type, including
exact length, integer width/endianness, digest representation, observation
bits, reason/status enums, and reserved-byte validation; include one encoded
success example and one malformed example. Add normative typed schemas and a
valid example for both corpus report documents, including the exact
success/cleanup invariants and typed terminal outcomes.

**Risk:** R-003, R-006, and R-008; peers or evidence validators can disagree
while falsely reporting the required topology/result.

### M0-009 — Corpus observation workload and observability cardinality are inconsistent

**Locations:** `15` lines 68, 135-150, and 246-254.  
**Finding:** CORP-FR-007 requires a “declared realtime clock source and kernel
randomness source,” but neither call, clock ID, read size, success rule, nor
call count is declared. That changes the syscall workload later milestones are
supposed to reuse. Separately, CORP-OBS-001 bounds state events by “fixed six
role/main identities,” while the specified topology has seven identities: two
main threads plus five workers.

**Required edit:** Name the exact time and randomness operations, arguments,
byte count, invocation point/count per process, and success/error criteria;
make the observation fields/tests reflect those choices. Correct the
CORP-OBS-001 identity count to seven and make its cardinality formula match
the explicit role table and state transitions.

**Risk:** R-003 and R-006; the reusable corpus can exercise different behavior
or omit one role from lifecycle evidence.

### M0-010 — The required `M0-RESOURCE-LIMIT-v1` READY inventory is absent

**Locations:** `16` lines 69, 138-159, and 204-215.  
**Finding:** SEC-FR-007 explicitly requires every domain accepting structured
or resource-bearing input to register an enforced row before becoming Ready.
Section 6.3 defines only the row shape; it does not contain a finite M0 row
inventory or `not_applicable` rationale set for domains `10`-`16`. The test
plan consequently cannot reconcile the limits already scattered across the
domain specs, and the phase does not satisfy its own readiness precondition.

**Required edit:** Add the finite M0 resource-limit table now. Include one row
for every applicable build/reference contract, governance/release record, CLI
input/output, CI report/log, performance document/comparator, corpus IPC/report
and process/thread/FD/deadline operation, and security inventory/diagnostic;
for each row reference the exact numeric owner requirement or give the scoped
`not_applicable` rationale allowed by SEC-FR-007. Add a cross-domain
reconciliation matrix to SEC-TEST-007.

**Risk:** R-201 and DOD-08; inputs can reach implementation without a complete
pre-allocation/side-effect bound despite a claimed READY gate.

### M0-011 — Zero M0 network capability conflicts with required dependency acquisition

**Locations:** `10` line 70 and lines 103-107; `13` lines 65-66; `16` lines
65-68 and 121-136.  
**Finding:** BUILD-FR-008 permits a dependency-acquisition phase before
network-disabled actions, while SEC-FR-006/SEC-NFR-004 require zero M0
`network.client` grants for every M0 component/job. A clean-clone Nix/Bzlmod
run cannot both acquire missing pinned inputs and satisfy an inventory in
which no acquisition component can use the network. The current text does not
define a prefetch boundary or trusted acquisition profile.

**Required edit:** Define one consistent acquisition model across `10`, `13`,
and `16`: either make a fully prepopulated/offline input store a stated gate
precondition, or define a separately scoped hash-verifying acquisition
component/profile and keep network denied to build actions and untrusted PR
code. Update the capability inventory, CI tests, and hermeticity evidence to
prove the chosen boundary without implying network access inside Bazel
actions.

**Risk:** R-001 and R-005; implementations must otherwise violate either the
canonical clean build or the least-privilege gate.

### M0-012 — The zero-secret scan population is not finite

**Locations:** `16` lines 64, 82, and 294-298.  
**Finding:** SEC-FR-002/NFR-001 alternately name source, “relevant
history/diff,” fixtures, packages, logs, caches, and evidence without defining
which revisions, generated paths, workflow attempts, caches, or package
closures comprise the blocking population. “Relevant” is not measurable, and
SEC-TEST-002 cannot prove the stated zero count from a copy/pasteable command.

**Required edit:** Enumerate the exact scan sets for a pull-request gate and a
release gate (revision range/history policy, tracked and generated trees,
package closure, named log/artifact/cache manifests, exclusions, and maximum
input behavior). Define how unavailable or truncated scan input affects the
gate, then update SEC-TEST-002/003 fixtures and evidence fields to reconcile
100% of those sets.

**Risk:** R-005 and R-202; secrets can be excluded from evidence by an
implementation-specific interpretation of “relevant.”

## Open questions

None. The required changes refine existing accepted requirements and owner
choices; they do not require a new product decision.

## Verdict

`needs_work`

M0 cannot be marked READY until M0-001 through M0-012 are resolved. Because
the current factory wave is M0 only, the unwritten M1-M11 domain specs are not
needed before M0 planning and `more_phases` is false.
