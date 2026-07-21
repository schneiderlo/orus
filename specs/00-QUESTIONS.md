# Discovery Questions

This log records the live Nemawashi discovery conversation for the current
factory run. The authoritative starting context is `specs/PROJECT-SEED.md`.

## Round 1 — First-wave boundaries and commercial posture

### Q-D01 — What should this factory wave deliver?

- **Why it matters:** The seed describes the full Orus roadmap through M11, but
  its immediate instruction is to begin with M0. Treating the whole roadmap as
  one implementation wave would make planning, verification, and performance
  gates too broad to be credible.
- **What it unlocks:** The specification boundary, acceptance criteria, task
  decomposition, and the point at which this run may truthfully declare done.
- **Options:**
  1. **M0 only (recommended):** deliver the commercial repository, hermetic
     Nix+Bazel build, engineering policy, benchmark foundation, and controlled
     concurrent example programs; make no deterministic recording claim.
  2. **M0 and M1:** also deliver the Linux process-tree/thread-control core in
     this wave; larger scope and a substantially later verification gate.
  3. **Through the first end-to-end slice (M0–M3):** deliver recording and
     deterministic replay now; highest integration risk and least useful as a
     single factory wave.
- **Declared default if unanswered:** M0 only, as directed by the seed.
- **User answer:** Begin with M0.
- **Status:** Answered by user; M0 only.

### Q-D02 — What distribution and license posture applies during M0?

- **Why it matters:** M0 calls for license/SBOM/dependency policy, while the seed
  explicitly forbids inventing a public project license. Distribution and
  outside contribution rights affect dependency admission and release outputs.
- **What it unlocks:** The exact license-decision placeholder, contributor
  guidance, packaging claims, and which release/distribution checks are valid.
- **Options:**
  1. **Private evaluation; defer the license (recommended):** add no `LICENSE`,
     accept no outside contributions, and produce no public redistribution
     until the owner makes a separate license decision.
  2. **Proprietary commercial:** document that distribution is governed by a
     separate commercial agreement; the exact legal text must still come from
     the owner or counsel.
  3. **Open-source intent:** choose a specific license in discovery or park that
     legal decision as blocking before public distribution.
- **Declared default if unanswered:** Private evaluation; no license invented,
  no public distribution, and no outside contributions.
- **User answer:** It will be MIT.
- **Status:** Answered by user; use the MIT License.

### Q-D03 — What host compatibility claim should M0 make?

- **Why it matters:** Linux tracing, `ptrace`, pidfds, seccomp, perf counters,
  and sandbox behavior vary by kernel and host policy. A vague "Linux x86-64"
  promise cannot support reproducible CI or future record/replay claims.
- **What it unlocks:** The initial CI runner definition, `orus doctor` checks,
  test skip/fail policy, and the boundary between a tested baseline and merely
  aspirational compatibility.
- **Options:**
  1. **Pinned reference environment only (recommended for M0):** test one
     Nix-pinned Linux x86-64 CI/benchmark environment, record its kernel/CPU
     facts, and defer a public compatibility matrix to an ADR with evidence.
  2. **One named enterprise distribution:** designate a specific distro and
     kernel family as the initial supported contract and test it continuously.
  3. **Multi-distribution matrix now:** support multiple distro/kernel families
     in M0; broader confidence at significantly greater CI and diagnosis cost.
- **Declared default if unanswered:** One pinned reference environment; no
  broader kernel or distro compatibility claim in M0.
- **User answer:** Default is fine.
- **Status:** Answered by user; use one pinned reference environment and make
  no broader compatibility claim in M0.

## Round 2 — Governance, performance operations, and gate confirmation

### Q-D04 — How should outside contributions be governed under MIT?

- **Why it matters:** MIT establishes the code license but does not establish
  contribution provenance or the review agreement. Orus has a commercial
  posture, so unclear contribution ownership becomes a support and legal risk.
- **What it unlocks:** The M0 contribution guide, pull-request checks, commit
  sign-off policy, and whether outside patches may be accepted.
- **Options:**
  1. **MIT plus DCO sign-off (recommended):** accept outside contributions
     with Developer Certificate of Origin sign-off and documented review rules;
     lightweight and automatable, without inventing a custom legal agreement.
  2. **Closed during M0:** publish under MIT but do not accept outside
     contributions until a later governance decision.
  3. **Contributor License Agreement:** require a CLA before accepting changes;
     stronger explicit rights handling but requires owner-approved legal text
     and a signing workflow.
- **Declared default if unanswered:** Outside contributions remain closed during
  M0; do not invent a DCO or CLA obligation on the owner's behalf.
- **User answer:** Option 2.
- **Status:** Answered by user; outside contributions remain closed during M0.

### Q-D05 — Where should M0 performance results be authoritative?

- **Why it matters:** Shared CI runners are too noisy for a 3% regression gate,
  while M0 still requires a reproducible baseline format and runner policy.
  The repository is hosted on GitHub but has no CI workflow yet.
- **What it unlocks:** CI topology, which benchmark checks block changes, the
  baseline provenance schema, and what M0 may claim about performance.
- **Options:**
  1. **GitHub CI plus a controlled-runner contract (recommended):** run
     functional, sanitizer, and advisory benchmark smoke checks in GitHub
     Actions; define the dedicated runner contract and treat only controlled
     measurements as authoritative once such a runner is provisioned.
  2. **Provision a self-hosted performance runner in M0:** make controlled
     regression checks blocking immediately; requires stable hardware and
     operational ownership now.
  3. **Use another named CI/benchmark service:** specify the provider and its
     stable performance environment before the M0 plan is generated.
- **Declared default if unanswered:** GitHub Actions for functional/advisory
  work, with an explicit controlled-runner contract and no precise performance
  claim from shared-runner measurements.
- **User answer:** Default is acceptable.
- **Status:** Answered by user; use GitHub Actions for functional and advisory
  checks, with authoritative performance results reserved for a controlled
  runner.

### Q-D06 — Are the remaining seed assumptions acceptable for M0?

- **Why it matters:** The discovery gate requires explicit confirmation of the
  assumptions log; otherwise downstream specifications may silently reinterpret
  foundational constraints.
- **What it unlocks:** Completion of discovery and handoff to the specification
  writer once Q-D04 and Q-D05 are also resolved.
- **Assumptions to confirm:** C++23; Bazel+Bzlmod and Nix flakes; no CMake;
  multi-process/multi-thread architecture and example workloads; correctness
  and performance-by-design as co-equal requirements; fail-closed unsupported
  behavior; recordings and verified replay as the source of execution truth;
  models optional and provider-independent.
- **Options:**
  1. **Confirm all (recommended):** accept A-D04 through A-D08 as written.
  2. **Confirm with corrections:** identify any assumption IDs and replacement
     wording.
  3. **Defer confirmation:** discovery remains open and downstream work does not
     start.
- **Declared default if unanswered:** Not confirmed; discovery remains open.
- **User answer:** Recommended option.
- **Status:** Answered by user; A-D04 through A-D08 are confirmed as written.
