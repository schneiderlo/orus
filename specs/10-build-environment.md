# Build Environment and Source-to-Release Contract

**Status:** Draft  
**Milestone:** M0  
**Owners:** Build/toolchain owner; release engineer  
**Last updated:** 2026-07-21  
**Depends on:** `specs/00-CHARTER.md`, `specs/01-GLOSSARY.md`, `specs/02-DECISIONS.md`, and `specs/03-RISKS.md` (all Ready)  
**Foundation decisions:** D-003, D-004, D-011  
**Risks addressed:** R-001, R-008, R-009

## 1. Purpose

This domain owns the one reproducible path from a clean Orus checkout to
development builds, tests, benchmarks, and the release package. Nix owns the
host environment and packaging closure; Bazel with Bzlmod owns every source
action. The consumers are developers, CI, release engineering, and every later
domain that introduces a native target.

The boundary ends at producing a correctly identified `orus` binary and
package. License/SBOM admission is owned by
`specs/11-governance-release.md`; CLI rendering by
`specs/12-cli-diagnostics.md`; job topology by
`specs/13-ci-quality.md`; and benchmark semantics by
`specs/14-performance-foundation.md`.

The M0 gate output is a retained clean-clone report proving that every
canonical Charter command succeeds in the single pinned Linux x86-64 reference
environment, with no CMake path or undeclared host dependency.

## 2. In Scope

- Root Nix flake, lock, supported-system declaration, development shell,
  checks, and `orus` package.
- Root Bazel/Bzlmod workspace, locks, Bazel pin, build settings, and target
  graph.
- Hermetic Clang/LLVM/LLD primary toolchain and GCC compatibility
  configuration.
- Dev, release, ASan, UBSan, TSan-applicability, fuzz, and benchmark entry
  points.
- Version/revision/configuration/compiler/target build facts.
- The versioned `M0-REFENV-v1` reference-environment contract.
- Clean-build, action-sandbox, offline-action, wrapper, and packaging
  contracts.

## 3. Out of Scope

- Target tracing, task control, recording, replay, or deterministic execution;
  their first owners are M1-M3 specs.
- Multi-distribution, multi-kernel, musl, ARM64, attach, or cross-architecture
  support; D-003 and D-011 deliberately limit the claim.
- Dependency legal admission, notices, SBOM contents, and publication
  authorization; owned by `specs/11-governance-release.md`.
- CI service topology and applicability policy; owned by
  `specs/13-ci-quality.md`.
- Provisioning a controlled performance runner; M0 only defines its contract
  in `specs/14-performance-foundation.md`.
- CMake or any alternate source build graph.

## 4. Functional Requirements

