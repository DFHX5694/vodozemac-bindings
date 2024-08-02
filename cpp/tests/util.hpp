#pragma once
#include <random>
#include <algorithm>
#include <array>
#include <vector>
#include <gtest/gtest.h>
#include <olm/olm.h>

inline std::array<uint8_t, 32> PICKLE_KEY = {};
// olm pickles are of variable length and are then hashed to obtain the actual key
inline std::vector<uint8_t> OLM_PICKLE_KEY = {0, 7, 2, 1};

[[nodiscard]] inline std::vector<uint8_t> gen_random(int len)
{
  auto rd = std::random_device{};
  auto ret = std::vector<uint8_t>(len, '\0');
  std::generate(ret.begin(), ret.end(), [&] { return rd(); });
  return ret;
}

inline void check_olm_error(std::size_t code)
{
  EXPECT_NE(code, olm_error());
}

template<class T>
[[nodiscard]] inline std::string as_std_string(const T &range)
{
  return std::string(range.begin(), range.end());
}

std::pair<std::vector<uint8_t>, OlmAccount *> new_olm_account()
{
  auto data = std::vector<uint8_t>(olm_account_size());
  auto account = olm_account(data.data());
  auto random = gen_random(olm_create_account_random_length(account));
  check_olm_error(olm_create_account(account, random.data(), random.size()));

  return {std::move(data), account};
}
