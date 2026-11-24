#define _CRT_SECURE_NO_WARNINGS
#define NOMINMAX

#include <stdio.h>
#include "AppState.h"
#include "ArduinoSerial.h"
#include "TimeDivision.h"
#include "FrameDropMonitor.h"
#include <GL/glut.h>

void SendTimeDivisionFrame(int frameIndex)
{
	static const int timeDivisionCommands[4] = {
		TIME_DIV_0, TIME_DIV_1, TIME_DIV_2, TIME_DIV_3
	};

	if (!IsArduinoOpen()) return;
	if (running != 1) return;

	SendArduinoCommand(timeDivisionCommands[frameIndex & 3]);
}

void SendHeldFrame(int frameIndex)
{
	static const int holdCommands[4] = {
		HOLD_FRAME_0, HOLD_FRAME_1, HOLD_FRAME_2, HOLD_FRAME_3
	};

	if (!IsArduinoOpen()) return;
	SendArduinoCommand(holdCommands[frameIndex & 3]);
}

void RestartTimeDivisionSync(const char* modeName)
{
	frameHoldMode = false;
	holdCommandPending = false;
	running = 1;
	kk = 0;
	displayedKk = 0;
	// Mode switches (e.g. video initialisation) stall on purpose.
	ResetFrameDropBaseline();

	SendArduinoCommand(RESET_SYNC);
	SendArduinoCommand(ENABLE_TIMEDIVISION);
	printf("time-division sync reset: %s (kk = 0)\n", modeName);
}

void HoldFrameAndBacklight(int frameIndex)
{
	const int selectedFrame = frameIndex & 3;

	frameHoldMode = true;
	holdCommandPending = true;
	running = 0;
	kk = selectedFrame;
	displayedKk = selectedFrame;
	printf("frame hold: ON (kk = %d)\n", selectedFrame);
	glutPostRedisplay();
}

void SelectFrameWhileHeld(int frameIndex)
{
	if (!frameHoldMode) {
		printf("q/w/e/r are available after stopping with t.\n");
		return;
	}

	HoldFrameAndBacklight(frameIndex);
}