| ID | Requirement | Acceptance criteria | Verification method | Traceability |
|---|---|---|---|---|
| BUILD-FR-001 | The repository shall commit the Nix and Bazel roots and immutable lock inputs required by D-004. | `flake.nix`, `flake.lock`, `MODULE.bazel`, `MODULE.bazel.lock`, `BUILD.bazel`, `.bazelrc`, and `.bazelversion` exist; the first implementation change selects a mutually compatible set of then-current stable Nixpkgs, Bazel, Clang/LLVM/LLD, and GCC releases; every exact coordinate and artifact digest is locked; locks resolve without mutation; `bazel mod graph` exits 0. | BUILD-TEST-001 root/lock inspection, exact-coordinate inventory, and clean resolution report. | G-01, G-02, C-02, D-004, R-001; resolved `q-0003`. |
| BUILD-FR-002 | Nix shall be the sole owner of developer, CI, benchmark, toolchain, and packaging environments. | Each canonical command enters the flake environment; a minimal host lacking compilers, headers, Bazel, and generators still succeeds through Nix; flake evaluation requires no secret. | BUILD-TEST-002 minimal-host integration test and environment-input manifest. | G-01, C-02, C-11, SM-03. |
| BUILD-FR-003 | Bazel with Bzlmod shall own all compilation, linking, generation, tests, benchmarks, and the release binary. | Bazel query/aquery accounts for every generated or compiled release input; no wrapper performs a source action outside Bazel; external modules are lock-resolved. | BUILD-TEST-003 query/aquery ownership audit. | G-02, C-02, R-001. |
| BUILD-FR-004 | The primary native toolchain shall compile C++23 with pinned Clang/LLVM and link release artifacts with LLD. | A C++23 conformance target passes; compile actions identify the selected pinned Clang; the release link action identifies LLD; no action resolves a host compiler. | BUILD-TEST-004 toolchain conformance plus action-log assertions. | C-01, D-004. |
| BUILD-FR-005 | The same reference environment shall expose a GCC compatibility configuration. | `bazel test --config=gcc //...` selects the declared GCC toolchain, compiles all applicable M0 C++ targets as C++23, and reports any target excluded by the M0 CI applicability inventory. | BUILD-TEST-005 GCC configuration test and applicability reconciliation. | SM-04, D-003, D-004. |
| BUILD-FR-006 | Every required configuration entry point shall be explicit and fail on host-toolchain fallback or silent empty execution. | `dev`, `release`, `asan`, `ubsan`, `tsan`, `fuzz`, `benchmark`, and `gcc` resolve to declared settings; an inapplicable target emits the recorded reason required by `M0-CI-APPLICABILITY-v1`; a deliberately broken toolchain fixture fails. | BUILD-TEST-006 configuration contract and negative fixture. | Charter 4.1, DOD-04, R-007. |
| BUILD-FR-007 | The repository shall contain no CMake or independent canonical build path. | A tracked-and-generated-file scan finds zero `CMakeLists.txt`, CMake preset/cache/generator metadata, executable CMake invocation, or documentation that presents a second source build path. Approved Bazel wrappers only delegate without adding actions. | BUILD-TEST-007 prohibited-path scanner with positive detection fixtures. | SM-02, C-02, D-004, R-001. |
| BUILD-FR-008 | Build actions shall be hermetic and network-independent after dependency acquisition. | With Nix/Bzlmod inputs already fetched, all canonical build/test actions run sandboxed with action-network access disabled and use zero undeclared host file, environment, tool, header, or library inputs. | BUILD-TEST-008 minimal-host offline action run plus sandbox/aquery evidence. | SM-03, R-001. |
| BUILD-FR-009 | The build shall embed the five authoritative `M0-BUILD-FACTS-v1` fields. | Product version, full source revision, configuration, compiler identity/version, and target platform are non-empty and correct in dev and release fixtures; a dirty/uncommitted source state is represented explicitly and cannot masquerade as the clean revision. | BUILD-TEST-009 stamped/unstamped golden fixtures consumed by CLI tests. | G-06, SM-06, R-008. |
| BUILD-FR-010 | The build domain shall publish exactly one `M0-REFENV-v1` supported contract and its validation ADR. | The committed contract has one immutable environment identity and declares Linux, x86-64, every locked Nixpkgs/Bazel/Clang/LLVM/LLD/GCC coordinate, target triple, kernel predicate, CPU/ISA baseline predicate, libc facts where applicable, and the support-language token `validated_reference`; the M0 reference-environment ADR records selection date, compatibility evidence, alternatives, consequences, and rollback; any other observed host is `unvalidated`. | BUILD-TEST-010 schema/ADR validation and reference/mismatch fixtures. | G-01, C-03, D-003, R-009; resolved `q-0003`. |
| BUILD-FR-011 | `nix build .#orus` shall produce the release package through the same Bazel release target used elsewhere. | The package contains one release `orus` executable, required runtime closure, build facts, and governance-required license/notices/SBOM files; package construction invokes no independent compile or link action. The release-evidence bundle remains external and binds the completed package SHA-256 digest, avoiding a circular identity. | BUILD-TEST-011 package-to-Bazel provenance inspection. | G-01, G-02, DOD-03. |
| BUILD-FR-012 | Documentation and convenience wrappers shall delegate to the Charter canonical commands. | Each executable wrapper has a machine-readable mapping to one or more canonical Nix+Bazel commands; a scan finds zero wrapper-only build configuration or unsupported-platform claim. | BUILD-TEST-012 documentation/wrapper conformance review. | Charter 7, C-12, R-001, R-009. |

## 5. Non-Functional Requirements

