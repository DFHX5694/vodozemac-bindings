pub mod account;
pub mod config;
pub mod ecies;
pub mod group_sessions;
pub mod sas;
pub mod session;
pub mod types;
pub mod utilities;

use account::{
    account_from_dehydrated_device, account_from_libolm_pickle, account_from_pickle, new_account,
    olm_message_from_parts, Account, OlmMessage, OneTimeKeyGenerationResult,
};
use ecies::{
    check_code_as_bytes, check_code_to_digit, ecies_initial_message_decode, ecies_message_decode,
    ecies_with_info, new_ecies, Ecies, EciesInitialMessage, EciesMessage, EstablishedEcies,
};
use group_sessions::{
    exported_session_key_from_base64, group_session_from_libolm_pickle, group_session_from_pickle,
    import_inbound_group_session, inbound_group_session_from_group_session,
    inbound_group_session_from_libolm_pickle, inbound_group_session_from_pickle,
    megolm_message_from_base64, new_group_session, new_inbound_group_session,
    session_key_from_base64, ExportedSessionKey, GroupSession, InboundGroupSession, MegolmMessage,
    SessionKey,
};
use sas::{mac_from_base64, new_sas, EstablishedSas, Mac, Sas, SasBytes};
use session::{
    session_from_libolm_pickle, session_from_pickle, session_keys_session_id, Session,
};
use types::{
    curve_key_from_base64, ed25519_key_from_base64, ed25519_signature_from_base64,
    Curve25519PublicKey, Ed25519PublicKey, Ed25519Signature,
};
use utilities::{base64_decode, base64_encode, library_version};

#[cxx::bridge]
pub mod ffi {
    #[namespace = "vodozemac"]
    extern "Rust" {
        fn base64_encode(input: &[u8]) -> String;
        fn base64_decode(input: &str) -> Result<Vec<u8>>;
        fn library_version() -> String;
    }

    #[namespace = "vodozemac::olm"]
    struct OlmSessionConfig {
        version: u8,
    }

    #[namespace = "vodozemac::olm"]
    struct OlmMessageParts {
        message_type: usize,
        ciphertext: String,
    }

    #[namespace = "vodozemac::olm"]
    pub struct InboundCreationResult {
        pub session: Box<Session>,
        pub plaintext: Vec<u8>,
    }

    #[namespace = "vodozemac::olm"]
    struct IdentityKeys {
        ed25519: Box<Ed25519PublicKey>,
        curve25519: Box<Curve25519PublicKey>,
    }

    #[namespace = "vodozemac::olm"]
    struct DehydratedDeviceResult {
        ciphertext: String,
        nonce: String,
    }

    #[namespace = "vodozemac::olm"]
    struct OneTimeKey {
        key_id: String,
        key: Box<Curve25519PublicKey>,
    }

    #[namespace = "vodozemac::olm"]
    #[derive(PartialEq, Eq)]
    struct SessionKeys {
        identity_key: Box<Curve25519PublicKey>,
        base_key: Box<Curve25519PublicKey>,
        one_time_key: Box<Curve25519PublicKey>,
    }

    #[namespace = "vodozemac::megolm"]
    struct MegolmSessionConfig {
        version: u8,
    }

    #[namespace = "vodozemac::megolm"]
    enum SessionOrdering {
        Equal,
        Better,
        Worse,
        Unconnected,
    }

    #[namespace = "vodozemac::megolm"]
    struct DecryptedMessage {
        plaintext: Vec<u8>,
        message_index: u32,
    }

    #[namespace = "vodozemac::ecies"]
    struct CheckCode {
        bytes: [u8; 2],
    }

    #[namespace = "vodozemac::ecies"]
    pub struct EciesInboundCreationResult {
        pub ecies: Box<EstablishedEcies>,
        pub message: Vec<u8>,
    }

    #[namespace = "vodozemac::ecies"]
    pub struct EciesOutboundCreationResult {
        pub ecies: Box<EstablishedEcies>,
        pub message: Box<EciesInitialMessage>,
    }

