# Build Environment and Source-to-Release Contract

**Status:** Draft  
**Milestone:** M0  
**Owners:** Build/toolchain owner; release engineer  
**Last updated:** 2026-07-21  
**Depends on:** `specs/00-CHARTER.md`, `specs/01-GLOSSARY.md`, `specs/02-DECISIONS.md`, and `specs/03-RISKS.md` (all Ready)  
**Foundation decisions:** D-003, D-004, D-011  
**Risks addressed:** R-001, R-005, R-008, R-009, R-201

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
| BUILD-FR-008 | Build actions shall be hermetic and network-independent after a separately scoped, hash-verifying input-acquisition phase. | The `m0_input_acquisition` profile may fetch only immutable, admitted coordinates named by the committed Nix/Bzlmod locks, at most 128 coordinates/4 GiB each/16 GiB total/1,200 seconds, verifies each expected digest before read-only-store promotion, executes no repository build/test action, and uses no credential. All later canonical build/test/package actions run with `network.client` denied and zero undeclared host input. An unadmitted lock change cannot acquire in an untrusted-PR run. | BUILD-TEST-008 exact acquisition limits/boundary, minimal-host offline-action, sandbox, and aquery evidence. | SM-03, R-001, R-005. |
| BUILD-FR-009 | The build shall embed the five authoritative `M0-BUILD-FACTS-v1` fields. | Product version, full source revision, configuration, compiler identity/version, and target platform are non-empty and correct in dev and release fixtures; a dirty/uncommitted source state is represented explicitly and cannot masquerade as the clean revision. | BUILD-TEST-009 stamped/unstamped golden fixtures consumed by CLI tests. | G-06, SM-06, R-008. |
| BUILD-FR-010 | The build domain shall publish exactly one `M0-REFENV-v1` supported contract and its validation ADR. | The committed document conforms byte-for-byte to Sections 6.3.1-6.3.4, has one derived `environment_id`, declares Linux, x86-64, every locked Nixpkgs/Bazel/Clang/LLVM/LLD/GCC coordinate, target triple, exact kernel/CPU/libc predicates, and `validated_reference`; the ADR records selection date, compatibility evidence, alternatives, consequences, and rollback. Validation of any maximum-valid contract/observed pair uses at most 64 MiB RSS and 10 seconds. Peak RSS of exactly 64 MiB and completion at exactly 10 seconds are permitted; the first attempted byte above 64 MiB or the first nanosecond after 10 seconds returns `BUILD_REFENV_RESOURCE_LIMIT` before a validated outcome. Unknown schema/operator/fact, malformed canonical bytes, or any failed mandatory outcome is never validated. | BUILD-TEST-010 canonical-schema/ADR validation, fixed-ID golden, reference/mismatch fixtures, and exact-bound/first-over resource goldens for `BUILD_REFENV_RESOURCE_LIMIT`. | G-01, C-03, D-003, R-008, R-009; resolved `q-0003`. |
| BUILD-FR-011 | `nix build .#orus` shall produce and identify the release package through the same Bazel release target used elsewhere. | The package contains one release `orus` executable, required runtime closure, build facts, and governance-required license/notices/SBOM files; package construction invokes no independent compile or link action. It emits distinct `orus_executable_sha256` over raw executable bytes and `package_tree_sha256` over the Section 6.4 manifest; the external release-evidence bundle separately binds `sbom_sha256` and each `evidence_object_sha256`, avoiding circular identity. The bounded package-tree identity walk uses one streaming process, at most 256 MiB RSS, and at most 1,200 seconds. Exactly 256 MiB and 1,200 seconds are permitted; the first attempted byte or nanosecond over either bound returns `BUILD_PACKAGE_IDENTITY_INVALID` before identity acceptance. | BUILD-TEST-011 package-to-Bazel provenance, identity, metadata-mutation, and exact-bound/first-over 256-MiB/1,200-second fixtures. | G-01, G-02, DOD-03, R-005, R-008, R-201. |
| BUILD-FR-012 | Documentation and convenience wrappers shall delegate to the Charter canonical commands. | Each executable wrapper has a machine-readable mapping to one or more canonical Nix+Bazel commands; a scan finds zero wrapper-only build configuration or unsupported-platform claim. | BUILD-TEST-012 documentation/wrapper conformance review. | Charter 7, C-12, R-001, R-009. |

## 5. Non-Functional Requirements

