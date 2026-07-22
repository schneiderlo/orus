#ifndef ORUS_CONTRACTS_CONTRACTS_H_
#define ORUS_CONTRACTS_CONTRACTS_H_

#include <array>
#include <cstddef>
#include <cstdint>
#include <expected>
#include <filesystem>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

namespace orus::contracts {

struct Error {
  std::string code;
  std::string document_schema;
  std::string field_path;
  std::string expected;
  std::string observed;
  std::optional<std::int64_t> limit;
  std::optional<std::int64_t> offset;
  std::string message;

  friend bool operator==(const Error&, const Error&) = default;
};

template <typename T>
using Result = std::expected<T, Error>;

struct JsonValue {
  using Array = std::vector<JsonValue>;
  using Object = std::vector<std::pair<std::string, JsonValue>>;
  using Storage = std::variant<std::nullptr_t, bool, std::int64_t, std::string, Array, Object>;

  Storage value{nullptr};

  JsonValue() = default;
  JsonValue(std::nullptr_t) : value(nullptr) {}
  JsonValue(bool input) : value(input) {}
  JsonValue(int input) : value(static_cast<std::int64_t>(input)) {}
  JsonValue(std::int64_t input) : value(input) {}
  JsonValue(std::string input) : value(std::move(input)) {}
  JsonValue(const char* input) : value(std::string(input)) {}
  JsonValue(Array input) : value(std::move(input)) {}
  JsonValue(Object input) : value(std::move(input)) {}

  const Object* AsObject() const;
  const Array* AsArray() const;
  const std::string* AsString() const;
  const std::int64_t* AsInteger() const;
  const bool* AsBoolean() const;
  bool IsNull() const;
};

struct ParseLimits {
  std::size_t maximum_bytes{16U * 1024U * 1024U};
  std::size_t maximum_depth{16};
};

Result<JsonValue> ParseCanonicalJson(
    std::string_view bytes,
    std::string_view document_schema,
    std::string_view noncanonical_code,
    ParseLimits limits = {});
Result<std::string> EmitCanonicalJson(
    const JsonValue& value,
    std::string_view document_schema,
    ParseLimits limits = {});
const JsonValue* FindMember(const JsonValue& value, std::string_view name);
Result<const JsonValue::Object*> RequireObject(
    const JsonValue& value, std::string_view schema, std::string_view path, std::string_view code);
Result<const JsonValue::Array*> RequireArray(
    const JsonValue& value, std::string_view schema, std::string_view path, std::string_view code);
Result<std::string_view> RequireString(
    const JsonValue& value, std::string_view schema, std::string_view path, std::string_view code);
Result<std::int64_t> RequireInteger(
    const JsonValue& value, std::string_view schema, std::string_view path, std::string_view code);
Result<bool> RequireBoolean(
    const JsonValue& value, std::string_view schema, std::string_view path, std::string_view code);

Result<std::string> NormalizeNfc(std::string_view utf8);
bool IsValidNfc(std::string_view utf8);

struct ResourceLimits {
  std::optional<std::uint64_t> input_bytes;
  std::optional<std::uint64_t> count;
  std::optional<std::uint64_t> work_units;
  std::optional<std::uint64_t> depth;
  std::optional<std::uint64_t> rss_bytes;
  std::optional<std::uint64_t> wall_time_ns;

  friend bool operator==(const ResourceLimits&, const ResourceLimits&) = default;
};

struct ResourceUsage {
  std::uint64_t input_bytes{};
  std::uint64_t count{};
  std::uint64_t work_units{};
  std::uint64_t depth{};
  std::uint64_t rss_bytes{};
  std::uint64_t wall_time_ns{};
};

Result<void> CheckResourceUsage(
    const ResourceUsage& usage,
    const ResourceLimits& limits,
    std::string_view schema,
    std::string_view code);
Result<std::int64_t> CheckedAdd(std::int64_t left, std::int64_t right, std::string_view schema);
Result<std::int64_t> CheckedSubtract(std::int64_t left, std::int64_t right, std::string_view schema);
Result<std::int64_t> CheckedMultiply(std::int64_t left, std::int64_t right, std::string_view schema);

class Sha256Digest {
 public:
  Sha256Digest() = default;
  explicit Sha256Digest(std::array<std::byte, 32> bytes) : bytes_(bytes) {}

  static Result<Sha256Digest> ParseHex(std::string_view hex, std::string_view schema = {});
  std::string Hex() const;
  std::span<const std::byte, 32> Bytes() const { return bytes_; }
  friend bool operator==(const Sha256Digest&, const Sha256Digest&) = default;

 private:
  std::array<std::byte, 32> bytes_{};
};

class Sha256Stream {
 public:
  Sha256Stream();
  Sha256Stream(Sha256Stream&&) noexcept;
  Sha256Stream& operator=(Sha256Stream&&) noexcept;
  ~Sha256Stream();

  Result<void> Update(std::span<const std::byte> bytes);
  Result<Sha256Digest> Finish();

