#include "orus/contracts/contracts.h"
#include "contracts/resource_monitor.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <limits>
#include <map>
#include <regex>
#include <set>
#include <tuple>

namespace orus::contracts {
namespace {

Error EvidenceError(
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
      .observed = std::string(observed.substr(0, 1024)),
      .limit = limit,
      .offset = std::nullopt,
      .message = std::string(message),
  };
}

bool IsHex(std::string_view value, std::size_t length = 64) {
  return value.size() == length && std::all_of(value.begin(), value.end(), [](char character) {
    return (character >= '0' && character <= '9') || (character >= 'a' && character <= 'f');
  });
}

bool IsId(std::string_view value) {
  if (value.empty() || value.size() > 128 ||
      !((value.front() >= 'a' && value.front() <= 'z') ||
        (value.front() >= '0' && value.front() <= '9'))) {
    return false;
  }
  return std::all_of(value.begin() + 1, value.end(), [](char character) {
    return (character >= 'a' && character <= 'z') ||
           (character >= '0' && character <= '9') || character == '.' ||
           character == '_' || character == ':' || character == '-';
  });
}

bool IsSchemaId(std::string_view value) {
  if (value.empty() || value.size() > 128 || !std::isalnum(static_cast<unsigned char>(value.front()))) {
    return false;
  }
  return std::all_of(value.begin() + 1, value.end(), [](char character) {
    return std::isalnum(static_cast<unsigned char>(character)) || character == '.' ||
           character == '_' || character == ':' || character == '-';
  });
}

bool IsRevision(std::string_view value) {
  return (value.size() == 40 || value.size() == 64) && IsHex(value, value.size());
}

bool IsRelPath(std::string_view value) {
  if (value.empty() || value.size() > 4096 || value.starts_with('/') || value.ends_with('/') ||
      !IsValidNfc(value)) {
    return false;
  }
  std::size_t start{};
  while (start <= value.size()) {
    const std::size_t slash = value.find('/', start);
    const auto component = value.substr(
        start, slash == std::string_view::npos ? value.size() - start : slash - start);
    if (component.empty() || component == "." || component == "..") return false;
    if (slash == std::string_view::npos) break;
    start = slash + 1;
  }
  return true;
}

bool IsRfc3339Seconds(std::string_view value) {
  if (value.size() != 20 || value[4] != '-' || value[7] != '-' || value[10] != 'T' ||
      value[13] != ':' || value[16] != ':' || value[19] != 'Z') {
    return false;
  }
  for (const auto index : {0U, 1U, 2U, 3U, 5U, 6U, 8U, 9U, 11U, 12U, 14U, 15U, 17U, 18U}) {
    if (value[index] < '0' || value[index] > '9') return false;
  }
  const auto number = [&](std::size_t offset) {
    return static_cast<unsigned>((value[offset] - '0') * 10 + (value[offset + 1] - '0'));
  };
  const unsigned month = number(5);
  const unsigned day = number(8);
  return month >= 1 && month <= 12 && day >= 1 && day <= 31 && number(11) <= 23 &&
         number(14) <= 59 && number(17) <= 59;
}

bool IsEnum(std::string_view value, std::initializer_list<std::string_view> allowed) {
  return std::find(allowed.begin(), allowed.end(), value) != allowed.end();
}

bool IsUint32(std::int64_t value) {
  return value >= 0 && static_cast<std::uint64_t>(value) <= std::numeric_limits<std::uint32_t>::max();
}

bool IsUint8(std::int64_t value) {
  return value >= 0 && value <= std::numeric_limits<std::uint8_t>::max();
}

bool IsInt32(std::int64_t value) {
  return value >= std::numeric_limits<std::int32_t>::min() &&
         value <= std::numeric_limits<std::int32_t>::max();
}

bool IsBoundedText(std::string_view value, std::size_t maximum, bool allow_empty = false) {
  return (allow_empty || !value.empty()) && value.size() <= maximum && IsValidNfc(value);
}

bool IsMetricUnit(std::string_view value) {
  return IsEnum(value, {"ns", "bytes", "count", "descriptors_per_second", "bytes_per_second", "ratio_ppb"});
}

std::optional<std::string_view> TopLevelStringMember(
    std::string_view bytes, std::string_view requested_name) {
  std::size_t depth{};
  for (std::size_t index = 0; index < bytes.size();) {
    const char character = bytes[index];
    if (character == '{' || character == '[') {
      ++depth;
      ++index;
      continue;
    }
    if (character == '}' || character == ']') {
      if (depth > 0) --depth;
      ++index;
      continue;
    }
    if (character != '"') {
      ++index;
      continue;
    }
    const std::size_t start = ++index;
    bool escaped{};
    while (index < bytes.size()) {
      if (escaped) {
        escaped = false;
      } else if (bytes[index] == '\\') {
        escaped = true;
      } else if (bytes[index] == '"') {
        break;
      }
      ++index;
    }
    if (index >= bytes.size()) return std::nullopt;
    const std::string_view token = bytes.substr(start, index - start);
    ++index;
    if (depth != 1 || token != requested_name || index >= bytes.size() || bytes[index] != ':') {
      continue;
    }
    ++index;
    if (index >= bytes.size() || bytes[index] != '"') return std::nullopt;
    const std::size_t value_start = ++index;
    escaped = false;
    while (index < bytes.size()) {
      if (escaped) {
        escaped = false;
      } else if (bytes[index] == '\\') {
        escaped = true;
      } else if (bytes[index] == '"') {
        return bytes.substr(value_start, index - value_start);
      }
      ++index;
    }
    return std::nullopt;
  }
  return std::nullopt;
}

bool IsSortedUniqueUint32Array(const JsonValue& value, std::size_t maximum) {
  const auto* rows = value.AsArray();
  if (rows == nullptr || rows->empty() || rows->size() > maximum) return false;
  std::optional<std::int64_t> previous;
  for (const auto& row : *rows) {
    const auto* integer = row.AsInteger();
    if (integer == nullptr || !IsUint32(*integer) || (previous && *previous >= *integer)) return false;
    previous = *integer;
  }
  return true;
}

bool IsSortedUniqueIdArray(const JsonValue& value, std::size_t maximum) {
  const auto* rows = value.AsArray();
  if (rows == nullptr || rows->empty() || rows->size() > maximum) return false;
  std::string_view previous;
  for (const auto& row : *rows) {
    const auto* id = row.AsString();
    if (id == nullptr || !IsId(*id) || (!previous.empty() && previous >= *id)) return false;
    previous = *id;
  }
  return true;
}

bool ExactFields(const JsonValue& value, std::initializer_list<std::string_view> names) {
  const auto* object = value.AsObject();
  if (object == nullptr || object->size() != names.size()) return false;
  std::set<std::string> actual;
  for (const auto& [name, unused] : *object) {
    (void)unused;
    actual.insert(name);
  }
  std::set<std::string> expected;
  for (const auto name : names) expected.emplace(name);
  return actual == expected;
}

Result<const JsonValue*> Member(
    const JsonValue& value,
    std::string_view name,
    std::string_view schema,
    std::string_view code,
    std::string_view prefix = "$.") {
  if (const auto* member = FindMember(value, name)) return member;
  return std::unexpected(EvidenceError(
      code, schema, std::string(prefix) + std::string(name), "required field", "missing",
      "required field is missing"));
}

Result<std::string_view> StringMember(
    const JsonValue& value, std::string_view name, std::string_view schema, std::string_view code,
    std::string_view prefix = "$." ) {
  auto member = Member(value, name, schema, code, prefix);
  if (!member) return std::unexpected(member.error());
  return RequireString(**member, schema, std::string(prefix) + std::string(name), code);
}

Result<std::int64_t> IntegerMember(
    const JsonValue& value, std::string_view name, std::string_view schema, std::string_view code,
    std::string_view prefix = "$." ) {
  auto member = Member(value, name, schema, code, prefix);
  if (!member) return std::unexpected(member.error());
  return RequireInteger(**member, schema, std::string(prefix) + std::string(name), code);
}

Result<bool> BooleanMember(
    const JsonValue& value, std::string_view name, std::string_view schema, std::string_view code,
    std::string_view prefix = "$." ) {
  auto member = Member(value, name, schema, code, prefix);
  if (!member) return std::unexpected(member.error());
  return RequireBoolean(**member, schema, std::string(prefix) + std::string(name), code);
}

Result<const JsonValue::Array*> ArrayMember(
    const JsonValue& value, std::string_view name, std::string_view schema, std::string_view code,
    std::string_view prefix = "$." ) {
  auto member = Member(value, name, schema, code, prefix);
  if (!member) return std::unexpected(member.error());
  return RequireArray(**member, schema, std::string(prefix) + std::string(name), code);
}

Result<void> ExactFieldSet(
    const JsonValue& value,
    std::initializer_list<std::string_view> fields,
    std::string_view schema,
    std::string_view code,
    std::string_view path = "$") {
  if (!ExactFields(value, fields)) {
    return std::unexpected(EvidenceError(
        code, schema, path, "exact documented field set", "missing or unknown field",
        "document field set is invalid"));
  }
  return {};
}

Result<void> ValidateHexMember(
    const JsonValue& value, std::string_view name, std::string_view schema, std::string_view code,
    std::size_t size = 64) {
  auto text = StringMember(value, name, schema, code);
  if (!text) return std::unexpected(text.error());
  if (!IsHex(*text, size)) {
    return std::unexpected(EvidenceError(
        code, schema, std::string("$.") + std::string(name), "lowercase hexadecimal", *text,
        "identity field is invalid"));
  }
  return {};
}

bool ValidMetric(const JsonValue& value, bool include_direction) {
  if (!ExactFields(value, include_direction ? std::initializer_list<std::string_view>{"direction", "name", "unit"}
                                            : std::initializer_list<std::string_view>{"name", "unit", "value"})) {
    return false;
  }
  const auto* name = FindMember(value, "name");
  const auto* unit = FindMember(value, "unit");
  if (name == nullptr || name->AsString() == nullptr || !IsId(*name->AsString()) ||
      unit == nullptr || unit->AsString() == nullptr || !IsMetricUnit(*unit->AsString())) {
    return false;
  }
  if (include_direction) {
    const auto* direction = FindMember(value, "direction");
    return direction != nullptr && direction->AsString() != nullptr &&
           IsEnum(*direction->AsString(), {"lower_is_better", "higher_is_better", "exact"});
  }
  return FindMember(value, "value") != nullptr && FindMember(value, "value")->AsInteger() != nullptr;
}

bool ValidStorage(const JsonValue& value, bool include_reference_rate) {
  if (!ExactFields(value, include_reference_rate
                              ? std::initializer_list<std::string_view>{
                                    "bytes", "cache_state", "device", "filesystem",
                                    "sequential_reference_bytes_per_second"}
                              : std::initializer_list<std::string_view>{
                                    "bytes", "cache_state", "device", "filesystem"})) {
    return false;
  }
  const auto* device = FindMember(value, "device");
  const auto* filesystem = FindMember(value, "filesystem");
  const auto* cache = FindMember(value, "cache_state");
  const auto* bytes = FindMember(value, "bytes");
  if (device == nullptr || device->AsString() == nullptr || !IsId(*device->AsString()) ||
      filesystem == nullptr || filesystem->AsString() == nullptr || !IsId(*filesystem->AsString()) ||
      cache == nullptr || cache->AsString() == nullptr ||
      !IsEnum(*cache->AsString(), {"cold", "warm", "mixed"}) || bytes == nullptr ||
      bytes->AsInteger() == nullptr || *bytes->AsInteger() < 0) {
    return false;
  }
  if (!include_reference_rate) return true;
  const auto* rate = FindMember(value, "sequential_reference_bytes_per_second");
  return rate != nullptr && rate->AsInteger() != nullptr && *rate->AsInteger() >= 0;
}

bool ValidRequirement(std::string_view value) {
  static const std::regex pattern("^[A-Z][A-Z0-9]*-(FR|NFR)-[0-9]{3}$");
  return std::regex_match(value.begin(), value.end(), pattern);
}

bool SameJson(const JsonValue& left, const JsonValue& right, std::string_view schema) {
  auto left_bytes = EmitCanonicalJson(left, schema);
  auto right_bytes = EmitCanonicalJson(right, schema);
  return left_bytes && right_bytes && *left_bytes == *right_bytes;
}

Result<void> CheckRecursiveBounds(
    const JsonValue& value, std::string_view schema, std::string_view code, std::size_t depth = 1) {
  if (depth > 16) {
    return std::unexpected(EvidenceError(
        code, schema, "$", "depth <=16", "depth exceeded", "document depth exceeded", 16));
  }
  if (const auto* string = value.AsString(); string != nullptr && string->size() > 4096) {
    return std::unexpected(EvidenceError(
        code, schema, "$", "string <=4096 bytes", "string exceeded", "document string bound exceeded", 4096));
  }
  if (const auto* array = value.AsArray()) {
    for (const auto& member : *array) {
      auto status = CheckRecursiveBounds(member, schema, code, depth + 1);
      if (!status) return status;
    }
  }
  if (const auto* object = value.AsObject()) {
    for (const auto& [name, member] : *object) {
      if (name.size() > 4096) {
        return std::unexpected(EvidenceError(
            code, schema, "$", "name <=4096 bytes", name, "member name bound exceeded", 4096));
      }
      auto status = CheckRecursiveBounds(member, schema, code, depth + 1);
      if (!status) return status;
    }
  }
  return {};
}

Result<void> ValidateDerivedId(const JsonValue& document, std::string_view member, std::string_view schema, std::string_view code) {
  auto declared = StringMember(document, member, schema, code);
  if (!declared) return std::unexpected(declared.error());
  if (!IsHex(*declared)) {
    return std::unexpected(EvidenceError(code, schema, std::string("$.") + std::string(member), "hex64", *declared, "derived identity is invalid"));
  }
  JsonValue::Object without;
  for (const auto& row : *document.AsObject()) {
    if (row.first != member) without.push_back(row);
  }
  auto bytes = EmitCanonicalJson(JsonValue(std::move(without)), schema);
  if (!bytes) return std::unexpected(bytes.error());
  auto digest = Sha256(*bytes);
  if (!digest || digest->Hex() != *declared) {
    return std::unexpected(EvidenceError(
        code, schema, std::string("$.") + std::string(member), digest ? digest->Hex() : "computable digest",
        *declared, "derived identity does not match canonical document content"));
  }
  return {};
}

Result<JsonValue> ParseDomain(
    std::string_view bytes,
    std::string_view fallback_schema,
    std::string_view noncanonical_code,
    ResourceUsage usage,
    std::string_view resource_code,
    std::uint64_t started_ns,
    std::uint64_t maximum_rss = 256U * 1024U * 1024U,
    std::uint64_t maximum_time = 120000000000ULL) {
  usage.input_bytes = std::max<std::uint64_t>(usage.input_bytes, bytes.size());
  auto resource = CheckResourceUsage(
      internal::ObserveResourceUsage(usage, started_ns),
      ResourceLimits{.input_bytes = 16U * 1024U * 1024U, .rss_bytes = maximum_rss, .wall_time_ns = maximum_time},
      fallback_schema, resource_code);
  if (!resource) return std::unexpected(resource.error());
  auto document = ParseCanonicalJson(
      bytes, fallback_schema, noncanonical_code,
      {.maximum_bytes = 16U * 1024U * 1024U, .maximum_depth = 16});
  if (!document) return document;
  resource = CheckResourceUsage(
      internal::ObserveResourceUsage(usage, started_ns),
      ResourceLimits{.input_bytes = 16U * 1024U * 1024U, .rss_bytes = maximum_rss,
                     .wall_time_ns = maximum_time},
      fallback_schema, resource_code);
  if (!resource) return std::unexpected(resource.error());
  return document;
}

Result<std::int64_t> Median(std::vector<std::int64_t> values, std::string_view schema) {
  if (values.empty()) {
    return std::unexpected(EvidenceError(
        "PERF_RELATIONSHIP_INVALID", schema, "$.statistics", "non-empty values", "empty",
        "statistics require at least one valid measured value"));
  }
  std::sort(values.begin(), values.end());
  const std::size_t middle = values.size() / 2;
  if (values.size() % 2 == 1) return values[middle];
  const __int128 sum = static_cast<__int128>(values[middle - 1]) + values[middle];
  const __int128 rounded = sum / 2 + ((sum % 2 != 0) ? (sum < 0 ? -1 : 1) : 0);
  if (rounded < std::numeric_limits<std::int64_t>::min() || rounded > std::numeric_limits<std::int64_t>::max()) {
    return std::unexpected(EvidenceError(
        "PERF_INTEGER_OVERFLOW", schema, "$.statistics", "signed 64-bit median", "overflow",
        "median overflowed"));
  }
  return static_cast<std::int64_t>(rounded);
}

Result<void> ValidateGovernanceCore(const JsonValue& document, std::string_view schema) {
  if (schema == "M0-SBOM-CONTRACT-v1") {
    auto fields = ExactFieldSet(document,
        {"component_count", "descriptor_id", "document_namespace", "generator", "normalization_profile",
         "orus_executable_sha256", "relationship_count", "sbom_sha256", "schema", "source_date_epoch",
         "source_revision", "spdx_document_id", "spdx_path", "validation_status"},
        schema, "GOV_FIELD_MISSING");
    if (!fields) return fields;
    for (const auto name : {"orus_executable_sha256", "sbom_sha256"}) {
      auto status = ValidateHexMember(document, name, schema, "GOV_DIGEST_INVALID");
      if (!status) return status;
    }
    auto revision = StringMember(document, "source_revision", schema, "GOV_FIELD_TYPE");
    auto profile = StringMember(document, "normalization_profile", schema, "GOV_FIELD_TYPE");
    auto status = StringMember(document, "validation_status", schema, "GOV_FIELD_TYPE");
    auto spdx_id = StringMember(document, "spdx_document_id", schema, "GOV_FIELD_TYPE");
    auto descriptor_id = StringMember(document, "descriptor_id", schema, "GOV_FIELD_TYPE");
    auto spdx_path = StringMember(document, "spdx_path", schema, "GOV_FIELD_TYPE");
    auto document_namespace = StringMember(document, "document_namespace", schema, "GOV_FIELD_TYPE");
    auto executable_digest = StringMember(document, "orus_executable_sha256", schema, "GOV_FIELD_TYPE");
    auto source_date_epoch = IntegerMember(document, "source_date_epoch", schema, "GOV_FIELD_TYPE");
    auto components = IntegerMember(document, "component_count", schema, "GOV_FIELD_TYPE");
    auto relationships = IntegerMember(document, "relationship_count", schema, "GOV_FIELD_TYPE");
    const std::string expected_namespace = revision && executable_digest
        ? std::string("https://spdx.orus.invalid/m0/") + std::string(*revision) + "/" +
              std::string(*executable_digest)
        : std::string();
    if (!revision || !IsRevision(*revision) || !descriptor_id || !IsId(*descriptor_id) ||
        !spdx_path || !IsRelPath(*spdx_path) || !document_namespace ||
        document_namespace->size() > 2048 || *document_namespace != expected_namespace ||
        !source_date_epoch || *source_date_epoch < 0 || *source_date_epoch > 253402300799LL ||
        !profile || *profile != "M0-CANONICAL-JSON-v1+SPDX-2.3-v1" || !status || *status != "valid" ||
        !spdx_id || *spdx_id != "SPDXRef-DOCUMENT" || !components || *components < 0 || *components > 100000 ||
        !relationships || *relationships < 0 || *relationships > 200000) {
      return std::unexpected(EvidenceError(
          "GOV_RELATIONSHIP_INVALID", schema, "$", "valid bounded descriptor relationships",
          "relationship mismatch", "SBOM descriptor relationship is invalid"));
    }
    auto generator = Member(document, "generator", schema, "GOV_FIELD_MISSING");
    if (!generator || !ExactFields(**generator, {"artifact_sha256", "component_id", "version"})) {
      return std::unexpected(EvidenceError(
          "GOV_FIELD_TYPE", schema, "$.generator", "exact generator object", "invalid",
          "SBOM generator identity is invalid"));
    }
    auto generator_digest = ValidateHexMember(**generator, "artifact_sha256", schema, "GOV_DIGEST_INVALID");
    if (!generator_digest) return generator_digest;
    auto component_id = StringMember(**generator, "component_id", schema, "GOV_FIELD_TYPE");
    auto version = StringMember(**generator, "version", schema, "GOV_FIELD_TYPE");
    if (!component_id || !IsId(*component_id) || !version || version->empty() || version->size() > 128) {
      return std::unexpected(EvidenceError(
          "GOV_FIELD_BOUND", schema, "$.generator", "bounded component identity and version", "invalid",
          "SBOM generator fields are invalid"));
    }
    return {};
  }

  if (schema != "M0-RELEASE-EVIDENCE-v1") {
    return std::unexpected(EvidenceError(
        "GOV_SCHEMA_UNKNOWN", schema, "$.schema", "M0-SBOM-CONTRACT-v1|M0-RELEASE-EVIDENCE-v1",
        schema, "governance schema is unknown"));
  }
  auto fields = ExactFieldSet(document,
      {"approvals", "build_id", "claim_scope", "environment_id", "evidence", "orus_executable_sha256",
       "package_tree_sha256", "release_id", "sbom_sha256", "schema", "source_revision", "state", "validators"},
      schema, "GOV_FIELD_MISSING");
  if (!fields) return fields;
  for (const auto name : {"build_id", "environment_id", "orus_executable_sha256", "package_tree_sha256", "sbom_sha256"}) {
    auto status = ValidateHexMember(document, name, schema, "GOV_DIGEST_INVALID");
    if (!status) return status;
  }
  auto claim_scope = StringMember(document, "claim_scope", schema, "GOV_FIELD_TYPE");
  auto state = StringMember(document, "state", schema, "GOV_FIELD_TYPE");
  auto release_id = StringMember(document, "release_id", schema, "GOV_FIELD_TYPE");
  auto source_revision = StringMember(document, "source_revision", schema, "GOV_FIELD_TYPE");
  auto evidence = ArrayMember(document, "evidence", schema, "GOV_FIELD_TYPE");
  auto validators = ArrayMember(document, "validators", schema, "GOV_FIELD_TYPE");
  auto approvals = ArrayMember(document, "approvals", schema, "GOV_FIELD_TYPE");
  if (!claim_scope || *claim_scope != "m0_only" || !state ||
      !IsEnum(*state, {"assembled", "validating", "rejected", "preapproval_validated"}) ||
      !release_id || !IsId(*release_id) || !source_revision || !IsRevision(*source_revision) ||
      !evidence || !validators || !approvals ||
      (*evidence)->empty() || (*evidence)->size() > 12 || (*validators)->size() > 12 || (*approvals)->size() > 3) {
    return std::unexpected(EvidenceError(
        "GOV_FIELD_BOUND", schema, "$", "bounded M0 release inventories", "invalid count/value",
        "release evidence inventory is invalid"));
  }
  static const std::set<std::string> kEvidenceTypes{
      "adr_approval", "build_facts", "canonical_commands", "ci_gate", "claim_scan", "corpus_reliability",
      "license_notice", "performance_tooling", "reference_environment", "sanitizer_fuzz", "sbom_descriptor",
      "security_controls"};
  static const std::set<std::string> kValidatorIds{
      "adr_protected_decisions", "build_reference", "canonical_commands", "ci_gate", "claim_scan",
      "corpus_reliability", "dependency_sbom", "license_notice", "performance_tooling", "sanitizer_fuzz",
      "security_controls", "subject_identity"};
  std::set<std::string> types;
  std::set<std::string> paths;
  for (const auto& row : **evidence) {
    if (!ExactFields(row, {"byte_length", "evidence_object_sha256", "path", "producer", "producer_version", "schema", "type"})) {
      return std::unexpected(EvidenceError("GOV_FIELD_TYPE", schema, "$.evidence[]", "exact evidence row", "invalid", "evidence row is invalid"));
    }
    auto type = StringMember(row, "type", schema, "GOV_FIELD_TYPE");
    auto path = StringMember(row, "path", schema, "GOV_FIELD_TYPE");
    auto length = IntegerMember(row, "byte_length", schema, "GOV_FIELD_TYPE");
    auto row_schema = StringMember(row, "schema", schema, "GOV_FIELD_TYPE");
    auto producer = StringMember(row, "producer", schema, "GOV_FIELD_TYPE");
    auto producer_version = StringMember(row, "producer_version", schema, "GOV_FIELD_TYPE");
    auto digest = ValidateHexMember(row, "evidence_object_sha256", schema, "GOV_DIGEST_INVALID");
    if (!digest) return digest;
    static const std::set<std::string_view> kForbiddenSchemas{
        "M0-RELEASE-APPROVAL-v1",
        "M0-RELEASE-EVIDENCE-v1",
        "M0-SECRET-SCAN-MANIFEST-v1",
        "M0-SECRET-SCAN-METADATA-v1",
        "M0-SECRET-SCAN-REPORT-v1",
    };
    const bool forbidden_cycle = row_schema && kForbiddenSchemas.contains(*row_schema);
    if (!type || !kEvidenceTypes.contains(std::string(*type)) || !path || path->empty() || !length || *length < 0 ||
        !row_schema || !IsSchemaId(*row_schema) || !producer || !IsId(*producer) ||
        !producer_version || producer_version->empty() || producer_version->size() > 128 ||
        !types.insert(std::string(*type)).second || !paths.insert(std::string(*path)).second ||
        !IsRelPath(*path) || forbidden_cycle) {
      return std::unexpected(EvidenceError(
          "GOV_RELATIONSHIP_INVALID", schema, "$.evidence[]", "unique finite subject-bound row",
          type ? *type : "invalid", "release evidence row relationship is invalid"));
    }
  }
  std::set<std::string> validator_ids;
  bool validators_pass = true;
  std::string previous_validator;
  for (const auto& row : **validators) {
    if (!ExactFields(row, {"diagnostic", "finding_count", "status", "validator_id", "version"})) {
      return std::unexpected(EvidenceError("GOV_FIELD_TYPE", schema, "$.validators[]", "exact validator row", "invalid", "validator row is invalid"));
    }
    auto id = StringMember(row, "validator_id", schema, "GOV_FIELD_TYPE");
    auto row_status = StringMember(row, "status", schema, "GOV_FIELD_TYPE");
    auto findings = IntegerMember(row, "finding_count", schema, "GOV_FIELD_TYPE");
    auto version = StringMember(row, "version", schema, "GOV_FIELD_TYPE");
    const auto* diagnostic = FindMember(row, "diagnostic");
    const bool diagnostic_valid = diagnostic != nullptr &&
        (diagnostic->IsNull() || (diagnostic->AsString() != nullptr && !diagnostic->AsString()->empty() &&
                                 diagnostic->AsString()->size() <= 4096));
    if (!id || !kValidatorIds.contains(std::string(*id)) || !validator_ids.insert(std::string(*id)).second ||
        (!previous_validator.empty() && previous_validator >= *id) || !row_status ||
        (*row_status != "pass" && *row_status != "fail") || !findings || *findings < 0 ||
        *findings > std::numeric_limits<std::uint32_t>::max() || !version || version->empty() ||
        version->size() > 128 || !diagnostic_valid || (*row_status == "pass" && !diagnostic->IsNull())) {
      return std::unexpected(EvidenceError("GOV_RELATIONSHIP_INVALID", schema, "$.validators[]", "finite unique validator", "invalid", "validator relationship is invalid"));
    }
    previous_validator = std::string(*id);
    validators_pass = validators_pass && *row_status == "pass" && *findings == 0 && diagnostic->IsNull();
  }
  std::set<std::string> roles;
  std::set<std::string> identities;
  std::string previous_role;
  for (const auto& row : **approvals) {
    if (!ExactFields(row, {"decision", "identity", "role", "time"})) {
      return std::unexpected(EvidenceError("GOV_FIELD_TYPE", schema, "$.approvals[]", "exact approval row", "invalid", "approval row is invalid"));
    }
    auto role = StringMember(row, "role", schema, "GOV_FIELD_TYPE");
    auto identity = StringMember(row, "identity", schema, "GOV_FIELD_TYPE");
    auto decision = StringMember(row, "decision", schema, "GOV_FIELD_TYPE");
    auto time = StringMember(row, "time", schema, "GOV_FIELD_TYPE");
    if (!role || !IsEnum(*role, {"product_owner", "release_owner", "security_owner"}) ||
        (!previous_role.empty() && previous_role >= *role) || !identity || !IsId(*identity) ||
        !decision || *decision != "approved" || !time || !IsRfc3339Seconds(*time) ||
        !roles.insert(std::string(*role)).second || !identities.insert(std::string(*identity)).second) {
      return std::unexpected(EvidenceError("GOV_RELATIONSHIP_INVALID", schema, "$.approvals[]", "unique approved role/identity", "invalid", "approval relationship is invalid"));
    }
    previous_role = std::string(*role);
  }
  if (*state == "preapproval_validated") {
    const std::set<std::string> expected_roles{"product_owner", "release_owner", "security_owner"};
    if (types != kEvidenceTypes || validator_ids != kValidatorIds || !validators_pass || roles != expected_roles ||
        (*evidence)->size() != 12 || (*validators)->size() != 12 || (*approvals)->size() != 3) {
      return std::unexpected(EvidenceError(
          "GOV_RELATIONSHIP_INVALID", schema, "$.state", "exact 12 evidence/12 passing validators/3 roles",
          "incomplete preapproval", "preapproval state is forged"));
    }
  } else if (*state != "assembled" && *state != "validating" && *state != "rejected") {
    return std::unexpected(EvidenceError("GOV_ENUM_INVALID", schema, "$.state", "known release state", *state, "release state is invalid"));
  }
  return {};
}

Result<void> ValidatePerformanceCore(const JsonValue& document, std::string_view schema) {
  auto bounds = CheckRecursiveBounds(document, schema, "PERF_FIELD_BOUND");
  if (!bounds) return bounds;
  if (schema == "M0-PERF-WORKLOAD-v1") {
    auto fields = ExactFieldSet(document,
        {"allocation_phase", "arguments", "concurrency", "data_size", "dataset", "executable",
         "introducing_requirement", "measured_operation", "metric", "owner", "registry_sha256", "sampling",
         "schema", "setup", "storage_applicable", "timeout_ms", "workload_id", "workload_version"},
        schema, "PERF_FIELD_MISSING");
    if (!fields) return fields;
    auto concurrency = IntegerMember(document, "concurrency", schema, "PERF_FIELD_TYPE");
    auto version = IntegerMember(document, "workload_version", schema, "PERF_FIELD_TYPE");
    auto timeout = IntegerMember(document, "timeout_ms", schema, "PERF_FIELD_TYPE");
    auto data_size = IntegerMember(document, "data_size", schema, "PERF_FIELD_TYPE");
    auto workload_id = StringMember(document, "workload_id", schema, "PERF_FIELD_TYPE");
    auto owner = StringMember(document, "owner", schema, "PERF_FIELD_TYPE");
    auto executable = StringMember(document, "executable", schema, "PERF_FIELD_TYPE");
    auto dataset = StringMember(document, "dataset", schema, "PERF_FIELD_TYPE");
    auto setup = StringMember(document, "setup", schema, "PERF_FIELD_TYPE");
    auto measured = StringMember(document, "measured_operation", schema, "PERF_FIELD_TYPE");
    auto allocation_phase = StringMember(document, "allocation_phase", schema, "PERF_FIELD_TYPE");
    auto requirement = StringMember(document, "introducing_requirement", schema, "PERF_FIELD_TYPE");
    auto storage_applicable = BooleanMember(document, "storage_applicable", schema, "PERF_FIELD_TYPE");
    auto arguments = ArrayMember(document, "arguments", schema, "PERF_FIELD_TYPE");
    auto metric_member = Member(document, "metric", schema, "PERF_FIELD_MISSING");
    auto digest = ValidateHexMember(document, "registry_sha256", schema, "PERF_DIGEST_INVALID");
    auto sampling_member = Member(document, "sampling", schema, "PERF_FIELD_MISSING");
    bool arguments_valid = arguments && (*arguments)->size() <= 64;
    if (arguments_valid) {
      arguments_valid = std::all_of((*arguments)->begin(), (*arguments)->end(), [](const JsonValue& argument) {
        return argument.AsString() != nullptr && IsBoundedText(*argument.AsString(), 1024, true);
      });
    }
    if (!concurrency || !IsUint32(*concurrency) || *concurrency < 1 || !version || !IsUint32(*version) ||
        *version < 1 || !timeout || !IsUint32(*timeout) || *timeout < 1 || *timeout > 600000 ||
        !data_size || *data_size < 0 || !workload_id || !IsId(*workload_id) || !owner || !IsId(*owner) ||
        !executable || executable->size() > 512 || !executable->starts_with("//") ||
        !dataset || !IsBoundedText(*dataset, 4096) || !setup || !IsBoundedText(*setup, 4096) ||
        !measured || !IsBoundedText(*measured, 4096) || !allocation_phase ||
        !IsEnum(*allocation_phase, {"none", "startup", "steady_state"}) || !requirement ||
        !ValidRequirement(*requirement) || !storage_applicable || !arguments_valid || !metric_member ||
        !ValidMetric(**metric_member, true) || !digest || !sampling_member ||
        !ExactFields(**sampling_member, {"minimum_measured_pairs", "mode", "resamples", "warmup_pairs"})) {
      return std::unexpected(EvidenceError("PERF_RELATIONSHIP_INVALID", schema, "$", "valid workload relationships", "invalid", "workload relationship is invalid"));
    }
    auto mode = StringMember(**sampling_member, "mode", schema, "PERF_FIELD_TYPE");
    auto resamples = IntegerMember(**sampling_member, "resamples", schema, "PERF_FIELD_TYPE");
    auto warmups = IntegerMember(**sampling_member, "warmup_pairs", schema, "PERF_FIELD_TYPE");
    auto minimum = IntegerMember(**sampling_member, "minimum_measured_pairs", schema, "PERF_FIELD_TYPE");
    if (!mode || !resamples || !IsUint32(*resamples) || !warmups || !IsUint32(*warmups) ||
        !minimum || !IsUint32(*minimum) || *minimum < 1 ||
        (*mode == "paired" && (*resamples != 10000 || *warmups != 5 || *minimum < 30)) ||
        (*mode == "single" && *resamples != 0) || (*mode != "paired" && *mode != "single")) {
      return std::unexpected(EvidenceError("PERF_RELATIONSHIP_INVALID", schema, "$.sampling", "valid sampling contract", "invalid", "sampling relationship is invalid"));
    }
    return {};
  }
  if (schema == "M0-PERF-RAW-SAMPLE-v1") {
    auto fields = ExactFieldSet(document,
        {"allocation", "background_utilization_ppm", "cpu", "duration_ns", "end_ns", "invalid_reason", "metric",
         "migration_count", "order", "pair_index", "phase", "run_id", "schema", "start_ns", "storage",
         "throttled", "valid", "workload_id", "workload_version"},
        schema, "PERF_FIELD_MISSING");
    if (!fields) return fields;
    auto start = IntegerMember(document, "start_ns", schema, "PERF_FIELD_TYPE");
    auto end = IntegerMember(document, "end_ns", schema, "PERF_FIELD_TYPE");
    auto duration = IntegerMember(document, "duration_ns", schema, "PERF_FIELD_TYPE");
    auto valid = BooleanMember(document, "valid", schema, "PERF_FIELD_TYPE");
    auto reason = StringMember(document, "invalid_reason", schema, "PERF_FIELD_TYPE");
    auto run_id = StringMember(document, "run_id", schema, "PERF_FIELD_TYPE");
    auto workload_id = StringMember(document, "workload_id", schema, "PERF_FIELD_TYPE");
    auto version = IntegerMember(document, "workload_version", schema, "PERF_FIELD_TYPE");
    auto phase = StringMember(document, "phase", schema, "PERF_FIELD_TYPE");
    auto order = StringMember(document, "order", schema, "PERF_FIELD_TYPE");
    auto pair_index = IntegerMember(document, "pair_index", schema, "PERF_FIELD_TYPE");
    auto cpu = IntegerMember(document, "cpu", schema, "PERF_FIELD_TYPE");
    auto migrations = IntegerMember(document, "migration_count", schema, "PERF_FIELD_TYPE");
    auto throttled = BooleanMember(document, "throttled", schema, "PERF_FIELD_TYPE");
    auto background = IntegerMember(document, "background_utilization_ppm", schema, "PERF_FIELD_TYPE");
    auto metric = Member(document, "metric", schema, "PERF_FIELD_MISSING");
    const auto* allocation = FindMember(document, "allocation");
    const auto* storage = FindMember(document, "storage");
    const bool allocation_valid = allocation != nullptr &&
        (allocation->IsNull() ||
         (ExactFields(*allocation, {"bytes", "calls"}) &&
          FindMember(*allocation, "calls")->AsInteger() != nullptr &&
          *FindMember(*allocation, "calls")->AsInteger() >= 0 &&
          FindMember(*allocation, "bytes")->AsInteger() != nullptr &&
          *FindMember(*allocation, "bytes")->AsInteger() >= 0));
    const bool storage_valid = storage != nullptr && (storage->IsNull() || ValidStorage(*storage, false));
    if (!start || !end || !duration || *start < 0 || *end < *start || *duration < 0 ||
        static_cast<__int128>(*end) - *start != *duration || !valid || !reason ||
        !IsEnum(*reason, {"none", "timeout", "process_failure", "migration", "throttle",
                         "background_load", "counter_error", "cancelled"}) ||
        (*valid != (*reason == "none")) || !run_id || !IsId(*run_id) || !workload_id ||
        !IsId(*workload_id) || !version || !IsUint32(*version) || !phase ||
        !IsEnum(*phase, {"warmup", "measured"}) || !order ||
        !IsEnum(*order, {"baseline_first", "candidate_first", "standalone"}) ||
        !pair_index || !IsUint32(*pair_index) || !cpu || !IsUint32(*cpu) || !migrations ||
        !IsUint32(*migrations) || !throttled || !background || !IsUint32(*background) ||
        *background > 1000000 || !metric || !ValidMetric(**metric, false) ||
        !allocation_valid || !storage_valid) {
      return std::unexpected(EvidenceError("PERF_RELATIONSHIP_INVALID", schema, "$", "consistent time/validity fields", "mismatch", "raw sample relationship is invalid"));
    }
    return {};
  }
  if (schema == "M0-CONTROLLED-RUNNER-v1") {
    auto fields = ExactFieldSet(document,
        {"authority", "conformance", "contract_id", "contract_sha256", "measurement", "predicates", "run_id",
         "runner_class", "runner_id", "schema"}, schema, "PERF_FIELD_MISSING");
    if (!fields) return fields;
    auto digest = ValidateDerivedId(document, "contract_sha256", schema, "PERF_DIGEST_INVALID");
    if (!digest) return digest;
    auto runner_class = StringMember(document, "runner_class", schema, "PERF_FIELD_TYPE");
    auto authority = StringMember(document, "authority", schema, "PERF_FIELD_TYPE");
    auto conformance = StringMember(document, "conformance", schema, "PERF_FIELD_TYPE");
    auto runner_id = StringMember(document, "runner_id", schema, "PERF_FIELD_TYPE");
    auto contract_id = StringMember(document, "contract_id", schema, "PERF_FIELD_TYPE");
    auto run_id = StringMember(document, "run_id", schema, "PERF_FIELD_TYPE");
    auto predicates = ArrayMember(document, "predicates", schema, "PERF_FIELD_TYPE");
    auto measurement = Member(document, "measurement", schema, "PERF_FIELD_MISSING");
    if (!runner_class || !IsEnum(*runner_class, {"controlled", "github_hosted_shared", "other"}) ||
        !authority || !IsEnum(*authority, {"authoritative", "advisory", "invalid"}) ||
        !conformance || !IsEnum(*conformance, {"pass", "fail", "unknown"}) ||
        !runner_id || !IsId(*runner_id) || !contract_id || !IsId(*contract_id) ||
        !run_id || !IsId(*run_id) || !predicates || (*predicates)->empty() ||
        (*predicates)->size() > 256 || !measurement ||
        !ExactFields(**measurement, {"affinity", "clock", "max_background_utilization_ppm",
                                     "migration_count", "preflight_ms", "storage_applicable",
                                     "throttle_events"})) {
      return std::unexpected(EvidenceError("PERF_RELATIONSHIP_INVALID", schema, "$", "truthful runner authority", "invalid", "runner authority is invalid"));
    }
    auto affinity = Member(**measurement, "affinity", schema, "PERF_FIELD_MISSING", "$.measurement.");
    auto clock = StringMember(**measurement, "clock", schema, "PERF_FIELD_TYPE", "$.measurement.");
    auto background = IntegerMember(**measurement, "max_background_utilization_ppm", schema, "PERF_FIELD_TYPE", "$.measurement.");
    auto migrations = IntegerMember(**measurement, "migration_count", schema, "PERF_FIELD_TYPE", "$.measurement.");
    auto preflight = IntegerMember(**measurement, "preflight_ms", schema, "PERF_FIELD_TYPE", "$.measurement.");
    auto storage = BooleanMember(**measurement, "storage_applicable", schema, "PERF_FIELD_TYPE", "$.measurement.");
    auto throttles = IntegerMember(**measurement, "throttle_events", schema, "PERF_FIELD_TYPE", "$.measurement.");
    if (!affinity || !IsSortedUniqueUint32Array(**affinity, 1024) || !clock || !IsId(*clock) ||
        !background || !IsUint32(*background) || !migrations || !IsUint32(*migrations) ||
        !preflight || *preflight != 5000 || !storage || !throttles || !IsUint32(*throttles)) {
      return std::unexpected(EvidenceError("PERF_FIELD_BOUND", schema, "$.measurement", "bounded controlled-runner measurement", "invalid", "runner measurement is invalid"));
    }
    std::string previous_path;
    bool predicates_pass = true;
    for (const auto& row : **predicates) {
      if (!ExactFields(row, {"expected", "observed", "operator", "path", "status"})) {
        return std::unexpected(EvidenceError("PERF_FIELD_TYPE", schema, "$.predicates[]", "exact predicate row", "invalid", "runner predicate is invalid"));
      }
      auto path = StringMember(row, "path", schema, "PERF_FIELD_TYPE");
      auto operation = StringMember(row, "operator", schema, "PERF_FIELD_TYPE");
      auto status = StringMember(row, "status", schema, "PERF_FIELD_TYPE");
      const auto* expected = FindMember(row, "expected");
      const auto* observed = FindMember(row, "observed");
      if (!path || !IsBoundedText(*path, 256) || (!previous_path.empty() && previous_path >= *path) ||
          !operation || !IsEnum(*operation, {"eq", "u32_eq", "set_eq", "max_exclusive"}) ||
          !status || !IsEnum(*status, {"pass", "mismatch", "unavailable"}) || expected == nullptr ||
          observed == nullptr || (observed->IsNull() != (*status == "unavailable"))) {
        return std::unexpected(EvidenceError("PERF_RELATIONSHIP_INVALID", schema, "$.predicates[]", "sorted typed predicate", "invalid", "runner predicate relationship is invalid"));
      }
      bool comparable = !observed->IsNull();
      bool matches = false;
      if (*operation == "eq") {
        const bool strings = expected->AsString() != nullptr && IsBoundedText(*expected->AsString(), 4096) &&
                             (!comparable || (observed->AsString() != nullptr && IsBoundedText(*observed->AsString(), 4096)));
        const bool booleans = expected->AsBoolean() != nullptr && (!comparable || observed->AsBoolean() != nullptr);
        comparable = comparable && (strings || booleans);
        matches = comparable && SameJson(*expected, *observed, schema);
      } else if (*operation == "set_eq") {
        comparable = comparable && IsSortedUniqueIdArray(*expected, 1024) && IsSortedUniqueIdArray(*observed, 1024);
        matches = comparable && SameJson(*expected, *observed, schema);
      } else {
        comparable = comparable && expected->AsInteger() != nullptr && IsUint32(*expected->AsInteger()) &&
                     observed->AsInteger() != nullptr && IsUint32(*observed->AsInteger());
        matches = comparable && (*operation == "u32_eq" ? *expected->AsInteger() == *observed->AsInteger()
                                                          : *observed->AsInteger() < *expected->AsInteger());
      }
      if ((!observed->IsNull() && !comparable) || (*status == "pass") != matches ||
          (*status == "mismatch") != (comparable && !matches)) {
        return std::unexpected(EvidenceError("PERF_RELATIONSHIP_INVALID", schema, "$.predicates[]", "truthful predicate status", "mismatch", "runner predicate status is forged"));
      }
      previous_path = std::string(*path);
      predicates_pass = predicates_pass && *status == "pass";
    }
    const bool controlled_pass = *runner_class == "controlled" && predicates_pass && *migrations == 0 &&
                                 *throttles == 0 && *background < 10000;
    const bool authority_valid = controlled_pass
        ? (*conformance == "pass" && *authority == "authoritative")
        : (*runner_class == "github_hosted_shared"
               ? (*conformance != "pass" && *authority == "advisory")
               : (*conformance != "pass" && *authority == "invalid"));
    if (!authority_valid) {
      return std::unexpected(EvidenceError("PERF_RELATIONSHIP_INVALID", schema, "$", "truthful runner authority", "mismatch", "runner authority is forged"));
    }
    return {};
  }
  if (schema == "M0-PERF-RESULT-v1") {
    auto fields = ExactFieldSet(document,
        {"artifact_sha256", "artifact_subject", "attempt_id", "build_id", "compiler", "concurrency",
         "configuration", "dataset", "host", "metric", "noise_state", "optimization", "raw_samples", "result_id",
         "role", "run_id", "runner", "sampling", "schema", "source_revision", "statistics", "storage",
         "tool_version", "validation_state", "workload_id", "workload_version"},
        schema, "PERF_FIELD_MISSING");
    if (!fields) return fields;
    for (const auto name : {"artifact_sha256", "build_id"}) {
      auto digest = ValidateHexMember(document, name, schema, "PERF_DIGEST_INVALID");
      if (!digest) return digest;
    }
    auto id = ValidateDerivedId(document, "result_id", schema, "PERF_DIGEST_INVALID");
    if (!id) return id;
    auto run_id = StringMember(document, "run_id", schema, "PERF_FIELD_TYPE");
    auto workload_id = StringMember(document, "workload_id", schema, "PERF_FIELD_TYPE");
    auto workload_version = IntegerMember(document, "workload_version", schema, "PERF_FIELD_TYPE");
    auto role = StringMember(document, "role", schema, "PERF_FIELD_TYPE");
    auto dataset = StringMember(document, "dataset", schema, "PERF_FIELD_TYPE");
    auto concurrency = IntegerMember(document, "concurrency", schema, "PERF_FIELD_TYPE");
    auto revision = StringMember(document, "source_revision", schema, "PERF_FIELD_TYPE");
    auto artifact_subject = StringMember(document, "artifact_subject", schema, "PERF_FIELD_TYPE");
    auto configuration = StringMember(document, "configuration", schema, "PERF_FIELD_TYPE");
    auto compiler = StringMember(document, "compiler", schema, "PERF_FIELD_TYPE");
    auto optimization = StringMember(document, "optimization", schema, "PERF_FIELD_TYPE");
    auto noise = StringMember(document, "noise_state", schema, "PERF_FIELD_TYPE");
    auto tool_version = StringMember(document, "tool_version", schema, "PERF_FIELD_TYPE");
    auto attempt_id = StringMember(document, "attempt_id", schema, "PERF_FIELD_TYPE");
    auto metric = Member(document, "metric", schema, "PERF_FIELD_MISSING");
    auto runner = Member(document, "runner", schema, "PERF_FIELD_MISSING");
    auto host = Member(document, "host", schema, "PERF_FIELD_MISSING");
    const auto* storage = FindMember(document, "storage");
    if (!run_id || !IsId(*run_id) || !workload_id || !IsId(*workload_id) ||
        !workload_version || !IsUint32(*workload_version) || *workload_version < 1 ||
        !role || !IsEnum(*role, {"baseline", "candidate", "standalone"}) ||
        !dataset || !IsBoundedText(*dataset, 4096) || !concurrency || !IsUint32(*concurrency) ||
        *concurrency < 1 || !revision || !IsRevision(*revision) || !artifact_subject ||
        !IsEnum(*artifact_subject, {"orus_executable", "benchmark_fixture"}) ||
        !configuration || !IsBoundedText(*configuration, 256) || !compiler ||
        !IsBoundedText(*compiler, 256) || !optimization || !IsBoundedText(*optimization, 256) ||
        !noise || !IsEnum(*noise, {"pass", "failed"}) || !tool_version ||
        !IsBoundedText(*tool_version, 128) || !attempt_id || !IsId(*attempt_id) ||
        !metric || !ValidMetric(**metric, true) || !runner ||
        !ExactFields(**runner, {"authority", "conformance", "contract_sha256", "runner_class", "runner_id"}) ||
        !host || !ExactFields(**host, {"affinity", "boost", "cpu", "frequency_khz", "governor",
                                      "image_sha256", "isa", "kernel", "numa"}) ||
        storage == nullptr || (!storage->IsNull() && !ValidStorage(*storage, true))) {
      return std::unexpected(EvidenceError("PERF_RELATIONSHIP_INVALID", schema, "$", "complete typed result provenance", "invalid", "result provenance is invalid"));
    }
    auto runner_id = StringMember(**runner, "runner_id", schema, "PERF_FIELD_TYPE");
    auto runner_class = StringMember(**runner, "runner_class", schema, "PERF_FIELD_TYPE");
    auto runner_conformance = StringMember(**runner, "conformance", schema, "PERF_FIELD_TYPE");
    auto runner_authority = StringMember(**runner, "authority", schema, "PERF_FIELD_TYPE");
    auto runner_digest = ValidateHexMember(**runner, "contract_sha256", schema, "PERF_DIGEST_INVALID");
    if (!runner_id || !IsId(*runner_id) || !runner_class ||
        !IsEnum(*runner_class, {"controlled", "github_hosted_shared", "other"}) ||
        !runner_conformance || !IsEnum(*runner_conformance, {"pass", "fail", "unknown"}) ||
        !runner_authority || !IsEnum(*runner_authority, {"authoritative", "advisory", "invalid"}) ||
        !runner_digest || (*runner_authority == "authoritative" &&
                           (*runner_class != "controlled" || *runner_conformance != "pass")) ||
        (*runner_class == "github_hosted_shared" && *runner_authority != "advisory")) {
      return std::unexpected(EvidenceError("PERF_RELATIONSHIP_INVALID", schema, "$.runner", "truthful runner authority", "invalid", "result runner identity is invalid"));
    }
    auto affinity = Member(**host, "affinity", schema, "PERF_FIELD_MISSING", "$.host.");
    auto cpu_text = StringMember(**host, "cpu", schema, "PERF_FIELD_TYPE", "$.host.");
    auto isa = StringMember(**host, "isa", schema, "PERF_FIELD_TYPE", "$.host.");
    auto numa = StringMember(**host, "numa", schema, "PERF_FIELD_TYPE", "$.host.");
    auto kernel = StringMember(**host, "kernel", schema, "PERF_FIELD_TYPE", "$.host.");
    auto governor = StringMember(**host, "governor", schema, "PERF_FIELD_TYPE", "$.host.");
    auto frequency = IntegerMember(**host, "frequency_khz", schema, "PERF_FIELD_TYPE", "$.host.");
    auto boost = StringMember(**host, "boost", schema, "PERF_FIELD_TYPE", "$.host.");
    auto image_digest = ValidateHexMember(**host, "image_sha256", schema, "PERF_DIGEST_INVALID");
    if (!affinity || !IsSortedUniqueUint32Array(**affinity, 1024) || !cpu_text ||
        !IsBoundedText(*cpu_text, 4096) || !isa || !IsBoundedText(*isa, 128) || !numa ||
        !IsBoundedText(*numa, 4096) || !kernel || !IsBoundedText(*kernel, 4096) || !governor ||
        !IsBoundedText(*governor, 4096) || !frequency || *frequency < 0 || !boost ||
        !IsEnum(*boost, {"enabled", "disabled", "unknown"}) || !image_digest) {
      return std::unexpected(EvidenceError("PERF_FIELD_BOUND", schema, "$.host", "bounded host identity", "invalid", "result host identity is invalid"));
    }
    auto sampling_member = Member(document, "sampling", schema, "PERF_FIELD_MISSING");
    auto raw_member = Member(document, "raw_samples", schema, "PERF_FIELD_MISSING");
    if (!sampling_member || !raw_member || !ExactFields(**sampling_member,
        {"invalid_pairs", "measured_pairs_attempted", "minimum_measured_pairs", "mode", "resamples", "timeout_ms",
         "valid_pairs", "warmup_pairs_completed", "warmup_pairs_required"}) ||
        !ExactFields(**raw_member, {"byte_length", "count", "path", "sha256", "storage"})) {
      return std::unexpected(EvidenceError("PERF_FIELD_TYPE", schema, "$", "exact sampling/raw objects", "invalid", "result nested object is invalid"));
    }
    auto attempted = IntegerMember(**sampling_member, "measured_pairs_attempted", schema, "PERF_FIELD_TYPE");
    auto valid_pairs = IntegerMember(**sampling_member, "valid_pairs", schema, "PERF_FIELD_TYPE");
    auto invalid_pairs = IntegerMember(**sampling_member, "invalid_pairs", schema, "PERF_FIELD_TYPE");
    auto warmups = IntegerMember(**sampling_member, "warmup_pairs_completed", schema, "PERF_FIELD_TYPE");
    auto required_warmups = IntegerMember(**sampling_member, "warmup_pairs_required", schema, "PERF_FIELD_TYPE");
    auto minimum = IntegerMember(**sampling_member, "minimum_measured_pairs", schema, "PERF_FIELD_TYPE");
    auto resamples = IntegerMember(**sampling_member, "resamples", schema, "PERF_FIELD_TYPE");
    auto timeout = IntegerMember(**sampling_member, "timeout_ms", schema, "PERF_FIELD_TYPE");
    auto mode = StringMember(**sampling_member, "mode", schema, "PERF_FIELD_TYPE");
    auto count = IntegerMember(**raw_member, "count", schema, "PERF_FIELD_TYPE");
    auto byte_length = IntegerMember(**raw_member, "byte_length", schema, "PERF_FIELD_TYPE");
    auto raw_storage = StringMember(**raw_member, "storage", schema, "PERF_FIELD_TYPE");
    const auto* raw_path = FindMember(**raw_member, "path");
    auto raw_digest = ValidateHexMember(**raw_member, "sha256", schema, "PERF_DIGEST_INVALID");
    const bool external_path = raw_path != nullptr && raw_path->AsString() != nullptr && IsRelPath(*raw_path->AsString());
    const bool embedded_path = raw_path != nullptr && raw_path->IsNull();
    if (!attempted || !IsUint32(*attempted) || !valid_pairs || !IsUint32(*valid_pairs) ||
        !invalid_pairs || !IsUint32(*invalid_pairs) || !warmups || !IsUint32(*warmups) ||
        !required_warmups || !IsUint32(*required_warmups) || !minimum || !IsUint32(*minimum) ||
        *minimum < 1 || !resamples || !IsUint32(*resamples) || !timeout || !IsUint32(*timeout) ||
        *timeout < 1 || *timeout > 600000 || !mode || !IsEnum(*mode, {"single", "paired"}) ||
        !count || !IsUint32(*count) || !byte_length || *byte_length < 0 || !raw_storage ||
        !IsEnum(*raw_storage, {"embedded", "external"}) ||
        (*raw_storage == "embedded" ? !embedded_path : !external_path) || !raw_digest ||
        static_cast<__int128>(*valid_pairs) + *invalid_pairs != *attempted ||
        static_cast<__int128>(*count) != static_cast<__int128>(*warmups) + *attempted || *count > 100000 ||
        *warmups != *required_warmups || *attempted < *minimum ||
        (*mode == "single" && (*required_warmups != 0 || *resamples != 0 || *role != "standalone")) ||
        (*mode == "paired" && (*required_warmups != 5 || *minimum < 30 || *resamples != 10000 ||
                               *role == "standalone"))) {
      return std::unexpected(EvidenceError("PERF_RELATIONSHIP_INVALID", schema, "$.sampling", "reconciled sample counts", "mismatch", "result sample counts are invalid"));
    }
    auto statistics = Member(document, "statistics", schema, "PERF_FIELD_MISSING");
    if (!statistics || !ExactFields(**statistics,
        {"maximum", "median", "median_absolute_deviation", "minimum", "percentiles"})) {
      return std::unexpected(EvidenceError("PERF_FIELD_TYPE", schema, "$.statistics", "exact statistics object", "invalid", "result statistics are invalid"));
    }
    auto minimum_value = IntegerMember(**statistics, "minimum", schema, "PERF_FIELD_TYPE");
    auto maximum_value = IntegerMember(**statistics, "maximum", schema, "PERF_FIELD_TYPE");
    auto median_value = IntegerMember(**statistics, "median", schema, "PERF_FIELD_TYPE");
    auto mad = IntegerMember(**statistics, "median_absolute_deviation", schema, "PERF_FIELD_TYPE");
    auto percentiles = ArrayMember(**statistics, "percentiles", schema, "PERF_FIELD_TYPE");
    if (!minimum_value || !maximum_value || *minimum_value > *maximum_value || !median_value ||
        *median_value < *minimum_value || *median_value > *maximum_value || !mad || *mad < 0 ||
        !percentiles || (*percentiles)->size() > 32) {
      return std::unexpected(EvidenceError("PERF_RELATIONSHIP_INVALID", schema, "$.statistics", "bounded ordered integer statistics", "invalid", "result statistics relationship is invalid"));
    }
    std::int64_t previous_rank{};
    for (const auto& row : **percentiles) {
      auto rank = IntegerMember(row, "rank_ppm", schema, "PERF_FIELD_TYPE");
      auto value = IntegerMember(row, "value", schema, "PERF_FIELD_TYPE");
      if (!ExactFields(row, {"rank_ppm", "value"}) || !rank || *rank < 1 || *rank > 999999 ||
          *rank <= previous_rank || !value || *value < *minimum_value || *value > *maximum_value) {
        return std::unexpected(EvidenceError("PERF_RELATIONSHIP_INVALID", schema, "$.statistics.percentiles", "sorted unique percentiles", "invalid", "result percentile relationship is invalid"));
      }
      previous_rank = *rank;
    }
    auto validation = StringMember(document, "validation_state", schema, "PERF_FIELD_TYPE");
    if (!validation || *validation != "schema_valid") {
      return std::unexpected(EvidenceError("PERF_RELATIONSHIP_INVALID", schema, "$.validation_state", "schema_valid", validation ? *validation : "invalid", "result is not accepted"));
    }
    return {};
  }
  if (schema == "M0-PERF-COMPARISON-v1") {
    auto fields = ExactFieldSet(document,
        {"authority", "baseline_result_id", "candidate_result_id", "comparator_version", "comparison_id",
         "golden_corpus_sha256", "lower_bound_ppb", "mismatches", "next_action", "noise_state",
         "point_estimate_ppb", "reason", "resamples", "schema", "seed_sha256", "significance_fired", "state",
         "threshold_fired", "threshold_ppb", "valid_pairs"}, schema, "PERF_FIELD_MISSING");
    if (!fields) return fields;
    auto id = ValidateDerivedId(document, "comparison_id", schema, "PERF_DIGEST_INVALID");
    if (!id) return id;
    auto baseline = StringMember(document, "baseline_result_id", schema, "PERF_FIELD_TYPE");
    auto candidate = StringMember(document, "candidate_result_id", schema, "PERF_FIELD_TYPE");
    auto version = StringMember(document, "comparator_version", schema, "PERF_FIELD_TYPE");
    auto threshold = IntegerMember(document, "threshold_ppb", schema, "PERF_FIELD_TYPE");
    auto state = StringMember(document, "state", schema, "PERF_FIELD_TYPE");
    auto reason = StringMember(document, "reason", schema, "PERF_FIELD_TYPE");
    auto action = StringMember(document, "next_action", schema, "PERF_FIELD_TYPE");
    auto authority = StringMember(document, "authority", schema, "PERF_FIELD_TYPE");
    auto noise = StringMember(document, "noise_state", schema, "PERF_FIELD_TYPE");
    auto resamples = IntegerMember(document, "resamples", schema, "PERF_FIELD_TYPE");
    auto valid_pairs = IntegerMember(document, "valid_pairs", schema, "PERF_FIELD_TYPE");
    auto golden_digest = ValidateHexMember(document, "golden_corpus_sha256", schema, "PERF_DIGEST_INVALID");
    auto mismatches = ArrayMember(document, "mismatches", schema, "PERF_FIELD_TYPE");
    const auto* seed = FindMember(document, "seed_sha256");
    const auto* point = FindMember(document, "point_estimate_ppb");
    const auto* lower = FindMember(document, "lower_bound_ppb");
    const auto* threshold_fired = FindMember(document, "threshold_fired");
    const auto* significance_fired = FindMember(document, "significance_fired");
    if (!baseline || !candidate || !IsHex(*baseline) || !IsHex(*candidate) || *baseline == *candidate ||
        !version || *version != "M0-PAIRED-BOOTSTRAP-v1" || !threshold || *threshold != 30000000 ||
        !state || !reason || !action || !authority || !IsEnum(*authority, {"authoritative", "advisory"}) ||
        !noise || !IsEnum(*noise, {"pass", "failed", "unknown"}) || !resamples || !IsUint32(*resamples) ||
        !valid_pairs || !IsUint32(*valid_pairs) || !golden_digest || !mismatches || (*mismatches)->size() > 256 ||
        seed == nullptr || point == nullptr || lower == nullptr || threshold_fired == nullptr || significance_fired == nullptr) {
      return std::unexpected(EvidenceError("PERF_RELATIONSHIP_INVALID", schema, "$", "valid comparison identities/state", "invalid", "comparison relationship is invalid"));
    }
    std::string previous_mismatch;
    for (const auto& row : **mismatches) {
      auto path = StringMember(row, "path", schema, "PERF_FIELD_TYPE");
      auto baseline_value = StringMember(row, "baseline", schema, "PERF_FIELD_TYPE");
      auto candidate_value = StringMember(row, "candidate", schema, "PERF_FIELD_TYPE");
      if (!ExactFields(row, {"baseline", "candidate", "path"}) || !path ||
          !IsBoundedText(*path, 1024) || (!previous_mismatch.empty() && previous_mismatch >= *path) ||
          !baseline_value || !IsBoundedText(*baseline_value, 1024, true) || !candidate_value ||
          !IsBoundedText(*candidate_value, 1024, true)) {
        return std::unexpected(EvidenceError("PERF_RELATIONSHIP_INVALID", schema, "$.mismatches", "sorted bounded mismatch rows", "invalid", "comparison mismatch inventory is invalid"));
      }
      previous_mismatch = std::string(*path);
    }
    const bool statistical = !point->IsNull() || !lower->IsNull() || !seed->IsNull() ||
                             !threshold_fired->IsNull() || !significance_fired->IsNull();
    if (statistical) {
      if (point->AsInteger() == nullptr || lower->AsInteger() == nullptr || seed->AsString() == nullptr ||
          !IsHex(*seed->AsString()) || threshold_fired->AsBoolean() == nullptr ||
          significance_fired->AsBoolean() == nullptr || *resamples != 10000 || *valid_pairs < 30 ||
          *threshold_fired->AsBoolean() != (*lower->AsInteger() > *threshold) ||
          *significance_fired->AsBoolean() != (*lower->AsInteger() > *threshold)) {
        return std::unexpected(EvidenceError("PERF_RELATIONSHIP_INVALID", schema, "$", "complete statistical disposition", "invalid", "comparison statistics are inconsistent"));
      }
    } else if (!point->IsNull() || !lower->IsNull() || !seed->IsNull() || !threshold_fired->IsNull() ||
               !significance_fired->IsNull() || *resamples != 0) {
      return std::unexpected(EvidenceError("PERF_RELATIONSHIP_INVALID", schema, "$", "all-null non-statistical fields", "invalid", "comparison non-statistical fields are inconsistent"));
    }
    const std::set<std::tuple<std::string, std::string, std::string>> valid_states{
        {"advisory_only", "PERF_ADVISORY_INPUT", "informational"},
        {"incomparable", "PERF_INCOMPARABLE", "rerun"},
        {"inconclusive", "PERF_INSUFFICIENT_SAMPLES", "rerun"},
        {"inconclusive", "PERF_NOISE_POLICY_FAILED", "rerun"},
        {"inconclusive", "PERF_SAMPLE_FAILED", "rerun"},
        {"no_regression_detected", "none", "none"},
        {"regression_requires_approval", "PERF_REGRESSION_REQUIRES_APPROVAL", "approval"},
    };
    if (!valid_states.contains({std::string(*state), std::string(*reason), std::string(*action)})) {
      return std::unexpected(EvidenceError("PERF_RELATIONSHIP_INVALID", schema, "$.state", "valid state/reason/action", "mismatch", "comparison terminal mapping is invalid"));
    }
    if (((*state == "incomparable") != !(*mismatches)->empty()) ||
        ((*state == "no_regression_detected" || *state == "regression_requires_approval") != statistical)) {
      return std::unexpected(EvidenceError("PERF_RELATIONSHIP_INVALID", schema, "$.state", "state-consistent mismatch/statistical fields", "mismatch", "comparison state payload is inconsistent"));
    }
    return {};
  }
  return std::unexpected(EvidenceError("PERF_SCHEMA_UNKNOWN", schema, "$.schema", "known M0 performance schema", schema, "performance schema is unknown"));
}

Result<void> ValidateCorpusCore(const JsonValue& document, std::string_view schema) {
  if (schema == "M0-CORPUS-RUN-v1") {
    auto fields = ExactFieldSet(document,
        {"attempt_id", "child_artifact_sha256", "cleanup", "deadline_ms", "diagnostics", "elapsed_ms",
         "environment_id", "fault_mode", "ipc", "lifecycle", "logical_result", "observations",
         "parent_artifact_sha256", "partitions", "passed", "run_index", "schema", "source_revision", "sums",
         "terminal", "topology"}, schema, "CORP_REPORT_FIELD_MISSING");
    if (!fields) return fields;
    for (const auto name : {"child_artifact_sha256", "environment_id", "parent_artifact_sha256"}) {
      auto digest = ValidateHexMember(document, name, schema, "CORP_REPORT_DIGEST_INVALID");
      if (!digest) return digest;
    }
    auto passed = BooleanMember(document, "passed", schema, "CORP_REPORT_FIELD_TYPE");
    auto terminal = StringMember(document, "terminal", schema, "CORP_REPORT_FIELD_TYPE");
    auto fault = StringMember(document, "fault_mode", schema, "CORP_REPORT_FIELD_TYPE");
    auto deadline = IntegerMember(document, "deadline_ms", schema, "CORP_REPORT_FIELD_TYPE");
    auto elapsed = IntegerMember(document, "elapsed_ms", schema, "CORP_REPORT_FIELD_TYPE");
    auto result = IntegerMember(document, "logical_result", schema, "CORP_REPORT_FIELD_TYPE");
    auto run_index = IntegerMember(document, "run_index", schema, "CORP_REPORT_FIELD_TYPE");
    auto attempt = StringMember(document, "attempt_id", schema, "CORP_REPORT_FIELD_TYPE");
    auto revision = StringMember(document, "source_revision", schema, "CORP_REPORT_FIELD_TYPE");
    auto parent_digest = StringMember(document, "parent_artifact_sha256", schema, "CORP_REPORT_FIELD_TYPE");
    auto child_digest = StringMember(document, "child_artifact_sha256", schema, "CORP_REPORT_FIELD_TYPE");
    static const std::set<std::string> kTerminals{
        "cancelled", "child_identity_mismatch", "cleanup_failed", "exec_failed", "ipc_protocol_error",
        "observation_failed", "result_mismatch", "success", "thread_lifecycle_failed", "timeout"};
    static const std::set<std::string> kFaults{
        "child_exit_before_ready", "child_worker_failure", "exec_failure", "hang_until_timeout", "ipc_close",
        "malformed_ipc", "none", "parent_worker_failure"};
    if (!terminal || !fault) {
      return std::unexpected(EvidenceError("CORP_REPORT_FIELD_TYPE", schema, "$", "string terminal/fault", "wrong type", "corpus run enum field has wrong type"));
    }
    if (!kTerminals.contains(std::string(*terminal)) || !kFaults.contains(std::string(*fault))) {
      return std::unexpected(EvidenceError("CORP_REPORT_ENUM_INVALID", schema, "$", "known terminal/fault", std::string(*terminal) + "/" + std::string(*fault), "corpus run enum is invalid"));
    }
    if (!passed || !deadline || *deadline != 10000 || !elapsed || *elapsed < 0 ||
        *elapsed > 10000 || !result || *result < 0 || !run_index || *run_index < 1 || *run_index > 100 ||
        !attempt || attempt->empty() || attempt->size() > 128 || !revision || !IsHex(*revision, revision->size()) ||
        (revision->size() != 40 && revision->size() != 64) || !parent_digest || !child_digest ||
        *parent_digest == *child_digest || (*passed != (*terminal == "success"))) {
      return std::unexpected(EvidenceError("CORP_REPORT_RELATIONSHIP_INVALID", schema, "$", "consistent run relationships", "mismatch", "corpus run relationship is invalid"));
    }
    static const std::map<std::string, std::string> kFaultTerminal{
        {"child_exit_before_ready", "thread_lifecycle_failed"}, {"child_worker_failure", "thread_lifecycle_failed"},
        {"exec_failure", "exec_failed"}, {"hang_until_timeout", "timeout"}, {"ipc_close", "ipc_protocol_error"},
        {"malformed_ipc", "ipc_protocol_error"}, {"parent_worker_failure", "thread_lifecycle_failed"}};

    auto topology_object = Member(document, "topology", schema, "CORP_REPORT_FIELD_MISSING");
    auto sums_object = Member(document, "sums", schema, "CORP_REPORT_FIELD_MISSING");
    auto cleanup_object = Member(document, "cleanup", schema, "CORP_REPORT_FIELD_MISSING");
    auto lifecycle_object = Member(document, "lifecycle", schema, "CORP_REPORT_FIELD_MISSING");
    auto diagnostics_rows = ArrayMember(document, "diagnostics", schema, "CORP_REPORT_FIELD_TYPE");
    auto ipc_rows = ArrayMember(document, "ipc", schema, "CORP_REPORT_FIELD_TYPE");
    auto partition_rows = ArrayMember(document, "partitions", schema, "CORP_REPORT_FIELD_TYPE");
    auto observation_rows = ArrayMember(document, "observations", schema, "CORP_REPORT_FIELD_TYPE");
    if (!topology_object || !sums_object || !cleanup_object || !lifecycle_object ||
        !diagnostics_rows || !ipc_rows || !partition_rows || !observation_rows ||
        !ExactFields(**topology_object,
                     {"child_worker_count", "parent_worker_count", "process_count", "roles",
                      "thread_count"}) ||
        !ExactFields(**sums_object, {"child", "combined", "parent"}) ||
        !ExactFields(**cleanup_object,
                     {"all_waits_complete", "fd_baseline_restored", "group_absent",
                      "ipc_closed", "temporary_resources_removed"}) ||
        !ExactFields(**lifecycle_object,
                     {"cancel_sent", "child_join_count", "child_wait_status", "child_waited",
                      "forced_kill", "parent_join_count", "parent_status"}) ||
        (*diagnostics_rows)->size() > 32 || (*ipc_rows)->size() > 5 ||
        (*partition_rows)->size() > 5 || (*observation_rows)->size() > 4) {
      return std::unexpected(EvidenceError(
          "CORP_REPORT_FIELD_BOUND", schema, "$", "complete bounded nested corpus fields", "invalid",
          "corpus nested field set or count is invalid"));
    }
    for (const auto name : {"process_count", "thread_count", "parent_worker_count",
                            "child_worker_count"}) {
      auto value = IntegerMember(**topology_object, name, schema, "CORP_REPORT_FIELD_TYPE");
      if (!value || !IsUint8(*value)) {
        return std::unexpected(EvidenceError(
            "CORP_REPORT_FIELD_BOUND", schema, std::string("$.topology.") + name, "uint8",
            value ? std::to_string(*value) : "wrong type", "topology count is invalid"));
      }
    }
    auto roles = ArrayMember(**topology_object, "roles", schema, "CORP_REPORT_FIELD_TYPE");
    std::set<std::string> role_names;
    if (!roles || (*roles)->size() != 7) {
      return std::unexpected(EvidenceError(
          "CORP_REPORT_FIELD_BOUND", schema, "$.topology.roles", "seven unique roles",
          roles ? std::to_string((*roles)->size()) : "wrong type", "topology role count is invalid", 7));
    }
    for (const auto& row : **roles) {
      auto role = StringMember(row, "role", schema, "CORP_REPORT_FIELD_TYPE");
      auto pid = IntegerMember(row, "host_pid", schema, "CORP_REPORT_FIELD_TYPE");
      auto tid = IntegerMember(row, "host_tid", schema, "CORP_REPORT_FIELD_TYPE");
      auto state = StringMember(row, "final_state", schema, "CORP_REPORT_FIELD_TYPE");
      if (!ExactFields(row, {"final_state", "host_pid", "host_tid", "role"}) || !role ||
          !IsEnum(*role, {"parent_main", "P0", "P1", "P2", "child_main", "C0", "C1"}) ||
          !role_names.insert(std::string(*role)).second || !pid || *pid < 1 || !IsUint32(*pid) ||
          !tid || *tid < 1 || !IsUint32(*tid) || !state ||
          !IsEnum(*state, {"exited", "joined"})) {
        return std::unexpected(EvidenceError(
            "CORP_REPORT_FIELD_TYPE", schema, "$.topology.roles[]", "unique typed role row",
            "invalid", "topology role row is invalid"));
      }
    }
    for (const auto name : {"parent", "child", "combined"}) {
      auto value = IntegerMember(**sums_object, name, schema, "CORP_REPORT_FIELD_TYPE");
      if (!value || *value < 0) {
        return std::unexpected(EvidenceError(
            "CORP_REPORT_FIELD_BOUND", schema, std::string("$.sums.") + name,
            "non-negative int64", value ? std::to_string(*value) : "wrong type",
            "corpus sum is invalid"));
      }
    }
    bool cleanup_complete = true;
    for (const auto name : {"all_waits_complete", "fd_baseline_restored", "group_absent",
                            "ipc_closed", "temporary_resources_removed"}) {
      auto value = BooleanMember(**cleanup_object, name, schema, "CORP_REPORT_FIELD_TYPE");
      if (!value) return std::unexpected(value.error());
      cleanup_complete = cleanup_complete && *value;
    }
    if (*fault != "none") {
      const auto found = kFaultTerminal.find(std::string(*fault));
      const std::string expected_terminal = cleanup_complete && found != kFaultTerminal.end()
                                                ? found->second
                                                : "cleanup_failed";
      if (found == kFaultTerminal.end() || *terminal != expected_terminal) {
        return std::unexpected(EvidenceError(
            "CORP_REPORT_RELATIONSHIP_INVALID", schema, "$.fault_mode",
            "exact fault terminal mapping", *terminal, "fault terminal mapping is invalid"));
      }
    }
    for (const auto name : {"parent_join_count", "child_join_count"}) {
      auto value = IntegerMember(**lifecycle_object, name, schema, "CORP_REPORT_FIELD_TYPE");
      if (!value || !IsUint8(*value)) {
        return std::unexpected(EvidenceError(
            "CORP_REPORT_FIELD_BOUND", schema, std::string("$.lifecycle.") + name, "uint8",
            value ? std::to_string(*value) : "wrong type", "lifecycle count is invalid"));
      }
    }
    for (const auto name : {"child_wait_status", "parent_status"}) {
      auto value = IntegerMember(**lifecycle_object, name, schema, "CORP_REPORT_FIELD_TYPE");
      if (!value || !IsInt32(*value)) {
        return std::unexpected(EvidenceError(
            "CORP_REPORT_FIELD_BOUND", schema, std::string("$.lifecycle.") + name, "int32",
            value ? std::to_string(*value) : "wrong type", "lifecycle status is invalid"));
      }
    }
    for (const auto name : {"child_waited", "cancel_sent", "forced_kill"}) {
      auto value = BooleanMember(**lifecycle_object, name, schema, "CORP_REPORT_FIELD_TYPE");
      if (!value) return std::unexpected(value.error());
    }
    for (const auto& row : **diagnostics_rows) {
      if (row.AsString() == nullptr || !IsBoundedText(*row.AsString(), 4096, true)) {
        return std::unexpected(EvidenceError(
            "CORP_REPORT_FIELD_BOUND", schema, "$.diagnostics[]", "NFC string <=4096 bytes",
            "invalid", "corpus diagnostic is invalid", 4096));
      }
    }
    for (const auto& row : **ipc_rows) {
      auto direction = StringMember(row, "direction", schema, "CORP_REPORT_FIELD_TYPE");
      auto type = StringMember(row, "type", schema, "CORP_REPORT_FIELD_TYPE");
      auto sequence = IntegerMember(row, "sequence", schema, "CORP_REPORT_FIELD_TYPE");
      auto wire_bytes = IntegerMember(row, "wire_bytes", schema, "CORP_REPORT_FIELD_TYPE");
      auto status = StringMember(row, "status", schema, "CORP_REPORT_FIELD_TYPE");
      if (!ExactFields(row, {"direction", "sequence", "status", "type", "wire_bytes"}) ||
          !direction || !IsEnum(*direction, {"child_to_parent", "parent_to_child"}) || !type ||
          !IsEnum(*type, {"READY", "START", "CHILD_RESULT", "ACK", "CANCEL"}) || !sequence ||
          !IsUint32(*sequence) || !wire_bytes || *wire_bytes < 0 || *wire_bytes > 65535 || !status ||
          !IsEnum(*status, {"accepted", "rejected"})) {
        return std::unexpected(EvidenceError(
            "CORP_REPORT_FIELD_TYPE", schema, "$.ipc[]", "bounded typed IPC row", "invalid",
            "corpus IPC row is invalid"));
      }
    }
    std::set<std::string> partition_roles;
    for (const auto& row : **partition_rows) {
      auto role = StringMember(row, "role", schema, "CORP_REPORT_FIELD_TYPE");
      auto first = IntegerMember(row, "first", schema, "CORP_REPORT_FIELD_TYPE");
      auto last = IntegerMember(row, "last", schema, "CORP_REPORT_FIELD_TYPE");
      auto sum = IntegerMember(row, "sum", schema, "CORP_REPORT_FIELD_TYPE");
      if (!ExactFields(row, {"first", "last", "role", "sum"}) || !role ||
          !IsEnum(*role, {"P0", "P1", "P2", "C0", "C1"}) ||
          !partition_roles.insert(std::string(*role)).second || !first || !IsUint32(*first) ||
          !last || !IsUint32(*last) || !sum || *sum < 0) {
        return std::unexpected(EvidenceError(
            "CORP_REPORT_FIELD_TYPE", schema, "$.partitions[]", "unique bounded partition row",
            "invalid", "corpus partition row is invalid"));
      }
    }
    std::set<std::pair<std::string, std::string>> observation_keys_all;
    for (const auto& row : **observation_rows) {
      auto process = StringMember(row, "process", schema, "CORP_REPORT_FIELD_TYPE");
      auto operation = StringMember(row, "operation", schema, "CORP_REPORT_FIELD_TYPE");
      auto calls = IntegerMember(row, "call_count", schema, "CORP_REPORT_FIELD_TYPE");
      auto success = BooleanMember(row, "success", schema, "CORP_REPORT_FIELD_TYPE");
      const auto* value = FindMember(row, "value_sha256");
      if (!ExactFields(row, {"call_count", "operation", "process", "success", "value_sha256"}) ||
          !process || !IsEnum(*process, {"parent", "child"}) || !operation ||
          !IsEnum(*operation, {"clock_gettime_realtime", "getrandom_32"}) || !calls ||
          !IsUint8(*calls) || !success || value == nullptr ||
          (!value->IsNull() && (value->AsString() == nullptr || !IsHex(*value->AsString()))) ||
          !observation_keys_all.emplace(std::string(*process), std::string(*operation)).second) {
        return std::unexpected(EvidenceError(
            "CORP_REPORT_FIELD_TYPE", schema, "$.observations[]", "unique bounded observation row",
            "invalid", "corpus observation row is invalid"));
      }
    }
    const bool success_shaped_failure = !*passed && *fault != "none" && cleanup_complete &&
        (*diagnostics_rows)->empty() && (*ipc_rows)->size() == 4 && (*partition_rows)->size() == 5 &&
        (*observation_rows)->size() == 4 && *result == 12502500 &&
        IntegerMember(**sums_object, "combined", schema, "CORP_REPORT_FIELD_TYPE").value_or(-1) == 12502500 &&
        IntegerMember(**lifecycle_object, "parent_status", schema, "CORP_REPORT_FIELD_TYPE").value_or(-1) == 0 &&
        IntegerMember(**lifecycle_object, "child_wait_status", schema, "CORP_REPORT_FIELD_TYPE").value_or(-1) == 0;
    if (success_shaped_failure) {
      return std::unexpected(EvidenceError(
          "CORP_REPORT_RELATIONSHIP_INVALID", schema, "$", "fault evidence differs from success",
          "success-shaped failure", "injected fault report contradicts its nested evidence"));
    }
    if (*passed) {
      auto topology = Member(document, "topology", schema, "CORP_REPORT_FIELD_MISSING");
      auto sums = Member(document, "sums", schema, "CORP_REPORT_FIELD_MISSING");
      auto cleanup = Member(document, "cleanup", schema, "CORP_REPORT_FIELD_MISSING");
      auto lifecycle = Member(document, "lifecycle", schema, "CORP_REPORT_FIELD_MISSING");
      auto diagnostics = ArrayMember(document, "diagnostics", schema, "CORP_REPORT_FIELD_TYPE");
      auto ipc = ArrayMember(document, "ipc", schema, "CORP_REPORT_FIELD_TYPE");
      auto partitions = ArrayMember(document, "partitions", schema, "CORP_REPORT_FIELD_TYPE");
      auto observations = ArrayMember(document, "observations", schema, "CORP_REPORT_FIELD_TYPE");
      if (!topology || !sums || !cleanup || !lifecycle || !diagnostics || !ipc || !partitions || !observations ||
          !ExactFields(**topology, {"child_worker_count", "parent_worker_count", "process_count", "roles", "thread_count"}) ||
          !ExactFields(**sums, {"child", "combined", "parent"}) ||
          !ExactFields(**cleanup, {"all_waits_complete", "fd_baseline_restored", "group_absent", "ipc_closed", "temporary_resources_removed"}) ||
          !ExactFields(**lifecycle, {"cancel_sent", "child_join_count", "child_wait_status", "child_waited", "forced_kill", "parent_join_count", "parent_status"}) ||
          !(*diagnostics)->empty() || (*ipc)->size() != 4 ||
          (*partitions)->size() != 5 || (*observations)->size() != 4 || *result != 12502500 ||
          IntegerMember(**topology, "process_count", schema, "CORP_REPORT_FIELD_TYPE").value_or(-1) != 2 ||
          IntegerMember(**topology, "thread_count", schema, "CORP_REPORT_FIELD_TYPE").value_or(-1) != 7 ||
          IntegerMember(**topology, "parent_worker_count", schema, "CORP_REPORT_FIELD_TYPE").value_or(-1) != 3 ||
          IntegerMember(**topology, "child_worker_count", schema, "CORP_REPORT_FIELD_TYPE").value_or(-1) != 2 ||
          IntegerMember(**sums, "parent", schema, "CORP_REPORT_FIELD_TYPE").value_or(-1) != 4501500 ||
          IntegerMember(**sums, "child", schema, "CORP_REPORT_FIELD_TYPE").value_or(-1) != 8001000 ||
          IntegerMember(**sums, "combined", schema, "CORP_REPORT_FIELD_TYPE").value_or(-1) != 12502500 ||
          IntegerMember(**lifecycle, "parent_join_count", schema, "CORP_REPORT_FIELD_TYPE").value_or(-1) != 3 ||
          IntegerMember(**lifecycle, "child_join_count", schema, "CORP_REPORT_FIELD_TYPE").value_or(-1) != 2 ||
          IntegerMember(**lifecycle, "child_wait_status", schema, "CORP_REPORT_FIELD_TYPE").value_or(-1) != 0 ||
          IntegerMember(**lifecycle, "parent_status", schema, "CORP_REPORT_FIELD_TYPE").value_or(-1) != 0 ||
          !BooleanMember(**lifecycle, "child_waited", schema, "CORP_REPORT_FIELD_TYPE").value_or(false) ||
          BooleanMember(**lifecycle, "cancel_sent", schema, "CORP_REPORT_FIELD_TYPE").value_or(true) ||
          BooleanMember(**lifecycle, "forced_kill", schema, "CORP_REPORT_FIELD_TYPE").value_or(true)) {
        return std::unexpected(EvidenceError("CORP_REPORT_FORGED_SUCCESS", schema, "$", "exact success relationships", "forged scalar success", "corpus success is forged"));
      }
      for (const auto name : {"all_waits_complete", "fd_baseline_restored", "group_absent", "ipc_closed", "temporary_resources_removed"}) {
        auto flag = BooleanMember(**cleanup, name, schema, "CORP_REPORT_FIELD_TYPE");
        if (!flag || !*flag) {
          return std::unexpected(EvidenceError("CORP_REPORT_FORGED_SUCCESS", schema, "$.cleanup", "all true", "false", "corpus cleanup success is forged"));
        }
      }

      auto roles = ArrayMember(**topology, "roles", schema, "CORP_REPORT_FIELD_TYPE");
      std::map<std::string, std::tuple<std::int64_t, std::int64_t, std::string>> role_rows;
      if (!roles || (*roles)->size() != 7) {
        return std::unexpected(EvidenceError("CORP_REPORT_FORGED_SUCCESS", schema, "$.topology.roles", "seven roles", "invalid count", "corpus role topology is forged"));
      }
      for (const auto& row : **roles) {
        auto role = StringMember(row, "role", schema, "CORP_REPORT_FIELD_TYPE");
        auto pid = IntegerMember(row, "host_pid", schema, "CORP_REPORT_FIELD_TYPE");
        auto tid = IntegerMember(row, "host_tid", schema, "CORP_REPORT_FIELD_TYPE");
        auto state = StringMember(row, "final_state", schema, "CORP_REPORT_FIELD_TYPE");
        if (!ExactFields(row, {"final_state", "host_pid", "host_tid", "role"}) || !role || !pid || *pid < 1 ||
            !tid || *tid < 1 || !state || !role_rows.emplace(std::string(*role), std::tuple{*pid, *tid, std::string(*state)}).second) {
          return std::unexpected(EvidenceError("CORP_REPORT_FORGED_SUCCESS", schema, "$.topology.roles", "unique typed roles", "invalid", "corpus role topology is forged"));
        }
      }
      const std::set<std::string> expected_roles{"C0", "C1", "P0", "P1", "P2", "child_main", "parent_main"};
      std::set<std::string> actual_roles;
      std::set<std::int64_t> tids;
      for (const auto& [name, row] : role_rows) {
        actual_roles.insert(name);
        tids.insert(std::get<1>(row));
      }
      const auto parent_pid = std::get<0>(role_rows["parent_main"]);
      const auto child_pid = std::get<0>(role_rows["child_main"]);
      bool topology_valid = actual_roles == expected_roles && tids.size() == 7 && parent_pid != child_pid &&
                            std::get<1>(role_rows["parent_main"]) == parent_pid &&
                            std::get<1>(role_rows["child_main"]) == child_pid;
      for (const auto name : {"parent_main", "P0", "P1", "P2"}) {
        topology_valid = topology_valid && std::get<0>(role_rows[name]) == parent_pid;
      }
      for (const auto name : {"child_main", "C0", "C1"}) {
        topology_valid = topology_valid && std::get<0>(role_rows[name]) == child_pid;
      }
      for (const auto name : {"P0", "P1", "P2", "C0", "C1"}) {
        topology_valid = topology_valid && std::get<2>(role_rows[name]) == "joined";
      }
      topology_valid = topology_valid && std::get<2>(role_rows["parent_main"]) == "exited" &&
                       std::get<2>(role_rows["child_main"]) == "exited";

      const std::array<std::tuple<std::string_view, std::int64_t, std::int64_t, std::int64_t>, 5> expected_partitions{{
          {"C0", 3001, 4000, 3500500}, {"C1", 4001, 5000, 4500500}, {"P0", 1, 1000, 500500},
          {"P1", 1001, 2000, 1500500}, {"P2", 2001, 3000, 2500500}}};
      for (std::size_t index = 0; index < expected_partitions.size(); ++index) {
        const auto& row = (**partitions)[index];
        const auto& [role, first, last, sum] = expected_partitions[index];
        topology_valid = topology_valid && ExactFields(row, {"first", "last", "role", "sum"}) &&
                         StringMember(row, "role", schema, "CORP_REPORT_FIELD_TYPE").value_or("") == role &&
                         IntegerMember(row, "first", schema, "CORP_REPORT_FIELD_TYPE").value_or(-1) == first &&
                         IntegerMember(row, "last", schema, "CORP_REPORT_FIELD_TYPE").value_or(-1) == last &&
                         IntegerMember(row, "sum", schema, "CORP_REPORT_FIELD_TYPE").value_or(-1) == sum;
      }

      const std::array<std::tuple<std::string_view, std::string_view, std::int64_t, std::int64_t>, 4> expected_ipc{{
          {"child_to_parent", "READY", 1, 56}, {"parent_to_child", "START", 2, 56},
          {"child_to_parent", "CHILD_RESULT", 3, 40}, {"parent_to_child", "ACK", 4, 32}}};
      for (std::size_t index = 0; index < expected_ipc.size(); ++index) {
        const auto& row = (**ipc)[index];
        const auto& [direction, type, sequence, bytes] = expected_ipc[index];
        topology_valid = topology_valid && ExactFields(row, {"direction", "sequence", "status", "type", "wire_bytes"}) &&
                         StringMember(row, "direction", schema, "CORP_REPORT_FIELD_TYPE").value_or("") == direction &&
                         StringMember(row, "type", schema, "CORP_REPORT_FIELD_TYPE").value_or("") == type &&
                         StringMember(row, "status", schema, "CORP_REPORT_FIELD_TYPE").value_or("") == "accepted" &&
                         IntegerMember(row, "sequence", schema, "CORP_REPORT_FIELD_TYPE").value_or(-1) == sequence &&
                         IntegerMember(row, "wire_bytes", schema, "CORP_REPORT_FIELD_TYPE").value_or(-1) == bytes;
      }

      std::set<std::pair<std::string, std::string>> observation_keys;
      for (const auto& row : **observations) {
        auto process = StringMember(row, "process", schema, "CORP_REPORT_FIELD_TYPE");
        auto operation = StringMember(row, "operation", schema, "CORP_REPORT_FIELD_TYPE");
        auto calls = IntegerMember(row, "call_count", schema, "CORP_REPORT_FIELD_TYPE");
        auto success = BooleanMember(row, "success", schema, "CORP_REPORT_FIELD_TYPE");
        const auto* value = FindMember(row, "value_sha256");
        topology_valid = topology_valid && ExactFields(row, {"call_count", "operation", "process", "success", "value_sha256"}) &&
                         process && (*process == "parent" || *process == "child") && operation &&
                         (*operation == "clock_gettime_realtime" || *operation == "getrandom_32") && calls &&
                         *calls == 1 && success && *success && value != nullptr &&
                         (value->IsNull() || (value->AsString() != nullptr && IsHex(*value->AsString()))) &&
                         observation_keys.emplace(std::string(*process), std::string(*operation)).second;
      }
      if (!topology_valid || observation_keys.size() != 4) {
        return std::unexpected(EvidenceError("CORP_REPORT_FORGED_SUCCESS", schema, "$", "exact row-level success relationships", "forged rows", "corpus success is forged"));
      }
    }
    return {};
  }
  if (schema == "M0-CORPUS-RELIABILITY-v1") {
    auto fields = ExactFieldSet(document,
        {"aggregate", "attempted_runs", "build_id", "environment_id", "failure_counts", "gate_profile", "passed",
         "passed_runs", "required_runs", "run_reports", "schema", "source_revision"},
        schema, "CORP_REPORT_FIELD_MISSING");
    if (!fields) return fields;
    auto reports = ArrayMember(document, "run_reports", schema, "CORP_REPORT_FIELD_TYPE");
    auto failures = ArrayMember(document, "failure_counts", schema, "CORP_REPORT_FIELD_TYPE");
    auto required = IntegerMember(document, "required_runs", schema, "CORP_REPORT_FIELD_TYPE");
    auto attempted = IntegerMember(document, "attempted_runs", schema, "CORP_REPORT_FIELD_TYPE");
    auto passed_runs = IntegerMember(document, "passed_runs", schema, "CORP_REPORT_FIELD_TYPE");
    auto passed = BooleanMember(document, "passed", schema, "CORP_REPORT_FIELD_TYPE");
    auto profile = StringMember(document, "gate_profile", schema, "CORP_REPORT_FIELD_TYPE");
    auto build_digest = ValidateHexMember(document, "build_id", schema, "CORP_REPORT_DIGEST_INVALID");
    auto environment_digest = ValidateHexMember(document, "environment_id", schema, "CORP_REPORT_DIGEST_INVALID");
    auto revision = StringMember(document, "source_revision", schema, "CORP_REPORT_FIELD_TYPE");
    auto aggregate = Member(document, "aggregate", schema, "CORP_REPORT_FIELD_MISSING");
    if (!reports || !failures || !required || !attempted || !passed_runs || !passed || !profile ||
        (*profile != "unit_fixture" && *profile != "m0_release") || !build_digest || !environment_digest || !revision ||
        !IsHex(*revision, revision->size()) || (revision->size() != 40 && revision->size() != 64) || !aggregate ||
        !ExactFields(**aggregate, {"cleanup_complete", "digests_valid", "indices_unique", "result_exact", "topology_exact", "zero_timeout"}) ||
        (*reports)->empty() || (*reports)->size() > 100 || (*failures)->size() > 9 ||
        !IsUint8(*required) || *required < 1 || *required > 100 || !IsUint8(*attempted) ||
        !IsUint8(*passed_runs) || *attempted != static_cast<std::int64_t>((*reports)->size()) ||
        *passed_runs > *attempted ||
        (*profile == "m0_release" && (*required != 100 || *attempted != 100))) {
      return std::unexpected(EvidenceError("CORP_REPORT_RELATIONSHIP_INVALID", schema, "$", "reconciled reliability counts", "mismatch", "reliability report relationship is invalid"));
    }
    std::set<std::int64_t> report_indices;
    std::int64_t prior_index{};
    for (const auto& row : **reports) {
      auto index = IntegerMember(row, "run_index", schema, "CORP_REPORT_FIELD_TYPE");
      auto path = StringMember(row, "path", schema, "CORP_REPORT_FIELD_TYPE");
      auto length = IntegerMember(row, "byte_length", schema, "CORP_REPORT_FIELD_TYPE");
      auto digest = ValidateHexMember(row, "sha256", schema, "CORP_REPORT_DIGEST_INVALID");
      if (!ExactFields(row, {"byte_length", "path", "run_index", "sha256"}) || !index || *index < 1 || *index > 100 ||
          *index <= prior_index || !report_indices.insert(*index).second || !path || !IsRelPath(*path) || !length ||
          *length < 0 || !digest) {
        return std::unexpected(EvidenceError("CORP_REPORT_RELATIONSHIP_INVALID", schema, "$.run_reports", "sorted unique bounded report references", "invalid", "run report reference is invalid"));
      }
      prior_index = *index;
    }
    std::int64_t failure_total{};
    std::string previous_terminal;
    static const std::set<std::string> kFailureTerminals{
        "cancelled", "child_identity_mismatch", "cleanup_failed", "exec_failed",
        "ipc_protocol_error", "observation_failed", "result_mismatch",
        "thread_lifecycle_failed", "timeout"};
    for (const auto& row : **failures) {
      auto terminal = StringMember(row, "terminal", schema, "CORP_REPORT_FIELD_TYPE");
      auto count = IntegerMember(row, "count", schema, "CORP_REPORT_FIELD_TYPE");
      if (!ExactFields(row, {"count", "terminal"}) || !terminal ||
          !kFailureTerminals.contains(std::string(*terminal)) || !count || *count <= 0 ||
          *count > 100 || !IsUint8(*count) ||
          (!previous_terminal.empty() && previous_terminal >= *terminal)) {
        return std::unexpected(EvidenceError("CORP_REPORT_RELATIONSHIP_INVALID", schema, "$.failure_counts", "sorted nonzero unique rows", "invalid", "failure-count row is invalid"));
      }
      previous_terminal = std::string(*terminal);
      auto added = CheckedAdd(failure_total, *count, schema);
      if (!added) return std::unexpected(added.error());
      failure_total = *added;
    }
    bool aggregate_pass = true;
    for (const auto name : {"cleanup_complete", "digests_valid", "indices_unique", "result_exact", "topology_exact", "zero_timeout"}) {
      auto value = BooleanMember(**aggregate, name, schema, "CORP_REPORT_FIELD_TYPE");
      if (!value) return std::unexpected(value.error());
      aggregate_pass = aggregate_pass && *value;
    }
    const bool complete_indices = report_indices.size() == static_cast<std::size_t>(*required) &&
                                  *report_indices.begin() == 1 && *report_indices.rbegin() == *required;
    const bool computed_pass = *attempted == *required && *passed_runs == *required && failure_total == 0 &&
                               aggregate_pass && complete_indices;
    if (failure_total != *attempted - *passed_runs || *passed != computed_pass) {
      return std::unexpected(EvidenceError("CORP_REPORT_FORGED_SUCCESS", schema, "$", "exact aggregate counts", "mismatch", "reliability aggregate is forged"));
    }
    return {};
  }
  return std::unexpected(EvidenceError("CORP_REPORT_SCHEMA_UNKNOWN", schema, "$.schema", "known corpus report schema", schema, "corpus schema is unknown"));
}

}  // namespace

