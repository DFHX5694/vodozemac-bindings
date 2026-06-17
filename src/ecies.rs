use super::Curve25519PublicKey;
use crate::ffi::{CheckCode, EciesInboundCreationResult, EciesOutboundCreationResult};
use anyhow::Result;

pub struct Ecies {
    inner: Option<vodozemac::ecies::Ecies>,
}

pub fn new_ecies() -> Box<Ecies> {
    Box::new(Ecies {
        inner: Some(vodozemac::ecies::Ecies::new()),
    })
}

pub fn ecies_with_info(info: &str) -> Box<Ecies> {
    Box::new(Ecies {
        inner: Some(vodozemac::ecies::Ecies::with_info(info)),
    })
}

impl Ecies {
    pub fn public_key(&self) -> Box<Curve25519PublicKey> {
        Curve25519PublicKey(self.inner.as_ref().expect("Ecies consumed").public_key()).into()
    }

    pub fn establish_outbound_channel(
        &mut self,
        their_public_key: &Curve25519PublicKey,
        initial_plaintext: &[u8],
    ) -> Result<EciesOutboundCreationResult> {
        let ecies = self
            .inner
            .take()
            .ok_or_else(|| anyhow::anyhow!("The Ecies object has already been used"))?;
        let result = ecies.establish_outbound_channel(their_public_key.0, initial_plaintext)?;

        Ok(EciesOutboundCreationResult {
            ecies: Box::new(EstablishedEcies { inner: result.ecies }),
            message: Box::new(EciesInitialMessage {
                inner: result.message,
            }),
        })
    }

    pub fn establish_inbound_channel(
        &mut self,
        message: &EciesInitialMessage,
    ) -> Result<EciesInboundCreationResult> {
        let ecies = self
            .inner
            .take()
            .ok_or_else(|| anyhow::anyhow!("The Ecies object has already been used"))?;
        let result = ecies.establish_inbound_channel(&message.inner)?;

        Ok(EciesInboundCreationResult {
            ecies: Box::new(EstablishedEcies { inner: result.ecies }),
            message: result.message,
        })
    }
}

pub struct EstablishedEcies {
    pub(crate) inner: vodozemac::ecies::EstablishedEcies,
}

impl EstablishedEcies {
    pub fn public_key(&self) -> Box<Curve25519PublicKey> {
        Curve25519PublicKey(self.inner.public_key()).into()
    }

    pub fn check_code(&self) -> CheckCode {
        let code = self.inner.check_code();
        CheckCode {
            bytes: *code.as_bytes(),
        }
    }

    pub fn encrypt(&mut self, plaintext: &[u8]) -> Box<EciesMessage> {
        EciesMessage {
            inner: self.inner.encrypt(plaintext),
        }
        .into()
    }

    pub fn decrypt(&mut self, message: &EciesMessage) -> Result<Vec<u8>> {
        Ok(self.inner.decrypt(&message.inner)?)
    }
}

pub fn check_code_as_bytes(code: &CheckCode) -> [u8; 2] {
    code.bytes
}

pub fn check_code_to_digit(code: &CheckCode) -> u8 {
    let first = (code.bytes[0] % 10) * 10;
    let second = code.bytes[1] % 10;
    first + second
}

pub struct EciesInitialMessage {
    pub(crate) inner: vodozemac::ecies::InitialMessage,
}

impl EciesInitialMessage {
    pub fn encode(&self) -> String {
        self.inner.encode()
    }

    pub fn public_key(&self) -> Box<Curve25519PublicKey> {
        Curve25519PublicKey(self.inner.public_key).into()
    }

    pub fn ciphertext(&self) -> Vec<u8> {
        self.inner.ciphertext.clone()
    }
}

pub fn ecies_initial_message_decode(message: &str) -> Result<Box<EciesInitialMessage>> {
    Ok(Box::new(EciesInitialMessage {
        inner: vodozemac::ecies::InitialMessage::decode(message)?,
    }))
}

pub struct EciesMessage {
    pub(crate) inner: vodozemac::ecies::Message,
}

impl EciesMessage {
    pub fn encode(&self) -> String {
        self.inner.encode()
    }

    pub fn ciphertext(&self) -> Vec<u8> {
        self.inner.ciphertext.clone()
    }
}

pub fn ecies_message_decode(message: &str) -> Result<Box<EciesMessage>> {
    Ok(Box::new(EciesMessage {
        inner: vodozemac::ecies::Message::decode(message)?,
    }))
}
