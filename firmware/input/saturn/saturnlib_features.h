#pragma once

// SaturnLib defines controller layouts and inline protocol readers differently
// for these feature switches. Every translation unit that includes SaturnLib
// must see the same values or the linker can select an incompatible reader.
#ifndef SATLIB_ENABLE_8BITDO_HOME_BTN
#define SATLIB_ENABLE_8BITDO_HOME_BTN
#endif

#ifndef SATLIB_ENABLE_MISSION6
#define SATLIB_ENABLE_MISSION6
#endif

#ifndef SATLIB_ENABLE_MEGATAP
#define SATLIB_ENABLE_MEGATAP
#endif

#ifndef SATLIB_ENABLE_SATTAP
#define SATLIB_ENABLE_SATTAP
#endif
