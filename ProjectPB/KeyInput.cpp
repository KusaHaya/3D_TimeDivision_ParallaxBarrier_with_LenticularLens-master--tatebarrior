#define _CRT_SECURE_NO_WARNINGS
#define NOMINMAX

#include <algorithm>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <mmsystem.h>
#include "AppState.h"
#include "ArduinoSerial.h"
#include "TimeDivision.h"
#include "DisplayLoop.h"
#include "KeyInput.h"
#include "AppShutdown.h"
#include "Settings.h"
#include "FrameDropMonitor.h"
#include "StillImage.h"
#include <GL/glut.h>

// Refresh manual adjustments immediately, including while a frame is held.
void RefreshBarrierAlignment()
{
    printf("[ALIGN] haba=%d SHIFT=%d slantY=%.3f threshold=%.2f\n",
        haba, SHIFT, barrierSlantY, barrierThresholdStep / 20.0f);
    glutPostRedisplay();
}

void AdjustBarrierWidth(int amount)
{
    const int previous = haba;
    haba = std::max(2, std::min(100000, haba + amount));
    if (ht) trackingBaseHaba += haba - previous;
    RefreshBarrierAlignment();
}

static void HandleKey(unsigned char key);

static void KeyEvent(unsigned char key, int x, int y) {
	HandleKey(key);
	UpdateStatusTitle();
}

static void HandleKey(unsigned char key) {
	switch (key) {
	case 27:
		ShutdownApp();
		exit(0);
		break;
	case 'q':
		SelectFrameWhileHeld(0);
		break;
	case 'w':
		SelectFrameWhileHeld(1);
		break;
	case 'e':
		SelectFrameWhileHeld(2);
		break;
	case 'r':
		SelectFrameWhileHeld(3);
		break;
	case 't':
		if (!frameHoldMode) {
			// kk already points to the next frame after swap, so hold the
			// frame that is actually visible instead.
			HoldFrameAndBacklight(displayedKk);
		}
		else {
			frameHoldMode = false;
			holdCommandPending = false;
			running = 1;
			kk = 0;
			SendArduinoCommand(RESET_SYNC);
			SendArduinoCommand(ENABLE_TIMEDIVISION);
			printf("frame hold: OFF (restart from kk = 0)\n");
			glutPostRedisplay();
		}
		break;
    case 'j':
        horizontalTrackingGain = std::max(0.1f, horizontalTrackingGain - 0.1f);
        printf("Horizontal tracking gain: %.2f\n", horizontalTrackingGain);
        glutPostRedisplay();
        break;
    case 'J':
        horizontalTrackingGain = std::min(10.0f, horizontalTrackingGain + 0.1f);
        printf("Horizontal tracking gain: %.2f\n", horizontalTrackingGain);
        glutPostRedisplay();
        break;
	case 'o':
		SHIFT += 1;
		RefreshBarrierAlignment();
		break;
	case 'O':
		SHIFT -= 1;
		RefreshBarrierAlignment();
		break;
	case 'u':
		barrierThresholdStep = std::max(1, barrierThresholdStep - 1);
		RefreshBarrierAlignment();
		break;
	case 'U':
		barrierThresholdStep = std::min(20, barrierThresholdStep + 1);
		RefreshBarrierAlignment();
		break;
	case 'C':
		showAlignmentCenter = !showAlignmentCenter;
		printf("[CENTER] %s | reference at mid-height: x=%.2f px, y=%.2f px | MiddleLine=%d\n",
			showAlignmentCenter ? "ON (diagnostic; disable for measurement)" : "OFF",
			(MiddleLine - barrierSlantY * (IM_H * 0.5f)) / 3.0f,
			IM_H * 0.5f, MiddleLine);
		glutPostRedisplay();
		break;
	case 'h':
		if (ht == 0 && cali_flag == 0) {
			cali_flag = 1;
		}
		else {
			ht = 0;
			cali_flag = 0;
			// Freeze the current alignment instead of restoring defaults.
			printf("head-tracking: OFF (alignment preserved)\n");
		}
		glutDisplayFunc(disp);
		glutPostRedisplay();
		break;
	case 'a':
		AdjustBarrierWidth(1);
		break;
	case 's':
		VideoMode->SetSpeed(200);
		break;
	case '1':
		img = 2;
		flag = 0;
		glutDisplayFunc(disp);
		break;
	case '2':
		img = 3;
		flag = 0;
		glutDisplayFunc(disp);
		break;
	case '3':
		img = 4;
		flag = 0;
		glutDisplayFunc(disp);
		break;
	case '4':
		img = 5;
		flag = 0;
		glutDisplayFunc(disp);
		break;
	case '5':
		img = 6;
		flag = 0;
		glutDisplayFunc(disp);
		break;
	case '6':
		img = 7;
		flag = 0;
		glutDisplayFunc(disp);
		break;
	case '7':
		img = 8;
		flag = 0;
		glutDisplayFunc(disp);
		break;
	case '8':
		// Red left eye / blue right eye for optical timing measurements.
		img = 11;
		mrk = 0;
		flag = 0;
		glutDisplayFunc(disp);
		glutPostRedisplay();
		break;
	case '0':
		img = 1;
		flag = 0;
		glutDisplayFunc(disp);
		break;
	case 'm':
		mrk = 1;
		flag = 1;

		if (!VideoSwitch)
		{
			VideoMode->Initialize();

			// 動画の初期化に失敗した場合は終了する。
			Sleep(100);

			VideoMode->SetSpeed(16000);

			// 初期化後に動画表示へ切り替える。
			VideoSwitch = true;
		}

		RestartTimeDivisionSync("video");
		glutPostRedisplay();
		break;
	case 'M':
		mrk = 0;
		flag = 0;
		RestartTimeDivisionSync("still image");
		glutPostRedisplay();
		break;
	case 'b':
		eyeright = 1;
		glutDisplayFunc(disp);
		glutPostRedisplay();
		break;
	case 'B':
		eyeright = 0;
		glutDisplayFunc(disp);
		glutPostRedisplay();
		break;
	case 'v':
		eyeleft = 1;
		glutDisplayFunc(disp);
		glutPostRedisplay();
		break;
	case 'V':
		eyeleft = 0;
		glutDisplayFunc(disp);
		glutPostRedisplay();
		break;
	case 'P':
		SaveSettings();
		break;
	case '?':
	case '/':
		PrintKeyHelp();
		PrintStatus();
		break;
	case '[':
		AdjustBarrierWidth(1);
		break;
	case ']':
		AdjustBarrierWidth(-1);
		break;
	case 'k':
		kk = (kk + 1) % 4;
		break;
	case 'K':
		printf("z: %f, haba: %d\n", tmpZ, haba);
		break;
	case 'f':
		barrierSlantY -= 0.02f;
		if (barrierSlantY < 0.1f) barrierSlantY = 0.1f;
		printf("barrierSlantY = %.3f, angle = %.3f deg\n",
			barrierSlantY,
			atan(-3.0f / barrierSlantY) * 180.0f / 3.14159265f);
		glutPostRedisplay();
		break;

	case 'F':
		barrierSlantY += 0.02f;
		printf("barrierSlantY = %.3f, angle = %.3f deg\n",
			barrierSlantY,
			atan(-3.0f / barrierSlantY) * 180.0f / 3.14159265f);
		glutPostRedisplay();
		break;

	}
}
static void KeyUp(unsigned
	char key, int x, int y) {
	switch (key) {
	case 's':
		VideoMode->SetSpeed(9000);
		break;
	}
}
void PrintKeyHelp()
{
	printf(
		"---- ProjectPB keys ------------------------------------------------------\n"
		" ESC        quit (backlight returns to all-on)\n"
		" 0-7, 8     still image (8 = red left / blue right timing pair)\n"
		" m / M      video / back to still image (restarts time-division at kk=0)\n"
		" t          hold the visible phase; again to resume\n"
		" q w e r    while held: show phase 0 1 2 3\n"
		" k          advance the display phase by one\n"
		" [ ] (a)    barrier width haba -1 / +1 ('a' = '[' kept for habit)\n"
		" o / O      phase shift SHIFT +1 / -1\n"
		" f / F      slant slantY -0.02 / +0.02\n"
		" u / U      correction threshold -0.05 / +0.05\n"
		" C          alignment centre guide on/off (turn off for measurements)\n"
		" v / V      left eye on / off      b / B   right eye on / off\n"
		" h          head tracking: set reference and start / stop\n"
		" j / J      horizontal tracking gain -0.1 / +0.1\n"
		" K          print tracking distance and haba\n"
		" s (hold)   video fast-forward while pressed\n"
		" P          save the alignment to ProjectPB_settings.ini\n"
		" ? or /     this list and the current status\n"
		"--------------------------------------------------------------------------\n");
}

