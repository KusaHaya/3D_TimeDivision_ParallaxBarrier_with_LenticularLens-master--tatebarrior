#define _CRT_SECURE_NO_WARNINGS
#define NOMINMAX

#include <stdio.h>
#include <map>
#include <string>
#include <vector>
#include "AppState.h"
#include "StillImage.h"

namespace {

// 数字キー 0～8 で選ぶ画像番号（KeyInput.cpp の img と一致させる）。
const int KeyImageNumbers[] = { 1, 2, 3, 4, 5, 6, 7, 8, 11 };

struct StillImagePair {
	GLuint left = 0;
	GLuint right = 0;
};
std::map<int, StillImagePair> loadedImages;
int currentImage = 0;

std::string ImagePath(int number, char eye)
{
	char path[64];
	sprintf(path, "./images/IDW/%03d%c.ppm", number, eye);
	return path;
}

// PPMヘッダーの次の数値を読む。途中の空白と # コメントは読み飛ばす。
bool ReadHeaderNumber(FILE* fp, int& value)
{
	int c = fgetc(fp);
	for (;;) {
		while (c == ' ' || c == '\t' || c == '\r' || c == '\n') c = fgetc(fp);
		if (c != '#') break;
		while (c != '\n' && c != EOF) c = fgetc(fp);
	}
	if (c < '0' || c > '9') return false;
	value = 0;
	while (c >= '0' && c <= '9') {
		value = value * 10 + (c - '0');
		if (value > 100000) return false;
		c = fgetc(fp);
	}
	// Exactly one whitespace byte separates the header from the pixel data.
	return c == ' ' || c == '\t' || c == '\r' || c == '\n';
}

GLuint UploadTexture(const std::vector<unsigned char>& pixels)
{
	GLuint texture = 0;
	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_2D, texture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, SIM_W, SIM_H, 0, GL_RGB, GL_UNSIGNED_BYTE, pixels.data());
	glBindTexture(GL_TEXTURE_2D, 0);
	return texture;
}

// 左右の画像を読み込んでGPUへ転送する。失敗したら理由を表示して false。
bool LoadImagePair(int number)
{
	std::vector<unsigned char> leftPixels, rightPixels;
	int width = 0, height = 0;
	std::string error;
	const std::string leftPath = ImagePath(number, 'l');
	const std::string rightPath = ImagePath(number, 'r');
	for (int eye = 0; eye < 2; ++eye) {
		const std::string& path = eye == 0 ? leftPath : rightPath;
		std::vector<unsigned char>& pixels = eye == 0 ? leftPixels : rightPixels;
		if (!ReadPpmFile(path, pixels, width, height, error)) {
			printf("[WARN] Image %d not available: %s\n", number, error.c_str());
			return false;
		}
		if (width != SIM_W || height != SIM_H) {
			printf("[WARN] Image %d not available: %s is %dx%d (expected %dx%d)\n",
				number, path.c_str(), width, height, SIM_W, SIM_H);
			return false;
		}
	}
	StillImagePair pair;
	pair.left = UploadTexture(leftPixels);
	pair.right = UploadTexture(rightPixels);
	loadedImages[number] = pair;
	return true;
}

}

bool ReadPpmFile(const std::string& path, std::vector<unsigned char>& pixels,
                 int& width, int& height, std::string& error)
{
	FILE* fp = fopen(path.c_str(), "rb");
	if (!fp) {
		error = "cannot open " + path;
		return false;
	}
	int maxValue = 0;
	bool ok = fgetc(fp) == 'P' && fgetc(fp) == '6';
	if (!ok) error = path + " is not a binary PPM (P6)";
	if (ok && !(ReadHeaderNumber(fp, width) && ReadHeaderNumber(fp, height) && ReadHeaderNumber(fp, maxValue))) {
		ok = false;
		error = "invalid PPM header in " + path;
	}
	if (ok && (width <= 0 || height <= 0 || maxValue != 255)) {
		ok = false;
		error = path + " must be 8-bit RGB (max value 255)";
	}
	if (ok) {
		pixels.resize((size_t)width * height * 3);
		if (fread(pixels.data(), 1, pixels.size(), fp) != pixels.size()) {
			ok = false;
			error = path + " is shorter than its header says";
		}
	}
	fclose(fp);
	return ok;
}

void PreloadStillImages()
{
	int loaded = 0;
	for (int number : KeyImageNumbers) {
		if (LoadImagePair(number)) ++loaded;
	}
	printf("Still images: %d of %d pairs loaded.\n", loaded, (int)(sizeof(KeyImageNumbers) / sizeof(KeyImageNumbers[0])));
}

bool SelectStillImage(int number)
{
	auto it = loadedImages.find(number);
	if (it == loadedImages.end()) {
		// Not preloaded (e.g. a new key): try once now.
		if (!LoadImagePair(number)) {
			printf("[WARN] Keeping image %d on screen.\n", currentImage);
			return false;
		}
		it = loadedImages.find(number);
	}
	imageL = it->second.left;
	imageR = it->second.right;
	currentImage = number;
	return true;
}

int CurrentStillImage()
{
	return currentImage;
}

void ReleaseStillImages()
{
	for (auto& entry : loadedImages) {
		glDeleteTextures(1, &entry.second.left);
		glDeleteTextures(1, &entry.second.right);
	}
	loadedImages.clear();
	imageL = imageR = 0;
	currentImage = 0;
}