Result<IntegerStatistics> ComputeIntegerStatistics(std::span<const std::int64_t> values, std::string_view schema) {
  std::vector<std::int64_t> sorted(values.begin(), values.end());
  if (sorted.empty()) {
    return std::unexpected(EvidenceError("PERF_RELATIONSHIP_INVALID", schema, "$.statistics", "non-empty values", "empty", "statistics input is empty"));
  }
  std::sort(sorted.begin(), sorted.end());
  auto median = Median(sorted, schema);
  if (!median) return std::unexpected(median.error());
  std::vector<std::int64_t> deviations;
  deviations.reserve(sorted.size());
  for (const auto value : sorted) {
    const __int128 difference = static_cast<__int128>(value) - *median;
    const __int128 absolute = difference < 0 ? -difference : difference;
    if (absolute > std::numeric_limits<std::int64_t>::max()) {
      return std::unexpected(EvidenceError("PERF_INTEGER_OVERFLOW", schema, "$.statistics.median_absolute_deviation", "signed 64-bit deviation", "overflow", "MAD deviation overflowed"));
    }
    deviations.push_back(static_cast<std::int64_t>(absolute));
  }
  auto mad = Median(std::move(deviations), schema);
  if (!mad) return std::unexpected(mad.error());
  return IntegerStatistics{.minimum = sorted.front(), .maximum = sorted.back(), .median = *median, .median_absolute_deviation = *mad};
}

