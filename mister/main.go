package main

import (
	"embed"
	"fmt"
	"github.com/wizzomafizzo/mrext/pkg/mister"
	"github.com/wizzomafizzo/mrext/pkg/utils"
	"io"
	"io/fs"
	"os"
	"os/exec"
	"os/signal"
	"path/filepath"
	"strings"
)

// TODO: when getting rid of usb quirks, add in a check for fast polling

const (
	reflexBinName = "reflex-linux-armv7"
	adaptQuirks   = "0x2341:0x8036:0x040"
)

//go:embed _files
var updaterFiles embed.FS

// extractUpdater extracts the embedded updater files to a temporary directory and returns the path to them.
func extractUpdater() (string, error) {
	tmp, err := os.MkdirTemp("", "reflex-updater-*")
	if err != nil {
		return "", err
	}

	topFiles, err := updaterFiles.ReadDir(".")
	if err != nil {
		return "", err
	} else if len(topFiles) != 1 {
		return "", fmt.Errorf("expected 1 top-level embedded folder, found %d files", len(topFiles))
	}

	embedPrefix := topFiles[0].Name()

	// this is a very complicated version of: cp -r <embedded files>/* <tmp folder>
	err = fs.WalkDir(updaterFiles, embedPrefix, func(path string, entry fs.DirEntry, err error) error {
		if path == embedPrefix {
			return nil
		}

		writePath := filepath.Join(tmp, strings.TrimPrefix(path, embedPrefix+"/"))

		if entry.IsDir() {
			return os.MkdirAll(writePath, 0755)
		}

		tmpFile, err := os.Create(writePath)
		if err != nil {
			return err
		}
		defer func(tmpFile *os.File) {
			_ = tmpFile.Close()
		}(tmpFile)

		file, err := updaterFiles.Open(path)
		if err != nil {
			return err
		}
		defer func(file fs.File) {
			_ = file.Close()
		}(file)

		_, err = io.Copy(tmpFile, file)
		if err != nil {
			return err
		}

		if filepath.Base(writePath) == reflexBinName {
			err = os.Chmod(writePath, 0755)
			if err != nil {
				return err
			}
		}

		return nil
	})
	if err != nil {
		return "", err
	}

	return tmp, nil
}

// cleanupUpdater removes the temporary directory created by extractUpdater.
func cleanupUpdater(tmp string) error {
	err := os.RemoveAll(tmp)
	if err != nil {
		return err
	}
	return nil
}

// tryUpdateUboot checks if the user needs the usbhid.quirks option set in their u-boot.txt, prompts them if they want
// to update it, and then updates it if they do. It is silent if u-boot.txt does not need updating.
func tryUpdateUboot() (bool, error) {
	quirks, err := mister.GetUsbHidQuirks()
	if err != nil {
		return false, err
	}

	if !utils.Contains(quirks, adaptQuirks) {
		answer := utils.YesOrNoPrompt(
			"Reflex Adapt requires changes to your system's u-boot.txt. Would you like to update it?",
		)
		if !answer {
			return false, nil
		}

		quirks = append(quirks, adaptQuirks)
		err = mister.UpdateUsbHidQuirks(quirks)
		if err != nil {
			return false, err
		}

		err = mister.EnableFastUsbPoll()
		if err != nil {
			return false, err
		}

		return true, nil
	}

	return false, nil
}

func clearTerminal() {
	fmt.Print("\033[H\033[2J")
}

func main() {
	updated, err := tryUpdateUboot()
	if err != nil {
		fmt.Printf("An error occurred while updating u-boot.txt: %s\n", err)
		os.Exit(1)
	}
	if updated {
		fmt.Println("Please power cycle your MiSTer for these changes to take effect.")
		os.Exit(0)
	}

	updaterDir, err := extractUpdater()
	if err != nil {
		fmt.Printf("An error occurred while extracting the updater: %s\n", err)
		_ = cleanupUpdater(updaterDir)
		os.Exit(1)
	}

	clearTerminal()

	cmd := exec.Command(filepath.Join(updaterDir, reflexBinName))
	cmd.Dir = updaterDir
	cmd.Stdout = os.Stdout
	cmd.Stderr = os.Stderr
	cmd.Stdin = os.Stdin

	// forward signal to the updater
	sigs := make(chan os.Signal, 1)
	signal.Notify(sigs)
	go func(cmd *exec.Cmd) {
		sig := <-sigs
		if cmd.Process != nil {
			_ = cmd.Process.Signal(sig)
		}
	}(cmd)

	err = cmd.Run()
	if err != nil {
		fmt.Printf("An error occurred while running the updater: %s\n", err)
		_ = cleanupUpdater(updaterDir)
		os.Exit(1)
	}

	_ = cleanupUpdater(updaterDir)
	os.Exit(0)
}
