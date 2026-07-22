# M0 build entry points

The only supported environment for this wave is the exact
`M0-REFENV-v1` contract in `config/m0-reference-environment.json`. Other
Linux x86-64 environments are unvalidated, even when a command happens to
succeed.

Enter and use the pinned environment through the Charter commands:

```text
nix flake check
nix develop --command bazel build --config=dev //...
nix develop --command bazel test --config=dev //...
nix develop --command bazel test --config=asan //...
nix develop --command bazel test --config=ubsan //...
nix develop --command bazel build --config=release //...
nix develop --command bazel test --config=benchmark //tests/benchmarks/...
nix develop --command bazel run //tools:format
```

The `gcc`, `tsan`, and `fuzz` configurations are explicit additional M0
entry points. Their required or scoped not-applicable behavior is recorded in
`config/build-applicability.json`; they do not establish an independent build graph. The compiler
wrappers fail unless their selected tools are exact Nix-store executables.
Linked test and application binaries embed the pinned GCC runtime-store path,
so they do not fall back to a host `libstdc++`.

Dependency source materialization is limited to the
`m0_input_acquisition` profile. Build, test, benchmark, and audit actions run
with `ORUS_NETWORK_ALLOWED=0` and Bazel's sandbox network default disabled; no
credential is part of the flake. The build test suite includes a negative
fixture that confirms its action has no external interface or default route.
