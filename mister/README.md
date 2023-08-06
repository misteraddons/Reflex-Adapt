# Reflex Updater - MiSTer Wrapper

This is a wrapper which bundles all the Reflex updater files into a single
binary and autoconfigures the user's MiSTer to use Adapt properly.

It will:
- Check for and configure VID/PID settings in any of the user's MiSTer.ini files
- Check for and configure u-boot.txt module parameters for USB HID quirks
- Extract the updater to `/tmp` and run it as normal

## Build

1. Install [Go](https://golang.org/doc/install)
2. Copy these updater files and folders to the `_files` directory:
   1. `firmware` directory
   2. `manifest.txt` file
   3. `reflex-linux-armv7` binary
3. Run `env GOARCH=arm GOARM=7 go build -o _bin/reflex_updater.sh`

This will output a final binary for MiSTer in the `_bin` directory.

## Todo

- [ ] Automate this build
- [ ] Automate creation of a repo .json file for the downloader script
- [ ] Run UPX on final binary