#pragma once
// 表示アプリ全体で使う固定値（画面サイズ・初期値・カメラ設置角）。

#define IM_W 1920 // image width
#define IM_H 1080 // image height

#define haba_first 330 // default 500
#define WID 528
#define HGT 297

#define REFRESH_RATE 120

#define SIM_W 1920 // calibration image width
#define SIM_H 1080 // calibration image height

#define SIN 0.139
#define COS 0.99

// ティーポットに貼り付けるテクスチャ。
// テクスチャの幅と高さ。

#define TEAPOT_HEIGHT 512
#define TEAPOT_WIDTH  512

#define SHGT 12 // ステンシルの単位高さ。
#define MIM_W 1920 + 1008  // ステンシルの横幅と余白。

const int MiddleDefault = 3420;
//const int MiddleDefault = 2340;
const float DefaultScale = 1.0f;
