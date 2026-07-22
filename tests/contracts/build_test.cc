#include "orus/contracts/contracts.h"

#include "tests/contracts/test_support.h"

#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <string>
#include <sys/stat.h>

#include <gtest/gtest.h>

namespace orus::contracts {
namespace {

std::string ObservedFromContract(const JsonValue& contract) {
  const auto environment_id = *FindMember(contract, "environment_id")->AsString();
  const auto& host = *FindMember(contract, "host")->AsObject();
  JsonValue::Object observed_host;
  for (const auto& [name, predicate] : host) {
    observed_host.emplace_back(name, *FindMember(predicate, "expected"));
  }
  JsonValue observed(JsonValue::Object{
      {"embedded_environment_id", environment_id},
      {"facts", JsonValue(JsonValue::Object{{"host", JsonValue(std::move(observed_host))}})},
      {"schema", "M0-REFENV-OBSERVED-v1"},
  });
  return EmitCanonicalJson(observed, "M0-REFENV-OBSERVED-v1").value();
}

std::string RecomputeReferenceId(std::string bytes) {
  auto parsed = ParseCanonicalJson(
      bytes, "M0-REFENV-v1", "BUILD_REFENV_NONCANONICAL", {.maximum_bytes = 64 * 1024, .maximum_depth = 8});
  EXPECT_TRUE(parsed) << (parsed ? "" : parsed.error().message);
  JsonValue::Object without_identity;
  std::string* declared{};
  for (auto& [name, value] : std::get<JsonValue::Object>(parsed->value)) {
    if (name == "environment_id") {
      declared = std::get_if<std::string>(&value.value);
    } else {
      without_identity.emplace_back(name, value);
    }
  }
  EXPECT_NE(declared, nullptr);
  *declared = Sha256(EmitCanonicalJson(JsonValue(std::move(without_identity)), "M0-REFENV-v1").value())->Hex();
  return EmitCanonicalJson(*parsed, "M0-REFENV-v1").value();
}

TEST(BuildFacts, ValidatesDevReleaseAndDirtyTruthfulness) {
  const BuildFacts dev{
      .product_version = "0.0.0-m0",
      .source_revision = "0123456789abcdef0123456789abcdef01234567-dirty",
      .configuration = "dev",
      .compiler = "clang 21.1.8",
      .target_platform = "linux-x86_64",
  };
  EXPECT_TRUE(MakeBuildFacts(dev, false));
  EXPECT_FALSE(MakeBuildFacts(dev, true));
  auto release = dev;
  release.source_revision = "0123456789abcdef0123456789abcdef01234567";
  release.configuration = "release";
  EXPECT_TRUE(MakeBuildFacts(release, true));
  release.compiler.clear();
  EXPECT_FALSE(MakeBuildFacts(release, true));
  release = dev;
  release.product_version.clear();
  EXPECT_EQ(MakeBuildFacts(release, false).error().code, "BUILD_METADATA_INVALID");
  release = dev;
  release.source_revision = "short";
  EXPECT_EQ(MakeBuildFacts(release, false).error().code, "BUILD_METADATA_INVALID");
  release = dev;
  release.configuration = "unknown";
  EXPECT_EQ(MakeBuildFacts(release, false).error().code, "BUILD_METADATA_INVALID");
  release = dev;
  release.target_platform.clear();
  EXPECT_EQ(MakeBuildFacts(release, false).error().code, "BUILD_METADATA_INVALID");
}

TEST(BuildFacts, EmbeddedFactsComeFromDeclaredBazelInputs) {
  auto embedded = EmbeddedBuildFacts();
  if (!embedded) {
    EXPECT_EQ(embedded.error().code, "BUILD_METADATA_INVALID");
    EXPECT_EQ(embedded.error().field_path, "$.source_revision");
    EXPECT_TRUE(embedded.error().observed.ends_with("-dirty"));
    return;
  }
  EXPECT_EQ(*FindMember(*embedded, "schema")->AsString(), "M0-BUILD-FACTS-v1");
  EXPECT_EQ(*FindMember(*embedded, "product_version")->AsString(), "0.0.0-m0");
  const auto revision = *FindMember(*embedded, "source_revision")->AsString();
  EXPECT_TRUE(revision.size() == 40 || revision.size() == 64 || revision.ends_with("-dirty"));
  EXPECT_FALSE(FindMember(*embedded, "configuration")->AsString()->empty());
  EXPECT_FALSE(FindMember(*embedded, "compiler")->AsString()->empty());
  EXPECT_EQ(*FindMember(*embedded, "target_platform")->AsString(), "linux-x86_64");
}

TEST(ReferenceEnvironment, FixedAndProductionDocumentsValidateExactly) {
  const auto contract_bytes = test::Read("tests/contracts/fixtures/reference-fixed.json");
  const auto observed_bytes = test::Read("tests/contracts/fixtures/reference-observed-fixed.json");
  auto fixed = ValidateReferenceEnvironment(contract_bytes, observed_bytes);
  ASSERT_TRUE(fixed) << fixed.error().message;
  EXPECT_EQ(fixed->contract_environment_id, "8eb5cc0dcbec4255bb8250b59870fa851273324d1e70a2dc835028027d82da94");
  EXPECT_EQ(fixed->overall, "validated_reference");
  ASSERT_EQ(fixed->outcomes.size(), 9);
  EXPECT_TRUE(std::all_of(fixed->outcomes.begin(), fixed->outcomes.end(), [](const auto& row) { return row.status == "pass"; }));

  const auto production_bytes = test::Read("config/m0-reference-environment.json");
  auto production_contract = ParseCanonicalJson(
      production_bytes, "M0-REFENV-v1", "BUILD_REFENV_NONCANONICAL", {.maximum_bytes = 64 * 1024, .maximum_depth = 8});
  ASSERT_TRUE(production_contract);
  auto production = ValidateReferenceEnvironment(production_bytes, ObservedFromContract(*production_contract));
  ASSERT_TRUE(production) << production.error().message;
  EXPECT_EQ(production->overall, "validated_reference");
  EXPECT_EQ(production->outcomes.size(), 9);
}

TEST(ReferenceEnvironment, MismatchUnavailableIdentityAndResourcesFailClosed) {
  const auto contract = test::Read("tests/contracts/fixtures/reference-fixed.json");
  auto observed = test::Read("tests/contracts/fixtures/reference-observed-fixed.json");
  const auto position = observed.find("6.12.0-fixture");
  ASSERT_NE(position, std::string::npos);
  observed.replace(position, std::string("6.12.0-fixture").size(), "6.13.0-fixture");
  auto mismatch = ValidateReferenceEnvironment(contract, observed);
  ASSERT_TRUE(mismatch);
  EXPECT_EQ(mismatch->overall, "unvalidated");
  EXPECT_EQ(std::count_if(mismatch->outcomes.begin(), mismatch->outcomes.end(), [](const auto& row) { return row.status == "mismatch"; }), 1);

  auto exact = ValidateReferenceEnvironment(contract, test::Read("tests/contracts/fixtures/reference-observed-fixed.json"),
                                            {.rss_bytes = 64 * 1024 * 1024, .wall_time_ns = 10000000000ULL});
  EXPECT_TRUE(exact);
  auto first_over = ValidateReferenceEnvironment(contract, test::Read("tests/contracts/fixtures/reference-observed-fixed.json"),
                                                 {.rss_bytes = 64 * 1024 * 1024 + 1ULL});
  ASSERT_FALSE(first_over);
  EXPECT_EQ(first_over.error().code, "BUILD_REFENV_RESOURCE_LIMIT");

  EXPECT_TRUE(ValidateReferenceEnvironment(
      contract, test::Read("tests/contracts/fixtures/reference-observed-fixed.json"),
      {.wall_time_ns = 10000000000ULL}));
  auto time_over = ValidateReferenceEnvironment(
      contract, test::Read("tests/contracts/fixtures/reference-observed-fixed.json"),
      {.wall_time_ns = 10000000001ULL});
  ASSERT_FALSE(time_over);
  EXPECT_EQ(time_over.error().code, "BUILD_REFENV_RESOURCE_LIMIT");

  EXPECT_TRUE(ValidateReferenceEnvironment(
      contract, test::Read("tests/contracts/fixtures/reference-observed-fixed.json"), {.count = 128, .depth = 8}));
  auto count_over = ValidateReferenceEnvironment(
      contract, test::Read("tests/contracts/fixtures/reference-observed-fixed.json"), {.count = 129});
  ASSERT_FALSE(count_over);
  EXPECT_EQ(count_over.error().code, "BUILD_REFENV_RESOURCE_LIMIT");

  auto wrong_identity = contract;
  wrong_identity.replace(wrong_identity.find("8eb5"), 4, "9eb5");
  auto invalid = ValidateReferenceEnvironment(wrong_identity, test::Read("tests/contracts/fixtures/reference-observed-fixed.json"));
  ASSERT_FALSE(invalid);
  EXPECT_EQ(invalid.error().code, "BUILD_REFENV_ID_MISMATCH");

  auto noncanonical = ValidateReferenceEnvironment(
      contract + "\n", test::Read("tests/contracts/fixtures/reference-observed-fixed.json"));
  ASSERT_FALSE(noncanonical);
  EXPECT_EQ(noncanonical.error().code, "BUILD_REFENV_NONCANONICAL");
}

TEST(ReferenceEnvironment, EveryFactMismatchAndUnavailableIsOrderedAndUnvalidated) {
  const auto contract = test::Read("tests/contracts/fixtures/reference-fixed.json");
  const auto observed = test::Read("tests/contracts/fixtures/reference-observed-fixed.json");
  const std::array<std::pair<std::string_view, std::string_view>, 9> values{{
      {"\"architecture\":\"x86_64\"", "\"architecture\":\"arm64\""},
      {"\"cpu_family\":6", "\"cpu_family\":7"},
      {"\"cpu_model\":85", "\"cpu_model\":86"},
      {"\"cpu_vendor\":\"GenuineIntel\"", "\"cpu_vendor\":\"OtherVendor\""},
      {"\"kernel_release\":\"6.12.0-fixture\"", "\"kernel_release\":\"6.13.0-fixture\""},
      {"\"libc_name\":\"glibc\"", "\"libc_name\":\"other\""},
      {"\"libc_version\":\"2.40\"", "\"libc_version\":\"2.41\""},
      {"\"os_family\":\"linux\"", "\"os_family\":\"other\""},
      {"\"required_isa\":[\"sse2\"]", "\"required_isa\":[\"avx\"]"},
  }};
  for (std::size_t index = 0; index < values.size(); ++index) {
    auto mismatch_bytes = observed;
    const auto position = mismatch_bytes.find(values[index].first);
    ASSERT_NE(position, std::string::npos);
    mismatch_bytes.replace(position, values[index].first.size(), values[index].second);
    auto mismatch = ValidateReferenceEnvironment(contract, mismatch_bytes);
    ASSERT_TRUE(mismatch) << index;
    EXPECT_EQ(mismatch->overall, "unvalidated");
    EXPECT_EQ(mismatch->outcomes[index].status, "mismatch");
    EXPECT_EQ(mismatch->outcomes[index].code, "BUILD_UNVALIDATED_ENVIRONMENT");
    EXPECT_EQ(std::count_if(mismatch->outcomes.begin(), mismatch->outcomes.end(), [](const auto& row) {
                return row.status != "pass";
              }),
              1);

    auto unavailable_bytes = observed;
    const auto unavailable_position = unavailable_bytes.find(values[index].first);
    unavailable_bytes.replace(
        unavailable_position, values[index].first.size(),
        std::string(values[index].first.substr(0, values[index].first.find(':') + 1)) + "null");
    auto unavailable = ValidateReferenceEnvironment(contract, unavailable_bytes);
    ASSERT_TRUE(unavailable) << index;
    EXPECT_EQ(unavailable->overall, "unvalidated");
    EXPECT_EQ(unavailable->outcomes[index].status, "unavailable");
    EXPECT_EQ(unavailable->outcomes[index].code, "BUILD_UNVALIDATED_ENVIRONMENT");
  }
}

TEST(ReferenceEnvironment, WrongSchemaOperatorAndEmbeddedIdentityFailPrecisely) {
  auto contract = test::Read("tests/contracts/fixtures/reference-fixed.json");
  auto wrong_schema = contract;
  wrong_schema.replace(wrong_schema.find("M0-REFENV-v1"), std::string("M0-REFENV-v1").size(), "M0-REFENV-v2");
  auto invalid_schema = ValidateReferenceEnvironment(
      wrong_schema, test::Read("tests/contracts/fixtures/reference-observed-fixed.json"));
  ASSERT_FALSE(invalid_schema);
  EXPECT_EQ(invalid_schema.error().code, "BUILD_REFENV_SCHEMA_UNKNOWN");

  auto wrong_operator = contract;
  const auto operation = wrong_operator.find("\"op\":\"eq\"");
  ASSERT_NE(operation, std::string::npos);
  wrong_operator.replace(operation, std::string("\"op\":\"eq\"").size(), "\"op\":\"ne\"");
  wrong_operator = RecomputeReferenceId(wrong_operator);
  auto invalid_operator = ValidateReferenceEnvironment(
      wrong_operator, test::Read("tests/contracts/fixtures/reference-observed-fixed.json"));
  ASSERT_FALSE(invalid_operator);
  EXPECT_EQ(invalid_operator.error().code, "BUILD_REFENV_OPERATOR_UNKNOWN");

  auto observed = test::Read("tests/contracts/fixtures/reference-observed-fixed.json");
  observed.replace(observed.find("8eb5"), 4, "9eb5");
  auto embedded = ValidateReferenceEnvironment(contract, observed);
  ASSERT_TRUE(embedded);
  EXPECT_EQ(embedded->overall, "unvalidated");
  EXPECT_TRUE(std::all_of(embedded->outcomes.begin(), embedded->outcomes.end(), [](const auto& row) {
    return row.status == "pass";
  }));
}

TEST(ReferenceEnvironment, RejectsEveryBoundedInventoryAndTypedPredicateViolation) {
  const auto contract = test::Read("tests/contracts/fixtures/reference-fixed.json");
  const auto observed = test::Read("tests/contracts/fixtures/reference-observed-fixed.json");
  const auto expect_contract_error = [&](std::string from, std::string to, std::string_view code) {
    auto changed = contract;
    const auto position = changed.find(from);
    ASSERT_NE(position, std::string::npos) << from;
    changed.replace(position, from.size(), to);
    changed = RecomputeReferenceId(std::move(changed));
    auto result = ValidateReferenceEnvironment(changed, observed);
    ASSERT_FALSE(result) << from;
    EXPECT_EQ(result.error().code, code);
  };
  expect_contract_error("\"name\":\"nixpkgs\"", "\"name\":\"Nixpkgs\"", "BUILD_REFENV_FIELD_INVALID");
  expect_contract_error("\"coordinate\":\"fixture:nixpkgs\"", "\"coordinate\":\"\"", "BUILD_REFENV_FIELD_INVALID");
  expect_contract_error("\"target_triple\":\"x86_64-unknown-linux-gnu\"", "\"target_triple\":\"\"", "BUILD_REFENV_FIELD_INVALID");
  expect_contract_error("\"version\":\"fixture-1\"", "\"version\":\"\"", "BUILD_REFENV_FIELD_INVALID");
  expect_contract_error("\"cpu_family\":{\"expected\":6,\"op\":\"u32_eq\"}",
                        "\"cpu_family\":{\"expected\":\"6\",\"op\":\"u32_eq\"}",
                        "BUILD_REFENV_FIELD_INVALID");
  expect_contract_error("\"cpu_family\":{\"expected\":6,\"op\":\"u32_eq\"}",
                        "\"cpu_family\":{\"expected\":6,\"op\":\"eq\"}",
                        "BUILD_REFENV_OPERATOR_UNKNOWN");
  expect_contract_error("\"expected\":[\"sse2\"]", "\"expected\":[\"sse2\",\"avx\"]",
                        "BUILD_REFENV_FIELD_INVALID");

  auto invalid_observed = observed;
  const auto family = invalid_observed.find("\"cpu_family\":6");
  ASSERT_NE(family, std::string::npos);
  invalid_observed.replace(family, std::string("\"cpu_family\":6").size(), "\"cpu_family\":\"6\"");
  auto wrong_type = ValidateReferenceEnvironment(contract, invalid_observed);
  ASSERT_FALSE(wrong_type);
  EXPECT_EQ(wrong_type.error().code, "BUILD_REFENV_FIELD_INVALID");

  invalid_observed = observed;
  const auto identity = invalid_observed.find("8eb5");
  ASSERT_NE(identity, std::string::npos);
  invalid_observed.replace(identity, 4, "zzzz");
  auto bad_identity = ValidateReferenceEnvironment(contract, invalid_observed);
  ASSERT_FALSE(bad_identity);
  EXPECT_EQ(bad_identity.error().code, "BUILD_REFENV_FIELD_INVALID");
}

TEST(PackageIdentity, FixedManifestAndMetadataMatrixAreExact) {
  constexpr std::string_view fixture =
      "{\"entries\":[{\"kind\":\"file\",\"mode\":420,\"path\":\"LICENSE\",\"sha256\":\"bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb\",\"size\":3},{\"kind\":\"file\",\"mode\":493,\"path\":\"bin/orus\",\"sha256\":\"aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa\",\"size\":4}],\"schema\":\"M0-PACKAGE-TREE-v1\"}";
  auto digest = Sha256(fixture);
  ASSERT_TRUE(digest);
  EXPECT_EQ(digest->Hex(), "6827b44d648423f6b79c38a5700fe2551feb45594630b742b6f70b3cfa444f40");

  const auto root = std::filesystem::path(std::getenv("TEST_TMPDIR")) / "package";
  std::filesystem::remove_all(root);
  std::filesystem::create_directories(root / "bin");
  std::ofstream(root / "LICENSE", std::ios::binary) << "MIT";
  std::ofstream(root / "bin/orus", std::ios::binary) << "orus";
  ASSERT_EQ(::chmod((root / "LICENSE").c_str(), 0644), 0);
  ASSERT_EQ(::chmod((root / "bin/orus").c_str(), 0755), 0);
  std::filesystem::create_symlink("bin/orus", root / "orus-link");
  auto first = IdentifyPackageTree(root);
  ASSERT_TRUE(first) << first.error().message;
  EXPECT_EQ(first->entry_count, 4);
  EXPECT_EQ(first->regular_bytes, 7);

  const auto changed_time = std::filesystem::last_write_time(root / "LICENSE") + std::chrono::seconds(1);
  std::filesystem::last_write_time(root / "LICENSE", changed_time);
  auto mtime = IdentifyPackageTree(root);
  ASSERT_TRUE(mtime);
  EXPECT_EQ(mtime->package_tree_sha256, first->package_tree_sha256);

  ASSERT_EQ(::chmod((root / "LICENSE").c_str(), 0600), 0);
  auto mode = IdentifyPackageTree(root);
  ASSERT_TRUE(mode);
  EXPECT_NE(mode->package_tree_sha256, first->package_tree_sha256);

  std::ofstream(root / "LICENSE", std::ios::binary | std::ios::trunc) << "BSD";
  auto content = IdentifyPackageTree(root);
  ASSERT_TRUE(content);
  EXPECT_NE(content->package_tree_sha256, mode->package_tree_sha256);

  std::filesystem::rename(root / "LICENSE", root / "NOTICE");
  auto path = IdentifyPackageTree(root);
  ASSERT_TRUE(path);
  EXPECT_NE(path->package_tree_sha256, content->package_tree_sha256);

  std::filesystem::remove(root / "orus-link");
  std::filesystem::create_symlink("NOTICE", root / "orus-link");
  auto target = IdentifyPackageTree(root);
  ASSERT_TRUE(target);
  EXPECT_NE(target->package_tree_sha256, path->package_tree_sha256);

  EXPECT_TRUE(IdentifyPackageTree(root, {}, ResourceUsage{.count = 100000, .rss_bytes = 256 * 1024 * 1024, .wall_time_ns = 1200000000000ULL}));
  auto over = IdentifyPackageTree(root, {}, ResourceUsage{.count = 100001});
  ASSERT_FALSE(over);
  EXPECT_EQ(over.error().code, "BUILD_PACKAGE_IDENTITY_INVALID");
  auto bytes_over = IdentifyPackageTree(root, {}, ResourceUsage{.input_bytes = 16ULL * 1024ULL * 1024ULL * 1024ULL + 1});
  ASSERT_FALSE(bytes_over);
  EXPECT_EQ(bytes_over.error().code, "BUILD_PACKAGE_IDENTITY_INVALID");

  PackageLimits observed_limits;
  observed_limits.maximum_rss_bytes = 1;
  auto observed_rss = IdentifyPackageTree(root, observed_limits);
  ASSERT_FALSE(observed_rss);
  EXPECT_EQ(observed_rss.error().code, "BUILD_PACKAGE_IDENTITY_INVALID");
  observed_limits.maximum_rss_bytes = 256ULL * 1024ULL * 1024ULL;
  observed_limits.maximum_wall_time_ns = 0;
  auto observed_deadline = IdentifyPackageTree(root, observed_limits);
  ASSERT_FALSE(observed_deadline);
  EXPECT_EQ(observed_deadline.error().code, "BUILD_PACKAGE_IDENTITY_INVALID");
}

TEST(PackageIdentity, RejectsHardLinksAndSpecialFilesBeforeIdentity) {
  const auto root = std::filesystem::path(std::getenv("TEST_TMPDIR")) / "hardlinks";
  std::filesystem::remove_all(root);
  std::filesystem::create_directories(root);
  std::ofstream(root / "a", std::ios::binary) << "x";
  std::filesystem::create_hard_link(root / "a", root / "b");
  auto result = IdentifyPackageTree(root);
  ASSERT_FALSE(result);
  EXPECT_EQ(result.error().code, "BUILD_PACKAGE_IDENTITY_INVALID");

  std::filesystem::remove_all(root);
  std::filesystem::create_directories(root);
  ASSERT_EQ(::mkfifo((root / "pipe").c_str(), 0600), 0);
  result = IdentifyPackageTree(root);
  ASSERT_FALSE(result);
  EXPECT_EQ(result.error().code, "BUILD_PACKAGE_IDENTITY_INVALID");
}

TEST(PackageIdentity, RejectsSymlinkEscapeAndIncludesUnsignedUtf8Paths) {
  const auto root = std::filesystem::path(std::getenv("TEST_TMPDIR")) / "symlinks";
  std::filesystem::remove_all(root);
  std::filesystem::create_directories(root / "nested");
  std::ofstream(root / "é", std::ios::binary) << "unicode";
  std::ofstream(root / "z", std::ios::binary) << "ascii";
  std::filesystem::create_symlink("../é", root / "nested" / "inside");
  auto valid = IdentifyPackageTree(root);
  ASSERT_TRUE(valid) << valid.error().message;
  const auto& entries = *FindMember(valid->manifest, "entries")->AsArray();
  EXPECT_TRUE(std::is_sorted(entries.begin(), entries.end(), [](const JsonValue& left, const JsonValue& right) {
    const auto& a = *FindMember(left, "path")->AsString();
    const auto& b = *FindMember(right, "path")->AsString();
    return std::lexicographical_compare(a.begin(), a.end(), b.begin(), b.end(), [](char x, char y) {
      return static_cast<unsigned char>(x) < static_cast<unsigned char>(y);
    });
  }));

  std::filesystem::remove(root / "nested" / "inside");
  std::filesystem::create_symlink("../../outside", root / "nested" / "inside");
  auto escaped = IdentifyPackageTree(root);
  ASSERT_FALSE(escaped);
  EXPECT_EQ(escaped.error().code, "BUILD_PACKAGE_IDENTITY_INVALID");
}

}  // namespace
}  // namespace orus::contracts
