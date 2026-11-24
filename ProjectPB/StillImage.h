#pragma once
// 静止画（images/IDW/NNNl.ppm, NNNr.ppm）の読み込み。

int ppm_reader(char *filename, unsigned char *pimage);
// 現在の img 番号の左右画像を読み込み、imageL / imageR に転送する。
void LoadStillImageTextures();
