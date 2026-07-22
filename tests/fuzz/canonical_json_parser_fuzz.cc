#include "orus/contracts/contracts.h"

#include <cstddef>
#include <cstdint>
#include <string_view>

extern "C" int LLVMFuzzerTestOneInput(const std::uint8_t* data, std::size_t size) {
  const std::string_view input(reinterpret_cast<const char*>(data), size);
  auto result = orus::contracts::ParseCanonicalJson(
      input,
      "M0-CANONICAL-JSON-v1",
      "CANONICAL_JSON_NONCANONICAL",
      {.maximum_bytes = 64U * 1024U, .maximum_depth = 16});
  if (result) {
    auto emitted = orus::contracts::EmitCanonicalJson(
        *result, "M0-CANONICAL-JSON-v1", {.maximum_bytes = 64U * 1024U, .maximum_depth = 16});
    if (!emitted || *emitted != input) __builtin_trap();
  }
  return 0;
}
