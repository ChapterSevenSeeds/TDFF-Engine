use clap::Parser;
use indicatif::{ProgressBar, ProgressIterator, ProgressStyle};
use itertools::Itertools;
use json_writer::JSONObjectWriter;
use regex::Regex;
use std::collections::{vec_deque, VecDeque};
use std::fs::File;
use std::{collections::HashMap, fs};
use walkdir::WalkDir;

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
    #[clap(short, long, value_delimiter = ',')]
    exclude: Option<String>,
    /// List of regular expression patterns to include in search.
    /// All other paths will be excluded.
    #[clap(short, long, value_delimiter = ',')]
    include: Option<String>,
    /// The minimum filesize to include in the search.
    #[clap(long)]
    minsize: Option<u64>,
    /// The maximum filesize to include in the search.
    #[clap(long)]
    maxsize: Option<u64>,
}

fn parse_array(arg: &str, delimiter: char, escape: char) -> Vec<String> {
    let mut temp: String = String::new();
    let mut result: Vec<String> = Vec::new();

    let mut inEscape: bool = false;
    let mut lastEscapeChar: char = '\0';
    let mut escapeSlashCount: u32 = 0;
    for c in arg.chars() {
        if c == escape && (!inEscape || c == lastEscapeChar) {
            if inEscape {
                if escapeSlashCount % 2 == 0 {
                    inEscape = false;
                }
            } else {
                lastEscapeChar = c;
                inEscape = true;
            }
        }

        if c == delimiter {
            if !inEscape {
                if !temp.is_empty() {
                    result.push(temp.clone());
                    temp.clear();
                }
            } else {
                temp += c.to_string().as_str();
            }
        } else {
            temp += c.to_string().as_str();
        }

        if inEscape && c == '\\' {
            escapeSlashCount += 1;
        } else {
            escapeSlashCount = 0;
        }
    }

    if !temp.is_empty() {
        result.push(temp);
    }

    return result;
}

fn main() {
    let args = Cli::parse();

    let exclude_items = parse_array(args.exclude.unwrap_or_default().as_str(), ',', '"');
    let include_items = parse_array(args.include.unwrap_or_default().as_str(), ',', '"');

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

    let min_size = match args.minsize {
        Some(size) => size,
        None => 0,
    };

    let max_size = match args.maxsize {
        Some(size) => size,
        None => u64::MAX,
    };

    let mut files_by_size: HashMap<u64, Vec<fs::DirEntry>> = HashMap::new();
    let bar = ProgressBar::new(u64::MAX);
    bar.set_style(ProgressStyle::with_template("{spinner} {msg}").unwrap());

    let mut directories = VecDeque::from([args.path.clone()]);

    while !directories.is_empty() {
        for entry in fs::read_dir(directories.pop_back().unwrap())
            .unwrap()
            .filter_map(|e| match e {
                Ok(entry) => {
                    let path_str = String::from(entry.path().to_str().unwrap());
                    let size = entry.metadata().unwrap().len();
                    if exclude_regex.is_some()
                        && exclude_regex.as_ref().unwrap().is_match(&path_str)
                    {
                        return None;
                    } else if include_regex.is_some()
                        && !include_regex.as_ref().unwrap().is_match(&path_str)
                    {
                        return None;
                    } else if size < min_size {
                        return None;
                    } else if size > max_size {
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
                bar.set_message(String::from(
                    entry.path().file_stem().unwrap().to_str().unwrap(),
                ));
                directories.push_front(entry.path());
            } else {
                let metadata = entry.metadata().unwrap();
                if metadata.is_file() {
                    files_by_size
                        .entry(metadata.len())
                        .or_insert(Vec::new())
                        .push(entry);
                }
            }
        }
    }

    bar.finish();

    let mut files_by_hash: HashMap<[u8; 32], Vec<&fs::DirEntry>> = HashMap::new();
    let size_dups: Vec<&fs::DirEntry> = files_by_size
        .iter()
        .filter(|entry| entry.1.len() >= 2)
        .flat_map(|entry| entry.1.to_owned())
        .collect();

    let bar = ProgressBar::new(size_dups.len().try_into().unwrap());
    bar.set_style(ProgressStyle::with_template("{bar:40.cyan/blue} {percent}%").unwrap());
    for file in size_dups.iter() {
        bar.inc(1);
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

    let mut object_str = String::from("");
    let mut object_writer = JSONObjectWriter::new(&mut object_str);
    object_writer.value("path", args.path.to_str());
    object_writer.value("output", args.output.as_path().to_str());
    object_writer.value("excluded", exclude_items.join(",").as_str());
    object_writer.value("included", include_items.join(",").as_str());
    object_writer.value("minsize", args.minsize.unwrap_or(0).to_string().as_str());
    object_writer.value(
        "maxsize",
        args.minsize.unwrap_or(u64::MAX).to_string().as_str(),
    );
    let mut duplicates = object_writer.array("duplicates");

    for (_, files) in files_by_hash.iter().filter(|(_, files)| files.len() >= 2) {
        let mut group = duplicates.array();
        for file in files {
            group.value(file.path().to_str().unwrap());
        }
    }

    duplicates.end();
    object_writer.end();

    if fs::write(args.output, object_str).is_err() {
        print!("Unable to create output file");
    }
}