Result<std::int64_t> NearestRankPercentile(
    std::span<const std::int64_t> values, std::uint32_t rank_ppm, std::string_view schema) {
  if (values.empty() || rank_ppm == 0 || rank_ppm >= 1000000) {
    return std::unexpected(EvidenceError("PERF_FIELD_BOUND", schema, "$.statistics.percentiles", "non-empty values and rank 1..999999", std::to_string(rank_ppm), "percentile input is invalid"));
  }
  std::vector<std::int64_t> sorted(values.begin(), values.end());
  std::sort(sorted.begin(), sorted.end());
  const __int128 numerator = static_cast<__int128>(rank_ppm) * sorted.size() + 999999;
  std::size_t rank = static_cast<std::size_t>(numerator / 1000000);
  rank = std::clamp<std::size_t>(rank, 1, sorted.size());
  return sorted[rank - 1];
}

Result<JsonValue> ValidateGovernanceDocument(std::string_view bytes, ResourceUsage usage) {
  const std::uint64_t started_ns = internal::MonotonicNowNs();
  const auto declared_schema = TopLevelStringMember(bytes, "schema");
  constexpr std::uint64_t kSbomDescriptorMaximumBytes = 64U * 1024U;
  if (declared_schema == "M0-SBOM-CONTRACT-v1" && bytes.size() > kSbomDescriptorMaximumBytes) {
    return std::unexpected(EvidenceError(
        "GOV_FIELD_BOUND", *declared_schema, "$", "document <=65536 bytes",
        std::to_string(bytes.size()), "SBOM descriptor byte bound exceeded",
        static_cast<std::int64_t>(kSbomDescriptorMaximumBytes)));
  }
  auto document = ParseDomain(
      bytes, "M0-GOVERNANCE-v1", "GOV_NONCANONICAL", usage, "GOV_RESOURCE_LIMIT",
      started_ns);
  if (!document) return document;
  auto schema = StringMember(*document, "schema", "M0-GOVERNANCE-v1", "GOV_SCHEMA_UNKNOWN");
  if (!schema) return std::unexpected(schema.error());
  usage.input_bytes = std::max<std::uint64_t>(usage.input_bytes, bytes.size());
  const ResourceLimits limits{
      .input_bytes = *schema == "M0-SBOM-CONTRACT-v1" ? 64U * 1024U : 16U * 1024U * 1024U,
      .count = *schema == "M0-SBOM-CONTRACT-v1" ? 200000U : 12U,
      .depth = *schema == "M0-SBOM-CONTRACT-v1" ? 32U : 16U,
      .rss_bytes = 256U * 1024U * 1024U,
      .wall_time_ns = 120000000000ULL,
  };
  auto resource = CheckResourceUsage(
      internal::ObserveResourceUsage(usage, started_ns), limits, *schema, "GOV_RESOURCE_LIMIT");
  if (!resource) return std::unexpected(resource.error());
  auto status = ValidateGovernanceCore(*document, *schema);
  if (!status) return std::unexpected(status.error());
  resource = CheckResourceUsage(
      internal::ObserveResourceUsage(usage, started_ns), limits, *schema, "GOV_RESOURCE_LIMIT");
  if (!resource) return std::unexpected(resource.error());
  return document;
}

