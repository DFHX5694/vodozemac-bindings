#include "../../target/cxxbridge/vodozemac/src/lib.rs.h"
#include <olm/outbound_group_session.h>
#include <olm/inbound_group_session.h>
#include <catch2/catch_test_macros.hpp>
#include "util.hpp"

using namespace rust;
using namespace vodozemac;

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

TEST_CASE("Creation", "[GroupSessionTest]") {
  auto [outbound, inbound] = create_session();

  auto outbound_id = outbound->session_id();
  auto inbound_id = inbound->session_id();

  REQUIRE(outbound_id.length() != 0);
  REQUIRE(outbound_id == inbound_id);
}

TEST_CASE("MessageIndex", "[GroupSessionTest]") {
  auto [outbound, inbound] = create_session();

  REQUIRE(outbound->message_index() == 0);
  REQUIRE(inbound->first_known_index() == 0);

  outbound->encrypt("Hello");
  auto inbound2 = megolm::new_inbound_group_session(*outbound->session_key());

  REQUIRE(outbound->message_index() == 1);
  REQUIRE(inbound2->first_known_index() == 1);
}

TEST_CASE("Pickle", "[GroupSessionTest]") {
  auto session = megolm::new_group_session();

  auto pickle = session->pickle(PICKLE_KEY);
  auto unpickled = REQUIRE_VODOZEMAC_OK(MAYBE_NOEXCEPT(megolm::group_session_from_pickle)(pickle, PICKLE_KEY));

  REQUIRE(session->session_id() == unpickled->session_id());
  REQUIRE(session->message_index() == unpickled->message_index());
}

TEST_CASE("PickleLibolm", "[GroupSessionTest]") {
  auto [_1, outbound, _2, inbound] = create_libolm_session();

  auto pickle = std::string(olm_pickle_outbound_group_session_length(outbound), '\0');
  check_olm_error(olm_pickle_outbound_group_session(outbound, OLM_PICKLE_KEY.data(), OLM_PICKLE_KEY.size(), pickle.data(), pickle.size()));
  auto unpickled = REQUIRE_VODOZEMAC_OK(
    MAYBE_NOEXCEPT(megolm::group_session_from_libolm_pickle)
    (String(pickle), Slice<const unsigned char>(OLM_PICKLE_KEY.data(), OLM_PICKLE_KEY.size())));

  auto session_id = std::vector<uint8_t>(olm_outbound_group_session_id_length(outbound));
  check_olm_error(olm_outbound_group_session_id(outbound, session_id.data(), session_id.size()));

  REQUIRE(as_std_string(session_id) == as_std_string(unpickled->session_id()));

  auto message_index = olm_outbound_group_session_message_index(outbound);

  REQUIRE(message_index == unpickled->message_index());
}

TEST_CASE("PickleInbound", "[GroupSessionTest]") {
  auto [outbound, inbound] = create_session();

  auto pickle = inbound->pickle(PICKLE_KEY);
  auto unpickled = REQUIRE_VODOZEMAC_OK(
      MAYBE_NOEXCEPT(megolm::inbound_group_session_from_pickle)
      (pickle, PICKLE_KEY));

  REQUIRE(inbound->session_id() == unpickled->session_id());
  REQUIRE(inbound->first_known_index() == unpickled->first_known_index());
}

TEST_CASE("PickleInboundLibolm", "[GroupSessionTest]") {
  auto [_1, outbound, _2, inbound] = create_libolm_session();

  auto pickle = std::string(olm_pickle_inbound_group_session_length(inbound), '\0');
  check_olm_error(olm_pickle_inbound_group_session(inbound, OLM_PICKLE_KEY.data(), OLM_PICKLE_KEY.size(), pickle.data(), pickle.size()));

  auto unpickled = REQUIRE_VODOZEMAC_OK(
      MAYBE_NOEXCEPT(megolm::inbound_group_session_from_libolm_pickle)(pickle, Slice<const unsigned char>(OLM_PICKLE_KEY.data(), OLM_PICKLE_KEY.size())));

  auto session_id = std::vector<uint8_t>(olm_inbound_group_session_id_length(inbound));
  check_olm_error(olm_inbound_group_session_id(inbound, session_id.data(), session_id.size()));

  REQUIRE(as_std_string(session_id) == static_cast<std::string>(unpickled->session_id()));

  auto first_known_index = olm_inbound_group_session_first_known_index(inbound);

  REQUIRE(first_known_index == unpickled->first_known_index());
}

TEST_CASE("UnpicklingFail", "[GroupSessionTest]") {
  REQUIRE_VODOZEMAC_ERROR(MAYBE_NOEXCEPT(megolm::group_session_from_pickle)("", PICKLE_KEY));
  REQUIRE_VODOZEMAC_ERROR(MAYBE_NOEXCEPT(megolm::inbound_group_session_from_pickle)("", PICKLE_KEY));
}

TEST_CASE("DecryptionFail", "[GroupSessionTest]") {
  auto res = create_session();
  // Because capturing structural bindings are c++20 and later,
  // we write it as such here
  auto &inbound = res.inbound;

  auto outbound2 = megolm::new_group_session();
  auto message = outbound2->encrypt("Hello");

  REQUIRE_VODOZEMAC_ERROR(inbound->MAYBE_NOEXCEPT(decrypt)(*message));
}

TEST_CASE("Encryption", "[GroupSessionTest]") {
  auto res = create_session();
  // Because capturing structural bindings are c++20 and later,
  // we write it as such here
  auto &outbound = res.outbound;
  auto &inbound = res.inbound;

  auto plaintext = "It's a secret to everybody";
  auto message = outbound->encrypt(plaintext);
  auto decrypted = REQUIRE_VODOZEMAC_OK(inbound->MAYBE_NOEXCEPT(decrypt)(*message));

  REQUIRE(as_std_string(decrypted.plaintext) == as_std_string(plaintext));
  REQUIRE(decrypted.message_index == 0);

  plaintext = "Another secret";
  message = outbound->encrypt(plaintext);
  decrypted = REQUIRE_VODOZEMAC_OK(inbound->MAYBE_NOEXCEPT(decrypt)(*message));

  REQUIRE(as_std_string(decrypted.plaintext) == std::string(plaintext));
  REQUIRE(decrypted.message_index == 1);
}

TEST_CASE("SessionExport", "[GroupSessionTest]") {
  auto res = create_session();
  // Because capturing structural bindings are c++20 and later,
  // we write it as such here
  auto &outbound = res.outbound;
  auto &inbound = res.inbound;
  auto exported = REQUIRE_VODOZEMAC_OK(inbound->MAYBE_NOEXCEPT(export_at)(0));
  auto imported = megolm::import_inbound_group_session(*exported);

  REQUIRE(outbound->session_id() == imported->session_id());

  auto plaintext = "It's a secret to everybody";
  auto message = outbound->encrypt(plaintext);
  auto decrypted = REQUIRE_VODOZEMAC_OK(imported->MAYBE_NOEXCEPT(decrypt)(*message));

  REQUIRE(as_std_string(decrypted.plaintext) == as_std_string(plaintext));
  REQUIRE(decrypted.message_index == 0);

  plaintext = "Another secret";
  message = outbound->encrypt(plaintext);
  decrypted = REQUIRE_VODOZEMAC_OK(imported->MAYBE_NOEXCEPT(decrypt)(*message));

  REQUIRE(as_std_string(decrypted.plaintext) == as_std_string(plaintext));
  REQUIRE(decrypted.message_index == 1);
}
