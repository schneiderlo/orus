#include "orus/contracts/contracts.h"

#include <gtest/gtest.h>

namespace {

TEST(PublicHeader, ConsumerUsesOnlyOrusOwnedTypes) {
  orus::contracts::JsonValue value(7);
  auto bytes = orus::contracts::EmitCanonicalJson(value, "consumer");
  ASSERT_TRUE(bytes);
  EXPECT_EQ(*bytes, "7");
}

}  // namespace
