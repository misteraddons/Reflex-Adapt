TARGET_NAMES = SNES Saturn PSX PCEngine Jaguar 3DO NeoGeo N64 Wii Popular

SNES.FLAGS = -DENABLE_REFLEX_SNES
Saturn.FLAGS = -DENABLE_REFLEX_SATURN
PSX.FLAGS = -DENABLE_REFLEX_PSX
PCEngine.FLAGS = -DENABLE_REFLEX_PCE
Jaguar.FLAGS = -DENABLE_REFLEX_JAGUAR
3DO.FLAGS = -DENABLE_REFLEX_3DO
NeoGeo.FLAGS = -DENABLE_REFLEX_NEOGEO
N64.FLAGS = -DENABLE_REFLEX_N64_NEW
GameCube.FLAGS = -DENABLE_REFLEX_GAMECUBE
Wii.FLAGS = -DENABLE_REFLEX_WII
#Nintendo.FLAGS = -DENABLE_REFLEX_GAMECUBE -DENABLE_REFLEX_SNES -DENABLE_REFLEX_WII -DENABLE_REFLEX_N64_NEW
Nintendo.FLAGS = -DENABLE_REFLEX_SNES -DENABLE_REFLEX_WII -DENABLE_REFLEX_N64_NEW
Popular.FLAGS = -DENABLE_REFLEX_SNES -DENABLE_REFLEX_SATURN -DENABLE_REFLEX_PSX -DENABLE_REFLEX_NEOGEO


PRJ_DIR = Reflex
BUILD_DIR = build
TARGET_DIR = firmware

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

clean:
	rm -rf $(TARGET_DIR)
	rm -rf $(BUILD_DIR)

.PHONY: all clean $(TARGET_NAMES)
