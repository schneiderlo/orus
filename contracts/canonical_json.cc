#include "orus/contracts/contracts.h"

#include <algorithm>
#include <charconv>
#include <cstdlib>
#include <limits>
#include <set>
#include <sstream>

#include <glaze/json.hpp>
#include <utf8proc.h>

namespace orus::contracts {
namespace {

Error MakeError(
    std::string_view code,
    std::string_view schema,
    std::string_view path,
    std::string_view expected,
    std::string_view observed,
    std::string_view message,
    std::optional<std::int64_t> limit = std::nullopt,
    std::optional<std::int64_t> offset = std::nullopt) {
  return Error{
      .code = std::string(code),
      .document_schema = std::string(schema),
      .field_path = std::string(path),
      .expected = std::string(expected),
      .observed = std::string(observed.substr(0, 1024)),
      .limit = limit,
      .offset = offset,
      .message = std::string(message),
  };
}

bool UnsignedLess(std::string_view left, std::string_view right) {
  return std::lexicographical_compare(
      left.begin(), left.end(), right.begin(), right.end(),
      [](char a, char b) { return static_cast<unsigned char>(a) < static_cast<unsigned char>(b); });
}

void AppendUtf8(std::uint32_t scalar, std::string& output) {
  if (scalar <= 0x7fU) {
    output.push_back(static_cast<char>(scalar));
  } else if (scalar <= 0x7ffU) {
    output.push_back(static_cast<char>(0xc0U | (scalar >> 6U)));
    output.push_back(static_cast<char>(0x80U | (scalar & 0x3fU)));
  } else if (scalar <= 0xffffU) {
    output.push_back(static_cast<char>(0xe0U | (scalar >> 12U)));
    output.push_back(static_cast<char>(0x80U | ((scalar >> 6U) & 0x3fU)));
    output.push_back(static_cast<char>(0x80U | (scalar & 0x3fU)));
  } else {
    output.push_back(static_cast<char>(0xf0U | (scalar >> 18U)));
    output.push_back(static_cast<char>(0x80U | ((scalar >> 12U) & 0x3fU)));
    output.push_back(static_cast<char>(0x80U | ((scalar >> 6U) & 0x3fU)));
    output.push_back(static_cast<char>(0x80U | (scalar & 0x3fU)));
  }
}

std::optional<std::uint32_t> HexScalar(std::string_view input) {
  if (input.size() != 4) {
    return std::nullopt;
  }
  std::uint32_t result{};
  for (const char character : input) {
    result <<= 4U;
    if (character >= '0' && character <= '9') {
      result |= static_cast<std::uint32_t>(character - '0');
    } else if (character >= 'a' && character <= 'f') {
      result |= static_cast<std::uint32_t>(character - 'a' + 10);
    } else if (character >= 'A' && character <= 'F') {
      result |= static_cast<std::uint32_t>(character - 'A' + 10);
    } else {
      return std::nullopt;
    }
  }
  return result;
}

class Parser {
 public:
  Parser(std::string_view input, std::string_view schema, std::string_view code, ParseLimits limits)
      : input_(input), schema_(schema), code_(code), limits_(limits) {}

  Result<JsonValue> Parse() {
    auto value = ParseValue(1, "$");
    if (!value) {
      return std::unexpected(value.error());
    }
    if (position_ != input_.size()) {
      return Failure("$", "end of document", "trailing bytes", "document has trailing bytes");
    }
    return value;
  }

 private:
  std::unexpected<Error> Failure(
      std::string_view path,
      std::string_view expected,
      std::string_view observed,
      std::string_view message) const {
    return std::unexpected(MakeError(
        code_, schema_, path, expected, observed, message, std::nullopt,
        static_cast<std::int64_t>(position_)));
  }

  Result<JsonValue> ParseValue(std::size_t depth, std::string path) {
    if (depth > limits_.maximum_depth) {
      return std::unexpected(MakeError(
          code_, schema_, path, "nesting within bound", "depth exceeded",
          "canonical JSON nesting depth exceeded", static_cast<std::int64_t>(limits_.maximum_depth),
          static_cast<std::int64_t>(position_)));
    }
    if (position_ >= input_.size()) {
      return Failure(path, "JSON value", "end of input", "value is missing");
    }
    switch (input_[position_]) {
      case '{':
        return ParseObject(depth, std::move(path));
      case '[':
        return ParseArray(depth, std::move(path));
      case '"': {
        auto parsed = ParseString(path);
        if (!parsed) return std::unexpected(parsed.error());
        return JsonValue(std::move(*parsed));
      }
      case 't':
        return ParseLiteral("true", JsonValue(true), path);
      case 'f':
        return ParseLiteral("false", JsonValue(false), path);
      case 'n':
        return ParseLiteral("null", JsonValue(nullptr), path);
      default:
        if (input_[position_] == '-' || (input_[position_] >= '0' && input_[position_] <= '9')) {
          return ParseInteger(path);
        }
        return Failure(path, "canonical JSON value", std::string(1, input_[position_]), "invalid value token");
    }
  }

