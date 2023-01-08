use openssl::sha;
use std::io::prelude::*;

pub struct Sha256Adapter {
    pub hasher: sha::Sha256
}

impl Sha256Adapter {
    pub fn new() -> Sha256Adapter {
        return Sha256Adapter {
            hasher: sha::Sha256::new()
        }
    }

    pub fn finish(self) -> [u8; 32] {
        self.hasher.finish()
    }
}

impl Write for Sha256Adapter {
    fn write(&mut self, buf: &[u8]) -> std::io::Result<usize> {
        self.hasher.update(buf);
        return std::io::Result::Ok(buf.len());
    }

    fn flush(&mut self) -> std::io::Result<()> {
        std::io::Result::Ok(())
    }
}