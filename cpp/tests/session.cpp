#include "../../target/cxxbridge/vodozemac/src/lib.rs.h"
#include "gtest/gtest.h"
#include <boost/json.hpp>
#include "util.hpp"

using namespace rust;
using namespace vodozemac;

struct SessionCreationResult {
  Box<olm::Account> alice;
  Box<olm::Account> bob;
  Box<olm::Session> session;
};

SessionCreationResult create_session() {
  Box<olm::Account> alice = olm::new_account();
  auto bob = olm::new_account();

  bob->generate_one_time_keys(1);

  auto one_time_keys = bob->one_time_keys();
  auto [key_id, one_time_key] = std::move(one_time_keys.front());

  auto identity_key = bob->curve25519_key();

  auto session = alice->create_outbound_session(*identity_key, *one_time_key);

  auto ret = SessionCreationResult{
      std::move(alice),
      std::move(bob),
      std::move(session),
  };

  return ret;
}

TEST(SessionTest, Creation) {
  auto [alice, bob, session] = create_session();

  auto session_id = session->session_id();

  EXPECT_NE(session_id.length(), 0);
}

TEST(SessionTest, IdUniqueness) {
  auto [alice1, bob1, session] = create_session();
  auto [alice2, bob2, session2] = create_session();

  auto session_id = session->session_id();
  auto session2_id = session2->session_id();

  EXPECT_STRNE(session_id.c_str(), session2_id.c_str());
}

TEST(SessionTest, Pickle) {
  auto [alice, bob, session] = create_session();

  auto pickle = session->pickle(PICKLE_KEY);
  auto unpickled = olm::session_from_pickle(pickle, PICKLE_KEY);

  auto session_id = session->session_id();
  auto session2_id = unpickled->session_id();

  EXPECT_STREQ(session_id.c_str(), session2_id.c_str());
}

TEST(SessionTest, PickleFromLibolm) {
  auto [_1, alice] = new_olm_account();
  auto [_2, bob] = new_olm_account();

  auto random = gen_random(olm_account_generate_one_time_keys_random_length(bob, 1));
  check_olm_error(olm_account_generate_one_time_keys(bob, 1, random.data(), random.size()));

  auto one_time_keys = std::string(olm_account_one_time_keys_length(bob), '\0');
  check_olm_error(olm_account_one_time_keys(bob, one_time_keys.data(), one_time_keys.size()));
  auto parsed_keys = boost::json::parse(one_time_keys).as_object().at("curve25519").as_object();
  auto key_id = as_std_string(parsed_keys.begin()->key());
  auto one_time_key = as_std_string(parsed_keys.begin()->value().as_string());

  auto identity_keys = std::string(olm_account_identity_keys_length(alice), '\0');
  check_olm_error(olm_account_identity_keys(alice, identity_keys.data(), identity_keys.size()));
  parsed_keys = boost::json::parse(identity_keys).as_object();
  auto identity_key = String(as_std_string(parsed_keys.at("curve25519").as_string()));

  auto data = std::vector<uint8_t>(olm_session_size());
  auto session = olm_session(data.data());
  random = gen_random(olm_create_outbound_session_random_length(session));
  check_olm_error(olm_create_outbound_session(session, alice, identity_key.data(), identity_key.size(), one_time_key.data(), one_time_key.size(), random.data(), random.size()));

  auto pickle = std::string(olm_pickle_session_length(session), '\0');
  check_olm_error(olm_pickle_session(session, OLM_PICKLE_KEY.data(), OLM_PICKLE_KEY.size(), pickle.data(), pickle.size()));

  auto unpickled = olm::session_from_libolm_pickle(pickle, Slice<const unsigned char>(OLM_PICKLE_KEY.data(), OLM_PICKLE_KEY.size()));

  auto session_id = std::string(olm_session_id_length(session), '\0');
  check_olm_error(olm_session_id(session, session_id.data(), session_id.size()));
  auto session2_id = unpickled->session_id();

  EXPECT_STREQ(session_id.c_str(), session2_id.c_str());
}

TEST(SessionTest, InvalidPickle) {
  EXPECT_ANY_THROW(olm::session_from_pickle("", PICKLE_KEY));
}

TEST(SessionTest, Encryption) {
  auto [alice, bob, session] = create_session();

  auto alice_key = alice->curve25519_key();
  auto plaintext = "It's a secret to everybody";

  auto message = session->encrypt(plaintext);

  auto [bob_session, decrypted] =
      bob->create_inbound_session(*alice_key, *message);

  EXPECT_STREQ(session->session_id().c_str(),
               bob_session->session_id().c_str());

  EXPECT_EQ(std::string(plaintext), as_std_string(decrypted));
}

TEST(SessionTest, InvalidDecryption) {
  auto parts = olm::OlmMessageParts{
      0,
      "",
  };

  EXPECT_ANY_THROW(olm::olm_message_from_parts(parts));
}

TEST(SessionTest, MultipleMessageDecryption) {
  auto [alice, bob, session] = create_session();

  auto alice_key = alice->curve25519_key();
  auto plaintext = "It's a secret to everybody";

  auto message = session->encrypt(plaintext);

  auto [bob_session, decrypted] =
      bob->create_inbound_session(*alice_key, *message);

  EXPECT_STREQ(session->session_id().c_str(),
               bob_session->session_id().c_str());

  EXPECT_EQ(std::string(plaintext), as_std_string(decrypted));

  plaintext = "Grumble grumble";

  message = bob_session->encrypt(plaintext);
  decrypted = session->decrypt(*message);

  EXPECT_EQ(std::string(plaintext), as_std_string(decrypted));
}

TEST(SessionTest, PreKeyMatches) {
  auto [alice, bob, session] = create_session();

  auto alice_key = alice->curve25519_key();
  auto plaintext = "It's a secret to everybody";

  auto message = session->encrypt(plaintext);

  auto [bob_session, decrypted] =
      bob->create_inbound_session(*alice_key, *message);

  plaintext = "Grumble grumble";
  message = session->encrypt(plaintext);

  EXPECT_TRUE(bob_session->session_matches(*message));
}

TEST(SessionTest, PreKeyDoesNotMatch) {
  auto [alice, bob, session] = create_session();
  auto [alice2, bob2, session2] = create_session();

  auto alice_key = alice->curve25519_key();
  auto plaintext = "It's a secret to everybody";

  auto message = session->encrypt(plaintext);

  auto [bob_session, decrypted] =
      bob->create_inbound_session(*alice_key, *message);

  plaintext = "Grumble grumble";
  message = session2->encrypt(plaintext);

  EXPECT_FALSE(bob_session->session_matches(*message));
}

TEST(SessionTest, InvalidOneTimeKey) {
  EXPECT_ANY_THROW(types::curve_key_from_base64(""));
}