void PrintStatus()
{
	printf("[STATUS] %s kk=%d | %s | tracking %s | haba=%d middle_line=%d shift=%d slant_y=%.3f threshold=%.2f gain=%.2f | eyes L%s R%s | guide %s | dropped total %llu\n",
		frameHoldMode ? "HOLD" : "RUN", displayedKk,
		mrk == 1 ? "video" : "still",
		ht == 1 ? "ON" : "OFF",
		haba, MiddleLine, SHIFT, barrierSlantY, barrierThresholdStep / 20.0f, horizontalTrackingGain,
		eyeleft ? "on" : "off", eyeright ? "on" : "off",
		showAlignmentCenter ? "ON" : "off",
		TotalDroppedFrames());
	if (mrk != 1) printf("[STATUS] still image %d\n", CurrentStillImage());
}

void UpdateStatusTitle()
{
	char title[160];
	sprintf(title, "ProjectPB | %s | %s | track %s | haba %d | shift %d | slant %.2f | thr %.2f",
		frameHoldMode ? "HOLD" : "RUN", mrk == 1 ? "video" : "still",
		ht == 1 ? "ON" : "OFF", haba, SHIFT, barrierSlantY, barrierThresholdStep / 20.0f);
	glutSetWindowTitle(title);
}

void RegisterKeyHandlers()
{
	glutKeyboardFunc(KeyEvent);
	glutKeyboardUpFunc(KeyUp);
}
