#pragma once
#include <random>
#include <algorithm>
#include <optional>
#include <array>
#include <vector>
#include <functional>
#include <catch2/catch_test_macros.hpp>
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
  REQUIRE(code != olm_error());
}

template<class T>
[[nodiscard]] inline std::string as_std_string(const T &range)
{
  return std::string(range.begin(), range.end());
}

[[nodiscard]] inline std::string as_std_string(const char *cStr)
{
  return std::string(cStr);
}

std::pair<std::vector<uint8_t>, OlmAccount *> new_olm_account()
{
  auto data = std::vector<uint8_t>(olm_account_size());
  auto account = olm_account(data.data());
  auto random = gen_random(olm_create_account_random_length(account));
  check_olm_error(olm_create_account(account, random.data(), random.size()));

  return {std::move(data), account};
}

#if VODOZEMAC_TEST_EXCEPTIONS
#define MAYBE_NOEXCEPT(funcName) funcName

template<class Func>
inline auto require_vodozemac_ok(Func &&f)
{
    try {
        return std::invoke(std::forward<Func>(f));
    } catch (const std::exception &e) {
        REQUIRE_NOTHROW(throw e);
        // the following line is unreachable
        std::abort();
    }
}

template<class Func>
inline std::string require_vodozemac_error(Func &&f)
{
    try {
        std::invoke(std::forward<Func>(f));
    } catch (const std::exception &e) {
         return e.what();
    }
    FAIL("expects an exception");
    return "";
}
#else
#define MAYBE_NOEXCEPT(funcName) funcName##_noexcept

template<class Func>
inline auto require_vodozemac_ok(Func &&f)
{
    auto res = std::invoke(std::forward<Func>(f));
    REQUIRE(res->has_value());
    return res->take_value();
}

template<class Func>
inline std::string require_vodozemac_error(Func &&f)
{
    auto res = std::invoke(std::forward<Func>(f));
    REQUIRE(!res->has_value());
    auto error = res->take_error();
    return as_std_string(error);
}
#endif
#define REQUIRE_VODOZEMAC_OK(...) require_vodozemac_ok([&] { return __VA_ARGS__; })
#define REQUIRE_VODOZEMAC_ERROR(...) require_vodozemac_error([&] { return __VA_ARGS__; })