    #[namespace = "vodozemac::types"]
    extern "Rust" {
        type Curve25519PublicKey;
        fn curve_key_from_base64(key: &str) -> Result<Box<Curve25519PublicKey>>;
        fn to_base64(self: &Curve25519PublicKey) -> String;
        type Ed25519PublicKey;
        fn ed25519_key_from_base64(key: &str) -> Result<Box<Ed25519PublicKey>>;
        fn to_base64(self: &Ed25519PublicKey) -> String;
        fn verify(self: &Ed25519PublicKey, message: &str, signature: &Ed25519Signature) -> Result<()>;
        fn verify_bytes(
            self: &Ed25519PublicKey,
            message: &[u8],
            signature: &Ed25519Signature,
        ) -> Result<()>;
        type Ed25519Signature;
        fn ed25519_signature_from_base64(key: &str) -> Result<Box<Ed25519Signature>>;
        fn to_base64(self: &Ed25519Signature) -> String;
    }

    #[namespace = "vodozemac::olm"]
    extern "Rust" {
        fn olm_session_config_v1() -> OlmSessionConfig;
        fn olm_session_config_v2() -> OlmSessionConfig;

        type Account;
        fn new_account() -> Box<Account>;
        fn identity_keys(self: &Account) -> IdentityKeys;
        fn ed25519_key(self: &Account) -> Box<Ed25519PublicKey>;
        fn curve25519_key(self: &Account) -> Box<Curve25519PublicKey>;
        fn sign(self: &Account, message: &str) -> Box<Ed25519Signature>;
        fn sign_bytes(self: &Account, message: &[u8]) -> Box<Ed25519Signature>;
        fn generate_one_time_keys(self: &mut Account, count: usize) -> Box<OneTimeKeyGenerationResult>;
        fn stored_one_time_key_count(self: &Account) -> usize;
        fn one_time_keys(self: &Account) -> Vec<OneTimeKey>;
        fn remove_one_time_key(self: &mut Account, public_key: &Curve25519PublicKey) -> bool;
        fn generate_fallback_key(self: &mut Account) -> Vec<Curve25519PublicKey>;
        fn fallback_key(self: &Account) -> Vec<OneTimeKey>;
        fn forget_fallback_key(self: &mut Account) -> bool;
        fn mark_keys_as_published(self: &mut Account);
        fn max_number_of_one_time_keys(self: &Account) -> usize;
        fn account_from_pickle(pickle: &str, pickle_key: &[u8; 32]) -> Result<Box<Account>>;
        fn account_from_libolm_pickle(pickle: &str, pickle_key: &[u8]) -> Result<Box<Account>>;
        fn account_from_dehydrated_device(
            ciphertext: &str,
            nonce: &str,
            key: &[u8; 32],
        ) -> Result<Box<Account>>;
        fn pickle(self: &Account, pickle_key: &[u8; 32]) -> String;
        fn to_libolm_pickle(self: &Account, pickle_key: &[u8]) -> Result<String>;
        fn to_dehydrated_device(self: &Account, key: &[u8; 32]) -> Result<DehydratedDeviceResult>;
        fn create_outbound_session(
            self: &Account,
            config: &OlmSessionConfig,
            identity_key: &Curve25519PublicKey,
            one_time_key: &Curve25519PublicKey,
        ) -> Result<Box<Session>>;
        fn create_inbound_session(
            self: &mut Account,
            config: &OlmSessionConfig,
            identity_key: &Curve25519PublicKey,
            message: &OlmMessage,
        ) -> Result<InboundCreationResult>;

        type OneTimeKeyGenerationResult;
        fn created(self: &OneTimeKeyGenerationResult) -> Vec<Curve25519PublicKey>;
        fn removed(self: &OneTimeKeyGenerationResult) -> Vec<Curve25519PublicKey>;

        type Session;
        fn session_id(self: &Session) -> String;
        fn session_keys(self: &Session) -> SessionKeys;
        fn session_keys_session_id(keys: &SessionKeys) -> String;
        fn session_matches(self: &Session, message: &OlmMessage) -> bool;
        fn has_received_message(self: &Session) -> bool;
        fn session_config(self: &Session) -> OlmSessionConfig;
        fn encrypt(self: &mut Session, plaintext: &str) -> Result<Box<OlmMessage>>;
        fn encrypt_bytes(self: &mut Session, plaintext: &[u8]) -> Result<Box<OlmMessage>>;
        fn decrypt(self: &mut Session, message: &OlmMessage) -> Result<Vec<u8>>;
        fn session_from_pickle(pickle: &str, pickle_key: &[u8; 32]) -> Result<Box<Session>>;
        fn session_from_libolm_pickle(pickle: &str, pickle_key: &[u8]) -> Result<Box<Session>>;
        fn pickle(self: &Session, pickle_key: &[u8; 32]) -> String;

        type OlmMessage;
        fn to_parts(self: &OlmMessage) -> OlmMessageParts;
        fn olm_message_from_parts(parts: &OlmMessageParts) -> Result<Box<OlmMessage>>;
    }

