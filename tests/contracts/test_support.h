#ifndef ORUS_TESTS_CONTRACTS_TEST_SUPPORT_H_
#define ORUS_TESTS_CONTRACTS_TEST_SUPPORT_H_

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>

namespace orus::contracts::test {

inline std::filesystem::path Runfile(std::string_view relative) {
  const char* source_directory = std::getenv("TEST_SRCDIR");
  const char* workspace = std::getenv("TEST_WORKSPACE");
  if (source_directory == nullptr || workspace == nullptr) {
    throw std::runtime_error("Bazel runfile environment is unavailable");
  }
  return std::filesystem::path(source_directory) / workspace / relative;
}

inline std::string Read(std::string_view relative) {
  std::ifstream input(Runfile(relative), std::ios::binary);
  if (!input) throw std::runtime_error("cannot open runfile " + std::string(relative));
  std::ostringstream output;
  output << input.rdbuf();
  return output.str();
}

}  // namespace orus::contracts::test

#endif  // ORUS_TESTS_CONTRACTS_TEST_SUPPORT_H_