| ID | Quality attribute and target | Conditions / workload | Acceptance criteria | Verification method and evidence |
|---|---|---|---|---|
| BUILD-NFR-001 | Reproducibility: 100% of Charter Section 7 commands exit 0. | Two clean checkouts of one revision in `M0-REFENV-v1`, with the same acquired input-store manifest and sandboxing enabled. | Every command succeeds in both runs; `orus_executable_sha256` and `package_tree_sha256` are identical; zero lockfile mutation occurs. A timestamp-only fixture leaves `package_tree_sha256` unchanged, while a path, mode, symlink-target, or non-executable-file change changes it. | BUILD-BENCH-001 clean-build JSON report, command logs, subject-named digests, and metadata-mutation matrix. |
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
set. Before them, the named `m0_input_acquisition` profile may use
`network.client` only to materialize committed, admitted, digest-pinned lock
entries into a read-only store. Its manifest is bounded to 128 coordinates,
each fetched blob to 4 GiB, total fetched bytes to 16 GiB, and wall time to
1,200 seconds; a bound excess is `BUILD_ACQUISITION_DENIED` before promotion.
It has no credential, repository-write,
build-action, cache-privileged-write, or publication capability. Repository
build/test/package actions begin only after acquisition succeeds and are
network-denied. Interruption may leave only a quarantined partial download or
tool-owned caches and declared output trees; a partial or hash-mismatched input
is never promoted to the store or interpreted as a package.

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

#### 6.3.1 `M0-CANONICAL-JSON-v1`

All cross-domain M0 JSON identities in specs `10`-`16` use this byte profile.
Input is UTF-8 without BOM. Duplicate object names, invalid UTF-8, lone
surrogates, non-NFC strings, floating-point/exponent numbers, integers outside
signed 64-bit, and unknown fields are rejected before identity calculation.
Object names are sorted by unsigned UTF-8 byte sequence; array order is
preserved. Emit no whitespace; use `,` and `:` separators. Integers are base-10
with no leading zero, explicit plus, or negative zero. Emit `true`, `false`, and
`null` literally. Strings use `\"` and `\\`; U+0008/0009/000A/000C/000D use
`\b`, `\t`, `\n`, `\f`, `\r`; other U+0000-U+001F scalars use lowercase
`\u00xx`; every other Unicode scalar is emitted directly as UTF-8. A canonical
document has no trailing LF. Output tools may append exactly one LF only when a
CLI contract explicitly says the LF is outside the identified document.

#### 6.3.2 Reference contract fields

The document is at most 64 KiB, nesting depth at most 8, and contains exactly
the following fields. `hex64` means `[0-9a-f]{64}`; `token` means NFC ASCII
`[a-z0-9][a-z0-9._-]{0,63}`.

| Path | Type / required bound | Rule |
|---|---|---|
| `schema` | literal `M0-REFENV-v1` | Required; another value is `BUILD_REFENV_SCHEMA_UNKNOWN`. |
| `environment_id` | `hex64` | Required; SHA-256 of canonical top-level object with this member removed. |
| `nix_system` | string, 1-64 bytes | Exactly `x86_64-linux` for M0. |
| `support_level` | enum | Exactly `validated_reference`. |
| `target_triple` | string, 1-128 bytes | Exact locked release target triple. |
| `inputs[]` | array, 1-128 unique rows | Row fields are `name:token`, `coordinate:string[1..256]`, and `sha256:hex64`; rows sorted by `name`; includes every locked Nix input. |
| `tools.{bazel,clang,llvm,lld,gcc}` | five required objects | Each has exactly `version:string[1..64]` and `artifact_sha256:hex64`. |
| `host.os_family`, `host.architecture`, `host.kernel_release`, `host.cpu_vendor`, `host.libc_name`, `host.libc_version` | predicate object | Exactly `{op:"eq",expected:string[1..256]}`; byte-for-byte NFC string equality. |
| `host.cpu_family`, `host.cpu_model` | predicate object | Exactly `{op:"u32_eq",expected:uint32}`; unsigned numeric equality. |
| `host.required_isa` | predicate object | Exactly `{op:"set_contains_all",expected:token[1..128]}`; expected tokens sorted/unique; observed set passes iff every expected token exists, with case-sensitive token equality. Extra observed ISA tokens do not broaden the support claim. |

No other operator or field is accepted. Predicates are combined with logical
AND; unavailable observed data fails its owning predicate.