Result<JsonValue> ValidatePerformanceDocument(std::string_view bytes, ResourceUsage usage) {
  const std::uint64_t started_ns = internal::MonotonicNowNs();
  auto document = ParseDomain(
      bytes, "M0-PERFORMANCE-v1", "PERF_NONCANONICAL", usage, "PERF_RESOURCE_LIMIT",
      started_ns);
  if (!document) return document;
  auto schema = StringMember(*document, "schema", "M0-PERFORMANCE-v1", "PERF_SCHEMA_UNKNOWN");
  if (!schema) return std::unexpected(schema.error());
  usage.input_bytes = std::max<std::uint64_t>(usage.input_bytes, bytes.size());
  const ResourceLimits limits{
      .input_bytes = 16U * 1024U * 1024U,
      .count = 100000U,
      .work_units = *schema == "M0-PERF-COMPARISON-v1"
                        ? std::optional<std::uint64_t>(10000U)
                        : std::nullopt,
      .depth = 16U,
      .rss_bytes = 256U * 1024U * 1024U,
      .wall_time_ns = 120000000000ULL,
  };
  auto resource = CheckResourceUsage(
      internal::ObserveResourceUsage(usage, started_ns), limits, *schema, "PERF_RESOURCE_LIMIT");
  if (!resource) return std::unexpected(resource.error());
  auto status = ValidatePerformanceCore(*document, *schema);
  if (!status) return std::unexpected(status.error());
  resource = CheckResourceUsage(
      internal::ObserveResourceUsage(usage, started_ns), limits, *schema, "PERF_RESOURCE_LIMIT");
  if (!resource) return std::unexpected(resource.error());
  return document;
}

