mod avr910;
mod page_map;
mod ports;

use anyhow::{anyhow, Context, Result};
use dialoguer::{theme::ColorfulTheme, Select};
use ihex::Record;
use indicatif::{ProgressBar, ProgressStyle};
use page_map::PageMap;

use console::{style, Style};

use std::{
    fs::File,
    io::{self, BufRead},
    path::{Path, PathBuf},
    time::Duration,
};

const PAGE_SIZE: usize = 128;
const SOFTWARD_ID: [u8; 7] = [67, 65, 84, 69, 82, 73, 78];
const MANIFEST_NAME: &'static str = "manifest.txt";

fn selection_theme() -> ColorfulTheme {
    ColorfulTheme {
        prompt_prefix: style("?".to_string()).for_stderr().yellow(),
        prompt_suffix: style(">".to_string()).for_stderr().black().bright(),
        success_prefix: style("+".to_string()).for_stderr().green(),
        success_suffix: style(":".to_string()).for_stderr().black().bright(),
        error_prefix: style("x".to_string()).for_stderr().red(),
        active_item_style: Style::new().for_stderr().black().on_cyan(),
        inactive_item_style: Style::new().for_stderr(),
        active_item_prefix: style(">".to_string()).for_stderr().cyan(),
        inactive_item_prefix: style(" ".to_string()).for_stderr(),
        ..Default::default()
    }
}

#[rustfmt::skip]
fn display_splash() {
    let logo = concat!(
        r"     ____  ______________    _______  __", "\n",
        r"    / __ \/ ____/ ____/ /   / ____/ |/ /", "\n",
        r"   / /_/ / __/ / /_  / /   / __/  |   / ", "\n",
        r"  / _, _/ /___/ __/ / /___/ /___ /   |  ", "\n",
        r" /_/ |_/_____/_/   /_____/_____//_/|_|  "
    );

    let subtitle =
        r"                       Reflex Adapter!  ";
    let info = concat!(
        r"          http://www.misteraddons.com/  "
    );
    
    println!();
    println!("{}", style(logo).magenta().bold());
    println!("{}", style(subtitle).cyan().bold());
    println!();
    println!("{}", style(info).blue());
    println!();
}

fn read_ihex<P: AsRef<Path>>(path: P, page_size: usize) -> Result<PageMap> {
    let ihex_data = std::fs::read_to_string(path)?;
    let mut page_map = PageMap::new(page_size);
    let reader = ihex::Reader::new(&ihex_data);
    for rec in reader {
        match rec? {
            Record::Data { offset, value } => {
                page_map.add_bytes(offset as usize, &value);
            }
            Record::EndOfFile => break,
            _ => println!("Not expected"),
        }
    }

    Ok(page_map)
}

fn flash_device<P: AsRef<Path>>(port_name: &str, file_name: P) -> Result<()> {
    let green_ok = style("OK").green().to_string();
    let red_error = style("ERROR").red().to_string();

    let bar_style =
        ProgressStyle::with_template("{prefix:10} {bar:25.cyan/blue} {msg}")?.progress_chars("#>-");

    print!("Reading {:#?} ... ", file_name.as_ref());
    let page_map = read_ihex(file_name, PAGE_SIZE)?;
    println!("{}", green_ok);

    print!("Opening port {} ... ", port_name);
    let port = avr910::Avr910::new(&port_name)?;
    println!("{}", green_ok);

    print!("Checking device ... ");
    let id = port.get_software_identifier()?;
    if id.cmp(&SOFTWARD_ID).is_ne() {
        return Err(anyhow!("Invalid software identifier from device."));
    }

    match port.get_block_size()? {
        Some(PAGE_SIZE) => {}
        Some(x) => return Err(anyhow!("Invalid block size {}, expected {}", x, PAGE_SIZE)),
        None => return Err(anyhow!("No block write support, invalid device.")),
    };

    println!("{}", green_ok);

    let page_addrs = page_map.get_page_addrs();

    {
        let progress = ProgressBar::new(page_addrs.len() as u64)
            .with_style(bar_style.clone())
            .with_prefix("Flashing");
        for k in &page_addrs {
            let file_data = page_map.get_page(*k).unwrap();
            if let Err(e) = port.write_flash(*k, &file_data) {
                progress.finish_with_message(red_error.clone());
                return Err(e);
            }
            progress.inc(1);
        }
        progress.finish_with_message(green_ok.clone());
    }

    {
        let progress = ProgressBar::new(page_addrs.len() as u64)
            .with_style(bar_style.clone())
            .with_prefix("Verifying");
        let mut flash_data = vec![0u8; 128];
        for k in &page_addrs {
            port.read_flash(*k, &mut flash_data)?;
            let file_data = page_map.get_page(*k).unwrap();
            if file_data.cmp(&flash_data).is_ne() {
                progress.finish_with_message(red_error.clone());
                return Err(anyhow!("Verification failed at page 0x{:04x}", k));
            }
            progress.inc(1);
        }
        progress.finish_with_message(green_ok.clone());
    }

    print!("Rebooting device ... ");
    port.finalize()?;
    println!("{}", green_ok);

    Ok(())
}

