use clap::Parser;
use indicatif::{HumanBytes, HumanCount, ProgressBar, ProgressStyle};
use itertools::Itertools;
use json_writer::JSONObjectWriter;
use parse_size::parse_size;
use rayon::prelude::{IntoParallelRefIterator, ParallelIterator};
use regex::Regex;
use std::collections::{HashMap, HashSet, VecDeque};
use std::fs;
use std::path::PathBuf;

use crate::sha_256_adapter::Sha256Adapter;
mod sha_256_adapter;

#[derive(Parser)]
struct Cli {
    /// The input folder.
    #[clap(short, long)]
    path: std::path::PathBuf,
    /// The output file.
    #[clap(short, long)]
    output: std::path::PathBuf,
    /// List of regular expression patterns to exclude in search.
    #[clap(short, long)]
    exclude: Option<String>,
    /// List of regular expression patterns to include in search.
    /// All other paths will be excluded.
    #[clap(short, long)]
    include: Option<String>,
    /// The minimum filesize to include in the search.
    #[clap(long, default_value = "0 B")]
    minsize: Option<String>,
    /// The maximum filesize to include in the search.
    #[clap(long, default_value = "18.446744073709551615 EB")]
    maxsize: Option<String>,
}

fn parse_array(arg: Option<String>, delimiter: char, escape: char) -> Vec<String> {
    if arg.is_none() {
        return Vec::new();
    }

    let mut temp: String = String::new();
    let mut result: Vec<String> = Vec::new();

    let mut in_escape: bool = false;
    for c in arg.unwrap().chars() {
        if c == escape {
            in_escape = !in_escape;
            continue;
        }

        if c == delimiter && !in_escape {
            if !temp.is_empty() {
                result.push(temp.clone());
                temp.clear();
            }

            continue;
        }

        temp.push(c);
    }

    if !temp.is_empty() {
        result.push(temp);
    }

    return result;
}

