#pragma once

#if defined(ADAPT_DISABLE_KONAMI_CODE)
inline void updateKonamiCodeObserver(bool) {}
#else
void updateKonamiCodeObserver(bool polled);
#endif
