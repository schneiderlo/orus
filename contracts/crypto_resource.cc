#include "orus/contracts/contracts.h"
#include "contracts/resource_monitor.h"

#include <algorithm>
#include <chrono>
#include <cstring>
#include <limits>
#include <sys/resource.h>

#include <openssl/evp.h>

namespace orus::contracts {
namespace {

Error ContractError(
    std::string_view code,
    std::string_view schema,
    std::string_view path,
    std::string_view expected,
    std::string_view observed,
    std::string_view message,
    std::optional<std::int64_t> limit = std::nullopt) {
  return Error{
      .code = std::string(code),
      .document_schema = std::string(schema),
      .field_path = std::string(path),
      .expected = std::string(expected),
      .observed = std::string(observed),
      .limit = limit,
      .offset = std::nullopt,
      .message = std::string(message),
  };
}

std::int64_t DiagnosticInteger(std::uint64_t value) {
  return value > static_cast<std::uint64_t>(std::numeric_limits<std::int64_t>::max())
             ? std::numeric_limits<std::int64_t>::max()
             : static_cast<std::int64_t>(value);
}

Result<void> CheckOne(
    std::string_view resource,
    std::uint64_t observed,
    const std::optional<std::uint64_t>& limit,
    std::string_view schema,
    std::string_view code) {
  if (limit && observed > *limit) {
    return std::unexpected(ContractError(
        code, schema, "$", resource, std::to_string(observed), "resource limit exceeded",
        DiagnosticInteger(*limit)));
  }
  return {};
}

Error OverflowError(std::string_view schema) {
  return ContractError(
      "PERF_INTEGER_OVERFLOW", schema, "$", "signed 64-bit result", "overflow",
      "checked integer operation overflowed");
}

}  // namespace

Result<void> CheckResourceUsage(
    const ResourceUsage& usage,
    const ResourceLimits& limits,
    std::string_view schema,
    std::string_view code) {
  for (const auto& result : {
           CheckOne("input_bytes", usage.input_bytes, limits.input_bytes, schema, code),
           CheckOne("count", usage.count, limits.count, schema, code),
           CheckOne("work_units", usage.work_units, limits.work_units, schema, code),
           CheckOne("depth", usage.depth, limits.depth, schema, code),
           CheckOne("rss_bytes", usage.rss_bytes, limits.rss_bytes, schema, code),
           CheckOne("wall_time_ns", usage.wall_time_ns, limits.wall_time_ns, schema, code),
       }) {
    if (!result) return result;
  }
  return {};
}

namespace internal {

std::uint64_t MonotonicNowNs() {
  const auto now = std::chrono::steady_clock::now().time_since_epoch();
  return static_cast<std::uint64_t>(
      std::chrono::duration_cast<std::chrono::nanoseconds>(now).count());
}

std::uint64_t CurrentProcessRssBytes() {
  struct rusage usage {};
  if (::getrusage(RUSAGE_SELF, &usage) != 0 || usage.ru_maxrss < 0) return 0;
  constexpr std::uint64_t kLinuxRusageUnit = 1024;
  const auto rss = static_cast<std::uint64_t>(usage.ru_maxrss);
  return rss > std::numeric_limits<std::uint64_t>::max() / kLinuxRusageUnit
             ? std::numeric_limits<std::uint64_t>::max()
             : rss * kLinuxRusageUnit;
}

ResourceUsage ObserveResourceUsage(ResourceUsage minimum, std::uint64_t started_ns) {
  const std::uint64_t now = MonotonicNowNs();
  minimum.rss_bytes = std::max(minimum.rss_bytes, CurrentProcessRssBytes());
  minimum.wall_time_ns = std::max(
      minimum.wall_time_ns, now >= started_ns ? now - started_ns : std::uint64_t{0});
  return minimum;
}

}  // namespace internal

Result<std::int64_t> CheckedAdd(std::int64_t left, std::int64_t right, std::string_view schema) {
  std::int64_t output{};
  if (__builtin_add_overflow(left, right, &output)) return std::unexpected(OverflowError(schema));
  return output;
}

Result<std::int64_t> CheckedSubtract(std::int64_t left, std::int64_t right, std::string_view schema) {
  std::int64_t output{};
  if (__builtin_sub_overflow(left, right, &output)) return std::unexpected(OverflowError(schema));
  return output;
}

Result<std::int64_t> CheckedMultiply(std::int64_t left, std::int64_t right, std::string_view schema) {
  std::int64_t output{};
  if (__builtin_mul_overflow(left, right, &output)) return std::unexpected(OverflowError(schema));
  return output;
}

Result<Sha256Digest> Sha256Digest::ParseHex(std::string_view hex, std::string_view schema) {
  if (hex.size() != 64) {
    return std::unexpected(ContractError(
        "DIGEST_INVALID", schema, "$", "64 lowercase hexadecimal digits", hex,
        "SHA-256 text has the wrong length"));
  }
  std::array<std::byte, 32> bytes{};
  auto nibble = [](char value) -> int {
    if (value >= '0' && value <= '9') return value - '0';
    if (value >= 'a' && value <= 'f') return value - 'a' + 10;
    return -1;
  };
  for (std::size_t index = 0; index < bytes.size(); ++index) {
    const int high = nibble(hex[index * 2]);
    const int low = nibble(hex[index * 2 + 1]);
    if (high < 0 || low < 0) {
      return std::unexpected(ContractError(
          "DIGEST_INVALID", schema, "$", "lowercase hexadecimal SHA-256", hex,
          "SHA-256 text has an invalid alphabet"));
    }
    bytes[index] = static_cast<std::byte>((high << 4) | low);
  }
  return Sha256Digest(bytes);
}

std::string Sha256Digest::Hex() const {
  static constexpr char kHex[] = "0123456789abcdef";
  std::string result;
  result.reserve(64);
  for (const std::byte value : bytes_) {
    const auto integer = std::to_integer<unsigned int>(value);
    result.push_back(kHex[integer >> 4U]);
    result.push_back(kHex[integer & 0x0fU]);
  }
  return result;
}

struct Sha256Stream::Impl {
  EVP_MD_CTX* context{EVP_MD_CTX_new()};
  bool initialized{};
  bool finished{};

