#define _CRT_SECURE_NO_WARNINGS
#define NOMINMAX

#include <stdio.h>
#include <windows.h>
#include <mmsystem.h>
#include "AppState.h"
#include "ArduinoSerial.h"
#include "HeadTracking.h"
#include "StillImage.h"
#include "DisplayLoop.h"
#include "AppShutdown.h"
#include "RenderPriority.h"

void ShutdownApp()
{
	static bool done = false;
	if (done) return;
	done = true;

	// 動画のデコードスレッドを止める。
	if (VideoMode) {
		while (!VideoMode->video_flag) VideoMode->dispose();
	}
	// バックライトを時分割から通常の全点灯に戻す。
	if (IsArduinoOpen()) {
		SendArduinoCommand(DISABLE_TIMEDIVISION);
		SendArduinoCommand(LIGHT_EXIT);
	}
	// OSC受信スレッドを止める。
	StopHeadTracking();
	// GLコンテキストがある間にテクスチャを解放する。
	ReleaseStillImages();
	RestoreRenderThreadPriority();
	timeEndPeriod(1);
	printf("ProjectPB: shut down.\n");
}
