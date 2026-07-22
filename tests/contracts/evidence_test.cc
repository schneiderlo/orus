#include "orus/contracts/contracts.h"

#include "tests/contracts/test_support.h"

#include <algorithm>
#include <array>
#include <cstdlib>
#include <limits>
#include <map>
#include <set>
#include <string>

#include <gtest/gtest.h>

namespace orus::contracts {
namespace {

JsonValue& MutableMember(JsonValue& value, std::string_view name) {
  for (auto& [candidate, member] : std::get<JsonValue::Object>(value.value)) {
    if (candidate == name) return member;
  }
  std::abort();
}

std::string Canonical(const JsonValue& value, std::string_view schema) {
  return EmitCanonicalJson(value, schema).value();
}

void RecomputeDerived(JsonValue& value, std::string_view member, std::string_view schema) {
  JsonValue::Object without;
  for (const auto& [name, row] : std::get<JsonValue::Object>(value.value)) {
    if (name != member) without.emplace_back(name, row);
  }
  MutableMember(value, member) = JsonValue(Sha256(Canonical(JsonValue(std::move(without)), schema))->Hex());
}

struct GovernanceBundleFixture {
  JsonValue manifest;
  std::vector<ReferencedDocument> documents;
};

void RefreshEvidenceReference(GovernanceBundleFixture& fixture, std::string_view path) {
  auto& rows = std::get<JsonValue::Array>(MutableMember(fixture.manifest, "evidence").value);
  const auto document = std::find_if(
      fixture.documents.begin(), fixture.documents.end(),
      [&](const ReferencedDocument& candidate) { return candidate.path == path; });
  ASSERT_NE(document, fixture.documents.end());
  const auto row = std::find_if(rows.begin(), rows.end(), [&](JsonValue& candidate) {
    return *FindMember(candidate, "path")->AsString() == path;
  });
  ASSERT_NE(row, rows.end());
  MutableMember(*row, "byte_length") = JsonValue(static_cast<std::int64_t>(document->bytes.size()));
  MutableMember(*row, "evidence_object_sha256") = JsonValue(Sha256(document->bytes)->Hex());
}

GovernanceBundleFixture CompleteGovernanceBundle() {
  GovernanceBundleFixture fixture{
      .manifest = ParseCanonicalJson(
          test::Read("tests/contracts/fixtures/release-evidence-assembled.json"),
          "M0-RELEASE-EVIDENCE-v1", "GOV_NONCANONICAL")
                      .value(),
  };
  MutableMember(fixture.manifest, "state") = JsonValue("preapproval_validated");

  const auto source_revision = *FindMember(fixture.manifest, "source_revision")->AsString();
  const auto executable_digest = *FindMember(fixture.manifest, "orus_executable_sha256")->AsString();
  const auto package_digest = *FindMember(fixture.manifest, "package_tree_sha256")->AsString();
  const auto spdx_namespace =
      std::string("https://spdx.orus.invalid/m0/") + source_revision + "/" + executable_digest;
  const auto spdx_bytes = Canonical(
      JsonValue::Object{
          {"SPDXID", "SPDXRef-DOCUMENT"},
          {"documentNamespace", spdx_namespace},
          {"packages", JsonValue::Array{JsonValue(JsonValue::Object{{"SPDXID", "SPDXRef-Package-fixture"}})}},
          {"relationships", JsonValue::Array{JsonValue(JsonValue::Object{{"relationshipType", "DESCRIBES"}})}},
          {"spdxVersion", "SPDX-2.3"},
      },
      "SPDX-2.3");
  const auto sbom_digest = Sha256(spdx_bytes)->Hex();
  MutableMember(fixture.manifest, "sbom_sha256") = JsonValue(sbom_digest);

  auto descriptor = ParseCanonicalJson(
      test::Read("tests/contracts/fixtures/sbom-descriptor.json"),
      "M0-SBOM-CONTRACT-v1", "GOV_NONCANONICAL")
                        .value();
  MutableMember(descriptor, "document_namespace") = JsonValue(spdx_namespace);
  MutableMember(descriptor, "orus_executable_sha256") = JsonValue(executable_digest);
  MutableMember(descriptor, "sbom_sha256") = JsonValue(sbom_digest);
  const auto descriptor_bytes = Canonical(descriptor, "M0-SBOM-CONTRACT-v1");

  const std::array<std::pair<std::string_view, std::string_view>, 12> evidence_contracts{{
      {"adr_approval", "M0-ADR-APPROVAL-v1"},
      {"build_facts", "M0-BUILD-FACTS-v1"},
      {"canonical_commands", "M0-CANONICAL-COMMANDS-v1"},
      {"ci_gate", "M0-CI-GATE-v1"},
      {"claim_scan", "M0-CLAIM-SCAN-v1"},
      {"corpus_reliability", "M0-CORPUS-RELIABILITY-v1"},
      {"license_notice", "M0-LICENSE-NOTICE-v1"},
      {"performance_tooling", "M0-PERFORMANCE-TOOLING-v1"},
      {"reference_environment", "M0-REFENV-v1"},
      {"sanitizer_fuzz", "M0-SANITIZER-FUZZ-v1"},
      {"sbom_descriptor", "M0-SBOM-CONTRACT-v1"},
      {"security_controls", "M0-SECURITY-CONTROLS-v1"},
  }};
  JsonValue::Array evidence;
  for (const auto& [type, schema] : evidence_contracts) {
    const std::string path = std::string("evidence/") + std::string(type) + ".json";
    std::string bytes;
    if (type == "sbom_descriptor") {
      bytes = descriptor_bytes;
    } else if (type == "security_controls") {
      bytes = Canonical(
          JsonValue::Object{
              {"package_tree_sha256", package_digest},
              {"schema", std::string(schema)},
          },
          schema);
    } else {
      bytes = Canonical(JsonValue::Object{{"schema", std::string(schema)}}, schema);
    }
    fixture.documents.push_back({path, bytes});
    evidence.emplace_back(JsonValue::Object{
        {"byte_length", static_cast<std::int64_t>(bytes.size())},
        {"evidence_object_sha256", Sha256(bytes)->Hex()},
        {"path", path},
        {"producer", std::string("tool.") + std::string(type)},
        {"producer_version", "1"},
        {"schema", std::string(schema)},
        {"type", std::string(type)},
    });
  }
  fixture.documents.push_back({"orus.spdx.json", spdx_bytes});
  MutableMember(fixture.manifest, "evidence") = JsonValue(std::move(evidence));

  const std::array<std::string_view, 12> validator_ids{
      "adr_protected_decisions", "build_reference", "canonical_commands", "ci_gate", "claim_scan",
      "corpus_reliability", "dependency_sbom", "license_notice", "performance_tooling", "sanitizer_fuzz",
      "security_controls", "subject_identity"};
  JsonValue::Array validators;
  for (const auto id : validator_ids) {
    validators.emplace_back(JsonValue::Object{
        {"diagnostic", nullptr}, {"finding_count", 0}, {"status", "pass"},
        {"validator_id", std::string(id)}, {"version", "1"},
    });
  }
  MutableMember(fixture.manifest, "validators") = JsonValue(std::move(validators));

  JsonValue::Array approvals;
  for (const auto role : {"product_owner", "release_owner", "security_owner"}) {
    approvals.emplace_back(JsonValue::Object{
        {"decision", "approved"}, {"identity", std::string(role) + ".fixture"}, {"role", role},
        {"time", "2026-07-22T12:00:00Z"},
    });
  }
  MutableMember(fixture.manifest, "approvals") = JsonValue(std::move(approvals));
  return fixture;
}

std::string ComparisonSeed(std::string_view baseline, std::string_view candidate) {
  const auto baseline_digest = Sha256Digest::ParseHex(baseline).value();
  const auto candidate_digest = Sha256Digest::ParseHex(candidate).value();
  constexpr std::string_view prefix = "M0-PAIRED-BOOTSTRAP-v1\n";
  std::vector<std::byte> input;
  for (const char character : prefix) {
    input.push_back(static_cast<std::byte>(static_cast<unsigned char>(character)));
  }
  input.insert(input.end(), baseline_digest.Bytes().begin(), baseline_digest.Bytes().end());
  input.insert(input.end(), candidate_digest.Bytes().begin(), candidate_digest.Bytes().end());
  return Sha256(std::span<const std::byte>(input))->Hex();
}

JsonValue StatisticalComparison(
    std::string_view authority,
    std::int64_t lower_bound,
    std::string_view state,
    std::string_view reason,
    std::string_view action) {
  auto comparison = ParseCanonicalJson(
      test::Read("tests/contracts/fixtures/perf-comparison.json"),
      "M0-PERF-COMPARISON-v1", "PERF_NONCANONICAL")
                        .value();
  MutableMember(comparison, "authority") = JsonValue(std::string(authority));
  MutableMember(comparison, "lower_bound_ppb") = JsonValue(lower_bound);
  MutableMember(comparison, "mismatches") = JsonValue(JsonValue::Array{});
  MutableMember(comparison, "next_action") = JsonValue(std::string(action));
  MutableMember(comparison, "noise_state") = JsonValue("pass");
  MutableMember(comparison, "point_estimate_ppb") = JsonValue(lower_bound);
  MutableMember(comparison, "reason") = JsonValue(std::string(reason));
  MutableMember(comparison, "resamples") = JsonValue(10000);
  MutableMember(comparison, "seed_sha256") = JsonValue(ComparisonSeed(
      *FindMember(comparison, "baseline_result_id")->AsString(),
      *FindMember(comparison, "candidate_result_id")->AsString()));
  const bool fired = lower_bound > 30000000;
  MutableMember(comparison, "significance_fired") = JsonValue(fired);
  MutableMember(comparison, "state") = JsonValue(std::string(state));
  MutableMember(comparison, "threshold_fired") = JsonValue(fired);
  MutableMember(comparison, "valid_pairs") = JsonValue(30);
  RecomputeDerived(comparison, "comparison_id", "M0-PERF-COMPARISON-v1");
  return comparison;
}

void ExpectComparisonRejected(JsonValue comparison) {
  RecomputeDerived(comparison, "comparison_id", "M0-PERF-COMPARISON-v1");
  auto result = ValidatePerformanceDocument(Canonical(comparison, "M0-PERF-COMPARISON-v1"));
  ASSERT_FALSE(result);
  EXPECT_EQ(result.error().code, "PERF_RELATIONSHIP_INVALID");
}

TEST(GovernanceContracts, AcceptsSharedFixturesAndRejectsForgedPreapproval) {
  EXPECT_TRUE(ValidateGovernanceDocument(test::Read("tests/contracts/fixtures/sbom-descriptor.json")));
  const auto assembled = test::Read("tests/contracts/fixtures/release-evidence-assembled.json");
  EXPECT_TRUE(ValidateGovernanceDocument(assembled));
  auto forged = assembled;
  const auto position = forged.find("assembled");
  ASSERT_NE(position, std::string::npos);
  forged.replace(position, std::string("assembled").size(), "preapproval_validated");
  auto invalid = ValidateGovernanceDocument(forged);
  ASSERT_FALSE(invalid);
  EXPECT_EQ(invalid.error().code, "GOV_RELATIONSHIP_INVALID");
}

TEST(GovernanceContracts, UnknownFieldsDigestsAndResourcesFailClosed) {
  auto descriptor = test::Read("tests/contracts/fixtures/sbom-descriptor.json");
  descriptor.insert(descriptor.find("\"generator\""), "\"extra\":1,");
  auto unknown = ValidateGovernanceDocument(descriptor);
  ASSERT_FALSE(unknown);
  EXPECT_EQ(unknown.error().code, "GOV_FIELD_MISSING");
  auto exact = ValidateGovernanceDocument(
      test::Read("tests/contracts/fixtures/sbom-descriptor.json"), {.rss_bytes = 256 * 1024 * 1024, .wall_time_ns = 120000000000ULL});
  EXPECT_TRUE(exact);
  auto over = ValidateGovernanceDocument(test::Read("tests/contracts/fixtures/sbom-descriptor.json"), {.rss_bytes = 256 * 1024 * 1024 + 1ULL});
  ASSERT_FALSE(over);
  EXPECT_EQ(over.error().code, "GOV_RESOURCE_LIMIT");
  EXPECT_EQ(over.error().schema, "M0-GOV-ERROR-v1");
  EXPECT_EQ(over.error().contract, "M0-SBOM-CONTRACT-v1");
  EXPECT_EQ(over.error().record_id, std::optional<std::string>("sbom.fixture.v1"));
  EXPECT_EQ(over.error().field_path, "$");
  EXPECT_EQ(over.error().expected, "peak_rss_bytes");
  EXPECT_EQ(over.error().limit, 256 * 1024 * 1024);
  EXPECT_TRUE(ValidateGovernanceDocument(
      test::Read("tests/contracts/fixtures/sbom-descriptor.json"), {.count = 200000, .depth = 32}));
  auto count_over = ValidateGovernanceDocument(
      test::Read("tests/contracts/fixtures/sbom-descriptor.json"), {.count = 200001});
  ASSERT_FALSE(count_over);
  EXPECT_EQ(count_over.error().code, "GOV_RESOURCE_LIMIT");
  EXPECT_TRUE(ValidateGovernanceDocument(
      test::Read("tests/contracts/fixtures/sbom-descriptor.json"),
      {.input_bytes = 64 * 1024}));
  auto bytes_over = ValidateGovernanceDocument(
      test::Read("tests/contracts/fixtures/sbom-descriptor.json"),
      {.input_bytes = 64 * 1024 + 1ULL});
  ASSERT_FALSE(bytes_over);
  EXPECT_EQ(bytes_over.error().code, "GOV_RESOURCE_LIMIT");
  const std::string oversized_descriptor =
      "{\"padding\":\"" + std::string(64 * 1024, 'x') +
      "\",\"schema\":\"M0-SBOM-CONTRACT-v1\"}";
  auto encoded_over = ValidateGovernanceDocument(oversized_descriptor);
  ASSERT_FALSE(encoded_over);
  EXPECT_EQ(encoded_over.error().code, "GOV_FIELD_BOUND");
}

TEST(GovernanceContracts, CompletePreapprovalAndEveryInventoryCardinalityAreExact) {
  auto fixture = CompleteGovernanceBundle();
  const auto manifest_bytes = Canonical(fixture.manifest, "M0-RELEASE-EVIDENCE-v1");
  EXPECT_TRUE(ValidateGovernanceBundle(manifest_bytes, fixture.documents));

  for (const auto inventory : {"evidence", "validators", "approvals"}) {
    auto incomplete = fixture.manifest;
    std::get<JsonValue::Array>(MutableMember(incomplete, inventory).value).pop_back();
    auto invalid = ValidateGovernanceDocument(Canonical(incomplete, "M0-RELEASE-EVIDENCE-v1"));
    ASSERT_FALSE(invalid) << inventory;
    EXPECT_EQ(invalid.error().code, "GOV_RELATIONSHIP_INVALID") << inventory;

    auto over = fixture.manifest;
    auto& rows = std::get<JsonValue::Array>(MutableMember(over, inventory).value);
    rows.push_back(rows.front());
    auto exceeded = ValidateGovernanceDocument(Canonical(over, "M0-RELEASE-EVIDENCE-v1"));
    ASSERT_FALSE(exceeded) << inventory;
    EXPECT_EQ(exceeded.error().code, "GOV_FIELD_BOUND") << inventory;
  }
}

TEST(GovernanceContracts, ApprovalTimesEnforceUtcRfc3339CalendarAndLeapSeconds) {
  const auto validate = [](std::string_view time) {
    auto fixture = CompleteGovernanceBundle();
    auto& approvals = std::get<JsonValue::Array>(MutableMember(fixture.manifest, "approvals").value);
    MutableMember(approvals.front(), "time") = JsonValue(std::string(time));
    return ValidateGovernanceDocument(Canonical(fixture.manifest, "M0-RELEASE-EVIDENCE-v1"));
  };

  for (const auto time : {
           "2000-02-29T00:00:00Z",
           "2024-02-29T23:59:59Z",
           "2026-04-30T23:59:59Z",
           "2026-12-31T23:59:59Z",
           "2016-12-31T23:59:60Z",
       }) {
    SCOPED_TRACE(time);
    EXPECT_TRUE(validate(time));
  }

  for (const auto time : {
           "1900-02-29T12:00:00Z",
           "2023-02-29T12:00:00Z",
           "2024-02-30T12:00:00Z",
           "2026-02-31T12:00:00Z",
           "2026-04-31T12:00:00Z",
           "2026-00-01T12:00:00Z",
           "2026-01-00T12:00:00Z",
           "2026-01-01T24:00:00Z",
           "2026-01-01T12:60:00Z",
           "2026-01-01T12:00:61Z",
           "2016-12-30T23:59:60Z",
           "2016-12-31T23:58:60Z",
       }) {
    SCOPED_TRACE(time);
    const auto invalid = validate(time);
    ASSERT_FALSE(invalid);
    EXPECT_EQ(invalid.error().schema, "M0-GOV-ERROR-v1");
    EXPECT_EQ(invalid.error().code, "GOV_RELATIONSHIP_INVALID");
    EXPECT_EQ(invalid.error().contract, "M0-RELEASE-EVIDENCE-v1");
    EXPECT_EQ(invalid.error().field_path, "$.approvals[]");
    EXPECT_EQ(invalid.error().record_id, std::optional<std::string>("orus.fixture.1"));
  }
}

TEST(GovernanceContracts, BundleResolvesCanonicalBytesSchemasSubjectsAndCrossLinks) {
  auto fixture = CompleteGovernanceBundle();
  const auto valid = [&] {
    return ValidateGovernanceBundle(
        Canonical(fixture.manifest, "M0-RELEASE-EVIDENCE-v1"), fixture.documents);
  };
  ASSERT_TRUE(valid());

  auto missing = fixture;
  missing.documents.erase(missing.documents.begin());
  auto rejected = ValidateGovernanceBundle(
      Canonical(missing.manifest, "M0-RELEASE-EVIDENCE-v1"), missing.documents);
  ASSERT_FALSE(rejected);
  EXPECT_EQ(rejected.error().code, "GOV_REFERENCE_UNRESOLVED");

  auto mutated = fixture;
  mutated.documents.front().bytes[mutated.documents.front().bytes.find("M0-")] = 'N';
  rejected = ValidateGovernanceBundle(
      Canonical(mutated.manifest, "M0-RELEASE-EVIDENCE-v1"), mutated.documents);
  ASSERT_FALSE(rejected);
  EXPECT_EQ(rejected.error().code, "GOV_DIGEST_INVALID");

  auto wrong_schema = fixture;
  auto& schema_row = std::get<JsonValue::Array>(MutableMember(wrong_schema.manifest, "evidence").value).front();
  MutableMember(schema_row, "schema") = JsonValue("M0-WRONG-EVIDENCE-v1");
  rejected = ValidateGovernanceBundle(
      Canonical(wrong_schema.manifest, "M0-RELEASE-EVIDENCE-v1"), wrong_schema.documents);
  ASSERT_FALSE(rejected);
  EXPECT_EQ(rejected.error().code, "GOV_REFERENCE_UNRESOLVED");

  auto substituted = fixture;
  auto& evidence = std::get<JsonValue::Array>(MutableMember(substituted.manifest, "evidence").value);
  MutableMember(evidence.front(), "evidence_object_sha256") =
      *FindMember(evidence[1], "evidence_object_sha256");
  rejected = ValidateGovernanceBundle(
      Canonical(substituted.manifest, "M0-RELEASE-EVIDENCE-v1"), substituted.documents);
  ASSERT_FALSE(rejected);
  EXPECT_EQ(rejected.error().code, "GOV_DIGEST_INVALID");

  auto wrong_package = fixture;
  const auto security = std::find_if(
      wrong_package.documents.begin(), wrong_package.documents.end(),
      [](const ReferencedDocument& document) {
        return document.path == "evidence/security_controls.json";
      });
  ASSERT_NE(security, wrong_package.documents.end());
  auto security_document = ParseCanonicalJson(
      security->bytes, "M0-SECURITY-CONTROLS-v1", "GOV_NONCANONICAL")
                               .value();
  MutableMember(security_document, "package_tree_sha256") = JsonValue(std::string(64, '0'));
  security->bytes = Canonical(security_document, "M0-SECURITY-CONTROLS-v1");
  RefreshEvidenceReference(wrong_package, security->path);
  rejected = ValidateGovernanceBundle(
      Canonical(wrong_package.manifest, "M0-RELEASE-EVIDENCE-v1"), wrong_package.documents);
  ASSERT_FALSE(rejected);
  EXPECT_EQ(rejected.error().code, "GOV_RELATIONSHIP_INVALID");

  auto wrong_sbom = fixture;
  const auto spdx = std::find_if(
      wrong_sbom.documents.begin(), wrong_sbom.documents.end(),
      [](const ReferencedDocument& document) { return document.path == "orus.spdx.json"; });
  ASSERT_NE(spdx, wrong_sbom.documents.end());
  spdx->bytes.replace(spdx->bytes.find("DESCRIBES"), std::string("DESCRIBES").size(), "CONTAINS");
  rejected = ValidateGovernanceBundle(
      Canonical(wrong_sbom.manifest, "M0-RELEASE-EVIDENCE-v1"), wrong_sbom.documents);
  ASSERT_FALSE(rejected);
  EXPECT_EQ(rejected.error().code, "GOV_DIGEST_INVALID");
}

TEST(GovernanceContracts, SubjectAndExactSchemaDagCannotEnterReleaseEvidence) {
  const auto assembled = test::Read("tests/contracts/fixtures/release-evidence-assembled.json");
  for (const auto path : {".", "../escape.json"}) {
    auto invalid_path = assembled;
    const auto start = invalid_path.find("evidence/build.json");
    invalid_path.replace(start, std::string("evidence/build.json").size(), path);
    auto invalid = ValidateGovernanceDocument(invalid_path);
    ASSERT_FALSE(invalid) << path;
    EXPECT_EQ(invalid.error().code, "GOV_RELATIONSHIP_INVALID") << path;
  }
  for (const auto path : {"evidence/release-evidence.json", "evidence/current-scan.json",
                          "evidence/final-scan.json", "evidence/final-marker.json"}) {
    auto neutral_path = assembled;
    const auto start = neutral_path.find("evidence/build.json");
    neutral_path.replace(start, std::string("evidence/build.json").size(), path);
    EXPECT_TRUE(ValidateGovernanceDocument(neutral_path)) << path;
  }
  for (const auto forbidden_schema : {
           "M0-RELEASE-EVIDENCE-v1",
           "M0-SECRET-SCAN-METADATA-v1",
           "M0-SECRET-SCAN-MANIFEST-v1",
           "M0-SECRET-SCAN-REPORT-v1",
           "M0-RELEASE-APPROVAL-v1",
       }) {
    auto forbidden = assembled;
    const auto start = forbidden.find("M0-BUILD-FACTS-v1");
    forbidden.replace(start, std::string("M0-BUILD-FACTS-v1").size(), forbidden_schema);
    auto invalid = ValidateGovernanceDocument(forbidden);
    ASSERT_FALSE(invalid) << forbidden_schema;
    EXPECT_EQ(invalid.error().code, "GOV_RELATIONSHIP_INVALID") << forbidden_schema;
  }

  auto wrong_subject = assembled;
  const auto digest = wrong_subject.find(std::string(64, 'f'));
  wrong_subject.replace(digest, 64, "sha256:ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff");
  auto invalid = ValidateGovernanceDocument(wrong_subject);
  ASSERT_FALSE(invalid);
  EXPECT_EQ(invalid.error().code, "GOV_DIGEST_INVALID");
}

TEST(PerformanceContracts, FiveFixedDocumentsMatchIdentitiesAndSchemas) {
  for (const auto file : {"perf-workload.json", "perf-raw-sample.json", "perf-runner.json", "perf-result.json", "perf-comparison.json"}) {
    auto result = ValidatePerformanceDocument(test::Read(std::string("tests/contracts/fixtures/") + file));
    EXPECT_TRUE(result) << file << ": " << (result ? "" : result.error().message);
  }
  EXPECT_EQ(Sha256(test::Read("tests/contracts/fixtures/perf-raw-sample.json"))->Hex(),
            "be68aca7cc59414f245daaf3dca7bfa22b6463d97e33e80771655a6bd58d8b78");
  EXPECT_EQ(Sha256(test::Read("tests/contracts/fixtures/perf-workload.json"))->Hex(),
            "6ae68e519f47866d9aca4574dbda964e3557b9536bb0819a6314d564aed745d0");
  EXPECT_EQ(Sha256(test::Read("tests/contracts/fixtures/perf-runner.json"))->Hex(),
            "29751cbe816b7edad016dddef96f5dbd28a9e5290e5c4b664e5f5effae1f3e62");
  EXPECT_EQ(Sha256(test::Read("tests/contracts/fixtures/perf-result.json"))->Hex(),
            "53bf9f36b775d084120636acbc80490257e8f32d3e9e662de3fd0dae3a751f4b");
  EXPECT_EQ(Sha256(test::Read("tests/contracts/fixtures/perf-comparison.json"))->Hex(),
            "8aede2e8179c52f9d6c96618510ebe6e4311030b6d91f292a58bca4d7a4c94c6");
  const auto raw_array = "[" + test::Read("tests/contracts/fixtures/perf-raw-sample.json") + "]";
  EXPECT_TRUE(ValidatePerformanceResultBundle(
      test::Read("tests/contracts/fixtures/perf-workload.json"), raw_array,
      test::Read("tests/contracts/fixtures/perf-result.json")));
}

TEST(PerformanceContracts, ResultBundleReconcilesRawBytesOrderStatisticsAndWorkload) {
  const auto workload_bytes = test::Read("tests/contracts/fixtures/perf-workload.json");
  const auto raw_object = test::Read("tests/contracts/fixtures/perf-raw-sample.json");
  const auto raw_array = "[" + raw_object + "]";
  const auto result_bytes = test::Read("tests/contracts/fixtures/perf-result.json");

  auto wrong_order = ParseCanonicalJson(
      raw_object, "M0-PERF-RAW-SAMPLE-v1", "PERF_NONCANONICAL").value();
  MutableMember(wrong_order, "order") = JsonValue("baseline_first");
  EXPECT_FALSE(ValidatePerformanceResultBundle(
      workload_bytes, "[" + Canonical(wrong_order, "M0-PERF-RAW-SAMPLE-v1") + "]",
      result_bytes));

  auto wrong_statistics = ParseCanonicalJson(
      result_bytes, "M0-PERF-RESULT-v1", "PERF_NONCANONICAL").value();
  MutableMember(MutableMember(wrong_statistics, "statistics"), "median") = JsonValue(99);
  RecomputeDerived(wrong_statistics, "result_id", "M0-PERF-RESULT-v1");
  EXPECT_FALSE(ValidatePerformanceResultBundle(
      workload_bytes, raw_array, Canonical(wrong_statistics, "M0-PERF-RESULT-v1")));

  auto wrong_counts = ParseCanonicalJson(
      result_bytes, "M0-PERF-RESULT-v1", "PERF_NONCANONICAL").value();
  auto& sampling = MutableMember(wrong_counts, "sampling");
  MutableMember(sampling, "valid_pairs") = JsonValue(0);
  MutableMember(sampling, "invalid_pairs") = JsonValue(1);
  RecomputeDerived(wrong_counts, "result_id", "M0-PERF-RESULT-v1");
  EXPECT_FALSE(ValidatePerformanceResultBundle(
      workload_bytes, raw_array, Canonical(wrong_counts, "M0-PERF-RESULT-v1")));

  auto wrong_workload = ParseCanonicalJson(
      workload_bytes, "M0-PERF-WORKLOAD-v1", "PERF_NONCANONICAL").value();
  MutableMember(wrong_workload, "allocation_phase") = JsonValue("startup");
  EXPECT_FALSE(ValidatePerformanceResultBundle(
      Canonical(wrong_workload, "M0-PERF-WORKLOAD-v1"), raw_array, result_bytes));

  auto changed_raw = raw_array;
  changed_raw.replace(changed_raw.find("\"duration_ns\":100"),
                      std::string("\"duration_ns\":100").size(), "\"duration_ns\":99");
  auto digest_mismatch = ValidatePerformanceResultBundle(workload_bytes, changed_raw, result_bytes);
  ASSERT_FALSE(digest_mismatch);
  EXPECT_EQ(digest_mismatch.error().code, "PERF_DIGEST_INVALID");
}

TEST(PerformanceContracts, RelationshipsNoncanonicalBytesAndResourcesReject) {
  auto raw = test::Read("tests/contracts/fixtures/perf-raw-sample.json");
  raw.replace(raw.find("\"duration_ns\":100"), std::string("\"duration_ns\":100").size(), "\"duration_ns\":99");
  auto mismatch = ValidatePerformanceDocument(raw);
  ASSERT_FALSE(mismatch);
  EXPECT_EQ(mismatch.error().code, "PERF_RELATIONSHIP_INVALID");
  auto noncanonical = test::Read("tests/contracts/fixtures/perf-workload.json") + "\n";
  auto rejected = ValidatePerformanceDocument(noncanonical);
  ASSERT_FALSE(rejected);
  EXPECT_EQ(rejected.error().code, "PERF_NONCANONICAL");
  EXPECT_TRUE(ValidatePerformanceDocument(test::Read("tests/contracts/fixtures/perf-workload.json"), {.rss_bytes = 256 * 1024 * 1024, .wall_time_ns = 120000000000ULL}));
  auto over = ValidatePerformanceDocument(test::Read("tests/contracts/fixtures/perf-workload.json"), {.wall_time_ns = 120000000001ULL});
  ASSERT_FALSE(over);
  EXPECT_EQ(over.error().code, "PERF_RESOURCE_LIMIT");
  EXPECT_TRUE(ValidatePerformanceDocument(
      test::Read("tests/contracts/fixtures/perf-comparison.json"),
      {.count = 100000, .work_units = 10000, .depth = 16}));
  auto count_over = ValidatePerformanceDocument(
      test::Read("tests/contracts/fixtures/perf-comparison.json"), {.count = 100001});
  ASSERT_FALSE(count_over);
  EXPECT_EQ(count_over.error().code, "PERF_RESOURCE_LIMIT");
  auto work_over = ValidatePerformanceDocument(
      test::Read("tests/contracts/fixtures/perf-comparison.json"), {.work_units = 10001});
  ASSERT_FALSE(work_over);
  EXPECT_EQ(work_over.error().code, "PERF_RESOURCE_LIMIT");
}

TEST(PerformanceContracts, NestedTypesEnumsBoundsAndConditionalFieldsFailClosed) {
  auto workload = ParseCanonicalJson(test::Read("tests/contracts/fixtures/perf-workload.json"),
                                     "M0-PERF-WORKLOAD-v1", "PERF_NONCANONICAL").value();
  MutableMember(workload, "allocation_phase") = JsonValue("during_setup");
  EXPECT_FALSE(ValidatePerformanceDocument(Canonical(workload, "M0-PERF-WORKLOAD-v1")));
  workload = ParseCanonicalJson(test::Read("tests/contracts/fixtures/perf-workload.json"),
                                "M0-PERF-WORKLOAD-v1", "PERF_NONCANONICAL").value();
  MutableMember(MutableMember(workload, "metric"), "unit") = JsonValue("seconds");
  EXPECT_FALSE(ValidatePerformanceDocument(Canonical(workload, "M0-PERF-WORKLOAD-v1")));
  workload = ParseCanonicalJson(test::Read("tests/contracts/fixtures/perf-workload.json"),
                                "M0-PERF-WORKLOAD-v1", "PERF_NONCANONICAL").value();
  MutableMember(workload, "arguments") = JsonValue(JsonValue::Array{JsonValue(std::string(1025, 'x'))});
  EXPECT_FALSE(ValidatePerformanceDocument(Canonical(workload, "M0-PERF-WORKLOAD-v1")));

  auto raw = ParseCanonicalJson(test::Read("tests/contracts/fixtures/perf-raw-sample.json"),
                                "M0-PERF-RAW-SAMPLE-v1", "PERF_NONCANONICAL").value();
  MutableMember(raw, "invalid_reason") = JsonValue("unknown");
  EXPECT_FALSE(ValidatePerformanceDocument(Canonical(raw, "M0-PERF-RAW-SAMPLE-v1")));
  raw = ParseCanonicalJson(test::Read("tests/contracts/fixtures/perf-raw-sample.json"),
                           "M0-PERF-RAW-SAMPLE-v1", "PERF_NONCANONICAL").value();
  MutableMember(raw, "cpu") = JsonValue(-1);
  EXPECT_FALSE(ValidatePerformanceDocument(Canonical(raw, "M0-PERF-RAW-SAMPLE-v1")));

  auto runner = ParseCanonicalJson(test::Read("tests/contracts/fixtures/perf-runner.json"),
                                   "M0-CONTROLLED-RUNNER-v1", "PERF_NONCANONICAL").value();
  auto& predicate = std::get<JsonValue::Array>(MutableMember(runner, "predicates").value).front();
  MutableMember(predicate, "status") = JsonValue("mismatch");
  RecomputeDerived(runner, "contract_sha256", "M0-CONTROLLED-RUNNER-v1");
  EXPECT_FALSE(ValidatePerformanceDocument(Canonical(runner, "M0-CONTROLLED-RUNNER-v1")));
  runner = ParseCanonicalJson(test::Read("tests/contracts/fixtures/perf-runner.json"),
                              "M0-CONTROLLED-RUNNER-v1", "PERF_NONCANONICAL").value();
  MutableMember(MutableMember(runner, "measurement"), "preflight_ms") = JsonValue(4999);
  RecomputeDerived(runner, "contract_sha256", "M0-CONTROLLED-RUNNER-v1");
  EXPECT_FALSE(ValidatePerformanceDocument(Canonical(runner, "M0-CONTROLLED-RUNNER-v1")));

  auto result = ParseCanonicalJson(test::Read("tests/contracts/fixtures/perf-result.json"),
                                   "M0-PERF-RESULT-v1", "PERF_NONCANONICAL").value();
  MutableMember(MutableMember(result, "sampling"), "valid_pairs") = JsonValue(2);
  RecomputeDerived(result, "result_id", "M0-PERF-RESULT-v1");
  EXPECT_FALSE(ValidatePerformanceDocument(Canonical(result, "M0-PERF-RESULT-v1")));
  result = ParseCanonicalJson(test::Read("tests/contracts/fixtures/perf-result.json"),
                              "M0-PERF-RESULT-v1", "PERF_NONCANONICAL").value();
  MutableMember(MutableMember(result, "host"), "affinity") =
      JsonValue(JsonValue::Array{JsonValue(2), JsonValue(2)});
  RecomputeDerived(result, "result_id", "M0-PERF-RESULT-v1");
  EXPECT_FALSE(ValidatePerformanceDocument(Canonical(result, "M0-PERF-RESULT-v1")));

  auto comparison = ParseCanonicalJson(test::Read("tests/contracts/fixtures/perf-comparison.json"),
                                       "M0-PERF-COMPARISON-v1", "PERF_NONCANONICAL").value();
  MutableMember(comparison, "mismatches") = JsonValue(JsonValue::Array{});
  RecomputeDerived(comparison, "comparison_id", "M0-PERF-COMPARISON-v1");
  EXPECT_FALSE(ValidatePerformanceDocument(Canonical(comparison, "M0-PERF-COMPARISON-v1")));
}

TEST(PerformanceContracts, ComparisonAuthorityNoiseStatisticsThresholdAndPrecedenceAreExact) {
  const auto advisory = StatisticalComparison(
      "advisory", 30010000, "advisory_only", "PERF_ADVISORY_INPUT", "informational");
  EXPECT_TRUE(ValidatePerformanceDocument(Canonical(advisory, "M0-PERF-COMPARISON-v1")));
  const auto clean = StatisticalComparison(
      "authoritative", 30000000, "no_regression_detected", "none", "none");
  EXPECT_TRUE(ValidatePerformanceDocument(Canonical(clean, "M0-PERF-COMPARISON-v1")));
  const auto regression = StatisticalComparison(
      "authoritative", 30000001, "regression_requires_approval",
      "PERF_REGRESSION_REQUIRES_APPROVAL", "approval");
  EXPECT_TRUE(ValidatePerformanceDocument(Canonical(regression, "M0-PERF-COMPARISON-v1")));

  auto missing_advisory_statistics = advisory;
  for (const auto field : {"lower_bound_ppb", "point_estimate_ppb", "seed_sha256",
                           "significance_fired", "threshold_fired"}) {
    MutableMember(missing_advisory_statistics, field) = JsonValue(nullptr);
  }
  MutableMember(missing_advisory_statistics, "resamples") = JsonValue(0);
  ExpectComparisonRejected(missing_advisory_statistics);

  auto forged_authority = advisory;
  MutableMember(forged_authority, "authority") = JsonValue("authoritative");
  ExpectComparisonRejected(forged_authority);

  auto noise_precedence = advisory;
  MutableMember(noise_precedence, "noise_state") = JsonValue("failed");
  ExpectComparisonRejected(noise_precedence);

  auto noise_reason = ParseCanonicalJson(
      test::Read("tests/contracts/fixtures/perf-comparison.json"),
      "M0-PERF-COMPARISON-v1", "PERF_NONCANONICAL")
                          .value();
  MutableMember(noise_reason, "mismatches") = JsonValue(JsonValue::Array{});
  MutableMember(noise_reason, "noise_state") = JsonValue("failed");
  MutableMember(noise_reason, "state") = JsonValue("inconclusive");
  MutableMember(noise_reason, "reason") = JsonValue("PERF_SAMPLE_FAILED");
  ExpectComparisonRejected(noise_reason);
  MutableMember(noise_reason, "reason") = JsonValue("PERF_NOISE_POLICY_FAILED");
  RecomputeDerived(noise_reason, "comparison_id", "M0-PERF-COMPARISON-v1");
  EXPECT_TRUE(ValidatePerformanceDocument(Canonical(noise_reason, "M0-PERF-COMPARISON-v1")));

  auto wrong_threshold_state = regression;
  MutableMember(wrong_threshold_state, "state") = JsonValue("no_regression_detected");
  MutableMember(wrong_threshold_state, "reason") = JsonValue("none");
  MutableMember(wrong_threshold_state, "next_action") = JsonValue("none");
  ExpectComparisonRejected(wrong_threshold_state);

  auto wrong_flags = clean;
  MutableMember(wrong_flags, "threshold_fired") = JsonValue(true);
  ExpectComparisonRejected(wrong_flags);

  auto wrong_seed = advisory;
  MutableMember(wrong_seed, "seed_sha256") = JsonValue(std::string(64, '0'));
  ExpectComparisonRejected(wrong_seed);

  auto comparison_with_mismatch = advisory;
  MutableMember(comparison_with_mismatch, "mismatches") = JsonValue(JsonValue::Array{
      JsonValue(JsonValue::Object{{"baseline", "a"}, {"candidate", "b"}, {"path", "host.cpu"}}),
  });
  ExpectComparisonRejected(comparison_with_mismatch);
}

TEST(PerformanceStatistics, NegativeTieMedianMadPercentileAndOverflowAreExact) {
  const std::array<std::int64_t, 4> values{-4, -3, 2, 3};
  auto result = ComputeIntegerStatistics(values);
  ASSERT_TRUE(result);
  EXPECT_EQ(result->minimum, -4);
  EXPECT_EQ(result->maximum, 3);
  EXPECT_EQ(result->median, -1);
  EXPECT_EQ(result->median_absolute_deviation, 3);
  EXPECT_EQ(NearestRankPercentile(values, 500000).value(), -3);
  const std::array<std::int64_t, 2> overflow{
      std::numeric_limits<std::int64_t>::min(), std::numeric_limits<std::int64_t>::max()};
  EXPECT_FALSE(ComputeIntegerStatistics(overflow));
}

TEST(CorpusContracts, BothExamplesValidateAndForgedSuccessFails) {
  const auto run = test::Read("tests/contracts/fixtures/corpus-run.json");
  EXPECT_TRUE(ValidateCorpusDocument(run));
  EXPECT_TRUE(ValidateCorpusDocument(test::Read("tests/contracts/fixtures/corpus-reliability.json")));
  auto forged = run;
  forged.replace(forged.find("12502500"), std::string("12502500").size(), "12502501");
  auto invalid = ValidateCorpusDocument(forged);
  ASSERT_FALSE(invalid);
  EXPECT_EQ(invalid.error().code, "CORP_REPORT_FORGED_SUCCESS");
}

TEST(CorpusContracts, ReliabilityBundleResolvesRunBytesAndContext) {
  const auto run_bytes = test::Read("tests/contracts/fixtures/corpus-run.json");
  auto reliability = ParseCanonicalJson(
      test::Read("tests/contracts/fixtures/corpus-reliability.json"),
      "M0-CORPUS-RELIABILITY-v1", "CORP_REPORT_NONCANONICAL").value();
  auto& reference = std::get<JsonValue::Array>(MutableMember(reliability, "run_reports").value).front();
  MutableMember(reference, "byte_length") = JsonValue(static_cast<std::int64_t>(run_bytes.size()));
  MutableMember(reference, "sha256") = JsonValue(Sha256(run_bytes)->Hex());
  const auto reliability_bytes = Canonical(reliability, "M0-CORPUS-RELIABILITY-v1");
  const std::array<ReferencedDocument, 1> runs{{{"runs/1.json", run_bytes}}};
  EXPECT_TRUE(ValidateCorpusReliabilityBundle(reliability_bytes, runs));

  auto wrong_digest = reliability;
  auto& wrong_reference =
      std::get<JsonValue::Array>(MutableMember(wrong_digest, "run_reports").value).front();
  MutableMember(wrong_reference, "sha256") = JsonValue(std::string(64, '0'));
  EXPECT_FALSE(ValidateCorpusReliabilityBundle(
      Canonical(wrong_digest, "M0-CORPUS-RELIABILITY-v1"), runs));

  auto wrong_context = ParseCanonicalJson(
      run_bytes, "M0-CORPUS-RUN-v1", "CORP_REPORT_NONCANONICAL").value();
  MutableMember(wrong_context, "environment_id") = JsonValue(std::string(64, 'd'));
  const auto wrong_run_bytes = Canonical(wrong_context, "M0-CORPUS-RUN-v1");
  auto matching_reference = reliability;
  auto& row = std::get<JsonValue::Array>(MutableMember(matching_reference, "run_reports").value).front();
  MutableMember(row, "byte_length") = JsonValue(static_cast<std::int64_t>(wrong_run_bytes.size()));
  MutableMember(row, "sha256") = JsonValue(Sha256(wrong_run_bytes)->Hex());
  const std::array<ReferencedDocument, 1> wrong_runs{{{"runs/1.json", wrong_run_bytes}}};
  auto rejected = ValidateCorpusReliabilityBundle(
      Canonical(matching_reference, "M0-CORPUS-RELIABILITY-v1"), wrong_runs);
  ASSERT_FALSE(rejected);
  EXPECT_EQ(rejected.error().code, "CORP_REPORT_RELATIONSHIP_INVALID");
}

TEST(CorpusContracts, FaultMappingAggregateCountsAndBoundsReject) {
  auto reliability = test::Read("tests/contracts/fixtures/corpus-reliability.json");
  reliability.replace(reliability.find("\"passed_runs\":1"), std::string("\"passed_runs\":1").size(), "\"passed_runs\":0");
  auto invalid = ValidateCorpusDocument(reliability);
  ASSERT_FALSE(invalid);
  EXPECT_EQ(invalid.error().code, "CORP_REPORT_FORGED_SUCCESS");
  EXPECT_TRUE(ValidateCorpusDocument(
      test::Read("tests/contracts/fixtures/corpus-run.json"), {.input_bytes = 1 * 1024 * 1024}));
  auto over = ValidateCorpusDocument(
      test::Read("tests/contracts/fixtures/corpus-run.json"), {.input_bytes = 1 * 1024 * 1024 + 1ULL});
  ASSERT_FALSE(over);
  EXPECT_EQ(over.error().code, "CORP_REPORT_FIELD_BOUND");
  const std::string oversized_run =
      "{\"padding\":\"" + std::string(1 * 1024 * 1024, 'x') +
      "\",\"schema\":\"M0-CORPUS-RUN-v1\"}";
  auto encoded_over = ValidateCorpusDocument(oversized_run);
  ASSERT_FALSE(encoded_over);
  EXPECT_EQ(encoded_over.error().code, "CORP_REPORT_FIELD_BOUND");
  EXPECT_TRUE(ValidateCorpusDocument(
      test::Read("tests/contracts/fixtures/corpus-reliability.json"), {.count = 100, .depth = 16}));
  auto count_over = ValidateCorpusDocument(
      test::Read("tests/contracts/fixtures/corpus-reliability.json"), {.count = 101});
  ASSERT_FALSE(count_over);
  EXPECT_EQ(count_over.error().code, "CORP_REPORT_FIELD_BOUND");
}

TEST(CorpusContracts, TopologyOwnershipAndNestedSuccessRowsRejectForgery) {
  const auto run = test::Read("tests/contracts/fixtures/corpus-run.json");
  const std::array<std::pair<std::string_view, std::string_view>, 7> mutations{{
      {"\"host_pid\":1,\"host_tid\":3", "\"host_pid\":9,\"host_tid\":3"},
      {"\"host_pid\":1,\"host_tid\":3", "\"host_pid\":1,\"host_tid\":1"},
      {"\"first\":3001", "\"first\":3002"},
      {"\"sequence\":1", "\"sequence\":2"},
      {"\"call_count\":1", "\"call_count\":2"},
      {"\"parent_join_count\":3", "\"parent_join_count\":2"},
      {"\"temporary_resources_removed\":true", "\"temporary_resources_removed\":false"},
  }};
  for (const auto& [from, to] : mutations) {
    auto forged = run;
    const auto position = forged.find(from);
    ASSERT_NE(position, std::string::npos) << from;
    forged.replace(position, from.size(), to);
    auto invalid = ValidateCorpusDocument(forged);
    ASSERT_FALSE(invalid) << from;
    EXPECT_EQ(invalid.error().code, "CORP_REPORT_FORGED_SUCCESS") << from;
  }

  auto same_image = run;
  const auto child = same_image.find(std::string(64, 'c'));
  ASSERT_NE(child, std::string::npos);
  same_image.replace(child, 64, std::string(64, 'b'));
  auto invalid = ValidateCorpusDocument(same_image);
  ASSERT_FALSE(invalid);
  EXPECT_EQ(invalid.error().code, "CORP_REPORT_RELATIONSHIP_INVALID");
}

TEST(CorpusContracts, FaultEnumsAndReliabilityRowsAreFiniteAndReconciled) {
  const auto run = test::Read("tests/contracts/fixtures/corpus-run.json");
  for (const auto& [fault, terminal] : std::array<std::pair<std::string_view, std::string_view>, 7>{{
           {"exec_failure", "exec_failed"},
           {"parent_worker_failure", "thread_lifecycle_failed"},
           {"child_worker_failure", "thread_lifecycle_failed"},
           {"child_exit_before_ready", "thread_lifecycle_failed"},
           {"malformed_ipc", "ipc_protocol_error"},
           {"ipc_close", "ipc_protocol_error"},
           {"hang_until_timeout", "timeout"},
       }}) {
    auto report = run;
    report.replace(report.find("\"fault_mode\":\"none\""), std::string("\"fault_mode\":\"none\"").size(),
                   std::string("\"fault_mode\":\"") + std::string(fault) + "\"");
    report.replace(report.find("\"passed\":true"), std::string("\"passed\":true").size(), "\"passed\":false");
    report.replace(report.find("\"terminal\":\"success\""), std::string("\"terminal\":\"success\"").size(),
                   std::string("\"terminal\":\"") + std::string(terminal) + "\"");
    auto success_shaped = ValidateCorpusDocument(report);
    ASSERT_FALSE(success_shaped) << fault;
    EXPECT_EQ(success_shaped.error().code, "CORP_REPORT_RELATIONSHIP_INVALID");
    report.replace(report.find("\"logical_result\":12502500"),
                   std::string("\"logical_result\":12502500").size(),
                   "\"logical_result\":0");
    EXPECT_TRUE(ValidateCorpusDocument(report)) << fault;

    auto cleanup_failure = report;
    cleanup_failure.replace(
        cleanup_failure.find("\"temporary_resources_removed\":true"),
        std::string("\"temporary_resources_removed\":true").size(),
        "\"temporary_resources_removed\":false");
    cleanup_failure.replace(
        cleanup_failure.find(std::string("\"terminal\":\"") + std::string(terminal) + "\""),
        std::string("\"terminal\":\"").size() + terminal.size() + 1,
        "\"terminal\":\"cleanup_failed\"");
    EXPECT_TRUE(ValidateCorpusDocument(cleanup_failure)) << fault;

    auto wrong_nested_type = report;
    wrong_nested_type.replace(
        wrong_nested_type.find("\"parent_join_count\":3"),
        std::string("\"parent_join_count\":3").size(),
        "\"parent_join_count\":\"three\"");
    EXPECT_FALSE(ValidateCorpusDocument(wrong_nested_type)) << fault;

    report.replace(report.find(std::string("\"terminal\":\"") + std::string(terminal) + "\""),
                   std::string("\"terminal\":\"").size() + terminal.size() + 1, "\"terminal\":\"cancelled\"");
    auto wrong = ValidateCorpusDocument(report);
    ASSERT_FALSE(wrong) << fault;
    EXPECT_EQ(wrong.error().code, "CORP_REPORT_RELATIONSHIP_INVALID");
  }

  const auto reliability = test::Read("tests/contracts/fixtures/corpus-reliability.json");
  for (const auto rows : {
           "[{\"count\":0,\"terminal\":\"timeout\"}]",
           "[{\"count\":1,\"terminal\":\"timeout\"},{\"count\":1,\"terminal\":\"timeout\"}]",
           "[{\"count\":1,\"terminal\":\"timeout\"},{\"count\":1,\"terminal\":\"cancelled\"}]",
       }) {
    auto report = reliability;
    report.replace(report.find("\"failure_counts\":[]"), std::string("\"failure_counts\":[]").size(),
                   std::string("\"failure_counts\":") + rows);
    auto invalid = ValidateCorpusDocument(report);
    ASSERT_FALSE(invalid);
    EXPECT_EQ(invalid.error().code, "CORP_REPORT_RELATIONSHIP_INVALID");
  }

  auto release = reliability;
  release.replace(release.find("unit_fixture"), std::string("unit_fixture").size(), "m0_release");
  auto incomplete = ValidateCorpusDocument(release);
  ASSERT_FALSE(incomplete);
  EXPECT_EQ(incomplete.error().code, "CORP_REPORT_RELATIONSHIP_INVALID");
}

TEST(SecurityResourceContracts, OwnedRowsAreFiniteUniqueAndLiteral) {
  const auto& rows = M0SharedResourceContracts();
  ASSERT_EQ(rows.size(), 7);
  EXPECT_TRUE(ValidateM0SharedResourceContracts(rows));
  std::set<std::string> identifiers;
  std::set<std::string> operations;
  for (const auto& row : rows) {
    EXPECT_EQ(row.schema, "M0-RESOURCE-LIMIT-v1");
    EXPECT_EQ(row.status, "applicable");
    EXPECT_TRUE(identifiers.insert(row.limit_id).second);
    EXPECT_TRUE(operations.insert(row.operation).second);
    EXPECT_FALSE(row.domain.empty());
    EXPECT_FALSE(row.parser.empty());
    EXPECT_FALSE(row.input_trust.empty());
    EXPECT_FALSE(row.units.empty());
    EXPECT_FALSE(row.derived_allocation_memory.empty());
    EXPECT_FALSE(row.process_thread_fd.empty());
    EXPECT_FALSE(row.cpu_time.empty());
    EXPECT_FALSE(row.storage_queue_io.empty());
    EXPECT_FALSE(row.enforcement_point.empty());
    EXPECT_FALSE(row.error.empty());
    EXPECT_FALSE(row.cleanup.empty());
    EXPECT_FALSE(row.tests.empty());
    EXPECT_FALSE(row.owner.empty());
    EXPECT_FALSE(row.owner_requirement.empty());
    EXPECT_FALSE(row.not_applicable_rationale.empty());

    ResourceUsage exact;
    if (row.limits.input_bytes) exact.input_bytes = *row.limits.input_bytes;
    if (row.limits.count) exact.count = *row.limits.count;
    if (row.limits.work_units) exact.work_units = *row.limits.work_units;
    if (row.limits.depth) exact.depth = *row.limits.depth;
    if (row.limits.rss_bytes) exact.rss_bytes = *row.limits.rss_bytes;
    if (row.limits.wall_time_ns) exact.wall_time_ns = *row.limits.wall_time_ns;
    EXPECT_TRUE(CheckResourceUsage(exact, row.limits, row.operation, row.error)) << row.limit_id;

    for (const auto resource : {"input", "count", "work", "depth", "rss", "time"}) {
      auto first_over = exact;
      bool applicable = true;
      if (resource == std::string_view("input") && row.limits.input_bytes) {
        ++first_over.input_bytes;
      } else if (resource == std::string_view("count") && row.limits.count) {
        ++first_over.count;
      } else if (resource == std::string_view("work") && row.limits.work_units) {
        ++first_over.work_units;
      } else if (resource == std::string_view("depth") && row.limits.depth) {
        ++first_over.depth;
      } else if (resource == std::string_view("rss") && row.limits.rss_bytes) {
        ++first_over.rss_bytes;
      } else if (resource == std::string_view("time") && row.limits.wall_time_ns) {
        ++first_over.wall_time_ns;
      } else {
        applicable = false;
      }
      if (applicable) {
        auto rejected = CheckResourceUsage(first_over, row.limits, row.operation, row.error);
        ASSERT_FALSE(rejected) << row.limit_id << ":" << resource;
        EXPECT_EQ(rejected.error().code, row.error);
      }
    }
    if (row.alternate_limits) {
      ResourceUsage alternate_exact;
      alternate_exact.input_bytes = row.alternate_limits->input_bytes.value_or(0);
      alternate_exact.count = row.alternate_limits->count.value_or(0);
      alternate_exact.depth = row.alternate_limits->depth.value_or(0);
      EXPECT_TRUE(CheckResourceUsage(
          alternate_exact, *row.alternate_limits, row.operation, row.error));
      ++alternate_exact.input_bytes;
      EXPECT_FALSE(CheckResourceUsage(
          alternate_exact, *row.alternate_limits, row.operation, row.error));
    }
  }

  EXPECT_EQ(identifiers, (std::set<std::string>{
                             "SEC-LIM-10-02", "SEC-LIM-10-03", "SEC-LIM-11-03", "SEC-LIM-11-04",
                             "SEC-LIM-14-01", "SEC-LIM-14-02", "SEC-LIM-15-03"}));

  for (const auto mutate : {"missing", "numeric", "error", "late", "cleanup", "trust", "rationale"}) {
    auto drifted = rows;
    if (mutate == std::string_view("missing")) {
      drifted.pop_back();
    } else if (mutate == std::string_view("numeric")) {
      ++*drifted.front().limits.input_bytes;
    } else if (mutate == std::string_view("error")) {
      drifted[2].error = "GOV_FIELD_BOUND";
    } else if (mutate == std::string_view("late")) {
      drifted[3].enforcement_point = "after_final_marker";
    } else if (mutate == std::string_view("cleanup")) {
      drifted[3].cleanup.clear();
    } else if (mutate == std::string_view("trust")) {
      drifted[3].input_trust.clear();
    } else {
      drifted[3].not_applicable_rationale.clear();
    }
    auto invalid = ValidateM0SharedResourceContracts(drifted);
    ASSERT_FALSE(invalid) << mutate;
    EXPECT_EQ(invalid.error().code, "SEC_RESOURCE_LIMIT") << mutate;
  }
}

}  // namespace
}  // namespace orus::contracts