#### 6.3.3 Observed facts and validation result

`M0-REFENV-OBSERVED-v1` is a maximum-32-KiB canonical JSON object containing
`schema`, `embedded_environment_id:hex64`, and `facts`. `facts` contains the
same nine leaf names under `host` as the table above: strings for the six
`eq` fields, uint32 for family/model, and a sorted unique token array for ISA.
Each leaf is either its declared type or `null` for unavailable; no host label,
distro inference, or derived environment hash is substituted.

`M0-REFENV-VALIDATION-v1` contains `schema`, `contract_environment_id`,
`embedded_environment_id`, `overall:validated_reference|unvalidated`, and
exactly nine ordered outcomes. Each outcome has `path`, `operator`, `expected`,
`observed` (typed value or `null`), `status:pass|mismatch|unavailable`, and
`code`. The environment is validated iff the recomputed contract ID equals
`environment_id`, the embedded ID equals it, and all nine outcomes pass.
Schema/canonicalization/operator errors return one of
`BUILD_REFENV_SCHEMA_UNKNOWN`, `BUILD_REFENV_NONCANONICAL`,
`BUILD_REFENV_FIELD_INVALID`, `BUILD_REFENV_OPERATOR_UNKNOWN`, or
`BUILD_REFENV_ID_MISMATCH`; a valid contract with a failed fact returns
`BUILD_UNVALIDATED_ENVIRONMENT`. Exceeding the validator's 64-MiB peak-RSS or
10-second wall-time bound returns `BUILD_REFENV_RESOURCE_LIMIT`; the diagnostic
names `resource` as either `rss_bytes` or `wall_time_ns`, the exact limit in bytes or
nanoseconds, and the observed/requested value. Diagnostics otherwise name
`path`, expected type/value, observed type/value, and the 64-KiB/128-row bound
as applicable.

#### 6.3.4 Fixed examples

The following is a schema fixture, not the selected production coordinate set.
Removing `environment_id`, canonicalizing the remaining object, and hashing it
must yield
`8eb5cc0dcbec4255bb8250b59870fa851273324d1e70a2dc835028027d82da94`.

```json
{"environment_id":"8eb5cc0dcbec4255bb8250b59870fa851273324d1e70a2dc835028027d82da94","host":{"architecture":{"expected":"x86_64","op":"eq"},"cpu_family":{"expected":6,"op":"u32_eq"},"cpu_model":{"expected":85,"op":"u32_eq"},"cpu_vendor":{"expected":"GenuineIntel","op":"eq"},"kernel_release":{"expected":"6.12.0-fixture","op":"eq"},"libc_name":{"expected":"glibc","op":"eq"},"libc_version":{"expected":"2.40","op":"eq"},"os_family":{"expected":"linux","op":"eq"},"required_isa":{"expected":["sse2"],"op":"set_contains_all"}},"inputs":[{"coordinate":"fixture:nixpkgs","name":"nixpkgs","sha256":"aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"}],"nix_system":"x86_64-linux","schema":"M0-REFENV-v1","support_level":"validated_reference","target_triple":"x86_64-unknown-linux-gnu","tools":{"bazel":{"artifact_sha256":"bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb","version":"fixture-1"},"clang":{"artifact_sha256":"cccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccc","version":"fixture-1"},"gcc":{"artifact_sha256":"dddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddd","version":"fixture-1"},"lld":{"artifact_sha256":"eeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeee","version":"fixture-1"},"llvm":{"artifact_sha256":"ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff","version":"fixture-1"}}}
```

A mismatch fixture changes observed `host.kernel_release` to
`6.13.0-fixture`; only that predicate is `mismatch`, and overall is
`unvalidated`. Changing `environment_id`, member order, or whitespace is an
identity/canonicalization error rather than a host mismatch.

### 6.4 `M0-CONTENT-IDENTITY-v1`

Subject names and bytes are fixed across build, governance, and security:

