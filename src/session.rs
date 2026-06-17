use super::{
    config::{from_olm_config},
    ffi::{OlmSessionConfig as FfiOlmSessionConfig, SessionKeys},
    Curve25519PublicKey, OlmMessage,
};
use anyhow::Result;

pub struct Session(pub(crate) vodozemac::olm::Session);

impl Session {
    pub fn session_id(&self) -> String {
        self.0.session_id()
    }

    pub fn pickle(&self, pickle_key: &[u8; 32]) -> String {
        self.0.pickle().encrypt(pickle_key)
    }

    pub fn encrypt(&mut self, plaintext: &str) -> Result<Box<OlmMessage>> {
        Ok(OlmMessage(self.0.encrypt(plaintext)?).into())
    }

    pub fn encrypt_bytes(&mut self, plaintext: &[u8]) -> Result<Box<OlmMessage>> {
        Ok(OlmMessage(self.0.encrypt(plaintext)?).into())
    }

    pub fn decrypt(&mut self, message: &OlmMessage) -> Result<Vec<u8>> {
        Ok(self.0.decrypt(&message.0)?)
    }

    pub fn session_keys(&self) -> SessionKeys {
        let session_keys = self.0.session_keys();

        SessionKeys {
            identity_key: Curve25519PublicKey(session_keys.identity_key).into(),
            base_key: Curve25519PublicKey(session_keys.base_key).into(),
            one_time_key: Curve25519PublicKey(session_keys.one_time_key).into(),
        }
    }

    /// Returns true if the given pre-key message was encrypted using the same
    /// session keys as this session. Normal Olm messages always return false.
    pub fn session_matches(&self, message: &OlmMessage) -> bool {
        if let vodozemac::olm::OlmMessage::PreKey(m) = &message.0 {
            self.0.session_keys() == m.session_keys()
        } else {
            false
        }
    }

    pub fn has_received_message(&self) -> bool {
        self.0.has_received_message()
    }

    pub fn session_config(&self) -> FfiOlmSessionConfig {
        from_olm_config(self.0.session_config()).into()
    }
}

pub fn session_keys_session_id(keys: &SessionKeys) -> String {
    let session_keys = vodozemac::olm::SessionKeys {
        identity_key: keys.identity_key.0,
        base_key: keys.base_key.0,
        one_time_key: keys.one_time_key.0,
    };
    session_keys.session_id()
}

pub fn session_from_pickle(pickle: &str, pickle_key: &[u8; 32]) -> Result<Box<Session>> {
    let pickle = vodozemac::olm::SessionPickle::from_encrypted(pickle, pickle_key)?;
    Ok(Session(vodozemac::olm::Session::from_pickle(pickle)).into())
}

pub fn session_from_libolm_pickle(pickle: &str, pickle_key: &[u8]) -> Result<Box<Session>> {
    Ok(Session(vodozemac::olm::Session::from_libolm_pickle(pickle, pickle_key)?).into())
}
