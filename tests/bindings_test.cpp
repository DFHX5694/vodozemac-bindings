#include "test_helpers.hpp"

#include <array>
#include <string>

TEST_CASE("library version is non-empty", "[utilities]") {
  REQUIRE_FALSE(vodozemac::library_version().empty());
}

TEST_CASE("base64 round trip", "[utilities]") {
  const std::vector<std::uint8_t> input{'h', 'e', 'l', 'l', 'o', ' ', 'm', 'a', 't', 'r', 'i', 'x'};
  const auto encoded = vodozemac::base64_encode(bytes_slice(input));
  const auto decoded = vodozemac::base64_decode(encoded);
  require_bytes_eq(decoded, input);
}

TEST_CASE("olm session encrypt returns result and config", "[olm]") {
  auto alice = vodozemac::olm::new_account();
  auto bob = vodozemac::olm::new_account();

  bob->generate_one_time_keys(1);
  const auto bob_identity = bob->curve25519_key();
  const auto bob_keys = bob->one_time_keys();
  REQUIRE(bob_keys.size() == 1U);

  const auto config = vodozemac::olm::olm_session_config_v1();
  auto alice_session = alice->create_outbound_session(config, *bob_identity, *bob_keys[0].key);

  const auto message = alice_session->encrypt_bytes(bytes_slice("hello"));
  REQUIRE_FALSE(alice_session->has_received_message());
  REQUIRE_FALSE(message->to_parts().ciphertext.empty());
  REQUIRE(alice_session->session_config().version == 1);
}

TEST_CASE("olm session config v2 is available", "[olm]") {
  REQUIRE(vodozemac::olm::olm_session_config_v2().version == 2);
}

TEST_CASE("account identity keys match individual keys", "[olm]") {
  const auto account = vodozemac::olm::new_account();
  const auto keys = account->identity_keys();

  REQUIRE(keys.ed25519->to_base64() == account->ed25519_key()->to_base64());
  REQUIRE(keys.curve25519->to_base64() == account->curve25519_key()->to_base64());
}

TEST_CASE("account dehydration round trip", "[olm]") {
  const auto account = vodozemac::olm::new_account();
  const auto expected_ed25519 = account->ed25519_key()->to_base64();
  const auto key = dehydration_key();

  const auto dehydrated = account->to_dehydrated_device(key);
  REQUIRE_FALSE(dehydrated.ciphertext.empty());
  REQUIRE_FALSE(dehydrated.nonce.empty());

  auto restored = vodozemac::olm::account_from_dehydrated_device(
      dehydrated.ciphertext, dehydrated.nonce, key);
  REQUIRE(restored->identity_keys().ed25519->to_base64() == expected_ed25519);
}

TEST_CASE("sign and verify bytes", "[olm]") {
  const auto account = vodozemac::olm::new_account();
  const auto message = std::vector<std::uint8_t>{0, 1, 2, 0xff, 0xfe};
  const auto signature = account->sign_bytes(bytes_slice(message));

  account->ed25519_key()->verify_bytes(bytes_slice(message), *signature);
}

TEST_CASE("megolm group session round trip", "[megolm]") {
  const auto config = vodozemac::megolm::megolm_session_config_v1();
  auto outbound = vodozemac::megolm::new_group_session(config);
  const auto session_key = outbound->session_key();
  auto inbound = vodozemac::megolm::new_inbound_group_session(*session_key, config);

  const auto message = outbound->encrypt_bytes(bytes_slice("room message"));
  const auto decrypted = inbound->decrypt(*message);
  require_bytes_eq(decrypted.plaintext, "room message");
}

TEST_CASE("inbound group session from group session", "[megolm]") {
  const auto config = vodozemac::megolm::megolm_session_config_v1();
  auto outbound = vodozemac::megolm::new_group_session(config);
  auto inbound = vodozemac::megolm::inbound_group_session_from_group_session(*outbound);
  REQUIRE(inbound->session_id() == outbound->session_id());
}

TEST_CASE("ecies channel establishment", "[ecies]") {
  auto alice = vodozemac::ecies::new_ecies();
  auto bob = vodozemac::ecies::new_ecies();

  const std::vector<std::uint8_t> plaintext{'q', 'r', ' ', 'l', 'o', 'g', 'i', 'n'};
  auto outbound = alice->establish_outbound_channel(*bob->public_key(), bytes_slice(plaintext));

  auto initial = vodozemac::ecies::ecies_initial_message_decode(outbound.message->encode());
  auto inbound = bob->establish_inbound_channel(*initial);

  require_bytes_eq(inbound.message, plaintext);

  const auto alice_code = vodozemac::ecies::check_code_as_bytes(outbound.ecies->check_code());
  const auto bob_code = vodozemac::ecies::check_code_as_bytes(inbound.ecies->check_code());
  REQUIRE(alice_code[0] == bob_code[0]);
  REQUIRE(alice_code[1] == bob_code[1]);
}

TEST_CASE("ecies encrypt decrypt after channel established", "[ecies]") {
  auto alice = vodozemac::ecies::new_ecies();
  auto bob = vodozemac::ecies::new_ecies();

  auto outbound = alice->establish_outbound_channel(*bob->public_key(), bytes_slice("setup"));
  auto initial = vodozemac::ecies::ecies_initial_message_decode(outbound.message->encode());
  auto inbound = bob->establish_inbound_channel(*initial);

  auto encrypted = inbound.ecies->encrypt(bytes_slice("payload"));
  const auto decrypted = outbound.ecies->decrypt(*encrypted);
  require_bytes_eq(decrypted, "payload");
}

TEST_CASE("sas diffie hellman and mac", "[sas]") {
  auto alice = vodozemac::sas::new_sas();
  auto bob = vodozemac::sas::new_sas();

  auto alice_pk = alice->public_key();
  auto bob_pk = bob->public_key();

  auto alice_sas = alice->diffie_hellman(*bob_pk);
  auto bob_sas = bob->diffie_hellman(*alice_pk);

  REQUIRE(alice_sas->our_public_key()->to_base64() == alice_pk->to_base64());
  REQUIRE(alice_sas->their_public_key()->to_base64() == bob_pk->to_base64());

  const rust::String info = "SAS";
  const auto alice_bytes = alice_sas->bytes(info);
  const auto bob_bytes = bob_sas->bytes(info);
  REQUIRE(alice_bytes->as_bytes() == bob_bytes->as_bytes());

  const auto mac = alice_sas->calculate_mac("content", info);
  bob_sas->verify_mac("content", info, *mac);
}
