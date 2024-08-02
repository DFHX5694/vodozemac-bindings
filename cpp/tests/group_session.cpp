#include "../../target/cxxbridge/vodozemac/src/lib.rs.h"
#include <olm/outbound_group_session.h>
#include <olm/inbound_group_session.h>
#include "gtest/gtest.h"
#include "util.hpp"

using namespace rust;

struct SessionCreationResult {
  Box<megolm::GroupSession> outbound;
  Box<megolm::InboundGroupSession> inbound;
};

SessionCreationResult create_session() {
  auto outbound = megolm::new_group_session();
  auto session_key = outbound->session_key();
  auto inbound = megolm::new_inbound_group_session(*session_key);

  auto ret = SessionCreationResult{
      std::move(outbound),
      std::move(inbound),
  };

  return ret;
}

struct LibolmSessionCreationResult {
  std::vector<uint8_t> outbound_data;
  OlmOutboundGroupSession *outbound;
  std::vector<uint8_t> inbound_data;
  OlmInboundGroupSession *inbound;
};

LibolmSessionCreationResult create_libolm_session() {
  LibolmSessionCreationResult res{
    std::vector<uint8_t>(olm_outbound_group_session_size()),
    0,
    std::vector<uint8_t>(olm_inbound_group_session_size()),
    0,
  };

  res.outbound = olm_outbound_group_session(res.outbound_data.data());
  auto random = gen_random(olm_init_outbound_group_session_random_length(res.outbound));
  check_olm_error(olm_init_outbound_group_session(res.outbound, random.data(), random.size()));
  auto session_key = std::vector<uint8_t>(olm_outbound_group_session_key_length(res.outbound));
  check_olm_error(olm_outbound_group_session_key(res.outbound, session_key.data(), session_key.size()));

  res.inbound = olm_inbound_group_session(res.inbound_data.data());
  olm_init_inbound_group_session(res.inbound, session_key.data(), session_key.size());
  return res;
}

TEST(GroupSessionTest, Creation) {
  auto [outbound, inbound] = create_session();

  auto outbound_id = outbound->session_id();
  auto inbound_id = inbound->session_id();

  EXPECT_NE(outbound_id.length(), 0);
  EXPECT_STREQ(outbound_id.c_str(), inbound_id.c_str());
}

TEST(GroupSessionTest, MessageIndex) {
  auto [outbound, inbound] = create_session();

  EXPECT_EQ(outbound->message_index(), 0);
  EXPECT_EQ(inbound->first_known_index(), 0);

  outbound->encrypt("Hello");
  auto inbound2 = megolm::new_inbound_group_session(*outbound->session_key());

  EXPECT_EQ(outbound->message_index(), 1);
  EXPECT_EQ(inbound2->first_known_index(), 1);
}

TEST(GroupSessionTest, Pickle) {
  auto session = megolm::new_group_session();

  auto pickle = session->pickle(PICKLE_KEY);
  auto unpickled = megolm::group_session_from_pickle(pickle, PICKLE_KEY);

  ASSERT_STREQ(session->session_id().c_str(), unpickled->session_id().c_str());
  EXPECT_EQ(session->message_index(), unpickled->message_index());
}

TEST(GroupSessionTest, PickleLibolm) {
  auto [_1, outbound, _2, inbound] = create_libolm_session();

  auto pickle = std::string(olm_pickle_outbound_group_session_length(outbound), '\0');
  check_olm_error(olm_pickle_outbound_group_session(outbound, OLM_PICKLE_KEY.data(), OLM_PICKLE_KEY.size(), pickle.data(), pickle.size()));
  auto unpickled = megolm::group_session_from_libolm_pickle(String(pickle), Slice<const unsigned char>(OLM_PICKLE_KEY.data(), OLM_PICKLE_KEY.size()));

  auto session_id = std::vector<uint8_t>(olm_outbound_group_session_id_length(outbound));
  check_olm_error(olm_outbound_group_session_id(outbound, session_id.data(), session_id.size()));

  ASSERT_EQ(as_std_string(session_id), static_cast<std::string>(unpickled->session_id()));

  auto message_index = olm_outbound_group_session_message_index(outbound);

  EXPECT_EQ(message_index, unpickled->message_index());
}

