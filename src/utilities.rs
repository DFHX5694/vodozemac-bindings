pub fn base64_encode(input: &[u8]) -> String {
    vodozemac::base64_encode(input)
}

pub fn base64_decode(input: &str) -> Result<Vec<u8>, anyhow::Error> {
    Ok(vodozemac::base64_decode(input)?)
}

pub fn library_version() -> String {
    vodozemac::VERSION.to_owned()
}
