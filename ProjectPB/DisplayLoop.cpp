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
#include "FrameDropMonitor.h"
#include "DisplayDiagnostics.h"
#include <GL/glut.h>

std::unique_ptr<vmlab::DrawVideo> VideoMode;

bool init(void) {
	if (glewInit() != GLEW_OK) {
		printf("[FATAL] GLEW could not be initialised.\n");
		return false;
	}
	typedef BOOL(WINAPI* PFNWGLSWAPINTERVALEXTPROC)(int);

	PFNWGLSWAPINTERVALEXTPROC pWglSwapIntervalEXT =
		(PFNWGLSWAPINTERVALEXTPROC)wglGetProcAddress("wglSwapIntervalEXT");

	if (pWglSwapIntervalEXT) {
		pWglSwapIntervalEXT(1);
	}
	else {
		printf("[WARN] V-Sync extension not available.\n");
	}

	if (!InitInterleaveRenderer()) return false;
	PreloadStillImages();
	ht = 0;
	printf("head-tracking: OFF\n");
	printf("haba = %d\n", haba);
	InitFrameDropMonitor();
	return true;
}

// Aggregate timings only; never print once per frame.
// A single Arduino write longer than this is reported. One byte at 115200 baud
// is ~0.09 ms on the wire and a USB full-speed frame is 1 ms, so a longer
// wait means the driver or the Arduino side held the render thread.
const double SerialWarnMs = 1.0;

void ReportFrameTiming(FrameClock::time_point begin, FrameClock::time_point beforeSwap,
    FrameClock::time_point afterSwap, FrameClock::time_point beforeSerial,
    FrameClock::time_point afterSerial)
{
    static auto lastReport = FrameClock::now();
    static unsigned frames = 0;
    static double drawMs = 0, swapMs = 0, serialMs = 0, serialMaxMs = 0;
    static unsigned slowSerial = 0;
    static bool warnedSerial = false;
    static FrameClock::time_point lastSerialWarning;
    auto ms = [](FrameClock::time_point a, FrameClock::time_point b) {
        return std::chrono::duration<double, std::milli>(b-a).count();
    };
    const double thisSerialMs = ms(beforeSerial, afterSerial);
    ++frames;
    drawMs += ms(begin, beforeSwap);
    swapMs += ms(beforeSwap, afterSwap);
    serialMs += thisSerialMs;
    if (thisSerialMs > serialMaxMs) serialMaxMs = thisSerialMs;
    if (thisSerialMs > SerialWarnMs) {
        ++slowSerial;
        if (!warnedSerial || afterSerial - lastSerialWarning >= std::chrono::seconds(1)) {
            printf("[SERIAL] Arduino write took %.3f ms (limit %.1f ms), kk=%d\n",
                thisSerialMs, SerialWarnMs, displayedKk);
            lastSerialWarning = afterSerial;
            warnedSerial = true;
        }
    }
    const double elapsed = std::chrono::duration<double>(afterSerial-lastReport).count();
    if (elapsed >= 2.0) {
        printf("[PERF] %.1f FPS | draw %.2f ms | swap %.2f ms | serial avg %.3f / max %.3f ms (slow %u) | dropped %u (total %llu) | %s\n",
            frames/elapsed, drawMs/frames, swapMs/frames, serialMs/frames, serialMaxMs, slowSerial,
            TakeDroppedFrames(), TotalDroppedFrames(),
            frameHoldMode ? "HOLD" : "RUN");
        frames = 0; drawMs = swapMs = serialMs = serialMaxMs = 0; slowSerial = 0; lastReport = afterSerial;
    }
}

void disp(void) {
    const auto frameBegin = FrameClock::now();
    UpdateTracking();

	// V-Sync is the frame clock. Advance video exactly once for the frame that
	// will now be presented instead of using an independent 120 Hz timer.
	if (!frameHoldMode && VideoSwitch && mrk == 1 && VideoMode) {
		VideoMode->Update(0);
	}

	// Only a full-screen quad is drawn, so no depth or stencil buffer is used.
	glDrawBuffer(GL_BACK);
	glClear(GL_COLOR_BUFFER_BIT);
	if (flag == 1) {
		// Keep the exact kk that is about to be presented. Key input can then
		// freeze this frame instead of the next frame in the sequence.
		displayedKk = kk;


		if (mrk == 1) {
			// 動画表示モード。



			glViewport(0, 0, IM_W, IM_H);
			renderInterleavedVideo(VideoMode->getVideoTextureID()); // 動画を描画する。

		}
		else {
			// 静止画表示モード。

			// 静止画用シェーダーで描画する。
			glViewport(0, 0, IM_W, IM_H);
			renderInterleavedImage();
		}

		// kk is advanced once after each V-Sync presentation.
	}

	else {
		// Images are preloaded; this only selects the textures.
		if (!SelectStillImage(img)) img = CurrentStillImage();

		qq = 0;
		flag = 1;
		// Texture preparation is not a presented time-division frame. Keep the
		// current front buffer and render kk=0 on the next display callback.
		// The load stalls on purpose, so it is not counted as a dropped frame.
		ResetFrameDropBaseline();
		glutPostRedisplay();
		return;
	}
	const auto beforeSwap = FrameClock::now();
	glutSwapBuffers();
	const auto afterSwap = FrameClock::now();

	// Print the environment once, after the window has its final placement.
	static bool diagnosticsPrinted = false;
	if (!diagnosticsPrinted) {
		diagnosticsPrinted = true;
		PrintDisplayDiagnostics();
	}

	// A held frame is intentionally shown for many refreshes.
	if (frameHoldMode) ResetFrameDropBaseline();
	else RecordPresentedFrame(afterSwap);

	// Measure only the Arduino write itself.
	const auto beforeSerial = FrameClock::now();
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

	const auto afterSerial = FrameClock::now();

	if (!frameHoldMode) ReportFrameTiming(frameBegin, beforeSwap, afterSwap, beforeSerial, afterSerial);

	// 提示したフレームに対応する時分割位相を進める。
	if (running && !frameHoldMode) {
		kk = (kk + 1) % 4;
		// Queue exactly one next draw. glutSwapBuffers() above waits for V-Sync,
		// so display phase and Arduino command cannot drift against a timer.
		glutPostRedisplay();
	}
}
