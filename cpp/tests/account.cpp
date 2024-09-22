#include "../../target/cxxbridge/vodozemac/src/lib.rs.h"
#include <string>
#include <unordered_set>
#include <olm/olm.h>
#include <boost/json.hpp>
#include <catch2/catch_test_macros.hpp>
#include "util.hpp"

using namespace rust;
using namespace vodozemac;

TEST_CASE("AccountCreation", "[AccountTest]") {
  auto alice = olm::new_account();
  auto key = alice->ed25519_key();
  auto encoded_key = key->to_base64();

  REQUIRE(encoded_key.length() != 0);
}

TEST_CASE("OneTimeKeyGeneration", "[AccountTest]") {
  auto alice = olm::new_account();

  REQUIRE(alice->one_time_keys().size() == 0);

  auto result = alice->generate_one_time_keys(10);
  REQUIRE(result->created().size() == 10);
  REQUIRE(alice->one_time_keys().size() == 10);

  alice->mark_keys_as_published();
  REQUIRE(alice->one_time_keys().size() == 0);
}

TEST_CASE("OneTimeKeysRemovedWhenTooMany", "[AccountTest]") {
  auto alice = olm::new_account();

  // Vodozemac will remove old one-time keys when the total number of generated one-time keys exceeds an internal constant.
  // That internal constant is not a part of the public API, that's why we keep generating new keys until the first removal.
  static constexpr std::size_t upper_bound = 1000000;
  // The long variable names are needed for better error messages in case the test fails.
  std::size_t total_number_of_generated_one_time_keys = 0;
  std::size_t total_number_of_one_time_keys_removed = 0;
  while (total_number_of_one_time_keys_removed == 0 && total_number_of_generated_one_time_keys < upper_bound) {
    auto result = alice->generate_one_time_keys(alice->max_number_of_one_time_keys());
    total_number_of_generated_one_time_keys += result->created().size();
    total_number_of_one_time_keys_removed += result->removed().size();
  }

  SECTION("After generating really many one-time keys the oldest ones should be removed") {
    REQUIRE(total_number_of_one_time_keys_removed > 0);
    REQUIRE(alice->one_time_keys().size() + total_number_of_one_time_keys_removed == total_number_of_generated_one_time_keys);
  }
}

TEST_CASE("FallbackKeyGeneration", "[AccountTest]") {
  auto alice = olm::new_account();
  REQUIRE(alice->fallback_key().size() == 0);

  auto previous_fallback_key = alice->generate_fallback_key();
  REQUIRE(alice->fallback_key().size() == 1);
  REQUIRE(previous_fallback_key.size() == 0);

  alice->mark_keys_as_published();
  REQUIRE(alice->fallback_key().size() == 0);
}

TEST_CASE("FallbackKeyDeprecation", "[AccountTest]") {
  auto alice = olm::new_account();
  REQUIRE(alice->fallback_key().size() == 0);

  auto previous_fallback_key = alice->generate_fallback_key();
  REQUIRE(alice->fallback_key().size() == 1);
  SECTION("When a fallback key is generated for the first time, there should not be any old fallback keys to remove.") {
    REQUIRE(previous_fallback_key.size() == 0);
  }

  alice->mark_keys_as_published();
  REQUIRE(alice->fallback_key().size() == 0);

  // olm::Account stores up to two fallback keys.
  // The old fallback key should not be removed, yet.
  previous_fallback_key = alice->generate_fallback_key();
  REQUIRE(alice->fallback_key().size() == 1);
  SECTION("After a fallback key has been generated twice, the old fallback key should not be removed yet.") {
    REQUIRE(previous_fallback_key.size() == 0);
  }

  alice->mark_keys_as_published();
  REQUIRE(alice->fallback_key().size() == 0);

  // Now we expect the old fallback key to be removed.
  previous_fallback_key = alice->generate_fallback_key();
  REQUIRE(alice->fallback_key().size() == 1);
  SECTION("After a fallback key has been generated three times, the old fallback key should be removed.") {
    REQUIRE(previous_fallback_key.size() == 1);
  }
}