| ID | Quality attribute and target | Conditions / workload | Acceptance criteria | Verification method and evidence |
|---|---|---|---|---|
| BUILD-NFR-001 | Reproducibility: 100% of Charter Section 7 commands exit 0. | Two clean checkouts of one revision in `M0-REFENV-v1`, with only declared inputs and sandboxing enabled. | Every command succeeds in both runs; normalized release packages have identical lowercase hexadecimal SHA-256 digests; zero lockfile mutation occurs. | BUILD-BENCH-001 clean-build JSON report, command logs, and package digests. |
| BUILD-NFR-002 | Hermeticity: 0 undeclared host inputs and 0 action-time network accesses. | Minimal host, prefetched locks, all Bazel build/test/package actions. | Sandbox/aquery audit reports zero undeclared inputs and network-denial log reports zero attempted access. | BUILD-TEST-008 retained action graph and denial log. |
| BUILD-NFR-003 | Build-path singularity: 0 CMake artifacts/invocations and 0 independent source-action paths. | Tracked files, generated metadata, docs, workflows, and executable wrappers at the release revision. | Scanner returns zero findings; each injected forbidden fixture is detected. | BUILD-TEST-007 SARIF/JSON scan report. |
| BUILD-NFR-004 | Platform truthfulness: exactly 1 M0 supported environment contract. | Release metadata, docs, `doctor` fixtures, and package at the release revision. | Exactly one `validated_reference` identity is named; all mismatch fixtures are `unvalidated`; zero broader distro/kernel/CPU claim exists. | BUILD-TEST-010 and release-claim scan. |
| BUILD-NFR-005 | Configuration completeness: 100% of named configurations resolve without silent skip or host fallback. | All M0 Bazel targets reconciled through `M0-CI-APPLICABILITY-v1`. | Every required cell executes; every not-applicable cell has a scoped rationale; the fallback/empty fixtures fail. | BUILD-TEST-006 machine-readable reconciliation. |

The future recorder/replay throughput and overhead objectives are not
applicable to this domain because BUILD-FR-001 through BUILD-FR-012 introduce
no target hot path. `specs/31-recorder-pipeline.md` and replay specs make
those objectives applicable.

## 6. Interfaces / Contracts

### 6.1 Build entry points

| Contract | Authoritative owner | Consumer | Required behavior |
|---|---|---|---|
| `flake devShells.x86_64-linux.default` | Nix flake | Developers and CI | Supplies pinned Bazel, Clang/LLVM/LLD, GCC, and declared tools; no secrets. |
| `flake checks.x86_64-linux` | Nix flake | `nix flake check` | Delegates source checks to Bazel and validates flake/package invariants. |
| `flake packages.x86_64-linux.orus` | Nix flake | Release engineer | Packages the Bazel release output and governance evidence. |
| Bazel configurations | `.bazelrc` and registered toolchains | Developers and CI | `dev`, `release`, `asan`, `ubsan`, `tsan`, `fuzz`, `benchmark`, `gcc`; unknown configuration is an error. |
| Build facts provider | Bazel-generated declared source | CLI and package metadata | Emits `M0-BUILD-FACTS-v1`; no runtime Git/tool invocation. |

Commands are synchronous and idempotent for an unchanged checkout and lock
set. Interruption may leave only tool-owned caches and declared output trees;
the next run does not interpret partial output as a valid package. Build
actions have no secret capability. Cache credentials, if configured outside
M0, are injected at runtime and never enter action inputs or retained logs.

### 6.2 `M0-BUILD-FACTS-v1`

| Field | Type / bound | Source | Rule |
|---|---|---|---|
| `schema` | literal string, 32 bytes max | build definition | Exactly `M0-BUILD-FACTS-v1`. |
| `product_version` | UTF-8 string, 1-64 bytes | declared version target | Non-empty semantic release identifier or documented pre-release identifier. |
| `source_revision` | 40- or 64-character lowercase hex plus optional `-dirty` | declared repository-status input | Full clean revision in releases; dirty state explicit elsewhere. |
| `configuration` | enum | Bazel setting | `dev`, `release`, or sanitizer/benchmark test configuration as applicable. |
| `compiler` | UTF-8 string, 1-128 bytes | selected toolchain | Compiler family plus exact version; never host auto-detection. |
| `target_platform` | UTF-8 string, 1-128 bytes | Bazel platform | Includes OS and architecture; M0 release is Linux x86-64. |

### 6.3 `M0-REFENV-v1`

