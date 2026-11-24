#define _CRT_SECURE_NO_WARNINGS
#define NOMINMAX

#include <stdio.h>
#include <windows.h>
#include "AppConfig.h"
#include "DisplayDiagnostics.h"
#include <GL/glew.h>

void PrintDisplayDiagnostics()
{
#ifdef _DEBUG
	const char* configuration = "Debug";
#else
	const char* configuration = "Release";
#endif
	char exePath[MAX_PATH] = "";
	char workDir[MAX_PATH] = "";
	GetModuleFileNameA(nullptr, exePath, MAX_PATH);
	GetCurrentDirectoryA(MAX_PATH, workDir);
	printf("[ENV] build: %s | exe: %s\n", configuration, exePath);
	printf("[ENV] working directory (shader and images are read from here): %s\n", workDir);
	printf("[ENV] GPU: %s | %s | OpenGL %s\n",
		(const char*)glGetString(GL_VENDOR), (const char*)glGetString(GL_RENDERER),
		(const char*)glGetString(GL_VERSION));

	HWND window = WindowFromDC(wglGetCurrentDC());
	RECT client = {};
	POINT origin = {0, 0};
	if (window && GetClientRect(window, &client) && ClientToScreen(window, &origin)) {
		const int width = client.right - client.left;
		const int height = client.bottom - client.top;
		const UINT dpi = GetDpiForWindow(window);
		printf("[ENV] drawing area on screen: x=%ld y=%ld size=%dx%d | DPI %u (%u%%)\n",
			origin.x, origin.y, width, height, dpi, dpi * 100 / 96);
		if (origin.x != 0 || origin.y != 0 || width != IM_W || height != IM_H || dpi != 96) {
			printf("[ENV][WARN] The drawing area is not exactly %dx%d at (0,0) with 100%% scaling;\n"
			       "             the barrier pattern is computed in window pixels and will be shifted or scaled on the panel.\n",
				IM_W, IM_H);
		}
	}
	else {
		printf("[ENV][WARN] Could not read the window position.\n");
	}
}
