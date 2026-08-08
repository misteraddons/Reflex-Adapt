#pragma once

void showBootSplashScreen();
bool isBootSplashScreenVisible();
bool isBootAutoDetectPending();
void markBootSplashScreenConsumed();
void suppressBootUsbDebugInfoOnce();
void suppressBootAutoDetectSplashOnce();
void showBootTraceMarker(const char* marker);
void maybePlayResolvedBootJingle(bool autoOutputProbeBoot, bool autoOutputResolvedBoot);
void showBootUsbDebugInfo();
