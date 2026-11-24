#define _CRT_SECURE_NO_WARNINGS
#define NOMINMAX

#include <stdio.h>
#include <windows.h>
#include "AppConfig.h"
#include "FrameDropMonitor.h"

namespace {
int refreshHz = REFRESH_RATE;
double periodMs = 1000.0 / REFRESH_RATE;

// A long interval alone is not proof of a missed refresh: the driver can
// return late from one swap and early from the next while the display keeps
// its pace. A long interval opens a short window; shorter intervals inside it
// pay back the excess, and only the time still missing at the end counts.
const double LongIntervalFactor = 1.5;
const int CompensationFrames = 4;
// Drops while the window and driver settle are reported separately.
const double StartupSeconds = 5.0;

bool hasPrevious = false;
FrameClock::time_point previousFrame;
bool startTimeKnown = false;
FrameClock::time_point startTime;

bool pending = false;
double pendingExcessMs = 0;
double pendingLongestMs = 0;
int pendingFramesLeft = 0;
FrameClock::time_point pendingAt;

unsigned droppedSinceReport = 0;
unsigned long long droppedTotal = 0;
unsigned long long startupDropped = 0;
bool startupReported = false;
bool warned = false;
FrameClock::time_point lastWarning;

void ResolvePending()
{
	pending = false;
	const int missed = (int)(pendingExcessMs / periodMs + 0.5);
	if (missed <= 0) return;

	const bool startup = std::chrono::duration<double>(pendingAt - startTime).count() < StartupSeconds;
	if (startup) {
		startupDropped += missed;
		return;
	}
	droppedSinceReport += missed;
	droppedTotal += missed;
	// Print at most once per second; the [PERF] line keeps the full count.
	if (!warned || pendingAt - lastWarning >= std::chrono::seconds(1)) {
		printf("[DROP] %d frame(s) missed: longest gap %.2f ms (expected %.2f ms), total %llu\n",
			missed, pendingLongestMs, periodMs, droppedTotal);
		lastWarning = pendingAt;
		warned = true;
	}
}
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
	pending = false;
}

void RecordPresentedFrame(FrameClock::time_point presentedAt)
{
	if (!startTimeKnown) {
		startTime = presentedAt;
		startTimeKnown = true;
	}
	if (!startupReported && std::chrono::duration<double>(presentedAt - startTime).count() >= StartupSeconds) {
		startupReported = true;
		printf("[DROP] startup: %llu frame(s) missed in the first %.0f s (not counted in total).\n",
			startupDropped, StartupSeconds);
	}
	if (hasPrevious) {
		const double ms = std::chrono::duration<double, std::milli>(presentedAt - previousFrame).count();
		if (pending) {
			if (ms > LongIntervalFactor * periodMs) {
				pendingExcessMs += ms - periodMs;
				if (ms > pendingLongestMs) pendingLongestMs = ms;
			}
			else if (ms < periodMs) {
				pendingExcessMs -= periodMs - ms;
			}
			if (--pendingFramesLeft <= 0) ResolvePending();
		}
		else if (ms > LongIntervalFactor * periodMs) {
			pending = true;
			pendingExcessMs = ms - periodMs;
			pendingLongestMs = ms;
			pendingFramesLeft = CompensationFrames;
			pendingAt = presentedAt;
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