TEST_CASE("MaxKeysTest", "[AccountTest]") {
  auto alice = olm::new_account();
  auto max_keys = alice->max_number_of_one_time_keys();

  REQUIRE(max_keys > 0);
  REQUIRE(max_keys < 1000);
}

TEST_CASE("PickleTest", "[AccountTest]") {
  auto alice = olm::new_account();

  auto pickle = alice->pickle(PICKLE_KEY);

  auto unpickled = olm::account_from_pickle(pickle, PICKLE_KEY);

  REQUIRE(alice->curve25519_key()->to_base64() ==
    unpickled->curve25519_key()->to_base64());
}

TEST_CASE("PickleFromLibolmTest", "[AccountTest]") {
  auto [data, alice] = new_olm_account();
  auto random = gen_random(olm_account_generate_one_time_keys_random_length(alice, 10));
  check_olm_error(olm_account_generate_one_time_keys(alice, 10, random.data(), random.size()));

  auto pickle = std::string(olm_pickle_account_length(alice), '\0');
  check_olm_error(olm_pickle_account(alice, OLM_PICKLE_KEY.data(), OLM_PICKLE_KEY.size(), pickle.data(), pickle.size()));

  auto unpickled = olm::account_from_libolm_pickle(pickle, Slice<const unsigned char>(OLM_PICKLE_KEY.data(), OLM_PICKLE_KEY.size()));

  {
    auto identity_keys = std::string(olm_account_identity_keys_length(alice), '\0');
    check_olm_error(olm_account_identity_keys(alice, identity_keys.data(), identity_keys.size()));
    auto parsed_keys = boost::json::parse(identity_keys).as_object();
    auto curve25519_key = String(as_std_string(parsed_keys.at("curve25519").as_string()));
    auto ed25519_key = String(as_std_string(parsed_keys.at("ed25519").as_string()));

    REQUIRE(curve25519_key ==
      unpickled->curve25519_key()->to_base64());
    REQUIRE(ed25519_key ==
      unpickled->ed25519_key()->to_base64());
  }

  {
    auto one_time_keys = std::string(olm_account_one_time_keys_length(alice), '\0');
    check_olm_error(olm_account_one_time_keys(alice, one_time_keys.data(), one_time_keys.size()));
    auto parsed_keys = boost::json::parse(one_time_keys).as_object().at("curve25519").as_object();
    // The key id will change from libolm to vodozemac, but the key themselves won't.
    auto one_time_keys_set = std::unordered_set<std::string>();
    for (const auto &pair : parsed_keys) {
      one_time_keys_set.insert(as_std_string(pair.value().as_string()));
    }

    auto unpickled_one_time_keys = unpickled->one_time_keys();
    auto unpickled_one_time_keys_set = std::unordered_set<std::string>();
    for (const auto &k : unpickled_one_time_keys) {
      unpickled_one_time_keys_set.insert(static_cast<std::string>(k.key->to_base64()));
    }

    REQUIRE(one_time_keys_set == unpickled_one_time_keys_set);
  }
}

TEST_CASE("SignAndVerify", "[AccountTest]") {
  auto alice = olm::new_account();
  auto key = alice->ed25519_key();
  std::string str = "{}";
  auto signature = alice->sign(Str(str));
  REQUIRE_NOTHROW(key->verify(Str(str), *signature));
  {
    auto cloned_signature = types::ed25519_signature_from_base64(signature->to_base64());
    REQUIRE_NOTHROW(key->verify(Str(str), *cloned_signature));
  }
  {
    auto cloned_key = types::ed25519_key_from_base64(key->to_base64());
    REQUIRE_NOTHROW(cloned_key->verify(Str(str), *signature));
  }

  std::string str2 = "{\"foo\": \"bar\"}";
  auto signature2 = alice->sign(Str(str2));
  REQUIRE_THROWS(key->verify(Str(str2), *signature));
  REQUIRE_THROWS(key->verify(Str(str), *signature2));
}