    #[namespace = "vodozemac::megolm"]
    extern "Rust" {
        fn megolm_session_config_v1() -> MegolmSessionConfig;
        fn megolm_session_config_v2() -> MegolmSessionConfig;

        type MegolmMessage;
        fn megolm_message_from_base64(message: &str) -> Result<Box<MegolmMessage>>;
        fn to_base64(self: &MegolmMessage) -> String;

        type SessionKey;
        fn session_key_from_base64(key: &str) -> Result<Box<SessionKey>>;
        fn to_base64(self: &SessionKey) -> String;

        type ExportedSessionKey;
        fn exported_session_key_from_base64(key: &str) -> Result<Box<ExportedSessionKey>>;
        fn to_base64(self: &ExportedSessionKey) -> String;

        type GroupSession;
        fn new_group_session(config: &MegolmSessionConfig) -> Result<Box<GroupSession>>;
        fn encrypt(self: &mut GroupSession, plaintext: &str) -> Box<MegolmMessage>;
        fn encrypt_bytes(self: &mut GroupSession, plaintext: &[u8]) -> Box<MegolmMessage>;
        fn session_id(self: &GroupSession) -> String;
        fn session_key(self: &GroupSession) -> Box<SessionKey>;
        fn message_index(self: &GroupSession) -> u32;
        fn session_config(self: &GroupSession) -> MegolmSessionConfig;
        fn pickle(self: &GroupSession, pickle_key: &[u8; 32]) -> String;
        fn group_session_from_pickle(
            pickle: &str,
            pickle_key: &[u8; 32],
        ) -> Result<Box<GroupSession>>;
        fn group_session_from_libolm_pickle(
            pickle: &str,
            pickle_key: &[u8],
        ) -> Result<Box<GroupSession>>;

        type InboundGroupSession;
        fn new_inbound_group_session(
            session_key: &SessionKey,
            config: &MegolmSessionConfig,
        ) -> Result<Box<InboundGroupSession>>;
        fn import_inbound_group_session(
            session_key: &ExportedSessionKey,
            config: &MegolmSessionConfig,
        ) -> Result<Box<InboundGroupSession>>;
        fn inbound_group_session_from_group_session(
            session: &GroupSession,
        ) -> Box<InboundGroupSession>;
        fn decrypt(
            self: &mut InboundGroupSession,
            message: &MegolmMessage,
        ) -> Result<DecryptedMessage>;
        fn session_id(self: &InboundGroupSession) -> String;
        fn first_known_index(self: &InboundGroupSession) -> u32;
        fn connected(self: &mut InboundGroupSession, other: &mut InboundGroupSession) -> bool;
        fn compare(
            self: &mut InboundGroupSession,
            other: &mut InboundGroupSession,
        ) -> SessionOrdering;
        fn export_at(
            self: &mut InboundGroupSession,
            message_index: u32,
        ) -> Result<Box<ExportedSessionKey>>;
        fn export_at_first_known_index(self: &InboundGroupSession) -> Box<ExportedSessionKey>;
        fn pickle(self: &InboundGroupSession, pickle_key: &[u8; 32]) -> String;
        fn merge(
            self: &mut InboundGroupSession,
            other: &mut InboundGroupSession,
        ) -> Result<Box<InboundGroupSession>>;
        fn inbound_group_session_from_pickle(
            pickle: &str,
            pickle_key: &[u8; 32],
        ) -> Result<Box<InboundGroupSession>>;
        fn inbound_group_session_from_libolm_pickle(
            pickle: &str,
            pickle_key: &[u8],
        ) -> Result<Box<InboundGroupSession>>;
    }

