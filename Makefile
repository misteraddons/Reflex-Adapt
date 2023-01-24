TARGET_NAMES = SNES Saturn PSX PCEngine Jaguar 3DO NeoGeo N64 GameCube Wii Nintendo

SNES.FLAGS = -DENABLE_REFLEX_SNES
Saturn.FLAGS = -DENABLE_REFLEX_SATURN
PSX.FLAGS = -DENABLE_REFLEX_PSX
PCEngine.FLAGS = -DENABLE_REFLEX_PCE
Jaguar.FLAGS = -DENABLE_REFLEX_JAGUAR
3DO.FLAGS = -DENABLE_REFLEX_3DO
NeoGeo.FLAGS = -DENABLE_REFLEX_NEOGEO
N64.FLAGS = -DENABLE_REFLEX_N64
GameCube.FLAGS = -DENABLE_REFLEX_GAMECUBE
Wii.FLAGS = -DENABLE_REFLEX_WII
Nintendo.FLAGS = -DENABLE_REFLEX_GAMECUBE -DENABLE_REFLEX_SNES -DENABLE_REFLEX_WII -DENABLE_REFLEX_N64


RELEASE_FILES = release_files/manifest.txt release_files/README.txt

PRJ_DIR = Reflex
BUILD_DIR = build
TARGET_DIR = firmware
RELEASE_DIR = release/reflex
UPDATER = reflex_updater.tar.gz


SRC = $(wildcard $(PRJ_DIR)/*.h $(PRJ_DIR)/*.c $(PRJ_DIR)/*.ino)

TARGETS = $(addsuffix .hex, $(addprefix $(TARGET_DIR)/, $(TARGET_NAMES)))

GCC_PATH := $(shell arduino-cli compile -b arduino:avr:leonardo Reflex --show-properties | grep runtime.tools.avr-gcc.path= | sed "s/.*=//")

all: $(TARGETS) $(TARGET_DIR)/sizes.txt

$(TARGET_DIR)/%.hex: $(SRC) | $(TARGET_DIR)
	@[ "$($*.FLAGS)" ] || ( echo ">> $*.FLAGS is not set"; exit 1 )
	arduino-cli compile -b arduino:avr:leonardo $(PRJ_DIR) --build-property "build.extra_flags={build.usb_flags} -DREFLEX_NO_DEFAULTS $($*.FLAGS)" -e --output-dir $(BUILD_DIR)/$*
	cp $(BUILD_DIR)/$*/Reflex.ino.elf $(TARGET_DIR)/$*.elf
	cp $(BUILD_DIR)/$*/Reflex.ino.hex $(TARGET_DIR)/$*.hex

$(TARGET_DIR)/sizes.txt: $(TARGETS)
	@cd $(TARGET_DIR) && $(GCC_PATH)/bin/avr-size $(notdir $(subst .hex,.elf,$^)) 2>&1 > sizes.txt
	@echo Size Summary 
	@cat $@

$(TARGET_DIR):
	mkdir -p $@

$(TARGET_NAMES): %: $(TARGET_DIR)/%.hex

release: $(TARGETS) $(UPDATER)
	rm -rf $(RELEASE_DIR)
	mkdir -p $(RELEASE_DIR)
	mkdir -p $(RELEASE_DIR)/firmware
	cp $(TARGETS) $(RELEASE_DIR)/firmware/
	cp $(RELEASE_FILES) $(RELEASE_DIR)/
	tar -zxf $(UPDATER) -O ./x86_64-pc-windows-msvc/reflex_updater.exe > $(RELEASE_DIR)/reflex.exe
	tar -zxf $(UPDATER) -O ./x86_64-unknown-linux-musl/reflex_updater > $(RELEASE_DIR)/reflex-linux-x86_64
	tar -zxf $(UPDATER) -O ./armv7-unknown-linux-musleabihf/reflex_updater > $(RELEASE_DIR)/reflex-linux-armv7
	tar -zxf $(UPDATER) -O ./x86_64-apple-darwin/reflex_updater > $(RELEASE_DIR)/reflex-macos
	chmod +x $(RELEASE_DIR)/reflex-macos $(RELEASE_DIR)/reflex-linux-armv7 $(RELEASE_DIR)/reflex-linux-x86_64

clean:
	rm -rf $(TARGET_DIR)
	rm -rf $(BUILD_DIR)

.PHONY: all clean $(TARGET_NAMES) release
