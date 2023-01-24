use anyhow::Result;
use serialport::{available_ports, SerialPortType};

#[cfg(target_os = "linux")]
use std::{
    fs::File,
    io::{BufRead, BufReader},
    path::PathBuf,
};

#[derive(Eq, PartialEq)]
pub struct PortInfo {
    pub port_name: String,
    vid: u16,
    pid: u16,
}

const BOOTLOADER_VID: u16 = 9025;
const BOOTLOADER_PID: u16 = 54;

/// Attempt to read vid and pid from devices uevent file
#[cfg(target_os = "linux")]
fn get_uevent_vid_pid(tty_path: &str) -> Result<Option<(u16, u16)>> {
    let path = PathBuf::from(tty_path).join("device/uevent");
    let file = File::open(path);
    if file.is_err() {
        return Ok(None);
    }
    for line_res in BufReader::new(file?).lines() {
        let line = line_res?;
        if let Some(("PRODUCT", value)) = line.split_once('=') {
            let parts: Vec<u16> = value
                .split('/')
                .map(|x| u16::from_str_radix(x, 16).unwrap_or(0u16))
                .collect();
            if parts.len() == 3 {
                return Ok(Some((parts[0], parts[1])));
            }

            return Ok(None);
        }
    }

    return Ok(None);
}

fn enumerate_ports() -> Result<Vec<PortInfo>> {
    let mut ports = Vec::new();

    for port in available_ports()? {
        match port.port_type {
            SerialPortType::UsbPort(info) => {
                if info.pid == BOOTLOADER_PID && info.vid == BOOTLOADER_VID {
                    ports.push(PortInfo {
                        port_name: port.port_name,
                        vid: info.vid,
                        pid: info.pid,
                    });
                }
            }

            #[cfg(target_os = "linux")]
            SerialPortType::Unknown => {
                if let Some((vid, pid)) = get_uevent_vid_pid(&port.port_name)? {
                    if vid == BOOTLOADER_VID && pid == BOOTLOADER_PID {
                        if let Some((_, devname)) = port.port_name.rsplit_once('/') {
                            ports.push(PortInfo {
                                port_name: format!("/dev/{}", devname),
                                vid: vid,
                                pid: pid,
                            });
                        }
                    }
                }
            }

            _ => {}
        };
    }

    return Ok(ports);
}

pub fn find_port(ignore_list: &[PortInfo]) -> Result<Option<PortInfo>> {
    for port in enumerate_ports()? {
        if ignore_list.iter().find(|x| *x == &port).is_some() {
            continue;
        }
        return Ok(Some(port));
    }
    return Ok(None);
}