fn main() {
    let args = Cli::parse();

    let exclude_items = parse_array(args.exclude, ',', '"');
    let include_items = parse_array(args.include, ',', '"');

    let mut exclude_regex: Option<Regex> = None;
    if exclude_items.len() > 0 {
        exclude_regex = Some(
            Regex::new(
                std::format!(
                    "((?i){})",
                    exclude_items
                        .iter()
                        .map(|item| std::format!("({})", item))
                        .intersperse(String::from("|"))
                        .collect::<String>(),
                )
                .as_str(),
            )
            .unwrap(),
        );
    }

    let mut include_regex: Option<Regex> = None;
    if include_items.len() > 0 {
        include_regex = Some(
            Regex::new(
                std::format!(
                    "((?i){})",
                    include_items
                        .iter()
                        .map(|item| std::format!("({})", item))
                        .intersperse(String::from("|"))
                        .collect::<String>(),
                )
                .as_str(),
            )
            .unwrap(),
        );
    }

    let min_size = match &args.minsize {
        Some(size) => match parse_size(size) {
            Ok(size) => size,
            Err(e) => {
                panic!("{}", e);
            }
        },
        None => 0,
    };

    let max_size = match &args.maxsize {
        Some(size) => match parse_size(size) {
            Ok(size) => size,
            Err(e) => {
                panic!("{}", e);
            }
        },
        None => u64::MAX,
    };

    let mut files_by_size: HashMap<u64, Vec<PathBuf>> = HashMap::new();
    let bar = ProgressBar::new(u64::MAX);
    bar.set_style(ProgressStyle::with_template("[{elapsed_precise}] {msg}").unwrap());

    let mut directories = VecDeque::from([args.path.clone()]);

    let mut total_files: u64 = 0;
    let mut total_dirs: u64 = 0;
    let mut total_bytes: u64 = 0;
    while !directories.is_empty() {
        for entry in fs::read_dir(directories.pop_back().unwrap())
            .unwrap()
            .filter_map(|e| match e {
                Ok(entry) => {
                    let path_str = String::from(entry.path().to_str().unwrap());
                    let size = entry.metadata().unwrap().len();
                    let is_dir = entry.metadata().unwrap().is_dir();
                    if exclude_regex.is_some()
                        && exclude_regex.as_ref().unwrap().is_match(&path_str)
                    {
                        return None;
                    } else if include_regex.is_some()
                        && !include_regex.as_ref().unwrap().is_match(&path_str)
                    {
                        return None;
                    } else if !is_dir && size < min_size {
                        return None;
                    } else if !is_dir && size > max_size {
                        return None;
                    }

                    return Some(entry);
                }
                Err(_err) => {
                    return None;
                }
            })
        {
            if entry.metadata().unwrap().is_dir() {
                total_dirs += 1;
                bar.set_message(std::format!(
                    "Folders: {} Files: {} Bytes: {}",
                    HumanCount(total_dirs),
                    HumanCount(total_files),
                    HumanBytes(total_bytes)
                ));
                directories.push_front(entry.path());
            } else {
                total_files += 1;
                let metadata = entry.metadata().unwrap();
                total_bytes += metadata.len();
                if metadata.is_file() {
                    files_by_size
                        .entry(metadata.len())
                        .or_insert(Vec::new())
                        .push(entry.path());
                }
            }
        }
    }

    bar.finish();

    let size_dups: Vec<PathBuf> = files_by_size
        .iter()
        .filter(|entry| entry.1.len() >= 2)
        .flat_map(|entry| entry.1.to_owned())
        .collect();

    let bar = ProgressBar::new(size_dups.len().try_into().unwrap());
    bar.set_style(
        ProgressStyle::with_template("[{elapsed_precise}] {bar:40.cyan/blue} {percent}%").unwrap(),
    );
    let files_by_hash: HashMap<[u8; 32], HashSet<(String, u64)>> = size_dups
        .par_iter()
        .fold(
            || HashMap::new(),
            |mut acc, cur| {
                bar.inc(1);
                let mut hasher = Sha256Adapter::new();

                let f = fs::File::open(cur);
                if f.is_err() {
                    return acc;
                }

                if std::io::copy(&mut f.unwrap(), &mut hasher).is_err() {
                    return acc;
                }

                let hash = hasher.finish();
                acc.entry(hash).or_insert(HashSet::new()).insert((
                    String::from(cur.to_str().unwrap()),
                    cur.metadata().unwrap().len(),
                ));

                return acc;
            },
        )
        .reduce(
            || HashMap::new(),
            |mut acc, cur| {
                for (hash, files) in cur.iter() {
                    acc.entry(*hash)
                        .or_insert(files.clone())
                        .extend(files.iter().map(|x| x.clone()));
                }

                acc
            },
        );

    let mut object_str = String::from("");
    let mut object_writer = JSONObjectWriter::new(&mut object_str);
    object_writer.value("path", args.path.to_str());
    object_writer.value("output", args.output.as_path().to_str());
    object_writer.value("excluded", exclude_items.join(",").as_str());
    object_writer.value("included", include_items.join(",").as_str());
    object_writer.value("minsize", args.minsize.unwrap().as_str());
    object_writer.value("maxsize", args.maxsize.unwrap().as_str());
    let mut duplicates = object_writer.array("duplicates");

    let mut duplicate_bytes: u64 = 0;
    for (_, files) in files_by_hash.iter().filter(|(_, files)| files.len() >= 2) {
        let mut group = duplicates.array();
        let mut first = true;
        for (file, bytes) in files {
            group.value(file);

            if !first {
                duplicate_bytes += bytes;
            } else {
                first = false;
            }
        }
    }

    duplicates.end();
    object_writer.value(
        "duplicate_bytes",
        HumanBytes(duplicate_bytes).to_string().as_str(),
    );
    object_writer.end();

    if fs::write(args.output, object_str).is_err() {
        print!("Unable to create output file");
    } else {
        print!("Done!");
    }
}
