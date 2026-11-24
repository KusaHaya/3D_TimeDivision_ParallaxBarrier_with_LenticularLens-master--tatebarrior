#pragma once
// 静止画（images/IDW/NNNl.ppm, NNNr.ppm）の読み込みと切り替え。
// キーで使う画像は起動時にまとめてGPUへ転送しておき、切り替え時はテクスチャを選ぶだけにする。
#include <string>
#include <vector>

// PPM(P6, 最大値255)を読み込む。失敗したら理由を error に入れて false を返す。
bool ReadPpmFile(const std::string& path, std::vector<unsigned char>& pixels,
                 int& width, int& height, std::string& error);

// キーで使う画像をすべて読み込む（init() から1回呼ぶ）。
void PreloadStillImages();
// 画像番号を表示対象にする（imageL / imageR を切り替える）。
// 読み込めない番号なら警告を出し、今の画像のまま false を返す。
bool SelectStillImage(int number);
// 現在表示している画像番号（まだない場合は0）。
int CurrentStillImage();
// GPU上の静止画テクスチャを解放する。
void ReleaseStillImages();
