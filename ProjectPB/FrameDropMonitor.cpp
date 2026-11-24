#define _CRT_SECURE_NO_WARNINGS
#define NOMINMAX

#include <stdio.h>
#include <windows.h>
#include "AppConfig.h"
#include "FrameDropMonitor.h"

namespace {
int refreshHz = REFRESH_RATE;
double periodMs = 1000.0 / REFRESH_RATE;
bool hasPrevious = false;
FrameClock::time_point previousFrame;
unsigned droppedSinceReport = 0;
unsigned long long droppedTotal = 0;
bool warned = false;
FrameClock::time_point lastWarning;
}

void InitFrameDropMonitor()
{
	DEVMODEA mode = {};
	mode.dmSize = sizeof(mode);
	// The window is created at (0, 0), i.e. on the primary display.
	if (EnumDisplaySettingsA(nullptr, ENUM_CURRENT_SETTINGS, &mode) && mode.dmDisplayFrequency > 1) {
		refreshHz = (int)mode.dmDisplayFrequency;
	}
	else {
		printf("[WARN] Could not read the display refresh rate; assuming %d Hz.\n", REFRESH_RATE);
	}
	periodMs = 1000.0 / refreshHz;
	printf("Display refresh: %d Hz. REFRESH_RATE in the Arduino sketch must also be %d.\n",
		refreshHz, refreshHz);
	if (refreshHz != 120 && refreshHz != 144) {
		printf("[WARN] The Arduino sketch has timing tables only for 120 Hz and 144 Hz.\n");
	}
}

int DisplayRefreshHz()
{
	return refreshHz;
}

void ResetFrameDropBaseline()
{
	hasPrevious = false;
}

void RecordPresentedFrame(FrameClock::time_point presentedAt)
{
	if (hasPrevious) {
		const double ms = std::chrono::duration<double, std::milli>(presentedAt - previousFrame).count();
		// Allow half a refresh of jitter before counting a missed refresh.
		const unsigned missed = ms > 1.5 * periodMs ? (unsigned)(ms / periodMs + 0.5) - 1 : 0;
		if (missed > 0) {
			droppedSinceReport += missed;
			droppedTotal += missed;
			// Print at most once per second; the [PERF] line keeps the full count.
			if (!warned || presentedAt - lastWarning >= std::chrono::seconds(1)) {
				printf("[DROP] %u frame(s) missed: %.2f ms between frames (expected %.2f ms), total %llu\n",
					missed, ms, periodMs, droppedTotal);
				lastWarning = presentedAt;
				warned = true;
			}
		}
	}
	previousFrame = presentedAt;
	hasPrevious = true;
}

unsigned TakeDroppedFrames()
{
	const unsigned dropped = droppedSinceReport;
	droppedSinceReport = 0;
	return dropped;
}

unsigned long long TotalDroppedFrames()
{
	return droppedTotal;
}
