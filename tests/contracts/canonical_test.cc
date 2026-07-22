#include "orus/contracts/contracts.h"

#include <array>
#include <limits>
#include <string>

#include <gtest/gtest.h>

namespace orus::contracts {
namespace {

TEST(CanonicalJson, AcceptsAndEmitsTheExactProfile) {
  const std::string bytes =
      "{\"a\":\"é\\b\\t\\n\\f\\r\\u0001\\\"\\\\\",\"arr\":[-9223372036854775808,0,9223372036854775807,true,false,null],\"z\":1}";
  auto parsed = ParseCanonicalJson(bytes, "fixture", "FIXTURE_NONCANONICAL");
  ASSERT_TRUE(parsed.has_value()) << parsed.error().message;
  auto emitted = EmitCanonicalJson(*parsed, "fixture");
  ASSERT_TRUE(emitted.has_value());
  EXPECT_EQ(*emitted, bytes);
  EXPECT_FALSE(emitted->ends_with('\n'));
}

TEST(CanonicalJson, RejectsEveryForbiddenByteAndNumberForm) {
  const std::array<std::string, 16> invalid{
      "\xef\xbb\xbf{}",
      "{ }",
      "{\"b\":1,\"a\":2}",
      "{\"a\":1,\"a\":2}",
      std::string("{\"a\":\"") + static_cast<char>(0xff) + "\"}",
      "{\"a\":\"\\ud800\"}",
      "{\"a\":\"é\"}",
      "{\"a\":1.0}",
      "{\"a\":1e2}",
      "{\"a\":9223372036854775808}",
      "{\"a\":+1}",
      "{\"a\":01}",
      "{\"a\":-0}",
      "{\"a\":\"\\x20\"}",
      "{\"a\":\"\\u0041\"}",
      "{}\n",
  };
  for (const auto& bytes : invalid) {
    auto parsed = ParseCanonicalJson(bytes, "fixture", "FIXTURE_NONCANONICAL");
    EXPECT_FALSE(parsed.has_value()) << bytes;
    if (!parsed) EXPECT_EQ(parsed.error().code, "FIXTURE_NONCANONICAL") << bytes;
  }
}

TEST(CanonicalJson, EnforcesExactByteAndDepthLimits) {
  EXPECT_TRUE(ParseCanonicalJson("{}", "fixture", "BAD", {.maximum_bytes = 2, .maximum_depth = 1}));
  auto bytes_over = ParseCanonicalJson("{}", "fixture", "BAD", {.maximum_bytes = 1, .maximum_depth = 1});
  ASSERT_FALSE(bytes_over);
  EXPECT_EQ(bytes_over.error().limit, 1);
  auto depth_over = ParseCanonicalJson("{\"a\":{}}", "fixture", "BAD", {.maximum_bytes = 16, .maximum_depth = 1});
  ASSERT_FALSE(depth_over);
  EXPECT_EQ(depth_over.error().limit, 1);
}

TEST(Unicode, Utf8procNfcIsContainedBehindOrusTypes) {
  auto normalized = NormalizeNfc("é");
  ASSERT_TRUE(normalized);
  EXPECT_EQ(*normalized, "é");
  EXPECT_TRUE(IsValidNfc("é"));
  EXPECT_FALSE(IsValidNfc("é"));
  EXPECT_FALSE(IsValidNfc(std::string("\xff", 1)));
}

TEST(CanonicalJson, TypedEmissionSortsNamesAndRejectsDuplicates) {
  JsonValue value(JsonValue::Object{{"z", 1}, {"a", "x"}});
  auto emitted = EmitCanonicalJson(value, "fixture");
  ASSERT_TRUE(emitted);
  EXPECT_EQ(*emitted, "{\"a\":\"x\",\"z\":1}");
  JsonValue duplicate(JsonValue::Object{{"a", 1}, {"a", 2}});
  EXPECT_FALSE(EmitCanonicalJson(duplicate, "fixture"));
}

TEST(Crypto, OpenSslEvpSha256MatchesStreamingAndFixedVector) {
  auto one_shot = Sha256("abc");
  ASSERT_TRUE(one_shot);
  EXPECT_EQ(one_shot->Hex(), "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad");
  Sha256Stream stream;
  const std::string a = "a";
  const std::string bc = "bc";
  ASSERT_TRUE(stream.Update(std::as_bytes(std::span(a.data(), a.size()))));
  ASSERT_TRUE(stream.Update(std::as_bytes(std::span(bc.data(), bc.size()))));
  auto streaming = stream.Finish();
  ASSERT_TRUE(streaming);
  EXPECT_EQ(*streaming, *one_shot);
  EXPECT_FALSE(stream.Finish());
  EXPECT_FALSE(Sha256Digest::ParseHex("ABC", "fixture"));
}

TEST(Identity, SubjectNamesCannotBeSubstituted) {
  const std::string bytes = "subject bytes";
  auto digest = Sha256(bytes);
  ASSERT_TRUE(digest);
  ContentIdentity identity{
      .subject = DigestSubject::kEvidenceObject,
      .digest = *digest,
      .byte_length = bytes.size(),
      .relative_path = "evidence/a.json",
  };
  EXPECT_TRUE(ValidateIdentityBinding(
      identity, DigestSubject::kEvidenceObject, std::as_bytes(std::span(bytes.data(), bytes.size())), "fixture"));
  EXPECT_FALSE(ValidateIdentityBinding(
      identity, DigestSubject::kSbom, std::as_bytes(std::span(bytes.data(), bytes.size())), "fixture"));
}

TEST(ResourceGuard, ExactBoundsPassAndFirstOverFailsBeforeUse) {
  const ResourceLimits limits{.input_bytes = 16, .count = 2, .depth = 3, .rss_bytes = 64, .wall_time_ns = 10};
  EXPECT_TRUE(CheckResourceUsage(
      {.input_bytes = 16, .count = 2, .depth = 3, .rss_bytes = 64, .wall_time_ns = 10},
      limits, "fixture", "LIMIT"));
  for (const ResourceUsage usage : {
           ResourceUsage{.input_bytes = 17, .count = 2, .depth = 3, .rss_bytes = 64, .wall_time_ns = 10},
           ResourceUsage{.input_bytes = 16, .count = 3, .depth = 3, .rss_bytes = 64, .wall_time_ns = 10},
           ResourceUsage{.input_bytes = 16, .count = 2, .depth = 4, .rss_bytes = 64, .wall_time_ns = 10},
           ResourceUsage{.input_bytes = 16, .count = 2, .depth = 3, .rss_bytes = 65, .wall_time_ns = 10},
           ResourceUsage{.input_bytes = 16, .count = 2, .depth = 3, .rss_bytes = 64, .wall_time_ns = 11},
       }) {
    auto result = CheckResourceUsage(usage, limits, "fixture", "LIMIT");
    ASSERT_FALSE(result);
    EXPECT_EQ(result.error().code, "LIMIT");
  }
}

TEST(CheckedInteger, OverflowReturnsTypedFailure) {
  EXPECT_EQ(CheckedAdd(2, 3, "fixture").value(), 5);
  EXPECT_FALSE(CheckedAdd(std::numeric_limits<std::int64_t>::max(), 1, "fixture"));
  EXPECT_FALSE(CheckedSubtract(std::numeric_limits<std::int64_t>::min(), 1, "fixture"));
  EXPECT_FALSE(CheckedMultiply(std::numeric_limits<std::int64_t>::max(), 2, "fixture"));
}

}  // namespace
}  // namespace orus::contracts