  Result<JsonValue> ParseLiteral(std::string_view literal, JsonValue value, std::string_view path) {
    if (input_.substr(position_, literal.size()) != literal) {
      return Failure(path, literal, input_.substr(position_, literal.size()), "invalid literal");
    }
    position_ += literal.size();
    return value;
  }

  Result<JsonValue> ParseInteger(std::string_view path) {
    const std::size_t start = position_;
    if (input_[position_] == '-') {
      ++position_;
      if (position_ >= input_.size()) {
        return Failure(path, "integer digit", "end of input", "incomplete integer");
      }
    }
    if (input_[position_] == '0') {
      ++position_;
      if (position_ < input_.size() && input_[position_] >= '0' && input_[position_] <= '9') {
        return Failure(path, "shortest base-10 integer", "leading zero", "leading zero is forbidden");
      }
    } else if (input_[position_] >= '1' && input_[position_] <= '9') {
      while (position_ < input_.size() && input_[position_] >= '0' && input_[position_] <= '9') {
        ++position_;
      }
    } else {
      return Failure(path, "integer digit", std::string(1, input_[position_]), "invalid integer");
    }
    const std::string_view token = input_.substr(start, position_ - start);
    if (token == "-0") {
      return Failure(path, "0", "-0", "negative zero is forbidden");
    }
    if (position_ < input_.size() &&
        (input_[position_] == '.' || input_[position_] == 'e' || input_[position_] == 'E' ||
         input_[position_] == '+')) {
      return Failure(path, "signed 64-bit integer", input_.substr(start, position_ - start + 1),
                     "floating-point, exponent, and plus forms are forbidden");
    }
    std::int64_t result{};
    const auto [end, error] = std::from_chars(token.data(), token.data() + token.size(), result);
    if (error != std::errc{} || end != token.data() + token.size()) {
      return Failure(path, "signed 64-bit integer", token, "integer is outside the signed 64-bit domain");
    }
    return JsonValue(result);
  }

  Result<std::string> ParseString(std::string_view path) {
    ++position_;
    std::string result;
    while (position_ < input_.size()) {
      const unsigned char character = static_cast<unsigned char>(input_[position_++]);
      if (character == '"') {
        if (!IsValidNfc(result)) {
          return std::unexpected(MakeError(
              code_, schema_, path, "valid NFC UTF-8", "invalid or non-NFC string",
              "JSON strings and names must be valid NFC UTF-8", std::nullopt,
              static_cast<std::int64_t>(position_ - 1)));
        }
        return result;
      }
      if (character < 0x20U) {
        return Failure(path, "escaped control scalar", "unescaped control", "control scalar is not escaped");
      }
      if (character != '\\') {
        result.push_back(static_cast<char>(character));
        continue;
      }
      if (position_ >= input_.size()) {
        return Failure(path, "escape", "end of input", "incomplete string escape");
      }
      const char escape = input_[position_++];
      switch (escape) {
        case '"': result.push_back('"'); break;
        case '\\': result.push_back('\\'); break;
        case '/': result.push_back('/'); break;
        case 'b': result.push_back('\b'); break;
        case 'f': result.push_back('\f'); break;
        case 'n': result.push_back('\n'); break;
        case 'r': result.push_back('\r'); break;
        case 't': result.push_back('\t'); break;
        case 'u': {
          if (position_ + 4 > input_.size()) {
            return Failure(path, "four hexadecimal digits", "truncated unicode escape", "unicode escape is truncated");
          }
          auto scalar = HexScalar(input_.substr(position_, 4));
          position_ += 4;
          if (!scalar) {
            return Failure(path, "four hexadecimal digits", "invalid unicode escape", "unicode escape is invalid");
          }
          if (*scalar >= 0xd800U && *scalar <= 0xdbffU) {
            if (position_ + 6 > input_.size() || input_[position_] != '\\' || input_[position_ + 1] != 'u') {
              return Failure(path, "low surrogate", "lone high surrogate", "lone surrogate is forbidden");
            }
            auto low = HexScalar(input_.substr(position_ + 2, 4));
            if (!low || *low < 0xdc00U || *low > 0xdfffU) {
              return Failure(path, "low surrogate", "invalid low surrogate", "surrogate pair is invalid");
            }
            position_ += 6;
            *scalar = 0x10000U + ((*scalar - 0xd800U) << 10U) + (*low - 0xdc00U);
          } else if (*scalar >= 0xdc00U && *scalar <= 0xdfffU) {
            return Failure(path, "Unicode scalar", "lone low surrogate", "lone surrogate is forbidden");
          }
          AppendUtf8(*scalar, result);
          break;
        }
        default:
          return Failure(path, "canonical string escape", std::string(1, escape), "unknown string escape");
      }
    }
    return std::unexpected(MakeError(
        code_, schema_, path, "closing quote", "end of input", "unterminated string", std::nullopt,
        static_cast<std::int64_t>(position_)));
  }

