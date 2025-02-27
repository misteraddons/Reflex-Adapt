# Reflex Adapt

<a href="https://misteraddons.com/collections/parts/products/reflex-adapt"><img src="images/purchase.png" alt="Purchase Reflex Adapt" title="Purchase Reflex Adapt" width="200"></a>

<a href="https://github.com/misteraddons/Reflex-Adapt/releases/download/v2.00/reflex-v2.00.zip"><img src="images/desktop-download.png" alt="Download Windows/Mac/Linux updater" title="Download Windows/Mac/Linux updater" width="140"></a>
<a href="https://github.com/misteraddons/Reflex-Adapt/releases/latest/download/reflex_updater.sh"><img src="images/mister-download.png" alt="Download MiSTer updater" title="Download MiSTer updater" width="140"></a>


Reflex Adapt converts original gaming input devices to USB for use with MiSTer/PC/Linux/Mac. The controllers show up as direct input devices and feature very low latency. 

The built in OLED display shows the current mode, and a realtime view of the buttons as they're pressed.

Press the "MODE" button to toggle between modes.

A limited number of modes fit in the internal memory so use the Updater to load different combos.

If you would like a specific combo added, please create an Issue.

## MiSTer Installation

1. Download [reflex_updater.sh](https://github.com/misteraddons/Reflex-Adapt/releases/latest/download/reflex_updater.sh) and copy it to the `Scripts` folder on your MiSTer's SD card
2. Launch the `reflex_updater` script from the Scripts menu on your MiSTer
3. When prompted, agree to have the Reflex Adapt repository added
4. Flash your Reflex Adapt with the desired firmware configuration and exit the updater
5. Launch `update_all` or `downloader` from the Scripts menu

The Reflex Adapt updater, controller mappings and core configs will now be automatically updated whenever Update All or Downloader is run, excluding those you have made changes to.

Alternatively, manually add the following to your `downloader.ini` file on the SD card:

```
[misteraddons/reflexadapt]
db_url = https://github.com/misteraddons/Reflex-Adapt/raw/main/reflexadapt.json.zip
```

## Building Firmware on Linux
1. Clone the repo and cd into the directory
2. Install [Arduino-cli](https://arduino.github.io/arduino-cli/installation): Use brew or the install script.
3. If you installed with the install script add to PATH
4. Install arduino:avr core `arduino-cli core install arduino:avr`
5. Install Arduino-LUFA:avr core `arduino-cli core install Arduino-LUFA:avr --additional-urls https://github.com/Palatis/Arduino-Lufa/raw/master/package_arduino-lufa_index.json`
6. make all

## Building reflex_updater
1. install [Rust](https://www.rust-lang.org/tools/install)
2. cd reflex_updater
3. cargo run or cargo build