The contract is a committed, low-volume structured document. It contains
`schema`, `environment_id` (lowercase 64-character hexadecimal SHA-256 digest
of the normalized contract excluding this field),
`nix_system`, locked Nix input revisions/hashes, Bazel and compiler
identities and exact versions, target triple, kernel release predicate, CPU vendor/model and
required ISA predicate, libc identity when relevant, and
`support_level=validated_reference`. All strings are at most 256 bytes; list
fields are at most 128 entries. Unknown schema, duplicate field, or failed
predicate is a typed failure, never a pass.

### 6.4 Design choice: environment/build ownership

| Alternative | Pros | Cons |
|---|---|---|
| Nix environment/package plus Bazel source graph (recommended) | One owner per layer; matches local, CI, benchmark, and release paths; binding D-004 choice. | Requires deliberate Nix-to-Bazel toolchain registration. |
| Nix-only source build | One tool at the top level. | Violates Bazel ownership and loses the required scalable target graph. |
| Bazel-only environment and packaging | One build graph. | Weakens pinned host/package ownership and conflicts with D-004. |

**Recommendation:** Apply the first alternative exactly. This is not an open
choice: D-004 is Accepted.

### 6.5 Design choice: initial reference-environment coordinates

| Alternative | Pros | Cons |
|---|---|---|
| Select a mutually compatible set of current stable Nixpkgs, Bazel, Clang/LLVM/LLD, and GCC releases, then lock it immutably (selected) | Starts M0 on supported tooling; exact locks make the choice reproducible and reversible. | Requires compatibility validation and a deliberate future update process. |
| Use an owner-supplied enterprise-pinned set | Could align with an existing operating standard. | No such set is named in the authoritative inputs, so it cannot define M0 now. |
| Select the oldest packaged set available | Exercises older tooling. | Adds maintenance and compatibility risk without a product requirement. |

**Recommendation:** Use the selected current-stable compatible set. The first
implementation change records the exact immutable coordinates, artifact
digests, compatibility matrix, and rollback in the M0 reference-environment
ADR; no floating channel or version range is part of `M0-REFENV-v1`. This
implements the human answer to `q-0003`.

## 7. Data Model

| Entity | Identity and lifecycle | Invariants / retention |
|---|---|---|
| Locked input set | Lowercase hexadecimal SHA-256 digest of committed Nix and Bzlmod locks; changes only by reviewed dependency update. | Resolution does not mutate locks during a gate run; retained in source history. |
| Toolchain | Bazel toolchain target plus content-addressed Nix inputs. | Compiler/linker identity in actions equals embedded build facts. |
| Build configuration | Stable enum plus normalized-settings SHA-256 digest. | Same name cannot silently change toolchain family or C++ level. |
| Reference environment | `environment_id` from `M0-REFENV-v1`. | Exactly one M0 supported identity; actual observed facts are separate from expected predicates. |
| Build artifact | Lowercase hexadecimal SHA-256 content digest plus `M0-BUILD-FACTS-v1`. | A package never identifies itself by filename alone; the digest is not described as a signature. |
| Build evidence | Source revision, command ID, exit status, action graph digest, logs, artifact SHA-256 digests. | Immutable for a gate run and retained with M0 release evidence. |

Build facts are authoritative evidence produced from declared inputs. Human
labels, local filenames, and observed success on another host are advisory and
cannot change support state.

## 8. Key Flows

1. **Clean source-to-release flow (BUILD-FR-001 through BUILD-FR-012).**
   The operator checks out a clean revision and locks; Nix resolves
   `M0-REFENV-v1`; Bazel registers the declared toolchain; canonical commands
   execute sandboxed; Bazel produces the release binary and facts; Nix packages
   that exact output; governance validation attaches release evidence; the
   clean-build report records all command results and SHA-256 digests.
2. **Cancellation and retry flow (BUILD-FR-002, BUILD-FR-008, BUILD-FR-011).**
   The caller interrupts Nix or Bazel; the process returns non-zero; no package
   is published; only owned caches/output trees may remain; a retry validates
   action inputs and reconstructs any incomplete output before success.
3. **Unsupported or mismatched environment flow (BUILD-FR-010).** The
   contract validator observes a failed OS, architecture, kernel, CPU, or
   environment predicate before a supported claim; it returns
   `BUILD_UNVALIDATED_ENVIRONMENT` with expected/observed facts; developers
   may experiment, but release evidence and authoritative benchmarks are
   rejected.
