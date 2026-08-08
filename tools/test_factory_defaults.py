#!/usr/bin/env python3
"""Release guards for Classic2USB factory and per-controller defaults."""

from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]


def read(path: str) -> str:
    return (ROOT / path).read_text(encoding="utf-8")


def require(source: str, token: str, label: str) -> None:
    if token not in source:
        raise AssertionError(f"{label}: missing {token!r}")


def block(source: str, start: str, end: str) -> str:
    return source.split(start, 1)[1].split(end, 1)[0]


def main() -> int:
    defaults = read("firmware/core/settings_store_per_mode_defaults.cpp")
    per_mode = read("firmware/core/settings_store_per_mode_quick.cpp")
    registry = read("firmware/core/settings_registry.h")
    default_modes = read("firmware/config/classic2usb/default_modes.h")
    setting_state = read("firmware/core/controller_settings_state.cpp")
    quick_visibility = read("firmware/menu/quick_config_visibility.cpp")
    runner = read("tools/run_release_checks.py")

    button_defaults = block(
        defaults,
        "uint8_t defaultButtonMapModeForInputMode",
        "uint8_t defaultZButtonModeForInputMode",
    )
    for token, label in (
        ("case RZORD_NES:      return 0;", "NES Name default"),
        ("case RZORD_SNES:     return 1;", "SNES Position default"),
        ("case RZORD_N64:      return 0;", "N64 Name default"),
        ("case RZORD_GAMECUBE: return 0;", "GameCube Name default"),
        ("case RZORD_WII:      return 1;", "Wii Classic Position default"),
        ("case RZORD_VBOY:     return 1;", "Virtual Boy Position default"),
    ):
        require(button_defaults, token, label)
    require(quick_visibility, 'return temp_btn_map ? "Position" : "Name";',
            "Name/Position labels")

    z_defaults = block(
        defaults,
        "uint8_t defaultZButtonModeForInputMode",
        "uint8_t defaultN64CStickModeForInputMode",
    )
    require(z_defaults, "case RZORD_N64:      return 1;", "N64 Z to L2 default")
    require(z_defaults, "case RZORD_GAMECUBE: return 0;", "GameCube Z to R1 default")
    require(quick_visibility, '? (temp_n64_z ? "Back" : "R1")', "GameCube Z labels")
    require(quick_visibility, ': (temp_n64_z ? "L2" : "L1");', "N64 Z labels")

    spinner_defaults = block(
        defaults,
        "uint8_t defaultSpinnerSpeedForInputMode",
        "uint8_t defaultRumbleLevelForInputMode",
    )
    require(spinner_defaults, "if (mode == RZORD_DRIVING)", "Atari Driving override")
    require(spinner_defaults, "return 3;", "Atari Driving 2x default")
    require(spinner_defaults, "return 2;", "Standard spinner 1x default")
    require(
        setting_state,
        'spinner_speed_labels[5] = { "0.25x", "0.5x", "1x", "2x", "4x" };',
        "Spinner value labels",
    )

    rumble_defaults = block(
        defaults,
        "uint8_t defaultRumbleLevelForInputMode",
        "uint8_t defaultNsoSpecialForInputMode",
    )
    require(rumble_defaults, "return RUMBLE_MODE_VARIABLE;", "Rumble Variable default")
    require(quick_visibility, 'case RUMBLE_MODE_VARIABLE: return "Variable";',
            "Rumble Variable label")
    require(quick_visibility, 'case RUMBLE_MODE_MAX: return "Max";',
            "Rumble Max label")
    require(registry, "RUMBLE_MODE_VARIABLE,         RUMBLE_MODE_OFF, RUMBLE_MODE_MAX",
            "Rumble registry default and range")

    trigger_defaults = block(
        defaults,
        "uint8_t defaultTriggerModeForInputMode",
        "bool isConcreteModeForPerSettings",
    )
    require(trigger_defaults, "if (mode == RZORD_GAMECUBE)", "GameCube trigger default")
    require(trigger_defaults, "if (mode == RZORD_WII)", "Wii Classic trigger default")
    if trigger_defaults.count("return TRIGGER_MODE_BOTH;") != 2:
        raise AssertionError("Only GameCube and Wii Classic should default triggers to Both")
    require(trigger_defaults, "return TRIGGER_MODE_ANALOG;", "Other trigger defaults")

    for token, label in (
        ("settings.rumble_level = defaultRumbleLevelForInputMode(mode);", "rumble record"),
        ("settings.trigger_mode = defaultTriggerModeForInputMode(mode);", "trigger record"),
        ("settings.spinner_speed = defaultSpinnerSpeedForInputMode(mode);", "spinner record"),
        ("settings.button_map_mode = defaultButtonMapModeForInputMode(mode);", "button-map record"),
        ("settings.n64_z_mode = defaultZButtonModeForInputMode(mode);", "Z-button record"),
        ("settings.gamecube_l_switch_mode = GAMECUBE_L_SWITCH_ZL;", "GameCube ZL default"),
        ("settings.analog_mouse_mode = ANALOG_MOUSE_OFF;", "Analog mouse Off default"),
        ("settings.analog_mouse_speed = ANALOG_MOUSE_SPEED_DEFAULT;", "Analog mouse pointer-speed default"),
    ):
        require(per_mode, token, label)

    require(
        registry,
        "offsetof(PerModeSettingsRecord, gamecube_l_switch_mode),    GAMECUBE_L_SWITCH_ZL",
        "GameCube L shoulder registry default",
    )
    require(
        registry,
        "offsetof(PerModeSettingsRecord, analog_mouse_mode), ANALOG_MOUSE_OFF",
        "Analog mouse mode registry default",
    )

    require(
        registry,
        "offsetof(PerModeSettingsRecord, analog_mouse_speed), ANALOG_MOUSE_SPEED_DEFAULT, 1, ANALOG_MOUSE_SPEED_MAX",
        "Analog mouse pointer-speed registry default and range",
    )

    require(default_modes, "#define DEFAULT_INPUT_MODE  RZORD_AUTODETECT",
            "Factory input Auto")
    require(
        registry,
        "offsetof(GlobalSettingsRecord, configured_output_mode), (int32_t)OUTPUT_AUTO",
        "Factory output Auto",
    )
    require(runner, '"tools/test_factory_defaults.py",',
            "Mandatory release-check registration")

    print("OK: Classic2USB factory defaults are pinned")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
