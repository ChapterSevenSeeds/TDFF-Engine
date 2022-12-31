use clap::Parser;
use itertools::Itertools;
use std::fs::File;
use std::{collections::HashMap, fs};
use walkdir::WalkDir;

use crate::sha_256_adapter::Sha256Adapter;
mod sha_256_adapter;

#[derive(Parser)]
struct Cli {
    /// The input folder.
    #[clap(short, long)]
    i: String,
    /// The output file.
    #[clap(short, long)]
    o: std::path::PathBuf,
}

fn main() {
    let args = Cli::parse();

    let mut files_by_size: HashMap<u64, Vec<walkdir::DirEntry>> = HashMap::new();
    for e in WalkDir::new(args.i).into_iter().filter_map(|e| e.ok()) {
        let metadata = e.metadata().unwrap();
        if metadata.is_file() {
            files_by_size
                .entry(metadata.len())
                .or_insert(Vec::new())
                .push(e);
        } else {
            print!("{}", e.path().display())
        }
    }

    let mut files_by_hash: HashMap<[u8; 32], Vec<&walkdir::DirEntry>> = HashMap::new();
    let size_dups: Vec<&walkdir::DirEntry> = files_by_size
        .iter()
        .filter(|entry| entry.1.len() >= 2)
        .flat_map(|entry| entry.1)
        .collect();

    let mut i: u64 = 0;
    for file in size_dups.iter() {
        print!("{} of {}\r", i, size_dups.len());
        i += 1;
        let mut hasher = Sha256Adapter::new();

        let mut f = File::open(file.path()).unwrap();
        if std::io::copy(&mut f, &mut hasher).is_err() {
            continue;
        }

        files_by_hash
            .entry(hasher.finish())
            .or_insert(Vec::new())
            .push(file.clone());
    }

    if fs::write(
        args.o,
        files_by_hash
            .iter()
            .filter(|entry| entry.1.len() >= 2)
            .map(|entry| {
                std::format!(
                    "GROUP\n{}",
                    entry
                        .1
                        .iter()
                        .map(|x| String::from(x.path().to_str().unwrap()))
                        .intersperse(String::from("\n"))
                        .collect::<String>()
                )
            })
            .intersperse(String::from("\n"))
            .collect::<String>(),
    ).is_err() {
        print!("Unable to create output file");
    }
}