| Field | Canonical bytes |
|---|---|
| `orus_executable_sha256` | SHA-256 of the regular `bin/orus` file bytes only. File name, mode, and other package metadata are excluded. |
| `package_tree_sha256` | SHA-256 of a canonical `M0-PACKAGE-TREE-v1` manifest. Walk the package root without following symlinks; normalize relative paths to NFC `/`-separated strings with no empty, `.`, `..`, absolute, or duplicate component; sort entries by unsigned UTF-8 path bytes. A regular-file row is exactly `path`, `kind=file`, POSIX permission bits `mode` as uint16 with file-type bits removed, `size` as non-negative signed-64 integer, and raw-file `sha256`; a symlink row is `path`, `kind=symlink`, `mode`, and exact NFC `target` bytes. Directories are represented with `path`, `kind=directory`, and `mode`. UID, GID, mtime/ctime/atime, xattrs, inode, Nix-store prefix, and traversal order are excluded. |
| `sbom_sha256` | SHA-256 of the completed canonical SPDX 2.3 JSON bytes; stored only in its external descriptor/evidence, never inside the hashed SBOM. |
| `evidence_object_sha256` | SHA-256 of the exact retained evidence-object bytes after that object's declared canonicalization; its reference also carries schema, relative path, and byte length. |

The package manifest is bounded to 100,000 entries, 4-KiB paths/targets, 16
GiB summed regular-file bytes, and a 256-MiB streaming hasher RSS. Bound excess,
special file types, hard-link ambiguity, path escape, or read mutation returns
`BUILD_PACKAGE_IDENTITY_INVALID` before release approval. The canonical fixture
`{"entries":[{"kind":"file","mode":420,"path":"LICENSE","sha256":"bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb","size":3},{"kind":"file","mode":493,"path":"bin/orus","sha256":"aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa","size":4}],"schema":"M0-PACKAGE-TREE-v1"}`
has `package_tree_sha256`
`6827b44d648423f6b79c38a5700fe2551feb45594630b742b6f70b3cfa444f40`.
A fixture that changes only mtime preserves this digest; changing `LICENSE`
mode while leaving `bin/orus` bytes unchanged preserves
`orus_executable_sha256` and changes `package_tree_sha256`.

### 6.5 Design choice: environment/build ownership

| Alternative | Pros | Cons |
|---|---|---|
| Nix environment/package plus Bazel source graph (recommended) | One owner per layer; matches local, CI, benchmark, and release paths; binding D-004 choice. | Requires deliberate Nix-to-Bazel toolchain registration. |
| Nix-only source build | One tool at the top level. | Violates Bazel ownership and loses the required scalable target graph. |
| Bazel-only environment and packaging | One build graph. | Weakens pinned host/package ownership and conflicts with D-004. |

**Recommendation:** Apply the first alternative exactly. This is not an open
choice: D-004 is Accepted.

### 6.6 Design choice: initial reference-environment coordinates

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

### 6.7 Design choice: dependency acquisition boundary

| Alternative | Pros | Cons |
|---|---|---|
| Separate hash-verifying acquisition profile, then network-denied actions (recommended) | Supports a clean clone with missing inputs while preserving offline hermetic build/test actions and one auditable network grant. | Requires a store manifest and explicit phase handoff. |
| Require a fully prepopulated store before the gate | Simplest zero-network execution and strongest offline precondition. | Moves acquisition evidence outside the reproducible clean-clone workflow. |
| Permit Nix/Bazel actions to fetch as needed | Minimal orchestration. | Network becomes an undeclared action input and violates R-001 hermeticity evidence. |

**Recommendation:** Use only `m0_input_acquisition` with the exact admission,
hash, capability, and resource restrictions above; all source actions consume
its read-only store with network denied.

## 7. Data Model

| Entity | Identity and lifecycle | Invariants / retention |
|---|---|---|
| Locked input set | Lowercase hexadecimal SHA-256 digest of committed Nix and Bzlmod locks; changes only by reviewed dependency update. | Resolution does not mutate locks during a gate run; retained in source history. |
| Toolchain | Bazel toolchain target plus content-addressed Nix inputs. | Compiler/linker identity in actions equals embedded build facts. |
| Build configuration | Stable enum plus normalized-settings SHA-256 digest. | Same name cannot silently change toolchain family or C++ level. |
| Reference environment | `environment_id` from `M0-REFENV-v1`. | Exactly one M0 supported identity; actual observed facts are separate from expected predicates. |
| Build artifact | `orus_executable_sha256` over raw executable bytes plus `M0-BUILD-FACTS-v1`. | Never conflated with the package tree; a digest is not a signature. |
| Package | `package_tree_sha256` over `M0-PACKAGE-TREE-v1`. | Paths, kinds, modes, sizes, symlink targets, and file content participate exactly as Section 6.4 defines; timestamps/owners do not. |
| Build evidence | Source revision, command ID, exit status, action graph digest, logs, and subject-named `orus_executable_sha256`, `package_tree_sha256`, `sbom_sha256`, or `evidence_object_sha256`. | Immutable for a gate run and retained with M0 release evidence. |

