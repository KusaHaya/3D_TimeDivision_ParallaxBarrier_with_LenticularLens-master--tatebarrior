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

static void KeyEvent(unsigned char key, int x, int y) {
	switch (key) {
	case 27:

		while (!VideoMode->video_flag) VideoMode->dispose();
		if (IsArduinoOpen()) {
			SendArduinoCommand(DISABLE_TIMEDIVISION);
			SendArduinoCommand(LIGHT_EXIT);
		}
		timeEndPeriod(1);
		exit(0);
		break;
	case 'Z':
		anmode = 0;
		flag = 0;
		glutDisplayFunc(disp);
		break;
	case 'A':
		anmode = 1;
		flag = 0;
		glutDisplayFunc(disp);
		break;
	case 'Q':
		anmode = 2;
		flag = 0;
		glutDisplayFunc(disp);
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
	case'z':
		if (random == 0) {
			mode = 0;
			printf("mode : PP\n");
		}
		else if (random == 1) {
			mode = 0;
			printf("mode : PP\n");
		}
		else if (random == 2) {
			mode = 2;
		}
		else {
			mode = 1;
			printf("mode : SP\n");
		}
		glutDisplayFunc(disp);
		break;
	case'x':
		if (random == 0) {
			mode = 1;
			printf("mode : SP\n");
		}
		else if (random == 1) {
			mode = 2;
			printf("mode : SS\n");
		}
		else if (random == 2) {
			mode = 0;
			printf("mode : PP\n");
		}
		else {
			mode = 2;
			printf("mode : SS\n");
		}
		glutDisplayFunc(disp);
		break;
	case'c':
		if (random == 0) {
			mode = 2;
			printf("mode : SS\n");
		}
		else if (random == 1) {
			mode = 1;
			printf("mode : SP\n");
		}
		else if (random == 2) {
			mode = 1;
			printf("mode : SP\n");
		}
		else {
			mode = 0;
			printf("mode : PP\n");
		}
		glutDisplayFunc(disp);
		break;
	case 'L':
		LiverMode = true;
		break;
	case 'l':
		LiverMode = false;
		break;
	case '1':
		img = 2;
		random = 0;
		flag = 0;
		glutDisplayFunc(disp);
		break;
	case '2':
		img = 3;
		random = 1;
		flag = 0;
		glutDisplayFunc(disp);
		break;
	case '3':
		img = 4;
		random = 2;
		flag = 0;
		glutDisplayFunc(disp);
		break;
	case '4':
		img = 5;
		random = 3;
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
		random = 0;
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
	case 'S':
		VideoMode->Mode3D = !VideoMode->Mode3D;
		glutDisplayFunc(disp);
		break;
		/*case 'p':
			VideoMode->printflag = false;
			glutDisplayFunc(disp);
			break;*/
	case 'R':
		ReserveLR = !ReserveLR;
		if (ReserveLR) printf("RL\n");
		else printf("LR\n");
		glutDisplayFunc(disp);
		break;
		//case 'f':
		//	zsft += 5;
		//	glutDisplayFunc(disp);
		//	break;
		//case 'F':
		//	zsft -= 5;
		//	glutDisplayFunc(disp);
		//	break;
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
	case'd':
		alphax += 0.1;
		if (alphax > 1.1)
			alphax = 0;
		break;
	case'D':
		habat += 1;
		break;
		// キーボードイベントを動画処理へ渡す。
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
static void KeySpecialEvent(int key, int x, int y) {
	if (key == GLUT_KEY_LEFT) {
		theta = (int)(theta + 1) % 360;
		glutDisplayFunc(disp);
	}
	if (key == GLUT_KEY_RIGHT) {
		theta = (int)(theta - 1 + 360) % 360;
		glutDisplayFunc(disp);
	}

}

void RegisterKeyHandlers()
{
	glutKeyboardFunc(KeyEvent);
	glutKeyboardUpFunc(KeyUp);
	glutSpecialFunc(KeySpecialEvent);
}