    #[namespace = "vodozemac::sas"]
    extern "Rust" {
        type Mac;
        fn mac_from_base64(mac: &str) -> Result<Box<Mac>>;
        fn to_base64(self: &Mac) -> String;
        type Sas;
        fn new_sas() -> Box<Sas>;
        fn public_key(self: &Sas) -> Box<Curve25519PublicKey>;
        fn diffie_hellman(
            self: &mut Sas,
            other_public_key: &Curve25519PublicKey,
        ) -> Result<Box<EstablishedSas>>;

        type EstablishedSas;
        fn our_public_key(self: &EstablishedSas) -> Box<Curve25519PublicKey>;
        fn their_public_key(self: &EstablishedSas) -> Box<Curve25519PublicKey>;
        fn bytes(self: &EstablishedSas, info: &str) -> Box<SasBytes>;
        fn bytes_raw(self: &EstablishedSas, info: &str, count: usize) -> Result<Vec<u8>>;
        fn calculate_mac(self: &EstablishedSas, input: &str, info: &str) -> Box<Mac>;
        fn verify_mac(self: &EstablishedSas, input: &str, info: &str, mac: &Mac) -> Result<()>;

        type SasBytes;
        fn emoji_indices(self: &SasBytes) -> [u8; 7];
        fn decimals(self: &SasBytes) -> [u16; 3];
        fn as_bytes(self: &SasBytes) -> [u8; 6];
    }

    #[namespace = "vodozemac::ecies"]
    extern "Rust" {
        type Ecies;
        fn new_ecies() -> Box<Ecies>;
        fn ecies_with_info(info: &str) -> Box<Ecies>;
        fn public_key(self: &Ecies) -> Box<Curve25519PublicKey>;
        fn establish_outbound_channel(
            self: &mut Ecies,
            their_public_key: &Curve25519PublicKey,
            initial_plaintext: &[u8],
        ) -> Result<EciesOutboundCreationResult>;
        fn establish_inbound_channel(
            self: &mut Ecies,
            message: &EciesInitialMessage,
        ) -> Result<EciesInboundCreationResult>;

        type EstablishedEcies;
        fn public_key(self: &EstablishedEcies) -> Box<Curve25519PublicKey>;
        fn check_code(self: &EstablishedEcies) -> CheckCode;
        fn check_code_as_bytes(code: &CheckCode) -> [u8; 2];
        fn check_code_to_digit(code: &CheckCode) -> u8;
        fn encrypt(self: &mut EstablishedEcies, plaintext: &[u8]) -> Box<EciesMessage>;
        fn decrypt(self: &mut EstablishedEcies, message: &EciesMessage) -> Result<Vec<u8>>;

        type EciesInitialMessage;
        fn ecies_initial_message_decode(message: &str) -> Result<Box<EciesInitialMessage>>;
        fn encode(self: &EciesInitialMessage) -> String;
        fn public_key(self: &EciesInitialMessage) -> Box<Curve25519PublicKey>;
        fn ciphertext(self: &EciesInitialMessage) -> Vec<u8>;

        type EciesMessage;
        fn ecies_message_decode(message: &str) -> Result<Box<EciesMessage>>;
        fn encode(self: &EciesMessage) -> String;
        fn ciphertext(self: &EciesMessage) -> Vec<u8>;
    }
}

impl From<&ffi::OlmSessionConfig> for config::OlmSessionConfig {
    fn from(value: &ffi::OlmSessionConfig) -> Self {
        config::OlmSessionConfig {
            version: value.version,
        }
    }
}

impl From<&ffi::MegolmSessionConfig> for config::MegolmSessionConfig {
    fn from(value: &ffi::MegolmSessionConfig) -> Self {
        config::MegolmSessionConfig {
            version: value.version,
        }
    }
}

impl From<config::OlmSessionConfig> for ffi::OlmSessionConfig {
    fn from(value: config::OlmSessionConfig) -> Self {
        Self {
            version: value.version,
        }
    }
}

impl From<config::MegolmSessionConfig> for ffi::MegolmSessionConfig {
    fn from(value: config::MegolmSessionConfig) -> Self {
        Self {
            version: value.version,
        }
    }
}

fn olm_session_config_v1() -> ffi::OlmSessionConfig {
    config::OlmSessionConfig::version_1().into()
}

fn olm_session_config_v2() -> ffi::OlmSessionConfig {
    config::OlmSessionConfig::version_2().into()
}

fn megolm_session_config_v1() -> ffi::MegolmSessionConfig {
    config::MegolmSessionConfig::version_1().into()
}

fn megolm_session_config_v2() -> ffi::MegolmSessionConfig {
    config::MegolmSessionConfig::version_2().into()
}
