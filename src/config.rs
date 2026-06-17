use anyhow::{anyhow, Result};

#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub struct OlmSessionConfig {
    pub version: u8,
}

impl OlmSessionConfig {
    pub const fn version_1() -> Self {
        Self { version: 1 }
    }

    pub const fn version_2() -> Self {
        Self { version: 2 }
    }
}

pub fn to_olm_config(config: &OlmSessionConfig) -> Result<vodozemac::olm::SessionConfig> {
    match config.version {
        1 => Ok(vodozemac::olm::SessionConfig::version_1()),
        2 => Ok(vodozemac::olm::SessionConfig::version_2()),
        version => Err(anyhow!("Unsupported Olm session config version: {version}")),
    }
}

pub fn from_olm_config(config: vodozemac::olm::SessionConfig) -> OlmSessionConfig {
    OlmSessionConfig {
        version: config.version(),
    }
}

#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub struct MegolmSessionConfig {
    pub version: u8,
}

impl MegolmSessionConfig {
    pub const fn version_1() -> Self {
        Self { version: 1 }
    }

    pub const fn version_2() -> Self {
        Self { version: 2 }
    }
}

pub fn to_megolm_config(config: &MegolmSessionConfig) -> Result<vodozemac::megolm::SessionConfig> {
    match config.version {
        1 => Ok(vodozemac::megolm::SessionConfig::version_1()),
        2 => Ok(vodozemac::megolm::SessionConfig::version_2()),
        version => Err(anyhow!("Unsupported Megolm session config version: {version}")),
    }
}

pub fn from_megolm_config(config: vodozemac::megolm::SessionConfig) -> MegolmSessionConfig {
    MegolmSessionConfig {
        version: config.version(),
    }
}