Result<JsonValue> ValidatePerformanceResultBundle(
    std::string_view workload_bytes,
    std::string_view raw_samples_bytes,
    std::string_view result_bytes,
    ResourceUsage usage) {
  const std::uint64_t started_ns = internal::MonotonicNowNs();
  auto workload = ValidatePerformanceDocument(workload_bytes, usage);
  if (!workload) return std::unexpected(workload.error());
  auto result = ValidatePerformanceDocument(result_bytes, usage);
  if (!result) return std::unexpected(result.error());
  auto raw_samples = ParseCanonicalJson(
      raw_samples_bytes, "M0-PERF-RAW-SAMPLE-ARRAY-v1", "PERF_NONCANONICAL",
      {.maximum_bytes = 16U * 1024U * 1024U, .maximum_depth = 16});
  if (!raw_samples) return std::unexpected(raw_samples.error());
  const auto* rows = raw_samples->AsArray();
  if (rows == nullptr || rows->empty() || rows->size() > 100000) {
    return std::unexpected(EvidenceError(
        "PERF_FIELD_BOUND", "M0-PERF-RESULT-v1", "$.raw_samples", "1-100000 rows",
        rows == nullptr ? "non-array" : std::to_string(rows->size()),
        "raw sample array bound is invalid", 100000));
  }

  usage.input_bytes = std::max<std::uint64_t>(
      usage.input_bytes,
      std::max({workload_bytes.size(), raw_samples_bytes.size(), result_bytes.size()}));
  usage.count = std::max<std::uint64_t>(usage.count, rows->size());
  const ResourceLimits limits{
      .input_bytes = 16U * 1024U * 1024U,
      .count = 100000U,
      .depth = 16U,
      .rss_bytes = 256U * 1024U * 1024U,
      .wall_time_ns = 120000000000ULL,
  };
  const auto check_resources = [&]() {
    return CheckResourceUsage(
        internal::ObserveResourceUsage(usage, started_ns), limits, "M0-PERF-RESULT-v1",
        "PERF_RESOURCE_LIMIT");
  };
  auto resource = check_resources();
  if (!resource) return std::unexpected(resource.error());

  auto workload_id = StringMember(*workload, "workload_id", "M0-PERF-WORKLOAD-v1", "PERF_FIELD_TYPE");
  auto workload_version = IntegerMember(*workload, "workload_version", "M0-PERF-WORKLOAD-v1", "PERF_FIELD_TYPE");
  auto workload_dataset = StringMember(*workload, "dataset", "M0-PERF-WORKLOAD-v1", "PERF_FIELD_TYPE");
  auto workload_concurrency = IntegerMember(*workload, "concurrency", "M0-PERF-WORKLOAD-v1", "PERF_FIELD_TYPE");
  auto workload_timeout = IntegerMember(*workload, "timeout_ms", "M0-PERF-WORKLOAD-v1", "PERF_FIELD_TYPE");
  auto workload_storage = BooleanMember(*workload, "storage_applicable", "M0-PERF-WORKLOAD-v1", "PERF_FIELD_TYPE");
  auto allocation_phase = StringMember(*workload, "allocation_phase", "M0-PERF-WORKLOAD-v1", "PERF_FIELD_TYPE");
  auto workload_metric = Member(*workload, "metric", "M0-PERF-WORKLOAD-v1", "PERF_FIELD_MISSING");
  auto workload_sampling = Member(*workload, "sampling", "M0-PERF-WORKLOAD-v1", "PERF_FIELD_MISSING");
  auto result_id = StringMember(*result, "workload_id", "M0-PERF-RESULT-v1", "PERF_FIELD_TYPE");
  auto result_version = IntegerMember(*result, "workload_version", "M0-PERF-RESULT-v1", "PERF_FIELD_TYPE");
  auto result_dataset = StringMember(*result, "dataset", "M0-PERF-RESULT-v1", "PERF_FIELD_TYPE");
  auto result_concurrency = IntegerMember(*result, "concurrency", "M0-PERF-RESULT-v1", "PERF_FIELD_TYPE");
  auto result_run = StringMember(*result, "run_id", "M0-PERF-RESULT-v1", "PERF_FIELD_TYPE");
  auto result_role = StringMember(*result, "role", "M0-PERF-RESULT-v1", "PERF_FIELD_TYPE");
  auto result_metric = Member(*result, "metric", "M0-PERF-RESULT-v1", "PERF_FIELD_MISSING");
  auto result_sampling = Member(*result, "sampling", "M0-PERF-RESULT-v1", "PERF_FIELD_MISSING");
  auto raw_descriptor = Member(*result, "raw_samples", "M0-PERF-RESULT-v1", "PERF_FIELD_MISSING");
  const auto* result_storage = FindMember(*result, "storage");
  if (!workload_id || !workload_version || !workload_dataset || !workload_concurrency ||
      !workload_timeout || !workload_storage || !allocation_phase || !workload_metric ||
      !workload_sampling || !result_id || !result_version || !result_dataset ||
      !result_concurrency || !result_run || !result_role || !result_metric || !result_sampling ||
      !raw_descriptor || result_storage == nullptr || *workload_id != *result_id ||
      *workload_version != *result_version || *workload_dataset != *result_dataset ||
      *workload_concurrency != *result_concurrency ||
      (*workload_storage != !result_storage->IsNull()) ||
      !SameJson(**workload_metric, **result_metric, "M0-PERF-RESULT-v1")) {
    return std::unexpected(EvidenceError(
        "PERF_RELATIONSHIP_INVALID", "M0-PERF-RESULT-v1", "$", "result fields equal workload",
        "mismatch", "workload and result are not reconciled"));
  }

  auto raw_length = IntegerMember(**raw_descriptor, "byte_length", "M0-PERF-RESULT-v1", "PERF_FIELD_TYPE");
  auto raw_count = IntegerMember(**raw_descriptor, "count", "M0-PERF-RESULT-v1", "PERF_FIELD_TYPE");
  auto raw_digest = StringMember(**raw_descriptor, "sha256", "M0-PERF-RESULT-v1", "PERF_FIELD_TYPE");
  auto computed_digest = Sha256(raw_samples_bytes);
  if (!raw_length || !raw_count || !raw_digest || !computed_digest ||
      *raw_length != static_cast<std::int64_t>(raw_samples_bytes.size()) ||
      *raw_count != static_cast<std::int64_t>(rows->size()) ||
      *raw_digest != computed_digest->Hex()) {
    return std::unexpected(EvidenceError(
        "PERF_DIGEST_INVALID", "M0-PERF-RESULT-v1", "$.raw_samples",
        "exact byte length/count/SHA-256", "mismatch",
        "raw sample bytes do not match the result descriptor"));
  }

  auto workload_mode = StringMember(**workload_sampling, "mode", "M0-PERF-WORKLOAD-v1", "PERF_FIELD_TYPE");
  auto workload_warmups = IntegerMember(**workload_sampling, "warmup_pairs", "M0-PERF-WORKLOAD-v1", "PERF_FIELD_TYPE");
  auto workload_minimum = IntegerMember(**workload_sampling, "minimum_measured_pairs", "M0-PERF-WORKLOAD-v1", "PERF_FIELD_TYPE");
  auto workload_resamples = IntegerMember(**workload_sampling, "resamples", "M0-PERF-WORKLOAD-v1", "PERF_FIELD_TYPE");
  auto result_mode = StringMember(**result_sampling, "mode", "M0-PERF-RESULT-v1", "PERF_FIELD_TYPE");
  auto result_warmups = IntegerMember(**result_sampling, "warmup_pairs_required", "M0-PERF-RESULT-v1", "PERF_FIELD_TYPE");
  auto result_completed = IntegerMember(**result_sampling, "warmup_pairs_completed", "M0-PERF-RESULT-v1", "PERF_FIELD_TYPE");
  auto result_minimum = IntegerMember(**result_sampling, "minimum_measured_pairs", "M0-PERF-RESULT-v1", "PERF_FIELD_TYPE");
  auto result_attempted = IntegerMember(**result_sampling, "measured_pairs_attempted", "M0-PERF-RESULT-v1", "PERF_FIELD_TYPE");
  auto result_resamples = IntegerMember(**result_sampling, "resamples", "M0-PERF-RESULT-v1", "PERF_FIELD_TYPE");
  auto result_timeout = IntegerMember(**result_sampling, "timeout_ms", "M0-PERF-RESULT-v1", "PERF_FIELD_TYPE");
  if (!workload_mode || !workload_warmups || !workload_minimum || !workload_resamples ||
      !result_mode || !result_warmups || !result_completed || !result_minimum ||
      !result_attempted || !result_resamples || !result_timeout ||
      *workload_mode != *result_mode || *workload_warmups != *result_warmups ||
      *workload_minimum != *result_minimum || *workload_resamples != *result_resamples ||
      *workload_timeout != *result_timeout) {
    return std::unexpected(EvidenceError(
        "PERF_RELATIONSHIP_INVALID", "M0-PERF-RESULT-v1", "$.sampling",
        "sampling copied from workload", "mismatch",
        "result sampling does not match its workload"));
  }

  const auto* workload_metric_name = FindMember(**workload_metric, "name");
  const auto* workload_metric_unit = FindMember(**workload_metric, "unit");
  std::vector<std::int64_t> measured_values;
  std::size_t warmup_index{};
  std::size_t measured_index{};
  std::size_t valid_measured_count{};
  std::size_t invalid_measured_count{};
  bool saw_measured{};
  bool noise_failed{};
  for (const auto& row : *rows) {
    auto row_status = ValidatePerformanceCore(row, "M0-PERF-RAW-SAMPLE-v1");
    if (!row_status) return std::unexpected(row_status.error());
    auto row_run = StringMember(row, "run_id", "M0-PERF-RAW-SAMPLE-v1", "PERF_FIELD_TYPE");
    auto row_workload = StringMember(row, "workload_id", "M0-PERF-RAW-SAMPLE-v1", "PERF_FIELD_TYPE");
    auto row_version = IntegerMember(row, "workload_version", "M0-PERF-RAW-SAMPLE-v1", "PERF_FIELD_TYPE");
    auto phase = StringMember(row, "phase", "M0-PERF-RAW-SAMPLE-v1", "PERF_FIELD_TYPE");
    auto pair_index = IntegerMember(row, "pair_index", "M0-PERF-RAW-SAMPLE-v1", "PERF_FIELD_TYPE");
    auto order = StringMember(row, "order", "M0-PERF-RAW-SAMPLE-v1", "PERF_FIELD_TYPE");
    auto valid = BooleanMember(row, "valid", "M0-PERF-RAW-SAMPLE-v1", "PERF_FIELD_TYPE");
    auto migrations = IntegerMember(row, "migration_count", "M0-PERF-RAW-SAMPLE-v1", "PERF_FIELD_TYPE");
    auto throttled = BooleanMember(row, "throttled", "M0-PERF-RAW-SAMPLE-v1", "PERF_FIELD_TYPE");
    auto background = IntegerMember(row, "background_utilization_ppm", "M0-PERF-RAW-SAMPLE-v1", "PERF_FIELD_TYPE");
    auto metric = Member(row, "metric", "M0-PERF-RAW-SAMPLE-v1", "PERF_FIELD_MISSING");
    const auto* allocation = FindMember(row, "allocation");
    const auto* storage = FindMember(row, "storage");
    const auto* metric_name = metric ? FindMember(**metric, "name") : nullptr;
    const auto* metric_unit = metric ? FindMember(**metric, "unit") : nullptr;
    if (!row_run || !row_workload || !row_version || !phase || !pair_index || !order || !valid ||
        !migrations || !throttled || !background || !metric || allocation == nullptr ||
        storage == nullptr || *row_run != *result_run || *row_workload != *workload_id ||
        *row_version != *workload_version || workload_metric_name == nullptr ||
        workload_metric_unit == nullptr || metric_name == nullptr || metric_unit == nullptr ||
        !SameJson(*workload_metric_name, *metric_name, "M0-PERF-RAW-SAMPLE-v1") ||
        !SameJson(*workload_metric_unit, *metric_unit, "M0-PERF-RAW-SAMPLE-v1") ||
        ((*allocation_phase == "none") != allocation->IsNull()) ||
        (*workload_storage != !storage->IsNull()) ||
        (*workload_mode == "single" && *order != "standalone") ||
        (*workload_mode == "paired" && *order == "standalone")) {
      return std::unexpected(EvidenceError(
          "PERF_RELATIONSHIP_INVALID", "M0-PERF-RESULT-v1", "$.raw_samples[]",
          "workload-bound sample", "mismatch",
          "raw sample is not bound to the workload/result context"));
    }
    if (*phase == "warmup") {
      if (saw_measured || *pair_index != static_cast<std::int64_t>(warmup_index++)) {
        return std::unexpected(EvidenceError(
            "PERF_RELATIONSHIP_INVALID", "M0-PERF-RESULT-v1", "$.raw_samples",
            "ordered contiguous warmup indices", "mismatch", "raw sample order is invalid"));
      }
    } else {
      saw_measured = true;
      if (*pair_index != static_cast<std::int64_t>(measured_index++)) {
        return std::unexpected(EvidenceError(
            "PERF_RELATIONSHIP_INVALID", "M0-PERF-RESULT-v1", "$.raw_samples",
            "ordered contiguous measured indices", "mismatch", "raw sample order is invalid"));
      }
      if (*valid) {
        measured_values.push_back(*FindMember(**metric, "value")->AsInteger());
        ++valid_measured_count;
      } else {
        ++invalid_measured_count;
      }
    }
    noise_failed = noise_failed || !*valid || *migrations != 0 || *throttled || *background >= 10000;
    resource = check_resources();
    if (!resource) return std::unexpected(resource.error());
  }
  if (warmup_index != static_cast<std::size_t>(*result_completed) ||
      measured_index != static_cast<std::size_t>(*result_attempted) ||
      IntegerMember(**result_sampling, "valid_pairs", "M0-PERF-RESULT-v1", "PERF_FIELD_TYPE")
              .value_or(-1) != static_cast<std::int64_t>(valid_measured_count) ||
      IntegerMember(**result_sampling, "invalid_pairs", "M0-PERF-RESULT-v1", "PERF_FIELD_TYPE")
              .value_or(-1) != static_cast<std::int64_t>(invalid_measured_count)) {
    return std::unexpected(EvidenceError(
        "PERF_RELATIONSHIP_INVALID", "M0-PERF-RESULT-v1", "$.raw_samples",
        "counts equal ordered raw rows", "mismatch", "raw sample counts are inconsistent"));
  }

  auto computed_statistics = ComputeIntegerStatistics(measured_values, "M0-PERF-RESULT-v1");
  auto statistics = Member(*result, "statistics", "M0-PERF-RESULT-v1", "PERF_FIELD_MISSING");
  if (!computed_statistics || !statistics ||
      IntegerMember(**statistics, "minimum", "M0-PERF-RESULT-v1", "PERF_FIELD_TYPE").value_or(0) != computed_statistics->minimum ||
      IntegerMember(**statistics, "maximum", "M0-PERF-RESULT-v1", "PERF_FIELD_TYPE").value_or(0) != computed_statistics->maximum ||
      IntegerMember(**statistics, "median", "M0-PERF-RESULT-v1", "PERF_FIELD_TYPE").value_or(0) != computed_statistics->median ||
      IntegerMember(**statistics, "median_absolute_deviation", "M0-PERF-RESULT-v1", "PERF_FIELD_TYPE").value_or(-1) != computed_statistics->median_absolute_deviation) {
    return std::unexpected(EvidenceError(
        "PERF_RELATIONSHIP_INVALID", "M0-PERF-RESULT-v1", "$.statistics",
        "statistics derived from valid measured raw values", "mismatch",
        "result statistics do not match raw samples"));
  }
  auto percentiles = ArrayMember(**statistics, "percentiles", "M0-PERF-RESULT-v1", "PERF_FIELD_TYPE");
  if (!percentiles) return std::unexpected(percentiles.error());
  for (const auto& percentile : **percentiles) {
    auto rank = IntegerMember(percentile, "rank_ppm", "M0-PERF-RESULT-v1", "PERF_FIELD_TYPE");
    auto value = IntegerMember(percentile, "value", "M0-PERF-RESULT-v1", "PERF_FIELD_TYPE");
    auto expected = rank ? NearestRankPercentile(
                               measured_values, static_cast<std::uint32_t>(*rank),
                               "M0-PERF-RESULT-v1")
                         : Result<std::int64_t>(std::unexpected(EvidenceError(
                               "PERF_FIELD_TYPE", "M0-PERF-RESULT-v1",
                               "$.statistics.percentiles", "rank", "missing",
                               "percentile rank is missing")));
    if (!rank || !value || !expected || *value != *expected) {
      return std::unexpected(EvidenceError(
          "PERF_RELATIONSHIP_INVALID", "M0-PERF-RESULT-v1", "$.statistics.percentiles",
          "nearest-rank value from raw samples", "mismatch",
          "result percentile does not match raw samples"));
    }
  }
  auto noise = StringMember(*result, "noise_state", "M0-PERF-RESULT-v1", "PERF_FIELD_TYPE");
  if (!noise || *noise != (noise_failed ? "failed" : "pass")) {
    return std::unexpected(EvidenceError(
        "PERF_RELATIONSHIP_INVALID", "M0-PERF-RESULT-v1", "$.noise_state",
        noise_failed ? "failed" : "pass", noise ? *noise : "missing",
        "result noise state does not match raw samples"));
  }
  resource = check_resources();
  if (!resource) return std::unexpected(resource.error());
  return result;
}

