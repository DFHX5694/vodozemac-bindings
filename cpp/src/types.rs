#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub struct Curve25519PublicKey(pub(crate) vodozemac::Curve25519PublicKey);

impl Curve25519PublicKey {
    pub fn from_base64(key: &str) -> Result<Box<Curve25519PublicKey>, anyhow::Error> {
        Ok(Curve25519PublicKey(vodozemac::Curve25519PublicKey::from_base64(key)?).into())
    }

    #[allow(clippy::wrong_self_convention)]
    pub fn to_base64(&self) -> String {
        self.0.to_base64()
    }
}

pub fn curve_key_from_base64(key: &str) -> Result<Box<Curve25519PublicKey>, anyhow::Error> {
    Curve25519PublicKey::from_base64(key)
}

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub struct Ed25519PublicKey(pub(crate) vodozemac::Ed25519PublicKey);

impl Ed25519PublicKey {
    pub fn from_base64(key: &str) -> Result<Box<Ed25519PublicKey>, anyhow::Error> {
        Ok(Ed25519PublicKey(vodozemac::Ed25519PublicKey::from_base64(key)?).into())
    }

    #[allow(clippy::wrong_self_convention)]
    pub fn to_base64(&self) -> String {
        self.0.to_base64()
    }

    pub fn verify(&self, message: &str, signature: &Ed25519Signature) -> Result<(), anyhow::Error> {
        Ok(self.0.verify(message.as_bytes(), &signature.0)?)
    }
}

pub fn ed25519_key_from_base64(key: &str) -> Result<Box<Ed25519PublicKey>, anyhow::Error> {
    Ed25519PublicKey::from_base64(key)
}

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub struct Ed25519Signature(pub(crate) vodozemac::Ed25519Signature);

impl Ed25519Signature {
    pub fn from_base64(key: &str) -> Result<Box<Ed25519Signature>, anyhow::Error> {
        Ok(Ed25519Signature(vodozemac::Ed25519Signature::from_base64(key)?).into())
    }

    #[allow(clippy::wrong_self_convention)]
    pub fn to_base64(&self) -> String {
        self.0.to_base64()
    }
}

pub fn ed25519_signature_from_base64(key: &str) -> Result<Box<Ed25519Signature>, anyhow::Error> {
    Ed25519Signature::from_base64(key)
}
