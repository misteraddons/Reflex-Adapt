package main

import (
	"embed"
	"fmt"
	"github.com/wizzomafizzo/mrext/pkg/config"
	"github.com/wizzomafizzo/mrext/pkg/mister"
	"github.com/wizzomafizzo/mrext/pkg/misterini"
	"github.com/wizzomafizzo/mrext/pkg/utils"
	"io"
	"io/fs"
	"os"
	"os/exec"
	"os/signal"
	"path/filepath"
	"strings"
)

const (
	quirksAreRequired = true // TODO: remove this after next main release
	reflexBinName     = "reflex-linux-armv7"
	adaptQuirks       = "0x16d0:0x127e:0x040"
	adaptVidPid       = "0x16d0127e"
	dbName            = "misteraddons/reflexadapt"
	dbUrl             = "https://github.com/misteraddons/Reflex-Adapt/raw/main/reflexadapt.json.zip"
	configFolder      = config.ScriptsConfigFolder + "/reflex"
	noDbFile          = configFolder + "/.no-db-reflexadapt"
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
	//goland:noinspection GoBoolExpressions
	if !quirksAreRequired {
		return nil
	}

	missing, err := misterini.GetInisWithout(misterini.KeyNoMergeVidpid, adaptVidPid)
	if err != nil {
		return err
	}

	// nothing to do
	if len(missing) == 0 {
		return nil
	}

	// prompt the user
	answer := utils.YesOrNoPrompt(
		"Some of your MiSTer.ini files are not configured for Reflex Adapt's multitap support. Would you like to update them?",
	)
	if !answer {
		return nil
	}

	// update the .ini files
	for _, mi := range missing {
		err := mi.Load()
		if err != nil {
			return err
		}

		err = mi.AddKey(misterini.KeyNoMergeVidpid, adaptVidPid)
		if err != nil {
			return err
		}

		err = mi.Save()
		if err != nil {
			return err
		}
	}

	return nil
}

// tryUpdateUboot checks if the user needs the usbhid.quirks option set in their u-boot.txt, prompts them if they want
// to update it, and then updates it if they do. It is silent if u-boot.txt does not need updating.
func tryUpdateUboot() (bool, error) {
	//goland:noinspection GoBoolExpressions
	if !quirksAreRequired {
		fastUsb, err := mister.IsFastUsbPollActive()
		if err != nil {
			return false, err
		}

		if !fastUsb {
			answer := utils.YesOrNoPrompt(
				"Reflex Adapt works best with fast USB polling enabled in your system's u-boot.txt. Would you like to enable it?",
			)
			if !answer {
				return false, nil
			}

			err = mister.EnableFastUsbPoll()
			if err != nil {
				return false, err
			}

			return true, nil
		} else {
			return false, nil
		}
	}

	quirks, err := mister.GetUsbHidQuirks()
	if err != nil {
		return false, err
	}

	if !utils.Contains(quirks, adaptQuirks) {
		answer := utils.YesOrNoPrompt(
			"Reflex Adapt requires changes to your system's u-boot.txt for fast USB polling and composite USB devices. Would you like to update it?",
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

// tryAddDb prompts if the user wants the updater repo db added to their downloader.ini file. Optionally, they can
// say no and the check will be disabled.
func tryAddDb() (bool, error) {
	//_ = os.MkdirAll(configFolder, 0755)
	//
	//if _, err := os.Stat(noDbFile); err == nil {
	//	return false, nil
	//}

	downloadIni, err := mister.LoadDownloaderIni()
	if err != nil {
		return false, err
	}
	if downloadIni.HasDb(dbName) {
		return false, nil
	}

	answer := utils.YesOrNoPrompt("Do you want Reflex Updater to automatically update with downloader and update_all?")
	if !answer {
		err := os.WriteFile(noDbFile, []byte{}, 0644)
		if err != nil {
			return false, err
		}
		return false, nil
	}

	err = downloadIni.AddDb(dbName, dbUrl)
	if err != nil {
		return false, err
	}

	err = downloadIni.Save()
	if err != nil {
		return false, err
	}

	return true, nil
}

func clearTerminal() {
	fmt.Print("\033[H\033[2J")
}

func main() {
	wasError := false

	//err := tryUpdateInis()
	//if err != nil {
	//	fmt.Printf("An error occurred while updating .ini files: %s\n", err)
	//	wasError = true
	//}

	updated, err := tryAddDb()
	if err != nil {
		fmt.Printf("An error occurred while updating downloader.ini: %s\n", err)
		wasError = true
	}
	if updated {
		utils.InfoPrompt("Please run downloader or update_all to get controller mappings after configuring Adapt.")
	}

	updated, err = tryUpdateUboot()
	if err != nil {
		fmt.Printf("An error occurred while updating u-boot.txt: %s\n", err)
		wasError = true
	}
	if updated {
		fmt.Println("Please power cycle your MiSTer for these changes to take effect.")
		os.Exit(0)
	}

	if wasError {
		utils.InfoPrompt("Errors occurred while setting up Adapt for MiSTer. You can safely configure the firmware of your Adapt, but it may not function correctly on MiSTer until the errors are addressed.")
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