  Result<JsonValue> ParseArray(std::size_t depth, std::string path) {
    ++position_;
    JsonValue::Array result;
    if (position_ < input_.size() && input_[position_] == ']') {
      ++position_;
      return JsonValue(std::move(result));
    }
    for (std::size_t index = 0;; ++index) {
      auto element = ParseValue(depth + 1, path + "[" + std::to_string(index) + "]");
      if (!element) return std::unexpected(element.error());
      result.push_back(std::move(*element));
      if (position_ >= input_.size()) {
        return Failure(path, "comma or closing bracket", "end of input", "array is unterminated");
      }
      if (input_[position_] == ']') {
        ++position_;
        return JsonValue(std::move(result));
      }
      if (input_[position_] != ',') {
        return Failure(path, "comma", std::string(1, input_[position_]), "array separator is invalid");
      }
      ++position_;
    }
  }

  Result<JsonValue> ParseObject(std::size_t depth, std::string path) {
    ++position_;
    JsonValue::Object result;
    std::optional<std::string> previous;
    if (position_ < input_.size() && input_[position_] == '}') {
      ++position_;
      return JsonValue(std::move(result));
    }
    for (;;) {
      if (position_ >= input_.size() || input_[position_] != '"') {
        return Failure(path, "object member name", "invalid member", "object member name is missing");
      }
      auto name = ParseString(path);
      if (!name) return std::unexpected(name.error());
      if (previous && !UnsignedLess(*previous, *name)) {
        const bool duplicate = *previous == *name;
        return Failure(path, "strictly unsigned-byte-sorted unique names", *name,
                       duplicate ? "duplicate object name" : "object names are not canonical byte sorted");
      }
      previous = *name;
      if (position_ >= input_.size() || input_[position_] != ':') {
        return Failure(path, "colon", "missing colon", "object member separator is invalid");
      }
      ++position_;
      auto member = ParseValue(depth + 1, path + "." + *name);
      if (!member) return std::unexpected(member.error());
      result.emplace_back(std::move(*name), std::move(*member));
      if (position_ >= input_.size()) {
        return Failure(path, "comma or closing brace", "end of input", "object is unterminated");
      }
      if (input_[position_] == '}') {
        ++position_;
        return JsonValue(std::move(result));
      }
      if (input_[position_] != ',') {
        return Failure(path, "comma", std::string(1, input_[position_]), "object separator is invalid");
      }
      ++position_;
    }
  }