fn wait_for_port(use_existing: bool) -> Result<Option<String>> {
    let mut ignore_list = vec![];
    if !use_existing {
        while let Some(name) = ports::find_port(&ignore_list)? {
            ignore_list.push(name);
        }
    }

    let style =
        ProgressStyle::with_template("{spinner:.cyan} {msg:.white.bold}")?.tick_chars("-\\|/+");
    let progress = ProgressBar::new_spinner()
        .with_style(style)
        .with_message("Waiting for reset");

    loop {
        if let Some(port_info) = ports::find_port(&ignore_list)? {
            progress.finish_with_message("Device found.");
            println!("");
            return Ok(Some(port_info.port_name));
        }
        std::thread::sleep(Duration::from_millis(250));
        progress.tick();
    }
}

struct ManifestEntry {
    desc: String,
    path: PathBuf,
}

fn read_manifest() -> Result<Vec<ManifestEntry>> {
    let cwd = std::env::current_dir()?;
    let mut exe_path = std::env::current_exe()?;

    exe_path.pop();

    let cwd_manifest = cwd.join(MANIFEST_NAME);
    let exe_manifest = exe_path.join(MANIFEST_NAME);

    let (file, root) = if cwd_manifest.exists() {
        (
            File::open(&cwd_manifest).with_context(|| format!("reading {:#?}", cwd_manifest))?,
            &cwd,
        )
    } else {
        (
            File::open(&exe_manifest).with_context(|| format!("reading {:#?}", exe_manifest))?,
            &exe_path,
        )
    };

    let mut manifest = vec![];
    for line_res in io::BufReader::new(file).lines() {
        let line = line_res?;
        if line.starts_with('#') {
            continue;
        }

        if let Some((path, desc)) = line.split_once(',') {
            manifest.push(ManifestEntry {
                desc: String::from(desc),
                path: root.join(path),
            });
        }
    }

    Ok(manifest)
}

fn main_interactive() -> Result<bool> {
    let manifest = read_manifest().context("Reading manifest file")?;
    let names: Vec<_> = manifest.iter().map(|x| x.desc.clone()).collect();
    let selection = Select::with_theme(&selection_theme())
        .items(&names)
        .with_prompt("Select Reflex Firmware (ESC or q to cancel)")
        .default(0)
        .report(false)
        .interact_opt()?;

    if let Some(selection) = selection {
        let entry = &manifest[selection];
        println!("\n> Firmware: {}", style(entry.desc.clone()).bold().green());
        println!(
            "\nPress the {} button on your Reflex device.",
            style("RESET").bold().bright().cyan()
        );
        let port_name = wait_for_port(false)?;
        if let Some(port_name) = port_name {
            flash_device(&port_name, &entry.path)?;
        }
        Ok(true)
    } else {
        println!("User cancelled.");
        Ok(false)
    }
}

fn wait_for_return() {
    let mut dummy = String::new();
    let _ = io::stdin().read_line(&mut dummy);
}

fn main() -> Result<()> {
    display_splash();

    match main_interactive() {
        Ok(true) => {
            println!("\nSuccess. Press RETURN to exit.");
            wait_for_return();
            Ok(())
        }
        Ok(false) => Ok(()),
        Err(e) => {
            eprintln!(
                "\nAn unexpected error was encountered:\n{:#}\n\nPress RETURN to exit.",
                e
            );
            wait_for_return();
            Err(e)
        }
    }
}