TEST(GroupSessionTest, PickleInbound) {
  auto [outbound, inbound] = create_session();

  auto pickle = inbound->pickle(PICKLE_KEY);
  auto unpickled =
      megolm::inbound_group_session_from_pickle(pickle, PICKLE_KEY);

  ASSERT_STREQ(inbound->session_id().c_str(), unpickled->session_id().c_str());
  EXPECT_EQ(inbound->first_known_index(), unpickled->first_known_index());
}

TEST(GroupSessionTest, PickleInboundLibolm) {
  auto [_1, outbound, _2, inbound] = create_libolm_session();

  auto pickle = std::string(olm_pickle_inbound_group_session_length(inbound), '\0');
  check_olm_error(olm_pickle_inbound_group_session(inbound, OLM_PICKLE_KEY.data(), OLM_PICKLE_KEY.size(), pickle.data(), pickle.size()));

  auto unpickled =
      megolm::inbound_group_session_from_libolm_pickle(pickle, Slice<const unsigned char>(OLM_PICKLE_KEY.data(), OLM_PICKLE_KEY.size()));

  auto session_id = std::vector<uint8_t>(olm_inbound_group_session_id_length(inbound));
  check_olm_error(olm_inbound_group_session_id(inbound, session_id.data(), session_id.size()));

  ASSERT_EQ(as_std_string(session_id), static_cast<std::string>(unpickled->session_id()));

  auto first_known_index = olm_inbound_group_session_first_known_index(inbound);

  EXPECT_EQ(first_known_index, unpickled->first_known_index());
}

TEST(GroupSessionTest, UnpicklingFail) {
  EXPECT_ANY_THROW(megolm::group_session_from_pickle("", PICKLE_KEY));
  EXPECT_ANY_THROW(megolm::inbound_group_session_from_pickle("", PICKLE_KEY));
}

TEST(GroupSessionTest, DecryptionFail) {
  auto [outbound, inbound] = create_session();

  auto outbound2 = megolm::new_group_session();
  auto message = outbound2->encrypt("Hello");

  EXPECT_ANY_THROW(inbound->decrypt(*message));
}

TEST(GroupSessionTest, Encryption) {
  auto [outbound, inbound] = create_session();

  auto plaintext = "It's a secret to everybody";
  auto message = outbound->encrypt(plaintext);
  auto decrypted = inbound->decrypt(*message);

  EXPECT_EQ(as_std_string(decrypted.plaintext), std::string(plaintext));
  EXPECT_EQ(decrypted.message_index, 0);

  plaintext = "Another secret";
  message = outbound->encrypt(plaintext);
  decrypted = inbound->decrypt(*message);

  EXPECT_EQ(as_std_string(decrypted.plaintext), std::string(plaintext));
  EXPECT_EQ(decrypted.message_index, 1);
}

TEST(GroupSessionTest, SessionExport) {
  auto [outbound, inbound] = create_session();
  auto imported = megolm::import_inbound_group_session(*inbound->export_at(0));

  EXPECT_STREQ(outbound->session_id().c_str(), imported->session_id().c_str());

  auto plaintext = "It's a secret to everybody";
  auto message = outbound->encrypt(plaintext);
  auto decrypted = imported->decrypt(*message);

  EXPECT_EQ(as_std_string(decrypted.plaintext), std::string(plaintext));
  EXPECT_EQ(decrypted.message_index, 0);

  plaintext = "Another secret";
  message = outbound->encrypt(plaintext);
  decrypted = imported->decrypt(*message);

  EXPECT_EQ(as_std_string(decrypted.plaintext), std::string(plaintext));
  EXPECT_EQ(decrypted.message_index, 1);
}
