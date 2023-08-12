# Reflex Adapt

Reflex Adapt converts original gaming input devices to USB for use with MiSTer/PC/Linux/Mac. The controllers show up as direct input devices and feature very low latency. 

The built in OLED display shows the current mode, and a realtime view of the buttons as they're pressed.

Press the "MODE" button to toggle between modes.

A limited number of modes fit in the internal memory so use the Updater to load different combos.

If you would like a specific combo added, please create an Issue.

## MiSTer

Install the Adapt firmware updater on MiSTer by downloading [reflex_updater.sh](https://github.com/misteraddons/Reflex-Adapt/releases/latest/download/reflex_updater.sh) and copying it to the `Scripts` folder on your SD card.

Alternatively, add the following to your `downloader.ini` file to install and auto-update through Downloader and Update All:

```
[misteraddons/reflexadapt]
db_url = https://github.com/misteraddons/Reflex-Adapt/releases/latest/download/reflexadapt.json
```

The `reflex_updater.sh` script can then be launched from the Scripts menu.
