use anyhow::{anyhow, Context, Result};
use serialport;
use std::cell::RefCell;
use std::time::Duration;

pub struct Avr910 {
    port: RefCell<Box<dyn serialport::SerialPort>>,
}

impl Avr910 {
    pub fn new(path: &str) -> Result<Avr910> {
        let port = serialport::new(path, 57600)
            .timeout(Duration::from_millis(5000))
            .open()?;

        Ok(Avr910 {
            port: RefCell::new(port),
        })
    }

    fn cmd(&self, command: char) -> Result<()> {
        let mut buf = [0; 4];
        command.encode_utf8(&mut buf);
        self.port
            .borrow_mut()
            .write_all(&buf[0..1])
            .with_context(|| format!("sending command"))?;
        Ok(())
    }

    fn cmd_with_data(&self, command: char, data: &[u8]) -> Result<()> {
        let mut buf = [0; 4];
        command.encode_utf8(&mut buf);
        self.port
            .borrow_mut()
            .write_all(&buf[0..1])
            .with_context(|| format!("sending command"))?;
        self.port
            .borrow_mut()
            .write_all(data)
            .with_context(|| format!("sending data"))?;
        Ok(())
    }

    fn reply_confirm(&self) -> Result<()> {
        let mut resp = [0; 1];
        self.port
            .borrow_mut()
            .read_exact(&mut resp)
            .with_context(|| format!("reading command confirmation"))?;
        if resp[0] == 0xd {
            return Ok(());
        } else {
            return Err(anyhow!(
                "expected confirmation but received: 0x{:02x}",
                resp[0]
            ));
        }
    }

    fn reply_yesno(&self) -> Result<bool> {
        let mut resp = [0; 1];
        self.port
            .borrow_mut()
            .read_exact(&mut resp)
            .with_context(|| format!("reading response"))?;
        if resp[0] == 0x59 {
            // Y
            return Ok(true);
        } else if resp[0] == 0x4e {
            // N
            return Ok(false);
        } else {
            return Err(anyhow!("expected Y/N but received: 0x{:02x}", resp[0]));
        }
    }

    fn reply_data(&self, data: &mut [u8]) -> Result<()> {
        self.port
            .borrow_mut()
            .read_exact(data)
            .with_context(|| format!("reading {} bytes", data.len()))?;
        return Ok(());
    }

    pub fn get_software_identifier(&self) -> Result<[u8; 7]> {
        let mut buf = [0u8; 7];
        self.cmd('S')?;
        self.reply_data(&mut buf)?;
        Ok(buf)
    }

    /*pub fn get_bootloader_version(&self) -> Result<u16> {
        self.cmd('V')?;
        let mut buf = [0u8; 2];
        self.reply_data(&mut buf)?;
        Ok(((buf[0] as u16 - 0x30) << 8) + (buf[1] as u16 - 0x30))
    }*/

    pub fn get_block_size(&self) -> Result<Option<usize>> {
        self.cmd('b')?;
        if self.reply_yesno()? {
            let mut buf = [0u8; 2];
            self.reply_data(&mut buf)?;
            Ok(Some(((buf[0] as usize) << 8) | (buf[1] as usize)))
        } else {
            Ok(None)
        }
    }

    fn set_addr(&self, addr: usize) -> Result<()> {
        let low = ((addr >> 1) & 0xff) as u8;
        let high = ((addr >> 9) & 0xff) as u8;
        self.cmd_with_data('A', &[high, low])?;
        self.reply_confirm()
    }

    pub fn read_flash(&self, addr: usize, data: &mut [u8]) -> Result<()> {
        self.set_addr(addr).context("set_addr")?;
        let length = data.len();
        let low = ((length) & 0xff) as u8;
        let high = ((length >> 8) & 0xff) as u8;
        self.cmd_with_data('g', &[high, low, 0x46])?;
        self.reply_data(data)
    }

    pub fn write_flash(&self, addr: usize, data: &[u8]) -> Result<()> {
        self.set_addr(addr).context("set_addr")?;
        let length = data.len();
        let low = ((length) & 0xff) as u8;
        let high = ((length >> 8) & 0xff) as u8;
        let mut v = Vec::with_capacity(3 + data.len());
        v.extend_from_slice(&[high, low, 0x46]);
        v.extend_from_slice(data);
        self.cmd_with_data('B', &v)?;
        self.reply_confirm()
    }

    pub fn finalize(&self) -> Result<()> {
        self.cmd('E')?;
        self.reply_confirm()
    }
}