Result<JsonValue> ValidateCorpusDocument(std::string_view bytes, ResourceUsage usage) {
  const std::uint64_t started_ns = internal::MonotonicNowNs();
  const auto declared_schema = TopLevelStringMember(bytes, "schema");
  constexpr std::uint64_t kCorpusRunMaximumBytes = 1U * 1024U * 1024U;
  if (declared_schema == "M0-CORPUS-RUN-v1" && bytes.size() > kCorpusRunMaximumBytes) {
    return std::unexpected(EvidenceError(
        "CORP_REPORT_FIELD_BOUND", *declared_schema, "$", "document <=1048576 bytes",
        std::to_string(bytes.size()), "corpus run byte bound exceeded",
        static_cast<std::int64_t>(kCorpusRunMaximumBytes)));
  }
  auto document = ParseDomain(
      bytes, "M0-CORPUS-v1", "CORP_REPORT_NONCANONICAL", usage,
      "CORP_REPORT_FIELD_BOUND", started_ns);
  if (!document) return document;
  auto schema = StringMember(*document, "schema", "M0-CORPUS-v1", "CORP_REPORT_SCHEMA_UNKNOWN");
  if (!schema) return std::unexpected(schema.error());
  usage.input_bytes = std::max<std::uint64_t>(usage.input_bytes, bytes.size());
  const ResourceLimits limits{
      .input_bytes = *schema == "M0-CORPUS-RUN-v1" ? 1U * 1024U * 1024U
                                                    : 16U * 1024U * 1024U,
      .count = 100U,
      .depth = 16U,
      .rss_bytes = 256U * 1024U * 1024U,
      .wall_time_ns = 120000000000ULL,
  };
  auto resource = CheckResourceUsage(
      internal::ObserveResourceUsage(usage, started_ns), limits, *schema,
      "CORP_REPORT_FIELD_BOUND");
  if (!resource) return std::unexpected(resource.error());
  auto status = ValidateCorpusCore(*document, *schema);
  if (!status) return std::unexpected(status.error());
  resource = CheckResourceUsage(
      internal::ObserveResourceUsage(usage, started_ns), limits, *schema,
      "CORP_REPORT_FIELD_BOUND");
  if (!resource) return std::unexpected(resource.error());
  return document;
}

