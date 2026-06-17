#pragma once

#include "vodozemac/src/lib.rs.h"

#include <catch2/catch_test_macros.hpp>

#include <array>
#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

inline rust::Slice<const std::uint8_t> bytes_slice(const std::vector<std::uint8_t> &data) {
  return rust::Slice<const std::uint8_t>(data.data(), data.size());
}

inline rust::Slice<const std::uint8_t> bytes_slice(const char *data) {
  return rust::Slice<const std::uint8_t>(
      reinterpret_cast<const std::uint8_t *>(data), std::strlen(data));
}

inline void require_bytes_eq(const rust::Vec<std::uint8_t> &actual,
                             const std::vector<std::uint8_t> &expected) {
  REQUIRE(actual.size() == expected.size());
  for (std::size_t i = 0; i < expected.size(); ++i) {
    REQUIRE(actual[i] == expected[i]);
  }
}

inline void require_bytes_eq(const rust::Vec<std::uint8_t> &actual, const char *expected) {
  require_bytes_eq(actual, std::vector<std::uint8_t>(
                               reinterpret_cast<const std::uint8_t *>(expected),
                               reinterpret_cast<const std::uint8_t *>(expected) +
                                   std::strlen(expected)));
}

inline std::array<std::uint8_t, 32> dehydration_key() {
  std::array<std::uint8_t, 32> key{};
  key.fill(7);
  return key;
}
