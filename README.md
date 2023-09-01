# Reflex Adapt

<a href="https://github.com/misteraddons/Reflex-Adapt/releases/download/v1.05/reflex-v1.05.zip"><img src="images/desktop-download.png" alt="Download Windows/Mac/Linux updater" title="Download Windows/Mac/Linux updater" width="140"></a>
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
