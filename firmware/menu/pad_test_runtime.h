#pragma once

// No-host controller service. AUTO keeps polling the left connector while no
// USB data host is present and exposes local rumble shortcuts on the home UI.
bool noHostControllerTestActive();
bool updateNoHostControllerTest();
const char* noHostControllerRumbleHint();