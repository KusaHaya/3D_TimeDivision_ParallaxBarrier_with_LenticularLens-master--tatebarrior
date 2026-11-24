#define _CRT_SECURE_NO_WARNINGS
#define NOMINMAX

#include <chrono>
#include <stdio.h>
#include <windows.h>
#include "AppState.h"
#include "HeadTracking.h"
#include "InterleaveRenderer.h"
#include "StillImage.h"
#include "TimeDivision.h"
#include "DisplayLoop.h"
#include <GL/glut.h>

std::unique_ptr<vmlab::DrawVideo> VideoMode;

void init(void) {
	glewInit();
	typedef BOOL(WINAPI* PFNWGLSWAPINTERVALEXTPROC)(int);

	PFNWGLSWAPINTERVALEXTPROC pWglSwapIntervalEXT =
		(PFNWGLSWAPINTERVALEXTPROC)wglGetProcAddress("wglSwapIntervalEXT");

	if (pWglSwapIntervalEXT) {
		pWglSwapIntervalEXT(1);
	}
	else {
		printf("[WARN] V-Sync extension not available.\n");
	}
	anmode = 0;

	glGenTextures(1, &imageL);
	glGenTextures(1, &imageR);
	InitInterleaveRenderer();
	ht = 0;
	printf("head-tracking: OFF\n");
	printf("mode : SS\n");
	printf("haba = %d\n", haba);
}

// Aggregate timings only; never print once per frame.
using FrameClock = std::chrono::steady_clock;
void ReportFrameTiming(FrameClock::time_point begin, FrameClock::time_point beforeSwap,
    FrameClock::time_point afterSwap, FrameClock::time_point afterSerial)
{
    static auto lastReport = FrameClock::now();
    static unsigned frames = 0;
    static double drawMs = 0, swapMs = 0, serialMs = 0;
    auto ms = [](FrameClock::time_point a, FrameClock::time_point b) {
        return std::chrono::duration<double, std::milli>(b-a).count();
    };
    ++frames;
    drawMs += ms(begin, beforeSwap);
    swapMs += ms(beforeSwap, afterSwap);
    serialMs += ms(afterSwap, afterSerial);
    const double elapsed = std::chrono::duration<double>(afterSerial-lastReport).count();
    if (elapsed >= 2.0) {
        printf("[PERF] %.1f FPS | draw %.2f ms | swap %.2f ms | serial %.2f ms | %s\n",
            frames/elapsed, drawMs/frames, swapMs/frames, serialMs/frames,
            frameHoldMode ? "HOLD" : "RUN");
        frames = 0; drawMs = swapMs = serialMs = 0; lastReport = afterSerial;
    }
}

void disp(void) {
    const auto frameBegin = FrameClock::now();
    UpdateTracking();
	int i, e;

	// V-Sync is the frame clock. Advance video exactly once for the frame that
	// will now be presented instead of using an independent 120 Hz timer.
	if (!frameHoldMode && VideoSwitch && mrk == 1 && VideoMode) {
		VideoMode->Update(0);
	}

	glDrawBuffer(GL_BACK);
	glEnable(GL_DEPTH_TEST);
	glDisable(GL_STENCIL_TEST);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
	if (flag == 1) {
		// Keep the exact kk that is about to be presented. Key input can then
		// freeze this frame instead of the next frame in the sequence.
		displayedKk = kk;


		glDisable(GL_TEXTURE_2D);

		if (mrk == 1) {
			// 動画表示モード。



			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			glViewport(0, 0, IM_W, IM_H);
			renderInterleavedVideo(VideoMode->getVideoTextureID()); // 動画を描画する。
		//	glUniform1f(glGetUniformLocation(videoshaderProgram, "middleLinePx"), (float)MiddleLine / 3.0f);

			glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

		}
		else {
			// 静止画表示モード。

			// 静止画用シェーダーで描画する。
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // 描画バッファーをクリアする。
			glViewport(0, 0, IM_W, IM_H);
			renderInterleavedImage();
			//	glUniform1f(glGetUniformLocation(shaderProgram, "middleLinePx"), (float)MiddleLine / 3.0f);

			glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
		}

		// kk is advanced once after each V-Sync presentation.
	}

	else {
		LoadStillImageTextures();

		qq = 0;
		flag = 1;
		// Texture preparation is not a presented time-division frame. Keep the
		// current front buffer and render kk=0 on the next display callback.
		glutPostRedisplay();
		return;
	}
	const auto beforeSwap = FrameClock::now();
	glutSwapBuffers();
	const auto afterSwap = FrameClock::now();

	// Send the hold command only after the selected display frame has been
	// presented, then leave that backlight pair continuously on.
	if (frameHoldMode && holdCommandPending) {
		SendHeldFrame(displayedKk);
		holdCommandPending = false;
	}
	else if (!frameHoldMode) {
		// Send one command for every frame that has just been presented.
		SendTimeDivisionFrame(displayedKk);
	}

	if (!frameHoldMode) ReportFrameTiming(frameBegin, beforeSwap, afterSwap, FrameClock::now());

	// 提示したフレームに対応する時分割位相を進める。
	if (running && !frameHoldMode) {
		kk = (kk + 1) % 4;
		// Queue exactly one next draw. glutSwapBuffers() above waits for V-Sync,
		// so display phase and Arduino command cannot drift against a timer.
		glutPostRedisplay();
	}
}
