#include "orus/contracts/contracts.h"

#include <algorithm>
#include <chrono>
#include <fstream>
#include <limits>
#include <regex>
#include <set>
#include <sys/stat.h>

namespace orus::contracts {
namespace {

Error BuildError(
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

bool IsHex(std::string_view value, std::size_t length) {
  return value.size() == length && std::all_of(value.begin(), value.end(), [](char character) {
    return (character >= '0' && character <= '9') || (character >= 'a' && character <= 'f');
  });
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
    std::string_view path = "$.") {
  if (const auto* member = FindMember(value, name)) return member;
  return std::unexpected(BuildError(
      code, schema, std::string(path) + std::string(name), "required field", "missing",
      "required field is missing"));
}

bool JsonEqual(const JsonValue& left, const JsonValue& right) {
  auto left_bytes = EmitCanonicalJson(left, "comparison");
  auto right_bytes = EmitCanonicalJson(right, "comparison");
  return left_bytes && right_bytes && *left_bytes == *right_bytes;
}

bool ValidConfiguration(std::string_view value) {
  static const std::set<std::string_view> values{
      "asan", "benchmark", "dev", "fuzz", "gcc", "release", "tsan", "ubsan"};
  return values.contains(value);
}

bool IsToken(std::string_view value) {
  if (value.empty() || value.size() > 64 ||
      !((value.front() >= 'a' && value.front() <= 'z') ||
        (value.front() >= '0' && value.front() <= '9'))) {
    return false;
  }
  return std::all_of(value.begin() + 1, value.end(), [](char character) {
    return (character >= 'a' && character <= 'z') ||
           (character >= '0' && character <= '9') || character == '.' ||
           character == '_' || character == '-';
  });
}

bool IsBoundedString(const JsonValue& value, std::size_t maximum) {
  const auto* text = value.AsString();
  return text != nullptr && !text->empty() && text->size() <= maximum && IsValidNfc(*text);
}

bool IsUint32(const JsonValue& value) {
  const auto* integer = value.AsInteger();
  return integer != nullptr && *integer >= 0 &&
         static_cast<std::uint64_t>(*integer) <= std::numeric_limits<std::uint32_t>::max();
}

bool IsSortedUniqueTokens(const JsonValue& value, bool allow_empty) {
  const auto* array = value.AsArray();
  if (array == nullptr || (!allow_empty && array->empty()) || array->size() > 128) return false;
  std::string_view previous;
  for (const auto& row : *array) {
    const auto* token = row.AsString();
    if (token == nullptr || !IsToken(*token) || (!previous.empty() && previous >= *token)) return false;
    previous = *token;
  }
  return true;
}

Result<void> ValidateRelativePath(std::string_view path) {
  if (path.empty() || path.size() > 4096 || path.starts_with('/') || path.ends_with('/') || !IsValidNfc(path)) {
    return std::unexpected(BuildError(
        "BUILD_PACKAGE_IDENTITY_INVALID", "M0-PACKAGE-TREE-v1", "$.entries[].path",
        "bounded NFC relative path", path, "package path is invalid"));
  }
  std::size_t start{};
  while (start <= path.size()) {
    const std::size_t slash = path.find('/', start);
    const std::string_view component = path.substr(start, slash == std::string_view::npos ? path.size() - start : slash - start);
    if (component.empty() || component == "." || component == "..") {
      return std::unexpected(BuildError(
          "BUILD_PACKAGE_IDENTITY_INVALID", "M0-PACKAGE-TREE-v1", "$.entries[].path",
          "non-empty non-dot components", path, "package path escapes or is ambiguous"));
    }
    if (slash == std::string_view::npos) break;
    start = slash + 1;
  }
  return {};
}

std::int64_t ModeBits(mode_t mode) {
  return static_cast<std::int64_t>(mode & 07777U);
}

Result<struct stat> ReadStat(const std::filesystem::path& path) {
  struct stat result {};
  if (::lstat(path.c_str(), &result) != 0) {
    return std::unexpected(BuildError(
        "BUILD_PACKAGE_IDENTITY_INVALID", "M0-PACKAGE-TREE-v1", path.string(),
        "readable package entry", "lstat failed", "package entry metadata cannot be read"));
  }
  return result;
}

bool SameReadIdentity(const struct stat& left, const struct stat& right) {
  return left.st_dev == right.st_dev && left.st_ino == right.st_ino && left.st_mode == right.st_mode &&
         left.st_size == right.st_size && left.st_mtim.tv_sec == right.st_mtim.tv_sec &&
         left.st_mtim.tv_nsec == right.st_mtim.tv_nsec;
}

bool UnsignedPathLess(std::string_view left, std::string_view right) {
  return std::lexicographical_compare(
      left.begin(), left.end(), right.begin(), right.end(), [](char a, char b) {
        return static_cast<unsigned char>(a) < static_cast<unsigned char>(b);
      });
}

}  // namespace

Result<JsonValue> MakeBuildFacts(const BuildFacts& facts, bool release) {
  static const std::regex kRevision("^[0-9a-f]{40}([0-9a-f]{24})?(-dirty)?$");
  auto invalid = [&](std::string_view field, std::string_view value, std::string_view expected) -> Result<JsonValue> {
    return std::unexpected(BuildError(
        "BUILD_METADATA_INVALID", "M0-BUILD-FACTS-v1", std::string("$.") + std::string(field),
        expected, value, "build fact is invalid"));
  };
  if (facts.product_version.empty() || facts.product_version.size() > 64) {
    return invalid("product_version", facts.product_version, "1-64 byte version");
  }
  if (!std::regex_match(facts.source_revision, kRevision) ||
      (release && facts.source_revision.ends_with("-dirty"))) {
    return invalid("source_revision", facts.source_revision, release ? "clean full revision" : "full revision with optional -dirty");
  }
  if (!ValidConfiguration(facts.configuration) || (release && facts.configuration != "release")) {
    return invalid("configuration", facts.configuration, release ? "release" : "known Bazel configuration");
  }
  if (facts.compiler.empty() || facts.compiler.size() > 128) {
    return invalid("compiler", facts.compiler, "1-128 byte declared compiler identity");
  }
  if (facts.target_platform.empty() || facts.target_platform.size() > 128 ||
      (release && facts.target_platform != "linux-x86_64")) {
    return invalid("target_platform", facts.target_platform, release ? "linux-x86_64" : "1-128 byte platform");
  }
  return JsonValue(JsonValue::Object{
      {"compiler", facts.compiler},
      {"configuration", facts.configuration},
      {"product_version", facts.product_version},
      {"schema", "M0-BUILD-FACTS-v1"},
      {"source_revision", facts.source_revision},
      {"target_platform", facts.target_platform},
  });
}

Result<ReferenceValidation> ValidateReferenceEnvironment(
    std::string_view contract_bytes,
    std::string_view observed_bytes,
    ResourceUsage usage) {
  usage.input_bytes = std::max<std::uint64_t>(usage.input_bytes, contract_bytes.size() + observed_bytes.size());
  auto resource = CheckResourceUsage(
      usage,
      ResourceLimits{.input_bytes = 96U * 1024U, .count = 128, .depth = 8,
                     .rss_bytes = 64U * 1024U * 1024U, .wall_time_ns = 10000000000ULL},
      "M0-REFENV-v1", "BUILD_REFENV_RESOURCE_LIMIT");
  if (!resource) return std::unexpected(resource.error());

  auto contract = ParseCanonicalJson(
      contract_bytes, "M0-REFENV-v1", "BUILD_REFENV_NONCANONICAL", {.maximum_bytes = 64U * 1024U, .maximum_depth = 8});
  if (!contract) return std::unexpected(contract.error());
  auto observed = ParseCanonicalJson(
      observed_bytes, "M0-REFENV-OBSERVED-v1", "BUILD_REFENV_NONCANONICAL", {.maximum_bytes = 32U * 1024U, .maximum_depth = 8});
  if (!observed) return std::unexpected(observed.error());

  if (!ExactFields(*contract, {"environment_id", "host", "inputs", "nix_system", "schema", "support_level", "target_triple", "tools"})) {
    return std::unexpected(BuildError(
        "BUILD_REFENV_FIELD_INVALID", "M0-REFENV-v1", "$", "exact reference fields",
        "missing or unknown field", "reference contract field set is invalid"));
  }
  auto schema_member = Member(*contract, "schema", "M0-REFENV-v1", "BUILD_REFENV_FIELD_INVALID");
  if (!schema_member) return std::unexpected(schema_member.error());
  auto schema = RequireString(**schema_member, "M0-REFENV-v1", "$.schema", "BUILD_REFENV_FIELD_INVALID");
  if (!schema || *schema != "M0-REFENV-v1") {
    return std::unexpected(BuildError(
        "BUILD_REFENV_SCHEMA_UNKNOWN", "M0-REFENV-v1", "$.schema", "M0-REFENV-v1",
        schema ? *schema : "non-string", "reference schema is unknown"));
  }
  auto environment_member = Member(*contract, "environment_id", *schema, "BUILD_REFENV_FIELD_INVALID");
  if (!environment_member) return std::unexpected(environment_member.error());
  auto environment_id = RequireString(**environment_member, *schema, "$.environment_id", "BUILD_REFENV_FIELD_INVALID");
  if (!environment_id || !IsHex(*environment_id, 64)) {
    return std::unexpected(BuildError(
        "BUILD_REFENV_FIELD_INVALID", *schema, "$.environment_id", "hex64",
        environment_id ? *environment_id : "non-string", "environment identity is invalid"));
  }

  JsonValue::Object identity_object;
  for (const auto& row : *contract->AsObject()) {
    if (row.first != "environment_id") identity_object.push_back(row);
  }
  auto identity_bytes = EmitCanonicalJson(JsonValue(identity_object), *schema, {.maximum_bytes = 64U * 1024U, .maximum_depth = 8});
  if (!identity_bytes) return std::unexpected(identity_bytes.error());
  auto computed = Sha256(*identity_bytes);
  if (!computed || computed->Hex() != *environment_id) {
    return std::unexpected(BuildError(
        "BUILD_REFENV_ID_MISMATCH", *schema, "$.environment_id",
        computed ? computed->Hex() : "computable SHA-256", *environment_id,
        "reference environment identity does not match its canonical content"));
  }

  for (const auto& [name, expected] : {
           std::pair{"nix_system", std::string_view("x86_64-linux")},
           std::pair{"support_level", std::string_view("validated_reference")},
       }) {
    auto member = Member(*contract, name, *schema, "BUILD_REFENV_FIELD_INVALID");
    if (!member) return std::unexpected(member.error());
    auto text = RequireString(**member, *schema, std::string("$.") + name, "BUILD_REFENV_FIELD_INVALID");
    if (!text || *text != expected) {
      return std::unexpected(BuildError(
          "BUILD_REFENV_FIELD_INVALID", *schema, std::string("$.") + name, expected,
          text ? *text : "non-string", "fixed reference field is invalid"));
    }
  }

  auto inputs_member = Member(*contract, "inputs", *schema, "BUILD_REFENV_FIELD_INVALID");
  if (!inputs_member) return std::unexpected(inputs_member.error());
  auto inputs = RequireArray(**inputs_member, *schema, "$.inputs", "BUILD_REFENV_FIELD_INVALID");
  if (!inputs || (*inputs)->empty() || (*inputs)->size() > 128) {
    return std::unexpected(BuildError(
        "BUILD_REFENV_FIELD_INVALID", *schema, "$.inputs", "1-128 rows", "invalid count",
        "reference input inventory is invalid", 128));
  }
  std::string previous_input;
  for (const auto& row : **inputs) {
    if (!ExactFields(row, {"coordinate", "name", "sha256"})) {
      return std::unexpected(BuildError(
          "BUILD_REFENV_FIELD_INVALID", *schema, "$.inputs[]", "coordinate/name/sha256",
          "invalid row", "reference input row is invalid"));
    }
    auto name_member = Member(row, "name", *schema, "BUILD_REFENV_FIELD_INVALID", "$.inputs[].");
    auto coordinate_member = Member(row, "coordinate", *schema, "BUILD_REFENV_FIELD_INVALID", "$.inputs[].");
    auto digest_member = Member(row, "sha256", *schema, "BUILD_REFENV_FIELD_INVALID", "$.inputs[].");
    if (!name_member || !coordinate_member || !digest_member) {
      return std::unexpected(!name_member ? name_member.error()
                                         : (!coordinate_member ? coordinate_member.error() : digest_member.error()));
    }
    auto name = RequireString(**name_member, *schema, "$.inputs[].name", "BUILD_REFENV_FIELD_INVALID");
    auto coordinate = RequireString(**coordinate_member, *schema, "$.inputs[].coordinate", "BUILD_REFENV_FIELD_INVALID");
    auto digest = RequireString(**digest_member, *schema, "$.inputs[].sha256", "BUILD_REFENV_FIELD_INVALID");
    if (!name || !IsToken(*name) || (!previous_input.empty() && previous_input >= *name) ||
        !coordinate || coordinate->empty() || coordinate->size() > 256 || !IsValidNfc(*coordinate) ||
        !digest || !IsHex(*digest, 64)) {
      return std::unexpected(BuildError(
          "BUILD_REFENV_FIELD_INVALID", *schema, "$.inputs[]", "sorted unique valid rows",
          name ? *name : "invalid", "reference input row relationship is invalid"));
    }
    previous_input = std::string(*name);
  }

  auto target_member = Member(*contract, "target_triple", *schema, "BUILD_REFENV_FIELD_INVALID");
  if (!target_member || !IsBoundedString(**target_member, 128)) {
    return std::unexpected(BuildError(
        "BUILD_REFENV_FIELD_INVALID", *schema, "$.target_triple", "NFC string[1..128]",
        target_member ? "invalid string" : "missing", "reference target triple is invalid"));
  }

  auto tools_member = Member(*contract, "tools", *schema, "BUILD_REFENV_FIELD_INVALID");
  if (!tools_member || !ExactFields(**tools_member, {"bazel", "clang", "gcc", "lld", "llvm"})) {
    return std::unexpected(BuildError(
        "BUILD_REFENV_FIELD_INVALID", *schema, "$.tools", "exact five tool identities",
        "invalid field set", "reference tool inventory is invalid"));
  }
  for (const auto tool : {"bazel", "clang", "gcc", "lld", "llvm"}) {
    const auto* identity = FindMember(**tools_member, tool);
    if (identity == nullptr || !ExactFields(*identity, {"artifact_sha256", "version"})) {
      return std::unexpected(BuildError(
          "BUILD_REFENV_FIELD_INVALID", *schema, std::string("$.tools.") + tool,
          "artifact_sha256/version", "invalid object", "reference tool identity is invalid"));
    }
    const auto* version = FindMember(*identity, "version");
    const auto* digest_value = FindMember(*identity, "artifact_sha256");
    const auto* digest = digest_value == nullptr ? nullptr : digest_value->AsString();
    if (version == nullptr || !IsBoundedString(*version, 64) || digest == nullptr || !IsHex(*digest, 64)) {
      return std::unexpected(BuildError(
          "BUILD_REFENV_FIELD_INVALID", *schema, std::string("$.tools.") + tool,
          "bounded version and hex64 artifact identity", "invalid value",
          "reference tool identity is invalid"));
    }
  }

  auto host_member = Member(*contract, "host", *schema, "BUILD_REFENV_FIELD_INVALID");
  if (!host_member) return std::unexpected(host_member.error());
  static constexpr std::array<std::string_view, 9> kPaths{
      "architecture", "cpu_family", "cpu_model", "cpu_vendor", "kernel_release",
      "libc_name", "libc_version", "os_family", "required_isa"};
  if (!ExactFields(**host_member, {"architecture", "cpu_family", "cpu_model", "cpu_vendor", "kernel_release", "libc_name", "libc_version", "os_family", "required_isa"})) {
    return std::unexpected(BuildError(
        "BUILD_REFENV_FIELD_INVALID", *schema, "$.host", "exact nine predicates", "invalid field set",
        "reference host predicate inventory is invalid"));
  }

  if (!ExactFields(*observed, {"embedded_environment_id", "facts", "schema"})) {
    return std::unexpected(BuildError(
        "BUILD_REFENV_FIELD_INVALID", "M0-REFENV-OBSERVED-v1", "$", "exact observed fields",
        "invalid field set", "observed reference facts field set is invalid"));
  }
  auto observed_schema_member = Member(*observed, "schema", "M0-REFENV-OBSERVED-v1", "BUILD_REFENV_FIELD_INVALID");
  auto embedded_member = Member(*observed, "embedded_environment_id", "M0-REFENV-OBSERVED-v1", "BUILD_REFENV_FIELD_INVALID");
  auto facts_member = Member(*observed, "facts", "M0-REFENV-OBSERVED-v1", "BUILD_REFENV_FIELD_INVALID");
  if (!observed_schema_member || !embedded_member || !facts_member) {
    return std::unexpected(!observed_schema_member ? observed_schema_member.error() : (!embedded_member ? embedded_member.error() : facts_member.error()));
  }
  auto observed_schema = RequireString(**observed_schema_member, "M0-REFENV-OBSERVED-v1", "$.schema", "BUILD_REFENV_FIELD_INVALID");
  auto embedded = RequireString(**embedded_member, "M0-REFENV-OBSERVED-v1", "$.embedded_environment_id", "BUILD_REFENV_FIELD_INVALID");
  if (!observed_schema || *observed_schema != "M0-REFENV-OBSERVED-v1") {
    return std::unexpected(BuildError(
        "BUILD_REFENV_SCHEMA_UNKNOWN", "M0-REFENV-OBSERVED-v1", "$.schema",
        "M0-REFENV-OBSERVED-v1", observed_schema ? *observed_schema : "invalid",
        "observed facts schema is invalid"));
  }
  if (!embedded || !IsHex(*embedded, 64)) {
    return std::unexpected(BuildError(
        "BUILD_REFENV_FIELD_INVALID", *observed_schema, "$.embedded_environment_id", "hex64",
        embedded ? *embedded : "non-string", "observed embedded identity is invalid"));
  }
  if (!ExactFields(**facts_member, {"host"})) {
    return std::unexpected(BuildError(
        "BUILD_REFENV_FIELD_INVALID", *observed_schema, "$.facts", "host only", "invalid field set",
        "observed facts root is invalid"));
  }
  auto observed_host_member = Member(**facts_member, "host", *observed_schema, "BUILD_REFENV_FIELD_INVALID", "$.facts.");
  if (!observed_host_member || !ExactFields(**observed_host_member, {"architecture", "cpu_family", "cpu_model", "cpu_vendor", "kernel_release", "libc_name", "libc_version", "os_family", "required_isa"})) {
    return std::unexpected(BuildError(
        "BUILD_REFENV_FIELD_INVALID", *observed_schema, "$.facts.host", "exact nine observed facts",
        "invalid field set", "observed host fact inventory is invalid"));
  }

  ReferenceValidation validation{
      .contract_environment_id = std::string(*environment_id),
      .embedded_environment_id = std::string(*embedded),
      .overall = "validated_reference",
      .outcomes = {},
  };
  if (*embedded != *environment_id) validation.overall = "unvalidated";

  for (const auto path : kPaths) {
    const JsonValue* predicate = FindMember(**host_member, path);
    const JsonValue* fact = FindMember(**observed_host_member, path);
    if (predicate == nullptr || fact == nullptr || !ExactFields(*predicate, {"expected", "op"})) {
      return std::unexpected(BuildError(
          "BUILD_REFENV_FIELD_INVALID", *schema, std::string("$.host.") + std::string(path),
          "expected/op predicate", "invalid predicate", "host predicate is invalid"));
    }
    const JsonValue* operation_value = FindMember(*predicate, "op");
    const JsonValue* expected_value = FindMember(*predicate, "expected");
    auto operation = RequireString(*operation_value, *schema, std::string("$.host.") + std::string(path) + ".op", "BUILD_REFENV_FIELD_INVALID");
    const std::string_view required_operation =
        path == std::string_view("required_isa") ? "set_contains_all"
        : (path == std::string_view("cpu_family") || path == std::string_view("cpu_model")) ? "u32_eq"
                                                                                             : "eq";
    if (!operation || *operation != required_operation) {
      return std::unexpected(BuildError(
          "BUILD_REFENV_OPERATOR_UNKNOWN", *schema, std::string("$.host.") + std::string(path) + ".op",
          required_operation, operation ? *operation : "non-string", "reference operator is unknown for this fact"));
    }
    bool available = !fact->IsNull();
    bool matches{};
    if (*operation == "set_contains_all") {
      if (!IsSortedUniqueTokens(*expected_value, false) || (available && !IsSortedUniqueTokens(*fact, true))) {
        return std::unexpected(BuildError(
            "BUILD_REFENV_FIELD_INVALID", *schema, std::string("$.host.") + std::string(path),
            "sorted unique token arrays", "invalid tokens", "ISA predicate or fact is invalid"));
      }
      const auto* expected_array = expected_value->AsArray();
      const auto* fact_array = fact->AsArray();
      if (available) {
        matches = std::all_of(expected_array->begin(), expected_array->end(), [&](const JsonValue& expected_token) {
          return std::any_of(fact_array->begin(), fact_array->end(), [&](const JsonValue& observed_token) {
            return JsonEqual(expected_token, observed_token);
          });
        });
      }
    } else if (*operation == "u32_eq") {
      if (!IsUint32(*expected_value) || (available && !IsUint32(*fact))) {
        return std::unexpected(BuildError(
            "BUILD_REFENV_FIELD_INVALID", *schema, std::string("$.host.") + std::string(path),
            "uint32 predicate and fact", "wrong type or bound", "numeric host fact is invalid"));
      }
      matches = available && JsonEqual(*expected_value, *fact);
    } else {
      if (!IsBoundedString(*expected_value, 256) || (available && !IsBoundedString(*fact, 256))) {
        return std::unexpected(BuildError(
            "BUILD_REFENV_FIELD_INVALID", *schema, std::string("$.host.") + std::string(path),
            "NFC string[1..256] predicate and fact", "wrong type or bound", "string host fact is invalid"));
      }
      matches = available && JsonEqual(*expected_value, *fact);
    }
    const std::string status = !available ? "unavailable" : (matches ? "pass" : "mismatch");
    if (status != "pass") validation.overall = "unvalidated";
    validation.outcomes.push_back(ReferenceOutcome{
        .path = std::string("host.") + std::string(path),
        .operation = std::string(*operation),
        .expected = *expected_value,
        .observed = *fact,
        .status = status,
        .code = status == "pass" ? "none" : "BUILD_UNVALIDATED_ENVIRONMENT",
    });
  }
  return validation;
}

Result<PackageIdentity> IdentifyPackageTree(
    const std::filesystem::path& root,
    const PackageLimits& limits,
    std::optional<ResourceUsage> injected_usage) {
  if (injected_usage) {
    auto status = CheckResourceUsage(
        *injected_usage,
        ResourceLimits{
            .input_bytes = limits.maximum_regular_bytes,
            .count = limits.maximum_entries,
            .rss_bytes = limits.maximum_rss_bytes,
            .wall_time_ns = limits.maximum_wall_time_ns,
        },
        "M0-PACKAGE-TREE-v1", "BUILD_PACKAGE_IDENTITY_INVALID");
    if (!status) return std::unexpected(status.error());
  }
  std::error_code filesystem_error;
  if (!std::filesystem::is_directory(root, filesystem_error) || filesystem_error) {
    return std::unexpected(BuildError(
        "BUILD_PACKAGE_IDENTITY_INVALID", "M0-PACKAGE-TREE-v1", "$", "readable package directory",
        root.string(), "package root is not a readable directory"));
  }
  const auto started = std::chrono::steady_clock::now();
  struct Row {
    std::string path;
    JsonValue value;
  };
  std::vector<Row> rows;
  std::uint64_t regular_bytes{};

  for (std::filesystem::recursive_directory_iterator iterator(
           root, std::filesystem::directory_options::none, filesystem_error), end;
       iterator != end; iterator.increment(filesystem_error)) {
    if (filesystem_error) {
      return std::unexpected(BuildError(
          "BUILD_PACKAGE_IDENTITY_INVALID", "M0-PACKAGE-TREE-v1", "$", "bounded package walk",
          filesystem_error.message(), "package walk failed"));
    }
    if (rows.size() == limits.maximum_entries) {
      return std::unexpected(BuildError(
          "BUILD_PACKAGE_IDENTITY_INVALID", "M0-PACKAGE-TREE-v1", "$.entries", "entry count within bound",
          std::to_string(rows.size() + 1), "package entry bound exceeded",
          static_cast<std::int64_t>(limits.maximum_entries)));
    }
    const auto relative = iterator->path().lexically_relative(root).generic_string();
    auto path_status = ValidateRelativePath(relative);
    if (!path_status) return std::unexpected(path_status.error());
    if (relative.size() > limits.maximum_path_bytes) {
      return std::unexpected(BuildError(
          "BUILD_PACKAGE_IDENTITY_INVALID", "M0-PACKAGE-TREE-v1", "$.entries[].path",
          "path within bound", relative, "package path bound exceeded",
          static_cast<std::int64_t>(limits.maximum_path_bytes)));
    }
    auto before = ReadStat(iterator->path());
    if (!before) return std::unexpected(before.error());

    if (S_ISREG(before->st_mode)) {
      if (before->st_nlink != 1 || before->st_size < 0) {
        return std::unexpected(BuildError(
            "BUILD_PACKAGE_IDENTITY_INVALID", "M0-PACKAGE-TREE-v1", relative,
            "unambiguous regular file", "hard link or negative size", "regular-file identity is ambiguous"));
      }
      const auto size = static_cast<std::uint64_t>(before->st_size);
      if (size > limits.maximum_regular_bytes - regular_bytes) {
        return std::unexpected(BuildError(
            "BUILD_PACKAGE_IDENTITY_INVALID", "M0-PACKAGE-TREE-v1", relative,
            "summed regular bytes within bound", std::to_string(regular_bytes + size),
            "package regular-byte bound exceeded", static_cast<std::int64_t>(limits.maximum_regular_bytes)));
      }
      regular_bytes += size;
      std::ifstream input(iterator->path(), std::ios::binary);
      if (!input) {
        return std::unexpected(BuildError(
            "BUILD_PACKAGE_IDENTITY_INVALID", "M0-PACKAGE-TREE-v1", relative,
            "readable regular file", "open failed", "package file cannot be opened"));
      }
      Sha256Stream hasher;
      std::array<std::byte, 64U * 1024U> buffer{};
      std::uint64_t read_bytes{};
      while (input) {
        input.read(reinterpret_cast<char*>(buffer.data()), static_cast<std::streamsize>(buffer.size()));
        const auto count = input.gcount();
        if (count > 0) {
          read_bytes += static_cast<std::uint64_t>(count);
          auto update = hasher.Update(std::span(buffer.data(), static_cast<std::size_t>(count)));
          if (!update) return std::unexpected(update.error());
        }
      }
      if (!input.eof() || read_bytes != size) {
        return std::unexpected(BuildError(
            "BUILD_PACKAGE_IDENTITY_INVALID", "M0-PACKAGE-TREE-v1", relative,
            std::to_string(size), std::to_string(read_bytes), "package file changed or failed during read"));
      }
      auto after = ReadStat(iterator->path());
      if (!after || !SameReadIdentity(*before, *after)) {
        return std::unexpected(BuildError(
            "BUILD_PACKAGE_IDENTITY_INVALID", "M0-PACKAGE-TREE-v1", relative,
            "stable entry during read", "metadata mutation", "package entry mutated while hashing"));
      }
      auto digest = hasher.Finish();
      if (!digest) return std::unexpected(digest.error());
      rows.push_back({relative, JsonValue(JsonValue::Object{
                                      {"kind", "file"},
                                      {"mode", ModeBits(before->st_mode)},
                                      {"path", relative},
                                      {"sha256", digest->Hex()},
                                      {"size", static_cast<std::int64_t>(size)},
                                  })});
    } else if (S_ISDIR(before->st_mode)) {
      rows.push_back({relative, JsonValue(JsonValue::Object{
                                      {"kind", "directory"},
                                      {"mode", ModeBits(before->st_mode)},
                                      {"path", relative},
                                  })});
    } else if (S_ISLNK(before->st_mode)) {
      const auto target = std::filesystem::read_symlink(iterator->path(), filesystem_error).generic_string();
      const auto target_path = std::filesystem::path(target);
      const auto parent_relative = iterator->path().parent_path().lexically_relative(root);
      const auto resolved_target = (parent_relative / target_path).lexically_normal().generic_string();
      const auto resolved_status = ValidateRelativePath(resolved_target);
      if (filesystem_error || target.empty() || target_path.is_absolute() || !resolved_status ||
          target.size() > limits.maximum_path_bytes || !IsValidNfc(target)) {
        return std::unexpected(BuildError(
            "BUILD_PACKAGE_IDENTITY_INVALID", "M0-PACKAGE-TREE-v1", relative,
            "bounded NFC package-confined symlink target", target, "symlink target is invalid"));
      }
      rows.push_back({relative, JsonValue(JsonValue::Object{
                                      {"kind", "symlink"},
                                      {"mode", ModeBits(before->st_mode)},
                                      {"path", relative},
                                      {"target", target},
                                  })});
    } else {
      return std::unexpected(BuildError(
          "BUILD_PACKAGE_IDENTITY_INVALID", "M0-PACKAGE-TREE-v1", relative,
          "regular file, directory, or symlink", "special file", "special package entries are forbidden"));
    }

    const auto elapsed = std::chrono::duration_cast<std::chrono::nanoseconds>(
                             std::chrono::steady_clock::now() - started)
                             .count();
    if (elapsed > static_cast<std::int64_t>(limits.maximum_wall_time_ns)) {
      return std::unexpected(BuildError(
          "BUILD_PACKAGE_IDENTITY_INVALID", "M0-PACKAGE-TREE-v1", "$", "wall_time_ns",
          std::to_string(elapsed), "package walk deadline exceeded",
          static_cast<std::int64_t>(limits.maximum_wall_time_ns)));
    }
  }
  std::sort(rows.begin(), rows.end(), [](const Row& left, const Row& right) {
    return UnsignedPathLess(left.path, right.path);
  });
  JsonValue::Array entries;
  for (auto& row : rows) entries.push_back(std::move(row.value));
  JsonValue manifest(JsonValue::Object{{"entries", std::move(entries)}, {"schema", "M0-PACKAGE-TREE-v1"}});
  auto bytes = EmitCanonicalJson(manifest, "M0-PACKAGE-TREE-v1");
  if (!bytes) return std::unexpected(bytes.error());
  auto digest = Sha256(*bytes);
  if (!digest) return std::unexpected(digest.error());
  return PackageIdentity{
      .manifest = std::move(manifest),
      .package_tree_sha256 = *digest,
      .regular_bytes = regular_bytes,
      .entry_count = rows.size(),
  };
}

}  // namespace orus::contracts