Build facts are authoritative evidence produced from declared inputs. Human
labels, local filenames, and observed success on another host are advisory and
cannot change support state.

## 8. Key Flows

1. **Clean source-to-release flow (BUILD-FR-001 through BUILD-FR-012).**
   The operator checks out a clean revision and locks; Nix resolves
   `M0-REFENV-v1`; Bazel registers the declared toolchain; canonical commands
   execute sandboxed; Bazel produces the release binary and facts; Nix packages
   that exact output; governance validation attaches release evidence; the
   clean-build report records all command results and subject-named SHA-256
   digests.
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
| BUILD-FAIL-002 | Acquisition requests an unadmitted/mutable/hash-mismatched input, or a build action resolves a host input or attempts network. [R-001, R-005] | Acquisition verifier before store promotion, or sandbox access boundary. | `BUILD_ACQUISITION_DENIED` or `BUILD_UNDECLARED_INPUT`; profile/action, coordinate, expected/observed digest, path/capability, toolchain. | Partial download quarantined/deleted; failing action output discarded; no publication. | Admit/pin the input or remove use; build actions never receive a network exception. | BUILD-FR-002, -003, -008 / BUILD-TEST-008. |
| BUILD-FAIL-003 | CMake or alternate source action is found. [R-001] | Repository/workflow scan before build gate. | `BUILD_PROHIBITED_PATH`; file, line/action, matched rule. | Gate stops; no package is accepted. | Remove path; dependency exception requires superseding owner-approved decision if it changes D-004. | BUILD-FR-007 / BUILD-TEST-007. |
| BUILD-FAIL-004 | Named config silently falls back or executes no required target. [R-007] | Config/applicability reconciliation. | `BUILD_CONFIG_INVALID`; config, target, selected toolchain, expected status. | No passing CI result. | Fix registration/matrix and rerun. | BUILD-FR-006 / BUILD-TEST-006. |
| BUILD-FAIL-005 | Embedded metadata is empty, wrong, or dirty release is attempted. [R-008] | Artifact validation before packaging. | `BUILD_METADATA_INVALID`; field, expected source, observed value. | Binary may remain a local output; package/publication rejected. | Rebuild from declared clean state. | BUILD-FR-009, -011 / BUILD-TEST-009. |
| BUILD-FAIL-006 | Actual platform does not satisfy `M0-REFENV-v1`. [R-009] | Environment validation before supported use. | `BUILD_UNVALIDATED_ENVIRONMENT`; contract ID, predicate, expected, observed, recoverable=true. | No supported or authoritative result; diagnostic retained. | Move to reference environment or proceed only as explicitly unvalidated. | BUILD-FR-010 / BUILD-TEST-010. |
| BUILD-FAIL-007 | Build/package is cancelled or partially written. | Process interruption and package finalization. | `BUILD_CANCELLED`; command ID, phase, exit/signal, partial-output path. | Partial package is not committed; owned temporary output cleaned on next run. | Safe automatic retry from declared inputs. | BUILD-FR-011 / BUILD-TEST-011. |
| BUILD-FAIL-008 | Reference JSON/ID/predicate is invalid, its validator exceeds 64 MiB RSS or 10 seconds, or package identity exceeds bounds/contains an unsupported entry. [R-005, R-008, R-009, R-201] | Canonical contract validation or package-tree walk before support/release acceptance. | Section 6.3 schema/content code, exactly `BUILD_REFENV_RESOURCE_LIMIT` for either reference-validator RSS/deadline excess, or `BUILD_PACKAGE_IDENTITY_INVALID`; path/operator/resource, limit, expected/observed. | No validated support state or accepted package identity; bounded temporary buffers released. | Correct/regenerate from declared inputs; never relabel mismatched bytes. | BUILD-FR-010, -011 / BUILD-TEST-010, -011. |

## 10. Observability

