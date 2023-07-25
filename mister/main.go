package main

import (
	"embed"
	"fmt"
	"github.com/wizzomafizzo/mrext-lib/pkg/mister"
	"github.com/wizzomafizzo/mrext-lib/pkg/utils"
	"io"
	"io/fs"
	"os"
	"os/exec"
	"path/filepath"
	"strings"
)

const (
	reflexBinName = "reflex-linux-armv7"
	vidKey        = "no_merge_vid"
	adaptVid      = "0x2341"
	pidKey        = "no_merge_pid"
	adaptPid      = "0x8036"
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

// tryUpdateInis checks if the user needs the merge vid/pid options set in any of their .ini files, prompts them
// if they want to update them, and then updates them if they do. It is silent if no .ini files need updating.
func tryUpdateInis() error {
	inis, err := mister.ListMisterInis()
	if err != nil {
		return err
	}

	missingVidPid := make([]int, 0)

	// check for the keys in each .ini file
	for i := range inis {
		id, data, err := mister.LoadMisterIni(i + 1)
		if err != nil {
			return err
		}

		section := data.Section(mister.MainIniSection)
		if section == nil {
			missingVidPid = append(missingVidPid, id)
			continue
		}

		if !section.HasKey(vidKey) || !section.HasKey(pidKey) {
			missingVidPid = append(missingVidPid, id)
			continue
		}

		vid := section.Key(vidKey).String()
		pid := section.Key(pidKey).String()

		if vid != adaptVid || pid != adaptPid {
			missingVidPid = append(missingVidPid, id)
			continue
		}
	}

	// nothing to do
	if len(missingVidPid) == 0 {
		return nil
	}

	// prompt the user
	answer := utils.YesOrNoPrompt(
		"Some of your .ini files are not configured correctly for Reflex Adapt. Would you like to update them?",
	)
	if !answer {
		return nil
	}

	// update the .ini files
	for _, id := range missingVidPid {
		_, data, err := mister.LoadMisterIni(id)
		if err != nil {
			return err
		}

		err = mister.UpdateMisterIni(data, vidKey, adaptVid)
		if err != nil {
			return err
		}

		err = mister.UpdateMisterIni(data, pidKey, adaptPid)
		if err != nil {
			return err
		}

		err = mister.SaveMisterIni(id, data)
		if err != nil {
			return err
		}
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
			"Your u-boot.txt is not configured correctly for Reflex Adapt. Would you like to update it?",
		)
		if !answer {
			return false, nil
		}

		quirks = append(quirks, adaptQuirks)
		err = mister.UpdateUsbHidQuirks(quirks)
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
	err := tryUpdateInis()
	if err != nil {
		fmt.Printf("An error occurred while updating .ini files: %s", err)
		os.Exit(1)
	}

	updated, err := tryUpdateUboot()
	if err != nil {
		fmt.Printf("An error occurred while updating u-boot.txt: %s", err)
		os.Exit(1)
	}
	if updated {
		fmt.Println("Please power cycle your MiSTer for u-boot.txt changes to take effect.")
		os.Exit(0)
	}

	updaterDir, err := extractUpdater()
	if err != nil {
		fmt.Printf("An error occurred while extracting the updater: %s", err)
		_ = cleanupUpdater(updaterDir)
		os.Exit(1)
	}

	clearTerminal()

	cmd := exec.Command(filepath.Join(updaterDir, reflexBinName))
	cmd.Dir = updaterDir
	cmd.Stdout = os.Stdout
	cmd.Stderr = os.Stderr
	cmd.Stdin = os.Stdin
	err = cmd.Run()
	if err != nil {
		fmt.Printf("An error occurred while running the updater: %s", err)
		_ = cleanupUpdater(updaterDir)
		os.Exit(1)
	}

	_ = cleanupUpdater(updaterDir)
	os.Exit(0)
}