Result<JsonValue> ValidateCorpusReliabilityBundle(
    std::string_view reliability_bytes,
    std::span<const ReferencedDocument> run_documents,
    ResourceUsage usage) {
  const std::uint64_t started_ns = internal::MonotonicNowNs();
  auto reliability = ValidateCorpusDocument(reliability_bytes, usage);
  if (!reliability) return std::unexpected(reliability.error());
  auto schema = StringMember(
      *reliability, "schema", "M0-CORPUS-RELIABILITY-v1", "CORP_REPORT_SCHEMA_UNKNOWN");
  if (!schema || *schema != "M0-CORPUS-RELIABILITY-v1") {
    return std::unexpected(EvidenceError(
        "CORP_REPORT_SCHEMA_UNKNOWN", "M0-CORPUS-RELIABILITY-v1", "$.schema",
        "M0-CORPUS-RELIABILITY-v1", schema ? *schema : "missing",
        "reliability bundle requires an aggregate document"));
  }
  auto reports = ArrayMember(
      *reliability, "run_reports", *schema, "CORP_REPORT_FIELD_TYPE");
  auto failures = ArrayMember(
      *reliability, "failure_counts", *schema, "CORP_REPORT_FIELD_TYPE");
  auto source_revision = StringMember(
      *reliability, "source_revision", *schema, "CORP_REPORT_FIELD_TYPE");
  auto environment_id = StringMember(
      *reliability, "environment_id", *schema, "CORP_REPORT_FIELD_TYPE");
  auto declared_passed = IntegerMember(
      *reliability, "passed_runs", *schema, "CORP_REPORT_FIELD_TYPE");
  auto aggregate = Member(*reliability, "aggregate", *schema, "CORP_REPORT_FIELD_MISSING");
  if (!reports || !failures || !source_revision || !environment_id || !declared_passed ||
      !aggregate || run_documents.size() != (*reports)->size()) {
    return std::unexpected(EvidenceError(
        "CORP_REPORT_RELATIONSHIP_INVALID", *schema, "$.run_reports",
        "one supplied document per report reference", std::to_string(run_documents.size()),
        "reliability run-document population is incomplete or excessive"));
  }
  std::map<std::string, std::string_view> documents_by_path;
  for (const auto& document : run_documents) {
    if (!IsRelPath(document.path) ||
        !documents_by_path.emplace(document.path, document.bytes).second) {
      return std::unexpected(EvidenceError(
          "CORP_REPORT_RELATIONSHIP_INVALID", *schema, "$.run_reports",
          "unique relative supplied paths", document.path,
          "supplied run-document path is invalid or duplicated"));
    }
  }

  usage.input_bytes = std::max<std::uint64_t>(usage.input_bytes, reliability_bytes.size());
  usage.count = std::max<std::uint64_t>(usage.count, run_documents.size());
  const ResourceLimits limits{
      .input_bytes = 16U * 1024U * 1024U,
      .count = 100U,
      .depth = 16U,
      .rss_bytes = 256U * 1024U * 1024U,
      .wall_time_ns = 120000000000ULL,
  };
  const auto check_resources = [&]() {
    return CheckResourceUsage(
        internal::ObserveResourceUsage(usage, started_ns), limits, *schema,
        "CORP_REPORT_FIELD_BOUND");
  };
  auto resource = check_resources();
  if (!resource) return std::unexpected(resource.error());

  std::int64_t computed_passed{};
  std::map<std::string, std::int64_t> terminal_counts;
  bool topology_exact = true;
  bool result_exact = true;
  bool zero_timeout = true;
  bool cleanup_complete = true;
  for (const auto& reference : **reports) {
    auto path = StringMember(reference, "path", *schema, "CORP_REPORT_FIELD_TYPE");
    auto index = IntegerMember(reference, "run_index", *schema, "CORP_REPORT_FIELD_TYPE");
    auto byte_length = IntegerMember(reference, "byte_length", *schema, "CORP_REPORT_FIELD_TYPE");
    auto digest = StringMember(reference, "sha256", *schema, "CORP_REPORT_FIELD_TYPE");
    if (!path || !index || !byte_length || !digest) {
      return std::unexpected(EvidenceError(
          "CORP_REPORT_FIELD_TYPE", *schema, "$.run_reports[]", "complete reference row",
          "invalid", "run reference cannot be resolved"));
    }
    const auto found = documents_by_path.find(std::string(*path));
    if (found == documents_by_path.end()) {
      return std::unexpected(EvidenceError(
          "CORP_REPORT_RELATIONSHIP_INVALID", *schema, "$.run_reports[].path",
          "resolved document", *path, "referenced run document is missing"));
    }
    const std::string_view bytes = found->second;
    usage.input_bytes = std::max<std::uint64_t>(usage.input_bytes, bytes.size());
    auto actual_digest = Sha256(bytes);
    if (*byte_length != static_cast<std::int64_t>(bytes.size()) || !actual_digest ||
        *digest != actual_digest->Hex()) {
      return std::unexpected(EvidenceError(
          "CORP_REPORT_DIGEST_INVALID", *schema, "$.run_reports[]",
          "exact referenced length and SHA-256", *path,
          "referenced run bytes do not match the aggregate row"));
    }
    auto run = ValidateCorpusDocument(bytes, usage);
    if (!run) return std::unexpected(run.error());
    auto run_schema = StringMember(*run, "schema", "M0-CORPUS-RUN-v1", "CORP_REPORT_SCHEMA_UNKNOWN");
    auto run_index = IntegerMember(*run, "run_index", "M0-CORPUS-RUN-v1", "CORP_REPORT_FIELD_TYPE");
    auto run_revision = StringMember(*run, "source_revision", "M0-CORPUS-RUN-v1", "CORP_REPORT_FIELD_TYPE");
    auto run_environment = StringMember(*run, "environment_id", "M0-CORPUS-RUN-v1", "CORP_REPORT_FIELD_TYPE");
    auto run_passed = BooleanMember(*run, "passed", "M0-CORPUS-RUN-v1", "CORP_REPORT_FIELD_TYPE");
    auto terminal = StringMember(*run, "terminal", "M0-CORPUS-RUN-v1", "CORP_REPORT_FIELD_TYPE");
    if (!run_schema || *run_schema != "M0-CORPUS-RUN-v1" || !run_index || *run_index != *index ||
        !run_revision || *run_revision != *source_revision || !run_environment ||
        *run_environment != *environment_id || !run_passed || !terminal) {
      return std::unexpected(EvidenceError(
          "CORP_REPORT_RELATIONSHIP_INVALID", *schema, "$.run_reports[]",
          "matching run index/source/environment context", *path,
          "referenced run context does not match the aggregate"));
    }
    if (*run_passed && *terminal == "success") {
      auto added = CheckedAdd(computed_passed, 1, *schema);
      if (!added) return std::unexpected(added.error());
      computed_passed = *added;
    } else {
      auto added = CheckedAdd(terminal_counts[std::string(*terminal)], 1, *schema);
      if (!added) return std::unexpected(added.error());
      terminal_counts[std::string(*terminal)] = *added;
    }
    topology_exact = topology_exact && *terminal != "thread_lifecycle_failed" &&
                     *terminal != "child_identity_mismatch" && *terminal != "exec_failed";
    result_exact = result_exact && *terminal != "result_mismatch";
    zero_timeout = zero_timeout && *terminal != "timeout";
    auto cleanup = Member(*run, "cleanup", "M0-CORPUS-RUN-v1", "CORP_REPORT_FIELD_MISSING");
    if (!cleanup) return std::unexpected(cleanup.error());
    for (const auto name : {"all_waits_complete", "fd_baseline_restored", "group_absent",
                            "ipc_closed", "temporary_resources_removed"}) {
      auto flag = BooleanMember(**cleanup, name, "M0-CORPUS-RUN-v1", "CORP_REPORT_FIELD_TYPE");
      if (!flag) return std::unexpected(flag.error());
      cleanup_complete = cleanup_complete && *flag;
    }
    resource = check_resources();
    if (!resource) return std::unexpected(resource.error());
  }
  std::map<std::string, std::int64_t> declared_failures;
  for (const auto& row : **failures) {
    auto terminal = StringMember(row, "terminal", *schema, "CORP_REPORT_FIELD_TYPE");
    auto count = IntegerMember(row, "count", *schema, "CORP_REPORT_FIELD_TYPE");
    if (!terminal || !count) return std::unexpected(EvidenceError(
        "CORP_REPORT_FIELD_TYPE", *schema, "$.failure_counts[]", "terminal/count", "invalid",
        "failure count cannot be reconciled"));
    declared_failures.emplace(std::string(*terminal), *count);
  }
  if (computed_passed != *declared_passed || terminal_counts != declared_failures) {
    return std::unexpected(EvidenceError(
        "CORP_REPORT_FORGED_SUCCESS", *schema, "$", "counts derived from referenced run documents",
        "mismatch", "aggregate pass/failure counts do not match referenced runs"));
  }
  const std::map<std::string_view, bool> expected_aggregate{
      {"cleanup_complete", cleanup_complete},
      {"digests_valid", true},
      {"indices_unique", true},
      {"result_exact", result_exact},
      {"topology_exact", topology_exact},
      {"zero_timeout", zero_timeout},
  };
  for (const auto& [name, expected] : expected_aggregate) {
    auto actual = BooleanMember(**aggregate, name, *schema, "CORP_REPORT_FIELD_TYPE");
    if (!actual || *actual != expected) {
      return std::unexpected(EvidenceError(
          "CORP_REPORT_FORGED_SUCCESS", *schema, std::string("$.aggregate.") + std::string(name),
          expected ? "true" : "false", actual && *actual ? "true" : "false",
          "aggregate relationship does not match referenced runs"));
    }
  }
  resource = check_resources();
  if (!resource) return std::unexpected(resource.error());
  return reliability;
}

