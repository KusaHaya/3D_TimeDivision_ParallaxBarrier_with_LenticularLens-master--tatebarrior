#define _CRT_SECURE_NO_WARNINGS
#define NOMINMAX

// 起動処理だけを置く。各機能は次のファイルにある。
//   AppConfig.h / AppState.*  : 固定値と共有状態
//   ArduinoSerial.*           : Arduinoとのシリアル通信
//   TimeDivision.*            : 時分割の位相同期とフレーム停止
//   HeadTracking.*            : 目の位置によるバリア補正
//   StillImage.*              : 静止画の読み込み
//   ShaderLoader.*            : シェーダーのコンパイル
//   InterleaveRenderer.*      : 左右画像の合成描画
//   DisplayLoop.*             : 表示コールバック（1フレームの流れ）
//   KeyInput.*                : キーボード操作

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <string>
#include <stdio.h>
#include <windows.h>
#include <mmsystem.h>
#include "AppState.h"
#include "ArduinoSerial.h"
#include "HeadTracking.h"
#include "DisplayLoop.h"
#include "KeyInput.h"
#include <GL/glut.h>

#pragma comment(lib, "winmm.lib")


FILE _iob[] = { *stdin, *stdout, *stderr };

extern "C" FILE * __cdecl __iob_func(void)
{
	return _iob;
}


int main(int argc, char ** argv) {
    if (const char* gainText = std::getenv("PROJECTPB_HORIZONTAL_GAIN")) {
        char* end = nullptr;
        const float gain = std::strtof(gainText, &end);
        if (end != gainText && *end == '\0' && std::isfinite(gain) && gain >= 0.1f && gain <= 10.0f)
            horizontalTrackingGain = gain;
        else std::cerr << "Invalid PROJECTPB_HORIZONTAL_GAIN; using 0.7.\n";
    }
    printf("Horizontal tracking gain: %.2f (j decreases, Shift+j increases)\n", horizontalTrackingGain);

	MMRESULT timerResult = timeBeginPeriod(1);
	if (timerResult != TIMERR_NOERROR) {
		printf("[WARN] Failed to set 1ms timer resolution.\n");
	}


    const char* serialPort = std::getenv("PROJECTPB_SERIAL_PORT");
    if (!serialPort || !*serialPort) serialPort = "COM3";
    std::string serialError;
    if (!OpenArduinoSerial(serialPort, serialError)) {
        std::cerr << "Cannot initialize Arduino on " << serialPort << ": "
                  << serialError << "\nEdit PROJECTPB_SERIAL_PORT in Start.cmd.\n";
        timeEndPeriod(1);
        return 1;
    }
	SendArduinoCommand(RESET_SYNC);
	SendArduinoCommand(ENABLE_TIMEDIVISION);
	glutInit(&argc, argv);
	glutInitWindowPosition(0, 0);
	glutInitWindowSize(IM_W, IM_H);

	glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH | GLUT_STENCIL | GLUT_STEREO);
	glutCreateWindow("Test");

	VideoMode = std::unique_ptr<vmlab::DrawVideo>(new vmlab::DrawVideo);
	VideoSwitch = false;

	init();
    StartHeadTracking();
	glutDisplayFunc(disp);
	RegisterKeyHandlers();
	//glutIdleFunc(disp);

	glutMainLoop();
	StopHeadTracking();
	timeEndPeriod(1);

	glDeleteTextures(1, &imageL);
	glDeleteTextures(1, &imageR);

	return 0;
}
