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
//   FrameDropMonitor.*        : コマ落ち検出
//   AppShutdown.*             : 終了処理
//   Settings.*                : 調整値の保存と読み込み
//   RenderPriority.*          : 描画スレッドの優先度

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
#include "AppShutdown.h"
#include "Settings.h"
#include "RenderPriority.h"
#include <GL/freeglut.h>

#pragma comment(lib, "winmm.lib")


FILE _iob[] = { *stdin, *stdout, *stderr };

extern "C" FILE * __cdecl __iob_func(void)
{
	return _iob;
}


int main(int argc, char ** argv) {
    // Saved alignment first; an explicit environment variable below still wins.
    LoadSettings();
    if (const char* gainText = std::getenv("PROJECTPB_HORIZONTAL_GAIN")) {
        char* end = nullptr;
        const float gain = std::strtof(gainText, &end);
        if (end != gainText && *end == '\0' && std::isfinite(gain) && gain >= 0.1f && gain <= 10.0f)
            horizontalTrackingGain = gain;
        else std::cerr << "Invalid PROJECTPB_HORIZONTAL_GAIN; keeping " << horizontalTrackingGain << ".\n";
    }
    printf("Horizontal tracking gain: %.2f (j decreases, Shift+j increases)\n", horizontalTrackingGain);
    PrintSettings("startup");

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

	// Only a full-screen quad is drawn: no depth, stencil or quad-buffer stereo.
	glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE);
	glutCreateWindow("Test");
	// Closing the window returns from glutMainLoop() and runs ShutdownApp().
	glutSetOption(GLUT_ACTION_ON_WINDOW_CLOSE, GLUT_ACTION_GLUTMAINLOOP_RETURNS);
	glutCloseFunc(ShutdownApp);

	VideoMode = std::unique_ptr<vmlab::DrawVideo>(new vmlab::DrawVideo);
	VideoSwitch = false;

	if (!init()) {
		ShutdownApp();
		return 1;
	}
	// After init: shader compilation and image loading need no priority boost.
	RaiseRenderThreadPriority();
	PrintKeyHelp();
	UpdateStatusTitle();
    StartHeadTracking();
	glutDisplayFunc(disp);
	RegisterKeyHandlers();
	//glutIdleFunc(disp);

	glutMainLoop();
	ShutdownApp();
	return 0;
}