  std::string_view input_;
  std::string schema_;
  std::string code_;
  ParseLimits limits_;
  std::size_t position_{};
};

Result<void> EmitValue(
    const JsonValue& value,
    std::string& output,
    std::string_view schema,
    const ParseLimits& limits,
    std::size_t depth,
    std::string_view path) {
  if (depth > limits.maximum_depth) {
    return std::unexpected(MakeError(
        "CANONICAL_JSON_RESOURCE_LIMIT", schema, path, "depth within bound", "depth exceeded",
        "canonical emission depth exceeded", static_cast<std::int64_t>(limits.maximum_depth)));
  }
  if (std::holds_alternative<std::nullptr_t>(value.value)) {
    output += "null";
  } else if (const auto* boolean = std::get_if<bool>(&value.value)) {
    output += *boolean ? "true" : "false";
  } else if (const auto* integer = std::get_if<std::int64_t>(&value.value)) {
    output += std::to_string(*integer);
  } else if (const auto* string = std::get_if<std::string>(&value.value)) {
    if (!IsValidNfc(*string)) {
      return std::unexpected(MakeError(
          "CANONICAL_JSON_NONCANONICAL", schema, path, "valid NFC UTF-8", "invalid or non-NFC",
          "string cannot be emitted canonically"));
    }
    output.push_back('"');
    static constexpr char kHex[] = "0123456789abcdef";
    for (const unsigned char character : *string) {
      switch (character) {
        case '"': output += "\\\""; break;
        case '\\': output += "\\\\"; break;
        case '\b': output += "\\b"; break;
        case '\t': output += "\\t"; break;
        case '\n': output += "\\n"; break;
        case '\f': output += "\\f"; break;
        case '\r': output += "\\r"; break;
        default:
          if (character < 0x20U) {
            output += "\\u00";
            output.push_back(kHex[character >> 4U]);
            output.push_back(kHex[character & 0x0fU]);
          } else {
            output.push_back(static_cast<char>(character));
          }
      }
    }
    output.push_back('"');
  } else if (const auto* array = std::get_if<JsonValue::Array>(&value.value)) {
    output.push_back('[');
    for (std::size_t index = 0; index < array->size(); ++index) {
      if (index != 0) output.push_back(',');
      auto status = EmitValue((*array)[index], output, schema, limits, depth + 1, path);
      if (!status) return status;
    }
    output.push_back(']');
  } else {
    auto object = std::get<JsonValue::Object>(value.value);
    std::sort(object.begin(), object.end(), [](const auto& left, const auto& right) {
      return UnsignedLess(left.first, right.first);
    });
    for (std::size_t index = 0; index < object.size(); ++index) {
      if (!IsValidNfc(object[index].first) ||
          (index != 0 && object[index - 1].first == object[index].first)) {
        return std::unexpected(MakeError(
            "CANONICAL_JSON_NONCANONICAL", schema, path, "unique NFC member names",
            object[index].first, "object member names are invalid"));
      }
    }
    output.push_back('{');
    for (std::size_t index = 0; index < object.size(); ++index) {
      if (index != 0) output.push_back(',');
      auto name_status = EmitValue(JsonValue(object[index].first), output, schema, limits, depth + 1, path);
      if (!name_status) return name_status;
      output.push_back(':');
      auto value_status = EmitValue(object[index].second, output, schema, limits, depth + 1, path);
      if (!value_status) return value_status;
    }
    output.push_back('}');
  }
  if (output.size() > limits.maximum_bytes) {
    return std::unexpected(MakeError(
        "CANONICAL_JSON_RESOURCE_LIMIT", schema, path, "document within byte bound", "size exceeded",
        "canonical emission byte bound exceeded", static_cast<std::int64_t>(limits.maximum_bytes)));
  }
  return {};
}

}  // namespace

const JsonValue::Object* JsonValue::AsObject() const { return std::get_if<Object>(&value); }
const JsonValue::Array* JsonValue::AsArray() const { return std::get_if<Array>(&value); }
const std::string* JsonValue::AsString() const { return std::get_if<std::string>(&value); }
const std::int64_t* JsonValue::AsInteger() const { return std::get_if<std::int64_t>(&value); }
const bool* JsonValue::AsBoolean() const { return std::get_if<bool>(&value); }
bool JsonValue::IsNull() const { return std::holds_alternative<std::nullptr_t>(value); }

Result<JsonValue> ParseCanonicalJson(
    std::string_view bytes,
    std::string_view document_schema,
    std::string_view noncanonical_code,
    ParseLimits limits) {
  if (bytes.size() > limits.maximum_bytes) {
    return std::unexpected(MakeError(
        noncanonical_code, document_schema, "$", "document within byte bound", "size exceeded",
        "canonical JSON document exceeds byte bound", static_cast<std::int64_t>(limits.maximum_bytes), 0));
  }
  if (bytes.starts_with("\xef\xbb\xbf")) {
    return std::unexpected(MakeError(
        noncanonical_code, document_schema, "$", "UTF-8 without BOM", "BOM", "BOM is forbidden", std::nullopt, 0));
  }
  Parser parser(bytes, document_schema, noncanonical_code, limits);
  auto value = parser.Parse();
  if (!value) return value;

  std::string canonical;
  auto emitted = EmitValue(*value, canonical, document_schema, limits, 1, "$");
  if (!emitted) {
    auto error = emitted.error();
    error.code = std::string(noncanonical_code);
    return std::unexpected(std::move(error));
  }
  if (canonical != bytes) {
    std::size_t mismatch{};
    while (mismatch < canonical.size() && mismatch < bytes.size() && canonical[mismatch] == bytes[mismatch]) {
      ++mismatch;
    }
    return std::unexpected(MakeError(
        noncanonical_code, document_schema, "$", "M0-CANONICAL-JSON-v1 bytes",
        "noncanonical byte form", "input is valid JSON but not canonical", std::nullopt,
        static_cast<std::int64_t>(mismatch)));
  }

  // Glaze is deliberately a private syntax/parser defense layer. The public
  // API and accepted value have already passed Orus byte, integer, NFC,
  // duplicate-name, order, and resource checks.
  glz::generic private_glaze_value{};
  std::string private_glaze_bytes(bytes);
  if (glz::read_json(private_glaze_value, private_glaze_bytes) != glz::error_code::none) {
    return std::unexpected(MakeError(
        noncanonical_code, document_schema, "$", "Glaze parse after Orus prevalidation",
        "Glaze rejected bytes", "private parser disagreement"));
  }
  return value;
}

Result<std::string> EmitCanonicalJson(
    const JsonValue& value, std::string_view document_schema, ParseLimits limits) {
  std::string output;
  auto status = EmitValue(value, output, document_schema, limits, 1, "$");
  if (!status) return std::unexpected(status.error());
  return output;
}

const JsonValue* FindMember(const JsonValue& value, std::string_view name) {
  const auto* object = value.AsObject();
  if (object == nullptr) return nullptr;
  const auto found = std::lower_bound(object->begin(), object->end(), name, [](const auto& row, std::string_view key) {
    return UnsignedLess(row.first, key);
  });
  return found != object->end() && found->first == name ? &found->second : nullptr;
}

Result<const JsonValue::Object*> RequireObject(
    const JsonValue& value, std::string_view schema, std::string_view path, std::string_view code) {
  if (const auto* result = value.AsObject()) return result;
  return std::unexpected(MakeError(code, schema, path, "object", "other type", "field has wrong type"));
}

Result<const JsonValue::Array*> RequireArray(
    const JsonValue& value, std::string_view schema, std::string_view path, std::string_view code) {
  if (const auto* result = value.AsArray()) return result;
  return std::unexpected(MakeError(code, schema, path, "array", "other type", "field has wrong type"));
}

Result<std::string_view> RequireString(
    const JsonValue& value, std::string_view schema, std::string_view path, std::string_view code) {
  if (const auto* result = value.AsString()) return *result;
  return std::unexpected(MakeError(code, schema, path, "string", "other type", "field has wrong type"));
}

Result<std::int64_t> RequireInteger(
    const JsonValue& value, std::string_view schema, std::string_view path, std::string_view code) {
  if (const auto* result = value.AsInteger()) return *result;
  return std::unexpected(MakeError(code, schema, path, "integer", "other type", "field has wrong type"));
}

Result<bool> RequireBoolean(
    const JsonValue& value, std::string_view schema, std::string_view path, std::string_view code) {
  if (const auto* result = value.AsBoolean()) return *result;
  return std::unexpected(MakeError(code, schema, path, "boolean", "other type", "field has wrong type"));
}

Result<std::string> NormalizeNfc(std::string_view utf8) {
  if (utf8.size() > static_cast<std::size_t>(std::numeric_limits<utf8proc_ssize_t>::max())) {
    return std::unexpected(MakeError(
        "CANONICAL_JSON_RESOURCE_LIMIT", "M0-CANONICAL-JSON-v1", "$", "bounded UTF-8", "size exceeded",
        "Unicode input cannot be represented by utf8proc"));
  }
  utf8proc_uint8_t* normalized = nullptr;
  const auto result = utf8proc_map(
      reinterpret_cast<const utf8proc_uint8_t*>(utf8.data()), static_cast<utf8proc_ssize_t>(utf8.size()),
      &normalized, static_cast<utf8proc_option_t>(UTF8PROC_STABLE | UTF8PROC_COMPOSE));
  if (result < 0 || normalized == nullptr) {
    if (normalized != nullptr) std::free(normalized);
    return std::unexpected(MakeError(
        "CANONICAL_JSON_NONCANONICAL", "M0-CANONICAL-JSON-v1", "$", "valid UTF-8",
        "invalid UTF-8", "utf8proc rejected the Unicode input"));
  }
  std::string output(reinterpret_cast<char*>(normalized), static_cast<std::size_t>(result));
  std::free(normalized);
  return output;
}

bool IsValidNfc(std::string_view utf8) {
  auto normalized = NormalizeNfc(utf8);
  return normalized.has_value() && *normalized == utf8;
}

}  // namespace orus::contracts