4. **Hidden-input flow (BUILD-FR-007, BUILD-FR-008).** Sandbox or scanner
   detects the first undeclared file/tool/network/CMake dependency, stops the
   action, records the exact action and input, and produces no valid package.

## 9. Failure Modes

| ID | Trigger | Required detection point | Typed outcome / diagnostic fields | Side effects and cleanup | Retry / recovery | Verifying requirements/tests |
|---|---|---|---|---|---|---|
| BUILD-FAIL-001 | Missing, unlocked, or mutated Nix/Bzlmod input. [R-001] | Resolution before source action. | `BUILD_LOCK_INVALID`; ecosystem, module/input, expected hash, observed state. | No build evidence or package; fetched cache may remain. | Correct lock through reviewed update, then rerun. | BUILD-FR-001 / BUILD-TEST-001. |
| BUILD-FAIL-002 | Action resolves host compiler/header/library/tool or attempts network. [R-001] | Sandbox access boundary. | `BUILD_UNDECLARED_INPUT`; action label, path/capability, toolchain, source. | Failing action output discarded; no publication. | Declare and pin input or remove use. | BUILD-FR-002, -003, -008 / BUILD-TEST-008. |
| BUILD-FAIL-003 | CMake or alternate source action is found. [R-001] | Repository/workflow scan before build gate. | `BUILD_PROHIBITED_PATH`; file, line/action, matched rule. | Gate stops; no package is accepted. | Remove path; dependency exception requires superseding owner-approved decision if it changes D-004. | BUILD-FR-007 / BUILD-TEST-007. |
| BUILD-FAIL-004 | Named config silently falls back or executes no required target. [R-007] | Config/applicability reconciliation. | `BUILD_CONFIG_INVALID`; config, target, selected toolchain, expected status. | No passing CI result. | Fix registration/matrix and rerun. | BUILD-FR-006 / BUILD-TEST-006. |
| BUILD-FAIL-005 | Embedded metadata is empty, wrong, or dirty release is attempted. [R-008] | Artifact validation before packaging. | `BUILD_METADATA_INVALID`; field, expected source, observed value. | Binary may remain a local output; package/publication rejected. | Rebuild from declared clean state. | BUILD-FR-009, -011 / BUILD-TEST-009. |
| BUILD-FAIL-006 | Actual platform does not satisfy `M0-REFENV-v1`. [R-009] | Environment validation before supported use. | `BUILD_UNVALIDATED_ENVIRONMENT`; contract ID, predicate, expected, observed, recoverable=true. | No supported or authoritative result; diagnostic retained. | Move to reference environment or proceed only as explicitly unvalidated. | BUILD-FR-010 / BUILD-TEST-010. |
| BUILD-FAIL-007 | Build/package is cancelled or partially written. | Process interruption and package finalization. | `BUILD_CANCELLED`; command ID, phase, exit/signal, partial-output path. | Partial package is not committed; owned temporary output cleaned on next run. | Safe automatic retry from declared inputs. | BUILD-FR-011 / BUILD-TEST-011. |

## 10. Observability

| Signal ID | Metric/event/log | Type, unit, and labels | Emission point / owner | Cardinality and cost bound | Alert/gate use | Verification |
|---|---|---|---|---|---|---|
| BUILD-OBS-001 | `build_command_result` | Event; command ID, config, revision, exit code, duration_ms | Canonical command wrapper / build owner | At most one start and terminal event per command; no per-compiler-input logs. | SM-01 gate and diagnosis. | BUILD-TEST-002. |
| BUILD-OBS-002 | `build_action_input_audit` | Counter/report; declared, undeclared, network_attempt counts | Post-build audit / build owner | One aggregate per action mnemonic and failing label; target-label cardinality bounded by Bazel graph. | SM-03 gate. | BUILD-TEST-008. |
| BUILD-OBS-003 | `build_environment_validation` | Event; contract ID, predicate, expected, observed, support level | Environment validator / build owner | One row per `M0-REFENV-v1` predicate; at most 128. | C-03/release gate. | BUILD-TEST-010. |
| BUILD-OBS-004 | `build_artifact_identity` | Event; artifact SHA-256 digest, facts schema, revision, config | Package finalization / release owner | One event per packaged artifact; no source paths or secrets. | DOD-03 provenance. | BUILD-TEST-011. |
| BUILD-OBS-005 | `build_prohibited_path_findings` | Gauge/report; finding count by stable rule | Pre-gate scanner / build owner | Stable rule cardinality at most 64; zero required. | SM-02 gate. | BUILD-TEST-007. |

