#define _CRT_SECURE_NO_WARNINGS
#define NOMINMAX

#include <cerrno>
#include <cmath>
#include <cstdlib>
#include <fstream>
#include <string>
#include <stdio.h>
#include <windows.h>
#include "AppState.h"
#include "Settings.h"

const char* const SettingsFileName = "ProjectPB_settings.ini";

namespace {

struct Setting {
	const char* key;
	const char* comment;
	int* intValue;
	float* floatValue;
	double* doubleValue;
	double minValue;
	double maxValue;
};

// haba と MiddleLine は追跡中に書き換わるため、保存時は SaveSettings() で基準値に置き換える。
const Setting Settings[] = {
	{"haba", "barrier width in sub-pixels ([ ] a)", &haba, nullptr, nullptr, 2, 100000},
	{"middle_line", "barrier phase origin in sub-pixels", &MiddleLine, nullptr, nullptr, -1000000, 1000000},
	{"shift", "manual phase shift (o O)", &SHIFT, nullptr, nullptr, -100000, 100000},
	{"slant_y", "vertical slant factor (f F)", nullptr, &barrierSlantY, nullptr, 0.1, 100},
	{"threshold_step", "correction threshold = value / 20 (u U)", &barrierThresholdStep, nullptr, nullptr, 1, 20},
	{"horizontal_gain", "horizontal tracking gain (j J)", nullptr, &horizontalTrackingGain, nullptr, 0.1, 10},
	{"camera_pos_x", "camera offset X in metres", nullptr, &PosX, nullptr, -10, 10},
	{"camera_pos_y", "camera offset Y in metres", nullptr, &PosY, nullptr, -10, 10},
	{"camera_pos_z", "camera offset Z in metres", nullptr, &PosZ, nullptr, -10, 10},
	{"camera_tilt_sin", "camera tilt sin", nullptr, nullptr, &cameraTiltSin, -1, 1},
	{"camera_tilt_cos", "camera tilt cos", nullptr, nullptr, &cameraTiltCos, -1, 1},
};

std::string Trim(const std::string& text)
{
	const size_t begin = text.find_first_not_of(" \t\r\n");
	if (begin == std::string::npos) return "";
	const size_t end = text.find_last_not_of(" \t\r\n");
	return text.substr(begin, end - begin + 1);
}

const Setting* FindSetting(const std::string& key)
{
	for (const Setting& setting : Settings) {
		if (key == setting.key) return &setting;
	}
	return nullptr;
}

bool ParseNumber(const std::string& text, double& value)
{
	if (text.empty()) return false;
	char* end = nullptr;
	errno = 0;
	value = std::strtod(text.c_str(), &end);
	return errno == 0 && end && *end == '\0' && std::isfinite(value);
}

}

bool LoadSettings()
{
	std::ifstream file(SettingsFileName);
	if (!file.is_open()) {
		printf("Settings: %s not found; using defaults (press P to save).\n", SettingsFileName);
		return false;
	}
	std::string line;
	int lineNumber = 0;
	int loaded = 0;
	while (std::getline(file, line)) {
		++lineNumber;
		if (lineNumber == 1 && line.size() >= 3 && (unsigned char)line[0] == 0xEF) line = line.substr(3);
		line = Trim(line);
		if (line.empty() || line[0] == '#' || line[0] == ';') continue;
		const size_t equals = line.find('=');
		if (equals == std::string::npos) {
			printf("[WARN] %s:%d: expected key = value\n", SettingsFileName, lineNumber);
			continue;
		}
		const std::string key = Trim(line.substr(0, equals));
		const std::string text = Trim(line.substr(equals + 1));
		const Setting* setting = FindSetting(key);
		double value = 0;
		if (!setting) {
			printf("[WARN] %s:%d: unknown key '%s'\n", SettingsFileName, lineNumber, key.c_str());
			continue;
		}
		if (!ParseNumber(text, value) || value < setting->minValue || value > setting->maxValue) {
			printf("[WARN] %s:%d: %s = '%s' is invalid (allowed %g to %g); keeping the current value\n",
				SettingsFileName, lineNumber, key.c_str(), text.c_str(),
				setting->minValue, setting->maxValue);
			continue;
		}
		if (setting->intValue) *setting->intValue = (int)std::lround(value);
		else if (setting->floatValue) *setting->floatValue = (float)value;
		else *setting->doubleValue = value;
		++loaded;
	}
	// Tracking starts from the loaded alignment.
	trackingBaseHaba = haba;
	trackingBaseMiddleLine = MiddleLine;
	printf("Settings: %d values loaded from %s.\n", loaded, SettingsFileName);
	return true;
}

bool SaveSettings()
{
	// While tracking, haba and MiddleLine follow the head; save the manual base instead.
	const int liveHaba = haba;
	const int liveMiddleLine = MiddleLine;
	if (ht == 1) {
		haba = trackingBaseHaba;
		MiddleLine = trackingBaseMiddleLine;
	}

	const std::string tempName = std::string(SettingsFileName) + ".tmp";
	bool ok = false;
	{
		std::ofstream file(tempName, std::ios::trunc);
		if (file.is_open()) {
			file << "# ProjectPB settings. Saved with the P key; read at startup.\n";
			file << "# Lines are key = value. Delete this file to return to the defaults.\n";
			for (const Setting& setting : Settings) {
				file << "\n# " << setting.comment << "\n" << setting.key << " = ";
				if (setting.intValue) file << *setting.intValue;
				else {
					char text[32];
					if (setting.floatValue) sprintf(text, "%.7g", *setting.floatValue);
					else sprintf(text, "%.15g", *setting.doubleValue);
					file << text;
				}
				file << "\n";
			}
			file.flush();
			ok = file.good();
		}
	}

	haba = liveHaba;
	MiddleLine = liveMiddleLine;

	// Replace the old file only after the new one is complete.
	if (ok) ok = MoveFileExA(tempName.c_str(), SettingsFileName, MOVEFILE_REPLACE_EXISTING) != 0;
	if (!ok) {
		printf("[ERROR] Could not save %s.\n", SettingsFileName);
		DeleteFileA(tempName.c_str());
		return false;
	}
	PrintSettings(ht == 1 ? "saved (tracking base values)" : "saved");
	return true;
}

void PrintSettings(const char* label)
{
	printf("[SETTINGS] %s: haba=%d middle_line=%d shift=%d slant_y=%.3f threshold=%.2f gain=%.2f "
	       "camera_pos=(%.3f, %.3f, %.3f) tilt=(sin %.3f, cos %.3f)\n",
		label, ht == 1 ? trackingBaseHaba : haba, ht == 1 ? trackingBaseMiddleLine : MiddleLine,
		SHIFT, barrierSlantY, barrierThresholdStep / 20.0f, horizontalTrackingGain,
		PosX, PosY, PosZ, cameraTiltSin, cameraTiltCos);
}
