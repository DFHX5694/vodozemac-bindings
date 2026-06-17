use super::{
    config::{from_megolm_config, to_megolm_config, MegolmSessionConfig as ConfigMegolmSessionConfig},
    ffi::{
        DecryptedMessage, MegolmSessionConfig as FfiMegolmSessionConfig, SessionOrdering,
    },
};
use anyhow::{anyhow, Result};
use std::error::Error;

pub struct GroupSession(vodozemac::megolm::GroupSession);

pub fn new_group_session(config: &FfiMegolmSessionConfig) -> Result<Box<GroupSession>> {
    Ok(GroupSession(vodozemac::megolm::GroupSession::new(to_megolm_config(
        &ConfigMegolmSessionConfig::from(config),
    )?))
    .into())
}

pub struct MegolmMessage(vodozemac::megolm::MegolmMessage);

pub fn megolm_message_from_base64(message: &str) -> Result<Box<MegolmMessage>> {
    Ok(MegolmMessage(vodozemac::megolm::MegolmMessage::from_base64(message)?).into())
}

impl MegolmMessage {
    pub fn to_base64(&self) -> String {
        self.0.to_base64()
    }
}

pub struct SessionKey(vodozemac::megolm::SessionKey);

pub fn session_key_from_base64(message: &str) -> Result<Box<SessionKey>> {
    Ok(SessionKey(vodozemac::megolm::SessionKey::from_base64(message)?).into())
}

impl SessionKey {
    pub fn to_base64(&self) -> String {
        self.0.to_base64()
    }
}

pub struct ExportedSessionKey(vodozemac::megolm::ExportedSessionKey);

pub fn exported_session_key_from_base64(message: &str) -> Result<Box<ExportedSessionKey>> {
    Ok(ExportedSessionKey(
        vodozemac::megolm::ExportedSessionKey::from_base64(message)?,
    )
    .into())
}

impl ExportedSessionKey {
    pub fn to_base64(&self) -> String {
        self.0.to_base64()
    }
}

impl GroupSession {
    pub fn session_id(&self) -> String {
        self.0.session_id()
    }

    pub fn message_index(&self) -> u32 {
        self.0.message_index()
    }

    pub fn session_key(&self) -> Box<SessionKey> {
        SessionKey(self.0.session_key()).into()
    }

    pub fn session_config(&self) -> FfiMegolmSessionConfig {
        from_megolm_config(self.0.session_config()).into()
    }

    pub fn encrypt(&mut self, plaintext: &str) -> Box<MegolmMessage> {
        MegolmMessage(self.0.encrypt(plaintext)).into()
    }

    pub fn encrypt_bytes(&mut self, plaintext: &[u8]) -> Box<MegolmMessage> {
        MegolmMessage(self.0.encrypt(plaintext)).into()
    }

    pub fn pickle(&self, pickle_key: &[u8; 32]) -> String {
        self.0.pickle().encrypt(pickle_key)
    }
}

pub fn group_session_from_pickle(pickle: &str, pickle_key: &[u8; 32]) -> Result<Box<GroupSession>> {
    let pickle = vodozemac::megolm::GroupSessionPickle::from_encrypted(pickle, pickle_key)?;
    Ok(GroupSession(vodozemac::megolm::GroupSession::from_pickle(pickle)).into())
}

pub fn group_session_from_libolm_pickle(pickle: &str, pickle_key: &[u8]) -> Result<Box<GroupSession>> {
    Ok(GroupSession(vodozemac::megolm::GroupSession::from_libolm_pickle(
        pickle, pickle_key,
    )?)
    .into())
}

pub struct InboundGroupSession(vodozemac::megolm::InboundGroupSession);

#[derive(Debug)]
struct InboundGroupSessionMergeError;
impl Error for InboundGroupSessionMergeError {}
impl std::fmt::Display for InboundGroupSessionMergeError {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        write!(f, "InboundGroupSessionMergeError")
    }
}

