#define _CRT_SECURE_NO_WARNINGS
#define NOMINMAX

#include <cstdlib>
#include <cstring>
#include <stdio.h>
#include <windows.h>
#include <avrt.h>
#include "RenderPriority.h"

#pragma comment(lib, "avrt.lib")

namespace {
HANDLE mmcssTask = nullptr;
bool threadPriorityRaised = false;
}

void RaiseRenderThreadPriority()
{
	const char* mode = std::getenv("PROJECTPB_RENDER_PRIORITY");
	if (mode && _stricmp(mode, "normal") == 0) {
		printf("Render thread priority: normal (PROJECTPB_RENDER_PRIORITY=normal).\n");
		return;
	}

	DWORD taskIndex = 0;
	mmcssTask = AvSetMmThreadCharacteristicsW(L"Games", &taskIndex);
	if (mmcssTask && AvSetMmThreadPriority(mmcssTask, AVRT_PRIORITY_HIGH)) {
		printf("Render thread priority: MMCSS \"Games\", high.\n");
		return;
	}
	if (mmcssTask) {
		AvRevertMmThreadCharacteristics(mmcssTask);
		mmcssTask = nullptr;
	}
	// MMCSS unavailable (service stopped or policy): fall back to a plain boost.
	threadPriorityRaised = SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_HIGHEST) != 0;
	printf("Render thread priority: %s (MMCSS unavailable, error %lu).\n",
		threadPriorityRaised ? "highest" : "normal", GetLastError());
}

void RestoreRenderThreadPriority()
{
	if (mmcssTask) {
		AvRevertMmThreadCharacteristics(mmcssTask);
		mmcssTask = nullptr;
	}
	if (threadPriorityRaised) {
		SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_NORMAL);
		threadPriorityRaised = false;
	}
}