const std::vector<ResourceContractRow>& M0SharedResourceContracts() {
  static const std::vector<ResourceContractRow> rows{
      {.limit_id = "SEC-LIM-10-02", .operation = "reference_environment", .limits = {.input_bytes = 96U * 1024U, .count = 128, .depth = 8, .rss_bytes = 64U * 1024U * 1024U, .wall_time_ns = 10000000000ULL}, .error = "BUILD_REFENV_RESOURCE_LIMIT", .owner_requirement = "BUILD-FR-010"},
      {.limit_id = "SEC-LIM-10-03", .operation = "package_tree_identity", .limits = {.input_bytes = 16ULL * 1024ULL * 1024ULL * 1024ULL, .count = 100000, .rss_bytes = 256U * 1024U * 1024U, .wall_time_ns = 1200000000000ULL}, .error = "BUILD_PACKAGE_IDENTITY_INVALID", .owner_requirement = "BUILD-FR-011"},
      {.limit_id = "SEC-LIM-11-03", .operation = "spdx_descriptor", .limits = {.input_bytes = 64U * 1024U, .count = 200000, .depth = 32, .rss_bytes = 256U * 1024U * 1024U, .wall_time_ns = 120000000000ULL}, .error = "GOV_RESOURCE_LIMIT", .owner_requirement = "GOV-FR-006"},
      {.limit_id = "SEC-LIM-11-04", .operation = "release_evidence", .limits = {.input_bytes = 16U * 1024U * 1024U, .count = 12, .depth = 16, .rss_bytes = 256U * 1024U * 1024U, .wall_time_ns = 120000000000ULL}, .error = "GOV_RESOURCE_LIMIT", .owner_requirement = "GOV-FR-008"},
      {.limit_id = "SEC-LIM-14-01", .operation = "performance_document", .limits = {.input_bytes = 16U * 1024U * 1024U, .count = 100000, .depth = 16}, .error = "PERF_RESOURCE_LIMIT", .owner_requirement = "PERF-FR-012"},
      {.limit_id = "SEC-LIM-14-02", .operation = "performance_comparator", .limits = {.count = 100000, .work_units = 10000, .rss_bytes = 256U * 1024U * 1024U, .wall_time_ns = 120000000000ULL}, .error = "PERF_RESOURCE_LIMIT", .owner_requirement = "PERF-FR-012"},
      {.limit_id = "SEC-LIM-15-03", .operation = "corpus_run_report", .limits = {.input_bytes = 1U * 1024U * 1024U, .count = 100, .depth = 16}, .alternate_limits = ResourceLimits{.input_bytes = 16U * 1024U * 1024U, .count = 100, .depth = 16}, .error = "CORP_REPORT_FIELD_BOUND", .owner_requirement = "CORP-FR-013"},
  };
  return rows;
}

}  // namespace orus::contracts
