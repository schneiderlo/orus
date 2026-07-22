# ADR-0001: M0 build toolchain, dependency stack, and reference environment

- Status: Accepted for M0-001 implementation
- Selection date: 2026-07-21
- Owners: build/toolchain owner and release engineer
- Decisions applied: D-003, D-004, D-011, q-0003, q-0012

## Decision

Use the immutable NixOS 26.05 `nixpkgs` revision
`fd1462031fdee08f65fd0b4c6b64e22239a77870`. It supplies Bazel 8.6.0,
Clang/LLVM/LLD 21.1.8, GCC 15.2.0, Python 3.14.6, OpenSSL 3.6.3, and
utf8proc 2.11.3. Bazel 8 was selected because the pinned rule/test modules
declare Bazel 8 compatibility and the stable Nixpkgs branch supplies it as one
tested environment closure.

Pin Glaze v7.5.0, GoogleTest v1.17.0, and Google Benchmark v1.9.5 by exact
Git revisions and NAR SHA-256 identities in `flake.lock`. Nix alone fetches
their source trees. Bzlmod repository rules expose only the source portions
needed by Bazel and supply Orus-owned build definitions, so no upstream build
script becomes a repository, developer, or CI build path. Glaze remains
behind the M0-002-owned canonicalization boundary; this task only proves that
the exact header set compiles under both selected compilers.

The complete direct inventory and artifact hashes are machine-readable in
`config/dependency-inventory.json`. Nix inputs and package outputs use the
lowercase hexadecimal representation of their locked NAR SHA-256; Bzlmod
archives use their locked archive SHA-256. Bootstrap admission reasoning is in
`third_party/bootstrap-admissions.json`. `MODULE.bazel.lock` retains the
resolved Bzlmod graph and registry metadata hashes.

## Compatibility evidence

The selected set is accepted only as one compatibility unit. The gate runs:

- flake evaluation and bootstrap contract checks;
- a lock-stable Bzlmod graph;
- C++23 and Glaze compilation with Clang 21.1.8/LLD 21.1.8;
- the same task tests with GCC 15.2.0;
- GoogleTest and Google Benchmark Bazel targets;
- ASan and UBSan configurations; and
- exact tool-version, Nix-store-path, acquisition-boundary, prohibited-path,
  and package-scoped coverage audits.

The production `M0-REFENV-v1` document records the validated WSL2 reference
host used for selection. That is one exact Linux x86-64 environment, not a
distribution, kernel-family, CPU-family, container, or broad WSL support
claim.

## Alternatives

- Bazel 9.1.0 was current, but the selected dependency modules explicitly
  advertise Bazel 8 compatibility and NixOS 26.05 packages Bazel 8.6.0 as a
  coherent stable environment.
- Host compilers and Bazelisk were rejected because they escape Nix ownership.
- Boost.JSON, nlohmann/json, and in-tree Unicode/cryptographic primitives were
  rejected by the owner-selected q-0012 Glaze-centered profile.
- Upstream build workflows were rejected because D-004 permits one Bazel
  source graph only.

## Consequences

The exact reference contract is narrow, the initial Nix closure is sizable,
and every pin update must be reviewed and revalidated as one unit. GCC is a
compatibility lane and does not replace Clang/LLD as the primary toolchain.
The bootstrap admissions are intentionally data records; M0-004 later adds
automated dependency/SBOM/notice reconciliation without taking ownership of
these selected pins.

## Rollback

Revert `flake.nix`, `flake.lock`, `MODULE.bazel`, `MODULE.bazel.lock`,
`.bazelversion`, `.bazelrc`, the inventory/admissions/reference bytes, and the
associated action evidence together. Never relabel old bytes with a new
version. A replacement set must rerun the complete compatibility matrix and
derive a new `environment_id` before it can be called validated.