Build evidence correlates by source revision, build configuration, reference
environment ID, action-graph digest, and artifact SHA-256 digest. M0 has no trace,
branch, execution, VPID, or VTID identity. Raw command logs, normalized action
audit, and artifact digests are retained in the M0 gate evidence bundle.

## 11. Test & Verification Plan

Copy/paste from the repository root after implementation:

```bash
nix flake check
nix develop --command bazel build --config=dev //...
nix develop --command bazel test --config=dev //...
nix develop --command bazel test --config=asan //...
nix develop --command bazel test --config=ubsan //...
nix develop --command bazel build --config=release //...
nix develop --command bazel test --config=benchmark //tests/benchmarks/...
nix develop --command bazel run //tools:format
nix develop --command bazel run //cli:orus -- --version
nix build .#orus
nix develop --command bazel test --config=gcc //...
nix develop --command bazel run //tools/build:hermeticity_audit
nix develop --command bazel run //tools/build:prohibited_path_scan
```

| Requirement ID | Test/benchmark/review ID | Level | Fixture/workload and environment | Pass criterion | Evidence artifact |
|---|---|---|---|---|---|
| BUILD-FR-001 | BUILD-TEST-001 | Integration/inspection | Clean checkout and committed locks in reference environment. | Required roots exist; locks unchanged; module graph resolves. | `build-lock-report.json`. |
| BUILD-FR-002, BUILD-FR-003 | BUILD-TEST-002/003 | End-to-end/analysis | Canonical commands on minimal host; Bazel query/aquery. | All succeed; every source action and input has one declared owner. | `clean-build-report.json`, `aquery.pb`. |
| BUILD-FR-004, BUILD-FR-005 | BUILD-TEST-004/005 | Integration | C++23 fixture under pinned Clang/LLD and GCC configs. | Primary and compatibility builds use expected tools and pass. | `toolchain-conformance.xml`. |
| BUILD-FR-006 | BUILD-TEST-006 | Negative integration | All named configs plus host-fallback and empty-execution fixtures. | Valid configs reconcile; both negative fixtures fail. | `configuration-contract.json`. |
| BUILD-FR-007, BUILD-NFR-003 | BUILD-TEST-007 | Static/negative | Repository plus injected CMake/invocation fixtures. | Real tree has zero findings; every injected fixture is detected. | `prohibited-paths.sarif`. |
| BUILD-FR-008, BUILD-NFR-002 | BUILD-TEST-008 | End-to-end/security | Prefetched inputs, denied network, sandbox-debug action set. | Zero undeclared input and zero network attempt. | `hermeticity-report.json`. |
| BUILD-FR-009 | BUILD-TEST-009 | Unit/golden | Dev, release, dirty, and missing-field metadata fixtures. | Values exact; invalid/dirty release fixture rejected. | `build-facts-junit.xml`. |
| BUILD-FR-010, BUILD-NFR-004 | BUILD-TEST-010 | Schema/integration | Reference plus OS/arch/kernel/CPU/input mismatch fixtures. | Reference passes every row; each mismatch is unvalidated and non-zero where mandatory. | `reference-environment-report.json`. |
| BUILD-FR-011 | BUILD-TEST-011 | End-to-end | Two clean package builds from same revision/input set. | Package uses Bazel output; required contents exist; normalized SHA-256 digests match. | `package-provenance.json`. |
| BUILD-FR-012 | BUILD-TEST-012 | Inspection | Docs and every executable convenience wrapper. | 100% map to canonical commands; zero independent action. | `wrapper-map.json`. |
| BUILD-NFR-001 | BUILD-BENCH-001 | End-to-end | Two clean checkouts; complete Charter command sequence. | 100% command success, no lock mutation, matching normalized package SHA-256 digests. | `reproducibility-report.json`. |
| BUILD-NFR-005 | BUILD-TEST-013 | Analysis | Bazel query result against M0 applicability inventory. | Every target appears exactly once and every required config executes. | `build-applicability-reconciliation.json`. |

## 12. Open Questions

No open questions.
