#include <expected>
#include <version>

#include <gtest/gtest.h>
#include <glaze/glaze.hpp>

struct GlazeFixture {
  int value{};
};

namespace {

static_assert(__cplusplus >= 202302L, "Orus requires C++23");
static_assert(__cpp_lib_expected >= 202202L, "the selected toolchain must provide std::expected");

TEST(ToolchainConformance, SelectsDeclaredCompiler) {
#if defined(ORUS_EXPECT_CLANG)
  EXPECT_TRUE(static_cast<bool>(__clang__));
#elif defined(ORUS_EXPECT_GCC)
  EXPECT_TRUE(static_cast<bool>(__GNUC__));
#ifdef __clang__
  FAIL() << "the GCC profile selected Clang";
#else
  SUCCEED();
#endif
#else
#error "BUILD_CONFIG_INVALID: compiler expectation is absent"
#endif
}

TEST(DependencyContainment, GlazeIsHeaderOnlyBehindTheBazelRepository) {
  GlazeFixture input{.value = 7};
  const auto bytes = glz::write_json(input);
  ASSERT_TRUE(bytes.has_value());
  EXPECT_EQ(*bytes, R"({"value":7})");
}

}  // namespace