  Impl() {
    initialized = context != nullptr && EVP_DigestInit_ex2(context, EVP_sha256(), nullptr) == 1;
  }
  ~Impl() { EVP_MD_CTX_free(context); }
};

Sha256Stream::Sha256Stream() : impl_(std::make_unique<Impl>()) {}
Sha256Stream::Sha256Stream(Sha256Stream&&) noexcept = default;
Sha256Stream& Sha256Stream::operator=(Sha256Stream&&) noexcept = default;
Sha256Stream::~Sha256Stream() = default;

Result<void> Sha256Stream::Update(std::span<const std::byte> bytes) {
  if (!impl_ || !impl_->initialized || impl_->finished ||
      EVP_DigestUpdate(impl_->context, bytes.data(), bytes.size()) != 1) {
    return std::unexpected(ContractError(
        "DIGEST_INVALID", "M0-CONTENT-IDENTITY-v1", "$", "active OpenSSL EVP SHA-256 stream",
        "unavailable or finished", "SHA-256 update failed"));
  }
  return {};
}

Result<Sha256Digest> Sha256Stream::Finish() {
  std::array<std::byte, 32> bytes{};
  unsigned int size{};
  if (!impl_ || !impl_->initialized || impl_->finished ||
      EVP_DigestFinal_ex(impl_->context, reinterpret_cast<unsigned char*>(bytes.data()), &size) != 1 ||
      size != bytes.size()) {
    return std::unexpected(ContractError(
        "DIGEST_INVALID", "M0-CONTENT-IDENTITY-v1", "$", "one complete SHA-256 digest",
        "EVP failure", "SHA-256 finalization failed"));
  }
  impl_->finished = true;
  return Sha256Digest(bytes);
}

Result<Sha256Digest> Sha256(std::span<const std::byte> bytes) {
  Sha256Stream stream;
  auto update = stream.Update(bytes);
  if (!update) return std::unexpected(update.error());
  return stream.Finish();
}

Result<Sha256Digest> Sha256(std::string_view bytes) {
  return Sha256(std::as_bytes(std::span(bytes.data(), bytes.size())));
}

std::string_view SubjectName(DigestSubject subject) {
  switch (subject) {
    case DigestSubject::kOrusExecutable: return "orus_executable";
    case DigestSubject::kPackageTree: return "package_tree";
    case DigestSubject::kSbom: return "sbom";
    case DigestSubject::kEvidenceObject: return "evidence_object";
  }
  return "unknown";
}

Result<void> ValidateIdentityBinding(
    const ContentIdentity& identity,
    DigestSubject expected_subject,
    std::span<const std::byte> exact_bytes,
    std::string_view schema) {
  if (identity.subject != expected_subject) {
    return std::unexpected(ContractError(
        "GOV_RELATIONSHIP_INVALID", schema, "$", SubjectName(expected_subject), SubjectName(identity.subject),
        "digest subject substitution is forbidden"));
  }
  if (identity.byte_length != exact_bytes.size()) {
    return std::unexpected(ContractError(
        "GOV_RELATIONSHIP_INVALID", schema, "$", std::to_string(exact_bytes.size()),
        std::to_string(identity.byte_length), "digest byte length does not match the subject"));
  }
  auto digest = Sha256(exact_bytes);
  if (!digest) return std::unexpected(digest.error());
  if (*digest != identity.digest) {
    return std::unexpected(ContractError(
        "GOV_DIGEST_INVALID", schema, "$", digest->Hex(), identity.digest.Hex(),
        "subject bytes do not match the declared SHA-256"));
  }
  return {};
}

}  // namespace orus::contracts