 private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};

Result<Sha256Digest> Sha256(std::span<const std::byte> bytes);
Result<Sha256Digest> Sha256(std::string_view bytes);

enum class DigestSubject {
  kOrusExecutable,
  kPackageTree,
  kSbom,
  kEvidenceObject,
};

struct ContentIdentity {
  DigestSubject subject;
  Sha256Digest digest;
  std::uint64_t byte_length{};
  std::string relative_path;
};

std::string_view SubjectName(DigestSubject subject);
Result<void> ValidateIdentityBinding(
    const ContentIdentity& identity,
    DigestSubject expected_subject,
    std::span<const std::byte> exact_bytes,
    std::string_view schema);

struct BuildFacts {
  std::string product_version;
  std::string source_revision;
  std::string configuration;
  std::string compiler;
  std::string target_platform;
};

Result<JsonValue> MakeBuildFacts(const BuildFacts& facts, bool release);
Result<JsonValue> EmbeddedBuildFacts();

struct ReferenceOutcome {
  std::string path;
  std::string operation;
  JsonValue expected;
  JsonValue observed;
  std::string status;
  std::string code;
};

struct ReferenceValidation {
  std::string contract_environment_id;
  std::string embedded_environment_id;
  std::string overall;
  std::vector<ReferenceOutcome> outcomes;
};

Result<ReferenceValidation> ValidateReferenceEnvironment(
    std::string_view contract_bytes,
    std::string_view observed_bytes,
    ResourceUsage usage = {});

struct PackageLimits {
  std::uint64_t maximum_entries{100000};
  std::uint64_t maximum_path_bytes{4096};
  std::uint64_t maximum_regular_bytes{16ULL * 1024ULL * 1024ULL * 1024ULL};
  std::uint64_t maximum_rss_bytes{256ULL * 1024ULL * 1024ULL};
  std::uint64_t maximum_wall_time_ns{1200000000000ULL};
};

struct PackageIdentity {
  JsonValue manifest;
  Sha256Digest package_tree_sha256;
  std::uint64_t regular_bytes{};
  std::uint64_t entry_count{};
};

Result<PackageIdentity> IdentifyPackageTree(
    const std::filesystem::path& root,
    const PackageLimits& limits = {},
    std::optional<ResourceUsage> injected_usage = std::nullopt);

struct IntegerStatistics {
  std::int64_t minimum{};
  std::int64_t maximum{};
  std::int64_t median{};
  std::int64_t median_absolute_deviation{};
};

Result<IntegerStatistics> ComputeIntegerStatistics(
    std::span<const std::int64_t> values,
    std::string_view schema = "M0-PERF-RESULT-v1");
Result<std::int64_t> NearestRankPercentile(
    std::span<const std::int64_t> values,
    std::uint32_t rank_ppm,
    std::string_view schema = "M0-PERF-RESULT-v1");

struct GovernanceError {
  std::string schema{"M0-GOV-ERROR-v1"};
  std::string code;
  std::string contract;
  std::string field_path;
  std::optional<std::string> record_id;
  std::string expected;
  std::string observed;
  std::optional<std::int64_t> limit;
  std::string message;

  friend bool operator==(const GovernanceError&, const GovernanceError&) = default;
};

template <typename T>
using GovernanceResult = std::expected<T, GovernanceError>;

struct ReferencedDocument {
  std::string path;
  std::string bytes;

  friend bool operator==(const ReferencedDocument&, const ReferencedDocument&) = default;
};

GovernanceResult<JsonValue> ValidateGovernanceDocument(
    std::string_view bytes,
    ResourceUsage usage = {});
GovernanceResult<JsonValue> ValidateGovernanceBundle(
    std::string_view manifest_bytes,
    std::span<const ReferencedDocument> referenced_documents,
    ResourceUsage usage = {});
Result<JsonValue> ValidatePerformanceDocument(
    std::string_view bytes,
    ResourceUsage usage = {});
Result<JsonValue> ValidatePerformanceResultBundle(
    std::string_view workload_bytes,
    std::string_view raw_samples_bytes,
    std::string_view result_bytes,
    ResourceUsage usage = {});
Result<JsonValue> ValidateCorpusDocument(
    std::string_view bytes,
    ResourceUsage usage = {});

Result<JsonValue> ValidateCorpusReliabilityBundle(
    std::string_view reliability_bytes,
    std::span<const ReferencedDocument> run_documents,
    ResourceUsage usage = {});

struct ResourceContractRow {
  std::string schema;
  std::string limit_id;
  std::string domain;
  std::string operation;
  std::string parser;
  std::string input_trust;
  std::string status;
  std::string units;
  ResourceLimits limits;
  std::optional<ResourceLimits> alternate_limits;
  std::string derived_allocation_memory;
  std::string process_thread_fd;
  std::string cpu_time;
  std::string storage_queue_io;
  std::string enforcement_point;
  std::string error;
  std::string cleanup;
  std::string tests;
  std::string owner;
  std::string owner_requirement;
  std::string not_applicable_rationale;

  friend bool operator==(const ResourceContractRow&, const ResourceContractRow&) = default;
};

const std::vector<ResourceContractRow>& M0SharedResourceContracts();
Result<void> ValidateM0SharedResourceContracts(
    std::span<const ResourceContractRow> rows);

}  // namespace orus::contracts

#endif  // ORUS_CONTRACTS_CONTRACTS_H_
