#include "../../target/cxxbridge/vodozemac/src/lib.rs.h"
#include <string>
#include <unordered_set>
#include <olm/olm.h>
#include <boost/json.hpp>
#include "gtest/gtest.h"
#include "util.hpp"

using namespace rust;

TEST(AccountTest, AccountCreation) {
  auto alice = olm::new_account();
  auto key = alice->ed25519_key();
  auto encoded_key = key->to_base64();

  EXPECT_NE(encoded_key.length(), 0);
}

TEST(AccountTest, OneTimeKeyGeneration) {
  auto alice = olm::new_account();

  EXPECT_EQ(alice->one_time_keys().size(), 0);

  alice->generate_one_time_keys(10);
  EXPECT_EQ(alice->one_time_keys().size(), 10);

  alice->mark_keys_as_published();
  EXPECT_EQ(alice->one_time_keys().size(), 0);
}

TEST(AccountTest, FallbackKeyGeneration) {
  auto alice = olm::new_account();
  EXPECT_EQ(alice->fallback_key().size(), 0);

  alice->generate_fallback_key();
  EXPECT_EQ(alice->fallback_key().size(), 1);

  alice->mark_keys_as_published();
  EXPECT_EQ(alice->one_time_keys().size(), 0);
}

TEST(AccountTest, MaxKeysTest) {
  auto alice = olm::new_account();
  auto max_keys = alice->max_number_of_one_time_keys();

  EXPECT_GT(max_keys, 0);
  EXPECT_LT(max_keys, 1000);
}

TEST(AccountTest, PickleTest) {
  auto alice = olm::new_account();

  auto pickle = alice->pickle(PICKLE_KEY);

  auto unpickled = olm::account_from_pickle(pickle, PICKLE_KEY);

  EXPECT_EQ(alice->curve25519_key()->to_base64(),
            unpickled->curve25519_key()->to_base64());
}

TEST(AccountTest, PickleFromLibolmTest) {
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

    EXPECT_EQ(curve25519_key,
      unpickled->curve25519_key()->to_base64());
    EXPECT_EQ(ed25519_key,
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

    EXPECT_EQ(one_time_keys_set, unpickled_one_time_keys_set);
  }
}

TEST(AccountTest, SignAndVerify) {
  auto alice = olm::new_account();
  auto key = alice->ed25519_key();
  std::string str = "{}";
  auto signature = alice->sign(Str(str));
  EXPECT_NO_THROW(key->verify(Str(str), *signature));
  {
    auto cloned_signature = types::ed25519_signature_from_base64(signature->to_base64());
    EXPECT_NO_THROW(key->verify(Str(str), *cloned_signature));
  }
  {
    auto cloned_key = types::ed25519_key_from_base64(key->to_base64());
    EXPECT_NO_THROW(cloned_key->verify(Str(str), *signature));
  }

  std::string str2 = "{\"foo\": \"bar\"}";
  auto signature2 = alice->sign(Str(str2));
  EXPECT_ANY_THROW(key->verify(Str(str2), *signature));
  EXPECT_ANY_THROW(key->verify(Str(str), *signature2));
}