pub fn new_inbound_group_session(
    session_key: &SessionKey,
    config: &FfiMegolmSessionConfig,
) -> Result<Box<InboundGroupSession>> {
    Ok(InboundGroupSession(vodozemac::megolm::InboundGroupSession::new(
        &session_key.0,
        to_megolm_config(&ConfigMegolmSessionConfig::from(config))?,
    ))
    .into())
}

pub fn import_inbound_group_session(
    session_key: &ExportedSessionKey,
    config: &FfiMegolmSessionConfig,
) -> Result<Box<InboundGroupSession>> {
    Ok(InboundGroupSession(vodozemac::megolm::InboundGroupSession::import(
        &session_key.0,
        to_megolm_config(&ConfigMegolmSessionConfig::from(config))?,
    ))
    .into())
}

pub fn inbound_group_session_from_group_session(
    session: &GroupSession,
) -> Box<InboundGroupSession> {
    InboundGroupSession(vodozemac::megolm::InboundGroupSession::from(&session.0)).into()
}

pub fn inbound_group_session_from_pickle(
    pickle: &str,
    pickle_key: &[u8; 32],
) -> Result<Box<InboundGroupSession>> {
    let pickle = vodozemac::megolm::InboundGroupSessionPickle::from_encrypted(pickle, pickle_key)?;
    Ok(InboundGroupSession(vodozemac::megolm::InboundGroupSession::from_pickle(pickle)).into())
}

pub fn inbound_group_session_from_libolm_pickle(
    pickle: &str,
    pickle_key: &[u8],
) -> Result<Box<InboundGroupSession>> {
    Ok(InboundGroupSession(vodozemac::megolm::InboundGroupSession::from_libolm_pickle(
        pickle, pickle_key,
    )?)
    .into())
}

fn to_binding_ordering(ordering: vodozemac::megolm::SessionOrdering) -> SessionOrdering {
    match ordering {
        vodozemac::megolm::SessionOrdering::Equal => SessionOrdering::Equal,
        vodozemac::megolm::SessionOrdering::Better => SessionOrdering::Better,
        vodozemac::megolm::SessionOrdering::Worse => SessionOrdering::Worse,
        vodozemac::megolm::SessionOrdering::Unconnected => SessionOrdering::Unconnected,
    }
}

impl InboundGroupSession {
    pub fn session_id(&self) -> String {
        self.0.session_id()
    }

    pub fn first_known_index(&self) -> u32 {
        self.0.first_known_index()
    }

    pub fn connected(&mut self, other: &mut InboundGroupSession) -> bool {
        self.0.connected(&mut other.0)
    }

    pub fn compare(&mut self, other: &mut InboundGroupSession) -> SessionOrdering {
        to_binding_ordering(self.0.compare(&mut other.0))
    }

    pub fn export_at(&mut self, index: u32) -> Result<Box<ExportedSessionKey>> {
        self.0
            .export_at(index)
            .map(ExportedSessionKey)
            .map(Box::new)
            .ok_or_else(|| anyhow!("Unknown message index"))
    }

    pub fn export_at_first_known_index(&self) -> Box<ExportedSessionKey> {
        ExportedSessionKey(self.0.export_at_first_known_index()).into()
    }

    pub fn decrypt(&mut self, message: &MegolmMessage) -> Result<DecryptedMessage> {
        let ret = self.0.decrypt(&message.0)?;

        Ok(DecryptedMessage {
            plaintext: ret.plaintext,
            message_index: ret.message_index,
        })
    }

    pub fn pickle(&self, pickle_key: &[u8; 32]) -> String {
        self.0.pickle().encrypt(pickle_key)
    }

    pub fn merge(&mut self, other: &mut InboundGroupSession) -> Result<Box<InboundGroupSession>> {
        if let Some(session) = self.0.merge(&mut other.0) {
            Ok(InboundGroupSession(session).into())
        } else {
            Err(anyhow::Error::from(InboundGroupSessionMergeError))
        }
    }
}