| Signal ID | Metric/event/log | Type, unit, and labels | Emission point / owner | Cardinality and cost bound | Alert/gate use | Verification |
|---|---|---|---|---|---|---|
| BUILD-OBS-001 | `build_command_result` | Event; command ID, config, revision, exit code, duration_ms | Canonical command wrapper / build owner | At most one start and terminal event per command; no per-compiler-input logs. | SM-01 gate and diagnosis. | BUILD-TEST-002. |
| BUILD-OBS-002 | `build_action_input_audit` | Counter/report; declared, undeclared, network_attempt counts | Post-build audit / build owner | One aggregate per action mnemonic and failing label; target-label cardinality bounded by Bazel graph. | SM-03 gate. | BUILD-TEST-008. |
| BUILD-OBS-003 | `build_environment_validation` | Event; contract ID, predicate, expected, observed, support level | Environment validator / build owner | One row per `M0-REFENV-v1` predicate; at most 128. | C-03/release gate. | BUILD-TEST-010. |
| BUILD-OBS-004 | `build_artifact_identity` | Event; subject enum, `orus_executable_sha256`/`package_tree_sha256`, facts schema, revision, config | Package finalization / release owner | Exactly one executable and one package-tree identity per package; no source paths or secrets. | DOD-03 provenance. | BUILD-TEST-011. |
| BUILD-OBS-005 | `build_prohibited_path_findings` | Gauge/report; finding count by stable rule | Pre-gate scanner / build owner | Stable rule cardinality at most 64; zero required. | SM-02 gate. | BUILD-TEST-007. |

Build evidence correlates by source revision, build configuration, reference
environment ID, action-graph digest, `orus_executable_sha256`, and
`package_tree_sha256`. M0 has no trace, branch, execution, VPID, or VTID
identity. Raw command logs, normalized action audit, and subject-named digests
are retained in the M0 gate evidence bundle.

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
| BUILD-FR-008, BUILD-NFR-002 | BUILD-TEST-008 | End-to-end/security | Admitted acquisition, unadmitted/hash-mismatch fixtures, then prefetched store with network-denied sandbox-debug action set. | Only the named acquisition profile fetches and verifies; invalid inputs never enter store; build actions have zero undeclared input/network attempt. | `acquisition-boundary.json`, `hermeticity-report.json`. |
| BUILD-FR-009 | BUILD-TEST-009 | Unit/golden | Dev, release, dirty, and missing-field metadata fixtures. | Values exact; invalid/dirty release fixture rejected. | `build-facts-junit.xml`. |
| BUILD-FR-010, BUILD-NFR-004 | BUILD-TEST-010 | Schema/integration/golden/resource | Fixed canonical example; reference plus schema/operator/ID/noncanonical and each observed-fact mismatch fixture; maximum-valid fixtures at exactly 64 MiB peak RSS and exactly 10 seconds; first-over fixtures attempting 64 MiB + 1 byte and 10 seconds + 1 nanosecond. | Fixed ID matches; reference has nine passes at either exact resource bound; malformed inputs return their Section 6.3 content code; both first-over resource fixtures return exactly `BUILD_REFENV_RESOURCE_LIMIT` before any validation outcome; each fact mismatch is unvalidated with exact path/outcome. | `reference-environment-report.json`, canonical-ID and resource goldens. |
| BUILD-FR-011 | BUILD-TEST-011 | End-to-end/resource/identity | Two clean packages; mtime, mode, path, symlink, special-file, 100,001-entry, and 16-GiB-bound fixtures; package walks at exactly 256 MiB peak RSS and 1,200 seconds; first-over walks at 256 MiB + 1 byte and 1,200 seconds + 1 nanosecond. | Required contents and both subject digests match across clean builds; mtime is excluded; included metadata/content changes only the specified identity; both exact-bound walks accept; invalid and both first-over walks return exactly `BUILD_PACKAGE_IDENTITY_INVALID` before identity acceptance. | `package-provenance.json`, `package-identity-matrix.json`. |
| BUILD-FR-012 | BUILD-TEST-012 | Inspection | Docs and every executable convenience wrapper. | 100% map to canonical commands; zero independent action. | `wrapper-map.json`. |
| BUILD-NFR-001 | BUILD-BENCH-001 | End-to-end | Two clean checkouts; complete Charter command sequence and identity metadata fixtures. | 100% command success, no lock mutation, matching executable/package-tree digests, exact inclusion/exclusion behavior. | `reproducibility-report.json`. |
| BUILD-NFR-005 | BUILD-TEST-013 | Analysis | Bazel query result against M0 applicability inventory. | Every target appears exactly once and every required config executes. | `build-applicability-reconciliation.json`. |

## 12. Open Questions

No open questions.
