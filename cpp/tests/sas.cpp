#include "../../target/cxxbridge/vodozemac/src/lib.rs.h"
#include <catch2/catch_test_macros.hpp>
#include "util.hpp"

using namespace rust;
using namespace vodozemac;

TEST_CASE("Creation", "[SasTest]") {
  auto alice = sas::new_sas();
  auto bob = sas::new_sas();

  auto alice_key = alice->public_key()->to_base64();
  auto bob_key = bob->public_key()->to_base64();

  REQUIRE(alice_key != bob_key);
}

TEST_CASE("SharedSecret", "[SasTest]") {
  auto alice = sas::new_sas();
  auto bob = sas::new_sas();

  auto alice_key = alice->public_key();
  auto bob_key = bob->public_key();

  auto alice_established = REQUIRE_VODOZEMAC_OK(alice->MAYBE_NOEXCEPT(diffie_hellman)(*bob_key));
  auto bob_established = REQUIRE_VODOZEMAC_OK(bob->MAYBE_NOEXCEPT(diffie_hellman)(*alice_key));
}

TEST_CASE("ShortAuthString", "[SasTest]") {
  auto alice = sas::new_sas();
  auto bob = sas::new_sas();

  auto alice_key = alice->public_key();
  auto bob_key = bob->public_key();

  auto alice_established = REQUIRE_VODOZEMAC_OK(alice->MAYBE_NOEXCEPT(diffie_hellman)(*bob_key));
  auto bob_established = REQUIRE_VODOZEMAC_OK(bob->MAYBE_NOEXCEPT(diffie_hellman)(*alice_key));

  auto alice_bytes = alice_established->bytes("");
  auto bob_bytes = bob_established->bytes("");

  REQUIRE(alice_bytes->emoji_indices() == bob_bytes->emoji_indices());
  REQUIRE(alice_bytes->decimals() == bob_bytes->decimals());
}

TEST_CASE("CalculateMac", "[SasTest]") {
  auto alice = sas::new_sas();
  auto bob = sas::new_sas();

  auto alice_key = alice->public_key();
  auto bob_key = bob->public_key();

  auto alice_established = REQUIRE_VODOZEMAC_OK(alice->MAYBE_NOEXCEPT(diffie_hellman)(*bob_key));
  auto bob_established = REQUIRE_VODOZEMAC_OK(bob->MAYBE_NOEXCEPT(diffie_hellman)(*alice_key));

  auto alice_mac = alice_established->calculate_mac("Hello", "world");
  auto bob_mac = bob_established->calculate_mac("Hello", "world");

  REQUIRE(alice_mac->to_base64() == bob_mac->to_base64());

  REQUIRE_VODOZEMAC_OK(alice_established->MAYBE_NOEXCEPT(verify_mac)("Hello", "world", *bob_mac));
  REQUIRE_VODOZEMAC_OK(bob_established->MAYBE_NOEXCEPT(verify_mac)("Hello", "world", *alice_mac));
  REQUIRE_VODOZEMAC_ERROR(bob_established->MAYBE_NOEXCEPT(verify_mac)("", "", *alice_mac));
}
