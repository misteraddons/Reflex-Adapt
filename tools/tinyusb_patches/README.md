# Reflex TinyUSB Patch Set

This directory records the small Classic2USB-specific changes that sit on top
of the vendored `third_party/Adafruit_TinyUSB_Arduino` library. PlatformIO
applies the patch as generated source middleware from `tools/pio_hooks.py`, so
the checked-in vendor tree remains close to upstream.

`xinput-autodetect-descriptor-hooks.patch` adds the descriptor override and
request hooks required by Xbox 360/XInput output and automatic host detection.

When refreshing the vendored TinyUSB library, build Classic2USB from a clean
tree. If a patch anchor no longer applies, port the intent forward and update
both the middleware and this tracked patch.
