use super::{
    config::{to_olm_config, OlmSessionConfig as ConfigOlmSessionConfig},
    ffi::{
        DehydratedDeviceResult, IdentityKeys, InboundCreationResult, OlmMessageParts, OneTimeKey,
        OlmSessionConfig as FfiOlmSessionConfig,
    },
    Curve25519PublicKey, Ed25519PublicKey, Ed25519Signature, Session,
};
use anyhow::Result;

pub struct OlmMessage(pub(crate) vodozemac::olm::OlmMessage);

impl OlmMessage {
    pub fn to_parts(&self) -> OlmMessageParts {
        let (message_type, ciphertext) = self.0.clone().to_parts();

        OlmMessageParts {
            ciphertext: vodozemac::base64_encode(ciphertext),
            message_type,
        }
    }
}

pub fn olm_message_from_parts(parts: &OlmMessageParts) -> Result<Box<OlmMessage>> {
    Ok(OlmMessage(vodozemac::olm::OlmMessage::from_parts(
        parts.message_type,
        vodozemac::base64_decode(&parts.ciphertext)?.as_slice(),
    )?)
    .into())
}

pub struct OneTimeKeyGenerationResult(vodozemac::olm::OneTimeKeyGenerationResult);

impl OneTimeKeyGenerationResult {
    pub fn created(&self) -> Vec<Curve25519PublicKey> {
        self.0
            .created
            .iter()
            .map(|key| Curve25519PublicKey(*key))
            .collect()
    }

    pub fn removed(&self) -> Vec<Curve25519PublicKey> {
        self.0
            .removed
            .iter()
            .map(|key| Curve25519PublicKey(*key))
            .collect()
    }
}

pub struct Account(vodozemac::olm::Account);

pub fn new_account() -> Box<Account> {
    Account(vodozemac::olm::Account::new()).into()
}

pub fn account_from_pickle(pickle: &str, pickle_key: &[u8; 32]) -> Result<Box<Account>> {
    let pickle = vodozemac::olm::AccountPickle::from_encrypted(pickle, pickle_key)?;
    Ok(Account(vodozemac::olm::Account::from_pickle(pickle)).into())
}

pub fn account_from_libolm_pickle(pickle: &str, pickle_key: &[u8]) -> Result<Box<Account>> {
    Ok(Account(vodozemac::olm::Account::from_libolm_pickle(pickle, pickle_key)?).into())
}

impl From<vodozemac::olm::InboundCreationResult> for InboundCreationResult {
    fn from(value: vodozemac::olm::InboundCreationResult) -> Self {
        Self {
            session: Session(value.session).into(),
            plaintext: value.plaintext,
        }
    }
}

impl Account {
    pub fn identity_keys(&self) -> IdentityKeys {
        let keys = self.0.identity_keys();
        IdentityKeys {
            ed25519: Ed25519PublicKey(keys.ed25519).into(),
            curve25519: Curve25519PublicKey(keys.curve25519).into(),
        }
    }

    pub fn ed25519_key(&self) -> Box<Ed25519PublicKey> {
        Ed25519PublicKey(self.0.ed25519_key()).into()
    }

    pub fn curve25519_key(&self) -> Box<Curve25519PublicKey> {
        Curve25519PublicKey(self.0.curve25519_key()).into()
    }

    pub fn sign(&self, message: &str) -> Box<Ed25519Signature> {
        Ed25519Signature(self.0.sign(message)).into()
    }

    pub fn sign_bytes(&self, message: &[u8]) -> Box<Ed25519Signature> {
        Ed25519Signature(self.0.sign(message)).into()
    }

    pub fn generate_one_time_keys(&mut self, count: usize) -> Box<OneTimeKeyGenerationResult> {
        OneTimeKeyGenerationResult(self.0.generate_one_time_keys(count)).into()
    }

    pub fn stored_one_time_key_count(&self) -> usize {
        self.0.stored_one_time_key_count()
    }

    pub fn one_time_keys(&self) -> Vec<OneTimeKey> {
        self.0
            .one_time_keys()
            .into_iter()
            .map(|(key_id, key)| OneTimeKey {
                key_id: key_id.to_base64(),
                key: Curve25519PublicKey(key).into(),
            })
            .collect()
    }

    pub fn remove_one_time_key(
        &mut self,
        public_key: &Curve25519PublicKey,
    ) -> bool {
        self.0.remove_one_time_key(public_key.0).is_some()
    }

    // cxx bridge does not support Option yet, that's why we are returning a vector
    // of length either 0 or 1.
    pub fn generate_fallback_key(&mut self) -> Vec<Curve25519PublicKey> {
        let mut result = Vec::new();
        if let Some(key) = self.0.generate_fallback_key() {
            result.push(Curve25519PublicKey(key));
        }
        result
    }

    pub fn fallback_key(&self) -> Vec<OneTimeKey> {
        self.0
            .fallback_key()
            .into_iter()
            .map(|(key_id, key)| OneTimeKey {
                key_id: key_id.to_base64(),
                key: Curve25519PublicKey(key).into(),
            })
            .collect()
    }

    pub fn forget_fallback_key(&mut self) -> bool {
        self.0.forget_fallback_key()
    }

    pub fn mark_keys_as_published(&mut self) {
        self.0.mark_keys_as_published()
    }

    pub fn max_number_of_one_time_keys(&self) -> usize {
        self.0.max_number_of_one_time_keys()
    }

    pub fn create_outbound_session(
        &self,
        config: &FfiOlmSessionConfig,
        identity_key: &Curve25519PublicKey,
        one_time_key: &Curve25519PublicKey,
    ) -> Result<Box<Session>> {
        let session = self.0.create_outbound_session(
            to_olm_config(&ConfigOlmSessionConfig::from(config))?,
            identity_key.0,
            one_time_key.0,
        )?;
        Ok(Box::new(Session(session)))
    }

    pub fn create_inbound_session(
        &mut self,
        config: &FfiOlmSessionConfig,
        identity_key: &Curve25519PublicKey,
        message: &OlmMessage,
    ) -> Result<InboundCreationResult> {
        if let vodozemac::olm::OlmMessage::PreKey(m) = &message.0 {
            Ok(self
                .0
                .create_inbound_session(
                    to_olm_config(&ConfigOlmSessionConfig::from(config))?,
                    identity_key.0,
                    m,
                )?
                .into())
        } else {
            anyhow::bail!("Invalid message type, a pre-key message is required")
        }
    }

    pub fn pickle(&self, pickle_key: &[u8; 32]) -> String {
        self.0.pickle().encrypt(pickle_key)
    }

    pub fn to_libolm_pickle(&self, pickle_key: &[u8]) -> Result<String> {
        Ok(self.0.to_libolm_pickle(pickle_key)?)
    }

    pub fn to_dehydrated_device(&self, key: &[u8; 32]) -> Result<DehydratedDeviceResult> {
        let result = self.0.to_dehydrated_device(key)?;
        Ok(DehydratedDeviceResult {
            ciphertext: result.ciphertext,
            nonce: result.nonce,
        })
    }
}

pub fn account_from_dehydrated_device(
    ciphertext: &str,
    nonce: &str,
    key: &[u8; 32],
) -> Result<Box<Account>> {
    Ok(Account(vodozemac::olm::Account::from_dehydrated_device(
        ciphertext, nonce, key,
    )?)
    .into())
}
